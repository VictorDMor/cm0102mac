#include "match_pl_ball_action_detail.hpp"

namespace cm::aot::match_pl::detail {
using namespace guest;

void BallAction::style_message(bool lost) {
    compare_logical(c, std::uint32_t(byte(0x45)), 46);
    unsigned id = 0x1fe9; std::uint32_t back = lost ? 0x1024a538 : 0x10248d60;
    switch (byte(0x45)) {
    case 1: id = 0x1ff1; back = lost ? 0x1024a488 : 0x10248cb0; break;
    case 12: id = 0x1ff4; back = lost ? 0x1024a50c : 0x10248d34; break;
    case 19: case 33: id = 0x1fee; back = lost ? 0x1024a404 : 0x10248c2c; break;
    case 21: id = 0x1ff0; back = lost ? 0x1024a4b4 : 0x10248cdc; break;
    case 34: id = 0x1ff2; back = lost ? 0x1024a430 : 0x10248c58; break;
    case 41: id = 0x1fef; back = lost ? 0x1024a45c : 0x10248c84; break;
    case 42: id = 0x1feb; back = lost ? 0x1024a380 : 0x10248ba8; break;
    case 43: id = 0x1fec; back = lost ? 0x1024a3ac : 0x10248bd4; break;
    case 44: id = 0x1fed; back = lost ? 0x1024a3d8 : 0x10248c00; break;
    case 45: id = 0x1fea; back = lost ? 0x1024a354 : 0x10248b7c; break;
    case 46: id = 0x1ff3; back = lost ? 0x1024a4e0 : 0x10248d08; break;
    }
    paired_message(id, back);
}

bool BallAction::contest() {
    if (compare_logical(c, opponent()) == 0) return false;
    c.set(3, std::uint32_t(std::int32_t(c.i16(player() + 0x1a0))));
    c.set(4, std::uint32_t(std::int32_t(c.i16(at(0x4a)))));
    if (compare(c, std::int16_t(c.r(4)), std::int32_t(c.r(3)) + 200) < 0) return false;
    leave(c, {{3, match()}, {4, opponent()}, {5, player()}, {6, c.u8(at(0x45))},
              {7, 108}, {8, at(0x43)}, {9, at(0x42)}, {10, at(0x41)}}); c.call(0x101f5050, 0x10248760); c.put8(at(0x40), c.r(3));
    if (compare(c, c.i8(match() + 0x7931)) != 0 && opposite_area(opponent())) {
        bool complain = protected_action(0x1024883c);
        if (!complain) complain = compare(c, byte(0x41)) != 0;
        if (!complain) {
            c.set(15, std::uint32_t(std::int32_t(c.i16(opponent() + 0x32)))); range(std::int32_t(c.r(15)), 14, 0x10248884, 0x1024895c, true);
            complain = compare(c, std::int32_t(c.r(14)), -10) < 0;
        }
        if (complain) { advance(0x102489e8); if (compare(c, modulo(3)) != 0) target_message(0x2168, opponent(), 0x10248a48); }
    }
    bool event_only = false;
    if (compare(c, byte(0x40)) != 0) {
        advance(0x10248a74);
        if (compare(c, modulo(10)) == 0 && compare(c, byte(0x44)) == 0) {
            c.put32(at(0x38), c.u8(at(0x43))); c.put32(at(0x3c), c.u8(at(0x42)));
            leave(c, {{3, match()}, {4, std::uint32_t(std::int32_t(byte(0x45)))}, {5, opponent()}, {6, player()},
                      {7, c.u8(at(0x44))}, {8, c.u8(at(0x40))}, {9, c.u8(player() + 0x105)}, {10, c.u8(player() + 0x106)}});
            c.call(0x101d68f0, 0x10248af4); event_only = true;
        } else { foul(); return true; }
    }
    if (!event_only && compare(c, byte(0x41)) != 0) {
        bool incident = opposite_area(player()); if (!incident) incident = compare(c, byte(0x44)) != 0;
        if (incident) c.put8(at(0xec), 1);
    }
    c.put16(opponent() + 0x19c, std::uint32_t(std::int32_t(byte(0x45))));
    c.set(3, c.u8(opponent() + 4)); c.put8(opponent() + 4, c.r(3) + 1);
    leave(c, {{3, match()}, {4, opponent()}}); c.call(0x101e3d50, 0x1024a03c);
    bool specialized = compare(c, int(c.r(3) & 0x30)) != 0;
    if (specialized) { leave(c, {{3, match()}, {4, opponent()}}); c.call(0x101e3d50, 0x1024a050); specialized = compare(c, int(std::uint16_t(c.r(3)) & 0x880)) != 0; }
    condition_delta(opponent(), specialized ? -59 : -70); lose(); return true;
}

void BallAction::disciplinary_queue() {
    leave(c, {{3, c.u32(c.toc(-0x7b3c))}, {4, c.u32(opponent() + 0x70)}, {5, 11}, {6, 2}});
    c.set_f(1, c.f32(singles + 4)); c.call(0x10303560, 0x10248e40); c.put64(at(0x94), fp.fctiwz(0, c.f(1))); c.set(14, c.u32(at(0x98)));
    if (compare_logical(c, match()) == 0 || compare_logical(c, c.u32(match() + 0x3114)) == 0) return;
    if (compare(c, c.u8(at(0x40)), 5) != 0 && compare(c, c.u8(at(0x40)), 4) != 0) return;
    if (compare(c, c.u8(at(0x43)), 10) != 0 && compare(c, c.u8(at(0x43)), 5) != 0 && compare(c, c.u8(at(0x43)), 11) != 0) return;
    if (compare_logical(c, c.u32(opponent() + 0x6c)) == 0 || compare_logical(c, c.u32(opponent() + 0x6c)) == 0) return;
    if (compare_logical(c, c.u32(opponent() + 0x6c)) != 0 && compare(c, c.i32(c.u32(opponent() + 0x6c)), c.i32(c.toc(-0x7be4))) >= 0) return;
    if (compare_logical(c, c.u32(player() + 0x6c)) != 0 && compare(c, c.i32(c.u32(player() + 0x6c)), c.i32(c.toc(-0x7be4))) >= 0) return;
    if (compare_logical(c, c.u32(opponent() + 0xfc)) == 0) return;
    leave(c, {{3, c.u32(opponent() + 0xfc)}, {4, 0}, {5, 1}, {6, 0}, {7, 0}}); c.call(0x1014c2e0, 0x10248f28);
    if (compare(c, std::int8_t(c.r(3))) != 0 || compare(c, c.i8(c.u32(opponent() + 0x6c) + 0x67), int(std::int8_t(c.r(14))) + 10) >= 0) return;
    const auto enough = [&](unsigned offset, std::uint32_t back) {
        advance(back); const auto pool_word = c.u16(c.u32(cursor)); const auto seed_word = c.u32(seed);
        const auto attribute = c.i8(c.u32(opponent() + 0x70) + offset); const auto word = std::uint32_t(pool_word) + seed_word;
        const auto draw = std::int32_t(word) % 5; c.set(5, shifted_sign(high(word, 0x66666667), 1));
        c.set(3, std::uint32_t(std::int32_t(attribute))); c.set(4, std::uint32_t(draw)); c.set(6, word);
        c.s.wr<1>(reg::xer_ca, c.r(14) <= 15);
        return compare(c, attribute, std::int8_t(15u - c.r(14) + std::uint32_t(draw))) > 0;
    };
    if (!enough(0x1c, 0x10248f6c) || !enough(0x25, 0x10248fe4)) return;
    advance(0x1024905c); c.set(6, c.u16(c.u32(cursor))); const auto seed_word = c.u32(seed);
    const auto rating = c.i16(c.u32(opponent() + 0x70) + 0xa); const auto word = c.r(6) + seed_word;
    const auto performance = std::int32_t(word) % 3500; shifted_sign(high(word, 0x2572fb07), 9);
    c.set(3, std::uint32_t(std::int32_t(rating))); c.set(4, std::uint32_t(performance)); c.set(5, word);
    if (compare(c, rating, std::int16_t(3000 + performance)) <= 0) return;
    advance(0x102490cc); const auto last_cursor = c.u32(cursor), last_seed = c.u32(seed);
    c.set(5, std::uint32_t(c.u16(last_cursor)) + last_seed); c.set(4, std::uint32_t(std::int32_t(c.r(5)) % 3));
    c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(14)))));
    if (compare(c, std::int8_t(c.r(14)), std::int32_t(c.r(4)) + 1) <= 0) return;
    c.put32(at(0x78), c.u32(c.u32(opponent() + 0x6c))); c.put32(at(0x7c), c.u32(c.u32(opponent() + 0xfc)));
    c.put32(at(0x90), c.u32(c.u32(player() + 0x6c))); c.put8(at(0x84), 0);
    enter(c, 0x10249144, {player()}); side(c, player()); c.set(8, 0); c.set(7, UINT32_MAX); c.set(6, 0x82); c.put8(at(0x85), c.r(3));
    // Descriptor padding87/8e/8f is inherited from this activation.
    c.put8(at(0x86), 0); c.put32(at(0x88), UINT32_MAX); c.put16(at(0x8c), 0x82); c.put32(at(0x80), 0);
    leave(c, {{3, c.u32(match() + 0x3114)}, {4, at(0x78)}, {5, 0x1c}}); c.call(0x10738d10, 0x1024917c);
}
}
