// MATCH_MAN.CPP callbacks block (match_man_callbacks.hpp): the side routines.
#include "match_man_callbacks.hpp"
#include "guest_values.hpp"
#include "match_man_callbacks_detail.hpp"
#include "match_man_detail.hpp"
#include "match_man_adapter.hpp"

namespace cm::aot::match_man {
using namespace guest;

// 0x10206cb0. Frame 0x50 saving r29-r31; r29 the side, r30 the flag, r31 the TOC word at
// -0x7a6c (the pointer to the squad registry global).
std::uint32_t release_side(Cpu& c, std::uint32_t side, std::uint32_t destroy) {
    Frame frame(c, 0x10206cb0, 0x50, {.first_gpr = 29});
    keep(c, {{31, c.toc(-0x7a6c)}, {30, destroy}, {29, side}});
    const auto registry = c.r(31);
    // `or.` compares the pointer as a signed word; the other tests are logical.
    if (compare(c, std::int32_t(side)) != 0) {
        const auto squad = c.u32(side + callback_side::squad);
        c.set(3, squad);
        if (compare_logical(c, squad) != 0 && compare_logical(c, c.u32(registry)) != 0) {
            // The squad is released unless it is held elsewhere: the check that follows
            // the side's kind byte answers 0 for the first call and non-0 for the second.
            bool release = compare(c, c.i8(side + callback_side::squad_kind)) == 0;
            if (!release) {
                c.set({{4, 1}, {5, 1}, {6, 0}, {7, 0}});
                c.call(0x1014c2e0, 0x10206d10);
                release = compare(c, std::int8_t(c.r(3))) == 0;
                if (!release) {
                    c.set(3, c.u32(c.toc(-0x7a54)));
                    c.set(4, c.u32(c.u32(side + callback_side::squad) + 0x90));  // side+0x34 reloaded (0x10206d1c)
                    c.call(0x101481d0, 0x10206d2c);
                    release = compare(c, std::int8_t(c.r(3))) != 0;
                }
            }
            if (release) {
                c.set({{3, c.u32(registry)}, {5, side + callback_side::squad_text}, {4, c.u32(side + callback_side::squad)},
                       {6, c.u8(side + callback_side::squad_kind)}});
                c.call(0x103b1c00, 0x10206d48);
            }
        }
        // `extsh.`: only the low halfword of the flag counts.
        if (compare(c, std::int16_t(destroy)) > 0) {
            c.set(3, side);
            c.call(0x107735a0, 0x10206d58);
        }
    }
    return result(c, side);
}

// 0x10206d80. Frame 0x60 saving r23-r31; r24..r31 hold the TOC words and counters the
// original keeps across its calls; r23 the member (unset before the first member).
std::uint32_t prepare_squad_actors(Cpu& c, std::uint32_t side) {
    Frame frame(c, 0x10206d80, 0x60, {.first_gpr = 23});
    keep(c, {{28, c.toc(-0x7b80)}, {25, side}, {29, c.toc(-0x7a64)}, {30, c.toc(-0x7aa4)},
             {31, c.toc(-0x7ba0)}, {24, c.toc(-0x7a6c)}});
    Kept<27> prepared(c, 0);
    Kept<26> index(c, 0);
    for (;; index = index + 1) {
        // The bound is reread every time: bench size (the high nibble) + 11.
        const auto extras = c.u32(side + callback_side::extras);
        const auto slot = std::int32_t(std::int8_t(index));
        c.set(4, std::uint32_t(slot));
        const auto bound = std::int32_t(std::int8_t((c.u8(extras + side_extras::bench_byte) >> 4) & 0xf)) + 11;
        if (compare(c, slot, bound) >= 0) break;

        c.set({{3, c.u32(c.r(24))}, {5, index}, {4, c.u32(side + callback_side::squad)},
               {6, c.u8(side + callback_side::squad_kind)}});
        c.call(0x103af800, 0x10206dcc);
        const auto member = c.r(3);
        c.set(23, member);
        if (compare(c, std::int32_t(member)) == 0) continue;

        c.set({{3, c.u32(c.r(31))}, {4, member}});
        c.call(0x10187680, 0x10206de0);
        if (compare(c, std::int8_t(c.r(3)), 2) == 0) continue;

        // A member suspended from this match is skipped (unless the side has no extras).
        const auto record = c.u32(side + callback_side::extras);
        c.set(5, record);
        if (compare(c, c.i32(record + side_extras::kind), callback_side::no_extras) != 0) {
            c.set({{3, c.u32(c.r(30))}, {4, member}, {5, c.u32(record + side_extras::suspension)}, {6, 1}, {7, 0}});
            c.call(0x100e7920, 0x10206e14);
            if (compare(c, std::int8_t(c.r(3))) != 0) continue;
        }

        c.set({{3, c.u32(c.r(29))}, {4, member}, {6, c.u32(side + callback_side::squad)}, {5, c.r(28)},
               {7, 0}, {8, 0}});
        c.call(0x102b7c70, 0x10206e38);
        if (compare_logical(c, c.r(3)) != 0) continue;

        c.set({{3, side}, {4, index}, {5, member}, {6, index}});
        call(c, 0x1020e8f0, 0x10206e54);
        if (compare(c, std::int8_t(c.r(3))) != 0) prepared = prepared + 1;
    }
    return result(c, prepared);
}

// 0x1020e8f0. Frame 0x80 saving r26-r31; r26 the side, r27 the index, r28 the staff, r29
// `tag`, r30 the TOC word -0x7ba8 (the message buffer), r31 the module's pool. The
// assertion's name and extension buffers are at sp+0x40 and sp+0x38.
std::uint32_t prepare_actor(Cpu& c, std::uint32_t side, std::uint32_t index, std::uint32_t staff, std::uint32_t tag) {
    Frame frame(c, 0x1020e8f0, 0x80, {.first_gpr = 26});
    keep(c, {{26, side}, {27, index}, {30, c.toc(-0x7ba8)}, {28, staff}, {31, c.toc(slots::pool)}, {29, tag}});
    const auto pool = c.r(31);
    const auto slot = std::int32_t(std::int8_t(index));
    const bool in_range = compare(c, slot) >= 0 && compare(c, slot, match_side::player_count - 1) <= 0;
    if (!compare(c, in_range)) {
        // The assertion formats the bad index first, then reports as every assert does.
        c.set({{3, c.toc(-0x7bb8)}, {4, pool + 0x3f}, {5, std::uint32_t(slot)}});
        c.call(0x1077b720, 0x1020e94c);
        report_assertion(c, {.at = 0x1020e960, .file = pool, .format = pool + 0xe, .title = pool + 0x1b,
                             .name = frame.at(0x40), .extension = frame.at(0x38), .line = 0xc37});
        c.set({{4, c.toc(-0x7bac)}, {3, 0xffffffff}});
        c.call(0x1076f7d0, 0x1020e9a8);  // exit(-1); where it returns, the original goes on
    }
    if (compare_logical(c, staff) == 0) return result(c, 0);
    const auto actor = c.u32(side + match_side::players) + std::uint32_t(slot) * match_player::size;
    c.set({{3, actor}, {4, c.u32(side + callback_side::club)}, {6, c.u8(side + callback_side::previous_slot)},
           {5, staff}, {7, index}, {8, tag}});
    c.call(entries::prepare_player, 0x1020e9e0);
    return result(c, 1);
}
}
