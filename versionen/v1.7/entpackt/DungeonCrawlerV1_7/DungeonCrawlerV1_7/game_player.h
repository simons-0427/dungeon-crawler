#ifndef GAME_PLAYER_H
#define GAME_PLAYER_H

#include "types.h"   // Ene

// Player actions and rendering. Movement timing/input is driven from the main
// loop (main.c), which calls TryMove()/UseItm(); the per-frame effect timers
// (pspd/pwlk/pshield) are also ticked there.
void UseItm(int slot);          // consume inventory item in slot (0-based)
// Gold Touch contact converts (flat coins, no CR x2). No-ops if pgold inactive.
void GoldTouchHitEnemy(Ene *e);
bool GoldTouchHitTrap(int tx, int ty);  // true if a '+' trap was converted
void TryMove(int dx, int dy);   // attempt a one-tile move, resolving collisions
void UpdPlayerHazards(void);    // standing-still hazards (blink trap firing underfoot)
void UpdPlayerFall(float dt);   // trapdoor fall timer → death when done
void DrawPlayer(float t);       // draw the player + active-effect auras + skin

// Current upgrade-tier duration. Shield < 0 means unlimited.
float SpeedEffectMax(void);
float ShieldEffectMax(void);
float GhostwalkEffectMax(void);
float GoldTouchEffectMax(void);

#endif // GAME_PLAYER_H
