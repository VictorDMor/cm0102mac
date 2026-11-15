#pragma once
// FINANCE.CPP, block daily: the finance manager's pass for one day (0x100fa6e0, run by the game's
// daily update) and the season roll of the accounts (0x100fcfd0).
//
// The daily pass has three parts, in this order:
//   1. On the one day the game fixes by date (0x107156f0), two clubs are moved onto rows of the
//      0x50-byte table and the list entries that refer to them follow (move_clubs_to_rows).
//   2. Every active club's account is brought back into a sane range, or refreshed on a Saturday
//      (settle_balances).
//   3. On the last day of a month every qualifying club pays and earns what the month brings and its
//      ledger rolls over (month_end); on any other day a few clubs get a small unscheduled event
//      from 0x10101400 (ordinary_day).
#include "guest_abi.hpp"
#include "guest_fpu.hpp"
#include "finance_ledger.hpp"
#include <cstdint>

namespace cm::aot::finance {
using guest::Cpu;

void daily_pass(Cpu&, std::uint32_t manager, std::uint32_t date);                      // 0x100fa6e0
void roll_season_ledgers(Cpu&, std::uint32_t manager, std::uint32_t season_slot);      // 0x100fcfd0

namespace daily {
// A local the original keeps in non-volatile register N across its calls. Unlike guest::Kept, the
// view can start from what an earlier phase left in the register. The callees save the register in
// their frames, so each assignment the original makes is made here too.
template <unsigned N>
class Held {
public:
    explicit Held(Cpu& cpu) : c(cpu) {}
    Held(Cpu& cpu, std::uint32_t value) : c(cpu) { c.set(N, value); }
    Held(const Held&) = delete;
    Held& operator=(std::uint32_t value) { c.set(N, value); return *this; }
    Held& operator=(const Held& other) { return *this = std::uint32_t(other); }
    operator std::uint32_t() const { return c.r(N); }
    std::int32_t i32() const { return std::int32_t(c.r(N)); }

private:
    Cpu& c;
};

// Offsets in a club record (0x248 bytes; shortlist_manager_records.hpp has more names).
namespace club {
inline constexpr std::uint32_t attendance = 0x08, ticket_0c = 0x0c, ticket_10 = 0x10, nation = 0x2c, division = 0x30,
                               ground = 0x3c, assistant = 0x80, manager = 0x90, squad = 0x98,  // 50 persons
                               reputation = 0x1ec, status = 0x23d, on_row = 0x23e, tier = 0x240, flag_241 = 0x241;
inline constexpr unsigned squad_size = 50;
}
// Account lines this block books beyond finance_ledger.hpp's (current period; the second copy is
// account::second_copy further on). Names are guesses from what the code computes.
namespace line {
inline constexpr LedgerLine overdraft{0x84, false, false},      // a share of a negative balance
                            running_8c{0x8c, false, false},     // costs from the ground, the tier and the division
                            earnings_a0{0xa0, false, true},     // clubs without a division table entry
                            earnings_a4{0xa4, false, true},
                            interest{0xb0, false, true},        // a share of a positive balance
                            earnings_b4{0xb4, false, true};
}
inline constexpr std::uint32_t account_pending = 0x0c,         // an amount a Saturday charges to expense_90
                               account_series = 0x18, account_class = 0x165, account_flag = 0x164;

// Whether the game's first day (the date cell at TOC -0x7a9c) lies after `date_at`.
bool first_day_after(Cpu&, std::uint32_t date_at);

class Pass {
public:
    Pass(Cpu& cpu, guest::Frame& frame, std::uint32_t manager, std::uint32_t date);
    void run();

private:
    // ---- finance_daily.cpp: the gate, the fixed day, the sweep over the accounts ----
    bool before_first_day() const;
    bool is_fixed_day();
    void move_clubs_to_rows();
    void settle_balances();
    template <unsigned ClubReg, unsigned EntryReg>
    void follow_list_entries(std::uint32_t iterator, std::uint32_t cell, std::uint32_t back_init,
                             std::uint32_t back_find, std::uint32_t back_next, std::uint32_t back_remove,
                             std::uint32_t back_insert, std::uint32_t back_done);
    void raise_attendance(std::uint32_t club, std::int32_t gain, std::int32_t floor);
    void confine_balance(std::uint32_t account, std::int64_t low, std::int64_t high);
    void drift_balance(std::uint32_t account);
    void saturday_charge(std::uint32_t account, std::uint32_t club);
    // ---- finance_daily_month.cpp: the last day of a month ----
    void month_end();
    void month_end_club(std::uint32_t account, std::uint32_t club);
    void sell_unwanted_players(std::uint32_t club);
    void consider_selling(std::uint32_t person, std::uint32_t record, std::uint32_t club);
    void ground_and_tier_costs(std::uint32_t account, std::uint32_t club);
    void balance_interest(std::uint32_t account, std::uint32_t club);
    void assistant_costs(std::uint32_t account, std::uint32_t club);
    void gate_earnings(std::uint32_t account, std::uint32_t club);
    void division_earnings(std::uint32_t account, std::uint32_t club);
    void running_costs(std::uint32_t account, std::uint32_t club);
    void roll_ledger(std::uint32_t account);
    // ---- finance_daily_day.cpp: an ordinary day, and the draws ----
    void ordinary_day();
    // ---- shared ----
    bool inactive(std::uint32_t club, std::uint32_t back);
    // Whether a club takes part in the finance rules: the division and the nation exist and are not
    // set aside by the nation's flag, the reputation or the game's own division.
    bool takes_part(std::uint32_t club) const;
    std::int32_t reputation(std::uint32_t club) const { return c.i16(club + club::reputation); }
    // The sample table.
    bool step_sample();
    void rewind_sample(std::uint32_t back);
    std::uint32_t current_sample() const;
    std::uint32_t draw_sample(std::uint32_t back);
    template <unsigned Result>
    std::int32_t draw_below(std::int32_t limit, std::uint32_t wrap_back);
    // Floating point: the original's int-to-double (magic word less the pool constant), the
    // pool's constants, and the truncation to a word.
    double exact(std::int32_t x);
    double rounded(std::int32_t x);
    double pool_f(std::uint32_t offset) const { return c.f64(pool + offset); }
    std::int32_t truncate(double value) { return std::int32_t(fpu.fctiwz(0, value)); }
    double single_of(double v) { return fpu.frsp(v); }
    void book_income(std::uint32_t account, const LedgerLine& line, std::int32_t amount);
    void book_expense(std::uint32_t account, const LedgerLine& line, std::uint32_t amount);

    Cpu& c;
    guest::Frame& frame;
    guest::Fpu fpu;
    std::uint32_t manager, date, pool;
};
}
}
