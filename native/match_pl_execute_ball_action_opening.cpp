#include "match_pl_execute_ball_action_detail.hpp"
#include "match_pl_adapter.hpp"

namespace cm::aot::match_pl::ball_action_detail {
using namespace guest;

constexpr std::uint32_t interval_multiplier = 0x14f8b589;

static std::uint32_t deficit(Cpu& c, std::uint32_t bits) {
    const auto fitness = std::uint32_t(std::int16_t(bits));
    c.s.wr<1>(reg::xer_ca, fitness <= 10000);
    return 10000 - fitness;
}

static std::uint32_t scaled_product(Cpu& c, std::uint32_t product) {
    const auto high = std::int32_t((std::int64_t(std::int32_t(product)) * interval_multiplier) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 0xfff) != 0);
    return std::uint32_t(high >> 12);
}

std::uint32_t truncated_interval(std::uint32_t shifted) {
    return shifted + (shifted >> 31);
}

static void fitness_pair(Cpu& c, Fpu& fp, unsigned first, unsigned second) {
    const auto first_condition = fp.fctiwz(1, c.f(31));
    const auto second_condition = fp.fctiwz(0, c.f(30));
    c.put64(c.sp() + first, first_condition);
    c.put64(c.sp() + second, second_condition);
}

static std::uint32_t fitness_interval(Cpu& c, unsigned first, unsigned second, unsigned operand) {
    c.set(operand, c.u32(c.sp() + first + 4));
    c.set(0, c.u32(c.sp() + second + 4));
    c.set(operand, deficit(c, c.r(operand)));
    c.set(0, deficit(c, c.r(0)));
    return scaled_product(c, c.r(operand) * c.r(0));
}

static void compare_fitness_bound(Cpu& c, Fpu& fp, std::int32_t bound) {
    const auto first = fp.fctiwz(1, c.f(31));
    c.set(5, 0x14f90000);
    const auto second = fp.fctiwz(0, c.f(30));
    c.set(3, bound > 0 ? 0x10000u : 0xffff0000u);
    c.set(5, interval_multiplier);
    c.put64(c.sp() + 0xc0, first);
    c.set(3, std::uint32_t(bound));
    c.put64(c.sp() + 0xc8, second);
    const auto shifted = fitness_interval(c, 0xc0, 0xc8, 4);
    c.set(4, shifted >> 31);
    c.set(0, truncated_interval(shifted));
    c.compare(0, std::int32_t(c.r(0)), bound);
}

void advance_stream(Cpu& c, std::uint32_t back) {
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(29)));
    c.set(3, c.r(3) + 2);
    c.compare_logical(0, c.r(3), c.r(0));
    c.put32(c.r(30), c.r(3));
    if ((c.cr(0) & 4) == 0) return;
    c.put32(c.r(30), c.r(28));
    c.call(0x1077b770, back);
    c.set(0, c.r(3) & 0xffff);
    c.put32(c.r(27), c.r(0));
}

static void sample_fitness_remainder(Cpu& c, Fpu& fp) {
    const auto stack = c.sp();
    const auto first = fp.fctiwz(0, c.f(31));
    c.set(3, c.u32(c.r(30)));
    const auto second = fp.fctiwz(2, c.f(30));
    c.set(7, 0x14f90000);
    const auto repeated_first = fp.fctiwz(1, c.f(31));
    c.set(5, 0x14f90000);
    c.put64(stack + 0x78, first);
    const auto repeated_second = fp.fctiwz(0, c.f(30));
    c.set(8, c.u16(c.r(3)));
    c.set(7, interval_multiplier);
    c.put64(stack + 0x80, second);
    c.set(5, interval_multiplier);
    c.set(3, c.u32(stack + 0x7c));
    c.set(0, c.u32(stack + 0x84));
    const auto first_deficit = deficit(c, c.r(3));
    c.set(3, first_deficit);
    c.set(4, c.u32(c.r(27)));
    c.set(0, deficit(c, c.r(0)));
    c.put64(stack + 0x88, repeated_first);
    c.set(6, first_deficit * c.r(0));
    c.set(0, c.r(8) + c.r(4));
    c.put64(stack + 0x90, repeated_second);
    c.set(4, c.u32(stack + 0x8c));
    c.set(3, c.u32(stack + 0x94));
    const auto shifted = scaled_product(c, c.r(6));
    c.set(4, deficit(c, c.r(4)));
    c.set(7, truncated_interval(shifted));
    c.set(3, deficit(c, c.r(3)));
    c.set(6, std::uint32_t(divide_signed<4>(c.r(0), c.r(7))));
    const auto repeated_product = c.r(4) * c.r(3);
    c.set(5, c.r(6) * c.r(7));
    c.set(3, scaled_product(c, repeated_product));
    c.set(4, c.r(3) >> 31);
    c.set(13, c.r(0) - c.r(5));
    c.compare(0, std::int32_t(c.r(3) + c.r(4)), 0);
    c.set(0, (c.cr(0) & 4) != 0 ? c.r(13) : 0u - c.r(13));
    c.set(13, c.r(0));
}

static void sample_fitness(Cpu& c) {
    Fpu fp(c);
    c.set_f(31, c.f32(c.r(15) + 0x50));
    c.set(4, 0x14f90000);
    c.set_f(30, c.f32(c.r(15) + 0x50));
    c.set(4, interval_multiplier);
    fitness_pair(c, fp, 0xc8, 0xc0);
    const auto shifted = fitness_interval(c, 0xc8, 0xc0, 3);
    c.set(3, shifted >> 31);
    c.set(0, truncated_interval(shifted));
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) {
        c.set(13, 0);
        return;
    }

    advance_stream(c, 0x1023ce2c);
    // The largest possible signed-halfword deficit product is 42,768
    // squared. Its scaled interval lies within both original bounds.
    compare_fitness_bound(c, fp, 65535);
    compare_fitness_bound(c, fp, -65535);
    sample_fitness_remainder(c, fp);
}

void prepare(Cpu& c, GuestNativeScope& scope) {
    const auto caller = c.sp();
    c.set(0, c.lr());
    c.put64(caller - 8, c.s.r<8>(reg::fpr(31)));
    c.put64(caller - 0x10, c.s.r<8>(reg::fpr(30)));
    c.put64(caller - 0x18, c.s.r<8>(reg::fpr(29)));
    auto saved = caller - 0x6c;
    for (unsigned reg = 13; reg != 32; ++reg, saved += 4) {
        c.s.wr<4>(2432, saved);
        c.put32(saved, c.r(reg));
    }
    c.s.wr<4>(2432, saved);
    c.set(15, c.r(3));
    c.set(26, c.toc(-0x51e8));
    c.set(16, c.r(4));
    c.set(27, c.toc(-0x7b90));
    c.set(17, c.r(5));
    c.set(28, c.toc(-0x7b8c));
    c.set(18, c.r(6));
    c.set(29, c.toc(-0x7b88));
    c.set(23, 0);
    c.set(30, c.toc(-0x7b84));
    c.set(31, c.toc(-0x51e0));
    c.put32(caller + 8, c.r(0));
    c.set(0, 0);
    const auto stack = caller - 0x150;
    c.put32(stack, caller);
    c.set(1, stack);
    scope.frame(stack);
    c.put8(stack + 0xd0, 0);
    call_owned(c, 0x10245160, 0x1023cd74);

    c.set(0, 0);
    c.set(4, 0);
    c.put16(stack + 0x4a, 0);
    c.set(0, std::uint32_t(std::int8_t(c.r(17))));
    c.set(3, 0);
    c.s.wr<8>(reg::fpr(29), c.s.r<8>(reg::fpr(1)));
    c.put8(stack + 0x48, 0);
    c.compare(0, std::int32_t(c.r(0)), -1);
    c.s.pc = 0x1023cd94;
    const auto owner_slot = c.r(15) + 0x1a4;
    c.set(4, c.u32(owner_slot));
    c.set(22, c.u8(c.r(4) + 0x791f));
    c.put8(stack + 0x42, 0);
    if (c.cr(0) & 2) {
        c.set(3, c.u32(owner_slot));
        c.set(4, c.u32(owner_slot));
        c.set(17, c.u8(c.r(3) + 0x791d));
        c.set(18, c.u8(c.r(4) + 0x791e));
    }
    sample_fitness(c);
    c.s.pc = ball_action::orientation_phase;
}

std::uint32_t signed_half(std::uint32_t value) { return std::uint32_t(std::int16_t(value)); }
std::uint32_t signed_byte(std::uint32_t value) { return std::uint32_t(std::int8_t(value)); }

std::uint32_t subtract_from(Cpu& c, std::uint32_t value, std::uint32_t limit) {
    c.s.wr<1>(reg::xer_ca, value <= limit);
    return limit - value;
}

static void orientation_bias(Cpu& c) {
    c.s.pc = ball_action::orientation_phase;
    c.set(3, c.u32(c.r(15) + 0x1a4));
    c.set(21, signed_half(c.r(13)));
    c.set(0, std::uint32_t(c.i16(c.r(15) + 0x19e)));
    c.set(3, std::uint32_t(c.i16(c.r(3) + 0x7950)));
    c.set(3, c.r(0) - c.r(3));
    c.call(0x10775540, 0x1023d19c);

    c.set(13, signed_half(c.r(3)));
    c.compare(0, std::int32_t(c.r(13)), 180);
    if (c.cr(0) & 4) {
        c.set(0, subtract_from(c, c.r(13), 360));
        c.set(13, signed_half(c.r(0)));
    }
    c.set(3, signed_half(c.r(13)));
    c.set(0, signed_byte(c.r(22)));
    c.set(4, c.r(3) << 1);
    c.set(3, 1);
    c.set(4, subtract_from(c, c.r(4), 360));
    c.set(0, subtract_from(c, c.r(0), 5));
    c.compare(0, std::int32_t(c.r(0)), 1);
    c.set(20, signed_half(c.r(4)));
    if (!(c.cr(0) & 8)) {
        c.set(0, signed_byte(c.r(22)));
        c.set(3, subtract_from(c, c.r(0), 5));
    }

    c.set(3, signed_half(c.r(3)));
    c.set(0, signed_half(c.r(13)));
    c.set(3, std::uint32_t(divide_signed<4>(c.r(20), c.r(3))));
    c.compare(0, std::int32_t(c.r(0)), 90);
    c.set(20, signed_half(c.r(3)));
    if (!(c.cr(0) & 8)) {
        c.set(0, c.r(20) << 1);
        c.set(20, signed_half(c.r(0)));
        return;
    }

    c.set(0, signed_half(c.r(13)));
    c.set(3, 0x66666667);
    const auto high = std::int32_t((std::int64_t(std::int32_t(c.r(0))) * c.r(3)) >> 32);
    c.s.wr<1>(reg::xer_ca, high < 0 && (std::uint32_t(high) & 3) != 0);
    c.set(0, std::uint32_t(high >> 2));
    c.set(3, c.r(0) >> 31);
    c.set(0, subtract_from(c, c.r(0) + c.r(3), 11));
    c.set(0, signed_half(c.r(0)) * c.r(20));
    c.set(20, signed_half(c.r(0)));
}

static void sample_surface_two(Cpu& c) {
    advance_stream(c, 0x1023d298);
    c.set(3, c.u32(c.r(30)));
    c.set(4, 0x55550000);
    c.set(0, c.u32(c.r(27)));
    c.set(4, 0x55555556);
    c.set(5, c.u16(c.r(3)));
    c.set(3, signed_byte(c.r(14)));
    c.set(5, c.r(5) + c.r(0));
    c.set(4, std::uint32_t((std::int64_t(std::int32_t(c.r(5))) * c.r(4)) >> 32));
    c.set(0, (c.r(4) + (c.r(4) >> 31)) * 3);
    c.set(4, c.r(5) - c.r(0));
    c.set(0, signed_half(c.r(3) * (c.r(4) + 1)));
    c.set(16, c.r(16) - c.r(0));
}

static void sample_surface_thirty_two(Cpu& c) {
    advance_stream(c, 0x1023d304);
    c.set(4, c.u32(c.r(30)));
    c.set(3, signed_byte(c.r(14)));
    c.set(0, c.u32(c.r(27)));
    c.set(4, c.u16(c.r(4)));
    c.set(4, c.r(4) + c.r(0));
    c.set(0, (c.r(4) << 29) & 0xe0000000u);
    c.set(4, c.r(4) >> 31);
    c.set(0, std::rotl(c.r(0) - c.r(4), 3));
    c.set(4, c.r(0) + c.r(4));
    c.set(0, signed_half(c.r(3) * (c.r(4) + 3)));
    c.set(16, c.r(16) - c.r(0));
}

void orient(Cpu& c) {
    orientation_bias(c);
    const auto match = c.r(15) + 0x1a4;
    c.set(3, c.u32(match));
    c.set(3, c.u8(c.r(3) + 0x7920));
    c.set(0, c.r(3) - 1);
    c.set(14, signed_byte(c.r(0)));
    c.compare(0, std::int32_t(c.r(14)), 0);
    const auto height = c.r(14);
    c.set(0, signed_half(height * height * height * 20));
    c.set(16, c.r(16) - c.r(0));
    if (!(c.cr(0) & 2)) {
        c.set(3, c.u32(match));
        c.set(0, std::uint32_t(c.i8(c.r(3) + 1)));
        c.compare(0, std::int32_t(c.r(0)), 32);
        if (c.cr(0) & 2) sample_surface_thirty_two(c);
        else if (c.cr(0) & 8) {
            c.compare(0, std::int32_t(c.r(0)), 2);
            if (c.cr(0) & 2) sample_surface_two(c);
        }
    }
    c.s.pc = ball_action::action_bias_phase;
}

constexpr unsigned facing_interval = 200;

static void draw_facing_sample(Cpu& c) {
    c.set(0, facing_interval);
    c.compare(0, facing_interval, 0);
    c.set(13, c.u8(c.r(actor_register) + 0x10f));
    c.set(14, c.u8(c.r(actor_register) + 0x108));
    advance_stream(c, 0x1023d39c);

    // The source fixes r19 at 200 and narrows it to a signed halfword
    // before either bound comparison. Keep the final compare and stream
    // effects. Neither the zero case nor the wide sampler can be selected.
    c.set(3, std::uint32_t(-65535));
    c.set(0, signed_half(c.r(19)));
    c.compare(0, std::int32_t(c.r(0)), -65535);
    c.set(5, c.u32(c.r(30)));
    c.set(3, signed_half(c.r(19)));
    c.set(4, c.u32(c.r(27)));
    c.set(0, signed_half(c.r(19)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    c.set(0, c.u16(c.r(5)));
    const auto sample = c.r(0) + c.r(4);
    c.set(4, sample);
    const auto interval = c.r(3);
    const auto remainder = sample - std::uint32_t(divide_signed<4>(sample, interval)) * interval;
    c.set(0, remainder);
    c.set(25, remainder);
}

static bool should_face_attack(Cpu& c, std::uint32_t actor) {
    const auto reach = signed_byte(c.r(13)) * signed_byte(c.r(14));
    c.set(3, signed_byte(c.r(13)));
    c.set(0, reach);
    c.compare(0, std::int32_t(reach), std::int32_t(c.r(25)));
    if (c.cr(0) & 4) return true;

    Fpu fp(c);
    c.set_f(0, c.f64(c.r(26) + 0xc8));
    fp.fcmp(0, c.f(29), c.f(0));
    if (c.cr(0) & 8) return true;

    c.set(3, c.u32(actor + actor_match));
    c.set(4, actor);
    c.set(5, 0);
    c.call(0x101d8d70, 0x1023d524);
    c.set(0, signed_byte(c.r(3)));
    c.compare(0, std::int32_t(c.r(0)), 0);
    return (c.cr(0) & 2) == 0;
}

void call_owned(Cpu& c, std::uint32_t entry, std::uint32_t back) {
    c.link(back);
    c.s.pc = entry;
    if (!try_modern_match_pl(c.s, entry)) run_until(c.s, back);
}

static void face_attack(Cpu& c, std::uint32_t actor) {
    const auto away = c.i8(actor + actor_side);
    c.set(0, std::uint32_t(away));
    c.compare(0, away, 0);
    const auto row = c.i8(actor + actor_row);
    c.set(0, std::uint32_t(row));
    c.set(3, actor);
    c.set(4, c.u8(actor + actor_column));
    c.set(6, 4);
    c.set(5, c.u8(actor + actor_row));
    c.compare(0, row, away ? 3 : 9);
    if (away ? row > 3 : row < 9) {
        c.set(7, std::uint32_t(c.i8(actor + actor_row)));
        c.set(0, c.r(7) + (away ? -3u : 3u));
    } else c.set(0, away ? 0 : 11);
    c.set(7, signed_byte(c.r(0)));
    call_owned(c, 0x10237830, away ? 0x1023d5b4 : 0x1023d574);
}

static void face_target_if_needed(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + actor_match));
    const auto current_ball_row = c.i8(c.r(3) + ball_row);
    c.set(0, std::uint32_t(current_ball_row));
    c.compare(0, current_ball_row, 4);
    if (current_ball_row >= 4) return;
    c.set(4, c.u8(actor + actor_column));
    c.set(3, actor);
    c.set(5, c.u8(actor + actor_row));
    c.set(6, c.r(17));
    c.set(7, c.r(18));
    call_owned(c, 0x10237830, 0x1023d5e4);
}

// This repeated bounds check uses different scratch registers depending on
// whether the caller needs an exemption or an area contribution.
void check_area_coordinates(Cpu& c, std::uint32_t actor, bool away,
                            unsigned coordinate, unsigned inside_register, unsigned result) {
    const auto check = [&](std::uint32_t offset, int boundary) {
        const auto value = c.i8(actor + offset);
        c.set(coordinate, std::uint32_t(value));
        c.compare(0, value, boundary);
        return value;
    };

    const auto column = c.i8(actor + actor_column);
    c.set(inside_register, 0);
    c.set(coordinate, std::uint32_t(column));
    c.compare(0, column, 2);
    if (column >= 2 && check(actor_column, 6) <= 6 &&
        check(actor_row, away ? 10 : 0) >= (away ? 10 : 0) &&
        check(actor_row, away ? 11 : 1) <= (away ? 11 : 1))
        c.set(inside_register, 1);
    c.set(result, c.r(inside_register));
}

static bool area_exempts_action(Cpu& c, std::uint32_t actor) {
    const auto side = c.u8(actor + actor_side);
    c.set(0, side);
    c.compare(0, side, 1);
    check_area_coordinates(c, actor, side == 1, 0, 3, 0);
    c.compare(0, std::int32_t(c.r(0)), 0);
    if (c.cr(0) & 2) return false;

    c.set(3, c.u32(actor + actor_match));
    c.set(4, actor);
    c.call(0x101e3d50, 0x1023d69c);
    c.set(0, signed_half(c.r(3)));
    c.compare(0, std::int32_t(c.r(0)), 1);
    return c.cr(0) & 2;
}

static std::int32_t area_reaction_interval(Cpu& c, std::uint32_t actor) {
    c.set(3, c.u32(actor + 0x6c));
    const auto side = c.i8(actor + actor_side);
    c.set(0, std::uint32_t(side));
    const auto tendency = c.i8(c.r(3) + 0x68);
    c.set(3, std::uint32_t(tendency));
    c.compare(0, side, 1);
    c.set(0, std::uint32_t(tendency * 200));
    check_area_coordinates(c, actor, side == 1, 3, 4, 3);
    c.set(3, c.r(3) * 500);
    const auto base = c.r(0) + c.r(3);
    const auto interval = base + 100;
    c.s.wr<1>(reg::xer_ca, interval < base);
    c.set(14, interval);
    c.compare(0, std::int32_t(interval), 0);
    return std::int32_t(interval);
}

void sample_narrow_interval(Cpu& c, std::int32_t interval, unsigned result) {
    c.set(3, c.u32(c.r(30)));
    c.compare(0, interval, 0);
    c.set(0, c.u32(c.r(27)));
    const auto sample = c.u16(c.r(3)) + c.r(0);
    c.set(3, sample);
    const auto remainder = sample - std::uint32_t(divide_signed<4>(sample, std::uint32_t(interval))) *
        std::uint32_t(interval);
    c.set(0, interval > 0 ? remainder : 0u - remainder);
    c.set(result, c.r(0));
}

void draw_narrow_interval(Cpu& c, std::int32_t interval, unsigned result, std::uint32_t refill_back) {
    if (interval == 0) {
        c.set(result, 0);
        return;
    }
    advance_stream(c, refill_back);
    c.set(3, std::uint32_t(-65535));
    c.compare(0, interval, -65535);
    sample_narrow_interval(c, interval, result);
}

void face(Cpu& c) {
    const auto actor = c.r(actor_register);
    c.set(3, c.u32(actor + actor_match));
    c.set(0, signed_half(c.r(13)));
    c.compare(0, std::int32_t(c.r(0)), 90);
    c.set(19, facing_interval);
    c.set(3, c.r(3) + 0x10000);
    c.set(24, c.u32(c.r(3) - 0x2008));
    if (!(c.cr(0) & 4)) {
        draw_facing_sample(c);
        if (should_face_attack(c, actor)) face_attack(c, actor);
        else face_target_if_needed(c, actor);
    }
    c.s.pc = ball_action::area_selection_phase;
}

void select_area(Cpu& c) {
    const auto actor = c.r(actor_register);
    c.s.pc = ball_action::area_selection_phase;
    if (area_exempts_action(c, actor)) {
        c.s.pc = ball_action::ball_state_selection_phase;
        return;
    }
    // The signed tendency byte and area bonus constrain this interval
    // to [-25,500, 26,000]. Only the original narrow draw is reachable.
    draw_narrow_interval(c, area_reaction_interval(c, actor), 13, 0x1023d79c);
    c.compare(0, std::int32_t(c.r(13)), 0);
    c.s.pc = ball_action::ball_state_selection_phase;
    if (!(c.cr(0) & 2)) return;

    constexpr unsigned area_action = 37;
    c.set(13, area_action);
    c.set(0, area_action * 2);
    c.set(3, actor + c.r(0));
    c.set(17, std::uint32_t(c.i16(c.r(3) + 0x11e)));
    c.s.pc = ball_action::selected_action_phase;
}
}
