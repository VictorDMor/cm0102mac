// MATCH_MAN.CPP's leaves (match_man_leaves.hpp).
#include "match_man_leaves.hpp"
#include "match_man_detail.hpp"

namespace cm::aot::match_man {
// 0x102107c0: walks the side's 20 player records (side+0x28, 0x1c4 each) and
// returns the first whose person (+0x6c) is `person`, else 0. The count is
// compared as a signed byte (extsb). Left as the original leaves them: r5 the
// count, r0 the last value compared, cr0 that compare.
std::uint32_t side_player_for_person(Cpu& c, std::uint32_t side, std::uint32_t person) {
    auto player = c.u32(side + match_side::players);
    std::uint32_t count = 0;
    for (;;) {
        const auto counted = std::int32_t(std::int8_t(count));
        c.set(0, std::uint32_t(counted));
        c.compare(0, counted, match_side::player_count);
        if (counted >= match_side::player_count) { player = 0; break; }
        const auto who = c.u32(player + match_player::person);
        c.set(0, who);
        c.compare_logical(0, who, person);
        if (who == person) break;
        ++count;
        player += match_player::size;
    }
    c.set(3, player);
    c.set(5, count);
    return player;
}
}
