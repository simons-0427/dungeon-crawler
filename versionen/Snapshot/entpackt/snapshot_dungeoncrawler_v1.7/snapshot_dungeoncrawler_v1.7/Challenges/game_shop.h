#ifndef GAME_SHOP_H
#define GAME_SHOP_H

// Shop persistence (SHOP_FILE): equipped/owned skins, upgrade levels, perks.
// The shop UI itself lives in DrawMenu() (game_ui.c) and writes these globals
// directly, calling SaveShop() after each purchase.
void LoadShop(void);
void SaveShop(void);

#endif // GAME_SHOP_H
