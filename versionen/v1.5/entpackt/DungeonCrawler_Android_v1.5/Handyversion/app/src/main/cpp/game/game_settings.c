// game_settings.c — load/save user preferences (audio stubs, gameplay, UI).
#include "game_settings.h"
#include "globals.h"
#include "constants.h"   // SETTINGS_FILE
#include "web_persist.h"
#include <stdio.h>

void LoadSettings(void) {
    WebPersistLoad(SETTINGS_FILE);
    FILE *f = fopen(SETTINGS_FILE, "r");
    if (!f) return;
    int mut = 0, fps = 0, storedControl = MOBILE_CONTROL_SPLIT, settingsVersion = 0;
    int fields = fscanf(f, "%f %f %f %d %d %f %d %f %d %d",
           &vol_master, &vol_music, &vol_sfx, &mut,
           &par_density, &cam_smooth, &fps, &hud_opacity,
           &storedControl, &settingsVersion);
    fclose(f);
    audio_mute = (bool)mut;
    show_fps   = (bool)fps;
    // Legacy files did not know the split controller. Upgrade them to the new
    // default; files written by this version retain the user's three-way choice.
    if (fields >= 10 && settingsVersion >= 2) {
        mobile_control_mode = storedControl;
        if (mobile_control_mode < MOBILE_CONTROL_SPLIT ||
            mobile_control_mode > MOBILE_CONTROL_DPAD)
            mobile_control_mode = MOBILE_CONTROL_SPLIT;
    } else {
        mobile_control_mode = MOBILE_CONTROL_SPLIT;
    }
    // Clamp all values to valid ranges.
    if (vol_master  < 0.0f)  vol_master  = 0.0f;
    if (vol_master  > 1.0f)  vol_master  = 1.0f;
    if (vol_music   < 0.0f)  vol_music   = 0.0f;
    if (vol_music   > 1.0f)  vol_music   = 1.0f;
    if (vol_sfx     < 0.0f)  vol_sfx     = 0.0f;
    if (vol_sfx     > 1.0f)  vol_sfx     = 1.0f;
    if (par_density < 0)     par_density = 0;
    if (par_density > 2)     par_density = 2;
    if (cam_smooth  < 1.0f)  cam_smooth  = 1.0f;
    if (cam_smooth  > 20.0f) cam_smooth  = 20.0f;
    if (hud_opacity < 0.2f)  hud_opacity = 0.2f;
    if (hud_opacity > 1.0f)  hud_opacity = 1.0f;
}

void SaveSettings(void) {
    FILE *f = fopen(SETTINGS_FILE, "w");
    if (!f) return;
    fprintf(f, "%g %g %g %d %d %g %d %g %d %d\n",
            vol_master, vol_music, vol_sfx, (int)audio_mute,
            par_density, cam_smooth, (int)show_fps, hud_opacity,
            mobile_control_mode, 2);
    fclose(f);
    WebPersistSave(SETTINGS_FILE);
}

void ResetSettingsToDefaults(void) {
    vol_master = DEFAULT_VOL_MASTER;
    vol_music = DEFAULT_VOL_MUSIC;
    vol_sfx = DEFAULT_VOL_SFX;
    audio_mute = (bool)DEFAULT_AUDIO_MUTE;
    par_density = DEFAULT_PARTICLE_DENSITY;
    cam_smooth = DEFAULT_CAMERA_SMOOTHNESS;
    mobile_control_mode = DEFAULT_MOBILE_CONTROL;
    show_fps = (bool)DEFAULT_SHOW_FPS;
    hud_opacity = DEFAULT_HUD_OPACITY;
    eneon = true;
    SaveSettings();
}
