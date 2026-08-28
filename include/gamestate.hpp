#ifndef _state_h
#define _state_h
#include <tonc.h>
#include <functional>

namespace GameState {
    enum State {
        SPLASH,
        MAIN_MENU,
        MAP,
        BOARD,
        STATE_COUNT
    };

    void ChangeState(State next);
    void UpdateState();
}

#endif