#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
std::int8_t signed_result(Cpu& c, std::uint32_t value) {
    const auto byte = std::int8_t(value);
    c.set(0, std::uint32_t(std::int32_t(byte)));
    return byte;
}
std::uint32_t masked_status(Cpu& c, std::uint32_t person) {
    const auto flags = c.u8(person + 0x6a);
    const auto filtered = std::uint8_t(flags & 0xf3);
    c.set(0, filtered);
    return result(c, std::uint32_t(std::int32_t(std::int8_t(filtered))));
}
// Bit1 of the contract's flags becomes the original's sign word. This is
// ABI residue as well as a named condition used by the relationship rules.
bool contract_mode(Cpu& c, std::uint8_t flags) {
    const bool set = flags & 2;
    c.set(0, std::uint32_t(flags & 3) << 30);
    c.s.wr<1>(guest::reg::xer_ca, set && (flags & 1));
    return set;
}
}

std::uint32_t classify_relationship(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address,
                                     std::uint32_t club_address) {
    Frame frame(c, 0x1007e820, 0x60, {.first_gpr = 27});
    Kept<29> person(c, person_address);
    Kept<27> scratch(c, c.toc(-0x7b78));
    Kept<28> object(c, manager_address);
    Kept<30> requested(c, club_address);
    Kept<31> resolved(c, 0);
    Program program(c);
    const auto kind = c.u8(person + 0x62); c.set(4, kind);
    if (compare(c, kind, 12) == 0) return result(c, 0);
    if (!nonzero(c, signed_result(c, program.call(0x100c7a20, 0x1007e860, {kind})))) return result(c, 0);
    const auto primary = Staff{c, person}.club().get(); c.set(0, primary);
    if (nonzero_logical(c, primary) && compare_logical(c, primary, requested) == 0) {
        if (nonzero(c, signed_result(c, program.call(0x100c7a20, 0x1007e88c, {c.u8(person + 0x62)}))) &&
            nonzero(c, signed_result(c, program.call(0x100d12d0, 0x1007e89c, {requested})))) {
            const auto flags = c.u8(person + 0x6a); c.set(0, flags);
            return result(c, (flags & 1) + 1);
        }
    }
    const auto current = Staff{c, person}.club().get(); c.set(3, current);
    if (nonzero_logical(c, current)) {
        resolved = program.call(0x100ce320, 0x1007e8cc, {current, frame.at(0x38), 1});
        if (nonzero(c, std::int32_t(std::uint32_t(resolved)))) {
            const auto raw_flag = c.u8(frame.at(0x38)); c.set(3, raw_flag);
            const auto flag = signed_result(c, raw_flag);
            if (nonzero(c, flag)) {
                const auto actual = Staff{c, person}.club().get(); c.set(0, actual);
                if (compare_logical(c, requested, actual) == 0) return result(c, 4);
            }
            c.set(0, std::uint32_t(std::int32_t(flag)));
            if (!nonzero(c, flag) && compare_logical(c, requested, resolved) == 0) return result(c, 3);
        }
    }
    const auto holder = c.toc(-0x7b7c);
    const auto contract = program.call(0x10082f20, 0x1007e91c, {c.u32(holder), person});
    const bool found = nonzero_logical(c, contract); c.put32(scratch, contract);
    std::uint32_t club = 0;
    if (found) {
        const auto id = c.u32(contract + 4);
        const auto clubs_holder = c.toc(-0x7be8); c.set(3, clubs_holder); c.set(0, id * 0x248);
        const auto clubs = c.u32(clubs_holder); c.set(3, clubs); club = clubs + id * 0x248;
    }
    Kept<27> contract_club(c, club);
    if (!nonzero_logical(c, contract_club)) return result(c, 0);
    const auto staff_club = Staff{c, person}.club().get(); c.set(0, staff_club);
    if (compare_logical(c, contract_club, staff_club) == 0 || compare_logical(c, contract_club, resolved) == 0)
        return result(c, 0);
    const auto own_contract = program.call(0x10082f20, 0x1007e974, {object, person});
    const auto flags = c.u8(own_contract + 0x38); c.set(0, flags);
    const bool requested_contract = compare_logical(c, contract_club, requested) == 0;
    const bool active = contract_mode(c, flags);
    Kept<28> mode(c, active ? 0xffffffff : 0);
    if (requested_contract && nonzero(c, signed_result(c, program.call(0x100d12d0, 0x1007e990, {requested})))) {
        c.set(0, active ? 0 : 32);
        return result(c, active ? 1 : 2);
    }
    const auto other = program.call(0x100ce320, 0x1007e9bc, {contract_club, frame.at(0x38), 1});
    if (nonzero_logical(c, other) &&
        (compare_logical(c, other, requested) == 0 || compare_logical(c, contract_club, requested) == 0)) {
        c.set(0, std::uint32_t(mode));
        return result(c, nonzero(c, std::int32_t(std::uint32_t(mode))) ? 3 : 4);
    }
    return result(c, 0);
}

std::uint32_t update_relationship(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address,
                                  std::uint32_t club_address, std::uint32_t output) {
    Frame frame(c, 0x1007ea10, 0x60, {.first_gpr = 23});
    Kept<28> requested(c, club_address);
    const bool present = nonzero(c, std::int32_t(club_address));
    Kept<31> rosters(c, c.toc(-0x7a6c));
    Kept<24> stats(c, c.toc(-0x7b3c));
    Kept<26> object(c, manager_address);
    Kept<25> contracts(c, c.toc(-0x7b7c));
    Kept<27> person(c, person_address);
    Kept<23> other_output(c, output);
    Program program(c);
    if (!present) return result(c, 1);
    if (nonzero_logical(c, other_output)) { c.set(0, 0); c.put32(other_output, 0); }
    const auto relationship = signed_result(c, program.call(0x1007e820, 0x1007ea68, {object, person, requested}));
    if (!nonzero(c, relationship)) return result(c, 1);
    c.set(0, std::uint32_t(std::int32_t(relationship)));
    const bool first = compare(c, relationship, 1) == 0;
    bool second = false;
    if (!first) second = compare(c, relationship, 2) == 0;
    if (first || second) {
        const auto check = program.call(0x10083110, first ? 0x1007ea90 : 0x1007eb14, {c.u32(contracts), person});
        bool different = false;
        if (nonzero_logical(c, check)) {
            const auto again = program.call(0x10083110, first ? 0x1007eaa4 : 0x1007eb28, {c.u32(contracts), person});
            const auto contract_club = c.i32(again + 4), id = c.i32(requested);
            c.set(3, std::uint32_t(contract_club)); c.set(0, std::uint32_t(id));
            different = compare(c, id, contract_club) != 0;
        }
        if (different) {
            const auto primary = program.call(0x10082f20, first ? 0x1007eac0 : 0x1007eb44, {object, person});
            const auto flags = c.u8(primary + 0x38);
            c.set(4, second); c.set(0, (flags & ~2u) | (second ? 2 : 0));
            c.put8(primary + 0x38, (flags & ~2u) | (second ? 2 : 0));
        } else {
            program.call(0x100ce200, first ? 0x1007eae0 : 0x1007eb64, {person, first ? 1u : 2u});
            program.call(0x103afb90, first ? 0x1007eaf8 : 0x1007eb7c,
                         {c.u32(rosters), requested, person, first ? 1u : 0u, 1});
        }
        return result(c, 1);
    }
    const bool third = compare(c, relationship, 3) == 0;
    std::uint32_t destination_address, source_address;
    if (third) {
        source_address = program.call(0x100ce320, 0x1007eb9c, {requested, 0, 1});
        destination_address = requested;
    } else {
        keep(c, {{30, requested}});
        destination_address = program.call(0x100ce320, 0x1007ebbc, {requested, 0, 1});
        source_address = requested;
    }
    Kept<29> destination(c, destination_address);
    Kept<30> source(c, source_address);
    if (nonzero_logical(c, other_output)) c.put32(other_output, destination);
    if (!nonzero(c, signed_result(c, program.call(0x100c7dd0, 0x1007ebdc,
                                                  {destination, c.u8(person + 0x62), person})))) return result(c, 0);
    const auto secondary = program.call(0x10083110, 0x1007ebf8, {c.u32(contracts), person});
    bool same_club = false;
    if (nonzero_logical(c, secondary)) {
        const auto again = program.call(0x10083110, 0x1007ec0c, {c.u32(contracts), person});
        const auto club = c.i32(again + 4), requested_id = c.i32(requested);
        c.set(3, std::uint32_t(club)); c.set(0, std::uint32_t(requested_id));
        same_club = compare(c, requested_id, club) == 0;
    }
    if (same_club) {
        program.call(0x10308350, 0x1007ec3c, {c.u32(stats), person, 0xffffffff, 0, 0, 0xffffffff, 0});
        program.call(0x100c8930, 0x1007ec54, {source, person, 0, 1, 1});
        program.call(0x103afb90, 0x1007ec6c, {c.u32(rosters), source, person, 1, 1});
        program.call(0x103afb90, 0x1007ec84, {c.u32(rosters), source, person, 0, 1});
        program.call(0x100c82a0, 0x1007eca0, {destination, person, c.u8(person + 0x62), 0, 1, 1});
    } else {
        const auto another = program.call(0x10083110, 0x1007ecb0, {c.u32(contracts), person});
        if (nonzero_logical(c, another)) {
            const auto primary = program.call(0x10082f20, 0x1007ecc4, {object, person});
            const auto flags = c.u8(primary + 0x38); c.set(4, flags);
            const bool bit = contract_mode(c, flags);
            c.set(0, bit ? 0xffffffff : 0); nonzero(c, bit ? -1 : 0);
            const auto changed = flags ^ 2u;
            c.set(0, bit ? 0 : 1); c.set(4, changed); c.put8(primary + 0x38, changed);
            program.call(0x100c8930, 0x1007ed0c, {source, person, 0, 0, 1});
            program.call(0x100c82a0, 0x1007ed28, {destination, person, 11, 0, 0, 1});
        } else {
            program.call(0x10308350, 0x1007ed4c, {c.u32(stats), person, 0xffffffff, 0, 0, 0xffffffff, 0});
            program.call(0x100c8930, 0x1007ed64, {source, person, 0, 1, 0});
            program.call(0x103afb90, 0x1007ed7c, {c.u32(rosters), source, person, 1, 1});
            program.call(0x103afb90, 0x1007ed94, {c.u32(rosters), source, person, 0, 1});
            program.call(0x100c82a0, 0x1007edb0, {destination, person, c.u8(person + 0x62), 0, 1, 0});
        }
    }
    return result(c, 1);
}

namespace {
std::uint32_t update_selected_relationship(Cpu& c, std::uint32_t entry, std::uint32_t manager,
                                           std::uint32_t person_address, std::uint32_t club_address, bool second) {
    Frame frame(c, entry, 0x50, {.first_gpr = 29});
    Kept<31> club(c, club_address); Kept<30> person(c, person_address); Kept<29> object(c, manager);
    Program program(c);
    const auto relationship = signed_result(c, program.call(0x1007e820, second ? 0x1007ee78 : 0x1007edf8));
    const auto first_code = second ? 2 : 1, other_code = second ? 4 : 3;
    if (compare(c, relationship, first_code) != 0 && compare(c, relationship, other_code) != 0) return result(c, 0);
    return program.call(0x1007ea10, second ? 0x1007eea0 : 0x1007ee20, {object, person, club, 0});
}
}
std::uint32_t update_first_relationship(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club) {
    return update_selected_relationship(c, 0x1007edd0, manager, person, club, false);
}
std::uint32_t update_second_relationship(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club) {
    return update_selected_relationship(c, 0x1007ee50, manager, person, club, true);
}

std::uint32_t relationship_status(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address,
                                  std::uint32_t club_address) {
    Frame frame(c, 0x1007eed0, 0x80, {.first_gpr = 27});
    Kept<29> requested(c, club_address);
    const bool present = nonzero(c, std::int32_t(club_address));
    Kept<30> message(c, c.toc(-0x7ba8)); Kept<31> pool(c, c.toc(slot::pool));
    Kept<27> object(c, manager_address); Kept<28> person(c, person_address);
    Program program(c);
    if (!present) {
        program.call(0x10739890, 0x1007ef10, {pool + 0x37, 0, 0, frame.at(0x40), frame.at(0x38)});
        const auto version = program.call(0x10768810, 0x1007ef14);
        program.call(0x1077b720, 0x1007ef30, {message, pool + 0x23b, version, frame.at(0x40), frame.at(0x38), 0xbcd});
        program.call(0x10769b60, 0x1007ef40, {pool + 0x248, message, 0});
        const auto flag = c.toc(-0x7bac); c.set(4, flag); c.set(0, 0); c.put8(flag, 0);
        return result(c, 1);
    }
    Kept<31> secondary(c, program.call(0x10083110, 0x1007ef58));
    if (!nonzero(c, std::int32_t(std::uint32_t(secondary)))) return masked_status(c, person);
    const auto contract_club = c.i32(secondary + 4), club_id = c.i32(requested);
    c.set(3, std::uint32_t(contract_club)); c.set(0, std::uint32_t(club_id));
    bool same = compare(c, contract_club, club_id) == 0;
    if (!same) {
        const auto resolved = program.call(0x100ce320, 0x1007ef80, {requested, 0, 1});
        if (nonzero_logical(c, resolved)) {
            const auto again = c.i32(secondary + 4), id = c.i32(resolved);
            c.set(4, std::uint32_t(again)); c.set(0, std::uint32_t(id)); same = compare(c, again, id) == 0;
        }
    }
    if (same) return masked_status(c, person);
    if (nonzero_logical(c, program.call(0x100ce320, 0x1007efb8, {requested, 0, 1}))) return result(c, 1);
    const auto primary = program.call(0x10082f20, 0x1007efd4, {object, person});
    const auto flags = c.u8(primary + 0x38);
    const bool bit = contract_mode(c, flags);
    c.set(0, bit ? 0 : 1);
    return result(c, bit ? 2 : 1);
}
}
