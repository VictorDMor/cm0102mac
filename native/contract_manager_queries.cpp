#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
// Each validation and index read is kept in its original order. In
// particular, the person's id is read again after validating it; the
// fields are mutable guest memory, not cached host records.
std::uint32_t find_contract(Cpu& c, std::uint32_t manager_address, std::uint32_t person, bool second) {
    if (!nonzero_logical(c, person)) return result(c, 0);
    const auto count_address = c.toc(slot::person_count);
    c.set(5, count_address);
    const auto checked_id = c.i32(person);
    c.set(6, std::uint32_t(checked_id));
    const auto count = c.i32(count_address);
    c.set(0, std::uint32_t(count));
    if (compare(c, checked_id, count) >= 0) return result(c, 0);

    const auto person_offset = c.u32(person) * Contract::size;
    c.set(0, person_offset);
    const auto index_holder = c.toc(slot::contract_index);
    c.set(4, index_holder);
    const auto index_base = c.u32(index_holder);
    const auto index_row = index_base + person_offset;
    c.set(4, second ? index_row : index_base);
    const auto index = c.i32(index_row + (second ? 4 : 0));
    c.set(0, std::uint32_t(index));
    if (compare(c, index) < 0) return result(c, 0);
    const auto offset = std::uint32_t(index) * Contract::size;
    c.set(0, offset);
    return result(c, Manager{c, manager_address}.records().get() + offset);
}
}

std::uint32_t primary_contract(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    return find_contract(c, manager, person, false);
}
std::uint32_t secondary_contract(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    return find_contract(c, manager, person, true);
}
std::uint32_t primary_terms(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    // This entry inlines the primary lookup; it has no call frame/link.
    const auto contract = find_contract(c, manager, person, false);
    return result(c, nonzero_logical(c, contract) ? c.u32(contract + 12) : 0);
}

std::uint32_t club_contract(Cpu& c, std::uint32_t manager_address, std::uint32_t person, std::uint32_t requested_club) {
    Frame frame(c, 0x10082f90, 0x50, {.first_gpr = 29});
    const auto count_address = c.toc(slot::person_count);
    c.set(6, count_address);
    Kept<31> club(c, requested_club);
    Program program(c);
    if (!nonzero_logical(c, person) || !nonzero_logical(c, club)) return result(c, 0);

    auto valid_person = [&] {
        if (!nonzero_logical(c, person)) return false;
        const auto id = c.i32(person), count = c.i32(count_address);
        c.set(5, std::uint32_t(id));
        c.set(0, std::uint32_t(count));
        return compare(c, id, count) < 0;
    };
    if (!valid_person()) return result(c, 0);

    Kept<30> primary(c, 0);
    if (valid_person()) {
        const auto offset = c.u32(person) * Contract::size;
        c.set(0, offset);
        const auto index_holder = c.toc(slot::contract_index);
        c.set(5, index_holder);
        const auto indices = c.u32(index_holder);
        c.set(5, indices);
        const auto index = c.i32(indices + offset);
        c.set(0, std::uint32_t(index));
        if (compare(c, index) >= 0) {
            const auto contract_offset = std::uint32_t(index) * Contract::size;
            c.set(0, contract_offset);
            const auto records = Manager{c, manager_address}.records().get();
            c.set(5, records);
            primary = records + contract_offset;
        }
    }
    if (!nonzero_logical(c, primary)) return result(c, 0);
    const auto primary_club = Contract{c, primary}.club_id().get();
    const auto club_id = c.i32(club);
    c.set(5, std::uint32_t(primary_club));
    c.set(0, std::uint32_t(club_id));
    if (compare(c, primary_club, club_id) == 0) return result(c, primary);

    // The secondary lookup is a real call of another rewritten routine;
    // the external club resolver retains its byte output and alias effects.
    Kept<29> secondary(c, program.call(0x10083110, 0x10083070, {manager_address, person}));
    if (nonzero(c, std::int32_t(std::uint32_t(secondary)))) {
        const auto secondary_club = Contract{c, secondary}.club_id().get();
        const auto id = c.i32(club);
        c.set(4, std::uint32_t(secondary_club));
        c.set(0, std::uint32_t(id));
        if (compare(c, secondary_club, id) == 0) return result(c, secondary);
    }
    const auto resolved = program.call(0x100ce320, 0x1008309c, {club, frame.at(0x38), 1});
    if (nonzero_logical(c, resolved)) {
        const auto flag = c.i8(frame.at(0x38));
        c.set(0, std::uint32_t(std::int32_t(flag)));
        if (!nonzero(c, flag)) club = resolved;
    }
    const auto final_primary_club = Contract{c, primary}.club_id().get();
    const auto resolved_id = c.i32(club);
    c.set(0, std::uint32_t(final_primary_club));
    c.set(3, std::uint32_t(resolved_id));
    if (compare(c, final_primary_club, resolved_id) == 0) return result(c, primary);
    if (nonzero_logical(c, secondary)) {
        const auto final_secondary_club = Contract{c, secondary}.club_id().get();
        c.set(0, std::uint32_t(final_secondary_club));
        if (compare(c, final_secondary_club, resolved_id) == 0) return result(c, secondary);
    }
    return result(c, 0);
}
}
