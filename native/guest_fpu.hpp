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
    static constexpr std::size_t fx = 1280, fex = 1281, vx = 1282, ox = 1283, ux = 1284, zx = 1285, xx = 1286,
                                 vxsnan = 1287, vxzdz = 1290, fi = 1294, fl = 1296, fg = 1297, fe = 1298, fu = 1299;
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
        set(fl, r < 0);
        set(fg, 0 < r);
        set(fe, r == 0);
        set(fu, std::isnan(r));
    }
    void summaries() {
        std::uint8_t invalid = 0;
        for (const std::size_t bit : {1287u, 1288u, 1289u, 1290u, 1291u, 1292u, 1301u, 1302u, 1303u}) invalid |= get(bit);
        set(vx, invalid);
        set(fx, get(fx) | get(ox) | get(ux) | get(zx) | get(xx));
        std::uint8_t enabled = 0;
        for (const std::size_t bit : {vx, ox, ux, zx, xx}) enabled ^= get(bit) & get(bit + 22);
        set(fex, enabled);
    }
    State& s;
};
}
