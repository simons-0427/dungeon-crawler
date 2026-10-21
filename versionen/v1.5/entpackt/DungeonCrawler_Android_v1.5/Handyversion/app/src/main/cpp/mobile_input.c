#include "mobile_input.h"
#include "game/constants.h"
#include "android_safe_area.h"

#include <math.h>
#include <stddef.h>

#define MOBILE_MAX_TOUCHES 10
#define STICK_DEADZONE 0.10f
#define MOVEMENT_AREA_RIGHT 0.52f
#define MOVEMENT_AREA_TOP 0.16f

typedef struct TouchSample {
    int id;
    Vector2 position;
} TouchSample;

static TouchSample previous[MOBILE_MAX_TOUCHES];
static int previousCount = 0;
static TouchSample pressed[MOBILE_MAX_TOUCHES];
static int pressedCount = 0;

static int stickId = -1;
static Vector2 stickCenter = { 0.0f, 0.0f };
static Vector2 stickFinger = { 0.0f, 0.0f };
static Vector2 stickDirection = { 0.0f, 0.0f };
static float stickStrength = 0.0f;
static float scrollWheel = 0.0f;
static int controlMode = MOBILE_CONTROL_SPLIT;
static int splitAxis = -1; // 0=horizontal left control, 1=vertical right control

static float StickRadius(void) {
    float radius = (float)GetScreenHeight() * 0.160f;
    if (radius < 96.0f) radius = 96.0f;
    if (radius > 140.0f) radius = 140.0f;
    return radius;
}

static Vector2 RestingStickCenter(void) {
    float radius = StickRadius();
    int safeLeft = 0;
    AndroidGetSafeInsets(&safeLeft, NULL, NULL, NULL);
    Vector2 center = {
        (float)GetScreenWidth()*0.060f,
        (float)GetScreenHeight()*0.700f
    };
    if (center.x < (float)safeLeft + radius + 14.0f) {
        center.x = (float)safeLeft + radius + 14.0f;
    }
    if (center.y > (float)GetScreenHeight() - radius - 18.0f) {
        center.y = (float)GetScreenHeight() - radius - 18.0f;
    }
    return center;
}

static Vector2 SplitRightCenter(void) {
    Vector2 center = RestingStickCenter();
    int safeRight = 0;
    AndroidGetSafeInsets(NULL, NULL, &safeRight, NULL);
    center.x = (float)GetScreenWidth() - (float)safeRight - StickRadius() - 14.0f;
    return center;
}

static int FindSample(const TouchSample *samples, int count, int id) {
    for (int i = 0; i < count; i++) {
        if (samples[i].id == id) return i;
    }
    return -1;
}

static bool InsideMovementArea(Vector2 point) {
    return point.x >= 0.0f && point.x <= (float)GetScreenWidth()*MOVEMENT_AREA_RIGHT &&
           point.y >= (float)GetScreenHeight()*MOVEMENT_AREA_TOP &&
           point.y <= (float)GetScreenHeight();
}

static bool InsideFixedDpad(Vector2 point) {
    Vector2 center = RestingStickCenter();
    float radius = StickRadius();
    float dx = fabsf(point.x - center.x);
    float dy = fabsf(point.y - center.y);
    return (dx <= radius*1.05f && dy <= radius*0.52f) ||
           (dy <= radius*1.05f && dx <= radius*0.52f);
}

static int SplitControlAt(Vector2 point) {
    float radius = StickRadius();
    Vector2 horizontal = RestingStickCenter();
    Vector2 vertical = SplitRightCenter();
    if (fabsf(point.x - horizontal.x) <= radius*1.25f &&
        fabsf(point.y - horizontal.y) <= radius*0.72f) return 0;
    if (fabsf(point.x - vertical.x) <= radius*0.72f &&
        fabsf(point.y - vertical.y) <= radius*1.25f) return 1;
    return -1;
}

static void ResetStick(void) {
    stickId = -1;
    stickCenter = (Vector2){ 0.0f, 0.0f };
    stickFinger = (Vector2){ 0.0f, 0.0f };
    stickDirection = (Vector2){ 0.0f, 0.0f };
    stickStrength = 0.0f;
    splitAxis = -1;
}

void MobileSolveFloatingStick(Vector2 *center, Vector2 finger, float maxRadius,
                              Vector2 *direction, float *strength) {
    if (direction) *direction = (Vector2){ 0.0f, 0.0f };
    if (strength) *strength = 0.0f;
    if (!center || maxRadius <= 0.0f) return;

    float dx = finger.x - center->x;
    float dy = finger.y - center->y;
    float distance = sqrtf(dx*dx + dy*dy);

    // Brawl-Stars-style floating base: only the excess distance moves the
    // center. The finger therefore stays on the rim instead of escaping it.
    if (distance > maxRadius && distance > 0.0001f) {
        float excess = distance - maxRadius;
        center->x += dx/distance*excess;
        center->y += dy/distance*excess;
        dx = finger.x - center->x;
        dy = finger.y - center->y;
        distance = maxRadius;
    }

    if (distance <= maxRadius*STICK_DEADZONE || distance <= 0.0001f) return;

    float radial = distance/maxRadius;
    if (radial > 1.0f) radial = 1.0f;
    float normalizedStrength = (radial - STICK_DEADZONE)/(1.0f - STICK_DEADZONE);
    if (normalizedStrength < 0.0f) normalizedStrength = 0.0f;
    if (normalizedStrength > 1.0f) normalizedStrength = 1.0f;

    if (direction) *direction = (Vector2){ dx/distance, dy/distance };
    if (strength) {
        // Immediate response with a mildly progressive ramp; no frame-to-frame
        // interpolation, so direction changes are visible in the same frame.
        *strength = normalizedStrength*(0.65f + 0.35f*normalizedStrength);
    }
}

Vector2 MobileCardinalizeDirection(Vector2 direction) {
    float absX = fabsf(direction.x);
    float absY = fabsf(direction.y);
    if (absX <= 0.0001f && absY <= 0.0001f) return (Vector2){ 0.0f, 0.0f };

    if (absX >= absY) {
        return (Vector2){ direction.x < 0.0f ? -1.0f : 1.0f, 0.0f };
    }
    return (Vector2){ 0.0f, direction.y < 0.0f ? -1.0f : 1.0f };
}

void MobileSolveFixedDpad(Vector2 center, Vector2 finger, float radius,
                          Vector2 *direction, float *strength) {
    if (direction) *direction = (Vector2){ 0.0f, 0.0f };
    if (strength) *strength = 0.0f;
    float dx = finger.x - center.x;
    float dy = finger.y - center.y;
    float distance = sqrtf(dx*dx + dy*dy);
    if (distance <= radius*STICK_DEADZONE || distance <= 0.0001f) return;
    if (direction) {
        *direction = MobileCardinalizeDirection((Vector2){ dx/distance, dy/distance });
    }
    if (strength) *strength = 1.0f;
}

void MobileSolveSplitControl(Vector2 center, Vector2 finger, float radius,
                             bool vertical, Vector2 *direction, float *strength) {
    if (direction) *direction = (Vector2){ 0.0f, 0.0f };
    if (strength) *strength = 0.0f;
    float delta = vertical ? finger.y - center.y : finger.x - center.x;
    if (fabsf(delta) <= radius*STICK_DEADZONE) return;
    if (direction) {
        *direction = vertical ? (Vector2){ 0.0f, delta < 0.0f ? -1.0f : 1.0f }
                              : (Vector2){ delta < 0.0f ? -1.0f : 1.0f, 0.0f };
    }
    if (strength) *strength = 1.0f;
}

void MobileInputUpdate(bool gameplayActive, int requestedControlMode) {
    TouchSample current[MOBILE_MAX_TOUCHES];
    int currentCount = GetTouchPointCount();
    if (currentCount > MOBILE_MAX_TOUCHES) currentCount = MOBILE_MAX_TOUCHES;

    for (int i = 0; i < currentCount; i++) {
        current[i].id = GetTouchPointId(i);
        current[i].position = GetTouchPosition(i);
    }

    pressedCount = 0;
    scrollWheel = 0.0f;
    for (int i = 0; i < currentCount; i++) {
        int oldIndex = FindSample(previous, previousCount, current[i].id);
        if (oldIndex < 0) {
            pressed[pressedCount++] = current[i];
        } else if (current[i].id != stickId) {
            float deltaY = current[i].position.y - previous[oldIndex].position.y;
            scrollWheel += deltaY / 40.0f;
        }
    }

    if (requestedControlMode < MOBILE_CONTROL_SPLIT ||
        requestedControlMode > MOBILE_CONTROL_DPAD)
        requestedControlMode = MOBILE_CONTROL_SPLIT;
    if (controlMode != requestedControlMode) ResetStick();
    controlMode = requestedControlMode;

    if (!gameplayActive) {
        ResetStick();
    } else {
        int stickIndex = FindSample(current, currentCount, stickId);
        if (stickId >= 0 && stickIndex < 0) {
            ResetStick();
        }

        if (stickId < 0) {
            for (int i = 0; i < pressedCount; i++) {
                int candidateAxis = -1;
                bool capturesStick = false;
                if (controlMode == MOBILE_CONTROL_SPLIT) {
                    candidateAxis = SplitControlAt(pressed[i].position);
                    capturesStick = candidateAxis >= 0;
                } else if (controlMode == MOBILE_CONTROL_DPAD) {
                    capturesStick = InsideFixedDpad(pressed[i].position);
                } else {
                    capturesStick = InsideMovementArea(pressed[i].position);
                }
                if (capturesStick) {
                    stickId = pressed[i].id;
                    splitAxis = candidateAxis;
                    if (controlMode == MOBILE_CONTROL_SPLIT)
                        stickCenter = splitAxis == 0 ? RestingStickCenter() : SplitRightCenter();
                    else if (controlMode == MOBILE_CONTROL_DPAD)
                        stickCenter = RestingStickCenter();
                    else
                        stickCenter = pressed[i].position;
                    stickFinger = pressed[i].position;
                    stickIndex = FindSample(current, currentCount, stickId);
                    break;
                }
            }
        }

        if (stickId >= 0 && stickIndex >= 0) {
            stickFinger = current[stickIndex].position;
            if (controlMode == MOBILE_CONTROL_SPLIT) {
                stickCenter = splitAxis == 0 ? RestingStickCenter() : SplitRightCenter();
                MobileSolveSplitControl(stickCenter, stickFinger, StickRadius(),
                                        splitAxis == 1, &stickDirection, &stickStrength);
            } else if (controlMode == MOBILE_CONTROL_DPAD) {
                stickCenter = RestingStickCenter();
                MobileSolveFixedDpad(stickCenter, stickFinger, StickRadius(),
                                     &stickDirection, &stickStrength);
            } else {
                Vector2 analogDirection = { 0.0f, 0.0f };
                MobileSolveFloatingStick(&stickCenter, stickFinger, StickRadius(),
                                         &analogDirection, &stickStrength);
                stickDirection = MobileCardinalizeDirection(analogDirection);
            }
        }
    }

    if (scrollWheel > 4.0f) scrollWheel = 4.0f;
    if (scrollWheel < -4.0f) scrollWheel = -4.0f;

    previousCount = currentCount;
    for (int i = 0; i < currentCount; i++) previous[i] = current[i];
}

bool MobileGetMoveVector(Vector2 *direction, float *strength) {
    if (direction) *direction = (Vector2){ 0.0f, 0.0f };
    if (strength) *strength = 0.0f;
    if (stickId < 0 || stickStrength <= 0.0f) return false;
    if (direction) *direction = stickDirection;
    if (strength) *strength = stickStrength;
    return true;
}

bool MobileTouchPressed(Rectangle rect) {
    for (int i = 0; i < pressedCount; i++) {
        if (pressed[i].id == stickId) continue;
        if (CheckCollisionPointRec(pressed[i].position, rect)) return true;
    }
    return false;
}

float MobileGetScrollWheel(void) {
    return scrollWheel;
}

static bool SameDirection(Vector2 a, Vector2 b) {
    return a.x == b.x && a.y == b.y;
}

static void DrawDirectionArrow(Vector2 center, float radius, Vector2 direction,
                               bool selected, bool active) {
    Vector2 side = { -direction.y, direction.x };
    Vector2 anchor = {
        center.x + direction.x*radius*0.58f,
        center.y + direction.y*radius*0.58f
    };
    Vector2 tip = {
        anchor.x + direction.x*radius*0.115f,
        anchor.y + direction.y*radius*0.115f
    };
    Vector2 back = {
        anchor.x - direction.x*radius*0.085f,
        anchor.y - direction.y*radius*0.085f
    };
    Vector2 left = {
        back.x + side.x*radius*0.095f,
        back.y + side.y*radius*0.095f
    };
    Vector2 right = {
        back.x - side.x*radius*0.095f,
        back.y - side.y*radius*0.095f
    };
    Color color = selected ? (Color){ 228, 247, 255, 255 }
                           : (Color){ 145, 170, 205, active ? 145 : 100 };
    DrawTriangle(tip, left, right, color);
}

static void DrawFourSectorPad(Vector2 center, float radius, bool active) {
    static const Vector2 directions[4] = {
        { 0.0f, -1.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { -1.0f, 0.0f }
    };
    static const float starts[4] = { -135.0f, -45.0f, 45.0f, 135.0f };

    DrawCircleV(center, radius + 10.0f, (Color){ 5, 8, 18, active ? 92 : 56 });
    DrawCircleV(center, radius, (Color){ 22, 27, 46, active ? 150 : 94 });
    for (int i = 0; i < 4; i++) {
        bool selected = active && stickStrength > 0.0f &&
                        SameDirection(stickDirection, directions[i]);
        Color fill = selected ? (Color){ 76, 190, 236, 128 }
                              : (Color){ 48, 58, 88, active ? 58 : 42 };
        DrawCircleSector(center, radius - 6.0f, starts[i], starts[i] + 90.0f,
                         20, fill);
        DrawDirectionArrow(center, radius, directions[i], selected, active);
    }

    for (int i = 0; i < 4; i++) {
        float angle = (-135.0f + (float)i*90.0f)*DEG2RAD;
        Vector2 edge = {
            center.x + cosf(angle)*(radius - 7.0f),
            center.y + sinf(angle)*(radius - 7.0f)
        };
        DrawLineEx(center, edge, 2.0f,
                   (Color){ 188, 211, 240, active ? 96 : 65 });
    }
    DrawCircleLinesV(center, radius, (Color){ 164, 213, 245, active ? 210 : 138 });
}

static void DrawFixedDpad(Vector2 center, float radius, bool active) {
    static const Vector2 directions[4] = {
        { 0.0f, -1.0f }, { 1.0f, 0.0f }, { 0.0f, 1.0f }, { -1.0f, 0.0f }
    };
    float half = radius*0.33f;
    Rectangle vertical = { center.x-half, center.y-radius, half*2.0f, radius*2.0f };
    Rectangle horizontal = { center.x-radius, center.y-half, radius*2.0f, half*2.0f };
    Color base = (Color){ 22, 27, 46, active ? 220 : 155 };
    DrawRectangleRounded(vertical, 0.22f, 8, base);
    DrawRectangleRounded(horizontal, 0.22f, 8, base);

    if (active && stickStrength > 0.0f) {
        Color selected = (Color){ 70, 173, 220, 190 };
        if (stickDirection.y < 0.0f)
            DrawRectangleRounded((Rectangle){center.x-half,center.y-radius,half*2.0f,radius},0.18f,6,selected);
        else if (stickDirection.y > 0.0f)
            DrawRectangleRounded((Rectangle){center.x-half,center.y,half*2.0f,radius},0.18f,6,selected);
        else if (stickDirection.x < 0.0f)
            DrawRectangleRounded((Rectangle){center.x-radius,center.y-half,radius,half*2.0f},0.18f,6,selected);
        else if (stickDirection.x > 0.0f)
            DrawRectangleRounded((Rectangle){center.x,center.y-half,radius,half*2.0f},0.18f,6,selected);
    }

    DrawRectangleLinesEx(vertical, 2.0f, (Color){ 164, 213, 245, active ? 190 : 125 });
    DrawRectangleLinesEx(horizontal, 2.0f, (Color){ 164, 213, 245, active ? 190 : 125 });
    DrawCircleV(center, half*0.78f, (Color){ 15, 19, 34, active ? 235 : 178 });
    for (int i = 0; i < 4; i++) {
        bool selected = active && stickStrength > 0.0f &&
                        SameDirection(stickDirection, directions[i]);
        DrawDirectionArrow(center, radius, directions[i], selected, active);
    }
}

static void DrawSplitControls(float radius, bool active) {
    Vector2 horizontalCenter = RestingStickCenter();
    Vector2 verticalCenter = SplitRightCenter();
    float half = radius*0.46f;
    Rectangle vertical = {
        verticalCenter.x-half, verticalCenter.y-radius,
        half*2.0f, radius*2.0f
    };
    Rectangle horizontal = {
        horizontalCenter.x-radius, horizontalCenter.y-half,
        radius*2.0f, half*2.0f
    };
    bool horizontalActive = active && splitAxis == 0;
    bool verticalActive = active && splitAxis == 1;
    Color base = (Color){ 22, 27, 46, active ? 210 : 155 };
    Color outline = (Color){ 164, 213, 245, active ? 190 : 125 };

    DrawRectangleRounded(vertical, 0.28f, 8, base);
    DrawRectangleRounded(horizontal, 0.28f, 8, base);

    if (verticalActive && stickStrength > 0.0f) {
        Rectangle selected = stickDirection.y < 0.0f
            ? (Rectangle){vertical.x, vertical.y, vertical.width, radius}
            : (Rectangle){vertical.x, verticalCenter.y, vertical.width, radius};
        DrawRectangleRounded(selected, 0.24f, 7, (Color){70,173,220,190});
    }
    if (horizontalActive && stickStrength > 0.0f) {
        Rectangle selected = stickDirection.x < 0.0f
            ? (Rectangle){horizontal.x, horizontal.y, radius, horizontal.height}
            : (Rectangle){horizontalCenter.x, horizontal.y, radius, horizontal.height};
        DrawRectangleRounded(selected, 0.24f, 7, (Color){70,173,220,190});
    }

    DrawRectangleLinesEx(vertical, 2.0f, outline);
    DrawRectangleLinesEx(horizontal, 2.0f, outline);
    DrawLineEx((Vector2){vertical.x, verticalCenter.y},
               (Vector2){vertical.x + vertical.width, verticalCenter.y}, 2.0f,
               (Color){188,211,240,verticalActive ? 105 : 70});
    DrawLineEx((Vector2){horizontalCenter.x, horizontal.y},
               (Vector2){horizontalCenter.x, horizontal.y + horizontal.height}, 2.0f,
               (Color){188,211,240,horizontalActive ? 105 : 70});

    DrawDirectionArrow(verticalCenter, radius, (Vector2){0.0f,-1.0f},
                       verticalActive && stickDirection.y < 0.0f, verticalActive);
    DrawDirectionArrow(verticalCenter, radius, (Vector2){0.0f,1.0f},
                       verticalActive && stickDirection.y > 0.0f, verticalActive);
    DrawDirectionArrow(horizontalCenter, radius, (Vector2){-1.0f,0.0f},
                       horizontalActive && stickDirection.x < 0.0f, horizontalActive);
    DrawDirectionArrow(horizontalCenter, radius, (Vector2){1.0f,0.0f},
                       horizontalActive && stickDirection.x > 0.0f, horizontalActive);
}

void MobileDrawStick(void) {
    bool active = (stickId >= 0);
    float radius = StickRadius();

    if (controlMode == MOBILE_CONTROL_SPLIT) {
        DrawSplitControls(radius, active);
        return;
    }

    Vector2 center = (active && controlMode == MOBILE_CONTROL_FLOAT)
                   ? stickCenter : RestingStickCenter();
    Vector2 knob = active ? stickFinger : center;

    if (controlMode == MOBILE_CONTROL_DPAD) {
        DrawFixedDpad(center, radius, active);
        return;
    }

    DrawFourSectorPad(center, radius, active);
    DrawCircleLinesV(center, radius*STICK_DEADZONE, (Color){ 190, 205, 235, active ? 72 : 45 });

    if (active && stickStrength > 0.0f) {
        DrawLineEx(center, knob, radius*0.115f,
                   (Color){ 102, 222, 255, (unsigned char)(55.0f + 85.0f*stickStrength) });
    }

    DrawCircleV(knob, radius*0.42f, (Color){ 18, 22, 38, active ? 232 : 138 });
    DrawCircleV(knob, radius*0.33f, (Color){ 72, 102, 155, active ? 205 : 112 });
    DrawCircleLinesV(knob, radius*0.42f, (Color){ 198, 224, 255, active ? 238 : 138 });
    DrawCircleV((Vector2){ knob.x - radius*0.10f, knob.y - radius*0.11f },
                radius*0.085f, (Color){ 240, 248, 255, active ? 105 : 58 });
}
