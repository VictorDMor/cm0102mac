#pragma once
#include <cstdint>
#include <cstdio>

namespace cm::aot {

// STAFF_SCREENS.CPP's player-profile builders 0x10389ce0 and 0x103998e0 walk
// the offer indices their screen keeps in its value 3 across redraws and read
// each offer's club through its slot, testing neither. True for a null slot
// and for a slot that an ended offer left null: the original then read the
// club from address 8. `site` is the guarded instruction. Said once per site
// and run, since the fault it replaces was the only trace of the action that
// ends an offer under an open profile.
inline bool profile_offer_ended(std::uint32_t site, std::uint32_t slot_or_offer) {
    if (slot_or_offer != 0) return false;
    static std::uint32_t said[4]{};
    for (auto& entry : said) {
        if (entry == site) break;
        if (entry != 0) continue;
        entry = site;
        std::fprintf(stderr, "Player profile kept an ended transfer offer: skipped at 0x%x\n", site);
        break;
    }
    return true;
}

}
