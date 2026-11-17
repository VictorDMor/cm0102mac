#include "transfer_manager_native_stages.hpp"
#include "transfer_manager_native_stages_detail.hpp"

namespace cm::aot::transfer_manager_native {
using namespace stages;

namespace {
constexpr std::uint32_t offer_flags = 0x38, offer_kind_byte = 0x10, offer_seen = 0x37, offer_deal = 0x28;
constexpr std::uint32_t news_ask = 0x102cdbf0, ask_terms = 0x104058e0, club_busy = 0x1014c2e0;
}

// 0x103e7cf0 (see the header). The offer is read again from its slot in the caller's frame wherever the original
// reads it, because 0x103e44b0 receives the slot's address.
void advance_offer(Cpu& c, std::uint32_t manager, std::uint32_t offer_arg) {
    constexpr std::uint32_t slot_at = 0x7c, answer_at = 0x38;
    constexpr std::uint32_t stage_waiting = 10, stage_asked = 11, stage_refused = 15, refused_times = 3;
    Frame frame(c, 0x103e7cf0, 0x60, {.first_gpr = 24});
    const auto people_cell = c.toc(toc::people), contracts_cell = c.toc(toc::contracts), found_cell = c.toc(toc::found_contract),
               clubs_cell = c.toc(toc::clubs), news_cell = c.toc(toc::news);
    const auto slot = frame.at(slot_at), answer = frame.at(answer_at);
    keep(c, 25, manager);
    keep(c, 31, clubs_cell);
    keep(c, 28, people_cell);
    keep(c, 24, 0);
    keep(c, 29, contracts_cell);
    keep(c, 27, news_cell);
    keep(c, 30, found_cell);
    c.put32(slot, offer_arg);
    const auto offer = [&] { return c.u32(slot); };
    const auto person_of_offer = [&] { return person_record(c, c.u32(offer() + offer_person)); };

    const auto club = club_record(c, c.u32(offer() + offer_club));
    keep(c, 26, club);
    auto contract = invoke(c, find_contract, 0x103e7d4c, {c.u32(contracts_cell), person_of_offer()});
    c.put32(found_cell, contract);
    const auto has_club = contract && contract_kind(c, contract) != no_club_contract_kind;
    if (has_club) keep(c, 24, 1);
    auto other_club = has_club ? club_record(c, c.u32(contract + 4)) : 0;
    keep(c, 31, other_club);
    contract = invoke(c, find_contract, 0x103e7db8, {c.u32(contracts_cell), person_of_offer()});
    c.put32(found_cell, contract);
    if (contract && contract_kind(c, contract) == no_club_contract_kind) {
        other_club = 0;
        keep(c, 31, 0);
    }

    if (c.u8(offer() + offer_stage) != stage_waiting) {
        if (c.u8(offer() + offer_kind_byte) != 2) return;
        if (c.u32(offer() + offer_listed)) {
            rewind_list(c, c.u32(offer() + offer_listed));
            while (const auto listed = invoke(c, next_listed, 0x103e7e40, {c.u32(offer() + offer_listed)}))
                if (c.u8(listed + 4) != 2) return;
        }
        if (c.u32(offer() + offer_bidders)) {
            rewind_list(c, c.u32(offer() + offer_bidders));
            for (;;) {
                const auto bidder = invoke(c, next_listed, 0x103e7ef0, {c.u32(offer() + offer_bidders)});
                keep(c, 28, bidder);
                if (!bidder) break;
                const auto other = [&] { return c.u32(c.u32(manager) + c.u32(bidder) * 4); };
                if (!other() || std::int32_t(c.u32(bidder)) == c.i32(offer() + offer_deal)) continue;
                c.put8(other() + offer_stage, stage_refused);
                c.put8(other() + offer_refusals, refused_times);
                invoke(c, 0x103eb4a0, 0x103e7ecc, {manager, other(), 0x14, 0});
                invoke(c, 0x103ec4f0, 0x103e7ee4, {manager, other()});
            }
        }
        if ((c.i8(offer() + offer_flags) >> 4) == 0 && std::int8_t(invoke(c, 0x103ecda0, 0x103e7f18, {manager, offer()})) != 0 &&
            c.u8(offer() + offer_refusals) == 0) {
            invoke_native(c, 0x103ed090, 0x103e7f38, {manager, offer()});
            return;
        }
    }
    c.put8(offer() + offer_stage, stage_asked);
    if (std::int8_t(invoke(c, news_ask, 0x103e7f54, {c.u32(news_cell), club})) != 0 ||
        (other_club && std::int8_t(invoke(c, news_ask, 0x103e7f70, {c.u32(news_cell), other_club})) != 0))
        invoke_native(c, ask_terms, 0x103e7f84, {manager, offer()});
    if (std::int8_t(invoke(c, club_busy, 0x103e7f9c, {club, 0, 1, 0, 0})) == 0) {
        const auto answered = std::int8_t(invoke_native(c, 0x103f9d60, 0x103e7fbc, {manager, offer(), club, answer, 1}));
        if (answered == 1) {
            const auto fee_step = std::uint32_t(std::int32_t(c.i16(answer)));
            if (c.u8(offer() + offer_refusals) < 3) invoke_native(c, 0x103e8500, 0x103e7fe8, {manager, offer(), club, fee_step});
            else invoke_native(c, 0x103e44b0, 0x103e8000, {manager, slot, 0, fee_step});
        } else {
            invoke_native(c, 0x103e80e0, 0x103e8014, {manager, offer(), club});
        }
    }
    if (c.i8(offer() + offer_seen) != 0) {
        invoke_native(c, 0x103e80e0, 0x103e8030, {manager, offer(), other_club});
        return;
    }
    if (other_club && std::int8_t(invoke(c, club_busy, 0x103e8054, {other_club, 0, 1, 0, 0})) != 0) return;
    if (!other_club) {
        invoke_native(c, 0x103e80e0, 0x103e80c4, {manager, offer(), other_club});
        return;
    }
    if (std::int8_t(invoke_native(c, 0x103f9d60, 0x103e807c, {manager, offer(), other_club, answer, 1})) == 1)
        invoke_native(c, 0x103e8500, 0x103e809c, {manager, offer(), other_club, 0});
    else
        invoke_native(c, 0x103e80e0, 0x103e80b0, {manager, offer(), other_club});
}

// 0x103ed170 (see the header).
std::uint32_t review_offer_people(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t quiet) {
    constexpr std::uint32_t shortlist = 0x988, club_interest = 0x2c, contract_state = 0x64, state_agreed = 4;
    constexpr std::uint32_t asked_stage = 0x13, agreed_stage = 0x10;
    Frame frame(c, 0x103ed170, 0x60, {.first_gpr = 22});
    const auto people_cell = c.toc(toc::people), clubs_cell = c.toc(toc::clubs), contracts_cell = c.toc(toc::contracts),
               found_cell = c.toc(toc::found_contract);
    keep(c, 24, offer);
    keep(c, 28, people_cell);
    keep(c, 23, manager);
    keep(c, 31, clubs_cell);
    keep(c, 29, contracts_cell);
    keep(c, 25, quiet);
    keep(c, 30, found_cell);
    const bool loud = std::int8_t(quiet) == 0;
    auto person = person_record(c, c.u32(offer + offer_person));
    keep(c, 26, person);
    const auto club = club_record(c, c.u32(offer + offer_club));
    keep(c, 22, club);
    auto contract = invoke(c, find_contract, 0x103ed1c8, {c.u32(contracts_cell), person});
    c.put32(found_cell, contract);
    const auto buyer = contract ? club_record(c, c.u32(contract + 4)) : 0;
    keep(c, 27, buyer);
    const auto asks_for = [&](std::uint32_t asked_person, std::uint32_t asked_club, std::uint32_t back) {
        return std::int8_t(invoke(c, 0x103ecef0, back, {manager, asked_person, asked_club, 0})) != 0;
    };
    // The contract the club offers the person, unless the person refuses it (0x103ed520).
    const auto acceptable = [&](std::uint32_t asked_person, std::uint32_t asked_club, std::uint32_t back_row, std::uint32_t back_refused,
                                std::uint32_t& row) {
        row = invoke(c, 0x1007f720, back_row, {c.u32(contracts_cell), c.u32(asked_person), c.u32(asked_club)});
        return row && std::int8_t(invoke_native(c, 0x103ed520, back_refused,
                                                {manager, asked_person, c.u32(asked_club + club_interest), row, 0, 0})) == 0;
    };

    if (asks_for(person, club, 0x103ed200)) {
        std::uint32_t row;
        if (acceptable(person, club, 0x103ed218, 0x103ed238, row)) {
            invoke_native(c, 0x1040a050, 0x103ed254, {manager, offer, club, asked_stage});
            c.put8(offer + offer_stage, agreed_stage);
            contract = invoke(c, find_contract, 0x103ed274, {c.u32(contracts_cell), person_record(c, c.u32(offer + offer_person))});
            c.put32(found_cell, contract);
            auto club_now = contract ? club_record(c, c.u32(contract + 4)) : 0;
            keep(c, 26, club_now);
            if (const auto held = invoke(c, 0x1007f720, 0x103ed2a8, {c.u32(contracts_cell), c.u32(offer + offer_person), c.u32(offer + offer_club)}))
                c.put8(held + contract_state, state_agreed);
            if (c.u32(offer + offer_listed)) {
                rewind_list(c, c.u32(offer + offer_listed));
                keep(c, 25, state_agreed);
                while (const auto entry = invoke(c, next_listed, 0x103ed2fc, {c.u32(offer + offer_listed)})) {
                    // club_now is 0 for a person without a contract: the original reads guest address 0 there.
                    if (const auto held = invoke(c, 0x1007f720, 0x103ed2e8, {c.u32(contracts_cell), c.u32(entry), c.u32(club_now)}))
                        c.put8(held + contract_state, state_agreed);
                }
            }
            person = person_record(c, c.u32(offer + offer_person));
            keep(c, 25, person);
            contract = invoke(c, find_contract, 0x103ed320, {c.u32(contracts_cell), person});
            c.put32(found_cell, contract);
            club_now = contract ? club_record(c, c.u32(contract + 4)) : 0;
            keep(c, 26, club_now);
            invoke(c, 0x1036f100, 0x103ed360, {c.u32(manager + shortlist), club_record(c, c.u32(offer + offer_club)), person});
            if (c.u32(offer + offer_listed)) {
                rewind_list(c, c.u32(offer + offer_listed));
                while (const auto entry = invoke(c, next_listed, 0x103ed39c, {c.u32(offer + offer_listed)}))
                    invoke(c, 0x1036f100, 0x103ed394, {c.u32(manager + shortlist), club_now, person_record(c, c.u32(entry))});
            }
            invoke(c, 0x103eb4a0, 0x103ed3b8, {manager, offer, 10, 0});
            return 0;
        }
        if (loud) invoke_native(c, 0x1040f270, 0x103ed3dc, {manager, person, club, offer});
    }

    if (c.u32(offer + offer_listed)) {
        rewind_list(c, c.u32(offer + offer_listed));
        for (;;) {
            const auto entry = invoke(c, next_listed, 0x103ed4cc, {c.u32(offer + offer_listed)});
            keep(c, 30, entry);
            if (!entry) break;
            const auto listed = person_record(c, c.u32(entry));
            keep(c, 26, listed);
            if (!asks_for(listed, buyer, 0x103ed418)) continue;
            std::uint32_t row;
            if (acceptable(listed, buyer, 0x103ed430, 0x103ed450, row)) {
                const auto fee = c.u32(offer + offer_fee) + c.u32(listed + 8);
                keep(c, 22, fee);
                invoke_native(c, 0x1040f780, 0x103ed478, {manager, offer, listed, fee});
                invoke(c, drop_listed, 0x103ed484, {c.u32(offer + offer_listed), entry});
                const auto status = c.u32(manager + status_rows) + c.u32(listed) * status_row_size + status_row_listed;
                c.put8(status, c.u8(status) - 1);
                c.put32(offer + offer_fee, fee);
            } else if (loud) {
                invoke_native(c, 0x1040f270, 0x103ed4c4, {manager, listed, buyer, offer});
            }
        }
        if (c.i16(c.u32(offer + offer_listed)) == 0) {
            invoke(c, list_destroy, 0x103ed4ec, {c.u32(offer + offer_listed), 1});
            c.put32(offer + offer_listed, 0);
        }
    }
    if (loud) invoke_native(c, 0x103e7cf0, 0x103ed508, {manager, offer});
    return 1;
}
}
