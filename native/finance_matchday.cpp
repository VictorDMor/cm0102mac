// FINANCE.CPP, block matchday (finance_matchday.hpp): a fixture's income and costs, and a club's ground works.
// Native-first: values live in native locals; registers are set only at the calls out of the routine, to what the
// original holds there (callees save r13-r31 in their frames and later code reads those bytes as stale stack).
#include "finance_matchday.hpp"
#include "finance_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>
#include <stdexcept>

namespace cm::aot::finance {
using guest::Fpu;
using guest::Frame;

namespace {
using Word = std::uint32_t;
constexpr Word u(std::int32_t x) { return Word(x); }
// A halfword as the original keeps it in a register (extsh).
constexpr Word sx(std::int16_t x) { return Word(std::int32_t(x)); }
constexpr Word sx8(std::int8_t x) { return Word(std::int32_t(x)); }
// 32-bit wrapping arithmetic on signed values.
constexpr std::int32_t add(std::int32_t a, std::int32_t b) { return std::int32_t(u(a) + u(b)); }
constexpr std::int32_t sub(std::int32_t a, std::int32_t b) { return std::int32_t(u(a) - u(b)); }
constexpr std::int32_t mul(std::int32_t a, std::int32_t b) { return std::int32_t(u(a) * u(b)); }
// `divw` as the generated code does it: it stops on a zero divisor (a draw whose sample is negative can reach one).
std::int32_t divw(std::int32_t a, std::int32_t b) {
    if (b == 0) throw std::runtime_error("Signed division by zero in translated code");
    return std::int32_t(std::int64_t(a) / b);
}

// Record offsets (names inferred from use).
namespace fixture {
constexpr Word competition = 0x14, home = 0x1c, away = 0x20,
               round_kind = 0x3a,   // byte: 4 and 6 are rounds of a cup, 7 and 8 finals (by use)
               broadcast_band = 0x3b,  // byte: -1..2
               cup_tie = 0x3c,      // byte: not a league fixture
               guaranteed = 0x40;   // byte
}
namespace match_record { constexpr Word crowd = 0x34, carried = 0x38; }
namespace crec {
constexpr Word index = 0, worth = 8, division = 0x2c, league = 0x30, board = 0x3c, owner = 0x80,
               reputation = 0x1ec, national = 0x23d, ground_works = 0x23e;
}
namespace board_record { constexpr Word seats = 4, roofed = 8, peak = 0xc; }
namespace division_record { constexpr Word level = 0x11c, group = 0x68; }
namespace owner_record { constexpr Word profile = 0x1c; }
namespace profile_record { constexpr Word patience = 0x40; }
namespace league_record { constexpr Word round = 0x18; }
namespace acct {
constexpr Word seats_sold = 0x18, ticket_price_scale = 0x160, gate_level = 0x164,
               stadium_costs = 0x8c, owner_funding = 0xb4, gate_income = 0xac, extras_income = 0xbc,
               match_income = 0xa0, broadcast_income = 0xb8;
}
// TOC words: cells holding a well-known club or group, compared with a record's word.
namespace cell {
constexpr std::int32_t rows = -0x79f0, level_floats = -0x55c0, floats = -0x559c, pace_test = -0x7b54,
                       club_a = -0x748c, club_b = -0x7490, club_c = -0x7494, club_d = -0x7498, club_e = -0x749c,
                       league_group = -0x7488, other_group = -0x7af4,
                       group_a = -0x757c, group_b = -0x7574, group_c = -0x7578;
}
constexpr Word callee_pace_test = 0x10494d20, callee_board_news = 0x10103380,
               callee_account = 0x10103510;

// The double pool (toc::doubles) with the original's int-to-double and fctiwz.
struct Fp {
    Cpu& c;
    Fpu fpu;
    Word pool;
    explicit Fp(Cpu& cpu) : c(cpu), fpu(cpu), pool(cpu.toc(toc::doubles)) {}
    double k(Word offset) const { return c.f64(pool + offset); }
    static double word(std::int32_t x) {
        return std::bit_cast<double>((std::uint64_t(0x43300000) << 32) | (u(x) ^ 0x80000000u));
    }
    double to_double(std::int32_t x) { return fpu.fsub(word(x), k(doubles::int_magic)); }
    double to_single(std::int32_t x) { return fpu.fsubs(word(x), k(doubles::int_magic)); }
    std::int32_t truncate(double value, unsigned reg = 0) { return std::int32_t(std::uint32_t(fpu.fctiwz(reg, value))); }
    // fctiwz of (pool constant at `offset`) * (integer): the common "scale a money figure" step.
    std::int32_t scaled(Word offset, std::int32_t x) {
        const auto d = to_double(x);
        const auto m = fpu.fmul(k(offset), d);
        return truncate(m);
    }
};

struct SampleCells { Word base, first, last, cursor; };
SampleCells sample_cells(Cpu& c) {
    return {c.toc(sample_table::base), c.toc(sample_table::first), c.toc(sample_table::last), c.toc(sample_table::cursor)};
}

// The draw below a nonzero `limit` within +-0xffff: the sample's remainder, with the limit's sign. The original
// also has the branch for a larger limit (the scaled place in the period plus a tenth of the draw below a tenth of
// the limit, through 0x100f2100); here no limit reaches it: 0x100f7cf0 draws below a signed byte (the owner's
// patience, branch at 0x100f7fb0..0x100f8084), 0x100f7270 below a halfword (the reputation, 0x100f79e8..0x100f7ab8
// and 0x100f7b44..0x100f7c14) and 0x100f8780 below a quarter of a halfword (0x100f8d14..0x100f8de4).
// `redraw_back` is the address after the original's `bl 0x1077b770`; the caller sets the registers for it.
std::int32_t draw_below(Cpu& c, std::int32_t limit, Word redraw_back) {
    if (limit < -0xffff || limit > 0xffff) throw std::logic_error("finance matchday: a draw below a limit past 0xffff");
    const auto sample = std::int32_t(draw(c, redraw_back));
    const auto rest = sub(sample, mul(divw(sample, limit), limit));
    return limit > 0 ? rest : sub(0, rest);
}

// Money into a ledger line (both copies) and out of / into the balance.
void book_line(Cpu& c, Word account, Word line, Word amount) {
    c.put32(account + line, c.u32(account + line) + amount);
    c.put32(account + line + account::second_copy, c.u32(account + line + account::second_copy) + amount);
}
void move_balance(Cpu& c, Word account, std::uint64_t delta) { c.put64(account + account::balance, c.u64(account + account::balance) + delta); }
std::uint64_t widen(std::int32_t amount) { return std::uint64_t(std::int64_t(amount)); }

// The competition level's cut of an amount: level 2 a quarter, 1 whole, any other a tenth.
std::int32_t by_level(std::int8_t level, std::int32_t x) {
    if (level == 2) return x / 4;
    if (level == 1) return x;
    return x / 10;
}
std::int8_t level_of(Cpu& c, Word club_record) {
    const auto division = c.u32(club_record + crec::division);
    return division != 0 ? c.i8(division + division_record::level) : std::int8_t(3);
}
// The pool factor chosen by which well-known club the record is (the cells hold pointers to the clubs' words).
bool is_cell_club(Cpu& c, std::int32_t slot, std::int32_t value) { return value == c.i32(c.toc(slot)); }
}

// ---- 0x100f95e0 ----
std::int32_t fixture_income(Cpu& c, std::uint32_t manager, std::uint32_t fixture_record, std::int32_t base) {
    Frame frame(c, 0x100f95e0, 0xa0, {.first_gpr = 25});
    Fp fp(c);
    c.set({{31, fp.pool}, {30, 0}, {29, u(base)}, {28, fixture_record}, {27, c.toc(toc::pool)}, {26, c.toc(toc::message)},
           {25, manager}});
    if (fixture_record == 0) {
        report_check(c, frame, {0x100f9624, 0x40, 0x38, 0x1063});
        return 0;
    }
    if (base == 0) return 0;
    const auto home_of = [&] { return c.u32(fixture_record + fixture::home); };
    Word account = 0;
    if (home_of() != 0) {
        if (std::int8_t(invoke(c, callee::club_is_inactive, 0x100f968c, {home_of()})) != 0) return 0;
        account = invoke_unit(c, callee_account, 0x100f96a8, {manager, home_of()});
        c.set(30, account);
    }
    std::int32_t amount;
    bool adjust = true;
    const auto competition = c.u32(fixture_record + fixture::competition);
    // The reputation step (both its sites): amount * (reputation / 500) * the pool factor.
    const auto by_reputation = [&] {
        const auto reputation = c.i16(home_of() + crec::reputation);
        const auto per = std::int32_t(reputation) / 500;
        const auto amount_double = fp.to_double(base);
        const auto per_double = fp.to_double(per);
        auto scaled = fp.fpu.fmul(fp.k(0x38), per_double);
        scaled = fp.fpu.fmul(amount_double, scaled);
        return fp.truncate(scaled);
    };
    if (c.i8(fixture_record + fixture::cup_tie) != 0 && competition != 0) {
        amount = mul(base, add(c.i16(competition + league_record::round), 8));
        adjust = false;
    } else if (competition == 0) {
        amount = by_reputation();
    } else {
        const auto paced = std::int8_t(invoke(c, callee_pace_test, 0x100f9758, {c.u32(c.toc(cell::pace_test)), competition}));
        if (paced != 0) {
            amount = by_reputation();
        } else {
            const auto home = home_of();
            const bool national = c.u8(home + crec::national) == 1 ||
                                  c.u8(c.u32(fixture_record + fixture::away) + crec::national) == 1;
            const auto per = std::int32_t(c.i16(home + crec::reputation)) / 500;
            amount = mul(base, add(per, national ? 4 : 3));
        }
    }
    // The club's own league: scaled by its price level.
    if (adjust && account != 0 && c.u32(home_of() + crec::league) == c.u32(fixture_record + fixture::competition)) {
        const auto level_float = double(c.f32(account + acct::ticket_price_scale));
        const auto first = fp.to_single(amount);
        const auto second = fp.to_single(amount);
        const auto adjusted = fp.fpu.multiply_add(0x100f9884, first, level_float, second);
        amount = fp.truncate(adjusted);
    }
    const auto club_record = home_of();
    const auto division = c.u32(club_record + crec::division);
    amount = by_level(level_of(c, club_record), amount);
    const auto league = c.u32(club_record + crec::league);
    const auto times = [&](Word factor) { amount = fp.scaled(factor, amount); };
    if (league != 0 && c.i32(league) == c.i32(c.toc(cell::league_group))) {
        times(0xa0);
    } else {
        const auto index = c.i32(club_record + crec::index);
        if (is_cell_club(c, cell::club_a, index)) times(0x0);
        else if (is_cell_club(c, cell::club_b, index)) times(0x58);
        else if (is_cell_club(c, cell::club_c, index)) times(0x70);
        else if (is_cell_club(c, cell::club_d, index)) times(0xd8);
        else if (is_cell_club(c, cell::club_e, index)) times(0x58);
    }
    if (division != 0 && c.i32(division + division_record::group) == c.i32(c.toc(cell::other_group))) times(0xe8);
    return amount;
}

// ---- 0x100f7cf0 ----
bool expand_ground(Cpu& c, std::uint32_t account, std::int32_t wanted, std::int32_t planned, std::uint32_t news,
                   std::uint32_t with_owner) {
    Frame frame(c, 0x100f7cf0, 0x90, {.first_gpr = 16});
    Fp fp(c);
    const auto cells = sample_cells(c);
    const auto clubs_cell = c.toc(toc::clubs), finances_cell = c.toc(toc::finances);
    c.set({{16, cells.base}, {17, cells.first}, {18, cells.last}, {19, cells.cursor}, {20, account}, {21, u(wanted)},
           {22, u(planned)}, {23, news}, {24, with_owner}, {25, 0}, {30, finances_cell}, {31, clubs_cell}});
    const auto club_record = [&] { return c.u32(clubs_cell) + c.u32(account + account::club) * club_size; };
    if (std::int8_t(invoke(c, callee::club_is_inactive, 0x100f7d44, {club_record()})) != 0) return false;
    const auto board = c.u32(club_record() + crec::board);
    if (board == 0) return false;
    const auto roofed_now = c.i32(board + board_record::roofed);
    auto added_first = sub(wanted, roofed_now);
    if (added_first < 0) added_first = 0;
    std::int32_t added_second;
    if (wanted == planned) {
        added_second = 0;
        const auto seats = c.i32(board + board_record::seats);
        if (seats > wanted && sub(seats, roofed_now) > 0) added_first = add(added_first, sub(seats, roofed_now));
    } else {
        added_second = sub(sub(planned, c.i32(board + board_record::seats)), added_first);
        if (added_second < 0) added_second = 0;
    }
    c.set({{26, u(added_second)}, {27, u(added_first)}});
    if (added_first == 0 && added_second == 0) return true;
    std::int16_t roofed = 0;
    if (added_first > roofed_now) {
        const auto span = sub(c.i32(board + board_record::seats), roofed_now);
        if (span > 0) {
            const auto room = sub(added_first, roofed_now);
            roofed = std::int16_t(room < span ? room : span);
        }
    }
    c.set(25, sx(roofed));

    struct Backs { Word split, one_kind; };  // addresses after the original's two calls to 0x10103380
    const auto settle = [&](Word cost, std::uint64_t cost_wide, Backs backs) {
        book_line(c, account, acct::stadium_costs, cost);
        move_balance(c, account, 0 - cost_wide);
        // 0x10103380 is told whether the new seats are all of one kind.
        const bool one_kind = roofed == 0 || added_second != 0;
        invoke_unit(c, callee_board_news, one_kind ? backs.one_kind : backs.split,
                    {c.u32(finances_cell), c.u32(account + account::club), news, one_kind ? 1u : 0u});
        if (wanted == planned) {
            const auto after = c.u32(club_record() + crec::board);
            c.put32(after + board_record::roofed, c.u32(after + board_record::seats));
        }
        return true;
    };
    const auto work_on_board = [&](std::int16_t first_part, std::int16_t second_part) {
        auto at = c.u32(club_record() + crec::board);
        c.put32(at + board_record::seats, c.u32(at + board_record::seats) + sx(first_part));
        at = c.u32(club_record() + crec::board);
        c.put32(at + board_record::roofed, c.u32(at + board_record::roofed) + sx(second_part));
    };
    const auto cost_of = [&] {
        const auto thousands = added_first / 1000;
        return u(add(add(add(mul(added_second, 0x32), mul(roofed, 0x4b)), mul(add(thousands, 1), 750000)),
                     mul(added_first, 0x7d)));
    };
    const auto raise_peak = [&] {
        const auto top = add(add(c.i32(board + board_record::seats), added_second), added_first);
        if (c.i32(board + board_record::peak) < top) c.put32(board + board_record::peak, u(top));
    };

    if (std::int8_t(with_owner) == 0) {
        // Built outright: no owner, no limit, the cost never negative.
        raise_peak();
        work_on_board(std::int16_t(add(added_first, added_second)), std::int16_t(add(added_first, roofed)));
        const auto cost = cost_of();
        return settle(cost, cost, {0x100f8480, 0x100f8498});
    }

    // With the owner: the ground may not pass 120,000 seats.
    if (c.i32(board + board_record::seats) > 0x1d4c0) return false;
    raise_peak();
    const auto cost = cost_of();
    const auto cost_wide = widen(std::int32_t(cost));
    c.set({{24, cost}, {29, sx(roofed)}});
    const auto owner = c.u32(club_record() + crec::owner);
    const auto profile = owner != 0 ? c.u32(owner + owner_record::profile) : 0;
    c.set(28, Word(std::int32_t(cost) >> 31));
    if (owner != 0 && profile != 0) {
        const auto cannot_afford = std::int64_t(cost_wide) >= balance_of(c, account);
        if (cannot_afford) {
            // The owner's patience: a cost under patience * 500,000 goes ahead, otherwise patience grows.
            const auto patience = c.i8(profile + profile_record::patience);
            if (std::int32_t(cost) >= mul(patience, 500000)) {
                if (patience < 0x14) c.put8(profile + profile_record::patience, c.u8(profile + profile_record::patience) + 1);
                return false;
            }
        }
        work_on_board(std::int16_t(add(added_first, added_second)), std::int16_t(add(added_first, roofed)));
        const auto patience = c.i8(c.u32(c.u32(club_record() + crec::owner) + owner_record::profile) + profile_record::patience);
        c.set(27, sx8(patience));
        std::int32_t drawn = 0;
        if (patience != 0) {
            drawn = draw_below(c, patience, 0x100f7f88);
        }
        c.set(16, u(drawn));
        if (drawn == 0 && std::int64_t(cost_wide) > balance_of(c, account)) {
            const auto limit = c.i8(c.u32(c.u32(club_record() + crec::owner) + owner_record::profile) + profile_record::patience);
            if (std::int32_t(cost) < mul(limit, 500000)) {
                book_line(c, account, acct::owner_funding, cost);
                move_balance(c, account, cost_wide);
            }
        }
        return settle(cost, cost_wide, {0x100f8198, 0x100f81b0});
    }
    // No owner to ask: a cost the club cannot cover is a gamble when it is a million or more.
    if (std::int64_t(cost_wide) >= balance_of(c, account) && std::int32_t(cost) >= 1000000) {
        const auto sample = std::int32_t(draw(c, 0x100f824c));
        if (sample % 5 != 0) return false;
    }
    work_on_board(std::int16_t(add(added_first, added_second)), std::int16_t(add(added_first, roofed)));
    return settle(cost, cost_wide, {0x100f832c, 0x100f8344});
}

// ---- 0x100f7270 ----
void matchday_takings(Cpu& c, std::uint32_t account, std::uint32_t credit_gate) {
    Frame frame(c, 0x100f7270, 0xc0, {.first_gpr = 22});
    Fp fp(c);
    const auto cells = sample_cells(c);
    const auto clubs_cell = c.toc(toc::clubs);
    c.set({{31, account}, {26, cells.base}, {25, credit_gate}, {27, cells.first}, {28, cells.last}, {29, cells.cursor},
           {30, fp.pool}});
    const auto club_record = c.u32(clubs_cell) + c.u32(account + 8) * club_size;
    const auto reputation = [&] { return std::int32_t(c.i16(club_record + crec::reputation)); };
    c.set(22, club_record);
    // A board that has not yet worked out its capacity gets one from the club's reputation (at least 1500 seats).
    if (club_record != 0) {
        if (const auto board = c.u32(club_record + crec::board); board != 0 && c.i32(board + board_record::seats) < 0x5dc) {
            const auto stands = fp.scaled(0xa8, reputation());
            c.put32(board + board_record::seats, u(stands));
            const auto again = c.u32(club_record + crec::board);
            if (c.i32(again + board_record::seats) < 0x5dc) c.put32(again + board_record::seats, 0x5dc);
        }
    }
    // The crowd the club draws: its row's figure and its worth, each against a floor, then against the seats.
    const auto rows = c.u32(c.u32(c.toc(cell::rows)));
    const std::int32_t row_figure = club_record != 0 ? c.i16(rows + c.u32(club_record + crec::index) * 0x4c + 8) : 0;
    auto figure = fp.to_double(row_figure);
    auto drawn = fp.fpu.fmul(fp.k(0x1a8), figure);
    const auto worth = fp.to_double(c.i32(club_record + crec::worth));
    drawn = fp.fpu.frsp(drawn);
    fp.fpu.fcmp(0, drawn, fp.k(0x138));
    auto floor_a = fp.k(0x138);
    if (drawn > floor_a) floor_a = drawn;
    const auto from_worth = fp.fpu.fmul(worth, floor_a);
    const auto crowd_a = fp.truncate(from_worth, 4);
    const auto reputation_double = fp.to_double(reputation());
    auto by_reputation = fp.fpu.fmul(fp.k(0x220), reputation_double);
    by_reputation = fp.fpu.frsp(by_reputation);
    auto floor_b = fp.k(0x38);
    fp.fpu.fcmp(0, by_reputation, floor_b);
    const auto crowd_double = fp.to_double(crowd_a);
    if (by_reputation > floor_b) floor_b = by_reputation;
    const auto from_reputation = fp.fpu.fmul(crowd_double, floor_b);
    const auto crowd_b = fp.truncate(from_reputation);
    // What the ground holds against that crowd.
    const auto board = c.u32(club_record + crec::board);
    double seats_limit;
    if (board != 0) {
        const auto seats_double = fp.to_double(c.i32(board + board_record::seats));
        seats_limit = fp.fpu.fmul(fp.k(0x228), seats_double);
    } else {
        seats_limit = fp.k(0x160);
    }
    const auto crowd_b_double = fp.to_double(crowd_b);
    fp.fpu.fcmp(0, crowd_b_double, seats_limit);
    std::int32_t crowd;
    if (crowd_b_double < seats_limit) {
        const auto again = fp.to_double(crowd_b);
        crowd = fp.truncate(again);
    } else if (board != 0) {
        const auto seats_double = fp.to_double(c.i32(board + board_record::seats));
        crowd = fp.truncate(fp.fpu.fmul(fp.k(0x228), seats_double));
    } else {
        crowd = fp.truncate(fp.k(0x160));
    }
    if (crowd == 0 || c.i32(account + acct::seats_sold) >= crowd) return;

    // More tickets sold: the gate level climbs (at most 8) and the sale is a share of the gap to the crowd.
    c.put8(account + acct::gate_level, c.u8(account + acct::gate_level) + 1);
    if (const auto level = c.i8(account + acct::gate_level); level < 0 || level > 8) c.put8(account + acct::gate_level, 8);
    const auto sold = c.i32(account + acct::seats_sold);
    const auto gap = sub(crowd, sold);
    const auto level_table = c.toc(cell::level_floats);
    const auto level_scale = double(c.f32(level_table + u(std::int32_t(c.i8(account + acct::gate_level))) * 4));
    const auto gap_single = fp.to_single(gap);
    const auto sale = fp.truncate(fp.fpu.fmuls(gap_single, level_scale));
    c.set(23, u(sale));
    const auto sample = std::int32_t(draw(c, 0x100f7564));
    const auto extra = mul(sale, add(sub(sample, mul(sample / 11, 11)), 0x5f)) / 100;
    c.put32(account + acct::seats_sold, c.u32(account + acct::seats_sold) + u(extra));
    c.set(24, u(extra));

    // The ticket price from the reputation.
    std::int32_t price;
    {
        const auto league = c.u32(club_record + crec::league);
        const auto reputation_double = fp.to_double(reputation());
        const auto pooled = fp.fpu.multiply_add(league != 0 ? 0x100f7600 : 0x100f7644, fp.k(0x238), reputation_double, fp.k(0x230));
        price = fp.truncate(fp.fpu.fmul(fp.k(0x28), pooled));
    }
    const auto board_now = c.u32(club_record + crec::board);
    auto gross = mul(price, extra);
    double per_seat;
    // The seats test is always true when the board exists: 0x100f72c8..0x100f7318 leave a board's seats at 0x5dc or more
    // (a smaller figure is replaced by 2.5 * reputation and then raised to 0x5dc), and nothing here changes them.
    if (board_now != 0 && c.i32(board_now + board_record::seats) != 0) {
        const auto seats_double = fp.to_double(c.i32(board_now + board_record::seats));
        per_seat = fp.fpu.fmul(fp.k(0x228), seats_double);
    } else {
        per_seat = fp.k(0x160);
    }
    {
        const auto sold_double = fp.to_double(c.i32(account + acct::seats_sold));
        auto filled = fp.fpu.fdiv(sold_double, per_seat);
        filled = fp.fpu.frsp(filled);
        c.put_f32(account + acct::ticket_price_scale, float(filled));
        const auto stored = double(c.f32(account + acct::ticket_price_scale));
        const auto full = fp.k(0x228);
        fp.fpu.fcmp(0, stored, full);
        if (stored > full) c.put_f32(account + acct::ticket_price_scale, c.f32(c.toc(cell::floats) + 0x10));
    }
    gross = by_level(level_of(c, club_record), gross);
    const auto league = c.u32(club_record + crec::league);
    const auto times = [&](Word factor) { gross = fp.scaled(factor, gross); };
    if (league != 0 && c.i32(league) == c.i32(c.toc(cell::league_group))) {
        times(0xa0);
    } else {
        const auto index = c.i32(club_record + crec::index);
        if (is_cell_club(c, cell::club_a, index)) times(0x0);
        else if (is_cell_club(c, cell::club_b, index)) times(0x58);
        else if (is_cell_club(c, cell::club_c, index)) times(0x70);
        else if (is_cell_club(c, cell::club_d, index)) times(0xd8);
        else if (is_cell_club(c, cell::club_e, index)) times(0x58);
        else if (const auto division = c.u32(club_record + crec::division);
                 division != 0 && c.i32(division + division_record::group) == c.i32(c.toc(cell::other_group))) {
            times(0xe8);
        }
    }
    if (std::int8_t(credit_gate) != 0) {
        book_line(c, account, acct::gate_income, u(gross));
        move_balance(c, account, widen(gross));
    }

    // The extras: two draws below the reputation, then a figure scaled by the reputation.
    const auto draw_reputation = [&](Word redraw_back) {
        const auto limit = std::int32_t(c.i16(club_record + crec::reputation));
        c.set(25, u(limit));
        return limit == 0 ? std::int32_t(0) : draw_below(c, limit, redraw_back);
    };
    const auto first_draw = draw_reputation(0x100f79c0);
    c.set(23, u(first_draw));
    const auto second_draw = draw_reputation(0x100f7b1c);
    c.set(22, u(second_draw));
    const auto draws_double = fp.to_single(add(first_draw, add(second_draw, 1)));
    const auto multiplied = fp.fpu.multiply_add(0x100f7c80, fp.k(0x180), draws_double, fp.k(0x18));
    const auto extra_single = fp.to_single(extra);
    const auto rounded = fp.fpu.frsp(multiplied);
    const auto takings = fp.truncate(fp.fpu.fmuls(extra_single, rounded));
    book_line(c, account, acct::extras_income, u(takings));
    move_balance(c, account, widen(takings));
}

// ---- 0x100f8780 ----
void split_fixture_income(Cpu& c, std::uint32_t manager, std::uint32_t fixture_record, std::uint32_t record) {
    Frame frame(c, 0x100f8780, 0x110, {.first_gpr = 17});
    Fp fp(c);
    const auto cells = sample_cells(c);
    c.set({{17, record}, {18, manager}, {19, c.toc(toc::success)}, {20, c.toc(toc::pool)}, {21, c.toc(toc::message)},
           {22, 0}, {24, 0}, {25, 0}, {26, cells.base}, {27, cells.first}, {28, cells.last}, {29, cells.cursor},
           {30, fp.pool}, {31, fixture_record}});
    if (record == 0) {
        report_check(c, frame, {0x100f87e0, 0x68, 0x40, 0xf55});
        return;
    }
    if (fixture_record == 0) {
        report_check(c, frame, {0x100f883c, 0x48, 0x38, 0xf5a});
        return;
    }
    const auto home_of = [&] { return c.u32(fixture_record + fixture::home); };
    const auto away_of = [&] { return c.u32(fixture_record + fixture::away); };
    Word home_account = 0, away_account = 0;
    if (home_of() != 0) {
        if (std::int8_t(invoke(c, callee::club_is_inactive, 0x100f888c, {home_of()})) != 0) return;
        home_account = invoke_unit(c, callee_account, 0x100f88a0, {manager, home_of()});
        c.set(25, home_account);
    }
    if (away_of() != 0) {
        if (std::int8_t(invoke(c, callee::club_is_inactive, 0x100f88b4, {away_of()})) != 0) return;
        away_account = invoke_unit(c, callee_account, 0x100f88c8, {manager, away_of()});
        c.set(24, away_account);
    }

    // The crowd, the figure carried in, and a bonus drawn from the crowd.
    const auto crowd = c.i32(record + match_record::crowd);
    std::int32_t carried = 0, bonus = 0;
    Word held_18 = u(crowd);  // the original leaves a value in r18 that later calls see
    c.set(18, held_18);
    if (crowd == 0) {
        c.set({{23, 0}, {19, 0}, {21, 0}});
    } else {
        carried = c.i32(record + match_record::carried);
        c.set({{19, u(crowd)}, {23, u(carried)}});
        const auto sample = std::int32_t(draw(c, 0x100f8900));
        const auto divisor = add(sub(sample, mul(sample / 10, 10)), 2);
        const auto quotient = divw(1, divisor);
        const auto crowd_single = fp.to_single(crowd);
        const auto quotient_double = fp.to_double(quotient);
        auto shaped = fp.fpu.fadd(fp.k(0x218), quotient_double);
        shaped = fp.fpu.frsp(shaped);
        shaped = fp.fpu.fmuls(crowd_single, shaped);
        bonus = fp.truncate(shaped);
        c.set(21, u(bonus));
    }

    // A levy by the crowd's size (the pool factor steps at 5,000, 10,000 and 20,000 for a national side).
    const auto home = home_of();
    const bool national = c.u8(home + crec::national) == 1 || c.u8(away_of() + crec::national) == 1;
    std::int32_t levy_a;
    if (national) {
        const Word factor = crowd > 0x4e20 ? 0x78 : crowd > 0x2710 ? 0x128 : crowd > 0x1388 ? 0x88 : 0x38;
        const auto crowd_single = fp.to_single(crowd);
        levy_a = fp.truncate(fp.fpu.fmul(fp.k(factor), crowd_single));
        if (c.i8(fixture_record + fixture::guaranteed) != 0 && levy_a < 500) levy_a = 500;
    } else {
        const auto crowd_single = fp.to_single(crowd);
        levy_a = fp.truncate(fp.fpu.fmul(fp.k(0x60), crowd_single));
    }
    c.set(20, u(levy_a));

    // A second levy: by the home club's reputation and the crowd, with draws.
    std::int32_t levy_b;
    if (c.i8(home + crec::national) == 1) {
        if (c.i16(home + crec::reputation) != 0) {
            const auto sample = std::int32_t(draw(c, 0x100f8b28));
            const auto reputation_double = fp.to_double(c.i16(home_of() + crec::reputation));
            auto per_rep = fp.fpu.fmul(fp.k(0x38), reputation_double);
            const auto crowd_single = fp.to_single(crowd);
            auto sum = fp.fpu.multiply_add(0x100f8bb0, fp.k(0xa0), crowd_single, per_rep);
            const auto rest_double = fp.to_double(sub(sample, mul(sample / 200, 200)));
            sum = fp.fpu.fadd(sum, rest_double);
            sum = fp.fpu.fadd(fp.k(0x248), sum);
            levy_b = fp.truncate(sum);
        } else {
            const auto sample = std::int32_t(draw(c, 0x100f8bf8));
            const auto crowd_single = fp.to_single(crowd);
            auto sum = fp.fpu.multiply_add(0x100f8c50, fp.k(0xa0), crowd_single, fp.k(0x250));
            const auto rest_double = fp.to_double(sub(sample, mul(sample / 400, 400)));
            sum = fp.fpu.fadd(sum, rest_double);
            levy_b = fp.truncate(sum);
        }
    } else if (const auto reputation = std::int32_t(c.i16(home + crec::reputation)); reputation != 0) {
        const auto reputation_double = fp.to_double(reputation);
        const auto limit = fp.truncate(fp.fpu.fmul(fp.k(0x60), reputation_double));
        held_18 = u(limit);
        c.set(18, held_18);
        std::int32_t drawn = 0;
        if (limit != 0) drawn = draw_below(c, limit, 0x100f8cec);
        c.set(17, u(drawn));
        const auto sample = std::int32_t(draw(c, 0x100f8e34));
        const auto crowd_single = fp.to_single(crowd);
        const auto drawn_single = fp.to_single(drawn);
        auto sum = fp.fpu.fadds(crowd_single, drawn_single);
        const auto rest_single = fp.to_single(sub(sample, mul(sample / 250, 250)));
        sum = fp.fpu.fadds(sum, rest_single);
        levy_b = fp.truncate(sum);
    } else {
        const auto first_sample = std::int32_t(draw(c, 0x100f8ee8));
        const auto first_rest = sub(first_sample, mul(first_sample / 250, 250));
        held_18 = u(first_rest);
        c.set(18, held_18);
        const auto second_sample = std::int32_t(draw(c, 0x100f8f3c));
        const auto crowd_single = fp.to_single(crowd);
        const auto first_single = fp.to_single(first_rest);
        auto sum = fp.fpu.fadds(crowd_single, first_single);
        const auto second_single = fp.to_single(sub(second_sample, mul(second_sample / 250, 250)));
        sum = fp.fpu.fadds(sum, second_single);
        levy_b = fp.truncate(sum);
    }

    // The broadcasting share: by the fixture's band, else by the kind of round.
    std::int32_t share = 0;
    switch (c.i8(fixture_record + fixture::broadcast_band)) {
        case 1: share = 75000; break;
        case 0: share = 25000; break;
        case 2: share = 125000; break;
        default: break;
    }
    if (share == 0) {
        const auto kind = c.i8(fixture_record + fixture::round_kind);
        if (kind == 8 || kind == 7) {
            const auto together = std::int32_t(c.i16(home + crec::reputation)) + std::int32_t(c.i16(away_of() + crec::reputation));
            share = together > 10000 ? 125000 : together > 0x1d4c ? 75000 : 25000;
        } else if (kind == 6 || kind == 4) {
            const auto round = std::int32_t(c.i16(c.u32(fixture_record + fixture::competition) + league_record::round));
            share = mul(round > 5 ? round : 5, kind == 6 ? 125000 : 25000) / 20;
        }
    }

    // The home club's division level cuts every figure (a quarter-share division: level 2; others a tenth).
    const auto home_now = home_of();
    const auto division = c.u32(home_now + crec::division);
    const auto level = division != 0 ? c.i8(division + division_record::level) : std::int8_t(3);
    const auto halve = [&] {
        share /= 5;
        bonus /= 2;
        levy_a /= 4;
        levy_b /= 2;
    };
    if (level == 2) {
        halve();
    } else if (level != 1) {
        share /= 10;
        bonus /= 10;
        levy_a /= 15;
        levy_b /= 10;
    }
    if (home_now != 0 && division != 0) {
        const auto group = c.i32(division + division_record::group);
        if (group == c.i32(c.toc(cell::group_a)) || group == c.i32(c.toc(cell::group_b)) ||
            group == c.i32(c.toc(cell::group_c))) {
            halve();
        }
    }
    const auto total = sub(sub(add(carried, bonus), levy_a), levy_b);

    if (c.i8(fixture_record + fixture::cup_tie) != 0 || c.u32(home_now + crec::league) != c.u32(fixture_record + fixture::competition)) {
        // Not a league match: the share goes to both clubs, and the total is split between them.
        const auto share_double = fp.to_double(share);
        const auto both = fp.truncate(fp.fpu.fmul(fp.k(0x38), share_double));
        for (const auto side : {home_account, away_account}) {
            book_line(c, side, acct::broadcast_income, u(both));
            move_balance(c, side, widen(both));
        }
        const auto half = total / 2;
        for (const auto side : {home_account, away_account}) {
            book_line(c, side, acct::match_income, u(half));
            move_balance(c, side, widen(half));
        }
        return;
    }
    // A league match: the home club's share and the away club's differ, and the home club takes the whole total.
    const auto share_first = fp.to_double(share);
    const auto home_part_double = fp.fpu.fmul(fp.k(0x70), share_first);
    const auto share_second = fp.to_double(share);
    const auto home_part = fp.truncate(home_part_double, 2);
    const auto away_part_double = fp.fpu.fmul(fp.k(0x60), share_second);
    const auto away_part = fp.truncate(away_part_double);
    book_line(c, home_account, acct::broadcast_income, u(home_part));
    move_balance(c, home_account, widen(home_part));
    book_line(c, away_account, acct::broadcast_income, u(away_part));
    move_balance(c, away_account, widen(away_part));
    book_line(c, home_account, acct::match_income, u(total));
    move_balance(c, home_account, widen(total));
    // A club with ground works under way pays for them out of the match: a share of its seats.
    const auto home_last = home_of();
    if (c.i8(home_last + crec::ground_works) != 0) {
        if (const auto board = c.u32(home_last + crec::board); board != 0) {
            const auto seats = c.i32(board + board_record::seats);
            c.set({{17, u(seats)}, {20, u(levy_a)}, {21, u(bonus)}, {22, u(share)}});
            const auto sample = std::int32_t(draw(c, 0x100f94f8));
            const auto seats_single = fp.to_single(seats);
            const auto step_single = fp.to_single(add(sub(sample, mul(sample / 15, 15)), 1));
            auto fraction = fp.fpu.fdiv(step_single, fp.k(0xf8));
            fraction = fp.fpu.frsp(fraction);
            fraction = fp.fpu.fadd(fp.k(0x208), fraction);
            fraction = fp.fpu.frsp(fraction);
            fraction = fp.fpu.fmuls(seats_single, fraction);
            const auto works = fp.truncate(fraction);
            book_line(c, home_account, acct::stadium_costs, u(works));
            move_balance(c, home_account, 0 - std::uint64_t(u(works)));
        }
    }
}
}
