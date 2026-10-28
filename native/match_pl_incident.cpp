#include "match_pl_detail.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void publish_incident(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10288230, 0x60, {.first_gpr = 26});
    const auto seed = c.toc(-0x7b90), begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    keep(c, {{31, player}, {26, seed}, {27, begin}, {30, UINT32_MAX}, {28, end}, {29, cursor}});
    const auto publish = [&](std::uint32_t message, std::uint32_t back, bool team_only = false) {
        const auto match = c.u32(player + 0x1a4);
        leave(c, {{3, c.u32(match + 0x79f4)}, {4, message}, {5, match},
                  {6, team_only ? 0u : c.u8(player + 0x2f)}, {7, team_only ? 0u : c.u8(player + 0x27)},
                  {8, UINT32_MAX}, {9, UINT32_MAX}, {10, 0}});
        c.call(0x10201180, back);
    };
    c.put8(frame.at(0x38), 0); publish(0x20dd, 0x10288288);
    leave(c, {{3, c.u32(player + 0x1a4)}, {4, player}, {5, frame.at(0x38)}, {6, 6}, {7, 0}});
    c.call(0x101f51e0, 0x102882a0);
    const auto status = c.i8(frame.at(0x38));
    if (compare(c, status, 4) == 0) keep(c, {{30, 0x1ff8}});
    else if (status < 4) {
        if (compare(c, status, 2) == 0) keep(c, {{30, 0x1ff9}});
        else if (status >= 2) keep(c, {{30, 0x1ff7}});
    } else if (compare(c, status, 9) == 0) keep(c, {{30, 0x210a}});
    const auto advance = [&](std::uint32_t back) {
        const auto next = c.u32(cursor) + 2;
        c.set(3, next); const auto ordering = compare_logical(c, next, c.u32(end));
        c.put32(cursor, next);
        if (ordering > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto sample = [&] { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); };
    if (compare(c, std::int16_t(c.r(30)), -1) > 0) {
        advance(0x10288318);
        auto sum = sample(); c.set(3, std::uint32_t(std::int32_t(sum) < 0));
        if (compare(c, std::int32_t(sum) % 8) != 0) {
            advance(0x10288368); sum = sample();
            const auto high = std::int32_t(std::uint64_t(std::int64_t(std::int32_t(sum)) * 0x55555556) >> 32);
            leave(c, {{3, std::uint32_t(high)}, {4, sum}});
            if (compare(c, std::int32_t(sum) % 3) != 0) {
                auto match = c.u32(player + 0x1a4);
                const bool other_side = c.i8(player + 0x27) == 0; compare(c, other_side);
                auto opponent = c.u32(c.u32(match + 0x3118) + (other_side ? 0x20 : 0x1c));
                c.set(3, opponent);
                if (compare_logical(c, c.u32(opponent + 0x2c)) != 0) {
                    match = c.u32(player + 0x1a4);
                    const bool next_side = c.i8(player + 0x27) == 0; compare(c, next_side);
                    opponent = c.u32(c.u32(match + 0x3118) + (next_side ? 0x20 : 0x1c));
                    c.set(3, c.u32(opponent + 0x2c));
                    if (compare(c, c.u8(c.r(3) + 0x119), 8) == 0) publish(0x20f5, 0x10288444, true);
                }
            }
            publish(0x2109, 0x1028846c);
        }
        publish(c.r(30), 0x10288494);
    }
    const auto raw_status = c.u8(frame.at(0x38));
    bool remove = compare(c, raw_status, 3) == 0;
    if (!remove) remove = compare(c, c.u8(frame.at(0x38)), 4) == 0;
    if (remove) { leave(c, {{3, c.u32(player + 0x1a4)}, {4, player}, {5, 0}}); c.call(0x101f1530, 0x102884bc); }
}
}
