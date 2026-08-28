#include "save.hpp"
#include "tonc_memmap.h"
#include <cstring>

namespace SaveManager {
    SaveFile mCurrentSave;

    bool HasSave(){
        return mCurrentSave.HasSave == 1;
    }
    
    void SetFlag(u32 idx, u8 val){
        mCurrentSave.Flags[idx] = val;
    }
    
    u8 GetFlag(u32 idx){
        return mCurrentSave.Flags[idx];
    }


    void Read(){
        for(u32 byte = 0; byte < sizeof(SaveFile); byte++) *((u8*)&mCurrentSave) = sram_mem[byte];
        if(mCurrentSave.Magic != 'ELEM') {
            mCurrentSave = SaveFile();
        }
    }

    void Write(){
        for(u32 byte = 0; byte < sizeof(SaveFile); byte++) sram_mem[byte] = *((u8*)&mCurrentSave);
    }
}

