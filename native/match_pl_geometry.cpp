#include "match_pl_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl {
using namespace guest;
namespace {
std::int8_t absolute(Cpu& c, std::int32_t value, std::uint32_t back) {
    c.set(3, std::uint32_t(value)); c.call(0x10775540, back);
    return std::int8_t(c.r(3));
}
std::uint32_t distance_offset(std::int8_t columns, std::int8_t rows) {
    return std::uint32_t(std::int32_t(rows)) * 36 + std::uint32_t(std::int32_t(columns)) * 4;
}
double signed_conversion(Cpu& c, std::uint32_t at, std::uint32_t value, double bias) {
    c.put32(at, 0x43300000); c.put32(at + 4, value ^ 0x80000000);
    Fpu fp(c); return fp.fsub(c.f64(at), bias);
}
}

double distance_to(Cpu& c, std::uint32_t player, std::int8_t column, std::int8_t row) {
    Frame frame(c, 0x1022e760, 0x40, {.first_gpr = 30});
    Kept<31> retained_row(c, c.r(5)); Kept<30> actor(c, player);
    const auto columns = absolute(c, Player{c, player}.column() - column, 0x1022e790);
    c.set(4, std::uint32_t(std::int32_t(std::int8_t(retained_row))));
    retained_row = std::uint32_t(std::int32_t(columns));
    const auto rows = absolute(c, Player{c, actor}.row() - row, 0x1022e7a8);
    const auto table = c.u32(c.r(2) - 0x6424);
    leave(c, {{3, std::uint32_t(std::int32_t(rows)) * 36}, {4, table}});
    c.set_f(1, c.f32(table + distance_offset(std::int8_t(retained_row), rows)));
    return c.f(1);
}
double distance_to_goal(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10245160, 0x40, {.first_gpr = 30});
    Kept<31> table(c, c.u32(c.r(2) - 0x6424)); Kept<30> actor(c, player);
    const bool second = compare(c, std::int8_t(Player{c, actor}.side())) != 0;
    const auto columns = absolute(c, Player{c, actor}.column() - 4, second ? 0x102451d8 : 0x10245198);
    const auto target_row = Player{c, actor}.row() - (second ? 0 : 11);
    actor = std::uint32_t(std::int32_t(columns));
    const auto rows = absolute(c, target_row, second ? 0x102451ec : 0x102451ac);
    c.set(3, std::uint32_t(std::int32_t(rows)) * 36);
    c.set_f(1, c.f32(table + distance_offset(std::int8_t(actor), rows)));
    return c.f(1);
}
std::uint32_t near_own_goal(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10270ba0, 0x40, {.first_gpr = 30});
    Kept<31> actor(c, player);
    c.set(4, 11);
    const auto team = std::int8_t(Player{c, player}.side());
    c.set(5, team == 0 ? 32 : std::countl_zero(std::uint32_t(std::int32_t(team))));
    Kept<30> target_row(c, team == 0 ? 0 : 11);
    const auto columns = absolute(c, Player{c, actor}.column() - 4, 0x10270be8);
    c.set(4, std::uint32_t(std::int32_t(std::int8_t(target_row))));
    const auto row_delta = Player{c, actor}.row() - std::int8_t(target_row);
    target_row = std::uint32_t(std::int32_t(columns));
    const auto rows = absolute(c, row_delta, 0x10270c00);
    const auto constants = c.u32(c.r(2) - 0x51e8), table = c.u32(c.r(2) - 0x6424);
    leave(c, {{3, constants}, {4, std::uint32_t(std::int32_t(rows)) * 36}, {5, table}});
    c.set_f(1, c.f32(table + distance_offset(std::int8_t(target_row), rows)));
    Fpu fp(c); fp.fcmp(0, c.f(1), c.f64(constants + 0x10));
    c.set_cr(0, c.cr(0) | ((c.cr(0) & 8) ? 2 : 0));
    if (!(c.cr(0) & 2)) {
        leave(c, {{3, Player{c, actor}.match()}, {4, actor}});
        c.call(0x101f0dd0, 0x10270c3c);
        if (compare(c, std::int8_t(c.r(3))) == 0) return result(c, 0);
    }
    return result(c, 1);
}
std::uint32_t can_face(Cpu& c, std::uint32_t player, std::uint32_t column, std::uint32_t row) {
    Frame frame(c, 0x10245050, 0x50, {.first_gpr = 31});
    const auto constants = c.u32(c.r(2) - 0x51e8);
    c.set(8, constants);
    const auto current_column = c.u8(player + 0x105), current_row = c.u8(player + 0x106);
    const auto columns = std::int8_t(column - current_column), rows = std::int8_t(row - current_row);
    leave(c, {{4, std::uint32_t(std::int32_t(columns))}, {5, std::uint32_t(std::int32_t(rows))}, {6, current_column}});
    const bool horizontal = compare(c, columns) != 0;
    if (!horizontal && compare(c, rows) == 0) return result(c, 0);
    const auto table = c.u32(c.r(2) - 0x641c);
    const auto column_offset = std::uint32_t(std::int32_t(columns)) * 48;
    const auto destination = table + column_offset + std::uint32_t(std::int32_t(rows)) * 2;
    const auto target_heading = c.i16(destination + 0x1c8), current_heading = c.i16(player + 0x19e);
    leave(c, {{4, std::uint32_t(std::int32_t(current_heading))}, {5, destination}, {6, std::uint32_t(std::int32_t(target_heading))}, {7, table}});
    if (compare(c, current_heading, target_heading) == 0) return result(c, 1);
    const auto trait = c.i8(player + 0x10f);
    const auto converted = signed_conversion(c, frame.at(0x40), std::uint32_t(std::int32_t(trait)), c.f64(constants + 0x80));
    Fpu fp(c); c.set_f(1, converted); c.set_f(2, c.f64(constants + 0x80)); c.set_f(3, c.f64(constants + 0x1f8));
    const auto threshold = fp.multiply_add(0x10245104, c.f(3), converted, c.f64(constants + 0x338));
    c.put64(frame.at(0x38), fp.fctiwz(0, threshold));
    Kept<31> limit(c, c.u32(frame.at(0x3c)));
    c.set(4, std::uint32_t(std::int32_t(target_heading))); c.set(5, std::uint32_t(std::int32_t(trait)));
    c.set(3, std::uint32_t(std::int32_t(current_heading) - target_heading));
    c.call(0x10775540, 0x10245118);
    auto difference = std::int16_t(c.r(3));
    if (compare(c, difference, 180) > 0) {
        const auto wide = std::uint32_t(360 - std::int32_t(difference));
        c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(difference)) <= 360);
        difference = std::int16_t(wide);
    }
    c.set(3, std::uint32_t(std::int32_t(difference)));
    return result(c, compare(c, difference, std::int16_t(limit)) < 0);
}

void scale_intensity(Cpu& c, std::uint32_t player, double factor) {
    Fpu fp(c);
    const auto intensity = fp.fmuls(c.f32(player + 0x1a8), factor);
    const auto constants = c.u32(c.r(2) - 0x51e8);
    c.put32(c.sp() - 8, 0x43300000);
    c.set_f(1, c.f64(constants + 0x80)); c.put_f32(player + 0x1a8, float(intensity));
    const auto staff = c.u32(player + 0x6c);
    c.set(4, staff); c.set_f(2, c.f32(player + 0x1a8));
    const auto trait = c.i8(staff + 0x69);
    auto floor = std::uint32_t(50) - std::uint32_t(std::int32_t(trait)) * 2;
    c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(trait)) * 2 <= 50);
    c.put32(c.sp() - 4, floor ^ 0x80000000);
    auto minimum = fp.fsubs(c.f64(c.sp() - 8), c.f(1));
    fp.fcmp(0, c.f(2), minimum);
    double selected;
    // PPC ble tests !GT, so an unordered comparison also takes the floor.
    if (!(c.cr(0) & 4)) {
        const auto reread_staff = c.u32(player + 0x6c);
        c.set(5, c.u8(reread_staff + 0x69)); c.set(4, c.u32(c.r(2) - 0x51e8));
        c.set_f(1, c.f64(c.r(4) + 0x80));
        floor = std::uint32_t(50) - std::uint32_t(std::int32_t(std::int8_t(c.r(5)))) * 2;
        c.s.wr<1>(reg::xer_ca, std::uint32_t(std::int32_t(std::int8_t(c.r(5)))) * 2 <= 50);
        c.put32(c.sp() - 16, 0x43300000); c.put32(c.sp() - 12, floor ^ 0x80000000);
        minimum = fp.fsubs(c.f64(c.sp() - 16), c.f(1)); selected = minimum;
    } else selected = c.f32(player + 0x1a8);
    c.put_f32(player + 0x1a8, float(selected));
    c.set(4, c.u32(c.r(2) - 0x51e8)); c.set_f(1, c.f32(player + 0x1a8));
    fp.fcmp(0, c.f(1), c.f64(c.r(4) + 0x3a8));
    if (c.cr(0) & 4) {
        c.set(4, c.u32(c.r(2) - 0x51e0)); c.put32(player + 0x1a8, c.u32(c.r(4) + 0x1c));
    }
}

std::int32_t selection_score(Cpu& c, std::uint32_t player, std::uint32_t opponent) {
    const auto constants = c.u32(c.r(2) - 0x51e8); c.set(5, constants);
    bool same = compare_logical(c, opponent) != 0;
    if (same) same = compare_logical(c, opponent, c.u32(player + 0x1ac)) == 0;
    const auto value = c.f32(player + 0x1a8); c.set_f(1, value);
    const auto trait = c.i8(player + 0x102); c.set(3, std::uint32_t(std::int32_t(trait)));
    Fpu fp(c); auto scaled = fp.fdiv(value, c.f64(constants + 0x3a8));
    if (same) {
        c.put32(c.sp() - 8, 0x43300000); c.put32(c.sp() - 4, std::uint32_t(std::int32_t(trait) + 5) ^ 0x80000000);
        c.set_f(2, c.f64(constants + 0x80)); c.set_f(3, c.f64(constants + 0xc0));
        scaled = fp.fadd(c.f64(constants + 0x30), scaled);
        c.set_f(1, fp.fsub(c.f64(c.sp() - 8), c.f(2)));
        c.set_f(1, fp.fmul(c.f(3), c.f(1))); scaled = fp.fmul(c.f(1), scaled);
        c.put64(c.sp() - 16, fp.fctiwz(0, scaled));
        return std::int32_t(result(c, c.u32(c.sp() - 12)));
    }
    c.put32(c.sp() - 16, 0x43300000); c.put32(c.sp() - 12, std::uint32_t(std::int32_t(trait)) ^ 0x80000000);
    c.set_f(4, c.f64(constants + 0x80)); c.set_f(2, fp.fadd(c.f64(constants + 0x30), scaled));
    c.set_f(1, c.f64(constants + 0x18)); c.set_f(3, fp.fsub(c.f64(c.sp() - 16), c.f(4)));
    const auto combined = fp.multiply_add(0x102876a0, c.f(3), c.f(2), c.f(1));
    c.set_f(1, combined); scaled = fp.fmul(combined, c.f64(constants + 0x28));
    c.put64(c.sp() - 8, fp.fctiwz(0, scaled));
    return std::int32_t(result(c, c.u32(c.sp() - 4)));
}

void improve_condition(Cpu& c, std::uint32_t player) {
    Frame frame(c, 0x10235db0, 0x50, {.first_gpr = 29});
    Kept<31> seed_slot(c, c.u32(c.r(2) - 0x7b90)); Kept<30> actor(c, player);
    const auto begin = c.u32(c.r(2) - 0x7b8c), end_slot = c.u32(c.r(2) - 0x7b88);
    Kept<29> cursor_slot(c, c.u32(c.r(2) - 0x7b84));
    leave(c, {{4, end_slot}, {5, begin}});
    c.put16(player + 0x19c, 103);
    if (compare(c, c.i8(player + 0x2c)) == 0) c.put8(actor + 0x2c, 1);
    const auto next = c.u32(cursor_slot) + 2;
    const bool within = compare_logical(c, next, c.u32(end_slot)) <= 0;
    c.put32(cursor_slot, next); c.set(3, next);
    if (!within) {
        c.put32(cursor_slot, begin); c.call(0x1077b770, 0x10235e18);
        c.put32(seed_slot, c.r(3) & 0xffff);
    }
    const auto sample = c.u16(c.u32(cursor_slot));
    const auto sum = std::uint32_t(sample + c.u32(seed_slot));
    const auto remainder = std::int32_t(sum) % 5;
    const auto constants = c.u32(c.r(2) - 0x51e8);
    c.put32(frame.at(0x38), 0x43300000); c.put32(frame.at(0x3c), (std::uint32_t(remainder) + 5) ^ 0x80000000);
    c.set_f(1, c.f64(constants + 0x80)); c.set_f(2, c.f32(actor + 0x50));
    Fpu fp(c); const auto increment = fp.fsubs(c.f64(frame.at(0x38)), c.f(1));
    const auto condition = fp.fadds(c.f(2), increment);
    c.put_f32(actor + 0x50, float(condition));
    leave(c, {{3, std::uint32_t(remainder)}, {4, 0x66666667}, {5, sum}, {6, sample}});
}
}
