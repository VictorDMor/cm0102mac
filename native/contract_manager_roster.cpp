#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
std::uint32_t queried_club(Cpu& c, Program& program, std::uint32_t person, std::uint32_t managers,
                           std::uint32_t current_contract, std::uint32_t clubs, std::uint32_t back,
                           unsigned output_register) {
    const auto contract = program.call(0x10082f20, back, {c.u32(managers), person});
    const bool present = nonzero_logical(c, contract); c.put32(current_contract, contract);
    std::uint32_t club = 0;
    if (present) {
        const auto id = c.u32(contract + 4); c.set(0, id);
        const auto base = c.u32(clubs); c.set(3, base); c.set(0, id * 0x248);
        club = base + id * 0x248;
    }
    c.set(output_register, club); return club;
}
bool valid_roster_person(Cpu& c, std::uint32_t person, std::uint32_t count_holder) {
    if (!nonzero_logical(c, person)) return false;
    const auto id = c.i32(person), count = c.i32(count_holder);
    c.set(3, std::uint32_t(id)); c.set(0, std::uint32_t(count));
    return compare(c, id, count) < 0;
}
}

void end_secondary_contract(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address, std::uint32_t club_address) {
    Frame frame(c, 0x1007fb20, 0x90, {.first_gpr = 22});
    Kept<28> interest(c, c.toc(-0x7bf0)); Kept<25> object(c, manager_address);
    Kept<29> managers(c, c.toc(-0x7b7c)); Kept<26> person(c, person_address);
    Kept<30> current_contract(c, c.toc(-0x7b78)); Kept<27> club(c, club_address);
    Kept<31> clubs(c, c.toc(-0x7be8)); Kept<22> error_buffer(c, c.toc(-0x7ba8));
    Kept<23> pool(c, c.toc(slot::pool)); Kept<24> contract(c, c.r(24)); Program program(c);
    contract = program.call(0x10083110, 0x1007fb58);
    if (!nonzero(c, std::int32_t(std::uint32_t(contract)))) {
        program.call(0x10739890, 0x1007fb78, {pool + 0x37, 0, 0, frame.at(0x48), frame.at(0x40)});
        const auto version = program.call(0x10768810, 0x1007fb7c);
        program.call(0x1077b720, 0x1007fb98, {error_buffer, pool + 0x23b, version, frame.at(0x48), frame.at(0x40), 0xe35});
        program.call(0x10769b60, 0x1007fba8, {pool + 0x248, error_buffer, 0});
        const auto enabled = c.toc(-0x7bac); c.set(3, enabled); c.set(0, 0); c.put8(enabled, 0); return;
    }
    program.call(0x10717890, 0x1007fbc8, {frame.at(0x38), c.toc(-0x7b80), 1});
    const auto year = c.u16(frame.at(0x3a)); c.set(0, year); c.set(3, 7); c.set(4, person); c.put16(contract + 0x32, year);
    const auto day = c.i16(frame.at(0x38)); c.set(0, extended(day)); c.put16(contract + 0x30, extended(day));
    const auto leap = c.u8(frame.at(0x3c)); c.set(0, leap); c.put8(contract + 0x34, leap);
    const auto flags = (c.u8(contract + 0x38) & 0x03u) | (7u << 2);
    c.set(0, flags); c.put8(contract + 0x38, flags);
    const auto employer = queried_club(c, program, person, managers, current_contract, clubs, 0x1007fbfc, 0);
    if (compare_logical(c, club, employer) == 0) {
        const auto attached = c.u32(person + 0x24); c.set(4, attached);
        if (!nonzero_logical(c, attached)) return;
        const auto interested = std::int8_t(program.call(0x102cdbf0, 0x1007fc3c, {c.u32(interest)}));
        c.set(0, std::uint32_t(std::int32_t(interested)));
        if (!nonzero(c, interested)) return;
        program.call(0x10091630, 0x1007fc54, {object, person, club});
    } else {
        const auto repeated = queried_club(c, program, person, managers, current_contract, clubs, 0x1007fc64, 0);
        if (!nonzero_logical(c, repeated)) return;
        queried_club(c, program, person, managers, current_contract, clubs, 0x1007fc9c, 4);
        const auto interested = std::int8_t(program.call(0x102cdbf0, 0x1007fcc8, {c.u32(interest)}));
        c.set(0, std::uint32_t(std::int32_t(interested)));
        if (!nonzero(c, interested)) return;
        program.call(0x10091630, 0x1007fce0, {object, person, club});
    }
}

void mark_expiring_roster(Cpu& c, std::uint32_t manager_address, std::uint32_t club_address, std::uint32_t context_value) {
    Frame frame(c, 0x1007fd00, 0x80, {.first_gpr = 21});
    Kept<22> club(c, club_address); const bool has_club = nonzero(c, std::int32_t(club_address));
    Kept<29> transfers(c, c.toc(-0x7b40)); Kept<30> staff_count(c, c.toc(slot::person_count));
    Kept<21> object(c, manager_address); Kept<31> today(c, c.toc(-0x7b80)); Kept<23> context(c, context_value);
    Kept<24> alternate(c, c.r(24)); Kept<25> contract(c, c.r(25)); Kept<26> roster(c, c.r(26));
    Kept<27> pass(c, c.r(27)); Kept<28> index(c, c.r(28)); Program program(c);
    if (!has_club) return;
    alternate = program.call(0x100ce320, 0x1007fd3c, {club, frame.at(0x38), 1});
    if (nonzero(c, std::int32_t(std::uint32_t(alternate)))) {
        const auto flag = c.i8(frame.at(0x38)); c.set(0, std::uint32_t(std::int32_t(flag)));
        if (!nonzero(c, flag)) return;
    }
    for (pass = 0;; pass = std::uint32_t(pass) + 1) {
        c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(pass)))));
        if (compare(c, std::int8_t(std::uint32_t(pass)), 2) >= 0) return;
        if (compare(c, std::int8_t(std::uint32_t(pass)), 1) == 0) club = alternate;
        if (!nonzero_logical(c, club)) continue;
        roster = club + 0x98;
        for (index = 0;; index = std::uint32_t(index) + 1, roster = std::uint32_t(roster) + 4) {
            c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(index)))));
            if (compare(c, std::int8_t(std::uint32_t(index)), 50) >= 0) break;
            const auto person = c.u32(roster); c.set(4, person);
            if (!valid_roster_person(c, person, staff_count)) continue;
            contract = program.call(0x10082f20, 0x1007fda4, {object});
            if (!nonzero(c, std::int32_t(std::uint32_t(contract)))) continue;
            const auto recorded = c.i32(contract + 4), current = c.i32(club);
            c.set(3, std::uint32_t(recorded)); c.set(0, std::uint32_t(current));
            if (compare(c, recorded, current) != 0) continue;
            const auto status = c.u8(contract + 0x1c); c.set(0, status);
            if (compare(c, status, 1) != 0) continue;
            const auto date = c.u32(today), flags = c.u32(today + 4);
            c.set(4, date); c.set(0, flags); c.put32(frame.at(0x3c), date); c.put32(frame.at(0x40), flags);
            program.call(0x10717890, 0x1007fde8, {frame.at(0x44), contract + 0x28, 45});
            const auto current_year = c.u16(frame.at(0x3e)), deadline_year = c.u16(frame.at(0x46));
            c.set(3, current_year); c.set(0, deadline_year);
            const auto years = compare_logical(c, current_year, deadline_year);
            bool before = years < 0;
            if (!years) {
                const auto current_day = c.i16(frame.at(0x3c)), deadline_day = c.i16(frame.at(0x44));
                c.set(0, extended(current_day)); c.set(4, extended(deadline_day));
                const auto bits = extended(deadline_day) ^ extended(current_day);
                c.set(3, std::uint32_t(std::int32_t(bits) >> 1));
                c.s.wr<1>(guest::reg::xer_ca, std::int32_t(bits) < 0 && (bits & 1));
                before = current_day < deadline_day;
            }
            c.set(0, before);
            if (nonzero(c, before)) continue;
            c.set(0, 2); c.put8(contract + 0x1c, 2);
            program.call(0x10410000, 0x1007fe54, {c.u32(transfers), c.u32(roster), context, 0, contract});
        }
    }
}

void mark_secondary_roster(Cpu& c, std::uint32_t manager_address, std::uint32_t club_address, std::uint32_t context_value) {
    Frame frame(c, 0x1007fe90, 0x70, {.first_gpr = 23});
    Kept<24> club(c, club_address); const bool has_club = nonzero(c, std::int32_t(club_address));
    Kept<30> transfers(c, c.toc(-0x7b40)); Kept<31> staff_count(c, c.toc(slot::person_count));
    Kept<23> object(c, manager_address); Kept<25> context(c, context_value);
    Kept<26> alternate(c, c.r(26)); Kept<27> roster(c, c.r(27));
    Kept<28> pass(c, c.r(28)); Kept<29> index(c, c.r(29)); Program program(c);
    if (!has_club) return;
    alternate = program.call(0x100ce320, 0x1007fec8, {club, frame.at(0x38), 1});
    if (nonzero(c, std::int32_t(std::uint32_t(alternate)))) {
        const auto flag = c.i8(frame.at(0x38)); c.set(0, std::uint32_t(std::int32_t(flag)));
        if (!nonzero(c, flag)) return;
    }
    for (pass = 0;; pass = std::uint32_t(pass) + 1) {
        c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(pass)))));
        if (compare(c, std::int8_t(std::uint32_t(pass)), 2) >= 0) return;
        if (compare(c, std::int8_t(std::uint32_t(pass)), 1) == 0) club = alternate;
        if (!nonzero_logical(c, club)) continue;
        roster = club + 0x98;
        for (index = 0;; index = std::uint32_t(index) + 1, roster = std::uint32_t(roster) + 4) {
            c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(index)))));
            if (compare(c, std::int8_t(std::uint32_t(index)), 50) >= 0) break;
            const auto person = c.u32(roster); c.set(4, person);
            if (!valid_roster_person(c, person, staff_count)) continue;
            const auto contract = program.call(0x10082f20, 0x1007ff34, {object});
            if (!nonzero_logical(c, contract)) continue;
            const auto recorded = c.i32(contract + 4), current = c.i32(club);
            c.set(4, std::uint32_t(recorded)); c.set(0, std::uint32_t(current));
            if (compare(c, recorded, current) != 0) continue;
            const auto status = c.u8(contract + 0x1f); c.set(0, status);
            if (compare(c, status, 1) != 0) continue;
            c.set(0, 2); c.put8(contract + 0x1f, 2);
            program.call(0x10410000, 0x1007ff78, {c.u32(transfers), c.u32(roster), context, 3, contract});
        }
    }
}

void reset_roster_interest(Cpu& c, std::uint32_t manager_address, std::uint32_t club_address, std::uint32_t notify_value) {
    Frame frame(c, 0x1007ffb0, 0x60, {.first_gpr = 25});
    Kept<28> club(c, club_address); const bool has_club = nonzero(c, std::int32_t(club_address));
    Kept<29> notifier(c, c.toc(-0x7a98)); Kept<31> staff_count(c, c.toc(slot::person_count));
    Kept<27> object(c, manager_address); Kept<25> cursor(c, c.r(25)); Kept<26> index(c, c.r(26));
    Kept<30> pass(c, c.r(30)); Program program(c);
    if (!has_club) return;
    c.set(0, std::uint32_t(std::int32_t(std::int8_t(notify_value))));
    if (nonzero(c, std::int8_t(notify_value))) {
        index = club + 0x98;
        for (cursor = 0;; cursor = std::uint32_t(cursor) + 1, index = std::uint32_t(index) + 4) {
            c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(cursor)))));
            if (compare(c, std::int8_t(std::uint32_t(cursor)), 50) >= 0) break;
            const auto person = c.u32(index); c.set(4, person);
            if (valid_roster_person(c, person, staff_count))
                program.call(0x104ade40, 0x10080018, {c.u32(notifier), person, club, 0});
        }
    }
    notifier = program.call(0x100ce320, 0x1008003c, {club, frame.at(0x38), 1});
    if (nonzero(c, std::int32_t(std::uint32_t(notifier)))) {
        const auto flag = c.i8(frame.at(0x38)); c.set(0, std::uint32_t(std::int32_t(flag)));
        if (!nonzero(c, flag)) return;
    }
    for (pass = 0;; pass = std::uint32_t(pass) + 1) {
        c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(pass)))));
        if (compare(c, std::int8_t(std::uint32_t(pass)), 2) >= 0) return;
        if (compare(c, std::int8_t(std::uint32_t(pass)), 1) == 0) club = notifier;
        if (!nonzero_logical(c, club)) continue;
        cursor = club + 0x98;
        for (index = 0;; index = std::uint32_t(index) + 1, cursor = std::uint32_t(cursor) + 4) {
            c.set(0, std::uint32_t(std::int32_t(std::int8_t(std::uint32_t(index)))));
            if (compare(c, std::int8_t(std::uint32_t(index)), 50) >= 0) break;
            const auto person = c.u32(cursor); c.set(4, person);
            if (!valid_roster_person(c, person, staff_count)) continue;
            const auto contract = program.call(0x10082f20, 0x100800a8, {object});
            if (!nonzero_logical(c, contract)) continue;
            const auto recorded = c.i32(contract + 4), current = c.i32(club);
            c.set(4, std::uint32_t(recorded)); c.set(0, std::uint32_t(current));
            if (compare(c, recorded, current) != 0) continue;
            for (const auto offset : {0x1fu, 0x1cu}) {
                const auto flag = c.u8(contract + offset); c.set(0, flag);
                if (compare(c, flag, 1) == 0) { c.set(0, 0); c.put8(contract + offset, 0); }
            }
        }
    }
}
}
