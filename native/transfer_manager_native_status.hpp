#pragma once
// TRANSFER_MANAGER.CPP native-first block "status": typed entry points (guest addresses for records).
#include "guest_abi.hpp"
#include "transfer_manager_native_detail.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native {
// The game's draw from a table of halfwords: the cursor advances two bytes (and past the limit it returns to
// the start and the seed word is redrawn from 0x1077b770), the value is the halfword at the cursor plus the
// seed. The four cells are the module's table-of-contents words -0x7b84 (cursor variable), -0x7b88 (limit
// variable), -0x7b8c (start) and -0x7b90 (seed variable).
struct HalfwordDraw {
    guest::Cpu& c;
    std::uint32_t cursor, limit, start, seed;
    explicit HalfwordDraw(guest::Cpu& cpu)
        : c(cpu), cursor(cpu.toc(-0x7b84)), limit(cpu.toc(-0x7b88)), start(cpu.toc(-0x7b8c)), seed(cpu.toc(-0x7b90)) {}
    // Advances the cursor; `back` is the return address of the generator call at that site.
    void advance(std::uint32_t back) const {
        const auto next = c.u32(cursor) + 2;
        c.put32(cursor, next);
        if (next > c.u32(limit)) {
            c.put32(cursor, start);
            const auto fresh = invoke(c, 0x1077b770, back);
            c.put32(seed, fresh & 0xffff);
        }
    }
    std::uint32_t value() const { return std::uint32_t(c.u16(c.u32(cursor))) + c.u32(seed); }
};

// 0x103e9220: what remains of a person's transfer status `status` (a flag of the manager's per-person
// status rows at +0x9a0): for status 4 the person's byte +0x5e less the row's byte +8, otherwise the
// signed halfword +0x42 of the person's contract (0 without one).
std::int32_t remaining_status_term(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::int8_t status);
// 0x103e8d30: after a match (`match` = the fixture, `lineups` = both sides' squad rows), for each side's club
// settles the status fees of its lined-up people and counts appearances/starts for the people the sides' clubs
// had under their status.
void match_transfer_statuses(guest::Cpu& c, std::uint32_t manager, std::uint32_t match, std::uint32_t lineups);
// 0x103ed090: sets the offer record's byte +0x34 to 10, then hands a weekday (18..24, drawn from the
// generator) to 0x103eb4a0 for it.
void start_offer_with_weekday(guest::Cpu& c, std::uint32_t offer_owner, std::uint32_t record);
// 0x103ed520: whether the person is refused (1) or not (0) by the other side: contract and offer terms, the
// club's interest row `row` (looked up when 0), age, form history and, with `noisy`, generator-drawn margins.
std::uint32_t person_refused(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t offer,
                             std::uint32_t row, std::uint32_t noisy);
// 0x103ef8a0: for the offer's person and each person of its list, settles the status fees the status row's
// flags 8, 2 and 4 ask for.
void settle_offer_statuses(guest::Cpu& c, std::uint32_t manager, std::uint32_t offer);
// 0x103f2560: for a person with status flag 8, finds the club's row for the person's club, takes its share of
// the fee (the offer's value, or the contract manager's) and, from 1000 up, posts the news and the finance
// entries.
void settle_status_fee(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::uint32_t offer);
// 0x103f2800: the same for status flags 2 and 5 from the row's `kind` tables (`target` is the row's byte
// +8 or -1, `extra` the offer); returns 1 when it posted.
// `kind` and `noisy` are the whole register words (the original keeps them in non-volatile registers; only
// the low byte is tested).
std::uint32_t settle_status_row_fee(guest::Cpu& c, std::uint32_t manager, std::uint32_t person, std::int32_t target,
                                    std::uint32_t kind, std::uint32_t extra);
}
