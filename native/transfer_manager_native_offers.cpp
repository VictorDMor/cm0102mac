#include "transfer_manager_native_offers.hpp"
#include "transfer_manager_native_offers_common.hpp"
#include <stdexcept>

namespace cm::aot::transfer_manager_native {
using namespace offers;

std::int32_t random_below(Cpu& c, std::int32_t n) {
    Frame frame(c, 0x103e3860, 0x80, {.first_gpr = 20});
    const Samples samples(c);
    const auto pool = c.toc(cell::floats);
    if (n == 0) return 0;
    guest::Fpu fpu(c);
    std::int32_t range_after_first = 0, first_part = 0;
    // The registers the original holds when a draw refills: the sample cells and the range in r25-r30, and for
    // the second draw also the first level's range and partial result.
    const auto hold = [&](bool second) {
        keep(c, 25, samples.base_cell);
        keep(c, 26, samples.first);
        keep(c, 27, samples.last_cell);
        keep(c, 28, samples.cursor_cell);
        keep(c, 29, pool);
        keep_signed(c, 30, n);
        if (second) {
            keep_signed(c, 22, range_after_first);
            keep_signed(c, 31, first_part);
        }
    };
    const auto fits_one_draw = [](std::int32_t range) { return range >= -0xffff && range <= 0xffff; };
    const auto first = samples.next(0x103e38b4, [&] { hold(false); });
    if (fits_one_draw(n)) return remainder_of(first, n);
    // A range past 0xffff: a scaled draw plus a draw over a tenth of the scale.
    const double scale = fpu.fdiv(to_double(fpu, c, pool, n), c.f64(pool + 0x10));
    const double part = fpu.fmul(scale, to_double(fpu, c, pool, std::int32_t(first) % 0xffff));
    first_part = std::int32_t(std::uint32_t(fpu.fctiwz(0, part)));
    range_after_first = std::int32_t(std::uint32_t(n) * 10u) / 0xffff;
    std::int32_t extra = 0;
    if (range_after_first != 0) {
        // The original has three more inline levels (0x103e39a8..0x103e3c38) and a call of itself (0x103e3c3c). None
        // can run: the second range is the first times ten, wrapping in a word, over 0xffff, which lies in
        // -32768..32768 for every word, so the second level always draws once and goes no deeper.
        // (unreachable: 0x103e39a8 follows only a range of at most 32768 in size)
        if (!fits_one_draw(range_after_first)) throw std::logic_error("random below: a second range past 0xffff");
        extra = remainder_of(samples.next(0x103e39a0, [&] { hold(true); }), range_after_first);
    }
    auto total = std::int32_t(std::uint32_t(first_part) + std::uint32_t(std::int32_t(std::uint32_t(extra) + 5u) / 10));
    const auto limit = std::int32_t(std::uint32_t(n) - 1u);
    if (total > limit) total = limit;
    return total;
}

namespace {
void reset_list_cursor(Cpu& c, std::uint32_t list) { c.put32(list + 0xc, c.u32(list + 4)); }
}

void close_offer_stage_2(Cpu& c, std::uint32_t manager, std::uint32_t slot, std::uint32_t flag, std::uint32_t reason) {
    Frame frame(c, 0x103e4390, 0x50, {.first_gpr = 27});
    const auto offer_record = c.u32(slot);
    keep(c, 27, reason);
    keep(c, 28, offer_record);
    keep(c, 29, manager);
    keep(c, 30, c.toc(toc::people));
    keep(c, 31, c.toc(toc::clubs));
    if (std::int8_t(flag)) c.put8(offer_record + offer::flagged, 1);
    c.put8(offer_record + offer::stage, 2);
    invoke(c, 0x103ec4f0, 0x103e43dc, {manager, offer_record});
    invoke_native(c, 0x103ffe80, 0x103e43ec, {manager, offer_record, reason});
    invoke(c, 0x103eb4a0, 0x103e4400, {manager, offer_record, 10, 0});
    invoke(c, 0x103f6190, 0x103e440c, {manager, offer_record});
    invoke(c, 0x103ec440, 0x103e4418, {manager, offer_record});
    tell_shortlists(c, manager, offer_record, 0x103e4460, 0x103e4498, false);
}

// The part of 0x103e4130 and 0x103e44b0 after the stage change: when the offer has other bidders and the
// contract's club is not exempt (0x1014c2e0), each bidder's shortlist is asked about the offer's club.
// `bidder` is the register the original reuses for the list's elements.
static void tell_bidders(Cpu& c, std::uint32_t manager, std::uint32_t offer_record, std::uint32_t contract_club,
                         std::uint32_t offer_club, std::uint32_t bidder_register, std::uint32_t hit_back,
                         std::uint32_t update_back, std::uint32_t exempt_back, std::uint32_t next_back) {
    const auto people_cell = c.toc(toc::people);
    if (!c.u32(offer_record + offer::list) || !contract_club) return;
    
    if (std::int8_t(invoke(c, 0x1014c2e0, exempt_back, {contract_club, 0, 1, 0, 0}))) return;
    reset_list_cursor(c, c.u32(offer_record + offer::list));
    for (;;) {
        // r8-r10 reach 0x10362d70 as the iterator's call left them.
        const auto bidder = invoke(c, next_in_list, next_back, {c.u32(offer_record + offer::list)});
        keep(c, bidder_register, bidder);
        if (!bidder) break;
        const auto person = c.u32(people_cell) + c.u32(bidder) * person_size;
        if (std::int8_t(invoke(c, 0x10362d70, hit_back, {c.u32(manager + shortlists), person, offer_club})))
            invoke(c, 0x10365590, update_back, {c.u32(manager + shortlists), person, offer_club, 1, 0});
    }
}

void close_offer_stage_2_with_bidders(Cpu& c, std::uint32_t manager, std::uint32_t slot, std::uint32_t flag, std::uint32_t reason) {
    Frame frame(c, 0x103e4130, 0x60, {.first_gpr = 23});
    const auto people_cell = c.toc(toc::people), clubs_cell = c.toc(toc::clubs), found_cell = c.toc(toc::found_contract);
    keep(c, 23, flag);
    keep(c, 24, manager);
    keep(c, 25, slot);
    keep(c, 26, reason);
    keep(c, 27, found_cell);
    keep(c, 30, people_cell);
    keep(c, 31, clubs_cell);
    const auto offer_record = c.u32(slot);
    keep(c, 28, offer_record);
    const auto contract = invoke(c, 0x10082f20, 0x103e417c,
                                 {c.u32(c.toc(toc::contracts)), person_of(c, offer_record)});
    c.put32(found_cell, contract);
    const auto contract_club = contract ? c.u32(clubs_cell) + c.u32(contract + 4) * club_size : 0;
    keep(c, 29, contract_club);
    const auto offer_club = c.u32(clubs_cell) + c.u32(offer_record + offer::club) * club_size;
    keep(c, 27, offer_club);
    if (std::int8_t(flag)) c.put8(c.u32(slot) + offer::flagged, 1);
    invoke_native(c, 0x103e5b70, 0x103e41d0, {manager, offer_record});
    // The hidden second argument of 0x103f3540 is the offer behind the slot.
    if (c.u8(c.u32(slot) + offer::stage) == 0x13) invoke(c, 0x103f3540, 0x103e41e8, {manager, c.u32(slot)});
    c.put8(offer_record + offer::stage, 2);
    invoke(c, 0x103ec4f0, 0x103e41fc, {manager, offer_record});
    invoke(c, 0x103ec5d0, 0x103e4208, {manager, offer_record});
    invoke_native(c, 0x103ffe80, 0x103e4218, {manager, offer_record, reason});
    invoke(c, 0x103eb4a0, 0x103e422c, {manager, offer_record, 0x19, 0});
    invoke(c, 0x103f6190, 0x103e4238, {manager, offer_record});
    tell_shortlists(c, manager, offer_record, 0x103e4280, 0x103e42d0, true);
    invoke(c, 0x103ec440, 0x103e42dc, {manager, offer_record});
    tell_bidders(c, manager, offer_record, contract_club, offer_club, 25, 0x103e433c, 0x103e4368, 0x103e4308, 0x103e4370);
}

void close_offer_stage_3(Cpu& c, std::uint32_t manager, std::uint32_t slot, std::uint32_t flag, std::uint32_t reason) {
    Frame frame(c, 0x103e44b0, 0x60, {.first_gpr = 22});
    const auto people_cell = c.toc(toc::people), clubs_cell = c.toc(toc::clubs), found_cell = c.toc(toc::found_contract);
    const auto contracts_cell = c.toc(toc::contracts);
    keep(c, 22, flag);
    keep(c, 23, manager);
    keep(c, 24, slot);
    keep(c, 25, reason);
    keep(c, 26, found_cell);
    keep(c, 29, people_cell);
    keep(c, 30, contracts_cell);
    keep(c, 31, clubs_cell);
    const auto offer_record = c.u32(slot);
    keep(c, 27, offer_record);
    const auto contract = invoke(c, 0x10082f20, 0x103e44fc, {c.u32(contracts_cell), person_of(c, offer_record)});
    c.put32(found_cell, contract);
    const auto contract_club = contract ? c.u32(clubs_cell) + c.u32(contract + 4) * club_size : 0;
    keep(c, 28, contract_club);
    const auto offer_club = c.u32(clubs_cell) + c.u32(offer_record + offer::club) * club_size;
    keep(c, 26, offer_club);
    if (std::int8_t(flag)) c.put8(c.u32(slot) + offer::flagged, 1);
    if (c.u8(c.u32(slot) + offer::stage) == 0x13) invoke(c, 0x103f3540, 0x103e455c, {manager, c.u32(slot)});
    if (c.i8(offer_record + offer::stage) >= 0xb) invoke(c, 0x10082d80, 0x103e4578, {c.u32(contracts_cell), c.u32(offer_record + offer::person)});
    c.put8(offer_record + offer::stage, 3);
    invoke(c, 0x103ec5d0, 0x103e458c, {manager, offer_record});
    invoke(c, 0x103ec4f0, 0x103e4598, {manager, offer_record});
    c.put8(offer_record + 0x10, 4);
    invoke(c, 0x103eb4a0, 0x103e45b4, {manager, offer_record, 10, 0});
    invoke(c, 0x103f6190, 0x103e45c0, {manager, offer_record});
    if (std::int16_t(reason) == 2) invoke_native(c, 0x103e5b70, 0x103e45d8, {manager, offer_record});
    tell_shortlists(c, manager, offer_record, 0x103e4620, 0x103e4670, true);
    // The news go through the transfer manager object, not the argument.
    invoke_native(c, 0x103ffe80, 0x103e4684, {c.u32(c.toc(toc::manager)), offer_record, reason});
    invoke(c, 0x103ec440, 0x103e4690, {manager, offer_record});
    tell_bidders(c, manager, offer_record, contract_club, offer_club, 24, 0x103e46f0, 0x103e471c, 0x103e46bc, 0x103e4724);
}

namespace {
// The person's row of the status table (ten bytes each): bit 4 cleared.
void clear_blocked_mark(Cpu& c, std::uint32_t manager, std::uint32_t person_index) {
    const auto row = c.u32(manager + status_rows) + person_index * status_row_size;
    c.put8(row, c.u8(row) & 0xef);
}
// The calls 0x103e9910 and 0x103e9ac0 make for each offer they withdraw (stage 0x10 is already set).
}

void withdraw_offer(Cpu& c, std::uint32_t manager, std::uint32_t offer_record, std::uint32_t news_a, std::uint32_t news_b, std::uint32_t flag) {
    Frame frame(c, 0x103e9910, 0x60, {.first_gpr = 25});
    keep(c, 25, flag);
    keep(c, 26, manager);
    keep(c, 27, offer_record);
    keep(c, 28, news_a);
    keep(c, 29, news_b);
    keep(c, 30, c.toc(toc::people));
    keep(c, 31, c.toc(toc::clubs));
    if (c.u8(offer_record + offer::stage) == 0x13) {
        clear_blocked_mark(c, manager, c.u32(offer_record + offer::person));
        if (c.u32(offer_record + offer::list)) {
            reset_list_cursor(c, c.u32(offer_record + offer::list));
            for (;;) {
                const auto bidder = invoke(c, next_in_list, 0x103e9998, {c.u32(offer_record + offer::list)});
                if (!bidder) break;
                clear_blocked_mark(c, manager, c.u32(bidder));
            }
        }
        if (std::int8_t(flag)) invoke(c, 0x103f3540, 0x103e99b4, {manager, offer_record});
    }
    c.put8(offer_record + offer::stage, 0x10);
    invoke(c, 0x103ec5d0, 0x103e99c8, {manager, offer_record});
    invoke(c, 0x103ec4f0, 0x103e99d4, {manager, offer_record});
    invoke_native(c, 0x1040a050, 0x103e99e8, {manager, offer_record, news_a, news_b});
    invoke(c, 0x103eb4a0, 0x103e99fc, {manager, offer_record, 0x14, 0});
    invoke(c, 0x103f6190, 0x103e9a08, {manager, offer_record});
    tell_shortlists(c, manager, offer_record, 0x103e9a50, 0x103e9aa0, true);
    invoke(c, 0x103ec440, 0x103e9aac, {manager, offer_record});
}

void withdraw_offers_of_club(Cpu& c, std::uint32_t manager, std::uint32_t club) {
    Frame frame(c, 0x103e9ac0, 0x130, {.first_gpr = 26});
    const auto list = frame.at(0x38), others = frame.at(0x48);
    const auto person_count = c.toc(cell::person_count);
    keep(c, 28, manager);
    keep(c, 29, club);
    keep(c, 31, person_count);
    invoke(c, 0x10738c00, 0x103e9ae4, {list});
    if (std::int8_t(invoke(c, 0x100c7d00, 0x103e9aec, {club}))) {
        invoke(c, 0x10738c20, 0x103e9b00, {list, 0xffffffff});
        return;
    }
    // The offers in the list: stage 0x10 and the bookkeeping calls (an offer at 0x13 is left; for the second
    // kind of list so is one marked protected). r26 holds the offer last taken.
    const auto withdraw_listed = [&](bool skip_protected) {
        reset_list_cursor(c, list);
        for (;;) {
            const auto element = invoke(c, next_in_list, skip_protected ? 0x103e9c98 : 0x103e9ba0, {list});
            if (!element) break;
            const auto offer_record = c.u32(c.u32(manager) + c.u32(element) * 4);
            keep(c, 26, offer_record);
            if (c.u8(offer_record + offer::stage) == 0x13) continue;
            if (skip_protected && c.i8(offer_record + offer::protected_from_withdrawal)) continue;
            c.put8(offer_record + offer::stage, 0x10);
            invoke(c, 0x103ec5d0, skip_protected ? 0x103e9c50 : 0x103e9b58, {manager, offer_record});
            invoke(c, 0x103ec4f0, skip_protected ? 0x103e9c5c : 0x103e9b64, {manager, offer_record});
            invoke(c, 0x103ec440, skip_protected ? 0x103e9c68 : 0x103e9b70, {manager, offer_record});
            invoke(c, 0x103eb4a0, skip_protected ? 0x103e9c7c : 0x103e9b84, {manager, offer_record, 10, 0});
            invoke_native(c, 0x1040a050, skip_protected ? 0x103e9c90 : 0x103e9b98, {manager, offer_record, club, 0xb});
        }
    };
    if (invoke(c, 0x103eb360, 0x103e9b14, {manager, club, list})) withdraw_listed(false);
    invoke(c, 0x10738ca0, 0x103e9bb0, {list});
    invoke(c, 0x100d06f0, 0x103e9bc0, {club, others, 0});
    // Up to fifty people linked to the club (a list in its record, 0x100d06f0), each with the offers that involve
    // them. r27 points at the current one and r30 counts them.
    for (std::uint32_t taken = 0; std::int8_t(taken) < 0x32; ++taken) {
        keep(c, 27, others + taken * 4);
        keep(c, 30, taken);
        const auto other = c.u32(others + taken * 4);  // a person record or 0
        if (other && std::int32_t(c.u32(other)) < std::int32_t(c.u32(person_count)) &&
            invoke(c, 0x103eaea0, 0x103e9bfc, {manager, other, list, 0}))
            withdraw_listed(true);
    }
    keep(c, 27, others + 0x32 * 4);
    keep(c, 30, 0x32);
    invoke(c, 0x10738c20, 0x103e9cc0, {list, 0xffffffff});
}
}
