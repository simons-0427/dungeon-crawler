#ifndef GAME_AUDIO_H
#define GAME_AUDIO_H

#include "types.h"   // GState

// Lifecycle
void  InitGameAudio(void);
void  UpdateGameAudio(GState state, float dt);
void  ShutdownGameAudio(void);

// Now-Playing display (queried by DrawHUD in game_ui.c)
const char *GetNowPlayingName(void);   // NULL when not displaying
float       GetNowPlayingAlpha(void);  // 0.0–1.0 fade factor

#endif // GAME_AUDIO_H
