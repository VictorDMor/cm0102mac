#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;

std::uint32_t adjust_morale(Cpu& c, std::uint32_t player, std::uint32_t change_word) {
    Frame frame(c, 0x10288f30, 0x80, {.first_gpr = 24});
    const auto doubles = c.toc(-0x51e8), seed = c.toc(-0x7b90), begin = c.toc(-0x7b8c), end = c.toc(-0x7b88), cursor = c.toc(-0x7b84);
    keep(c, {{24, change_word}, {26, doubles}, {27, seed}, {31, player}, {28, begin}, {29, end}, {30, cursor}});
    const auto change = std::int8_t(change_word);
    if (compare(c, change) == 0) {
        c.set(3, c.u32(player + 0x1a4));
        if (compare(c, c.i32(c.r(3) + 0x3100), -1) == 0) {
            bool skip = compare(c, c.i16(player + 0x36), 7500) <= 0;
            if (!skip) skip = compare(c, c.i8(player + 0x17), 10) >= 0;
            if (skip) return result(c, 0);
        }
    }
    compare(c, change);
    const auto old_raw = c.u8(player + 0x17); const auto old = std::int8_t(old_raw);
    keep(c, {{25, old_raw}});
    const auto advance = [&](std::uint32_t back) {
        const auto next = c.u32(cursor) + 2;
        c.set(3, next); const auto ordering = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
        if (ordering > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
    };
    const auto threshold = [&](unsigned divisor, std::uint32_t back) {
        advance(back);
        const auto word = c.u16(c.u32(cursor)); const auto value = std::uint32_t(word) + c.u32(seed);
        const auto magic = divisor == 21 ? 0x30c30c31u : 0x2e8ba2e9u; const auto shift = divisor == 21 ? 2 : 1;
        const auto high = std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * magic) >> 32);
        c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << shift) - 1)));
        const auto remainder = std::int32_t(value) % int(divisor);
        leave(c, {{3, std::uint32_t(remainder)}, {4, std::uint32_t((high >> shift) < 0)}, {5, value}, {6, word}});
        return remainder;
    };
    const auto narrow_adjustment = [&](int bound, std::uint32_t back) {
        // A signed-byte change plus/minus one lies in [-129,128].
        // The expanded wide RNG arms are unreachable on both paths.
        c.set(3, std::uint32_t(std::int32_t(change)));
        c.s.wr<1>(reg::xer_ca, bound < 0); compare(c, bound);
        advance(back);
        const auto word = c.u16(c.u32(cursor)); const auto bias = c.u32(seed); const auto value = std::uint32_t(word) + bias;
        leave(c, {{3, value}, {4, std::uint32_t(bound)}, {5, bias}, {6, word}});
        c.s.wr<1>(reg::xer_ca, bound < 0); compare(c, bound);
        const auto remainder = std::int32_t(value) % bound;
        const auto draw = bound < 0 ? -remainder : remainder;
        keep(c, {{27, std::uint32_t(draw)}});
        return draw;
    };
    int value = std::int8_t(std::uint32_t(old_raw) + change_word);
    if (change > 0) {
        c.set(3, std::uint32_t(std::int32_t(old)));
        if (compare(c, old + change, 10) > 0) {
            const auto random = threshold(21, 0x10288fd8);
            if (compare(c, random, old + change) < 0) value = std::int8_t(int(old_raw) + narrow_adjustment(change + 1, 0x10289058));
        } else compare(c, change);
    } else if (compare(c, change) < 0) {
        c.set(3, std::uint32_t(std::int32_t(old)));
        if (compare(c, old + change, 10) < 0) {
            const auto random = threshold(11, 0x10289214);
            if (compare(c, random, old + change) > 0) value = std::int8_t(int(old_raw) + narrow_adjustment(change - 1, 0x10289294));
        }
    }
    c.set(6, std::uint32_t(value));
    const auto score = c.i16(player + 0x36);
    if (compare(c, score, 7500) >= 0) {
        Fpu fp(c);
        const auto convert = [&](int number, unsigned at) {
            c.put32(frame.at(at), 0x43300000); c.put32(frame.at(at + 4), std::uint32_t(number) ^ 0x80000000);
            return fp.fsub(c.f64(frame.at(at)), c.f64(doubles + 0x80));
        };
        c.set(3, std::uint32_t(std::int32_t(score)) ^ 0x80000000); c.set(4, 0x43300000); c.set(5, std::uint32_t(value));
        const auto converted_score = convert(score, 0x50), converted_morale = convert(value, 0x48);
        c.set_f(1, fp.fdiv(converted_score, c.f64(doubles + 0x3d8)));
        c.set_f(2, converted_morale); c.set_f(3, c.f64(doubles + 0x80));
        auto minimum = fp.fsub(c.f(1), c.f64(doubles + 0x3e0));
        fp.fcmp(0, converted_morale, minimum);
        if (c.cr(0) & 4) {
            c.set(3, std::uint32_t(value) ^ 0x80000000); c.set_f(1, c.f64(doubles + 0x80));
            minimum = convert(value, 0x58);
        } else {
            c.set(3, std::uint32_t(std::int32_t(score))); c.set_f(3, c.f64(doubles + 0x80));
            c.set_f(2, convert(score, 0x40)); c.set_f(1, fp.fdiv(c.f(2), c.f64(doubles + 0x3d8)));
            minimum = fp.fsub(c.f(1), c.f64(doubles + 0x3e0));
        }
        c.put64(frame.at(0x38), fp.fctiwz(0, minimum)); c.set(6, c.u32(frame.at(0x3c)));
        value = std::int8_t(c.r(6));
    }
    int morale;
    if (compare(c, value, 1) < 0) morale = 1;
    else morale = compare(c, value, 20) > 0 ? 20 : value;
    leave(c, {{4, std::uint32_t(morale)}, {6, std::uint32_t(morale)}});
    c.put8(player + 0x17, std::uint32_t(morale));
    return result(c, compare(c, morale, old) != 0);
}
}
