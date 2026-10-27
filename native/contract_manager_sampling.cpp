#include "contract_manager_random.hpp"
#include <limits>

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
bool fits_table(Cpu& c, std::int32_t bound) {
    c.set(3, 0x10000); c.set(0, 0xffff);
    if (compare(c, bound, 0xffff) > 0) return false;
    c.set(3, 0xffff0000); c.set(0, 0xffff0001);
    return compare(c, bound, -0xffff) >= 0;
}

std::int32_t narrow_sample(Cpu& c, const RandomTable& random, std::int32_t bound) {
    const auto cursor = random.position();
    c.set(3, cursor);
    const bool positive = compare(c, bound) > 0;
    const auto salt = c.u32(random.salt);
    c.set(0, salt);
    const auto draw = c.u16(cursor) + salt;
    c.set(3, draw);
    const auto quotient = std::uint32_t(divide_signed<4>(draw, std::uint32_t(bound)));
    const auto remainder = draw - quotient * std::uint32_t(bound);
    c.set(0, remainder);
    return std::int32_t(positive ? remainder : 0u - remainder);
}
}

std::int32_t sample_range(Cpu& c, std::int32_t bound) {
    Frame frame(c, 0x100788f0, 0x80, {.first_gpr = 20});
    Kept<30> range(c, std::uint32_t(bound));
    const bool has_range = nonzero(c, bound);
    Kept<25> constants(c, c.toc(-0x571c));
    Kept<26> salt(c, c.toc(-0x7b90));
    Kept<27> table(c, c.toc(-0x7b8c));
    Kept<28> end(c, c.toc(-0x7b88));
    Kept<29> cursor(c, c.toc(-0x7b84));
    const RandomTable random{c, salt, table, end, cursor};
    if (!has_range) return std::int32_t(result(c, 0));
    random.advance(0x10078944);
    if (fits_table(c, bound)) return std::int32_t(result(c, std::uint32_t(narrow_sample(c, random, bound))));

    // The source has four inlined refinement levels and then recursion.
    // Its tenfold bound wraps to int32 before division by 65535, so the
    // first refinement is always in [-32768,32768]. The deeper wide arms
    // are unreachable: one narrow refinement expresses the whole algorithm.
    static_assert(std::numeric_limits<std::int32_t>::max() / 65535 == 32768);
    static_assert(std::numeric_limits<std::int32_t>::min() / 65535 == -32768);
    c.put32(frame.at(0x4c), std::uint32_t(bound) ^ 0x80000000u);
    const auto position = random.position();
    c.set(3, position);
    c.put32(frame.at(0x48), 0x43300000);
    guest::Fpu fpu(c);
    const auto bias = c.f64(constants + 8);
    const auto encoded_bound = c.f64(frame.at(0x48));
    const auto divisor = c.f64(constants);
    c.set_f(2, bias);
    const auto converted = fpu.fsub(encoded_bound, bias);
    const auto entry = c.u16(position);
    const auto current_salt = c.u32(salt);
    const auto share = fpu.fdiv(converted, divisor);
    c.set_f(1, share);
    c.put32(frame.at(0x40), 0x43300000);
    const auto draw = std::uint32_t(entry) + current_salt;
    const auto whole = divide_65535(std::int32_t(draw));
    const auto multiple = std::uint32_t(whole.value) * 0xffff;
    const auto tenfold = std::int32_t(std::uint32_t(bound) * 10);
    const auto coarse = divide_65535(tenfold);
    Kept<22> refinement_range(c, std::uint32_t(coarse.value));
    const bool needs_refinement = nonzero(c, coarse.value);
    c.put32(frame.at(0x44), (draw - multiple) ^ 0x80000000u);
    const auto remainder = fpu.fsub(c.f64(frame.at(0x40)), bias);
    const auto integer = fpu.fctiwz(0, fpu.fmul(share, remainder));
    c.put64(frame.at(0x38), integer);
    Kept<31> part(c, c.u32(frame.at(0x3c)));
    // The original leaves these values across its possible rand() call.
    c.set({{3, position}, {4, coarse.sign}, {5, multiple}, {6, whole.sign},
           {7, draw}, {8, entry}, {9, 0x43300000}});
    std::int32_t refined = 0;
    if (needs_refinement) {
        random.advance(0x10078a30);
        fits_table(c, coarse.value);
        refined = narrow_sample(c, random, coarse.value);
    }
    Kept<24> refinement(c, std::uint32_t(refined));
    const auto tenth = rounded_tenth(refined);
    c.set(3, std::uint32_t(tenth.value)); c.set(4, tenth.sign);
    const auto dividend = std::int32_t(std::uint32_t(refined) + 5);
    const auto high = multiply_high(0x66666667, dividend);
    c.s.wr<1>(guest::reg::xer_ca, high < 0 && (std::uint32_t(high) & 3));
    const auto limit = std::int32_t(std::uint32_t(bound) - 1);
    const auto total = std::int32_t(std::uint32_t(part) + std::uint32_t(tenth.value));
    c.set(0, std::uint32_t(limit));
    part = std::uint32_t(compare(c, total, limit) > 0 ? limit : total);
    return std::int32_t(result(c, part));
}
}
