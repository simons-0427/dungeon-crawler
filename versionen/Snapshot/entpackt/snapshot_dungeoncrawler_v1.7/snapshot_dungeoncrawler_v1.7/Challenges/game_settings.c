// game_settings.c — load/save user preferences (audio, gameplay, UI, binds).
#include "game_settings.h"
#include "globals.h"
#include "constants.h"   // SETTINGS_FILE, KB_MAX_BINDS
#include "types.h"
#include "game_items.h"  // IK_COL
#include "game_utils.h"  // window geometry persistence
#include <stdio.h>
#include <string.h>

static void PackBinds(int act) {
    int w = 0;
    for (int i = 0; i < KB_MAX_BINDS; i++) {
        if (keybinds[act][i]) keybinds[act][w++] = keybinds[act][i];
    }
    for (int i = w; i < KB_MAX_BINDS; i++) keybinds[act][i] = 0;
}

static void BindDefault(int act, int k0, int k1) {
    keybinds[act][0] = k0;
    keybinds[act][1] = k1;
    for (int i = 2; i < KB_MAX_BINDS; i++) keybinds[act][i] = 0;
}

void ResetKeybindsToDefault(void) {
    memset(keybinds, 0, sizeof(keybinds));
    BindDefault(ACT_MOVE_UP,      KEY_W,    KEY_UP);
    BindDefault(ACT_MOVE_DOWN,    KEY_S,    KEY_DOWN);
    BindDefault(ACT_MOVE_LEFT,    KEY_A,    KEY_LEFT);
    BindDefault(ACT_MOVE_RIGHT,   KEY_D,    KEY_RIGHT);
    BindDefault(ACT_USE_1,        KEY_ONE,  0);
    BindDefault(ACT_USE_2,        KEY_TWO,  0);
    BindDefault(ACT_USE_3,        KEY_THREE,0);
    BindDefault(ACT_USE_4,        KEY_FOUR, 0);
    BindDefault(ACT_USE_5,        KEY_FIVE, 0);
    BindDefault(ACT_USE_SELECTED, KEY_SPACE, BIND_FROM_MOUSE(MOUSE_BUTTON_RIGHT));
    BindDefault(ACT_PAUSE,        KEY_ESCAPE,0);
    BindDefault(ACT_FULLSCREEN,   KEY_F11,  0);
    BindDefault(ACT_CONFIRM,      KEY_ENTER, 0);
    BindDefault(ACT_MENU_NEXT,    KEY_TAB,   0);
    BindDefault(ACT_MENU_LEFT,    KEY_LEFT,  0);
    BindDefault(ACT_MENU_RIGHT,   KEY_RIGHT, 0);
    BindDefault(ACT_MENU_UP,      KEY_UP,    0);
    BindDefault(ACT_MENU_DOWN,    KEY_DOWN,  0);
    BindDefault(ACT_MENU_BACK,    KEY_Q,     0);
}

int CountKeybinds(int act) {
    if (act < 0 || act >= ACT_COUNT) return 0;
    int n = 0;
    for (int i = 0; i < KB_MAX_BINDS; i++) if (keybinds[act][i]) n++;
    return n;
}

int FindActionWithKey(int key) {
    if (!key) return -1;
    for (int a = 0; a < ACT_COUNT; a++)
        for (int b = 0; b < KB_MAX_BINDS; b++)
            if (keybinds[a][b] == key) return a;
    return -1;
}

// PLAY-time actions can all fire in one frame. CONFIRM only fires on the
// menu / death / next-round screens. FULLSCREEN is polled always, so it
// collides with both. SPACE on USE SELECTED + NEXT ROUND is allowed.
static unsigned ActionContext(int act) {
    switch (act) {
        case ACT_MOVE_UP:
        case ACT_MOVE_DOWN:
        case ACT_MOVE_LEFT:
        case ACT_MOVE_RIGHT:
        case ACT_USE_1:
        case ACT_USE_2:
        case ACT_USE_3:
        case ACT_USE_4:
        case ACT_USE_5:
        case ACT_USE_SELECTED:
        case ACT_PAUSE:
            return 1u;
        case ACT_CONFIRM:
            return 2u;
        case ACT_MENU_NEXT:
        case ACT_MENU_LEFT:
        case ACT_MENU_RIGHT:
        case ACT_MENU_UP:
        case ACT_MENU_DOWN:
            return 4u;        // menus; may share keys with movement
        case ACT_MENU_BACK:
            return 2u | 4u;   // death overlay + menus (default Q)
        case ACT_FULLSCREEN:
            return 1u | 2u | 4u;
        default:
            return ~0u;
    }
}

static int FindConflict(int key, int act, int exceptIndex) {
    if (!key || act < 0 || act >= ACT_COUNT) return -1;
    unsigned ctx = ActionContext(act);
    for (int a = 0; a < ACT_COUNT; a++) {
        for (int b = 0; b < KB_MAX_BINDS; b++) {
            if (keybinds[a][b] != key) continue;
            if (a == act && b == exceptIndex) continue;
            if (a == act || (ActionContext(a) & ctx) != 0) return a;
        }
    }
    return -1;
}

int FindConflictingAction(int key, int act) {
    return FindConflict(key, act, -1);
}

bool AddKeybind(int act, int key) {
    if (!key || act < 0 || act >= ACT_COUNT) return false;
    PackBinds(act);
    if (CountKeybinds(act) >= KB_MAX_BINDS) return false;
    if (FindConflict(key, act, -1) >= 0) return false;
    keybinds[act][CountKeybinds(act)] = key;
    return true;
}

bool ReplaceKeybind(int act, int index, int key) {
    if (!key || act < 0 || act >= ACT_COUNT) return false;
    if (index < 0 || index >= KB_MAX_BINDS || !keybinds[act][index]) return false;
    if (keybinds[act][index] == key) return true;
    if (FindConflict(key, act, index) >= 0) return false;
    keybinds[act][index] = key;
    return true;
}

bool RemoveKeybind(int act, int index) {
    if (act < 0 || act >= ACT_COUNT) return false;
    if (index < 0 || index >= KB_MAX_BINDS) return false;
    if (!keybinds[act][index]) return false;
    if (CountKeybinds(act) <= 1) return false;
    for (int i = index; i < KB_MAX_BINDS - 1; i++)
        keybinds[act][i] = keybinds[act][i + 1];
    keybinds[act][KB_MAX_BINDS - 1] = 0;
    return true;
}

void FormatKeyName(int key, char *buf, int n) {
    if (!buf || n <= 0) return;
    buf[0] = '\0';
    if (BIND_IS_MOUSE(key)) {
        int btn = MOUSE_FROM_BIND(key);
        const char *mn = "MOUSE";
        switch (btn) {
            case MOUSE_BUTTON_LEFT:    mn = "LMB";    break;
            case MOUSE_BUTTON_RIGHT:   mn = "RMB";    break;
            case MOUSE_BUTTON_MIDDLE:  mn = "MMB";    break;
            case MOUSE_BUTTON_SIDE:    mn = "MOUSE4"; break;
            case MOUSE_BUTTON_EXTRA:   mn = "MOUSE5"; break;
            case MOUSE_BUTTON_FORWARD: mn = "MOUSE6"; break;
            case MOUSE_BUTTON_BACK:    mn = "MOUSE7"; break;
            default: break;
        }
        strncpy(buf, mn, (size_t)n);
        buf[n - 1] = '\0';
        return;
    }
    switch (key) {
        case KEY_SPACE:          strncpy(buf, "SPACE",   (size_t)n); break;
        case KEY_ESCAPE:         strncpy(buf, "ESC",     (size_t)n); break;
        case KEY_ENTER:          strncpy(buf, "ENTER",   (size_t)n); break;
        case KEY_TAB:            strncpy(buf, "TAB",     (size_t)n); break;
        case KEY_BACKSPACE:      strncpy(buf, "BKSP",    (size_t)n); break;
        case KEY_INSERT:         strncpy(buf, "INS",     (size_t)n); break;
        case KEY_DELETE:         strncpy(buf, "DEL",     (size_t)n); break;
        case KEY_RIGHT:          strncpy(buf, "RIGHT",   (size_t)n); break;
        case KEY_LEFT:           strncpy(buf, "LEFT",    (size_t)n); break;
        case KEY_DOWN:           strncpy(buf, "DOWN",    (size_t)n); break;
        case KEY_UP:             strncpy(buf, "UP",      (size_t)n); break;
        case KEY_PAGE_UP:        strncpy(buf, "PGUP",    (size_t)n); break;
        case KEY_PAGE_DOWN:      strncpy(buf, "PGDN",    (size_t)n); break;
        case KEY_HOME:           strncpy(buf, "HOME",    (size_t)n); break;
        case KEY_END:            strncpy(buf, "END",     (size_t)n); break;
        case KEY_CAPS_LOCK:      strncpy(buf, "CAPS",    (size_t)n); break;
        case KEY_LEFT_SHIFT:     strncpy(buf, "L-SHIFT", (size_t)n); break;
        case KEY_LEFT_CONTROL:   strncpy(buf, "L-CTRL",  (size_t)n); break;
        case KEY_LEFT_ALT:       strncpy(buf, "L-ALT",   (size_t)n); break;
        case KEY_LEFT_SUPER:     strncpy(buf, "L-WIN",   (size_t)n); break;
        case KEY_RIGHT_SHIFT:    strncpy(buf, "R-SHIFT", (size_t)n); break;
        case KEY_RIGHT_CONTROL:  strncpy(buf, "R-CTRL",  (size_t)n); break;
        case KEY_RIGHT_ALT:      strncpy(buf, "R-ALT",   (size_t)n); break;
        case KEY_RIGHT_SUPER:    strncpy(buf, "R-WIN",   (size_t)n); break;
        case KEY_KB_MENU:        strncpy(buf, "MENU",    (size_t)n); break;
        case KEY_KP_0:           strncpy(buf, "KP0",     (size_t)n); break;
        case KEY_KP_1:           strncpy(buf, "KP1",     (size_t)n); break;
        case KEY_KP_2:           strncpy(buf, "KP2",     (size_t)n); break;
        case KEY_KP_3:           strncpy(buf, "KP3",     (size_t)n); break;
        case KEY_KP_4:           strncpy(buf, "KP4",     (size_t)n); break;
        case KEY_KP_5:           strncpy(buf, "KP5",     (size_t)n); break;
        case KEY_KP_6:           strncpy(buf, "KP6",     (size_t)n); break;
        case KEY_KP_7:           strncpy(buf, "KP7",     (size_t)n); break;
        case KEY_KP_8:           strncpy(buf, "KP8",     (size_t)n); break;
        case KEY_KP_9:           strncpy(buf, "KP9",     (size_t)n); break;
        case KEY_KP_DECIMAL:     strncpy(buf, "KP.",     (size_t)n); break;
        case KEY_KP_DIVIDE:      strncpy(buf, "KP/",     (size_t)n); break;
        case KEY_KP_MULTIPLY:    strncpy(buf, "KP*",     (size_t)n); break;
        case KEY_KP_SUBTRACT:    strncpy(buf, "KP-",     (size_t)n); break;
        case KEY_KP_ADD:         strncpy(buf, "KP+",     (size_t)n); break;
        case KEY_KP_ENTER:       strncpy(buf, "KP-ENT",  (size_t)n); break;
        case KEY_KP_EQUAL:       strncpy(buf, "KP=",     (size_t)n); break;
        case KEY_MINUS:          strncpy(buf, "-",       (size_t)n); break;
        case KEY_EQUAL:          strncpy(buf, "=",       (size_t)n); break;
        case KEY_LEFT_BRACKET:   strncpy(buf, "[",       (size_t)n); break;
        case KEY_RIGHT_BRACKET:  strncpy(buf, "]",       (size_t)n); break;
        case KEY_BACKSLASH:      strncpy(buf, "\\",      (size_t)n); break;
        case KEY_SEMICOLON:      strncpy(buf, ";",       (size_t)n); break;
        case KEY_APOSTROPHE:     strncpy(buf, "'",       (size_t)n); break;
        case KEY_COMMA:          strncpy(buf, ",",       (size_t)n); break;
        case KEY_PERIOD:         strncpy(buf, ".",       (size_t)n); break;
        case KEY_SLASH:          strncpy(buf, "/",       (size_t)n); break;
        case KEY_GRAVE:          strncpy(buf, "`",       (size_t)n); break;
        default:
            if (key >= KEY_F1 && key <= KEY_F12)
                snprintf(buf, (size_t)n, "F%d", key - KEY_F1 + 1);
            else if (key >= 32 && key <= 126)
                snprintf(buf, (size_t)n, "%c", key);
            else
                snprintf(buf, (size_t)n, "K%d", key);
            break;
    }
    buf[n - 1] = '\0';
}

#ifdef DEBUG_ROOMS
bool ActionDown(int act) {
    switch (act) {
        case ACT_MOVE_UP:    return IsKeyDown(KEY_W);
        case ACT_MOVE_DOWN:  return IsKeyDown(KEY_S);
        case ACT_MOVE_LEFT:  return IsKeyDown(KEY_A);
        case ACT_MOVE_RIGHT: return IsKeyDown(KEY_D);
        case ACT_CONFIRM:    return IsKeyDown(KEY_ENTER);
        default:             return false;
    }
}

bool ActionPressed(int act) {
    switch (act) {
        case ACT_USE_1:        return IsKeyPressed(KEY_ONE);
        case ACT_USE_2:        return IsKeyPressed(KEY_TWO);
        case ACT_USE_3:        return IsKeyPressed(KEY_THREE);
        case ACT_USE_4:        return IsKeyPressed(KEY_FOUR);
        case ACT_USE_5:        return IsKeyPressed(KEY_FIVE);
        case ACT_USE_SELECTED: return IsKeyPressed(KEY_SPACE) ||
                                      IsMouseButtonPressed(MOUSE_BUTTON_RIGHT);
        case ACT_PAUSE:        return IsKeyPressed(KEY_ESCAPE);
        case ACT_FULLSCREEN:   return IsKeyPressed(KEY_F11);
        case ACT_CONFIRM:      return IsKeyPressed(KEY_ENTER);
        case ACT_MENU_NEXT:    return IsKeyPressed(KEY_TAB);
        case ACT_MENU_LEFT:    return IsKeyPressed(KEY_LEFT);
        case ACT_MENU_RIGHT:   return IsKeyPressed(KEY_RIGHT);
        case ACT_MENU_UP:      return IsKeyPressed(KEY_UP);
        case ACT_MENU_DOWN:    return IsKeyPressed(KEY_DOWN);
        case ACT_MENU_BACK:    return IsKeyPressed(KEY_Q);
        default:               return false;
    }
}

bool ActionPressedKey(int act) {
    return ActionPressed(act);
}
#else
static bool BindDown(int k) {
    if (BIND_IS_MOUSE(k)) {
        int btn = MOUSE_FROM_BIND(k);
        if (btn < 0 || btn > BIND_MOUSE_MAX_BTN) return false;
        return IsMouseButtonDown(btn);
    }
    return k != 0 && IsKeyDown(k);
}

static bool BindPressed(int k) {
    if (BIND_IS_MOUSE(k)) {
        int btn = MOUSE_FROM_BIND(k);
        if (btn < 0 || btn > BIND_MOUSE_MAX_BTN) return false;
        return IsMouseButtonPressed(btn);
    }
    return k != 0 && IsKeyPressed(k);
}

bool ActionDown(int act) {
    if (act < 0 || act >= ACT_COUNT) return false;
    for (int i = 0; i < KB_MAX_BINDS; i++) {
        if (BindDown(keybinds[act][i])) return true;
    }
    return false;
}

bool ActionPressed(int act) {
    if (act < 0 || act >= ACT_COUNT) return false;
    for (int i = 0; i < KB_MAX_BINDS; i++) {
        if (BindPressed(keybinds[act][i])) return true;
    }
    return false;
}

bool ActionPressedKey(int act) {
    if (act < 0 || act >= ACT_COUNT) return false;
    for (int i = 0; i < KB_MAX_BINDS; i++) {
        int k = keybinds[act][i];
        if (k > 0 && IsKeyPressed(k)) return true;
    }
    return false;
}
#endif

void FormatActionBinds(int act, char *buf, int n) {
    if (!buf || n <= 0) return;
    buf[0] = '\0';
#ifdef DEBUG_ROOMS
    const char *d = "UNBOUND";
    switch (act) {
        case ACT_MOVE_UP:      d = "W / UP";     break;
        case ACT_MOVE_DOWN:    d = "S / DOWN";   break;
        case ACT_MOVE_LEFT:    d = "A / LEFT";   break;
        case ACT_MOVE_RIGHT:   d = "D / RIGHT";  break;
        case ACT_USE_1:        d = "1";          break;
        case ACT_USE_2:        d = "2";          break;
        case ACT_USE_3:        d = "3";          break;
        case ACT_USE_4:        d = "4";          break;
        case ACT_USE_5:        d = "5";          break;
        case ACT_USE_SELECTED: d = "SPACE / RMB"; break;
        case ACT_PAUSE:        d = "ESC";        break;
        case ACT_FULLSCREEN:   d = "F11";        break;
        case ACT_CONFIRM:      d = "ENTER";      break;
        case ACT_MENU_NEXT:    d = "TAB";        break;
        case ACT_MENU_LEFT:    d = "LEFT";       break;
        case ACT_MENU_RIGHT:   d = "RIGHT";      break;
        case ACT_MENU_UP:      d = "UP";         break;
        case ACT_MENU_DOWN:    d = "DOWN";       break;
        case ACT_MENU_BACK:    d = "Q";          break;
        default: break;
    }
    strncpy(buf, d, (size_t)n);
    buf[n - 1] = '\0';
    return;
#else
    if (act < 0 || act >= ACT_COUNT) {
        strncpy(buf, "UNBOUND", (size_t)n);
        buf[n - 1] = '\0';
        return;
    }
    int used = 0;
    for (int i = 0; i < KB_MAX_BINDS; i++) {
        int k = keybinds[act][i];
        if (!k) continue;
        char kn[24];
        FormatKeyName(k, kn, (int)sizeof(kn));
        int knl = (int)strlen(kn);
        if (knl <= 0) continue;
        if (used == 0) {
            if (knl >= n) knl = n - 1;
            memcpy(buf, kn, (size_t)knl);
            buf[knl] = '\0';
            used = knl;
        } else {
            if (used + 3 + knl >= n) break;
            memcpy(buf + used, " / ", 3);
            used += 3;
            memcpy(buf + used, kn, (size_t)knl);
            used += knl;
            buf[used] = '\0';
        }
    }
    if (used == 0) strncpy(buf, "UNBOUND", (size_t)n);
    buf[n - 1] = '\0';
#endif
}

static void AppendBindToken(char *buf, int n, int *used, const char *tok) {
    int knl = (int)strlen(tok);
    if (knl <= 0 || !used) return;
    if (*used == 0) {
        if (knl >= n) knl = n - 1;
        memcpy(buf, tok, (size_t)knl);
        buf[knl] = '\0';
        *used = knl;
    } else if (*used + 1 + knl < n) {
        buf[(*used)++] = '/';
        memcpy(buf + *used, tok, (size_t)knl);
        *used += knl;
        buf[*used] = '\0';
    }
}

void FormatMoveHint(char *buf, int n) {
    if (!buf || n <= 0) return;
    buf[0] = '\0';
    const int acts[4] = { ACT_MOVE_UP, ACT_MOVE_LEFT, ACT_MOVE_DOWN, ACT_MOVE_RIGHT };
    int used = 0;
    for (int i = 0; i < 4; i++) {
        int key = keybinds[acts[i]][0];
        if (!key) continue;
        char kn[24];
        FormatKeyName(key, kn, (int)sizeof(kn));
        AppendBindToken(buf, n, &used, kn);
    }
    if (used == 0) strncpy(buf, "MOVE", (size_t)n);
    buf[n - 1] = '\0';
}

void FormatUseSlotHint(char *buf, int n) {
    if (!buf || n <= 0) return;
    buf[0] = '\0';
    char keys[5][16];
    int got = 0;
    for (int a = 0; a < 5; a++) {
        int key = keybinds[ACT_USE_1 + a][0];
        if (!key) continue;
        FormatKeyName(key, keys[got], (int)sizeof(keys[got]));
        got++;
    }
    if (got == 0) {
        strncpy(buf, "1-5", (size_t)n);
        buf[n - 1] = '\0';
        return;
    }
    if (got == 5) {
        bool seq = true;
        for (int i = 0; i < 5; i++) {
            if (!keys[i][0] || keys[i][1] != '\0') { seq = false; break; }
            if (i > 0 && keys[i][0] != (char)(keys[0][0] + i)) { seq = false; break; }
        }
        if (seq) {
            snprintf(buf, (size_t)n, "%c-%c", keys[0][0], keys[4][0]);
            buf[n - 1] = '\0';
            return;
        }
    }
    int used = 0;
    for (int i = 0; i < got; i++)
        AppendBindToken(buf, n, &used, keys[i]);
    buf[n - 1] = '\0';
}

static void EnsureDefaultsIfEmpty(void) {
    for (int a = 0; a < ACT_COUNT; a++) {
        PackBinds(a);
        if (CountKeybinds(a) == 0) {
            // Restore just this action from the default table.
            int tmp[ACT_COUNT][KB_MAX_BINDS];
            memcpy(tmp, keybinds, sizeof(tmp));
            ResetKeybindsToDefault();
            memcpy(tmp[a], keybinds[a], sizeof(tmp[a]));
            memcpy(keybinds, tmp, sizeof(tmp));
        }
    }
}

static Color RGB(int r, int g, int b) {
    return (Color){ (unsigned char)r, (unsigned char)g, (unsigned char)b, 255 };
}

void ApplyColorblindMode(void) {
    int m = colorblind_mode;
    if (m < CB_OFF) m = CB_OFF;
    if (m > CB_HIGHCONTRAST) m = CB_HIGHCONTRAST;
    colorblind_mode = m;

    // Walls / floor stay muted so entities pop.
    pal_b  = RGB(12, 10, 18);
    pal_f  = RGB(28, 24, 40);
    pal_w  = RGB(50, 45, 66);
    pal_wh = RGB(72, 65, 92);
    pal_wd = RGB(18, 15, 28);

    if (m == CB_OFF) {
        pal_trap  = RGB(195, 40, 40);
        pal_tres  = RGB(255, 200, 50);
        pal_pl    = RGB(90, 220, 120);
        pal_pd    = RGB(230, 55, 55);
        pal_coin  = RGB(255, 215, 0);
        pal_port  = RGB(80, 160, 255);
        pal_ene   = RGB(220, 65, 65);
        pal_shd   = RGB(80, 150, 255);
        pal_wlk   = RGB(180, 80, 255);
        pal_swd   = RGB(255, 60, 120);
        pal_boss  = RGB(170, 40, 200);
        pal_plate = RGB(95, 210, 185);
        pal_cann  = RGB(225, 180, 90);
        pal_fire  = RGB(255, 110, 30);
        IK_COL[0] = RGB(80, 220, 80);    // SPD
        IK_COL[1] = RGB(80, 150, 255);   // SHD
        IK_COL[2] = RGB(255, 140, 40);   // BMB
        IK_COL[3] = RGB(180, 80, 255);   // WLK
        IK_COL[4] = RGB(255, 60, 120);   // SWD
        IK_COL[5] = RGB(255, 200, 40);   // GLD
        return;
    }

    if (m == CB_REDGREEN) {
        // Wong / IBM palette: sky player, orange traps, magenta enemies,
        // yellow damage, gold loot, blue portal. No red-vs-green pairs.
        pal_pl    = RGB(86, 180, 233);
        pal_pd    = RGB(240, 228, 66);
        pal_trap  = RGB(230, 159, 0);
        pal_ene   = RGB(204, 121, 167);
        pal_tres  = RGB(255, 200, 50);
        pal_coin  = RGB(255, 225, 80);
        pal_port  = RGB(0, 114, 178);
        pal_shd   = RGB(0, 158, 115);
        pal_wlk   = RGB(140, 80, 220);
        pal_swd   = RGB(255, 109, 182);
        pal_boss  = RGB(88, 40, 180);
        pal_plate = RGB(0, 180, 170);
        pal_cann  = RGB(240, 200, 80);
        pal_fire  = RGB(255, 210, 70);
        IK_COL[0] = RGB(86, 180, 233);
        IK_COL[1] = RGB(0, 158, 115);
        IK_COL[2] = RGB(230, 159, 0);
        IK_COL[3] = RGB(140, 80, 220);
        IK_COL[4] = RGB(255, 109, 182);
        IK_COL[5] = RGB(255, 200, 50);
        return;
    }

    if (m == CB_BLUEYELLOW) {
        // Tritanopia: move portals/shields off blue, loot off yellow-gold.
        pal_pl    = RGB(80, 230, 120);
        pal_pd    = RGB(230, 50, 70);
        pal_trap  = RGB(200, 40, 50);
        pal_ene   = RGB(160, 20, 90);
        pal_tres  = RGB(255, 90, 90);
        pal_coin  = RGB(255, 255, 255);
        pal_port  = RGB(0, 200, 120);
        pal_shd   = RGB(255, 110, 170);
        pal_wlk   = RGB(210, 80, 90);
        pal_swd   = RGB(255, 70, 140);
        pal_boss  = RGB(255, 90, 180);
        pal_plate = RGB(40, 200, 110);
        pal_cann  = RGB(255, 120, 90);
        pal_fire  = RGB(255, 70, 50);
        pal_w     = RGB(55, 55, 58);
        pal_wh    = RGB(80, 80, 84);
        IK_COL[0] = RGB(80, 230, 120);
        IK_COL[1] = RGB(255, 110, 170);
        IK_COL[2] = RGB(255, 90, 50);
        IK_COL[3] = RGB(210, 80, 90);
        IK_COL[4] = RGB(255, 70, 140);
        IK_COL[5] = RGB(255, 90, 90);
        return;
    }

    // High contrast: unique hues, darker floors, brighter actors.
    pal_b     = RGB(6, 6, 8);
    pal_f     = RGB(18, 18, 22);
    pal_w     = RGB(88, 88, 92);
    pal_wh    = RGB(130, 130, 135);
    pal_wd    = RGB(10, 10, 12);
    pal_pl    = RGB(245, 250, 255);
    pal_pd    = RGB(255, 240, 40);
    pal_trap  = RGB(255, 220, 0);
    pal_ene   = RGB(255, 0, 200);
    pal_tres  = RGB(255, 140, 0);
    pal_coin  = RGB(255, 230, 40);
    pal_port  = RGB(0, 255, 255);
    pal_shd   = RGB(80, 170, 255);
    pal_wlk   = RGB(200, 110, 255);
    pal_swd   = RGB(255, 70, 180);
    pal_boss  = RGB(255, 70, 255);
    pal_plate = RGB(0, 255, 180);
    pal_cann  = RGB(255, 200, 60);
    pal_fire  = RGB(255, 255, 180);
    IK_COL[0] = RGB(245, 250, 255);
    IK_COL[1] = RGB(80, 170, 255);
    IK_COL[2] = RGB(255, 140, 0);
    IK_COL[3] = RGB(200, 110, 255);
    IK_COL[4] = RGB(255, 70, 180);
    IK_COL[5] = RGB(255, 230, 40);
}

void LoadSettings(void) {
    ResetKeybindsToDefault();
    FILE *f = fopen(SETTINGS_FILE, "r");
    if (!f) { ApplyColorblindMode(); return; }
    int mut = 0, fps = 0;
    int saved_menu_mask = 0, saved_gameplay_mask = 0;
    int saved_menu_random = 0, saved_gameplay_random = 0, settings_version = 0;
    int saved_photosense = 0, saved_shake = 0, saved_loud = 0, saved_focus = 1, saved_skip = 0, saved_exitw = 1;
    int saved_showseed = 0;
    // Line 1 only — extra %d must not steal ACT_COUNT off the keybind line.
    char line1[512];
    if (!fgets(line1, (int)sizeof(line1), f)) { fclose(f); return; }
    int readCount = sscanf(line1, "%f %f %f %d %d %f %d %f %d %d %d %d %d %d %d %d %d %d %d %d",
           &vol_master, &vol_music, &vol_sfx, &mut,
           &par_density, &cam_smooth, &fps, &hud_opacity,
           &saved_menu_mask, &saved_gameplay_mask,
           &saved_menu_random, &saved_gameplay_random, &settings_version,
           &saved_photosense, &saved_shake, &saved_loud, &saved_focus, &saved_skip,
           &saved_exitw, &saved_showseed);
    audio_mute = (bool)mut;
    show_fps   = (bool)fps;
    // Clamp all values to valid ranges.
    if (vol_master  < 0.0f)  vol_master  = 0.0f;
    if (vol_master  > 1.0f)  vol_master  = 1.0f;
    if (vol_music   < 0.0f)  vol_music   = 0.0f;
    if (vol_music   > 1.0f)  vol_music   = 1.0f;
    if (vol_sfx     < 0.0f)  vol_sfx     = 0.0f;
    if (vol_sfx     > 1.0f)  vol_sfx     = 1.0f;
    if (par_density < 0)     par_density = 0;
    if (par_density > 2)     par_density = 2;
    if (cam_smooth  < 1.0f)  cam_smooth  = 1.0f;
    if (cam_smooth  > 20.0f) cam_smooth  = 20.0f;
    if (hud_opacity < 0.2f)  hud_opacity = 0.2f;
    if (hud_opacity > 1.0f)  hud_opacity = 1.0f;

    // Version 90+ stores the user's window/fullscreen choice and geometry.
    // New installs get a near-fullscreen window; existing choices are kept.
    // Version 89+ stores show-seed (default OFF).
    // Version 88+ stores exit-warning (default ON).
    // Version 87+ stores skip-play-anim (default OFF).
    // Version 86+ stores loud-sfx default ON (v85 shipped it default-off).
    // Version 85+ stores screen-shake (default OFF) and focus-pause (ON).
    // Version 84+ stores photosensitivity (default OFF).
    // Version 83+ stores mouse-select (default OFF) + colorblind.
    // v80-82 saved mouse-select with the old default ON — ignore that field.
    // Version 79 stores only the two track masks. Version 78 briefly stored
    // extra RANDOM ROTATION flags; migrate those modes into track masks.
    if (readCount >= 11 && (saved_menu_random == 80 || saved_menu_random == 81
                            || saved_menu_random == 82 || saved_menu_random == 83
                            || saved_menu_random == 84 || saved_menu_random == 85
                            || saved_menu_random == 86 || saved_menu_random == 87
                            || saved_menu_random == 88 || saved_menu_random == 89
                            || saved_menu_random == 90)) {
        music_menu_tracks     = (unsigned int)saved_menu_mask;
        music_gameplay_tracks = (unsigned int)saved_gameplay_mask;
        if (saved_menu_random >= 83 && readCount >= 12)
            mouse_item_select = (bool)saved_gameplay_random;
        else
            mouse_item_select = false;
        if (saved_menu_random >= 82 && readCount >= 13) {
            colorblind_mode = settings_version;
            if (colorblind_mode < 0) colorblind_mode = 0;
            if (colorblind_mode > CB_HIGHCONTRAST) colorblind_mode = CB_HIGHCONTRAST;
        }
        if (saved_menu_random >= 84 && readCount >= 14)
            photosensitivity = saved_photosense != 0;
        else
            photosensitivity = false;
        if (saved_menu_random >= 85 && readCount >= 17) {
            screen_shake = saved_shake != 0;
            focus_pause  = saved_focus != 0;
            // v85 saved loud_sfx default-off; treat those as ON unless v86+.
            loud_sfx = (saved_menu_random >= 86) ? (saved_loud != 0) : true;
        } else {
            screen_shake = false;
            loud_sfx     = true;
            focus_pause  = true;
        }
        if (saved_menu_random >= 87 && readCount >= 18)
            skip_play_anim = saved_skip != 0;
        else
            skip_play_anim = false;
        if (saved_menu_random >= 88 && readCount >= 19)
            exit_warning = saved_exitw != 0;
        else
            exit_warning = true;
        if (saved_menu_random >= 89 && readCount >= 20)
            show_seed = saved_showseed != 0;
        else
            show_seed = false;
        int ac = 0, mb = 0;
        bool binds_complete = false;
        if (fscanf(f, "%d %d", &ac, &mb) == 2) {
            if (ac < 0) ac = 0;
            if (mb < 0) mb = 0;
            if (ac > ACT_COUNT + 8) ac = ACT_COUNT + 8;
            if (mb > KB_MAX_BINDS + 4) mb = KB_MAX_BINDS + 4;
            int tmp = 0;
            for (int a = 0; a < ac; a++) {
                for (int b = 0; b < mb; b++) {
                    if (fscanf(f, "%d", &tmp) != 1) goto binds_done;
                    if (a < ACT_COUNT && b < KB_MAX_BINDS) keybinds[a][b] = tmp;
                }
            }
            binds_complete = true;
        }
    binds_done:
        EnsureDefaultsIfEmpty();
        // v80 had SPACE-only for USE SELECTED; RMB was hardcoded. Fold it in.
        if (saved_menu_random == 80) {
            int rmb = BIND_FROM_MOUSE(MOUSE_BUTTON_RIGHT);
            if (FindActionWithKey(rmb) < 0 &&
                CountKeybinds(ACT_USE_SELECTED) < KB_MAX_BINDS)
                keybinds[ACT_USE_SELECTED][CountKeybinds(ACT_USE_SELECTED)] = rmb;
        }
        if (binds_complete) {
            int win_fs = 0, win_x = 0, win_y = 0, win_w = 0, win_h = 0;
            if (fscanf(f, "%d %d %d %d %d",
                       &win_fs, &win_x, &win_y, &win_w, &win_h) == 5) {
                SetPersistedWindowState(win_fs, win_x, win_y, win_w, win_h);
            }
        }
    } else if (readCount >= 13 && settings_version == 78) {
        music_menu_tracks = saved_menu_random
                              ? 0x0Fu : (unsigned int)saved_menu_mask;
        music_gameplay_tracks = saved_gameplay_random
                                  ? 0x0Eu : (unsigned int)saved_gameplay_mask;
    } else if (readCount >= 11 && saved_menu_random == 79) {
        music_menu_tracks     = (unsigned int)saved_menu_mask;
        music_gameplay_tracks = (unsigned int)saved_gameplay_mask;
    // Version 77 already used masks, but zero meant OFF for the menu and
    // RANDOM for gameplay. Preserve the old gameplay rotation as a playlist.
    } else if (readCount >= 11 && saved_menu_random == 77) {
        music_menu_tracks     = (unsigned int)saved_menu_mask;
        music_gameplay_tracks = saved_gameplay_mask != 0
                                  ? (unsigned int)saved_gameplay_mask : 0x0Eu;
    } else {
        // Older files stored one menu track and one gameplay track.
        music_menu_tracks = (saved_menu_mask >= 0 && saved_menu_mask <= 3)
                              ? (1u << saved_menu_mask) : 0u;
        music_gameplay_tracks = (saved_gameplay_mask >= 1 && saved_gameplay_mask <= 3)
                                  ? (1u << saved_gameplay_mask) : 0x0Eu;
    }
    unsigned int musicMask = (1u << MUSIC_TRACK_COUNT) - 1u;
    music_menu_tracks     &= musicMask;
    music_gameplay_tracks &= musicMask;
    fclose(f);
    ApplyColorblindMode();
}

void SaveSettings(void) {
    CaptureWindowState();
    FILE *f = fopen(SETTINGS_FILE, "w");
    if (!f) return;
    fprintf(f, "%g %g %g %d %d %g %d %g %d %d %d %d %d %d %d %d %d %d %d %d\n",
            vol_master, vol_music, vol_sfx, (int)audio_mute,
            par_density, cam_smooth, (int)show_fps, hud_opacity,
            (int)music_menu_tracks, (int)music_gameplay_tracks, 90,
            (int)mouse_item_select, colorblind_mode, (int)photosensitivity,
            (int)screen_shake, (int)loud_sfx, (int)focus_pause, (int)skip_play_anim,
            (int)exit_warning, (int)show_seed);
    fprintf(f, "%d %d", ACT_COUNT, KB_MAX_BINDS);
    for (int a = 0; a < ACT_COUNT; a++)
        for (int b = 0; b < KB_MAX_BINDS; b++)
            fprintf(f, " %d", keybinds[a][b]);
    fprintf(f, "\n");
    int win_fs = 0, win_x = 0, win_y = 0, win_w = SW, win_h = SH;
    GetPersistedWindowState(&win_fs, &win_x, &win_y, &win_w, &win_h);
    fprintf(f, "%d %d %d %d %d\n", win_fs, win_x, win_y, win_w, win_h);
    fclose(f);
}

#ifdef DEBUG_ROOMS
int debugSecretChance = 1, debugPortalChance = 1;
static int DebugChanceClamp(int value) {
    return value < 1 ? 1 : value > 100 ? 100 : value;
}
void SaveDebugRoomSettings(void) {
    if (!DirectoryExists("Debug_Test") && MakeDirectory("Debug_Test") != 0) {
        TraceLog(LOG_WARNING, "Cannot create Debug_Test"); return;
    }
    FILE *f = fopen("Debug_Test/room_chances.txt", "w");
    if (!f) { TraceLog(LOG_WARNING, "Cannot save debug room chances"); return; }
    fprintf(f, "secret_room_percent=%d\nartifact_portal_percent=%d\n",
            debugSecretChance, debugPortalChance);
    fclose(f);
}
void LoadDebugRoomSettings(void) {
    debugSecretChance = debugPortalChance = 1;
    FILE *f = fopen("Debug_Test/room_chances.txt", "r");
    if (f) {
        char line[128]; int value;
        while (fgets(line,sizeof(line),f)) {
            if (sscanf(line,"secret_room_percent=%d",&value)==1)
                debugSecretChance=DebugChanceClamp(value);
            else if (sscanf(line,"artifact_portal_percent=%d",&value)==1)
                debugPortalChance=DebugChanceClamp(value);
        }
        fclose(f);
    }
    SaveDebugRoomSettings();
}
void SetDebugRoomChance(bool portal, int value) {
    if (portal) debugPortalChance=DebugChanceClamp(value);
    else debugSecretChance=DebugChanceClamp(value);
    SaveDebugRoomSettings();
}
#endif
