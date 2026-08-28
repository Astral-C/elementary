#ifndef _map_h
#define _map_h
#include <tonc_types.h>

namespace WorldMap {
    void Init();
    void Update();
    bool GetMapFlag(u8 x, u8 y);

    struct Level {
        u8 x, y;
        u16 level_id;
    };

    struct FlagCol {
        u8 x, y;
        u16 flag;
    };
    
    struct WorldMap {
        u32 LevelCount;
        u32 FlagColCount;
        u32 WorldTilesLen;
        u32 WorldMapLen;
        u32 WorldPalLen;
        const unsigned int* WorldTiles;
        const u16* WorldPal;
        const Level* const Levels;
        const FlagCol* const FlagTiles;
        const u8 Collision[1024];
        const s32 WorldMap[1024];
    };

    extern const WorldMap Maps[3];
}

#endif