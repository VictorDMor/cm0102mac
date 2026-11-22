// The font engine, block `fnt_a` (FNT, first third, 0x107559a0..0x10757fb0): the TrueType interpreter's entry and its two loops, the
// rounding and projection callbacks, and the opcodes that set the vectors, the reference points and the zone pointers, move the stack
// and shift points. Every routine here is reached through a pointer (the opcode table the installer 0x1075a500 builds, the function
// words of the execution context) except the three internal helpers and the entry 0x10756310 that new_hint_font, set_hint_size and
// load_glyph_program call. The game runs the entry and the plain loop at load (symbol.ttf's fpgm) and no handler of this block; the
// others are checked with cases only.
//
// Contract. A handler is called through the glue with r3 = the execution context (interpreter::, a record on the entry's stack) and
// returns to the loop or to another handler, which reload what they read; the registers it leaves are compared in full. Where the
// original calls a fixed-point helper (UTIL) the leftovers it leaves in r4-r8 are kept: `Vol` is the shadow of r3-r10 updated where the
// original's instructions write them. The entry and the loops build the original's frames (the callees run below them), hold the
// registers the original holds at the calls through the glue, and the entry takes the original's setjmp: an illegal opcode (0x107559a0)
// or a bad table longjmps to it with 0x1101 or 0x1102, which it returns as its result.
#include "font_engine_detail.hpp"
#include <array>

namespace cm::aot::font_engine {
namespace {
using guest::Frame;
constexpr std::uint32_t indirect_glue = 0x10781320;


// A halfword as a register holds it after lha, and the 16.16 to halfword steps the original takes.
constexpr std::uint32_t lha(const Cpu& c, std::uint32_t at) { return std::uint32_t(std::int32_t(c.i16(at))); }
constexpr std::uint32_t extsh(std::uint32_t v) { return std::uint32_t(std::int32_t(std::int16_t(v))); }
constexpr std::int32_t s32(std::uint32_t v) { return std::int32_t(v); }
// srawi: an arithmetic shift of a register.
constexpr std::uint32_t sra(std::uint32_t v, unsigned n) { return std::uint32_t(std::int32_t(v) >> n); }

// r3-r10 as the original's instructions hold them (index = register number). The fixed-point helpers write r3 and the leftovers they
// leave in r4-r8 (the registers the original already holds there are what they keep).
struct Vol {
    std::array<std::uint32_t, 11> r{};
    explicit Vol(const Cpu& c) { load(c); }
    void load(const Cpu& c) { for (unsigned n = 3; n <= 10; ++n) r[n] = c.r(n); }
    void store(Cpu& c) const { for (unsigned n = 3; n <= 10; ++n) c.set(n, r[n]); }
    Leftovers left() const { return {r[4], r[5], r[6], r[7], r[8]}; }
    std::uint32_t done(std::int32_t result, const Leftovers& l) {
        r[3] = std::uint32_t(result); r[4] = l.r4; r[5] = l.r5; r[6] = l.r6; r[7] = l.r7; r[8] = l.r8;
        return r[3];
    }
    std::uint32_t fix_mul(std::uint32_t a, std::uint32_t b) { auto l = left(); return done(fix_multiply(s32(a), s32(b), l), l); }
    std::uint32_t fix_div(std::uint32_t a, std::uint32_t b) { auto l = left(); return done(fix_divide(s32(a), s32(b), l), l); }
    std::uint32_t scale_mul(std::uint32_t a, std::uint32_t b) { auto l = left(); return done(scale_multiply(s32(a), s32(b), l), l); }
    std::uint32_t dist(std::uint32_t a, std::uint32_t b) { auto l = left(); return done(distance(s32(a), s32(b), l), l); }
};

// `bl 0x10781320` with r12 = the descriptor: the glue calls through it; the loop and the handlers reload the TOC from sp+0x14 afterwards.
void call_glue(Cpu& c, Vol& v, std::uint32_t descriptor, std::uint32_t back) {
    v.store(c);
    c.set(12, descriptor);
    c.call(indirect_glue, back);
    c.set(2, c.u32(c.sp() + 0x14));
    v.load(c);
}

// The word a handler pops off the interpreter's stack (the pointer is stored first).
std::uint32_t pop(Cpu& c, std::uint32_t context) {
    const auto top = c.u32(context + interpreter::stack_top) - 4;
    c.put32(context + interpreter::stack_top, top);
    return c.u32(top);
}
void push(Cpu& c, std::uint32_t context, std::uint32_t value) {
    const auto top = c.u32(context + interpreter::stack_top);
    c.put32(context + interpreter::stack_top, top + 4);
    c.put32(top, value);
}

// The sign-keeping rounder of the grid functions: |value| + compensation + bias, masked, the sign put back; where the sign would flip the
// result is zero (a zero value keeps what the arithmetic gave).
constexpr std::int32_t grid_round(std::int32_t value, std::int32_t compensation, std::uint32_t bias, std::uint32_t mask) {
    const auto n = value >= 0 ? wrap((std::uint32_t(value) + std::uint32_t(compensation) + bias) & mask)
                              : wrap(negate((negate(std::uint32_t(value)) + std::uint32_t(compensation) + bias) & mask));
    if (s32(std::uint32_t(value) ^ std::uint32_t(n)) >= 0 || value == 0) return n;
    return 0;
}
}

// 0x107559a0: the handler of ENDF outside a definition and of every undefined opcode: longjmp(context + 0x5c, 0x1101) to the entry's setjmp.
void illegal_opcode(Cpu& c, std::uint32_t context) {
    Frame frame(c, 0x107559a0, 0x40, {});
    c.set({{3, context + interpreter::jump_buffer}, {4, interpreter::error_illegal_opcode}});
    c.call(tsimem::longjmp_entry, 0x107559b8);
}

// ---- The rounding functions (the graphic state's round_function; called through the glue with (context, value, compensation)) ----------
std::int32_t round_to_double_grid(std::int32_t value, std::int32_t compensation) { return grid_round(value, compensation, 0x10, 0xffffffe0u); }
std::int32_t round_down_to_grid(std::int32_t value, std::int32_t compensation) { return grid_round(value, compensation, 0, 0xffffffc0u); }
std::int32_t round_up_to_grid(std::int32_t value, std::int32_t compensation) { return grid_round(value, compensation, 0x3f, 0xffffffc0u); }
std::int32_t round_to_grid(std::int32_t value, std::int32_t compensation) { return grid_round(value, compensation, 0x20, 0xffffffc0u); }
std::int32_t round_to_half_grid(std::int32_t value, std::int32_t compensation) {
    const auto up = [&](std::uint32_t distance) { return ((distance + std::uint32_t(compensation)) & 0xffffffc0u) + 0x20; };
    const auto n = value >= 0 ? wrap(up(std::uint32_t(value))) : wrap(negate(up(negate(std::uint32_t(value)))));
    if (s32(std::uint32_t(value) ^ std::uint32_t(n)) >= 0 || value == 0) return n;
    return value < 0 ? -0x20 : 0x20;
}
std::int32_t round_off(std::int32_t value) { return value; }

// 0x10755b60: the super round: the period's mask with the phase and the threshold of the graphic state.
void round_super(Cpu& c, std::uint32_t context, std::int32_t value, std::int32_t compensation) {
    using namespace graphic_state;
    const auto g = c.u32(context + interpreter::state);
    const auto phase = lha(c, g + round_phase);
    const auto span = lha(c, g + round_threshold) - phase;
    const auto mask = c.u32(g + round_mask);
    std::uint32_t n;
    c.set({{6, g}, {7, phase}});
    if (value >= 0) {
        c.set(5, span);
        n = ((std::uint32_t(value) + std::uint32_t(compensation) + span) & mask) + phase;
    } else {
        const auto masked = ((negate(std::uint32_t(value)) + std::uint32_t(compensation) + span) & mask) + phase;
        c.set(8, masked);
        n = negate(masked);
    }
    if (!(s32(std::uint32_t(value) ^ n) >= 0 || value == 0)) n = value > 0 ? phase : negate(phase);
    c.set(3, n);
}

// 0x10755be0: the 45-degree round: the same, in units of the 2.30 period (fix_divide, mask, fix_multiply).
void round_super_45(Cpu& c, std::uint32_t context, std::int32_t value, std::int32_t compensation) {
    using namespace graphic_state;
    Frame frame(c, 0x10755be0, 0x40, {.first_gpr = 30});
    Vol v(c);
    const auto g = c.u32(context + interpreter::state);
    const auto phase = lha(c, g + round_phase);
    const auto period = [&] { return c.u32(g + round_period_45); };
    v.r[6] = phase;
    v.r[4] = lha(c, g + round_threshold) - phase;
    const auto distance = value >= 0 ? std::uint32_t(value) : negate(std::uint32_t(value));
    const auto quotient = v.fix_div(distance + std::uint32_t(compensation) + v.r[4], sra(period(), 14));
    const auto product = v.fix_mul(quotient & 0xffffffc0u, sra(period(), 14));
    auto n = value >= 0 ? product + phase : negate(product + lha(c, g + round_phase));
    if (!(s32(std::uint32_t(value) ^ n) >= 0 || value == 0)) n = value > 0 ? lha(c, g + round_phase) : negate(lha(c, g + round_phase));
    v.r[3] = n;
    v.store(c);
}

// ---- The move and project functions the context's words hold ----------------------------------------------------------------------
// 0x10755cc0: the point of a zone moved by `delta` along the freedom vector (the 2.14 components through scale_multiply when the
// vectors are one axis' worth apart, else through fix_divide by the projection . freedom product), the touched bits set.
void move_general(Cpu& c, std::uint32_t context, std::uint32_t zone, std::uint32_t point, std::uint32_t delta) {
    Frame frame(c, 0x10755cc0, 0x50, {.first_gpr = 27});
    Vol v(c);
    const auto dot = lha(c, context + interpreter::vector_dot), free_x = lha(c, context + interpreter::freedom_x), free_y = lha(c, context + interpreter::freedom_y);
    v.r[4] = free_x;
    const auto apply = [&](std::uint32_t array, std::uint8_t bit, std::uint32_t moved) {
        v.r[5] = c.u32(zone + array);
        v.r[4] = point << 2;
        c.put32(v.r[5] + v.r[4], c.u32(v.r[5] + v.r[4]) + moved);
        v.r[3] = c.u32(zone + zone::touched);
        c.put8(v.r[3] + point, c.u8(v.r[3] + point) | bit);
    };
    if (dot == 0x4000) {
        if (free_x != 0) apply(zone::xs, zone::touched_x, v.scale_mul(delta, free_x));
        if (free_y != 0) apply(zone::ys, zone::touched_y, v.scale_mul(delta, free_y));
    } else {
        if (free_x != 0) { const auto q = v.fix_div(free_x, dot); apply(zone::xs, zone::touched_x, v.fix_mul(delta, q)); }
        if (free_y != 0) { const auto q = v.fix_div(free_y, dot); apply(zone::ys, zone::touched_y, v.fix_mul(delta, q)); }
    }
    v.store(c);
}

// 0x10755e00, 0x10755e30: a point moved along x only, or y only (the freedom vector is an axis): `delta` added, the touched bit set.
void move_x(Cpu& c, std::uint32_t zone_at, std::uint32_t point, std::uint32_t delta) {
    const auto xs = c.u32(zone_at + zone::xs);
    c.put32(xs + (point << 2), c.u32(xs + (point << 2)) + delta);
    const auto touched = c.u32(zone_at + zone::touched);
    c.put8(touched + point, c.u8(touched + point) | zone::touched_x);
    c.set({{3, touched}, {7, xs}});
}
void move_y(Cpu& c, std::uint32_t zone_at, std::uint32_t point, std::uint32_t delta) {
    const auto ys = c.u32(zone_at + zone::ys);
    c.put32(ys + (point << 2), c.u32(ys + (point << 2)) + delta);
    const auto touched = c.u32(zone_at + zone::touched);
    c.put8(touched + point, c.u8(touched + point) | zone::touched_y);
    c.set({{3, touched}, {7, ys}});
}
// 0x10755f20, 0x10755f30: the length along an axis is the coordinate itself.
std::int32_t project_x(std::int32_t x, std::int32_t) { return x; }
std::int32_t project_y(std::int32_t, std::int32_t y) { return y; }

// 0x10755e60, 0x10755ec0: the length of (x, y) along the projection vector, or the dual projection vector (scale_multiply by the 2.14 components).
void project_general(Cpu& c, std::uint32_t context, std::uint32_t x, std::uint32_t y) {
    Frame frame(c, 0x10755e60, 0x40, {.first_gpr = 30});
    Vol v(c);
    const auto along_y = v.scale_mul(y, lha(c, context + interpreter::projection_y));
    const auto along_x = v.scale_mul(x, lha(c, context + interpreter::projection_x));
    v.r[3] = along_x + along_y;
    v.store(c);
}
void dual_project_general(Cpu& c, std::uint32_t context, std::uint32_t x, std::uint32_t y) {
    Frame frame(c, 0x10755ec0, 0x40, {.first_gpr = 30});
    Vol v(c);
    const auto along_y = v.scale_mul(y, lha(c, context + interpreter::dual_y));
    const auto along_x = v.scale_mul(x, lha(c, context + interpreter::dual_x));
    v.r[3] = along_x + along_y;
    v.store(c);
}

// 0x10755f40: a cvt entry as stored. 0x10756020: the single width value as stored.
std::uint32_t read_cvt(Cpu& c, std::uint32_t context, std::uint32_t index) {
    return c.u32(c.u32(c.u32(context + interpreter::state) + graphic_state::cvt) + (index << 2));
}
std::uint32_t single_width(Cpu& c, std::uint32_t context) { return c.u32(c.u32(context + interpreter::state) + graphic_state::single_width_value); }

// 0x10755f60, 0x10756030: the same scaled by the ratio of the projection vector's axes: ratio_up for a vertical one, ratio_down for
// a horizontal one, and for a slanted one ratio_up * (py^2) + ratio_down * (px^2) (each square a 2.14 value from its 2.14 component).
namespace {
std::uint32_t axis_ratio(Cpu& c, Vol& v, std::uint32_t context, std::uint32_t g) {
    const auto px = lha(c, context + interpreter::projection_x), py = lha(c, context + interpreter::projection_y);
    if (py == 0) return v.r[4] = c.u32(g + graphic_state::ratio_down);
    if (px == 0) return v.r[4] = c.u32(g + graphic_state::ratio_up);
    const auto square = [](std::uint32_t h) { return extsh(sra(h * h + 0x2000, 14)); };
    v.r[4] = square(py);
    const auto vertical = v.scale_mul(c.u32(g + graphic_state::ratio_up), v.r[4]);
    v.r[4] = square(px);
    const auto horizontal = v.scale_mul(c.u32(g + graphic_state::ratio_down), v.r[4]);
    return v.r[4] = vertical + horizontal;
}
}
void read_cvt_scaled(Cpu& c, std::uint32_t context, std::uint32_t index) {
    Frame frame(c, 0x10755f60, 0x50, {.first_gpr = 28});
    Vol v(c);
    const auto g = c.u32(context + interpreter::state);
    const auto ratio = axis_ratio(c, v, context, g);
    v.fix_mul(c.u32(c.u32(g + graphic_state::cvt) + (index << 2)), ratio);
    v.store(c);
}
void single_width_scaled(Cpu& c, std::uint32_t context) {
    Frame frame(c, 0x10756030, 0x50, {.first_gpr = 29});
    Vol v(c);
    const auto g = c.u32(context + interpreter::state);
    const auto ratio = axis_ratio(c, v, context, g);
    v.fix_mul(c.u32(g + graphic_state::single_width_value), ratio);
    v.store(c);
}
// 0x107560e0: a cvt entry moved by `delta`.
void shift_cvt(Cpu& c, std::uint32_t context, std::uint32_t index, std::uint32_t delta) {
    const auto cvt = c.u32(c.u32(context + interpreter::state) + graphic_state::cvt);
    c.put32(cvt + (index << 2), c.u32(cvt + (index << 2)) + delta);
    c.set({{3, index << 2}, {4, cvt}});
}

// ---- The loops and the entry ------------------------------------------------------------------------------------------------------
// 0x10756280: run the program from `start` while the instruction pointer is below `end`: fetch the opcode, advance, store the opcode in the
// context and call the table's descriptor through the glue with (context, the opcode's address). The instruction pointer is put back at the end.
void run(Cpu& c, std::uint32_t context, std::uint32_t start, std::uint32_t end) {
    Frame frame(c, 0x10756280, 0x50, {.first_gpr = 28});
    const auto g = c.u32(context + interpreter::state);
    const auto old_ip = c.u32(context + interpreter::ip);
    c.put32(context + interpreter::ip, start);
    const auto table = c.u32(g + graphic_state::opcode_table);
    c.set(3, g);
    c.set({{28, context}, {29, end}, {30, old_ip}, {31, table}});
    while (c.u32(context + interpreter::ip) < end) {
        const auto at = c.u32(context + interpreter::ip);
        c.put32(context + interpreter::ip, at + 1);
        const auto opcode = c.u8(at);
        c.put32(context + interpreter::opcode, opcode);
        c.set({{3, context}, {4, at}, {12, c.u32(table + 4 * opcode)}});
        c.call(indirect_glue, 0x107562dc);
        c.set(2, c.u32(c.sp() + 0x14));
    }
    c.put32(context + interpreter::ip, old_ip);
}

// 0x10756100: the same with a trace callback (the context's `trace` word): before each opcode the round function is classified (the eight
// round descriptors of the TOC give 1, 0, 2, 3, 4, 5, 6, 7; any other, 0xffff) into the context's round_index and the callback called with
// the context; a callback that clears `trace` ends the loop. Without a trace callback the loop returns at once, the instruction pointer left at `start`.
void run_traced(Cpu& c, std::uint32_t context, std::uint32_t start, std::uint32_t end) {
    Frame frame(c, 0x10756100, 0x70, {.first_gpr = 19});
    const std::array<std::int32_t, 8> slots = {-0x5b58, -0x5b54, -0x5b50, -0x5b4c, -0x5b48, -0x5b44, -0x5b40, -0x5b3c};  // r23..r30
    const std::array<std::uint32_t, 8> classes = {7, 6, 5, 4, 3, 2, 0, 1};
    std::array<std::uint32_t, 8> round{};
    for (unsigned n = 0; n < 8; ++n) round[n] = c.toc(slots[n]);
    const auto g = c.u32(context + interpreter::state);
    const auto old_ip = c.u32(context + interpreter::ip);
    c.put32(context + interpreter::ip, start);
    const auto table = c.u32(g + graphic_state::opcode_table);
    const auto hold = [&] {
        c.set({{19, context}, {20, end}, {21, old_ip}, {22, table}, {31, g}});
        for (unsigned n = 0; n < 8; ++n) c.set(23 + n, round[n]);
    };
    c.set(3, g);
    hold();
    if (c.u32(context + interpreter::trace) == 0) return;
    while (c.u32(context + interpreter::ip) < end) {
        const auto current = c.u32(g + graphic_state::round_function);
        std::uint32_t index = 0xffff;
        for (unsigned n = 0; n < 8; ++n)
            if (current == round[7 - n]) { index = classes[7 - n]; break; }
        c.put16(context + interpreter::round_index, index);
        c.set({{3, context}, {12, c.u32(context + interpreter::trace)}});
        c.call(indirect_glue, 0x10756218);
        c.set(2, c.u32(c.sp() + 0x14));
        if (c.u32(context + interpreter::trace) == 0) break;
        const auto at = c.u32(context + interpreter::ip);
        c.put32(context + interpreter::ip, at + 1);
        const auto opcode = c.u8(at);
        c.put32(context + interpreter::opcode, opcode);
        c.set({{3, context}, {4, at}, {12, c.u32(table + 4 * opcode)}});
        c.call(indirect_glue, 0x1075624c);
        c.set(2, c.u32(c.sp() + 0x14));
    }
    c.put32(context + interpreter::ip, old_ip);
}

// 0x10756310: run a program (the hint font's prep, fpgm or a glyph's instructions) over the zones: the execution context is built on this
// frame (both axes' vectors, the reference points at 0, the zone pointers at the glyph zone), the size-dependent checks made, the loop
// called through the context's `run` word under a setjmp, and the result returned: 0, or the code a longjmp brings (0x1101 an illegal
// opcode, 0x1102 a bad opcode table), or 0x1102 where the first table entry is not the installer's.
std::uint32_t interpret(Cpu& c, std::uint32_t zones, std::uint32_t start, std::uint32_t end, std::uint32_t g, std::uint32_t traced) {
    Frame frame(c, 0x10756310, 0x1d0, {.first_gpr = 24});
    const auto local = frame.at(0x38);
    const auto glyph_zone = zones + 0x2c;
    Vol v(c);
    v.r[6] = c.toc(-0x3c54);
    v.r[8] = 0;
    v.r[9] = c.toc(-0x3c4c);
    v.r[10] = 0x4000;
    c.set({{24, c.toc(-0x3c50)}, {25, 0}, {26, glyph_zone}, {27, start}, {28, end}, {29, g}, {30, traced}, {31, local}});
    // The record: both axes (2.14), the reference points, the zone pointers at the glyph zone, the axis functions.
    for (const auto x : {interpreter::projection_x, interpreter::freedom_x, interpreter::dual_x}) c.put16(local + x, 0x4000);
    for (const auto y : {interpreter::projection_y, interpreter::freedom_y, interpreter::dual_y}) c.put16(local + y, 0);
    for (const auto zp : {interpreter::zp0, interpreter::zp1, interpreter::zp2}) c.put32(local + zp, glyph_zone);
    for (const auto rp : {interpreter::rp0, interpreter::rp1, interpreter::rp2}) c.put32(local + rp, 0);
    c.put32(local + interpreter::zones, zones);
    c.put32(local + interpreter::state, g);
    c.put16(local + interpreter::vector_dot, 0x4000);
    c.put16(local + interpreter::loop, 0);
    c.put32(local + interpreter::move, c.toc(-0x3c4c));
    c.put32(local + interpreter::project, c.toc(-0x3c50));
    c.put32(local + interpreter::dual_project, c.toc(-0x3c50));
    c.put32(local + interpreter::scale_cvt, c.toc(-0x3c54));
    c.put32(local + interpreter::single_width, c.toc(-0x3c58));
    const auto leave = [&](std::uint32_t result) { v.r[3] = result; v.store(c); return result; };
    // cr0 as the original's last compare leaves it: the jump buffer keeps the condition register and setjmp returns it in r6.
    c.compare_logical(0, c.u8(g + graphic_state::current_program), 1);
    if (c.u8(g + graphic_state::current_program) != 1) {
        c.compare_logical(0, c.u16(g + graphic_state::ppem), 1);
        if (c.u16(g + graphic_state::ppem) <= 1) return leave(0);
        c.compare(0, c.i8(g + graphic_state::mode), 0);
        if (c.i8(g + graphic_state::mode) == 0) {
            // Mode 0 (both ratios): the scaled cvt and single width functions, and the sizes' check that the scale is worth a run.
            c.put32(local + interpreter::scale_cvt, c.toc(-0x3c5c));
            c.put32(local + interpreter::single_width, c.toc(-0x3c60));
            v.r[3] = c.toc(-0x3c5c);
            const auto down = s32(v.fix_mul(c.u32(g + graphic_state::ppem_fixed), c.u32(g + graphic_state::ratio_down)));
            c.compare(0, down, 0x10000);
            v.r[4] = 0x10000;
            if (down <= 0x10000) return leave(0);
            const auto up = s32(v.fix_mul(c.u32(g + graphic_state::ppem_fixed), c.u32(g + graphic_state::ratio_up)));
            c.compare(0, up, 0x10000);
            v.r[4] = 0x10000;
            if (up <= 0x10000) return leave(0);
        }
        c.compare(0, c.i8(g + graphic_state::size_flag_c8), 0);
        if (c.i8(g + graphic_state::size_flag_c8) != 0) {
            // A definition-checked pass: the table the installer built must still start with the first handler's descriptor.
            const auto first = c.u32(c.u32(g + graphic_state::opcode_table));
            v.r[3] = first & 0xffffff;
            c.compare(0, s32(v.r[3]), s32(c.toc(-0x3c64) & 0xffffff));
            if (v.r[3] != (c.toc(-0x3c64) & 0xffffff)) return leave(0x1102);
        } else {
            v.r[4] = lha(c, g + graphic_state::ppem_scale);
            c.compare(0, s32(extsh(v.r[4])), 0);
            if (extsh(v.r[4]) != 0) {
                v.r[3] = g;
                v.r[4] = extsh(v.r[4]);
                call_glue(c, v, c.u32(g + graphic_state::scale_callback), 0x10756478);
                c.put32(g + graphic_state::single_width_value, v.r[3]);
            }
        }
    }
    c.put32(local + interpreter::stack_top, c.u32(g + graphic_state::stack));
    v.r[3] = local + interpreter::jump_buffer;
    v.store(c);
    c.call(dispatched::setjmp_entry, 0x10756490);
    std::uint32_t result = c.r(3);
    if (result == 0) {
        try {
            const auto table = c.u32(g + graphic_state::opcode_table);
            c.put32(g + graphic_state::trig_vectors, table + 0x400);
            c.put32(g + graphic_state::trig_angles, c.u32(g + graphic_state::trig_vectors) + 0xa0);
            c.set(3, c.u32(c.u32(g + graphic_state::trig_vectors) + 0xc));
            if (c.r(3) - 0x2d410000u != 0x3ccd) {
                c.set({{3, local + interpreter::jump_buffer}, {4, interpreter::error_bad_table}});
                c.call(tsimem::longjmp_entry, 0x107564d4);
            }
            c.put32(local + interpreter::trace, traced);
            c.put32(local + interpreter::run, traced != 0 ? c.toc(-0x3c68) : c.toc(-0x3c6c));
            c.set({{3, local}, {4, start}, {5, end}, {12, c.u32(local + interpreter::run)}});
            c.call(indirect_glue, 0x10756504);
            c.set(2, c.u32(c.sp() + 0x14));
            c.set(3, 0);
            result = 0;
        } catch (const GuestContextTransfer& transfer) {
            // The longjmp of an illegal opcode or a bad table lands at the setjmp's return with the code.
            if (transfer.state != &c.s || c.s.pc != 0x10756490 || c.sp() != frame.base()) throw;
            result = c.r(3);
        }
    }
    return result;
}

// ---- Vectors --------------------------------------------------------------------------------------------------------------------------
namespace {
// The product of the projection and freedom vectors as the move functions divide by it: each component pair multiplied, rounded to 2.14,
// summed as a halfword and kept away from zero (|dot| < 0x400 becomes +-0x4000). Stored in the context's vector_dot; the pieces are what
// the original leaves in registers.
struct Dot { std::uint32_t px, product_y, shifted_x; };
Dot set_dot(Cpu& c, std::uint32_t context) {
    const auto px = lha(c, context + interpreter::projection_x), fx = lha(c, context + interpreter::freedom_x);
    const auto py = lha(c, context + interpreter::projection_y), fy = lha(c, context + interpreter::freedom_y);
    const auto product_x = px * fx, product_y = py * fy;
    const auto shifted_x = sra(product_x + 0x2000, 14), shifted_y = sra(product_y + 0x2000, 14);
    auto dot = extsh(shifted_x + shifted_y);
    if (s32(dot) > -0x400 && s32(dot) < 0x400) dot = s32(dot) >= 0 ? 0x4000 : extsh(-0x4000);
    c.put16(context + interpreter::vector_dot, dot);
    return {px, product_y, shifted_x};
}
// The dot product for an axis: the clamp alone (the value is one component of the other vector).
void set_dot_to(Cpu& c, std::uint32_t context, std::uint32_t value) {
    auto dot = value;
    if (s32(dot) > -0x400 && s32(dot) < 0x400) dot = s32(dot) >= 0 ? 0x4000 : extsh(-0x4000);
    c.put16(context + interpreter::vector_dot, dot);
}
}

// 0x10756520, 0x10756560 (SVTCA): both vectors along y or x, the axis functions of that axis (the toc words -0x3c70, -0x3c74 / -0x3c4c, -0x3c50).
void svtca_y(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    c.put16(context + projection_x, 0); c.put16(context + freedom_x, 0);
    c.put16(context + projection_y, 0x4000); c.put16(context + freedom_y, 0x4000);
    c.put32(context + move, c.toc(-0x3c70));
    c.put32(context + project, c.toc(-0x3c74));
    c.put32(context + dual_project, c.toc(-0x3c74));
    c.put16(context + vector_dot, 0x4000);
    c.set({{4, 0x4000}, {5, c.toc(-0x3c74)}});
}
void svtca_x(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    c.put16(context + projection_x, 0x4000); c.put16(context + freedom_x, 0x4000);
    c.put16(context + projection_y, 0); c.put16(context + freedom_y, 0);
    c.put32(context + move, c.toc(-0x3c4c));
    c.put32(context + project, c.toc(-0x3c50));
    c.put32(context + dual_project, c.toc(-0x3c50));
    c.put16(context + vector_dot, 0x4000);
    c.set({{4, 0}, {5, 0x4000}, {6, c.toc(-0x3c50)}});
}
// 0x107565a0 (SPVTCA, the opcode's low bit picks x): the projection vector along an axis (and the dual-project function word; the dual vector itself is not written), the dot product from the
// freedom vector's component; 0x10756630 (SFVTCA): the freedom vector along an axis, the dot product from the projection vector's.
void spvtca(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    if (c.u32(context + opcode) & 1) {
        c.put16(context + projection_x, 0x4000); c.put16(context + projection_y, 0);
        c.put32(context + project, c.toc(-0x3c50));
        set_dot_to(c, context, lha(c, context + freedom_x));
        c.set(4, 0);
    } else {
        c.put16(context + projection_x, 0); c.put16(context + projection_y, 0x4000);
        c.put32(context + project, c.toc(-0x3c74));
        set_dot_to(c, context, lha(c, context + freedom_y));
        c.set(4, 0x4000);
    }
    c.put32(context + move, c.toc(-0x3c78));
    c.put32(context + dual_project, c.u32(context + project));
}
void sfvtca(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    if (c.u32(context + opcode) & 1) {
        c.put16(context + freedom_x, 0x4000); c.put16(context + freedom_y, 0);
        set_dot_to(c, context, lha(c, context + projection_x));
        c.set(4, 0x4000);
    } else {
        c.put16(context + freedom_x, 0); c.put16(context + freedom_y, 0x4000);
        set_dot_to(c, context, lha(c, context + projection_y));
        c.set(4, 0);
    }
    c.put32(context + move, c.toc(-0x3c78));
}

namespace {
// The unit vector (2.14) along the line from point p2 of zone `to` to point p1 of zone `from`, by the coordinate arrays at `x_array` and
// `y_array` of the two zones: the differences shifted up 10, their distance, the quotients shifted down 2. `hold` receives the
// two values the callers keep in registers. Leaves the registers' util leftovers in v.
struct Line { std::uint32_t dx10, dy10, quotient_x, quotient_y; };
Line line_direction(Vol& v, std::uint32_t dx, std::uint32_t dy) {
    Line line;
    line.dx10 = rlwinm(dx, 10, 0, 21);
    line.dy10 = rlwinm(dy, 10, 0, 21);
    const auto length = v.dist(line.dx10, line.dy10);
    line.quotient_x = v.fix_div(line.dx10, length);
    line.quotient_y = v.fix_div(line.dy10, length);
    return line;
}
// The opcode's low bit turns the vector a quarter turn (x, y) -> (-y, x).
void quarter_turn(Cpu& c, std::uint32_t context, std::uint32_t x_at, std::uint32_t y_at) {
    const auto y = lha(c, context + y_at), x = lha(c, context + x_at);
    c.put16(context + y_at, x);
    c.put16(context + x_at, negate(y));
}
}

// 0x107566b0 (SPVTL), 0x10756a00 (SFVTL): the projection (freedom) vector along the line between two points: pops p2 then p1, the line from
// zp2's p2 to zp1's p1; the vector is the unit direction (turned a quarter for the odd opcode); the dot product recomputed. SPVTL also sets
// the move and project functions of a free direction.
void spvtl(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    Frame frame(c, 0x107566b0, 0x50, {.first_gpr = 28});
    Vol v(c);
    const auto p2 = pop(c, context), p1 = pop(c, context);
    const auto ce2 = c.u32(context + zp2), ce1 = c.u32(context + zp1);
    // The reads in the original's order (y2, x2, x1, y1): a point past the arrays faults at the same address.
    const auto y2 = c.u32(c.u32(ce2 + zone::ys) + (p2 << 2));
    const auto x2 = c.u32(c.u32(ce2 + zone::xs) + (p2 << 2));
    const auto x1 = c.u32(c.u32(ce1 + zone::xs) + (p1 << 2));
    const auto y1 = c.u32(c.u32(ce1 + zone::ys) + (p1 << 2));
    v.r[5] = y1; v.r[6] = y2; v.r[7] = p1 << 2; v.r[8] = ce1;
    const auto line = line_direction(v, x1 - x2, y1 - y2);
    c.put16(context + projection_x, sra(line.quotient_x, 2));
    c.put16(context + projection_y, sra(line.quotient_y, 2));
    if (c.u32(context + opcode) & 1) quarter_turn(c, context, projection_x, projection_y);
    const auto dot = set_dot(c, context);
    v.r[4] = dot.shifted_x; v.r[5] = dot.px; v.r[3] = c.toc(-0x3c78);
    c.put32(context + move, c.toc(-0x3c78));
    c.put32(context + project, c.toc(-0x3c7c));
    c.put32(context + dual_project, c.u32(context + project));
    v.store(c);
}
void sfvtl(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    Frame frame(c, 0x10756a00, 0x50, {.first_gpr = 28});
    Vol v(c);
    const auto p2 = pop(c, context), p1 = pop(c, context);
    const auto ce2 = c.u32(context + zp2), ce1 = c.u32(context + zp1);
    // The reads in the original's order (y2, x2, x1, y1): a point past the arrays faults at the same address.
    const auto y2 = c.u32(c.u32(ce2 + zone::ys) + (p2 << 2));
    const auto x2 = c.u32(c.u32(ce2 + zone::xs) + (p2 << 2));
    const auto x1 = c.u32(c.u32(ce1 + zone::xs) + (p1 << 2));
    const auto y1 = c.u32(c.u32(ce1 + zone::ys) + (p1 << 2));
    v.r[5] = y1; v.r[6] = y2; v.r[7] = p1 << 2; v.r[8] = ce1;
    const auto line = line_direction(v, x1 - x2, y1 - y2);
    c.put16(context + freedom_x, sra(line.quotient_x, 2));
    c.put16(context + freedom_y, sra(line.quotient_y, 2));
    if (c.u32(context + opcode) & 1) quarter_turn(c, context, freedom_x, freedom_y);
    const auto dot = set_dot(c, context);
    v.r[4] = dot.shifted_x; v.r[5] = dot.px; v.r[3] = dot.product_y;
    c.put32(context + move, c.toc(-0x3c78));
    v.store(c);
}
// 0x10756820 (SDPVTL): both the projection vector (from the current coordinates) and the dual projection vector (from the original scaled
// ones, zone words +8 and +0xc) along the line; the three function words of the general case.
void sdpvtl(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    Frame frame(c, 0x10756820, 0x50, {.first_gpr = 26});
    Vol v(c);
    const auto p2 = pop(c, context), p1 = pop(c, context);
    const auto ce2 = c.u32(context + zp2), ce1 = c.u32(context + zp1);
    const auto fetch = [&](std::uint32_t zone_at, std::uint32_t array, std::uint32_t point) { return c.u32(c.u32(zone_at + array) + (point << 2)); };
    v.r[6] = fetch(ce2, zone::ys, p2); v.r[5] = fetch(ce1, zone::ys, p1); v.r[7] = ce1;
    const auto first_x1 = fetch(ce1, zone::xs, p1), first_x2 = fetch(ce2, zone::xs, p2);
    const auto first = line_direction(v, first_x1 - first_x2, v.r[5] - v.r[6]);
    c.put16(context + projection_x, sra(first.quotient_x, 2));
    c.put16(context + projection_y, sra(first.quotient_y, 2));
    v.r[7] = ce1;
    v.r[6] = fetch(ce2, zone::original_ys, p2); v.r[5] = fetch(ce1, zone::original_ys, p1);
    const auto second_x1 = fetch(ce1, zone::original_xs, p1), second_x2 = fetch(ce2, zone::original_xs, p2);
    const auto second = line_direction(v, second_x1 - second_x2, v.r[5] - v.r[6]);
    c.put16(context + dual_x, sra(second.quotient_x, 2));
    c.put16(context + dual_y, sra(second.quotient_y, 2));
    if (c.u32(context + opcode) & 1) {
        quarter_turn(c, context, projection_x, projection_y);
        quarter_turn(c, context, dual_x, dual_y);
    }
    const auto dot = set_dot(c, context);
    v.r[4] = dot.shifted_x; v.r[5] = dot.px;
    c.put32(context + move, c.toc(-0x3c78));
    c.put32(context + project, c.toc(-0x3c7c));
    c.put32(context + dual_project, c.toc(-0x3c80));
    v.r[3] = c.toc(-0x3c7c);
    v.store(c);
}
// 0x10756b60 (SPVFS), 0x10756c00 (SFVFS): the vector from two popped components (y first), the dot product recomputed.
void spvfs(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    const auto y = pop(c, context);
    c.put16(context + projection_y, y);
    const auto x = pop(c, context);
    c.put16(context + projection_x, x);
    const auto dot = set_dot(c, context);
    c.put32(context + move, c.toc(-0x3c78));
    c.put32(context + project, c.toc(-0x3c7c));
    c.put32(context + dual_project, c.u32(context + project));
    c.set({{4, c.toc(-0x3c78)}, {5, dot.shifted_x}, {6, dot.px}});
}
void sfvfs(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    const auto y = pop(c, context);
    c.put16(context + freedom_y, y);
    const auto x = pop(c, context);
    c.put16(context + freedom_x, x);
    const auto dot = set_dot(c, context);
    c.put32(context + move, c.toc(-0x3c78));
    c.set({{4, dot.product_y}, {5, dot.shifted_x}, {6, dot.px}});
}
// 0x10756c90 (GPV), 0x10756cc0 (GFV): the projection (freedom) vector's components pushed; 0x10756cf0 (SFVTPV): the freedom vector made the projection vector.
void gpv(Cpu& c, std::uint32_t context) {
    const auto first = c.u32(context + interpreter::stack_top);
    push(c, context, lha(c, context + interpreter::projection_x));
    const auto second = c.u32(context + interpreter::stack_top);
    push(c, context, lha(c, context + interpreter::projection_y));
    c.set({{4, second}, {5, lha(c, context + interpreter::projection_y)}});
    (void)first;
}
void gfv(Cpu& c, std::uint32_t context) {
    push(c, context, lha(c, context + interpreter::freedom_x));
    const auto second = c.u32(context + interpreter::stack_top);
    push(c, context, lha(c, context + interpreter::freedom_y));
    c.set({{4, second}, {5, lha(c, context + interpreter::freedom_y)}});
}
void sfvtpv(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    c.put16(context + freedom_x, c.u16(context + projection_x));
    c.put16(context + freedom_y, c.u16(context + projection_y));
    c.put16(context + vector_dot, 0x4000);
    c.put32(context + move, c.toc(-0x3c78));
    c.set({{4, 0x4000}, {5, c.u16(context + projection_y)}});
}

// 0x10756d20 (ISECT): the point (the fifth value) moved to the intersection of the line through two points of zp0 (values 2 and 1) and the line
// through two points of zp1 (values 4 and 3), both axes touched. The slopes come from fix_divide; parallel lines take the midpoint of the
// four points; a numerator of zero puts the point on the second line's first point.
void isect(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    Frame frame(c, 0x10756d20, 0x70, {.first_gpr = 20});
    Vol v(c);
    const auto top = c.u32(context + stack_top);
    const auto ce0 = c.u32(context + zp0), ce1 = c.u32(context + zp1), ce2 = c.u32(context + zp2);
    const auto w1 = c.u32(top - 4), w2 = c.u32(top - 8), w3 = c.u32(top - 12), w4 = c.u32(top - 16), point = c.u32(top - 20);
    const auto at = [&](std::uint32_t array, std::uint32_t index) { return c.u32(array + (index << 2)); };
    const auto x0 = c.u32(ce0 + zone::xs), y0 = c.u32(ce0 + zone::ys), xb = c.u32(ce1 + zone::xs), yb = c.u32(ce1 + zone::ys);
    // The reads in the original's order.
    const std::uint32_t a0x = at(x0, w2), a1x = at(x0, w1), a0y = at(y0, w2), a1y = at(y0, w1);
    const std::uint32_t b0x = at(xb, w4), b1x = at(xb, w3), b0y = at(yb, w4), b1y = at(yb, w3);
    const std::uint32_t dax = a1x - a0x, day = a1y - a0y, dbx = b1x - b0x, dby = b1y - b0y;
    v.r[5] = w1 << 2; v.r[6] = y0; v.r[7] = xb; v.r[8] = w4 << 2; v.r[9] = w3 << 2; v.r[10] = yb;
    c.put32(context + stack_top, top - 20);
    const auto touched = c.u32(ce2 + zone::touched);
    c.put8(touched + point, c.u8(touched + point) | 3);
    v.r[4] = touched;
    v.r[3] = ce2;
    const auto xs = c.u32(ce2 + zone::xs), ys = c.u32(ce2 + zone::ys);
    const auto put = [&](std::uint32_t x, std::uint32_t y) { c.put32(xs + (point << 2), x); c.put32(ys + (point << 2), y); };
    std::uint32_t numerator = 0, denominator = 0;
    bool resolve = true;
    if (day == 0) {
        if (dbx == 0) { put(b0x, a0y); resolve = false; }
        else { numerator = b0y - a0y; denominator = v.r[4] = negate(dby); }
    } else if (dax == 0) {
        if (dby == 0) { put(a0x, b0y); resolve = false; }
        else { numerator = b0x - a0x; denominator = v.r[4] = negate(dbx); }
    } else {
        v.r[5] = sra(day, 31); v.r[3] = sra(dax, 31);
        const auto abs_day = (v.r[5] ^ day) - v.r[5], abs_dax = (v.r[3] ^ dax) - v.r[3];
        v.r[4] = abs_day;
        if (s32(abs_dax) > s32(abs_day)) {
            v.r[3] = day; v.r[4] = dax;
            const auto slope = v.fix_div(day, dax);
            v.r[3] = b0x - a0x; v.r[4] = slope;
            const auto run = v.fix_mul(v.r[3], slope);
            numerator = (b0y - a0y) - run;
            v.r[4] = slope; v.r[3] = dbx;
            const auto rise = v.fix_mul(dbx, slope);
            denominator = v.r[4] = rise - dby;
        } else {
            v.r[3] = dax; v.r[4] = day;
            const auto slope = v.fix_div(dax, day);
            v.r[3] = b0y - a0y; v.r[4] = slope;
            const auto rise = v.fix_mul(v.r[3], slope);
            numerator = rise - (b0x - a0x);
            v.r[4] = slope; v.r[3] = dby;
            const auto run = v.fix_mul(dby, slope);
            denominator = v.r[4] = dbx - run;
        }
    }
    if (resolve) {
        if (denominator == 0) {
            // Parallel: the midpoint of the four points.
            v.r[3] = sra(dax, 1); v.r[4] = point << 2;
            const auto x = sra(b0x + (sra(dbx, 1) + a0x + v.r[3]), 1);
            v.r[3] = sra(day, 1);
            const auto y = sra(b0y + (sra(dby, 1) + a0y + v.r[3]), 1);
            put(x, y);
        } else {
            v.r[6] = sra(denominator, 31); v.r[3] = sra(numerator, 31);
            v.r[5] = (v.r[6] ^ denominator) - v.r[6];
            const auto abs_numerator = (v.r[3] ^ numerator) - v.r[3];
            if (s32(abs_numerator) < s32(v.r[5])) {
                v.r[3] = numerator;
                const auto q = v.fix_div(numerator, denominator);
                v.r[3] = dbx; v.r[4] = q;
                const auto x = b0x + v.fix_mul(dbx, q);
                v.r[3] = dby; v.r[4] = q;
                const auto y = b0y + v.fix_mul(dby, q);
                put(x, y);
            } else if (numerator == 0) {
                put(b0x, b0y);
            } else {
                v.r[3] = denominator; v.r[4] = numerator;
                const auto q = v.fix_div(denominator, numerator);
                v.r[3] = dbx; v.r[4] = q;
                const auto x = b0x + v.fix_div(dbx, q);
                v.r[3] = dby; v.r[4] = q;
                const auto y = b0y + v.fix_div(dby, q);
                put(x, y);
            }
        }
    }
    v.store(c);
}

// ---- Graphic state words and reference points -----------------------------------------------------------------------------------------
// 0x10756fc0 (SMD), 0x10756fe0 (SCVTCI), 0x10757000 (SSWCI): a popped word stored in the state (minimum distance, control value cut-in, single width cut-in).
namespace {
void pop_into_state(Cpu& c, std::uint32_t context, std::uint32_t field) {
    const auto top = c.u32(context + interpreter::stack_top) - 4;
    c.put32(context + interpreter::stack_top, top);
    const auto g = c.u32(context + interpreter::state);
    c.put32(g + field, c.u32(top));
    c.set({{3, g}, {4, top}});
}
}
void smd(Cpu& c, std::uint32_t context) { pop_into_state(c, context, graphic_state::minimum_distance); }
void scvtci(Cpu& c, std::uint32_t context) { pop_into_state(c, context, graphic_state::control_value_cut_in); }
void sswci(Cpu& c, std::uint32_t context) { pop_into_state(c, context, graphic_state::single_width_cut_in); }
// 0x10757020 (SSW): the single width as the font's units; the scale callback turns it into pixels (stored as the single width value).
void ssw(Cpu& c, std::uint32_t context) {
    Frame frame(c, 0x10757020, 0x40, {.first_gpr = 31});
    const auto top = c.u32(context + interpreter::stack_top) - 4;
    const auto g = c.u32(context + interpreter::state);
    c.set(31, g);
    c.put32(context + interpreter::stack_top, top);
    c.set(3, g);
    c.put16(g + graphic_state::ppem_scale, c.u32(top));
    c.set({{4, lha(c, g + graphic_state::ppem_scale)}, {12, c.u32(g + graphic_state::scale_callback)}});
    c.call(indirect_glue, 0x10757058);
    c.set(2, c.u32(c.sp() + 0x14));
    c.put32(g + graphic_state::single_width_value, c.r(3));
}
// 0x10757080 (SRP0, SRP1, SRP2, SLOOP, POP): the popped value into rp0, rp1, rp2 or the loop count less one (the original jumps through a
// table of the opcodes 0x10..0x21; the others only pop).
void set_reference_point(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    const auto value = pop(c, context);
    const auto code = c.u32(context + opcode);
    c.set({{4, code}, {5, value}});
    const auto index = code - 0x10;
    if (index > 0x11) return;
    c.set(4, c.u32(c.toc(-0x3c84) + 4 * index));
    switch (index) {
    case 0: c.put32(context + rp0, value); break;
    case 1: c.put32(context + rp1, value); break;
    case 2: c.put32(context + rp2, value); break;
    case 7: c.put16(context + loop, value - 1); break;
    default: break;
    }
}
// 0x107570e0 (SZP0, SZP1, SZP2, SZPS): the zone pointer(s) set to the popped zone number (0x2c bytes a zone of the zone array).
void set_zone_pointer(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    const auto number = pop(c, context);
    const auto code = c.u32(context + opcode);
    const auto zone_at = c.u32(context + zones) + number * zone::size;
    c.set({{4, zone_at}, {5, c.u32(context + zones)}});
    switch (code) {
    case 0x13: c.put32(context + zp0, zone_at); break;
    case 0x14: c.put32(context + zp1, zone_at); break;
    case 0x15: c.put32(context + zp2, zone_at); break;
    case 0x16: c.put32(context + zp2, zone_at); c.put32(context + zp1, zone_at); c.put32(context + zp0, zone_at); break;
    default: break;
    }
}
// 0x10757290: the round state from the SROUND/S45ROUND operand: the period (flag set: the grid period 0x20, 0x40, 0x80 or 0x3e7 by bits 6-7,
// and its mask; clear: the 2.30 45-degree period 0x2d413ccd, halved, as is, doubled or 0x3e7, and the grid period from it), the phase by bits 4-5 and the
// threshold by bits 0-3.
void set_round_state(Cpu& c, std::uint32_t context, std::uint32_t value, std::uint32_t flag) {
    using namespace graphic_state;
    Vol v(c);
    const auto g = c.u32(context + interpreter::state);
    v.r[3] = g;
    v.r[6] = value & 0xc0;
    if (flag != 0) {
        c.put16(g + round_period, v.r[6] == 0 ? 0x20 : v.r[6] == 0x40 ? 0x40 : v.r[6] == 0x80 ? 0x80 : 0x3e7);
        v.r[5] = lha(c, g + round_period);
        c.put32(g + round_mask, ~(v.r[5] - 1));
    } else {
        v.r[5] = 0x2d410000;
        auto period = std::uint32_t(0x2d413ccd);
        c.put32(g + round_period_45, period);
        if (v.r[6] == 0) period = sra(period, 1);
        else if (v.r[6] == 0x80) period <<= 1;
        else if (v.r[6] != 0x40) period = 0x3e7;
        c.put32(g + round_period_45, period);
        v.r[5] = period;
        c.put16(g + round_period, sra(v.r[5] + 0x800000, 24));
    }
    const auto phase = value & 0x30;
    if (phase == 0x20) { v.r[5] = lha(c, g + round_period); c.put16(g + round_phase, sra(v.r[5] + 1, 1)); }
    else if (phase == 0x10) { v.r[5] = lha(c, g + round_period); c.put16(g + round_phase, sra(v.r[5] + 2, 2)); }
    else if (phase == 0x30) { v.r[6] = lha(c, g + round_period); v.r[5] = v.r[6] + v.r[6]; c.put16(g + round_phase, sra(v.r[6] + (v.r[5] + 2), 2)); }
    else if (phase == 0) c.put16(g + round_phase, 0);
    v.r[4] = value & 0xf;
    if (v.r[4] == 0) {
        v.r[4] = lha(c, g + round_period);
        c.put16(g + round_threshold, v.r[4] - 1);
    } else {
        v.r[4] = (v.r[4] - 4) * lha(c, g + round_period);
        c.put16(g + round_threshold, sra(v.r[4] + 4, 3));
    }
    v.store(c);
}
// 0x10757150 (SROUND), 0x107571a0 (S45ROUND): the round state from the popped operand, the super rounding function selected.
void sround(Cpu& c, std::uint32_t context) {
    Frame frame(c, 0x10757150, 0x40, {.first_gpr = 31});
    const auto value = pop(c, context);
    c.set({{4, value}, {5, 1}});
    const auto g = c.u32(context + interpreter::state);
    set_round_state(c, context, value, 1);
    c.put32(g + graphic_state::round_function, c.toc(-0x5b54));
}
void sround_45(Cpu& c, std::uint32_t context) {
    Frame frame(c, 0x107571a0, 0x40, {.first_gpr = 31});
    const auto value = pop(c, context);
    c.set({{4, value}, {5, 0}});
    const auto g = c.u32(context + interpreter::state);
    set_round_state(c, context, value, 0);
    c.put32(g + graphic_state::round_function, c.toc(-0x5b58));
}
// 0x107571f0 (RTG, RTHG, RTDG, RDTG, RUTG, ROFF): the round function of the graphic state chosen by the opcode.
void set_round_function(Cpu& c, std::uint32_t context) {
    const auto code = c.u32(context + interpreter::opcode);
    const auto g = c.u32(context + interpreter::state);
    c.set(3, g);
    std::int32_t slot = 0;
    switch (code) {
    case 0x18: slot = -0x5b3c; break;
    case 0x19: slot = -0x5b40; break;
    case 0x3d: slot = -0x5b44; break;
    case 0x7a: slot = -0x5b50; break;
    case 0x7d: slot = -0x5b48; break;
    case 0x7c: slot = -0x5b4c; break;
    default: return;
    }
    c.put32(g + graphic_state::round_function, c.toc(slot));
}

// ---- The stack ----------------------------------------------------------------------------------------------------------------------
// 0x10757420: the glyph's advance width pushed: the difference of the original x of the first phantom point after the last outline point and the one before.
void advance_width(Cpu& c, std::uint32_t context) {
    const auto zone_at = c.u32(context + interpreter::zones) + zone::size;
    const auto top = c.u32(context + interpreter::stack_top);
    const auto count = lha(c, zone_at + zone::contour_count);
    const auto last = lha(c, c.u32(zone_at + zone::contour_ends) + 2 * count - 2);
    const auto original_xs = c.u32(zone_at + zone::original_xs);
    const auto index = (last + 1) << 2;
    const auto left = c.u32(original_xs + index), right = c.u32(original_xs + index + 4);
    c.set({{4, top}, {5, right}, {6, left}, {7, original_xs}});
    c.put32(context + interpreter::stack_top, top + 4);
    c.put32(top, right - left);
}
// 0x10757470 (DUP).
void dup(Cpu& c, std::uint32_t context) {
    const auto top = c.u32(context + interpreter::stack_top);
    const auto value = c.u32(top - 4);
    c.set({{4, value}, {5, top}});
    c.put32(context + interpreter::stack_top, top + 4);
    c.put32(top, value);
}
// 0x10757490 (CLEAR): the stack emptied (to the state's stack base).
void clear_stack(Cpu& c, std::uint32_t context) {
    const auto g = c.u32(context + interpreter::state);
    c.set(4, g);
    c.put32(context + interpreter::stack_top, c.u32(g + graphic_state::stack));
}
// 0x107574a0 (SWAP): the two top values exchanged (r3 left holding the stack pointer, r4 the lower value).
void swap_top(Cpu& c, std::uint32_t context) {
    const auto top = c.u32(context + interpreter::stack_top);
    const auto upper = c.u32(top - 4), lower = c.u32(top - 8);
    c.put32(top - 8, upper);
    c.put32(top - 4, lower);
    c.set({{3, top}, {4, lower}});
}
// 0x107574c0 (DEPTH): the number of values on the stack pushed.
void stack_depth(Cpu& c, std::uint32_t context) {
    const auto g = c.u32(context + interpreter::state);
    const auto top = c.u32(context + interpreter::stack_top);
    const auto bytes = top - c.u32(g + graphic_state::stack);
    c.set({{4, bytes}, {5, top}});
    c.put32(context + interpreter::stack_top, top + 4);
    c.put32(top, std::uint32_t(s32(bytes) / 4));
}
// 0x107574f0 (CINDEX): the value k places down (k popped) copied over the popped one's place: the stack pointer stays.
void copy_index(Cpu& c, std::uint32_t context) {
    auto top = c.u32(context + interpreter::stack_top) - 4;
    const auto k = c.u32(top);
    const auto from = top - (k << 2);
    c.set({{3, from}, {4, top}});
    c.put32(top, c.u32(from));
}
// 0x10757510 (MINDEX): the value k places down moved to the top, the ones above it moved down one place.
void move_index(Cpu& c, std::uint32_t context) {
    auto top = c.u32(context + interpreter::stack_top) - 4;
    auto k = c.u32(top);
    auto at = top - (k << 2);
    const auto moved = c.u32(at);
    if (k != 0) {
        do {
            c.put32(at, c.u32(at + 4));
            --k;
            at += 4;
        } while (k != 0);
        top -= 4;
    }
    c.put32(top, moved);
    top += 4;
    c.put32(context + interpreter::stack_top, top);
    c.set({{4, k}, {5, moved}, {6, at}, {7, top}});
}
// 0x10757560 (ROLL): the third value from the top moved to the top.
void roll(Cpu& c, std::uint32_t context) {
    const auto top = c.u32(context + interpreter::stack_top);
    const auto first = c.u32(top - 4), second = c.u32(top - 8), third = c.u32(top - 12);
    c.put32(top - 8, first);
    c.put32(top - 12, second);
    c.put32(top - 4, third);
    c.set({{3, top}, {4, second}});
}

// ---- Points ---------------------------------------------------------------------------------------------------------------------------
// 0x10757580 (MDAP, the low bit rounds): the popped point becomes rp0 and rp1; with the low bit set the distance to move it is the
// round function's change of its projection (project: (context, x, y); round: (context, value, 0)); the move function applies it.
void mdap(Cpu& c, std::uint32_t context) {
    Frame frame(c, 0x10757580, 0x50, {.first_gpr = 28});
    const auto zone_at = c.u32(context + interpreter::zp0), g = c.u32(context + interpreter::state);
    const auto top = c.u32(context + interpreter::stack_top) - 4;
    c.put32(context + interpreter::stack_top, top);
    const auto point = c.u32(top);
    c.put32(context + interpreter::rp1, point);
    c.put32(context + interpreter::rp0, point);
    c.set({{28, context}, {29, point}, {30, zone_at}, {31, g}, {4, top}});
    if (c.u32(context + interpreter::opcode) & 1) {
        c.set({{4, c.u32(c.u32(zone_at + zone::xs) + (point << 2))}, {5, c.u32(c.u32(zone_at + zone::ys) + (point << 2))}, {12, c.u32(context + interpreter::project)}});
        c.call(indirect_glue, 0x107575e8);
        c.set(2, c.u32(c.sp() + 0x14));
        const auto projected = c.r(3);
        c.set({{12, c.u32(g + graphic_state::round_function)}, {31, projected}, {3, context}, {5, 0}, {4, projected}});
        c.call(indirect_glue, 0x10757604);
        c.set(2, c.u32(c.sp() + 0x14));
        c.set(6, c.r(3) - projected);
    } else {
        c.set(6, 0);
    }
    c.set({{12, c.u32(context + interpreter::move)}, {3, context}, {4, zone_at}, {5, point}});
    c.call(indirect_glue, 0x10757628);
    c.set(2, c.u32(c.sp() + 0x14));
}

// 0x10757650 (MIAP, the low bit rounds with the cut-in): the cvt entry (scale_cvt) is the distance the popped point should have along the
// projection vector; in the twilight zone the point is first put there (original and current coordinates), then moved by the difference (the
// rounded value when the low bit is set and the entry is within the cut-in of the point's own distance); rp0 and rp1 are the point.
void miap(Cpu& c, std::uint32_t context) {
    Frame frame(c, 0x10757650, 0x50, {.first_gpr = 26});
    Vol v(c);
    const auto zone_at = c.u32(context + interpreter::zp0), g = c.u32(context + interpreter::state);
    c.set({{27, context}, {28, zone_at}, {31, g}});
    auto top = c.u32(context + interpreter::stack_top) - 4;
    c.put32(context + interpreter::stack_top, top);
    v.r[4] = c.u32(top);
    call_glue(c, v, c.u32(context + interpreter::scale_cvt), 0x10757684);
    auto distance = v.r[3];
    c.set(29, distance);
    top = c.u32(context + interpreter::stack_top) - 4;
    v.r[4] = top;
    c.put32(context + interpreter::stack_top, top);
    const auto point = c.u32(top);
    c.set(30, point);
    c.put32(context + interpreter::rp1, point);
    c.put32(context + interpreter::rp0, point);
    if (zone_at == c.u32(context + interpreter::zones)) {
        v.r[4] = lha(c, context + interpreter::projection_x);
        c.set(26, point << 2);
        auto scaled = v.scale_mul(distance, v.r[4]);
        v.r[4] = c.u32(zone_at + zone::original_xs); c.put32(v.r[4] + (point << 2), scaled);
        v.r[4] = c.u32(zone_at + zone::xs); c.put32(v.r[4] + (point << 2), scaled);
        v.r[3] = distance; v.r[4] = lha(c, context + interpreter::projection_y);
        scaled = v.scale_mul(distance, v.r[4]);
        v.r[4] = c.u32(zone_at + zone::original_ys); c.put32(v.r[4] + (point << 2), scaled);
        v.r[4] = c.u32(zone_at + zone::ys); c.put32(v.r[4] + (point << 2), scaled);
    }
    v.r[4] = c.u32(c.u32(zone_at + zone::xs) + (point << 2));
    v.r[5] = c.u32(c.u32(zone_at + zone::ys) + (point << 2));
    v.r[3] = context;
    call_glue(c, v, c.u32(context + interpreter::project), 0x10757708);
    const auto along = v.r[3];
    c.set(26, along);
    if (c.u32(context + interpreter::opcode) & 1) {
        v.r[3] = distance - along;
        const auto cut_in = c.u32(g + graphic_state::control_value_cut_in);
        v.r[4] = sra(v.r[3], 31);
        const auto difference = (v.r[4] ^ v.r[3]) - v.r[4];
        v.r[3] = difference;
        if (s32(difference) > s32(cut_in)) { distance = along; c.set(29, distance); }
        v.r[3] = context; v.r[4] = distance; v.r[5] = 0;
        call_glue(c, v, c.u32(g + graphic_state::round_function), 0x10757750);
        distance = v.r[3];
        c.set(29, distance);
    }
    distance -= along;
    c.set(29, distance);
    v.r[3] = context; v.r[4] = zone_at; v.r[5] = point; v.r[6] = distance;
    call_glue(c, v, c.u32(context + interpreter::move), 0x10757774);
}

// 0x10757790 (IUP, the low bit for x): every contour of zp2 re-interpolated on the axis: runs of untouched points between two touched ones
// are placed in proportion to their original (unscaled) coordinates, points outside the range of the two follow the nearer one's shift
// (a zero range shifts them all alike). Contours without a touched point are left.
void iup(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    Frame frame(c, 0x10757790, 0xa0, {.first_gpr = 13});
    Vol v(c);
    const auto zone_at = c.u32(context + zp2);
    c.put32(frame.at(0x3c), zone_at);
    v.r[4] = c.u32(context + opcode);
    v.r[3] = zone_at;
    const bool x_axis = (v.r[4] & 1) != 0;
    const auto touched = c.u32(zone_at + zone::touched);
    const auto bit = std::uint32_t(x_axis ? zone::touched_x : zone::touched_y);
    const auto current = c.u32(zone_at + (x_axis ? zone::xs : zone::ys)), original = c.u32(zone_at + (x_axis ? zone::original_xs : zone::original_ys)),
               unscaled = c.u32(zone_at + (x_axis ? zone::unscaled_xs : zone::unscaled_ys));
    const auto is_touched = [&](std::uint32_t point) { return (c.u8(touched + point) & bit) != 0; };
    const auto unscaled_at = [&](std::uint32_t point) { return lha(c, unscaled + 2 * point); };
    std::uint32_t contour = 0, offset = 0;
    c.put16(frame.at(0x44), 0);
    c.put32(frame.at(0x48), 0);
    while (true) {
        v.r[3] = zone_at;
        if (!(s32(extsh(contour)) < s32(lha(c, zone_at + zone::contour_count)))) break;
        v.r[4] = c.u32(zone_at + zone::contour_starts);
        v.r[3] = c.u32(zone_at + zone::contour_ends);
        const auto start = lha(c, v.r[4] + offset), end = lha(c, v.r[3] + offset);
        const auto next_of = [&](std::uint32_t point) { return point == end ? start : point + 1; };
        auto point = start;
        v.r[3] = touched + point;
        while (!is_touched(point) && !(s32(point) > s32(end))) { ++point; ++v.r[3]; }
        if (s32(point) > s32(end)) { offset += 2; contour = extsh(contour + 1); c.put32(frame.at(0x48), offset); c.put16(frame.at(0x44), contour); continue; }
        const auto first = point;
        c.put32(frame.at(0x40), first);
        while (true) {
            const auto a = point;
            v.r[4] = a;
            const auto next = next_of(a);
            v.r[3] = next;
            point = next;
            if (next == first) break;
            if (is_touched(next)) continue;
            // An untouched run begins at `next`: find the touched point `b` that ends it.
            auto candidate = next;
            std::uint32_t b;
            while (true) {
                b = next_of(candidate);
                v.r[3] = b;
                if (is_touched(b)) break;
                candidate = b;
            }
            v.r[3] = unscaled_at(a);
            std::uint32_t lower, upper;
            if (s32(unscaled_at(a)) < s32(unscaled_at(b))) { lower = a; upper = b; v.r[5] = a; v.r[4] = b; }
            else { lower = b; upper = a; v.r[5] = b; }
            const auto unscaled_lower = unscaled_at(lower), unscaled_upper = unscaled_at(upper);
            v.r[3] = lower << 2;
            const auto current_lower = c.u32(current + (lower << 2)), original_lower = c.u32(original + (lower << 2));
            const auto range = unscaled_upper - unscaled_lower, shift_lower = current_lower - original_lower;
            const auto store = [&](std::uint32_t p, std::uint32_t value) { c.put32(current + (p << 2), value); };
            if (range == 0) {
                while (point != b) {
                    v.r[4] = point << 2;
                    v.r[3] = c.u32(current + v.r[4]) + shift_lower;
                    store(point, v.r[3]);
                    point = next_of(point);
                }
            } else {
                v.r[3] = 0x10000; v.r[4] = upper << 2;
                const auto original_upper = c.u32(original + (upper << 2)), current_upper = c.u32(current + (upper << 2));
                const auto shift_upper = current_upper - original_upper;
                const auto current_range = current_upper - current_lower;
                if (s32(range) < 0x8000 && s32(current_range) < 0x8000) {
                    v.r[4] = sra(range, 1);
                    while (point != b) {
                        v.r[3] = point << 2;
                        v.r[5] = c.u32(original + v.r[3]);
                        std::uint32_t moved;
                        if (s32(v.r[5]) > s32(original_lower) && s32(v.r[5]) < s32(original_upper))
                            moved = std::uint32_t(s32(current_range * (unscaled_at(point) - unscaled_lower) + v.r[4]) / s32(range)) + current_lower;
                        else moved = v.r[5] + (s32(v.r[5]) < s32(original_upper) ? shift_lower : shift_upper);
                        store(point, moved);
                        point = next_of(point);
                    }
                } else {
                    bool divide = true;
                    std::uint32_t ratio = 0;
                    c.put32(frame.at(0x38), 1);
                    while (point != b) {
                        const auto original_point = c.u32(original + (point << 2));
                        std::uint32_t moved;
                        if (s32(original_point) <= s32(original_lower)) moved = original_point + shift_lower;
                        else if (s32(original_point) >= s32(original_upper)) moved = original_point + shift_upper;
                        else {
                            if (divide) { ratio = v.fix_div(current_range, range); divide = false; c.put32(frame.at(0x38), 0); }
                            v.r[4] = ratio;
                            moved = v.fix_mul(unscaled_at(point) - unscaled_lower, ratio) + current_lower;
                        }
                        store(point, moved);
                        point = next_of(point);
                    }
                }
            }
            if (point == first) break;
        }
        offset += 2;
        contour = extsh(contour + 1);
        c.put32(frame.at(0x48), offset);
        c.put16(frame.at(0x44), contour);
    }
    v.r[3] = lha(c, zone_at + zone::contour_count);
    v.store(c);
}

// 0x10757ab0: the displacement SHP, SHC and SHZ apply: the reference point (rp1 in zp0 for the odd opcodes, else rp2 in zp1) moved by how
// far it is from its original position along the projection vector (the project function through the glue of (current - original)), then
// carried along the freedom vector (scale_multiply, or fix_divide by the dot product): *dx, *dy (only where the freedom vector has that
// component), the reference point to *reference, the zone returned. The three callers read the slots back from their frames.
std::uint32_t reference_shift(Cpu& c, std::uint32_t context, std::uint32_t dx_at, std::uint32_t dy_at, std::uint32_t reference_at) {
    using namespace interpreter;
    Frame frame(c, 0x10757ab0, 0x60, {.first_gpr = 25});
    const bool odd = (c.u32(context + opcode) & 1) != 0;
    const auto point = c.u32(context + (odd ? rp1 : rp2));
    const auto zone_at = c.u32(context + (odd ? zp0 : zp1));
    c.set({{26, context}, {27, dx_at}, {28, dy_at}, {29, reference_at}, {30, zone_at}, {31, point}});
    Vol v(c);
    const auto index = point << 2;
    // Read in the original's order (0x10757b08..0x10757b14), so a point past the arrays faults at the same address.
    const auto ox = c.u32(c.u32(zone_at + zone::original_xs) + index);
    const auto x = c.u32(c.u32(zone_at + zone::xs) + index);
    const auto oy = c.u32(c.u32(zone_at + zone::original_ys) + index);
    const auto y = c.u32(c.u32(zone_at + zone::ys) + index);
    v.r[3] = context; v.r[4] = x - ox; v.r[5] = y - oy; v.r[6] = x; v.r[7] = ox;
    call_glue(c, v, c.u32(context + project), 0x10757b28);
    const auto along = v.r[3];
    c.set(25, along);
    const auto dot = lha(c, context + vector_dot);
    v.r[4] = dot;
    const auto free_x = [&] { return lha(c, context + freedom_x); };
    const auto free_y = [&] { return lha(c, context + freedom_y); };
    if (dot != 0x4000) {
        v.r[3] = free_x();
        if (extsh(v.r[3]) != 0) { const auto q = v.fix_div(v.r[3], v.r[4]); v.r[4] = q; c.put32(dx_at, v.fix_mul(along, q)); }
        v.r[3] = free_y();
        if (extsh(v.r[3]) != 0) { v.r[4] = lha(c, context + vector_dot); const auto q = v.fix_div(v.r[3], v.r[4]); v.r[4] = q; c.put32(dy_at, v.fix_mul(along, q)); }
    } else {
        v.r[4] = free_x();
        if (extsh(v.r[4]) != 0) c.put32(dx_at, v.scale_mul(along, v.r[4]));
        v.r[4] = free_y();
        if (extsh(v.r[4]) != 0) { v.r[3] = along; c.put32(dy_at, v.scale_mul(along, v.r[4])); }
    }
    c.put32(reference_at, point);
    v.r[3] = zone_at;
    v.store(c);
    return zone_at;
}

// 0x10757bd0 (SHP): the displacement added to each popped point of zp2 on the axes the freedom vector has, the loop count times (loop+1 points).
void shp(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    Frame frame(c, 0x10757bd0, 0x50, {.first_gpr = 31});
    c.set(31, context);
    c.link(0x10757bf4);
    reference_shift(c, context, frame.at(0x40), frame.at(0x3c), frame.at(0x38));
    Vol v(c);
    v.r[9] = c.u32(frame.at(0x3c));
    v.r[8] = c.u32(frame.at(0x40));
    v.r[7] = c.u32(context + zp2);
    v.r[6] = lha(c, context + loop);
    while (s32(extsh(v.r[6])) >= 0) {
        const auto top = c.u32(context + stack_top) - 4;
        c.put32(context + stack_top, top);
        v.r[3] = top;
        v.r[5] = c.u32(top);
        if (lha(c, context + freedom_x) != 0) {
            v.r[4] = c.u32(v.r[7] + zone::xs);
            v.r[3] = v.r[5] << 2;
            c.put32(v.r[4] + v.r[3], c.u32(v.r[4] + v.r[3]) + v.r[8]);
            v.r[3] = c.u32(v.r[7] + zone::touched);
            c.put8(v.r[3] + v.r[5], c.u8(v.r[3] + v.r[5]) | zone::touched_x);
        }
        if (lha(c, context + freedom_y) != 0) {
            v.r[4] = c.u32(v.r[7] + zone::ys);
            v.r[3] = v.r[5] << 2;
            c.put32(v.r[4] + v.r[3], c.u32(v.r[4] + v.r[3]) + v.r[9]);
            v.r[3] = c.u32(v.r[7] + zone::touched);
            c.put8(v.r[3] + v.r[5], c.u8(v.r[3] + v.r[5]) | zone::touched_y);
        }
        v.r[6] -= 1;
    }
    c.put16(context + loop, 0);
    v.store(c);
}

// 0x10757ca0 (SHC): the displacement added to every point of the popped contour of zp2 except the reference point itself (when it is in zp2).
void shc(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    Frame frame(c, 0x10757ca0, 0x50, {.first_gpr = 31});
    c.set(31, context);
    c.link(0x10757cc4);
    reference_shift(c, context, frame.at(0x40), frame.at(0x3c), frame.at(0x38));
    Vol v(c);
    const auto reference_zone = v.r[3];
    v.r[7] = 0; v.r[8] = c.u32(frame.at(0x38)); v.r[6] = 0;
    if (lha(c, context + freedom_x) != 0) v.r[6] = c.u32(frame.at(0x40));
    if (lha(c, context + freedom_y) != 0) v.r[7] = c.u32(frame.at(0x3c));
    const auto top = c.u32(context + stack_top) - 4;
    c.put32(context + stack_top, top);
    v.r[9] = c.u32(context + zp2);
    v.r[4] = c.u32(top);
    if (s32(v.r[4]) < s32(lha(c, v.r[9] + zone::contour_count))) {
        v.r[5] = c.u32(v.r[9] + zone::contour_starts);
        const auto offset = v.r[4] << 1;
        v.r[4] = c.u32(v.r[9] + zone::contour_ends);
        v.r[10] = lha(c, v.r[5] + offset);
        const auto last = lha(c, v.r[4] + offset);
        v.r[5] = v.r[10] << 2;
        auto remaining = extsh(last - v.r[10]);
        while (s32(extsh(remaining)) >= 0) {
            if (!(v.r[10] == v.r[8] && reference_zone == v.r[9])) {
                if (v.r[6] != 0) {
                    v.r[4] = c.u32(v.r[9] + zone::xs);
                    c.put32(v.r[4] + v.r[5], c.u32(v.r[4] + v.r[5]) + v.r[6]);
                    v.r[4] = c.u32(v.r[9] + zone::touched);
                    c.put8(v.r[4] + v.r[10], c.u8(v.r[4] + v.r[10]) | zone::touched_x);
                }
                if (v.r[7] != 0) {
                    v.r[4] = c.u32(v.r[9] + zone::ys);
                    c.put32(v.r[4] + v.r[5], c.u32(v.r[4] + v.r[5]) + v.r[7]);
                    v.r[4] = c.u32(v.r[9] + zone::touched);
                    c.put8(v.r[4] + v.r[10], c.u8(v.r[4] + v.r[10]) | zone::touched_y);
                }
            }
            v.r[5] += 4; v.r[10] += 1; remaining -= 1;
        }
    }
    v.store(c);
}

// 0x10757dc0 (SHZ): the displacement added to every point of the popped zone except the reference point (when it is in that zone), on the
// axes the freedom vector has; the y points are marked touched.
void shz(Cpu& c, std::uint32_t context) {
    using namespace interpreter;
    Frame frame(c, 0x10757dc0, 0x50, {.first_gpr = 31});
    c.set(31, context);
    c.link(0x10757de4);
    reference_shift(c, context, frame.at(0x3c), frame.at(0x38), frame.at(0x40));
    Vol v(c);
    const auto reference_zone = v.r[3];
    v.r[4] = c.u32(frame.at(0x3c));
    v.r[5] = c.u32(frame.at(0x38));
    const auto top = c.u32(context + stack_top) - 4;
    c.put32(context + stack_top, top);
    const auto offset = c.u32(top) * zone::size;
    v.r[6] = c.u32(context + zones);
    const auto target = v.r[6] + offset;
    v.r[7] = target;
    v.r[6] = lha(c, target + zone::contour_count);
    const bool same = reference_zone == target;
    v.r[3] = c.u32(target + zone::contour_starts);
    v.r[7] = c.u32(target + zone::contour_ends);
    v.r[6] = v.r[6] << 1;
    v.r[3] = lha(c, v.r[3]);
    v.r[6] = lha(c, v.r[7] + v.r[6] - 2);
    if (!same) c.put32(frame.at(0x40), 0xffffffff);
    const auto first = v.r[3], last = v.r[6];
    v.r[10] = 0;
    v.r[7] = lha(c, context + freedom_x);
    if (v.r[7] != 0) {
        v.r[9] = c.u32(context + zones);
        v.r[8] = first << 2;
        v.r[7] = c.u32(frame.at(0x40));
        v.r[9] = c.u32(v.r[9] + offset);
        v.r[7] -= first; v.r[7] -= 1;
        v.r[9] += v.r[8];
        v.r[8] = extsh(v.r[7]);
        const auto sweep = [&] {
            while (true) {
                v.r[7] = extsh(v.r[8]);
                if (!(s32(v.r[7]) >= 0)) break;
                v.r[7] = c.u32(v.r[9]); v.r[8] -= 1; v.r[7] += v.r[4]; c.put32(v.r[9], v.r[7]); v.r[9] += 4;
            }
        };
        sweep();
        v.r[7] = c.u32(frame.at(0x40));
        if (v.r[7] == 0xffffffff) { v.r[7] = last - first; v.r[8] = extsh(v.r[7]); }
        else { v.r[7] = last - v.r[7]; v.r[9] += 4; v.r[7] -= 1; v.r[8] = extsh(v.r[7]); }
        sweep();
        v.r[10] = 1;
    }
    v.r[4] = lha(c, context + freedom_y);
    if (v.r[4] != 0) {
        v.r[7] = c.u32(context + zones);
        v.r[8] = first << 2;
        v.r[4] = c.u32(frame.at(0x40));
        v.r[7] += offset;
        v.r[4] -= first;
        v.r[9] = c.u32(v.r[7] + zone::ys);
        v.r[7] = c.u32(v.r[7] + zone::touched);
        const auto r0 = v.r[4] - 1;
        v.r[8] = v.r[9] + v.r[8];
        v.r[7] += first;
        v.r[4] = extsh(r0);
        const auto sweep = [&](std::uint32_t flag) {
            while (s32(extsh(v.r[4])) >= 0) {
                c.put32(v.r[8], c.u32(v.r[8]) + v.r[5]);
                v.r[4] -= 1; v.r[8] += 4;
                c.put8(v.r[7], c.u8(v.r[7]) | flag);
                v.r[7] += 1;
            }
        };
        sweep(v.r[10]);
        if (c.u32(frame.at(0x40)) == 0xffffffff) v.r[4] = extsh(last - first);
        else { v.r[3] = last - c.u32(frame.at(0x40)); v.r[8] += 4; v.r[7] += 1; v.r[4] = extsh(v.r[3] - 1); }
        v.r[3] = v.r[10] | 2;
        sweep(v.r[3]);
    }
    v.store(c);
}
}
