#include "match_pl_ball_action_detail.hpp"

namespace cm::aot::match_pl::detail {
using namespace guest;

void BallAction::lose() {
    bool foul_comment = protected_action(0x1024a084);
    if (!foul_comment) foul_comment = compare(c, byte(0x41)) != 0;
    if (!foul_comment) {
        c.set(15, std::uint32_t(std::int32_t(c.i16(opponent() + 0x32)))); range(std::int32_t(c.r(15)), 14, 0x1024a0cc, 0x1024a1a4, true);
        foul_comment = compare(c, std::int32_t(c.r(14)), -10) < 0;
    }
    if (foul_comment) foul_comment = defensive_area(opponent());
    if (foul_comment) { advance(0x1024a2dc); foul_comment = compare(c, binary()) == 0; }
    if (foul_comment) {
        style_message(true); advance(0x1024a558); const auto choice = modulo(50);
        bool one = compare(c, choice, 1) == 0, ordinary = false;
        if (!one) ordinary = choice < 1 ? compare(c, choice) >= 0 : compare(c, choice, 6) < 0;
        side_message(one ? 0x20ea : ordinary ? 0x20eb : 0x20e8, unsigned(std::int8_t(c.u8(opponent() + 0x27)) == 0),
                     one ? 0x1024a624 : ordinary ? 0x1024a5e8 : 0x1024a660);
    } else {
        compare_logical(c, std::uint32_t(byte(0x45)), 46);
        switch (byte(0x45)) {
        case 1:
            enter(c, 0x1024a69c, {opponent(), std::uint32_t(std::int32_t(std::int16_t(c.i16(at(0x4a)) - 200)))});
            react_to_contact_failure(c, opponent(), std::int16_t(c.r(4))); if (compare(c, std::int8_t(c.r(3))) == 0) return; break;
        case 19: case 33: case 45: paired_message(0x1f62, 0x1024a700); break;
        case 34: case 44: paired_message(0x2150, 0x1024a72c); break;
        case 41: paired_message(0x1f61, 0x1024a6d4); break;
        case 42: case 43: paired_message(0x1f64, 0x1024a758); break;
        case 21: case 46: {
            advance(0x1024a77c); const auto draw = modulo(6); c.put32(at(0x94), 0x43300000);
            c.set_f(1, c.f32(singles + 0xe4)); c.set_f(0, c.f32(opponent() + 0xb0)); c.set_f(2, c.f64(doubles + 0x80));
            c.set_f(3, fp.fadds(c.f(1), c.f(0))); c.set_f(0, c.f32(player() + 0xb0)); c.put32(at(0x98), std::uint32_t(draw) ^ 0x80000000);
            c.set_f(1, fp.fsubs(c.f64(at(0x94)), c.f(2))); c.set_f(1, fp.fadds(c.f(3), c.f(1))); fp.fcmp(0, c.f(1), c.f(0));
            const bool shorter = (c.cr(0) & 8) != 0; paired_message(shorter ? 0x2164 : 0x1f5d, shorter ? 0x1024a810 : 0x1024a83c); break;
        }
        }
    }
    enter(c, 0x1024a844, {opponent()}); acquire_ball(c, opponent());
    c.put16(opponent() + 0x1a0, std::uint32_t(c.i16(at(0x4a))) - 400);
    if (compare(c, c.i16(opponent() + 0x1a0), 200) > 0) c.put16(opponent() + 0x1a0, 200);
    else if (compare(c, c.i16(opponent() + 0x1a0), -200) < 0) c.put16(opponent() + 0x1a0, std::uint32_t(-200));
    const auto action = byte(0x45); c.set(14, 0); c.set(4, 100);
    if (compare(c, action, 33) == 0 || (action > 33 && compare(c, action, 45) == 0)) c.set(4, 250);
    else if (action < 33 && compare(c, action, 19) == 0) c.set(4, 400);
    bool request = compare(c, c.i16(opponent() + 0x1a0), std::int16_t(c.r(4))) < 0;
    if (request) request = compare(c, c.u8(match() + 0x791f), 4) != 0;
    if (request) request = compare(c, c.i8(match() + 0x791f), -1) != 0;
    if (request) {
        leave(c, {{3, match()}, {4, c.u8(player() + 0x105)}, {5, c.u8(player() + 0x106)},
                  {6, 1}, {7, unsigned(std::int8_t(actor.side()) == 0)}, {8, 0}, {9, 1}, {10, 0}}); c.call(0x101f1100, 0x1024a924);
        if (compare(c, c.i8(match() + 0x7928)) == 0) {
            c.set(14, c.u8(opponent() + 0x10f)); c.set(15, c.u8(opponent() + 0x110)); advance(0x1024a95c);
            c.set(5, sample()); c.set(6, c.u16(c.u32(cursor))); const auto first = c.r(5), next = c.u32(cursor) + 2;
            c.set(3, next); c.put32(cursor, next); c.set(4, std::uint32_t(high(first, 0x66666667)));
            const auto order = compare_logical(c, next, c.u32(end)), first_high = std::int32_t(c.r(4));
            shifted_sign(first_high, 2); c.set(3, unsigned(first_high < 0)); c.set(16, std::uint32_t(std::int32_t(first) % 10));
            if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, 0x1024a9b4); c.put32(seed, c.r(3) & 0xffff); }
            c.set(3, c.u32(cursor)); c.set(4, 0x66666667); c.set(5, c.u32(seed)); c.set(7, c.u16(c.r(3)) + c.r(5));
            const auto second_high = high(c.r(7), 0x66666667); shifted_sign(second_high, 2); c.set(5, unsigned(second_high < 0));
            c.set(4, std::uint32_t(std::int32_t(c.r(7)) / 10) * 10); c.set(22, std::uint32_t(std::int32_t(c.r(7)) % 10));
            const auto better = unsigned(std::int32_t(std::int8_t(c.r(14))) > std::int32_t(c.r(16)));
            const auto other = unsigned(std::int32_t(std::int8_t(c.r(15))) > std::int32_t(c.r(22)));
            const auto x = std::uint32_t(std::int32_t(std::int8_t(c.r(14)))) ^ c.r(16);
            const auto y = std::uint32_t(std::int32_t(std::int8_t(c.r(15)))) ^ c.r(22);
            c.set(4, y & std::uint32_t(std::int32_t(std::int8_t(c.r(15))))); c.set(5, std::uint32_t(std::int32_t(y) >> 1));
            c.set(6, std::uint32_t(std::int32_t(std::int8_t(c.r(15)))));
            const auto bound = 3u - better - other; c.s.wr<1>(reg::xer_ca, 1);
            // The two comparisons reduce this bound to1..3. The original
            // zero/wide paths cannot run, even for negative seed words.
            range(std::int32_t(bound), 17, 0x1024aa50, 0x1024ac08, true);
            // Keep the caller image consumed by the following role queries.
            // The draw has already read the pool/seed; do not read them again.
            const auto word = c.r(3); c.set(9, word); c.set(8, bound); c.set(7, std::uint32_t(divide_signed<4>(word, bound)));
            c.set(6, std::uint32_t(std::int32_t(y) >> 1));
            c.set(4, c.r(6) - (y & std::uint32_t(std::int32_t(std::int8_t(c.r(15))))));
            c.set(5, c.r(7) * bound); c.set(3, other);
            shifted_sign(std::int32_t(x), 1); c.s.wr<1>(reg::xer_ca, 1); compare(c, std::int32_t(bound));
            c.put8(opponent() + 0x2c, c.r(17)); c.set(14, 1);
        } else if (compare(c, byte(0xec)) != 0) { enter(c, 0x1024ad68, {player()}); publish_incident(c, player()); }
    } else if (compare(c, c.i8(match() + 0x791f), 4) < 0) {
        if (compare(c, c.i8(match() + 0x791f), -1) > 0) c.put8(match() + 0x791f, 0);
        bool first_side = compare(c, c.i8(opponent() + 0x27)) == 0;
        if (first_side) first_side = compare(c, c.i8(match() + 0x791f), -1) != 0;
        const auto row = actor.row(); const auto projected = first_side ? (compare(c, row, 9) < 0 ? row + 3 : 11) : (compare(c, row, 3) > 0 ? row - 3 : 0);
        enter(c, first_side ? 0x1024adfc : 0x1024ae3c, {opponent(), c.u8(player() + 0x105), c.u8(player() + 0x106), 4, std::uint32_t(std::int32_t(std::int8_t(projected)))});
        set_heading(c, opponent(), c.r(4), c.r(5), 4, c.r(7));
        const auto elapsed = c.i16(match() + 0x7948); const auto product = high(std::uint32_t(std::int32_t(elapsed)), 0x2e8ba2e9);
        shifted_sign(product, 1); c.set(3, unsigned(product < 0)); c.set(15, std::uint32_t(std::int32_t(std::int8_t(elapsed / 11))));
        const bool special = compare(c, byte(0x45), 21) == 0; const auto attempts = special ? 7u : 4u, successes = special ? 8u : 5u;
        if (compare_logical(c, c.u8(opponent() + attempts)) != 0) {
            c.set(5, c.u8(opponent() + successes)); c.set(3, c.u8(opponent() + attempts)); c.set(4, 0x43300000);
            c.put32(at(0x98), c.r(5)); c.set_f(2, c.f64(doubles + 0xf0)); c.put32(at(0x94), 0x43300000); c.set_f(1, c.f64(doubles + 0xf0));
            c.set_f(0, c.f64(at(0x94))); c.put32(at(0xa0), c.r(3)); c.set_f(2, fp.fsubs(c.f(0), c.f(2))); c.put32(at(0x9c), 0x43300000);
            c.set_f(0, c.f64(at(0x9c))); c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.set_f(30, fp.fdivs(c.f(2), c.f(0)));
        } else c.set_f(30, c.f32(singles));
        advance(special ? 0x1024aee0 : 0x1024b044);
        const auto sample_cursor = c.u32(cursor), sample_seed = c.u32(seed);
        c.set(5, std::uint32_t(c.u16(sample_cursor)) + sample_seed); shifted_sign(high(c.r(5), 0x66666667), 3);
        c.set(3, std::uint32_t(std::int32_t(std::int8_t(c.r(15))))); c.set(4, std::uint32_t(std::int32_t(c.r(5)) % 20));
        const auto threshold = std::int8_t(c.r(4) + (special ? 55 : 65));
        if (compare(c, std::int8_t(c.r(15)), threshold) > 0) {
            const bool second = compare(c, c.i8(opponent() + 0x27)) != 0;
            bool announce = compare(c, c.i8(c.u32(opponent() + 0x1a4) + (second ? 0xe03d : 0xe073))) == 0;
            if (announce) { c.set_f(0, c.f64(doubles + 0x288)); fp.fcmp(0, c.f(30), c.f(0)); announce = (c.cr(0) & 4) != 0; }
            if (announce) announce = compare_logical(c, c.u8(opponent() + (special ? 8 : 6)), special ? 3 : 1) > 0;
            if (announce && special) announce = compare_logical(c, c.u8(opponent() + 9)) != 0;
            if (announce) announce = compare(c, c.i16(opponent() + 0x36), 6750) > 0;
            if (announce) announce = compare(c, c.i8(opponent() + 0x100)) == 0;
            if (announce) { paired_message(special ? 0x21ce : 0x21cd, special ? 0x1024afc8 : 0x1024b120); c.put8(opponent() + 0x100, 1); }
        }
    }
    c.set(4, c.u8(opponent() + 5)); c.put8(opponent() + 5, c.r(4) + 1);
    enter(c, 0x1024b13c, {opponent()}); current_role(c, opponent()); bool ordinary = compare(c, int(c.r(3) & 4)) == 0;
    if (ordinary) { enter(c, 0x1024b14c, {opponent()}); current_role(c, opponent()); ordinary = compare(c, int(c.r(3) & 2)) == 0; }
    const bool delayed = compare(c, std::int8_t(c.r(14))) != 0; condition_delta(opponent(), (delayed ? 120 : 200) * (ordinary ? 2 : 1));
    c.put8(player() + 0x2c, std::uint32_t(c.u8(player() + 0x2c)) + c.u8(at(0x47)));
    enter(c, 0x1024b1b4, {opponent()}); near_own_goal(c, opponent());
    if (compare(c, std::int8_t(c.r(3))) != 0 && compare(c, c.i16(player() + 0x2a)) > 0) c.put8(opponent() + 6, std::uint32_t(c.u8(opponent() + 6)) + 1);
    if (compare(c, std::int8_t(c.r(14))) != 0) { advance(0x1024b1fc); if (compare(c, modulo(7)) == 0) target_message(0x1f41, opponent(), 0x1024b264); }
}
}
