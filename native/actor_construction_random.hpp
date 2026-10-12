#pragma once
#include "actor_construction_abi.hpp"

namespace cm::aot::actor_construction {
// Consume the original pool in its original order. Refilling remains a
// characterized dependency; both paths expose the same pointer/epoch writes.
inline void advance_pool(ConstructionCall& a, std::uint32_t start) {
    a.at(start); a.load(3, 30, 0); a.load(0, 29, 0); a.offset(3, 3, 2);
    a.compare_registers(3, 0, false); a.store(30, 0, 3);
    if (!a.nonpositive()) {
        a.store(30, 0, 28); a.call(0x1077b770); a.mask(0, 3, 0xffff); a.store(27, 0, 0);
    }
    a.at(start + 0x28);
}

// The export uses a signed remainder corrected toward the bound's sign. Large
// bounds use its double conversion and rounding helper, with the same final
// clamp. This is one policy with two original register/stack allocation shapes.
template<unsigned Bound, unsigned Result, int BoundSlot, int RandomSlot, int ResultSlot, bool FirstLayout = false>
void draw_uniform(ConstructionCall& a, std::uint32_t start, bool test_already_executed = false) {
    const auto end = start + 0x16c;
    if (test_already_executed) a.at(start + 4);
    else { a.at(start); a.compare(Bound, 0); }
    if (a.equal()) { a.value(Result, 0); a.jump(end); a.at(end); return; }
    advance_pool(a, start + 0x10);
    a.value(3, 0x10000); a.offset(3, 3, -1); a.compare_registers(Bound, 3);
    bool large = a.positive();
    if (!large) {
        a.value(3, 0xffff0000); a.offset(3, 3, 1); a.compare_registers(Bound, 3);
        large = !a.at_least();
    }
    if (large) {
        a.at(start + 0x58); a.exclusive_flags(3, Bound, 0x80000000); a.value(0, 0x43300000); a.store(1, BoundSlot + 4, 3);
        a.value(6, 0x80010000); a.load(FirstLayout ? 3 : 7, 30, 0); a.value(FirstLayout ? 5 : 4, FirstLayout ? 0x10000 : 0x43300000);
        a.store(1, BoundSlot, 0); a.value(FirstLayout ? 0 : 5, FirstLayout ? 0x43300000 : 0x10000);
        a.floating_load(2, 26, 0x80); a.value(FirstLayout ? 4 : 3, 0x80010000); a.floating_load(1, 1, BoundSlot);
        a.offset(6, 6, -0x7fff); a.floating_load(0, 26, 0); a.offset(5, 5, -1); a.floating_subtract(1, 1, 2);
        a.load<2>(8, FirstLayout ? 3 : 7, 0); a.load(7, 27, 0); a.multiply(FirstLayout ? 3 : 0, Bound, 10);
        if constexpr (FirstLayout) { a.store(1, RandomSlot, 0); a.offset(4, 4, -0x7fff); }
        else { a.offset(3, 3, -0x7fff); a.store(1, RandomSlot, 4); }
        a.floating_difference(2, 1, 0, true);
        a.add(FirstLayout ? 0 : 7, 8, 7); a.multiply_high(FirstLayout ? 6 : 4, 6, FirstLayout ? 0 : 7);
        a.floating_load(1, 26, 0x80); a.add(FirstLayout ? 6 : 4, FirstLayout ? 6 : 4, FirstLayout ? 0 : 7);
        a.shift_right(FirstLayout ? 6 : 4, FirstLayout ? 6 : 4, 15);
        a.mask(FirstLayout ? 7 : 6, FirstLayout ? 6 : 4, 1, false, 1);
        a.add(FirstLayout ? 6 : 4, FirstLayout ? 6 : 4, FirstLayout ? 7 : 6);
        a.product(FirstLayout ? 5 : 4, FirstLayout ? 6 : 4, 5);
        a.subtract(Result, FirstLayout ? 5 : 4, FirstLayout ? 0 : 7);
        a.exclusive_flags(FirstLayout ? 0 : 4, Result, 0x80000000); a.store(1, RandomSlot + 4, FirstLayout ? 0 : 4);
        a.multiply_high(FirstLayout ? 0 : 3, FirstLayout ? 4 : 3, FirstLayout ? 3 : 0);
        a.floating_load(0, 1, RandomSlot); a.floating_subtract(0, 0, 1);
        a.add(0, FirstLayout ? 0 : 3, FirstLayout ? 3 : 0); a.shift_right(0, 0, 15); a.floating_product(0, 2, 0);
        a.mask(3, 0, 1, false, 1); a.add(3, 0, 3); a.floating_integer(0, 0); a.floating_store(0, 1, ResultSlot);
        a.load(Result, 1, ResultSlot + 4); a.call(0x10226520);
        a.value(4, 0x66660000); a.offset(3, 3, 5); a.offset(4, 4, 0x6667); a.offset(0, Bound, -1);
        a.multiply_high(3, 4, 3); a.shift_right(3, 3, 2); a.mask(4, 3, 1, false, 1); a.add(3, 3, 4);
        a.add(Result, Result, 3); a.compare_registers(Result, 0);
        if (!a.nonpositive()) { a.offset(Result, Bound, -1); a.jump(end); }
    } else {
        a.at(start + 0x13c); a.load(3, 30, 0); a.compare(Bound, 0); a.load(0, 27, 0); a.load<2>(3, 3, 0);
        a.add(3, 3, 0); a.divide(0, 3, Bound); a.product(0, 0, Bound); a.subtract(Result, 0, 3); a.negate(0, Result);
        if (!a.nonpositive()) a.move(0, Result);
        a.at(end - 4); a.move(Result, 0);
    }
    a.at(end);
}

// A second draw selects among the original probability bands. Its conversion,
// rounding dependency, and saturation are shared by the three bands.
inline void draw_probability_threshold(ConstructionCall& a, std::uint32_t start,
    std::uint32_t rounding_argument, int factor_offset, std::uint32_t limit_high, int limit_low,
    bool retain_pick = false) {
    if (retain_pick) {
        a.at(start); a.load(3, 30, 0); a.move(14, 13); a.load(0, 29, 0); a.offset(3, 3, 2);
        a.compare_registers(3, 0, false); a.store(30, 0, 3);
        if (!a.nonpositive()) { a.store(30, 0, 28); a.call(0x1077b770); a.mask(0, 3, 0xffff); a.store(27, 0, 0); }
        a.at(start + 0x2c);
    } else advance_pool(a, start);
    a.load(3, 30, 0); a.value(5, 0x80010000); a.value(0, 0x43300000); a.value(4, 0x10000);
    a.load<2>(7, 3, 0); a.offset(5, 5, -0x7fff); a.load(6, 27, 0); a.offset(4, 4, -1);
    a.store(1, 0x3c, 0); a.value(3, rounding_argument); a.add(6, 7, 6); a.floating_load(1, 26, 0x80);
    a.multiply_high(0, 5, 6); a.floating_load(2, 26, factor_offset); a.add(0, 0, 6); a.shift_right(0, 0, 15);
    a.mask(5, 0, 1, false, 1); a.add(0, 0, 5); a.product(0, 0, 4); a.subtract(13, 0, 6);
    a.exclusive_flags(0, 13, 0x80000000); a.store(1, 0x40, 0); a.floating_load(0, 1, 0x3c); a.floating_subtract(0, 0, 1);
    a.floating_product(0, 2, 0); a.floating_integer(0, 0); a.floating_store(0, 1, 0x44); a.load(13, 1, 0x48); a.call(0x10226520);
    a.value(4, 0x66660000); a.offset(0, 3, 5); a.offset(4, 4, 0x6667); a.value(3, limit_high);
    a.multiply_high(0, 4, 0); a.offset(3, 3, limit_low); a.shift_right(0, 0, 2); a.mask(4, 0, 1, false, 1);
    a.add(0, 0, 4); a.add(13, 13, 0); a.compare_registers(13, 3);
    if (!a.nonpositive()) { a.value(13, limit_high); a.offset(13, 13, limit_low); }
    a.at(start + (retain_pick ? 0xd8 : 0xd4));
}

// Fixed small draws retain the compiler's signed constant-division sequence.
inline void draw_small(ConstructionCall& a, std::uint32_t start, std::uint32_t high, std::int32_t low,
                       unsigned shift, std::uint32_t divisor, bool negative = false) {
    advance_pool(a, start);
    a.load(4, 30, 0); a.value(3, high); a.load(0, 27, 0); a.offset(3, 3, low); a.load<2>(4, 4, 0);
    a.add(4, 4, 0); a.multiply_high(0, 3, 4); a.shift_right(0, 0, shift); a.mask(3, 0, 1, false, 1);
    a.add(0, 0, 3); a.multiply(0, 0, divisor); a.subtract(0, 0, 4);
    if (negative) a.negate(0, 0);
}
}
