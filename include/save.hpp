#ifndef _save_h
#define _save_h
#include <tonc.h>

namespace SaveManager {

    typedef struct SaveFile {
        u32 Magic { 'ELEM' };
        u8 HasSave { 0 };
        u8 UnlockedWorldCount { 0 };
        u8 Flags[100] { 0 };
    } SaveFile;
    
    void SetFlag(u32 idx, u8 val);
    u8 GetFlag(u32 idx);

    bool HasSave();

    void Read();
    void Write();
}

#endif