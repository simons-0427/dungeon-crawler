// game_map.c — round generation and tile/map rendering.
#include "game_map.h"
#include "globals.h"
#include "constants.h"
#include "types.h"
#include "game_items.h"      // IK_COL
#include "game_utils.h"      // UpdateCameraOffset
#include "game_boss.h"       // SpawnBoss
#include "game_particles.h"  // Burst (lava spread)
#include "game_save.h"       // UpdateAndSaveStats (lava death)
#include "game_sfx.h"        // PlaySfx
#include <math.h>
#include <string.h>          // memset
#include <stdlib.h>          // abs

// ─── GENERATE ROUND: NORMAL ───────────────────────────────────────────────────
static void GenNormalRound(void) {
    // Map size grows with roundn (like V1.5). Walls/traps grow with normn only
    // (completed NORMAL floors — boss/challenge don't densify the maze).
    // Coin Rush: denser traps + enemies, more coins.
    mh = 13 + (roundn - 1); if (mh > 42) mh = 42;
    mw = 23 + 2 * (roundn - 1); if (mw > 76) mw = 76;
    // Wall chance (#): 9% in round 1, then +1% per completed normal (normn).
    // Coin Rush: 10% + normn (max 30).
    int tp = 9 + normn;  if (tp > 34) tp = 34;
    if (coinRush) { tp = 10 + normn; if (tp > 30) tp = 30; }
    // Trap chance (+): fixed 5% band above walls (like V1.5).
    int xp = tp + 5;
    if (xp > 55) xp = 55;

    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++) {
        if (x==0||y==0||x==mw-1||y==mh-1) { map[y][x]='#'; continue; }
        int r = GetRandomValue(0,99);
        map[y][x] = (r < tp) ? '#' : (r < xp) ? '+' : '.';
    }

    px = mw/2; py = mh/2; map[py][px] = '.';
    // Clear a small spawn pocket so Coin Rush traps don't spawn-kill.
    if (coinRush) {
        for (int dy=-1; dy<=1; dy++) for (int dx=-1; dx<=1; dx++)
            if (py+dy>0 && py+dy<mh-1 && px+dx>0 && px+dx<mw-1) map[py+dy][px+dx] = '.';
    }

    for (;;) {
        int tx = GetRandomValue(1,mw-2), ty = GetRandomValue(1,mh-2);
        if (map[ty][tx]!='#' && !(tx==px&&ty==py)) { map[ty][tx]='T'; break; }
    }

    for (int i = 0; i < MAX_PRT; i++) prts[i].on = false;
    for (int i = 0; i < MAX_PRT; i++) {
        // Coin Rush: rarer free portals — you work for the loot.
        if (GetRandomValue(0, coinRush ? 3 : 1)==0) {
            for (int t = 0; t < 300; t++) {
                int ox = GetRandomValue(1,mw-2), oy = GetRandomValue(1,mh-2);
                if (map[oy][ox]=='.') {
                    prts[i] = (PrtObj){ox, oy, true};
                    map[oy][ox] = 'O'; break;
                }
            }
        }
    }

    for (int i = 0; i < MAX_COI; i++) coins[i].on = false;
    // Normal: base 8–17, +1 coin per round. Coin Rush: twice that many piles.
    int nc = 8 + GetRandomValue(0, 9) + (roundn - 1);
    if (nc < 1) nc = 1;
    if (coinRush) nc *= 2;
    if (nc > MAX_COI) nc = MAX_COI;
    for (int placed=0, t=0; placed<nc && t<1200; t++) {
        int cx = GetRandomValue(1,mw-2), cy = GetRandomValue(1,mh-2);
        if (map[cy][cx]=='.') {
            coins[placed++] = (CoinObj){cx, cy, true};
            map[cy][cx] = '$';
        }
    }

    for (int i = 0; i < MAX_ITM; i++) itms[i].on = false;
    int placed = 0;

    // Coin Rush: always one Gold Touch on the floor at/near the spawn.
    if (coinRush) {
        static const int off[8][2] = {
            {0,1},{1,0},{-1,0},{0,-1},{1,1},{-1,1},{1,-1},{-1,-1}
        };
        bool got = false;
        for (int d = 0; d < 8 && !got; d++) {
            int ix = px + off[d][0], iy = py + off[d][1];
            if (ix < 1 || iy < 1 || ix > mw - 2 || iy > mh - 2) continue;
            if (map[iy][ix] != '.') continue;
            itms[placed++] = (ItmObj){ix, iy, true, IK_GLD};
            map[iy][ix] = 'I';
            got = true;
        }
        // Fallback: put it on the spawn tile itself (player steps off & back to grab).
        if (!got) {
            itms[placed++] = (ItmObj){px, py, true, IK_GLD};
            map[py][px] = 'I';
        }
    }

    // Random floor loot (never includes Gold Touch). Leave room after the CR spawn item.
    int ni = coinRush ? (2 + GetRandomValue(0,1)) : (2 + GetRandomValue(0,1));
    if (placed + ni > MAX_ITM) ni = MAX_ITM - placed;
    for (int t = 0; placed < MAX_ITM && ni > 0 && t < 400; t++) {
        int ix = GetRandomValue(1,mw-2), iy = GetRandomValue(1,mh-2);
        if (map[iy][ix]=='.') {
            itms[placed++] = (ItmObj){ix, iy, true, (IKind)GetRandomValue(0, IK_GLD - 1)};
            map[iy][ix] = 'I';
            ni--;
        }
    }

    for (int i = 0; i < MAX_ENE; i++) enes[i].on = false;
    if (eneon) {
        int ne;
        if (coinRush) {
            ne = 6 + roundn; if (ne > MAX_ENE) ne = MAX_ENE;
        } else {
            ne = 1 + roundn/2; if (ne > MAX_NORMAL_ENE) ne = MAX_NORMAL_ENE;
        }
        int minDist = 7;
        for (int i=0, t=0; i<ne && t<800; t++) {
            int ex = GetRandomValue(1,mw-2), ey = GetRandomValue(1,mh-2);
            if (map[ey][ex]!='#' && abs(ex-px)+abs(ey-py) > minDist) {
                // Same chase tempo in Normal and Coin Rush
                float cd = 0.65f + GetRandomValue(0, 35)*0.01f;
                enes[i++] = (Ene){(float)ex,(float)ey, 0, cd, true};
            }
        }
    }
}

// ─── GENERATE ROUND: BOSS ARENA (every 10th round) ────────────────────────────
// Fully open arena — no interior walls, just the border. Nowhere to hide:
// dodging the dash and the fire breath is pure movement skill. Spike plates
// and cannons are the only fixtures; no treasure, no portals — the exit
// treasure only appears once the boss is dead (see BossDie).
static void GenBossArena(void) {
    mw = 27; mh = 19;

    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++)
        map[y][x] = (x==0||y==0||x==mw-1||y==mh-1) ? '#' : '.';

    // Spike plates: the player arms them by stepping on them ('^' -> '*');
    // only armed plates hurt the boss.
    static const int PLT[4][2] = { {13,5}, {13,13}, {4,9}, {22,9} };
    for (int p = 0; p < 4; p++)
        map[PLT[p][1]][PLT[p][0]] = '^';

    // Cannons: stand on one to fire a ball at the boss (see game_boss.c).
    map[mh/2][2]    = 'C';
    map[mh/2][mw-3] = 'C';

    px = mw/2; py = mh-3;       // player south, boss north (SpawnBoss)

    for (int i = 0; i < MAX_PRT; i++) prts[i].on = false;

    for (int i = 0; i < MAX_COI; i++) coins[i].on = false;
    int bossCoins = coinRush ? 36 : 6;
    if (bossCoins > MAX_COI) bossCoins = MAX_COI;
    for (int placed=0, t=0; placed<bossCoins && t<900; t++) {
        int cx = GetRandomValue(1,mw-2), cy = GetRandomValue(1,mh-2);
        if (map[cy][cx]=='.' && !(cx==px&&cy==py) && map[cy][cx]!='^' && map[cy][cx]!='C') {
            coins[placed++] = (CoinObj){cx, cy, true};
            map[cy][cx] = '$';
        }
    }

    // Three items, the first guaranteed to be a sword.
    for (int i = 0; i < MAX_ITM; i++) itms[i].on = false;
    for (int placed=0, t=0; placed<3 && t<400; t++) {
        int ix = GetRandomValue(1,mw-2), iy = GetRandomValue(2,mh-2);
        if (map[iy][ix]=='.' && abs(ix-px)+abs(iy-py) > 3) {
            IKind k = (placed == 0) ? IK_SWD : (IKind)GetRandomValue(0, IK_GLD - 1);
            itms[placed++] = (ItmObj){ix, iy, true, k};
            map[iy][ix] = 'I';
        }
    }

    // Regular enemies stay out of the arena; the boss summons his own adds.
    for (int i = 0; i < MAX_ENE; i++) enes[i].on = false;

    SpawnBoss();
}

// ─── CHALLENGE-ROOM HELPERS ───────────────────────────────────────────────────
// Scatter n coins onto random free floor tiles (clears any prior placement).
static void PlaceCoins(int n) {
    for (int i = 0; i < MAX_COI; i++) coins[i].on = false;
    for (int y = 1; y < mh-1; y++)
    for (int x = 1; x < mw-1; x++)
        if (map[y][x] == '$') map[y][x] = '.';
    if (n > MAX_COI) n = MAX_COI;
    for (int placed=0, t=0; placed<n && t<800; t++) {
        int cx = GetRandomValue(1,mw-2), cy = GetRandomValue(1,mh-2);
        if (map[cy][cx]=='.' && !(cx==px&&cy==py)) {
            coins[placed++] = (CoinObj){cx, cy, true};
            map[cy][cx] = '$';
        }
    }
}

// Scatter n items; kind0 forces the first item's kind (IK_NONE = random).
static void PlaceItems(int n, IKind kind0) {
    for (int i = 0; i < MAX_ITM; i++) itms[i].on = false;
    if (n > MAX_ITM) n = MAX_ITM;
    for (int placed=0, t=0; placed<n && t<600; t++) {
        int ix = GetRandomValue(1,mw-2), iy = GetRandomValue(1,mh-2);
        if (map[iy][ix]=='.' && abs(ix-px)+abs(iy-py) > 3) {
            IKind k = (placed == 0 && kind0 != IK_NONE)
                    ? kind0 : (IKind)GetRandomValue(0, IK_GLD - 1);
            itms[placed++] = (ItmObj){ix, iy, true, k};
            map[iy][ix] = 'I';
        }
    }
}

// ─── GENERATE ROUND: TRAP ROOM (no enemies) ───────────────────────────────────
// Looks like a normal room (scattered walls, current-progression size) but with
// no enemies. The danger is overlaid: plenty of static spike traps '+', static
// lava pools 'L' (they don't spread — that's the lava room's job), and horizontal
// arrow lanes 'r' fed by a dispenser 'D' at the far-left of each lane. Each lane
// fires a synchronized west->east volley; only the brief fire window is lethal,
// after a generous telegraph. Triple coins.
static void GenTrapRound(void) {
    // Same footprint as a normal room of the current round.
    mh = 13 + (roundn - 1); if (mh > 42) mh = 42;
    mw = 23 + 2 * (roundn - 1); if (mw > 76) mw = 76;

    // Organic scattered-wall base (density tracks normn).
    int tp = 10 + normn/2; if (tp > 26) tp = 26;
    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++) {
        if (x==0||y==0||x==mw-1||y==mh-1) { map[y][x]='#'; continue; }
        map[y][x] = (GetRandomValue(0,99) < tp) ? '#' : '.';
    }

    px = mw/2; py = mh-2;                         // south; treasure far north
    for (int dy=-1; dy<=1; dy++) for (int dx=-1; dx<=1; dx++)
        if (py+dy>0 && py+dy<mh-1 && px+dx>0 && px+dx<mw-1) map[py+dy][px+dx]='.';

    // Arrow lanes: a few full-width rows carved clear. A dispenser 'D' sits at
    // the far-left (x=1) and fires arrows east across the 'r' cells. Kept well
    // clear of the spawn row so you never start inside a volley.
    for (int y = 2; y <= mh-4; y += 3 + GetRandomValue(0,1)) {
        if (y >= py-1) break;
        map[y][1] = 'D';                                  // dispenser, far left
        for (int x = 2; x < mw-1; x++) map[y][x] = 'r';   // the lane it feeds
    }

    // Static spike traps on the remaining floor — a proper minefield.
    // Density grows with normn; Coin Rush starts denser (still leave corridors).
    int spikePct = coinRush ? (22 + normn/2) : (13 + normn/2);
    if (spikePct > 35) spikePct = 35;
    for (int y = 1; y < mh-1; y++)
    for (int x = 1; x < mw-1; x++)
        if (map[y][x]=='.' && GetRandomValue(0,99) < spikePct) map[y][x] = '+';

    // Trapdoors: floor-lookalike hatches that drop you into the void on step.
    // Sparse enough to weave around; denser in Coin Rush trap variants.
    int hatchPct = coinRush ? (9 + normn/4) : (6 + normn/4);
    if (hatchPct > 14) hatchPct = 14;
    for (int y = 1; y < mh-1; y++)
    for (int x = 1; x < mw-1; x++)
        if (map[y][x]=='.' && GetRandomValue(0,99) < hatchPct) map[y][x] = 'h';

    // Static lava pools — a handful of small blobs to weave around.
    int pools = 3 + normn/3;
    for (int p = 0, t = 0; p < pools && t < 300; t++) {
        int lx = GetRandomValue(2, mw-3), ly = GetRandomValue(2, mh-4);
        if (map[ly][lx] != '.') continue;
        int r = GetRandomValue(0,1);
        for (int dy = 0; dy <= r; dy++) for (int dx = 0; dx <= r; dx++) {
            int nx = lx+dx, ny = ly+dy;
            if (nx>0 && nx<mw-1 && ny>0 && ny<mh-1 && map[ny][nx]=='.') map[ny][nx]='L';
        }
        p++;
    }

    // Re-clear the spawn pocket so no overlay landed on top of the player.
    for (int dy=-1; dy<=1; dy++) for (int dx=-1; dx<=1; dx++)
        if (py+dy>0 && py+dy<mh-1 && px+dx>0 && px+dx<mw-1) map[py+dy][px+dx]='.';

    // Always leave a safe exit north to the first arrow lane (nothing substantial:
    // only clear walls/hazards on the spawn column between pocket and that line).
    {
        int firstLaneY = -1;
        for (int y = py - 1; y >= 2; y--) {
            for (int x = 1; x < mw - 1; x++)
                if (map[y][x] == 'r' || map[y][x] == 'D') { firstLaneY = y; break; }
            if (firstLaneY >= 0) break;
        }
        // Path tiles: just south of the first lane, up through spawn.
        int yStop = (firstLaneY >= 0) ? (firstLaneY + 1) : 2;
        if (yStop < 1) yStop = 1;
        for (int y = py; y >= yStop; y--) {
            char c = map[y][px];
            if (c == '#' || c == '+' || c == 'L' || c == 'h')
                map[y][px] = '.';
        }
    }

    for (int t = 0; ; t++) {
        int tx = GetRandomValue(1,mw-2), ty = GetRandomValue(1,2);
        if (map[ty][tx]=='.') { map[ty][tx]='T'; break; }
        if (t > 300) { map[1][mw/2] = 'T'; break; }
    }

    // No portals in a trap room — you don't get to skip the gauntlet.
    for (int i = 0; i < MAX_PRT; i++) prts[i].on = false;

    PlaceCoins(coinRush ? (50 + GetRandomValue(0, 16) + roundn)
                        : (24 + GetRandomValue(0, 8)));
    PlaceItems(2, IK_NONE);
    for (int i = 0; i < MAX_ENE; i++) enes[i].on = false;
}

// ─── GENERATE ROUND: DARK ROOM ────────────────────────────────────────────────
// Same maze + enemy count as Normal, under fog (DrawDarkness). Coins use the
// Normal formula with base 20 instead of 8: 20 + 0..9 + (roundn-1).
static void GenDarkRound(void) {
    GenNormalRound();
    // Enemies left as GenNormalRound placed them (same as Normal).
    int nc = 20 + GetRandomValue(0, 9) + (roundn - 1);
    if (nc < 1) nc = 1;
    if (nc > MAX_COI) nc = MAX_COI;
    PlaceCoins(nc);
}

// ─── GENERATE ROUND: THE FLOOR IS LAVA (no enemies) ───────────────────────────
// Looks like a normal room (scattered walls, current-progression size), no
// enemies — but the lava floods up from the south each tick (UpdLava). It
// converts every adjacent floor/trap/coin/item tile; walls block it, so the
// scattered walls form brief safe pockets on the way north. Spawn south, treasure
// north, a light spike scatter, and a guaranteed speed item to outrun the front.
static void GenLavaRound(void) {
    mh = 13 + (roundn - 1); if (mh > 42) mh = 42;
    mw = 23 + 2 * (roundn - 1); if (mw > 76) mw = 76;

    int tp = 10 + normn/2; if (tp > 24) tp = 24;
    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++) {
        if (x==0||y==0||x==mw-1||y==mh-1) { map[y][x]='#'; continue; }
        map[y][x] = (GetRandomValue(0,99) < tp) ? '#' : '.';
    }

    // Light spike scatter so the flee north isn't a straight sprint.
    int lavaSpikes = coinRush ? (9 + normn/3) : (4 + normn/3);
    if (lavaSpikes > 18) lavaSpikes = 18;
    for (int y = 1; y < mh-1; y++)
    for (int x = 1; x < mw-1; x++)
        if (map[y][x]=='.' && GetRandomValue(0,99) < lavaSpikes) map[y][x] = '+';

    px = mw/2; py = mh-3;
    for (int dy=-1; dy<=1; dy++) for (int dx=-1; dx<=1; dx++)
        if (py+dy>0 && py+dy<mh-1 && px+dx>0 && px+dx<mw-1) map[py+dy][px+dx] = '.';

    // Lava seeds: full south row + the lowest stretch of both flanks. The front
    // creeps up from here (see UpdLava). Seeded AFTER the pocket clear so the
    // south row below the spawn is already hot.
    for (int x = 1; x < mw-1; x++) if (map[mh-2][x] != '#') map[mh-2][x] = 'L';
    for (int k = 0; k < 3; k++) {
        if (map[mh-3-k][1]    == '.') map[mh-3-k][1]    = 'L';
        if (map[mh-3-k][mw-2] == '.') map[mh-3-k][mw-2] = 'L';
    }
    lavaTick = 1.2f - 0.03f * (roundn/10);
    if (lavaTick < 0.85f) lavaTick = 0.85f;

    for (int t = 0; ; t++) {
        int tx = GetRandomValue(1,mw-2), ty = GetRandomValue(1,2);
        if (map[ty][tx]=='.') { map[ty][tx]='T'; break; }
        if (t > 300) { map[1][mw/2] = 'T'; break; }
    }
    // No portals — the only way out is the run north to the treasure.
    for (int i = 0; i < MAX_PRT; i++) prts[i].on = false;

    PlaceCoins(coinRush ? (50 + GetRandomValue(0, 16) + roundn)
                        : (24 + GetRandomValue(0, 8)));
    PlaceItems(2, IK_SPD);                        // speed is the key item here
    for (int i = 0; i < MAX_ENE; i++) enes[i].on = false;
}

// ─── REACHABILITY GUARANTEE ───────────────────────────────────────────────────
// After generation, make sure the treasure (and, in a lava room, the guaranteed
// speed item) can actually be reached from the spawn. BFS over everything that
// isn't a wall/dispenser — traps and lava are hazards, not topology. Any
// walled-in target gets an L-shaped corridor carved to it: rare, allowed to be
// ugly, because the alternative is a soft-lock (a guaranteed death in a lava
// room, where the advancing front punishes a dead end with no way through).
static void EnsureReachable(void) {
    static bool seen[MAP_ROWS][MAP_COLS];
    static int  qx[MAP_ROWS*MAP_COLS], qy[MAP_ROWS*MAP_COLS];
    memset(seen, 0, sizeof(seen));
    int qn = 0, qh = 0;
    qx[qn] = px; qy[qn] = py; qn++; seen[py][px] = true;
    while (qh < qn) {
        int x = qx[qh], y = qy[qh]; qh++;
        static const int D4[4][2] = { {1,0},{-1,0},{0,1},{0,-1} };
        for (int d = 0; d < 4; d++) {
            int nx = x + D4[d][0], ny = y + D4[d][1];
            if (nx < 1 || ny < 1 || nx > mw-2 || ny > mh-2) continue;
            if (seen[ny][nx] || map[ny][nx] == '#' || map[ny][nx] == 'D') continue;
            seen[ny][nx] = true;
            qx[qn] = nx; qy[qn] = ny; qn++;
        }
    }

    for (int ty = 1; ty < mh-1; ty++)
    for (int tx = 1; tx < mw-1; tx++) {
        bool target = (map[ty][tx] == 'T') ||
                      (roomKind == RK_LAVA && itms[0].on &&
                       itms[0].x == tx && itms[0].y == ty);
        if (!target || seen[ty][tx]) continue;
        // Carve an L: down the spawn's column, then along the target's row.
        // Only walls fall — hazards/coins on the path stay as they are.
        int step = (ty > py) ? 1 : -1;
        for (int y = py; y != ty; y += step)
            if (map[y][px] == '#' || map[y][px] == 'D') map[y][px] = '.';
        step = (tx > px) ? 1 : -1;
        for (int x = px; x != tx; x += step)
            if (map[ty][x] == '#' || map[ty][x] == 'D') map[ty][x] = '.';
    }
}

// ─── GENERATE ROUND ───────────────────────────────────────────────────────────
void GenRound(void) {
    StatsInc(&global_rounds);

    // forceRoom is one-shot: consumed for this floor only.
    if (forceRoom >= 0) {
        roomKind = (RoomKind)forceRoom;
        forceRoom = -1;
    } else if (roundn % 10 == 0) {
        roomKind = RK_BOSS;
    } else if (roundn >= 6) {
        // 5% trap, 5% dark, 5% lava, 5% coin rush — rest normal.
        int r = GetRandomValue(0, 99);
        if      (r < 5)  roomKind = RK_TRAP;
        else if (r < 10) roomKind = RK_DARK;
        else if (r < 15) roomKind = RK_LAVA;
        else if (r < 20) roomKind = RK_COIN_RUSH;
        else             roomKind = RK_NORMAL;
    } else {
        roomKind = RK_NORMAL;
    }

    // Gameplay flag used by spawns / coin value / HUD (true only on RK_COIN_RUSH).
    coinRush = (roomKind == RK_COIN_RUSH);

    // Gold Touch is Coin-Rush only — strip any carried GLD outside that floor.
    if (!coinRush) {
        int w = 0;
        for (int i = 0; i < pinvn; i++)
            if (pinv[i] != IK_GLD) pinv[w++] = pinv[i];
        pinvn = w;
        pgold = 0;   // midas form ends when you leave Coin Rush
    }

    boss.on   = false;
    trapPhase = 0; lavaTimer = 0; lavaElapsed = 0;
    bannerT   = (roomKind == RK_NORMAL) ? 0 : 2.6f;

    switch (roomKind) {
        case RK_BOSS:      GenBossArena();   break;
        case RK_TRAP:      GenTrapRound();   break;
        case RK_DARK:      GenDarkRound();   break;
        case RK_LAVA:      GenLavaRound();   break;
        case RK_COIN_RUSH: GenNormalRound(); break;  // same generator, denser via coinRush
        default:           GenNormalRound(); break;
    }

    EnsureReachable();   // no walled-in treasure, no soft-locks

    UpdateCameraOffset();
    cam.target   = (Vector2){WX(px), WY(py)};
    cam.zoom     = 1.0f;
    cam.rotation = 0;

    pdead  = false; pmt = 0; pmd = 0.13f;
    pspd   = 0;
    pwlk   = 0;
    pgold  = 0;
    pfall  = 0;
    pshield = false; pshield_dur = 0;
    if (roundn == 1) pshield_perk = (perk_shield2 > 0);
    sttime = 0;
    memset(pars, 0, sizeof(pars));
}

// ─── DRAW TILE ────────────────────────────────────────────────────────────────
void DrawTile(int x, int y, float t) {
    int wx = x*CELL, wy = y*CELL;
    char c = map[y][x];
    char dc = c;

    DrawRectangle(wx, wy, CELL, CELL, CB);

    if (dc=='#') {
        DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, CW);
        DrawRectangle(wx+1, wy+1, CELL-2, 3, CWH);
        DrawRectangle(wx+1, wy+1, 3, CELL-2, CWH);
        DrawRectangle(wx+1, wy+CELL-4, CELL-2, 3, CWD);
        DrawRectangle(wx+CELL-4, wy+1, 3, CELL-2, CWD);
    } else {
        DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, CF);

        if (dc=='+') {
            DrawRectangle(wx + 3, wy + 3, CELL - 6, CELL - 6, (Color){20, 15, 25, 255});
            DrawRectangleLines(wx + 3, wy + 3, CELL - 6, CELL - 6, (Color){60, 50, 70, 255});
            for(int i = 0; i < 3; i++) {
                for(int j = 0; j < 3; j++) {
                    int spx = wx + 8 + i * 8;
                    int spy = wy + 8 + j * 8;
                    DrawCircle(spx, spy, 2, (Color){190, 190, 210, 255});
                    DrawRectangle(spx - 1, spy - 2, 2, 2, CTRAP);
                }
            }

        } else if (dc=='h') {
            // Closed trapdoor: still floor-like, but a notch cooler/darker than
            // CF {28,24,40} so careful eyes catch it — not a loud trap tile.
            DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, (Color){20, 19, 32, 255});
            // faint mid seam
            DrawLine(wx+5, wy+CELL/2, wx+CELL-5, wy+CELL/2, (Color){14, 12, 22, 100});
            // soft corner ticks
            DrawPixel(wx+4, wy+4, (Color){42, 38, 56, 150});
            DrawPixel(wx+CELL-5, wy+4, (Color){42, 38, 56, 150});
            DrawPixel(wx+4, wy+CELL-5, (Color){42, 38, 56, 130});
            DrawPixel(wx+CELL-5, wy+CELL-5, (Color){42, 38, 56, 130});
            // whisper of a rim (still quiet)
            DrawRectangleLines(wx+2, wy+2, CELL-4, CELL-4, (Color){16, 14, 26, 55});

        } else if (dc=='H') {
            // Open hatch — black pit after the player steps on it.
            DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, (Color){8, 6, 12, 255});
            DrawRectangle(wx+4, wy+4, CELL-8, CELL-8, (Color){2, 1, 4, 255});
            DrawRectangleLines(wx+3, wy+3, CELL-6, CELL-6, (Color){55, 48, 70, 180});
            float p = 0.5f + 0.5f*sinf(t*10.0f + x + y);
            DrawCircle(wx+CELL/2, wy+CELL/2, (int)(5+2*p),
                       (Color){20, 12, 30, (uchar)(40+40*p)});

        } else if (dc=='r') {
            // Arrow-sweep lane (fed west->east by the 'D' dispenser). Only the
            // brief fire window is lethal, and the warning escalates in two
            // unmistakable stages: first a big pulsing "!" on every lane tile
            // (get out NOW), then the red arrows — the things that kill.
            // Idle: faint chevrons so the lane itself stays readable.
            float cyc       = fmodf(trapPhase, ARROW_PERIOD);
            float fireStart = ARROW_PERIOD - ARROW_FIRE;
            float warnStart = fireStart - ARROW_WARN;
            bool  lethal    = cyc >= fireStart;
            bool  warn      = cyc >= warnStart && cyc < fireStart;

            if (warn) {
                // Stage 1: "!" — loud, yellow, impossible to miss.
                float wp = 0.5f + 0.5f*sinf(t*16.0f);
                DrawRectangle(wx+2, wy+2, CELL-4, CELL-4,
                              (Color){235, 200, 60, (uchar)(14 + 20*wp)});
                const char *ex = "!";
                int fs = 22;
                DrawText(ex, wx + CELL/2 - MeasureText(ex, fs)/2, wy + CELL/2 - fs/2,
                         fs, (Color){255, 220, 60, (uchar)(190 + 65*wp)});
            } else {
                // Idle: faint chevrons. Fire: the red killers, with a bright
                // head racing east along the lane.
                Color ac = lethal ? (Color){255, 80, 60, 255}
                                  : (Color){95, 85, 78, 140};
                if (lethal) {
                    float head = (cyc - fireStart) / ARROW_FIRE;   // west (0) -> east (1)
                    float fx   = (mw > 2) ? (float)(x - 1) / (float)(mw - 2) : 0.0f;
                    if (fabsf(fx - head) < 0.10f) ac = (Color){255, 235, 180, 255};
                }
                int my = wy + CELL/2;
                for (int a = 0; a < 2; a++) {   // chevrons always point east
                    int ox = wx + 6 + a*12;
                    DrawTriangle((Vector2){ox, my-5}, (Vector2){ox+7, my}, (Vector2){ox, my+5}, ac);
                }
                if (lethal) {
                    float p = 0.5f + 0.5f*sinf(t*12.0f);
                    DrawCircle(wx+CELL/2, wy+CELL/2, (int)(10+3*p),
                               (Color){255, 80, 60, (uchar)(16+22*p)});
                }
            }

        } else if (dc=='D') {
            // Arrow dispenser: a wall-set device at the far-left of a lane. Dim
            // while idle, charges amber during the telegraph, muzzle-flashes as
            // it looses the volley — so you always know a sweep is coming.
            float cyc       = fmodf(trapPhase, ARROW_PERIOD);
            float fireStart = ARROW_PERIOD - ARROW_FIRE;
            float warnStart = fireStart - ARROW_WARN;
            bool  lethal    = cyc >= fireStart;
            bool  warn      = cyc >= warnStart && cyc < fireStart;
            float wp        = warn ? (cyc - warnStart) / ARROW_WARN : 0.0f;   // 0->1 charge

            // Housing.
            DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, (Color){40, 34, 30, 255});
            DrawRectangleLines(wx+1, wy+1, CELL-2, CELL-2, CCANN);
            // Muzzle pointing east.
            Color mc = lethal ? (Color){255, 235, 180, 255}
                     : warn   ? (Color){235, (uchar)(150+80*wp), 60, 255}
                     :          (Color){120, 95, 70, 255};
            DrawRectangle(wx+CELL-12, wy+CELL/2-4, 10, 8, mc);
            DrawTriangle((Vector2){wx+CELL-3, wy+CELL/2-6},
                         (Vector2){wx+CELL+1, wy+CELL/2},
                         (Vector2){wx+CELL-3, wy+CELL/2+6}, mc);
            if (lethal) {
                float p = 0.5f + 0.5f*sinf(t*20.0f);
                DrawCircle(wx+CELL-2, wy+CELL/2, (int)(6+4*p),
                           (Color){255, 210, 120, (uchar)(120+80*p)});
            } else if (warn) {
                DrawCircle(wx+CELL/2, wy+CELL/2, (int)(4+4*wp),
                           (Color){235, 150, 60, (uchar)(60+120*wp)});
            }

        } else if (dc=='L') {
            // Lava: hot, animated, lethal.
            float p  = 0.5f + 0.5f*sinf(t*2.2f + x*0.9f + y*1.3f);
            DrawRectangle(wx+1, wy+1, CELL-2, CELL-2, (Color){185, 55, 12, 255});
            DrawRectangle(wx+3, wy+3, CELL-6, CELL-6,
                          (Color){235, (uchar)(90+50*p), 22, 255});
            DrawCircle(wx+CELL/2 + (int)(6*sinf(t*1.1f + x)),
                       wy+CELL/2 + (int)(5*cosf(t*1.4f + y)),
                       5, (Color){255, (uchar)(175+60*p), 60, (uchar)(130+80*p)});

        } else if (dc=='^' || dc=='*') {
            // Spike plate (boss rounds): '^' dormant until the player steps on
            // it, '*' armed and glowing — the boss takes damage on it.
            bool armed = (dc=='*');
            Color base = armed ? (Color){16, 30, 28, 255}  : (Color){22, 22, 30, 255};
            Color rim  = armed ? (Color){60, 120, 105, 255} : (Color){55, 55, 70, 255};
            Color spk  = armed ? CPLATE : (Color){95, 100, 115, 255};
            DrawRectangle(wx + 3, wy + 3, CELL - 6, CELL - 6, base);
            DrawRectangleLines(wx + 3, wy + 3, CELL - 6, CELL - 6, rim);
            for (int i = 0; i < 3; i++) {
                float sx0 = wx + 8 + i * 8;
                DrawTriangle((Vector2){sx0, wy + 9},
                             (Vector2){sx0 + 3, wy + CELL - 9},
                             (Vector2){sx0 - 3, wy + CELL - 9}, spk);
            }
            if (armed) {
                float p = 0.5f + 0.5f*sinf(t * 2.5f + x*0.6f + y*0.4f);
                DrawCircle(wx + CELL/2, wy + CELL/2, (int)(11 + 3*p),
                           (Color){CPLATE.r, CPLATE.g, CPLATE.b, (uchar)(16 + 22*p)});
            }

        } else if (dc=='C') {
            // Cannon base pad — heavily highlighted floor tile; the barrel,
            // reload ring and countdown are drawn on top by game_boss.c.
            float p = 0.5f + 0.5f*sinf(t * 2.0f + x*0.4f + y*0.4f);
            DrawCircle(wx+CELL/2, wy+CELL/2, (int)(16+3*p),
                       (Color){CCANN.r, CCANN.g, CCANN.b, (uchar)(30+18*p)});
            DrawRectangle(wx + 2, wy + 2, CELL - 4, CELL - 4, (Color){46, 36, 24, 255});
            DrawRectangleLines(wx + 1, wy + 1, CELL - 2, CELL - 2, CCANN);
            DrawRectangleLines(wx + 2, wy + 2, CELL - 4, CELL - 4, (Color){80, 70, 62, 255});
            DrawCircleLines(wx + CELL/2, wy + CELL/2, 13, (Color){CCANN.r, CCANN.g, CCANN.b, 220});

        } else if (dc=='T') {
            float p = 0.5f + 0.5f*sinf(t * 3.0f);
            Color tc = CTRES; tc.a = (uchar)(200 + 55*p);
            DrawRectangle(wx+6,  wy+9,  CELL-12, CELL-14, tc);
            DrawRectangle(wx+6,  wy+9,  CELL-12, 4, (Color){255,240,180,230});
            DrawCircle(wx+CELL/2, wy+CELL/2+2, 3, (Color){200,150,30,255});
            DrawCircle(wx+CELL/2, wy+CELL/2,
                       (int)(12 + 5*p), (Color){255,200,50,(int)(18+20*p)});

        } else if (dc=='$') {
            float p = sinf(t * 4.0f + x*0.5f + y*0.7f);
            int r = (int)(5 + p*1.5f);
            // Coin Rush: two coins side-by-side (visual = double value).
            if (coinRush) {
                int ox = 5;
                for (int s = -1; s <= 1; s += 2) {
                    int cx = wx + CELL/2 + s * ox;
                    int cy = wy + CELL/2;
                    DrawCircle(cx+1, cy+2, r - 1, (Color){160,120,0,80});
                    DrawCircle(cx,   cy,   r - 1, CCOIN);
                    DrawCircle(cx-1, cy-1, (int)((r-1)*0.4f)+1, (Color){255,245,160,200});
                }
            } else {
                DrawCircle(wx+CELL/2+1, wy+CELL/2+2, r, (Color){160,120,0,80});
                DrawCircle(wx+CELL/2,   wy+CELL/2,   r, CCOIN);
                DrawCircle(wx+CELL/2-1, wy+CELL/2-1,
                           (int)(r*0.4f)+1, (Color){255,245,160,200});
            }

        } else if (dc=='I') {
            IKind k = IK_NONE;
            for (int i=0;i<MAX_ITM;i++)
                if (itms[i].on&&itms[i].x==x&&itms[i].y==y) { k=itms[i].k; break; }
            if (k != IK_NONE) {
                Color ic = IK_COL[k];
                float p = 0.5f + 0.5f*sinf(t*3.0f + (float)(x+y)*0.3f);
                DrawTriangle(
                    (Vector2){wx+CELL/2, wy+4},
                    (Vector2){wx+CELL-4, wy+CELL/2},
                    (Vector2){wx+CELL/2, wy+CELL-4}, ic);
                DrawTriangle(
                    (Vector2){wx+CELL/2, wy+4},
                    (Vector2){wx+CELL/2, wy+CELL-4},
                    (Vector2){wx+4,      wy+CELL/2}, ic);
                Color gc = ic; gc.a = (uchar)(55 * p);
                DrawCircle(wx+CELL/2, wy+CELL/2, (int)(14+3*p), gc);
            }

        } else if (dc=='O') {
            float p = 0.5f + 0.5f*sinf(t*2.0f + (float)(x+y)*0.2f);
            DrawCircle(wx+CELL/2, wy+CELL/2,
                       (int)(12+5*p), (Color){80,160,255,(int)(65+50*p)});
            DrawCircle(wx+CELL/2, wy+CELL/2,
                       (int)(7+2*p),  (Color){160,210,255,(int)(140+60*p)});
            DrawCircleLines(wx+CELL/2, wy+CELL/2,
                            (int)(12+5*p), (Color){120,190,255,200});
        }
    }
}

// ─── DRAW MAP ─────────────────────────────────────────────────────────────────
void DrawMap(float t) {
    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++)
        DrawTile(x, y, t);
}

// ─── ARROW LANES (RK_TRAP) ────────────────────────────────────────────────────
// Every ARROW_PERIOD seconds the lanes ('r') fire one synchronized volley that
// is lethal for the last ARROW_FIRE seconds of the cycle. Safe the rest of the
// time — cross between volleys.
bool ArrowLaneLethal(void) {
    float cyc = fmodf(trapPhase, ARROW_PERIOD);
    return cyc >= (ARROW_PERIOD - ARROW_FIRE);
}

// ─── LAVA SPREAD (RK_LAVA) ────────────────────────────────────────────────────
// The room starts at its existing lavaTick rate, then gains spread speed
// linearly over active play time. Walls block the flow. Reaching the player
// kills (shield saves once and keeps that tile dry for a moment — but the front
// keeps coming).
void UpdLava(float dt) {
    if (roomKind != RK_LAVA || gs != GS_PLAY) return;
    lavaElapsed += dt;
    float spreadRate = 1.0f/lavaTick + LAVA_ACCEL_RATE*lavaElapsed;
    float currentTick = 1.0f/spreadRate;
    if (currentTick < LAVA_MIN_TICK) currentTick = LAVA_MIN_TICK;
    lavaTimer += dt;
    if (lavaTimer < currentTick) return;
    lavaTimer -= currentTick;

    static int qx[MAP_ROWS*MAP_COLS], qy[MAP_ROWS*MAP_COLS];
    int qn = 0;
    for (int y = 1; y < mh-1; y++)
    for (int x = 1; x < mw-1; x++) {
        char c = map[y][x];
        if (c!='.' && c!='+' && c!='$' && c!='I') continue;
        if (map[y-1][x]=='L' || map[y+1][x]=='L' ||
            map[y][x-1]=='L' || map[y][x+1]=='L') {
            qx[qn] = x; qy[qn] = y; qn++;
        }
    }

    for (int i = 0; i < qn; i++) {
        int x = qx[i], y = qy[i];
        char c = map[y][x];
        if (c == '$') for (int k=0;k<MAX_COI;k++)
            if (coins[k].on && coins[k].x==x && coins[k].y==y) { coins[k].on=false; break; }
        if (c == 'I') for (int k=0;k<MAX_ITM;k++)
            if (itms[k].on && itms[k].x==x && itms[k].y==y) { itms[k].on=false; break; }

        if (x == px && y == py) {
            if (pshield || pshield_perk) {
                if (pshield) pshield = false;
                else pshield_perk = false;
                map[y][x] = '.';                 // the shield keeps this tile dry
                Burst(WX(x), WY(y), CSHD, 22, 4);
                PlaySfx(SFX_SHIELD_TRIGGER);
                continue;
            }
            Burst(WX(x), WY(y), CFIRE, 32, 5);
            PlaySfx(SFX_DEATH_TRAP);
            StatsInc(&global_deaths_lava);
            pdead = true; gs = GS_DEAD; sttime = 0;
            UpdateAndSaveStats();
        }
        map[y][x] = 'L';
        if ((i & 3) == 0) Burst(WX(x), WY(y), CFIRE, 4, 2);   // sparse sparks
    }
}

// ─── DARKNESS OVERLAY (RK_DARK) ───────────────────────────────────────────────
// Drawn in world space after map/entities/player: full sight within
// DARK_LIGHT_R tiles, dusk out to DARK_DIM_R, near-black beyond. Treasure and
// portals glow through; coins leave a faint glint; enemies beyond the light
// betray themselves only by their eyes.
void DrawDarkness(void) {
    for (int y = 0; y < mh; y++)
    for (int x = 0; x < mw; x++) {
        float dx = (float)(x - px), dy = (float)(y - py);
        float d  = sqrtf(dx*dx + dy*dy);
        int a;
        if      (d <= DARK_LIGHT_R) a = 0;
        else if (d <  DARK_DIM_R)   a = (int)((d - DARK_LIGHT_R) / (DARK_DIM_R - DARK_LIGHT_R) * 235);
        else                        a = 247;
        if (a > 0) DrawRectangle(x*CELL, y*CELL, CELL, CELL, (Color){0,0,0,(uchar)a});

        char c = map[y][x];
        if (c == 'T') {
            float p = 0.5f + 0.5f*sinf((float)GetTime()*3.0f);
            DrawCircle(x*CELL+CELL/2, y*CELL+CELL/2, (int)(14+5*p),
                       (Color){CTRES.r, CTRES.g, CTRES.b, (uchar)(40+40*p)});
            DrawCircle(x*CELL+CELL/2, y*CELL+CELL/2, 4, CTRES);
        } else if (c == 'O') {
            float p = 0.5f + 0.5f*sinf((float)GetTime()*2.0f + x);
            DrawCircle(x*CELL+CELL/2, y*CELL+CELL/2, (int)(12+4*p),
                       (Color){CPORT.r, CPORT.g, CPORT.b, (uchar)(45+45*p)});
            DrawCircle(x*CELL+CELL/2, y*CELL+CELL/2, 3, (Color){160,210,255,220});
        } else if (c == '$' && a > 120) {
            DrawCircle(x*CELL+CELL/2, y*CELL+CELL/2, 2, (Color){255,215,0,80});
        }
    }

    // Glowing eyes: the only warning of an enemy outside the light.
    if (eneon) {
        for (int i = 0; i < MAX_ENE; i++) {
            if (!enes[i].on) continue;
            int ex = (int)(enes[i].x+0.5f), ey = (int)(enes[i].y+0.5f);
            float dd = sqrtf((float)((ex-px)*(ex-px) + (ey-py)*(ey-py)));
            if (dd > DARK_DIM_R - 1.0f && dd <= 9.0f) {
                DrawCircle(ex*CELL+CELL/2-4, ey*CELL+CELL/2-2, 2, (Color){255,60,60,220});
                DrawCircle(ex*CELL+CELL/2+4, ey*CELL+CELL/2-2, 2, (Color){255,60,60,220});
            }
        }
    }
}
