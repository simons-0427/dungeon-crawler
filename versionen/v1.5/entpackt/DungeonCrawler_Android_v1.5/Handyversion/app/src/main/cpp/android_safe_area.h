#ifndef ANDROID_SAFE_AREA_H
#define ANDROID_SAFE_AREA_H

#if defined(PLATFORM_ANDROID)
void AndroidGetSafeInsets(int *left, int *top, int *right, int *bottom);
#else
static inline void AndroidGetSafeInsets(int *left, int *top, int *right, int *bottom)
{
    if (left) *left = 0;
    if (top) *top = 0;
    if (right) *right = 0;
    if (bottom) *bottom = 0;
}
#endif

#endif
