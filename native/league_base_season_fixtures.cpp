// LEAGUE.CPP, block season: 0x104a28d0, the fixture list of a round's date. Called by the season setup for every
// round of kind 3 and by the daily update (block rollover).
//
// The round the date falls in (found in the league's schedule by the date) has up to eight match slots; each slot of
// a kind 0..2 takes, from the fixture manager's list, the fixture of the matching competition with the best
// strength (a random draw from each club's rating, or a fixed 100/50 for kind 3), and files it with the slot's word
// and its news. The frame is a stack-reader site: its prologue image is read by GAME.CPP, COMP and CUP.CPP, and the
// dates it builds at sp+0x68.. keep stale bytes MAIN.CPP and qsort left that DATE.CPP's 0x10715a30 reads (the
// stack contract in .local/rewrites/league/brief.md), so every date and record the original passes by address lives
// at its frame offset here and nothing else is written to the frame.
#include "date.hpp"
#include "guest_values.hpp"
#include "league_base_detail.hpp"
#include "league_base_season.hpp"
#include <array>
#include <stdexcept>

namespace cm::aot::league_base::season {
using namespace detail;

namespace {
struct Held {
    Cpu& c;
    std::array<std::uint32_t, 32> v{};
    explicit Held(Cpu& cpu) : c(cpu) { for (unsigned n = 13; n < 32; ++n) v[n] = c.r(n); }
    void publish() const { for (unsigned n = 13; n < 32; ++n) c.set(n, v[n]); }
};
// Frame offsets of what the original passes by address.
namespace local {
inline constexpr std::int32_t latest = 0x88, from = 0x80, to = 0x78, today = 0x68, iterator = 0x90,
                              key = 0xc0, record = 0x10c, news = 0x15c;
}
// The fixture manager's record (0x50 bytes), as the list's node and the copy in the frame lay it out (inferred).
namespace node {
inline constexpr std::uint32_t kind_byte = 0x42, home_club = 0x1c, away_club = 0x20, competition = 0x18;
}
}

void schedule_fixtures(Cpu& c, std::uint32_t league_object, std::uint32_t date_argument) {
    const Frame frame(c, 0x104a28d0, 0x2b0, {.first_gpr = 13});
    Held h(c);
    h.v[26] = c.toc(slot::doubles);
    h.v[27] = c.toc(slot::seed);
    h.v[28] = c.toc(slot::start);
    h.v[29] = c.toc(slot::end);
    h.v[30] = c.toc(slot::cursor);
    h.v[13] = c.toc(slot::news);
    h.v[31] = c.toc(slot::fixtures);
    h.v[18] = c.toc(slot::message);
    h.v[17] = c.toc(toc::pool);
    h.v[15] = league_object;
    h.v[16] = date_argument;
    const auto at = [&](std::int32_t offset) { return frame.at(offset); };
    const auto manager = [&] { return c.u32(h.v[31]); };

    // Three dates start as the default: the latest, the window's start and end.
    c.set(3, at(local::latest));
    h.publish();
    c.link(0x104a2914);
    date::set_default(c, at(local::latest));
    c.set(3, at(local::from));
    h.publish();
    c.link(0x104a291c);
    date::set_default(c, at(local::from));
    c.set(3, at(local::to));
    h.publish();
    c.link(0x104a2924);
    date::set_default(c, at(local::to));

    const auto rounds = std::int32_t(c.i16(league_object + field::schedule_count));
    if (rounds == 0) return;
    const auto schedule = c.u32(league_object + field::schedule);
    const auto last_round = schedule + std::uint32_t(rounds) * league::round_row;
    const auto league_year = std::int32_t(c.i16(league_object + field::year));
    const auto limit = std::int32_t(std::uint32_t(league_year) + c.u16(last_round - 0x4a));
    const auto year = std::int32_t(c.u16(date_argument + 2));
    // The date is after the schedule's last round: nothing to do. (The original leaves its last compare's
    // registers: the last round's address in r4 and the date's year in r5.)
    c.set({{4, last_round}, {5, std::uint32_t(year)}});
    if (year > limit) return;
    if (limit == year && std::int32_t(c.i16(date_argument)) > std::int32_t(c.i16(last_round - 0x4c))) return;

    // The round the date is in, by a binary search of the schedule on (year - league year, day).
    c.put16(at(local::key), c.u16(date_argument));
    c.put16(at(local::key + 2), c.u16(date_argument + 2) - std::uint32_t(league_year));
    h.publish();
    const auto row = invoke(c, entry::bsearch, 0x104a299c,
                            {at(local::key), c.u32(league_object + field::schedule), sext16(c.u16(league_object + field::schedule_count)),
                             league::round_row, c.toc(slot::order_4714)});
    h.v[14] = row;
    if (!row || c.i8(row + 0xe) == -1) return;

    // The list the fixture manager's candidates are read from. A search answering 0 empties it.
    h.publish();
    invoke(c, entry::list_start, 0x104a29bc, {at(local::iterator)});
    const auto found = invoke(c, entry::find_fixture, 0x104a29e8,
                              {manager(), c.u16(date_argument + 2), c.u16(date_argument), c.u8(row + 4),
                               c.u32(c.u32(league_object + field::record)), 0xffffffffu, c.u8(league_object + field::owner),
                               at(local::iterator)});
    // The original's `bge` after the record form `rlwinm.` of the result's low half can only take its branch, so
    // the assertion block after 0x104a2a00 (line 0xba0, 0x104a2a04..0x104a2a64) is dead code.
    if ((found & 0xffff) == 0) {
        c.set({{3, at(local::iterator)}, {4, 0xffffffffu}});
        h.publish();
        invoke(c, entry::list_end, 0x104a29fc, {at(local::iterator), 0xffffffffu});
        return;
    }

    // The slots (eight bytes each from row+8): count them up to the first unused one, and sort them when any has a
    // flag in its word (0x7fc0).
    std::int8_t count = 0;
    bool flagged = false;
    for (std::uint32_t offset = 0; count < 8; offset += 8, ++count) {
        const auto slot_at = row + offset;
        if (c.i8(slot_at + 0xe) == -1) break;
        if (c.u32(slot_at + 8) & 0x7fc0u) flagged = true;
    }
    // r5 and r6 hold the count and the flag after the loop (the registers a later callee leaves as it finds them).
    c.set({{5, std::uint32_t(std::int32_t(count))}, {6, flagged ? 1u : 0u}});
    if (flagged) {
        h.publish();
        invoke(c, entry::qsort, 0x104a2acc, {row + 8, std::uint32_t(std::int32_t(count)), 8, c.toc(slot::by_word)});
    }

    const auto pool = Pool::at(c);
    // The inline draw of the original: the cursor steps on, then the sample reduces by the bound. A bound beyond
    // +-65535 would take the scaled arm and call 0x104a3350, but the bounds here are a rating halfword (at most
    // 32768 in size) and a rating plus 1500 (at most 34267), so that arm (0x104a2d1c.., 0x104a2e7c..) cannot run.
    const auto draw = [&](std::int32_t bound, std::uint32_t back) {
        h.publish();  // r20, r21 and r25 as the original holds them at the rand call (0x104a2cf0, 0x104a2e50)
        pool.advance(back);
        if (bound >= 0x10000 || bound <= -0x10000) throw std::logic_error("0x104a28d0: draw bound beyond 65535 (unreachable)");
        return reduce(pool.sample(), bound);
    };

    h.v[22] = 0;
    h.v[24] = 0;
    for (; std::int8_t(h.v[22]) < 8; h.v[22] = h.v[22] + 1, h.v[24] += 8) {
        const auto slot_at = row + h.v[24];
        h.v[23] = slot_at;
        if (c.i8(slot_at + 0xe) == -1) break;
        const auto place = c.i8(slot_at + 0xd);
        if (place < 0 || place > 2) continue;

        // Today, the nearest day of the slot's weekday, and a window of days around today to test the clubs' dates.
        c.set({{3, at(local::today)}, {4, sext16(c.u16(date_argument))}, {5, c.u16(date_argument + 2)}});
        h.publish();
        c.link(0x104a2b14);
        date::from_day_and_year(c, at(local::today), sext16(c.u16(date_argument)), c.u16(date_argument + 2));
        h.publish();
        c.link(0x104a2b28);
        date::nearest_weekday(c, at(local::today), sext8(c.u8(slot_at + 0xc)), at(local::latest));
        // Whether today is before the nearest day: years as unsigned halfwords, then the signed days through the
        // original's branch-free test.
        bool before;
        const auto today_year = c.u16(at(local::today + 2)), nearest_year = c.u16(at(local::latest + 2));
        if (today_year < nearest_year) before = true;
        else if (today_year > nearest_year) before = false;
        else {
            const auto nearest = std::int32_t(c.i16(at(local::latest)));
            const auto difference = std::uint32_t(nearest) ^ std::uint32_t(std::int32_t(c.i16(at(local::today))));
            const auto shifted = std::int32_t(difference) >> 1;
            before = ((std::uint32_t(shifted) - (difference & std::uint32_t(nearest))) >> 31) != 0;
        }
        const auto copy_date = [&](std::int32_t from_offset, std::int32_t to_offset) {
            c.put16(at(to_offset + 2), c.u16(at(from_offset + 2)));
            c.put16(at(to_offset), c.u16(at(from_offset)));
            c.put8(at(to_offset + 4), c.u8(at(from_offset + 4)));
        };
        if (!before) {
            h.publish();
            c.link(0x104a2be8);
            date::subtract_days(c, at(0x50), at(local::today), 5);
            copy_date(0x50, local::from);
            c.set({{3, at(0x48)}, {4, at(local::today)}, {5, 2}, {6, sext16(c.u16(at(0x50)))}});
            h.publish();
            c.link(0x104a2c10);
            date::subtract_days(c, at(0x48), at(local::today), 2);
            copy_date(0x48, local::to);
        } else {
            h.publish();
            c.link(0x104a2b94);
            date::add_days(c, at(0x60), at(local::today), 2);
            copy_date(0x60, local::from);
            c.set({{3, at(0x58)}, {4, at(local::today)}, {5, 5}, {6, sext16(c.u16(at(0x60)))}});
            h.publish();
            c.link(0x104a2bbc);
            date::add_days(c, at(0x58), at(local::today), 5);
            copy_date(0x58, local::to);
        }
        // The list's cursor goes back to its start.
        c.put32(at(local::iterator + 0xc), c.u32(at(local::iterator + 4)));
        h.v[18] = 0;
        h.v[17] = 0xffffffffu;
        for (;;) {
            h.publish();
            const auto candidate = invoke(c, entry::list_next, 0x104a304c, {at(local::iterator)});
            h.v[19] = candidate;  // `or. r19,r3,r3` at 0x104a304c: r19 is 0 when the list is exhausted
            if (!candidate) break;
            if (c.i8(league_object + field::owner) != c.i8(candidate + node::kind_byte)) continue;
            const bool fixed = c.u8(slot_at + 0xe) == 3;
            std::int32_t strength;
            if (fixed) {
                h.publish();
                const auto has = std::int8_t(invoke(c, entry::has_fixtures, 0x104a2c68, {c.u32(candidate + node::home_club)}));
                const auto competition = c.u32(candidate + node::competition);
                const auto nation = competition ? c.u32(competition + 0x10) : 0;
                strength = 0x32;
                if (has != 0 && competition && nation) {
                    const auto wanted = c.u32(nation + 0x18);
                    if (c.u32(c.u32(candidate + node::home_club) + 0x2c) == wanted ||
                        c.u32(c.u32(candidate + node::away_club) + 0x2c) == wanted)
                        strength = 0x64;
                }
                h.v[20] = std::uint32_t(strength);
            } else {
                h.v[20] = sext16(c.u16(c.u32(candidate + node::home_club) + 0x1ec));
                const auto first_bound = std::int32_t(h.v[20]);
                if (first_bound == 0) h.v[21] = 0;
                else h.v[21] = std::uint32_t(draw(first_bound, 0x104a2cf4));
                const auto second_bound = std::int32_t(sext16(c.u16(c.u32(candidate + node::away_club) + 0x1ec)) + 0x5dc);
                h.v[25] = std::uint32_t(second_bound);
                if (second_bound == 0) h.v[20] = 0;
                else h.v[20] = std::uint32_t(draw(second_bound, 0x104a2e54));
                strength = std::int16_t(h.v[21] + h.v[20]);
                h.v[20] = sext16(std::uint32_t(strength));
                // A club that cannot play inside the window scores nothing (0x1048b180 asks the manager: the home club
                // first, the away club only when the home club can).
                const auto blocked = [&](std::uint32_t club, std::uint32_t back) {
                    c.put32(at(0x38), 0xfffffffeu);
                    c.put32(at(0x3c), 0xffffffffu);
                    h.publish();
                    return (invoke(c, entry::date_blocked, back,
                                   {manager(), c.u16(at(local::from)), c.u16(at(local::from + 2)), c.u16(at(local::to)),
                                    c.u16(at(local::to + 2)), c.u32(club), 0, 0xffffffffu}) & 0xffff) != 0;
                };
                if (blocked(c.u32(candidate + node::home_club), 0x104a2fd0) || blocked(c.u32(candidate + node::away_club), 0x104a3020)) {
                    strength = 0;
                    h.v[20] = 0;
                }
            }
            if (std::int16_t(strength) > std::int16_t(h.v[17])) {
                h.v[18] = candidate;
                h.v[17] = std::uint32_t(strength);
            }
        }
        const auto best = h.v[18];
        if (!best) continue;

        // The best fixture, copied into the frame (the record the manager's routines take by address).
        const auto copy_at = at(local::record);
        for (std::uint32_t at_byte = 0; at_byte < 0x24; at_byte += 4) c.put32(copy_at + at_byte, c.u32(best + at_byte));
        for (std::uint32_t at_byte = 0x24; at_byte < 0x3a; at_byte += 2) c.put16(copy_at + at_byte, c.u16(best + at_byte));
        for (std::uint32_t at_byte = 0x3a; at_byte < 0x4e; ++at_byte) c.put8(copy_at + at_byte, c.u8(best + at_byte));
        c.put16(copy_at + 0x4e, c.u16(best + 0x4e));
        if (c.u8(slot_at + 0xe) != 3) {
            // The news item 3: twelve fields from the record and the slot.
            const auto item = at(local::news);
            h.publish();
            invoke(c, entry::news_start, 0x104a31c0, {item, 3, 0});
            const std::uint32_t fields[12] = {c.u32(at(0x118)), c.u32(at(0x11c)), c.u32(at(0x10c)), sext16(c.u16(at(0x13e))),
                                              sext16(c.u16(at(0x13c))), c.u16(at(0x136)), c.u16(at(0x134)), c.u8(at(0x14b)),
                                              sext16(c.u16(at(local::latest))), c.u16(at(local::latest + 2)),
                                              sext8(c.u8(slot_at + 0xd)), sext8(c.u8(slot_at + 0xe))};
            static constexpr std::uint32_t backs[12] = {0x104a31d0, 0x104a31e0, 0x104a31f0, 0x104a3200, 0x104a3210, 0x104a3220,
                                                        0x104a3230, 0x104a3240, 0x104a3250, 0x104a3260, 0x104a3274, 0x104a3288};
            for (std::uint32_t n = 0; n < 12; ++n) invoke(c, entry::news_field, backs[n], {item, n, fields[n]});
            invoke(c, entry::news_file, 0x104a3298, {c.u32(h.v[13]), item, c.u32(at(0x128))});
            invoke(c, entry::news_file, 0x104a32a8, {c.u32(h.v[13]), item, c.u32(at(0x12c))});
        }
        invoke(c, entry::remove_fixture, 0x104a32b4, {manager(), copy_at});
        c.put16(at(0x136), c.u16(at(local::latest)));
        c.put16(at(0x134), c.u16(at(local::latest + 2)));
        c.put8(at(0x14b), c.u8(slot_at + 0xd));
        c.put8(at(0x147), c.u8(slot_at + 0xe));
        invoke(c, entry::file_fixture, 0x104a32ec,
               {c.u32(c.toc(slot::fixture_cell)), c.u32(slot_at + 8), copy_at, c.u32(league_object + field::owner_word)});
        invoke(c, entry::add_fixture, 0x104a32fc, {manager(), copy_at, 1});
        c.put8(slot_at + 0xe, 0xff);
        invoke(c, entry::list_remove, 0x104a3310, {at(local::iterator), h.v[18]});
    }
    h.publish();
    invoke(c, entry::list_end, 0x104a3330, {at(local::iterator), 0xffffffffu});
}

}
