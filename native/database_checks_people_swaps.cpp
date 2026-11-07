// Fixtures of the people block, part "swaps" (database_checks_people.cpp): 0x100b3710 and 0x100b3c70.
// Worlds are built from a seeded mixer: a club table (0x248-byte rows with 50 person pointers at +0x98), a
// people table (0x70-byte rows), player records, a sample pool whose end is near the cursor (so draws wrap
// often) and a date record. The salt cell holds 16-bit values only (the game's invariant: a negative sum
// would index outside the squad).
#include "database_checks_people_detail.hpp"

namespace cm::aot::database_checks {
namespace {
using namespace people_checks;
constexpr std::uint32_t clubs_cell = data + 0x100, club_count_cell = data + 0x104, people_count_cell = data + 0x108,
                        people_cell = data + 0x10c, date_record = data + 0x180, pool_end_cell = data + 0x1f00,
                        cursor_cell = data + 0x1f10, salt_cell = data + 0x1f20, clubs = data + 0x400,
                        people = data + 0x1000, pool_start = data + 0x2000, players = data + 0x3000;
constexpr std::uint32_t club_stride = 0x248, person_stride = 0x70, player_stride = 0x200, max_people = 16;

void put_toc(Cpu& c, std::int32_t offset, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(offset), value); }

// The pool: a cursor between the start and the end (`end_slots` samples after the start), a salt, random samples.
void pool_world(Cpu& c, Mix& mix, std::uint32_t end_slots) {
    put_toc(c, -0x7b84, cursor_cell);
    put_toc(c, -0x7b90, salt_cell);
    put_toc(c, -0x7b88, pool_end_cell);
    put_toc(c, -0x7b8c, pool_start);
    c.put32(pool_end_cell, pool_start + 2 * end_slots);
    c.put32(cursor_cell, pool_start + 2 * mix.below(end_slots + 1));
    c.put32(salt_cell, mix.below(4) == 0 ? mix.pick<std::uint32_t>({0, 1, 0xffff, 0x7fff, 0x8000}) : mix.below(0x10000));
    constexpr std::uint32_t edges[] = {0, 1, 2, 3, 0xffff, 0x7fff, 0x8000};
    for (std::uint32_t i = 0; i < 0x100; ++i)
        c.put16(pool_start + 2 * i, mix.below(4) == 0 ? edges[mix.below(7)] : mix.below(0x10000));
}

std::uint32_t person_at(std::uint32_t i) { return people + i * person_stride; }
std::uint32_t player_at(std::uint32_t i) { return players + i * player_stride; }

// ---- 0x100b3710 ----
void swap_world(Cpu& c, Mix& mix, std::int32_t club_count, std::int32_t people_count, std::uint32_t person_total,
                std::uint32_t end_slots, bool narrow) {
    put_toc(c, -0x7be8, clubs_cell);
    put_toc(c, -0x7b50, club_count_cell);
    put_toc(c, -0x7be4, people_count_cell);
    put_toc(c, -0x7be0, people_cell);
    c.put32(clubs_cell, clubs);
    c.put32(club_count_cell, std::uint32_t(club_count));
    c.put32(people_count_cell, std::uint32_t(people_count));
    c.put32(people_cell, people);
    pool_world(c, mix, end_slots);
    put_toc(c, -0x7a7c, data + 0x1f40);  // the settling routines' hook word: unset
    c.put32(data + 0x1f40, 0);
    // Ratings are close (within 14 of 100) or, a quarter of the cases, spread over the whole range.
    const bool wide = !narrow && mix.below(2) == 0;
    for (std::uint32_t i = 0; i < max_people; ++i) {
        const auto person = person_at(i), player = player_at(i);
        for (std::uint32_t at = 0; at < person_stride; at += 4) c.put32(person + at, mix.next());
        const bool shared = mix.below(10) == 0 && i > 0;
        c.put32(person + 0x14, mix.below(6) == 0 ? 0 : (shared ? player_at(i - 1) : player));
        c.put8(person + 0x5e, mix.pick<std::uint32_t>({0, 0, 0, 1, 5, 255}));
        // Closeness: the byte at +0x5d is within 4 of its neighbours most of the time.
        c.put8(person + 0x5d, wide ? mix.below(256) : std::uint32_t(std::int32_t(mix.below(8)) - 3));
        // The attribute bytes the settling routines (called for every person with a player record, as C++) walk:
        // -127 ends their slot arrays and would keep them from settling, so it never occurs.
        for (std::uint32_t at = 0; at < player_stride; ++at) {
            const auto byte = mix.next() & 0xff;
            c.put8(player + at, byte == 0x81 ? 0x80 : byte);
        }
        constexpr std::uint32_t odd[] = {0, 0x8000, 0x7fff, 0xffff, 0x7ff0, 0x8010};
        c.put16(player + 4, wide ? (mix.below(5) == 0 ? odd[mix.below(6)] : mix.below(0x10000)) : 100 + mix.below(15));
        c.put8(player + 0x11, mix.below(3) == 0 ? 0x14 : mix.below(256));
    }
    for (std::int32_t club = 0; club < 3; ++club)
        for (std::uint32_t slot = 0; slot < 50; ++slot)
            c.put32(clubs + club * club_stride + 0x98 + 4 * slot,
                    mix.below(8) == 0 ? 0 : person_at(mix.below(person_total)));
}

void add_swap_fixtures(std::vector<Fixture>& out) {
    Mix mix{0x5a9d17};
    // The settling routines (0x100c2cb0, 0x100c2800) run as C++; only their hook 0x103d3f70 is stubbed.
    const std::vector<Answer> answers{{0x103d3f70, {0}, 3}};
    constexpr std::int32_t club_counts[] = {0, 1, 2, 3, -1, 1, 2, 3};
    constexpr std::int32_t people_counts[] = {0, 1, 5, 16, -3, 8};
    constexpr std::uint32_t totals[] = {1, 2, 6, 16, 4, 9};
    constexpr std::uint32_t ends[] = {0, 1, 2, 7, 31, 63, 100};
    for (unsigned n = 0; n < 72; ++n) {
        const auto clubs_n = club_counts[n % 8];
        const auto people_n = people_counts[(n / 3) % 6];
        const auto total = totals[(n / 2) % 6];
        const auto end_slots = ends[mix.below(7)];
        auto draws = script(mix, 6);
        const auto world = mix.next();
        const bool narrow = n % 3 != 0;
        out.push_back(make(0x100b3710,
                           "swap clubs " + std::to_string(clubs_n) + " people " + std::to_string(people_n) + " of " +
                               std::to_string(total) + " pool " + std::to_string(end_slots) + (narrow ? " narrow" : " wide"),
                           draws,
                           [=](Cpu& c) {
                               Mix local{world};
                               swap_world(c, local, clubs_n, people_n, total, end_slots, narrow);
                           },
                           n % 2 == 1, answers));
    }
}

// ---- 0x100b3c70 ----
void counter_world(Cpu& c, Mix& mix, std::int32_t people_count, std::uint32_t end_slots) {
    put_toc(c, -0x7be0, people_cell);
    put_toc(c, -0x7be4, people_count_cell);
    put_toc(c, -0x7b80, date_record);
    put_toc(c, -0x5614, data + 0x1c00);
    c.put32(people_count_cell, std::uint32_t(people_count));
    c.put32(people_cell, people);
    pool_world(c, mix, end_slots);
    constexpr std::uint16_t year = 2000, day = 300;
    c.put16(date_record, year);
    c.put16(date_record + 2, day);
    constexpr std::int32_t ages[] = {-3, 0, 10, 19, 20, 21, 25, 26, 29, 30, 31, 45, 100, 127, 128, 200, 255, 24, 27};
    constexpr std::uint32_t ratings[] = {0x4f, 0x50, 0x51, 0x8b, 0x8c, 0x8d, 0x8000, 0x7fff, 0xffff, 0, 0x60, 0x100, 0x90, 0x8d, 0x200, 0x95};
    constexpr std::uint32_t wide_ratings[] = {0x128d, 0x128e, 0x128f, 0x8000, 0, 0xffff, 0x2000};
    constexpr std::uint32_t closeness[] = {0x14, 0x15, 0x16, 0x30, 0x7f, 0x80, 0, 0x1020, 0xff20, 0xff14, 0xffff, 0x18, 0x40, 0x21};
    for (std::uint32_t i = 0; i < max_people; ++i) {
        const auto person = person_at(i), player = player_at(i);
        for (std::uint32_t at = 0; at < person_stride; at += 4) c.put32(person + at, mix.next());
        for (std::uint32_t at = 0; at < player_stride; at += 4) c.put32(player + at, mix.next());
        c.put32(person + 0xc, mix.below(8) == 0 ? 0 : 1 + mix.below(100));
        c.put32(person + 0x14, mix.below(8) == 0 ? 0 : player);
        const auto e = mix.pick<std::uint32_t>({0, 0, 0, 0, 0, 1, 2, 5, 10, 11, 12, 40, 128, 200, 255});
        c.put8(person + 0x5e, e);
        c.put8(person + 0x5f, mix.pick<std::uint32_t>({0, 1, 2, 3, 4, 5, 6, (e - 1) & 0xff, e, (e + 1) & 0xff, 255, 0}));
        c.put16(person + 0x5c, closeness[mix.below(14)]);
        const auto gap = mix.pick<std::int32_t>({0, 0, 0, 1, -1, 3});
        c.put16(person + 0x34, std::uint32_t(std::int32_t(year) + gap));
        c.put16(person + 0x36, std::uint32_t(day - ages[mix.below(19)]));
        c.put16(player + 4, ratings[mix.below(16)]);
        c.put16(player + 8, wide_ratings[mix.below(7)]);
        c.put8(player + 0x14, mix.below(2) == 0 ? 0x14 : mix.below(256));
        c.put8(player + 0x15, mix.below(2) == 0 ? 0x14 : mix.below(256));
    }
}

void add_counter_fixtures(std::vector<Fixture>& out) {
    Mix mix{0x77c3a1};
    constexpr std::int32_t counts[] = {0, 1, 16, 16, 7, -2, 16, 12, 16};
    constexpr std::uint32_t ends[] = {0, 1, 2, 7, 31, 63, 100};
    for (unsigned n = 0; n < 120; ++n) {
        const auto count = counts[n % 9];
        const auto end_slots = ends[mix.below(7)];
        const auto draws = script(mix, 8);
        const auto world = mix.next();
        out.push_back(make(0x100b3c70,
                           "counters " + std::to_string(count) + " pool " + std::to_string(end_slots), draws,
                           [=](Cpu& c) {
                               Mix local{world};
                               counter_world(c, local, count, end_slots);
                           },
                           n % 2 == 1));
    }
}
}

std::vector<Fixture> people_swaps_fixtures() {
    std::vector<Fixture> out;
    add_swap_fixtures(out);
    add_counter_fixtures(out);
    return out;
}
}
