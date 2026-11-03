#include "match_pl_movement_detail.hpp"

namespace cm::aot::match_pl {
using namespace guest;

void PlayerMovement::initialize_cell() {
    c.put16(frame.at(0x48), 0);
    c.s.pc = 0x1022e83c;
    c.set(0, c.u8(actor.address + 0x105));
    c.set(3, std::uint32_t(std::int8_t(c.r(16))));
    c.put8(frame.at(0x116), c.r(0));
    c.set(0, c.u8(actor.address + 0x106));
    c.put8(frame.at(0x115), c.r(0));
    c.set(0, c.u8(frame.at(0x116)));
    c.put8(frame.at(0x40), c.r(4));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    c.set(3, c.u8(actor.address + 0x2c));
    c.set(0, c.r(3) + 1);
    c.put8(actor.address + 0x2c, c.r(0));
    c.set(0, 0);
    c.put8(frame.at(0x114), 0);
}

bool PlayerMovement::stationary_request() {
    if (!(c.cr(0) & 2)) return false;
    c.set(0, c.u8(frame.at(0x115)));
    c.set(3, std::uint32_t(std::int8_t(c.r(17))));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) != 0) return false;
    c.set(0, std::uint32_t(std::int16_t(c.r(18))));
    if (compare(c, std::int32_t(c.r(0)), 15) == 0) return false;
    c.set(0, std::uint32_t(std::int16_t(c.r(18))));
    return compare(c, std::int32_t(c.r(0)), 256) != 0;
}

void PlayerMovement::refill_pool(std::uint32_t back, unsigned first, unsigned second, unsigned second_field) {
    c.set(3, c.u32(c.r(30)));
    c.set(0, c.u32(c.r(29)));
    c.set(3, c.r(3) + 2);
    c.set(first, c.u8(actor.address + 0x107));
    c.set(second, c.u8(actor.address + second_field));
    const bool wrapped = compare_logical(c, c.r(3), c.r(0)) > 0;
    c.put32(c.r(30), c.r(3));
    if (wrapped) {
        c.put32(c.r(30), c.r(28));
        c.call(0x1077b770, back);
        c.set(0, c.r(3) & 0xffff);
        c.put32(c.r(27), c.r(0));
    }
}

std::uint32_t PlayerMovement::movement_draw(std::int32_t magic, unsigned shift, unsigned divisor, bool mixed,
                            unsigned first, unsigned second) {
    c.set(3, c.u32(c.r(30)));
    c.set(4, std::uint32_t(magic));
    c.set(5, c.u32(c.r(27)));
    c.set(6, c.u16(c.r(3)));
    c.set(3, std::uint32_t(std::int8_t(c.r(first))));
    c.set(0, std::uint32_t(std::int8_t(c.r(second))));
    c.set(6, c.r(6) + c.r(5));
    auto high = std::uint32_t((std::int64_t(magic) * std::int32_t(c.r(6))) >> 32);
    if (mixed) high += c.r(6);
    c.s.wr<1>(reg::xer_ca, std::int32_t(high) < 0 && (high & ((1u << shift) - 1)));
    const auto quotient = std::int32_t(high) >> shift;
    c.set(5, unsigned(quotient < 0));
    c.set(4, divisor * std::uint32_t(quotient + (quotient < 0)));
    return c.r(6) - c.r(4);
}

bool PlayerMovement::next_neighbour(unsigned index) {
    c.set(0, c.u8(actor.address + 0x104));
    c.set(3, std::uint32_t(std::int8_t(c.r(index))));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    return compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) < 0;
}

void PlayerMovement::react_neighbours(std::uint32_t query_back, std::uint32_t refill_back, std::uint32_t response_back,
                      unsigned index, unsigned first, unsigned second) {
    c.set(index, 0);
    while (next_neighbour(index)) {
        c.set(0, std::uint32_t(std::int8_t(c.r(index))));
        c.set(3, actor.match());
        c.set(0, c.r(0) * 4);
        c.set(5, c.u8(actor.address + 0x105));
        c.set(4, actor.address + c.r(0));
        c.set(6, c.u8(actor.address + 0x106));
        c.set(4, c.u32(c.r(4) + 0x1b4));
        c.call(0x101d8120, query_back);
        c.set(0, std::uint32_t(std::int8_t(c.r(3))));
        if (compare(c, std::int32_t(c.r(0))) != 0) {
            refill_pool(refill_back, first, second, 0x10a);
            const auto remainder = movement_draw(std::int32_t(0x91a2b3c5u), 9, 900, true, first, second);
            c.set(3, c.r(3) * c.r(0));
            c.set(4, remainder);
            c.set(0, c.r(4) + 50);
            if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) <= 0) {
                c.set(0, std::uint32_t(std::int8_t(c.r(index))) * 4);
                c.set(3, actor.address + c.r(0));
                c.set(3, c.u32(c.r(3) + 0x1b4));
                c.call(0x102836a0, response_back);
            }
        }
        c.set(index, c.r(index) + 1);
    }
}

bool PlayerMovement::carries_ball(std::uint32_t back) {
    c.set(3, actor.address);
    c.call(0x102775c0, back);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    return compare(c, std::int32_t(c.r(0))) != 0;
}

bool PlayerMovement::turns_in_place() {
    if (carries_ball(0x1022e9b4)) return false;
    c.set({{3, actor.address}, {4, c.r(16)}, {5, c.r(17)}});
    c.call(0x10245050, 0x1022e9cc);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) != 0) return false;
    for (unsigned field : {0x10fu, 0x10du, 0x108u}) {
        c.set(0, std::uint32_t(c.i8(actor.address + field)));
        if (compare(c, std::int32_t(c.r(0))) != 0) return false;
    }
    c.set({{4, c.u8(actor.address + 0x105)}, {3, actor.address}, {5, c.u8(actor.address + 0x106)},
           {6, c.r(16)}, {7, c.r(17)}});
    c.call(0x10237830, 0x1022ea10);
    react_neighbours(0x1022ea38, 0x1022ea68, 0x1022ead0);
    c.set(0, 111);
    c.put16(actor.address + 0x19c, c.r(0));
    return true;
}

void PlayerMovement::approach_axis(unsigned coordinate, unsigned target) {
    c.set(3, std::uint32_t(std::int8_t(c.r(coordinate))));
    const int ordering = compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (ordering < 0) c.set(coordinate, c.r(coordinate) + 1);
    else {
        c.set(3, std::uint32_t(std::int8_t(c.r(coordinate))));
        c.set(0, std::uint32_t(std::int8_t(c.r(target))));
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) > 0)
            c.set(coordinate, c.r(coordinate) - 1);
    }
}

void PlayerMovement::approach_row() {
    c.set(0, std::uint32_t(std::int8_t(c.r(17))));
    approach_axis(24, 17);
}

bool PlayerMovement::changed_cell() {
    c.set(3, std::uint32_t(actor.column()));
    c.set(0, std::uint32_t(std::int8_t(c.r(16))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) != 0) return true;
    c.set(3, std::uint32_t(actor.row()));
    c.set(0, std::uint32_t(std::int8_t(c.r(17))));
    return compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) != 0;
}

std::uint32_t PlayerMovement::signed_side() const { return std::uint32_t(std::int8_t(actor.side())); }

std::uint32_t PlayerMovement::cell_index() const { return std::uint32_t(c.i8(actor.address + 0x28)); }

// Preserve the two live match reads and the successive side/slot reads.
// The original fills the departing slot with the old cell's last occupant.
void PlayerMovement::remove_cell_occupant() {
    const auto column = actor.column();
    const auto row = actor.row();
    const auto first_side = signed_side();
    const auto source_list_side = signed_side();
    const auto target_list_side = signed_side();
    const auto index = cell_index();
    c.set(12, actor.match());
    c.set(4, actor.match());
    c.set(11, std::uint32_t(column * 0x450));
    c.set(10, c.r(11) + std::uint32_t(row * 0x5c));
    c.set(5, c.r(12) + c.r(10) + 0xa08);
    c.set(8, std::uint32_t(c.i8(c.r(5) + first_side + 0x58)));
    const auto old_cell = c.r(5);
    const auto last = old_cell + source_list_side * 0x2c + c.r(8) * 4 - 4;
    c.put32(old_cell + target_list_side * 0x2c + index * 4, c.u32(last));

    c.set(9, std::uint32_t(std::int8_t(c.r(16)) * 0x450));
    c.set(8, std::uint32_t(std::int8_t(c.r(17)) * 0x5c));
    c.set(7, old_cell + signed_side());
    c.set(6, c.u8(c.r(7) + 0x58));
    c.put8(c.r(7) + 0x58, c.r(6) - 1);
    c.set(3, c.r(4) + c.r(9) + c.r(8) + 0xa08);
    c.set(4, signed_side() * 0x2c);
    c.set(0, c.u32(old_cell + c.r(4) + cell_index() * 4));
    if (compare_logical(c, c.r(0)) != 0) {
        const auto replacement_side = signed_side();
        const auto replacement_slot = cell_index();
        c.set(6, c.u8(actor.address + 0x28));
        c.set(0, replacement_side * 0x2c + replacement_slot * 4);
        c.set(4, c.u32(old_cell + c.r(0)));
        c.put8(c.r(4) + 0x28, c.r(6));
    }
}

void PlayerMovement::insert_cell_occupant() {
    c.set(4, c.r(3) + signed_side());
    c.set(0, c.u8(c.r(4) + 0x58));
    c.put8(actor.address + 0x28, c.r(0));
    c.set(5, c.r(3) + signed_side());
    c.set(4, c.u8(c.r(5) + 0x58));
    c.set(0, c.r(4) + 1);
    c.put8(c.r(5) + 0x58, c.r(0));
    c.set(4, signed_side() * 0x2c);
    c.set(0, c.r(4) + cell_index() * 4);
    c.put32(c.r(3) + c.r(0), actor.address);
}

void PlayerMovement::migrate_cell() {
    c.set(0, cell_index());
    if (compare(c, std::int32_t(c.r(0)), -1) > 0) remove_cell_occupant();
    else {
        c.set(4, std::uint32_t(std::int8_t(c.r(16)) * 0x450));
        const auto side = signed_side();
        c.set(5, actor.match());
        c.set(0, side);
        c.set(3, c.r(5) + c.r(4) + std::uint32_t(std::int8_t(c.r(17)) * 0x5c) + 0xa08);
    }
    insert_cell_occupant();
}

bool PlayerMovement::update_defensive_extent(unsigned field, int empty_row, bool first_side) {
    c.set(4, actor.match());
    c.set(3, c.r(4) + 0x10000);
    c.set(0, c.u32(c.r(3) + field));
    if (compare_logical(c, c.r(0)) != 0) {
        c.set(3, c.u32(c.r(3) + field));
        c.set(3, c.u8(c.r(3) + 0x106));
    } else c.set(3, std::uint32_t(empty_row));
    c.set(0, std::uint32_t(actor.row()));
    c.set(3, std::uint32_t(std::int8_t(c.r(3))));
    const auto order = compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
    if (!(first_side ? order < 0 : order > 0)) return false;
    c.set(3, actor.match() + 0x10000);
    c.put32(c.r(3) + field, actor.address);
    return true;
}

void PlayerMovement::publish_defensive_cell() {
    c.put8(actor.address + 0x105, c.r(16));
    c.put8(actor.address + 0x106, c.r(17));
    c.set(0, signed_side());
    if (compare(c, std::int32_t(c.r(0))) == 0 && update_defensive_extent(-0x1fec, -1, true)) return;
    c.set(0, actor.side());
    if (compare(c, std::int32_t(c.r(0)), 1) == 0) update_defensive_extent(-0x1fe8, 9, false);
}

void PlayerMovement::select_surface_pace() {
    c.set(3, actor.match());
    const auto surface = int(c.i8(c.r(3) + 1));
    c.set(0, std::uint32_t(surface));
    unsigned coefficient = 0x34;
    switch (surface) {
    case 1: coefficient = 0x98; break;
    case 2: coefficient = 0x9c; break;
    case 4: case 16: coefficient = 0x18; break;
    case 8: coefficient = 0xa0; break;
    case 32: coefficient = 0xa4; break;
    case 64: coefficient = 0x78; break;
    }
    c.set_f(31, c.f32(c.r(31) + coefficient));
    c.set(3, actor.match());
    const auto weather = int(c.i8(c.r(3) + 0x30d8));
    c.set(0, std::uint32_t(weather));
    if (compare(c, weather, 3) == 0) {
        c.set_f(0, c.f32(c.r(31) + 0x18));
        c.set_f(31, fp.fmuls(c.f(31), c.f(0)));
    } else if (weather > 3 && compare(c, weather, 5) < 0) {
        c.set_f(0, c.f32(c.r(31) + 0x78));
        c.set_f(31, fp.fmuls(c.f(31), c.f(0)));
    }
}

void PlayerMovement::apply_pace_coefficient(unsigned divisor, unsigned coefficient) {
    c.set_f(1, c.f32(actor.address + 0xd8));
    c.set_f(0, c.f64(c.r(26) + divisor));
    c.set_f(2, c.f64(c.r(26) + coefficient));
    c.set_f(0, fp.fdiv(c.f(1), c.f(0)));
    c.set_f(0, fp.fsub(c.f(2), c.f(0)));
    c.set_f(0, fp.frsp(c.f(0)));
    c.set_f(30, fp.fmuls(c.f(30), c.f(0)));
}

void PlayerMovement::measure_move_pace() {
    c.set_f(2, c.f32(actor.address + 0xd8));
    c.set_f(1, c.f64(c.r(26) + 0x28));
    c.set_f(0, c.f64(c.r(26) + 0xd0));
    c.set(0, c.u8(frame.at(0x116)));
    c.set_f(1, fp.multiply_add(0x102352b8, c.f(2), c.f(1), c.f(0)));
    c.set_f(0, c.f64(c.r(26) + 0x48));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(0, c.u8(actor.address + 0x105));
    c.set_f(2, c.f64(c.r(26) + 0x18));
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set_f(1, fp.frsp(c.f(1)));
    c.set_f(3, c.f64(c.r(26) + 0xc8));
    c.set(3, c.r(0) - c.r(3));
    c.set_f(0, fp.fdiv(c.f(1), c.f(0)));
    c.set_f(0, fp.fsub(c.f(2), c.f(0)));
    c.set_f(30, fp.fadd(c.f(3), c.f(0)));
    c.set_f(30, fp.frsp(c.f(30)));
    c.call(0x10775540, 0x102352f0);
    c.set(0, c.u8(frame.at(0x115)));
    c.set(13, std::uint32_t(std::int8_t(c.r(3))));
    c.set(4, c.u8(actor.address + 0x106));
    c.set(5, std::uint32_t(std::int8_t(c.r(0))));
    c.set(0, std::uint32_t(std::int8_t(c.r(4))));
    c.set(3, c.r(0) - c.r(5));
    c.call(0x10775540, 0x1023530c);
    c.set(3, std::uint32_t(std::int8_t(c.r(3))));
    c.set(0, std::uint32_t(std::int8_t(c.r(13))));
    c.set(4, c.r(3) * 36);
    c.set(5, c.toc(-0x6424));
    c.set(0, c.r(0) * 4);
    c.set(3, actor.address);
    c.set(0, c.r(4) + c.r(0));
    c.set_f(0, c.f32(c.r(5) + c.r(0)));
    c.set_f(0, fp.fmuls(c.f(30), c.f(0)));
    c.set_f(30, fp.fmuls(c.f(31), c.f(0)));
    c.call(0x10289560, 0x10235338);
    c.set(0, c.r(3));
    c.set(3, actor.address);
    c.set(13, std::uint32_t(std::int16_t(c.r(0))));
    c.call(0x102895e0, 0x10235348);
    c.set(0, std::uint32_t(std::int16_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(13))) == 0) return;
    c.set(3, actor.address);
    c.call(0x1028ac50, 0x1023535c);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0)), 1) > 0) {
        apply_pace_coefficient(0x38, 0x168);
        return;
    }
    c.set(3, actor.address);
    c.call(0x10289560, 0x10235390);
    c.set(0, c.r(3) & 4);
    bool role = compare(c, std::int32_t(c.r(0))) != 0;
    if (!role) {
        c.set(3, actor.address);
        c.call(0x102895e0, 0x102353a0);
        c.set(0, c.r(3) & 4);
        role = compare(c, std::int32_t(c.r(0))) != 0;
    }
    apply_pace_coefficient(0x38, role ? 0x290 : 0x190);
}

void PlayerMovement::query_pace_tactics(std::uint32_t back) {
    c.set(0, actor.side());
    c.set(5, actor.match());
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(4, c.u8(actor.address + 0x19));
    c.set(3, c.r(5) + c.r(0) * 0x18e0 + 0x7c6c);
    c.call(0x10113e10, back);
}

bool PlayerMovement::pace_needs_tactics() {
    query_pace_tactics(0x10235404);
    c.set({{6, c.r(4) & 0x8000}, {7, 0}, {5, 0}, {4, 0}});
    c.set(3, c.r(6));
    c.set(0, c.r(3));
    if (compare(c, std::int32_t(c.r(0))) != 0) return true;
    query_pace_tactics(0x10235450);
    c.set({{6, c.r(4) & 0x2000}, {4, 0}, {5, 0}});
    c.set(3, c.r(6));
    c.set(0, c.r(3));
    if (compare(c, std::int32_t(c.r(0))) == 0) return false;
    c.set(0, actor.side());
    c.set(4, actor.match());
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(3, c.r(4) + c.r(0) * 0x18e0 + 0x7c6c);
    c.set(0, c.u32(c.r(3) + 0x588) & 0x100);
    return compare(c, std::int32_t(c.r(0))) != 0;
}

void PlayerMovement::describe_capture(unsigned message, std::uint32_t back) {
    c.set({{3, actor.match()}, {4, message}, {7, actor.side()}, {8, UINT32_MAX}});
    c.set(3, c.u32(c.r(3) + 0x79f4));
    c.set({{9, UINT32_MAX}, {6, c.u8(actor.address + 0x2f)}, {10, 0}, {5, actor.match()}});
    c.call(0x10201180, back);
}

void PlayerMovement::acquire_and_reward(unsigned points, std::uint32_t back) {
    c.set(3, actor.address);
    c.call(0x1022e3a0, back);
    c.set(3, std::uint32_t(c.i16(actor.address + 0x36)));
    c.set(0, c.r(3) + points);
    c.put16(actor.address + 0x36, c.r(0));
}

bool PlayerMovement::exceptional_goal_capture() {
    c.set(3, actor.match());
    c.set(6, c.u8(actor.address + 0x106));
    c.set(0, std::uint32_t(c.i8(c.r(3) + 0x1e6)));
    c.set(5, c.u8(actor.address + 0x105));
    const bool loose = compare(c, std::int32_t(c.r(0))) != 0;
    c.set(4, actor.side());
    const auto side = int(std::int8_t(c.r(4)));
    const auto column = int(std::int8_t(c.r(5)));
    const auto row = int(std::int8_t(c.r(6)));
    if (loose && side == 0 && row >= 10 && column >= 3 && column <= 5) return true;
    c.set(0, std::uint32_t(c.i8(c.r(3) + 0x1e6)));
    return compare(c, std::int32_t(c.r(0))) != 0 && side == 1 && row <= 1 && column >= 3 && column <= 5;
}

bool PlayerMovement::loose_ball_in_cell() {
    c.set(3, actor.match());
    c.set(0, std::uint32_t(c.i8(c.r(3) + 0x791f)));
    if (compare(c, std::int32_t(c.r(0)), 5) >= 0) return false;
    c.set(3, actor.match());
    c.set(0, std::uint32_t(c.i8(c.r(3) + 0x1de)));
    if (compare(c, std::int32_t(c.r(0))) != 0) return false;
    c.set(3, actor.match());
    c.set(0, std::uint32_t(c.i8(c.r(3) + 0x7924)));
    if (compare(c, std::int32_t(c.r(0)), -1) != 0) return false;
    for (const unsigned field : {0x105u, 0x106u}) {
        c.set(3, actor.match());
        c.set(0, c.u8(actor.address + field));
        c.set(4, c.u8(c.r(3) + (field == 0x105 ? 0x791d : 0x791e)));
        c.set(3, std::uint32_t(std::int8_t(c.r(0))));
        c.set(0, std::uint32_t(std::int8_t(c.r(4))));
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) != 0) return false;
    }
    for (const unsigned field : {0xdff8u, 0xdff4u}) {
        c.set(3, actor.match() + 0x10000);
        c.set(0, c.u32(c.r(3) + field - 0x10000));
        if (compare_logical(c, actor.address, c.r(0)) == 0) return false;
    }
    return true;
}

void PlayerMovement::capture_unowned_ball() {
    if (!loose_ball_in_cell()) return;
    c.set(3, actor.match() + 0x10000);
    c.set(0, c.u32(c.r(3) - 0x2008));
    if (compare_logical(c, c.r(0)) != 0) {
        c.set(3, actor.match() + 0x10000);
        c.set(3, c.u32(c.r(3) - 0x2008));
        c.call(0x10283690, 0x10235610);
        c.set(0, signed_side());
        c.set(3, std::uint32_t(std::int8_t(c.r(3))));
        if (compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(3))) == 0) return;
        describe_capture(0x1f60, 0x1023564c);
        acquire_and_reward(28, 0x10235654);
        return;
    }
    const bool first_side = signed_side() == 0;
    c.set({{4, c.u8(actor.address + 0x105)}, {3, actor.address}, {5, c.u8(actor.address + 0x106)},
           {6, 4}, {0, first_side ? 11u : 0u}, {7, first_side ? 11u : 0u}});
    c.call(0x10237830, 0x10235698);
    const bool by_goal = exceptional_goal_capture();
    c.set(0, unsigned(by_goal));
    compare(c, std::int32_t(c.r(0)));
    describe_capture(by_goal ? 0x2113 : 0x1f46, by_goal ? 0x10235760 : 0x1023578c);
    c.set(4, actor.match());
    c.set(0, 0);
    c.set(3, actor.address);
    c.put8(c.r(4) + 0x791f, 0);
    acquire_and_reward(4, 0x102357a0);
}

int PlayerMovement::read_ball_coordinate(unsigned field) {
    c.set(3, actor.match());
    c.set(0, std::uint32_t(c.i8(c.r(3) + field)));
    return std::int32_t(c.r(0));
}

void PlayerMovement::synchronize_ball() {
    if (!carries_ball(0x102354d0)) {
        c.set(0, std::uint32_t(c.i8(frame.at(0x114))));
        if (compare(c, std::int32_t(c.r(0))) == 0) {
            capture_unowned_ball();
            return;
        }
    }
    c.set(0, c.u8(actor.address + 0x105));
    c.set(3, actor.match());
    c.put8(c.r(3) + 0x791d, c.r(0));
    c.set(0, c.u8(actor.address + 0x106));
    c.set(3, actor.match());
    c.put8(c.r(3) + 0x791e, c.r(0));
    if (compare(c, read_ball_coordinate(0x791d)) < 0 || compare(c, read_ball_coordinate(0x791e)) < 0 ||
        compare(c, read_ball_coordinate(0x791e), 12) >= 0 || compare(c, read_ball_coordinate(0x791d), 9) >= 0) {
        c.set({{3, actor.match()}, {4, actor.side()}});
        c.call(0x101f0c00, 0x10235550);
    }
}

std::int32_t PlayerMovement::focus_high(std::uint32_t value) {
    return std::int32_t((std::int64_t(std::int32_t(value)) * 0x55555556) >> 32);
}

void PlayerMovement::face_ball(bool far_goal, std::uint32_t back) {
    if (far_goal) {
        const auto match_for_row = actor.match();
        const auto match_for_column = actor.match();
        const auto ball_row = std::uint32_t(c.i8(match_for_row + 0x791e));
        const auto ball_column = std::uint32_t(c.i8(match_for_column + 0x791d));
        const auto column_sum = std::uint32_t(std::int8_t(c.r(16))) + ball_column;
        const auto row_sum = std::uint32_t(std::int8_t(c.r(17))) + ball_row + 11;
        c.set(4, c.u8(actor.address + 0x105));
        c.set(8, column_sum + 4);
        c.set(5, c.u8(actor.address + 0x106));
        c.set(9, 0x55555556);
        c.set(6, std::uint32_t(focus_high(c.r(8))));
        c.set(7, std::uint32_t(focus_high(row_sum)));
    } else {
        const auto match_for_column = actor.match();
        const auto match_for_row = actor.match();
        const auto ball_column = std::uint32_t(c.i8(match_for_column + 0x791d));
        const auto ball_row = std::uint32_t(c.i8(match_for_row + 0x791e));
        const auto column_sum = std::uint32_t(std::int8_t(c.r(16))) + ball_column;
        const auto row_sum = std::uint32_t(std::int8_t(c.r(17))) + ball_row;
        c.set(4, c.u8(actor.address + 0x105));
        c.set(5, c.u8(actor.address + 0x106));
        c.set(8, std::uint32_t(focus_high(column_sum + 4)));
        c.set(6, c.r(8));
        c.set(7, std::uint32_t(focus_high(row_sum)));
    }
    c.set(3, actor.address);
    c.set(6, std::uint32_t(std::int8_t(c.r(6) + (c.r(6) >> 31))));
    c.set(0, c.r(7) + (c.r(7) >> 31));
    c.set(7, std::uint32_t(std::int8_t(c.r(0))));
    c.call(0x10237830, back);
}

bool PlayerMovement::same_ball_side_or_target() {
    c.set(3, actor.match());
    c.set(0, actor.side());
    c.set(4, c.u8(c.r(3) + 0x7924));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(0, std::uint32_t(std::int8_t(c.r(4))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0) return true;
    c.set(3, actor.match() + 0x10000);
    c.set(0, c.u32(c.r(3) - 0x2008));
    if (compare_logical(c, c.r(0)) == 0) return false;
    c.set(3, actor.match());
    c.set(0, actor.side());
    c.set(3, c.r(3) + 0x10000);
    c.set(4, c.u32(c.r(3) - 0x2008));
    c.set(3, std::uint32_t(std::int8_t(c.r(0))));
    c.set(0, std::uint32_t(c.i8(c.r(4) + 0x27)));
    return compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0;
}

void PlayerMovement::face_movement_context() {
    c.set(0, signed_side());
    const bool away = compare(c, std::int32_t(c.r(0))) != 0;
    if (carries_ball(away ? 0x10235aac : 0x102357c0)) return;
    c.set(0, actor.side());
    c.set(3, actor.match());
    c.set(0, std::uint32_t(std::int8_t(c.r(0))));
    c.set(4, c.u8(c.r(3) + 0x7924));
    c.set(3, c.r(0) == 0);
    c.set(0, std::uint32_t(std::int8_t(c.r(4))));
    if (compare(c, std::int32_t(c.r(3)), std::int32_t(c.r(0))) == 0) {
        face_ball(away, away ? 0x10235b4c : 0x1023585c);
        return;
    }
    c.set(3, actor.match() + 0x10000);
    c.set(0, c.u32(c.r(3) - 0x2008));
    if (compare_logical(c, actor.address, c.r(0)) == 0) {
        face_ball(!away, away ? 0x10235bd0 : 0x102358e4);
        return;
    }
    if (same_ball_side_or_target()) {
        face_ball(!away, away ? 0x10235c94 : 0x102359ac);
        return;
    }
    c.set(3, actor.address);
    c.call(0x102721a0, away ? 0x10235ca0 : 0x102359b8);
    c.set(0, std::uint32_t(std::int8_t(c.r(3))));
    if (compare(c, std::int32_t(c.r(0))) != 0) face_ball(away, away ? 0x10235d18 : 0x10235a2c);
    else face_ball(!away, away ? 0x10235d88 : 0x10235aa0);
}

MovementPhase PlayerMovement::prepare() {
    initialize_cell();
    if (stationary_request()) {
        react_neighbours(0x1022e8cc, 0x1022e8fc, 0x1022e964);
        return MovementPhase::returned;
    }
    if (!carries_ball(0x1022e988)) {
        c.set({{3, actor.match()}, {4, actor.address}, {5, c.r(18)}, {6, 0}, {7, UINT32_MAX}, {8, 1}});
        c.call(0x101f5350, 0x1022e9ac);
    }
    if (turns_in_place()) return MovementPhase::returned;

    c.set(23, c.u8(actor.address + 0x105));
    c.set(0, std::uint32_t(std::int8_t(c.r(16))));
    c.set(24, c.u8(actor.address + 0x106));
    approach_axis(23, 16);
    approach_row();
    if (!carries_ball(0x1022eb5c)) {
        refill_pool(0x1022eb8c, 21, 22, 0x107);
        const auto remainder = movement_draw(0x10624dd3, 6, 1000, false, 21, 22);
        c.set(0, c.r(3) * c.r(0));
        c.set(4, remainder);
        if (compare(c, std::int32_t(c.r(0)), std::int32_t(c.r(4))) > 0) {
            c.set(0, std::uint32_t(std::int8_t(c.r(16))));
            approach_axis(23, 16);
            approach_row();
        }
    }
    c.set({{16, c.r(23)}, {17, c.r(24)}});
    return carries_ball(0x1022ec40) ? MovementPhase::carrier : MovementPhase::noncarrier;
}

void PlayerMovement::move_without_ball() {
    c.s.pc = 0x10234e40;
    if (changed_cell()) migrate_cell();
    publish_defensive_cell();
    react_neighbours(0x10235110, 0x10235140, 0x102351a8, 18, 13, 14);
}

void PlayerMovement::consume_move_condition() {
    c.set(0, std::uint32_t(c.i8(actor.address + 0x19)));
    if (compare(c, std::int32_t(c.r(0))) < 0) return;
    select_surface_pace();
    measure_move_pace();
    if (pace_needs_tactics()) apply_pace_coefficient(0x2a0, 0x298);
    c.set_f(1, c.f(30));
    c.set({{3, actor.address}, {5, UINT32_MAX}});
    c.call(0x10239590, 0x102354c8);
}

void PlayerMovement::finish_without_ball() {
    synchronize_ball();
    face_movement_context();
}
}
