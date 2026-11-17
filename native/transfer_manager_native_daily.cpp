#include "transfer_manager_native_daily.hpp"
#include "transfer_manager_native_detail.hpp"
#include <stdexcept>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
using daily::Lookup;
constexpr std::uint32_t terms_check = 0x103f9d60, answer_terms = 0x103e8500, answer_refused = 0x103e9910,
                        rest_of_offer = 0x103ebf50, plan_offer = 0x103efaa0, release_contract = 0x10082ea0,
                        note_offer_taken = 0x10407460, offer_with_contract = 0x103f2ec0, set_stage = 0x103eb4a0,
                        settle_offer = 0x103ed170;
constexpr std::uint32_t offer_taken_flag = 0x37, offer_kind = 0x38;
constexpr std::uint32_t terms_code_at = 0x38;  // the halfword 0x103f9d60 leaves in the caller's frame

// The offer's phase: the signed top four bits of its byte +0x38.
std::int32_t phase_of(Cpu& c, std::uint32_t offer) { return std::int32_t(c.i8(offer + offer_kind)) >> 4; }
std::uint32_t signed_half(Cpu& c, std::uint32_t at) { return std::uint32_t(std::int32_t(c.i16(at))); }
// The offer's club test through the club's status flag (0x1014c2e0 with its fixed arguments).
bool club_blocked(Cpu& c, std::uint32_t club, std::uint32_t back) {
    return std::int8_t(invoke(c, daily::club_flag, back, {club, 0, 1, 0, 0})) != 0;
}
}

void decide_answered_offer(Cpu& c, std::uint32_t manager, std::uint32_t offer, std::uint32_t final_answer) {
    Frame frame(c, 0x103e8280, 0x60, {.first_gpr = 24});
    Lookup look{c};
    keep(c, 24, look.found_cell);
    keep(c, 25, manager);
    keep(c, 26, offer);
    keep(c, 27, final_answer);
    const auto person = look.person_of(offer);
    keep(c, 28, person);
    keep(c, 29, 0);
    keep(c, 30, look.contracts);
    keep(c, 31, look.clubs);
    const auto found = look.find(person, 0x103e82cc);
    keep(c, 29, look.usable(found));
    const auto contract_club = look.club_of_found(found);
    keep(c, 29, contract_club);
    const auto club = look.offer_club_of(offer);
    keep(c, 24, club);
    const auto code_at = frame.at(terms_code_at);
    const auto first_code = invoke_native(c, terms_check, 0x103e8340, {manager, offer, club, code_at, 0});
    keep(c, 27, first_code);
    if (std::int8_t(final_answer) == 0 || c.i16(code_at) != 9) {
        const bool blocked = club_blocked(c, club, 0x103e8370);
        if (!blocked && std::int8_t(first_code) == 1) {
            invoke_native(c, answer_terms, 0x103e8398, {manager, offer, club, 0});
            return;
        }
        if (std::int8_t(first_code) == 2 || std::int8_t(first_code) == 1) {
            invoke_native(c, answer_refused, 0x103e83c8, {manager, offer, club, signed_half(c, code_at), 1});
            return;
        }
    }
    if (contract_club != 0) {
        const auto code = invoke_native(c, terms_check, 0x103e83ec, {manager, offer, contract_club, code_at, 0});
        keep(c, 27, code);
        const bool blocked = club_blocked(c, contract_club, 0x103e8408);
        if (!blocked && std::int8_t(code) == 1) {
            invoke_native(c, answer_terms, 0x103e8430, {manager, offer, contract_club, 0});
            return;
        }
        if (std::int8_t(code) == 2 || std::int8_t(code) == 1) {
            invoke_native(c, answer_refused, 0x103e8460, {manager, offer, contract_club, signed_half(c, code_at), 1});
            return;
        }
    }
    invoke_native(c, rest_of_offer, 0x103e8470, {manager, offer});
    invoke(c, release_contract, 0x103e847c, {c.u32(look.contracts), c.u32(offer + daily::offer_person)});
    invoke_native(c, note_offer_taken, 0x103e8488, {manager, offer});
    if (std::int8_t(c.u8(offer + offer_taken_flag)) != 0 && contract_club != 0 && phase_of(c, offer) == 3) {
        if (const auto again = invoke(c, daily::find_contract, 0x103e84c0, {c.u32(look.contracts), person})) {
            invoke(c, offer_with_contract, 0x103e84d8, {manager, offer, again + 0x30});
            return;
        }
    }
    invoke_native(c, plan_offer, 0x103e84e8, {manager, offer});
}

void run_due_offers(Cpu& c, std::uint32_t manager) {
    constexpr std::uint32_t rows_at = 0xa50, rows_count = 0xa58, row_size = 12, row_offer = 8;
    constexpr std::uint32_t offers_array = 0, offer_stage_after = 0x14;
    Frame frame(c, 0x103f3100, 0xd0, {.first_gpr = 18});
    Lookup look{c};
    const auto date_cell = c.toc(-0x7b80), flag_cell = c.toc(-0x7bac), message_cell = c.toc(-0x7ba8), strings = c.toc(-0x4b9c);
    keep(c, 22, 0);
    keep(c, 23, date_cell);
    keep(c, 24, look.people);
    keep(c, 25, look.contracts);
    keep(c, 20, 0);
    keep(c, 26, look.found_cell);
    keep(c, 21, 0);
    keep(c, 27, look.clubs);
    keep(c, 28, flag_cell);
    keep(c, 29, message_cell);
    keep(c, 30, strings);
    keep(c, 31, manager);
    const auto code_at = frame.at(terms_code_at);
    c.put16(code_at, 0);
    std::uint32_t done = 0;
    std::uint32_t index = 0;
    for (; std::int32_t(index) < c.i32(manager + rows_count); ++index) {
        const auto row = c.u32(manager + rows_at) + index * row_size;
        // The original copies the date into frame slots, and later routines read leftovers there.
        c.put32(frame.at(0x3c), c.u32(date_cell));
        c.put32(frame.at(0x40), c.u32(date_cell + 4));
        // The rows are in date order: the first one that is still ahead ends the pass.
        if (daily::before(c.i16(date_cell), c.u16(date_cell + 2), c.i16(row), c.u16(row + 2))) break;
        const auto offer = c.u32(c.u32(manager + offers_array) + c.u32(row + row_offer) * 4);
        keep(c, 19, offer);
        if (offer == 0) {
            guest::report_assertion(c, {0x103f31d8, strings + 0x37, strings + 0x6ff, strings + 0x70c, frame.at(0x74),
                                        frame.at(0x4c), 0x2845});
            keep(c, 20, ++done);
        } else if (std::int8_t(invoke_native(c, settle_offer, 0x103f322c, {manager, offer, 1})) == 0) {
            keep(c, 20, ++done);
        } else {
            const auto club = look.offer_club_of(offer);
            const auto code = invoke_native(c, terms_check, 0x103f3260, {manager, offer, club, code_at, 0});
            if (std::int8_t(code) != 0 && c.i16(code_at) != 9) {
                invoke_native(c, answer_refused, 0x103f3294, {manager, offer, club, signed_half(c, code_at), 0});
                keep(c, 20, ++done);
            } else {
                // The offer's person's club twice more, as the original asks before it acts.
                const auto person = look.person_of(offer);
                keep(c, 18, 0);
                auto found = look.find(person, 0x103f32b8);
                if (look.usable(found)) keep(c, 18, 1);
                auto contract_club = look.usable(found) ? look.club_of_found(found) : 0;
                bool handled = false;
                if (contract_club != 0) {
                    keep(c, 18, 0);
                    found = look.find(person, 0x103f3328);
                    if (look.usable(found)) keep(c, 18, 1);
                    contract_club = look.usable(found) ? look.club_of_found(found) : 0;
                    const auto second = invoke_native(c, terms_check, 0x103f3388, {manager, offer, contract_club, code_at, 0});
                    if (std::int8_t(second) != 0 && c.i16(code_at) != 9) {
                        keep(c, 18, 0);
                        found = look.find(person, 0x103f33b8);
                        if (look.usable(found)) keep(c, 18, 1);
                        contract_club = look.usable(found) ? look.club_of_found(found) : 0;
                        invoke_native(c, answer_refused, 0x103f3418, {manager, offer, contract_club, signed_half(c, code_at), 0});
                        keep(c, 20, ++done);
                        handled = true;
                    }
                }
                if (!handled) {
                    invoke_native(c, note_offer_taken, 0x103f342c, {manager, offer});
                    invoke_native(c, plan_offer, 0x103f3438, {manager, offer});
                    c.put8(offer + daily::offer_stage, offer_stage_after);
                    invoke(c, set_stage, 0x103f3454, {manager, offer, 10, 0});
                    keep(c, 20, ++done);
                }
            }
        }
        keep(c, 22, (index + 1) * row_size);
        keep(c, 21, index + 1);
    }
    // The count of rows handled cannot be negative; the original's assertion for it never fires.
    if (std::int32_t(done) < 0) throw std::logic_error("run_due_offers: negative count (0x103f3474 is unreachable)");
    if (done != 0) {
        std::uint32_t at = 0, from = done * row_size;
        for (std::uint32_t i = done; std::int32_t(i) < c.i32(manager + rows_count); ++i, at += row_size, from += row_size) {
            const auto source = c.u32(manager + rows_at) + from, target = c.u32(manager + rows_at) + at;
            c.put16(target + 2, c.u16(source + 2));
            c.put16(target, c.u16(source));
            c.put8(target + 4, c.u8(source + 4));
            c.put32(target + 8, c.u32(source + 8));
        }
        c.put32(manager + rows_count, c.u32(manager + rows_count) - done);
    }
}
}
