#include "match_pl_detail.hpp"

namespace cm::aot::match_pl {
using namespace guest;

std::uint32_t adjust_action_bias(Cpu& c, std::uint32_t player, std::uint32_t bias) {
    Frame frame(c, 0x102857c0, 0x80, {.first_gpr = 22});
    const auto seed = c.toc(-0x7b90), begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    keep(c, {{27, c.toc(-0x51e8)}, {29, player}, {30, seed}, {24, bias}, {26, begin}, {25, end}, {31, cursor}});
    const auto role_bit = [&](std::uint32_t back, std::uint32_t bit) {
        enter(c, back, {player}); current_role(c, player);
        return compare(c, c.r(3) & bit) != 0;
    };
    if (role_bit(0x102857f0, 1)) return result(c, bias);
    const auto advance = [&](std::uint32_t back) {
        const auto next = c.u32(cursor) + 2;
        c.set(3, next); const auto ordering = compare_logical(c, next, c.u32(end));
        c.put32(cursor, next);
        if (ordering > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto sample = [&] { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); };
    const auto quotient_sign = [&](std::uint32_t value, std::uint32_t multiplier, unsigned shift) {
        const auto high = std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << shift) - 1)));
        return std::uint32_t((high >> shift) < 0);
    };
    // All three bound traits are signed bytes, excluding the original's
    // expanded wide-sampling branches and their recursive sample_range calls.
    const auto trait_sample = [&](unsigned field, unsigned bound_register, unsigned result_register, std::uint32_t back) {
        const auto profile = c.u32(player + 0x70); c.set(3, profile);
        const auto bound = c.i8(profile + field);
        c.set(bound_register, std::uint32_t(std::int32_t(bound)));
        std::int32_t draw = 0;
        if (compare(c, bound) != 0) {
            advance(back); const auto value = sample(); c.set(3, value); compare(c, bound);
            // Widen before remainder so INT32_MIN / -1 stays defined in C++.
            draw = std::int32_t(std::int64_t(std::int32_t(value)) % bound);
            if (bound < 0) draw = -draw;
        }
        c.set(result_register, std::uint32_t(draw));
        return draw;
    };
    const auto beats_random_threshold = [&](std::int32_t draw, std::uint32_t back) {
        advance(back); const auto value = sample();
        leave(c, {{3, quotient_sign(value, 0x63e7063f, 4)}, {4, value}});
        return compare(c, draw, std::int32_t(value) % 41) > 0;
    };
    const auto next = c.u32(cursor) + 2; c.set(3, next);
    keep(c, {{22, c.u8(player + 0x10d)}});
    const auto ordering = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
    if (ordering > 0) { c.put32(cursor, begin); c.call(0x1077b770, 0x10285824); c.put32(seed, c.r(3) & 0xffff); }
    const auto first = sample(); const auto residual = std::int32_t(first) % 21;
    leave(c, {{3, std::uint32_t(residual)}, {4, quotient_sign(first, 0x30c30c31, 2)}, {5, first}});
    if (compare(c, std::int8_t(c.r(22)) * 2, residual) >= 0) return result(c, std::uint32_t(std::int32_t(std::int8_t(c.r(24)))));

    bool forward = false;
    if (!role_bit(0x10285874, 0x80)) forward = beats_random_threshold(trait_sample(0x18, 23, 28, 0x102858b4), 0x10285a10);
    std::uint32_t aggregate;
    if (forward) {
        c.set(3, compare(c, c.i8(player + 0x27)) == 0 ? 1 : 7);
        aggregate = std::uint32_t(std::int32_t(std::int8_t(c.r(3))));
    } else aggregate = std::uint32_t(std::int32_t(std::int8_t(bias)));
    keep(c, {{28, aggregate}});

    bool central = false;
    if (!role_bit(0x10285a78, 0x200) && !role_bit(0x10285a88, 0x100) && !role_bit(0x10285a98, 0x400))
        central = beats_random_threshold(trait_sample(0x19, 22, 23, 0x10285ad8), 0x10285c34);
    aggregate += central ? 4u : std::uint32_t(std::int32_t(std::int8_t(bias)));
    keep(c, {{28, aggregate}});
    bool backward = false;
    if (!role_bit(0x10285c8c, 0x800)) backward = beats_random_threshold(trait_sample(0x17, 23, 22, 0x10285ccc), 0x10285e28);
    std::int32_t last;
    if (backward) { c.set(3, compare(c, c.i8(player + 0x27)) == 0 ? 7 : 1); last = std::int8_t(c.r(3)); }
    else last = std::int8_t(bias);
    aggregate += std::uint32_t(last); keep(c, {{28, aggregate}});
    const auto average = std::int8_t(aggregate) / 3;
    keep(c, {{24, std::uint32_t(average)}});
    return result(c, std::uint32_t(std::int32_t(std::int8_t(average))));
}
}
