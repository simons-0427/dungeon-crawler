#ifndef MOBILE_PLATFORM_H
#define MOBILE_PLATFORM_H
#include "raylib.h"
void MobilePrepareStorage(void);
bool MobileBackPressed(void);
void MobileRequestSeedInput(const char *seed);
bool MobilePollSeedInput(char *buffer, unsigned capacity);
#endif
