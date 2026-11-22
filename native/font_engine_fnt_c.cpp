// The font engine, block `fnt_c` (FNT's last third, 0x10759250..0x1075ac40): the TrueType interpreter's flow control and its definition
// opcodes, the two jump-table dispatchers (comparison and arithmetic, unary), the rounding and delta opcodes, MDRP and MIRP, the instruction
// that skips an instruction's operands, the function that fills the 256-entry opcode table and the one that builds the interpreter's state
// tables (0x1075a970, called by the hint font's constructor).
//
// Every handler takes the execution context (font_engine_detail.hpp, `interpreter`) in r3 and is reached through the indirect glue
// (0x10781320) from the interpreter's loop, which reads only the context's instruction pointer afterwards; what a handler leaves in
// r3-r10 goes on to the next handler and, at the end, to whoever called the interpreter, so each routine keeps a shadow `v` of r3-r10
// (v[n] = rn) updated where the original's instructions write them and stores it before every call and at the return. The function words
// of the context and the state hold descriptors that are called through the glue exactly as the original does (the font program can
// redefine opcodes, so nothing here is switched on in place of a table or descriptor); the reload of r2 after each call is the original's.
#include "font_engine_detail.hpp"
#include <array>

namespace cm::aot::font_engine {
namespace {
using guest::Frame;
constexpr std::uint32_t glue = 0x10781320, longjmp_entry = 0x10773f30;
// The entries this block calls into (FNT's first third, still generated): the rounding function (value in r4, compensation in r5).
constexpr std::uint32_t round_entry = 0x10755ab0;

constexpr std::int32_t extsh(std::uint32_t v) { return std::int16_t(v); }
constexpr std::uint32_t reg(std::int32_t v) { return std::uint32_t(v); }
using Volatile = std::array<std::uint32_t, 11>;
void store(Cpu& c, const Volatile& v) { for (unsigned n = 3; n <= 10; ++n) c.set(n, v[n]); }
void load(Cpu& c, Volatile& v) { for (unsigned n = 3; n <= 10; ++n) v[n] = c.r(n); }
// sraw: the shift amount is the register's low six bits; 32..63 leave the sign.
constexpr std::int32_t shift_right(std::int32_t value, std::uint32_t amount) {
    amount &= 63;
    return amount >= 32 ? (value < 0 ? -1 : 0) : value >> amount;
}
constexpr std::int32_t half_sra(std::uint32_t v, unsigned n) { return std::int32_t(v) >> n; }

// `bl glue` with r12 = the descriptor, then `lwz r2,0x14(r1)`. The caller stores its shadow before and loads it after.
void call_through(Cpu& c, std::uint32_t descriptor, std::uint32_t back) {
    c.set(12, descriptor);
    c.call(glue, back);
    c.set(2, c.u32(c.sp() + 0x14));
}
// The next instruction byte: ip is read, stored back plus one, the byte stored as the current opcode.
std::uint32_t fetch(Cpu& c, std::uint32_t ctx, std::uint32_t& previous_ip) {
    previous_ip = c.u32(ctx + interpreter::ip);
    c.put32(ctx + interpreter::ip, previous_ip + 1);
    const auto op = c.u8(previous_ip);
    c.put32(ctx + interpreter::opcode, op);
    return op;
}
}

// 0x10759520: the instruction pointer past the operands of the current opcode (NPUSHB 0x40, NPUSHW 0x41, PUSHB 0xb0..0xb7, PUSHW 0xb8..0xbf;
// every other opcode has none). Leaves r4 = the opcode and r5 = the instruction pointer, as the callers' loops read them.
void skip_operands(Cpu& c, std::uint32_t ctx) {
    const auto op = c.u32(ctx + interpreter::opcode);
    auto at = c.u32(ctx + interpreter::ip);
    c.set(4, op);
    if (op >= 0xb0) {
        if (op < 0xc0) {
            if (op >= 0xb8) at += 2 * (op - 0xb7);
            else at += op - 0xaf;
        }
    } else if (op == 0x41) {
        at += 1 + 2 * std::uint32_t(c.u8(at));
    } else if (op == 0x40) {
        at += 1 + std::uint32_t(c.u8(at));
    }
    c.set(5, at);
    c.put32(ctx + interpreter::ip, at);
}

// 0x10759250: the opcodes 0x50..0x55 (LT, LTEQ, GT, GTEQ, EQ, NEQ), 0x5a AND, 0x5b OR, 0x60 ADD, 0x61 SUB, 0x62 DIV, 0x63 MUL, 0x8b MAX, 0x8c MIN:
// pops two values (a below b), pushes one. The original selects through a jump table by opcode - 0x50 (r5 keeps the table's entry) and
// pops both for an opcode with no case. The comparisons use the compiler's sign idioms; the registers they leave are modelled one by one.
void stack_arithmetic(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10759250, 0x40, {.first_gpr = 30});
    Volatile v;
    load(c, v);
    const auto index = c.u32(ctx + interpreter::opcode) - 0x50;
    auto top = c.u32(ctx + interpreter::stack_top);
    const auto b = c.u32(top - 4), a = c.u32(top - 8);
    top -= 8;
    v[3] = a;
    v[4] = b;
    bool push = false;
    std::uint32_t result = 0;
    if (index <= 0x3c) {
        v[5] = c.u32(c.toc(-0x3c88) + 4 * index);
        switch (index) {
        case 0: {  // LT: the sign of (a ^ b) >> 1 less ((a ^ b) & b)
            const auto t = b ^ a;
            v[3] = reg(half_sra(t, 1));
            result = (v[3] - (t & b)) >> 31;
            push = true;
            break;
        }
        case 1: {  // LTEQ: b >= a as sign(b) + (a >> 31) + carry of b - a
            v[6] = reg(half_sra(b, 31));
            v[5] = a >> 31;
            result = v[6] + v[5] + (b >= a ? 1u : 0u);
            push = true;
            break;
        }
        case 2: {  // GT
            const auto t = a ^ b;
            v[4] = reg(half_sra(t, 1));
            result = (v[4] - (t & a)) >> 31;
            push = true;
            break;
        }
        case 3: {  // GTEQ
            v[6] = reg(half_sra(a, 31));
            v[5] = b >> 31;
            result = v[6] + v[5] + (a >= b ? 1u : 0u);
            push = true;
            break;
        }
        case 4:  // EQ: the count of leading zeros of b - a, divided by 32
            result = std::uint32_t(std::countl_zero(b - a)) >> 5;
            push = true;
            break;
        case 5: {  // NEQ
            v[5] = b - a;
            result = (v[5] | (a - b)) >> 31;
            push = true;
            break;
        }
        case 10:  // AND
            v[3] = (a != 0 && b != 0) ? 1 : 0;
            result = v[3];
            push = true;
            break;
        case 11:  // OR
            v[3] = (a != 0 || b != 0) ? 1 : 0;
            result = v[3];
            push = true;
            break;
        case 16: result = a + b; push = true; break;
        case 17: result = a - b; push = true; break;
        case 18:  // DIV: (a << 6) / b, signed
            result = std::uint32_t(divide_signed<4>(rlwinm(a, 6, 0, 25), b));
            push = true;
            break;
        case 19: {  // MUL: the 2.30 product helper (0x10767dc0)
            Leftovers left;
            v[3] = reg(frac_multiply(wrap(a), wrap(b), left));
            v[4] = left.r4; v[5] = left.r5; v[6] = left.r6; v[7] = left.r7; v[8] = left.r8;
            result = v[3];
            push = true;
            break;
        }
        case 0x3b:  // MAX
            if (wrap(a) < wrap(b)) v[3] = b;
            result = v[3];
            push = true;
            break;
        case 0x3c:  // MIN
            if (wrap(a) > wrap(b)) v[3] = b;
            result = v[3];
            push = true;
            break;
        default: break;
        }
    }
    if (push) {
        c.put32(top, result);
        top += 4;
    }
    c.put32(ctx + interpreter::stack_top, top);
    store(c, v);
}

// 0x10759430: the opcodes 0x56 ODD, 0x57 EVEN (through the rounding function 0x10755ab0), 0x5c NOT, 0x64 ABS, 0x65 NEG, 0x66 FLOOR, 0x67 CEILING:
// pops one value, pushes one (the jump table has more entries than opcodes: those pop and push nothing).
void stack_unary(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10759430, 0x50, {.first_gpr = 29});
    Volatile v;
    load(c, v);
    const auto op = c.u32(ctx + interpreter::opcode);
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    const auto value = c.u32(top);
    v[4] = value;
    const auto index = op - 0x56;
    bool push = false;
    std::uint32_t result = 0;
    if (index <= 0x11) {
        v[3] = c.u32(c.toc(-0x3c8c) + 4 * index);
        switch (index) {
        case 0:
        case 1: {
            v[3] = ctx;
            v[5] = 0;
            store(c, v);
            c.call(round_entry, 0x10759484);
            load(c, v);
            auto rounded = reg(half_sra(v[3], 6));
            if (op == 0x56) rounded += 1;
            v[3] = rounded;
            result = (rounded & 1) ^ 1;
            push = true;
            break;
        }
        case 6: result = std::uint32_t(std::countl_zero(value)) >> 5; push = true; break;
        case 14: {  // ABS by the xor and subtract idiom: INT_MIN stays
            v[3] = reg(half_sra(value, 31));
            result = (v[3] ^ value) - v[3];
            push = true;
            break;
        }
        case 15: result = negate(value); push = true; break;
        case 16: result = value & 0xffffffc0u; push = true; break;  // FLOOR
        case 17: v[4] = value + 0x3f; result = v[4] & 0xffffffc0u; push = true; break;  // CEILING
        default: break;
        }
    } else {
        v[3] = ctx;
    }
    if (push) {
        c.put32(top, result);
        top += 4;
    }
    c.put32(ctx + interpreter::stack_top, top);
    store(c, v);
}

// 0x107595a0 IF (0x58): pops the condition; when it is zero, skips forward to the matching ELSE or EIF (nested IFs counted, operands of
// the pushes skipped), leaving the instruction pointer after it.
void opcode_if(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x107595a0, 0x40, {.first_gpr = 30});
    Volatile v;
    load(c, v);
    const auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    v[3] = top;
    if (c.u32(top) == 0) {
        std::int32_t depth = 1;
        while (depth != 0) {
            std::uint32_t at;
            const auto op = fetch(c, ctx, at);
            v[3] = at;
            if (op == 0x58) {
                ++depth;
            } else if (op == 0x1b) {
                if (depth == 1) depth = 0;
            } else if (op == 0x59) {
                --depth;
            } else {  // every other opcode
                v[3] = ctx;
                store(c, v);
                c.link(0x10759634);
                skip_operands(c, ctx);
                load(c, v);
            }
        }
    }
    store(c, v);
}

// 0x10759660 ELSE (0x1b): skips forward to the matching EIF.
void opcode_else(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10759660, 0x40, {.first_gpr = 30});
    Volatile v;
    load(c, v);
    std::int32_t depth = 1;
    while (depth != 0) {
        std::uint32_t at;
        const auto op = fetch(c, ctx, at);
        v[3] = at;
        if (op == 0x59) {
            --depth;
        } else if (op == 0x58) {
            ++depth;
        } else {
            v[3] = ctx;
            store(c, v);
            c.link(0x107596c4);
            skip_operands(c, ctx);
            load(c, v);
        }
    }
    store(c, v);
}

// 0x10759700 JMPR (0x1c): the instruction pointer moves by the popped offset (the pointer is already past the opcode).
void opcode_jump(Cpu& c, std::uint32_t ctx) {
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto target = c.u32(top) + c.u32(ctx + interpreter::ip);
    c.set(4, target);
    c.put32(ctx + interpreter::ip, target - 1);
}
// 0x10759730 JROT (0x78) and 0x10759770 JROF (0x79): pop the condition and the offset; jump when the condition is nonzero (JROT) or zero (JROF).
void opcode_jump_if(Cpu& c, std::uint32_t ctx, bool when_true) {
    auto top = c.u32(ctx + interpreter::stack_top);
    const bool taken = (c.u32(top - 4) != 0) == when_true;
    if (taken) {
        top -= 8;
        const auto target = c.u32(top) + c.u32(ctx + interpreter::ip);
        c.set(4, target);
        c.put32(ctx + interpreter::ip, target - 1);
    } else {
        top -= 8;
    }
    c.set(5, top);
    c.put32(ctx + interpreter::stack_top, top);
}

// 0x107597b0 ROUND[ab] (0x68..0x6b): the value rounded through the state's round function (a descriptor called with the context, the
// value and a zero compensation).
void opcode_round(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x107597b0, 0x40, {.first_gpr = 31});
    Volatile v;
    load(c, v);
    const auto state_at = c.u32(ctx + interpreter::state);
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    v[5] = 0;
    v[6] = state_at;
    v[4] = c.u32(top);
    v[3] = ctx;
    store(c, v);
    call_through(c, c.u32(state_at + graphic_state::round_function), 0x107597e4);
    load(c, v);
    top = c.u32(ctx + interpreter::stack_top);
    c.put32(ctx + interpreter::stack_top, top + 4);
    c.put32(top, v[3]);
    v[4] = top;
    store(c, v);
}
// 0x10759810 NROUND[ab] (0x6c..0x6f): the value popped and pushed back unchanged.
void opcode_no_round(Cpu& c, std::uint32_t ctx) {
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto value = c.u32(top);
    c.put32(ctx + interpreter::stack_top, top + 4);
    c.put32(top, value);
    c.set({{4, top}, {5, value}});
}

// 0x10759840: a distance moved to the single width value when it lies within the state's single width cut-in of it (the sign is kept).
// (context, distance) -> the distance.
std::uint32_t single_width_cut_in(Cpu& c, std::uint32_t ctx, std::uint32_t distance) {
    Frame frame(c, 0x10759840, 0x40, {.first_gpr = 30});
    Volatile v;
    load(c, v);
    const auto state_at = c.u32(ctx + interpreter::state);
    v[3] = ctx;
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::single_width), 0x10759864);
    load(c, v);
    const auto single = v[3];
    const auto cut = c.u32(state_at + graphic_state::single_width_cut_in);
    const auto nearer = [&](std::uint32_t d) {  // |d - single| < cut-in, by the xor and subtract idiom; leaves r4 and r5 as it does
        const auto diff = d - single;
        v[5] = reg(half_sra(diff, 31));
        v[4] = (v[5] ^ diff) - v[5];
        return wrap(v[4]) < wrap(cut);
    };
    std::uint32_t result = distance;
    if (wrap(distance) >= 0) {
        if (nearer(distance)) result = single;
    } else {
        result = negate(distance);
        if (nearer(result)) result = single;
        result = negate(result);
    }
    v[3] = result;
    store(c, v);
    return result;
}

// MDRP's and MIRP's rounding step, the minimum distance and the zone lookups share the state's words; below, the two opcodes.
// 0x107598e0 MDRP[abcde] (0xc0..0xdf): moves the popped point so its distance from rp0 along the projection vector equals the original
// distance (measured on the unscaled coordinates, or on the original coordinates when either zone is the twilight zone), scaled, optionally
// cut to the single width, rounded (0x04) and kept at least the minimum distance (0x08); rp1 = rp0, rp2 = the point, rp0 = the point with 0x10.
void opcode_mdrp(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x107598e0, 0x60, {.first_gpr = 25});
    Volatile v;
    load(c, v);
    const auto zp0_at = c.u32(ctx + interpreter::zp0), zp1_at = c.u32(ctx + interpreter::zp1), state_at = c.u32(ctx + interpreter::state);
    const auto zones_at = c.u32(ctx + interpreter::zones), rp0_old = c.u32(ctx + interpreter::rp0);
    const auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto point = c.u32(top);
    v[4] = top;
    v[5] = zp0_at;
    std::uint32_t distance;
    const auto rp0_x4 = rp0_old << 2, point_x4 = point << 2;
    if (zp0_at == zones_at || zp1_at == zones_at) {
        const auto rx = c.u32(c.u32(zp0_at + zone::original_xs) + rp0_x4), ry = c.u32(c.u32(zp0_at + zone::original_ys) + rp0_x4);
        const auto px = c.u32(c.u32(zp1_at + zone::original_xs) + point_x4), py = c.u32(c.u32(zp1_at + zone::original_ys) + point_x4);
        v[3] = ctx; v[4] = px - rx; v[5] = py - ry; v[6] = px; v[7] = rx; v[8] = point_x4;
        store(c, v);
        call_through(c, c.u32(ctx + interpreter::dual_project), 0x10759964);
        load(c, v);
        distance = v[3];
    } else {
        const auto hw = [&](std::uint32_t array, std::uint32_t index) { return reg(extsh(c.u16(array + 2 * index))); };
        const auto rx = hw(c.u32(zp0_at + zone::unscaled_xs), rp0_old), ry = hw(c.u32(zp0_at + zone::unscaled_ys), rp0_old);
        const auto px = hw(c.u32(zp1_at + zone::unscaled_xs), point), py = hw(c.u32(zp1_at + zone::unscaled_ys), point);
        v[3] = ctx; v[4] = px - rx; v[5] = py - ry; v[6] = px; v[7] = rx; v[8] = point << 1;
        store(c, v);
        call_through(c, c.u32(ctx + interpreter::dual_project), 0x107599a8);
        load(c, v);
        v[4] = v[3];
        v[3] = state_at;
        store(c, v);
        call_through(c, c.u32(state_at + graphic_state::scale_callback), 0x107599bc);
        load(c, v);
        distance = v[3];
    }
    if (c.u32(state_at + graphic_state::single_width_cut_in) != 0) {
        v[3] = ctx; v[4] = distance;
        store(c, v);
        c.link(0x107599dc);
        distance = single_width_cut_in(c, ctx, distance);
        load(c, v);
    }
    const auto op = c.u32(ctx + interpreter::opcode);
    const auto original = distance;
    if (op & 4) {
        v[3] = ctx; v[4] = distance; v[5] = 0;
        store(c, v);
        call_through(c, c.u32(state_at + graphic_state::round_function), 0x10759a04);
        load(c, v);
        distance = v[3];
    }
    if (op & 8) {
        const auto minimum = c.u32(state_at + graphic_state::minimum_distance);
        if (wrap(original) >= 0) {
            if (wrap(distance) < wrap(minimum)) distance = minimum;
        } else {
            const auto floor = negate(minimum);
            if (wrap(distance) > wrap(floor)) distance = floor;
        }
    }
    {
        const auto zp0_now = c.u32(ctx + interpreter::zp0);
        const auto rx = c.u32(c.u32(zp0_now + zone::xs) + rp0_x4), ry = c.u32(c.u32(zp0_now + zone::ys) + rp0_x4);
        const auto px = c.u32(c.u32(zp1_at + zone::xs) + point_x4), py = c.u32(c.u32(zp1_at + zone::ys) + point_x4);
        v[3] = ctx; v[4] = px - rx; v[5] = py - ry; v[6] = px; v[7] = rx; v[8] = point_x4;
        store(c, v);
        call_through(c, c.u32(ctx + interpreter::project), 0x10759a84);
        load(c, v);
        distance -= v[3];
    }
    v[3] = ctx; v[4] = zp1_at; v[5] = point; v[6] = distance;
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::move), 0x10759aa4);
    c.put32(ctx + interpreter::rp1, rp0_old);
    c.put32(ctx + interpreter::rp2, point);
    if (op & 0x10) c.put32(ctx + interpreter::rp0, point);
}

// 0x10759ae0 MIRP[abcde] (0xe0..0xff): the popped point is moved to the distance from rp0 that the popped cvt entry gives (scaled; made
// positive or negative like the original distance when the state's auto flip is set; the original distance replaces the cvt value when the two are
// further apart than the control value cut-in; rounded with 0x04, kept to the minimum distance with 0x08); rp1 = rp0, rp2 = the point, rp0 =
// the point with 0x10. A point of the twilight zone first gets its original position from the cvt value along the projection vector.
// The comparisons against the descriptors of the x and y projections (TOC -0x3c50, -0x3c74) take the shortcut of the single coordinate.
void opcode_mirp(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10759ae0, 0x70, {.first_gpr = 20});
    Volatile v;
    load(c, v);
    const auto project_x = c.toc(-0x3c50), project_y = c.toc(-0x3c74);
    const auto state_at = c.u32(ctx + interpreter::state), zp0_at = c.u32(ctx + interpreter::zp0), zp1_at = c.u32(ctx + interpreter::zp1), op = c.u32(ctx + interpreter::opcode);
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    v[3] = ctx;
    v[4] = c.u32(top);
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::scale_cvt), 0x10759b24);
    load(c, v);
    std::uint32_t cvt = v[3];
    if (c.u32(state_at + graphic_state::single_width_cut_in) != 0) {
        v[3] = ctx; v[4] = cvt;
        store(c, v);
        c.link(0x10759b44);
        cvt = single_width_cut_in(c, ctx, cvt);
        load(c, v);
    }
    top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    v[3] = top;
    const auto point = c.u32(top), rp0_old = c.u32(ctx + interpreter::rp0);
    c.put32(ctx + interpreter::rp1, rp0_old);
    c.put32(ctx + interpreter::rp2, point);
    if (op & 0x10) c.put32(ctx + interpreter::rp0, point);
    const auto rp0_x4 = rp0_old << 2, point_x4 = point << 2;
    // 0x10767e40 on the cvt distance and a vector component (r3 = cvt, r4 = the component; leaves r4-r6 as the helper does).
    const auto scaled = [&](std::uint32_t component) {
        v[3] = cvt;
        v[4] = component;
        Leftovers left{v[4], v[5], v[6], v[7], v[8]};
        v[3] = reg(scale_multiply(wrap(cvt), wrap(component), left));
        v[4] = left.r4; v[5] = left.r5; v[6] = left.r6; v[7] = left.r7; v[8] = left.r8;
        return v[3];
    };
    if (zp1_at == c.u32(ctx + interpreter::zones)) {
        auto product = scaled(reg(extsh(c.u16(ctx + interpreter::projection_x))));
        v[5] = c.u32(zp0_at + zone::original_xs);
        v[4] = c.u32(zp1_at + zone::original_xs);
        auto value = c.u32(v[5] + rp0_x4) + product;
        v[3] = cvt;
        c.put32(v[4] + point_x4, value);
        v[4] = c.u32(zp1_at + zone::xs);
        c.put32(v[4] + point_x4, value);
        product = scaled(reg(extsh(c.u16(ctx + interpreter::projection_y))));
        v[5] = c.u32(zp0_at + zone::original_ys);
        v[4] = c.u32(zp1_at + zone::original_ys);
        value = c.u32(v[5] + rp0_x4) + product;
        c.put32(v[4] + point_x4, value);
        v[3] = c.u32(zp1_at + zone::ys);
        c.put32(v[3] + point_x4, value);
    }
    // The distance between the rp0 and the point along a projection function: a descriptor equal to the x (y) projection's reads that
    // coordinate alone; any other is called with the two differences.
    const auto measure = [&](std::uint32_t descriptor, std::uint32_t x_array, std::uint32_t y_array, std::uint32_t back) {
        if (descriptor == project_x || descriptor == project_y) {
            const auto array = descriptor == project_x ? x_array : y_array;
            v[5] = c.u32(zp0_at + array);
            v[4] = rp0_x4;
            v[3] = c.u32(zp1_at + array);
            v[4] = c.u32(v[5] + v[4]);
            return c.u32(v[3] + point_x4) - v[4];
        }
        v[7] = c.u32(zp0_at + x_array);
        v[5] = c.u32(zp0_at + y_array);
        v[8] = point_x4;
        v[6] = c.u32(zp1_at + x_array);
        v[3] = ctx;
        v[4] = c.u32(zp1_at + y_array);
        v[7] = c.u32(v[7] + rp0_x4);
        v[5] = c.u32(v[5] + rp0_x4);
        const auto py = c.u32(v[4] + point_x4);
        v[6] = c.u32(v[6] + point_x4);
        v[5] = py - v[5];
        v[4] = v[6] - v[7];
        store(c, v);
        call_through(c, descriptor, back);
        load(c, v);
        return v[3];
    };
    const auto original = measure(c.u32(ctx + interpreter::dual_project), zone::original_xs, zone::original_ys, 0x10759c60);
    const auto actual = measure(c.u32(ctx + interpreter::project), zone::xs, zone::ys, 0x10759cf4);
    if (c.u8(state_at + graphic_state::auto_flip) != 0 && wrap(original ^ cvt) < 0) cvt = negate(cvt);
    if (op & 4) {
        const auto difference = cvt - original;
        v[4] = reg(half_sra(difference, 31));
        v[3] = (v[4] ^ difference) - v[4];
        if (wrap(v[3]) > wrap(c.u32(state_at + graphic_state::control_value_cut_in))) cvt = original;
        v[3] = ctx; v[4] = cvt; v[5] = 0;
        store(c, v);
        call_through(c, c.u32(state_at + graphic_state::round_function), 0x10759d50);
        load(c, v);
        cvt = v[3];
    }
    if (op & 8) {
        const auto minimum = c.u32(state_at + graphic_state::minimum_distance);
        if (wrap(original) >= 0) {
            if (wrap(cvt) < wrap(minimum)) cvt = minimum;
        } else {
            const auto floor = negate(minimum);
            if (wrap(cvt) > wrap(floor)) cvt = floor;
        }
    }
    v[3] = ctx; v[4] = zp1_at; v[5] = point; v[6] = cvt - actual;
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::move), 0x10759da4);
}

// A function definition is 8 bytes, indexed by its number without a bounds check (state +0x90); a program's base is at state + 0x9c + 4 * its
// number (prep 0, fpgm 1).

// 0x10759dc0 CALL (0x2b): the interpreter's run function (the context's word +0x50, a descriptor) runs the function's range of its program.
void opcode_call(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10759dc0, 0x40, {});
    Volatile v;
    load(c, v);
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    const auto state_at = c.u32(ctx + interpreter::state);
    c.put32(ctx + interpreter::stack_top, top);
    const auto function = c.u32(top);
    const auto definition = c.u32(state_at + graphic_state::function_defs) + (function << 3);
    const auto base = c.u32(state_at + 0x9c + 4 * (c.u16(definition + graphic_state::definition_program) & 0xffffu));
    const auto start = base + c.u32(definition + graphic_state::definition_start);
    v[3] = ctx; v[4] = start; v[5] = start + c.u16(definition + graphic_state::definition_length); v[6] = definition; v[7] = state_at;
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::run), 0x10759e14);
}

// 0x10759e30 FDEF (0x2c): the function number popped; its definition (start in the current program, length) recorded while the
// instruction pointer runs to the matching ENDF (0x2d), skipping every operand on the way.
void opcode_function_def(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10759e30, 0x50, {.first_gpr = 29});
    Volatile v;
    load(c, v);
    const auto top = c.u32(ctx + interpreter::stack_top) - 4;
    const auto state_at = c.u32(ctx + interpreter::state);
    c.put32(ctx + interpreter::stack_top, top);
    const auto function = c.u32(top);
    const auto defs = c.u32(state_at + graphic_state::function_defs);
    const auto program = c.u8(state_at + graphic_state::current_program);
    const auto definition = defs + (function << 3);
    c.put16(definition + graphic_state::definition_program, program);
    const auto base = c.u32(state_at + 0x9c + 4 * (std::uint32_t(program) & 0xffffu));
    c.put32(definition + graphic_state::definition_start, c.u32(ctx + interpreter::ip) - base);
    const auto first = c.u32(ctx + interpreter::ip);
    v[4] = defs;
    v[5] = state_at;
    v[3] = base;
    for (;;) {
        std::uint32_t at;
        const auto op = fetch(c, ctx, at);
        v[3] = at;
        if (op == 0x2d) break;
        v[3] = ctx;
        store(c, v);
        skip_operands(c, ctx);
        load(c, v);
    }
    v[3] = c.u32(ctx + interpreter::ip) - first;
    c.put16(definition + graphic_state::definition_length, v[3] - 1);
    store(c, v);
}

// 0x10759ef0 LOOPCALL (0x2a): the function and the count popped; the function's range run that many times (a count of zero or less runs
// none; the loop variable is the count less one, tested as a halfword).
void opcode_loop_call(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10759ef0, 0x50, {.first_gpr = 27});
    Volatile v;
    load(c, v);
    auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto state_at = c.u32(ctx + interpreter::state);
    const auto function = c.u32(top);
    const auto definition = c.u32(state_at + graphic_state::function_defs) + (function << 3);
    const auto run_descriptor = c.u32(ctx + interpreter::run);
    const auto program = c.u16(definition + graphic_state::definition_program);
    top = c.u32(ctx + interpreter::stack_top) - 4;
    const auto offset = c.u32(definition + graphic_state::definition_start);
    const auto base = c.u32(state_at + 0x9c + 4 * std::uint32_t(program));
    const auto length = c.u16(definition + graphic_state::definition_length);
    c.put32(ctx + interpreter::stack_top, top);
    const auto end = offset + (length + base);
    const auto start = base + offset;
    v[3] = top; v[4] = base; v[5] = state_at; v[6] = offset; v[7] = definition;
    std::int32_t n = extsh(c.u32(top)) - 1;
    while (extsh(reg(n)) >= 0) {
        v[3] = ctx; v[4] = start; v[5] = end;
        store(c, v);
        call_through(c, run_descriptor, 0x10759f7c);
        load(c, v);
        --n;
    }
    store(c, v);
}

// 0x10759fa0 (the opcodes no table entry defines: 0x83, 0x84, 0x8f..0xaf): the instruction definition for the opcode run like a function, or
// the error 0x1101 (longjmp(context + 0x5c, 0x1101)) when none is defined.
void opcode_undefined(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x10759fa0, 0x40, {});
    Volatile v;
    load(c, v);
    const auto state_at = c.u32(ctx + interpreter::state);
    v[7] = state_at;
    const auto wanted = c.u32(ctx + interpreter::opcode) & 0xffu;
    std::int32_t n = extsh(c.u16(state_at + graphic_state::instruction_def_count)) - 1;
    auto at = c.u32(state_at + graphic_state::instruction_defs);
    v[5] = wanted;
    bool found = false;
    while (extsh(reg(n)) >= 0) {
        if (c.u8(at + graphic_state::definition_opcode) == wanted) { found = true; break; }
        at += 8;
        --n;
    }
    v[4] = reg(n);
    if (!found) at = 0;
    v[6] = at;
    if (at == 0) {
        v[3] = ctx + interpreter::jump_buffer;
        v[4] = interpreter::error_illegal_opcode;
        store(c, v);
        c.call(longjmp_entry, 0x1075a000);
        return;
    }
    const auto program = c.u8(at + graphic_state::definition_program);
    const auto offset = c.u32(at);
    const auto length = c.u16(at + graphic_state::definition_length);
    const auto start = c.u32(state_at + 0x9c + 4 * std::uint32_t(program)) + offset;
    v[4] = start; v[5] = start + length;
    store(c, v);
    call_through(c, c.u32(ctx + interpreter::run), 0x1075a02c);
}

// 0x1075a040 IDEF (0x89): the opcode number popped; an instruction definition for it (the existing one, or a new one appended) records
// the current program, the opcode and the start, then the instruction pointer runs to the matching ENDF.
void opcode_instruction_def(Cpu& c, std::uint32_t ctx) {
    Frame frame(c, 0x1075a040, 0x50, {.first_gpr = 29});
    Volatile v;
    load(c, v);
    const auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto state_at = c.u32(ctx + interpreter::state);
    const auto number = c.u32(top);
    std::int32_t n = extsh(c.u16(state_at + graphic_state::instruction_def_count)) - 1;
    auto at = c.u32(state_at + graphic_state::instruction_defs);
    const auto wanted = number & 0xffu;
    v[6] = state_at; v[7] = wanted;
    bool found = false;
    while (extsh(reg(n)) >= 0) {
        if (c.u8(at + graphic_state::definition_opcode) == wanted) { found = true; break; }
        at += 8;
        --n;
    }
    v[4] = reg(n);
    if (!found) at = 0;
    const auto program = c.u8(state_at + graphic_state::current_program);
    const auto first = c.u32(ctx + interpreter::ip);
    const auto base = c.u32(state_at + 0x9c + 4 * std::uint32_t(program));
    v[8] = program; v[9] = base;
    v[3] = state_at + 4 * std::uint32_t(program);
    if (!found) {
        const auto count = reg(extsh(c.u16(state_at + graphic_state::instruction_def_count)));
        v[4] = count;
        v[5] = c.u32(c.u32(ctx + interpreter::state) + graphic_state::instruction_defs);
        v[3] = count + 1;
        c.put16(state_at + graphic_state::instruction_def_count, count + 1);
        at = v[5] + (count << 3);
    }
    c.put8(at + graphic_state::definition_program, program);
    c.put8(at + graphic_state::definition_opcode, wanted);
    c.put32(at, c.u32(ctx + interpreter::ip) - base);
    for (;;) {
        std::uint32_t previous;
        const auto op = fetch(c, ctx, previous);
        v[3] = previous;
        if (op == 0x2d) break;
        v[3] = ctx;
        store(c, v);
        skip_operands(c, ctx);
        load(c, v);
    }
    v[3] = c.u32(ctx + interpreter::ip) - first;
    c.put16(at + graphic_state::definition_length, v[3] - 1);
    store(c, v);
}

// 0x1075a150 UTP (0x29): the popped point's touched flag cleared along the axes the freedom vector has (x: bit 0, y: bit 1).
void opcode_untouch(Cpu& c, std::uint32_t ctx) {
    const auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto flags = c.u32(c.u32(ctx + interpreter::zp0) + zone::touched);
    const auto point = c.u32(top);
    c.set({{4, flags}, {5, point}});
    if (extsh(c.u16(ctx + interpreter::freedom_x)) != 0) c.put8(flags + point, c.u8(flags + point) & ~zone::touched_x);
    if (extsh(c.u16(ctx + interpreter::freedom_y)) != 0) c.put8(flags + point, c.u8(flags + point) & ~zone::touched_y);
}

// 0x1075a1a0 SDB (0x5e) and 0x1075a1c0 SDS (0x5f): the delta base and the delta shift of the state from the popped value.
void opcode_set_delta(Cpu& c, std::uint32_t ctx, std::uint32_t field) {
    const auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto value = c.u32(top);
    const auto state_at = c.u32(ctx + interpreter::state);
    c.set({{3, state_at}, {4, top}});
    c.put16(state_at + field, value);
}

// 0x1075a1e0 (DELTAP1..3, DELTAC1..3): `count` pairs (a point or cvt number below an argument whose high nibble selects the ppem
// less the base and whose low nibble the step) popped; for each pair whose ppem (the state's halfword at +0xc, scaled by the projection vector's
// ratios unless the state's mode byte is set) matches, `function` is called with (context, zone zp0, number, the step in 26.6 shifted right by
// the delta shift). The pairs are searched like a sorted table: a binary search on the argument's high bits first, then a forward scan.
void delta_opcode(Cpu& c, std::uint32_t ctx, std::uint32_t function, std::uint32_t base, std::uint32_t shift) {
    Frame frame(c, 0x1075a1e0, 0x60, {.first_gpr = 24});
    Volatile v;
    load(c, v);
    const auto top = c.u32(ctx + interpreter::stack_top) - 4;
    c.put32(ctx + interpreter::stack_top, top);
    const auto n = c.u32(top);
    v[3] = n << 3;
    v[4] = n;
    const auto words = n << 1;  // r30
    const auto pairs = top - (n << 3);
    c.put32(ctx + interpreter::stack_top, pairs);
    const auto state_at = c.u32(ctx + interpreter::state);
    std::uint32_t ppem = c.u16(state_at + 0xc);  // r31
    if (static_cast<std::int8_t>(c.u8(state_at + 0xb6)) == 0) {
        const auto y = extsh(c.u16(ctx + interpreter::projection_y)), x = extsh(c.u16(ctx + interpreter::projection_x));
        v[3] = reg(y);
        // (component^2 + 0x2000) >> 14 as a halfword: the weight of a vector component in the ratio.
        const auto arithmetic_term = [](std::int32_t component) {
            return reg(extsh(reg(half_sra(reg(component) * reg(component) + 0x2000, 14))));
        };
        std::uint32_t ratio;
        if (y == 0) {
            ratio = c.u32(state_at + 0xb8);
        } else if (x == 0) {
            ratio = c.u32(state_at + 0xbc);
        } else {
            Leftovers left{v[4], v[5], v[6], v[7], v[8]};
            v[4] = arithmetic_term(y);
            v[3] = c.u32(state_at + 0xbc);
            const auto first = reg(scale_multiply(wrap(v[3]), wrap(v[4]), left));
            v[3] = first; v[4] = left.r4; v[5] = left.r5; v[6] = left.r6;
            v[4] = arithmetic_term(x);
            v[3] = c.u32(state_at + 0xb8);
            const auto second = reg(scale_multiply(wrap(v[3]), wrap(v[4]), left));
            v[3] = second; v[4] = left.r4; v[5] = left.r5; v[6] = left.r6;
            ratio = first + second;
        }
        v[4] = ratio;
        v[3] = ppem;
        Leftovers left{v[4], v[5], v[6], v[7], v[8]};
        ppem = reg(fix_multiply(wrap(ppem), wrap(ratio), left));
        v[3] = ppem; v[4] = left.r4; v[5] = left.r5; v[6] = left.r6; v[7] = left.r7; v[8] = left.r8;
    }
    const auto wanted = ppem - reg(extsh(base));
    v[6] = wanted;
    if (wrap(wanted) < 0x10 && wrap(wanted) >= 0) {
        std::int32_t span = half_sra(words, 1) & ~1;  // r5
        const auto key = std::int32_t(wanted << 4);
        std::int32_t index = 0;  // r31, in words
        v[5] = reg(span);
        while (span > 2) {
            const auto at = c.u32(ctx + interpreter::stack_top) + ((reg(index) + reg(span)) << 2);
            v[4] = c.u32(ctx + interpreter::stack_top);
            v[3] = c.u32(at) & 0xfffffff0u;
            if (wrap(v[3]) < key) index += span;
            span = (span >> 1) & ~1;
            v[5] = reg(span);
        }
        auto offset = reg(index) << 2;  // r25
        while (index < wrap(words)) {
            const auto entry = c.u32(ctx + interpreter::stack_top) + offset;  // r7
            v[7] = entry;
            const auto argument = c.u32(entry);
            v[3] = argument;
            const auto candidate = std::int32_t(argument & 0xfffffff0u);
            if (candidate == key) {
                const auto step = argument & 0xf;  // r6
                const auto number = c.u32(entry + 4);
                // (step - 8) + 0x80000008 carries out exactly when step < 8: the steps 0..7 count as -8..-1, 8..15 as 1..8.
                const auto offset_step = step < 8 ? 8u : 7u;
                v[6] = step - offset_step;
                v[4] = c.u32(ctx + interpreter::zp0);
                v[5] = number;
                v[7] = offset_step;
                v[6] = reg(shift_right(wrap(rlwinm(v[6], 6, 0, 25)), reg(extsh(shift))));
                v[3] = ctx;
                store(c, v);
                call_through(c, function, 0x1075a36c);
                load(c, v);
            } else if (candidate > key) {
                break;
            }
            offset += 8;
            index += 2;
        }
    }
    store(c, v);
}

// 0x1075a3a0, 0x1075a3d0, 0x1075a410 DELTAP1..3 (0x5d, 0x71, 0x72): the points moved through the context's move function; the delta base
// is the state's, raised by 0 / 16 / 32 for the three ranges of ppem. 0x1075a450, 0x1075a480, 0x1075a4c0 DELTAC1..3 (0x73..0x75): the same
// over cvt entries, through the function the TOC's -0x3c90 slot points to (0x107560e0).
void opcode_delta(Cpu& c, std::uint32_t ctx, std::uint32_t entry, std::uint32_t function, std::uint32_t range_base, std::uint32_t back) {
    Frame frame(c, entry, 0x40, {});
    const auto state_at = c.u32(ctx + interpreter::state);
    const auto base = reg(extsh(reg(extsh(c.u16(state_at + graphic_state::delta_base)) + std::int32_t(range_base))));
    const auto shift = reg(extsh(c.u16(state_at + graphic_state::delta_shift)));
    c.set({{4, function}, {5, base}, {6, shift}});
    c.link(back);
    delta_opcode(c, ctx, function, base, shift);
}

// 0x1075a500: the 256-entry opcode table at `table`: the descriptor of each opcode's handler, by TOC slot (a font program cannot change
// the table's source: IDEF adds instruction definitions that 0x10759fa0, the entry of every opcode this table leaves undefined, runs). The
// slot of each index below, from the original's loads and stores; the opcodes with no handler of their own (0x83, 0x84, 0x8f..0xaf and the
// five runs after them) take the slots the last five fills use. Leaves r3 = r4 = -1, r5 = the end of the table and r6..r10 as it loads them.
namespace {
constexpr std::array<std::int16_t, 0x8f> table_slots = {
    -0x3c64, -0x3c94, -0x3c98, -0x3c98, -0x3c9c, -0x3c9c, -0x3ca0, -0x3ca0, -0x3ca4, -0x3ca4, -0x3ca8, -0x3cac, -0x3cb0, -0x3cb4, -0x3cb8, -0x3cbc,
    -0x3cc0, -0x3cc0, -0x3cc0, -0x3cc4, -0x3cc4, -0x3cc4, -0x3cc4, -0x3cc0, -0x3cc8, -0x3cc8, -0x3ccc, -0x3cd0, -0x3cd4, -0x3cd8, -0x3cdc, -0x3ce0,
    -0x3ce4, -0x3cc0, -0x3ce8, -0x3cec, -0x3cf0, -0x3cf4, -0x3cf8, -0x3cfc, -0x3d00, -0x3d04, -0x3d08, -0x3d0c, -0x3d10, -0x3d14, -0x3d18, -0x3d18,
    -0x3d1c, -0x3d1c, -0x3d20, -0x3d20, -0x3d24, -0x3d24, -0x3d28, -0x3d28, -0x3d2c, -0x3d30, -0x3d34, -0x3d34, -0x3d38, -0x3cc8, -0x3d3c, -0x3d3c,
    -0x3d40, -0x3d44, -0x3d48, -0x3d4c, -0x3d50, -0x3d54, -0x3d58, -0x3d58, -0x3d5c, -0x3d60, -0x3d60, -0x3d64, -0x3d68, -0x3d6c, -0x3d70, -0x3d74,
    -0x3d78, -0x3d78, -0x3d78, -0x3d78, -0x3d78, -0x3d78, -0x3d7c, -0x3d7c, -0x3d80, -0x3d84, -0x3d78, -0x3d78, -0x3d7c, -0x3d88, -0x3d8c, -0x3d90,
    -0x3d78, -0x3d78, -0x3d78, -0x3d78, -0x3d7c, -0x3d7c, -0x3d7c, -0x3d7c, -0x3d94, -0x3d94, -0x3d94, -0x3d94, -0x3d98, -0x3d98, -0x3d98, -0x3d98,
    -0x3d9c, -0x3da0, -0x3da4, -0x3da8, -0x3dac, -0x3db0, -0x3db4, -0x3db8, -0x3dbc, -0x3dc0, -0x3cc8, -0x3d14, -0x3cc8, -0x3cc8, -0x3dc4, -0x3dc8,
    -0x3dcc, -0x3dd0, -0x3dd4, -0x3dd8, -0x3dd8, -0x3ddc, -0x3de0, -0x3de0, -0x3de4, -0x3de8, -0x3dec, -0x3d78, -0x3d78, -0x3df0, -0x3df4};
// The runs filled after them: 33 words of one slot (0x8f..0xaf), 8, 8, 32, 32 (0xb0..0xff).
struct Fill { std::int16_t slot; unsigned count; };
constexpr std::array<Fill, 5> fills = {{{-0x3dd8, 33}, {-0x3df8, 8}, {-0x3dfc, 8}, {-0x3e00, 32}, {-0x3e04, 32}}};
}
void install_opcode_table(Cpu& c, std::uint32_t table) {
    std::uint32_t at = table;
    // The original loads each slot and stores it in the order of its listing; the table's final bytes are what is compared.
    for (const auto slot : table_slots) { c.put32(at, c.toc(slot)); at += 4; }
    for (const auto& fill : fills) {
        const auto descriptor = c.toc(fill.slot);
        for (unsigned n = 0; n < fill.count; ++n) { c.put32(at, descriptor); at += 4; }
    }
    c.set({{3, 0xffffffffu}, {4, 0xffffffffu}, {5, at}, {6, c.toc(-0x3df0)}, {7, c.toc(-0x3dec)}, {8, c.toc(-0x3de8)}, {9, c.toc(-0x3de4)}, {10, c.toc(-0x3ddc)}});
}

// 0x1075a970: the interpreter's state tables (the hint font's constructor calls it with the graphic state): the opcode table in the state's scratch
// block (+0x8c) filled through 0x1075a500, then after it (+0x400) 39 words of unit vectors (2.30 fractions, the table the angle opcode searches)
// and, 0xa0 bytes on, 20 halfwords (their angles); the state's +0xc0 and +0xc4 point at them.
void build_interpreter_tables(Cpu& c, std::uint32_t state_at) {
    Frame frame(c, 0x1075a970, 0x70, {.first_gpr = 20});
    const auto scratch = c.u32(state_at + graphic_state::opcode_table);
    c.set(3, scratch);
    c.link(0x1075a98c);
    install_opcode_table(c, scratch);
    const auto vectors = c.u32(state_at + graphic_state::opcode_table) + 0x400;
    c.put32(state_at + graphic_state::trig_vectors, vectors);
    const auto angles = vectors + 0xa0;
    c.put32(state_at + graphic_state::trig_angles, angles);
    static constexpr std::array<std::uint32_t, 39> unit_vectors = {
        0x00000000, 0x40000000, 0xd2bec333, 0x2d413ccd, 0xe360da3a, 0x393e4b8b, 0xebc2ec9e, 0x3cb73a27, 0xdc7fcac0, 0x35404fe1, 0xf07a4bdc, 0x3e16d092,
        0xd999999a, 0x33333333, 0xf372d54f, 0x3ec1d573, 0xe83b229a, 0x3b6c2980, 0xdf128065, 0x36e12a02, 0xf57a7cac, 0x3f2113fb, 0xd804ff10, 0x31f9c12c,
        0xf6f2f3d7, 0x3f5b551f, 0xee6af813, 0x3d899bbd, 0xe6ca06b6, 0x3ad345ad, 0xd7073803, 0x312a8996, 0xe03f4277, 0x37914baf, 0xf80fd09e, 0x3f817b11,
        0xe987318b, 0x3becd137, 0x3414358e};
    static constexpr std::array<std::uint16_t, 20> angle_values = {
        0x40, 0x5b, 0x8f, 0xca, 0xe7, 0x108, 0x140, 0x146, 0x159, 0x175, 0x185, 0x19a, 0x1c5, 0x1d2, 0x1e7, 0x1f4, 0x204, 0x204, 0x223, 0x227};
    for (unsigned n = 0; n < unit_vectors.size(); ++n) c.put32(vectors + 4 * n, unit_vectors[n]);
    for (unsigned n = 0; n < angle_values.size(); ++n) c.put16(angles + 2 * n, angle_values[n]);
    c.set({{3, 0x3414358e}, {4, vectors}, {5, angles}, {6, 0xdaccfe52}, {7, 0x223}, {8, 0x3becd137}, {9, 0xe987318b}, {10, 0x3f817b11}});
}
}
