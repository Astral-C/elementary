#include <cstring>
#include <maxmod.h>

#include "gamestate.hpp"
#include "menubg.png.h"
#include "menumid.png.h"
#include "menufg.png.h"
#include "menutitle.png.h"
#include "newgame.png.h"
#include "loadgame.png.h"
#include "cursor.png.h"
#include "soundbank.h"

#include "mainmenu.hpp"
#include "save.hpp"

#include "mgba.h"
#include "tonc_core.h"
#include "tonc_input.h"
#include "tonc_math.h"
#include "tonc_memdef.h"
#include "tonc_memmap.h"
#include "tonc_oam.h"
#include "tonc_types.h"
#include "tonc_video.h"
#include "map.hpp"

namespace MainMenu {
    enum OBJ {
        CURSOR,
        BTN_SHADOW,
        NEW_GAME,
        LOAD_GAME,
        MENU_OBJ_MAX
    };
    
    OBJ_ATTR mSprites[128] {};
    u16 mCursorPositions[2][2]{
        { 94, 104 },
        { 94, 124 },
    };
    u16 mShadowPositions[2]{
        102,
        122
    };
    u16 mCursorIndex { 0 };
    u16 mButtonTileStart[2]{ 2, 10 };
    u32 mTime { 0 };
    s8 mFadeAlpha { 32 };
    
    void Init(){
        oam_init(mSprites, 128);
        REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0 | DCNT_BG1 | DCNT_BG2 | DCNT_BG3;

        REG_BG0CNT = BG_BUILD(0, 27, BG_REG_32x32, BG_4BPP, 1, 0, 0);
        REG_BG1CNT = BG_BUILD(1, 28, BG_REG_32x32, BG_4BPP, 1, 0, 0);
        REG_BG2CNT = BG_BUILD(2, 29, BG_REG_32x32, BG_4BPP, 2, 0, 0);
        REG_BG3CNT = BG_BUILD(3, 30, BG_REG_32x32, BG_4BPP, 3, 0, 0);
        
        // init menu graphics
        memcpy32(&tile_mem_obj[0][1], cursor_pngTiles, cursor_pngTilesLen / 4);
        memcpy32(&tile_mem_obj[0][2], newgame_pngTiles, newgame_pngTilesLen / 4);
        memcpy32(&tile_mem_obj[0][10], loadgame_pngTiles, loadgame_pngTilesLen / 4);

        memcpy32(&se_mem[27][0], menutitle_pngMap, menutitle_pngMapLen / 4);
        memcpy32(&se_mem[28][0], menufg_pngMap, menufg_pngMapLen / 4);
        memcpy32(&se_mem[29][0], menumid_pngMap, menumid_pngMapLen / 4);
        memcpy32(&se_mem[30][0], menubg_pngMap, menubg_pngMapLen / 4);
        memcpy32(&tile_mem[0][0], menutitle_pngTiles, menutitle_pngTilesLen / 4);
        memcpy32(&tile_mem[1][0], menufg_pngTiles, menufg_pngTilesLen / 4);
        memcpy32(&tile_mem[2][0], menumid_pngTiles, menumid_pngTilesLen / 4);
        memcpy32(&tile_mem[3][0], menubg_pngTiles, menubg_pngTilesLen / 4);

        memcpy32(&pal_bg_bank[1][0], menubg_pngPal, menubg_pngPalLen / 4);

        memcpy32(&pal_obj_bank[0][1], newgame_pngPal, newgame_pngPalLen / 4);
        memcpy32(&pal_obj_bank[1][1], newgame_pngPal, newgame_pngPalLen / 4);

        memset16(&pal_obj_bank[12], CLR_BLACK, 16);
        memset16(&pal_obj_bank[13], CLR_BLACK, 16);
        
        obj_set_attr(&mSprites[CURSOR], ATTR0_4BPP | ATTR0_SQUARE, ATTR1_SIZE_8, ATTR2_BUILD(1, 0, 0));
        obj_set_attr(&mSprites[BTN_SHADOW], ATTR0_4BPP | ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_BUILD(2, 13, 1));
        obj_set_attr(&mSprites[NEW_GAME], ATTR0_4BPP | ATTR0_WIDE, ATTR1_SIZE_32x16, ATTR2_BUILD(2, 0, 0));
        obj_set_attr(&mSprites[LOAD_GAME], ATTR0_4BPP | ATTR0_WIDE | (SaveManager::HasSave() ? 0 : ATTR0_HIDE), ATTR1_SIZE_32x16, ATTR2_BUILD(10, 0, 0));
        
        obj_set_pos(&mSprites[CURSOR], mCursorPositions[0][0], mCursorPositions[0][1]);
        obj_set_pos(&mSprites[BTN_SHADOW], 105, mShadowPositions[mCursorIndex]);
        obj_set_pos(&mSprites[NEW_GAME], 104, 100);
        obj_set_pos(&mSprites[LOAD_GAME], 104, 120);

        mmSetModuleVolume(128);
        mmSetEffectsVolume(1024);
        mmStart(MOD_RASPBERRY_JAM, MM_PLAY_LOOP);
    }
    
    void Update(){
        key_poll();
        
        if(mFadeAlpha > 0){
            oam_copy(oam_mem, mSprites, MENU_OBJ_MAX);
            clr_fade_fast(&pal_bg_bank[1][0], CLR_WHITE, &pal_bg_bank[0][0], 16, mFadeAlpha);
            clr_fade_fast(&pal_bg_bank[1][0], CLR_WHITE, &pal_obj_bank[0][0], 16, mFadeAlpha);
            clr_fade_fast(&pal_bg_bank[13][0], CLR_WHITE, &pal_obj_bank[12][0], 16, mFadeAlpha);
            mFadeAlpha--;
            return;
        } else if(mFadeAlpha == 0){
            mFadeAlpha--;
        }

        if(SaveManager::HasSave()){
            if(key_hit(KEY_UP)) {
                mCursorIndex = (mCursorIndex - 1) % 2;
                obj_set_pos(&mSprites[BTN_SHADOW], 104, mShadowPositions[mCursorIndex]);
                mmEffect(SFX_MENU_HIGHLIGHT);
            }
            if(key_hit(KEY_DOWN)) {
                mCursorIndex = (mCursorIndex + 1) % 2;
                obj_set_pos(&mSprites[BTN_SHADOW], 104, mShadowPositions[mCursorIndex]);
                mmEffect(SFX_MENU_HIGHLIGHT);
            }
        }
        
        obj_set_pos(&mSprites[CURSOR], mCursorPositions[mCursorIndex][0], mCursorPositions[mCursorIndex][1] + (lu_sin(mTime << 10) >> 12));

        if(key_hit(KEY_A) || key_hit(KEY_START)){
            if(mCursorIndex == 0){
                mmStop();
                mmEffect(SFX_MENU_SELECT);
                WorldMap::SetWorld(0);
                GameState::ChangeState(GameState::MAP);
            }
        }

        REG_BG0VOFS = lu_sin((mTime << 10) + 620) >> 11;
        REG_BG1HOFS = -(mTime >> 1);
        REG_BG2HOFS = mTime >> 2;
        REG_BG3HOFS = -(mTime >> 3);
        
        mTime++;
        oam_copy(oam_mem, mSprites, 128);
    }
}