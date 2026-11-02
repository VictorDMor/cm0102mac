#include "cup_stage_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::cup_stage {
using namespace detail;
std::uint32_t random_below(Cpu& c, std::uint32_t bound) {
    Frame f(c, 0x104be7e0, 0x80, {.first_gpr = 20});
    Fpu fp(c);
    Kept<30> limit(c, bound);
    keep(c, {{25, c.toc(slot::doubles)}, {26, c.toc(slot::rng_seed)}, {27, c.toc(slot::rng_start)},
             {28, c.toc(slot::rng_end)}, {29, c.toc(slot::rng_cursor)}});
    if (compare(c, std::int32_t(bound)) == 0) return 0;
    const Stream rng{c, c.r(29), c.r(28), c.r(27), c.r(26)};
    const auto small_sample = [&](std::uint32_t range) {
        c.set(3, c.u32(rng.cursor));
        compare(c, std::int32_t(range));
        const auto sum = rng.sample();
        c.set(3, sum);
        const auto remainder = std::int32_t(std::int64_t(std::int32_t(sum)) % std::int32_t(range));
        return std::int32_t(range) > 0 ? std::uint32_t(remainder) : 0u - std::uint32_t(remainder);
    };
    rng.advance(0x104be834);
    c.set(3, 0x10000);
    const bool above = compare(c, std::int32_t(bound), 65535) > 0;
    if (!above) {
        c.set(3, 0xffff0000);
        if (compare(c, std::int32_t(bound), -65535) >= 0) return small_sample(bound);
    }
    const auto constants = c.r(25), cursor = c.u32(rng.cursor);
    c.put32(f.at(0x4c), bound ^ 0x80000000u); c.put32(f.at(0x48), 0x43300000);
    c.set_f(2, c.f64(constants + 8));
    c.set_f(1, fp.fsub(c.f64(f.at(0x48)), c.f(2)));
    c.set_f(1, fp.fdiv(c.f(1), c.f64(constants)));
    const auto sample = c.u16(cursor);
    const auto sum = std::uint32_t(sample) + c.u32(rng.seed);
    const auto wrapped_product = std::int32_t(bound * 10u);
    Kept<22> reduced(c, std::uint32_t(wrapped_product / 65535));
    const auto quotient = std::int32_t(sum) / 65535;
    const auto remainder = std::uint32_t(std::int32_t(sum) % 65535);
    c.put32(f.at(0x40), 0x43300000); c.put32(f.at(0x44), remainder ^ 0x80000000u);
    c.set_f(0, fp.fsub(c.f64(f.at(0x40)), c.f(2)));
    c.set_f(0, fp.fmul(c.f(1), c.f(0)));
    c.put64(f.at(0x38), fp.fctiwz(0, c.f(0)));
    Kept<31> sampled(c, c.u32(f.at(0x3c)));
    // Wrapping multiplication bounds the reduced range to [-32768,32768], so
    // the deeper inlined wide-range branches and the recursion can't run.
    const auto high = [](std::int32_t x) {
        return std::int32_t((std::int64_t(std::int32_t(0x80008001u)) * x) >> 32);
    };
    const auto sum_shift = std::int32_t(std::uint32_t(high(std::int32_t(sum))) + sum) >> 15;
    const auto reduced_shift = std::int32_t(std::uint32_t(high(wrapped_product)) + std::uint32_t(wrapped_product)) >> 15;
    leave(c, {{3, cursor}, {4, std::uint32_t(reduced_shift) >> 31}, {5, std::uint32_t(quotient) * 65535u},
              {6, std::uint32_t(sum_shift) >> 31}, {7, sum}, {8, sample}, {9, 0x43300000}});
    std::uint32_t additional = 0;
    if (compare(c, std::int32_t(std::uint32_t(reduced))) != 0) {
        rng.advance(0x104be920);
        c.set(3, 0x10000); compare(c, std::int32_t(std::uint32_t(reduced)), 65535);
        c.set(3, 0xffff0000); compare(c, std::int32_t(std::uint32_t(reduced)), -65535);
        additional = small_sample(reduced);
    }
    c.set(24, additional);
    const auto correction = std::int32_t(additional + 5) / 10;
    sampled = std::uint32_t(sampled) + std::uint32_t(correction);
    if (compare(c, std::int32_t(std::uint32_t(sampled)), std::int32_t(bound - 1)) > 0) sampled = bound - 1;
    const auto signed_add = std::int32_t(additional + 5);
    const auto high_ten = std::int32_t((std::int64_t(0x66666667) * signed_add) >> 32) >> 2;
    c.set(4, std::uint32_t(high_ten) >> 31);
    return sampled;
}

}
