// game_settings.c — load/save user preferences (audio, gameplay, UI).
#include "game_settings.h"
#include "globals.h"
#include "constants.h"   // SETTINGS_FILE
#include <stdio.h>

void LoadSettings(void) {
    FILE *f = fopen(SETTINGS_FILE, "r");
    if (!f) return;
    int mut = 0, fps = 0;
    int saved_menu_mask = 0, saved_gameplay_mask = 0;
    int saved_menu_random = 0, saved_gameplay_random = 0, settings_version = 0;
    int readCount = fscanf(f, "%f %f %f %d %d %f %d %f %d %d %d %d %d",
           &vol_master, &vol_music, &vol_sfx, &mut,
           &par_density, &cam_smooth, &fps, &hud_opacity,
           &saved_menu_mask, &saved_gameplay_mask,
           &saved_menu_random, &saved_gameplay_random, &settings_version);
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

    // Version 79 stores only the two track masks. Version 78 briefly stored
    // extra RANDOM ROTATION flags; migrate those modes into track masks.
    if (readCount >= 13 && settings_version == 78) {
        music_menu_tracks = saved_menu_random
                              ? 0x0Fu : (unsigned int)saved_menu_mask;
        music_gameplay_tracks = saved_gameplay_random
                                  ? 0x0Eu : (unsigned int)saved_gameplay_mask;
    } else if (readCount >= 11 && saved_menu_random == 79) {
        music_menu_tracks     = (unsigned int)saved_menu_mask;
        music_gameplay_tracks = (unsigned int)saved_gameplay_mask;
    // Version 77 already used masks, but zero meant OFF for the menu and
    // RANDOM for gameplay. Preserve the old gameplay rotation as a playlist.
    } else if (readCount >= 11 && saved_menu_random == 77) {
        music_menu_tracks     = (unsigned int)saved_menu_mask;
        music_gameplay_tracks = saved_gameplay_mask != 0
                                  ? (unsigned int)saved_gameplay_mask : 0x0Eu;
    } else {
        // Older files stored one menu track and one gameplay track.
        music_menu_tracks = (saved_menu_mask >= 0 && saved_menu_mask <= 3)
                              ? (1u << saved_menu_mask) : 0u;
        music_gameplay_tracks = (saved_gameplay_mask >= 1 && saved_gameplay_mask <= 3)
                                  ? (1u << saved_gameplay_mask) : 0x0Eu;
    }
    unsigned int musicMask = (1u << MUSIC_TRACK_COUNT) - 1u;
    music_menu_tracks     &= musicMask;
    music_gameplay_tracks &= musicMask;
}

void SaveSettings(void) {
    FILE *f = fopen(SETTINGS_FILE, "w");
    if (!f) return;
    fprintf(f, "%g %g %g %d %d %g %d %g %d %d %d\n",
            vol_master, vol_music, vol_sfx, (int)audio_mute,
            par_density, cam_smooth, (int)show_fps, hud_opacity,
            (int)music_menu_tracks, (int)music_gameplay_tracks, 79);
    fclose(f);
}
