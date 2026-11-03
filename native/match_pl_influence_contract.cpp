#include "match_pl_influence.hpp"

namespace cm::aot::match_pl {
using namespace guest;

bool PlayerInfluence::ordinary_player() {
    if (compare_logical(c, c.u32(actor.address + 0x6c)) == 0) return true;
    c.set(3, c.u32(actor.address + 0x6c));
    const auto count = c.u32(c.r(19));
    c.set(3, c.u32(c.r(3)));
    return compare(c, std::int32_t(c.r(3)), std::int32_t(count)) < 0;
}

std::int8_t PlayerInfluence::staff_trait(unsigned field) {
    c.set(3, c.u32(actor.address + 0x6c));
    return c.i8(c.r(3) + field);
}

void PlayerInfluence::scale_rating(unsigned field, unsigned coefficient) {
    c.set_f(1, c.f32(actor.address + field));
    c.set_f(0, c.f32(floats + coefficient));
    c.set_f(0, fp.fmuls(c.f(1), c.f(0)));
    c.put_f32(actor.address + field, float(c.f(0)));
}

void PlayerInfluence::scale_six(const std::array<unsigned, 6>& fields, const std::array<unsigned, 6>& coefficients) {
    c.set_f(1, c.f32(actor.address + fields[0]));
    c.set_f(0, c.f32(floats + coefficients[0]));
    c.set_f(4, c.f32(floats + coefficients[1]));
    c.set_f(0, fp.fmuls(c.f(1), c.f(0)));
    for (unsigned i = 2; i < 5; ++i) c.set_f(5 - i, c.f32(floats + coefficients[i]));
    c.put_f32(actor.address + fields[0], float(c.f(0)));
    c.set_f(0, c.f32(floats + coefficients[5]));
    for (unsigned i = 1; i < 6; ++i) {
        const unsigned result = 5 - i;
        c.set_f(result + 1, c.f32(actor.address + fields[i]));
        c.set_f(result, fp.fmuls(c.f(result + 1), c.f(result)));
        c.put_f32(actor.address + fields[i], float(c.f(result)));
    }
}

bool PlayerInfluence::contract_random_test(unsigned bound, std::uint32_t back) {
    const auto cursor = c.r(25), seed = c.r(24);
    const auto next = c.u32(cursor) + 2;
    const bool refill = compare_logical(c, next, c.u32(c.r(22))) > 0;
    c.set(3, next);
    c.put32(cursor, next);
    if (refill) {
        c.put32(cursor, c.r(23));
        c.call(0x1077b770, back);
        c.put32(seed, c.r(3) & 0xffff);
    }
    const bool add_age = bound == 50;
    const auto pool_address = c.u32(cursor);
    c.set(add_age ? 4 : 5, pool_address);
    c.set(add_age ? 5 : 4, bound == 20 ? 0x66660000 : 0x51ec0000);
    c.set(3, c.u32(actor.address + 0x6c));
    const auto multiplier = bound == 20 ? 0x66666667u : 0x51eb851fu;
    c.set(add_age ? 5 : 4, multiplier);
    const auto word = c.u16(pool_address);
    c.set(add_age ? 7 : 6, word);
    if (add_age) c.set(4, std::int32_t(std::int8_t(c.r(26))));
    const auto bias = c.u32(seed);
    c.set(add_age ? 6 : 5, bias);
    const auto composure = c.i8(c.r(3) + 0x66);
    const auto value = word + bias;
    c.set(add_age ? 6 : 5, value);

    // The original uses a signed multiply-high and shift for each constant
    // remainder. Preserve its carry as well as the signed result.
    const auto high = std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    const unsigned shift = add_age ? 4 : 3;
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & ((1u << shift) - 1)));
    const auto quotient = high >> shift;
    c.set(4, quotient < 0);
    c.set(3, value - std::uint32_t(quotient + (quotient < 0)) * bound);
    return compare(c, composure + (add_age ? std::int8_t(c.r(26)) : 0), std::int32_t(c.r(3))) < 0;
}

void PlayerInfluence::contract_composure_penalty() {
    c.set(3, c.u32(actor.address + 0x6c));
    if (compare_logical(c, c.u32(c.r(3) + 0x14)) == 0 ||
        compare_logical(c, c.r(27)) == 0 ||
        compare(c, c.i8(c.r(27) + 0x4c), -25) >= 0) return;
    const auto previous_club = [&] {
        c.set(3, c.u32(actor.address + 0x6c));
        c.set(3, c.u32(c.r(3) + 0x14));
        return c.r(3);
    };
    if (compare(c, c.i8(previous_club() + 0x44), 15) >= 0 ||
        compare(c, staff_trait(0x67), 10) >= 0 ||
        compare(c, c.i16(previous_club() + 0xa), 3750) <= 0) return;

    const auto side = actor.side();
    c.set(3, actor.match());
    const bool away = compare(c, std::int8_t(side)) != 0;
    c.set(3, c.u32(c.r(3) + 0x3118));
    c.set(3, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
    c.set(4, c.u32(actor.address + 0x6c));
    c.set(3, std::int32_t(c.i16(c.r(3) + 0x1ec)));
    c.set(4, c.u32(c.r(4) + 0x14));
    const auto limit = std::int32_t(c.r(3)) + 250;
    c.set(3, std::int32_t(c.i16(c.r(4) + 0xa)));
    if (compare(c, std::int32_t(c.r(3)), limit) <= 0) return;

    c.set_f(2, c.f32(actor.address + 0xf8));
    c.set_f(1, c.f32(floats + 0x74));
    c.set_f(0, c.f32(floats + 0x14));
    c.set_f(1, fp.fmuls(c.f(2), c.f(1)));
    c.put_f32(actor.address + 0xf8, float(c.f(1)));
    c.set_f(1, c.f32(actor.address + 0x80));
    c.set_f(0, fp.fmuls(c.f(1), c.f(0)));
    c.put_f32(actor.address + 0x80, float(c.f(0)));
    if (compare(c, c.i8(c.r(27) + 0x4c), -75) >= 0) return;

    c.set_f(1, c.f32(actor.address + 0xf4));
    c.set_f(0, c.f32(floats + 0x14));
    c.set_f(3, c.f32(floats + 0x14));
    c.set_f(4, fp.fmuls(c.f(1), c.f(0)));
    c.set_f(2, c.f32(floats + 0x74));
    c.set_f(1, c.f32(floats + 0x14));
    c.set_f(0, c.f32(floats + 0x74));
    c.put_f32(actor.address + 0xf4, float(c.f(4)));
    constexpr unsigned fields[]{0xa0, 0x98, 0x9c, 0xc4};
    for (unsigned i = 0; i < std::size(fields); ++i) {
        const auto result = 3 - i;
        c.set_f(result + 1, c.f32(actor.address + fields[i]));
        c.set_f(result, fp.fmuls(c.f(result + 1), c.f(result)));
        c.put_f32(actor.address + fields[i], float(c.f(result)));
    }
}

void PlayerInfluence::contract_adjustments() {
    c.set(3, c.u32(actor.address + 0x6c));
    c.set(4, c.u32(c.r(17)));
    const auto id = c.u32(c.r(3));
    c.set(3, c.r(4) + id * 100);
    if (compare(c, c.u8(c.r(3) + 0xa), 8) > 0 && compare(c, staff_trait(0x67), 15) < 0) {
        scale_six({0x80, 0xf8, 0xf4, 0xa0, 0x98, 0xc4}, {0x74, 0x74, 0x14, 0x14, 0x14, 0x74});
    }

    const auto contract = c.r(27);
    if (compare_logical(c, contract) == 0) return;
    const auto flag = [&](unsigned mask) { return compare(c, c.u32(contract + 0x50) & mask) != 0; };
    if (flag(0x40000) && contract_random_test(50, 0x1022abf8))
        scale_six({0xf8, 0xa0, 0xf4, 0x98, 0x94, 0xc4}, {0x74, 0x14, 0x14, 0x74, 0x74, 0x74});
    contract_composure_penalty();

    if (flag(0x17)) {
        if (compare(c, staff_trait(0x67), 15) > 0 && compare(c, staff_trait(0x63), 15) > 0) scale_rating(0xf8, 0x78);
        else if (compare(c, staff_trait(0x67), 5) < 0) scale_rating(0xf8, 0x14);
    }
    if (flag(0x2088)) {
        c.set(3, c.u32(actor.address + 0x6c));
        c.set(3, c.u32(c.r(3) + 0x14));
        const bool elite = compare(c, c.i16(c.r(3) + 0xa), 4250) > 0;
        if ((elite && compare(c, staff_trait(0x67), 10) < 0) || compare(c, staff_trait(0x67), 5) < 0)
            scale_rating(0xf8, 0x14);
    }
    c.set(3, 0x38da00);
    if (flag(c.r(3))) {
        c.set(3, c.u32(actor.address + 0x6c));
        if (compare(c, c.u8(c.r(3) + 0x67), 1) == 0) scale_rating(0xf8, 0x7c);
        else if (compare(c, staff_trait(0x67), 6) < 0) scale_rating(0xf8, 0x74);
    }
    const auto draw = [&](unsigned mask, unsigned bound, std::uint32_t back) {
        if (!flag(mask)) return;
        bool vulnerable = compare(c, staff_trait(0x66), 5) < 0 &&
            compare(c, staff_trait(0x67), 6) < 0 &&
            compare(c, staff_trait(0x69), 10) < 0;
        if (vulnerable) {
            c.set(3, c.u32(actor.address + 0x6c));
            c.set(3, c.u32(c.r(3) + 0x14));
            vulnerable = compare(c, c.i16(c.r(3) + 0xc), 5250) > 0;
        }
        if (vulnerable)
            scale_six({0xf8, 0xa0, 0xf4, 0x98, 0x94, 0xc4}, {0x7c, 0x14, 0x14, 0x14, 0x7c, 0x14});
        else if (contract_random_test(bound, back))
            scale_six({0xf8, 0x94, 0xa0, 0xf4, 0x98, 0xc4}, {0x74, 0x74, 0x14, 0x14, 0x14, 0x74});
    };
    draw(0x20000, 20, 0x1022afd4);
    draw(0x40, 25, 0x1022b160);
}
}
