#ifndef GAME_CONSOLE_WIN32_H
#define GAME_CONSOLE_WIN32_H

#include <stdbool.h>
#include <stddef.h>

// The viewer runs in a separate process so closing its window cannot close the game.
bool GameConsoleRunViewerIfRequested(int argc, char **argv);
void GameConsoleInit(void);
bool GameConsoleSetEnabled(bool enabled);
bool GameConsoleUpdate(void); // true if the viewer was closed by the user
void GameConsoleShutdown(void);
void GameConsoleSetLogDetail(int detail); // 0 errors, 1 warnings, 2 info, 3 debug
bool GameConsoleOpenLogFile(void);
bool GameConsoleOpenUrl(const char *url);
void GameConsoleGetSystemInfo(char *out, size_t capacity);

#endif
