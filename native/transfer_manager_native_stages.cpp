#include "transfer_manager_native_stages.hpp"
#include "transfer_manager_native_stages_detail.hpp"

namespace cm::aot::transfer_manager_native {
using namespace stages;

// 0x103e80e0: moves an offer on once its club has answered. Stage 11 becomes 13 for the club of the offer's own
// person and 12 otherwise, stage 12 or 13 becomes 14 when the other side has answered too; at 14 the offer is
// either settled (when the date it was agreed for has come) or sent back to 0x103eb4a0.
void route_answered_offer(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t club) {
    constexpr std::uint32_t date_cell = 0x7b80, offer_flags = 0x38, answered = 14;
    Frame frame(c, 0x103e80e0, 0x60, {.first_gpr = 29});
    const auto date_at = frame.at(0x40), now_at = frame.at(0x38);
    keep(c, 31, offer);
    keep(c, 30, manager);
    keep(c, 29, club);
    invoke(c, 0x10715880, 0x103e810c, {date_at});
    const bool own_club = club && c.i32(club) == c.i32(offer + offer_club);
    const auto stage = [&] { return c.i8(offer + offer_stage); };
    if (own_club) {
        if (stage() == 11) c.put8(offer + offer_stage, 13);
        else if (stage() == 12) c.put8(offer + offer_stage, answered);
    } else {
        if (stage() == 11) c.put8(offer + offer_stage, 12);
        else if (stage() == 13) c.put8(offer + offer_stage, answered);
    }
    if (c.u8(offer + offer_stage) != answered) return;
    if ((c.i8(offer + offer_flags) >> 4) != 0) {
        invoke_native(c, 0x103f89e0, 0x103e81b0, {manager, offer, date_at});
        // The game's date is copied beside the offer's date and compared with it: the year's low halfword
        // unsigned, then the high halfword signed.
        const auto today = c.toc(-date_cell);
        c.put32(now_at, c.u32(today));
        c.put32(now_at + 4, c.u32(today + 4));
        const std::uint16_t today_low = c.u16(today + 2), due_low = c.u16(date_at + 2);
        bool due;
        if (today_low < due_low) due = true;
        else if (today_low > due_low) due = false;
        else due = c.i16(today) < c.i16(date_at);
        if (due) {
            invoke_native(c, 0x103ebf50, 0x103e821c, {manager, offer});
            const auto contracts = c.toc(toc::contracts);
            invoke(c, 0x10082ea0, 0x103e822c, {c.u32(contracts), c.u32(offer + offer_person)});
            invoke_native(c, 0x1040c220, 0x103e823c, {manager, offer, date_at});
            invoke(c, 0x103f2ec0, 0x103e824c, {manager, offer, date_at});
            return;
        }
    }
    invoke(c, 0x103eb4a0, 0x103e8264, {manager, offer, 0, 1});
}

// 0x103ebf50: settles the people of an offer that has been agreed: removes the offer's person and each listed
// person from the other bids (0x103e95a0), and for a listed person with a status row's second flag sends that
// person's other offers back to stage 15 with a refusal count of 3.
void settle_agreed_offer(Cpu& c, std::uint32_t manager, std::uint32_t offer) {
    constexpr std::uint32_t row_other_offers = 1, other_offer_waiting = 0x13, refused = 0x0f, refused_times = 3;
    Frame frame(c, 0x103ebf50, 0x70, {.first_gpr = 28});
    const auto people_cell = c.toc(toc::people);
    const auto outer_list = frame.at(0x48), person_offers = frame.at(0x38);
    keep(c, 31, people_cell);
    keep(c, 30, offer);
    keep(c, 29, manager);
    invoke(c, list_construct, 0x103ebf80, {outer_list});
    invoke_native(c, 0x103e95a0, 0x103ebfa4, {manager, c.u32(people_cell) + c.u32(offer + offer_person) * person_size, offer, 0, 0});
    if (const auto listed = c.u32(offer + offer_listed)) {
        rewind_list(c, listed);
        for (;;) {
            const auto entry = invoke(c, next_listed, 0x103ec0b4, {c.u32(offer + offer_listed)});
            keep(c, 28, entry);
            if (!entry) break;
            invoke_native(c, 0x103e95a0, 0x103ebfe0, {manager, c.u32(people_cell) + c.u32(entry) * person_size, offer, 0, 0});
            const auto person = c.u32(people_cell) + c.u32(entry) * person_size;
            keep(c, 28, person);
            invoke(c, list_construct, 0x103ebff8, {person_offers});
            if (c.u8(c.u32(manager + status_rows) + c.u32(person) * status_row_size + row_other_offers) != 0) {
                invoke(c, 0x103eaea0, 0x103ec028, {manager, person, person_offers, 0});
                c.put32(person_offers + list_cursor, c.u32(person_offers + 4));
                for (;;) {
                    const auto other = invoke(c, next_listed, 0x103ec098, {person_offers});
                    if (!other) break;
                    const auto other_offer = c.u32(c.u32(manager) + c.u32(other) * 4);
                    keep(c, 28, other_offer);
                    if (c.u8(other_offer + offer_stage) == other_offer_waiting) invoke(c, 0x103f3540, 0x103ec060, {manager, other_offer});
                    c.put8(other_offer + offer_stage, refused);
                    c.put8(other_offer + offer_refusals, refused_times);
                    invoke(c, 0x103ec4f0, 0x103ec07c, {manager, other_offer});
                    invoke(c, 0x103eb4a0, 0x103ec090, {manager, other_offer, 10, 0});
                }
            }
            invoke(c, list_destroy, 0x103ec0ac, {person_offers, 0xffffffff});
        }
    }
    invoke(c, list_destroy, 0x103ec0c8, {outer_list, 0xffffffff});
}

namespace {
// Frees the offer's listed-people list once it has emptied (its count word is zero).
void free_emptied_list(Cpu& c, std::uint32_t offer, std::uint32_t back) {
    const auto list = c.u32(offer + offer_listed);
    if (c.i16(list) != 0) return;
    invoke(c, list_destroy, back, {list, 1});
    c.put32(offer + offer_listed, 0);
}
}

// 0x103e95a0: takes `person` off the offer's listed people. With `as_listed_person` the offer is the one that
// lists them (the club asked about is the person's own, and the fee grows by their value); without it the
// person is taken off every other offer that lists them. Whatever the offer's fee became is handed to
// 0x104099f0 and the offer goes back to 0x103e7cf0.
void remove_listed_person(Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t offer,
                          std::uint32_t as_listed_person, std::uint32_t extra) {
    Frame frame(c, 0x103e95a0, 0x70, {.first_gpr = 25});
    const auto people_cell = c.toc(toc::people), found_cell = c.toc(toc::found_contract);
    const auto bids = frame.at(0x38), offer_slot = frame.at(0x90);
    keep(c, 30, people_cell);
    keep(c, 26, manager);
    keep(c, 31, found_cell);
    keep(c, 27, person);
    keep(c, 25, as_listed_person);
    keep(c, 28, extra);
    keep(c, 29, 0);
    c.put32(offer_slot, offer);
    invoke(c, list_construct, 0x103e95d8, {bids});
    std::uint32_t found = 0;
    const auto status_row = [&] { return c.u32(manager + status_rows) + c.u32(person) * status_row_size; };
    const auto mark_found = [&] { found = 1; keep(c, 29, 1); };
    const auto took_off = [&] { c.put8(status_row() + status_row_listed, c.u8(status_row() + status_row_listed) - 1); };

    if (std::int8_t(as_listed_person) != 0) {
        keep(c, 25, 0);
        const auto contract = invoke(c, find_contract, 0x103e9604,
                                     {c.u32(c.toc(toc::contracts)), person_record(c, c.u32(c.u32(offer_slot) + offer_person))});
        c.put32(found_cell, contract);
        const auto contract_has_club = contract && contract_kind(c, contract) != no_club_contract_kind;
        if (contract_has_club) keep(c, 25, 1);
        const auto club = contract_has_club ? club_record(c, c.u32(contract + 4)) : 0;
        keep(c, 30, club);
        if (c.u32(c.u32(offer_slot) + offer_listed)) {
            rewind_list(c, c.u32(c.u32(offer_slot) + offer_listed));
            for (;;) {
                const auto entry = invoke(c, next_listed, 0x103e96c0, {c.u32(c.u32(offer_slot) + offer_listed)});
                if (!entry) break;
                if (c.i32(person) != c.i32(entry)) continue;
                mark_found();
                invoke(c, drop_listed, 0x103e9690, {c.u32(c.u32(offer_slot) + offer_listed), entry});
                took_off();
                break;
            }
            if (std::int8_t(found) != 0) {
                const auto fee = c.u32(c.u32(offer_slot) + offer_fee) + c.u32(person + 8);
                keep(c, 25, fee);
                invoke_native(c, 0x104099f0, 0x103e96f4, {manager, c.u32(offer_slot), person, fee, extra});
                if (club && std::int8_t(invoke(c, club_test, 0x103e9714, {club, 0, 1, 0, 0})) == 0) {
                    invoke_native(c, 0x103ee1b0, 0x103e972c, {manager, offer_slot, 1});
                    if (std::int8_t(invoke(c, 0x103fb390, 0x103e9738, {manager, c.u32(offer_slot)})) != 0) {
                        free_emptied_list(c, c.u32(offer_slot), 0x103e975c);
                        invoke(c, list_destroy, 0x103e9774, {bids, 0xffffffff});
                        return;
                    }
                }
                c.put32(c.u32(offer_slot) + offer_fee, fee);
                invoke_native(c, 0x103e7cf0, 0x103e978c, {manager, c.u32(offer_slot)});
            }
            free_emptied_list(c, c.u32(offer_slot), 0x103e97a8);
        }
    } else if (c.u8(status_row() + status_row_listed) != 0) {
        invoke(c, 0x103eb020, 0x103e97f4, {manager, person_record(c, c.u32(c.u32(offer_slot) + offer_person)), bids});
        c.put32(bids + list_cursor, c.u32(bids + 4));
        for (;;) {
            const auto bid = invoke(c, next_listed, 0x103e98e8, {bids});
            if (!bid) break;
            const auto other = c.u32(c.u32(manager) + c.u32(bid) * 4);
            keep(c, 30, other);
            if (other == c.u32(offer_slot)) continue;
            if (c.u32(other + offer_listed)) {
                rewind_list(c, c.u32(other + offer_listed));
                for (;;) {
                    const auto entry = invoke(c, next_listed, 0x103e987c, {c.u32(other + offer_listed)});
                    if (!entry) break;
                    if (c.i32(person) != c.i32(entry)) continue;
                    mark_found();
                    invoke(c, drop_listed, 0x103e9854, {c.u32(other + offer_listed), entry});
                    took_off();
                    break;
                }
                free_emptied_list(c, other, 0x103e989c);
            }
            if (std::int8_t(found) != 0) {
                const auto fee = c.u32(other + offer_fee) + c.u32(person + 8);
                keep(c, 25, fee);
                invoke_native(c, 0x104099f0, 0x103e98d0, {manager, other, person, fee, 0});
                c.put32(other + offer_fee, fee);
                invoke_native(c, 0x103e7cf0, 0x103e98e0, {manager, other});
            }
        }
    }
    invoke(c, list_destroy, 0x103e98fc, {bids, 0xffffffff});
}

// 0x103e6cb0: the answer to an offer for `person` from the contract side. When the contract check passes and the
// person is the offer's own, the shortlist entry (kind 0x1036d240) is carried over to the other club and the
// offer goes to 0x103e44b0; for a listed person the fee grows by their value and 0x103e95a0 takes them off;
// when the check fails the contract manager is told (0x1007f200).
void answer_person_offer(Cpu& c, std::uint32_t manager, std::uint32_t subject, std::uint32_t person, std::uint32_t offer) {
    constexpr std::uint32_t offer_slot_at = 0xd4, offer_seen_stage = 0x11, shortlist = 0x988;
    Frame frame(c, 0x103e6cb0, 0xb0, {.first_gpr = 28});
    const auto contracts = c.toc(toc::contracts);
    const auto slot = frame.at(offer_slot_at), check = frame.at(0x38);
    keep(c, 31, person);
    keep(c, 30, subject);
    keep(c, 29, manager);
    keep(c, 28, contracts);
    c.put32(slot, offer);
    invoke(c, 0x103721f0, 0x103e6ce8, {check});
    const auto settings = c.toc(-0x4ba0);
    c.set_f(1, c.f64(settings + 0x80));
    const auto failed = invoke(c, 0x10080d30, 0x103e6d10,
                               {c.u32(contracts), person, subject, check, c.u32(c.u32(slot) + 0x28), settings, c.r(9), 2});
    const auto unlike_eleven = [](std::int32_t value) { return std::uint32_t(value != 11); };
    if (std::int8_t(failed) != 0) {
        invoke(c, 0x1007f200, 0x103e6e04,
               {c.u32(contracts), check, c.u32(c.u32(slot) + 0x28), c.u32(c.u32(slot) + 0x2c), 0xb, 0, 1, 0});
        return;
    }
    if (c.i32(person) == c.i32(c.u32(slot) + offer_person)) {
        const auto entry = invoke(c, 0x1036d240, 0x103e6d5c,
                                  {c.u32(manager + shortlist), subject, person, unlike_eleven(c.i8(c.u32(slot) + offer_seen_stage)), 0});
        if (entry) {
            const auto seen = std::int32_t(c.i8(entry));
            invoke(c, 0x10365590, 0x103e6d94,
                   {c.u32(manager + shortlist), person, subject, 1, unlike_eleven(seen), std::uint32_t(seen), c.r(9), c.r(10)});
        }
        invoke_native(c, 0x103e44b0, 0x103e6da8, {manager, slot, 0, 0x16});
    } else {
        const auto held = c.u32(slot);
        c.put32(held + offer_fee, c.u32(held + offer_fee) + c.u32(person + 8));
        invoke_native(c, 0x103e95a0, 0x103e6dd8, {c.u32(c.toc(toc::manager)), person, c.u32(slot), 1, 0x16});
    }
}
}
