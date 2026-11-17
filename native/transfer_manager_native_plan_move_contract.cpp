// TRANSFER_MANAGER.CPP, block "plan": one player moves to the buyer (0x103efec0), second part: the
// contract ends, the person leaves the old club's lists and the history rows move on.
#include "transfer_manager_native_plan.hpp"
#include "transfer_manager_native_plan_move.hpp"
#include <stdexcept>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
namespace {
constexpr std::uint32_t slot_fee = 0x50, slot_route = 0x54, slot_manager_flag = 0x120;
constexpr std::uint32_t person_contract_club = 0x24, person_slots = 0x98, slot_count = 50;
}

void PlayerMove::end_contract() {
    if (std::int8_t(invoke(c, 0x1014c2e0, 0x103f0724, {club, 1, 1, 0, 0})) != 0) {
        const auto staff = c.u32(club + 0x90);
        if (std::int8_t(invoke(c, 0x1030fbf0, 0x103f0740, {cell_value(-0x7b1c), person, staff})) != 0)
            invoke(c, 0x10310310, 0x103f075c, {cell_value(-0x7b1c), person, c.u32(club + 0x90)});
    }
    const auto flags = c.u32(manager + 0x9a0) + c.u32(person) * 10;
    c.put8(flags, std::uint8_t(c.u8(flags) & 0xef));
    if (invoke(c, 0x10083110, 0x103f0780, {contracts(), person}) == 0) {
        zero_contract_path();
        return;
    }
    if (club != c.u32(person + person_contract_club)) {
        if (invoke(c, 0x100ce320, 0x103f07a4, {club, 0, 1}) != c.u32(person + person_contract_club)) {
            invoke(c, 0x100845b0, 0x103f0958, {contracts(), person});
            const auto club_now = found_club_or_zero(person, 0x103f0964);
            invoke(c, 0x100df360, 0x103f099c, {cell_value(-0x7aa4), person, club_now, club});
            return;
        }
    }
    note_released_person_a();
}

// 0x103f07b0: the person's contract club is the person's next club: its terms are settled and the person is
// moved on through the squad and the old club's lists.
void PlayerMove::note_released_person_a() {
    const auto contract_club_now = found_club_or_zero(person, 0x103f07bc);
    invoke(c, 0x10308350, 0x103f0804, {cell_value(-0x7b3c), person, c.u32(frame.at(slot_fee)), 0, 0, c.u32(contract_club_now), 0});
    released = 1;
    keep(c, 14, released);
    const auto manager_flag = invoke(c, 0x104add10, 0x103f081c, {cell_value(-0x7a98), person, c.u32(person + person_contract_club)});
    c.put8(frame.at(slot_manager_flag), manager_flag);
    invoke(c, 0x100df950, 0x103f0834, {cell_value(-0x7aa4), person});
    invoke(c, 0x10084b10, 0x103f0840, {contracts(), person});
    invoke(c, 0x100c8930, 0x103f0858, {c.u32(person + person_contract_club), person, 0, 1, 0});
    c.put32(person + person_contract_club, found_club_or_zero(person, 0x103f0864));
    const auto found = invoke(c, 0x10082f20, 0x103f0898, {contracts(), person});
    keep(c, 23, found);
    news_flag = found;
    if (std::int8_t(invoke(c, 0x100d12d0, 0x103f08a4, {c.u32(person + person_contract_club)})) != 0) {
        // The contract's second flag (0x02 of its byte +0x38) picks the kind of the call.
        invoke(c, 0x100ce200, (c.u8(found + 0x38) & 2) == 0 ? 0x103f08cc : 0x103f08dc, {person, (c.u8(found + 0x38) & 2) == 0 ? 1u : 2u});
        return;
    }
    const auto list = invoke(c, 0x100ce320, 0x103f08f0, {c.u32(person + person_contract_club), 0, 1});
    keep(c, 23, list);
    news_flag = list;
    if (list == 0) {
        invoke(c, 0x100ce200, 0x103f0948, {person, 1});
        return;
    }
    invoke(c, 0x100ce200, 0x103f0904, {person, 1});
    // The 50 slots of that list: every one that holds the person points the person's contract club at the list.
    std::uint32_t offset = 0;
    for (std::int8_t slot = 0; slot < std::int8_t(slot_count); ++slot, offset += 4)
        if (c.u32(list + person_slots + offset) == person) c.put32(person + person_contract_club, list);
}

// 0x103f09a0: the person has no contract of the first kind.
void PlayerMove::zero_contract_path() {
    const auto found = invoke(c, 0x10082f20, 0x103f09ac, {contracts(), person});
    c.put32(found_cell, found);
    const bool fifth_kind = found != 0 && (c.i8(found + 0x38) >> 2) == 5;
    if (fifth_kind) {
        const auto manager_flag = invoke(c, 0x104add10, 0x103f09f8, {cell_value(-0x7a98), person, c.u32(person + person_contract_club)});
        c.put8(frame.at(slot_manager_flag), manager_flag);
        invoke(c, 0x100c8930, 0x103f0a18, {c.u32(person + person_contract_club), person, 0, 1, 0});
        c.put32(person + person_contract_club, 0);
        return;
    }
    const auto club_now = found_club_or_zero(person, 0x103f0a30);
    invoke(c, 0x100df360, 0x103f0a68, {cell_value(-0x7aa4), person, club_now, club});
}

// 0x103f0a68 .. 0x103f0e60: the person's condition record, the history rows and the lists of the clubs.
void PlayerMove::leave_old_club() {
    const auto record = c.u32(person + 0x14);
    if (record != 0) {
        if (c.i8(person + 0x6d) != 0) {
            const auto constants = c.toc(-0x4ba4);
            c.set_f(1, double(c.f32(constants + 0x2c)));
            invoke(c, 0x10303560, 0x103f0a9c, {cell_value(-0x7b3c), record, 4, 1});
            const double limit = double(c.f32(c.toc(-0x4ba4) + 0x30));
            fpu.fcmp(0, c.f(1), limit);
            c.put8(person + 0x6d, c.f(1) > limit ? 8 : 0);
        } else {
            c.put8(person + 0x6d, 0);
        }
        if (std::int8_t(released) == 0) {
            const auto route_is_one = c.i8(frame.at(slot_route)) == 1 ? 1u : 0u;
            invoke(c, 0x10308350, 0x103f0b00, {cell_value(-0x7b3c), person, c.u32(frame.at(slot_fee)), route_is_one, 0, c.u32(club), free_move});
        }
    }
    // The person's four history rows move one place back; the newest takes the club and the day count.
    if (found_club_or_zero(person, 0x103f0b0c) != 0) {
        const auto rows = [&] { return c.u32(shortlists_cell) + c.u32(person) * 0x64; };
        for (const std::uint32_t at : {0x14u, 0x1cu, 0x24u, 0x2cu}) {
            c.put32(rows() + at, c.u32(rows() + at + 8));
            c.put8(rows() + at + 4, c.u8(rows() + at + 12));
        }
        const auto newest = found_club_or_zero(person, 0x103f0bc4);
        c.put32(rows() + 0x34, c.u32(newest));
        c.put8(rows() + 0x38, std::uint8_t(c.u16(date + 2) - c.u16(person + 0x4e)));
    }
    if (c.u32(person + person_contract_club) != 0) {
        if (std::int8_t(invoke(c, 0x100d3040, 0x103f0c38, {person, 0})) == 0)
            c.put8(c.u32(manager_table() + 0xdc) + c.u32(c.u32(person + person_contract_club)) * 0xc + 9, 100);
        invoke(c, 0x103afb90, 0x103f0c7c, {cell_value(-0x7a6c), c.u32(person + person_contract_club), person, 1, 1});
        invoke(c, 0x103afb90, 0x103f0c98, {cell_value(-0x7a6c), c.u32(person + person_contract_club), person, 0, 1});
        invoke(c, 0x100c8930, 0x103f0cb0, {c.u32(person + person_contract_club), person, 0, 1, 0});
    }
    invoke(c, 0x10430790, 0x103f0cc4, {cell_value(-0x7b20), club, 0});
    invoke(c, 0x100c82a0, 0x103f0ce0, {club, person, c.u8(contract + 0x78), date, 1, 0});
    const auto next_slot = c.u32(contract + 0x7c);
    if (next_slot != 0xffffffff) {
        if (c.u32(person + 0x1c) == 0) c.put32(person + 0x1c, cell_value(-0x79f8) + next_slot * 0x44);
        c.put32(contract + 0x7c, 0xffffffff);
    }
    // What the old club does about the person (by the role the person had), then what the new club does.
    const auto role = std::int8_t(person_kind);
    const auto club_is_open = [&](std::uint32_t target, std::uint32_t back) {
        return std::int8_t(invoke(c, 0x1014c2e0, back, {target, 1, 1, 0, 0})) != 0;
    };
    if (role == 9) {
        if (person_club != 0) {
            if (club_is_open(person_club, 0x103f0d40)) {
                invoke(c, 0x1030f950, 0x103f0d58, {cell_value(-0x7b1c), c.u32(person_club + 0x90)});
            } else {
                invoke_native(c, 0x103f2c00, 0x103f0d74, {manager, person_club, 9});
            }
        }
    } else if (role != 0xb) {
        if (person_club != 0 && club_is_open(person_club, 0x103f0da0)) {
            invoke(c, 0x103d2b30, 0x103f0dbc, {cell_value(-0x7a7c), person_club, 0});
        } else if (person_club != 0) {
            invoke_native(c, 0x103f2c00, 0x103f0dd8, {manager, person_club, person_kind});
        }
    }
    const auto now_role = c.i8(person + 0x62);
    if (now_role == 9) {
        if (club_is_open(club, 0x103f0e00))
            invoke(c, 0x1030f950, 0x103f0e18, {cell_value(-0x7b1c), c.u32(club + 0x90)});
    } else if (now_role != 0xb) {
        const auto old_club = c.u32(person + person_contract_club);
        if (old_club != 0 && std::int8_t(invoke(c, 0x1014c2e0, 0x103f0e44, {old_club, 1, 1, 0, 0})) != 0)
            invoke(c, 0x103d2b30, 0x103f0e60, {cell_value(-0x7a7c), c.u32(person + person_contract_club), 0});
    }
}
}
