// LEAGUE.CPP, block `rollover`: the end of the season, the league's virtual +0x9c (league_base_rollover.hpp).
// The original decides every club's result in three passes over the standings (places, promotion and relegation
// slots, the best place a club can still reach), each in char/halfword arithmetic that wraps; the glue calls to the
// virtual +0x48 and +0x70 read r9/r10, which the passes leave behind, so those registers (and r19, the played-matches
// byte of the last row compared) are set where the passes define them.
#include "league_base_detail.hpp"
#include "league_base_rollover.hpp"

namespace cm::aot::league_base {
namespace {
namespace cell {
inline constexpr std::int32_t
    database = -0x7b7c,        // cell: the database object 0x1007fe90 acts on (inferred)
    ratings_a = -0x7a4c,       // cells whose words the table of places stores (inferred): 0x2628, 0x25fc
    ratings_b = -0x7a30,       //   the words stored for the best and the next places
    places_source = -0x7a48,   // cell: the object 0x104b7e30 asks for the limits of a place (inferred)
    kind_b = -0x7a70,          // cell: a statistics kind the club's competition is compared with (inferred)
    kind_a = -0x7a1c,          //   likewise
    place_table = -0x7984;     // cell: the table 0x104a2570 reads and stores in (8 bytes per club id; -1: unset)
}
namespace field {
inline constexpr std::uint32_t
    rules = 0xc0,              // four bytes: automatic promotion, playoff clubs, byte_c2, relegation
    win_points = 0xc4,
    flags = 0xdc,
    finished_flag = 0xe0,
    prize_done = 0xe6;
}
namespace row_field {
inline constexpr std::uint32_t played = 0x14, points = 0x1c, outcome = 0x40, place = 0x42, best = 0x43;
}
// The outcomes (the standings row's byte +0x40; inferred from the transitions below): -1 undecided, 0 relegation
// playoff?, 1, 2, 3 promoted, 5 champion, 6, 7, -2, -4.
std::int32_t s8(std::uint32_t value) { return std::int8_t(value); }
std::int32_t s16(std::uint32_t value) { return std::int16_t(value); }
}

void end_of_season(Cpu& c, std::uint32_t league) {
    const Frame frame(c, 0x104a1c30, 0xa0, {.first_gpr = 19});
    const auto database = c.toc(cell::database);
    c.set({{25, c.toc(cell::ratings_a)}, {22, league}, {26, c.toc(cell::ratings_b)}, {23, 0xffffffffu}, {27, c.toc(cell::places_source)},
           {28, c.toc(cell::kind_b)}, {29, c.toc(cell::kind_a)}, {30, c.toc(cell::place_table)}, {31, database},
           {20, c.toc(toc::message)}, {21, c.toc(toc::pool)}});
    const auto clubs = [&] { return s16(c.u16(league + competition::club_count)); };
    const auto rows = [&] { return c.u32(league + league::standings); };
    if (c.i16(league + league::half_a4) == -1 && clubs() > 0 && sext8(c.u8(league + competition::byte_45)) != 0)
        c.put16(league + league::half_a4, std::uint32_t(clubs() - 1) * std::uint32_t(c.i16(league + competition::rounds)));
    if (sext8(c.u8(league + field::finished_flag)) != 0) return;
    invoke(c, 0x1049eb30, 0x104a1cb8, {league, 0});
    const auto array = invoke(c, 0x10775130, 0x104a1cc0, {sext16(c.u16(league + competition::club_count))});
    c.set(24, array);
    if (!array) {
        report(c, frame, {0x104a1cdc, 0x48, 0x40, 0x7c9});
        return;
    }
    invoke(c, 0x10778300, 0x104a1d2c, {array, 0xffffffffu, sext16(c.u16(league + competition::club_count))});
    invoke(c, 0x1077fc28, 0x104a1d3c, {league + field::rules, frame.at(0x3c), 4});
    c.set(2, c.u32(c.sp() + 0x14));
    const auto counts = [&](std::int32_t index) { return frame.at(0x3c) + std::uint32_t(index); };
    // Without a decided champion the first club that is not already placed is the "champion" (r23 as a char).
    std::int32_t champion = -1;
    if ((c.i16(league + field::flags) & 1) != 0) {
        std::uint32_t offset = 0;
        for (std::int32_t index = 0; s16(std::uint32_t(index)) < clubs(); ++index, offset += 0x44) {
            const auto outcome = s8(c.u8(rows() + offset + row_field::outcome));
            if (outcome != -2 && outcome != -4 && outcome != 3) { champion = s8(std::uint32_t(index)); c.set(23, std::uint32_t(champion)); break; }
        }
    }
    // Pass 1: from the top, the first clubs get the first two place groups (counts 0 and 1) that are not placed yet.
    {
        std::int32_t r4 = 0, r5 = 0;
        std::uint32_t r9 = 0;
        while (s16(std::uint32_t(r4)) < clubs() && s8(std::uint32_t(r5)) <= 1) {
            std::uint32_t r8 = r9;
            const auto group = s8(std::uint32_t(r5));
            while (s8(c.u8(counts(group))) != 0 && s16(std::uint32_t(r4)) < clubs()) {
                const auto outcome = s8(c.u8(rows() + r8 + row_field::outcome));
                if (outcome != -2 && outcome != -4 && outcome != 3) {
                    c.put8(array + std::uint32_t(s16(std::uint32_t(r4))), std::uint32_t(r5));
                    c.put8(counts(group), c.u8(counts(group)) - 1u);
                }
                r8 += 0x44; r9 += 0x44; ++r4;
            }
            ++r5;
        }
        c.set(9, r9);
        // Pass 2: from the bottom, the groups 3 and 2.
        r4 = s16(std::uint32_t(clubs() - 1));
        r5 = 3;
        r9 = std::uint32_t(r4) * 0x44u;
        while (s16(std::uint32_t(r4)) >= 0 && s8(std::uint32_t(r5)) >= 2) {
            std::uint32_t r8 = r9;
            const auto group = s8(std::uint32_t(r5));
            while (s8(c.u8(counts(group))) != 0 && s16(std::uint32_t(r4)) >= 0) {
                const auto outcome = s8(c.u8(rows() + r8 + row_field::outcome));
                if (outcome != 0 && outcome != 1 && outcome != 5) {
                    c.put8(array + std::uint32_t(s16(std::uint32_t(r4))), std::uint32_t(r5));
                    c.put8(counts(group), c.u8(counts(group)) - 1u);
                }
                r8 -= 0x44; r9 -= 0x44; --r4;
            }
            --r5;
        }
        c.set(9, r9);
    }
    // Pass 3: every club whose place and best place differ gets its place by the clubs above it, and its best place
    // from the clubs below it, by points that can still be reached.
    std::uint32_t row_offset = 0;
    for (std::int32_t index = 0; s16(std::uint32_t(index)) < clubs(); ++index, row_offset += 0x44) {
        const auto row = rows() + row_offset;
        if (s8(c.u8(row + row_field::place)) == s8(c.u8(row + row_field::best))) continue;
        const std::int32_t at = s16(std::uint32_t(index));
        const std::int32_t number = at + 1;
        c.put8(row + row_field::place, std::uint32_t(number));
        c.set(10, 0);
        const auto win = s8(c.u8(league + field::win_points));
        const std::int32_t target = c.i16(league + league::half_a4);
        const std::int32_t played = s8(c.u8(rows() + row_offset + row_field::played));
        const std::uint32_t potential =
            std::uint32_t(c.i16(rows() + row_offset + row_field::points)) + std::uint32_t(win) * (std::uint32_t(target) - std::uint32_t(played));
        c.set(9, potential);
        std::uint32_t other_offset = 0;
        std::int32_t compared = 0;
        for (; s16(std::uint32_t(compared)) < at; ++compared, other_offset += 0x44) {
            const auto all = rows();
            const auto this_row = row_offset + all, other = all + other_offset;
            const std::int32_t limit = c.i16(league + league::half_a4);
            const std::int32_t mine = s8(c.u8(this_row + row_field::played));
            c.set(19, std::uint32_t(mine));
            const std::int32_t theirs = s8(c.u8(other + row_field::played));
            const std::int32_t their_points = c.i16(other + row_field::points);
            bool lower;
            if (mine < limit && theirs < limit) lower = !(std::int32_t(potential) < their_points);
            else if (theirs < limit) lower = std::int32_t(potential) == their_points;
            else if (mine < limit) lower = !(std::int32_t(potential) < their_points);
            else lower = std::int32_t(potential) == their_points && sext8(c.u8(league + field::prize_done)) != 0;
            if (lower) c.put8(this_row + row_field::place, c.u8(this_row + row_field::place) - 1u);
            c.set(10, std::uint32_t(compared) + 1);
        }
        c.put8(rows() + row_offset + row_field::best, std::uint32_t(number));
        std::int32_t r6 = s8(std::uint32_t(number));
        std::uint32_t scan_offset = std::uint32_t(r6) * 0x44u;
        for (; s16(std::uint32_t(r6)) < clubs(); ++r6, scan_offset += 0x44) {
            const auto all = rows();
            const auto other = all + scan_offset, this_row = all + row_offset;
            const std::int32_t other_played = s8(c.u8(other + row_field::played));
            const std::int32_t limit = c.i16(league + league::half_a4);
            const std::int32_t their_points = c.i16(other + row_field::points);
            const std::uint32_t reach = std::uint32_t(their_points) +
                                        std::uint32_t(s8(c.u8(league + field::win_points))) * (std::uint32_t(limit) - std::uint32_t(other_played));
            c.set({{10, std::uint32_t(limit)}, {9, std::uint32_t(their_points)}});
            const std::int32_t this_played = s8(c.u8(this_row + row_field::played));
            const std::int32_t mine = c.i16(this_row + row_field::points);
            bool better;
            if (this_played < limit && other_played < limit) better = !(std::int32_t(reach) < mine);
            else if (other_played < limit) better = !(std::int32_t(reach) < mine);
            else if (this_played < limit) better = std::int32_t(reach) == mine;
            else better = std::int32_t(reach) == their_points && sext8(c.u8(league + field::prize_done)) != 0;
            if (better) c.put8(this_row + row_field::best, c.u8(this_row + row_field::best) + 1u);
        }
    }
    // The decisions, club by club.
    std::int32_t champion_next = s8(std::uint32_t(champion)) + 1;
    c.set({{23, std::uint32_t(champion_next)}, {21, 0}, {20, 0}});
    std::uint32_t offset = 0;
    for (std::uint32_t index = 0; s16(index) < clubs(); ++index, offset += 0x44) {
        const auto row = rows() + offset;
        const auto notify = [&](std::uint32_t outcome, std::uint32_t back) {
            virtual_call(c, 0x48, back, {league, c.u32(rows() + offset), outcome, c.u8(league + competition::owner), 0, 0});
        };
        const std::int32_t best = s8(c.u8(row + row_field::best));
        if (champion_next == best) {
            const auto status = s8(c.u8(row + row_field::outcome));
            if (status != 3 && status != -2 && status != -4 && status != 5) {
                c.put8(row + row_field::outcome, 5);
                notify(5, 0x104a21c8);
            }
        } else {
            const auto group = s8(c.u8(array + std::uint32_t(best - 1)));
            if (group == 0) {
                const auto status = s8(c.u8(row + row_field::outcome));
                if ((status == -1 || status == 1) && status != -4 && status != -2) {
                    const auto flags = std::uint32_t(c.i16(league + field::flags));
                    if ((flags & 2) != 0) {
                        if (status != 0) { c.put8(row + row_field::outcome, 0); notify(0, 0x104a2250); }
                    } else if ((flags & 4) != 0) {
                        if (status != 7) { c.put8(row + row_field::outcome, 7); notify(7, 0x104a2298); }
                    } else if ((flags & 8) != 0) {
                        if (status != 6) { c.put8(row + row_field::outcome, 6); notify(6, 0x104a22e0); }
                    }
                }
            } else if (group == 1) {
                if (s8(c.u8(row + row_field::outcome)) == -1) { c.put8(row + row_field::outcome, 1); notify(1, 0x104a233c); }
            } else {
                const auto next = s8(c.u8(array + std::uint32_t(s8(c.u8(row + row_field::place)) - 1)));
                if (next == 3) {
                    invoke(c, 0x1007fe90, 0x104a2370, {c.u32(database), c.u32(row), c.u32(league + competition::record)});
                    const auto status = s8(c.u8(rows() + offset + row_field::outcome));
                    if (status == -1 || status == 2 || status == -4) {
                        c.put8(rows() + offset + row_field::outcome, 3);
                        notify(3, 0x104a23c8);
                    }
                } else if (next == 2) {
                    const auto status = s8(c.u8(row + row_field::outcome));
                    if (status == -1 || status == -2) { c.put8(row + row_field::outcome, 2); notify(2, 0x104a2420); }
                }
            }
        }
        // A club in the middle slots, none of them still reachable: the playoff-less outcome -2.
        if ((c.i16(league + field::flags) & 0x20) != 0) {
            const auto here = rows() + offset;
            const auto promoted = s8(c.u8(league + field::rules)), middle = s8(c.u8(league + field::rules + 2));
            const auto place = s8(c.u8(here + row_field::place));
            if (place > promoted + middle) {
                const auto relegated = s8(c.u8(league + field::rules + 3));
                const auto best_place = s8(c.u8(here + row_field::best));
                if (!(best_place > (clubs() - relegated) - middle)) {
                    std::int32_t scan = s16(std::uint32_t(place - 1));
                    std::int32_t all_open = 1;
                    for (; s16(std::uint32_t(scan)) < best_place && s8(std::uint32_t(all_open)) != 0; ++scan)
                        if (s8(c.u8(array + std::uint32_t(s16(std::uint32_t(scan))))) != -1) all_open = 0;
                    if (s8(std::uint32_t(all_open)) != 0 && s8(c.u8(here + row_field::outcome)) == -1) {
                        c.put8(here + row_field::outcome, 0xfe);
                        notify(0xfffffffeu, 0x104a250c);
                    }
                }
            }
        }
        // A club of a top place group whose competition has no entry yet gets a rating word by its place.
        {
            const auto here = rows() + offset;
            const auto place = s8(c.u8(here + row_field::place));
            if (place == s8(c.u8(here + row_field::best)) && place < 9 && s8(c.u8(league + competition::owner)) == -1 &&
                sext8(invoke(c, 0x104646d0, 0x104a2554, {c.u32(c.u32(league + competition::record))})) != 0 &&
                c.i32(league + competition::parent) == -1) {
                const auto table = c.u32(c.toc(cell::place_table));
                const auto club = c.u32(rows() + offset);
                if (c.i32(table + (c.u32(club) << 3)) == -1) {
                    const auto record = c.u32(league + competition::record);
                    if (const auto stats = c.u32(record + 8); stats != 0 && c.u32(stats + 0x68) != c.u32(c.toc(cell::kind_a))) {
                        if (const auto owner = c.u32(stats + 0x74); owner != 0 && c.u32(owner + 8) == c.u32(c.toc(cell::kind_b))) {
                            invoke(c, 0x104b7e30, 0x104a25d4, {c.u32(c.toc(cell::places_source)), stats, frame.at(0x39), frame.at(0x38)});
                            const auto at = rows() + offset;
                            const auto limit_a = s8(c.u8(frame.at(0x39)));
                            const auto place_now = s8(c.u8(at + row_field::place));
                            const auto slot = c.u32(c.u32(at)) << 3;
                            if (place_now <= limit_a) c.put32(table + slot, c.u32(c.toc(cell::ratings_b)));
                            else if (place_now <= s8(c.u8(frame.at(0x38))) + limit_a - 1) c.put32(table + slot, c.u32(c.toc(cell::ratings_a)));
                        }
                    }
                }
            }
        }
        {
            const auto here = rows() + offset;
            virtual_call(c, 0x70, 0x104a2670,
                         {league, c.u32(here), c.u8(league + competition::owner), (index + 1) & 0xffu, c.u8(here + row_field::place),
                          c.u8(here + row_field::best)});
        }
        c.set({{20, offset + 0x44}, {21, index + 1}});
    }
    invoke(c, 0x10775170, 0x104a2694, {array});
}
}
