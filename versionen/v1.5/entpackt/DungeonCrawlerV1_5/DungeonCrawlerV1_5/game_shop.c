// game_shop.c — shop state persistence (skins, upgrade levels, perks).
#include "game_shop.h"
#include "globals.h"
#include "constants.h"   // SHOP_FILE, SKIN_COUNT
#include <stdio.h>

// Save format is self-describing for skin_owned[] and ach_claimed[][]: each
// block is prefixed with the element count it was written with. This lets
// LoadShop stay correct even after SKIN_COUNT or the achievement count grow
// again later — a save from an older/smaller build fills only as many slots
// as it has data for (rest stay at their zero-initialized default) instead of
// reading past the end of one block and corrupting the next.
void LoadShop(void) {
    FILE *f = fopen(SHOP_FILE, "r");
    if (f) {
        int savedSkinCount = 0;
        fscanf(f, "%d %d %d %d %d %d %d %d %d %d",
               &skin_equipped,
               &spd_level, &shd_level, &wlk_level, &swd_level, &bmb_level,
               &perk_slots, &perk_magnet, &perk_shield2,
               &savedSkinCount);
        for (int i = 0; i < savedSkinCount; i++) {
            int v = 0;
            fscanf(f, "%d", &v);
            if (i < SKIN_COUNT) skin_owned[i] = v;
        }

        int savedAchRows = 0;
        fscanf(f, "%d", &savedAchRows);
        for (int i = 0; i < savedAchRows; i++)
            for (int j = 0; j < 6; j++) {
                int v = 0;
                fscanf(f, "%d", &v);
                if (i < 23) ach_claimed[i][j] = v;
            }
        fclose(f);
    }
    skin_owned[0] = 1;
}

void SaveShop(void) {
    FILE *f = fopen(SHOP_FILE, "w");
    if (f) {
        fprintf(f, "%d %d %d %d %d %d %d %d %d %d",
                skin_equipped,
                spd_level, shd_level, wlk_level, swd_level, bmb_level,
                perk_slots, perk_magnet, perk_shield2,
                SKIN_COUNT);
        for (int i = 0; i < SKIN_COUNT; i++) fprintf(f, " %d", skin_owned[i]);
        fprintf(f, "\n%d\n", 23);
        for (int i = 0; i < 23; i++) {
            for (int j = 0; j < 6; j++) fprintf(f, "%d ", ach_claimed[i][j]);
            fprintf(f, "\n");
        }
        fclose(f);
    }
}
