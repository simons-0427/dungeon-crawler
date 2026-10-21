#include "mobile_input.h"

#include <math.h>
#include <stdio.h>

static int failures = 0;

static void ExpectNear(const char *label, float actual, float expected, float tolerance)
{
    if (fabsf(actual - expected) <= tolerance) return;
    fprintf(stderr, "%s: expected %.5f, got %.5f\n", label, expected, actual);
    failures++;
}

static void TestInsideRadius(void)
{
    Vector2 center = { 100.0f, 100.0f };
    Vector2 direction;
    float strength;

    MobileSolveFloatingStick(&center, (Vector2){ 130.0f, 140.0f }, 100.0f,
                             &direction, &strength);

    ExpectNear("inside center x", center.x, 100.0f, 0.001f);
    ExpectNear("inside center y", center.y, 100.0f, 0.001f);
    ExpectNear("inside direction x", direction.x, 0.6f, 0.001f);
    ExpectNear("inside direction y", direction.y, 0.8f, 0.001f);
    if (!(strength > 0.0f && strength < 1.0f)) {
        fprintf(stderr, "inside strength: expected value between zero and one, got %.5f\n", strength);
        failures++;
    }
}

static void TestCenterFollowsExcess(void)
{
    Vector2 center = { 100.0f, 100.0f };
    Vector2 direction;
    float strength;

    MobileSolveFloatingStick(&center, (Vector2){ 300.0f, 100.0f }, 100.0f,
                             &direction, &strength);

    ExpectNear("follow center x", center.x, 200.0f, 0.001f);
    ExpectNear("follow center y", center.y, 100.0f, 0.001f);
    ExpectNear("follow direction x", direction.x, 1.0f, 0.001f);
    ExpectNear("follow direction y", direction.y, 0.0f, 0.001f);
    ExpectNear("follow strength", strength, 1.0f, 0.001f);
}

static void TestDiagonalDirectionSurvivesFollow(void)
{
    Vector2 center = { 10.0f, 20.0f };
    Vector2 finger = { 210.0f, 220.0f };
    Vector2 direction;
    float strength;
    const float rootHalf = 0.70710678f;

    MobileSolveFloatingStick(&center, finger, 80.0f, &direction, &strength);

    ExpectNear("diagonal remaining distance x", finger.x - center.x, 80.0f*rootHalf, 0.002f);
    ExpectNear("diagonal remaining distance y", finger.y - center.y, 80.0f*rootHalf, 0.002f);
    ExpectNear("diagonal direction x", direction.x, rootHalf, 0.002f);
    ExpectNear("diagonal direction y", direction.y, rootHalf, 0.002f);
    ExpectNear("diagonal strength", strength, 1.0f, 0.001f);
}

static void TestDeadzone(void)
{
    Vector2 center = { 50.0f, 50.0f };
    Vector2 direction = { 5.0f, 5.0f };
    float strength = 5.0f;

    MobileSolveFloatingStick(&center, (Vector2){ 56.0f, 50.0f }, 100.0f,
                             &direction, &strength);

    ExpectNear("deadzone direction x", direction.x, 0.0f, 0.001f);
    ExpectNear("deadzone direction y", direction.y, 0.0f, 0.001f);
    ExpectNear("deadzone strength", strength, 0.0f, 0.001f);
}

static void TestCardinalDirections(void)
{
    Vector2 right = MobileCardinalizeDirection((Vector2){ 0.8f, 0.3f });
    Vector2 left = MobileCardinalizeDirection((Vector2){ -0.9f, 0.2f });
    Vector2 down = MobileCardinalizeDirection((Vector2){ 0.2f, 0.7f });
    Vector2 up = MobileCardinalizeDirection((Vector2){ -0.1f, -0.6f });

    ExpectNear("cardinal right x", right.x, 1.0f, 0.001f);
    ExpectNear("cardinal right y", right.y, 0.0f, 0.001f);
    ExpectNear("cardinal left x", left.x, -1.0f, 0.001f);
    ExpectNear("cardinal left y", left.y, 0.0f, 0.001f);
    ExpectNear("cardinal down x", down.x, 0.0f, 0.001f);
    ExpectNear("cardinal down y", down.y, 1.0f, 0.001f);
    ExpectNear("cardinal up x", up.x, 0.0f, 0.001f);
    ExpectNear("cardinal up y", up.y, -1.0f, 0.001f);
}

static void TestFixedDpad(void)
{
    Vector2 direction;
    float strength;

    MobileSolveFixedDpad((Vector2){ 100.0f, 100.0f },
                         (Vector2){ 115.0f, 30.0f }, 100.0f,
                         &direction, &strength);
    ExpectNear("fixed dpad up x", direction.x, 0.0f, 0.001f);
    ExpectNear("fixed dpad up y", direction.y, -1.0f, 0.001f);
    ExpectNear("fixed dpad active strength", strength, 1.0f, 0.001f);

    MobileSolveFixedDpad((Vector2){ 100.0f, 100.0f },
                         (Vector2){ 105.0f, 104.0f }, 100.0f,
                         &direction, &strength);
    ExpectNear("fixed dpad deadzone x", direction.x, 0.0f, 0.001f);
    ExpectNear("fixed dpad deadzone y", direction.y, 0.0f, 0.001f);
    ExpectNear("fixed dpad deadzone strength", strength, 0.0f, 0.001f);
}

static void TestSplitControls(void)
{
    Vector2 direction;
    float strength;

    MobileSolveSplitControl((Vector2){ 100.0f, 100.0f },
                            (Vector2){ 180.0f, 35.0f }, 100.0f, true,
                            &direction, &strength);
    ExpectNear("split vertical ignores x", direction.x, 0.0f, 0.001f);
    ExpectNear("split vertical up", direction.y, -1.0f, 0.001f);
    ExpectNear("split vertical strength", strength, 1.0f, 0.001f);

    MobileSolveSplitControl((Vector2){ 300.0f, 100.0f },
                            (Vector2){ 230.0f, 170.0f }, 100.0f, false,
                            &direction, &strength);
    ExpectNear("split horizontal left", direction.x, -1.0f, 0.001f);
    ExpectNear("split horizontal ignores y", direction.y, 0.0f, 0.001f);
    ExpectNear("split horizontal strength", strength, 1.0f, 0.001f);
}

int main(void)
{
    TestInsideRadius();
    TestCenterFollowsExcess();
    TestDiagonalDirectionSurvivesFollow();
    TestDeadzone();
    TestCardinalDirections();
    TestFixedDpad();
    TestSplitControls();

    if (failures != 0) return 1;
    puts("mobile_input_math_test: all tests passed");
    return 0;
}
