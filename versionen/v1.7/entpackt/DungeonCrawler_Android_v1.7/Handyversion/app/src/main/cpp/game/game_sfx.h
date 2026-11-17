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
    SFX_DEATH_ENEMY,
    SFX_DEATH_TRAP,
    SFX_TREASURE,
    SFX_PORTAL,
    SFX_ARROW,          // trap-room arrow volley launching
    SFX_BOSS_SPAWN,     // the Warden arrives
    SFX_BOSS_AIM,       // telegraph shared by the dash and the fire breath
    SFX_BOSS_CHARGE,    // the dash itself
    SFX_BOSS_FIRE,      // the beam itself
    SFX_BOSS_ADDS,      // summoning enemies
    SFX_BOSS_DAMAGE,    // the Warden takes damage
    SFX_BOSS_DEFEAT,    // the Warden dies
    SFX_TITLE_DOOR_OPEN,
    SFX_TITLE_WOOSH,
    SFX_KEY_PICKUP,
    SFX_ARTIFACT_PICKUP,
    SFX_SIDE_ROOM_GATE,
    SFX_LOCKED_OBJECT,
    SFX_COIN_PILE,
    SFX_COUNT
} SfxId;

// Call after InitGameAudio (audio device must already be open).
void InitSfx(void);
void PlaySfx(SfxId id);
void UpdateSfx(float dt);
void ShutdownSfx(void);

#endif // GAME_SFX_H
