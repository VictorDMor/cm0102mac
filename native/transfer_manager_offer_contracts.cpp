#include "transfer_manager_offer_contracts.hpp"
#include "guest_values.hpp"
#include "transfer_manager_adapter.hpp"
#include "transfer_manager_permissions.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;
constexpr auto staff_table = -0x7be0, club_table = -0x7be8, contracts = -0x7b7c,
               cached_primary = -0x7b78;
constexpr std::uint32_t person_size = 0x70, club_size = 0x248, next_list_row = 0x10738e00,
    primary_contract = 0x10082f20, remove_pending = 0x1007f840, remove_pending_origin = 0x1007f930,
    find_pending = 0x1007f720, forget_approaches = 0x1036f100, regional_approval = 0x103ecef0,
    approval_allowed = 0x103ee1a0;

struct Offer {
    Cpu& c;
    std::uint32_t address;
    std::uint32_t secondary() const { return c.u32(address + 4); }
    std::uint32_t club() const { return c.u32(address + 8); }
    std::uint32_t staff() const { return c.u32(address + 0xc); }
    std::uint32_t origin() const { return c.u32(address + 0x28); }
};

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (auto arg : args) c.set(r++, arg);
    c.call(entry, back);
    return c.r(3);
}

bool reset_secondary(Cpu& c, const Offer& offer) {
    const auto list = offer.secondary();
    c.set(3, list);
    if (!nonzero_logical(c, list)) return false;
    const auto head = c.u32(list + 4);
    c.set(0, head);
    c.put32(list + 0xc, head);
    return true;
}

std::uint32_t primary_club(Cpu& c, std::uint32_t contract, std::uint32_t cache, std::uint32_t clubs) {
    const bool found = nonzero_logical(c, contract);
    c.put32(cache, contract);
    if (!found) return 0;
    const auto offset = c.u32(contract + 4) * club_size;
    const auto base = c.u32(clubs);
    c.set({{0, offset}, {3, base}});
    return base + offset;
}

std::uint32_t player_from_id(Cpu& c, std::uint32_t people, std::uint32_t id) {
    const auto offset = id * person_size;
    c.set(0, offset);
    return c.u32(people) + offset;
}

bool byte_result(Cpu& c, std::uint32_t value) {
    const auto byte = std::int8_t(value);
    c.set(0, std::uint32_t(byte));
    return nonzero(c, byte);
}

bool check_regional_approval(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t club,
                             std::uint32_t back) {
    c.set({{3, manager}, {4, person}, {5, club}, {6, 0}});
    if (transfer_manager_direct_calls_enabled()) {
        c.link(back);
        c.s.pc = regional_approval;
        requires_regional_transfer_approval(c, manager, person, club, 0);
        c.finish();
    } else {
        c.call(regional_approval, back);
    }
    return byte_result(c, c.r(3));
}

bool check_approval_gate(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t back) {
    return byte_result(c, call(c, approval_allowed, back, {manager, person}));
}
}

void remove_offer_pending_contracts(Cpu& c, std::uint32_t offer_address) {
    Frame frame(c, 0x103ec440, 0x50, {.first_gpr = 27});
    const Kept<27> offer_register(c, offer_address);
    const Kept<28> people(c, c.toc(staff_table));
    const auto clubs = c.toc(club_table);
    const Kept<29> ledger(c, c.toc(contracts));
    const Offer offer{c, offer_register};
    const auto staff_offset = offer.staff() * person_size, club_offset = offer.club() * club_size;
    const auto staff_base = c.u32(people), club_base = c.u32(clubs);
    c.set({{0, club_offset}, {6, staff_base}});
    call(c, remove_pending, 0x103ec488, {c.u32(ledger), staff_base + staff_offset, club_base + club_offset});
    const Kept<30> clear_flag(c, 0);
    c.put8(offer.address + 0x10, clear_flag);
    if (!reset_secondary(c, offer)) return;
    while (true) {
        const Kept<31> row(c, call(c, next_list_row, 0x103ec4d0, {offer.secondary()}));
        if (!nonzero(c, row)) return;
        const auto player = player_from_id(c, people, c.u32(row));
        call(c, remove_pending_origin, 0x103ec4c4, {c.u32(ledger), player, offer.origin()});
        c.put8(row + 4, clear_flag);
    }
}

void forget_offer_approaches(Cpu& c, std::uint32_t manager, std::uint32_t offer_address) {
    Frame frame(c, 0x103ec4f0, 0x50, {.first_gpr = 26});
    const Kept<29> transfers(c, manager);
    const Kept<31> people(c, c.toc(staff_table));
    const Kept<30> offer_register(c, offer_address);
    const auto ledger = c.toc(contracts);
    Kept<27> employer(c, c.toc(cached_primary));
    const Kept<28> clubs(c, c.toc(club_table));
    const Offer offer{c, offer_register};
    const Kept<26> staff(c, player_from_id(c, people, offer.staff()));
    const auto contract = call(c, primary_contract, 0x103ec534, {c.u32(ledger), staff});
    employer = primary_club(c, contract, employer, clubs);
    const auto club_offset = offer.club() * club_size;
    c.set(0, club_offset);
    call(c, forget_approaches, 0x103ec574, {c.u32(transfers + 0x988), c.u32(clubs) + club_offset, staff});
    if (!reset_secondary(c, offer)) return;
    while (const auto row = call(c, next_list_row, 0x103ec5b0, {offer.secondary()})) {
        nonzero_logical(c, row);
        const auto player = player_from_id(c, people, c.u32(row));
        call(c, forget_approaches, 0x103ec5a8, {c.u32(transfers + 0x988), employer, player});
    }
    nonzero_logical(c, 0);
}

void mark_offer_contracts_for_withdrawal(Cpu& c, std::uint32_t offer_address) {
    Frame frame(c, 0x103ec5d0, 0x50, {.first_gpr = 28});
    const auto people = c.toc(staff_table);
    const Kept<30> ledger(c, c.toc(contracts));
    Kept<31> marker(c, c.toc(cached_primary));
    const Kept<28> offer_register(c, offer_address);
    const Offer offer{c, offer_register};
    const auto staff = player_from_id(c, people, offer.staff());
    const auto contract = call(c, primary_contract, 0x103ec614, {c.u32(ledger), staff});
    const auto club = primary_club(c, contract, marker, contract ? c.toc(club_table) : 0);
    const Kept<29> employer(c, club);
    const auto pending = call(c, find_pending, 0x103ec64c, {c.u32(ledger), offer.staff(), offer.club()});
    if (nonzero_logical(c, pending)) {
        c.set(0, 4);
        c.put8(pending + 0x64, 4);
    }
    if (!reset_secondary(c, offer)) return;
    marker = 4;
    while (true) {
        const auto row = call(c, next_list_row, 0x103ec69c, {offer.secondary()});
        c.set(4, row);
        if (!nonzero(c, row)) return;
        const auto extra = call(c, find_pending, 0x103ec688, {c.u32(ledger), c.u32(row), c.u32(employer)});
        if (nonzero_logical(c, extra)) c.put8(extra + 0x64, marker);
    }
}

std::uint32_t offer_requires_regional_approval(Cpu& c, std::uint32_t manager, std::uint32_t offer_address) {
    Frame frame(c, 0x103ecda0, 0x60, {.first_gpr = 25});
    const Kept<26> transfers(c, manager);
    const Kept<27> offer_register(c, offer_address);
    const Kept<28> people(c, c.toc(staff_table));
    const Kept<29> ledger(c, c.toc(contracts));
    const Kept<30> cache(c, c.toc(cached_primary));
    const Kept<31> clubs(c, c.toc(club_table));
    const Offer offer{c, offer_register};
    const auto staff_offset = offer.staff() * person_size, club_offset = offer.club() * club_size;
    const auto staff_base = c.u32(people), club_base = c.u32(clubs);
    c.set({{0, club_offset}, {7, staff_base}});
    if (check_regional_approval(c, transfers, staff_base + staff_offset, club_base + club_offset, 0x103ecdf0) &&
        check_approval_gate(c, transfers, player_from_id(c, people, offer.staff()), 0x103ece10))
        return result(c, 1);
    if (!reset_secondary(c, offer)) return result(c, 0);
    while (true) {
        const Kept<25> row(c, call(c, next_list_row, 0x103ececc, {offer.secondary()}));
        if (!nonzero(c, row)) return result(c, 0);
        const auto staff = player_from_id(c, people, offer.staff());
        const auto contract = call(c, primary_contract, 0x103ece50, {c.u32(ledger), staff});
        const auto club = primary_club(c, contract, cache, clubs);
        const auto extra = player_from_id(c, people, c.u32(row));
        if (check_regional_approval(c, transfers, extra, club, 0x103ece90) &&
            check_approval_gate(c, transfers, player_from_id(c, people, c.u32(row)), 0x103eceb0))
            return result(c, 1);
    }
}
}
