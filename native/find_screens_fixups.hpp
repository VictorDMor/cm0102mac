#pragma once
#include "aot_runtime.hpp"
#include <cstdio>

namespace cm::aot {

// FIND_SCREENS.CPP's club-list builders 0x10104b90 and 0x101050f0 (cases of
// 0x10103850) keep in r20 the nation whose clubs they list, taken from a value
// of the screen's history record, and never check it. True when there is no
// nation: the original then read the title's byte nation+0x115, and every row
// its filter kept, through page zero. Said once per run, since the fault it
// replaces was the only trace of the navigation that opens a list this way.
inline bool find_list_without_nation(const State& state) {
    if (state.r<4>(80) != 0) return false;
    static bool said = false;
    if (!said) {
        said = true;
        std::fputs("Club list opened without a nation: title value 0, no clubs listed\n", stderr);
    }
    return true;
}

}
