// 0x10661a50: the figure the press puts on a story about a person.
#include "media_values_detail.hpp"

namespace cm::aot::media {
using namespace detail;

// Starts from the world's two ratios and the person's three sample ratings (0x10303560, 0x10303090),
// scales it by how far it falls below the cutoff and by the contract's rating over the club's (the club's
// clamped to 2000-7500, the contract's to at least 2000), then adjusts it for the story kind. `scale`
// arrives in f1 and the figure is returned there; a standing record that cannot be found gives the
// pool's constant.
void story_value(Cpu& c, double scale, std::uint32_t person, std::uint32_t club, std::uint32_t standing, std::uint32_t kind_register,
                 std::uint32_t grade_register) {
    Frame frame(c, 0x10661a50, 0xa0, {.first_gpr = 24, .first_fpr = 26});
    Fpu fpu(c);
    const auto singles = c.toc(-0x4200), doubles = c.toc(-0x41fc);
    keep(c, {{24, c.toc(-0x7b3c)}, {25, person}, {26, club}, {27, standing}, {28, kind_register}, {29, grade_register}, {30, singles}, {31, doubles}});
    c.set_f(26, scale);
    const auto single = [&](std::uint32_t offset) { return double(c.f32(singles + offset)); };
    const auto pooled = [&](std::uint32_t offset) { return c.f64(doubles + offset); };
    compare(c, std::int32_t(standing));
    if (standing == 0) c.set(27, Program{c}.call(0x10082f90, 0x10661aa4, {c.u32(c.toc(-0x7b7c)), person, club}));
    if (!nonzero_logical(c, c.r(27))) {
        c.set_f(1, single(8));
        return;
    }
    const auto world = [&] { return c.u32(c.r(24)); };
    // f31: the world's ratio of two doubles, or the pool's constant when the divisor equals the pool's marker.
    fpu.fcmp(0, c.f64(world() + 0x18), pooled(0x28));
    const double ratio = (c.cr(0) & 2) ? pooled(0x20) : fpu.fdiv(c.f64(world() + 0x10), c.f64(world() + 0x18));
    const double f31 = fpu.frsp(ratio);
    c.set_f(31, f31);
    const auto sample = [&](std::uint32_t which, std::uint32_t back) {
        c.set_f(1, single(0));
        Program{c}.call(0x10303560, back, {world(), c.u32(person + 0x14), 6, which});
        return c.f(1);
    };
    const double f30 = sample(1, 0x10661afc);
    c.set_f(30, f30);
    const double f29 = sample(0x11, 0x10661b1c);
    c.set_f(29, f29);
    const auto grade = byte_word(Program{c}.call(0x10303090, 0x10661b2c, {world(), c.u32(person + 0x14)}));
    const double f28 = fpu.frsp(fpu.fdiv(fpu.fsub(stacked_integer(c, frame.at(0x40), signed_word(grade)), pooled(8)), pooled(0x30)));
    c.set_f(28, f28);
    const double fourth = sample(2, 0x10661b70);
    fpu.fcmp(0, f30, pooled(0x38));
    const double ceiling = (c.cr(0) & 4) ? f30 : pooled(0x38);
    double share = fpu.frsp(ceiling);
    const double over = fpu.fsubs(f28, f31);
    const double cost = fpu.fdivs(scale, single(0x10));
    const double under = fpu.fsubs(f29, f31);
    share = fpu.fdivs(fourth, share);
    const double sum = fpu.fadds(over, under);
    fpu.fcmp(0, f30, single(0x14));
    double value = fpu.fsubs(sum, cost);
    if (c.cr(0) & 8) value = fpu.fmuls(value, fpu.fdivs(f30, single(0x14)));
    // The contract's rating over the club's: ratings through the int-to-double trick, in stack pairs at
    // `contract_at` and `club_at`, which the scaling at kind 17 swaps. r3-r5 keep the last words built.
    const auto rating_ratio = [&](std::uint32_t contract_at, std::uint32_t club_at) {
        const std::int32_t club_rating = std::clamp<std::int32_t>(c.i16(club + 0x1ec), 2000, 7500);
        const std::int32_t contract_rating = std::max<std::int32_t>(c.i16(c.u32(person + 0x14) + 0xa), 2000);
        leave(c, {{3, signed_word(club_rating)}, {4, 0x43300000}, {5, signed_word(contract_rating)}});
        const double bias = pooled(8);
        const double top = fpu.fsubs(stacked_integer(c, contract_at, signed_word(contract_rating)), bias);
        const double bottom = fpu.fsubs(stacked_integer(c, club_at, signed_word(club_rating)), bias);
        return fpu.fdivs(top, bottom);
    };
    fpu.fcmp(0, value, pooled(0x28));
    if (c.cr(0) & 8) value = fpu.fmuls(value, rating_ratio(frame.at(0x40), frame.at(0x38)));
    // value + (integer) / divisor, the integer through the int-to-double pair at sp+0x38.
    const auto add_scaled = [&](std::int32_t integer, std::uint32_t divisor) {
        const double bias = pooled(8);
        const double top = fpu.fsubs(stacked_integer(c, frame.at(0x38), signed_word(integer)), bias);
        value = fpu.fadds(value, fpu.fdivs(top, single(divisor)));
    };
    const auto state = [&] { return std::int32_t(c.u8(c.r(27) + 0x60) & 0xf); };
    // The compiler's compare trees over the state, each compare into cr0 as it makes them. Shape A
    // (kinds 0, 3, 17) tells 0, 1, 2, 3 and 4 or more apart.
    const auto state_a = [&](std::int32_t v) {
        if (const auto order = compare(c, v, 2); order == 0) return 2;
        else if (order > 0) return compare(c, v, 4) >= 0 ? 4 : 3;
        return compare(c, v, 1) >= 0 ? 1 : 0;
    };
    const auto subtract_step = [&](std::int32_t step) {
        if (step == 1) value = fpu.fsubs(value, single(0x24));
        else if (step == 2) value = fpu.fsubs(value, single(0x28));
        else if (step == 3) value = fpu.fsubs(value, single(0x2c));
    };
    const auto add_step = [&](std::int32_t step) {
        if (step == 1) value = fpu.fadds(value, single(0x34));
        else if (step == 2) value = fpu.fadds(value, single(0x38));
        else if (step == 3) value = fpu.fadds(value, single(0x3c));
    };
    // The compare tree over the kind, as the compiler built it; the first compare is against 12.
    enum class Kind { none, reachable, nearness, standing, age, form, trend, morale, shift, tough, scaled, linear, special };
    const auto kind = half_word(kind_register);
    const auto classify = [&]() {
        const auto against = [&](std::int32_t k) { return compare(c, kind, k); };
        if (const auto top = against(0xc); top == 0) return Kind::morale;
        else if (top > 0) {
            if (const auto order = against(0x1e); order == 0) return Kind::tough;
            else if (order > 0) {
                if (const auto far = against(0x52); far == 0) return Kind::linear;
                else if (far > 0) return Kind::none;
                return against(0x23) == 0 ? Kind::special : Kind::none;
            }
            if (const auto middle = against(0x11); middle == 0) return Kind::scaled;
            else if (middle > 0) return Kind::none;
            return against(0xe) >= 0 ? Kind::none : Kind::shift;
        }
        if (const auto order = against(5); order == 0) return Kind::none;
        else if (order > 0) {
            if (const auto high = against(8); high == 0) return Kind::none;
            else if (high > 0) return against(0xa) >= 0 ? Kind::none : Kind::reachable;
            return against(7) >= 0 ? Kind::trend : Kind::form;
        }
        if (const auto low = against(3); low == 0) return Kind::standing;
        else if (low > 0) return Kind::age;
        if (const auto zero = against(0); zero == 0) return Kind::nearness;
        else if (zero > 0) return Kind::reachable;
        return Kind::none;
    };
    switch (classify()) {
    case Kind::nearness: {
        const auto step = state_a(state());
        if (step == 1 || step == 2 || step == 3) subtract_step(step);
        else value = fpu.fadds(value, single(0x30));
        break;
    }
    case Kind::reachable: {
        c.set_f(27, value);
        const auto distance = Program{c}.call(0x10718130, 0x10661d28, {c.toc(-0x7b80), c.u32(person + 0x4c), c.u32(person + 0x50)});
        add_scaled(std::int32_t(0xc8u - distance), 0x20);
        break;
    }
    case Kind::standing:
        value = fpu.fadds(value, fpu.fsubs(f29, f31));
        add_step(state_a(state()));
        break;
    case Kind::age: {
        const auto rating = c.i16(person + 0x5c);
        c.set(3, extended(rating));
        add_scaled(35 - byte_word(std::uint32_t(rating)), 0x40);
        break;
    }
    case Kind::form: {
        const auto contract = c.u32(person + 0x14);
        c.set(3, contract);
        add_scaled(10 - byte_word(c.u8(contract + 0x25)), 0xc);
        break;
    }
    case Kind::trend: {
        const auto rating = byte_word(std::uint32_t(c.i16(person + 0x5c)));
        c.set(3, std::uint32_t(rating));
        add_scaled(rating - 0x13, 0x40);
        break;
    }
    case Kind::morale: {
        const auto contract = c.u32(person + 0x14);
        const auto morale = byte_word(c.u8(contract + 0x2f));
        c.set(3, std::uint32_t(morale));
        add_scaled(morale - 0xa, 0x40);
        break;
    }
    case Kind::shift: value = fpu.fadds(value, fpu.fsubs(f28, f31)); break;
    case Kind::scaled: {
        fpu.fcmp(0, value, pooled(0x28));
        if (c.cr(0) & 4) value = fpu.fdivs(value, rating_ratio(frame.at(0x38), frame.at(0x40)));
        const auto step = state_a(state());
        if (step < 4) subtract_step(step);
        break;
    }
    case Kind::tough: value = fpu.fsubs(value, fpu.fdivs(scale, single(0x20))); break;
    case Kind::special: {
        // Shape B: 3, then 7 or other above it; 1, 2 and 0 below.
        const auto v = state();
        if (const auto order = compare(c, v, 3); order == 0) value = fpu.fadds(value, single(0x3c));
        else if (order > 0) {
            if (compare(c, v, 7) == 0) {
                if (compare(c, byte_word(c.r(29)), 3) == 0) value = single(8);
                else value = fpu.fsubs(value, single(0x40));
            }
        } else if (const auto low = compare(c, v, 1); low == 0) value = fpu.fadds(value, single(0x34));
        else if (low > 0) value = fpu.fadds(value, single(0x38));
        else if (compare(c, v, 0) >= 0) value = fpu.fsubs(value, single(0x3c));
        break;
    }
    case Kind::linear: {
        const double slope = single(0x18), offset = single(0x1c);
        value = fpu.fsubs(fpu.multiply_add(0x10661d0c, slope, value, share), offset);
        break;
    }
    case Kind::none: break;
    }
    c.set_f(1, value);
}
}
