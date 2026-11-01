// MATCH_ENG.CPP, team_a: a random mark character.
#include "match_eng_team_a.hpp"

namespace cm::aot::match_eng {
using namespace guest;

// 0x101ec7b0
std::uint32_t random_mark(Cpu& c) {
    Frame frame(c, 0x101ec7b0, 0x70, {.first_gpr = 31});
    const auto pool = c.toc(-0x52cc);
    keep(c, {{31, pool}});
    draw(c, 4, 0x101ec7cc);
    const auto draw = std::int32_t(c.r(3));
    // 0..3 pick '*', '-', '+', '.'.
    std::uint32_t mark = 0;
    auto order = compare(c, draw, 2);
    if (order == 0) {
        mark = '+';
    } else if (order > 0) {
        if (compare(c, draw, 4) < 0) mark = '.';
    } else {
        order = compare(c, draw, 0);
        if (order == 0) mark = '*';
        else if (order > 0) mark = '-';
    }
    if (mark) return result(c, mark);
    const Assertion assertion{.at = 0x101ec828, .file = pool + 0x45, .format = pool + 0x53, .title = pool + 0x60,
                              .name = frame.at(0x3d), .extension = frame.at(0x38), .line = 0x2f3a};
    report_assertion(c, assertion);
    c.set(3, u(-1));
    c.call(0x1076f7d0, 0x101ec870);
    return result(c, u(-1));
}
}
