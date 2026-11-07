// Player Initialisation, the player record (+0x14) of a person: its three abilities and the potential.
#include "database_generation.hpp"
#include "database_init_detail.hpp"
#include "database_lookups.hpp"
#include "database_records.hpp"

namespace cm::aot::database::init {
namespace {
// The player record's 16-bit ability fields (inferred): current, potential and three more.
constexpr std::uint32_t current_ability = 0x4, potential_ability = 0x6, ability_a = 0x8, ability_b = 0xa, ability_c = 0xc;

}

// 100ba2f0: the three abilities (+0x8, +0xa, +0xc) come from each other, from the current ability (r15) and
// the potential (r28, -2/-1/0 marking a special draw), each drawn when outside 1-200, scaled for a row without
// a club (the +0x5e flag), and clamped to 1-200.
void Gen::player_abilities(std::uint32_t p, std::uint32_t player) {
    auto& current = g[15];
    auto& potential = g[28];
    auto& a8 = g[25];  // +0x8
    auto& ab = g[24];  // +0xa
    auto& ac = g[26];  // +0xc
    auto& luck = g[13];
    auto& second = g[27];
    auto& scale = g[16];
    const auto valid = [&](std::uint32_t r) { return in_rating_range(r, 1, 200); };
    const auto club = [&] { return c.u32(p + person::club); };
    const auto nation = [&] { return c.u32(p + person::nation); };

    current = u32(s16(c.u16(player + current_ability)));
    potential = u32(s16(c.u16(player + potential_ability)));
    if (rem_s(next(), 2000) == 0) potential = u32(-2);
    else if (rem_s(next(), 750) == 0) potential = u32(-1);
    else if (rem_s(next(), 100) == 0) potential = 0;
    a8 = u32(s16(c.u16(player + ability_a)));
    ab = u32(s16(c.u16(player + ability_b)));
    ac = u32(s16(c.u16(player + ability_c)));
    c.put8(player + 0x45, 0x0a);
    // 100ba44c: the nation's byte +0xac drives a luck value, drawn when it is 0.
    luck = u32(s8(u32(s16(c.u16(nation() + 0xac)))));
    if (s8(luck) == 0) {
        c.set({{3, c.toc(-0x7bb8)}, {4, c.toc(-0x5618) + 0x8ab}, {5, u32(s16(c.u16(nation() + 0xac)))}});
        call(0x1077b720, 0x100ba468);
        luck = u32(s8(u32(rem_s(next(), 10) + 1)));
    }
    if (club() != 0) {
        second = u32(s8(u32(s16(c.u16(club() + 0x1ec)))));
        if (s8(second) == 0) {
            if (s8(c.u8(at(0x304))) != 0) c.put8(at(0x304), 0);
            second = u32(s8(u32(rem_s(next(), 4) + 1)));
        }
    } else {
        second = u32(s8(u32(rem_s(next(), 2) + 1)));
    }
    scale = ext16(((s8(luck) + s8(second) * 3) >> 2) * 10);
    if (s16(scale) > 0x50) scale = 0x50;

    // The default for a missing current ability: a draw below `scale`, plus one, held to 25.
    const auto from_scale = [&](std::uint32_t& target) {
        luck = u32(below(s16(scale)));
        target = ext16(std::int32_t(luck) + 1);
        if (s16(target) > 25) target = 25;
    };
    // A row without a club keeps (1 - k) of each ability, k being 0.25 with the +0x5e flag and 0.6 without
    // (the fused multiply-add keeps one rounding: x - k * x). `base` is the first of three stack slots.
    const auto without_club = [&](std::uint32_t& target, std::uint32_t base, std::uint32_t site_flagged,
                                  std::uint32_t site_unflagged) {
        if (club() != 0) return;
        const bool flagged = c.u8(p + person::role_flag) != 0;
        const auto k = constant(flagged ? 0x28 : 0x30);
        const auto magic = constant(0x18);
        const auto value = s16(target);
        const auto first = converted(base, value);
        const auto x = fpu.fsub(first, magic);
        const auto second = converted(base + 8, value);
        const auto y = fpu.fsub(second, magic);
        const auto fused = fpu.multiply_add(flagged ? site_flagged : site_unflagged, k, x, y);
        const auto bits = fpu.fctiwz(0, fused);
        c.put64(at(base + 0x10), bits);
        c.set_f(1, magic);
        c.set_f(2, x);
        c.set_f(3, k);
        target = std::uint32_t(bits);
    };
    const auto hold = [&](std::uint32_t& target) {  // 1 to 200 (100babb0)
        const auto first = s16(target) >= 1 ? s16(target) : 1;
        target = ext16(first);
        const auto second_value = s16(target) > 200 ? 200 : s16(target);
        target = ext16(second_value);
    };
    // Each stage below reads `first/second` from fields drawn above.
    const auto lower = [&](std::uint32_t reg_value) { return s16(reg_value); };

    // 100ba5c8: +0xa
    if (!valid(ab)) {
        if (valid(a8) && club() != 0 && nation() == c.u32(club() + 0x2c)) {
            luck = u32(rem_s(next(), 10));
            const auto plus = rem_s(next(), 10);
            auto v = s16(u32((lower(a8) - std::int32_t(luck)) + plus));
            if (v < 1) v = 1;
            if (s16(u32(v)) > 200) v = 200;
            ab = ext16(v);
        } else if (valid(ac)) {
            if (s16(ac) < 0xb4) ab = ext16((s16(ac) + 0x14) - rem_s(next(), 10));
            else ab = ext16(rem_s(next(), 10) + 0xaa);
        } else if (s16(potential) == -1 && s16(current) > 100) {
            ab = ext16(rem_s(next(), 40) + 0x3c);
        } else if (valid(current)) {
            if (s16(current) > 0x4b) ab = ext16(s16(current) - rem_s(next(), 50));
            else ab = ext16(rem_s(next(), 25) + 1);
        } else {
            from_scale(ab);
        }
        without_club(ab, 0x2ec, 0x100bab48, 0x100baba0);
        hold(ab);
    }
    // 100babdc: +0x8
    if (!valid(a8)) {
        if (valid(ab)) {
            if (s16(ab) < 10) a8 = ext16(rem_s(next(), 5) + 1);
            else if (s16(ab) > 0xbe) a8 = ext16(rem_s(next(), 10) + 0xb9);
            else {
                luck = u32(rem_s(next(), 10));
                const auto plus = rem_s(next(), 10);
                a8 = ext16((s16(ab) - std::int32_t(luck)) + plus);
            }
        } else if (valid(ac)) {
            if (s16(ac) > 0xb4) a8 = ext16(rem_s(next(), 10) + 0xb4);
            else a8 = ext16((s16(ac) + 0x14) - rem_s(next(), 10));
        } else if (s16(potential) == -1) {
            a8 = ext16(rem_s(next(), 20) + 0x50);
        } else if (valid(current)) {
            if (s16(current) > 0x32) a8 = ext16(s16(current) - rem_s(next(), 50));
            else a8 = ext16(rem_s(next(), 25) + 1);
        } else {
            from_scale(a8);
        }
        without_club(a8, 0x2ec, 0x100bb1f0, 0x100bb248);
        hold(a8);
    }
    // 100bb284: +0xc
    if (!valid(ac)) {
        if (valid(a8)) {
            if (s16(a8) > 100) ac = u32(scaled(s16(a8), 0x28, 0x2ec, 0x2f4));
            else ac = ext16(rem_s(next(), 3) + 1);
        } else if (valid(ab)) {
            ac = u32(scaled(s16(ab), s16(ab) < 0xaf ? 0x38 : 0x28, 0x2ec, 0x2f4));
        } else if (valid(current)) {
            // 100bb3ec: a quarter of the current ability bounds a draw. Its zero test and the bound's
            // scalings are floating point; the bound is never zero for a value of 1-200 (a quarter) but may
            // truncate to 0, where the division answers 0.
            const auto magic = constant(0x18), k = constant(0x28), zero = constant(0x40);
            const auto quarter = fpu.fmul(k, fpu.fsub(converted(0x2ec, s16(current)), magic));
            c.set_f(2, magic);
            c.set_f(3, k);
            c.set_f(1, quarter);
            c.set_f(0, zero);
            fpu.fcmp(0, quarter, zero);
            if (quarter == zero) {
                luck = 0;
            } else {
                const auto sample = next();
                scaled(s16(current), 0x28, 0x2ec, 0x2f4);  // the bound against +65535
                scaled(s16(current), 0x28, 0x2ec, 0x2f4);  // and against -65535
                // 100bb6a4: both again, as the divisor and as the sign test.
                const auto first_minus = fpu.fsub(converted(0x2ac, s16(current)), magic);
                const auto first_product = fpu.fmul(k, first_minus);
                const auto divisor_bits = fpu.fctiwz(2, first_product);
                c.put64(at(0x2b4), divisor_bits);
                const auto second_minus = fpu.fsub(converted(0x2bc, s16(current)), magic);
                const auto second_product = fpu.fmul(k, second_minus);
                const auto sign_bits = fpu.fctiwz(0, second_product);
                c.put64(at(0x2c4), sign_bits);
                c.set_f(1, k);
                const auto divisor = std::int32_t(std::uint32_t(divisor_bits));
                const auto remainder = rem_s(sample, divisor);
                luck = u32(std::int32_t(std::uint32_t(sign_bits)) > 0 ? remainder : std::int32_t(0u - std::uint32_t(remainder)));
            }
            // 100bb73c
            scale = std::int32_t(luck) == 0 ? 0 : u32(below(std::int32_t(luck)));
            ac = ext16(std::int32_t(scale) + 1);
        } else {
            ac = ext16(div_s(s16(scale), 3));
            if (s16(ac) > 5) ac = 5;
        }
        without_club(ac, 0x2ac, 0x100bb938, 0x100bb990);
        hold(ac);
    }
    // 100bb9cc
    if (club() == 0 || c.u32(club() + 0x2c) == nation()) a8 = ab;
    player_age_and_dates(p, player);
}
}
