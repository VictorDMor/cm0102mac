// FINANCE.CPP, block daily: an ordinary day's unscheduled events, the season roll (0x100fcfd0) and
// the helpers the passes share (the sample table, the floating-point conversions, the bookings).
#include "finance_daily.hpp"
#include "finance_detail.hpp"
#include <bit>
#include <stdexcept>

namespace cm::aot::finance {
using guest::Frame;

namespace {
constexpr std::uint32_t unscheduled_event = 0x10101400, club_class = 0x10191fd0, move_bytes = 0x10739ef0,
                        fill_bytes = 0x10778300, refresh_season = 0x100fd210;
constexpr std::uint32_t season_roll_entry = 0x100fcfd0;
constexpr std::uint32_t club_type_size = 0x4c;
constexpr CheckSite no_season_slot{0x100fd078, 0x48, 0x40, 0x15cf};
}

namespace daily {
bool Pass::inactive(std::uint32_t club_at, std::uint32_t back) {
    return std::int8_t(invoke(c, callee::club_is_inactive, back, {club_at})) != 0;
}

bool Pass::takes_part(std::uint32_t club_at) const {
    const auto division_at = c.u32(club_at + club::division);
    if (division_at == 0) return false;
    const auto nation_at = c.u32(club_at + club::nation);
    if (nation_at == 0) return false;
    const auto flag = c.i8(nation_at + 0x120);
    if (flag != 0 && c.u32(division_at) == c.u32(c.r(18))) return false;
    return flag != 0 || reputation(club_at) >= 5000;
}

// ---- the sample table (finance_detail.hpp's draw, in the steps this routine interleaves) ----
bool Pass::step_sample() {
    const auto cursor_cell = c.toc(sample_table::cursor);
    const auto next = c.u32(cursor_cell) + 2;
    c.put32(cursor_cell, next);
    return next > c.u32(c.toc(sample_table::last));
}

void Pass::rewind_sample(std::uint32_t back) {
    c.put32(c.toc(sample_table::cursor), c.toc(sample_table::first));
    invoke(c, sample_table::redraw, back);
    c.put32(c.toc(sample_table::base), c.r(3) & 0xffff);
}

std::uint32_t Pass::current_sample() const {
    return std::uint32_t(c.u16(c.u32(c.toc(sample_table::cursor)))) + c.u32(c.toc(sample_table::base));
}

std::uint32_t Pass::draw_sample(std::uint32_t back) {
    if (step_sample()) rewind_sample(back);
    return current_sample();
}

// The inline draw below a limit (the same code as 0x100f2100, inlined at six sites): the table's
// number modulo the limit, with the limit's sign. The original has a floating-point branch for
// limits beyond +-0xffff (it calls 0x100f2100 with limit * 10 / 0xffff), which no site reaches: the
// limits are an int16 reputation (0x100fb0c8), an extsh'd int16 (0x100fb44c), (0x1f - byte) * 10 in
// -960..1590 (0x100fb9b4), a byte + 1 (0x100fbb4c), a byte * 10 (0x100fbe90), and 10..43 (0x100fce00).
// The caller has tested the limit against 0 (a draw with limit 0 is not made).
template <unsigned Result>
std::int32_t Pass::draw_below(std::int32_t limit, std::uint32_t wrap_back) {
    if (step_sample()) rewind_sample(wrap_back);
    if (limit > 0xffff || limit < -0xffff) throw std::logic_error("draw below a limit beyond 16 bits: no site reaches it");
    const auto sample = std::int32_t(current_sample());
    const auto quotient = std::int32_t(std::int64_t(sample) / limit);
    const auto rest = std::uint32_t(sample) - std::uint32_t(quotient) * std::uint32_t(limit);
    const auto result = limit > 0 ? rest : 0u - rest;
    c.set(Result, result);
    return std::int32_t(result);
}
template std::int32_t Pass::draw_below<15>(std::int32_t, std::uint32_t);
template std::int32_t Pass::draw_below<20>(std::int32_t, std::uint32_t);

// ---- floating point ----
double Pass::exact(std::int32_t x) {
    const auto bits = (std::uint64_t(0x43300000) << 32) | (std::uint32_t(x) ^ 0x80000000u);
    return fpu.fsub(std::bit_cast<double>(bits), pool_f(doubles::int_magic));
}
double Pass::rounded(std::int32_t x) {
    const auto bits = (std::uint64_t(0x43300000) << 32) | (std::uint32_t(x) ^ 0x80000000u);
    return fpu.fsubs(std::bit_cast<double>(bits), pool_f(doubles::int_magic));
}

// ---- bookings (the original does these inline: a line, its second copy, the balance) ----
void Pass::book_income(std::uint32_t account, const LedgerLine& ledger_line, std::int32_t amount) {
    book(c, account, ledger_line, std::uint64_t(std::int64_t(amount)));
}
void Pass::book_expense(std::uint32_t account, const LedgerLine& ledger_line, std::uint32_t amount) {
    book(c, account, ledger_line, std::uint64_t(amount));
}

// A club that takes part, whose account is of class 1 or 2 and whose division is not one the table
// leaves out, gets a draw below 10 or more (the reputation's distance below 11000, in 250s); when the draw comes up 0
// the unscheduled-event routine is told so (its third argument is 1).
void Pass::ordinary_day() {
    Held<16> index(c, 0);
    Held<19> account(c, c.u32(manager + manager::accounts));
    Held<17> club_h(c, c.u32(c.toc(toc::clubs)));
    while (index.i32() < c.i32(c.r(22))) {
        const std::uint32_t club_at = club_h, account_at = account;
        if (inactive(club_at, 0x100fcd40)) return;
        if (takes_part(club_at)) {
            // The division word is not 0 here (takes_part), so the original's second test of it is dead.
            bool skipped = false;
            const auto table = c.u32(c.r(14));
            if (table != 0) {
                const auto number = c.i32(c.u32(club_at + club::division));
                if (number > 0 && number < c.i32(c.r(13)) - 1 && reputation(club_at) < 0x109a &&
                    c.u32(table + std::uint32_t(number) * 4) == 0)
                    skipped = true;
            }
            const auto kind = c.i8(account_at + account_class);
            if (!skipped && (kind == 2 || kind == 1)) {
                const std::int32_t reach = (0x2af8 - reputation(club_at)) / 250;
                Held<15> limit(c, 10);
                if (reach >= 10) limit = std::uint32_t(reach);
                // limit >= 10, so the original's branch for a limit of 0 (0x100fce2c) is dead.
                if (limit.i32() == 0) throw std::logic_error("limit is at least 10 (0x100fce24)");
                Held<20> drawn(c);
                draw_below<20>(limit.i32(), 0x100fce54);
                invoke_unit(c, unscheduled_event, 0x100fcf94, {manager, club_at, std::uint32_t(std::uint32_t(drawn) == 0)});
            }
        }
        index = index + 1;
        account = account + account::size;
        club_h = club_h + club_size;
    }
}
}

// The season roll: for every club of the account's kind (the club class's table row at TOC
// -0x7b44 is the caller's slot), refresh the account (0x100fd210), then move the second period's
// lines (0x110..0x160) over the first (0xc0..0x110) and clear them.
void roll_season_ledgers(Cpu& c, std::uint32_t manager, std::uint32_t season_slot) {
    Frame frame(c, season_roll_entry, 0x90, {.first_gpr = 25});
    c.set({{26, manager}, {27, season_slot}, {30, c.toc(-0x7b44)}, {31, c.toc(toc::club_count)},
           {29, c.toc(toc::message)}, {28, c.toc(toc::pool)}});
    if (daily::first_day_after(c, c.toc(toc::today))) return;
    if (season_slot == 0) {
        report_check(c, frame, no_season_slot);
        return;
    }
    daily::Held<29> index(c, 0);
    daily::Held<25> offset(c, 0);
    daily::Held<28> club_h(c, c.u32(c.toc(toc::clubs)));
    while (index.i32() < c.i32(c.r(31))) {
        const std::uint32_t club_at = club_h;
        if (std::int8_t(invoke(c, callee::club_is_inactive, 0x100fd0d8, {club_at})) != 0) return;
        if (c.u32(club_at + daily::club::division) != 0 && c.u32(club_at + daily::club::nation) != 0) {
            const auto nation_at = c.u32(club_at + daily::club::nation);
            if (c.i8(nation_at + 0x120) != 0 || c.i16(club_at + daily::club::reputation) >= 5000) {
                const auto type = std::int8_t(invoke(c, club_class, 0x100fd118, {club_at}));
                if (c.r(30) + std::uint32_t(std::int32_t(type)) * club_type_size == season_slot) {
                    invoke_unit(c, refresh_season, 0x100fd138, {manager, club_at});
                    auto accounts = c.u32(manager + manager::accounts);
                    invoke(c, move_bytes, 0x100fd150, {accounts + offset + 0xc0, accounts + offset + 0x110, 0x50});
                    accounts = c.u32(manager + manager::accounts);
                    invoke(c, fill_bytes, 0x100fd168, {accounts + offset + 0x110, 0, 0x50});
                }
            }
        }
        offset = offset + account::size;
        index = index + 1;
        club_h = club_h + club_size;
    }
}
}
