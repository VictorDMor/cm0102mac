#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
// One activation owns the mutable action and the two ordered byte outputs.
// Kept values retain the original callee-save image at external boundaries.
class ActionRating {
    Cpu& c;
    Frame frame;
    Fpu fp;
    Player actor;
    const std::uint32_t doubles, floats, seed, begin, end, cursor, strings, message, success;
    Kept<17> draw;
    Kept<18> score;
    Kept<19> holder;
    Kept<24> possession_penalty;
    Kept<27> condition_penalty;
    Kept<28> facing_penalty;
    std::uint32_t at(unsigned offset) const { return frame.at(offset); }
    std::uint32_t action_address() const { return c.r(21); }
    std::uint32_t first_output() const { return c.r(26); }
    std::uint32_t second_output() const { return c.r(25); }
    void advance(std::uint32_t back);
    static std::int32_t high(std::uint32_t value, std::int32_t multiplier) {
        return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * multiplier) >> 32);
    }
    unsigned shifted_sign(std::int32_t value, unsigned bits) {
        c.s.wr<1>(reg::xer_ca, value < 0 && (std::uint32_t(value) & ((1u << bits) - 1)));
        return unsigned(value < 0);
    }
    static std::uint32_t oriented(std::uint32_t word, std::int32_t bound) {
        const auto remainder = word - std::uint32_t(divide_signed<4>(word, std::uint32_t(bound))) * std::uint32_t(bound);
        return bound > 0 ? remainder : 0u - remainder;
    }
    bool own_area();
    void measure_distance();
    void initial_draw();
    void distance_draw();
    std::int32_t distance_bound(unsigned slot);
    struct FitnessBound { std::int32_t value; unsigned sign; };
    FitnessBound fitness_bound(unsigned first_slot, unsigned second_slot, unsigned first_fpr, unsigned second_fpr);
    void fitness_draw();
    void facing_draw();
    void diagnose(bool missing_holder);
    void select_rating();
    void write_outputs();
public:
    ActionRating(Cpu& cpu, std::uint32_t player, std::uint32_t action, std::uint32_t first, std::uint32_t second)
        : c(cpu), frame(c, 0x10270c70, 0x140, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 30}),
          fp(c), actor{c, player}, doubles(c.toc(-0x51e8)), floats(c.toc(-0x51e0)),
          seed(c.toc(-0x7b90)), begin(c.toc(-0x7b8c)), end(c.toc(-0x7b88)), cursor(c.toc(-0x7b84)),
          strings(c.toc(-0x51e4)), message(c.toc(-0x7ba8)), success(c.toc(-0x7bac)),
          draw(c, c.r(17)), score(c), holder(c, c.toc(-0x6424)), possession_penalty(c),
          condition_penalty(c, floats), facing_penalty(c) {
        keep(c, {{31, player}, {26, first}, {25, second}, {21, action}, {13, success}, {14, message},
                 {22, doubles}, {29, seed}, {23, begin}, {20, end}, {30, cursor}, {16, strings}});
        c.set(3, actor.match()); c.set(15, c.u8(c.r(3) + 0x791f));
        c.put8(first, 0); c.put8(second, 0);
    }
    std::uint32_t run();
};

void ActionRating::advance(std::uint32_t back) {
    const auto next = c.u32(cursor) + 2; c.set(3, next);
    const auto order = compare_logical(c, next, c.u32(end)); c.put32(cursor, next);
    if (order > 0) { c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff); }
}

bool ActionRating::own_area() {
    const bool upper = compare(c, actor.side(), 1) == 0; c.set(3, 0);
    bool area = compare(c, actor.column(), 2) >= 0;
    if (area) area = compare(c, actor.column(), 6) <= 0;
    if (area) area = compare(c, actor.row(), upper ? 10 : 0) >= 0;
    if (area) area = compare(c, actor.row(), upper ? 11 : 1) <= 0;
    c.set(3, area); return compare(c, int(area)) != 0;
}

void ActionRating::measure_distance() {
    const bool upper = compare(c, actor.side(), 1) == 0;
    c.set(3, std::uint32_t(int(actor.column()) - 4));
    c.call(0x10775540, upper ? 0x10270dc0 : 0x10270e00);
    draw = extended(std::int8_t(c.r(3)));
    c.set(3, std::uint32_t(int(actor.row()) - (upper ? 11 : 0)));
    c.call(0x10775540, upper ? 0x10270dd4 : 0x10270e14);
    const auto row_delta = std::int8_t(c.r(3));
    c.set(3, std::uint32_t(int(row_delta) * 36));
    c.set_f(30, c.f32(std::uint32_t(holder) + c.r(3) + std::uint32_t(int(std::int8_t(std::uint32_t(draw))) * 4)));
}

void ActionRating::initial_draw() {
    c.set_f(1, c.f32(floats + 0x2c)); c.set_f(0, c.f32(actor.address + 0x78));
    c.put64(at(0xd8), fp.fctiwz(0, fp.fadds(c.f(1), c.f(0)))); draw = c.u32(at(0xdc));
    const auto bound = std::int32_t(std::uint32_t(draw));
    if (compare(c, bound) == 0) { holder = 0; return; }
    advance(0x10270e74); c.set(3, 65535); bool narrow = compare(c, bound, 65535) <= 0;
    if (narrow) { c.set(3, std::uint32_t(-65535)); narrow = compare(c, bound, -65535) >= 0; }
    if (narrow) {
        c.set(3, c.u32(cursor)); compare(c, bound); const auto bias = c.u32(seed);
        c.set(3, std::uint32_t(c.u16(c.r(3))) + bias);
        holder = oriented(c.r(3), bound); return;
    }
    c.set(3, std::uint32_t(bound) ^ 0x80000000); c.put32(at(0xdc), c.r(3));
    const auto pool = c.u32(cursor); c.set(3, pool); c.put32(at(0xd8), 0x43300000);
    c.set_f(2, c.f64(doubles + 0x80)); c.set_f(1, c.f64(at(0xd8))); c.set_f(0, c.f64(doubles));
    c.set_f(1, fp.fsub(c.f(1), c.f(2))); c.set(8, c.u16(pool)); c.set(7, c.u32(seed));
    const auto numerator = std::uint32_t(bound) * 10; c.set(3, numerator); c.put32(at(0xd0), 0x43300000);
    c.set(4, 0x80008001); c.set_f(2, fp.fdiv(c.f(1), c.f(0)));
    const auto word = c.r(8) + c.r(7);
    const auto quotient = std::int32_t(word) / 65535;
    const auto partial = std::int32_t(std::uint32_t(high(word, std::int32_t(0x80008001))) + word);
    c.set(7, shifted_sign(partial, 15)); c.set(6, std::uint32_t(quotient)); c.set(5, std::uint32_t(quotient) * 65535);
    holder = word - c.r(5); c.set_f(1, c.f64(doubles + 0x80));
    c.put32(at(0xd4), std::uint32_t(holder) ^ 0x80000000);
    c.set_f(0, c.f64(at(0xd0))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    shifted_sign(std::int32_t(std::uint32_t(high(numerator, std::int32_t(0x80008001))) + numerator), 15);
    c.set_f(0, fp.fmul(c.f(2), c.f(0))); c.set(3, std::uint32_t(std::int32_t(numerator) / 65535));
    c.put64(at(0xc8), fp.fctiwz(0, c.f(0))); holder = c.u32(at(0xcc));
    enter(c, 0x10270f4c); sample_range(c, c.r(3));
    const auto correction = c.r(3) + 5;
    c.set(4, shifted_sign(high(correction, 0x66666667), 2)); c.set(3, std::uint32_t(std::int32_t(correction) / 10));
    holder = std::uint32_t(holder) + c.r(3);
    if (compare(c, std::int32_t(std::uint32_t(holder)), std::int32_t(std::uint32_t(bound) - 1)) > 0) holder = std::uint32_t(bound) - 1;
}

std::int32_t ActionRating::distance_bound(unsigned slot) {
    c.set_f(0, c.f32(floats + 0x2c)); c.set_f(0, fp.fmuls(c.f(0), c.f(30)));
    c.put64(at(slot), fp.fctiwz(0, c.f(0))); return c.i32(at(slot + 4));
}

void ActionRating::distance_draw() {
    c.set_f(1, c.f32(floats + 0x2c)); c.set_f(0, c.f32(floats));
    c.set_f(1, fp.fmuls(c.f(1), c.f(30))); fp.fcmp(0, c.f(1), c.f(0));
    if (c.cr(0) & 2) { draw = 0; return; }
    advance(0x10270fec); c.set(3, 65535);
    bool narrow = compare(c, distance_bound(0xc8), 65535) <= 0;
    if (narrow) { c.set(3, std::uint32_t(-65535)); narrow = compare(c, distance_bound(0xc8), -65535) >= 0; }
    if (narrow) {
        c.set_f(0, c.f32(floats + 0x2c)); c.set(3, c.u32(cursor)); c.set_f(1, fp.fmuls(c.f(0), c.f(30)));
        c.set_f(0, c.f32(floats + 0x2c)); c.set(3, c.u16(c.r(3))); const auto word = c.r(3) + c.u32(seed);
        c.set_f(0, fp.fmuls(c.f(0), c.f(30))); c.put64(at(0xa8), fp.fctiwz(1, c.f(1))); c.set(5, word);
        c.put64(at(0xb0), fp.fctiwz(0, c.f(0))); c.set(4, c.u32(at(0xac)));
        // The floating zero guard precedes integer conversion. At 0x102711a4
        // the reference lowers a zero divisor to a dead zero quotient: its
        // only product is by that same divisor before r3 is overwritten.
        c.set(3, c.r(4) ? divide_signed<4>(word, c.r(4)) : 0);
        const auto sign_bound = c.i32(at(0xb4)); compare(c, sign_bound);
        const auto remainder = word - c.r(3) * c.r(4); draw = sign_bound > 0 ? remainder : 0u - remainder;
        return;
    }
    c.set_f(0, c.f32(floats + 0x2c)); const auto pool = c.u32(cursor); c.set(3, pool);
    c.set_f(1, fp.fmuls(c.f(0), c.f(30)));
    c.set_f(0, c.f32(floats + 0x2c)); c.put32(at(0xd0), 0x43300000); c.set_f(0, fp.fmuls(c.f(0), c.f(30)));
    const auto first_bits = fp.fctiwz(1, c.f(1)), second_bits = fp.fctiwz(0, c.f(0));
    c.set(8, c.u16(pool)); const auto word = c.r(8) + c.u32(seed);
    c.put64(at(0xc8), first_bits); c.set_f(3, c.f64(doubles + 0x80));
    c.set(3, c.u32(at(0xcc)) ^ 0x80000000); c.set(6, 65535); c.put32(at(0xd8), 0x43300000);
    c.set(4, 0x80008001); c.put32(at(0xd4), c.r(3)); c.set_f(2, c.f64(doubles));
    const auto partial = std::int32_t(std::uint32_t(high(word, std::int32_t(0x80008001))) + word);
    c.set(7, shifted_sign(partial, 15)); c.put64(at(0xb8), second_bits);
    c.set_f(0, c.f64(at(0xd0))); c.set(5, std::uint32_t(std::int32_t(word) / 65535) * 65535);
    c.set_f(1, c.f64(doubles + 0x80)); c.set_f(0, fp.fsub(c.f(0), c.f(3)));
    c.set(3, c.u32(at(0xbc))); c.set_f(2, fp.fdiv(c.f(0), c.f(2)));
    draw = word - c.r(5); c.put32(at(0xdc), std::uint32_t(draw) ^ 0x80000000);
    const auto numerator = c.r(3) * 10;
    c.set_f(0, c.f64(at(0xd8))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    shifted_sign(std::int32_t(std::uint32_t(high(numerator, std::int32_t(0x80008001))) + numerator), 15);
    c.set(3, std::uint32_t(std::int32_t(numerator) / 65535)); c.put64(at(0xc0), fp.fctiwz(0, c.f(0))); draw = c.u32(at(0xc4));
    enter(c, 0x10271114); sample_range(c, c.r(3));
    const auto correction = c.r(3) + 5;
    c.set(4, shifted_sign(high(correction, 0x66666667), 2));
    const auto bound = distance_bound(0xb0); c.set(3, std::uint32_t(bound));
    draw = std::uint32_t(draw) + std::uint32_t(std::int32_t(correction) / 10);
    if (compare(c, std::int32_t(std::uint32_t(draw)), std::int32_t(std::uint32_t(bound) - 1)) > 0) {
        c.set(3, std::uint32_t(distance_bound(0xa8))); draw = c.r(3) - 1;
    }
}

ActionRating::FitnessBound ActionRating::fitness_bound(unsigned first_slot, unsigned second_slot, unsigned first_fpr, unsigned second_fpr) {
    const auto first = fp.fctiwz(first_fpr, c.f(31));
    const auto second = fp.fctiwz(second_fpr, c.f(30));
    c.put64(at(first_slot), first); c.put64(at(second_slot), second);
    const auto a = std::uint32_t(10000 - std::int16_t(c.u32(at(first_slot + 4))));
    const auto b = std::uint32_t(10000 - std::int16_t(c.u32(at(second_slot + 4))));
    const auto product = a * b;
    const auto sign = shifted_sign(high(product, 0x14f8b589), 12);
    return {std::int32_t(product) / 50000, sign};
}

void ActionRating::fitness_draw() {
    c.set_f(31, c.f32(actor.address + 0x50)); c.set(4, 0x14f8b589); c.set_f(30, c.f32(actor.address + 0x50));
    const auto initial = fitness_bound(0xa8, 0xb0, 1, 0);
    c.set(3, initial.sign);
    if (compare(c, initial.value) == 0) { draw = 0; return; }
    advance(0x10271310);
    c.set(5, 0x14f8b589); c.set(3, 65535);
    auto bound = fitness_bound(0xa8, 0xb0, 1, 0); c.set(4, bound.sign); compare(c, bound.value, 65535);
    c.set(5, 0x14f8b589); c.set(3, std::uint32_t(-65535));
    bound = fitness_bound(0xa8, 0xb0, 1, 0); c.set(4, bound.sign); compare(c, bound.value, -65535);
    // Each converted condition narrows to int16. Even an arbitrary signed
    // product / 50000 is within [-42949,42949], so both wide arms are dead.
    const auto first_bits = fp.fctiwz(0, c.f(31)); c.set(4, c.u32(cursor));
    const auto second_bits = fp.fctiwz(2, c.f(30)); c.set(3, 0x14f8b589);
    const auto repeated_first = fp.fctiwz(1, c.f(31)); c.set(6, 0x14f8b589);
    c.put64(at(0x88), first_bits); const auto repeated_second = fp.fctiwz(0, c.f(30));
    c.set(8, c.u16(c.r(4))); c.put64(at(0x90), second_bits);
    const auto a = std::uint32_t(10000 - std::int16_t(c.u32(at(0x8c))));
    const auto b = std::uint32_t(10000 - std::int16_t(c.u32(at(0x94))));
    c.set(4, a); c.set(5, c.u32(seed));
    c.put64(at(0x98), repeated_first); c.put64(at(0xa0), repeated_second);
    const auto word = c.r(8) + c.r(5), product = a * b;
    const auto first_high = high(product, 0x14f8b589);
    c.set(8, shifted_sign(first_high, 12)); c.set(7, std::uint32_t(std::int32_t(product) / 50000));
    const auto repeated_a = std::uint32_t(10000 - std::int16_t(c.u32(at(0x9c))));
    const auto repeated_b = std::uint32_t(10000 - std::int16_t(c.u32(at(0xa4))));
    c.set(5, std::uint32_t(divide_signed<4>(word, c.r(7))) * c.r(7));
    const auto second_product = repeated_a * repeated_b;
    c.set(4, shifted_sign(high(second_product, 0x14f8b589), 12));
    c.set(3, std::uint32_t(std::int32_t(second_product) / 50000) - c.r(4));
    draw = word - c.r(5);
    if (compare(c, std::int32_t(c.r(3) + c.r(4))) <= 0) draw = 0u - std::uint32_t(draw);
}

void ActionRating::facing_draw() {
    c.set(3, actor.match()); c.set(3, std::uint32_t(int(c.i16(actor.address + 0x19e)) - c.i16(c.r(3) + 0x7950)));
    c.call(0x10775540, 0x10271688);
    auto difference = std::int16_t(c.r(3));
    if (compare(c, difference, 180) > 0) difference = std::int16_t(360 - difference);
    c.set(3, actor.match()); c.set(3, c.u8(c.r(3) + 0x791f));
    facing_penalty = extended(std::int16_t(360 - 2 * int(difference)));
    auto divisor = 5 - int(std::int8_t(c.r(3)));
    if (compare(c, divisor, 1) < 0) divisor = 1;
    else { c.set(3, actor.match()); divisor = 5 - int(c.i8(c.r(3) + 0x791f)); }
    c.set(3, extended(std::int16_t(std::uint32_t(facing_penalty))));
    facing_penalty = extended(std::int16_t(divide_signed<4>(c.r(3), extended(std::int16_t(divisor)))));
    const auto bound = std::int16_t(std::uint32_t(facing_penalty));
    if (compare(c, bound) == 0) { draw = 0; }
    else {
        advance(0x10271718); c.set(3, 65535); compare(c, bound, 65535);
        c.set(3, std::uint32_t(-65535)); compare(c, bound, -65535);
        // A signed-halfword facing bound cannot enter either wide arm.
        c.set(5, c.u32(cursor)); c.set(3, extended(bound)); c.set(4, c.u32(seed)); compare(c, bound);
        c.set(4, std::uint32_t(c.u16(c.r(5))) + c.r(4)); draw = oriented(c.r(4), bound);
    }
    facing_penalty = extended(std::int16_t(std::uint32_t(draw)));
}

void ActionRating::diagnose(bool missing_holder) {
    const auto name_slot = missing_holder ? 0x68u : 0x48u, line_slot = missing_holder ? 0x40u : 0x38u;
    leave(c, {{3, strings + 0x8f}, {4, 0}, {5, 0}, {6, at(name_slot)}, {7, at(line_slot)}});
    c.call(0x10739890, missing_holder ? 0x10271240 : 0x10271fec);
    c.call(0x10768810, missing_holder ? 0x10271244 : 0x10271ff0);
    leave(c, {{5, c.r(3)}, {3, message}, {4, strings + 0x9c}, {6, at(name_slot)}, {7, at(line_slot)}, {8, missing_holder ? 0x4573u : 0x4645u}});
    c.call(0x1077b720, missing_holder ? 0x10271260 : 0x1027200c);
    leave(c, {{4, message}, {3, strings + 0xa9}, {5, 0}});
    c.call(0x10769b60, missing_holder ? 0x10271270 : 0x1027201c);
    if (missing_holder) c.set(3, std::uint32_t(-1));
    c.put8(success, 0);
    if (missing_holder) c.call(0x1076f7d0, 0x10271280);
}

void ActionRating::select_rating() {
    c.set(5, actor.match()); c.set(4, actor.address); c.set(3, actor.match()); c.set(5, c.u8(c.r(5) + 0x7920));
    const auto delay = std::int8_t(c.r(5) - 1); c.set(5, extended(delay));
    draw = extended(std::int16_t(20 * int(delay) * int(delay) * int(delay)));
    c.call(0x101e3d50, 0x102718a0);
    if (compare(c, std::int16_t(c.r(3)), 1) == 0 && own_area()) {
        const auto action = compare(c, std::int8_t(c.r(15)), 4) == 0 ? 1u : 41u;
        c.put8(action_address(), action);
        const auto live_action = c.i8(action_address());
        c.set(3, actor.address + std::uint32_t(int(live_action) * 2));
        score = extended(std::int16_t(std::uint32_t(c.i16(c.r(3) + 0x11e)) - std::uint32_t(possession_penalty) - std::uint32_t(condition_penalty)));
        return;
    }
    c.set(3, extended(c.i8(action_address()))); const auto action = std::int32_t(c.r(3));
    const auto index = c.r(3) - 12;
    if (compare_logical(c, index, 38) > 0) return;
    c.set(3, c.u32(c.toc(-0x5244) + index * 4)); c.s.wr<4>(reg::ctr, c.r(3));
    unsigned offset = 0, penalties = 2; std::uint32_t fallback_back = 0;
    switch (action) {
    case 12: offset = 0x136; penalties = 4; break;
    case 19: offset = 0x144; fallback_back = 0x10271b18; break;
    case 21: offset = 0x148; penalties = 4; break;
    case 33: offset = 0x160; fallback_back = 0x10271a98; break;
    case 34: offset = 0x162; fallback_back = 0x10271a18; break;
    case 37: offset = 0x168; penalties = 1; break;
    case 42: offset = 0x172; break;
    case 43: offset = 0x174; break;
    case 44: offset = 0x176; break;
    case 45: offset = 0x178; break;
    case 46: offset = 0x17a; break;
    case 47: offset = 0x17c; break;
    case 49: offset = 0x180; penalties = 1; break;
    case 50: offset = 0x182; penalties = 1; break;
    default: return;
    }
    auto value = std::uint32_t(c.i16(actor.address + offset)) - std::uint32_t(possession_penalty);
    if (penalties >= 2) value -= std::uint32_t(condition_penalty);
    if (penalties == 4) value -= std::uint32_t(draw) + std::uint32_t(facing_penalty);
    score = extended(std::int16_t(value));
    if (!fallback_back || compare(c, std::int32_t(std::uint32_t(score)), 2) >= 0) return;
    advance(fallback_back); c.set(3, c.u32(cursor)); const auto bias = c.u32(seed);
    c.set(3, c.u16(c.r(3))); const auto word = c.r(3) + bias;
    c.set(3, word >> 31); const auto remainder = std::int32_t(word) % 2;
    if (compare(c, remainder) == 0) return;
    c.set(3, std::uint32_t(c.i16(actor.address + 0x178)) - std::uint32_t(possession_penalty));
    c.put8(action_address(), 45); score = extended(std::int16_t(c.r(3) - std::uint32_t(condition_penalty)));
}

void ActionRating::write_outputs() {
    const auto action = c.i8(action_address());
    if (compare_logical(c, std::uint32_t(std::int32_t(action)), 50) <= 0) {
        c.set(3, c.u32(c.toc(-0x5248) + unsigned(action) * 4)); c.s.wr<4>(reg::ctr, c.r(3));
        switch (action) {
        case 1: case 41: {
            advance(action == 1 ? 0x10271cc0 : 0x10271c64);
            c.set(4, c.u32(cursor)); c.set(3, c.u32(seed)); c.set(4, c.u16(c.r(4))); c.set(3, c.r(4) + c.r(3));
            const auto word = c.r(3); c.set(4, word >> 31); c.set(3, std::uint32_t(std::int32_t(word) % 2) + 2);
            c.put8(second_output(), c.r(3)); c.put8(first_output(), 1); return;
        }
        case 12: c.set(3, 0); c.put8(second_output(), 0); c.put8(first_output(), 0); return;
        case 19: case 33: case 45: c.set(3, 2); c.put8(second_output(), 2); c.put8(first_output(), 2); return;
        case 21: c.set(3, 2); c.put8(second_output(), 2); c.put8(first_output(), 1); return;
        case 36: case 37: case 43: c.set(3, 2); c.put8(second_output(), 2); c.put8(first_output(), 0); return;
        case 44: c.set(3, 2); c.put8(second_output(), 2); c.put8(first_output(), 5); return;
        case 34: case 42: case 46: case 47: case 49: case 50: {
            c.put8(second_output(), action == 49 ? 0 : 2);
            const auto back = action == 34 ? 0x10271d4cu : action == 42 ? 0x10271de4u : action == 46 ? 0x10271e50u :
                              action == 47 ? 0x10271ea8u : action == 49 ? 0x10271f08u : 0x10271f68u;
            advance(back); c.set(3, c.u32(cursor)); const auto bias = c.u32(seed);
            c.set(3, c.u16(c.r(3))); const auto word = c.r(3) + bias;
            const bool four = action == 47 || action == 49;
            const auto remainder = std::int32_t(word) % (four ? 4 : 2);
            c.set(3, four || action == 34 || action == 50 ? std::uint32_t(remainder) : word >> 31);
            c.put8(first_output(), std::uint32_t(remainder) + (action == 42 || action == 46 ? 0 : 1)); return;
        }
        default: break;
        }
    }
    c.set(3, 0); c.put8(second_output(), 0); c.put8(first_output(), 0); diagnose(false);
}

std::uint32_t ActionRating::run() {
    bool skip_geometry = compare(c, c.u8(action_address()), 37) == 0;
    if (!skip_geometry) skip_geometry = compare(c, c.u8(action_address()), 21) == 0;
    if (!skip_geometry) skip_geometry = own_area();
    if (!skip_geometry) {
        measure_distance(); initial_draw(); distance_draw();
        if (compare(c, std::int32_t(std::uint32_t(holder)), std::int32_t(std::uint32_t(draw))) < 0) {
            c.set(3, 0); c.put8(second_output(), 0); c.set(3, std::uint32_t(-20)); c.put8(first_output(), 0); return c.r(3);
        }
    }
    c.set(3, actor.match() + 0x10000); holder = c.u32(c.r(3) - 0x2000);
    if (compare(c, std::int32_t(std::uint32_t(holder))) == 0 &&
        compare(c, c.u8(action_address()), 21) != 0 && compare(c, c.u8(action_address()), 37) != 0) {
        c.set(3, actor.match()); c.set(4, c.u8(action_address())); c.call(0x101ee530, 0x10271220);
        if (compare(c, std::int8_t(c.r(3))) == 0) diagnose(true);
        else if (compare_logical(c, holder, actor.address) == 0) possession_penalty = extended(c.i16(std::uint32_t(holder) + 0x1a0));
        else possession_penalty = 0;
    } else if (compare_logical(c, holder, actor.address) == 0) possession_penalty = extended(c.i16(std::uint32_t(holder) + 0x1a0));
    else possession_penalty = 0;
    fitness_draw(); const auto holder_order = compare_logical(c, holder);
    condition_penalty = extended(std::int16_t(std::uint32_t(draw)));
    if (!holder_order) facing_draw();
    select_rating(); write_outputs(); return result(c, score);
}
}

std::uint32_t rate_action(Cpu& c, std::uint32_t player, std::uint32_t action,
                          std::uint32_t first_output, std::uint32_t second_output) {
    ActionRating rating(c, player, action, first_output, second_output);
    return rating.run();
}
}
