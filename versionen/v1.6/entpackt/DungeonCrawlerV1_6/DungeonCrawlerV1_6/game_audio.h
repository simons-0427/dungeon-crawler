#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

#include "types.h"   // GState

typedef enum {
    MUSIC_ACCESS_INCLUDED = 0,
    MUSIC_ACCESS_PURCHASE,
    MUSIC_ACCESS_ACHIEVEMENT
} MusicAccess;

// Lifecycle
void  InitGameAudio(void);
void  UpdateGameAudio(GState state, float dt);
void  ShutdownGameAudio(void);

// Track metadata used by the music-selection screen.
int         GetMusicTrackCount(void);
const char *GetMusicTrackName(int index);
MusicAccess GetMusicTrackAccess(int index);
int         GetMusicTrackPrice(int index);
const char *GetMusicTrackAccessLabel(int index);
bool        IsMusicTrackUnlocked(int index);

// Now-Playing display (queried by DrawHUD in game_ui.c)
const char *GetNowPlayingName(void);   // NULL when not displaying
float       GetNowPlayingAlpha(void);  // 0.0–1.0 fade factor

#endif // GAME_AUDIO_H
