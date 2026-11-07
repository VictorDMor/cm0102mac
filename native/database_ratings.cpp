// DATABASE.CPP's attribute ratings (database.hpp).
#include "database.hpp"
#include "database_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::database {
namespace {
using guest::Fpu;

// The constants the routines load from the pool at TOC -0x5614 (the pool
// pointer is read first; these reads have no effect the game can see and
// the pool is never unmapped). Only 0x100d2d70 reads the tenth.
struct Constants {
    double conversion, half, two, ten, twenty, three, one;
};
Constants constants(Cpu& c, std::uint32_t pool) {
    return {c.f64(pool + 0x18), c.f64(pool + 0x48), c.f64(pool + 0xa0), c.f64(pool + 0xb0),
            c.f64(pool + 0xb8), c.f64(pool + 0xc0), c.f64(pool + 0xc8)};
}

// The compiler's int-to-double conversion: the words 0x43300000 and
// value ^ 0x80000000 stored below r1, loaded as a double, minus 2^52 + 2^31.
constexpr std::uint32_t conversion_high = 0x43300000;
double converted(std::uint32_t flipped) {
    return std::bit_cast<double>(std::uint64_t(conversion_high) << 32 | flipped);
}

// From x on, the three routines run the same instructions: scale, clamp to
// [1, 20] and truncate. The result is stored below r1 (stfd) and read back.
std::int32_t rating(Cpu& c, Fpu& fpu, const Constants& k, double x) {
    const double scaled = fpu.fdiv(x, k.twenty);
    const double r = fpu.frsp(fpu.fadd(k.ten, scaled));
    const double square = fpu.fmuls(r, r);
    const double sum = fpu.fadd(r, fpu.fdiv(square, k.ten));
    const double s = fpu.frsp(fpu.fdiv(sum, k.three));
    // The original adds 0.5 again wherever it uses the value after a compare.
    double shown = fpu.fadd(k.half, s);
    fpu.fcmp(0, shown, k.one);
    shown = shown > k.one ? fpu.fadd(k.half, s) : k.one;  // ble: 1.0 unless greater
    fpu.fcmp(0, shown, k.twenty);
    double clamped;
    if (!(shown < k.twenty)) {  // bge: 20.0 unless less
        clamped = k.twenty;
    } else {
        shown = fpu.fadd(k.half, s);
        fpu.fcmp(0, shown, k.one);
        clamped = shown > k.one ? fpu.fadd(k.half, s) : k.one;
    }
    const auto bits = fpu.fctiwz(0, clamped);
    c.put64(c.sp() - 0x18, bits);
    c.set_f(1, shown);
    c.set_f(2, k.three);
    c.set_f(3, sum);
    c.set_f(4, square);
    c.set_f(5, s);
    const auto result = std::uint32_t(bits);
    c.set(3, result);
    return std::int32_t(result);
}

// 0x100d2f50, 0x100d2e60: x = the 16-bit value at `field` + 2 * value.
std::int32_t rating_from(Cpu& c, std::uint32_t record, std::int8_t value, std::uint32_t field) {
    const auto base = std::uint32_t(std::int32_t(c.i16(record + field)));
    const auto pool = c.toc(-0x5614);
    const auto k = constants(c, pool);
    const auto flipped_value = std::uint32_t(std::int32_t(value)) ^ 0x80000000u;
    const auto flipped_base = base ^ 0x80000000u;
    const auto sp = c.sp();
    c.put32(sp - 0x4, flipped_value);
    c.put32(sp - 0x8, conversion_high);
    c.put32(sp - 0xc, flipped_base);
    c.put32(sp - 0x10, conversion_high);
    Fpu fpu(c);
    const double adjustment = fpu.fsub(converted(flipped_value), k.conversion);
    const double from = fpu.fsub(converted(flipped_base), k.conversion);
    const double x = fpu.multiply_add(field == rated::value_24 ? 0x100d2fac : 0x100d2ebc, k.two, adjustment, from);
    c.set(0, conversion_high);
    c.set(4, conversion_high);
    c.set(5, flipped_value);
    c.set(6, pool);
    c.set_f(6, k.two);
    c.set_f(7, k.ten);
    return rating(c, fpu, k, x);
}
}

std::int32_t attribute_rating(Cpu& c, std::uint32_t record, std::int8_t value) {
    return rating_from(c, record, value, rated::value_24);
}
std::int32_t attribute_rating_04(Cpu& c, std::uint32_t record, std::int8_t value) {
    return rating_from(c, record, value, rated::value_04);
}

// 0x100d2d70: x = a tenth of the 16-bit value at +0x4 + 2 * value, in that
// order of operations (fmul, then the multiply-add).
std::int32_t attribute_rating_tenth(Cpu& c, std::uint32_t record, std::int8_t value) {
    const auto pool = c.toc(-0x5614);
    const auto base = std::uint32_t(std::int32_t(c.i16(record + rated::value_04)));
    const auto k = constants(c, pool);
    const double tenth = c.f64(pool + 0x60);
    const auto flipped_value = std::uint32_t(std::int32_t(value)) ^ 0x80000000u;
    const auto flipped_base = base ^ 0x80000000u;
    const auto sp = c.sp();
    c.put32(sp - 0x10, conversion_high);
    c.put32(sp - 0xc, flipped_value);
    c.put32(sp - 0x4, flipped_base);
    c.put32(sp - 0x8, conversion_high);
    Fpu fpu(c);
    const double doubled = fpu.fmul(k.two, fpu.fsub(converted(flipped_value), k.conversion));
    const double from = fpu.fsub(converted(flipped_base), k.conversion);
    const double x = fpu.multiply_add(0x100d2dd0, tenth, from, doubled);
    c.set(0, flipped_base);
    c.set(4, conversion_high);
    c.set(5, pool);
    c.set_f(6, k.ten);
    return rating(c, fpu, k, x);
}
}
