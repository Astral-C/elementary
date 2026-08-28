#include "map.hpp"
#include "save.hpp"
#include "board.hpp"
#include "gamestate.hpp"
#include "mgba.h"
#include "tonc_bios.h"
#include "tonc_input.h"
#include "tonc_math.h"
#include "tonc_memdef.h"
#include "tonc_memmap.h"
#include "tonc_oam.h"
#include "tonc_types.h"
#include "tonc_video.h"
#include "worldmap_0_tileset.png.h"
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

    u32 mBGOffset[2] { 0, 0 };
    u32 mPlayerPos[2] { 64, 64 };
    u32 mPlayerGridPos[2] { 4, 4 };

    OBJ_ATTR mSprites[10] {};
    EWRAM_DATA u32 mCurrentWorld { 0 };
    const WorldMap* Map = &Maps[0];

    int mTransHeight { 0 };
    EWRAM_DATA u16 mTargetLevelID = { 0xFFFF };
    bool mBoardEnter = false, mBoardExit = false;
    
    void SetWorld(u32 world){
        Map = &Maps[world];
    }
    
    void Init(){
        OAM_CLEAR();
        
        oam_init(&mSprites[0], 10);
        
        REG_DISPCNT = DCNT_MODE0 | DCNT_OBJ | DCNT_OBJ_1D | DCNT_BG0 | DCNT_BG1;
        REG_BG0CNT = BG_CBB(0) | BG_SBB(27) | BG_REG_64x64 | BG_4BPP;
        REG_BG1CNT = BG_CBB(1) | BG_SBB(31) | BG_REG_32x32 | BG_4BPP;

        memcpy32(&tile_mem[0][0], worldmap_0_tileset_pngTiles, worldmap_0_tileset_pngTilesLen / 4);
        memcpy32(&pal_bg_bank[0][0], worldmap_0_tileset_pngPal, worldmap_0_tileset_pngPalLen / 4);
        //memcpy32(&se_mem[27][0], worldmap_0_pngMap, worldmap_0_pngMapLen / 4);

        u16* blocks[4] = { &se_mem[27][0], &se_mem[28][0], &se_mem[29][0], &se_mem[30][0] };
        for(u32 my = 0; my < 16; my++){
            for(u32 mx = 0; mx < 16; mx++){
                u32 tx = mx << 1, ty = my << 1;
                u32 tileIdx[4] = { (my * 32 + mx), (my * 32 + (mx + 16)), ((16 + my) * 32 + mx), ((16 + my) * 32 + (mx + 16)) };
                s32 tileEntries[4] = { Map->WorldMap[tileIdx[0]], Map->WorldMap[tileIdx[1]], Map->WorldMap[tileIdx[2]], Map->WorldMap[tileIdx[3]] };

                for(u32 y = 0; y < 2; y++){
                    for(u32 x = 0; x < 2; x++){
                        u32 dx = tx + x, dy = ty + y;
                        for(u32 sbb = 0; sbb < 4; sbb++){
                            u32 tile = tileEntries[sbb];
                            u8 flipx = tile >> 31, flipy = (tile >> 30) & 1; 
                            
                            blocks[sbb][dy * 32 + dx] = SE_BUILD(((tile & 0x00FFFFFF) << 2) + ((y << 1) + ((x + flipx) & 1)), 0, flipx, 0);
                        }
                    }
                }
            }
        }
        
        memcpy32(&tile_mem_obj[0][1], player_worldmap_pngTiles, player_worldmap_pngTilesLen / 4);
        memcpy32(&pal_obj_bank[0][0], player_worldmap_pngPal, player_worldmap_pngPalLen / 4);
        
        obj_set_attr(&mSprites[PLAYER], ATTR0_4BPP | ATTR0_SQUARE, ATTR1_SIZE_16, ATTR2_BUILD(1, 0, 0));
        obj_set_pos(&mSprites[PLAYER], 64, 64);

        mBGOffset[0] = 0;
        mBGOffset[1] = 0;
        mPlayerPos[0] = 64;
        mPlayerPos[1] = 64;
        mPlayerGridPos[0] = 4;
        mPlayerGridPos[1] = 4;

        REG_BG0HOFS = 0; //mBGOffset[0];
        REG_BG0VOFS = 0; //mBGOffset[1];
        REG_BG1HOFS = 0; //mBGOffset[0];
        REG_BG1VOFS = 0; //mBGOffset[1];
    }

    void Update(){
        key_poll();

        if(mBoardEnter && mTransHeight < 84){
            REG_WIN0V = min(mTransHeight << 8, 80 << 8) | max(160 - mTransHeight, 80);
            mTransHeight += 8;
            return;
        } else if(mBoardEnter && mTransHeight >= 84) {
            REG_WINOUT = WIN_BUILD(0, 0);
            REG_WIN0V = 0;
            REG_WIN0H = 0;
            mBoardEnter = false;
            Board::SetTargetBoard(mTargetLevelID);
            GameState::ChangeState(GameState::BOARD);
            return;
        }
        
        if(mBoardExit && mTransHeight > 0){
            REG_WIN0V = max(mTransHeight << 8, 0) | min(160, (160 - mTransHeight));
            mTransHeight -= 8;
            return;
        } else if(mBoardExit && mTransHeight <= 0) {
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

        if(key_hit(KEY_A)){
            for(u32 i = 0; i < Map->LevelCount; i++){
                if(Map->Levels[i].x == mPlayerGridPos[0] && Map->Levels[i].y == mPlayerGridPos[1]){
                    // Do transition
                    mTargetLevelID = Map->Levels[i].level_id;
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
            if(key_hit(KEY_DIR)) idx = mPlayerGridPos[1] * 32 + mPlayerGridPos[0];
            if(key_hit(KEY_UP)){
                if(Map->Collision[idx - 32] == 1 || (Map->Collision[idx - 32] == 2 && GetMapFlag(mPlayerGridPos[0], mPlayerGridPos[1] - 1))){
                    mPlayerGridPos[1]--;
                    mMoving = true;
                }
            } else if(key_hit(KEY_DOWN)){
                if(Map->Collision[idx + 32] == 1 || (Map->Collision[idx + 32] == 2 && GetMapFlag(mPlayerGridPos[0], mPlayerGridPos[1] + 1))){
                    mPlayerGridPos[1]++;
                    mMoving = true;
                }
            } else if(key_hit(KEY_RIGHT)){
                if(Map->Collision[idx + 1] == 1 || (Map->Collision[idx + 1] == 2 && GetMapFlag(mPlayerGridPos[0] + 1, mPlayerGridPos[1]))){
                    mPlayerGridPos[0]++;
                    mMoving = true;
                }
            } else if(key_hit(KEY_LEFT)){
                if(Map->Collision[idx - 1] == 1 || (Map->Collision[idx - 1] == 2 && GetMapFlag(mPlayerGridPos[0] - 1, mPlayerGridPos[1]))){
                    mPlayerGridPos[0]--;
                    mMoving = true;
                }
            }
        }
        
        obj_set_pos(&mSprites[PLAYER], mPlayerPos[0], mPlayerPos[1] + (lu_sin(mTime << 9) >> 10));
        if((mTime % 30) == 0) mSprites[PLAYER].attr2 = ATTR2_BUILD((mSprites[PLAYER].attr2 & ATTR2_ID_MASK) == 5 ? 1 : 5, 0, 0);
        if((mTime % 10) == 0){
            memcpy32(&tile_mem[0][12], water_tile_anim_pngTiles + (32 * mTilesetFrame), 32); // 4 tiles = 32 words
            memcpy32(&tile_mem[0][16], water_tile_anim_pngTiles + (32 * mTilesetFrame), 32);
            mTilesetFrame = (mTilesetFrame + 1) % 4;
        }
        
        REG_BG0VOFS = mBGOffset[1];
        REG_BG0HOFS = mBGOffset[0];
        oam_copy(oam_mem, mSprites, 10);
        mTime++;
    }

    bool GetMapFlag(u8 x, u8 y){
        for (u32 i = 0; i < Map->FlagColCount; i++) {
            if(Map->FlagTiles[i].x == x && Map->FlagTiles[i].y == y && SaveManager::GetFlag(Map->FlagTiles[i].flag) == 1){
                return true;
            }
        }
        return false;
    }
}