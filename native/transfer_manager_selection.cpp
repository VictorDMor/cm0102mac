#include "transfer_manager_selection.hpp"
#include "transfer_manager_adapter.hpp"
#include "transfer_manager_collections.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;

constexpr std::int32_t contracts_slot = -0x7b7c, message_slot = -0x7ba8,
    pool_slot = -0x4b9c, success_slot = -0x7bac, people_slot = -0x7be0;
constexpr std::uint32_t registrations_offset = 0xa5c, status_table_offset = 0x9a0,
    lookup_registered_contract = 0x10083110, associated_club = 0x100ce320, staff_size = 0x70,
    append_node = 0x10738d10, next_node = 0x10738e00, construct_list = 0x10738c00,
    reset_list = 0x10738ca0, destroy_list = 0x10738c20, offer_value = 0x10416710;

std::uint32_t call(Cpu& c, std::uint32_t address, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned index = 3;
    for (const auto value : args) c.set(index++, value);
    c.call(address, back);
    return c.r(3);
}

void registration_diagnostic(Cpu& c, std::uint32_t pool, std::uint32_t message,
                             std::uint32_t name, std::uint32_t extension, std::uint32_t line,
                             std::uint32_t split_return) {
    call(c, 0x10739890, split_return, {pool + 0x37, 0, 0, name, extension});
    const auto version = call(c, 0x10768810, split_return + 4);
    call(c, 0x1077b720, split_return + 0x20, {message, pool + 0x6ff, version, name, extension, line});
    call(c, 0x10769b60, split_return + 0x30, {pool + 0x70c, message, 0});
}

struct List {
    Cpu& cpu;
    std::uint32_t address;

    void rewind() const {
        const auto first = cpu.u32(address + 4);
        cpu.set(0, first);
        cpu.put32(address + 0xc, first);
    }

    std::uint32_t next(std::uint32_t back) const { return call(cpu, next_node, back, {address}); }
};

struct OfferTable {
    Cpu& cpu;
    std::uint32_t manager;

    std::uint32_t start() const { return cpu.u32(manager); }
    std::int32_t count() const { return cpu.i32(manager + 4); }
};

struct OfferRow {
    Cpu& cpu;
    std::uint32_t address;

    std::uint32_t linked_offers() const { return cpu.u32(address); }
    std::int32_t club_index() const { return cpu.i32(address + 8); }
    std::int32_t person_index() const { return cpu.i32(address + 0xc); }
    std::uint8_t state() const { return cpu.u8(address + 0x34); }
    std::uint8_t open_to_clubs() const { return cpu.u8(address + 0x35); }
};

bool qualifies_for_club(Cpu& c, const OfferRow& row, std::int32_t id) {
    const auto club = row.club_index();
    c.set({{3, std::uint32_t(club)}, {0, std::uint32_t(id)}});
    if (compare(c, club, id) != 0) {
        const auto open = row.open_to_clubs();
        c.set(0, open);
        if (compare(c, open, 1) != 0) return false;
    }
    const auto state = row.state();
    c.set(0, state);
    return compare(c, state, 2) == 0;
}

std::uint32_t first_special_index(Cpu& c, const List& list,
                                  const OfferTable& offers, std::uint32_t back) {
    list.rewind();
    for (;;) {
        const auto record = list.next(back);
        if (!nonzero_logical(c, record)) return 0xffffffff;
        const auto index = c.u32(record);
        const auto base = offers.start();
        const auto offset = index * 4u;
        const auto row = c.u32(base + offset);
        const auto state = OfferRow{c, row}.state();
        c.set({{4, index}, {3, row}, {0, state}});
        if (compare(c, state, 19) == 0) return index;
    }
}

std::uint32_t collect_special_candidates(Cpu& c, std::uint32_t manager, std::uint32_t person,
                                         std::uint32_t list, bool by_club) {
    const auto back = by_club ? 0x103f4490u : 0x103f4424u;
    const auto target = by_club ? 0x103eb020u : 0x103eaea0u;
    if (!by_club) c.set(6, 0);
    c.set({{3, manager}, {4, person}, {5, list}});
    if (!transfer_manager_direct_calls_enabled()) {
        c.call(target, back);
        return c.r(3);
    }
    c.link(back);
    c.s.pc = target;
    const auto count = by_club ? collect_primary_club_offers(c, manager, person, list) :
                                 collect_staff_offers(c, manager, person, list, 0);
    c.finish();
    return count;
}
}

void append_staff_registration(Cpu& c, std::uint32_t manager, std::uint32_t person,
                               std::uint32_t kind, std::uint32_t details) {
    Frame frame(c, 0x103e6290, 0x90, {.first_gpr = 26});
    const auto contracts = c.toc(contracts_slot);
    c.set(7, contracts);
    const Kept<26> transfers(c, manager);
    const Kept<30> message(c, c.toc(message_slot));
    const Kept<27> staff(c, person);
    const Kept<31> pool(c, c.toc(pool_slot));
    const Kept<28> registration_kind(c, kind);
    const Kept<29> registration_details(c, details);
    if (nonzero_logical(c, call(c, lookup_registered_contract, 0x103e62c4, {c.u32(contracts)}))) {
        registration_diagnostic(c, pool, message, frame.at(0x4c), frame.at(0x38), 0xbe6, 0x103e62e4);
        const auto success = c.toc(success_slot);
        c.set(3, success);
        c.put8(success, 0);
        return;
    }
    const auto row = frame.at(0x40);
    c.put32(row, c.u32(staff));
    c.put8(row + 4, registration_kind);
    c.put32(row + 8, registration_details);
    call(c, append_node, 0x103e6344, {transfers + registrations_offset, row, 12});
}

std::uint32_t count_staff_registrations_for_club(Cpu& c, std::uint32_t manager, std::uint32_t club) {
    Frame frame(c, 0x103e6360, 0x80, {.first_gpr = 26});
    const Kept<27> requested_club(c, club);
    const bool has_club = nonzero(c, std::int32_t(club));
    const Kept<29> staff(c, c.toc(people_slot));
    const Kept<30> message(c, c.toc(message_slot));
    const Kept<26> transfers(c, manager);
    Kept<31> pool_or_peer(c, c.toc(pool_slot));
    Kept<28> count(c, 0);
    if (!has_club) {
        registration_diagnostic(c, pool_or_peer, message, frame.at(0x40), frame.at(0x38), 0xc04, 0x103e63a4);
        const auto success = c.toc(success_slot);
        c.set({{4, success}, {3, 0}});
        c.put8(success, 0);
        return 0;
    }
    pool_or_peer = call(c, associated_club, 0x103e63f8, {requested_club, 0, 1});
    const List registrations{c, transfers + registrations_offset};
    registrations.rewind();
    for (;;) {
        const auto row = registrations.next(0x103e6444);
        if (!nonzero_logical(c, row)) return result(c, count);
        const auto index = c.u32(row);
        const auto people = c.u32(staff);
        const auto employer = c.u32(people + index * staff_size + 0x24);
        c.set(4, people);
        if (compare_logical(c, employer, requested_club) == 0 ||
            (compare_logical(c, employer, pool_or_peer) == 0 && nonzero_logical(c, pool_or_peer)))
            count = count + 1;
    }
}

std::uint32_t has_staff_registration(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    Frame frame(c, 0x103e6470, 0x40, {.first_gpr = 30});
    const Kept<31> staff(c, person);
    const Kept<30> transfers(c, manager);
    const List registrations{c, transfers + registrations_offset};
    registrations.rewind();
    for (;;) {
        const auto row = registrations.next(0x103e64b8);
        if (!nonzero_logical(c, row)) return result(c, 0);
        const auto staff_id = c.i32(staff);
        c.set(4, std::uint32_t(staff_id));
        if (compare(c, staff_id, c.i32(row)) == 0) return result(c, 1);
    }
}

std::uint32_t best_qualifying_offer(Cpu& c, std::uint32_t manager,
                                    std::uint32_t person, std::uint32_t club) {
    Frame frame(c, 0x103f4240, 0x50, {.first_gpr = 26});
    const Kept<27> transfers(c, manager);
    const Kept<28> buying_club(c, club);
    Kept<30> best(c, 0);
    Kept<29> highest(c, 0xffffffff);
    const auto person_id = c.i32(person);
    const auto table = c.u32(manager + status_table_offset);
    const auto status = table + std::uint32_t(person_id) * 10u;
    c.set({{6, std::uint32_t(person_id)}, {3, status}});
    if (!nonzero_logical(c, c.u8(status + 1))) return result(c, 0);
    const OfferTable offers{c, transfers};

    for (std::uint32_t index = 0;; ++index) {
        const auto size = offers.count();
        const auto offset = index * 4u;
        c.set({{4, offset}, {5, index}, {0, std::uint32_t(size)}});
        if (compare(c, std::int32_t(index), size) >= 0) return result(c, 0);
        const auto base = offers.start();
        const Kept<31> first(c, c.u32(base + offset));
        c.set(3, base);
        if (!nonzero_logical(c, first)) continue;
        const OfferRow family{c, first};
        if (compare(c, family.person_index(), person_id) != 0) continue;
        const List linked{c, family.linked_offers()};
        c.set(3, linked.address);
        if (!nonzero_logical(c, linked.address))
            return result(c, qualifies_for_club(c, family, c.i32(buying_club)) ? family.address : 0);

        linked.rewind();
        for (;;) {
            const auto node = List{c, family.linked_offers()}.next(0x103f432c);
            if (!nonzero_logical(c, node)) return result(c, best);
            const auto linked_index = c.u32(node);
            const auto rows = offers.start();
            const auto club_index = c.i32(buying_club);
            const Kept<26> candidate(c, c.u32(rows + linked_index * 4u));
            c.set(4, rows);
            const OfferRow row{c, candidate};
            if (!qualifies_for_club(c, row, club_index)) continue;
            const auto amount = call(c, offer_value, 0x103f4314, {candidate, 0, 0, 0, 1});
            if (compare(c, std::int32_t(amount), std::int32_t(std::uint32_t(highest))) <= 0) continue;
            best = candidate;
            highest = amount;
        }
    }
}

std::uint32_t find_special_offer(Cpu& c, std::uint32_t manager, std::uint32_t person,
                                 std::uint32_t out_detail) {
    Frame frame(c, 0x103f43b0, 0x60, {.first_gpr = 28});
    const Kept<30> output(c, out_detail);
    const Kept<29> staff(c, person);
    const Kept<28> transfers(c, manager);
    const List candidates{c, frame.at(0x38)};
    call(c, construct_list, 0x103f43e0, {candidates.address});
    const OfferTable offers{c, transfers};
    Kept<31> selected(c, 0xffffffff);
    const auto staff_id = c.u32(staff);
    const auto status_table = c.u32(transfers + status_table_offset);
    const auto marker = c.u8(status_table + staff_id * 10u) & 0x10u;
    c.set(3, status_table);
    c.set(0, marker);
    if (!nonzero(c, std::int32_t(marker))) {
        call(c, destroy_list, 0x103f4408, {candidates.address, 0xffffffff});
        return result(c, 0);
    }

    const auto direct = collect_special_candidates(c, transfers, staff, candidates.address, false);
    if (nonzero(c, std::int32_t(direct))) selected = first_special_index(c, candidates, offers, 0x103f4468);
    if (compare(c, std::int32_t(std::uint32_t(selected)), -1) == 0) {
        call(c, reset_list, 0x103f4480, {candidates.address});
        const auto via_club = collect_special_candidates(c, transfers, staff, candidates.address, true);
        if (nonzero(c, std::int32_t(via_club))) selected = first_special_index(c, candidates, offers, 0x103f44d8);
    }
    if (compare(c, std::int32_t(std::uint32_t(selected)), -1) == 0) {
        const auto id = c.u32(staff);
        const auto base = c.u32(transfers + status_table_offset);
        const auto offset = id * 10u;
        const auto flags = c.u8(base + offset) & ~0x10u;
        c.set({{6, base}, {5, offset}, {0, flags}});
        c.put8(base + offset, flags);
        call(c, destroy_list, 0x103f4570, {candidates.address, 0xffffffff});
        return result(c, 0);
    }

    if (nonzero_logical(c, output)) {
        constexpr std::uint32_t detail_size = 12;
        for (std::uint32_t n = 0;; ++n) {
            const auto size = c.i32(transfers + 0xa58);
            const auto offset = n * detail_size;
            c.set({{3, offset}, {5, n}, {0, std::uint32_t(size)}});
            if (compare(c, std::int32_t(n), size) >= 0) break;
            const auto base = c.u32(transfers + 0xa50);
            const auto detail = base + offset;
            c.set(4, base);
            if (compare(c, std::int32_t(std::uint32_t(selected)), c.i32(detail + 8)) == 0) {
                c.set(0, detail);
                c.put32(output, detail);
                break;
            }
        }
    }
    call(c, destroy_list, 0x103f4544, {candidates.address, 0xffffffff});
    return result(c, 1);
}

std::uint32_t club_passes_division_gate(Cpu& c, std::uint32_t club) {
    constexpr std::int32_t restricted_nation_slot = -0x75b0, permitted_division_slot = -0x6a74;
    const auto nation = c.u32(club + 0x2c);
    c.set(5, nation);
    if (!nonzero_logical(c, nation)) return result(c, 1);
    const auto restriction = c.toc(restricted_nation_slot);
    const auto nation_index = c.i32(nation + 0x68);
    const auto restricted = c.i32(restriction);
    c.set({{4, restriction}, {5, std::uint32_t(nation_index)}, {0, std::uint32_t(restricted)}});
    if (compare(c, nation_index, restricted) != 0) return result(c, 1);
    const auto division = c.u32(club + 0x30);
    c.set(4, division);
    if (!nonzero_logical(c, division)) return result(c, 0);
    const auto index = c.i32(division);
    const auto permitted = c.i32(c.toc(permitted_division_slot));
    c.set({{4, std::uint32_t(index)}, {0, std::uint32_t(permitted)}});
    return result(c, compare(c, index, permitted) == 0);
}
}
