#include <tonc.h>
#include "mgba.h"
#include "map.hpp"
#include "maxmod.h"
#include "save.hpp"
#include "board.hpp"
#include "gamestate.hpp"
#include "soundbank.h"
#include "water_tile_anim.png.h"
#include "player_worldmap.png.h"

namespace WorldMap {

    enum OBJ {
        PLAYER,
        LEVEL_MARKER,
        SPRITE_MAX
    };
    
    u32 mFadeAlpha { 32 };
    u32 mTime { 0 };
    u32 mTilesetFrame { 0 };
    bool mMoving { false };


    OBJ_ATTR mSprites[128] {};
    EWRAM_DATA u32 mCurrentWorld { 0 };
    const WorldMap* Map = &Maps[0];
    
    int mTransHeight { 0 };
    EWRAM_DATA u16 mTargetLevelID = { 0xFFFF };
    bool mBoardEnter = false, mBoardExit = false;

    s32 mBGOffset[2] { 0, 0 };
    s32 mPlayerPos[2] { 64, 64 };
    s32 mPlayerGridPos[2] { 4, 4 };
    
    void SetWorld(u32 world){
        Map = &Maps[world];

        mBGOffset[0] = 0; mBGOffset[1] = 0;
        mPlayerPos[0] = 64; mPlayerPos[1] = 64;
        mPlayerGridPos[0] = 4; mPlayerGridPos[1] = 4;        
    }
    
    void Init(){
        
        oam_init(&mSprites[0], 128);
        
        REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0 | DCNT_BG1;
        REG_BG0CNT = BG_CBB(0) | BG_SBB(27) | BG_REG_64x64 | BG_4BPP;
        REG_BG1CNT = BG_CBB(1) | BG_SBB(31) | BG_REG_32x32 | BG_4BPP;

        if(mBoardExit){
            REG_DISPCNT |= DCNT_WIN0;
        }

        memcpy32(&tile_mem[0][0], Map->WorldTiles, Map->WorldTilesLen / 4);
        memcpy32(&pal_bg_bank[0][0], Map->WorldPal, Map->WorldPalLen / 4);
        memcpy32(&se_mem[27][0], Map->WorldMapData, Map->WorldMapLen / 4);
        
        memcpy32(&tile_mem_obj[0][1], player_worldmap_pngTiles, player_worldmap_pngTilesLen / 4);
        memcpy32(&pal_obj_bank[0][0], player_worldmap_pngPal, player_worldmap_pngPalLen / 4);

        obj_set_attr(&mSprites[PLAYER], ATTR0_4BPP | ATTR0_SQUARE, ATTR1_SIZE_16, ATTR2_BUILD(1, 0, 0));
        obj_set_pos(&mSprites[PLAYER], mPlayerPos[0], mPlayerPos[1]);
        
        for(u32 f = 0; f < Map->FlagColCount; f++){
            if(SaveManager::GetFlag(Map->FlagTiles[f].Flag)){
                // get 0 - 3 sbb index from 16x16 tile coords
                u32 blockX = Map->FlagTiles[f].x >> 4, blockY = Map->FlagTiles[f].y >> 4;
                u32 sbbIndex = 27 + (blockY * 2 + blockX);
                u32 localX = Map->FlagTiles[f].x - (blockX << 4), localY = Map->FlagTiles[f].y - (blockY << 4);
                for (u32 i = 0; i < 2; i++) {
                    for (u32 j = 0; j < 2; j++) {
                        se_mem[sbbIndex][((localY << 1) + j) * 32 + ((localX << 1) + i)] = SE_BUILD(84 + ((j << 1) + i), 0, 0, 0);
                    }
                }
            }
        }
        mmStart(MOD_YEAR2014, MM_PLAY_LOOP);
    }

    void OnExitBoard(){
        mBoardExit = true;
    }

    void Update(){
        key_poll();

        if(mBoardEnter && mTransHeight < 80){
            REG_WIN0V = min(mTransHeight << 8, 80 << 8) | max(160 - mTransHeight, 80);
            mTransHeight += 8;
            return;
        } else if(mBoardEnter && mTransHeight >= 80) {
            REG_WINOUT = WIN_BUILD(0, 0);
            REG_WIN0V = 0;
            REG_WIN0H = 0;
            mBoardEnter = false;
            mmStop();
            Board::SetTargetBoard(mTargetLevelID);
            GameState::ChangeState(GameState::BOARD);
            return;
        }
        
        if(mBoardExit && mTransHeight > 0){
            REG_BG0VOFS = mBGOffset[1];
            REG_BG0HOFS = mBGOffset[0];
            REG_WIN0V = max(mTransHeight << 8, 0) | min(160, (160 - mTransHeight));
            mTransHeight -= 8;
            return;
        } else if(mBoardExit && mTransHeight <= 0) {
            REG_DISPCNT &= ~DCNT_WIN0;
            mBoardExit = false;
        }

        if((mPlayerPos[0] + mBGOffset[0]) > (mPlayerGridPos[0] << 4)){
            if(mPlayerPos[0] <= 32){
                mBGOffset[0]--;
            } else {
                mPlayerPos[0]--;
            }
        } else if((mPlayerPos[0] + mBGOffset[0]) < (mPlayerGridPos[0] << 4)){
            if(mPlayerPos[0] >= 200){
                mBGOffset[0]++;
            } else {
                mPlayerPos[0]++;
            }
        } else if((mPlayerPos[1] + mBGOffset[1]) > (mPlayerGridPos[1] << 4)){
            if(mPlayerPos[1] <= 32){
                mBGOffset[1]--;
            } else {
                mPlayerPos[1]--;
            }
        } else if((mPlayerPos[1] + mBGOffset[1]) < (mPlayerGridPos[1] << 4)){
            if(mPlayerPos[1] >= 112){
                mBGOffset[1]++;
            } else {
                mPlayerPos[1]++;
            }
        } else {
            mMoving = false;
        }

        if(key_hit(KEY_A) && !mMoving){
            for(u32 i = 0; i < Map->LevelCount; i++){
                if(Map->Levels[i].x == mPlayerGridPos[0] && Map->Levels[i].y == mPlayerGridPos[1]){
                    // Do transition
                    mTargetLevelID = Map->Levels[i].LevelID;
                    REG_DISPCNT |= DCNT_WIN0;
                    REG_WININ = WIN_BUILD(WIN_BG0 | WIN_OBJ, 0);
                    REG_WINOUT = WIN_BUILD(0, 0);
                    REG_WIN0H = 240;
                    REG_WIN0V = 160;
                    mBoardEnter = true;
                    return; 
                } 
            }
        }
        
        if(mMoving == false){
            u32 idx = 0;
            if(key_is_down(KEY_DIR)) idx = mPlayerGridPos[1] * 32 + mPlayerGridPos[0];
            if(key_is_down(KEY_UP)){
                if(Map->Collision[idx - 32] == 1 || (Map->Collision[idx - 32] == 2 && GetMapFlag(mPlayerGridPos[0], mPlayerGridPos[1] - 1))){
                    mPlayerGridPos[1]--;
                    mMoving = true;
                }
            } else if(key_is_down(KEY_DOWN)){
                if(Map->Collision[idx + 32] == 1 || (Map->Collision[idx + 32] == 2 && GetMapFlag(mPlayerGridPos[0], mPlayerGridPos[1] + 1))){
                    mPlayerGridPos[1]++;
                    mMoving = true;
                }
            } else if(key_is_down(KEY_RIGHT)){
                if(Map->Collision[idx + 1] == 1 || (Map->Collision[idx + 1] == 2 && GetMapFlag(mPlayerGridPos[0] + 1, mPlayerGridPos[1]))){
                    mPlayerGridPos[0]++;
                    mMoving = true;
                }
            } else if(key_is_down(KEY_LEFT)){
                if(Map->Collision[idx - 1] == 1 || (Map->Collision[idx - 1] == 2 && GetMapFlag(mPlayerGridPos[0] - 1, mPlayerGridPos[1]))){
                    mPlayerGridPos[0]--;
                    mMoving = true;
                }
            }
        }
        
        obj_set_pos(&mSprites[PLAYER], mPlayerPos[0], mPlayerPos[1] + (lu_sin(mTime << 9) >> 10));
        if((mTime % 30) == 0) mSprites[PLAYER].attr2 = ATTR2_BUILD((mSprites[PLAYER].attr2 & ATTR2_ID_MASK) == 5 ? 1 : 5, 0, 0);

        for(u32 i = 0; i < Map->TileAnimCount; i++){
            Map->AnimatedTiles[i].Frame += Map->AnimatedTiles[i].Speed;
            u32 curFrame = fx2int(Map->AnimatedTiles[i].Frame);
            if(curFrame >= Map->AnimatedTiles[i].FrameCount){
                Map->AnimatedTiles[i].Frame = 0;
                curFrame = 0;
            }
            
            if(curFrame != Map->AnimatedTiles[i].PrevFrame){
                memcpy32(&tile_mem[0][Map->AnimatedTiles[i].BaseIndex], Map->AnimatedTiles[i].AnimTiles + (32 * curFrame), 32); // 4 tiles = 32 words
                Map->AnimatedTiles[i].PrevFrame = curFrame;
            }
        }
        
        REG_BG0VOFS = mBGOffset[1];
        REG_BG0HOFS = mBGOffset[0];
        oam_copy(oam_mem, mSprites, 128);
        mTime++;
    }

    bool GetMapFlag(u8 x, u8 y){
        for (u32 i = 0; i < Map->FlagColCount; i++) {
            if(Map->FlagTiles[i].x == x && Map->FlagTiles[i].y == y && SaveManager::GetFlag(Map->FlagTiles[i].Flag) == 1){
                return true;
            }
        }
        return false;
    }
}