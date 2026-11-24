// LEAGUE.CPP, block `table`: the order of the standings. 0x1049eb30 sorts a league's rows with qsort and the
// comparators below (the comparators read the league and an alternative comparator descriptor from TOC words
// -0x3750 / -0x3754 that the sort stores); with the position tie-break first it runs the head-to-head pass
// (0x1049f030) and then a bubble pass over the league's own rows.
//
// The routines whose generated callers read r4-r10 after the call (0x1049eb30, 0x1049f030, 0x1049f9a0,
// 0x1049fdc0: league_base_volatile_readers.inc) leave those registers as the original does: each comparator
// writes the registers the original writes on the path it takes, and the sort's own writes follow the listing.
// Registers that are only scratch in the original (r16-r27 inside the swap, r25/r26/r29/r31 at 0x1049f030's
// qsort call) are not reproduced where no callee that can reach an import runs; see the comments.
#include "match_play.hpp"
#include "league_base_table_detail.hpp"

namespace cm::aot::league_base {
namespace st = table::row;
using table::call_descriptor;
using table::lbz_extsb;
using table::lha;
using table::put_toc;

namespace {
constexpr std::uint32_t tie_break_at = league::tie_breaks;  // +0xc7: four signed bytes

// The comparators' shared opening: a null club makes any pair equal; otherwise the league's alternative
// comparator (TOC -0x3754) answers first and, when its answer is nonzero, again: the second answer is the
// result. True when `result` is final (r3 holds it).
bool decided(Cpu& c, std::uint32_t left, std::uint32_t right, std::uint32_t first_back, std::uint32_t second_back,
             std::uint32_t& result) {
    if (c.u32(left) == 0 || c.u32(right) == 0) {
        result = 0;
        c.set(3, 0);
        return true;
    }
    if (const auto descriptor = c.toc(toc::sort_compare)) {
        c.set({{3, left}, {4, right}});
        if (call_descriptor(c, descriptor, first_back) != 0) {
            c.set({{3, left}, {4, right}});
            result = call_descriptor(c, c.toc(toc::sort_compare), second_back);
            return true;
        }
    }
    return false;
}

// Two rows' clubs by their first word, ascending (the order of two clubs with nothing to separate them).
// Leaves r4 (the left club) and r3 as the original does.
std::uint32_t order_by_club(Cpu& c, std::uint32_t left, std::uint32_t right) {
    const auto first = c.u32(left);
    c.set(4, first);
    if (first != 0) {
        const auto second = c.u32(right);
        c.set(3, second);
        if (second != 0) {
            const auto word = c.u32(second);
            c.set(3, word);
            return c.u32(first) - word;
        }
    }
    return 0;
}
}

std::int32_t compare_standings(Cpu& c, std::uint32_t left, std::uint32_t right) {
    const Frame frame(c, 0x1049f9a0, 0x50, {.first_gpr = 29});
    const auto league = c.toc(toc::sort_league);
    const auto group = c.u32(league + table::field::group);
    c.set({{5, league}, {29, group}, {30, right}, {31, left}});
    const auto done = [&](std::uint32_t value) { c.set(3, value); return std::int32_t(value); };
    std::uint32_t result = 0;
    if (decided(c, left, right, 0x1049f9f8, 0x1049fa14, result)) return done(result);
    const auto offset = group << 4;
    const auto a = left + offset, b = right + offset;
    c.set({{4, offset}, {3, a}, {5, b}});
    const auto points_a = lha(c, a + 0x1c);  // the group's points
    const auto points_b = lha(c, b + 0x1c);
    c.set(6, points_b);
    if (points_a != points_b) return done(points_b - points_a);
    const auto played = c.u8(a + st::totals);  // r0: the left row's played byte, kept for the test after the loop
    c.set(3, sext8(played));
    bool counted = sext8(played) != 0;
    if (!counted) {
        c.set(3, lbz_extsb(c, b + st::totals));
        counted = c.r(3) != 0;
    }
    if (!counted) return done(order_by_club(c, left, right));
    c.set({{3, 4}, {9, league}, {5, 0}});
    std::uint32_t k = 0;
    for (; k < 4; ++k) {
        c.set(5, k);
        if (group == 0) {
            c.set(6, league);
            const auto kind = lbz_extsb(c, league + tie_break_at + k);
            c.set(3, kind);
            if (std::int32_t(kind) == table::tie_break::keep_order)
                return done(lbz_extsb(c, left + st::position) - lbz_extsb(c, right + st::position));
        }
        const auto raw = c.u8(league + tie_break_at + k);
        c.set(6, raw);
        const auto kind = std::int32_t(sext8(raw));
        c.set(3, std::uint32_t(kind));
        if (kind == 0) break;
        switch (kind) {
        case table::tie_break::wins: {
            const auto x = lbz_extsb(c, a + 0x15), y = lbz_extsb(c, b + 0x15);
            c.set({{6, x}, {3, y}});
            if (x != y) return done(y - x);
            break;
        }
        case table::tie_break::goal_difference: {
            const auto a_against = lha(c, a + 0x1a), a_scored = lha(c, a + 0x18);
            const auto b_against = lha(c, b + 0x1a), b_scored = lha(c, b + 0x18);
            const auto difference_a = a_scored - a_against, difference_b = b_scored - b_against;
            c.set({{10, b}, {8, a_against}, {7, difference_a}, {6, b_against}, {3, difference_b}});
            if (difference_a != difference_b) return done(difference_b - difference_a);
            break;
        }
        case table::tie_break::goals_scored: {
            const auto x = lha(c, a + 0x18), y = lha(c, b + 0x18);
            c.set({{6, x}, {3, y}});
            if (x != y) return done(y - x);
            break;
        }
        case table::tie_break::away_goals: {
            const auto x = lha(c, left + st::away_scored), y = lha(c, right + st::away_scored);
            c.set({{3, x}, {6, y}});
            if (x != y) return done(y - x);
            break;
        }
        case table::tie_break::position: {
            const auto x = lbz_extsb(c, left + st::position), y = lbz_extsb(c, right + st::position);
            c.set({{6, x}, {3, y}});
            if (x != y) return done(y - x);
            break;
        }
        default: break;  // 6 with a group other than 0, and values outside 0..5: no decision
        }
    }
    c.set(5, k);  // the counter after the loop: 4 when no tie-break was 0, else where the list ended
    if (group == 0) {
        // The league's own table, nothing else decided: both clubs have played every match and each club has
        // a record at +0x3c whose first word orders them (larger first).
        c.set(3, league);
        const auto limit = lha(c, league + table::field::played_limit);
        c.set(5, limit);
        if (sext8(played) == limit) {
            c.set(3, right + offset);
            if (lbz_extsb(c, right + offset + st::totals) == limit) {
                const auto club_b = c.u32(right);
                const auto record_b = c.u32(club_b + 0x3c);
                c.set({{3, club_b}, {4, record_b}});
                if (record_b != 0) {
                    const auto record_a = c.u32(c.u32(left) + 0x3c);
                    c.set(3, record_a);
                    if (record_a != 0) {
                        const auto word_a = c.u32(record_a), word_b = c.u32(record_b);
                        const auto difference = word_b - word_a;
                        c.set(3, difference);
                        if (difference != 0) return done(difference);
                    }
                }
            }
        }
    }
    return done(order_by_club(c, left, right));
}

std::int32_t compare_standings_by_name(Cpu& c, std::uint32_t left, std::uint32_t right) {
    const Frame frame(c, 0x1049fc80, 0x50, {.first_gpr = 29});
    const auto league = c.toc(toc::sort_league);
    const auto group = c.u32(league + table::field::group);
    c.set({{5, league}, {29, group}, {30, right}, {31, left}});
    const auto done = [&](std::uint32_t value) { c.set(3, value); return std::int32_t(value); };
    std::uint32_t result = 0;
    if (decided(c, left, right, 0x1049fcd8, 0x1049fcf4, result)) return done(result);
    const auto offset = group << 4;
    const auto a = left + offset, b = right + offset;
    c.set({{4, a}, {3, b}});
    const auto points_a = lha(c, a + 0x1c), points_b = lha(c, b + 0x1c);
    c.set(5, points_b);
    if (points_a != points_b) return done(points_b - points_a);
    if (sext8(c.u8(a + st::totals)) == 0 && sext8(c.u8(b + st::totals)) == 0) return done(order_by_club(c, left, right));
    // Played clubs: the position byte, larger first, then the club names.
    const auto position = lbz_extsb(c, right + st::position) - lbz_extsb(c, left + st::position);
    c.set(3, position);
    if (position != 0) return done(position);
    const auto name_a = c.u32(left) + 0x1ee, name_b = c.u32(right) + 0x1ee;
    return done(invoke(c, comp::callee::strcmp, 0x1049fd98, {name_a, name_b}));
}

std::int32_t compare_head_to_head(Cpu& c, std::uint32_t left, std::uint32_t right) {
    const Frame frame(c, 0x1049fdc0, 0x50, {.first_gpr = 28});
    const auto league = c.toc(toc::sort_league);
    const auto group = c.u32(league + table::field::group);
    std::uint32_t entry = 0;  // r29: the left row's head-to-head entry for the right club
    c.set({{29, 0}, {30, right}, {31, left}, {5, league}, {28, group}});
    const auto done = [&](std::uint32_t value) { c.set(3, value); return std::int32_t(value); };
    std::uint32_t result = 0;
    if (decided(c, left, right, 0x1049fe20, 0x1049fe3c, result)) return done(result);
    const auto offset = group << 4;
    const auto a = left + offset, b = right + offset;
    c.set({{4, a}, {3, b}});
    const auto points_a = lha(c, a + 0x1c), points_b = lha(c, b + 0x1c);
    c.set(5, points_b);
    if (points_a != points_b) return done(points_b - points_a);
    if (sext8(c.u8(a + st::totals)) == 0 && sext8(c.u8(b + st::totals)) == 0) return done(order_by_club(c, left, right));
    // Played clubs: the left row's head-to-head entry of the right club.
    c.set({{4, league}, {7, 0}, {6, 0}});
    std::uint32_t counter = 0, at = 0;  // r7, r6
    for (;;) {
        const auto clubs = lha(c, league + competition::club_count);
        c.set(3, clubs);
        const auto tested = sext16(counter);
        c.set(5, tested);
        if (!(std::int32_t(tested) < std::int32_t(clubs - 1))) break;
        const auto table_pointer = c.u32(left + st::head_to_head);
        c.set(5, table_pointer);
        const auto club = c.u32(right);
        const auto candidate = c.u32(table_pointer + at);
        c.set(3, candidate);
        if (candidate == club) {
            entry = table_pointer + (sext16(counter) << 4);
            c.set(29, entry);
            break;
        }
        at += table::entry::size;
        ++counter;
        c.set({{6, at}, {7, counter}});
    }
    if (entry == 0) return done(0);
    const auto won = lbz_extsb(c, entry + table::entry::won), lost = lbz_extsb(c, entry + table::entry::lost);
    c.set({{3, won}});
    if (std::int32_t(won) > std::int32_t(lost)) return done(0xffffffffu);
    if (std::int32_t(won) < std::int32_t(lost)) return done(1);
    const auto scored = lha(c, entry + table::entry::scored), conceded = lha(c, entry + table::entry::conceded);
    c.set({{4, scored}, {3, conceded}});
    if (std::int32_t(scored) > std::int32_t(conceded)) return done(0xffffffffu);
    // subfc / rlwinm / subfe: the conceded sign bit, minus the scored sign bit, minus one, plus the carry of
    // scored - conceded (scored >= conceded as unsigned words).
    const std::uint32_t carry = scored >= conceded ? 1u : 0u;
    return done((conceded >> 31) - (scored >> 31) - 1u + carry);
}

std::int32_t compare_tied(Cpu& c, std::uint32_t left, std::uint32_t right) {
    // A leaf (no frame). It is also a comparator a league's descriptor can name, called through the pointer glue,
    // so it leaves r5-r9 as the original does (r0 and CTR are not compared).
    const auto done = [&](std::uint32_t value) { c.set(3, value); return std::int32_t(value); };
    if (c.u32(left) == 0 || c.u32(right) == 0) return done(0);
    const auto points_a = lha(c, left + 0x1c), points_b = lha(c, right + 0x1c);
    c.set(5, points_b);
    if (points_a != points_b) return done(points_b - points_a);
    const auto league = c.toc(toc::sort_league);
    c.set({{8, league}, {9, 0}});
    for (std::uint32_t k = 0; k < 4; ++k) {
        c.set(9, k);
        const auto raw = c.u8(league + tie_break_at + k);
        c.set(5, raw);
        const auto kind = std::int32_t(sext8(raw));
        if (kind == 0) break;
        switch (kind) {
        case table::tie_break::wins: {
            const auto x = lbz_extsb(c, left + 0x15), y = lbz_extsb(c, right + 0x15);
            c.set(5, y);
            if (x != y) return done(y - x);
            break;
        }
        case table::tie_break::goal_difference: {
            const auto a_against = lha(c, left + 0x1a), b_against = lha(c, right + 0x1a);
            const auto difference_a = lha(c, left + 0x18) - a_against;
            const auto difference_b = lha(c, right + 0x18) - b_against;
            c.set({{7, a_against}, {6, difference_a}, {5, b_against}});
            if (difference_a != difference_b) return done(difference_b - difference_a);
            break;
        }
        case table::tie_break::goals_scored: {
            const auto x = lha(c, left + 0x18), y = lha(c, right + 0x18);
            c.set(5, y);
            if (x != y) return done(y - x);
            break;
        }
        case table::tie_break::away_goals: {
            const auto x = lha(c, left + st::away_scored), y = lha(c, right + st::away_scored);
            c.set(5, y);
            if (x != y) return done(y - x);
            break;
        }
        default: break;  // 4, 6 and values outside 1..5 do not separate two copy rows
        }
        c.set(9, k + 1);
    }
    return done(0);
}

void rank_by_head_to_head(Cpu& c, std::uint32_t league, std::uint32_t rows) {
    // The generated callers read r7-r10 after this call (league_base_volatile_readers.inc): the loops below set
    // r3-r10 where the original does, so what the last callee (delete[]) does not overwrite is the original's.
    const Frame frame(c, 0x1049f030, 0x80, {.first_gpr = 25});
    const auto message = c.toc(toc::message), pool = c.toc(toc::pool);
    c.set({{25, pool}, {26, message}, {27, league}, {28, rows}});
    const auto clubs = std::int32_t(c.i16(league + competition::club_count));
    const auto copy = invoke(c, 0x10775200, 0x1049f05c, {std::uint32_t(clubs), league::standing_row});
    c.set(30, copy);
    if (copy == 0) {
        report(c, frame, {0x1049f078, 0x40, 0x38, 0x17d});
        return;
    }
    namespace en = table::entry;
    // Phase 1: each row's copy takes the club and the head-to-head entries of the clubs level with it on points.
    c.set({{7, 0}, {5, 0}});
    std::uint32_t i = 0, offset_i = 0;  // r7, r5
    for (;;) {
        const auto count = lha(c, league + competition::club_count);
        c.set(4, count);
        if (!(std::int32_t(sext16(i)) < std::int32_t(count))) break;
        const auto source = rows + offset_i, target = copy + offset_i;
        c.set({{4, source}, {6, target}, {9, c.u32(source)}, {8, 0}, {3, 0}});
        c.put32(target, c.u32(source));
        std::uint32_t j = 0, offset_j = 0;  // r8, r3
        for (;;) {
            const auto bound = lha(c, league + competition::club_count);  // r11
            const auto tested = sext16(j);
            c.set(9, tested);
            if (!(std::int32_t(tested) < std::int32_t(bound))) break;
            if (sext16(i) != sext16(j)) {
                const auto other = rows + offset_j;
                const auto points_source = lha(c, source + 0x1c), points_other = lha(c, other + 0x1c);
                c.set({{10, points_source}, {9, points_other}});
                if (points_source == points_other) {
                    c.set(9, bound - 1);
                    std::uint32_t k = 0, at = 0;
                    for (;;) {
                        const auto step = sext16(k);
                        c.set(10, step);
                        if (!(std::int32_t(step) < std::int32_t(bound) - 1)) break;
                        const auto entries = c.u32(source + st::head_to_head);
                        const auto club = c.u32(other);
                        c.set(10, club);
                        if (c.u32(entries + at) == club) {
                            const auto entry = entries + (sext16(k) << 4);
                            c.put8(target + 0x14, c.u8(target + 0x14) + sext8(c.u8(entry + en::played)));
                            c.put8(target + 0x15, c.u8(target + 0x15) + sext8(c.u8(entry + en::won)));
                            c.put8(target + 0x16, c.u8(target + 0x16) + sext8(c.u8(entry + en::drawn)));
                            c.put8(target + 0x17, c.u8(target + 0x17) + sext8(c.u8(entry + en::lost)));
                            c.put16(target + 0x18, c.u16(target + 0x18) + c.u16(entry + en::scored));
                            c.put16(target + 0x1a, c.u16(target + 0x1a) + c.u16(entry + en::conceded));
                            const auto sum = lha(c, target + 0x1c) + lha(c, entry + en::points);
                            c.put16(target + 0x1c, sum);
                            c.set({{10, c.u32(source + st::head_to_head)}, {9, sum}});
                            break;
                        }
                        at += en::size;
                        ++k;
                    }
                }
            }
            offset_j += league::standing_row;
            ++j;
            c.set({{3, offset_j}, {8, j}});
        }
        offset_i += league::standing_row;
        ++i;
        c.set({{5, offset_i}, {7, i}});
    }
    // Phase 2: sort the copy by compare_tied (TOC -0x4710) and number the rows: a club level with the next
    // one on every compared key shares its position.
    c.set({{27, league}, {28, rows}, {30, copy}});
    invoke(c, callee::sort, 0x1049f248, {copy, c.r(4), league::standing_row, c.toc(toc::order_4710)});
    std::uint32_t rank = std::uint32_t(clubs);  // r29
    std::uint32_t row_index = 0;                 // r31
    for (;; ++row_index) {
        const auto count = lha(c, league + competition::club_count);
        c.set(6, count);
        if (!(std::int32_t(sext16(row_index)) < std::int32_t(count))) break;
        const auto copy_row = copy + row_index * league::standing_row;
        c.set({{7, 0}, {3, rows}, {5, 0}});
        std::uint32_t j2 = 0, offset = 0;
        for (;;) {
            if (!(std::int32_t(sext16(j2)) < std::int32_t(count))) break;
            const auto club = c.u32(copy_row);
            c.set(4, club);
            if (club == c.u32(rows + offset)) {
                c.put8(rows + sext16(j2) * league::standing_row + st::position, rank);
                break;
            }
            offset += league::standing_row;
            ++j2;
            c.set({{5, offset}, {7, j2}});
        }
        const auto next = sext16(row_index) + 1;
        if (std::int32_t(next) < std::int32_t(lha(c, league + competition::club_count))) {
            c.set({{25, row_index * league::standing_row}, {26, copy_row}, {29, rank}, {31, row_index}, {30, copy}});
            c.set({{3, copy + row_index * league::standing_row}, {4, copy + next * league::standing_row}});
            c.link(0x1049f2d4);
            if (compare_tied(c, copy_row, copy + next * league::standing_row) != 0) --rank;
        }
    }
    c.set({{25, row_index * league::standing_row}, {26, copy + row_index * league::standing_row}, {29, rank}, {30, copy}, {31, row_index}});
    invoke(c, comp::callee::free_block, 0x1049f304, {copy});
}

std::uint32_t sort_standings(Cpu& c, std::uint32_t league, std::uint32_t rows) {
    const LeagueTurn turn(c.s);
    const Frame frame(c, 0x1049eb30, 0xa0, {.first_gpr = 13});
    const auto descriptor = c.toc(toc::standings_order);  // r13
    std::uint32_t keep_order_found = 0, from_first = 0;   // r14: the keep-order code was found, r15: the position code came first
    c.set({{13, descriptor}, {14, 0}, {15, 0}, {28, league}});
    if (rows == 0) rows = c.u32(league + league::standings);
    // The original keeps the row array in its parameter slot (entry sp+0x1c, in the caller's frame).
    c.put32(frame.at(0xbc), rows);
    if (c.i16(league + table::field::played_limit) == -1 && c.i16(league + competition::club_count) > 0 &&
        c.i8(league + competition::byte_45) != 0) {
        const auto clubs = lha(c, league + competition::club_count);
        c.put16(league + table::field::played_limit, (clubs - 1) * lha(c, league + competition::rounds));
    }
    const auto count_register = [&] { return lha(c, league + competition::club_count); };
    if (rows == 0) {
        c.set(3, 1);
        return 1;
    }
    put_toc(c, toc::sort_league, league);
    // The tie-break list: the keep-order code ends it (and skips the alternative comparator); the position
    // code, first in the list, with head-to-head entries present, runs the head-to-head pass.
    std::uint32_t index = 0;  // r4
    c.set(4, 0);
    while (std::int32_t(sext8(index)) < 4) {
        const auto at = sext8(index);
        if (c.u32(league + table::field::group) == 0 &&
            std::int32_t(sext8(c.u8(league + tie_break_at + at))) == table::tie_break::keep_order) {
            keep_order_found = 1;
            c.set(14, 1);
            break;
        }
        const auto kind = std::int32_t(sext8(c.u8(league + tie_break_at + at)));
        if (kind == 0) break;
        if (kind == table::tie_break::position && c.i16(league + competition::club_count) != 0 &&
            c.u32(rows + st::head_to_head) != 0) {
            if (sext8(index) == 0) {
                from_first = 1;
                c.set(15, 1);
            }
            c.set({{3, league}, {4, rows}});
            c.link(0x1049ec3c);
            rank_by_head_to_head(c, league, rows);
            break;
        }
        ++index;
        c.set(4, index);
    }
    const auto sort_rows = [&](std::uint32_t order, std::uint32_t back) {
        invoke(c, callee::sort, back, {rows, count_register(), league::standing_row, order});
    };
    if (keep_order_found) {
        // keep-order: the rows are sorted only while the group is 0.
        if (c.u32(league + table::field::group) == 0) sort_rows(descriptor, 0x1049ef90);
    } else {
        const auto alternative = c.u32(league + table::field::compare);
        c.set(3, alternative);
        put_toc(c, toc::sort_compare, alternative);
        if (!from_first) {
            sort_rows(descriptor, 0x1049ef6c);
        } else {
            // Position tie-break first: sort by name (descriptor -0x615c), then bubble the league's own
            // rows until a pass makes no swap.
            std::uint32_t swapped = 1;  // r14
            c.set(14, 1);
            sort_rows(c.toc(toc::standings_order_2), 0x1049ec84);
            while (sext8(swapped) != 0) {
                std::uint32_t pass = 0, offset = 0;  // r29, r31
                swapped = 0;
                c.set({{29, 0}, {14, 0}, {31, 0}});
                for (;;) {
                    const auto clubs = count_register();
                    const auto at = sext8(pass);
                    c.set({{3, clubs}, {4, at}});
                    if (!(std::int32_t(at) < std::int32_t(clubs) - 1)) break;
                    const auto base = c.u32(league + league::standings);
                    const auto next = (at + 1) * league::standing_row;  // r30
                    c.set({{4, base + next}, {3, base + offset}, {30, next}});
                    c.link(0x1049ecb0);
                    if (compare_head_to_head(c, base + offset, base + next) == 0) {
                        const auto base_again = c.u32(league + league::standings);
                        c.set({{3, base_again + offset}, {4, base_again + next}});
                        c.link(0x1049ecc8);
                        if (compare_standings(c, base_again + offset, base_again + next) > 0) {
                            const auto upper = base_again + offset, lower = base_again + next;
                            // The two rows exchange every field except the padding at +0x1e, +0x2e, +0x3e
                            // and +0x1f/+0x2f/+0x3f (the original copies field by field).
                            const auto saved = read_row(c, upper);
                            // The registers the swap leaves behind (the sort's next calls and its exit see them).
                            c.set({{13, lha(c, upper + 0x18)}, {4, lha(c, upper + 0x1c)}, {5, c.u32(upper + 0x20)},
                                   {6, c.u8(upper + 0x24)}, {7, c.u8(upper + 0x25)}, {8, c.u8(upper + 0x26)},
                                   {9, c.u8(upper + 0x27)}, {10, lha(c, upper + 0x28)}, {11, lha(c, upper + 0x2a)},
                                   {12, lha(c, upper + 0x2c)}, {27, c.u32(upper + 0x30)}, {26, c.u8(upper + 0x34)},
                                   {25, c.u8(upper + 0x35)}, {24, c.u8(upper + 0x36)}, {23, c.u8(upper + 0x37)},
                                   {22, lha(c, upper + 0x38)}, {21, lha(c, upper + 0x3a)}, {20, lha(c, upper + 0x3c)},
                                   {19, c.u8(upper + 0x40)}, {18, c.u8(upper + 0x41)}, {17, c.u8(upper + 0x42)},
                                   {16, c.u8(upper + 0x43)}, {3, lower}, {14, 1}, {15, c.u8(upper + 0x17)}});
                            copy_row(c, lower, upper);
                            write_row(c, lower, saved);
                            swapped = 1;
                        }
                    }
                    offset += league::standing_row;
                    ++pass;
                    c.set({{31, offset}, {29, pass}});
                }
            }
        }
    }
    if (c.i8(league + competition::owner) == -2) {
        c.set(3, 1);
        return 1;
    }
    if (c.u32(league + table::field::group) != 0) {
        c.set(3, 1);
        return 1;
    }
    // Every row's position byte is its place in the sort; its history byte for the matches played so far
    // (when the row has one) is the same number.
    std::uint32_t counter = 0, offset = 0;  // r6, r4
    c.set({{6, 0}, {4, 0}});
    for (;;) {
        const auto clubs = count_register();
        const auto at = sext8(counter);
        c.set(3, at);
        if (!(std::int32_t(at) < std::int32_t(clubs))) break;
        const auto row = rows + offset;
        c.put8(row + st::position, counter);
        const auto history = c.u32(row + st::history);
        c.set({{3, row}, {5, history}});
        if (history != 0) {
            const auto played = lbz_extsb(c, row + st::totals);
            c.set(3, c.u8(row + st::totals));
            if (std::int32_t(played) > 0) {
                const auto limit = lha(c, league + table::field::played_limit);
                c.set(3, played);
                if (!(std::int32_t(played) > std::int32_t(limit))) c.put8(history + played - 1, counter);
            }
        }
        offset += league::standing_row;
        ++counter;
        c.set({{4, offset}, {6, counter}});
    }
    c.set(3, 1);
    return 1;
}

}
