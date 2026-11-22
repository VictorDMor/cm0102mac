// The font engine, block `fnt_b` (FNT's second third, 0x10757fb0..0x10759250): the TrueType interpreter's graphic-state and point-movement
// opcodes: SHPIX, IP, MSIRP, ALIGNRP, ALIGNPTS, the angle adjustment 0x7f, the pushes (NPUSHB, NPUSHW, PUSHB, PUSHW), the storage and cvt
// opcodes (WS, RS, WCVTP, WCVTF, RCVT), GC, SCFS, MD, MPPEM, MPS, GETINFO, FLIPPT, FLIPRGON, FLIPRGOFF, SANGW, SCANCTRL, SCANTYPE, INSTCTRL, FLIPON,
// FLIPOFF and DEBUG. Each is called through the indirect glue by the interpreter's loop with r3 = the execution context (font_engine_detail.hpp,
// `interpreter`); the game's fonts run none of them (symbol.ttf's fpgm only defines functions), so the checks are cases.
//
// A handler leaves r3-r10 to whoever runs next (and, at the end, to the interpreter's callers), so each keeps a shadow `v` of r3-r10 (v[n] = rn)
// updated where the original's instructions write them, stored before every call and at the return. The calls through the context's and the
// state's function words are made through the glue as the original does (the interpreter replaces them: the project, move and scale functions
// are descriptors in the context), the helpers of UTIL directly.
#include "font_engine_detail.hpp"
#include <array>

namespace cm::aot::font_engine {
namespace {
using guest::Frame;
constexpr std::uint32_t glue = 0x10781320;
// FNT's first third, still generated: the generic move (context, zone, point, distance).
constexpr std::uint32_t move_entry = 0x10755cc0;
// `bl 0x10755cc0` (FNT's generic move: context, zone, point, distance), at the return address of the original's call.
void call_through_move(Cpu& c, std::uint32_t back) { c.call(move_entry, back); }

constexpr std::int32_t extsh(std::uint32_t v) { return std::int16_t(v); }
constexpr std::uint32_t reg(std::int32_t v) { return std::uint32_t(v); }
constexpr std::int32_t sra(std::uint32_t v, unsigned n) { return std::int32_t(v) >> n; }
using Volatile = std::array<std::uint32_t, 11>;
void store(Cpu& c, const Volatile& v) { for (unsigned n = 3; n <= 10; ++n) c.set(n, v[n]); }
void load(Cpu& c, Volatile& v) { for (unsigned n = 3; n <= 10; ++n) v[n] = c.r(n); }
void call_through(Cpu& c, std::uint32_t descriptor, std::uint32_t back) {
    c.set(12, descriptor);
    c.call(glue, back);
    c.set(2, c.u32(c.sp() + 0x14));
}
// The value on top of the operand stack, popped (the stack-top word is stored first, as the original does).
std::uint32_t pop(Cpu& c, std::uint32_t ctx) {
    const auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    return c.u32(top);
}
std::uint32_t at(std::uint32_t array, std::uint32_t index) { return array + (index << 2); }
}

// 0x10757fb0 SHPIX (0x38): the popped amount scaled by the freedom vector's x and y (0x10767e40) added to each popped point of zp2 (a loop of
// `loop` + 1 points), their touched flags set.
void opcode_shift_pixels(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10757fb0, 0x50, {.first_gpr = 28});
    Volatile v;
    load(c, v);
    std::uint32_t shift_x = 0, shift_y = 0;  // r31, r30
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    v[3] = top;
    v[4] = reg(extsh(c.u16(ctx + interpreter::freedom_x)));
    const auto amount = c.u32(top);
    const auto scaled = [&](std::uint32_t component) {
        v[3] = amount;
        v[4] = component;
        Leftovers left{v[4], v[5], v[6], v[7], v[8]};
        v[3] = reg(scale_multiply(wrap(amount), wrap(component), left));
        v[4] = left.r4; v[5] = left.r5; v[6] = left.r6;
        return v[3];
    };
    if (extsh(v[4]) != 0) shift_x = scaled(v[4]);
    v[4] = reg(extsh(c.u16(ctx + interpreter::freedom_y)));
    if (extsh(v[4]) != 0) shift_y = scaled(v[4]);
    const auto zone_at = c.u32(ctx + interpreter::zp2);
    v[7] = zone_at;
    v[6] = reg(extsh(c.u16(ctx + interpreter::loop)));
    while (extsh(v[6]) >= 0) {
        top = c.u32(ctx + interpreter::stack_top) - 4;
        c.put32(ctx + interpreter::stack_top, top);
        v[3] = top;
        const auto point = c.u32(top);
        v[5] = point;
        if (extsh(c.u16(ctx + interpreter::freedom_x)) != 0) {
            v[4] = c.u32(zone_at + zone::xs);
            v[3] = point << 2;
            c.put32(at(v[4], point), c.u32(at(v[4], point)) + shift_x);
            v[3] = c.u32(zone_at + zone::touched);
            c.put8(v[3] + point, c.u8(v[3] + point) | zone::touched_x);
        }
        if (extsh(c.u16(ctx + interpreter::freedom_y)) != 0) {
            v[4] = c.u32(zone_at + zone::ys);
            v[3] = point << 2;
            c.put32(at(v[4], point), c.u32(at(v[4], point)) + shift_y);
            v[3] = c.u32(zone_at + zone::touched);
            c.put8(v[3] + point, c.u8(v[3] + point) | zone::touched_y);
        }
        v[6] = v[6] - 1;
    }
    c.put16(ctx + interpreter::loop, 0);
    store(c, v);
}

// 0x107580d0 IP (0x39): each popped point of zp2 moved to keep its original position between rp1 and rp2: the ratio of the current to the
// original distance rp1 -> rp2 (a 16.16 quotient, 1.0 when the original distance is zero), applied to the point's original distance from rp1.
void opcode_interpolate(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x107580d0, 0x60, {.first_gpr = 23});
    Volatile v;
    load(c, v);
    const auto zp0 = c.u32(ctx + interpreter::zp0), zp1 = c.u32(ctx + interpreter::zp1), zp2 = c.u32(ctx + interpreter::zp2);
    const auto rp1_x4 = c.u32(ctx + interpreter::rp1) << 2, rp2_x4 = c.u32(ctx + interpreter::rp2) << 2;
    const auto measure = [&](std::uint32_t xs, std::uint32_t ys, std::uint32_t descriptor, std::uint32_t back) {
        v[7] = c.u32(c.u32(zp0 + xs) + rp1_x4);
        v[6] = c.u32(c.u32(zp1 + xs) + rp2_x4);
        v[5] = c.u32(c.u32(zp0 + ys) + rp1_x4);
        v[3] = ctx;
        v[4] = v[6] - v[7];
        v[5] = c.u32(c.u32(zp1 + ys) + rp2_x4) - v[5];
        store(c, v);
        call_through(c, descriptor, back);
        load(c, v);
        return v[3];
    };
    v[5] = c.u32(zp0 + zone::ys);
    v[4] = c.u32(zp1 + zone::ys);
    const auto current = measure(zone::xs, zone::ys, c.u32(ctx + interpreter::project), 0x10758138);
    const auto original = measure(zone::original_xs, zone::original_ys, c.u32(ctx + interpreter::dual_project), 0x10758174);
    std::uint32_t ratio = 0x10000;
    v[4] = v[3];
    if (v[4] != 0) {
        Leftovers left{v[4], v[5], v[6], v[7], v[8]};
        v[3] = current;
        ratio = reg(fix_divide(wrap(current), wrap(original), left));
        v[3] = ratio; v[4] = left.r4; v[5] = left.r5; v[6] = left.r6;
    }
    for (std::int32_t n = extsh(c.u16(ctx + interpreter::loop)); n >= 0; n = extsh(reg(n - 1))) {
        const auto top = c.u32(ctx + interpreter::stack_top) - 4;
        c.put32(ctx + interpreter::stack_top, top);
        const auto point = c.u32(top), point_x4 = point << 2;
        v[3] = ctx;
        v[6] = c.u32(zp0 + zone::original_xs);
        v[5] = c.u32(zp2 + zone::original_xs);
        v[4] = c.u32(zp2 + zone::original_ys);
        v[7] = c.u32(rp1_x4 + v[6]);
        v[6] = c.u32(v[5] + point_x4);
        const auto rp1_y = c.u32(rp1_x4 + c.u32(zp0 + zone::original_ys));
        v[5] = rp1_y;
        const auto py = c.u32(v[4] + point_x4);
        v[4] = v[6] - v[7];
        v[5] = py - rp1_y;
        store(c, v);
        call_through(c, c.u32(ctx + interpreter::dual_project), 0x107581dc);
        load(c, v);
        Leftovers left{v[4], v[5], v[6], v[7], v[8]};
        const auto target = reg(fix_multiply(wrap(v[3]), wrap(ratio), left));
        v[3] = target; v[4] = left.r4; v[5] = left.r5; v[6] = left.r6; v[7] = left.r7; v[8] = left.r8;
        const auto rp1_x = c.u32(rp1_x4 + c.u32(zp0 + zone::xs)), rp1_ys = c.u32(rp1_x4 + c.u32(zp0 + zone::ys));
        v[6] = c.u32(zp0 + zone::xs);
        v[5] = c.u32(zp2 + zone::xs);
        v[3] = c.u32(zp0 + zone::ys);
        v[4] = c.u32(zp2 + zone::ys);
        const auto px = c.u32(v[5] + point_x4), py_now = c.u32(v[4] + point_x4);
        v[7] = rp1_x;
        v[6] = px;
        v[5] = rp1_ys;
        v[3] = ctx;
        v[4] = px - rp1_x;
        v[5] = py_now - rp1_ys;
        store(c, v);
        call_through(c, c.u32(ctx + interpreter::project), 0x10758220);
        load(c, v);
        const auto moved = target - v[3];
        v[6] = moved; v[3] = ctx; v[4] = zp2; v[5] = point;
        store(c, v);
        call_through(c, c.u32(ctx + interpreter::move), 0x1075823c);
        load(c, v);
    }
    c.put16(ctx + interpreter::loop, 0);
    store(c, v);
}

// 0x10758270 MSIRP[a] (0x3a, 0x3b): the popped point of zp1 moved to the popped distance from rp0 (a twilight zone point first gets its original
// position that far from rp0's along the projection vector); rp1 = rp0, rp2 = the point, rp0 = the point when the opcode's bit 0 is set.
void opcode_move_indirect(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10758270, 0x60, {.first_gpr = 24});
    Volatile v;
    load(c, v);
    const auto zp0 = c.u32(ctx + interpreter::zp0), zp1 = c.u32(ctx + interpreter::zp1), rp0 = c.u32(ctx + interpreter::rp0);
    const auto distance = pop(c, ctx);
    const auto point = pop(c, ctx);
    v[3] = c.u32(ctx + interpreter::stack_top);
    const auto rp0_x4 = rp0 << 2, point_x4 = point << 2;
    const auto scaled = [&](std::uint32_t component) {
        v[3] = distance;
        v[4] = component;
        Leftovers left{v[4], v[5], v[6], v[7], v[8]};
        v[3] = reg(scale_multiply(wrap(distance), wrap(component), left));
        v[4] = left.r4; v[5] = left.r5; v[6] = left.r6;
        return v[3];
    };
    if (zp1 == c.u32(ctx + interpreter::zones)) {
        auto product = scaled(reg(extsh(c.u16(ctx + interpreter::projection_x))));
        v[5] = c.u32(zp0 + zone::original_xs);
        v[4] = c.u32(zp1 + zone::original_xs);
        auto value = c.u32(v[5] + rp0_x4) + product;
        c.put32(v[4] + point_x4, value);
        product = scaled(reg(extsh(c.u16(ctx + interpreter::projection_y))));
        v[5] = c.u32(zp0 + zone::original_ys);
        v[4] = c.u32(zp1 + zone::original_ys);
        value = c.u32(v[5] + rp0_x4) + product;
        c.put32(v[4] + point_x4, value);
        v[4] = c.u32(zp1 + zone::original_xs);
        v[3] = c.u32(zp1 + zone::xs);
        c.put32(v[3] + point_x4, c.u32(v[4] + point_x4));
        v[4] = c.u32(zp1 + zone::original_ys);
        v[3] = c.u32(zp1 + zone::ys);
        c.put32(v[3] + point_x4, c.u32(v[4] + point_x4));
    }
    v[7] = c.u32(c.u32(zp0 + zone::xs) + rp0_x4);
    v[5] = c.u32(zp0 + zone::ys);
    v[8] = point_x4;
    v[6] = c.u32(c.u32(zp1 + zone::xs) + point_x4);
    v[3] = ctx;
    v[4] = c.u32(zp1 + zone::ys);
    v[5] = c.u32(v[5] + rp0_x4);
    const auto y_now = c.u32(v[4] + point_x4);
    v[5] = y_now - v[5];
    v[4] = v[6] - v[7];
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::project), 0x10758360);
    load(c, v);
    const auto delta = distance - v[3];
    v[3] = ctx; v[4] = zp1; v[5] = point; v[6] = delta;
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::move), 0x10758380);
    c.put32(ctx + interpreter::rp1, rp0);
    c.put32(ctx + interpreter::rp2, point);
    if (c.u32(ctx + interpreter::opcode) & 1) c.put32(ctx + interpreter::rp0, point);
}

// 0x107583b0 ALIGNRP (0x3c): each popped point of zp1 moved onto rp0 (by the negative of its distance along the projection vector).
void opcode_align_to_rp(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x107583b0, 0x50, {.first_gpr = 27});
    Volatile v;
    load(c, v);
    const auto zp0 = c.u32(ctx + interpreter::zp0), zp1 = c.u32(ctx + interpreter::zp1);
    const auto rp0_x4 = c.u32(ctx + interpreter::rp0) << 2;
    v[4] = c.u32(zp0 + zone::xs);
    v[3] = c.u32(zp0 + zone::ys);
    const auto rp_x = c.u32(v[4] + rp0_x4), rp_y = c.u32(v[3] + rp0_x4);
    while (extsh(c.u16(ctx + interpreter::loop)) >= 0) {
        const auto top = c.u32(ctx + interpreter::stack_top) - 4;
        c.put32(ctx + interpreter::stack_top, top);
        const auto point = c.u32(top), point_x4 = point << 2;
        v[3] = ctx;
        v[5] = c.u32(zp1 + zone::xs);
        v[4] = c.u32(zp1 + zone::ys);
        const auto px = c.u32(v[5] + point_x4), py = c.u32(v[4] + point_x4);
        v[5] = px;
        v[4] = px - rp_x;
        v[5] = py - rp_y;
        store(c, v);
        call_through(c, c.u32(ctx + interpreter::project), 0x10758420);
        load(c, v);
        v[6] = negate(v[3]); v[3] = ctx; v[4] = zp1; v[5] = point;
        store(c, v);
        call_through(c, c.u32(ctx + interpreter::move), 0x1075843c);
        load(c, v);
        v[3] = reg(extsh(c.u16(ctx + interpreter::loop)));
        c.put16(ctx + interpreter::loop, v[3] - 1);
    }
    c.put16(ctx + interpreter::loop, 0);
    store(c, v);
}

// 0x10758480 ALIGNPTS (0x27): two popped points moved to meet halfway along the projection vector (the point popped first is read in zp0 and moved in zp1, the other read in zp1 and moved in zp0).
void opcode_align_points(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10758480, 0x50, {.first_gpr = 27});
    Volatile v;
    load(c, v);
    const auto second = pop(c, ctx);
    const auto first = pop(c, ctx);
    v[5] = c.u32(ctx + interpreter::stack_top) + 0;
    v[8] = second << 2;
    const auto zp0 = c.u32(ctx + interpreter::zp0), zp1 = c.u32(ctx + interpreter::zp1);
    v[7] = c.u32(zp0 + zone::xs);
    v[6] = c.u32(zp0 + zone::xs);
    v[4] = c.u32(zp0 + zone::ys);
    const auto first_x4 = first << 2;
    v[7] = c.u32(c.u32(zp1 + zone::xs) + first_x4);
    v[5] = c.u32(c.u32(zp1 + zone::ys) + first_x4);
    const auto y2 = c.u32(v[4] + v[8]);
    v[6] = c.u32(v[6] + v[8]);
    v[5] = y2 - v[5];
    v[4] = v[6] - v[7];
    v[3] = ctx;
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::project), 0x107584f4);
    load(c, v);
    const auto distance = v[3], half = reg(sra(distance, 1));
    v[4] = c.u32(ctx + interpreter::zp0); v[5] = first; v[6] = half; v[3] = ctx;
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::move), 0x10758518);
    load(c, v);
    v[4] = c.u32(ctx + interpreter::zp1); v[3] = ctx; v[5] = second; v[6] = half - distance;
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::move), 0x10758534);
}

// 0x10758550 SANGW (0x7e): the angle weight of the state from the popped value.
void opcode_angle_weight(Cpu& c, std::uint32_t ctx) {
    const auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto value = c.u32(top), state_at = c.u32(ctx + interpreter::state);
    c.set({{3, state_at}, {4, top}});
    c.put16(state_at + graphic_state::angle_weight, value);
}

// 0x10758570 FLIPPT (0x80): the on-curve flag of `loop` + 1 popped points of zp0 toggled.
void opcode_flip_points(Cpu& c, std::uint32_t ctx) {
    const auto flags = c.u32(c.u32(ctx + interpreter::zp0) + zone::on_curve);
    auto top = c.u32(ctx + interpreter::stack_top);
    std::uint32_t last = c.r(7);
    std::int32_t n = extsh(c.u16(ctx + interpreter::loop));
    c.set({{4, flags}, {5, top}, {6, reg(n)}});
    while (extsh(reg(n)) >= 0) {
        top -= 4;
        last = c.u32(top);
        --n;
        c.put8(flags + last, c.u8(flags + last) ^ 1);
    }
    c.set({{5, top}, {6, reg(n)}, {7, last}});
    c.put16(ctx + interpreter::loop, 0);
    c.put32(ctx + interpreter::stack_top, top);
}

// 0x107585c0 FLIPRGON (0x81) and 0x10758610 FLIPRGOFF (0x82): the on-curve flags of the points from the first popped to the second set / cleared.
void opcode_flip_range_on(Cpu& c, std::uint32_t ctx) {
    auto top = c.u32(ctx + interpreter::stack_top);
    const auto end = c.u32(top - 4);
    top -= 8;
    const auto start = c.u32(top);
    auto ptr = c.u32(c.u32(ctx + interpreter::zp0) + zone::on_curve) + start;
    auto n = reg(extsh(end - start));
    c.set({{7, top}, {5, start}});
    while (extsh(n) >= 0) {
        c.put8(ptr, c.u8(ptr) | 1);
        --n;
        ++ptr;
    }
    c.set({{4, n}, {6, ptr}});
    c.put32(ctx + interpreter::stack_top, top);
}
void opcode_flip_range_off(Cpu& c, std::uint32_t ctx) {
    const auto flags = c.u32(c.u32(ctx + interpreter::zp0) + zone::on_curve);
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto end = c.u32(top);
    c.set(5, top);
    top -= 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto start = c.u32(top);
    auto ptr = flags + start;
    auto n = reg(extsh(end - start));
    c.set(4, top);
    while (extsh(n) >= 0) {
        c.put8(ptr, c.u8(ptr) & 0xfe);
        --n;
        ++ptr;
    }
    c.set({{3, n}, {6, ptr}});
}

// 0x10758670 SCANCTRL (0x85) and 0x107586a0 SCANTYPE (0x8d): the scan converter's control halfword and type of the state.
void opcode_scan_control(Cpu& c, std::uint32_t ctx) {
    const auto state_at = c.u32(ctx + interpreter::state);
    auto top = c.u32(ctx + interpreter::stack_top);
    const auto word = c.u32(state_at + graphic_state::scan_control);
    top -= 4;
    c.put32(ctx + interpreter::stack_top, top);
    c.set({{3, word & 0xffff0000u}, {4, top}, {5, state_at}});
    c.put32(state_at + graphic_state::scan_control, (word & 0xffff0000u) | c.u32(top));
}
void opcode_scan_type(Cpu& c, std::uint32_t ctx) {
    auto top = c.u32(ctx + interpreter::stack_top);
    const auto state_at = c.u32(ctx + interpreter::state);
    top -= 4;
    c.put32(ctx + interpreter::stack_top, top);
    c.set({{4, top}, {5, state_at}});
    const auto value = c.u32(top);
    if (value > 2) return;
    c.put32(state_at + graphic_state::scan_control, (c.u32(state_at + graphic_state::scan_control) & 0xffff) | (value << 16));
}

// 0x10758710 INSTCTRL (0x8e): in the prep only (the state's byte +0xc8), selector 1 sets the state's flag word's bit 0, selector 2 its bit 1.
void opcode_instruction_control(Cpu& c, std::uint32_t ctx) {
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    const auto state_at = c.u32(ctx + interpreter::state);
    c.put32(ctx + interpreter::stack_top, top);
    const auto selector = c.u32(top);
    top -= 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto value = c.u32(top);
    c.set({{4, value}, {5, selector}, {6, state_at}});
    if (static_cast<std::int8_t>(c.u8(state_at + 0xc8)) == 0) return;
    const auto flags = c.u32(state_at + graphic_state::instruct_control);
    if (selector == 1) {
        c.put32(state_at + graphic_state::instruct_control, flags & ~1u);
        c.set(3, c.u32(state_at + graphic_state::instruct_control));
        c.put32(state_at + graphic_state::instruct_control, c.r(3) | (value & 1));
    } else if (selector == 2) {
        c.put32(state_at + graphic_state::instruct_control, flags & ~2u);
        c.set(3, c.u32(state_at + graphic_state::instruct_control));
        c.put32(state_at + graphic_state::instruct_control, c.r(3) | (value & 2));
    }
}

// 0x10758ae0: `count` bytes (`bytes` nonzero) or words (big-endian halfwords, sign-extended) of the instruction stream pushed on the stack; the
// instruction pointer moves past them. 0x10758b60 PUSHB[n], 0x10758b90 NPUSHB, 0x10758bd0 PUSHW[n], 0x10758c00 NPUSHW come through it.
void push_stream(Cpu& c, std::uint32_t ctx, std::uint32_t count, std::uint32_t bytes) {
    auto top = c.u32(ctx + interpreter::stack_top);
    auto ptr = c.u32(ctx + interpreter::ip);
    std::int32_t n = wrap(count - 1);
    c.set({{5, top}, {6, ptr}});
    std::uint32_t high = c.r(4);
    if (bytes != 0) {
        while (extsh(reg(n)) >= 0) {
            c.put32(top, c.u8(ptr));
            --n; ++ptr; top += 4;
        }
    } else {
        while (extsh(reg(n)) >= 0) {
            high = std::uint32_t(c.u8(ptr)) << 8;
            c.put32(top, reg(extsh(high + c.u8(ptr + 1))));
            --n; ptr += 2; top += 4;
        }
    }
    c.set({{4, high}, {5, top}, {6, ptr}, {7, reg(n)}});
    c.put32(ctx + interpreter::stack_top, top);
    c.put32(ctx + interpreter::ip, ptr);
}
void opcode_push_bytes(Cpu& c, std::uint32_t ctx, std::uint32_t entry, std::uint32_t back, std::uint32_t kind) {
    Frame frame(c, entry, 0x40, {});
    const auto bytes = kind & 1;
    std::uint32_t count;
    c.set(5, bytes);
    if (kind & 2) {  // the count is in the instruction stream
        const auto ip_at = c.u32(ctx + interpreter::ip);
        c.put32(ctx + interpreter::ip, ip_at + 1);
        count = c.u8(ip_at);
    } else {  // the count comes from the opcode (PUSHB[n]: n + 1; PUSHW[n]: n + 1)
        const auto op = c.u32(ctx + interpreter::opcode);
        count = reg(extsh(op - (bytes ? 0xafu : 0xb7u)));
    }
    c.set(4, count);
    c.link(back);
    push_stream(c, ctx, count, bytes);
}

// 0x10758c40 WS (0x42) and 0x10758c80 RS (0x43): the storage area (state +4) written and read at the popped index.
void opcode_write_storage(Cpu& c, std::uint32_t ctx) {
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto value = c.u32(top);
    top -= 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto storage = c.u32(c.u32(ctx + interpreter::state) + graphic_state::storage);
    c.set({{3, storage}, {4, top}, {5, value}});
    c.put32(storage + (c.u32(top) << 2), value);
}
void opcode_read_storage(Cpu& c, std::uint32_t ctx) {
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto index = c.u32(top);
    const auto storage = c.u32(c.u32(ctx + interpreter::state) + graphic_state::storage);
    const auto value = c.u32(storage + (index << 2));
    c.set({{4, top}, {5, value}});
    c.put32(ctx + interpreter::stack_top, top + 4);
    c.put32(top, value);
}

// 0x10758cc0 WCVTP (0x44): the popped value stored at the popped cvt index (state +8); a nonzero value is then read back through the context's
// scaling function and, when that differs, replaced by the value times the ratio of the two (0x10767eb0 then 0x10767d40).
void opcode_write_cvt_pixels(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10758cc0, 0x50, {.first_gpr = 29});
    Volatile v;
    load(c, v);
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto value = c.u32(top);
    top -= 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto state_at = c.u32(ctx + interpreter::state);
    const auto index = c.u32(top);
    const auto cvt = c.u32(state_at + graphic_state::cvt);
    v[5] = cvt;
    v[4] = index;
    c.put32(cvt + (index << 2), value);
    if (value != 0) {
        store(c, v);
        call_through(c, c.u32(ctx + interpreter::scale_cvt), 0x10758d1c);
        load(c, v);
        v[4] = v[3];
        if (v[4] != 0 && v[4] != value) {
            Leftovers left{v[4], v[5], v[6], v[7], v[8]};
            v[3] = value;
            const auto ratio = reg(fix_divide(wrap(value), wrap(v[4]), left));
            v[3] = ratio; v[4] = left.r4; v[5] = left.r5; v[6] = left.r6;
            v[4] = ratio;
            v[3] = value;
            Leftovers second{v[4], v[5], v[6], v[7], v[8]};
            const auto scaled = reg(fix_multiply(wrap(value), wrap(ratio), second));
            v[3] = scaled; v[4] = second.r4; v[5] = second.r5; v[6] = second.r6; v[7] = second.r7; v[8] = second.r8;
            v[4] = c.u32(c.u32(ctx + interpreter::state) + graphic_state::cvt);
            c.put32(v[4] + (index << 2), scaled);
        }
    }
    store(c, v);
}

// 0x10758d70 WCVTF (0x70): the popped value converted by the state's scale callback and stored at the popped cvt index.
void opcode_write_cvt_font_units(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10758d70, 0x40, {.first_gpr = 30});
    Volatile v;
    load(c, v);
    auto top = c.u32(ctx + interpreter::stack_top);
    const auto state_at = c.u32(ctx + interpreter::state);
    top -= 4;
    c.put32(ctx + interpreter::stack_top, top);
    v[4] = c.u32(top);
    top -= 4;
    c.put32(ctx + interpreter::stack_top, top);
    v[5] = top;
    v[3] = state_at;
    const auto index = c.u32(top);
    store(c, v);
    call_through(c, c.u32(state_at + graphic_state::scale_callback), 0x10758db4);
    load(c, v);
    v[4] = c.u32(state_at + graphic_state::cvt);
    c.put32(v[4] + (index << 2), v[3]);
    store(c, v);
}

// 0x10758de0 RCVT (0x45): the popped cvt index read through the context's scaling function and pushed.
void opcode_read_cvt(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10758de0, 0x40, {.first_gpr = 31});
    Volatile v;
    load(c, v);
    const auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    v[4] = c.u32(top);
    v[3] = ctx;
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::scale_cvt), 0x10758e0c);
    load(c, v);
    const auto push = c.u32(ctx + interpreter::stack_top);
    c.put32(ctx + interpreter::stack_top, push + 4);
    c.put32(push, v[3]);
    v[4] = push;
    store(c, v);
}

// 0x10758e40 GC[a] (0x46, 0x47): the coordinate of the popped point of zp2 along the projection vector (current position), or with bit 0 the
// original position along the dual projection vector; pushed.
void opcode_get_coordinate(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10758e40, 0x40, {.first_gpr = 31});
    Volatile v;
    load(c, v);
    const auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto point = c.u32(top);
    const auto zone_at = c.u32(ctx + interpreter::zp2);
    v[5] = point;
    v[6] = zone_at;
    v[3] = ctx;
    const bool original = (c.u32(ctx + interpreter::opcode) & 1) != 0;
    const auto xs = original ? zone::original_xs : zone::xs, ys = original ? zone::original_ys : zone::ys;
    v[4] = c.u32(zone_at + xs);
    v[5] = c.u32(zone_at + ys);
    v[4] = c.u32(v[4] + (point << 2));
    v[5] = c.u32(v[5] + (point << 2));
    store(c, v);
    call_through(c, c.u32(ctx + (original ? interpreter::dual_project : interpreter::project)), original ? 0x10758e90 : 0x10758eb4);
    load(c, v);
    const auto push = c.u32(ctx + interpreter::stack_top);
    c.put32(ctx + interpreter::stack_top, push + 4);
    c.put32(push, v[3]);
    v[4] = push;
    store(c, v);
}

// 0x10758ee0 SCFS (0x48): the popped point of zp2 moved along the freedom vector until its coordinate along the projection vector is the
// popped value; a twilight zone point's original position set to the result.
void opcode_set_coordinate(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10758ee0, 0x50, {.first_gpr = 27});
    Volatile v;
    load(c, v);
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto value = c.u32(top);
    top -= 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto zone_at = c.u32(ctx + interpreter::zp2);
    const auto point = c.u32(top), point_x4 = point << 2;
    v[5] = c.u32(zone_at + zone::ys);
    v[4] = c.u32(zone_at + zone::xs);
    v[4] = c.u32(v[4] + point_x4);
    v[5] = c.u32(v[5] + point_x4);
    v[3] = ctx;
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::project), 0x10758f34);
    load(c, v);
    v[6] = value - v[3]; v[3] = ctx; v[4] = zone_at; v[5] = point;
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::move), 0x10758f50);
    load(c, v);
    if (zone_at == c.u32(ctx + interpreter::zones)) {
        v[4] = c.u32(zone_at + zone::xs);
        v[3] = c.u32(zone_at + zone::original_xs);
        c.put32(v[3] + point_x4, c.u32(v[4] + point_x4));
        v[4] = c.u32(zone_at + zone::ys);
        v[3] = c.u32(zone_at + zone::original_ys);
        c.put32(v[3] + point_x4, c.u32(v[4] + point_x4));
    }
    store(c, v);
}

// 0x10758fa0 MD[a] (0x49, 0x4a): the distance between two popped points along the projection vector: current positions (0x49) or, for 0x4a, the
// original unscaled ones along the dual projection vector then through the state's scale callback; one value pushed in place of the two.
void opcode_measure_distance(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10758fa0, 0x50, {.first_gpr = 29});
    Volatile v;
    load(c, v);
    const auto op = c.u32(ctx + interpreter::opcode);
    auto top = c.u32(ctx + interpreter::stack_top);
    const auto state_at = c.u32(ctx + interpreter::state);
    const auto a = c.u32(top - 4), b = c.u32(top - 8);
    const auto zp0 = c.u32(ctx + interpreter::zp0), zp1 = c.u32(ctx + interpreter::zp1);
    v[6] = a;
    v[5] = b;
    if (((op - 0x49) & 1) != 0) {
        const auto half = [&](std::uint32_t array, std::uint32_t index) { return reg(extsh(c.u16(array + 2 * index))); };
        v[9] = zp0;
        v[7] = c.u32(zp1 + zone::unscaled_xs);
        v[6] = c.u32(zp0 + zone::unscaled_xs);
        v[5] = c.u32(zp1 + zone::unscaled_ys);
        v[4] = c.u32(zp0 + zone::unscaled_ys);
        v[7] = half(v[7], a);
        v[6] = half(v[6], b);
        v[5] = half(v[5], a);
        const auto y = half(v[4], b);
        v[4] = v[6] - v[7];
        v[5] = y - v[5];
        v[3] = ctx;
        v[8] = b << 1;
        store(c, v);
        call_through(c, c.u32(ctx + interpreter::dual_project), 0x1075901c);
        load(c, v);
        v[4] = v[3];
        v[3] = state_at;
        store(c, v);
        call_through(c, c.u32(state_at + graphic_state::scale_callback), 0x10759030);
        load(c, v);
    } else {
        v[9] = zp0;
        v[7] = c.u32(zp1 + zone::xs);
        v[6] = c.u32(zp0 + zone::xs);
        v[5] = c.u32(zp1 + zone::ys);
        v[4] = c.u32(zp0 + zone::ys);
        v[8] = b << 2;
        v[7] = c.u32(v[7] + (a << 2));
        v[6] = c.u32(v[6] + v[8]);
        v[5] = c.u32(v[5] + (a << 2));
        const auto y = c.u32(v[4] + v[8]);
        v[4] = v[6] - v[7];
        v[5] = y - v[5];
        v[3] = ctx;
        store(c, v);
        call_through(c, c.u32(ctx + interpreter::project), 0x10759078);
        load(c, v);
    }
    c.put32(top - 8, v[3]);
    top -= 4;
    c.put32(ctx + interpreter::stack_top, top);
    store(c, v);
}

// 0x107590b0 MPPEM (0x4b): the ppem (the state's halfword +0xc, scaled by the projection vector's ratios unless the state's mode byte is set), pushed.
void opcode_measure_ppem(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x107590b0, 0x50, {.first_gpr = 28});
    Volatile v;
    load(c, v);
    const auto state_at = c.u32(ctx + interpreter::state);
    std::uint32_t ppem = c.u16(state_at + 0xc);
    if (static_cast<std::int8_t>(c.u8(state_at + 0xb6)) == 0) {
        const auto y = extsh(c.u16(ctx + interpreter::projection_y)), x = extsh(c.u16(ctx + interpreter::projection_x));
        v[3] = reg(y);
        std::uint32_t ratio;
        if (y == 0) {
            ratio = c.u32(state_at + 0xb8);
        } else if (x == 0) {
            ratio = c.u32(state_at + 0xbc);
        } else {
            const auto term = [](std::int32_t component) { return reg(extsh(reg(sra(reg(component) * reg(component) + 0x2000, 14)))); };
            Leftovers left{v[4], v[5], v[6], v[7], v[8]};
            v[4] = term(y);
            v[3] = c.u32(state_at + 0xbc);
            const auto first = reg(scale_multiply(wrap(v[3]), wrap(v[4]), left));
            v[3] = first; v[4] = left.r4; v[5] = left.r5; v[6] = left.r6;
            v[4] = term(x);
            v[3] = c.u32(state_at + 0xb8);
            const auto second = reg(scale_multiply(wrap(v[3]), wrap(v[4]), left));
            v[3] = second; v[4] = left.r4; v[5] = left.r5; v[6] = left.r6;
            ratio = first + second;
        }
        v[4] = ratio;
        v[3] = ppem;
        Leftovers left{v[4], v[5], v[6], v[7], v[8]};
        ppem = reg(fix_multiply(wrap(ppem), wrap(ratio), left)) & 0xffffu;
        v[3] = ppem; v[4] = left.r4; v[5] = left.r5; v[6] = left.r6; v[7] = left.r7; v[8] = left.r8;
    }
    const auto push = c.u32(ctx + interpreter::stack_top);
    v[3] = push;
    v[4] = ppem & 0xffffu;
    c.put32(ctx + interpreter::stack_top, push + 4);
    c.put32(push, v[4]);
    store(c, v);
}

// 0x10759190 MPS (0x4c): the state's halfword +0xe (the point size) pushed.
void opcode_measure_point_size(Cpu& c, std::uint32_t ctx) {
    const auto state_at = c.u32(ctx + interpreter::state);
    const auto push = c.u32(ctx + interpreter::stack_top);
    const auto value = c.u16(state_at + 0xe);
    c.set({{4, push}, {5, value}});
    c.put32(ctx + interpreter::stack_top, push + 4);
    c.put32(push, value);
}

// 0x107591b0 GETINFO (0x88): the popped selector's bits answered: bit 0 the version (1), bit 1 the rotated flag (answer 0x100), bit 2 the
// stretched flag (0x200) of the state's byte +0xb7.
void opcode_get_info(Cpu& c, std::uint32_t ctx) {
    const auto state_at = c.u32(ctx + interpreter::state);
    const auto selector = pop(c, ctx);
    std::uint32_t answer = 0;
    if (selector & 1) answer |= 1;
    if ((selector & 2) && (c.u8(state_at + graphic_state::program_flags) & 1)) answer |= 0x100;
    if ((selector & 4) && (c.u8(state_at + graphic_state::program_flags) & 2)) answer |= 0x200;
    const auto push = c.u32(ctx + interpreter::stack_top);
    c.set({{4, push}, {5, state_at}, {6, answer}});
    c.put32(ctx + interpreter::stack_top, push + 4);
    c.put32(push, answer);
}

// 0x10759220 FLIPON (0x4d), 0x10759230 FLIPOFF (0x4e): the state's auto flip byte; 0x10759240 DEBUG (0x4f): pops a value.
void opcode_set_auto_flip(Cpu& c, std::uint32_t ctx, std::uint32_t on) {
    const auto state_at = c.u32(ctx + interpreter::state);
    c.set(3, state_at);
    c.put8(state_at + graphic_state::auto_flip, on);
}
void opcode_debug(Cpu& c, std::uint32_t ctx) {
    const auto top = c.u32(ctx + interpreter::stack_top);
    c.set(4, top);
    c.put32(ctx + interpreter::stack_top, top - 4);
}

// 0x10758790 (0x7f): the popped point's direction from rp0 (its dy and dx, made positive and ordered so the smaller comes first) is matched against
// the 20 unit vectors of the state's trig table (+0xc0, the angle each stands for in the halfwords at +0xc4): the closest within a bound, weighted
// by the state's angle weight, becomes the projection vector (the swap and the signs undone), the dot product is recomputed and the point is
// moved by its distance from rp0 along the new vector (the point's zone and rp0's roles are as the original uses them); the projection
// vector and the dot product are then restored.
void opcode_adjust_angle(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10758790, 0xa0, {.first_gpr = 13});
    Volatile v;
    load(c, v);
    const auto state_at = c.u32(ctx + interpreter::state);
    const auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto zp0 = c.u32(ctx + interpreter::zp0), zp1 = c.u32(ctx + interpreter::zp1);
    const auto point = c.u32(top), point_x4 = point << 2, rp0_x4 = c.u32(ctx + interpreter::rp0) << 2;
    v[8] = zp1; v[7] = rp0_x4;
    v[4] = c.u32(c.u32(zp0 + zone::ys) + rp0_x4);
    const auto point_y = c.u32(c.u32(zp1 + zone::ys) + point_x4);
    v[6] = c.u32(c.u32(zp0 + zone::xs) + rp0_x4);
    v[5] = c.u32(zp1 + zone::xs);
    v[3] = c.u32(v[5] + point_x4);
    const auto saved_x = c.u16(ctx + interpreter::projection_x), saved_y = c.u16(ctx + interpreter::projection_y), saved_dot = c.u16(ctx + interpreter::vector_dot);
    std::int32_t small = wrap(point_y - v[4]), large = wrap(v[3] - v[6]);  // dy, dx
    bool negative_y = false, negative_x = false, swapped = false;
    if (small < 0) { small = wrap(negate(reg(small))); negative_y = true; }
    if (large < 0) { large = wrap(negate(reg(large))); negative_x = true; }
    if (small > large) { std::swap(small, large); swapped = true; }
    const auto a = rlwinm(negate(reg(small)), 10, 0, 21), b = rlwinm(reg(large), 10, 0, 21);
    v[3] = a; v[4] = b;  // before the helper: its early returns leave r4 as passed
    Leftovers left{v[4], v[5], v[6], v[7], v[8]};
    const auto length = reg(distance(wrap(a), wrap(b), left));
    v[3] = length; v[4] = left.r4; v[5] = left.r5; v[6] = left.r6; v[7] = left.r7; v[8] = left.r8;
    const auto along_a = reg(fix_divide(wrap(a), wrap(length), left));
    v[3] = along_a; v[4] = left.r4; v[5] = left.r5; v[6] = left.r6;
    const auto along_b = reg(fix_divide(wrap(b), wrap(length), left));
    v[3] = along_b; v[4] = left.r4; v[5] = left.r5; v[6] = left.r6;
    v[5] = reg(sra(along_a, 2)); v[4] = 0xc910000u;
    const auto target_x = reg(extsh(v[5])), target_y = reg(extsh(reg(sra(along_b, 2))));
    const std::int32_t bound = 0xc910000 + 0x7b7;
    auto vectors = c.u32(state_at + graphic_state::trig_vectors), angles = c.u32(state_at + graphic_state::trig_angles);
    std::int32_t best_score = 0x280, best = -1;
    for (std::int32_t index = 0; extsh(reg(index)) < 0x14; ++index) {
        const auto angle = extsh(c.u16(angles));
        angles += 2;
        if (angle >= best_score) break;
        c.put16(ctx + interpreter::projection_x, c.u32(vectors));
        c.put16(ctx + interpreter::projection_y, c.u32(vectors + 4));
        vectors += 8;
        const auto px = reg(extsh(c.u16(ctx + interpreter::projection_x))), py = reg(extsh(c.u16(ctx + interpreter::projection_y)));
        v[4] = py;
        v[5] = px - target_x;
        v[6] = reg(sra(v[5], 31));
        v[7] = py - target_y;
        v[3] = reg(sra(v[7], 31));
        v[5] = (v[6] ^ v[5]) - v[6];
        v[7] = (v[3] ^ v[7]) - v[3];
        const auto reach = wrap(v[5]) > wrap(v[7]) ? wrap(v[5] + (reg(sra(v[7], 1)))) : wrap(v[7] + reg(sra(v[5], 1)));
        if (reach > bound) continue;
        Leftovers scale{v[4], v[5], v[6], v[7], v[8]};
        v[3] = reg(scale_multiply(small, wrap(py), scale));
        v[4] = scale.r4; v[5] = scale.r5; v[6] = scale.r6;
        const auto first = v[3];
        v[4] = px;
        v[3] = reg(scale_multiply(large, wrap(px), scale));
        v[4] = scale.r4; v[5] = scale.r5; v[6] = scale.r6;
        v[3] += first;
        if (wrap(v[3]) < 0) v[3] = negate(v[3]);
        const auto total = wrap(reg(sra(reg(extsh(c.u16(state_at + graphic_state::angle_weight))) * v[3], 6)) + reg(angle));
        if (total < best_score) { best_score = total; best = extsh(reg(index)); }
    }
    v[6] = 0;
    if (best >= 0) {
        const auto entry = c.u32(state_at + graphic_state::trig_vectors) + (reg(best) << 3);
        v[3] = c.u32(state_at + graphic_state::trig_vectors); v[4] = reg(best) << 3;
        c.put16(ctx + interpreter::projection_x, c.u32(entry));
        c.put16(ctx + interpreter::projection_y, c.u32(entry + 4));
        v[3] = entry;
        if (swapped) {
            const auto y2 = c.u16(ctx + interpreter::projection_y), x2 = c.u16(ctx + interpreter::projection_x);
            v[3] = reg(extsh(y2));
            c.put16(ctx + interpreter::projection_y, x2);
            c.put16(ctx + interpreter::projection_x, y2);
        }
        if (negative_x) c.put16(ctx + interpreter::projection_x, negate(reg(extsh(c.u16(ctx + interpreter::projection_x)))));
        if (negative_y) c.put16(ctx + interpreter::projection_y, negate(reg(extsh(c.u16(ctx + interpreter::projection_y)))));
        v[5] = reg(extsh(c.u16(ctx + interpreter::projection_x)));
        v[4] = reg(extsh(c.u16(ctx + interpreter::freedom_x)));
        v[3] = reg(extsh(c.u16(ctx + interpreter::projection_y)));
        const auto fy = reg(extsh(c.u16(ctx + interpreter::freedom_y)));
        v[4] = v[5] * v[4];
        v[3] = v[3] * fy;
        v[4] = reg(sra(v[4] + 0x2000, 14));
        auto dot = extsh(reg(sra(v[3] + 0x2000, 14) + sra(v[4], 0)));
        if (dot > -0x400 && dot < 0x400) dot = dot >= 0 ? 0x4000 : -0x4000;
        c.put16(ctx + interpreter::vector_dot, reg(dot));
        v[3] = c.u32(ctx + interpreter::zp0);
        v[7] = c.u32(ctx + interpreter::zp1);
        const auto rp0_again = c.u32(ctx + interpreter::rp0) << 2;
        v[6] = c.u32(v[3] + zone::xs);
        v[4] = c.u32(v[3] + zone::ys);
        v[5] = c.u32(v[7] + zone::xs);
        v[3] = c.u32(v[7] + zone::ys);
        v[7] = c.u32(v[6] + point_x4);
        v[6] = c.u32(v[5] + rp0_again);
        v[5] = c.u32(v[4] + point_x4);
        const auto rp0_y = c.u32(v[3] + rp0_again);
        const auto reach_x = v[6] - v[7];
        v[4] = reg(extsh(c.u16(ctx + interpreter::projection_y)));
        v[3] = rp0_y - v[5];
        Leftovers scale{v[4], v[5], v[6], v[7], v[8]};
        v[3] = reg(scale_multiply(wrap(v[3]), wrap(v[4]), scale));
        v[4] = scale.r4; v[5] = scale.r5; v[6] = scale.r6;
        v[4] = reg(extsh(c.u16(ctx + interpreter::projection_x)));
        const auto kept = v[3];
        v[3] = reach_x;
        v[3] = reg(scale_multiply(wrap(v[3]), wrap(v[4]), scale));
        v[4] = scale.r4; v[5] = scale.r5; v[6] = scale.r6;
        v[6] = v[3] + kept;
    }
    v[4] = c.u32(ctx + interpreter::zp1); v[3] = ctx; v[5] = point;
    store(c, v);
    call_through_move(c, 0x10758aac);
    c.put16(ctx + interpreter::projection_x, saved_x);
    c.put16(ctx + interpreter::projection_y, saved_y);
    c.put16(ctx + interpreter::vector_dot, saved_dot);
}
}
