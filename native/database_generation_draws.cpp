// DATABASE.CPP's generation block, part 9: 0x100d3570 (adjust_by_draws) and 0x100da5b0 (generate_value),
// person values changed by draws of the sample pool (database_generation.hpp).
//
// Every draw here is the compiler's inline copy of range_sample for a bound that is a byte-sized expression
// (|bound| <= 384): advance the pool cursor (a wrap draws from rng_core), then |(sample + salt) % bound|. The
// original's arms for a bound beyond +-65535 (floating point and the call of range_sample) therefore cannot run
// and are not written. Registers: only those a call or the return can see are kept (the non-volatile ones the
// routine holds across the calls, r3-r6, cr0 and XER[CA]).
#include "database_generation.hpp"
#include "database_generation_detail.hpp"

namespace cm::aot::database {
using namespace generation;
namespace {
constexpr std::uint32_t player_field = 0x14;

std::int32_t byte_of(std::uint32_t register_value) { return std::int8_t(register_value); }
std::uint32_t extended(std::int32_t value) { return std::uint32_t(value); }
// The pool words as these two routines hold them: r23 the salt's address, r24 the start, r25 the end's address,
// r26 the cursor's address, r27 the constant pool.
Pool pool_in_registers_23(Cpu& c) {
    const auto pool = load_pool(c);
    c.set({{23, pool.salt_at}, {24, pool.start}, {25, pool.end_at}, {26, pool.cursor_at}, {27, pool.constants}});
    return pool;
}
// addic.: the carry goes to XER[CA] and the sum is compared with 0 in cr0.
std::int32_t add_immediate_recording(Cpu& c, std::uint32_t value, std::int32_t immediate) {
    const std::uint64_t total = std::uint64_t(value) + std::uint32_t(immediate);
    c.s.wr<1>(guest::reg::xer_ca, std::uint8_t(total >> 32));
    compare(c, std::int32_t(std::uint32_t(total)));
    return std::int32_t(std::uint32_t(total));
}
}

// 0x100d3570. A frame of 0x80 saving r21-r31. r31 the person, r28 the amount (a byte; the register is kept
// whole), r29 a flag, r30 the player's current value (player = person +0x14, value at +0x45), r21 the new value
// and r22 a draw.
// The value moves by the amount, and past 10 (up) or below 10 (down) a draw decides how far; the result is
// clamped to 1-20, and a positive amount with a flag stays at most the current value when the person's contract
// (CONTRACT_MANAGER) qualifies. The new value is stored; the answer is 1 if it changed.
std::uint32_t adjust_by_draws(Cpu& c, std::uint32_t person, std::uint32_t amount, std::uint32_t flag) {
    constexpr std::uint32_t entry = 0x100d3570;
    c.set({{3, person}, {4, amount}, {5, flag}});
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x80, {.first_gpr = 21});
    c.set({{28, amount}, {31, person}, {29, flag}});
    const auto pool = pool_in_registers_23(c);
    const auto delta = byte_of(amount);
    compare(c, delta);
    if (delta == 0) {
        c.set(3, 0);
        return 0;
    }
    const auto player = c.u32(person + player_field);
    const auto current = load8(c, 30, player + 0x45);
    const auto sum = byte_of(current) + delta;
    std::uint32_t value;  // the new value (r21) before the clamp
    if (delta > 0 && compare(c, sum, 10) > 0) {
        // Up past 10: a draw below 21 decides whether the full amount applies.
        advance(c, pool, entry + 0x84);
        const auto sample = std::uint32_t(c.u16(c.u32(pool.cursor_at)));
        c.set(6, sample);
        const auto total = sample + c.u32(pool.salt_at);
        const auto high = std::int32_t((std::int64_t(0x30c30c31) * std::int32_t(total)) >> 32);
        const auto shifted = std::int32_t(srawi(c, std::uint32_t(high), 2));  // divide by 4 sets XER[CA]
        const auto quotient = shifted + std::int32_t(std::uint32_t(shifted) >> 31);  // so total / 21 in all
        const auto residue = std::int32_t(total - std::uint32_t(quotient) * 21);
        if (compare(c, residue, sum) >= 0) {
            value = extended(byte_of(current + amount));
        } else {
            const auto bound = add_immediate_recording(c, extended(delta), 1);
            std::int32_t drawn = 0;  // a bound of 0 leaves the draw at 0 (r21 = 0)
            if (bound != 0) {
                advance(c, pool, entry + 0x104);
                drawn = draw_below(c, pool, bound, 6);
            }
            value = extended(byte_of(current + extended(drawn)));
        }
    } else if (compare(c, delta) < 0 && compare(c, sum, 10) < 0) {
        // Down below 10: the mean of three bytes of the person (+0x64, the player's +0x2f, +0x66) bounds a draw,
        // and the value only moves if that draw exceeds the sum.
        const auto total = byte_of(c.u8(person + 0x64)) + byte_of(c.u8(player + 0x2f)) + byte_of(c.u8(person + 0x66));
        const auto high = std::int32_t((std::int64_t(0x55555556) * total) >> 32);
        const auto mean = high + std::int32_t(std::uint32_t(high) >> 31);
        c.set(21, extended(mean));
        compare(c, mean);
        std::int32_t drawn = 0;
        if (mean != 0) {
            advance(c, pool, entry + 0x304);
            drawn = draw_below(c, pool, mean, 3);
        }
        c.set(22, extended(drawn));
        if (compare(c, drawn, sum) <= 0) {
            value = extended(byte_of(current + amount));
        } else {
            // subic. r0,delta,1: bound delta - 1 (never 0 for a negative delta); its carry is 1.
            const auto bound = add_immediate_recording(c, extended(delta), -1);
            std::int32_t second = 0;
            if (bound != 0) {
                advance(c, pool, entry + 0x488);
                second = draw_below(c, pool, bound, 6);
            }
            value = extended(byte_of(current + extended(second)));
        }
    } else {
        value = extended(byte_of(current + amount));
    }
    c.set(21, value);
    // Clamp to 1-20 (the compares leave cr0).
    std::int32_t clamped = byte_of(value);
    if (compare(c, clamped, 1) < 0) clamped = 1;
    else if (compare(c, clamped, 0x14) > 0) clamped = 0x14;
    c.set(21, extended(clamped));
    compare(c, byte_of(flag));
    std::int32_t result = clamped;
    if (byte_of(flag) != 0 && compare(c, delta) > 0) {
        c.set(3, c.u32(c.toc(-0x7b7c)));
        c.set(4, person);
        c.set(5, c.u32(person + 0x24));
        c.call_in_place(0x10082f90, entry + 0x668);
        if (compare_logical(c, c.r(3)) != 0 && compare(c, std::int32_t(c.u32(c.r(3) + 0x50))) != 0) {
            // With a qualifying contract the value stays at most the current one.
            result = compare(c, clamped, byte_of(current)) < 0 ? clamped : byte_of(current);
            c.set(21, extended(result));
        }
    }
    c.set(4, extended(result));
    const auto before = byte_of(current);
    c.set(5, c.u32(person + player_field));
    c.put8(c.r(5) + 0x45, c.r(21));
    c.set(3, extended(byte_of(((extended(before) - extended(result)) | (extended(result) - extended(before))) >> 31)));
    return c.r(3);
}

namespace {
// 0x100da5b0's six steps over the person's preferences record (person +0x18): a non-empty slot survives a draw
// below a bound taken from a byte of the person (+0x65 plus 15 for the first three slots, 35 less +0x69 for the
// rest) only when the draw is not 0. A bound of 0 draws 0.
struct Step {
    std::uint32_t slot, field;
    bool from_byte_plus_15;
    std::uint32_t draw_back;  // the return address of the draw's random call
    unsigned bound_register, result_register;  // r24 / r25 hold the bound and the draw, swapped from step to step
};
constexpr Step steps[] = {{0x1c, 0x65, true, 0x80, 24, 25}, {0x20, 0x65, true, 0x218, 25, 24},
                          {0x24, 0x65, true, 0x3b0, 25, 24}, {0x28, 0x69, false, 0x54c, 25, 24},
                          {0x2c, 0x69, false, 0x6e8, 25, 24}, {0x30, 0x69, false, 0x884, 24, 25}};
}

// 0x100da5b0. A frame of 0x70 saving r24-r31. r30 the person; r28/r27/r26/r31 the pool's salt, start, end and
// cursor words, r29 the constant pool.
std::uint32_t generate_value(Cpu& c, std::uint32_t person) {
    constexpr std::uint32_t entry = 0x100da5b0;
    c.set(3, person);
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x70, {.first_gpr = 24});
    compare(c, std::int32_t(person));
    const auto pool = load_pool(c);
    c.set({{28, pool.salt_at}, {27, pool.start}, {26, pool.end_at}, {31, pool.cursor_at}, {29, pool.constants},
           {30, person}});
    if (person == 0) return c.r(3);
    if (compare_logical(c, c.u32(person + 0x18)) == 0) return c.r(3);
    for (const auto& step : steps) {
        const auto preferences = load32(c, 3, person + 0x18);
        if (compare_logical(c, c.u32(preferences + step.slot)) == 0) continue;
        const auto byte = c.u8(person + step.field);
        // The first three bounds are the signed byte + 15 (addic.), the others 35 - the signed byte (subfic).
        std::int32_t bound;
        if (step.from_byte_plus_15) {
            c.set(3, extended(byte_of(byte)));
            bound = add_immediate_recording(c, extended(byte_of(byte)), 0xf);
        } else {
            const std::uint64_t total = std::uint64_t(~extended(byte_of(byte))) + 0x23 + 1;
            c.s.wr<1>(guest::reg::xer_ca, std::uint8_t(total >> 32));
            bound = std::int32_t(std::uint32_t(total));
            compare(c, bound);
        }
        c.set(step.bound_register, std::uint32_t(bound));
        std::int32_t draw = 0;
        if (bound == 0) {
            c.set(step.result_register, 0);
        } else {
            advance(c, pool, entry + step.draw_back);
            draw = draw_below(c, pool, bound, 3);
            c.set(step.result_register, extended(draw));
        }
        if (compare(c, draw) != 0) continue;
        c.set(3, c.u32(person + 0x18));
        c.put32(c.r(3) + step.slot, 0);
    }
    return c.r(3);
}
}
