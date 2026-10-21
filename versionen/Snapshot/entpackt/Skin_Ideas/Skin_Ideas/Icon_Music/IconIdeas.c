// IconIdeas.c — pure 2D music-note icons for the main-menu button row.
// Matches Settings gear + Future rocket: flat solid shapes, violet/lavender
// palette, no full-bleed background art. Transparent PNG on export.
//
// Keep Beamed Black + new menu-matching variants (with live animation in picker).
//
// Build: IconIdeas-bauen.bat
// Output: flat_selected_icon.png + flat_icon_selection.json

#include "raylib.h"

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#define CONCEPT_COUNT 10
#define EXPORT_SIZE 1024
#define PATH_BUFFER_SIZE 2048

#define OUT_PNG  "flat_selected_icon.png"
#define OUT_JSON "flat_icon_selection.json"

// Palette aligned with game_ui.c main-menu icons:
//   Settings gear: ic (160,148,210) / hover (230,220,255)
//   Rocket:        ic (160,95,220)  / hover (225,175,255)
static const Color COL_GEAR      = { 160, 148, 210, 255 };
static const Color COL_GEAR_HI   = { 230, 220, 255, 255 };
static const Color COL_ROCKET    = { 160,  95, 220, 255 };
static const Color COL_ROCKET_HI = { 225, 175, 255, 255 };
static const Color COL_BLACK     = {  18,  16,  24, 255 };
static const Color COL_SHEEN     = { 245, 240, 255, 210 };

typedef struct IconConcept {
    const char *key;
    const char *name;
    const char *description;
} IconConcept;

static const IconConcept CONCEPTS[CONCEPT_COUNT] = {
    { "beamed_black",       "Beamed Black",      "Behalten: dicke Doppel-Achtel, schwarz" },
    { "beamed_gear",        "Beamed Gear",       "Settings-Lavendel, sanftes Wippen" },
    { "beamed_rocket",      "Beamed Rocket",     "Raketen-Violett, Puls" },
    { "beamed_gear_hover",  "Beamed Gear Hover", "Settings-Hover-Weisslila + Bounce" },
    { "beamed_rocket_hover","Beamed Rocket Hover","Raketen-Hover-Pinkviolett + Bounce" },
    { "quaver_gear",        "Quaver Gear",       "Einzelnote, Settings-Farben, Flag-Wippe" },
    { "quaver_rocket",      "Quaver Rocket",     "Einzelnote, Raketen-Farben, Flag-Wippe" },
    { "beamed_eq",          "Beamed EQ",         "Doppel-Achtel + animierte EQ-Balken" },
    { "beamed_orbit",       "Beamed Orbit",      "Doppel-Achtel + umlaufende Punkte" },
    { "beamed_blend",       "Beamed Blend",      "Wechselt Gear <-> Rocket Farbe" },
};

static float MinF(float a, float b) { return (a < b) ? a : b; }

static Color LerpColor(Color a, Color b, float t)
{
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;
    return (Color){
        (unsigned char)(a.r + (b.r - a.r) * t),
        (unsigned char)(a.g + (b.g - a.g) * t),
        (unsigned char)(a.b + (b.b - a.b) * t),
        (unsigned char)(a.a + (b.a - a.a) * t)
    };
}

static Color Lighten(Color c, float amount)
{
    return (Color){
        (unsigned char)fminf(255.0f, c.r + (255 - c.r) * amount),
        (unsigned char)fminf(255.0f, c.g + (255 - c.g) * amount),
        (unsigned char)fminf(255.0f, c.b + (255 - c.b) * amount),
        c.a
    };
}

static void DrawCheckerboard(Rectangle r, float cell)
{
    int cols = (int)ceilf(r.width / cell);
    int rows = (int)ceilf(r.height / cell);
    for (int y = 0; y < rows; y++) {
        for (int x = 0; x < cols; x++) {
            bool dark = ((x + y) & 1) != 0;
            Color c = dark ? (Color){ 42, 40, 55, 255 } : (Color){ 58, 55, 72, 255 };
            float px = r.x + (float)x * cell;
            float py = r.y + (float)y * cell;
            float pw = cell, ph = cell;
            if (px + pw > r.x + r.width)  pw = r.x + r.width - px;
            if (py + ph > r.y + r.height) ph = r.y + r.height - py;
            DrawRectangle((int)px, (int)py, (int)pw, (int)ph, c);
        }
    }
}

static void EllipseRot(float cx, float cy, float rx, float ry, float ang, Color col)
{
    const int SEG = 28;
    Vector2 pts[SEG + 1];
    float ca = cosf(ang), sa = sinf(ang);
    for (int i = 0; i <= SEG; i++) {
        float th = (float)i / (float)SEG * 2.0f * PI;
        float ex = cosf(th) * rx, ey = sinf(th) * ry;
        pts[i] = (Vector2){ cx + ex * ca - ey * sa, cy + ex * sa + ey * ca };
    }
    for (int i = 0; i < SEG; i++)
        DrawTriangle((Vector2){ cx, cy }, pts[i + 1], pts[i], col);
}

// ── Chunky 2D music glyphs (menu-style: solid, thick, no backdrop) ───────────

// Beamed pair. bob = vertical offset of whole glyph; sway = slight lean (rad).
// headBobL/R = per-head vertical offset for dual bounce.
static void DrawBeamedNote(float cx, float cy, float s, Color fill,
                           float bob, float sway, float headBobL, float headBobR)
{
    float gap = s * 1.05f;
    float hxL = cx - gap * 0.50f + sinf(sway) * s * 0.04f;
    float hxR = cx + gap * 0.50f + sinf(sway) * s * 0.04f;
    float hyL = cy + bob + s * 0.32f + headBobL;
    float hyR = cy + bob + s * 0.32f + headBobR;
    const float headAng = -0.48f + sway * 0.4f;

    for (int side = 0; side < 2; side++) {
        float hx = (side == 0) ? hxL : hxR;
        float hy = (side == 0) ? hyL : hyR;
        EllipseRot(hx, hy, s * 0.46f, s * 0.34f, headAng, fill);
        EllipseRot(hx - s * 0.07f, hy - s * 0.04f, s * 0.15f, s * 0.09f, headAng,
                   Lighten(fill, 0.28f));
    }

    // Stems connect to each head, beams use average top
    float stemL = hxL + s * 0.30f;
    float stemR = hxR + s * 0.30f;
    float stemW = s * 0.18f;
    float stemTopL = hyL - s * 1.22f;
    float stemTopR = hyR - s * 1.22f;
    DrawLineEx((Vector2){ stemL, hyL }, (Vector2){ stemL, stemTopL }, stemW, fill);
    DrawLineEx((Vector2){ stemR, hyR }, (Vector2){ stemR, stemTopR }, stemW, fill);

    // Double beam follows the two stem tops (slight slope if heads bounce differently)
    float beamThick0 = s * 0.22f;
    float beamThick1 = s * 0.16f;
    DrawLineEx((Vector2){ stemL - s * 0.02f, stemTopL + s * 0.02f },
               (Vector2){ stemR + s * 0.02f, stemTopR + s * 0.02f }, beamThick0, fill);
    DrawLineEx((Vector2){ stemL - s * 0.02f, stemTopL + s * 0.34f },
               (Vector2){ stemR + s * 0.02f, stemTopR + s * 0.34f }, beamThick1, fill);
}

// Single quaver. flagSway tilts the flag ribbon.
static void DrawQuaverNote(float cx, float cy, float s, Color fill,
                           float bob, float sway, float flagSway)
{
    float hx = cx - s * 0.04f + sinf(sway) * s * 0.05f;
    float hy = cy + bob + s * 0.28f;
    const float headAng = -0.48f + sway * 0.35f;

    EllipseRot(hx, hy, s * 0.52f, s * 0.38f, headAng, fill);
    EllipseRot(hx - s * 0.08f, hy - s * 0.05f, s * 0.18f, s * 0.10f, headAng,
               Lighten(fill, 0.30f));

    float stemX = hx + s * 0.36f;
    float stemBot = hy;
    float stemTop = hy - s * 1.38f;
    float stemW = s * 0.20f;
    DrawLineEx((Vector2){ stemX, stemBot }, (Vector2){ stemX, stemTop }, stemW, fill);

    // Flag wags with flagSway
    float fs = flagSway;
    Vector2 top = { stemX, stemTop };
    Vector2 a = { stemX + s * (0.70f + 0.08f * fs), stemTop + s * 0.10f };
    Vector2 b = { stemX + s * (0.82f + 0.10f * fs), stemTop + s * 0.42f };
    Vector2 c = { stemX + s * (0.48f + 0.06f * fs), stemTop + s * 0.58f };
    Vector2 d = { stemX + s * (0.12f + 0.04f * fs), stemTop + s * 0.82f };
    Vector2 e = { stemX - s * 0.02f, stemTop + s * 0.42f };
    DrawTriangle(top, a, e, fill);
    DrawTriangle(a, b, c, fill);
    DrawTriangle(e, c, d, fill);
    DrawTriangle(
        (Vector2){ stemX - s * 0.04f, stemTop + s * 0.02f },
        (Vector2){ stemX + s * (0.62f + 0.08f * fs), stemTop + s * 0.18f },
        (Vector2){ stemX + s * 0.10f, stemTop + s * 0.55f }, fill);
}

// Tiny EQ bars under the note (menu-adjacent musical cue).
static void DrawMiniEQ(float cx, float cy, float s, Color col, float t)
{
    const int n = 5;
    float barW = s * 0.14f;
    float gap = s * 0.06f;
    float total = (float)n * barW + (float)(n - 1) * gap;
    float x0 = cx - total * 0.5f;
    for (int i = 0; i < n; i++) {
        float wave = 0.35f + 0.65f * (0.5f + 0.5f * sinf(t * 3.2f + i * 0.9f));
        float h = s * 0.55f * wave;
        float x = x0 + (float)i * (barW + gap);
        DrawRectangleV((Vector2){ x, cy - h }, (Vector2){ barW, h }, col);
    }
}

// Orbiting dots (same language as soft UI accents, not a full glow disc).
static void DrawOrbitDots(float cx, float cy, float s, Color col, float t)
{
    for (int k = 0; k < 5; k++) {
        float a = t * 1.4f + k * (2.0f * PI / 5.0f);
        float d = s * 1.15f;
        DrawCircleV((Vector2){ cx + cosf(a) * d, cy + sinf(a) * d * 0.72f },
                    s * 0.07f, ColorAlpha(col, 0.75f));
    }
}

// ── Concept art (t = animation time; export uses t=0 for a clean frame) ──────

static void DrawIconArt(int concept, Rectangle bounds, float t)
{
    float cx = bounds.x + bounds.width * 0.50f;
    float cy = bounds.y + bounds.height * 0.50f;
    float s  = bounds.width * 0.34f;

    float bob = sinf(t * 2.0f) * s * 0.04f;
    float sway = sinf(t * 1.3f) * 0.08f;
    float headL = sinf(t * 2.6f) * s * 0.05f;
    float headR = sinf(t * 2.6f + PI) * s * 0.05f;
    float flag = sinf(t * 3.0f);

    switch (concept) {
        case 0: // Beamed Black — static (kept), still chunky
            DrawBeamedNote(cx, cy, s, COL_BLACK, 0.0f, 0.0f, 0.0f, 0.0f);
            break;

        case 1: // Beamed Gear — settings lavender + gentle rock
            DrawBeamedNote(cx, cy, s, COL_GEAR, bob * 0.6f, sway, 0.0f, 0.0f);
            break;

        case 2: // Beamed Rocket — rocket violet + scale pulse
        {
            float pulse = 1.0f + 0.04f * sinf(t * 2.8f);
            DrawBeamedNote(cx, cy, s * pulse, COL_ROCKET, bob * 0.5f, sway * 0.7f, 0.0f, 0.0f);
            break;
        }

        case 3: // Beamed Gear Hover — bright lavender + dual head bounce
            DrawBeamedNote(cx, cy, s, COL_GEAR_HI, bob, sway, headL, headR);
            break;

        case 4: // Beamed Rocket Hover
            DrawBeamedNote(cx, cy, s, COL_ROCKET_HI, bob, sway, headL, headR);
            break;

        case 5: // Quaver Gear
            DrawQuaverNote(cx, cy, s, COL_GEAR, bob * 0.7f, sway, flag);
            break;

        case 6: // Quaver Rocket
            DrawQuaverNote(cx, cy, s, COL_ROCKET, bob * 0.7f, sway, flag);
            break;

        case 7: // Beamed + EQ bars
            DrawBeamedNote(cx, cy - s * 0.18f, s * 0.88f, COL_GEAR, bob * 0.4f, sway * 0.5f, 0.0f, 0.0f);
            DrawMiniEQ(cx, cy + s * 0.95f, s * 0.55f, COL_ROCKET, t);
            break;

        case 8: // Beamed + orbit dots
            DrawOrbitDots(cx, cy, s, COL_ROCKET_HI, t);
            DrawBeamedNote(cx, cy, s, COL_ROCKET, bob * 0.5f, sway, 0.0f, 0.0f);
            break;

        case 9: // Blend gear <-> rocket color over time
        {
            float u = 0.5f + 0.5f * sinf(t * 1.1f);
            Color col = LerpColor(COL_GEAR, COL_ROCKET, u);
            DrawBeamedNote(cx, cy, s, col, bob * 0.6f, sway, headL * 0.5f, headR * 0.5f);
            break;
        }

        default:
            break;
    }

    (void)COL_SHEEN;
}

// ── UI ───────────────────────────────────────────────────────────────────────

static void DrawCenteredText(const char *text, float centerX, float y, int fontSize, Color color)
{
    int width = MeasureText(text, fontSize);
    DrawText(text, (int)(centerX - (float)width * 0.5f), (int)y, fontSize, color);
}

static Rectangle GetCardRectangle(int index, int screenWidth, int screenHeight)
{
    const int columns = 5;
    const int rows = (CONCEPT_COUNT + columns - 1) / columns;
    const float margin = 28.0f;
    const float gap = 14.0f;
    const float top = 108.0f;
    const float bottom = 128.0f;
    float width = ((float)screenWidth - margin * 2.0f - gap * (float)(columns - 1)) / (float)columns;
    float height = ((float)screenHeight - top - bottom - gap * (float)(rows - 1)) / (float)rows;
    int column = index % columns;
    int row = index / columns;
    return (Rectangle){
        margin + (float)column * (width + gap),
        top + (float)row * (height + gap),
        width,
        height
    };
}

static Rectangle GetSaveButtonRectangle(int screenWidth, int screenHeight)
{
    return (Rectangle){ (float)screenWidth - 330.0f, (float)screenHeight - 91.0f, 302.0f, 58.0f };
}

static void DrawConceptCard(int index, Rectangle card, bool hovered, bool selected, bool saved, float t)
{
    Color fill = hovered ? (Color){ 34, 37, 58, 255 } : (Color){ 25, 28, 45, 255 };
    Color outline = selected ? (Color){ 89, 255, 150, 255 } : (Color){ 74, 79, 108, 255 };
    float previewSize = MinF(card.width - 22.0f, card.height - 80.0f);
    Rectangle preview = {
        card.x + (card.width - previewSize) * 0.5f,
        card.y + 10.0f,
        previewSize,
        previewSize
    };

    DrawRectangleRounded(card, 0.055f, 14, fill);
    DrawRectangleRoundedLinesEx(card, 0.055f, 14, selected ? 4.0f : 2.0f, outline);

    // Menu-button-like dark plate behind glyph (like settings/rocket bgc) —
    // still "no full art background", just a plate so colors read like the main menu.
    DrawCheckerboard(preview, previewSize / 8.0f);
    // Soft menu plate (matches icon button fill, optional for readability)
    Color plate = (index == 0)
        ? (Color){ 16, 12, 28, 200 }
        : (index == 2 || index == 4 || index == 6 || index == 8)
            ? (Color){ 20, 9, 32, 210 }
            : (Color){ 16, 12, 28, 210 };
    DrawRectangleRounded(preview, 0.14f, 8, plate);
    DrawRectangleRoundedLinesEx(preview, 0.14f, 8, 1.5f,
        (index == 2 || index == 4 || index == 6 || index == 8)
            ? (Color){ 110, 55, 165, 150 }
            : (Color){ 95, 85, 140, 150 });

    DrawIconArt(index, preview, t);

    DrawCircleV((Vector2){ card.x + 24.0f, card.y + 24.0f }, 16.0f,
                selected ? (Color){ 89, 255, 150, 255 } : (Color){ 13, 15, 26, 230 });
    DrawCenteredText(TextFormat("%d", index + 1), card.x + 24.0f, card.y + 15.0f, 18,
                     selected ? (Color){ 10, 31, 22, 255 } : RAYWHITE);

    if (saved) {
        Rectangle badge = { card.x + card.width - 108.0f, card.y + 10.0f, 96.0f, 26.0f };
        DrawRectangleRounded(badge, 0.35f, 10, (Color){ 89, 255, 150, 235 });
        DrawCenteredText("GESPEICHERT", badge.x + badge.width * 0.5f, badge.y + 6.0f, 12,
                         (Color){ 10, 31, 22, 255 });
    }

    float labelY = card.y + card.height - 56.0f;
    DrawCenteredText(CONCEPTS[index].name, card.x + card.width * 0.5f, labelY, 16, RAYWHITE);
    DrawCenteredText(CONCEPTS[index].description, card.x + card.width * 0.5f, labelY + 24.0f, 12,
                     (Color){ 174, 180, 202, 255 });
}

static void DrawSelectorUi(int screenWidth, int screenHeight, int selected, int savedSelection,
                           int hovered, const char *status, float t)
{
    DrawRectangleGradientEx((Rectangle){ 0, 0, (float)screenWidth, (float)screenHeight },
                            (Color){ 10, 12, 22, 255 }, (Color){ 18, 16, 31, 255 },
                            (Color){ 27, 15, 38, 255 }, (Color){ 13, 19, 31, 255 });

    DrawText("ICON-IDEAS  —  Menue-Musiknote (Settings/Rakete-Stil)", 28, 18, 26, RAYWHITE);
    DrawText("Farben 1:1 aus game_ui: Gear-Lavendel & Raketen-Violett. Animation live. Export ohne BG-Art.",
             30, 58, 16, (Color){ 178, 184, 207, 255 });
    DrawText(TextFormat("Gear  (%d,%d,%d)   Rocket  (%d,%d,%d)",
                        COL_GEAR.r, COL_GEAR.g, COL_GEAR.b,
                        COL_ROCKET.r, COL_ROCKET.g, COL_ROCKET.b),
             30, 80, 14, (Color){ 160, 148, 210, 220 });

    for (int i = 0; i < CONCEPT_COUNT; i++) {
        DrawConceptCard(i, GetCardRectangle(i, screenWidth, screenHeight), i == hovered,
                        i == selected, i == savedSelection, t);
    }

    Rectangle footer = { 0, (float)screenHeight - 115.0f, (float)screenWidth, 115.0f };
    DrawRectangleRec(footer, (Color){ 11, 13, 23, 247 });
    DrawLine(0, (int)footer.y, screenWidth, (int)footer.y, (Color){ 77, 82, 111, 255 });

    if (selected >= 0) {
        DrawText(TextFormat("Markiert: %d - %s", selected + 1, CONCEPTS[selected].name), 30,
                 screenHeight - 92, 22, (Color){ 89, 255, 150, 255 });
    } else {
        DrawText("Noch kein Icon markiert.", 30, screenHeight - 92, 22,
                 (Color){ 255, 205, 91, 255 });
    }
    DrawText(status, 30, screenHeight - 55, 16, (Color){ 172, 178, 201, 255 });
    DrawText("Tasten 1-9, 0  |  ENTER speichert  |  ESC schliesst", 30, screenHeight - 29, 14,
             (Color){ 116, 123, 150, 255 });

    Rectangle saveButton = GetSaveButtonRectangle(screenWidth, screenHeight);
    bool saveHover = CheckCollisionPointRec(GetMousePosition(), saveButton);
    Color buttonColor = (selected < 0) ? (Color){ 60, 63, 76, 255 }
                                       : (saveHover ? (Color){ 112, 255, 166, 255 } : (Color){ 75, 225, 133, 255 });
    DrawRectangleRounded(saveButton, 0.16f, 12, buttonColor);
    DrawRectangleRoundedLinesEx(saveButton, 0.16f, 12, 2.0f,
                                (selected < 0) ? (Color){ 92, 96, 111, 255 } : (Color){ 181, 255, 210, 255 });
    DrawCenteredText("AUSWAHL SPEICHERN", saveButton.x + saveButton.width * 0.5f, saveButton.y + 17.0f, 21,
                     (selected < 0) ? (Color){ 145, 148, 158, 255 } : (Color){ 8, 32, 20, 255 });
}

static void BuildApplicationPath(char *output, size_t outputSize, const char *filename)
{
    const char *directory = GetApplicationDirectory();
    size_t length = strlen(directory);
    bool hasSeparator = (length > 0 && (directory[length - 1] == '/' || directory[length - 1] == '\\'));
    snprintf(output, outputSize, "%s%s%s", directory, hasSeparator ? "" : "\\", filename);
}

static bool ExportSelectedIcon(int selected)
{
    char pngPath[PATH_BUFFER_SIZE];
    BuildApplicationPath(pngPath, sizeof(pngPath), OUT_PNG);

    RenderTexture2D render = LoadRenderTexture(EXPORT_SIZE, EXPORT_SIZE);
    if (render.id == 0) return false;

    BeginTextureMode(render);
        ClearBackground(BLANK); // transparent — glyph only
        // Neutral rest pose for export (t=0)
        DrawIconArt(selected, (Rectangle){ 0, 0, EXPORT_SIZE, EXPORT_SIZE }, 0.0f);
    EndTextureMode();

    Image image = LoadImageFromTexture(render.texture);
    ImageFlipVertical(&image);
    bool exported = ExportImage(image, pngPath);
    UnloadImage(image);
    UnloadRenderTexture(render);
    return exported;
}

static bool SaveSelection(int selected)
{
    char jsonPath[PATH_BUFFER_SIZE];
    char selectedAt[64] = "unknown";
    time_t now = time(NULL);
    struct tm localTime;

    if (selected < 0 || selected >= CONCEPT_COUNT) return false;
    if (!ExportSelectedIcon(selected)) return false;

    if (localtime_s(&localTime, &now) == 0)
        strftime(selectedAt, sizeof(selectedAt), "%Y-%m-%dT%H:%M:%S", &localTime);

    BuildApplicationPath(jsonPath, sizeof(jsonPath), OUT_JSON);
    FILE *file = fopen(jsonPath, "wb");
    if (file == NULL) return false;

    fprintf(file,
            "{\n"
            "  \"schema_version\": 1,\n"
            "  \"tool\": \"icon_ideas_menu_music\",\n"
            "  \"style\": \"flat_2d_menu_matched_music_note\",\n"
            "  \"selected_id\": %d,\n"
            "  \"selected_number\": %d,\n"
            "  \"key\": \"%s\",\n"
            "  \"name\": \"%s\",\n"
            "  \"selected_at\": \"%s\",\n"
            "  \"source_png\": \"%s\",\n"
            "  \"source_size\": 1024,\n"
            "  \"transparent\": true\n"
            "}\n",
            selected, selected + 1, CONCEPTS[selected].key, CONCEPTS[selected].name,
            selectedAt, OUT_PNG);

    return fclose(file) == 0;
}

static int LoadSelection(void)
{
    char jsonPath[PATH_BUFFER_SIZE];
    char buffer[4096];
    int selected = -1;
    BuildApplicationPath(jsonPath, sizeof(jsonPath), OUT_JSON);

    FILE *file = fopen(jsonPath, "rb");
    if (file == NULL) return -1;
    size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, file);
    fclose(file);
    buffer[bytesRead] = '\0';

    char *position = strstr(buffer, "\"selected_id\"");
    if (position != NULL && sscanf(position, "\"selected_id\": %d", &selected) == 1 &&
        selected >= 0 && selected < CONCEPT_COUNT)
        return selected;
    return -1;
}

int main(void)
{
    SetTraceLogLevel(LOG_WARNING);
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(1500, 880, "Icon-Ideas — Menue-Musiknote (Settings/Rakete)");
    if (!IsWindowReady()) return 1;

    SetWindowMinSize(1100, 700);
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);

    int selected = LoadSelection();
    // Old selections beyond new count are invalid
    if (selected >= CONCEPT_COUNT) selected = -1;
    int savedSelection = selected;
    char status[256];
    if (selected >= 0)
        snprintf(status, sizeof(status), "Geladen: %s  ->  %s", CONCEPTS[selected].name, OUT_PNG);
    else
        snprintf(status, sizeof(status), "Export: %s (transparent) + %s", OUT_PNG, OUT_JSON);

    const int numberKeys[CONCEPT_COUNT] = {
        KEY_ONE, KEY_TWO, KEY_THREE, KEY_FOUR, KEY_FIVE,
        KEY_SIX, KEY_SEVEN, KEY_EIGHT, KEY_NINE, KEY_ZERO
    };

    while (!WindowShouldClose()) {
        float t = (float)GetTime();
        int sw = GetScreenWidth();
        int sh = GetScreenHeight();
        Vector2 mouse = GetMousePosition();
        int hovered = -1;

        for (int i = 0; i < CONCEPT_COUNT; i++) {
            if (CheckCollisionPointRec(mouse, GetCardRectangle(i, sw, sh))) hovered = i;
            if (IsKeyPressed(numberKeys[i])) {
                selected = i;
                snprintf(status, sizeof(status), "Markiert: %s", CONCEPTS[i].name);
            }
        }

        if (hovered >= 0 && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            selected = hovered;
            snprintf(status, sizeof(status), "Markiert: %s", CONCEPTS[selected].name);
        }

        Rectangle saveButton = GetSaveButtonRectangle(sw, sh);
        bool saveRequested = (selected >= 0 && IsKeyPressed(KEY_ENTER)) ||
                             (selected >= 0 && CheckCollisionPointRec(mouse, saveButton) &&
                              IsMouseButtonPressed(MOUSE_BUTTON_LEFT));
        if (saveRequested) {
            if (SaveSelection(selected)) {
                savedSelection = selected;
                snprintf(status, sizeof(status), "Gespeichert: %s + %s", OUT_PNG, OUT_JSON);
            } else {
                snprintf(status, sizeof(status), "Speichern fehlgeschlagen.");
            }
        }

        if (IsKeyPressed(KEY_ESCAPE)) break;

        BeginDrawing();
            ClearBackground(BLACK);
            DrawSelectorUi(sw, sh, selected, savedSelection, hovered, status, t);
        EndDrawing();
    }

    CloseWindow();
    return 0;
}
