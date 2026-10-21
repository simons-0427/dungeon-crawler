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

static float shake_time = 0.0f;
static float shake_dur  = 0.20f;
static float shake_mag  = 0.0f;

void AddScreenShake(float mag, float dur) {
    if (!screen_shake || photosensitivity) return;
    if (mag <= 0.0f) return;
    if (dur < 0.05f) dur = 0.05f;
    float left = (shake_dur > 0.0f && shake_time > 0.0f)
        ? shake_mag * (shake_time / shake_dur) : 0.0f;
    if (mag >= left) {
        shake_mag  = mag;
        shake_dur  = dur;
        shake_time = dur;
    }
}

void UpdateCameraOffset(void) {
    float ox = GetScreenWidth() * 0.5f;
    float oy = GetScreenHeight() * 0.5f;
    if (shake_time > 0.0f) {
        shake_time -= GetFrameTime();
        if (shake_time < 0.0f) shake_time = 0.0f;
        float k = (shake_dur > 0.0f) ? (shake_time / shake_dur) : 0.0f;
        ox += ((float)GetRandomValue(-100, 100) / 100.0f) * shake_mag * k;
        oy += ((float)GetRandomValue(-100, 100) / 100.0f) * shake_mag * k;
    }
    cam.offset = (Vector2){ox, oy};
}

void ToggleFullscreenNow(void);  // defined below; ApplyPersistedWindowState calls it

static int  persist_fs = 0;  // New installs start in a large window; saved user choice overrides this.
static int  persist_x = 0, persist_y = 0;
static int  persist_w = SW, persist_h = SH;
static bool persist_pos_valid = false;

static void ClampWindowSize(int *w, int *h) {
    if (!w || !h) return;
    if (*w < 640) *w = 640;
    if (*h < 360) *h = 360;
    if (*w > 7680) *w = 7680;
    if (*h > 4320) *h = 4320;
}

static bool WindowTouchesAMonitor(int x, int y, int w, int h) {
    int n = GetMonitorCount();
    if (n <= 0) return true;
    Rectangle wr = { (float)x, (float)y, (float)w, (float)h };
    for (int i = 0; i < n; i++) {
        Vector2 mp = GetMonitorPosition(i);
        Rectangle mr = { mp.x, mp.y, (float)GetMonitorWidth(i), (float)GetMonitorHeight(i) };
        if (CheckCollisionRecs(wr, mr)) return true;
    }
    return false;
}

static void CaptureWindowedGeom(void) {
    Vector2 p = GetWindowPosition();
    persist_x = (int)p.x;
    persist_y = (int)p.y;
    persist_w = GetScreenWidth();
    persist_h = GetScreenHeight();
    ClampWindowSize(&persist_w, &persist_h);
    persist_pos_valid = true;
}

void SetPersistedWindowState(int fullscreen, int x, int y, int w, int h) {
    persist_fs = fullscreen ? 1 : 0;
    persist_x = x;
    persist_y = y;
    persist_w = w;
    persist_h = h;
    ClampWindowSize(&persist_w, &persist_h);
    persist_pos_valid = true;
}

void GetPersistedWindowState(int *fullscreen, int *x, int *y, int *w, int *h) {
    if (fullscreen) *fullscreen = persist_fs;
    if (x) *x = persist_x;
    if (y) *y = persist_y;
    if (w) *w = persist_w;
    if (h) *h = persist_h;
}

bool GameWindowIsFullscreen(void) {
    return persist_fs != 0
        || IsWindowFullscreen()
        || IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE);
}

void CaptureWindowState(void) {
    if (!IsWindowReady()) return;
    if (GameWindowIsFullscreen()) {
        persist_fs = 1;
        return;
    }
    persist_fs = 0;
    CaptureWindowedGeom();
}

static void EnterBorderlessFs(void) {
    if (IsWindowFullscreen()) ToggleFullscreen();
    if (!IsWindowState(FLAG_WINDOW_UNDECORATED))
        CaptureWindowedGeom();
    persist_fs = 1;
    int mon = GetCurrentMonitor();
    Vector2 mp = GetMonitorPosition(mon);
    SetWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowPosition((int)mp.x, (int)mp.y);
    SetWindowSize(GetMonitorWidth(mon), GetMonitorHeight(mon));
}

static void LeaveBorderlessFs(void) {
    persist_fs = 0;
    if (IsWindowFullscreen())
        ToggleFullscreen();
    if (IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE))
        ToggleBorderlessWindowed();
    if (IsWindowState(FLAG_WINDOW_UNDECORATED))
        ClearWindowState(FLAG_WINDOW_UNDECORATED);
    SetWindowSize(persist_w, persist_h);
    if (persist_pos_valid)
        SetWindowPosition(persist_x, persist_y);
}

void ApplyPersistedWindowState(void) {
    if (!IsWindowReady()) return;
    SetWindowMinSize(640, 360);

    // First launch: use a near-fullscreen window while keeping the title bar
    // and resize handles available. Once saved, the user's geometry is used.
    if (!persist_pos_valid) {
        int mon = GetCurrentMonitor();
        int monitor_count = GetMonitorCount();
        if (monitor_count <= 0) {
            mon = 0;
        } else if (mon < 0 || mon >= monitor_count) {
            mon = 0;
        }

        int monitor_w = monitor_count > 0 ? GetMonitorWidth(mon) : SW;
        int monitor_h = monitor_count > 0 ? GetMonitorHeight(mon) : SH;
        if (monitor_w <= 0) monitor_w = SW;
        if (monitor_h <= 0) monitor_h = SH;

        Vector2 monitor_pos = monitor_count > 0
            ? GetMonitorPosition(mon)
            : (Vector2){ 0.0f, 0.0f };
        persist_w = (int)(monitor_w * 0.94f);
        persist_h = (int)(monitor_h * 0.90f);
        ClampWindowSize(&persist_w, &persist_h);
        persist_x = (int)monitor_pos.x + (monitor_w - persist_w) / 2;
        persist_y = (int)monitor_pos.y + (monitor_h - persist_h) / 2;
        persist_pos_valid = true;
    }

    SetWindowSize(persist_w, persist_h);
    if (persist_pos_valid) {
        int x = persist_x, y = persist_y;
        if (!WindowTouchesAMonitor(x, y, persist_w, persist_h)) {
            int mon = GetCurrentMonitor();
            Vector2 mp = GetMonitorPosition(mon);
            x = (int)mp.x + (GetMonitorWidth(mon) - persist_w) / 2;
            y = (int)mp.y + (GetMonitorHeight(mon) - persist_h) / 2;
            persist_x = x;
            persist_y = y;
        }
        SetWindowPosition(x, y);
    }
    UpdateCameraOffset();
    if (persist_fs) EnterBorderlessFs();
}

void ToggleFullscreenNow(void) {
    static double lastToggle = -100.0;
    double now = GetTime();
    if (now - lastToggle < 0.25) return;
    lastToggle = now;

    if (GameWindowIsFullscreen()) LeaveBorderlessFs();
    else EnterBorderlessFs();
    UpdateCameraOffset();
}

bool PointInRect(Vector2 p, Rectangle r) {
    return CheckCollisionPointRec(p, r);
}
