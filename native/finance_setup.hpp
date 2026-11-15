#pragma once
// FINANCE.CPP, block setup: the draw below a limit, a club's account at the start of a game, the
// manager's array of accounts and its saved-game file, and the days-since question.
// Contract and quirks: docs/cpp-rewrites.md (see finance.hpp).
#include "finance_detail.hpp"
#include "guest_abi.hpp"
#include <bit>
#include <cstdint>

namespace cm::aot::finance {
using guest::Cpu;

// A number below `limit` (0 for 0; the sign of the limit is the sign of the answer) from the game's
// shared sample table; limits past 0xffff scale a sample and add a draw below a tenth-scale limit.
std::int32_t random_below(Cpu&, std::int32_t limit);                                     // 0x100f2100
// A club's account as the game starts: the opening balance from the club's funds with a random
// jitter, the ledger cleared, the attendance figure for clubs of the top divisions.
void start_account(Cpu&, std::uint32_t account, std::uint32_t club);                     // 0x100f1050
// The finance manager's constructor: the accounts array (one per club), read from the saved game
// when `file` is not null, else started from the clubs. Returns the manager.
std::uint32_t create_manager(Cpu&, std::uint32_t manager, std::uint32_t file);           // 0x100f84e0
// Reads the accounts from the saved game's file; false (and the success flag cleared) on a failure.
bool read_accounts(Cpu&, std::uint32_t manager, std::uint32_t file);                     // 0x100f9b00
// Writes the accounts to the saved game's file.
void write_accounts(Cpu&, std::uint32_t manager, std::uint32_t file);                    // 0x100f9c50
// Days since the date kept in the item's row, when today falls in the window the row allows (else
// 0); `*year_reached` says whether this year is the row's last. What the item is: unknown.
std::uint32_t days_since_recorded(Cpu&, std::uint32_t item, std::uint32_t year_reached); // 0x100f9d80

// Shared by the routines that expand the draw inline.
namespace setup {
// The word `magic` leaves on the stack for an int-to-double: the high word 0x43300000 and the
// integer biased by 0x80000000.
inline double magic(std::int32_t x) {
    return std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (std::uint32_t(x) ^ 0x80000000u));
}
// The sample draw: the cursor cell advances by one halfword; past the last entry it returns to the first and the base
// is drawn again (0x1077b770, `back` is the address after that `bl`; `hold()` sets the non-volatile registers just
// before). The sample is the halfword under the cursor plus the base, a word.
template <class Hold>
inline std::uint32_t sample(Cpu& c, std::uint32_t back, Hold&& hold) {
    const auto cursor_cell = c.toc(sample_table::cursor), base_cell = c.toc(sample_table::base);
    const auto next = c.u32(cursor_cell) + 2;
    c.put32(cursor_cell, next);
    if (next > c.u32(c.toc(sample_table::last))) {
        c.put32(cursor_cell, c.toc(sample_table::first));
        hold();
        c.call(sample_table::redraw, back);
        c.put32(base_cell, c.r(3) & 0xffff);
    }
    return std::uint32_t(c.u16(c.u32(cursor_cell))) + c.u32(base_cell);
}
// `sample` taken as a remainder of the limit `n` (the original's divw, mullw, subf; the sign of the limit sets the
// sign of the answer). A zero limit leaves the sample whole: the quotient of divw by 0 is multiplied by 0 again.
inline std::int32_t remainder_in(std::uint32_t sample, std::int32_t n) {
    const auto quotient = n == 0 ? 0 : std::int32_t(std::int64_t(std::int32_t(sample)) / n);
    const auto rest = std::int32_t(sample - std::uint32_t(quotient) * std::uint32_t(n));
    return n > 0 ? rest : std::int32_t(0u - std::uint32_t(rest));
}
inline bool fits_one_draw(std::int32_t n) { return n >= -0xffff && n <= 0xffff; }
// Wrapping word arithmetic the original does in registers.
inline std::int32_t word(std::uint32_t x) { return std::int32_t(x); }
}
}
