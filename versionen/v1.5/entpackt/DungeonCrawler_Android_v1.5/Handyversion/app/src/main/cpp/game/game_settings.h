#ifndef GAME_SETTINGS_H
#define GAME_SETTINGS_H

// Canonical defaults used both at first launch and by RESET TO DEFAULT.
#define DEFAULT_VOL_MASTER          1.0f
#define DEFAULT_VOL_MUSIC           0.8f
#define DEFAULT_VOL_SFX             1.0f
#define DEFAULT_AUDIO_MUTE          0
#define DEFAULT_PARTICLE_DENSITY    1
#define DEFAULT_CAMERA_SMOOTHNESS   9.0f
#define DEFAULT_MOBILE_CONTROL      0
#define DEFAULT_SHOW_FPS            0
#define DEFAULT_HUD_OPACITY         1.0f

// User-preference persistence (SETTINGS_FILE).
// These settings are independent of game progress (shop/stats live elsewhere).
void LoadSettings(void);
void SaveSettings(void);
void ResetSettingsToDefaults(void);

#endif // GAME_SETTINGS_H
