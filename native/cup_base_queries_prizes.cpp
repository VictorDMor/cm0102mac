// CUP.CPP, block `queries`: the prize money a tie pays (0x10485930: COMP's prize news item and the clubs' cash through
// FINANCE) and the rank the cup gives its clubs (0x10485b10, `CUP::rank_teams`: new[], COMP_UTIL's sort and COMP_STATS).
// The items and the cash write news records and the club accounts: the other modules' routines are called through the
// dispatcher (COMP.CPP's item directly) with the non-volatile registers the original holds set before each call.
#include "aot_runtime.hpp"
#include "comp_news.hpp"
#include "cup_base_queries_detail.hpp"

namespace cm::aot::cup_base::queries {
namespace {
constexpr std::int32_t cash_cell = -0x7ab8,    // cell: the club-cash object FINANCE.CPP's 0x10103510 takes
    title_buffer = -0x79dc, text_buffer = -0x79e0;  // the localized dialog's two buffers (rank_teams' allocation failure)
std::int32_t divide(std::int32_t dividend, std::int32_t divisor) {
    return std::int32_t(divide_signed<4>(std::uint32_t(dividend), std::uint32_t(divisor)));
}
std::uint32_t half_word(Cpu& c, std::uint32_t at) { return sext16(c.u16(at)); }
}

void pay_prizes(Cpu& c, std::uint32_t cup, std::uint32_t winner, std::uint32_t loser, std::uint32_t round, std::uint32_t fixture) {
    const Frame frame(c, 0x10485930, 0x50, {.first_gpr = 26});
    Held h(c);
    const auto cash = c.toc(cash_cell);
    // r26 the cup, r27 the winner's entrant row, r28 the loser's, r29 the loser's amount, r30 the winner's, r31 the cash cell.
    auto& winner_amount = h.v[30]; auto& loser_amount = h.v[29];
    h.v[26] = cup; h.v[27] = winner; h.v[28] = loser; h.v[31] = cash;
    c.set(8, 0);
    auto flag = std::uint32_t(c.u8(winner + 5));
    if (c.i16(fixture + fixture::code) == 0x136 && std::int8_t(flag) == 1) flag = 3;
    c.set(10, flag);
    const auto index = sext16(round);
    const auto rounds = c.u32(cup + cup::rounds);
    c.set(4, rounds);
    const auto row = index * cup::round_row;
    winner_amount = c.u32(rounds + row + round::prize_a);
    loser_amount = c.u32(rounds + row + round::prize_b);
    if (winner_amount == 0xffffffffu) {
        // The round pays a share of the pool: the pool over the rounds left, over the round's ties. A loser's prize of -1 too
        // is a quarter, the winner's three quarters; a fixed loser's prize leaves the whole share to the winner.
        // (The original also tests "the winner's prize is -1 and the loser's is not" there, which cannot happen: dead.)
        const auto left = std::int32_t(c.i16(cup + competition::rounds)) - std::int32_t(index);
        const auto per_round = divide(std::int32_t(c.u32(cup + pool_money)), left);
        c.set(5, std::uint32_t(per_round));
        const auto share = divide(per_round, c.i16(rounds + row + round::ties));
        if (loser_amount == 0xffffffffu) {
            winner_amount = std::uint32_t(std::int32_t(std::uint32_t(share) * 3) / 4);
            loser_amount = std::uint32_t(share / 4);
        } else {
            winner_amount = std::uint32_t(share);
        }
    }
    std::uint32_t next_label = 0;
    if (std::int32_t(index) + 1 < std::int32_t(c.i16(cup + competition::rounds))) {
        c.set(4, c.u32(cup + cup::rounds));
        const auto label = half_word(c, c.u32(cup + cup::rounds) + (index + 1) * cup::round_row + round::label);
        if (label != 0x8c) next_label = label;
    }
    c.set(8, next_label);
    if (winner_amount != 0) {
        const auto club = c.u32(winner);
        c.set({{3, cup}, {4, club}, {5, std::uint32_t(std::int32_t(winner_amount) >> 31)}, {6, winner_amount}, {7, 0xffffffffu},
               {8, next_label}, {9, fixture}, {10, flag}});
        h.publish();
        c.link(0x10485a54);
        comp::news_prize_money(c, cup, club, std::uint32_t(std::int32_t(winner_amount) >> 31), winner_amount, 0xffffffffu, next_label,
                               fixture, flag);
        h.publish();
        const auto account = invoke(c, 0x10103510, 0x10485a60, {c.u32(cash), club});
        h.publish();
        invoke(c, 0x100f71f0, 0x10485a68, {account, winner_amount});
        c.put32(cup + pool_money, c.u32(cup + pool_money) - winner_amount);
    }
    if (loser_amount != 0) {
        h.publish();
        const auto account = invoke(c, 0x10103510, 0x10485a88, {c.u32(cash), c.u32(loser)});
        h.publish();
        invoke(c, 0x100f71f0, 0x10485a90, {account, loser_amount});
        c.put32(cup + pool_money, c.u32(cup + pool_money) - loser_amount);
    }
}

namespace {
struct MemoryFailure { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; };
// "CUP::rank_teams() : cannot malloc clubs array": the assertion text with the pool's message, the localized dialog, the
// success flag cleared and exit(-1). r22 and r23 are the dialog's two buffers.
void memory_failure(Cpu& c, const Frame& frame, Held& h, const MemoryFailure& f) {
    const auto pool = c.toc(toc::pool), message = c.toc(toc::message);
    h.publish();
    c.set({{3, pool}, {4, 0}, {5, 0}, {6, frame.at(f.name)}, {7, frame.at(f.extension)}});
    c.call(0x10739890, f.at + 4);
    c.call(0x10768810, f.at + 8);
    c.set({{5, c.r(3)}, {3, message}, {4, pool + text::report_format_text}, {6, frame.at(f.name)}, {7, frame.at(f.extension)},
           {9, pool + text::rank_memory}, {8, f.line}});
    c.call(0x1077b720, f.at + 0x28);
    c.set({{3, h.v[23]}, {4, pool + text::memory_title}});
    c.call(0x10727890, f.at + 0x34);
    c.set({{3, h.v[22]}, {5, message}, {4, pool + text::memory_message}});
    c.call(0x10727890, f.at + 0x44);
    c.set({{3, h.v[23]}, {4, h.v[22]}, {5, 0}});
    c.call(0x10769b60, f.at + 0x54);
    c.put8(c.toc(toc::success), 0);
    invoke(c, callee::exit, f.at + 0x64, {0xffffffffu});
}
}

void rank_teams(Cpu& c, std::uint32_t cup) {
    const Frame frame(c, 0x10485b10, 0xb0, {.first_gpr = 22});
    Held h(c);
    // r22 and r23 the localized dialog's buffers, r24 the pool, r25 the message buffer, r26 the cup, r27 and r28 the rank
    // counters (1), r29 the clubs handed out (0), r30 the success flag, r31 the array.
    h.v[22] = c.toc(text_buffer); h.v[23] = c.toc(title_buffer); h.v[24] = c.toc(toc::pool); h.v[25] = c.toc(toc::message);
    h.v[26] = cup; h.v[27] = 1; h.v[28] = 1; h.v[29] = 0; h.v[30] = c.toc(toc::success);
    const auto clubs = [&] { return std::int32_t(c.i16(cup + competition::club_count)); };
    if (c.u32(cup + competition::stats) == 0) {
        h.publish();
        report(c, frame, {0x10485b64, 0x68, 0x40, 0x1819});
        return;
    }
    h.publish();
    const auto array = invoke(c, callee::allocate_block, 0x10485bb0, {std::uint32_t(clubs()) << 2});
    h.v[31] = array;
    if (array == 0) {
        memory_failure(c, frame, h, {0x10485bcc, 0x48, 0x38, 0x1821});
        return;
    }
    h.v[30] = 0;
    for (; std::int32_t(h.v[30]) < clubs(); ++h.v[30]) c.put32(array + h.v[30] * 4, c.u32(c.u32(cup + cup::entrants) + h.v[30] * cup::entrant_row));
    h.publish();
    invoke(c, 0x10477b40, 0x10485c74, {array, sext16(c.u16(cup + competition::club_count))});
    // The rounds from the last but one (the last round's ties are the one before the final) down to the first; the clubs
    // in sorted order get their rank: the counter r27 advances per club of a round (per tie slot in the second pass).
    h.v[30] = sext16(c.u16(cup + competition::rounds) - 2);
    h.v[25] = h.v[30] * cup::round_row;
    const auto club_at = [&](std::uint32_t slot) { return c.u32(array + sext16(slot) * 4); };
    const auto rank = [&](std::uint32_t club, std::uint32_t value, std::uint32_t back) {
        h.publish();
        invoke(c, 0x10457490, back, {c.u32(cup + competition::stats), club, value});
    };
    auto& slot = h.v[29]; auto& counter = h.v[27]; auto& base = h.v[28]; auto& round_at = h.v[30]; auto& offset = h.v[25];
    auto& limit_offset = h.v[24]; auto& pass = h.v[22];
    for (; std::int32_t(round_at) >= 0 && club_at(slot) != 0; --round_at, offset -= cup::round_row) {
        if (std::int32_t(round_at) == std::int32_t(c.i16(cup + competition::rounds)) - 2) {
            limit_offset = offset + round::ties;
            for (pass = 0; std::int32_t(pass) < std::int32_t(c.i16(c.u32(cup + cup::rounds) + limit_offset)); ++pass) {
                const auto club = club_at(slot);
                ++slot;
                rank(club, base, 0x10485cd8);
                ++counter;
            }
        }
        base = counter;
        limit_offset = offset + round::ties;
        for (pass = 0; std::int32_t(pass) < std::int32_t(c.i16(c.u32(cup + cup::rounds) + limit_offset)); ++pass) {
            const auto club = club_at(slot);
            if (club != 0) {
                ++slot;
                rank(club, base, 0x10485d24);
            }
            ++counter;
        }
    }
    h.publish();
    invoke(c, callee::free_block, 0x10485d5c, {array});
}

}
