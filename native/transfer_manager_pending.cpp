#include "transfer_manager_pending.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using guest::Cpu;
constexpr std::uint32_t pending_contract = 0x40;
constexpr std::int32_t contracts_slot = -0x7b7c, clubs_slot = -0x7be8;
constexpr std::uint32_t find_pending_transfer = 0x1007f7b0, club_size = 0x248;

struct Manager {
    Cpu& cpu;
    std::uint32_t address;

    std::uint32_t staff_status_table() const { return cpu.u32(address + 0x9a0); }
};

struct Person {
    Cpu& cpu;
    std::uint32_t address;

    std::uint32_t status_offset() const { return cpu.u32(address) * 10u; }
};

void write_pending_marker(Cpu& c, Manager manager, Person person, bool pending) {
    const auto offset = person.status_offset();
    const auto table = manager.staff_status_table();
    const auto flags = c.u8(table + offset);
    const auto changed = pending ? flags | pending_contract : flags & ~pending_contract;
    c.put8(table + offset, changed);
    c.set({{0, changed}, {3, offset}, {4, table}});
}
}

void mark_pending_contract(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    write_pending_marker(c, {c, manager}, {c, person}, true);
}

void clear_pending_contract(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    write_pending_marker(c, {c, manager}, {c, person}, false);
}

std::uint32_t find_pending_contract(Cpu& c, std::uint32_t manager, std::uint32_t person,
                                    std::uint32_t destination) {
    guest::Frame frame(c, 0x103ea290, 0x50, {.first_gpr = 29});
    const guest::Kept<31> output(c, destination);
    const guest::Kept<30> staff(c, person);
    const guest::Kept<29> transfers(c, manager);

    const auto staff_id = c.u32(person);
    const auto status_table = Manager{c, manager}.staff_status_table();
    const auto marker = c.u8(status_table + staff_id * 10u) & pending_contract;
    c.set({{0, marker}, {3, status_table}, {4, staff_id}});
    if (!guest::nonzero(c, std::int32_t(marker))) return guest::result(c, 0);

    c.set(3, c.u32(c.toc(contracts_slot)));
    c.call(find_pending_transfer, 0x103ea2d8);
    const auto contract = c.r(3);
    if (!guest::nonzero_logical(c, contract)) {
        // The lookup can change the current record. Reload the index and table
        // before clearing the stale flag, just as for a fresh cache access.
        const auto offset = Person{c, staff}.status_offset();
        const auto table = Manager{c, transfers}.staff_status_table();
        const auto flags = c.u8(table + offset) & ~pending_contract;
        c.put8(table + offset, flags);
        c.set({{0, flags}, {4, offset}, {5, table}});
        return guest::result(c, 0);
    }

    if (guest::nonzero_logical(c, output)) {
        const auto club_index = c.u32(contract + 4);
        const auto clubs = c.u32(c.toc(clubs_slot));
        c.set(3, clubs);
        c.put32(output, clubs + club_index * club_size);
    }
    return guest::result(c, 1);
}

}
