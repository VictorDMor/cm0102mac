#include "match_pl_construction.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
std::int32_t signed_high(std::uint32_t value, std::uint32_t multiplier) {
    return std::int32_t(std::uint64_t(std::int64_t(std::int32_t(value)) * std::int32_t(multiplier)) >> 32);
}
int arithmetic_shift(Cpu& c, int value, unsigned bits) {
    c.s.wr<1>(reg::xer_ca, value < 0 && (std::uint32_t(value) & ((1u << bits) - 1)));
    return value >> bits;
}
}

std::uint32_t construct_player(Cpu& c, std::uint32_t player, std::uint32_t match,
    std::uint32_t staff, std::uint32_t side, std::uint32_t initial, std::uint32_t slot) {
    PlayerConstruction activation(c, player, match, staff, side, initial, slot);
    activation.run();
    return c.r(3);
}

PlayerConstruction::PlayerConstruction(Cpu& cpu, std::uint32_t player, std::uint32_t match,
    std::uint32_t staff_address, std::uint32_t side, std::uint32_t initial, std::uint32_t initial_slot)
    : c(cpu), frame(c, 0x10223ac0, 0xd0, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 30}),
      fp(c), actor{c, player}, staff(staff_address), side_argument(side), slot(initial_slot),
      doubles(c.toc(-0x51e8)), floats(c.toc(-0x51e0)), seed(c.toc(-0x7b90)),
      begin(c.toc(-0x7b8c)), end(c.toc(-0x7b88)), cursor(c.toc(-0x7b84)), staff_count(c.toc(-0x7be4)) {
    keep(c, {{16, staff}, {18, slot}, {23, c.toc(-0x7b3c)}, {14, c.toc(-0x7b80)},
        {15, player}, {24, c.toc(-0x7b5c)}, {17, side_argument}, {13, c.toc(-0x7ba0)},
        {20, 0}, {25, staff_count}, {19, 0}, {26, doubles}, {27, seed}, {28, begin},
        {29, end}, {30, cursor}, {31, floats}});
    c.put32(player + 0x1a4, match); c.put32(player + 0x6c, staff);
    c.put8(player, initial); c.put8(player + 0x2f, slot);
}

void PlayerConstruction::advance(std::uint32_t back) {
    const auto previous = c.u32(cursor); c.set(3, previous);
    const auto limit = c.u32(end), next = previous + 2;
    c.set(3, next);
    const auto order = compare_logical(c, next, limit);
    c.put32(cursor, next);
    if (order > 0) {
        c.put32(cursor, begin); c.call(0x1077b770, back); c.put32(seed, c.r(3) & 0xffff);
    }
}

std::uint32_t PlayerConstruction::fallback_index(std::uint32_t count) {
    if (compare(c, std::int32_t(count)) == 0) return 0;
    advance(0x10223bb8);
    c.set(3, 65535);
    bool wide = compare(c, std::int32_t(count), 65535) > 0;
    if (!wide) {
        c.set(3, std::uint32_t(-65535)); wide = compare(c, std::int32_t(count), -65535) < 0;
    }
    if (!wide) {
        const auto draw_at = c.u32(cursor); c.set(3, draw_at);
        const bool positive = compare(c, std::int32_t(count)) > 0;
        const auto bias = c.u32(seed), draw = std::uint32_t(c.u16(draw_at)) + bias;
        c.set(3, draw);
        const auto quotient = std::uint32_t(divide_signed<4>(draw, count));
        const auto residual = draw - quotient * count;
        return positive ? residual : 0u - residual;
    }

    // The count is a full word here. Its wide arm is reachable, unlike the
    // byte-bounded draws in periodic balance. Keep the original signed wrap
    // before reduction and execute the owned rounding draw directly.
    c.set(3, count ^ 0x80000000); c.put32(at(0x60), c.r(3));
    c.set(6, 0x80010000); c.set(3, c.u32(cursor)); c.set(5, 0x10000);
    c.put32(at(0x5c), 0x43300000);
    c.set_f(2, c.f64(doubles + 0x80)); c.set(4, 0x80010000);
    c.set_f(1, c.f64(at(0x5c))); c.set(6, 0x80008001);
    c.set_f(0, c.f64(doubles)); c.set(5, 65535);
    c.set_f(1, fp.fsub(c.f(1), c.f(2)));
    const auto word = c.u16(c.r(3)); c.set(8, word);
    const auto bias = c.u32(seed); c.set(7, bias); c.set(3, count * 10);
    c.put32(at(0x54), 0x43300000); c.set(4, 0x80008001);
    c.set_f(2, fp.fdiv(c.f(1), c.f(0)));
    const auto draw = std::uint32_t(word) + bias;
    const auto product = std::uint32_t(signed_high(draw, 0x80008001)) + draw;
    c.set_f(1, c.f64(doubles + 0x80));
    const auto shifted = arithmetic_shift(c, std::int32_t(product), 15);
    c.set(7, shifted < 0); c.set(6, std::uint32_t(shifted) + c.r(7));
    c.set(5, c.r(6) * 65535);
    const auto residual = draw - c.r(5); keep(c, {{22, residual}});
    c.put32(at(0x58), residual ^ 0x80000000);
    const auto scaled = count * 10;
    const auto reduction_product = std::uint32_t(signed_high(scaled, 0x80008001)) + scaled;
    c.set_f(0, c.f64(at(0x54))); c.set_f(0, fp.fsub(c.f(0), c.f(1)));
    const auto reduction_shift = arithmetic_shift(c, std::int32_t(reduction_product), 15);
    c.set_f(0, fp.fmul(c.f(2), c.f(0)));
    const auto correction_bound = std::uint32_t(reduction_shift) + unsigned(reduction_shift < 0);
    c.set(3, correction_bound);
    c.put64(at(0x4c), fp.fctiwz(0, c.f(0)));
    Kept<22> coarse(c, c.u32(at(0x50)));
    enter(c, 0x10223c90); sample_range(c, correction_bound);
    c.set(4, 0x66660000);
    const auto rounded = c.r(3) + 5; c.set(3, rounded); c.set(4, 0x66666667);
    const auto rounding_shift = arithmetic_shift(c, signed_high(rounded, 0x66666667), 2);
    c.set(3, rounding_shift); c.set(4, rounding_shift < 0);
    const auto correction = std::uint32_t(rounding_shift) + c.r(4); c.set(3, correction);
    const auto candidate = std::uint32_t(coarse) + correction;
    return compare(c, std::int32_t(candidate), std::int32_t(count - 1)) > 0 ? count - 1 : candidate;
}

void PlayerConstruction::choose_record() {
    if (compare_logical(c, c.u32(staff + 0x14)) != 0) {
        c.put32(actor.address + 0x70, c.u32(staff + 0x14));
    } else {
        const bool away = compare(c, std::int8_t(side_argument)) != 0;
        c.set(3, c.u32(actor.match() + 0x3118)); c.set(5, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
        c.set(8, c.r(5) + 0x1ee); c.set(3, c.toc(-0x7bb8)); c.set(4, c.toc(-0x51e4));
        c.set(5, c.u32(staff)); c.set(6, c.u32(staff + 0x28)); c.set(7, c.u32(staff + 0x2c));
        c.call(0x1077b720, 0x10223b80);
        c.set(3, c.toc(-0x74d4));
        Kept<21> count(c, c.u32(c.r(3))); Kept<22> selected(c, fallback_index(count));
        c.set(3, c.toc(-0x7b38)); c.set(3, c.u32(c.r(3)));
        c.put32(actor.address + 0x70, c.r(3) + std::uint32_t(selected) * 0x48);
    }
    bool synthetic = false;
    if (compare_logical(c, c.u32(actor.address + 0x6c)) != 0) {
        c.set(3, c.u32(actor.address + 0x6c));
        const auto count = c.i32(staff_count); c.set(3, c.u32(c.r(3)));
        synthetic = compare(c, std::int32_t(c.r(3)), count) >= 0;
    }
    if (synthetic) c.put8(actor.address + 0x17, 10);
    else { c.set(3, c.u32(actor.address + 0x70)); c.put8(actor.address + 0x17, c.u8(c.r(3) + 0x45)); }
    bool known = true;
    if (compare_logical(c, staff) != 0) {
        c.set(3, c.u32(staff)); known = compare(c, std::int32_t(c.r(3)), c.i32(staff_count)) < 0;
    }
    if (known) {
        // This intentionally reads the preexisting side. The supplied side
        // does not replace it until the later name/preference phase.
        const auto old_side = actor.side(); c.set(3, actor.match());
        const bool away = compare(c, std::int8_t(old_side)) != 0;
        c.set(3, c.u32(c.r(3) + 0x3118)); c.set(5, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
        c.set(3, c.toc(-0x7b7c)); c.set(4, staff); c.set(3, c.u32(c.r(3)));
        c.call(0x10082f90, 0x10223d8c); keep(c, {{20, c.r(3)}});
    }
}

void PlayerConstruction::copy_attributes_and_clubs() {
    c.set(3, c.u32(actor.address + 0x70));
    const bool away = compare(c, std::int8_t(side_argument)) != 0;
    c.put16(actor.address + 0x48, extended(c.i16(c.r(3) + 0xa)));
    for (const auto [source, destination] : {std::pair{8u, 0x4au}, {0xcu, 0x4cu}}) {
        c.set(3, c.u32(actor.address + 0x70)); c.put16(actor.address + destination, extended(c.i16(c.r(3) + source)));
    }
    c.set(3, c.u32(actor.match() + 0x3118));
    c.put32(actor.address + 0xfc, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
    c.set(3, c.u32(actor.match() + 0x3118));
    keep(c, {{21, c.u32(c.r(3) + (away ? 0x1c : 0x20))}});
}

void PlayerConstruction::run() {
    choose_record(); copy_attributes_and_clubs();
    initialize_fitness(); initialize_affiliation(); initialize_eligibility();
    copy_preferences_and_names(); inspect_special_club_context();
    initialize_random_preferences();
    if (prepare_personality_factors()) {
        Kept<14> bound(c, personality_bound());
        Kept<13> selected(c, variable_draw(bound, 0x10224e94, 0x10224f6c));
        choose_personality_band();
    }
    apply_personality_attributes();
    if (!apply_coach_influence()) apply_fallback_influence();
    if (own_competition_has_history()) { apply_standings_pressure(); apply_club_history_pressure(); }
    apply_goalkeeper_context(); apply_staff_context(); finish_attributes();
    frame.leave(); c.finish();
}

void PlayerConstruction::initialize_fitness() {
    c.set(3, c.u32(c.r(13))); c.set(4, staff); c.set(5, 0);
    c.call(0x101878d0, 0x10223e08);
    const auto fitness = extended(std::int16_t(c.r(3))); c.set(3, fitness);
    c.put32(at(0x4c), 0x43300000); c.set_f(1, c.f64(doubles + 0x80)); c.set(4, staff);
    c.put32(at(0x50), fitness ^ 0x80000000); c.set(5, 0); c.set(3, c.toc(-0x7a7c));
    c.set_f(0, c.f64(at(0x4c))); c.set_f(0, fp.fsubs(c.f(0), c.f(1)));
    c.put_f32(actor.address + 0x50, float(c.f(0)));
    c.set_f(0, c.f32(actor.address + 0x50)); c.put_f32(actor.address + 0x54, float(c.f(0)));
    c.set(3, c.u32(c.r(3))); c.call(0x103d0990, 0x10223e48);
    c.set_f(0, c.f32(floats)); c.set(4, staff); c.put_f32(actor.address + 0x5c, float(c.f(0)));
    c.set(3, c.u32(c.r(13))); c.call(0x10187680, 0x10223e5c);
    if (compare(c, std::int8_t(c.r(3)), 2) == 0) c.put8(actor.address + 0x19, -2);
    else if (compare(c, std::int8_t(slot)) >= 0 && compare(c, std::int8_t(slot), 10) <= 0) {
        c.put8(actor.address + 0x19, slot);
        enter(c, 0x10223e94, {actor.address}); current_role(c, actor.address);
        c.put16(actor.address + 0x1c2, c.r(3));
    } else {
        c.set(3, 6400); c.put8(actor.address + 0x19, -1);
        c.put16(actor.address + 0x36, 6400); c.put16(actor.address + 0x1c2, 0);
    }
}

bool PlayerConstruction::opponent_lists_club() {
    for (const auto offset : {0x74u, 0x78u, 0x7cu}) {
        c.set(3, c.u32(actor.address + 0xfc));
        if (compare_logical(c, c.r(3), c.u32(c.r(21) + offset)) == 0) return true;
    }
    return false;
}

void PlayerConstruction::initialize_affiliation() {
    c.set(3, 0x43300000); c.put8(actor.address + 0x1f, -1);
    c.set(6, -1); c.set(5, -1); c.put8(actor.address + 0x22, -1);
    c.set_f(1, c.f64(doubles + 0x80)); c.set(4, c.r(21));
    c.put8(actor.address + 0x20, -1); c.put8(actor.address + 0x21, -1);
    c.set(5, c.u32(actor.address + 0x6c)); c.put32(at(0x4c), c.r(3));
    const auto doubled = std::uint32_t(std::int32_t(c.i8(c.r(5) + 0x69))) * 2;
    const auto intensity = 50u - doubled;
    c.s.wr<1>(reg::xer_ca, doubled <= 50); c.set(3, intensity ^ 0x80000000);
    c.put32(at(0x50), c.r(3)); c.set_f(0, c.f64(at(0x4c)));
    c.set_f(0, fp.fsubs(c.f(0), c.f(1))); c.put_f32(actor.address + 0x1a8, float(c.f(0)));
    c.put16(actor.address + 0x32, 0);
    const auto change = [&](unsigned offset, int amount) {
        c.set(3, extended(c.i16(actor.address + offset)));
        c.put16(actor.address + offset, c.r(3) + std::uint32_t(amount));
    };
    c.set(3, c.u32(actor.address + 0x6c)); c.call(0x100d9480, 0x10223f1c);
    if (compare(c, std::int8_t(c.r(3))) != 0) change(0x32, 10);
    else {
        c.set(3, c.u32(actor.address + 0x6c)); c.set(4, c.r(21)); c.call(0x100d94d0, 0x10223f40);
        if (compare(c, std::int8_t(c.r(3))) != 0) change(0x32, -20);
    }
    if (opponent_lists_club()) change(0x32, -10);
    c.put16(actor.address + 0x34, 10);
    c.set(3, c.u32(actor.address + 0x6c)); c.set(4, c.u32(actor.address + 0xfc));
    c.call(0x100d9480, 0x10223fa4);
    if (compare(c, std::int8_t(c.r(3))) != 0) change(0x34, 30);
    else {
        c.set(3, c.u32(actor.address + 0x6c)); c.set(4, c.u32(actor.address + 0xfc));
        c.call(0x100d94d0, 0x10223fc8);
        if (compare(c, std::int8_t(c.r(3))) != 0) change(0x34, -10);
    }
    if (opponent_lists_club()) change(0x34, 5);
    c.set(4, 0); c.put8(actor.address + 0x1d, 0); c.set(3, 0);
    c.put8(actor.address + 0x1e, 0); c.put16(actor.address + 0x3a, 0); c.put8(actor.address + 0x30, 0);
}

bool PlayerConstruction::eligibility_requires_history() {
    c.set(3, actor.match());
    if (compare(c, c.i8(c.r(3) + 0x7931)) == 0) return false;
    if (compare(c, std::int8_t(side_argument)) == 0) {
        c.set(3, c.u32(actor.match() + 0x3118));
        if (compare(c, c.i8(c.r(3) + 0x40)) == 0) return false;
    }
    if (compare(c, std::int8_t(side_argument)) != 0) {
        c.set(3, c.u32(actor.match() + 0x3118));
        if (compare(c, c.i8(c.r(3) + 0x41)) == 0) return false;
    }
    if (compare_logical(c, staff) != 0) {
        c.set(3, c.u32(staff));
        if (compare(c, std::int32_t(c.r(3)), c.i32(staff_count)) >= 0) return false;
    }
    c.set(3, c.u32(actor.match() + 0x3118));
    return compare(c, c.u8(c.r(3) + 0x3a), 1) != 0;
}

bool PlayerConstruction::competition_history_available() {
    c.set(3, c.u32(actor.address + 0xfc)); c.set(3, c.u32(c.r(3) + 0x2c));
    if (compare(c, c.i8(c.r(3) + 0x120)) == 0) return false;
    c.set(3, c.u32(actor.address + 0xfc));
    if (compare_logical(c, c.u32(c.r(3) + 0x30)) == 0) return false;
    c.set(3, c.u32(actor.address + 0xfc)); c.set(4, c.u32(c.r(24)));
    c.set(3, c.u32(c.r(3) + 0x30));
    if (compare_logical(c, c.u32(c.r(4) + c.u32(c.r(3)) * 4)) == 0) return false;
    c.set(4, c.u32(actor.address + 0xfc)); c.set(3, c.toc(-0x7a24));
    c.set(4, c.u32(c.r(4) + 0x2c));
    const auto nation = c.i32(c.r(3)); c.set(3, c.u32(c.r(4) + 0x68));
    if (compare(c, std::int32_t(c.r(3)), nation) != 0) return true;
    return compare_logical(c, c.u16(c.r(14) + 2), 2002) >= 0;
}

void PlayerConstruction::unknown_eligibility() {
    c.set(3, -1000); c.put16(actor.address + 0x3e, -1000);
    c.set(5, -1000); c.set(4, -1000); c.set(3, -1000); c.put16(actor.address + 0x40, -1000);
    c.put16(actor.address + 0x42, -1000); c.put16(actor.address + 0x44, -1000);
    c.put16(actor.address + 0x46, -1000); c.put8(actor.address + 0x31, 0);
}

void PlayerConstruction::special_club_eligibility() {
    const auto eligible = c.u8(staff + 0x5e) == 0;
    c.set(5, -1000); c.set(4, -1000); c.set(3, -1000); c.set(6, eligible);
    c.put8(actor.address + 0x31, eligible); c.put16(actor.address + 0x3e, -1000);
    c.set(5, c.u8(staff + 0x5f)); c.put16(actor.address + 0x40, c.r(5));
    c.put16(actor.address + 0x44, -1000); c.put16(actor.address + 0x46, -1000); c.put16(actor.address + 0x42, -1000);
}

void PlayerConstruction::ordinary_eligibility() {
    c.put16(actor.address + 0x40, -1000); c.set(3, c.u32(actor.match() + 0x3118));
    bool use_attributes = compare(c, c.i32(c.r(3)), -1) > 0;
    if (use_attributes) {
        c.set(3, c.u32(c.u32(actor.match() + 0x3118))); c.call(0x104646d0, 0x10224208);
        use_attributes = compare(c, std::int8_t(c.r(3))) != 0;
    }
    if (use_attributes) {
        c.set(3, c.u32(c.r(23))); c.set(5, 2); c.set(4, c.u32(actor.address + 0x70));
        c.call(0x10306a80, 0x10224220);
        const auto converted = fp.fctiwz(0, c.f(1)); c.set(4, staff); c.put64(at(0x4c), converted);
        c.put16(actor.address + 0x42, c.u32(at(0x50)));
        c.set(3, c.u32(c.r(23))); c.call(0x10305ae0, 0x1022423c); c.put16(actor.address + 0x44, c.r(3));
    } else {
        c.set(3, -1000); c.put16(actor.address + 0x42, -1000); c.put16(actor.address + 0x44, -1000);
    }
    c.set(3, c.u32(c.r(23))); c.set(4, staff); c.set(5, c.u32(actor.address + 0xfc));
    c.call(0x10305700, 0x10224264);
    const bool eligible = std::int16_t(c.r(3)) == 0; c.set(4, staff); c.put8(actor.address + 0x31, eligible);
    c.set(3, c.u32(c.r(23))); c.set(5, c.u32(actor.address + 0xfc)); c.call(0x103058f0, 0x10224284);
    c.put16(actor.address + 0x3e, c.r(3)); c.put16(actor.address + 0x46, -1000);
    if (compare_logical(c, c.u32(staff + 0x24)) == 0) return;
    c.set(3, c.u32(staff + 0x24)); c.set(4, c.r(14)); c.call(0x100d9440, 0x102242a8);
    if (compare(c, std::int8_t(c.r(3))) == 0) return;
    c.set(3, c.u32(c.r(23))); c.set(5, 6); c.set(4, c.u32(actor.address + 0x70)); c.set(6, 1);
    c.set_f(1, c.f32(floats + 4)); c.call(0x10303560, 0x102242c8);
    c.set_f(0, c.f64(doubles + 8)); fp.fcmp(0, c.f(1), c.f(0));
    c.set_cr(0, c.cr(0) | ((c.cr(0) & 4) ? 2 : 0));
    if (!(c.cr(0) & 2)) return;
    c.set(3, c.u32(c.r(23))); c.set(5, 6); c.set(4, c.u32(actor.address + 0x70)); c.set(6, 2);
    c.set_f(1, c.f32(floats + 4)); c.call(0x10303560, 0x102242f0);
    c.put64(at(0x4c), fp.fctiwz(0, c.f(1))); c.put16(actor.address + 0x46, c.u32(at(0x50)));
}

void PlayerConstruction::initialize_eligibility() {
    if (eligibility_requires_history()) {
        c.set(3, c.u32(actor.address + 0xfc)); c.call(0x100ce250, 0x102240b4);
        if (compare(c, std::int8_t(c.r(3))) != 0) { unknown_eligibility(); return; }
        c.set(3, c.u32(actor.address + 0xfc)); c.call(0x100c7d00, 0x102240f8);
        if (compare(c, std::int8_t(c.r(3))) != 0) special_club_eligibility();
        else if (competition_history_available()) ordinary_eligibility();
        else unknown_eligibility();
        return;
    }
    c.set(3, c.u32(actor.address + 0xfc)); c.call(0x100c7d00, 0x1022430c);
    bool special = compare(c, std::int8_t(c.r(3))) != 0;
    if (special) {
        c.set(3, c.u32(actor.address + 0xfc)); c.call(0x100ce250, 0x1022431c);
        special = compare(c, std::int8_t(c.r(3))) == 0;
    }
    if (special) {
        c.set(3, c.u32(actor.match() + 0x3118)); special = compare(c, c.i8(c.r(3) + 0x40)) != 0;
    }
    if (special) special_club_eligibility(); else unknown_eligibility();
}

void PlayerConstruction::copy_preferences_and_names() {
    c.set(4, c.u32(actor.address + 0x70)); c.set(3, c.u32(actor.address + 0x70));
    c.set(4, c.u8(c.r(4) + 0x30)); const auto other = c.i8(c.r(3) + 0x3b);
    c.set(3, std::int32_t(std::int8_t(c.r(4))));
    const bool first = compare(c, std::int32_t(c.r(3)), other) > 0;
    c.put8(actor.address + 0x2d, !first); c.set(3, c.u32(actor.address + 0x70));
    const auto preference = c.u8(c.r(3) + (first ? 0x3b : 0x30));
    c.s.wr<1>(reg::xer_ca, preference <= 20); c.put8(actor.address + 0x2e, 20u - preference);
    for (const auto source : {0x28u, 0x2cu, 0x30u}) {
        if (compare_logical(c, c.u32(staff + source)) != 0) {
            c.set(3, c.u32(staff + source)); c.put32(actor.address + 0x60 + source - 0x28, c.u32(c.r(3) + 0x20));
        } else c.put32(actor.address + 0x60 + source - 0x28, -1);
    }
    c.put8(actor.address + 0x27, side_argument);
}

void PlayerConstruction::inspect_special_club_context() {
    if (compare_logical(c, c.u32(actor.address + 0xfc)) == 0) return;
    c.set(3, c.u32(actor.address + 0xfc));
    if (compare_logical(c, c.u32(c.r(3) + 0x2c)) == 0) return;
    c.set(3, c.u32(actor.address + 0xfc));
    if (compare(c, c.i16(c.r(3) + 0x1ec), 5750) <= 0) return;
    const auto side = std::int8_t(actor.side()); c.set(4, actor.match());
    c.set(3, std::uint32_t(std::int32_t(side)) * 0xd8);
    // The original tests the wrapped controller address itself for zero.
    if (compare(c, std::int32_t(c.r(4) + c.r(3) + 0x7aac)) != 0) return;
    const auto reread_side = actor.side(); c.set(3, actor.match());
    const bool away = compare(c, std::int8_t(reread_side)) != 0;
    c.set(3, c.u32(c.r(3) + 0x3118)); c.set(3, c.u32(c.r(3) + (away ? 0x20 : 0x1c)));
    c.set(4, 0); c.set(5, 1); c.set(6, 0); c.set(7, 0); c.call(0x1014c2e0, 0x102244e8);
    if (compare(c, std::int8_t(c.r(3))) != 0) return;
    c.set(3, c.u32(actor.address + 0xfc)); c.set(3, c.u32(c.r(3) + 0x2c));
    if (compare(c, c.i8(c.r(3) + 0x120)) == 0) keep(c, {{19, 1}});
}
}
