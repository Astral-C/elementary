#include <cstring>
#include <maxmod.h>
#include "gamestate.hpp"

#include "intro.hpp"
#include "mainmenu.hpp"

#include "mgba.h"

#include "splash.png.h"
#include "jamsplash.png.h"
#include "title.png.h"
#include "soundbank.h"

namespace Splash {
    u32 mTime = 0;
    u8 mAlpha = 32;
    u8 mStage = 0;
    u8 mScreen = 0;
    u8 mCardCount = 3;
    Screen Cards[3] = {
        {
            .mTime = 125,
            .mSFX = SFX_WOW,
            .mTiles = title_pngTiles,
            .mPalette = title_pngPal,
            .mMap = title_pngMap,
            .mTilesLen = title_pngTilesLen,
            .mPaletteLen = title_pngPalLen,
            .mMapLen = splash_pngMapLen
        },
        {
            .mTime = 220,
            .mSFX = SFX_SQUEAK,
            .mTiles = splash_pngTiles,
            .mPalette = splash_pngPal,
            .mMap = splash_pngMap,
            .mTilesLen = splash_pngTilesLen,
            .mPaletteLen = splash_pngPalLen,
            .mMapLen = splash_pngMapLen
        },
        {
            .mTime = 125,
            .mSFX = SFX_GBA_DING,
            .mTiles = jamsplash_pngTiles,
            .mPalette = jamsplash_pngPal,
            .mMap = jamsplash_pngMap,
            .mTilesLen = jamsplash_pngTilesLen,
            .mPaletteLen = jamsplash_pngPalLen,
            .mMapLen = jamsplash_pngMapLen
        }
    };
    
    mm_sfxhand CardEffectHandle;
    
    void Init(){
        memcpy(&se_mem[24][0], Cards[mScreen].mMap, Cards[mScreen].mMapLen);
        memcpy(&tile_mem[0][0], Cards[mScreen].mTiles, Cards[mScreen].mTilesLen);
        memcpy(&pal_bg_bank[1][0], Cards[mScreen].mPalette, Cards[mScreen].mPaletteLen);
        memset(&pal_bg_bank[0][0], CLR_BLACK, 32);
    
        REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;
        REG_BG0CNT = BG_SBB(24) | BG_CBB(0) | BG_4BPP | BG_REG_32x32;
    }
    
    void Update(){
        key_poll();
        
        if(key_hit(KEY_ANY)){
            mStage = 3;
            mmEffectCancel(CardEffectHandle);
        }
    
        switch (mStage) {
            case 0:
                mAlpha -= 2;
                clr_fade_fast(&pal_bg_bank[1][0], CLR_BLACK, &pal_bg_bank[0][0], 16, mAlpha);
    
                if(mAlpha <= 0){
                    CardEffectHandle = mmEffect(Cards[mScreen].mSFX);
                    mStage = 1;
                }
                
                break;
            case 1:
                Cards[mScreen].mTime--;
                if(Cards[mScreen].mTime == 0){
                    mmEffectRelease(CardEffectHandle);
                    //mmEffectCancel(CardEffectHandle);
                    mStage = 2;
                }
                break;
            case 2:
                mAlpha += 2;
                clr_fade_fast(&pal_bg_bank[1][0], CLR_BLACK, &pal_bg_bank[0][0], 16, mAlpha);
        
                if(mAlpha >= 32){
                    if(mScreen == mCardCount-1){
                        mStage = 3;
                    } else {
                        mScreen++;
                        memcpy(&se_mem[24][0], Cards[mScreen].mMap, Cards[mScreen].mMapLen);
                        memcpy(&tile_mem[0][0], Cards[mScreen].mTiles, Cards[mScreen].mTilesLen);
                        memcpy(&pal_bg_bank[1][0], Cards[mScreen].mPalette, Cards[mScreen].mPaletteLen);
                        memset(&pal_bg_bank[0][0], CLR_BLACK, 32);
                        mStage = 0;
                    }
                }
                break;
            case 3:
                mStage = 4;
                GameState::ChangeState(GameState::MAIN_MENU);
                break;
            default:
                break;
        }
    }
}