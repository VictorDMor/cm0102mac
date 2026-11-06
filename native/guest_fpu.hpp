#pragma once
// PowerPC floating point as the generated code computes it, for routines
// rewritten as plain C++: each operation's result and the FPSCR bits it
// leaves (one byte per bit at 1280 + n, as Ghidra's register file has them).
// FPSCR reaches setjmp's jmp_buf, so a rewrite leaves it as the original.
//
// Each method follows the p-code of its mnemonic in the export (they are the
// reference; checked for every FP instruction of PLAYER_STATS.CPP):
// - the result's class into FL, FG, FE, FU;
// - XX |= FI, except for fsub and fsubs, whose p-code omits it;
// - VXSNAN |= isnan(each operand);
// - fdiv, fdivs: ZX |= (divisor == 0), VXZDZ |= (both 0);
// - then the summaries VX, FX and FEX.
// Single-precision forms round the double result to single. Multiply-adds
// round once (fused_multiply_add.hpp, as the generator emits them). fctiwz
// is the generator's original_fctiwz. Nothing here changes the rounding mode
// (always round-to-nearest in the game).
#include "floating_point.hpp"
#include "fused_multiply_add.hpp"
#include "guest_abi.hpp"
#include <bit>
#include <cmath>

namespace cm::aot::guest {
class Fpu {
public:
    explicit Fpu(Cpu& c) : s(c.s) {}

    double fadd(double a, double b) { return status(a + b, true, {a, b}); }
    double fsub(double a, double b) { return status(a - b, false, {a, b}); }
    double fmul(double a, double b) { return status(a * b, true, {a, b}); }
    double fdiv(double a, double b) { return divided(a / b, a, b); }
    double fadds(double a, double b) { return status(single(a + b), true, {a, b}); }
    double fsubs(double a, double b) { return status(single(a - b), false, {a, b}); }
    double fmuls(double a, double b) { return status(single(a * b), true, {a, b}); }
    double fdivs(double a, double b) { return divided(single(a / b), a, b); }
    // The multiply-add at `site` in the original (d = a*c +/- b, single or
    // double, negated or not, as that instruction is: fused_site_kind), so
    // a rewrite cannot drift from it. Rounded once.
    double multiply_add(std::uint32_t site, double a, double c, double b) {
        return status(original_multiply_add(fused_site_kind(site), std::bit_cast<std::uint64_t>(a),
                                            std::bit_cast<std::uint64_t>(c), std::bit_cast<std::uint64_t>(b)),
                      true, {a, b, c});
    }
    double frsp(double a) { return status(single(a), true, {a}); }
    // fcmpo/fcmpu field,a,b: FPCC and the CR field (LT, GT, EQ, unordered).
    // The export's p-code sets no invalid bits for either.
    void fcmp(unsigned field, double a, double b) {
        const bool less = a < b, greater = b < a, equal = a == b;
        set(fl, less);
        set(fg, greater);
        set(fe, equal);
        s.wr<1>(reg::cr(field), std::uint8_t(less << 3 | greater << 2 | equal << 1 | (std::isnan(a) || std::isnan(b))));
    }
    // fctiwz into FPR `n` (its 64 bits, the low word the integer); returns
    // those bits. The original stores them to the stack (stfd) to read the
    // low word, so the high word reaches the stack image too.
    std::uint64_t fctiwz(unsigned n, double value) {
        s.wr<8>(reg::fpr(n), std::bit_cast<std::uint64_t>(value));
        original_fctiwz(s, reg::fpr(n), reg::fpr(n));
        return s.r<8>(reg::fpr(n));
    }
    // Single-precision loads and stores (lfs, stfs) set no status.
    static double single(double value) { return double(float(value)); }

private:
    static constexpr std::size_t fx = 1280, fex = 1281, vx = 1282, ox = 1283, zx = 1285, xx = 1286,
                                 vxsnan = 1287, vxzdz = 1290, fi = 1294, fl = 1296, fg = 1297, fe = 1298;
    std::uint8_t get(std::size_t bit) const { return std::uint8_t(s.r<1>(bit)); }
    void set(std::size_t bit, std::uint32_t value) { s.wr<1>(bit, value & 1); }
    double divided(double r, double a, double b) {
        result_class(r);
        set(zx, get(zx) | (b == 0));
        set(xx, get(xx) | get(fi));
        set(vxsnan, get(vxsnan) | std::isnan(a) | std::isnan(b));
        set(vxzdz, get(vxzdz) | (a == 0 && b == 0));
        summaries();
        return r;
    }
    double status(double r, bool inexact, std::initializer_list<double> operands) {
        result_class(r);
        if (inexact) set(xx, get(xx) | get(fi));
        std::uint8_t nan = get(vxsnan);
        for (const double operand : operands) nan |= std::isnan(operand);
        set(vxsnan, nan);
        summaries();
        return r;
    }
    void result_class(double r) {
        // FL, FG, FE and FU are four consecutive big-endian bytes.
        s.wr<4>(fl, std::uint32_t(r < 0) << 24 | std::uint32_t(0 < r) << 16 |
                    std::uint32_t(r == 0) << 8 | std::uint32_t(std::isnan(r)));
    }
    void summaries() {
        // Invalid detail bits occupy bytes 1287–1292 and 1301–1303.
        const auto invalid = (s.r<8>(vxsnan) & 0x0101010101010000ULL) | (s.r<4>(1300) & 0x00010101u);
        set(vx, invalid != 0);
        // FX is sticky. FEX uses the export's XOR across VX through XX and
        // their enable bytes. The masks select only the low bit of each byte.
        set(fx, get(fx) | ((s.r<4>(ox) & 0x01010101u) != 0));
        const auto enabled = s.r<8>(vx) & s.r<8>(vx + 22) & 0x0101010101000000ULL;
        set(fex, std::popcount(enabled) & 1);
    }
    State& s;
};
}
