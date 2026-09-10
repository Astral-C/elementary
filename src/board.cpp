#include <tonc.h>
#include "board.hpp"
#include "gamestate.hpp"
#include "map.hpp"
#include "maxmod.h"
#include "mgba.h"
#include "save.hpp"
#include "board1.png.h"
#include "plyr.png.h"
#include "soundbank.h"

namespace Board {
    enum PLAYER_ELEMENT {
        PLAYER_NONE,
        PLAYER_FIRE,
        PLAYER_WATER,
        PLAYER_ICE,
        ELEMENT_MAX
    };

    enum TILE_ELEMENT {
        TILE_NONE,
        TILE_SOLID,
        TILE_WATER,
        TILE_UNLIT,
        TILE_LIT,
        TILE_ICE,
        TILE_SETEL_FIRE,
        TILE_SETEL_WATER,
        TILE_SETEL_ICE,
        TILE_GOAL,
        TILE_MAX
    };
    
    int mTransHeight { 80 };
    bool mBoardComplete { false } , mBoardEnter { false }, mBoardExit { false }, mHelpOpen { false };
    u32 mLitTorches { 0 };
    
    int mPlayerElement { 0 };
    int mPlayerFrame { 0 };
    OBJ_ATTR mSprites[128];

    u32 mBGOffset[2] { 0, 0 };
    u32 mPlayerPos[2] { 0, 0 };
    s32 mMoveDir[2] { 0, 0 };
    u32 mMoveTimer { 16 };

    bool mMoving { false };
    u32 mTimer { 0 };

    u32 mResetWait { 0 };
    u32 mElementRemaining { 0 };
    
    const BoardDef* mCurrentBoard { &Boards[0] };
    u8 CurrentBoardCol[1024] { 0 };
    
    void SetTargetBoard(u16 boardID){
        mCurrentBoard = &Boards[boardID];
    }

    void SetMetaTile(u32 tx, u32 ty, u32 tileID){
        u32 blockX = tx >> 4, blockY = ty >> 4;
        u32 sbbIndex = 27 + (blockY * 2 + blockX);
        u32 localX = tx - (blockX << 4), localY = ty - (blockY << 4);
        for (u32 i = 0; i < 2; i++) {
            for (u32 j = 0; j < 2; j++) {
                se_mem[sbbIndex][((localY << 1) + j) * 32 + ((localX << 1) + i)] = SE_BUILD((tileID << 2) + ((j << 1) + i), 0, 0, 0);
            }
        }
    }

    // used for load and reset on start btn
    void Reset(){
        mLitTorches = 0;

        // Init Player Info
        mPlayerPos[0] = (SCR_W >> 1) - 8;
        mPlayerPos[1] = (SCR_H >> 1) - 8;

        // Center on spawn pos
        mBGOffset[0] = (mCurrentBoard->SpawnX << 4) - (SCR_W >> 1) - 8;
        mBGOffset[1] = (mCurrentBoard->SpawnY << 4) - (SCR_H >> 1) - 8;

        REG_BG0HOFS = max(mBGOffset[0], 0);
        REG_BG0VOFS = min(mBGOffset[1], 512 - 240);
        
        mBoardComplete = false;
        mHelpOpen = false;
        
        mPlayerFrame = 0;
    
        mMoveDir[0] = 0;
        mMoveDir[1] = 0;
        mMoveTimer = 16;
    
        mElementRemaining = 0;
        mMoving = false;
        mTimer = 0;
        
        mPlayerElement = PLAYER_NONE;
        memcpy32(&CurrentBoardCol[0], mCurrentBoard->Collision, 1024 / 4);
        
        // Should probably be done with the same external tool that does other maps buuuuut
        // dont feel like fixing it. i am. ill.
        u16* blocks[4] = { &se_mem[27][0], &se_mem[28][0], &se_mem[29][0], &se_mem[30][0] };
        for(u32 my = 0; my < 16; my++){
            for(u32 mx = 0; mx < 16; mx++){
                u32 tx = mx << 1, ty = my << 1;
                u32 tileIdx[4] = { (my * 32 + mx), (my * 32 + (mx + 16)), ((16 + my) * 32 + mx), ((16 + my) * 32 + (mx + 16)) };
                s32 tileEntries[4] = { mCurrentBoard->BoardMapData[tileIdx[0]], mCurrentBoard->BoardMapData[tileIdx[1]], mCurrentBoard->BoardMapData[tileIdx[2]], mCurrentBoard->BoardMapData[tileIdx[3]] };

                for(u32 y = 0; y < 2; y++){
                    for(u32 x = 0; x < 2; x++){
                        u32 dx = tx + x, dy = ty + y;
                        for(u32 sbb = 0; sbb < 4; sbb++){
                            u32 tile = tileEntries[sbb];
                            u8 flipx = tile >> 31, flipy = (tile >> 30) & 1; 
                            
                            blocks[sbb][dy * 32 + dx] = SE_BUILD(((tile & 0x00FFFFFF) << 2) + ((((y + flipy) & 1) << 1) + ((x + flipx) & 1)), 0, flipx, flipy);
                        }
                    }
                }
            }
        }
        
        obj_set_attr(&mSprites[0], ATTR0_4BPP | ATTR0_SQUARE, ATTR1_SIZE_16, ATTR2_BUILD(1, 0, 0));
        obj_set_pos(&mSprites[0], (SCR_W >> 1) - 8, (SCR_H >> 1) - 8);
    }
    
    void Init(){
        oam_init(mSprites, 128);
        oam_copy(oam_mem, mSprites, 128);
        mBoardComplete = false;
        SaveManager::SetFlag(mCurrentBoard->FlagOnComplete, 0);
        mBoardEnter = true;
        mTransHeight = 80;
        REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0 | DCNT_WIN0 | DCNT_BG1;
        REG_BG1CNT = BG_CBB(1) | BG_SBB(26) | BG_REG_32x32 | BG_4BPP | BG_PRIO(0);
        REG_BG2CNT = BG_CBB(1) | BG_SBB(25) | BG_REG_32x32 | BG_4BPP | BG_PRIO(1);
        REG_BG0CNT = BG_CBB(0) | BG_SBB(27) | BG_REG_64x64 | BG_4BPP | BG_PRIO(2);
        REG_BG1HOFS = 0;
        REG_BG1VOFS = 0;

        REG_WININ = WIN_BUILD(WIN_BG0 | WIN_BG1 | WIN_OBJ, 0);
        REG_WINOUT = WIN_BUILD(0, 0);
        REG_WIN0H = 240;

        REG_BLDCNT = BLD_BUILD(BLD_BG2 | BLD_BG1, BLD_BG0 | BLD_OBJ, 1);
        REG_BLDALPHA = BLDA_BUILD(31, 8);

        tte_init_se(1, (BG_CBB(1) | BG_SBB(26) | BG_REG_32x32 | BG_4BPP), 0xF000, CLR_WHITE, 1, nullptr, nullptr);

        memcpy32(&tile_mem[0][0], mCurrentBoard->BoardTiles, mCurrentBoard->BoardTilesLen / 4);
        memcpy32(&pal_bg_bank[0][0], mCurrentBoard->BoardPal, mCurrentBoard->BoardPalLen / 4);
        memcpy32(&tile_mem_obj[0][1], plyr_pngTiles, plyr_pngTilesLen / 4);

        memset16(&se_mem[25][0], SE_BUILD(95, 2, 0, 0), 1024);

        pal_bg_bank[2][1] = CLR_BLACK;
        tile_mem[1][95] = {
            0x11111111, 0x11111111, 0x11111111, 0x11111111,
            0x11111111, 0x11111111, 0x11111111, 0x11111111
        };
        tte_write("#{P:168,158}Help: L");
        
        Reset();
        mmStart(MOD_BLUE_INTERMISSION, MM_PLAY_LOOP);
    }
    
    void HandleCollision(u32 idx, u32 tx, u32 ty, u32 cidx, u32 cx, u32 cy){
        u8 leaving_col = CurrentBoardCol[cidx];
        u8 target_col = CurrentBoardCol[idx];

        switch (leaving_col) {
            case TILE_SETEL_ICE:
                CurrentBoardCol[cidx] = TILE_NONE;
                SetMetaTile(cx, cy, 0);
                break;
            case TILE_SETEL_FIRE:
                CurrentBoardCol[cidx] = TILE_NONE;
                SetMetaTile(cx, cy, 0);
                break;
            case TILE_SETEL_WATER:
                CurrentBoardCol[cidx] = TILE_NONE;
                SetMetaTile(cx, cy, 0);
                break;      
            case TILE_ICE:
                switch (mPlayerElement) {
                    case PLAYER_FIRE: {
                        CurrentBoardCol[cidx] = TILE_WATER;
                        SetMetaTile(cx, cy, 1);
                        mmEffect(SFX_SPLASH);
                        break;
                    }
                    default:
                        break;
                }
                break;
        }

        switch (target_col) {
            case TILE_WATER:
                switch (mPlayerElement) {
                    case PLAYER_ICE: {
                        mElementRemaining--;
                        if(mElementRemaining == 0){
                            mPlayerElement = PLAYER_NONE;
                        }
                        CurrentBoardCol[idx] = TILE_ICE;
                        mmEffect(SFX_SLEIGHBELL);
                        SetMetaTile(tx, ty, 15);
                    }                        
                    case PLAYER_WATER:
                        mMoving = true;
                        break;
                    default:
                        break;
                }
                break;
            case TILE_ICE:
                switch (mPlayerElement) {
                    case PLAYER_WATER:
                        mPlayerElement = PLAYER_ICE;
                    default:
                        mMoving = true;
                        break;
                }
                break;
            case TILE_UNLIT:
                if(mPlayerElement == PLAYER_FIRE){
                    mPlayerElement = PLAYER_NONE;
                    CurrentBoardCol[idx] = TILE_LIT;
                    mmEffect(SFX_IGNITE);
                    SetMetaTile(tx, ty, 3);

                    mLitTorches++;
                    if(mLitTorches == mCurrentBoard->GoalFlag){
                        CurrentBoardCol[(mCurrentBoard->GoalY * 32) + mCurrentBoard->GoalX] = TILE_GOAL;
                        SetMetaTile(mCurrentBoard->GoalX, mCurrentBoard->GoalY, 19);
                    }
                }
                mMoving = true;
                break;
            case TILE_LIT:
                switch(mPlayerElement){
                    case PLAYER_ICE:
                        mPlayerElement = PLAYER_WATER;
                        mElementRemaining = 0;
                        mMoving = true;
                        break;
                    case PLAYER_WATER:
                        CurrentBoardCol[idx] = TILE_UNLIT;
                        SetMetaTile(tx, ty, 2);
                    case PLAYER_FIRE:
                        mMoving = true;
                        break;
                }
                break;
            case TILE_SETEL_FIRE: {
                mMoving = true;
                mPlayerElement = PLAYER_FIRE;
                mmEffect(SFX_IGNITE);
                break;
            }
            case TILE_SETEL_ICE: {
                mMoving = true;
                mPlayerElement = PLAYER_ICE;
                mElementRemaining = 16;
                mmEffect(SFX_SLEIGHBELL);
                break;
            }
            case TILE_SETEL_WATER: {
                mMoving = true;
                mPlayerElement = PLAYER_WATER;
                mmEffect(SFX_SPLASH);
                break;
            }
            case TILE_GOAL: {
                mMoving = true;
                mBoardExit = true;
                mBoardComplete = true;
                mTransHeight = 0;
                REG_WIN0V = 160;
                REG_DISPCNT |= DCNT_WIN0;
                SaveManager::SetFlag(mCurrentBoard->FlagOnComplete, 1);
                WorldMap::OnExitBoard();
                break;
            }
            case TILE_SOLID:
                mMoving = false;
                break;
            case TILE_NONE:
            default:
                mMoving = true;
                break;
        }
    }
    
    void Update(){        
        if(mResetWait > 0){
            mResetWait--;
            return;
        }
        
        key_poll();
        
        mSprites[0].attr2 = ATTR2_BUILD(1 + (mPlayerElement << 4) + (((mPlayerFrame >> 8) % ELEMENT_MAX) << 2), 0, 2);
        mPlayerFrame += 50;

        obj_set_pos(&mSprites[0], mPlayerPos[0], mPlayerPos[1]);

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
            REG_WIN0V = min(mTransHeight, 80) << 8 | max(160 - mTransHeight, 80);
            mTransHeight += 8;
            return;
        } else if(mBoardExit) {
            if(mBoardComplete){
                mmStop();
                GameState::ChangeState(GameState::MAP);
            } else {
                REG_WIN0V = 80 << 8 | 80;
                Reset();
                mResetWait = 30;
                mBoardEnter = true;
            }
            mBoardExit = false;
            return;
        }

        if(!mMoving && key_hit(KEY_R)){
            REG_DISPCNT |= DCNT_WIN0;
            mBoardExit = true;
            return;
        }
        
        //if(key_hit(KEY_A)){
        //    mPlayerElement = (mPlayerElement + 1) % 4;
        //}
        if(key_hit(KEY_L) && !mHelpOpen){
            tte_erase_rect(168, 158, 240, 160);
            tte_write("#{P:32,8}Light all the fire pits!\
                #{P:104,24}Ice#{P:16,32}Freezes 16 water tiles\
                #{P:104,48}Fire#{P:16,56}Melts ice, light 1 pit\
                #{P:104,72}Water#{P:16,80}Walk on water,#{P:16,88}extinquishes fire\
                #{P:64,144}R: Reset Board");
            REG_DISPCNT |= DCNT_BG2;
            mSprites[0].attr0 |= ATTR0_BLEND;
            mHelpOpen = true;
            mmEffect(SFX_MENU_SELECT);
        } else if(mHelpOpen && key_hit(KEY_ANY)){
            REG_DISPCNT &= ~DCNT_BG2;
            mSprites[0].attr0 &= ~ATTR0_BLEND;
            tte_erase_rect(0, 0, 240, 160);
            tte_write("#{P:168,158}Help: L");
            mHelpOpen = false;
        }


        if(mMoving == false){
            u32 idx = 0;
            u32 tx = (mPlayerPos[0] + mBGOffset[0]) >> 4, ty = (mPlayerPos[1] + mBGOffset[1]) >> 4;
            if(key_is_down(KEY_DIR)) idx = (ty * 32 + tx);
            if(key_is_down(KEY_UP)){
                HandleCollision(idx - 32, tx, ty - 1, idx, tx, ty);
                if(mMoving) mMoveDir[1] = -16;
            } else if(key_is_down(KEY_DOWN)){
                HandleCollision(idx + 32, tx, ty + 1, idx, tx, ty);
                if(mMoving) mMoveDir[1] = 16;
            } else if(key_is_down(KEY_RIGHT)){
                HandleCollision(idx + 1, tx + 1, ty, idx, tx, ty);
                if(mMoving) mMoveDir[0] = 16;
            } else if(key_is_down(KEY_LEFT)){
                HandleCollision(idx - 1, tx - 1, ty, idx, tx, ty);
                if(mMoving) mMoveDir[0] = -16;
            }
        } else {
            if(mMoveDir[0] > 0){
                if(mPlayerPos[0] + 1 > 160) {
                    mBGOffset[0]++;
                } else {
                    mPlayerPos[0]++;
                }
                mMoveDir[0]--;
            } else if(mMoveDir[0] < 0){
                if(mPlayerPos[0] - 1 < 64) {
                    mBGOffset[0]--;
                } else {
                    mPlayerPos[0]--;
                }
                mMoveDir[0]++;
            } else if(mMoveDir[1] > 0){
                if(mPlayerPos[1] + 1 > 96) {
                    mBGOffset[1]++;
                } else {
                    mPlayerPos[1]++;
                }
                mMoveDir[1]--;
            } else if(mMoveDir[1] < 0){
                if(mPlayerPos[1] - 1 < 64) {
                    mBGOffset[1]--;
                } else {
                    mPlayerPos[1]--;
                }
                mMoveDir[1]++;
            } else if(mMoveDir[0] == 0 && mMoveDir[1] == 0) {
                mMoving = false;
                if(mBoardComplete){
                    mBoardExit = true;
                }
            }
        }
        
        for(u32 i = 0; i < mCurrentBoard->TileAnimCount; i++){
            mCurrentBoard->AnimatedTiles[i].Frame += mCurrentBoard->AnimatedTiles[i].Speed;
            u32 curFrame = fx2int(mCurrentBoard->AnimatedTiles[i].Frame);
            if(curFrame >= mCurrentBoard->AnimatedTiles[i].FrameCount){
                mCurrentBoard->AnimatedTiles[i].Frame = 0;
                curFrame = 0;
            }
            
            if(curFrame != mCurrentBoard->AnimatedTiles[i].PrevFrame){
                memcpy32(&tile_mem[0][mCurrentBoard->AnimatedTiles[i].BaseIndex], mCurrentBoard->AnimatedTiles[i].AnimTiles + (32 * curFrame), 32); // 4 tiles = 32 words
                mCurrentBoard->AnimatedTiles[i].PrevFrame = curFrame;
            }
        }
        REG_BG0HOFS = mBGOffset[0];
        REG_BG0VOFS = mBGOffset[1];
        mTimer++;
    }
};