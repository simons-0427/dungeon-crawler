#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H

// Player actions and rendering. Movement timing/input is driven from the main
// loop (main.c), which calls TryMove()/UseItm(); the per-frame effect timers
// (pspd/pwlk/pshield) are also ticked there.
void UseItm(int slot);          // consume inventory item in slot (0-based)
void TryMove(int dx, int dy);   // attempt a one-tile move, resolving collisions
void DrawPlayer(float t);       // draw the player + active-effect auras + skin

#endif // GAME_PLAYER_H
