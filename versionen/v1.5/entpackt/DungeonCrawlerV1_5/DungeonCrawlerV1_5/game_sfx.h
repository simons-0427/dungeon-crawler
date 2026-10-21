#ifndef GAME_SFX_H
#define GAME_SFX_H

typedef enum {
    SFX_COIN_PICKUP = 0,
    SFX_ITEM_PICKUP,
    SFX_SPEED,
    SFX_SHIELD_ACTIVATE,
    SFX_SHIELD_TRIGGER,
    SFX_SWORD,
    SFX_GHOSTWALK,
    SFX_BOMB,
    SFX_ENEMY_KILL,
    SFX_DEATH_ENEMY,
    SFX_DEATH_TRAP,
    SFX_TREASURE,
    SFX_PORTAL,
    SFX_COUNT
} SfxId;

// Call after InitGameAudio (audio device must already be open).
void InitSfx(void);
void PlaySfx(SfxId id);
void ShutdownSfx(void);

#endif // GAME_SFX_H
