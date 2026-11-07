// DATABASE.CPP's people block, part "attributes": 0x100c2800 and 0x100c2cb0 settle a person's attribute slots,
// 0x100c2520 rescales every person's slots and calls them (database_people_attributes.hpp). No random draws.
// The two settling routines share their loops (settle_slots); they differ in the start (the second derives its
// base from weighted attribute bytes), in two slots the second skips, in the weight of two slots and in the
// work after the slots go back. Everything the game can observe is the original's: memory (the frame's
// slots, flags and floating-point conversion words), the floating-point registers and FPSCR, XER[CA] (srawi,
// subfc/subfe and the abs leaf), and the registers its callees save.
#include "database_generation.hpp"
#include "database_generation_detail.hpp"
#include "database_people_attributes.hpp"
#include "database_people_detail.hpp"
#include "guest_fpu.hpp"
#include <algorithm>
#include <bit>

namespace cm::aot::database {
using namespace people;
namespace {
using guest::Fpu;

constexpr std::uint32_t settle_entry = 0x100c2800, weighted_entry = 0x100c2cb0, rescale_entry = 0x100c2520;
constexpr std::uint32_t abs_leaf = 0x10775540, hook_notify = 0x103d3f70;
constexpr std::int32_t hook_slot = -0x7a7c, pool_slot = -0x5614;  // TOC words: the hook word's address, the constants
constexpr std::uint32_t player_record = 0x14, attribute_record = 0x1c;  // a person's records (inferred)
constexpr std::uint32_t player_kind = 0xf;  // inferred: the player's position/kind byte
constexpr std::uint32_t conversion_high = 0x43300000;  // the compiler's int-to-double conversion (see database_ratings.cpp)
constexpr std::int32_t terminator = -0x7f;             // ends a slot array
constexpr std::uint32_t slot_size = 4, slot_total = 63;

// Pool constants (doubles at the pool pointer + offset).
constexpr std::uint32_t pool_conversion = 0x18;

std::int32_t extsh(std::uint32_t v) { return std::int16_t(v); }
std::int32_t byte_of(std::int32_t v) { return std::int8_t(v); }  // `extsb`

double converted(std::uint32_t flipped) { return std::bit_cast<double>(std::uint64_t(conversion_high) << 32 | flipped); }
// The compiler's integer to double: the words 0x43300000 and value ^ 0x80000000 stored at `at`, loaded as a double.
double convert_via_stack(Cpu& c, std::uint32_t at, std::int32_t value) {
    const auto flipped = std::uint32_t(value) ^ 0x80000000u;
    c.put32(at, conversion_high);
    c.put32(at + 4, flipped);
    return converted(flipped);
}

// The frame all three routines of this file share: slot values (2 bytes) with a flag byte at +2 of 4 bytes each,
// per-slot flag bytes of the settling, and three count bytes the attribute copy fills.
struct Slots {
    Cpu& c;
    std::uint32_t array, flags;
    std::uint32_t at(std::int32_t index) const { return array + std::uint32_t(index) * slot_size; }
    std::int32_t get(std::int32_t index) const { return c.i16(at(index)); }
    void set(std::int32_t index, std::int32_t value) const { c.put16(at(index), std::uint32_t(value)); }
    std::int32_t flag(std::int32_t index) const { return c.i8(flags + std::uint32_t(index)); }
    void set_flag(std::int32_t index, std::uint32_t value) const { c.put8(flags + std::uint32_t(index), value); }
    // The byte at +2 of the slot (a mark the copy sets).
    std::int32_t mark(std::int32_t index) const { return c.i8(at(index) + 2); }
    void fill_with_terminator() const {
        for (std::uint32_t i = 0; i < slot_total; ++i) {
            c.put16(array + i * slot_size, std::uint32_t(terminator));
            c.put8(array + i * slot_size + 2, std::uint32_t(terminator));
        }
    }
};

// Calls the hook word's routine 0x103d3f70 (when the word is set) with the word, the person and `flag`.
void notify_hook(Cpu& c, std::uint32_t person, std::uint32_t flag, std::uint32_t back) {
    const auto hook = c.r(30);
    if (compare_logical(c, load32(c, 0, hook)) == 0) return;
    c.set({{3, c.u32(hook)}, {4, person}, {5, flag}});
    c.call(hook_notify, back);
}

// |value| by the C library's leaf, which leaves XER[CA], r0 and r4 as its srawi/xor/subf do.
std::int32_t absolute(Cpu& c, std::int32_t value, std::uint32_t back) {
    c.set(3, std::uint32_t(value));
    c.call(abs_leaf, back);
    return std::int32_t(c.r(3));
}

// What differs between the two settling routines.
struct Variant {
    bool skip_special;                    // leave slots 0x1e and 0x18 alone
    std::uint32_t weight_at;              // pool offset of the weight of the slots 0x11 and 0x12
    std::uint32_t conversion_at, out_at;  // frame offsets of the conversion words and of the stored integer
    std::uint32_t abs_back;               // return address of the abs call
};
constexpr Variant plain{false, 0x80, 0x1a4, 0x19c, 0x100c28ac};
constexpr Variant weighted{true, 0xa0, 0x19c, 0x1a4, 0x100c33dc};

// The registers the original keeps across the passes (r20-r28): the second hook call's callee saves them.
struct Kept {
    std::int32_t pass = 0, total = 0, counted = 0, limit = 0, within_sum = 0, within = 0, outlier = -1, index = 0;
};

struct Frame3 {
    std::uint32_t base, array, flags, end_at, first_at;  // frame addresses: slots, flags, count byte (0x38), first byte (0x39)
};

// Two passes over the slots between the first one the copy reports (+2) and the end byte:
// 1. the limit starts at 30 and grows by 5 until 12 slots (by absolute value) are within it, remembering the
//    sum of the ones within and the largest of the others (the outlier);
// 2. the slots within (and the outlier's flag) are shifted by base - average, clamped to +-0x7d; the others
//    count (pass 0, positive, player kind under 15) towards a total (slots 0x11/0x12 scaled by a weight);
//    a slot whose mark byte is 1 while slot 0 is under 15 becomes -0x78;
// 3. with more than one counted and a total over 0x55, the unflagged slots drop by the excess per counted.
Kept settle_slots(Cpu& c, Fpu& fpu, const Variant& variant, const Frame3& f, std::uint32_t player, std::uint32_t pool,
                  std::int32_t base) {
    const Slots slots{c, f.array, f.flags};
    Kept k;
    const auto first_index = [&] { return byte_of(std::int32_t(c.u8(f.first_at)) + 2); };
    const auto end_index = [&] { return std::int32_t(c.i8(f.end_at)); };
    const auto special = [&](std::int32_t i) { return variant.skip_special && (byte_of(i) == 0x1e || byte_of(i) == 0x18); };
    for (k.pass = 0; byte_of(k.pass) < 2; ++k.pass) {
        k.counted = 0;
        k.total = 0;
        k.limit = 0x1e;
        do {
            k.within_sum = 0;
            k.within = 0;
            k.outlier = -1;
            for (k.index = first_index(); byte_of(k.index) < end_index(); ++k.index) {
                if (special(k.index)) continue;
                const auto i = byte_of(k.index);
                const auto value = slots.get(i);
                if (absolute(c, value, variant.abs_back) <= extsh(std::uint32_t(k.limit))) {
                    ++k.within;
                    slots.set_flag(i, 1);
                    k.within_sum += value;
                } else {
                    slots.set_flag(i, 0);
                    if (byte_of(k.outlier) == -1 || slots.get(i) > slots.get(byte_of(k.outlier))) k.outlier = k.index;
                }
            }
            if (byte_of(k.within) < 12) k.limit = extsh(std::uint32_t(k.limit + 5));
        } while (byte_of(k.within) < 12);
        if (byte_of(k.outlier) > -1) slots.set_flag(byte_of(k.outlier), 1);
        const auto delta = extsh(std::uint32_t(base - extsh(std::uint32_t(k.within_sum / byte_of(k.within)))));
        for (k.index = first_index(); byte_of(k.index) < end_index(); ++k.index) {
            const auto i = byte_of(k.index);
            if (special(k.index) || i == byte_of(k.outlier)) continue;
            if (slots.flag(i) == 1) {
                slots.set(i, slots.get(i) + delta);
                const auto moved = slots.get(i);
                slots.set(i, moved < -0x7d ? -0x7d : moved > 0x7d ? 0x7d : moved);
            } else if (byte_of(k.pass) == 0 && slots.get(i) > 0 && byte_of(c.u8(player + player_kind)) < 0xf) {
                ++k.counted;
                if (i >= 0x11 && i < 0x13) {
                    // Slots 0x11 and 0x12 count by a weight: trunc(weight * slot), through the stack.
                    const auto slot = slots.get(i);
                    const auto conversion = c.f64(pool + pool_conversion);
                    const auto weight = c.f64(pool + variant.weight_at);
                    const auto product = fpu.fmul(weight, fpu.fsub(convert_via_stack(c, f.base + variant.conversion_at, slot), conversion));
                    const auto bits = fpu.fctiwz(0, product);
                    c.put64(f.base + variant.out_at, bits);
                    c.set_f(1, conversion);
                    c.set_f(2, weight);
                    k.total += std::int32_t(std::uint32_t(bits));
                } else {
                    k.total += slots.get(i);
                }
            } else {
                slots.set_flag(i, 1);
            }
            if (slots.get(0) < 0xf && slots.mark(i) == 1) slots.set(i, -0x78);
        }
        for (k.index = first_index(); byte_of(k.index) < end_index() && k.counted > 1 && k.total > 0x55; ++k.index) {
            const auto i = byte_of(k.index);
            if (special(k.index) || slots.flag(i) == 1) continue;
            slots.set(i, slots.get(i) - extsh(std::uint32_t((k.total - 0x55) / k.counted)));
            if (slots.get(i) < 0) slots.set(i, 0);
        }
    }
    return k;
}

// The registers the second hook call's callee saves (r19-r31 as the original holds them there).
void keep_registers(Cpu& c, const Kept& k, std::int32_t base) {
    guest::keep(c, {{20, std::uint32_t(k.pass)}, {21, std::uint32_t(base)}, {22, std::uint32_t(k.total)},
                    {23, std::uint32_t(k.counted)}, {24, std::uint32_t(k.outlier)}, {25, std::uint32_t(k.limit)},
                    {26, std::uint32_t(k.within)}, {27, std::uint32_t(k.within_sum)}, {28, std::uint32_t(k.index)}});
}

// Moves two bytes of the player towards their average (the average of their sign-extended values, rounded
// towards zero; srawi leaves XER[CA]); answers the average the original leaves as r4 (an `extsh`).
std::uint32_t pull_together(Cpu& c, std::uint32_t player, std::uint32_t a, std::uint32_t b) {
    const auto sum = std::int32_t(byte_of(c.u8(player + a))) + byte_of(c.u8(player + b));
    const auto average = extsh(generation::srawi(c, std::uint32_t(sum) + (std::uint32_t(sum) >> 31), 1));
    c.put8(player + a, c.u8(player + a) - std::uint32_t(byte_of(average)));
    c.put8(player + b, c.u8(player + b) - std::uint32_t(byte_of(average)));
    return std::uint32_t(average);
}
}

// 0x100c2800. A frame of 0x1f0 saving r19-r31: r19 the person, r29 its player record, r30 the hook word's
// address, r31 the constants' pool. Slots 0xa0 (63 of 4 bytes), flags 0x3c, count bytes 0x38-0x3a.
void settle_player_attributes(Cpu& c, std::uint32_t person) {
    constexpr std::uint32_t entry = settle_entry;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x1f0, {.first_gpr = 19});
    compare(c, std::int32_t(person));
    c.set(19, person);
    c.set(30, c.toc(hook_slot));
    c.set(31, c.toc(pool_slot));
    if (person == 0) return;
    const auto player = load32(c, 29, person + player_record);
    if (compare_logical(c, player) == 0) return;
    notify_hook(c, person, 0, entry + 0x48);
    const Frame3 f{frame.base(), frame.at(0xa0), frame.at(0x3c), frame.at(0x38), frame.at(0x39)};
    constexpr std::int32_t base = -0x14;
    guest::enter(c, entry + 0x68, {player, person, f.array, frame.at(0x39), frame.at(0x3a), frame.at(0x38)});
    copy_all_attributes_to_slots(c, player, person, f.array, frame.at(0x39), frame.at(0x3a), frame.at(0x38));
    Fpu fpu(c);
    const auto k = settle_slots(c, fpu, plain, f, player, c.r(31), base);
    compare(c, 2, 2);  // the pass loop's last compare (r20 = 2)
    guest::enter(c, entry + 0x420, {player, person, f.array});
    copy_all_attributes_from_slots(c, player, person, f.array);
    const auto kind = byte_of(load8(c, 0, player + player_kind));
    c.set(0, std::uint32_t(kind));
    if (compare(c, kind, 0xf) >= 0) {
        const auto average = pull_together(c, player, 0x2a, 0x3a);
        c.set(3, std::uint32_t(byte_of(average)));
        c.set(4, average);
    }
    if (compare_logical(c, load32(c, 0, c.r(30))) == 0) return;
    keep_registers(c, k, base);
    c.set({{3, c.u32(c.r(30))}, {4, person}, {5, 1}});
    c.call(hook_notify, entry + 0x490);
}

namespace {
// The pool constants of the weighted start (doubles at the pool pointer + offset), as the original loads them.
struct WeightedConstants {
    double conversion, k70, k48, k88, k90, k50, k98;
};

// x / 15 rounded towards zero by the compiler's multiply-high sequence (its srawi leaves XER[CA]).
std::int32_t divide_by_15(Cpu& c, std::int32_t x) {
    const auto high = std::int32_t((std::int64_t(std::int32_t(0x88888889u)) * x) >> 32);
    const auto shifted = std::int32_t(generation::srawi(c, std::uint32_t(high + x), 3));
    return shifted + std::int32_t(std::uint32_t(shifted) >> 31);
}

// The base of the weighted routine (inferred): eight attribute bytes (the smaller of +0x3b/+0x30 is the fifth)
// weighted, summed, scaled and set against the player's 16-bit +0x4 give x; below -14 four attribute bytes drop
// by x / 15 (at most 2, never below 1); the base is -20 + x, lowered by the excess of a product of two bytes
// over +0x4 (at most 25) and clamped to -40..0. The floating-point sequence, its stack words, f0-f4, FPSCR
// and XER[CA] are the original's.
std::int32_t weighted_base(Cpu& c, Fpu& fpu, guest::Frame& frame, std::uint32_t player, std::uint32_t pool) {
    const auto byte = [&](std::uint32_t offset) { return byte_of(c.u8(player + offset)); };
    const WeightedConstants k{c.f64(pool + 0x18), c.f64(pool + 0x70), c.f64(pool + 0x48), c.f64(pool + 0x88),
                              c.f64(pool + 0x90), c.f64(pool + 0x50), c.f64(pool + 0x98)};
    const auto a1 = convert_via_stack(c, frame.at(0x224), byte(0x1b));
    const auto a2 = convert_via_stack(c, frame.at(0x214), byte(0x2e));
    const auto a3 = convert_via_stack(c, frame.at(0x204), byte(0x2f));
    const auto a4 = convert_via_stack(c, frame.at(0x1f4), byte(0x36));
    const auto smaller = byte(0x3b) < byte(0x30) ? byte(0x3b) : byte(0x30);
    const auto a5 = convert_via_stack(c, frame.at(0x1e4), smaller);
    const auto a6 = convert_via_stack(c, frame.at(0x1d4), byte(0x3f));
    const auto a7 = convert_via_stack(c, frame.at(0x1c4), byte(0x40));
    const auto a8 = convert_via_stack(c, frame.at(0x1b4), byte(0x44));
    const auto as_double = [](std::uint64_t bits) { return std::bit_cast<double>(bits); };
    // The instructions' order matters for FPSCR (each result's class, and the inexact flag the next one takes over).
    const auto v1 = fpu.fsub(a1, k.conversion);
    double f4 = fpu.fmul(k.k70, v1);
    const auto v2 = fpu.fsub(a2, k.conversion);
    double f2 = fpu.fmul(k.k48, v2);
    const auto bits_a = fpu.fctiwz(3, f4);
    const auto v3 = fpu.fsub(a3, k.conversion);
    const auto bits_b = fpu.fctiwz(2, f2);
    c.put64(frame.at(0x21c), bits_a);
    double f0 = fpu.fmul(k.k88, v3);
    c.put64(frame.at(0x20c), bits_b);
    const auto bits_c = fpu.fctiwz(2, f0);
    c.put64(frame.at(0x1fc), bits_c);
    const auto v4 = fpu.fsub(a4, k.conversion);
    f0 = fpu.fmul(k.k90, v4);
    const auto bits_d = fpu.fctiwz(0, f0);
    c.put64(frame.at(0x1ec), bits_d);
    const auto v5 = fpu.fsub(a5, k.conversion);
    f4 = fpu.fmul(k.k50, v5);
    const auto v6 = fpu.fsub(a6, k.conversion);
    const auto bits_e = fpu.fctiwz(4, f4);
    f2 = fpu.fmul(k.k88, v6);
    const auto v7 = fpu.fsub(a7, k.conversion);
    const auto bits_f = fpu.fctiwz(3, f2);
    f2 = fpu.fmul(k.k88, v7);
    c.put64(frame.at(0x1dc), bits_e);
    c.put64(frame.at(0x1cc), bits_f);
    const auto v8 = fpu.fsub(a8, k.conversion);
    const auto bits_g = fpu.fctiwz(1, f2);
    const auto bits_h = fpu.fctiwz(0, v8);
    c.put64(frame.at(0x1bc), bits_g);
    c.put64(frame.at(0x1ac), bits_h);
    // The low words of the eight integers add up.
    std::uint32_t total = 0;
    for (const auto bits : {bits_a, bits_b, bits_c, bits_d, bits_e, bits_f, bits_g, bits_h}) total += std::uint32_t(bits);
    // total / 0x20 -ish: scaled by k98 through the stack.
    const auto sum_double = convert_via_stack(c, frame.at(0x1a4), std::int32_t(total));
    const auto f1 = fpu.fsub(sum_double, k.conversion);
    const auto quotient = fpu.fdiv(f1, k.k98);
    const auto bits_i = fpu.fctiwz(0, quotient);
    c.put64(frame.at(0x19c), bits_i);
    c.set_f(0, as_double(bits_i));
    c.set_f(1, f1);
    c.set_f(2, k.conversion);
    c.set_f(3, as_double(bits_f));
    c.set_f(4, as_double(bits_e));
    const auto scaled = std::int32_t(std::uint32_t(bits_i));

    const auto limit = extsh(c.u16(player + 4));
    const auto half = std::int32_t(generation::srawi(c, std::uint32_t(limit) + (std::uint32_t(limit) >> 31), 1));
    const auto difference = half + 0x37 - scaled;
    const auto x = std::int32_t(generation::srawi(c, std::uint32_t(difference) + (std::uint32_t(difference) >> 31), 1));
    if (compare(c, x, -0xe) < 0) {
        for (const std::uint32_t offset : {0x1bu, 0x36u, 0x40u, 0x44u}) {
            const auto lowered = byte(offset) + std::max(divide_by_15(c, x), -2);
            c.put8(player + offset, lowered < 1 ? 1u : std::uint32_t(lowered));
        }
    }
    std::int32_t base = -0x14 + x;
    const auto product = byte(0x36) * byte(0x1b);
    const auto mean = extsh(generation::srawi(c, std::uint32_t(product) + (std::uint32_t(product) >> 31), 1));
    const auto bound = extsh(c.u16(player + 4));
    if (compare(c, mean, bound) > 0) base -= std::min(mean - bound, 0x19);
    if (compare(c, base, -0x28) < 0) return -0x28;
    // subfc/subfe: a positive base becomes 0 (and XER[CA] is left by the pair).
    const std::uint32_t sign = std::uint32_t(base) >> 31, carry_in = base == 0;
    c.s.wr<1>(guest::reg::xer_ca, sign + carry_in >= 1);
    return base > 0 ? 0 : base;
}
}

// 0x100c2cb0. A frame of 0x270 saving r19-r31 (as 0x100c2800: r19 person, r29 player record, r30 hook word,
// r31 pool; r22 starts at 0).
void settle_player_attributes_weighted(Cpu& c, std::uint32_t person) {
    constexpr std::uint32_t entry = weighted_entry;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x270, {.first_gpr = 19});
    compare(c, std::int32_t(person));
    c.set(19, person);
    c.set(30, c.toc(hook_slot));
    c.set(31, c.toc(pool_slot));
    c.set(22, 0);
    if (person == 0) return;
    const auto player = load32(c, 29, person + player_record);
    if (compare_logical(c, player) == 0) return;
    notify_hook(c, person, 0, entry + 0x4c);
    Fpu fpu(c);
    const auto base = weighted_base(c, fpu, frame, player, c.r(31));
    const Frame3 f{frame.base(), frame.at(0xa0), frame.at(0x3c), frame.at(0x38), frame.at(0x39)};
    Slots{c, f.array, f.flags}.fill_with_terminator();
    guest::enter(c, entry + 0x6d0, {player, person, f.array, frame.at(0x39), frame.at(0x3a), frame.at(0x38)});
    copy_all_attributes_to_slots(c, player, person, f.array, frame.at(0x39), frame.at(0x3a), frame.at(0x38));
    const auto k = settle_slots(c, fpu, weighted, f, player, c.r(31), base);
    compare(c, 2, 2);  // the pass loop's last compare (r20 = 2)
    guest::enter(c, entry + 0xad0, {player, person, f.array});
    copy_all_attributes_from_slots(c, player, person, f.array);
    const auto kind = load8(c, 0, player + player_kind);
    if (compare(c, std::int32_t(kind), 0x14) == 0) {
        // A player of kind 20: bytes +0x2a/+0x3a towards their mean, and +0x1e/+0x24/+0x39 towards theirs.
        const auto pair = pull_together(c, player, 0x2a, 0x3a);
        c.set(3, std::uint32_t(byte_of(pair)));
        c.set(4, pair);
        const auto byte = [&](std::uint32_t offset) { return byte_of(c.u8(player + offset)); };
        const auto sum = byte(0x1e) + (byte(0x39) + byte(0x24));
        const auto high = std::int32_t((std::int64_t(0x55555556) * sum) >> 32);
        const auto mean = extsh(std::uint32_t(high) + (std::uint32_t(high) >> 31));
        for (const std::uint32_t offset : {0x1eu, 0x39u, 0x24u})
            c.put8(player + offset, c.u8(player + offset) - std::uint32_t(byte_of(mean)));
        c.set({{3, std::uint32_t(byte_of(mean))}, {4, std::uint32_t(byte_of(mean))}, {5, std::uint32_t(mean)},
               {6, 0x55555556}});
    }
    if (compare_logical(c, load32(c, 0, c.r(30))) == 0) return;
    keep_registers(c, k, base);
    c.set({{3, c.u32(c.r(30))}, {4, person}, {5, 1}});
    c.call(hook_notify, entry + 0xba4);
}

// 0x100c2520. A frame of 0x150 saving r27-r31: r27 the person row, r28 its index, r29 the second record
// (+0x1c), r30 the player record (+0x14), r31 the count's address. Slots at frame +0x3c, count bytes 0x38-0x3b.
void rescale_people_attributes(Cpu& c) {
    constexpr std::uint32_t entry = rescale_entry;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x150, {.first_gpr = 27});
    const auto table_at = c.toc(person_row::table_slot);
    c.set(3, table_at);
    c.set(28, 0);
    const auto count_at = c.toc(person_row::count_slot);
    c.set(31, count_at);
    const Slots slots{c, frame.at(0x3c), 0};
    // One slot of the scaling both records get: times 10, lowered by the record's 16-bit value, clamped to
    // +-200 and halved (towards zero).
    const auto rescale = [&](std::int32_t index, std::int32_t lowered_by) {
        slots.set(index, slots.get(index) * 10);
        slots.set(index, slots.get(index) - lowered_by);
        if (slots.get(index) > 200) slots.set(index, 200);
        else if (slots.get(index) < -200) slots.set(index, -200);
        slots.set(index, slots.get(index) / 2);
    };
    std::uint32_t person = load32(c, 27, table_at);
    for (std::int32_t index = 0;; ++index, person += person_row::stride) {
        if (index != 0) {  // the loop's increments
            c.set(28, std::uint32_t(index));
            c.set(27, person);
        }
        load32(c, 0, count_at);
        if (compare(c, index, std::int32_t(c.r(0))) >= 0) break;
        const auto player = load32(c, 30, person + player_record);
        const auto record = load32(c, 29, person + attribute_record);
        if (compare_logical(c, player) != 0) {
            slots.fill_with_terminator();
            compare(c, 63, 63);
            guest::enter(c, entry + 0x84, {player, person, slots.array, frame.at(0x3a), frame.at(0x3b), frame.at(0x39)});
            copy_all_attributes_to_slots(c, player, person, slots.array, frame.at(0x3a), frame.at(0x3b), frame.at(0x39));
            std::int32_t i = byte_of(std::int32_t(c.u8(frame.at(0x3a))) + 2);
            while (compare(c, byte_of(i), byte_of(c.u8(frame.at(0x39)))) < 0) rescale(byte_of(i++), extsh(c.u16(player + 4)));
            guest::enter(c, entry + 0x16c, {player, person, slots.array});
            copy_all_attributes_from_slots(c, player, person, slots.array);
            guest::enter(c, entry + 0x174, {person});
            settle_player_attributes_weighted(c, person);
            guest::enter(c, entry + 0x17c, {person});
            settle_player_attributes(c, person);
        }
        if (compare_logical(c, record) == 0) continue;
        slots.fill_with_terminator();
        compare(c, 63, 63);
        guest::enter(c, entry + 0x1cc, {record, person, slots.array, frame.at(0x38)});
        copy_attributes_to_slots(c, record, slots.array, frame.at(0x38));
        // This record's slots run until the terminator (no end count).
        std::int32_t i = c.u8(frame.at(0x38));
        while (compare(c, slots.get(byte_of(i)), terminator) != 0) rescale(byte_of(i++), extsh(c.u16(record + 0x24)));
        guest::enter(c, entry + 0x2b0, {record, person, slots.array});
        copy_attributes_from_slots(c, record, slots.array);
        guest::enter(c, entry + 0x2b8, {person});
        balance_attributes(c, person);
    }
}
}
