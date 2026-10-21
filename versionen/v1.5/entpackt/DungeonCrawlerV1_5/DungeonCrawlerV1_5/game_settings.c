// game_settings.c — load/save user preferences (audio stubs, gameplay, UI).
#include "game_settings.h"
#include "globals.h"
#include "constants.h"   // SETTINGS_FILE
#include <stdio.h>

void LoadSettings(void) {
    FILE *f = fopen(SETTINGS_FILE, "r");
    if (!f) return;
    int mut = 0, fps = 0;
    fscanf(f, "%f %f %f %d %d %f %d %f",
           &vol_master, &vol_music, &vol_sfx, &mut,
           &par_density, &cam_smooth, &fps, &hud_opacity);
    fclose(f);
    audio_mute = (bool)mut;
    show_fps   = (bool)fps;
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
    fprintf(f, "%g %g %g %d %d %g %d %g\n",
            vol_master, vol_music, vol_sfx, (int)audio_mute,
            par_density, cam_smooth, (int)show_fps, hud_opacity);
    fclose(f);
}
