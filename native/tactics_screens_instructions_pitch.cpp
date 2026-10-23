// The pitch the tactics screens draw (0x103cbb30): grass with lighter
// stripes, the lines, the centre circle, both penalty areas with their
// arcs, goal areas, goals and penalty spots, across (goals left and right)
// or upright. Positions inside the pitch are its inner corners offset by
// multiples of a scale, computed in doubles and truncated as the original
// does, with every floating-point status bit and stack temporary it leaves.
#include "tactics_screens_instructions.hpp"
#include "floating_point.hpp"
#include "fused_multiply_add.hpp"
#include <algorithm>
#include <array>
#include <bit>
#include <cmath>
#include <initializer_list>
#include <stdexcept>

namespace cm::aot::tactics_screens::instructions {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t draw_shape = 0x10323470, make_colour = 0x1076bb20;
// Offsets into the constant table at TOC -0x4cb0 (doubles).
constexpr std::uint32_t bias = 0x8;                     // 2^52 + 2^31: the int-to-double idiom
// In yards (times the scale): the margin round the pitch, which is also
// where the penalty arc starts; the circles' radius; the arc's far end and
// half the penalty area's width; the penalty area's depth; the penalty spot;
// the goal area's depth.
constexpr std::uint32_t margin = 0x18, radius = 0x20, reach = 0x28, area = 0x30, spot = 0x40, goal_area = 0x48;
constexpr std::uint32_t one = 0x38;  // 1.0

// Double arithmetic as the generated code does it, with its FPSCR bits (one
// byte per bit at 1280 + n): the result class, the sticky invalid (any NaN
// operand), the summaries, and, for the additions and multiply-adds, XX |= FI.
class Fpu {
public:
    explicit Fpu(State& state) : s(state) {}
    double sub(double a, double b) { return status(a - b, {a, b}, false); }
    double add(double a, double b) { return status(a + b, {a, b}, true); }
    double multiply_add(std::uint8_t kind, double a, double c, double b) {
        const auto r = original_multiply_add(kind, std::bit_cast<std::uint64_t>(a), std::bit_cast<std::uint64_t>(c),
                                             std::bit_cast<std::uint64_t>(b));
        return status(r, {a, c, b}, true);
    }

private:
    static constexpr std::size_t fx = 1280, fex = 1281, vx = 1282, xx = 1286, vxsnan = 1287, fi = 1294, fl = 1296,
                                 fg = 1297, fe = 1298, fu = 1299;
    std::uint8_t get(std::size_t bit) const { return std::uint8_t(s.r<1>(bit)); }
    void set(std::size_t bit, std::uint32_t value) { s.wr<1>(bit, value); }
    double status(double r, std::initializer_list<double> operands, bool inexact) {
        set(fl, r < 0); set(fg, 0 < r); set(fe, r == 0); set(fu, std::isnan(r));
        if (inexact) set(xx, get(xx) | get(fi));
        std::uint8_t nan = 0;
        for (const auto operand : operands) nan |= std::isnan(operand);
        set(vxsnan, get(vxsnan) | nan);
        std::uint8_t invalid = 0;
        for (const std::size_t bit : {1287u, 1288u, 1289u, 1290u, 1291u, 1292u, 1301u, 1302u, 1303u}) invalid |= get(bit);
        set(vx, invalid);
        set(fx, get(fx) | get(1283) | get(1284) | get(1285) | get(xx));
        std::uint8_t enabled = 0;
        for (const std::size_t bit : {vx, std::size_t(1283), std::size_t(1284), std::size_t(1285), xx})
            enabled ^= get(bit) & get(bit + 22);
        set(fex, enabled);
        return r;
    }
    State& s;
};

class Pitch {
public:
    // `across`: the path at 0x103cbc2c, whose conversions take stack slots
    // from 0x320 down; the upright one (0x103ccf9c) takes them from 0x68 up.
    Pitch(Cpu& cpu, const Frame& f, bool across)
        : c(cpu), frame(f), fpu(cpu.s), slot(across ? 0x320 : 0x68), step(across ? -0x10 : 0x10) {
        const auto start = across ? 0x103cbc2cu : 0x103ccf9cu;
        site = std::lower_bound(fused_sites, fused_sites + fused_site_count, start,
                                [](const FusedSite& s, std::uint32_t value) { return s.address < value; });
    }
    // x + k * scale and x - k * scale, truncated; the `_less_one` and
    // `_plus_one` forms subtract or add 1 before truncating.
    std::uint32_t plus(std::uint32_t k, std::uint32_t x) { return convert(k, x, false, 0); }
    std::uint32_t minus(std::uint32_t k, std::uint32_t x) { return convert(k, x, true, 0); }
    std::uint32_t plus_less_one(std::uint32_t k, std::uint32_t x) { return convert(k, x, false, -1); }
    std::uint32_t minus_plus_one(std::uint32_t k, std::uint32_t x) { return convert(k, x, true, 1); }
    // One shape (0x10323470): its kind, colour and fill colour, and corners.
    void draw(std::uint32_t kind, std::uint32_t colour, std::uint32_t fill, const std::array<std::uint32_t, 4>& at,
              std::uint32_t back) {
        auto arg = frame.at(0x38);
        for (const auto word : {kind, colour, 0u, 0u, 0u, fill, 0u, 0u, 0u, 0u, c.r(30)}) {
            c.put32(arg, word);
            arg += 4;
        }
        c.set({{3, c.r(27)}, {4, 1}, {5, at[0]}, {6, at[1]}, {7, at[2]}, {8, at[3]}, {9, 0}, {10, 0}});
        c.call(draw_shape, back);
    }

private:
    double constant(std::uint32_t offset) const { return c.f64(c.r(31) + offset); }
    std::uint32_t convert(std::uint32_t k, std::uint32_t x, bool minus, int adjust) {
        // (double)x by the 0x43300000 idiom, k * scale +- it (one multiply-add,
        // whose form the original instruction gives), then truncation.
        const auto in = frame.at(slot);
        const auto out = in + std::uint32_t(step > 0 ? 8 : -8);
        slot += step;
        c.put32(in, 0x43300000);
        c.put32(in + 4, x ^ 0x80000000u);
        const auto value = fpu.sub(c.f64(in), constant(bias));
        const auto kind = (site++)->kind;
        if (bool(kind & fused_negate) != minus || bool(kind & fused_subtract) != minus || (kind & fused_single))
            throw std::logic_error("Unexpected multiply-add in the pitch drawing");
        auto r = fpu.multiply_add(kind, constant(k), c.f(31), value);
        if (adjust < 0) r = fpu.sub(r, constant(one));
        if (adjust > 0) r = fpu.add(constant(one), r);
        c.set_f(0, r);
        original_fctiwz(c.s, guest::reg::fpr(0), guest::reg::fpr(0));
        c.put64(out, c.s.r<8>(guest::reg::fpr(0)));
        return c.u32(out + 4);
    }
    Cpu& c;
    const Frame& frame;
    Fpu fpu;
    std::int32_t slot, step;
    const FusedSite* site;
};

// from + (to - from) * parts / 7 and the middle of [from, to], in the
// original's wrapping 32-bit arithmetic with signed division.
std::uint32_t seventh(std::uint32_t from, std::uint32_t to, std::uint32_t parts) {
    return from + std::uint32_t(std::int32_t((to - from) * parts) / 7);
}
std::uint32_t middle(std::uint32_t from, std::uint32_t to) { return from + std::uint32_t(std::int32_t(to - from + 1) / 2); }
}

std::uint32_t draw_pitch(Cpu& c, double scale, std::uint32_t left, std::uint32_t top, std::uint32_t right,
                         std::uint32_t bottom, std::uint32_t out_left, std::uint32_t out_top, std::uint32_t out_right,
                         std::uint32_t out_bottom) {
    Frame frame(c, 0x103cbb30, 0x370,
                {.first_gpr = 18, .gpr_offset = -0x48, .first_fpr = 31, .fpr_offset = -0x8});
    // Stack arguments: byte 9 (+0x3b) across, short 10 (+0x3e) passed on to every shape.
    const auto id = std::uint32_t(std::int32_t(c.i16(frame.caller() + 0x3e)));
    c.set({{22, left}, {23, top}, {18, right}, {19, bottom}, {28, out_left}, {29, out_top}, {25, out_right},
           {26, out_bottom}, {30, id}, {31, c.toc(-0x4cb0)}, {27, c.toc(-0x7bb0)}});
    c.set_f(31, scale);
    // The grass colours, made once; the lines' colour, made each time.
    const auto once = [&](std::int32_t made, std::int32_t colour, std::uint32_t value, std::uint32_t back) {
        if (c.i8(c.r(2) + std::uint32_t(made))) return;
        c.set({{3, 0}, {4, value}, {5, 0}, {6, 0}});
        c.call(make_colour, back);
        c.put16(c.r(2) + std::uint32_t(colour), c.r(3));
        c.put8(c.r(2) + std::uint32_t(made), 1);
    };
    once(-0x3832, -0x3830, 0x40, 0x103cbb98);
    once(-0x3836, -0x3834, 0x38, 0x103cbbc8);
    const bool plain = std::int16_t(id) == -1;
    c.set({{3, 0}, {4, plain ? 0x80u : 0x50u}, {5, 0}, {6, 0}});
    c.call(make_colour, plain ? 0x103cbbf4 : 0x103cbc14);
    c.set(24, c.r(3));
    const auto lines = c.r(24);
    const auto grass = std::uint32_t(c.u16(c.r(2) - 0x3830)), stripe = std::uint32_t(c.u16(c.r(2) - 0x3834));
    const auto l = left, t = top, r = right, b = bottom;

    const bool across = c.i8(frame.caller() + 0x3b) != 0;
    Pitch p(c, frame, across);
    const auto background = plain ? 0x30u : 0x70u;
    // The inner pitch: the outer rectangle less the margin, written out.
    const auto inner = [&] {
        c.put32(out_left, p.plus(margin, l));
        c.put32(out_top, p.plus(margin, t));
        c.put32(out_right, p.minus(margin, r));
        c.put32(out_bottom, p.minus(margin, b));
        const auto L = c.u32(out_left), T = c.u32(out_top), R = c.u32(out_right), B = c.u32(out_bottom);
        c.set({{21, middle(L, R)}, {20, middle(T, B)}});
        return std::array<std::uint32_t, 4>{L, T, R, B};
    };
    if (across) {
        p.draw(background, grass, 0, {l, t, r, b}, plain ? 0x103cbcb4 : 0x103cbd38);
        p.draw(0x10, stripe, 0, {l + 1, seventh(t, b, 1), r - 1, seventh(t, b, 2)}, 0x103cbdf8);
        p.draw(0x10, stripe, 0, {l + 1, seventh(t, b, 3), r - 1, seventh(t, b, 4)}, 0x103cbec0);
        p.draw(0x10, stripe, 0, {l + 1, seventh(t, b, 5), r - 1, seventh(t, b, 6)}, 0x103cbf88);
        const auto [L, T, R, B] = inner();
        const auto cx = c.r(21), cy = c.r(20);
        p.draw(0x220, lines, 0, {L, T, R, B}, 0x103cc118);
        p.draw(0x400, lines, 0, {cx, T, cx, B}, 0x103cc190);
        p.draw(0x10020, lines, lines, {p.minus(radius, cx), p.minus(radius, cy), p.plus(radius, cx), p.plus(radius, cy)},
               0x103cc2b8);
        // The left end: the penalty arc (a circle round the spot, its part
        // inside the area painted over with grass, then with the stripe it
        // crosses), the spot, the goal area and the penalty area.
        p.draw(0x10020, lines, lines, {p.plus(margin, L), p.minus(radius, cy), p.plus(reach, L), p.plus(radius, cy)},
               0x103cc3e8);
        p.draw(0x10, grass, 0, {p.plus(margin, L), p.minus(radius, cy), p.plus_less_one(area, L), p.plus(radius, cy)},
               0x103cc528);
        p.draw(0x10, stripe, 0, {p.plus(margin, L), seventh(t, b, 3), p.plus_less_one(area, L), seventh(t, b, 4)},
               0x103cc654);
        p.draw(0x400, lines, 0, {p.plus(spot, L), cy, p.plus(spot, L), cy}, 0x103cc728);
        p.draw(0x220, lines, 0, {L, p.minus(radius, cy), p.plus(goal_area, L), p.plus(radius, cy)}, 0x103cc828);
        p.draw(0x220, lines, 0, {L, p.minus(reach, cy), p.plus(area, L), p.plus(reach, cy)}, 0x103cc928);
        // The right end.
        p.draw(0x10020, lines, lines, {p.minus(reach, R), p.minus(radius, cy), p.minus(margin, R), p.plus(radius, cy)},
               0x103cca58);
        p.draw(0x10, grass, 0, {p.minus_plus_one(area, R), p.minus(radius, cy), p.minus(margin, R), p.plus(radius, cy)},
               0x103ccb98);
        p.draw(0x10, stripe, 0, {p.minus_plus_one(area, R), seventh(t, b, 3), p.minus(margin, R), seventh(t, b, 4)},
               0x103cccc4);
        p.draw(0x400, lines, 0, {p.minus(spot, R), cy, p.minus(spot, R), cy}, 0x103ccd9c);
        p.draw(0x220, lines, 0, {p.minus(goal_area, R), p.minus(radius, cy), R, p.plus(radius, cy)}, 0x103cce98);
        p.draw(0x220, lines, 0, {p.minus(area, R), p.minus(reach, cy), R, p.plus(reach, cy)}, 0x103ccf94);
    } else {
        p.draw(background, grass, 0, {l, t, r, b}, plain ? 0x103cd024 : 0x103cd0a8);
        p.draw(0x10, stripe, 0, {seventh(l, r, 1), t + 1, seventh(l, r, 2), b - 1}, 0x103cd16c);
        p.draw(0x10, stripe, 0, {seventh(l, r, 3), t + 1, seventh(l, r, 4), b - 1}, 0x103cd234);
        p.draw(0x10, stripe, 0, {seventh(l, r, 5), t + 1, seventh(l, r, 6), b - 1}, 0x103cd2fc);
        const auto [L, T, R, B] = inner();
        const auto cx = c.r(21), cy = c.r(20);
        p.draw(0x220, lines, 0, {L, T, R, B}, 0x103cd48c);
        p.draw(0x400, lines, 0, {L, cy, R, cy}, 0x103cd504);
        p.draw(0x10020, lines, lines, {p.minus(radius, cx), p.minus(radius, cy), p.plus(radius, cx), p.plus(radius, cy)},
               0x103cd628);
        // The top end.
        p.draw(0x10020, lines, lines, {p.minus(radius, cx), p.plus(margin, T), p.plus(radius, cx), p.plus(reach, T)},
               0x103cd758);
        p.draw(0x10, grass, 0, {p.minus(radius, cx), p.plus(margin, T), p.plus(radius, cx), p.plus_less_one(area, T)},
               0x103cd894);
        p.draw(0x10, stripe, 0, {seventh(l, r, 3), p.plus(margin, T), seventh(l, r, 4), p.plus_less_one(area, T)},
               0x103cd9c4);
        p.draw(0x400, lines, 0, {cx, p.plus(spot, T), cx, p.plus(spot, T)}, 0x103cda9c);
        p.draw(0x220, lines, 0, {p.minus(radius, cx), T, p.plus(radius, cx), p.plus(goal_area, T)}, 0x103cdb98);
        p.draw(0x220, lines, 0, {p.minus(reach, cx), T, p.plus(reach, cx), p.plus(area, T)}, 0x103cdc98);
        // The bottom end.
        p.draw(0x10020, lines, lines, {p.minus(radius, cx), p.minus(reach, B), p.plus(radius, cx), p.minus(margin, B)},
               0x103cddc4);
        p.draw(0x10, grass, 0, {p.minus(radius, cx), p.minus_plus_one(area, B), p.plus(radius, cx), p.minus(margin, B)},
               0x103cdf04);
        p.draw(0x10, stripe, 0, {seventh(l, r, 3), p.minus_plus_one(area, B), seventh(l, r, 4), p.minus(margin, B)},
               0x103ce034);
        p.draw(0x400, lines, 0, {cx, p.minus(spot, B), cx, p.minus(spot, B)}, 0x103ce10c);
        p.draw(0x220, lines, 0, {p.minus(radius, cx), p.minus(goal_area, B), p.plus(radius, cx), B}, 0x103ce208);
        p.draw(0x220, lines, 0, {p.minus(reach, cx), p.minus(area, B), p.plus(reach, cx), B}, 0x103ce308);
    }
    return 1;
}
}
