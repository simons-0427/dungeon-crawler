// game_utils.c — small shared helpers: star shapes, camera & fullscreen, hit-test.
#include "game_utils.h"
#include "globals.h"     // cam
#include "constants.h"   // SW, SH
#include <math.h>

void DrawCustomStar(float cx, float cy, float outerRadius, float innerRadius, Color color) {
    Vector2 points[11];
    for (int i = 0; i < 10; i++) {
        float angle = -3.14159265f / 2.0f + i * (3.14159265f / 5.0f);
        float r = (i % 2 == 0) ? outerRadius : innerRadius;
        points[i] = (Vector2){ cx + cosf(angle) * r, cy + sinf(angle) * r };
    }
    points[10] = points[0];

    for (int i = 0; i < 10; i++) {
        DrawTriangle((Vector2){cx, cy}, points[i + 1], points[i], color);
    }
}

void DrawCustomStarLines(float cx, float cy, float outerRadius, float innerRadius, Color color) {
    Vector2 points[10];
    for (int i = 0; i < 10; i++) {
        float angle = -3.14159265f / 2.0f + i * (3.14159265f / 5.0f);
        float r = (i % 2 == 0) ? outerRadius : innerRadius;
        points[i] = (Vector2){ cx + cosf(angle) * r, cy + sinf(angle) * r };
    }
    for (int i = 0; i < 10; i++) {
        DrawLineV(points[i], points[(i + 1) % 10], color);
    }
}

void UpdateCameraOffset(void) {
    cam.offset = (Vector2){GetScreenWidth() * 0.5f, GetScreenHeight() * 0.5f};
}

void ToggleFullscreenNow(void) {
    if (!IsWindowFullscreen()) {
        int monitor = GetCurrentMonitor();
        SetWindowSize(GetMonitorWidth(monitor), GetMonitorHeight(monitor));
        ToggleFullscreen();
    } else {
        ToggleFullscreen();
        SetWindowSize(SW, SH);
    }
    UpdateCameraOffset();
}

bool PointInRect(Vector2 p, Rectangle r) {
    return CheckCollisionPointRec(p, r);
}
