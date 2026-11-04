#include "transfer_manager_rejection.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;

constexpr std::int32_t contracts_slot = -0x7b7c, last_contract_slot = -0x7b78,
    clubs_slot = -0x7be8, people_slot = -0x7be0, messages_slot = -0x7ba8, pool_slot = -0x4b9c;
constexpr std::uint32_t person_size = 0x70, club_size = 0x248,
    primary_contract = 0x10082f20, contract_with_club = 0x10082f90,
    club_is_controlled = 0x1014c2e0, clubs_are_rivals = 0x10464160, copy_text = 0x10727890;

enum Reason : std::int16_t {
    no_reason = 0,
    unacceptable_offer = 10,
    indispensable = 0x200,
    important_first_team = 0x400,
    rival = 0x800,
    same_division = 0x1000,
    future_prospect = 0x2000,
};

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args) {
    unsigned r = 3;
    for (auto value : args) c.set(r++, value);
    c.call(entry, back);
    return c.r(3);
}

std::uint32_t lookup_primary(Cpu& c, std::uint32_t contracts, std::uint32_t person,
                              std::uint32_t cache, std::uint32_t back) {
    const auto found = call(c, primary_contract, back, {c.u32(contracts), person});
    compare_logical(c, found);
    c.put32(cache, found);
    return found;
}

std::uint32_t employer_for(Cpu& c, std::uint32_t contract, std::uint32_t club_table_holder) {
    if (!contract) return 0;
    const auto index = c.u32(contract + 4);
    const auto clubs = c.u32(club_table_holder);
    c.set(3, clubs);
    return clubs + index * club_size;
}

std::uint32_t squad_status(Cpu& c, std::uint32_t contract) { return c.u8(contract + 0x60) & 0xf; }

std::uint32_t reason_for_status(Cpu& c, std::uint32_t status) {
    const auto middle = compare(c, std::int32_t(status), 4);
    if (middle == 0) return no_reason;
    if (middle < 0) {
        const auto low = compare(c, std::int32_t(status), 1);
        return low < 0 ? no_reason : low == 0 ? indispensable : important_first_team;
    }
    return compare(c, std::int32_t(status), 6) >= 0 ? no_reason : future_prospect;
}

bool byte_result(Cpu& c, std::uint32_t value) {
    const auto byte = std::int8_t(value);
    c.set(0, std::uint32_t(byte));
    return nonzero(c, byte);
}

bool names_rival(Cpu& c, std::uint32_t club, std::uint32_t other) {
    for (std::uint32_t offset : {0x74, 0x78, 0x7c}) {
        const auto listed = c.u32(club + offset);
        c.set(0, listed);
        if (compare_logical(c, listed, other) == 0) return true;
    }
    return false;
}

struct ReasonText {
    std::uint32_t offset = 0;
    std::uint32_t back = 0;
};

ReasonText select_reason_text(Cpu& c, std::int16_t reason) {
    // The search also leaves the last comparison in CR0 for unlisted codes.
    const auto against_key = compare(c, reason, important_first_team);
    if (against_key == 0) return {0x1912, 0x103ea704};
    if (against_key < 0) {
        const auto against_special = compare(c, reason, unacceptable_offer);
        if (against_special == 0) return {0x19a0, 0x103ea734};
        if (against_special < 0)
            return compare(c, reason, no_reason) == 0 ? ReasonText{0x18ae, 0x103ea6e4} : ReasonText{};
        return compare(c, reason, indispensable) == 0 ? ReasonText{0x18e9, 0x103ea6f4} : ReasonText{};
    }
    const auto against_division = compare(c, reason, same_division);
    if (against_division == 0) return {0x196d, 0x103ea724};
    if (against_division < 0)
        return compare(c, reason, rival) == 0 ? ReasonText{0x19c2, 0x103ea744} : ReasonText{};
    return compare(c, reason, future_prospect) == 0 ? ReasonText{0x1940, 0x103ea714} : ReasonText{};
}
}

std::uint32_t rejection_reason_from_squad_status(Cpu& c, std::uint32_t offer) {
    Frame frame(c, 0x103ea350, 0x50, {.first_gpr = 29});
    const auto people = c.toc(people_slot);
    const Kept<30> contracts(c, c.toc(contracts_slot));
    const Kept<31> cache(c, c.toc(last_contract_slot));
    const auto person_index = c.u32(offer + 0xc);
    const Kept<29> person(c, c.u32(people) + person_index * person_size);
    const auto primary = lookup_primary(c, contracts, person, cache, 0x103ea390);
    const auto club = employer_for(c, primary, primary ? c.toc(clubs_slot) : 0);
    const auto contract = call(c, contract_with_club, 0x103ea3c4, {c.u32(contracts), person, club});
    if (!nonzero_logical(c, contract)) return result(c, no_reason);
    return result(c, reason_for_status(c, squad_status(c, contract)));
}

std::uint32_t rejection_reason_allowed(Cpu& c, std::uint32_t reason_bits, std::uint32_t offer) {
    Frame frame(c, 0x103ea440, 0x50, {.first_gpr = 27});
    const Kept<27> row(c, offer);
    const auto people = c.toc(people_slot);
    c.set(5, people);
    const Kept<30> reason_word(c, reason_bits);
    const auto contracts = c.toc(contracts_slot);
    Kept<31> cache_or_employer(c, c.toc(last_contract_slot));
    Kept<29> table_or_buyer(c, c.toc(clubs_slot));
    const auto person_index = c.u32(row + 0xc);
    const Kept<28> person(c, c.u32(people) + person_index * person_size);
    const auto primary = lookup_primary(c, contracts, person, cache_or_employer, 0x103ea484);
    cache_or_employer = employer_for(c, primary, table_or_buyer);
    const auto buyer_index = c.u32(row + 8);
    const auto reason = std::int16_t(std::uint32_t(reason_word));
    const bool age_gate = compare(c, reason, future_prospect) == 0;
    const auto clubs = c.u32(table_or_buyer);
    c.set(4, clubs);
    table_or_buyer = clubs + buyer_index * club_size;
    if (age_gate && compare(c, std::int8_t(c.i16(person + 0x5c)), 24) >= 0) return result(c, 0);

    const auto employer = std::uint32_t(cache_or_employer);
    const auto buyer = std::uint32_t(table_or_buyer);
    if (compare(c, reason, same_division) == 0 && nonzero_logical(c, employer)) {
        const auto seller_division = c.u32(employer + 0x30);
        const auto buyer_division = c.u32(buyer + 0x30);
        c.set(3, seller_division);
        if (compare_logical(c, seller_division, buyer_division) != 0) return result(c, 0);
    }
    if (compare(c, reason, rival) != 0 || !nonzero_logical(c, buyer) || !nonzero_logical(c, employer))
        return result(c, 1);
    if (byte_result(c, call(c, club_is_controlled, 0x103ea53c, {employer, 0, 1, 0, 0})) &&
        byte_result(c, call(c, club_is_controlled, 0x103ea55c, {buyer, 0, 1, 0, 0}))) return result(c, 1);
    if (byte_result(c, call(c, clubs_are_rivals, 0x103ea578, {buyer, employer}))) return result(c, 1);
    return result(c, names_rival(c, buyer, employer) || names_rival(c, employer, buyer));
}

void describe_rejection_reason(Cpu& c, std::uint32_t reason_bits, std::uint32_t output, std::int32_t capacity) {
    Frame frame(c, 0x103ea600, 0x70, {.first_gpr = 30});
    const bool too_small = compare(c, capacity, 50) < 0;
    const Kept<31> pool(c, c.toc(pool_slot));
    const Kept<30> message(c, c.toc(messages_slot));
    if (too_small) {
        const auto name = frame.at(0x40), extension = frame.at(0x38);
        call(c, 0x10739890, 0x103ea63c, {pool + 0x37, 0, 0, name, extension});
        const auto version = call(c, 0x10768810, 0x103ea640, {});
        call(c, 0x1077b720, 0x103ea65c, {message, pool + 0x6ff, version, name, extension, 0x163c});
        call(c, 0x10769b60, 0x103ea66c, {pool + 0x70c, message, 0});
        const auto success = c.toc(-0x7bac);
        c.set(3, success);
        c.put8(success, 0);
        return;
    }
    const auto reason = std::int16_t(reason_bits);
    const auto text = select_reason_text(c, reason);
    if (!text.offset) return;
    if (reason == no_reason) c.set(5, pool + 0x18e8);
    call(c, copy_text, text.back, {output, pool + text.offset});
}

std::uint32_t squad_status_for_rejection_reason(Cpu& c, std::uint32_t person_address, std::uint32_t reason_bits) {
    Frame frame(c, 0x103ea760, 0x50, {.first_gpr = 28});
    const Kept<31> cache(c, c.toc(last_contract_slot));
    const Kept<30> contracts(c, c.toc(contracts_slot));
    const Kept<29> reason_word(c, reason_bits);
    const Kept<28> person(c, person_address);
    const auto primary = lookup_primary(c, contracts, person, cache, 0x103ea794);
    const auto club = employer_for(c, primary, primary ? c.toc(clubs_slot) : 0);
    const auto contract = call(c, contract_with_club, 0x103ea7c8, {c.u32(contracts), person, club});
    if (!nonzero_logical(c, contract)) return result(c, 0);
    const auto reason = std::int16_t(std::uint32_t(reason_word));
    const auto current = squad_status(c, contract);
    const auto against_key = compare(c, reason, important_first_team);
    if (against_key == 0) return result(c, 2);
    if (against_key < 0) return result(c, compare(c, reason, indispensable) == 0 ? 1 : current);
    return result(c, compare(c, reason, future_prospect) == 0 ? 5 : current);
}
}
