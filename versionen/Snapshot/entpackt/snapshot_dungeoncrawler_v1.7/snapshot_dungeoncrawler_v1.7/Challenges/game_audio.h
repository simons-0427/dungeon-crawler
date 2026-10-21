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
void  RequestGameplayMusicStartDelay(void);
void  ShutdownGameAudio(void);

// Track metadata used by the music-selection screen.
int         GetMusicTrackCount(void);
const char *GetMusicTrackName(int index);
float       GetMusicTrackLength(int index); // seconds; 0 when the audio is unavailable
MusicAccess GetMusicTrackAccess(int index);
int         GetMusicTrackPrice(int index);
const char *GetMusicTrackAccessLabel(int index);
bool        IsMusicTrackUnlocked(int index);
void        SetSoundtrackShopOpen(bool open);
void        SetSoundtrackPreviewTrack(int index); // -1 stops preview; locked tracks may be previewed
int         GetSoundtrackPreviewTrack(void);
float       GetSoundtrackPreviewTimePlayed(int index); // seconds for the active shop preview
void        SeekSoundtrackPreview(int index, float seconds);

// Now-Playing display (queried by DrawHUD in game_ui.c)
const char *GetNowPlayingName(void);   // NULL when not displaying
float       GetNowPlayingAlpha(void);  // 0.0–1.0 fade factor

#endif // GAME_AUDIO_H
