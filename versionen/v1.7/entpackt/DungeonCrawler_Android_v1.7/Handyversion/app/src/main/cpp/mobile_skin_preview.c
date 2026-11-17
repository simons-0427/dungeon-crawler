#include "mobile_skin_preview.h"
#include "game/skins.h"
#include "game/constants.h"
#include "rlgl.h"
#include <math.h>

#include "mobile_skin_bounds.h"

// Use one composition and radius for every rarity. Gameplay artwork is intact.
static void DrawArtwork(int id,float cx,float cy,float t) {
    const float r=32.0f;
    Color c=GetSkinColor(id);
    if(id>=49){DrawSelectedFixSkin(id,cx,cy,r);return;}
    if(id>=43){DrawAcceptedSkin(id,cx,cy,r,t);return;}
    switch(id){
        case 28: DrawRareBack(id,cx,cy,r,t);DrawSkull(cx,cy,r,t);return;
        case 33: DrawApexBladelord(cx,cy,r,t);return;
        case 34: DrawConstellation(cx,cy,r,t);return;
        case 36: DrawPoptartCat(cx,cy,r,t);return;
        case 37: DrawApexRex(cx,cy,r,t);return;
        case 38: DrawSkullvoidCrowned(cx,cy,r,t);return;
        case 39: DrawBloodscaleVanguard(cx,cy,r,t);return;
        case 40: DrawStormforgedAegis(cx,cy,r,t);return;
        case 41: DrawSunforgedCrusader(cx,cy,r,t);return;
        case 42: DrawBlackHole(cx,cy,r,t);return;
    }
    bool epic=id==27||id==29||id==32;
    bool rare=(id>=22&&id<=26)||id==30||id==31||id==35;
    if(epic)DrawEpicBack(id,cx,cy,r,t);
    if(rare)DrawRareBack(id,cx,cy,r,t);
    DrawCircleV((Vector2){cx,cy},r,c);
    if(epic)DrawEpicFace(id,cx,cy,r,t);
    else {
        DrawCircleV((Vector2){cx-5,cy-7},r/2,
                     (Color){CLAMP255(c.r+55),CLAMP255(c.g+55),CLAMP255(c.b+55),120});
        if(rare)DrawRareFace(id,cx,cy,r,t);
        else {
            DrawCircleV((Vector2){cx-9,cy-5},4,(Color){10,10,22,255});
            DrawCircleV((Vector2){cx+9,cy-5},4,(Color){10,10,22,255});
        }
    }
}

void MobileDrawSkinPreview(int id,float cx,float cy,float side,float time){
    if(id<0||id>=SKIN_COUNT)return;
    // Offscreen cards do not need animation draw calls.
    if(cy+side<GetScreenHeight()*0.0f||cy-side>GetScreenHeight())return;
    Rectangle b=mobileSkinBounds[id];
    float scale=side/fmaxf(b.width,b.height);
    rlPushMatrix();
    rlTranslatef(cx,cy,0);
    rlScalef(scale,scale,1);
    rlTranslatef(-b.x-b.width/2,-b.y-b.height/2,0);
    DrawArtwork(id,0,0,time);
    rlPopMatrix();
}
