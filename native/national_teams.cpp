#include "national_teams_adapter.hpp"
#include "guest_abi.hpp"
#include "routine_check.hpp"

#include <cstdlib>

namespace cm::aot {
namespace {
using guest::Cpu;
bool declining = std::getenv("CM_NATIONAL_TEAMS_ORIGINAL") != nullptr;
constexpr auto selection_context_entry = 0x102ae9c0u;
constexpr auto selection_flag_entry = 0x102aea20u, clear_selection_flag_entry = 0x102aea50u;
constexpr auto selection_value_entry = 0x102af620u, selection_slot_entry = 0x102af650u;
constexpr auto assigned_staff_entry = 0x102b7450u;
constexpr unsigned nation_record_size = 0x74, selection_slot_size = 0x14, staff_record_size = 0x6c;
constexpr unsigned nation_index_offset = 0x68, selection_slots_offset = 0x40;
constexpr unsigned selection_value_offset = 0xa, selection_flag_offset = 0xb;
constexpr int unassigned = -1;

void block(Cpu& c, std::uint32_t pc, unsigned count) {
    c.s.step(pc);
    c.s.charge(count - 1);
}

bool record_nonzero_byte(Cpu& c, std::uint32_t raw) {
    const auto value = std::int8_t(raw);
    c.set(0, std::uint32_t(value));
    const auto high = c.cr(0) & 0xf0;
    c.compare(0, value, 0);
    c.set_cr(0, high | c.cr(0));
    return value != 0;
}

void selection_context(Cpu& c) {
    GuestNativeScope scope(c.s, selection_context_entry);
    block(c, selection_context_entry, 7);
    const auto caller = c.sp(), frame = caller - 0x40, mode = c.r(5);
    c.set(0, c.lr());
    c.put32(caller - 4, c.r(31));
    c.set({{31, mode}, {3, c.r(4)}});
    c.put32(caller + 8, c.r(0));
    c.put32(frame, caller);
    c.set(1, frame);
    scope.frame(frame);
    c.call(0x100ce250, 0x102ae9dc);

    block(c, 0x102ae9dc, 2);
    const bool national = record_nonzero_byte(c, c.r(3));
    bool alternate = true;
    if (!national) {
        block(c, 0x102ae9e4, 2);
        alternate = record_nonzero_byte(c, mode);
    }
    if (national || alternate) {
        block(c, 0x102ae9f4, 2);
        if (record_nonzero_byte(c, mode)) {
            block(c, 0x102ae9fc, 2);
            c.set(3, 1);
        } else {
            block(c, 0x102aea04, 1);
            c.set(3, 2);
        }
    } else {
        block(c, 0x102ae9ec, 2);
        c.set(3, 0);
    }

    block(c, 0x102aea08, 5);
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.set_lr(c.r(0));
    c.set(31, c.u32(caller - 4));
    c.finish();
}

enum class SelectionAccess { flag, clear_flag, value, address };

template<SelectionAccess Access>
void selection_slot(Cpu& c) {
    constexpr auto entry = Access == SelectionAccess::flag ? selection_flag_entry :
        Access == SelectionAccess::clear_flag ? clear_selection_flag_entry :
        Access == SelectionAccess::value ? selection_value_entry : selection_slot_entry;
    block(c, entry, Access == SelectionAccess::clear_flag ? 10 : 9);
    const auto nation = c.u32(c.r(4) + nation_index_offset);
    c.set(4, nation);
    const auto displacement = std::uint32_t(std::int8_t(c.r(5))) * selection_slot_size;
    c.set(0, displacement);
    const auto records = c.u32(c.r(3) + 4);
    c.set(5, records);
    const auto nation_offset = nation * nation_record_size;
    if constexpr (Access == SelectionAccess::address) {
        c.set(3, records + nation_offset + displacement + selection_slots_offset);
    } else {
        const auto slot = records + nation_offset + displacement;
        c.set(3, slot);
        if constexpr (Access == SelectionAccess::clear_flag) {
            c.set(6, 0);
            c.put8(slot + selection_slots_offset + selection_flag_offset, 0);
        } else {
            constexpr auto field = Access == SelectionAccess::flag ? selection_flag_offset : selection_value_offset;
            c.set(3, c.u8(slot + selection_slots_offset + field));
        }
    }
    c.finish();
}

void assigned_staff(Cpu& c) {
    block(c, assigned_staff_entry, 2);
    const auto nation = c.r(4);
    c.compare_logical(0, nation, 0);
    if (nation) {
        block(c, 0x102b7458, 7);
        const auto nation_id = c.u32(nation + nation_index_offset);
        c.set(0, nation_id);
        const auto records = c.u32(c.r(3) + 4);
        c.set(3, records);
        const auto nation_offset = nation_id * nation_record_size;
        c.set(0, nation_offset);
        c.set(3, records + nation_offset);
        const auto staff = c.u32(records + nation_offset + 4);
        c.set(0, staff);
        c.compare(0, std::int32_t(staff), unassigned);
        if (std::int32_t(staff) > unassigned) {
            block(c, 0x102b7474, 5);
            const auto slot = c.toc(-0x7bbc);
            c.set(3, slot);
            c.set(0, staff * staff_record_size);
            c.set(3, c.u32(slot) + c.r(0));
            c.finish();
            return;
        }
    }
    block(c, 0x102b7488, 2);
    c.set(3, 0);
    c.finish();
}
}

void national_teams_run_original(State& state, std::uint32_t entry) {
    run_original_body(state, entry, declining);
}

void national_teams_run_rewrite(State& state, std::uint32_t entry) {
    Cpu c(state);
    switch (entry) {
    case selection_context_entry: selection_context(c); break;
    case selection_flag_entry: selection_slot<SelectionAccess::flag>(c); break;
    case clear_selection_flag_entry: selection_slot<SelectionAccess::clear_flag>(c); break;
    case selection_value_entry: selection_slot<SelectionAccess::value>(c); break;
    case selection_slot_entry: selection_slot<SelectionAccess::address>(c); break;
    case assigned_staff_entry: assigned_staff(c); break;
    default: throw std::logic_error("Unknown NATIONAL_TEAMS entry " + hex_address(entry));
    }
}

bool try_modern_national_teams(State& state, std::uint32_t entry) {
    if (declining || (routine_checker && !routine_checker->wants(entry))) return false;
    if (routine_checker)
        routine_checker->check(state, entry, national_teams_run_original, national_teams_run_rewrite,
                               RoutineChecker::result_r3);
    national_teams_run_rewrite(state, entry);
    return true;
}
}
