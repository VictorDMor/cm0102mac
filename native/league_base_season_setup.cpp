// LEAGUE.CPP, block season: the season setup 0x104a00a0 (the derived leagues call it with the league object) and
// its round-robin leaf 0x104a10d0.
//
// The setup lays a round-robin over the standings: a matrix of `n + 1` row pointers (n = the club count rounded up to
// even), each row n words, filled by 0x104a10d0; asks the fixtures block (0x104a4970) for the order of the standings
// unless the competition is one of the two the game gives its own order; builds a fixture record in its frame (the
// 0x50 bytes at sp+0xa4: its stale bytes are read: the byte at sp+0xf3 holds MAIN.CPP's leftovers, CLAUDE.md's
// stack contract) and files one fixture per match: from the league's preset fixtures (+0xe8) when they are of the
// league's year, else from the matrix. Finally it asks 0x104a28d0 to schedule the fixtures of every round of kind 3.
#include "competition_overrides.hpp"
#include "date.hpp"
#include "guest_values.hpp"
#include "league_base_detail.hpp"
#include "league_base_season.hpp"
#include <array>
#include <stdexcept>

namespace cm::aot::league_base::season {
using namespace detail;

void pair_rounds(Cpu& c, std::uint32_t table, std::int32_t teams) {
    const auto row = [&](std::int32_t team) { return c.u32(table + std::uint32_t(team) * 4); };
    const auto cell = [&](std::uint32_t row_address, std::int32_t column) { return row_address + std::uint32_t(column) * 4; };
    // Every row but column 0 starts empty (column 0 is never written: the heap's bytes stay).
    for (std::int32_t team = 1; team <= teams; ++team)
        for (std::int32_t column = 1; column < teams; ++column) c.put32(cell(row(team), column), 0);
    // The registers the original leaves for its caller (r4-r10; it reads r9 and r10 and the checker compares all):
    // the arithmetic of the last loops that ran. Each is set only by the regime that writes it.
    std::uint32_t r6 = std::uint32_t(teams) - 2, r7 = c.r(7), r8 = c.r(8), r10 = c.r(10);
    // The two clubs that meet in the first fixture of every team: team k plays the last team in round
    // `column`, +n for the home side (k in the first half) and -n for the other.
    if (teams > 1) {
        const auto half = (teams + std::int32_t(std::uint32_t(teams) >> 31)) >> 1;
        std::int32_t column = teams - 1;
        for (std::int32_t k = 1; k <= teams - 1; ++k) {
            if (k <= half) {
                c.put32(cell(row(k), column), std::uint32_t(teams));
                c.put32(cell(row(teams), column), 0u - std::uint32_t(k));
                r8 = 0u - std::uint32_t(k);
            } else {
                c.put32(cell(row(k), column), 0u - std::uint32_t(teams));
                c.put32(cell(row(teams), column), std::uint32_t(k));
                r8 = 0u - std::uint32_t(teams);
            }
            r7 = row(teams);
            r10 = std::uint32_t(column) << 2;
            if (column == teams - 2) column = 1;
            else if (column == teams - 1) column = 2;
            else column += 2;
        }
    }
    // The circle method: team m is the pivot of the pass m = n-1 .. 2; every team k meets, in the next free column
    // (cycling 1..n-1 from m-1), the pivot, home and away alternating from the pass's parity.
    if (teams - 1 >= 2) {
        for (std::int32_t pivot = teams - 1; pivot >= 2; --pivot) {
            std::uint32_t parity = std::uint32_t(teams - 1 - pivot) & 1u;
            std::int32_t column = pivot - 1;
            bool assigned_away = false;
            for (std::int32_t k = 1; k <= teams; ++k) {
                const auto own = row(k);
                assigned_away = false;
                if (c.u32(cell(own, column)) == 0) {
                    if (parity == 0) {
                        c.put32(cell(own, column), 0u - std::uint32_t(pivot));
                        c.put32(cell(row(pivot), column), std::uint32_t(k));
                    } else {
                        c.put32(cell(own, column), std::uint32_t(pivot));
                        c.put32(cell(row(pivot), column), 0u - std::uint32_t(k));
                        assigned_away = true;
                    }
                    parity = 1 - parity;
                }
                column = column == teams - 1 ? 1 : column + 1;
                if (pivot == 2 && k == teams) {
                    r8 = assigned_away ? 0u - std::uint32_t(teams) : own;
                    r7 = std::uint32_t(column);
                }
            }
        }
        r6 = table + 4 + std::uint32_t(teams) * 4;
        r10 = 4 + std::uint32_t(teams) * 4;
    }
    c.set({{3, 1}, {6, r6}, {7, r7}, {8, r8}, {9, std::uint32_t(teams) - 1}, {10, r10}});
}

namespace {
// The registers r13-r31 as the original holds them at each of its calls (the allocator, the fixture manager and the
// date routines can reach imports, where a replay compares them): `v[n]` is the original's rN, updated where the
// original writes it, so a register a path has not reached yet is what the original has too (the entry value).
struct Held {
    Cpu& c;
    std::array<std::uint32_t, 32> v{};
    explicit Held(Cpu& cpu) : c(cpu) { for (unsigned n = 13; n < 32; ++n) v[n] = c.r(n); }
    void publish() const { for (unsigned n = 13; n < 32; ++n) c.set(n, v[n]); }
};
// The fixture record the setup builds in its frame at sp+0xa4 (0x50 bytes; the layout is the fixture manager's).
namespace fixture {
inline constexpr std::int32_t competition = 0xa4, word_ac = 0xac, home = 0xb0, away = 0xb4, record = 0xb8,
                              home_club = 0xc0, away_club = 0xc4, year = 0xc8, day = 0xca, season_year = 0xcc,
                              season_day = 0xce, league_year = 0xd0, zero_d4 = 0xd4, code = 0xd6, round_number = 0xd8,
                              league_half_de = 0xda, round_half = 0xdc, kind_c6 = 0xde, byte_df = 0xdf, byte_e1 = 0xe1,
                              byte_e2 = 0xe2, round_kind = 0xe3, home_zero = 0xe4, away_zero = 0xe5, owner = 0xe6,
                              minus_ones = 0xe7, flags = 0xf3;
}
}

bool set_up(Cpu& c, std::uint32_t league_object) {
    // The career's own values (competition_overrides.hpp), before anything here reads the record: only object
    // fields, no register, stack byte or call. Without entries it only reads the object (the host's record of
    // defaults); without a platform it does nothing.
    competition_overrides::at_setup(c.s, league_object, competition_overrides::Kind::league);
    const Frame frame(c, 0x104a00a0, 0x130, {.first_gpr = 19});
    Held h(c);
    std::uint32_t& walker = h.v[19];   // the pointer into the matrix, later the round offset or a counter
    std::uint32_t& counter = h.v[21];
    std::uint32_t& offset = h.v[26];
    std::uint32_t& pairs = h.v[29];     // n
    std::uint32_t& previous = h.v[30];  // the round the draw last gave (-1 at first)
    std::uint32_t& matrix = h.v[31];
    const auto toc_pool = c.toc(toc::pool);
    h.v[24] = c.toc(slot::fixtures);
    h.v[25] = c.toc(slot::fixture_cell);
    h.v[26] = c.toc(slot::flag);
    h.v[20] = c.toc(slot::message);
    h.v[27] = toc_pool;
    h.v[28] = league_object;
    previous = 0xffffffffu;
    const auto clubs = std::int32_t(c.i16(league_object + field::clubs));
    const auto sign = std::uint32_t(clubs) >> 31;
    // The club count rounded up to even (a negative count is rounded toward zero the same way).
    pairs = std::uint32_t(clubs) + (((std::uint32_t(clubs) & 1u) ^ sign) - sign);
    h.publish();
    c.put8(frame.at(0x38), 0);
    c.link(0x104a00f4);
    date::set_default(c, frame.at(0x5c));
    c.put8(league_object + field::byte_e0, 0);
    matrix = invoke(c, entry::allocate_array, 0x104a0108, {(pairs + 1) << 2 & ~3u});
    if (!matrix) {
        h.publish();
        report(c, frame, {0x104a0124, 0x84, 0x54, 0x3fc});
        return false;
    }
    walker = matrix + 4;
    for (counter = 1; std::int32_t(counter) <= std::int32_t(pairs); ++counter, walker += 4) {
        h.publish();
        const auto block = invoke(c, entry::allocate_array, 0x104a0180, {pairs << 2 & ~3u});
        if (!block) {
            report(c, frame, {0x104a019c, 0x64, 0x4c, 0x405});
            invoke(c, entry::release_array, 0x104a01e0, {matrix});
            return false;
        }
        c.put32(walker, block);
    }
    c.set({{3, league_object}, {4, matrix}, {5, pairs}});
    pair_rounds(c, matrix, std::int32_t(pairs));
    // The fixtures block orders the standings, except for the two competitions the game gives its own order.
    const auto record = c.u32(league_object + field::record);
    c.set(4, record);
    bool ordered = false;
    if (record) {
        const auto id = c.u32(record);
        ordered = id == c.u32(c.toc(slot::first_special)) || id == c.u32(c.toc(slot::second_special));
    }
    if (!ordered) {
        c.set({{3, league_object}, {4, pairs}});
        h.publish();
        invoke(c, entry::fixture_order, 0x104a0248, {league_object, pairs});
    }

    // The fixture record: fields written as the original writes them (the other bytes keep what the stack held).
    const auto at = [&](std::int32_t offset_in_frame) { return frame.at(offset_in_frame); };
    const auto record_pointer = c.u32(league_object + field::record);
    auto flags = c.u8(at(fixture::flags));
    flags = std::uint8_t(flags & ~0x20u);
    c.put32(at(fixture::competition), c.u32(record_pointer));
    c.put32(at(fixture::record), record_pointer);
    c.put16(at(fixture::league_year), sext16(c.u16(league_object + field::year)));
    c.put16(at(fixture::zero_d4), 0);
    c.put16(at(fixture::code), c.u16(league_object + field::code));
    c.put8(at(fixture::kind_c6), c.u8(league_object + field::kind_c6));
    c.put16(at(fixture::league_half_de), c.u16(league_object + field::half_de));
    c.put16(at(fixture::round_half), c.u16(league_object + field::round_half));
    for (std::int32_t n = 0; n < 10; ++n) c.put8(at(fixture::minus_ones + n), 0xff);
    c.put32(at(fixture::word_ac), 0xffffffffu);
    c.put8(at(fixture::owner), c.u8(league_object + field::owner));
    c.put8(at(fixture::byte_e1), 0);
    c.put8(at(fixture::byte_e2), 0);
    c.put8(at(fixture::byte_df), 0xff);
    c.put8(at(fixture::flags), flags);
    h.publish();
    virtual_call(c, 0x84, 0x104a02ec, {league_object, at(fixture::competition), c.u8(league_object + field::owner),
                                       0xffffffffu, 0, record_pointer});

    // The last round's date, kept in the record (year +0xc8, day +0xca).
    const auto rounds = c.u32(league_object + field::schedule);
    const auto last_round = rounds + std::uint32_t(std::int32_t(c.i16(league_object + field::schedule_count))) * league::round_row;
    const auto last_day = sext16(c.u16(last_round - 0x4c));
    const auto last_year = (sext16(c.u16(league_object + field::year)) + c.u16(last_round - 0x4a)) & 0xffff;
    c.set({{3, frame.at(0x3c)}, {4, last_day}, {5, last_year}});
    h.publish();
    c.link(0x104a031c);
    date::from_day_and_year(c, frame.at(0x3c), last_day, last_year);
    c.put16(at(0x5e), c.u16(at(0x3e)));
    c.put16(at(0x5c), c.u16(at(0x3c)));
    c.put8(at(0x60), c.u8(at(0x40)));
    c.put16(at(fixture::day), c.u16(at(0x5c)));
    c.put16(at(fixture::year), c.u16(at(0x5e)));

    const auto count_of_clubs = [&] { return std::int32_t(c.i16(league_object + field::clubs)); };
    const auto standings_row = [&](std::int32_t index) { return c.u32(league_object + field::standings) + std::uint32_t(index) * league::standing_row; };
    // The flag byte's two bits the original sets after each fixture: the record's round is the season's last.
    const auto put_flag = [&](std::uint8_t mask, bool on) {
        const auto old = c.u8(at(fixture::flags));
        c.put8(at(fixture::flags), on ? old | mask : old & ~mask);
    };
    // The year of the season's first preset row decides whether the preset fixtures are this season's.
    const auto preset = c.u32(league_object + field::preset_fixtures);
    if (preset && std::int32_t(c.i16(league_object + field::year)) == std::int32_t(c.u16(preset + 2))) {
        // The league's own preset fixtures: round after round, the pairs each of two standings rows' clubs.
        std::uint32_t& round = h.v[21];
        std::uint32_t& pair = h.v[20];
        std::uint32_t& first = h.v[22];
        std::uint32_t& second = h.v[23];
        round = 0;
        offset = 0;
        for (; std::int32_t(round) < std::int32_t(c.i16(league_object + field::schedule_count)); ++round, offset += league::round_row) {
            pair = 0;
            for (;; ++pair) {
                const auto half = std::int32_t(count_of_clubs() + std::int32_t(std::uint32_t(count_of_clubs()) >> 31)) >> 1;
                if (std::int32_t(pair) >= half) break;
                const auto preset_row = c.u32(league_object + field::preset_fixtures) + ((pair + round * std::uint32_t(half)) << 4);
                c.put16(at(fixture::round_number), round);
                first = second = 0xffffffffu;
                for (std::int32_t index = 0; index < count_of_clubs(); ++index) {
                    const auto id = c.u32(c.u32(standings_row(index)));
                    if (c.u32(preset_row + PresetFixture::home) == id) first = std::uint32_t(index);
                    if (c.u32(preset_row + PresetFixture::away) == id) second = std::uint32_t(index);
                }
                if (std::int32_t(first) == -1 || std::int32_t(second) == -1) {
                    h.publish();
                    invoke(c, entry::assert_text, 0x104a0414, {toc_pool + 0x1e, toc_pool, 0x458});
                }
                const auto home_row = standings_row(std::int32_t(first)), away_row = standings_row(std::int32_t(second));
                const auto home_club = c.u32(home_row), away_club = c.u32(away_row);
                c.put32(at(fixture::home_club), home_club);
                c.put32(at(fixture::away_club), away_club);
                c.put8(at(fixture::home_zero), c.i8(home_row + 0x41) == 0);
                c.put8(at(fixture::away_zero), c.i8(away_row + 0x41) == 0);
                c.put32(at(fixture::home), home_club ? c.u32(home_club) : 0xffffffffu);
                c.put32(at(fixture::away), away_club ? c.u32(away_club) : 0xffffffffu);
                h.publish();
                invoke(c, entry::file_fixture, 0x104a04b8,
                       {c.u32(h.v[25]), c.u32(c.u32(league_object + field::schedule) + 0x48 + offset), at(fixture::competition),
                        c.u32(league_object + field::owner_word)});
                const auto base = c.u32(league_object + field::schedule);
                put_flag(0xc0, false);
                c.put16(at(fixture::season_year), sext16(c.u16(league_object + field::year)) + c.u16(base + offset + 2));
                c.put16(at(fixture::season_day), c.u16(base + offset));
                c.put8(at(fixture::round_kind), c.u8(base + offset + 4));
                put_flag(0x10, std::int32_t(c.i16(at(fixture::round_number))) == std::int32_t(c.i16(league_object + field::schedule_count)) - 1 ||
                                   c.i8(base + offset + 0xe) == 4);
                invoke(c, entry::add_fixture, 0x104a054c, {c.u32(h.v[24]), at(fixture::competition), 1});
            }
        }
        if (const auto array = c.u32(league_object + field::preset_fixtures))
            invoke(c, entry::release_array, 0x104a0590, {array - league::array_header});
        c.put32(league_object + field::preset_fixtures, 0);
    } else {
        // Otherwise the matrix gives the fixtures: draw the round (0x104a9600), then each club's opponent in the
        // matrix column of the round.
        std::uint32_t& last_pair = h.v[27];
        std::uint32_t& step = h.v[20];
        std::uint32_t& club = h.v[21];
        std::uint32_t& column = h.v[23];
        std::uint32_t& pointer = h.v[19];
        std::uint32_t& scaled = h.v[22];
        last_pair = pairs - 1;
        for (step = 1; std::int32_t(step) <= std::int32_t(last_pair) * std::int32_t(c.i16(league_object + field::legs)); ++step) {
            // `previous` is the round the draw gives: the round of every match of this step.
            h.publish();
            c.set({{3, previous}, {4, at(0x38)}, {5, c.u32(c.u32(league_object + field::record))}, {6, sext16(c.u16(league_object + field::clubs))},
                   {7, sext16(c.u16(league_object + field::legs))}, {8, sext16(c.u16(league_object + field::schedule_count))},
                   {9, sext16(c.u16(league_object + field::flags))}});
            c.link(0x104a05c8);
            previous = std::uint32_t(next_round(c, previous, at(0x38), c.u32(c.u32(league_object + field::record)),
                                                sext16(c.u16(league_object + field::clubs)), sext16(c.u16(league_object + field::legs)),
                                                sext16(c.u16(league_object + field::schedule_count)), sext16(c.u16(league_object + field::flags))));
            if (std::int32_t(last_pair) == 0) throw std::logic_error("0x104a00a0: n - 1 is zero (unreachable: n is even)");
            const auto step_remainder = std::int32_t(step) % std::int32_t(last_pair);
            const auto in_cycle = std::uint32_t(step_remainder == 0 ? std::int32_t(last_pair) : step_remainder);
            c.put16(at(fixture::round_number), previous);
            column = (in_cycle << 2) & ~3u;
            pointer = matrix + 4;
            offset = 4;
            for (club = 1; std::int32_t(club) <= count_of_clubs(); ++club, pointer += 4, offset += 4) {
                const auto opponent = std::int32_t(c.u32(c.u32(pointer) + column));
                if (opponent == 0) continue;
                // The cycle's parity swaps the sides every pass; so does the season's (owner + year) parity.
                const auto cycles = std::int32_t(step - 1) / std::int32_t(pairs - 1);
                const bool odd_cycle = (cycles & 1) != 0;
                const auto season = std::int32_t(c.i8(league_object + field::owner)) + std::int32_t(c.i16(league_object + field::year));
                const bool odd_season = (season & 1) != 0;
                const auto other_row = c.u32(matrix + (std::uint32_t(opponent > 0 ? opponent : -opponent) << 2));
                c.put32(other_row + column, 0);
                const auto other = std::int32_t(opponent > 0 ? opponent - 1 : -opponent - 1);
                const auto own = std::int32_t(club) - 1;
                // Which of the two plays at home: see the table in the comment above 0x104a0624 of the listing.
                bool own_first;
                if (!odd_cycle) own_first = opponent > 0 ? odd_season : !odd_season;
                else own_first = opponent > 0 ? !odd_season : odd_season;
                const auto first_index = own_first ? own : other, second_index = own_first ? other : own;
                const auto limit = count_of_clubs() - 1;
                if (first_index < 0 || first_index > limit || second_index < 0 || second_index > limit) continue;
                std::uint32_t home_club, away_club;
                bool home_zero, away_zero;
                const auto owner = std::int32_t(c.u32(league_object + field::owner_word));
                const auto rows = c.u32(league_object + field::standings);
                const auto first_row = rows + std::uint32_t(first_index) * league::standing_row,
                           second_row = rows + std::uint32_t(second_index) * league::standing_row;
                bool swapped = false;
                if (owner != -1) {
                    const auto second_club = c.u32(second_row);
                    const auto region = c.u32(second_club + 0x3c);
                    swapped = region != 0 && std::uint32_t(owner) == c.u32(region);
                }
                if (swapped) {
                    home_club = c.u32(second_row);
                    away_club = c.u32(first_row);
                    home_zero = c.i8(second_row + 0x41) == 0;
                    away_zero = c.i8(first_row + 0x41) == 0;
                } else {
                    home_club = c.u32(first_row);
                    away_club = c.u32(second_row);
                    home_zero = c.i8(first_row + 0x41) == 0;
                    away_zero = c.i8(second_row + 0x41) == 0;
                }
                c.put32(at(fixture::home_club), home_club);
                c.put32(at(fixture::away_club), away_club);
                c.put8(at(fixture::home_zero), home_zero);
                c.put8(at(fixture::away_zero), away_zero);
                c.put32(at(fixture::home), home_club ? c.u32(home_club) : 0xffffffffu);
                c.put32(at(fixture::away), away_club ? c.u32(away_club) : 0xffffffffu);
                scaled = previous * league::round_row;
                h.publish();
                invoke(c, entry::file_fixture, 0x104a0958,
                       {c.u32(h.v[25]), c.u32(c.u32(league_object + field::schedule) + 0x48 + scaled), at(fixture::competition),
                        c.u32(league_object + field::owner_word)});
                const auto base = c.u32(league_object + field::schedule);
                put_flag(0xc0, false);
                c.put16(at(fixture::season_year), sext16(c.u16(league_object + field::year)) + c.u16(base + scaled + 2));
                c.put16(at(fixture::season_day), c.u16(base + scaled));
                c.put8(at(fixture::round_kind), c.u8(base + scaled + 4));
                put_flag(0x10, std::int32_t(c.i16(at(fixture::round_number))) == std::int32_t(c.i16(league_object + field::schedule_count)) - 1);
                invoke(c, entry::add_fixture, 0x104a09d8, {c.u32(h.v[24]), at(fixture::competition), 1});
            }
        }
    }
    // The matrix goes: each row, then the array.
    walker = 1;
    h.v[20] = matrix + 4;
    for (; std::int32_t(walker) <= std::int32_t(pairs); ++walker, h.v[20] += 4) {
        h.publish();
        invoke(c, entry::release_array, 0x104a0a18, {c.u32(h.v[20])});
    }
    h.publish();
    invoke(c, entry::release_array, 0x104a0a30, {matrix});
    // The fixtures of every round of kind 3 (a round whose matches the manager already has, inferred) are scheduled.
    h.v[20] = 0;
    walker = 0;
    for (; std::int32_t(h.v[20]) < std::int32_t(c.i16(league_object + field::schedule_count)); ++h.v[20], walker += league::round_row) {
        const auto round = c.u32(league_object + field::schedule) + walker;
        c.set(4, round);
        if (c.u8(round + 0xe) != 3) continue;
        const auto day = sext16(c.u16(round)), year = (sext16(c.u16(league_object + field::year)) + c.u16(round + 2)) & 0xffff;
        c.set({{3, frame.at(0x44)}, {4, day}, {5, year}});
        h.publish();
        c.link(0x104a0a70);
        date::from_day_and_year(c, frame.at(0x44), day, year);
        c.set({{3, league_object}, {4, frame.at(0x44)}});
        h.publish();
        c.link(0x104a0a7c);
        schedule_fixtures(c, league_object, frame.at(0x44));
    }
    c.set(3, 1);
    return true;
}

}
