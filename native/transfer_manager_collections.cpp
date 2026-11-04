#include "transfer_manager_collections.hpp"
#include "transfer_manager_adapter.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;

constexpr std::int32_t staff_count_slot = -0x7be4, contracts_slot = -0x7b7c,
    last_contract_slot = -0x7b78, clubs_slot = -0x7be8;
constexpr std::uint32_t filter_entry = 0x103fb390, next_list_item = 0x10738e00,
    append_list_item = 0x10738d10, primary_contract = 0x10082f20, club_size = 0x248;

struct OfferArray {
    Cpu& cpu;
    std::uint32_t address;

    std::uint32_t start() const { return cpu.u32(address); }
    std::int32_t size() const { return cpu.i32(address + 4); }
    std::uint32_t slot(std::uint32_t index) const { return start() + index * 4; }
};

struct Offer {
    Cpu& cpu;
    std::uint32_t address;

    std::int32_t club_index() const { return cpu.i32(address + 8); }
    std::int32_t staff_index() const { return cpu.i32(address + 0xc); }
    std::uint32_t linked_offers() const { return cpu.u32(address); }
    std::uint32_t linked_staff() const { return cpu.u32(address + 4); }
    std::uint32_t index_address() const { return address + 0x28; }
};

bool outside_staff_table(Cpu& c, std::int32_t id) {
    const auto staff_count = c.toc(staff_count_slot);
    c.set(3, staff_count);
    const auto count = c.i32(staff_count);
    c.set(0, std::uint32_t(count));
    return compare(c, id, count) >= 0;
}

void reset_list(Cpu& c, std::uint32_t list) {
    const auto head = c.u32(list + 4);
    c.set(0, head);
    c.put32(list + 0xc, head);
}

std::uint32_t next(Cpu& c, std::uint32_t list, std::uint32_t back) {
    c.set(3, list);
    c.call(next_list_item, back);
    return c.r(3);
}

void append_index(Cpu& c, std::uint32_t output, std::uint32_t index_address, std::uint32_t back) {
    c.set({{3, output}, {4, index_address}, {5, 4}});
    c.call(append_list_item, back);
}

bool filtered(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t back) {
    c.set({{3, manager}, {4, offer}});
    if (transfer_manager_direct_calls_enabled()) {
        c.link(back);
        c.s.pc = filter_entry;
        offer_is_filtered(c, offer);
        c.finish();
    } else {
        c.call(filter_entry, back);
    }
    const auto decision = std::int8_t(c.r(3));
    c.set(0, std::uint32_t(decision));
    return nonzero(c, decision);
}

std::uint32_t contract_club(Cpu& c, std::uint32_t contract, std::uint32_t clubs) {
    if (!contract) return 0;
    const auto offset = c.u32(contract + 4) * club_size;
    const auto table = c.u32(clubs);
    c.set({{0, offset}, {3, table}});
    return table + offset;
}

std::uint32_t lookup_contract(Cpu& c, std::uint32_t contracts, std::uint32_t person,
                               std::uint32_t last_contract, std::uint32_t back) {
    c.set({{3, c.u32(contracts)}, {4, person}});
    c.call(primary_contract, back);
    const auto contract = c.r(3);
    compare_logical(c, contract);
    c.put32(last_contract, contract);
    return contract;
}

std::uint32_t slot(Cpu& c, const OfferArray& offers, std::int32_t index) {
    const auto size = offers.size();
    c.set(0, std::uint32_t(size));
    if (compare(c, index, size) >= 0) return result(c, offers.start());
    const auto offset = std::uint32_t(index) * 4;
    c.set(0, offset);
    return result(c, offers.start() + offset);
}
}

std::uint32_t offer_is_filtered(Cpu& c, std::uint32_t offer) {
    const auto kind = c.u8(offer + 0x34);
    c.set(5, kind);
    const auto matches_pair = [&](unsigned first) {
        const auto distance = std::uint8_t(kind - first);
        c.set(0, distance);
        return compare_logical(c, distance, 1) <= 0;
    };
    if (!matches_pair(2)) {
        const auto signed_kind = std::int8_t(kind);
        c.set(3, std::uint32_t(signed_kind));
        if (compare(c, signed_kind, 20) != 0 && !matches_pair(15) &&
            compare(c, signed_kind, 4) != 0) return result(c, 0);
    }
    c.set(0, std::uint32_t(std::int8_t(kind)));
    if (compare(c, std::int8_t(kind), 15) == 0) {
        const auto stage = c.u8(offer + 0x36);
        c.set(0, stage);
        if (compare_logical(c, stage, 3) < 0) return result(c, 0);
    }
    return result(c, 1);
}

std::uint32_t primary_offer_slot(Cpu& c, std::uint32_t manager, std::int32_t index) {
    return slot(c, {c, manager}, index);
}

std::uint32_t secondary_offer_slot(Cpu& c, std::uint32_t manager, std::int32_t index) {
    return slot(c, {c, manager + 0xc}, index);
}

std::uint32_t collect_staff_offers(Cpu& c, std::uint32_t manager, std::uint32_t person,
                                   std::uint32_t output, std::uint32_t include_filtered) {
    Frame frame(c, 0x103eaea0, 0x60, {.first_gpr = 25});
    const bool has_person = nonzero_logical(c, person);
    const Kept<28> transfers(c, manager);
    const Kept<29> list(c, output);
    // Only the low byte is tested, but the whole saved argument reaches calls.
    const Kept<30> mode(c, include_filtered);
    Kept<31> first_offer(c, 0);
    const Kept<27> staff_id(c, c.u32(person));
    if (has_person && outside_staff_table(c, std::int32_t(std::uint32_t(staff_id)))) return result(c, 0);
    const OfferArray offers{c, transfers};
    auto cursor = offers.start();
    std::uint32_t index = 0;
    for (;;) {
        const auto size = offers.size();
        c.set({{4, cursor}, {5, index}, {0, std::uint32_t(size)}});
        if (compare(c, std::int32_t(index), size) >= 0) break;
        const auto candidate = c.u32(cursor);
        c.set(3, candidate);
        if (nonzero_logical(c, candidate) &&
            compare(c, Offer{c, candidate}.staff_index(), std::int32_t(std::uint32_t(staff_id))) == 0) {
            first_offer = candidate;
            break;
        }
        ++index;
        cursor += 4;
    }
    if (!nonzero_logical(c, first_offer)) return result(c, 0);
    const Offer first{c, first_offer};
    const auto children = first.linked_offers();
    c.set(3, children);
    if (!nonzero_logical(c, children)) {
        if (!nonzero(c, std::int8_t(std::uint32_t(mode))) &&
            filtered(c, transfers, first_offer, 0x103eafd4)) return result(c, 0);
        if (nonzero_logical(c, list)) append_index(c, list, first.index_address(), 0x103eaff4);
        return result(c, 1);
    }

    Kept<25> count(c, 0);
    reset_list(c, children);
    for (;;) {
        const Kept<26> node(c, next(c, first.linked_offers(), 0x103eafb0));
        if (!nonzero(c, std::int32_t(std::uint32_t(node)))) break;
        const auto item_index = c.u32(node);
        const auto base = offers.start();
        const auto candidate = c.u32(base + item_index * 4u);
        c.set({{3, base}, {4, candidate}});
        if (!nonzero_logical(c, candidate)) continue;
        if (compare(c, std::int32_t(std::uint32_t(staff_id)), Offer{c, candidate}.staff_index()) != 0) continue;
        if (!nonzero(c, std::int8_t(std::uint32_t(mode))) &&
            filtered(c, transfers, candidate, 0x103eaf84)) continue;
        if (nonzero_logical(c, list)) append_index(c, list, node, 0x103eafa4);
        count = count + 1;
    }
    return result(c, count);
}

std::uint32_t collect_primary_club_offers(Cpu& c, std::uint32_t manager, std::uint32_t person,
                                          std::uint32_t output) {
    Frame frame(c, 0x103eb020, 0x60, {.first_gpr = 25});
    const Kept<26> staff(c, person);
    const bool has_person = nonzero(c, std::int32_t(person));
    Kept<31> contracts_or_cursor(c, c.toc(contracts_slot));
    Kept<30> last_contract_or_index(c, c.toc(last_contract_slot));
    const Kept<25> transfers(c, manager);
    Kept<28> clubs_or_index(c, c.toc(clubs_slot));
    const Kept<27> list(c, output);
    Kept<29> count(c, 0);
    if (has_person) {
        const auto id = c.i32(staff);
        c.set(4, std::uint32_t(id));
        if (outside_staff_table(c, id)) return result(c, 0);
    }
    const auto id = c.u32(staff);
    const auto table = c.u32(manager + 0x9a0);
    c.set(3, table + id * 10u);
    if (!nonzero_logical(c, c.u8(table + id * 10u + 2))) return result(c, 0);

    const auto first = lookup_contract(c, contracts_or_cursor, staff, last_contract_or_index, 0x103eb09c);
    if (!nonzero_logical(c, contract_club(c, first, clubs_or_index))) return result(c, 0);
    // The first lookup is a presence gate. The original repeats the query
    // and reloads its employer before walking the primary offer slots.
    const auto second = lookup_contract(c, contracts_or_cursor, staff, last_contract_or_index, 0x103eb0dc);
    const auto employer = contract_club(c, second, clubs_or_index);
    c.set(3, employer);
    clubs_or_index = c.u32(employer);
    const OfferArray offers{c, transfers};
    last_contract_or_index = 0;
    contracts_or_cursor = offers.start();
    while (compare(c, std::int32_t(std::uint32_t(last_contract_or_index)), offers.size()) < 0) {
        const auto candidate = c.u32(contracts_or_cursor);
        c.set(4, candidate);
        if (nonzero_logical(c, candidate) &&
            compare(c, Offer{c, candidate}.club_index(), std::int32_t(std::uint32_t(clubs_or_index))) == 0 &&
            !filtered(c, transfers, candidate, 0x103eb130)) {
            const auto offer_at_cursor = [&] { return Offer{c, c.u32(contracts_or_cursor)}; };
            const auto staff_list = offer_at_cursor().linked_staff();
            c.set(3, staff_list);
            if (nonzero_logical(c, staff_list)) {
                reset_list(c, staff_list);
                for (;;) {
                    const auto node = next(c, offer_at_cursor().linked_staff(), 0x103eb194);
                    if (!nonzero_logical(c, node)) break;
                    const auto member = c.i32(node);
                    c.set(3, std::uint32_t(member));
                    if (compare(c, member, c.i32(staff)) != 0) continue;
                    if (nonzero_logical(c, list)) append_index(c, list, offer_at_cursor().index_address(), 0x103eb180);
                    count = count + 1;
                    break;
                }
            }
            const auto staff_id = c.u32(staff);
            const auto status_table = c.u32(transfers + 0x9a0);
            const auto offset = staff_id * 10u;
            c.set({{4, status_table}, {3, offset}});
            if (compare(c, std::int32_t(std::uint32_t(count)), c.u8(status_table + offset + 2)) == 0) break;
        }
        last_contract_or_index = last_contract_or_index + 1;
        contracts_or_cursor = contracts_or_cursor + 4;
    }
    return result(c, count);
}

std::uint32_t collect_secondary_club_offers(Cpu& c, std::uint32_t manager, std::uint32_t person,
                                            std::uint32_t output) {
    Frame frame(c, 0x103eb1f0, 0x60, {.first_gpr = 24});
    const Kept<29> staff(c, person);
    const bool has_person = nonzero(c, std::int32_t(person));
    Kept<27> last_contract_or_offer(c, c.toc(last_contract_slot));
    const Kept<28> transfers(c, manager);
    const Kept<30> list(c, output);
    Kept<31> count(c, 0);
    if (has_person) {
        const auto id = c.i32(staff);
        c.set(4, std::uint32_t(id));
        if (outside_staff_table(c, id)) return result(c, 0);
    }
    const auto contract = lookup_contract(c, c.toc(contracts_slot), staff, last_contract_or_offer, 0x103eb244);
    const auto employer = contract_club(c, contract, contract ? c.toc(clubs_slot) : 0);
    c.set(3, employer);
    if (!nonzero_logical(c, employer)) return result(c, 0);
    const Kept<24> club_index(c, c.u32(employer));
    const OfferArray primary{c, transfers}, secondary{c, transfers + 0xc};
    Kept<25> index(c, 0);
    Kept<26> cursor(c, secondary.start());
    while (compare(c, std::int32_t(std::uint32_t(index)), secondary.size()) < 0) {
        last_contract_or_offer = c.u32(cursor);
        const Offer offer{c, last_contract_or_offer};
        if (nonzero_logical(c, offer.address) &&
            compare(c, offer.club_index(), std::int32_t(std::uint32_t(club_index))) == 0) {
            const auto index_value = c.u32(offer.index_address());
            const auto base = primary.start();
            const auto original_offer = c.u32(base + index_value * 4u);
            c.set({{3, base}, {4, original_offer}});
            if (nonzero_logical(c, original_offer) && !filtered(c, transfers, original_offer, 0x103eb2c4)) {
                const auto staff_list = offer.linked_staff();
                c.set(3, staff_list);
                if (nonzero_logical(c, staff_list)) {
                    reset_list(c, staff_list);
                    for (;;) {
                        const auto node = next(c, offer.linked_staff(), 0x103eb324);
                        if (!nonzero_logical(c, node)) break;
                        const auto member = c.i32(node);
                        c.set(3, std::uint32_t(member));
                        if (compare(c, member, c.i32(staff)) != 0) continue;
                        if (nonzero_logical(c, list)) append_index(c, list, offer.index_address(), 0x103eb310);
                        count = count + 1;
                        break;
                    }
                }
            }
        }
        index = index + 1;
        cursor = cursor + 4;
    }
    return result(c, count);
}

std::uint32_t collect_offers_from_club(Cpu& c, std::uint32_t manager, std::uint32_t club,
                                       std::uint32_t output) {
    Frame frame(c, 0x103eb360, 0x50, {.first_gpr = 28});
    Kept<30> count(c, 0);
    const Kept<29> list(c, output);
    const Kept<28> transfers(c, manager);
    const Kept<31> club_index(c, c.u32(club));
    const auto index_address = frame.at(0x38);
    const OfferArray offers{c, transfers};
    c.put32(index_address, 0);
    for (;;) {
        const auto index = c.i32(index_address), size = offers.size();
        c.set({{4, std::uint32_t(index)}, {0, std::uint32_t(size)}});
        if (compare(c, index, size) >= 0) break;
        const auto base = offers.start();
        const auto candidate = c.u32(base + std::uint32_t(index) * 4u);
        c.set({{3, base}, {4, candidate}});
        if (nonzero_logical(c, candidate) && !filtered(c, transfers, candidate, 0x103eb3b4)) {
            const auto current = c.u32(offers.slot(c.u32(index_address)));
            c.set(3, current);
            if (compare(c, std::int32_t(std::uint32_t(club_index)), Offer{c, current}.club_index()) == 0) {
                if (nonzero_logical(c, list)) append_index(c, list, index_address, 0x103eb3f0);
                count = count + 1;
            }
        }
        const auto previous = c.u32(index_address);
        c.set(3, previous);
        c.put32(index_address, previous + 1);
    }
    return result(c, count);
}
}
