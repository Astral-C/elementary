#include "intro.hpp"
#include "mainmenu.hpp"
#include "map.hpp"
#include "board.hpp"
#include "mgba.h"
#include <functional>
#include "gamestate.hpp"

namespace GameState {
    std::function<void()> mStateInit[STATE_COUNT] = {
        Splash::Init,
        MainMenu::Init,
        WorldMap::Init,
        Board::Init
    };
    
    std::function<void()> mStateUpdate[STATE_COUNT] = {
        Splash::Update,
        MainMenu::Update,
        WorldMap::Update,
        Board::Update
    };
    
    State mCurrentState { SPLASH };
    
    void UpdateState(){
        //mgba_printf(MGBA_LOG_DEBUG, "Current State is %d", mCurrentState);
        mStateUpdate[mCurrentState]();
    }
    
    void ChangeState(State next){
        mStateInit[next]();
        mgba_printf(MGBA_LOG_DEBUG, "Changing State to %d", next);
        mCurrentState = next;
    }
}