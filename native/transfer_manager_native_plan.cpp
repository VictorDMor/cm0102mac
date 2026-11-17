// TRANSFER_MANAGER.CPP, block "plan": the small routines around concluding an agreed offer.
#include "transfer_manager_native_detail.hpp"
#include "transfer_manager_native_plan.hpp"
#include <stdexcept>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t find_contract = 0x10082f20, find_contract_of = 0x1007f720, list_next = 0x10738e00,
                        bsearch_rows = 0x10775600, qsort_rows = 0x10768300, copy_date = 0x10717a10,
                        assert_prepare = 0x10739890, assert_context = 0x10768810, assert_text = 0x1077b720,
                        assert_report = 0x10769b60;
constexpr std::int32_t text_cell = -0x4b9c, report_cell = -0x7ba8, flag_cell = -0x7bac;
constexpr std::uint32_t manager_flags = 0x9a0, manager_rows = 0xa6c, manager_row_count = 0xa70, row_size = 0x10,
                        flag_row = 0x20;

// The original's assertion report for a missing record: the same four calls with its two frame slots.
void report_missing(Cpu& c, const Frame& frame, std::uint32_t slot_a, std::uint32_t slot_b, std::uint32_t line,
                    const std::uint32_t (&backs)[4]) {
    const auto text = c.toc(text_cell), context = c.toc(report_cell);
    invoke(c, assert_prepare, backs[0], {text + 0x37, 0, 0, frame.at(slot_a), frame.at(slot_b)});
    const auto number = invoke(c, assert_context, backs[1]);
    invoke(c, assert_text, backs[2], {context, text + 0x6ff, number, frame.at(slot_a), frame.at(slot_b), line});
    invoke(c, assert_report, backs[3], {text + 0x70c, context, 0});
    c.put8(c.toc(flag_cell), 0);
}
}

void pass_on_person_of_kind(Cpu& c, std::uint32_t, std::uint32_t club, std::uint32_t kind) {
    Frame frame(c, 0x103f2c00, 0x70, {.first_gpr = 30});
    keep(c, 31, c.toc(text_cell));
    keep(c, 30, c.toc(report_cell));
    const auto count_cell = c.toc(-0x7be4);
    if (club == 0) {
        report_missing(c, frame, 0x40, 0x38, 0x274d, {0x103f2c40, 0x103f2c44, 0x103f2c60, 0x103f2c70});
        return;
    }
    // The original dispatches through a table of the compiler's switch (types 6-15).
    const auto type = std::uint32_t(std::int32_t(std::int8_t(kind))) - 6;
    if (type >= 10) return;
    std::uint32_t wanted = kind;
    switch (c.u32(c.toc(-0x4bb8) + type * 4)) {
    case 0x103f2cb8: break;
    case 0x103f2ca4: wanted = 8; break;
    case 0x103f2cac: wanted = 6; break;
    case 0x103f2cb4: return;
    default: throw std::logic_error("pass_on_person_of_kind: unexpected switch target");
    }
    auto person = c.u32(c.toc(toc::people));
    const auto count = c.u32(count_cell);
    if (std::int32_t(count) <= 0) return;
    for (std::uint32_t i = 0; i < count; ++i, person += person_size) {
        if (c.u32(person + 0x24) != 0) continue;
        if (c.i8(person + 0x62) != std::int8_t(wanted)) continue;
        if (c.u32(person + 0x1c) == 0) continue;
        if (c.u32(person + 0xc) != c.u32(club + 0x2c)) continue;
        invoke(c, 0x100c82a0, 0x103f2d28, {club, person, wanted, c.toc(-0x7b80), 1, 0});
        return;
    }
}

void update_person_row(Cpu& c, std::uint32_t manager, std::uint32_t person_index, std::uint32_t club) {
    Frame frame(c, 0x103edef0, 0xb0, {.first_gpr = 24});
    const auto count_cell = c.toc(-0x7be4), compare = c.toc(-0x4ba8), date = c.toc(-0x7b80), report = c.toc(report_cell),
               text = c.toc(text_cell);
    keep(c, 31, text);
    keep(c, 30, report);
    keep(c, 29, date);
    keep(c, 28, compare);
    keep(c, 27, club);
    keep(c, 26, person_index);
    keep(c, 25, manager);
    keep(c, 24, count_cell);
    // The person's row in the sorted row table, only when its status flags say it has one. A null index
    // pointer is read through, as the original does.
    const auto find_row = [&](std::uint32_t key_slot, std::uint32_t back) -> std::uint32_t {
        if (person_index != 0 && std::int32_t(c.u32(person_index)) >= std::int32_t(c.u32(count_cell))) return 0;
        const auto index = c.u32(person_index);
        if (!(c.u8(c.u32(manager + manager_flags) + index * 10) & flag_row)) return 0;
        c.put32(frame.at(key_slot), index);
        return invoke(c, bsearch_rows, back,
                      {frame.at(key_slot), c.u32(manager + manager_rows), c.u32(manager + manager_row_count), row_size, compare});
    };
    if (club != 0 && std::int8_t(invoke(c, 0x103ecef0, 0x103edf2c, {manager, person_index, club, 1})) != 0) {
        const auto row = find_row(0x58, 0x103edf90);
        if (row == 0) {
            // No row yet: the date is copied into the frame and the other routine files the offer.
            invoke(c, copy_date, 0x103ee064, {frame.at(0x38), date, 0x34});
            invoke(c, 0x103ec940, 0x103ee080, {manager, person_index, c.u32(club + 0x2c), c.u16(date + 2), 0, 1});
            return;
        }
        const auto row_club = c.u32(c.toc(-0x7b98)) + c.u32(row + 0xc) * 0x128;
        const auto buyer = c.u32(club + 0x2c);
        // The first test (0x103ecef0, which returns 0 for a club whose +0x2c is 0, 0x103ecf18-0x103ecf28) passed, and
        // nothing between it and this read writes memory: the original's report for a missing buyer (0x103edfb4, line
        // 0x1dc1) cannot be reached.
        if (buyer == 0) throw std::logic_error("0x103edfb4: the buyer record is never missing after 0x103ecef0 accepted the club");
        constexpr std::uint8_t club_kind_free = 0x0e;
        // 0x103ecef0 accepted the club only if the buyer's kind byte is 0xe (0x103ecf94-0x103ecf9c), so the second
        // operand of the original's test (0x103ee024) is always false.
        if (row_club != buyer && c.u8(row_club + 0x118) != club_kind_free)
            c.put16(row + 8, c.u16(date + 2));
        c.put16(row + 4, 0);
        c.put16(row + 6, 0);
        c.put32(row + 0xc, c.u32(c.u32(club + 0x2c) + 0x68));
        return;
    }
    // The same call with the same arguments answered 0 at 0x103edf28 (0x103ecef0 only reads, and writes a flag byte on its
    // own assertion path), so it answers 0 again: the original's early return at 0x103ee0a4 cannot be taken.
    if (club != 0 && std::int8_t(invoke(c, 0x103ecef0, 0x103ee0a0, {manager, person_index, club, 1})) != 0)
        throw std::logic_error("0x103ee0a4: 0x103ecef0 answered differently to the same question");
    // Otherwise the person's row is dropped: its flag is cleared, the last row moves into its place, the table is sorted again.
    const auto row = find_row(0x48, 0x103ee104);
    if (row == 0) return;
    const auto flags = c.u32(manager + manager_flags) + c.u32(person_index) * 10;
    c.put8(flags, std::uint8_t(c.u8(flags) & ~flag_row));
    const auto last = c.u32(manager + manager_rows) + (c.u32(manager + manager_row_count) - 1) * row_size;
    c.put32(row, c.u32(last));
    c.put16(row + 4, c.u16(last + 4));
    c.put16(row + 6, c.u16(last + 6));
    c.put16(row + 8, c.u16(last + 8));
    c.put32(row + 0xc, c.u32(last + 0xc));
    c.put32(manager + manager_row_count, c.u32(manager + manager_row_count) - 1);
    invoke(c, qsort_rows, 0x103ee180, {c.u32(manager + manager_rows), c.u32(manager + manager_row_count), row_size, compare});
}

void conclude_agreed_offer(Cpu& c, std::uint32_t manager, std::uint32_t offer) {
    constexpr std::uint32_t offer_list = 4, offer_club = 8, offer_person = 0xc, list_cursor = 0xc, list_start = 4,
                            person_contract = 0x24, person_slots = 0x98, slot_count = 50;
    Frame frame(c, 0x103efaa0, 0x60, {.first_gpr = 22});
    const auto people_cell = c.toc(toc::people), clubs_cell = c.toc(toc::clubs), contracts_cell = c.toc(toc::contracts),
               found_cell = c.toc(toc::found_contract), squads_cell = c.toc(-0x7a98);
    keep(c, 22, manager);
    keep(c, 23, offer);
    keep(c, 28, people_cell);
    keep(c, 31, clubs_cell);
    keep(c, 29, contracts_cell);
    keep(c, 26, 0);
    keep(c, 27, squads_cell);
    keep(c, 30, found_cell);
    const auto person_at = [&](std::uint32_t index) { return c.u32(people_cell) + index * person_size; };
    const auto club_at = [&](std::uint32_t index) { return c.u32(clubs_cell) + index * club_size; };
    const auto person = person_at(c.u32(offer + offer_person));
    keep(c, 25, person);
    const auto club = club_at(c.u32(offer + offer_club));
    keep(c, 24, club);
    const auto found = invoke(c, find_contract, 0x103efafc, {c.u32(contracts_cell), person});
    c.put32(found_cell, found);
    // The buyer's rival club: the person's contract club, unless the contract's kind (byte >> 2) is 5.
    const bool has_club = found != 0 && (c.i8(found + 0x38) >> 2) != 5;
    const auto contract_club = has_club ? club_at(c.u32(c.u32(found_cell) + 4)) : 0;
    keep(c, 26, contract_club);
    invoke(c, 0x101a2eb0, 0x103efb58, {c.u32(c.toc(-0x79f0)), offer});
    invoke(c, 0x103faba0, 0x103efb64, {manager, offer});
    invoke_native(c, 0x103ef8a0, 0x103efb70, {manager, offer});
    if (contract_club) invoke(c, 0x103f1b30, 0x103efb84, {manager, offer});
    if (c.u8(offer + 0x24) != 1) invoke(c, 0x10385aa0, 0x103efba0, {c.u32(c.toc(-0x7b48)), offer});
    invoke(c, 0x10012fb0, 0x103efbb0, {c.u32(c.toc(-0x7afc)), offer});
    if (const auto list = c.u32(offer + offer_list)) {
        c.put32(list + list_cursor, c.u32(list + list_start));
        // The first listed person's slot in its contract's table of 50 is emptied (a null entry is read through).
        const auto entry = invoke(c, list_next, 0x103efbcc, {c.u32(offer + offer_list)});
        const auto listed = person_at(c.u32(entry));
        for (std::int8_t slot = 0; slot < std::int8_t(slot_count); ++slot) {
            const auto table = c.u32(listed + person_contract);
            if (c.u32(table + person_slots + std::uint32_t(slot) * 4) == listed) {
                c.put32(table + person_slots + std::uint32_t(slot) * 4, 0);
                break;
            }
        }
    }
    if (std::int32_t(c.u32(offer + 0x20)) != 0 && c.u32(person + 0x1c) == 0) invoke(c, 0x104113e0, 0x103efc54, {manager, offer});
    invoke_native(c, 0x103efec0, 0x103efc68, {manager, person, club, offer});
    const auto settle_squad = [&](std::uint32_t target, std::uint32_t back_check, std::uint32_t back_a, std::uint32_t back_b) {
        if (std::int8_t(invoke(c, 0x1014c2e0, back_check, {target, 1, 1, 0, 0})) != 0) return;
        if (std::int8_t(invoke(c, 0x104ae570, back_a, {c.u32(squads_cell), target, 0})) != 0)
            invoke(c, 0x104ad840, back_b, {c.u32(squads_cell), target, 0});
    };
    settle_squad(club, 0x103efc80, 0x103efc98, 0x103efcb0);
    if (const auto list = c.u32(offer + offer_list)) {
        c.put32(list + list_cursor, c.u32(list + list_start));
        for (;;) {
            const auto entry = invoke(c, list_next, 0x103efcf0, {c.u32(offer + offer_list)});
            if (!entry) break;
            invoke_native(c, 0x103efec0, 0x103efce8, {manager, person_at(c.u32(entry)), contract_club, offer});
        }
        settle_squad(contract_club, 0x103efd10, 0x103efd28, 0x103efd40);
    }
    c.put8(offer + 0x34, 0x14);
    invoke(c, 0x103eb4a0, 0x103efd5c, {manager, offer, 10, 0});
    invoke(c, 0x103f6190, 0x103efd68, {manager, offer});
    const auto buyer_person = person_at(c.u32(offer + offer_person));
    keep(c, 24, buyer_person);
    const auto found_again = invoke(c, find_contract, 0x103efd84, {c.u32(contracts_cell), buyer_person});
    c.put32(found_cell, found_again);
    const auto person_club = found_again ? club_at(c.u32(found_again + 4)) : 0;
    keep(c, 25, person_club);
    invoke(c, 0x1036f100, 0x103efdc4, {c.u32(manager + 0x988), club_at(c.u32(offer + offer_club)), buyer_person});
    if (const auto list = c.u32(offer + offer_list)) {
        c.put32(list + list_cursor, c.u32(list + list_start));
        for (;;) {
            const auto entry = invoke(c, list_next, 0x103efe00, {c.u32(offer + offer_list)});
            if (!entry) break;
            invoke(c, 0x1036f100, 0x103efdf8, {c.u32(manager + 0x988), person_club, person_at(c.u32(entry))});
        }
    }
    const auto found_last = invoke(c, find_contract, 0x103efe20, {c.u32(contracts_cell), person_at(c.u32(offer + offer_person))});
    c.put32(found_cell, found_last);
    const auto last_club = found_last ? club_at(c.u32(found_last + 4)) : 0;
    keep(c, 24, last_club);
    const auto mark = [&](std::uint32_t contract) { if (contract) c.put8(contract + 0x64, 4); };
    mark(invoke(c, find_contract_of, 0x103efe54, {c.u32(contracts_cell), c.u32(offer + offer_person), c.u32(offer + offer_club)}));
    if (const auto list = c.u32(offer + offer_list)) {
        c.put32(list + list_cursor, c.u32(list + list_start));
        keep(c, 22, 4);
        for (;;) {
            const auto entry = invoke(c, list_next, 0x103efea4, {c.u32(offer + offer_list)});
            if (!entry) break;
            mark(invoke(c, find_contract_of, 0x103efe90, {c.u32(contracts_cell), c.u32(entry), c.u32(last_club)}));
        }
    }
}
}
