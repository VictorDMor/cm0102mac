#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;

std::uint32_t release_quote(Cpu& c, std::uint32_t manager_address, std::uint32_t context_value,
                             std::uint32_t proposed_club, std::uint32_t person_address) {
    Frame frame(c, 0x10089a40, 0x80, {.first_gpr = 25});
    Kept<29> person(c, person_address); const bool has_person = nonzero(c, std::int32_t(person_address));
    const auto managers = c.toc(-0x7b7c); c.set(6, managers);
    Kept<30> error_buffer(c, c.toc(-0x7ba8)); Kept<26> object(c, manager_address);
    Kept<31> pool(c, c.toc(slot::pool)); Kept<27> context(c, context_value);
    Kept<28> target(c, proposed_club); Kept<25> club(c, c.r(25)); Program program(c);
    const auto manager = c.u32(managers); c.set(6, manager);
    std::uint32_t primary = 0;
    if (has_person) {
        const auto count_holder = c.toc(slot::person_count); c.set(3, count_holder);
        const auto id = c.i32(person), count = c.i32(count_holder);
        c.set(4, std::uint32_t(id)); c.set(0, std::uint32_t(count));
        if (compare(c, id, count) < 0) {
            const auto repeated = c.u32(person); c.set(0, repeated);
            const auto holder = c.toc(slot::contract_index); c.set(3, holder);
            const auto indices = c.u32(holder), offset = repeated * Contract::size;
            c.set(0, offset); c.set(3, indices);
            const auto index = c.i32(indices + offset); c.set(0, std::uint32_t(index));
            if (compare(c, index) >= 0) {
                const auto record_offset = std::uint32_t(index) * Contract::size; c.set(0, record_offset);
                const auto records = c.u32(manager); c.set(3, records); primary = records + record_offset;
            }
        }
    }
    c.set(4, primary);
    const auto current = c.toc(-0x7b78); c.set(3, current);
    const bool present = nonzero_logical(c, primary); c.put32(current, primary);
    if (present) {
        const auto id = c.u32(primary + 4); c.set(0, id);
        const auto holder = c.toc(-0x7be8); c.set(3, holder); c.set(0, id * 0x248);
        const auto clubs = c.u32(holder); c.set(3, clubs); club = clubs + id * 0x248;
    } else club = 0;
    const auto usable = std::int8_t(program.call(0x1014c2e0, 0x10089b10, {club, 0, 1, 0, 0}));
    c.set(0, std::uint32_t(std::int32_t(usable)));
    if (!nonzero(c, usable)) {
        program.call(0x10739890, 0x10089b30, {pool + 0x37, 0, 0, frame.at(0x40), frame.at(0x38)});
        const auto version = program.call(0x10768810, 0x10089b34);
        program.call(0x1077b720, 0x10089b50, {error_buffer, pool + 0x23b, version, frame.at(0x40), frame.at(0x38), 0x1ef9});
        program.call(0x10769b60, 0x10089b60, {pool + 0x248, error_buffer, 0});
        const auto enabled = c.toc(-0x7bac); c.set(3, enabled); c.set(0, 0); c.put8(enabled, 0);
        return result(c, c.u32(person + 8));
    }
    const auto located = program.call(0x10082f90, 0x10089b84, {object, person, club});
    Kept<31> contract(c, located);
    if (!nonzero(c, std::int32_t(std::uint32_t(contract)))) return result(c, 0);
    const auto flags = c.u8(contract + 0x38);
    const auto role = std::int32_t(std::int8_t(flags)) >> 2;
    c.set(0, std::uint32_t(role)); c.s.wr<1>(guest::reg::xer_ca, role < 0 && (flags & 2));
    if (compare(c, role, 2) != 0) return result(c, 0);
    const auto locked = c.i8(contract + 0x54); c.set(0, std::uint32_t(std::int32_t(locked)));
    if (nonzero(c, locked)) return result(c, 0);
    Kept<30> base_value(c, c.u32(person + 8));
    const auto limited = std::int8_t(program.call(0x10380520, 0x10089bc0));
    c.set(0, std::uint32_t(std::int32_t(limited)));
    if (nonzero(c, limited)) {
        const auto ceiling = c.i32(contract + 0x24); c.set(0, std::uint32_t(ceiling));
        base_value = compare(c, std::int32_t(std::uint32_t(base_value)), ceiling) < 0 ? std::uint32_t(base_value) : std::uint32_t(ceiling);
        c.set(0, base_value);
    }
    const auto assessed = program.call(0x10085860, 0x10089bfc, {object, context, target, person, 0, 1, 1});
    const auto quoted = std::int32_t(assessed), base = std::int32_t(std::uint32_t(base_value));
    const auto quoted_order = compare(c, quoted, base); club = base_value;
    if (quoted_order > 0) club = assessed;
    const auto capped = std::int8_t(program.call(0x10380520, 0x10089c14, {contract}));
    c.set(0, std::uint32_t(std::int32_t(capped)));
    if (nonzero(c, capped)) {
        const auto ceiling = c.i32(contract + 0x24); c.set(0, std::uint32_t(ceiling));
        club = compare(c, std::int32_t(std::uint32_t(club)), ceiling) < 0 ? std::uint32_t(club) : std::uint32_t(ceiling);
        c.set(0, club);
    }
    const auto protected_person = std::int8_t(program.call(0x100d3150, 0x10089c3c, {person, 0}));
    c.set(0, std::uint32_t(std::int32_t(protected_person)));
    if (nonzero(c, protected_person)) return result(c, base_value);
    const auto active = c.u8(contract + 0x5c) & 4u; c.set(0, active);
    if (!nonzero(c, std::int32_t(active))) return result(c, base_value);
    const auto squad_role = c.u8(contract + 0x60) & 15u; c.set(0, squad_role);
    if (compare(c, std::int32_t(squad_role), 1) == 0) return result(c, club);
    c.set(0, std::uint32_t(std::int32_t(std::int8_t(squad_role))));
    if (compare(c, std::int32_t(squad_role), 5) == 0) return result(c, club);
    if (compare(c, std::int32_t(squad_role), 2) == 0 || compare(c, std::int32_t(squad_role), 3) == 0) {
        const auto sum = std::uint32_t(club) + std::uint32_t(base_value); c.set(3, sum);
        const auto rounded = sum + (sum >> 31); c.set(0, rounded);
        c.s.wr<1>(guest::reg::xer_ca, std::int32_t(rounded) < 0 && (rounded & 1));
        return result(c, std::uint32_t(std::int32_t(rounded) >> 1));
    }
    return result(c, base_value);
}
}
