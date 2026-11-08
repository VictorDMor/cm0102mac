// 0x10211e70: what the place's own bits (the two words the caller passes, tested through r21's low word:
// the high word's masks are all zero) add to the modifier for the person's attributes
// (match_man_rating_c_detail.hpp).
#include "match_man_adapter.hpp"
#include "match_man_rating_c_detail.hpp"

namespace cm::aot::match_man::placement {

// `and`, `xor`, `or.` of the place's low word with `mask` (the high word is masked with 0): cr0 of the result. The
// compiler's register use depends on which test of the chain it is (r0 is not compared): a first test of a chain
// leaves the hit in r3 (and r4) with r5 or r6 cleared; a second test leaves only the hit.
bool Placement::place_has(u32 mask, Test test) {
    const u32 hit = u32(second_word) & mask;
    switch (test) {
    case Test::first_small: c.set({{3, hit}, {4, hit}, {5, 0}}); break;
    case Test::second_small: c.set({{3, hit}, {4, hit}, {5, hit}}); break;
    case Test::first_group: c.set({{4, hit}, {5, hit}, {6, 0}}); break;
    case Test::second_group: c.set({{4, hit}, {5, hit}}); break;
    case Test::third_group_second: break;
    case Test::first_high: c.set({{3, hit}, {4, 0}, {5, 0}}); break;
    case Test::first_high_r4: c.set({{3, hit}, {4, 0}}); break;
    case Test::second_high: c.set(3, hit); break;
    }
    compare(c, i32(hit), 0);
    return hit != 0;
}
// rlwinm. of the caller's extra word (0x1a8) into r4 (the first two chains) or r0 (the rest).
bool Placement::extra_has(u32 mask, bool into_r4) {
    const u32 hit = c.u32(at(at::extra)) & mask;
    if (into_r4) c.set(4, hit);
    compare(c, i32(hit), 0);
    return hit != 0;
}
// `lbz r0,0x1a7; extsb r3,r0; rlwinm. r0,r3,...`: the place's role byte (the low byte of the word at 0x1a4) in r3.
bool Placement::role_byte_has(u32 mask) {
    const u32 byte = c.u8(at(at::place_role) + 3);
    c.set(3, reg(s8(byte)));
    compare(c, i32(byte & mask), 0);
    return (byte & mask) != 0;
}
// The H bodies' first test (`lbz r0,0x1a7; rlwinm. r0,r0`) leaves r3 alone.
bool Placement::role_byte_bit(u32 mask) {
    const u32 hit = u32(c.u8(at(at::place_role) + 3)) & mask;
    compare(c, i32(hit), 0);
    return hit != 0;
}

// trunc((value as a double) / the pool's divisor): the int-to-double words at `pair_at`, the truncated
// result stored at `result_at`.
u32 Placement::scaled_term(u32 pair_at, i32 value, u32 divisor_offset, u32 result_at, double* quotient, bool into_f1) {
    const double converted = fpu.fsub(store_pair(c, at(pair_at), value), conversion());
    if (into_f1) c.set_f(1, converted);
    const double divided = fpu.fdiv(converted, D(divisor_offset));
    if (quotient) *quotient = divided;
    return truncate_at(c, fpu, at(result_at), divided);
}

// The three attributes (0x2e, 0x36, 0x3c above ten) each over the pool's divisor, with the stack slots the
// original gives each pair and result.
u32 Placement::three_attributes(u32 divisor_offset, const u32 (&pairs)[3], const u32 (&results)[3]) {
    double quotient = 0;
    const u32 first = scaled_term(pairs[0], stat(0x2e) - 10, divisor_offset, results[0], nullptr, false);
    const u32 second_term = scaled_term(pairs[1], stat(0x36) - 10, divisor_offset, results[1], &quotient, false);
    fpu.fctiwz(1, quotient);  // the compiler converts the second quotient (f1) before the third's conversion
    const u32 third = scaled_term(pairs[2], stat(0x3c) - 10, divisor_offset, results[2], nullptr, false);
    modifier = u32(modifier) + first;
    modifier = u32(modifier) + second_term + third;
    return second_term;
}

// Each bit of the place's low word asks for an adjustment from some attributes of the player. The
// divisions are the compiler's multiply-high by a constant (exact for the signed bytes read): `/ 7`,
// `/ 14`, `/ 15`, `/ 9`, `/ 12`, `/ 3`, `/ 10` and the `(x + sign) >> 1` halving. After each, the registers the
// compiler leaves (r3 the last truncated term or the last quotient's sign...) are set as the original leaves them.
void Placement::place_bonuses() {
    const auto mh = [](u32 magic, i32 value) { return appraisal::mulhw(i32(magic), value); };
    const auto sgn = [](i32 value) -> u32 { return value < 0 ? 1u : 0u; };
    const auto flip = [](i32 value) { return reg(value) ^ 0x80000000u; };
    if (!wants(bit(0x10))) return;
    // Positions of the kind 4 (or 1 with the extra bit 2), 8 and 0x10 (the extra bits 4, 8).
    if (place_has(4, Test::first_small) || (place_has(1, Test::second_small) && extra_has(2, true))) {
        const u32 term = scaled_term(0x90, stat(0x40) - 10, 0x20, 0x98);
        modifier = u32(modifier) + term + reg(stat(0x37) / 7);
        c.set({{3, term}, {4, flip(stat(0x40) - 10)}, {5, reg(stat(0x40))}});
    } else if (place_has(8, Test::first_group) || (place_has(1, Test::second_group) && extra_has(4, true))) {
        const u32 pairs[3] = {0x90, 0xa0, 0x88}, results[3] = {0x98, 0xa8, 0x80};
        const u32 second_term = three_attributes(0x80, pairs, results);
        c.set({{3, second_term}, {4, flip(stat(0x36) - 10)}, {5, 0x43300000}, {6, flip(stat(0x2e) - 10)}});
    } else if (place_has(0x10, Test::first_group) || (place_has(1, Test::third_group_second) && extra_has(8, false))) {
        const u32 pairs[3] = {0x80, 0x90, 0xa0}, results[3] = {0x88, 0x98, 0xa8};
        const u32 second_term = three_attributes(0x20, pairs, results);
        c.set({{3, second_term}, {4, flip(stat(0x36) - 10)}, {5, 0x43300000}, {6, flip(stat(0x2e) - 10)}});
    }
    if (place_has(0x8000, Test::first_high) || (place_has(0x2000, Test::second_high) && extra_has(0x100, false))) {
        // The two ratings above fifteen times 0x10775540's answer, over the pool's divisor.
        const i32 first_argument = stat(0x3c) - 0xf;
        c.set(3, reg(first_argument));
        c.call(0x10775540, 0x10213ad4);
        const u32 first = scaled_term(0x80, i32(reg(first_argument) * c.r(3)), 0x80, 0x88);
        modifier = u32(modifier) + first;
        const i32 second_argument = stat(0x44) - 0xf;
        c.set(3, reg(second_argument));
        c.call(0x10775540, 0x10213b28);
        const i32 product = i32(reg(second_argument) * c.r(3));
        const u32 second_term = scaled_term(0x90, product, 0x80, 0x98);
        modifier = u32(modifier) + second_term + reg(stat(0x1e) / 10);
        c.set({{3, second_term}, {4, flip(product)}, {5, reg(product)}, {6, reg(stat(0x44))}});
    } else if (role_byte_bit(1)) {
        const u32 term = scaled_term(0x80, stat(0x28) - 10, 0x20, 0x88);
        modifier = u32(modifier) + reg(stat(0x43) / 12) + term + reg(stat(0x33) / 12);
        c.set({{3, term}, {4, reg(stat(0x43) / 12)}, {5, 0x2aab0000}, {6, c.u8(data + 0x43)}});
    }
    if (role_byte_has(2)) {
        modifier = u32(modifier) + reg(stat(0x39) / 7);
        c.set(3, sgn((mh(0x92492493, stat(0x39)) + stat(0x39)) >> 2));
    } else if (role_byte_has(4)) {
        modifier = u32(modifier) + reg(stat(0x32) / 7);
        c.set(3, sgn((mh(0x92492493, stat(0x32)) + stat(0x32)) >> 2));
    }
    if (place_has(0x40, Test::first_high)) {
        modifier = u32(modifier) + scaled_term(0x80, stat(0x3b) - 0x14, 0x368, 0x88);
        c.set(3, reg(stat(0x3b)));
    } else if (place_has(0x100, Test::second_high)) {
        modifier = u32(modifier) + scaled_term(0x80, stat(0x30) - 0x14, 0x368, 0x88);
        c.set(3, reg(stat(0x30)));
    }
    if (place_has(0x200000, Test::first_high_r4)) {
        modifier = u32(modifier) + reg(stat(0x1a)) - 0x14 + reg(stat(0x33) / 9) + reg(stat(0x43) / 10);
        c.set({{3, sgn(mh(0x66666667, stat(0x43)) >> 2)}, {4, reg(stat(0x33) / 9)}, {5, reg(stat(0x1a))}});
    }
    if (place_has(0x20000, Test::first_high)) {
        modifier = u32(modifier) + reg((stat(0x2e) - 10) / 4);
        c.set(3, reg(stat(0x2e)));
    } else if (place_has(0x40000000, Test::second_high)) {
        modifier = u32(modifier) + reg((10 - stat(0x2e)) / 4);
    }
    if (place_has(0x80000000, Test::first_high)) {
        modifier = u32(modifier) + reg((10 - stat(0x2e)) / 4);
    } else if (place_has(0x80000, Test::second_high)) {
        modifier = u32(modifier) + reg((stat(0x2e) - 10) / 4);
        c.set(3, reg(stat(0x2e)));
    }
    if (place_has(0x800000, Test::first_high_r4)) {
        const i32 span = stat(0x3c) - 10;
        modifier = u32(modifier) + reg(half(span)) + reg(stat(0x43) / 15) + reg(stat(0x27) / 15);
        c.set({{3, sgn((mh(0x88888889, stat(0x27)) + stat(0x27)) >> 3)}, {4, reg(stat(0x43) / 15)}, {5, reg(half(span))},
               {6, sgn(span)}, {7, reg(span)}});
    }
    if (place_has(0x2000000, Test::first_high_r4)) {
        modifier = u32(modifier) + reg(stat(0x26) / 7);
        c.set(3, sgn((mh(0x92492493, stat(0x26)) + stat(0x26)) >> 2));
    }
    if (place_has(0x8000000, Test::first_high_r4)) {
        const i32 span = stat(0x40) - 10;
        modifier = u32(modifier) + reg(span / 3) + reg(half(stat(0x3d) - 10));
        c.set({{3, reg(span / 3)}, {4, sgn(mh(0x55555556, span))}, {5, reg(mh(0x55555556, span))}});
    }
    if (place_has(0x20000000, Test::first_high_r4)) {
        modifier = u32(modifier) + reg(stat(0x31) / 14) + reg(stat(0x27) / 14) + reg((stat(0x28) - 10) / 2);
        const i32 d31 = stat(0x31), d27 = stat(0x27);
        c.set({{3, reg(d27 / 14)}, {4, sgn((mh(0x92492493, d27) + d27) >> 3)}, {5, reg(d31 / 14)},
               {6, reg((mh(0x92492493, d31) + d31) >> 3)}, {7, reg(mh(0x92492493, d31))}});
    }
}
}
