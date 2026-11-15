// FINANCE.CPP, block daily: the entry of the daily pass (0x100fa6e0), its gate and the two sweeps that
// run every day: the fixed day's club moves and the accounts' sanity range (finance_daily.hpp).
#include "finance_daily.hpp"
#include "finance_detail.hpp"
#include <stdexcept>

namespace cm::aot::finance {
using guest::Frame;

namespace {
constexpr std::uint32_t unscheduled_check = 0x100fa6e0;  // the entry
// TOC words of this routine.
constexpr std::int32_t first_day = -0x7a9c,          // the game's first day: nothing happens before it
    first_club_cell = -0x74a8, second_club_cell = -0x74ac, special_club_cell = -0x7494;
constexpr std::uint32_t season_day_maker = 0x107156f0, next_day = 0x10717890, month_of = 0x10718360,
                        weekday_of = 0x10715b20, compare_text = 0x1077dcd0;
constexpr std::uint32_t refresh_account = 0x100f4f80, charge_clubs = 0x100fa1f0, pending_deal = 0x100f9d80,
                        settle_deal = 0x100f7270, place_club = 0x101030c0;
constexpr std::uint32_t list_start = 0x10738c00, list_next = 0x10738e00, list_end = 0x10738c20,
                        list_find = 0x1048b180, list_remove = 0x1048a140, list_insert = 0x104888f0;
constexpr std::uint32_t row_size = 0x50;
constexpr std::uint32_t row_name = 0x18, row_money = 0x04;
constexpr std::uint32_t first_name_at = 0x162, second_name_at = 0x169;  // in the unit's string pool
constexpr std::int16_t saturday = 6;
}

namespace daily {
// The original compares the game's first day with a date as one halfword pair: the low halfwords
// as unsigned, then the high halfwords through a sign trick that comes out as "greater".
bool first_day_after(Cpu& c, std::uint32_t date_at) {
    const auto word = c.u32(c.toc(first_day));
    const std::uint32_t low = word & 0xffff, other_low = c.u16(date_at + 2);
    if (low > other_low) return true;
    if (low < other_low) return false;
    const auto a = std::int32_t(std::int16_t(word >> 16)), b = std::int32_t(c.i16(date_at));
    const auto x = std::uint32_t(a ^ b);
    const auto t = std::uint32_t(std::int32_t(x) >> 1) - (x & std::uint32_t(a));
    return (t >> 31) != 0;
}

Pass::Pass(Cpu& cpu, Frame& f, std::uint32_t manager_object, std::uint32_t date_at)
    : c(cpu), frame(f), fpu(cpu), manager(manager_object), date(date_at), pool(cpu.toc(toc::doubles)) {
    // What the prologue loads: the cells and tables the passes read, and the sample table's
    // cells (r24-r27, as the original keeps them).
    c.set({{29, manager}, {30, date}, {13, c.toc(-0x7b64)}, {18, c.toc(-0x7a0c)}, {19, c.toc(-0x7568)},
           {20, c.toc(-0x7a60)}, {21, c.toc(-0x7548)}, {22, c.toc(toc::club_count)}, {23, c.toc(-0x7b7c)},
           {24, c.toc(sample_table::base)}, {25, c.toc(sample_table::first)}, {26, c.toc(sample_table::last)},
           {27, c.toc(sample_table::cursor)}, {28, pool}, {14, c.toc(-0x7b5c)}, {31, c.toc(toc::pool)}});
}

bool Pass::before_first_day() const { return first_day_after(c, date); }

// The day the game fixes by date: 0x107156f0 builds it in the frame (at +0x5c, and returns that
// address) and the pass compares its first word with today's. The callee writes the word at +0x5c;
// the second word, at frame offset 0x60 (the unit's one stale stack read, 0x100fa7ac), is left from
// earlier calls. The original copies it to a slot it never reads (0x48), so its value reaches nothing.
bool Pass::is_fixed_day() {
    const auto made = invoke(c, season_day_maker, 0x100fa7ac, {frame.at(0x5c), 0x10, 5, 0x7d4, 0xffffffff});
    const auto word = c.u32(made);
    [[maybe_unused]] const auto stale_word = c.u32(frame.at(0x60));
    return word == c.u32(date);
}

void Pass::run() {
    if (before_first_day()) return;
    if (is_fixed_day()) {
        // Both clubs must be named (-1: none).
        if (c.u32(c.toc(first_club_cell)) != 0xffffffffu && c.u32(c.toc(special_club_cell)) != 0xffffffffu)
            move_clubs_to_rows();
    }
    settle_balances();
    // The last day of a month: tomorrow falls in another month.
    invoke(c, next_day, 0x100fb004, {frame.at(0x54), date, 1});
    Held<15> tomorrow(c, std::uint32_t(std::int32_t(std::int8_t(invoke(c, month_of, 0x100fb00c, {frame.at(0x54)})))));
    const auto today = std::int32_t(std::int8_t(invoke(c, month_of, 0x100fb018, {date})));
    if (today == tomorrow.i32()) ordinary_day();
    else month_end();
}

void Pass::raise_attendance(std::uint32_t club_at, std::int32_t gain, std::int32_t floor) {
    c.put32(club_at + club::attendance, c.u32(club_at + club::attendance) + std::uint32_t(gain));
    if (c.i32(club_at + club::attendance) < floor) c.put32(club_at + club::attendance, std::uint32_t(floor));
}

// The sequence of a list walk the two club moves share: start the iterator, find the entries of the
// date's window (the cell names the club), and for each entry of the moved club replace its link.
template <unsigned ClubReg, unsigned EntryReg>
void Pass::follow_list_entries(std::uint32_t iterator, std::uint32_t cell, std::uint32_t back_init, std::uint32_t back_find,
                               std::uint32_t back_next, std::uint32_t back_remove, std::uint32_t back_insert,
                               std::uint32_t back_done) {
    const auto it = frame.at(std::int32_t(iterator));
    invoke(c, list_start, back_init, {it});
    c.put32(frame.at(0x38), 0xfffffffe);  // the stack arguments of 0x1048b180
    c.put32(frame.at(0x3c), 0xffffffff);
    const std::uint32_t low = c.u16(date + 2);
    invoke(c, list_find, back_find, {c.u32(c.r(20)), c.u16(date), low, 0xc8, (low + 1) & 0xffff,
                                     c.u32(c.toc(std::int32_t(cell))), it, 0xffffffff});
    c.put32(frame.at(std::int32_t(iterator) + 0xc), c.u32(frame.at(std::int32_t(iterator) + 4)));
    Held<EntryReg> entry(c);
    for (;;) {
        entry = invoke(c, list_next, back_next, {it});
        if (std::uint32_t(entry) == 0) break;
        const auto club_at = c.r(ClubReg);
        if (c.u32(entry + 0x1c) == club_at) {
            invoke(c, list_remove, back_remove, {c.u32(c.r(20)), entry});
            c.put32(entry + 0x18, c.u32(club_at + club::ground));
            c.put32(entry + 4, c.u32(c.u32(club_at + club::ground)));
            invoke(c, list_insert, back_insert, {c.u32(c.r(20)), entry, 1});
        }
    }
    invoke(c, list_end, back_done, {it, 0xffffffff});
}

// On the fixed day the two named clubs take the table's rows whose names begin with the pool's
// first and second text. The first club only moves when its row differs; the second also turns the
// row it leaves negative. Each move resets the club's attendance by tier and sets its fixed costs,
// has 0x101030c0 place the club on the row, and links the list entries to the new row.
void Pass::move_clubs_to_rows() {
    const auto clubs = c.u32(c.toc(toc::clubs));
    {
        Held<16> club_h(c, clubs + c.u32(c.toc(first_club_cell)) * club_size);
        Held<15> row(c, 0);
        Held<17> offset(c, 0);
        while (row.i32() < c.i32(c.r(19))) {
            if (invoke(c, compare_text, 0x100fa844, {c.u32(c.r(21)) + offset + row_name, c.r(31) + first_name_at, 6}) == 0) {
                const std::uint32_t club_at = club_h;
                if (c.i32(c.u32(club_at + club::ground)) == row.i32()) break;
                c.put8(club_at + club::on_row, 1);
                const auto row_at = c.u32(c.r(21)) + row * row_size;
                c.put32(club_at + club::ground, row_at);
                const auto rep = reputation(club_at);
                if (rep > 0xbb8) raise_attendance(club_at, 0xabe, 0x30d4);
                else if (rep > 0x5dc) raise_attendance(club_at, 0x6d6, 0x1d4c);
                else raise_attendance(club_at, 0x4e2, 0x1676);
                c.put32(club_at + club::ticket_0c, 0xea6);
                c.put32(club_at + club::ticket_10, 0x7530);
                invoke_unit(c, place_club, 0x100fa914, {manager, club_at, c.u32(c.r(21)) + row * row_size});
                follow_list_entries<16, 15>(0x74, std::uint32_t(first_club_cell), 0x100fa91c, 0x100fa95c, 0x100fa9b8, 0x100fa98c,
                                            0x100fa9b0, 0x100fa9cc);
                break;
            }
            offset = offset + row_size;
            row = row + 1;
        }
    }
    Held<16> row(c, 0);
    Held<17> offset(c, 0);
    Held<15> club_h(c, clubs + c.u32(c.toc(special_club_cell)) * club_size);
    while (row.i32() < c.i32(c.r(19))) {
        const auto at = c.u32(c.r(21)) + offset;
        // Only rows set aside (money below -10000) with the second text are taken.
        if (c.i32(at + row_money) < -0x2710 &&
            invoke(c, compare_text, 0x100faa30, {at + row_name, c.r(31) + second_name_at, 8}) == 0) {
            const std::uint32_t club_at = club_h;
            if (c.i32(c.u32(club_at + club::ground)) == row.i32()) return;
            c.put8(club_at + club::on_row, 1);
            const auto row_at = row * row_size;
            const auto old_row = c.u32(club_at + club::ground);
            c.put32(old_row + row_money, 0u - c.u32(old_row + row_money));
            c.put32(club_at + club::ground, c.u32(c.r(21)) + row_at);
            const auto new_row = c.u32(club_at + club::ground);
            if (c.i32(new_row + row_money) < 0) c.put32(new_row + row_money, 0u - c.u32(new_row + row_money));
            const auto rep = reputation(club_at);
            if (rep > 0x1a5e) raise_attendance(club_at, 0x1d4c, 0x88b8);
            else if (rep > 0x1482) raise_attendance(club_at, 0x1388, 0x57e4);
            else raise_attendance(club_at, 0xabe, 0x4556);
            c.put32(club_at + club::ticket_0c, 0x109a);
            c.put32(club_at + club::ticket_10, 0xd6d8);
            invoke_unit(c, place_club, 0x100fab30, {manager, club_at, c.u32(c.r(21)) + row_at});
            follow_list_entries<15, 16>(0x64, std::uint32_t(second_club_cell), 0x100fab38, 0x100fab78, 0x100fabd0, 0x100faba4,
                                        0x100fabc8, 0x100fabe4);
            return;
        }
        offset = offset + row_size;
        row = row + 1;
    }
}

// An account outside the range its club's kind allows is pulled back: amateurs to within 10000,
// nations' second tier within 25000 (250000 above reputation 2500), the rest within 2250000, where
// one time in five the correction overshoots to the other side by 175000.
void Pass::confine_balance(std::uint32_t account, std::int64_t low, std::int64_t high) {
    const auto balance = std::int64_t(c.u64(account + account::balance));
    if (balance < low) c.put64(account + account::balance, std::uint64_t(low));
    else if (high < balance) c.put64(account + account::balance, std::uint64_t(high));
}

void Pass::drift_balance(std::uint32_t account) {
    const auto balance = std::int64_t(c.u64(account + account::balance));
    if (balance < -2'250'000) {
        const auto sample = std::int32_t(draw_sample(0x100fae20));
        c.put64(account + account::balance, std::uint64_t(sample % 5 == 0 ? 175'000 : -2'250'000));
    } else if (2'250'000 < balance) {
        const auto sample = std::int32_t(draw_sample(0x100faed0));
        c.put64(account + account::balance, std::uint64_t(sample % 5 == 0 ? -175'000 : 2'250'000));
    }
}

// On a Saturday a club of the first kind pays the amount its account has pending, and its pending
// deal (0x100f9d80) is settled or cleared.
void Pass::saturday_charge(std::uint32_t account, std::uint32_t club_at) {
    // `extsh`, then compare with 6: the halfword, not the byte.
    if (std::int16_t(invoke(c, weekday_of, 0x100faf44, {date})) != saturday) return;
    if (c.u32(club_at + club::nation) == 0) return;
    invoke_unit(c, refresh_account, 0x100faf64, {account});
    book_expense(account, ledger::expense_90, c.u32(account + account_pending));
    invoke_unit(c, charge_clubs, 0x100fafa8, {manager, club_at});
    if (invoke_unit(c, pending_deal, 0x100fafb8, {manager, club_at, frame.at(0x41)}) != 0) {
        invoke_unit(c, settle_deal, 0x100fafcc, {account, c.u8(frame.at(0x41))});
    } else {
        c.put32(account + account_series, 0);
        c.put8(account + account_flag, 0);
    }
}

void Pass::settle_balances() {
    Held<17> index(c, 0);
    Held<15> account(c, c.u32(manager + manager::accounts));
    Held<16> club_h(c, c.u32(c.toc(toc::clubs)));
    while (index.i32() < c.i32(c.r(22))) {
        const std::uint32_t club_at = club_h, account_at = account;
        if (c.u32(club_at + club::nation) != 0) {
            if (inactive(club_at, 0x100fac28)) return;  // the sweep ends; the rest of the pass goes on
            const auto division = c.u32(club_at + club::division);
            bool confine = true;
            if (division != 0) {
                const auto flag = c.i8(c.u32(club_at + club::nation) + 0x120);
                confine = flag != 0 ? c.u32(division) == c.u32(c.r(18)) : reputation(club_at) < 5000;
            }
            if (confine) {
                const auto status = c.i8(club_at + club::status);
                if (status == 3) confine_balance(account_at, -10'000, 10'000);
                else if (status == 2) {
                    if (reputation(club_at) > 0x9c4) confine_balance(account_at, -250'000, 250'000);
                    else confine_balance(account_at, -25'000, 25'000);
                } else drift_balance(account_at);
            } else {
                saturday_charge(account_at, club_at);
            }
        }
        index = index + 1;
        account = account + account::size;
        club_h = club_h + club_size;
    }
}

void daily_pass_entry(Cpu& c, std::uint32_t manager, std::uint32_t date) {
    Frame frame(c, unscheduled_check, 0x120, {.first_gpr = 13, .gpr_offset = -0x5c, .first_fpr = 31});
    Pass pass(c, frame, manager, date);
    pass.run();
}
}

void daily_pass(Cpu& c, std::uint32_t manager, std::uint32_t date) { daily::daily_pass_entry(c, manager, date); }
}
