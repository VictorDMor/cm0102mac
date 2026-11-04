#include "transfer_manager_permissions.hpp"
#include "transfer_manager_adapter.hpp"
#include "transfer_manager_collections.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;

constexpr std::int32_t staff_count_slot = -0x7be4, staff_slot = -0x7be0, club_slot = -0x7be8,
    contract_slot = -0x7b7c, last_contract_slot = -0x7b78, source_slot = -0x4b9c,
    message_slot = -0x7ba8, success_slot = -0x7bac, approval_comparator_slot = -0x4ba8;
constexpr std::uint32_t nation_exempts_person = 0x100ca6a0, find_primary_contract = 0x10082f20,
    find_approval = 0x10775600, club_size = 0x248, person_size = 0x70,
    create_list = 0x10738c00, discard_list = 0x10738c20, next_list_row = 0x10738e00,
    viewing_club_is_controlled = 0x1014c490, person_related_to_viewer = 0x1030fbf0,
    pending_contract_with_club = 0x1007f720;

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (auto value : args) c.set(r++, value);
    c.call(entry, back);
    return c.r(3);
}

bool byte_result(Cpu& c, std::uint32_t value) {
    const auto byte = std::int8_t(value);
    c.set(0, std::uint32_t(byte));
    return nonzero(c, byte);
}

std::int32_t contract_kind(Cpu& c, std::uint32_t contract) {
    const auto flags = c.i8(contract + 0x38);
    c.s.wr<1>(reg::xer_ca, flags < 0 && (flags & 2));
    return std::int32_t(flags) >> 2;
}

std::uint8_t transfer_flags(Cpu& c, std::uint32_t manager, std::uint32_t person) {
    const auto id = c.u32(person);
    return c.u8(c.u32(manager + 0x9a0) + id * 10u);
}

bool is_live_primary(Cpu& c, std::uint32_t offer, std::uint32_t staff_table,
                     std::uint32_t contracts, std::uint32_t cache, std::uint32_t back) {
    const auto id = c.u32(offer + 0xc);
    const auto people = c.u32(staff_table);
    const auto contract = call(c, find_primary_contract, back, {c.u32(contracts), people + id * person_size});
    const bool exists = nonzero_logical(c, contract);
    c.put32(cache, contract);
    return exists && compare(c, contract_kind(c, contract), 5) != 0;
}

std::uint32_t employer(Cpu& c, std::uint32_t cache, std::uint32_t club_table) {
    const auto contract = c.u32(cache);
    const auto clubs = c.u32(club_table);
    const auto index = c.u32(contract + 4);
    c.set({{3, contract}, {4, clubs}});
    return clubs + index * club_size;
}

void fill_offer_indices(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t list) {
    constexpr auto back = 0x103f5f5c, target = 0x103eaea0;
    c.set({{3, manager}, {4, person}, {5, list}, {6, 0}});
    if (!transfer_manager_direct_calls_enabled()) {
        c.call(target, back);
        return;
    }
    c.link(back);
    c.s.pc = target;
    collect_staff_offers(c, manager, person, list, 0);
    c.finish();
}
}

std::uint32_t requires_regional_transfer_approval(Cpu& c, std::uint32_t manager,
                                                  std::uint32_t person_address, std::uint32_t club,
                                                  std::uint32_t mode) {
    Frame frame(c, 0x103ecef0, 0x90, {.first_gpr = 27});
    c.set(0, c.lr());
    const bool has_club = nonzero_logical(c, club);
    const Kept<28> message(c, c.toc(message_slot));
    const Kept<29> pool(c, c.toc(source_slot));
    const Kept<30> transfers(c, manager);
    const Kept<31> person(c, person_address);
    const Kept<27> strict(c, mode);
    if (!has_club) return result(c, 0);
    const auto nation = c.u32(club + 0x2c);
    c.set(3, nation);
    if (!nonzero_logical(c, nation)) return result(c, 0);
    if (!nonzero_logical(c, person)) {
        const auto name = frame.at(0x50), extension = frame.at(0x38);
        call(c, 0x10739890, 0x103ecf50, {pool + 0x37, 0, 0, name, extension});
        const auto version = call(c, 0x10768810, 0x103ecf54);
        call(c, 0x1077b720, 0x103ecf70, {message, pool + 0x6ff, version, name, extension, 0x1c22});
        call(c, 0x10769b60, 0x103ecf80, {pool + 0x70c, message, 0});
        const auto success = c.toc(success_slot);
        c.set({{4, success}, {3, 0}});
        c.put8(success, 0);
        return 0;
    }
    const auto nation_type = c.u8(nation + 0x118);
    c.set(0, nation_type);
    if (compare(c, nation_type, 14) != 0) return result(c, 0);
    if (byte_result(c, call(c, nation_exempts_person, 0x103ecfac, {person, 0}))) return result(c, 0);
    if (byte_result(c, strict)) return result(c, 1);

    auto contract = call(c, find_primary_contract, 0x103ecfd4, {c.u32(c.toc(contract_slot)), person});
    if (nonzero_logical(c, contract)) {
        const auto kind = contract_kind(c, contract);
        c.set(0, std::uint32_t(kind));
        if (compare(c, kind, 2) != 0) contract = 0;
    }
    c.set(3, contract);
    if (!nonzero_logical(c, contract)) return result(c, 1);
    if (nonzero_logical(c, person)) {
        const auto count_address = c.toc(staff_count_slot);
        const auto id = c.i32(person);
        const auto count = c.i32(count_address);
        c.set({{0, std::uint32_t(count)}, {4, std::uint32_t(id)}});
        if (compare(c, id, count) >= 0) {
            compare_logical(c, 0);
            return result(c, 1);
        }
    }
    const auto id = c.u32(person);
    c.set(4, id);
    const auto table = c.u32(transfers + 0x9a0);
    c.set(3, table);
    const auto approved = c.u8(table + id * 10u) & 0x20u;
    c.set(0, approved);
    if (!nonzero(c, std::int32_t(approved))) {
        compare_logical(c, 0);
        return result(c, 1);
    }
    c.put32(frame.at(0x40), id);
    const auto comparator = c.toc(approval_comparator_slot);
    const auto records = c.u32(transfers + 0xa6c), count = c.u32(transfers + 0xa70);
    const auto approval = call(c, find_approval, 0x103ed060, {frame.at(0x40), records, count, 16, comparator});
    return result(c, !nonzero_logical(c, approval));
}

std::uint32_t find_nation_transfer_record(Cpu& c, std::uint32_t manager, std::uint32_t club) {
    const auto nation = c.u32(club + 0x2c);
    c.set(6, nation);
    auto cursor = manager + 0x9e0;
    for (std::int32_t index = 0; ; ++index, cursor += 4) {
        c.set({{5, cursor}, {4, std::uint32_t(index)}});
        c.set(0, std::uint32_t(index));
        if (compare(c, index, 27) >= 0) return result(c, 0);
        const auto row = c.u32(cursor);
        c.set(3, row);
        const auto owner = c.u32(row + 0xc);
        c.set(0, owner);
        if (compare_logical(c, owner, nation) == 0) return row;
    }
}

std::uint32_t has_visible_offer(Cpu& c, std::uint32_t manager, std::uint32_t person,
                                 std::uint32_t viewer_mode) {
    Frame frame(c, 0x103f5e50, 0x80, {.first_gpr = 21});
    Kept<21> viewers_or_active_contract(c, c.toc(-0x7bb0));
    const Kept<22> transfers(c, manager);
    const Kept<27> people(c, c.toc(staff_slot));
    const Kept<23> staff(c, person);
    const Kept<28> contracts(c, c.toc(contract_slot));
    Kept<24> mode_or_offer(c, viewer_mode);
    const Kept<29> cache(c, c.toc(last_contract_slot));
    const Kept<30> clubs(c, c.toc(club_slot));
    const auto list = frame.at(0x38);
    call(c, create_list, 0x103f5e88, {list});
    const auto finish = [&](std::uint32_t back, std::uint32_t answer) {
        call(c, discard_list, back, {list, 0xffffffff});
        return result(c, answer);
    };
    if (nonzero_logical(c, staff)) {
        const auto count = c.toc(staff_count_slot);
        const auto id = c.i32(staff);
        c.set(4, std::uint32_t(id));
        if (compare(c, id, c.i32(count)) >= 0) return finish(0x103f5eb0, 0);
    }
    if (!byte_result(c, mode_or_offer)) {
        const bool pending = nonzero(c, transfer_flags(c, transfers, staff) & 1u);
        return pending ? finish(0x103f5ee4, 1) : finish(0x103f616c, 0);
    }

    const Kept<26> viewer_index(c, viewers_or_active_contract + 0x2c58);
    const Kept<25> viewer_records(c, viewers_or_active_contract + 4);
    const auto current_viewer = [&] {
        const auto offset = std::uint32_t(std::int32_t(c.i16(viewer_index))) * 0x2c0;
        return c.u32(viewer_records + offset);
    };
    const auto viewer = current_viewer();
    c.set(5, viewer);
    if (!nonzero_logical(c, viewer)) {
        const auto id = c.u32(staff);
        const auto flags = c.u32(transfers + 0x9a0);
        c.set(5, flags);
        viewers_or_active_contract = c.u8(flags + id * 10u) & 1u;
        call(c, discard_list, 0x103f5f2c, {list, 0xffffffff});
        return result(c, viewers_or_active_contract);
    }

    const Kept<31> related(c, call(c, person_related_to_viewer, 0x103f5f44, {c.u32(c.toc(-0x7b1c)), staff}));
    fill_offer_indices(c, transfers, staff, list);
    c.put32(list + 0xc, c.u32(list + 4));
    for (;;) {
        const auto node = call(c, next_list_row, 0x103f60fc, {list});
        if (!nonzero_logical(c, node)) break;
        const auto index = c.u32(node);
        const auto table = c.u32(transfers);
        c.set(3, table);
        mode_or_offer = c.u32(table + index * 4u);
        if (byte_result(c, c.u8(mode_or_offer + 0x35))) return finish(0x103f60ec, 1);
        const auto buyer_index = c.u32(mode_or_offer + 8);
        const auto buyer = c.u32(clubs) + buyer_index * club_size;
        if (byte_result(c, call(c, viewing_club_is_controlled, 0x103f5fa4, {buyer, 0, 1})))
            return finish(0x103f5fb8, 1);

        viewers_or_active_contract = 0;
        viewers_or_active_contract = is_live_primary(c, mode_or_offer, people, contracts, cache, 0x103f5fdc);
        const auto first_seller = nonzero(c, std::int32_t(std::uint32_t(viewers_or_active_contract))) ?
            employer(c, cache, clubs) : 0;
        if (nonzero_logical(c, first_seller)) {
            const auto offer = std::uint32_t(mode_or_offer);
            mode_or_offer = 0;
            mode_or_offer = is_live_primary(c, offer, people, contracts, cache, 0x103f604c);
            const auto second_seller = nonzero(c, std::int32_t(std::uint32_t(mode_or_offer))) ?
                employer(c, cache, clubs) : 0;
            if (byte_result(c, call(c, viewing_club_is_controlled, 0x103f60a4, {second_seller, 0, 1})))
                return finish(0x103f60b8, 1);
        }
        if (byte_result(c, related)) return finish(0x103f60d8, 1);
    }

    const auto record = current_viewer();
    c.set(3, record);
    const auto managed_club = c.u32(record + 0x24);
    c.set(5, managed_club);
    if (nonzero_logical(c, managed_club)) {
        const auto contract = call(c, pending_contract_with_club, 0x103f612c,
                                   {c.u32(contracts), c.u32(staff), c.u32(managed_club)});
        if (nonzero_logical(c, contract) && compare(c, contract_kind(c, contract), 5) == 0)
            return finish(0x103f6158, 1);
    }
    return finish(0x103f616c, 0);
}
}
