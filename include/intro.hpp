#ifndef __intro_h
#define __intro_h
#include <tonc.h>
#include <maxmod.h>
#include "gamestate.hpp"

namespace Splash {
    struct Screen {
        u8 mTime;
        u8 mSFX;
        const unsigned int* mTiles;
        const u16* mPalette;
        const u16* mMap;
        u32 mTilesLen;
        u32 mPaletteLen;
        u32 mMapLen;
    };

    void Init();
    void Update();
}
#endif