#ifndef _map_h
#define _map_h
#include <tonc_types.h>

namespace WorldMap {
    void Init();
    void Update();
    void OnExitBoard();
    void SetWorld(u32 world);
    bool GetMapFlag(u8 x, u8 y);

    struct Level {
        u8 x { 0 }, y { 0 };
        u16 LevelID { 0 };
    };

    struct FlagCol {
        u8 x { 0 }, y { 0 };
        u16 Flag { 0 };
    };

    struct TileAnimation {
        u16 BaseIndex { 0 };
        u16 FrameCount { 0 };
        u32 Speed { 0 };
        u32 PrevFrame { 0 };
        FIXED Frame { 0 };
        u32 AnimTilesLen { 0 };
        const unsigned int* AnimTiles { nullptr };
    };
    
    struct WorldMap {
        u32 LevelCount;
        u32 FlagColCount;
        u32 TileAnimCount;
        u32 WorldTilesLen;
        u32 WorldMapLen;
        u32 WorldPalLen;
        const unsigned int* WorldTiles;
        const u16* WorldPal;
        const Level* const Levels;
        const FlagCol* const FlagTiles;
        const u8 Collision[1024];
        const u8* const WorldMapData;
        TileAnimation* AnimatedTiles;
    };

    extern const WorldMap Maps[3];
}

#endif