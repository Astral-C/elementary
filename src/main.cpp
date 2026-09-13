#include <maxmod.h>
#include "gamestate.hpp"
#include "intro.hpp"
#include "mgba.h"
#include "save.hpp"
#include "tonc_irq.h"
#include "soundbank.bin.h"


int main(void) {
    mgba_open();
    REG_DISPCNT = DCNT_MODE0 | DCNT_BG0;

    irq_init(NULL);
    irq_enable(II_VBLANK);
    irq_add(II_VBLANK, mmVBlank);
    
    mmInitDefault((mm_addr)soundbank_bin, 16);
    
    SaveManager::Read();

    GameState::ChangeState(GameState::SPLASH);
    mmSetModuleVolume(1024);
    mmSetEffectsVolume(1024);
    
    while (1) {
        VBlankIntrWait();
        GameState::UpdateState();
        mmFrame();
    }
    mgba_close();
}
