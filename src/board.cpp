#include "board.hpp"
#include "maxmod.h"
#include "mgba.h"
#include "tonc_input.h"
#include "tonc_memdef.h"
#include "board1.png.h"
#include "plyr.png.h"
#include "tonc_memmap.h"
#include "tonc_oam.h"
#include "tonc_types.h"

namespace Board {
    int mTransHeight { 80 };
    EWRAM_DATA u16 mLevelID = { 0xFFFF };
    bool mBoardComplete = false, mBoardEnter = false, mBoardExit = false;

    int mPlayerElement { 0 };
    int mPlayerFrame = 0;
    OBJ_ATTR mSprites[1];
    int mPlayerX { 32 }, mPlayerY { 32 };
    
    void SetTargetBoard(u16 boardID){
        mLevelID = boardID;
    }

    void Init(){
        OAM_CLEAR();
        mBoardEnter = true;
        mTransHeight = 80;
        REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0 | /* DCNT_BG1 | */ DCNT_WIN0;
        REG_BG0CNT = BG_CBB(0) | BG_SBB(24) | BG_REG_32x32 | BG_4BPP;
        //REG_BG1CNT = BG_CBB(1) | BG_SBB(31) | BG_REG_32x32 | BG_4BPP;
    
        REG_BG0HOFS = 0;
        REG_BG0VOFS = 0;
        //REG_BG1HOFS = 0;
        //REG_BG1VOFS = 0;
        REG_WININ = WIN_BUILD(WIN_BG0 | WIN_BG1 | WIN_OBJ, 0);
        REG_WINOUT = WIN_BUILD(0, 0);
        REG_WIN0H = 240;

        memcpy32(&tile_mem[0][0], board1_pngTiles, board1_pngTilesLen / 4);
        memcpy32(&se_mem[24][0], board1_pngMap, board1_pngMapLen / 4);
        memcpy32(&pal_bg_bank[0][0], board1_pngPal, board1_pngPalLen / 4);

        
        memcpy32(&tile_mem_obj[0][1], plyr_pngTiles, plyr_pngTilesLen / 4);
        obj_set_attr(&mSprites[0], ATTR0_4BPP | ATTR0_SQUARE, ATTR1_SIZE_16, ATTR2_BUILD(1, 0, 0));
        obj_set_pos(&mSprites[0], 32, 32);
    }

    void Update(){
        key_poll();

        mSprites[0].attr2 = ATTR2_BUILD(1 + (mPlayerElement << 4) + (((mPlayerFrame >> 8) % 4) << 2), 0, 0);
        mPlayerFrame += 50;
        oam_copy(oam_mem, &mSprites[0], sizeof(mSprites)/sizeof(OBJ_ATTR));

        if(mBoardEnter && mTransHeight > 0){
            REG_WIN0V = max(mTransHeight << 8, 0) | min(160, (160 - mTransHeight));
            mTransHeight -= 8;
            return;
        } else if(mBoardEnter) {
            REG_DISPCNT &= ~DCNT_WIN0;
            mBoardEnter = false;
            return;
        }
        if(mBoardExit && mTransHeight < 80){
            REG_WIN0V = min(mTransHeight << 8, 80) | max(160 - mTransHeight, 80);
            mTransHeight += 8;
            return;
        } else if(mBoardExit) {
            mBoardExit = false;
            return;
        }

        if(key_hit(KEY_A)){
            mPlayerElement = (mPlayerElement + 1) % 4;
        }


        obj_set_pos(&mSprites[0], mPlayerX, mPlayerY);
    }
};