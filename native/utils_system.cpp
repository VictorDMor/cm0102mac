#include "utils_detail.hpp"
#include "rng_core.hpp"

namespace cm::aot::utils {
using namespace detail;
namespace {
constexpr std::uint32_t microseconds = 0x10780600, flush_events = 0x10781308, internet_start = 0x10781290,
                        internet_launch_url = 0x107812a8, internet_stop = 0x107812c0;
constexpr std::uint32_t mouse_and_key_events = 0x1e;
// The range of the salt drawn with the sample pool.
constexpr std::uint32_t pool_salts = 65535;
std::uint32_t high_product(std::uint32_t a, std::uint32_t b) {
    return std::uint32_t((std::int64_t(std::int32_t(a)) * std::int32_t(b)) >> 32);
}
// `srawi`, which sets the carry.
std::uint32_t shift_right(Cpu& c, std::uint32_t value, unsigned shift) {
    c.s.wr<1>(reg::xer_ca, std::int32_t(value) < 0 && (value & ((1u << shift) - 1)) != 0);
    return std::uint32_t(std::int32_t(value) >> shift);
}
}

std::uint32_t milliseconds(Cpu& c) {
    Frame frame(c, 0x10746a30, 0x40, {});
    const auto clock = frame.at(0x38);
    carbon_call(c, microseconds, 0x10746a44, {clock});
    call(c, 0x10773870, 0x10746a5c, {c.u32(clock), c.u32(clock + 4), 0, 1000});
    return result(c, c.r(4));
}

void flush_input(Cpu& c) {
    Frame frame(c, 0x10747080, 0x40, {});
    carbon_call(c, flush_events, 0x10747098, {mouse_and_key_events, 0});
}

void open_url(Cpu& c, std::uint32_t url) {
    Frame frame(c, 0x107471d0, 0x150, {.first_gpr = 31});
    const auto instance = frame.at(0x40), pascal = frame.at(0x44), range_start = frame.at(0x3c), range_end = frame.at(0x38);
    keep(c, {{31, url}});
    const auto signature = c.u32(c.toc(-0x5bbc) + 0x14);
    if (nonzero(c, std::int16_t(carbon_call(c, internet_start, 0x107471f4, {instance, signature})))) return;
    call(c, 0x1074c5d0, 0x1074720c, {pascal, url});
    const auto length = c.u8(pascal);
    c.put32(range_start, 0);
    c.put32(range_end, length);
    carbon_call(c, internet_launch_url, 0x10747238,
                {c.u32(instance), c.toc(strings_slot) + 0x25a, pascal + 1, length, range_start, range_end});
    carbon_call(c, internet_stop, 0x10747244, {c.u32(instance)});
}

void initialize_random(Cpu& c, std::uint32_t seed) {
    // By hand rather than with Frame: saves and table reads interleave, because
    // the RNG core's checks pin the order of every access and the whole register
    // file (native/rng_core_checks.cpp), so that a pointer into this frame acts
    // as in the original.
    const auto caller = c.sp(), base = caller - 0x50;
    GuestNativeScope scope(c.s, 0x107474a0);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    const auto cursor = c.toc(-0x7b84);
    c.set(31, cursor);
    c.put32(caller - 8, c.r(30));
    const auto pool = c.toc(-0x7b8c);
    c.set(30, pool);
    c.put32(caller - 12, c.r(29));
    const auto last = c.toc(-0x7b88);
    c.set(29, last);
    c.put32(caller - 16, c.r(28));
    Kept<28> chosen(c, seed);
    // A record form leaves the upper bits of the field's byte alone.
    const auto upper_bits = c.cr(0) & 0xf0;
    const bool seeded = nonzero(c, std::int32_t(seed));
    c.set_cr(0, std::uint8_t(upper_bits | c.cr(0)));
    c.put32(caller + 8, c.lr());
    c.put32(base, caller);
    c.set(1, base);
    scope.frame(base);

    if (!seeded) chosen = call(c, 0x1077f900, 0x107474d8, {0});
    call(c, 0x1077b790, 0x107474e4, {chosen});
    const auto report = c.toc(strings_slot) + 0x25b;
    c.set(5, chosen);
    call(c, format_text, 0x107474f8, {c.toc(-0x7bb8), report});

    // Signed division by a constant, as compiled: the quotient and its sign
    // correction stay in r4 and r5 for the second draw, and at return r4 holds the
    // second quotient times 65535 and r7 its correction.
    const auto first = call(c, 0x1077b770, 0x107474fc);
    const auto first_shifted = shift_right(c, high_product(0x291ee171, first), 13);
    const auto first_negative = first_shifted >> 31;
    const auto first_quotient = first_shifted + first_negative;
    c.set(0, pool + 2 * (first - first_quotient * rng::pool_samples));
    c.put32(cursor, c.r(0));
    leave(c, {{4, first_quotient}, {5, first_negative}});
    const auto second = call(c, 0x1077b770, 0x10747534);
    const auto start = c.u32(cursor);
    const auto end = pool + 2 * (rng::pool_samples - 1);
    const auto salt = c.toc(-0x7b90);
    compare_logical(c, start, pool);
    c.put32(last, end);
    const auto shifted = shift_right(c, high_product(0x80008001, second) + second, 15);
    const auto negative = shifted >> 31;
    const auto scaled = (shifted + negative) * pool_salts;
    c.put32(salt, second - scaled);
    leave(c, {{3, second - scaled}, {4, scaled}, {5, salt}, {6, pool_salts}, {7, negative}, {8, start}});
    if (start < pool || compare_logical(c, start, end) > 0) c.put32(cursor, pool);

    c.set(0, c.u32(base + 0x58));
    c.set(1, caller);
    c.set_lr(c.r(0));
    for (unsigned saved = 31; saved >= 28; --saved) c.set(saved, c.u32(caller - (32 - saved) * 4));
}
}
