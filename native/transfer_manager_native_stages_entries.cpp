#include "guest_fpu.hpp"
#include "transfer_manager_native_stages.hpp"
#include "transfer_manager_native_stages_detail.hpp"
#include <bit>

namespace cm::aot::transfer_manager_native {
using namespace stages;

namespace {
constexpr std::uint32_t offer_seen_by = 0x11, offer_mode = 0x24, offer_deal = 0x28, offer_second_deal = 0x2c, shortlist = 0x988;
constexpr std::uint32_t person_state = 0x6d;
constexpr std::uint32_t stage_refused_early = 0x10, stage_settled = 0x14;
}

// 0x103e70f0 (see the header). r13-r31 are all live across its calls; the three registers it leaves unset on a path
// without a club (r18-r20) hold what the caller left.
void accept_offer(Cpu& c, std::uint32_t manager, std::uint32_t person, std::int32_t index, std::uint32_t terms,
                  std::uint32_t kind, std::uint32_t extra) {
    Frame frame(c, 0x103e70f0, 0x140, {.first_gpr = 13});
    std::uint32_t first_club = c.r(20), second_club = c.r(19), mode = c.r(18);
    const auto squad = c.toc(-0x7a98), date = c.toc(-0x7b80), people_cell = c.toc(toc::people), contracts_cell = c.toc(toc::contracts),
               found_cell = c.toc(toc::found_contract), clubs_cell = c.toc(toc::clubs), flag_cell = c.toc(-0x7bac),
               log_cell = c.toc(-0x7ba8), strings = c.toc(-0x4b9c);
    keep(c, 29, manager);
    keep(c, 30, person);
    keep(c, 31, terms);
    keep(c, 16, kind);
    keep(c, 17, extra);
    keep(c, 22, squad);
    keep(c, 13, date);
    keep(c, 23, people_cell);
    keep(c, 24, contracts_cell);
    keep(c, 25, found_cell);
    keep(c, 26, clubs_cell);
    keep(c, 14, flag_cell);
    keep(c, 27, log_cell);
    keep(c, 28, strings);
    const auto report = [&](std::uint32_t first, std::uint32_t second, std::uint32_t line, const ReportBack& back) {
        report_bad_offer(c, strings, log_cell, flag_cell, frame.at(first), frame.at(second), line, back);
    };
    if (index < 0) return report(0x9c, 0x54, 0xe0c, {0x103e7158, 0x103e715c, 0x103e7178, 0x103e7188});
    if (index >= c.i32(manager + 4)) return report(0x7c, 0x4c, 0xe11, {0x103e71b8, 0x103e71bc, 0x103e71d8, 0x103e71e8});
    const auto offer = c.u32(c.u32(manager) + std::uint32_t(index) * 4);
    keep(c, 21, offer);
    if (!offer) return;
    keep(c, 15, 0);
    c.put8(offer + offer_refusals, 0);
    const auto person_word = c.i32(person);
    const auto offer_person_word = c.i32(offer + offer_person);
    if (person_word == offer_person_word) {
        // The person's own offer: the first club is the offer's, the second the person's current one.
        mode = 2;
        keep(c, 18, 2);
        c.put8(offer + offer_kind, 2);
        c.put8(offer + offer_seen_by, kind);
        first_club = club_record(c, c.u32(offer + offer_club));
        keep(c, 20, first_club);
        const auto contract = invoke(c, find_contract, 0x103e7244, {c.u32(contracts_cell), person});
        c.put32(found_cell, contract);
        const bool has_club = contract && contract_kind(c, contract) != no_club_contract_kind;
        if (has_club) keep(c, 15, 1);
        second_club = has_club ? club_record(c, c.u32(contract + 4)) : 0;
        keep(c, 19, second_club);
    } else if (!c.u32(offer + offer_listed)) {
        report(0x5c, 0x44, 0xe4d, {0x103e7354, 0x103e7358, 0x103e7374, 0x103e7384});
    } else {
        // A listed person: the first club is the person's current one, the second the offer's. Without a match in
        // the list the registers stay as they were.
        rewind_list(c, c.u32(offer + offer_listed));
        while (const auto entry = invoke(c, next_listed, 0x103e7330, {c.u32(offer + offer_listed)})) {
            if (c.i32(entry) != c.i32(person)) continue;
            mode = 2;
            keep(c, 18, 2);
            c.put8(entry + 4, 2);
            c.put8(entry + 6, kind);
            const auto contract = invoke(c, find_contract, 0x103e72e8, {c.u32(contracts_cell), person_record(c, c.u32(offer + offer_person))});
            c.put32(found_cell, contract);
            first_club = contract ? club_record(c, c.u32(contract + 4)) : 0;
            keep(c, 20, first_club);
            second_club = club_record(c, c.u32(offer + offer_club));
            keep(c, 19, second_club);
            break;
        }
    }

    if (c.u8(offer + offer_mode) != 1) {
        c.put32(frame.at(0x38), 0);
        invoke_native(c, 0x10403e20, 0x103e7684, {manager, person, first_club, second_club, mode, offer, 0, extra});
        invoke_native(c, 0x103e7cf0, 0x103e7690, {manager, offer});
        return;
    }
    const auto verdict = frame.at(0x40);
    c.put16(verdict, 0);
    const auto rank = std::int32_t(std::int8_t(invoke(c, 0x100c9070, 0x103e73b0, {first_club, 0, 0})));
    if (rank + 1 > 0x32) {
        c.put16(verdict, 4);
    } else if (const auto judge = invoke(c, 0x103f8380, 0x103e73d8, {manager, first_club})) {
        // A virtual call through the judge's table (slot +0x28) through the loader's glue at 0x10781320.
        c.set(12, c.u32(c.u32(judge) + 0x28));
        invoke(c, 0x10781320, 0x103e7414,
               {judge, club_record(c, c.u32(offer + offer_club)), person_record(c, c.u32(offer + offer_person)), 0, verdict,
                c.u32(clubs_cell)});
        c.set(2, c.u32(frame.at(0x14)));
    }
    const auto refusal = c.i16(verdict);
    if (refusal != 0) {
        invoke_native(c, 0x1040e2d0, 0x103e7430, {manager, offer, std::uint32_t(std::int32_t(refusal))});
        c.put8(offer + offer_stage, stage_refused_early);
        invoke(c, 0x103ec5d0, 0x103e7444, {manager, offer});
        invoke(c, 0x103ec4f0, 0x103e7450, {manager, offer});
        invoke(c, 0x103eb4a0, 0x103e7464, {manager, offer, 5, 0});
        invoke(c, 0x103f6190, 0x103e7470, {manager, offer});
        return;
    }
    invoke_native(c, 0x1040ddc0, 0x103e7484, {manager, offer, terms});
    invoke(c, 0x103faba0, 0x103e7490, {manager, offer});
    invoke(c, 0x10083e50, 0x103e749c, {c.u32(contracts_cell), terms});
    if (c.u8(person + person_state) == 2) c.put8(person + person_state, 0x10);
    invoke(c, 0x103864b0, 0x103e74c8, {c.u32(c.toc(-0x7b48)), person, second_club, first_club});
    if (std::int8_t(invoke(c, 0x10362d70, 0x103e74d8, {c.u32(manager + shortlist), person, first_club})) != 0)
        invoke(c, 0x10365590, 0x103e74f8, {c.u32(manager + shortlist), person, first_club, 5, 0});
    invoke(c, 0x100df500, 0x103e7514, {c.u32(c.toc(-0x7aa4)), person_record(c, c.u32(offer + offer_person))});
    invoke(c, 0x104ade40, 0x103e7534, {c.u32(squad), person_record(c, c.u32(offer + offer_person)), first_club, 0});
    invoke(c, 0x100ce200, 0x103e7540, {person, 1});
    if (std::int8_t(invoke(c, club_test, 0x103e7558, {first_club, 0, 1, 0, 0})) == 0 &&
        std::int8_t(invoke(c, 0x104ae570, 0x103e7570, {c.u32(squad), first_club, 0})) != 0)
        invoke(c, 0x104ad840, 0x103e7588, {c.u32(squad), first_club, person});
    invoke(c, 0x10006600, 0x103e7598, {c.u32(c.toc(-0x7b24)), person});
    // The record 0x103f2d50 gets: the signing's people and clubs, the game's date, the three deals' amounts.
    const auto record = frame.at(0xbc);
    invoke(c, 0x10715880, 0x103e75a0, {frame.at(0xdc)});
    c.put32(record + 0x10, c.u32(first_club));
    c.put32(record + 0x14, c.u32(second_club));
    c.put16(record + 0x22, c.u16(date + 2));
    c.put16(record + 0x20, c.u16(date));
    c.put8(record + 0x24, c.u8(date + 4));
    c.put32(record, c.u32(person));
    c.put32(record + 4, c.u32(c.u32(person + offer_deal) + 0x20));
    c.put32(record + 8, c.u32(c.u32(person + offer_second_deal) + 0x20));
    c.put32(record + 0xc, c.u32(c.u32(person + 0x30) + 0x20));
    c.put8(record + 0x1c, 2);
    c.put32(record + 0x18, 0);
    invoke(c, 0x103f2d50, 0x103e7610, {manager, record});
    c.put8(offer + offer_stage, stage_settled);
    invoke(c, 0x103eb4a0, 0x103e762c, {manager, offer, 5, 0});
    invoke(c, 0x103f6190, 0x103e7638, {manager, offer});
    invoke(c, 0x103ec5d0, 0x103e7644, {manager, offer});
    invoke(c, 0x10363b80, 0x103e7654, {c.u32(manager + shortlist), person, 1});
}

// 0x103e76c0 (see the header).
void reject_offer(Cpu& c, std::uint32_t manager, std::uint32_t person, std::int32_t index, std::uint32_t listed_only) {
    constexpr std::uint32_t stage_refused = 0x0f, stage_waiting = 10, refused_times = 3, fee_scale_high = 0x88, fee_scale_mid = 0x90,
                            fee_scale_low = 0x98, pool_zero = 8, rating = 0x1ec, mood = 0xa;
    Frame frame(c, 0x103e76c0, 0x100, {.first_gpr = 18});
    const auto pool = c.toc(-0x4ba0), people_cell = c.toc(toc::people), contracts_cell = c.toc(toc::contracts),
               found_cell = c.toc(toc::found_contract), clubs_cell = c.toc(toc::clubs), flag_cell = c.toc(-0x7bac),
               log_cell = c.toc(-0x7ba8), strings = c.toc(-0x4b9c);
    keep(c, 20, pool);
    keep(c, 21, people_cell);
    keep(c, 28, manager);
    keep(c, 22, contracts_cell);
    keep(c, 29, person);
    keep(c, 23, found_cell);
    keep(c, 30, listed_only);
    keep(c, 24, clubs_cell);
    keep(c, 25, flag_cell);
    keep(c, 26, log_cell);
    keep(c, 27, strings);
    const auto report = [&](std::uint32_t first, std::uint32_t second, std::uint32_t line, const ReportBack& back) {
        report_bad_offer(c, strings, log_cell, flag_cell, frame.at(first), frame.at(second), line, back);
    };
    if (index < 0) return report(0x98, 0x50, 0xf49, {0x103e771c, 0x103e7720, 0x103e773c, 0x103e774c});
    if (index >= c.i32(manager + 4)) return report(0x78, 0x48, 0xf4e, {0x103e777c, 0x103e7780, 0x103e779c, 0x103e77ac});
    const auto slot_offset = std::uint32_t(index) * 4;
    keep(c, 19, slot_offset);
    const auto offer = c.u32(c.u32(manager) + slot_offset);
    keep(c, 31, offer);
    if (!offer) return;
    // The original reads the person's word, the offer's person and then the fee; the order shows in what faults first.
    const auto person_word = c.i32(person);
    const auto offer_person_word = c.i32(offer + offer_person);
    const auto fee = c.u32(offer + offer_fee);
    keep(c, 18, fee);
    const bool quiet = listed_only != 0;
    const auto advance = [&] { invoke_native(c, 0x103e7cf0, 0x103e7cd8, {manager, offer}); };
    if (person_word == offer_person_word) {
        c.put8(offer + offer_kind, 4);
        keep(c, 25, 0);
        const auto person_club = club_record(c, c.u32(offer + offer_club));
        keep(c, 19, person_club);
        const auto contract = invoke(c, find_contract, 0x103e7804, {c.u32(contracts_cell), person});
        c.put32(found_cell, contract);
        const bool has_club = contract && contract_kind(c, contract) != no_club_contract_kind;
        if (has_club) keep(c, 25, 1);
        const auto other_club = has_club ? club_record(c, c.u32(contract + 4)) : 0;
        keep(c, 23, other_club);
        c.put8(offer + offer_stage, stage_refused);
        if (!quiet && std::int8_t(invoke(c, club_test, 0x103e7878, {person_club, 1, 1, 0, 0})) != 0 && c.u8(offer + offer_mode) != 1)
            c.put8(offer + offer_refusals, c.u8(offer + offer_refusals) + 1);
        else
            c.put8(offer + offer_refusals, refused_times);
        invoke(c, 0x103ec4f0, 0x103e78b0, {manager, offer});
        if (std::int8_t(invoke(c, club_test, 0x103e78c8, {person_club, 0, 1, 0, 0})) == 0) {
            const auto seen = std::int32_t(c.i8(offer + 0x11));
            invoke(c, 0x10365590, 0x103e7900,
                   {c.u32(manager + shortlist), person, person_club, 1, std::uint32_t(seen != 0xb), std::uint32_t(seen)});
        }
        invoke(c, 0x103eb4a0, 0x103e7914, {manager, offer, stage_waiting, 0});
        invoke(c, 0x103f6190, 0x103e7920, {manager, offer});
        if (c.u32(offer + offer_listed)) {
            rewind_list(c, c.u32(offer + offer_listed));
            while (const auto entry = invoke(c, next_listed, 0x103e795c, {c.u32(offer + offer_listed)}))
                invoke(c, 0x1007f840, 0x103e7954, {c.u32(contracts_cell), person_record(c, c.u32(entry)), other_club});
        }
        if (!quiet) {
            const auto mood_holder = c.u32(person + 0x14);
            if (person_club && other_club && mood_holder) {
                const auto strong = c.i16(person_club + rating), weak = c.i16(other_club + rating);
                if (strong > weak) {
                    guest::Fpu fpu(c);
                    const auto gap = std::int32_t(std::int16_t(std::uint32_t(strong) - std::uint32_t(weak)));
                    const auto scale_at = (strong > 0x1e46 && c.i16(mood_holder + mood) < 0x128e) ? fee_scale_high
                                          : strong > 0x109a                                       ? fee_scale_mid
                                                                                                  : fee_scale_low;
                    const auto bits = (std::uint64_t(0x43300000) << 32) | (std::uint32_t(gap) ^ 0x80000000u);
                    const auto shifted = fpu.fsub(std::bit_cast<double>(bits), c.f64(pool + pool_zero));
                    const auto step = std::int32_t(fpu.fctiwz(0, fpu.fmul(c.f64(pool + scale_at), shifted)));
                    c.put16(mood_holder + mood, std::uint32_t(c.i16(mood_holder + mood)) + std::uint32_t(step));
                    const auto holder = c.u32(person + 0x14);
                    if (c.i16(holder + mood) > 10000) c.put16(holder + mood, 10000);
                }
            }
            c.put32(frame.at(0x38), fee);
            invoke_native(c, 0x10403e20, 0x103e7aa0, {manager, person, person_club, other_club, 4, offer, listed_only, 0});
        }
        advance();
        return;
    }

    if (!c.u32(offer + offer_listed)) {
        report(0x58, 0x40, 0x101f, {0x103e7c94, 0x103e7c98, 0x103e7cb4, 0x103e7cc4});
        advance();
        return;
    }
    rewind_list(c, c.u32(offer + offer_listed));
    for (;;) {
        const auto entry = invoke(c, next_listed, 0x103e7c70, {c.u32(offer + offer_listed)});
        keep(c, 25, entry);
        if (!entry) break;
        if (c.i32(entry) != c.i32(person)) continue;
        const auto share = invoke(c, 0x10084be0, 0x103e7af8,
                                  {c.u32(contracts_cell), person, club_record(c, c.u32(offer + offer_club)), 0, 0xffffffff, 0});
        const auto person_row = person_record(c, c.u32(offer + offer_person));
        const auto new_fee = c.u32(offer + offer_fee) + share;
        keep(c, 21, new_fee);
        const auto contract = invoke(c, find_contract, 0x103e7b18, {c.u32(contracts_cell), person_row});
        c.put32(found_cell, contract);
        const auto contract_club_ref = contract ? club_record(c, c.u32(contract + 4)) : 0;
        keep(c, 20, contract_club_ref);
        const auto offer_club_ref = club_record(c, c.u32(offer + offer_club));
        keep(c, 22, offer_club_ref);
        invoke(c, drop_listed, 0x103e7b58, {c.u32(offer + offer_listed), entry});
        if (c.i16(c.u32(offer + offer_listed)) == 0) {
            invoke(c, list_destroy, 0x103e7b70, {c.u32(offer + offer_listed), 1});
            c.put32(offer + offer_listed, 0);
        }
        const auto row = c.u32(manager + status_rows) + c.u32(person) * status_row_size + status_row_listed;
        c.put8(row, c.u8(row) - 1);
        if (!quiet) {
            c.put32(frame.at(0x38), new_fee);
            invoke_native(c, 0x10403e20, 0x103e7bc4, {manager, person, contract_club_ref, offer_club_ref, 4, offer, listed_only, 0});
        }
        c.put32(offer + offer_fee, new_fee);
        if (contract_club_ref && std::int8_t(invoke(c, club_test, 0x103e7be8, {contract_club_ref, 0, 1, 0, 0})) == 0) {
            invoke_native(c, 0x103ee1b0, 0x103e7c04, {manager, c.u32(manager) + slot_offset, 1});
            if (std::int8_t(invoke(c, 0x103fb390, 0x103e7c10, {manager, offer})) != 0) return;
        }
        if (std::int8_t(invoke(c, club_test, 0x103e7c30, {offer_club_ref, 0, 1, 0, 0})) == 0) {
            invoke_native(c, 0x103e4d70, 0x103e7c50, {c.u32(c.toc(toc::manager)), c.u32(manager) + slot_offset, 1});
            if (std::int8_t(invoke(c, 0x103fb390, 0x103e7c5c, {manager, offer})) != 0) return;
        }
        break;
    }
    advance();
}
}
