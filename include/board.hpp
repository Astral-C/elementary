#include <tonc.h>
#include "map.hpp"

namespace Board {

    struct BoardDef {
        u16 SpawnX, SpawnY;
        u32 GoalX, GoalY;
        u32 GoalFlag;
        u32 FlagOnComplete;
        u32 TileAnimCount;
        u32 BoardPalLen;
        u32 BoardTilesLen;
        const unsigned int* BoardTiles;
        const u16* BoardPal;
        const s32 BoardMapData[1024];
        u8 Collision[1024];
        WorldMap::TileAnimation* AnimatedTiles;
    };

    void SetTargetBoard(u16 boardID);
    void Init();
    void Update();

    extern const BoardDef Boards[5];
}