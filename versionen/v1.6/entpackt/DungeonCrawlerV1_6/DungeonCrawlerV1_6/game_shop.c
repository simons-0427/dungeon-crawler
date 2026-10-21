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
        fscanf(f, "%d %d %d %d %d %d %d %d %d %d %d",
               &skin_equipped,
               &spd_level, &shd_level, &wlk_level, &swd_level, &bmb_level, &gld_level,
               &perk_slots, &perk_magnet, &perk_shield2,
               &savedSkinCount);
        if (gld_level < 0) gld_level = 0;
        if (gld_level > 5) gld_level = 5;
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
                if (i < 32) ach_claimed[i][j] = v;
            }

        // Optional block added with the soundtrack shop. Older files end
        // after the achievement block and leave new slots at their defaults.
        int savedMusicCount = 0;
        if (fscanf(f, "%d", &savedMusicCount) == 1) {
            for (int i = 0; i < savedMusicCount; i++) {
                int v = 0;
                fscanf(f, "%d", &v);
                if (i < MUSIC_TRACK_COUNT) music_owned[i] = v ? 1 : 0;
            }
        }
        fclose(f);
    }
    skin_owned[0] = 1;
    // The original four tracks and the new default track are included for
    // every player, including players upgrading from an older save.
    for (int i = 0; i <= MUSIC_DEFAULT_TRACK && i < MUSIC_TRACK_COUNT; i++)
        music_owned[i] = 1;
}

void SaveShop(void) {
    FILE *f = fopen(SHOP_FILE, "w");
    if (f) {
        fprintf(f, "%d %d %d %d %d %d %d %d %d %d %d",
                skin_equipped,
                spd_level, shd_level, wlk_level, swd_level, bmb_level, gld_level,
                perk_slots, perk_magnet, perk_shield2,
                SKIN_COUNT);
        for (int i = 0; i < SKIN_COUNT; i++) fprintf(f, " %d", skin_owned[i]);
        fprintf(f, "\n%d\n", 32);
        for (int i = 0; i < 32; i++) {
            for (int j = 0; j < 6; j++) fprintf(f, "%d ", ach_claimed[i][j]);
            fprintf(f, "\n");
        }
        fprintf(f, "\n%d\n", MUSIC_TRACK_COUNT);
        for (int i = 0; i < MUSIC_TRACK_COUNT; i++)
            fprintf(f, "%d ", music_owned[i] ? 1 : 0);
        fprintf(f, "\n");
        fclose(f);
    }
}
