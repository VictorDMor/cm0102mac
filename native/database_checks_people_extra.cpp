// Fixtures of the people block, part "extra" (database_checks_people.cpp): 0x100a3db0, the non-playing staff
// generated for clubs. The real 0x100c7d00, 0x100ce320, 0x100c4400, 0x100c4cf0 and 0x100c53e0 run (their worlds
// are copied from database_checks_records.cpp and database_checks_generation.cpp); the assert calls, the
// formatter and 0x107158a0 are stubbed and record their arguments. A stub of 0x107158a0 can set r31 (the number
// of people generated) to 0x5207, which reaches the four "20,000 people" asserts.
#include "database_checks_people_detail.hpp"

namespace cm::aot::database_checks {
namespace {
using namespace people_checks;

// Fixture memory (data +): nation rows 0x1000, TOC cells 0x1300-0x1800, pool 0x2000, people 0x4000, names 0x8000,
// the 0x128-byte club-like table of the category switch 0x10000, the unmarked-record table 0x12000, the
// 0x100ce320 code cells 0x13400, clubs 0x14000, their linked records 0x16000, list entries 0x1c000.
constexpr std::uint32_t rows = data + 0x1000, names = data + 0x8000, category_clubs = data + 0x10000,
                        unmarked = data + 0x12000, code_cells = data + 0x13400, clubs = data + 0x14000,
                        linked_records = data + 0x16000, entries = data + 0x1c000, people = data + 0x4000;
constexpr std::uint32_t pool_start = data + 0x2000, pool_end_cell = data + 0x1f00, cursor_cell = data + 0x1f10,
                        salt_cell = data + 0x1f20;
constexpr std::uint32_t club_count_cell = data + 0x1340, nation_cell = data + 0x1344, clubs_cell = data + 0x1348,
                        people_cell = data + 0x134c, people_count_cell = data + 0x1350, success_cell = data + 0x1360,
                        text_pool = data + 0x1500, message = data + 0x1600, result_buffer = data + 0x1700;
constexpr std::uint32_t club_stride = 0x248, linked_stride = 0x300;
constexpr std::uint32_t club_table_slots = 10, nations = 12;

constexpr std::int32_t category_slots[] = {-0x7584, -0x7588, -0x74d8, -0x74e0, -0x7a28, -0x7594, -0x7a00, -0x7af4,
                                           -0x7598, -0x759c, -0x75a0, -0x7a14, -0x7a24, -0x75a8, -0x79fc, -0x75ac,
                                           -0x7504, -0x7a18, -0x75b0, -0x75b4, -0x75b8, -0x75bc, -0x75c0, -0x7514,
                                           -0x75c4, -0x750c, -0x74e8, -0x75a4};

// The name tables the real pickers read (0x100c4400, 0x100c4cf0): as add_category_picker of the generation fixtures.
void name_world(Cpu& c, Mix& local, std::int32_t category) {
    constexpr std::uint32_t index_cells = data + 0x1400, category_count_cell = data + 0x1300,
                            category_table_cell = data + 0x1304;
    constexpr std::array<std::int32_t, 10> counts{0, 3, 5, 6, 10, 20, 50, 100, 101, 200};
    auto put = [&](std::int32_t at, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(at), value); };
    put(-0x7580, data + 0x100);
    c.put32(data + 0x100, rows);
    const auto bound = local.below(400);
    for (const std::int32_t slot : {-0x7550, -0x754c}) put(slot, data + 0x110);
    c.put32(data + 0x110, bound);
    put(-0x752c, data + 0x120);
    put(-0x7530, data + 0x128);
    c.put32(data + 0x128, names);
    c.put32(data + 0x120, names);
    put(-0x7b04, category_count_cell);
    c.put32(category_count_cell, 1 + local.below(club_table_slots));
    put(-0x7b98, category_table_cell);
    c.put32(category_table_cell, category_clubs);
    put(-0x7590, data + 0x1380);
    put(-0x758c, data + 0x1384);
    c.put32(data + 0x1380, local.below(club_table_slots));
    c.put32(data + 0x1384, local.below(club_table_slots));
    unsigned k = 0;
    for (const auto slot : category_slots) {
        put(slot, index_cells + 4 * k);
        c.put32(index_cells + 4 * k, local.below(club_table_slots));
        ++k;
    }
    for (unsigned nation = 0; nation < nations; ++nation) {
        c.put32(rows + nation * 0x24 + 0, local.below(20));
        c.put32(rows + nation * 0x24 + 4, local.below(20));
        c.put32(rows + nation * 0x24 + 0x18, counts[local.below(counts.size())]);
        c.put32(rows + nation * 0x24 + 0x1c, counts[local.below(counts.size())]);
    }
    for (std::uint32_t i = 0; i < 500; ++i) {
        c.put8(names + 0x2c * i + 0x28, local.below(256));
        const auto length = local.below(6);
        for (std::uint32_t j = 0; j < length; ++j) c.put8(names + 0x2c * i + j, local.below(5) == 0 ? '.' : 'a' + local.below(5));
        c.put8(names + 0x2c * i + length, 0);
    }
    for (std::uint32_t i = 1; i <= 100; ++i) c.put8(names - 0x2c * i + 0x28, local.below(256));
    for (unsigned i = 0; i < club_table_slots; ++i) {
        c.put32(category_clubs + i * 0x128 + 0x68, local.below(nations));
        c.put8(category_clubs + i * 0x128 + 0x119, local.below(2) ? std::uint32_t(category) : local.below(40));
    }
}

// The sample pool of the generator's inline draws and the table 0x100c53e0 picks from.
void pool_world(Cpu& c, Mix& local) {
    auto put = [&](std::int32_t at, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(at), value); };
    put(-0x7b84, cursor_cell);
    put(-0x7b90, salt_cell);
    put(-0x7b88, pool_end_cell);
    put(-0x7b8c, pool_start);
    c.put32(pool_end_cell, pool_start + 0x80);
    const std::array<std::uint32_t, 4> cursors{pool_start, pool_start + 0x7c, pool_start + 0x7e, pool_start + 0x80};
    c.put32(cursor_cell, cursors[local.below(4)]);
    constexpr std::array<std::uint32_t, 8> salts{0, 1, 2, 0x7fff, 0xffff, 0x80000000u, 0xfffffff0u, 0xfffffffeu};
    c.put32(salt_cell, local.below(3) == 0 ? salts[local.below(8)] : local.below(0x8000));
    for (std::uint32_t i = 0; i < 0x50; ++i) c.put16(pool_start + 2 * i, local.below(0x10000));
    // 0x100c53e0: a table of 0x2c-byte records, with or without a record whose first byte is 0.
    put(-0x7554, data + 0x100 + 0x40);
    c.put32(data + 0x140, std::uint32_t(std::array<std::int32_t, 6>{0, 1, 5, 30, 100, -7}[local.below(6)]));
    put(-0x7534, data + 0x144);
    c.put32(data + 0x144, unmarked);
    for (std::uint32_t i = 0; i < 100; ++i) c.put8(unmarked + 0x2c * i, 1 + local.below(255));
    if (local.below(2)) c.put8(unmarked + 0x2c * local.below(5), 0);
    const auto states = local.below(4);
    c.put8(c.r(2) - 0x39e9, states & 1);
    c.put32(c.r(2) - 0x39e8, (states >> 1) ? local.below(20) : 0xffffffffu);
}

// 0x100ce320's code words (as database_checks_records.cpp): one pair's code word is 5 and its index word 6, so a
// club whose first word is 5 has club 6 (found byte 1) and one whose word is 6 has club 5 (found 0).
void code_world(Cpu& c) {
    // The 82 pairs' TOC offsets (database_records_ce320.hpp): code -0x76ec/-0x782c ... every TOC slot the table
    // reads gets a cell; those cells hold distinct non-matching values except the first pair.
    constexpr std::int32_t first_code = -0x76ec, first_index = -0x782c;
    std::uint32_t next = 0;
    const auto cell = [&](std::int32_t at, std::uint32_t value) {
        c.put32(c.r(2) + std::uint32_t(at), code_cells + 4 * next);
        c.put32(code_cells + 4 * next, value);
        ++next;
    };
    // All words the generated routine can read lie in these two TOC ranges (-0x7968..-0x75cc), set to distinct values.
    for (std::int32_t at = -0x7968; at <= -0x75cc; at += 4) {
        if (at == first_code) cell(at, 5);
        else if (at == first_index) cell(at, 6);
        else cell(at, 0x1000 + next);
    }
    for (unsigned k = 0; k < 3; ++k) {
        constexpr std::int32_t owner_types[3] = {-0x7a18, -0x7af4, -0x79fc};
        c.put32(c.r(2) + std::uint32_t(owner_types[k]), data + 0x1800 + 4 * k);
        c.put32(data + 0x1800 + 4 * k, 7 + k);
    }
}

constexpr std::array<std::int32_t, 16> ratings{-32768, -600, -500, 0, 500, 501, 1000, 2000, 5000, 7499, 7500, 7501,
                                               15000, 20000, 30000, 32767};

// The world of one case: the clubs (with their linked records and lists), the people table and the shared words.
void world(Cpu& c, std::uint32_t seed, std::uint32_t club_count, std::uint32_t person_count, unsigned mode) {
    Mix local{seed};
    const std::int32_t category = std::int32_t(local.below(38)) - 3;
    name_world(c, local, category);
    pool_world(c, local);
    code_world(c);
    const auto put = [&](std::int32_t at, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(at), value); };
    // Words the routine itself reads.
    put(-0x7a10, nation_cell);
    put(-0x7be8, clubs_cell);
    c.put32(clubs_cell, clubs);
    put(-0x7be0, people_cell);
    c.put32(people_cell, people);
    put(-0x7be4, people_count_cell);
    c.put32(people_count_cell, person_count);
    put(-0x7b50, club_count_cell);
    c.put32(club_count_cell, club_count);
    put(-0x7bac, success_cell);
    c.put8(success_cell, 1);
    put(-0x7ba8, message);
    put(-0x5618, text_pool);
    put(-0x7bb8, result_buffer);
    // c7d00's range words: the low word is the category world's club-count cell (-0x7b04).
    c.put32(data + 0x1300, local.pick<std::uint32_t>({0, 0, 1, 1, 2, 3}));
    const std::array<std::uint32_t, 5> nation_values{7, 8, 9, 6, 5};
    const auto player_nation = nation_values[local.below(5)];
    c.put32(nation_cell, player_nation);
    // The clubs. Their first word is the code 0x100ce320 maps and 0x100c7d00 ranges.
    constexpr std::array<std::uint32_t, 14> words{0, 0, 0, 1, 2, 3, 4, 5, 6, 7, 8, 20, 0x80000000u, 0xffffffffu};
    for (unsigned i = 0; i < 8; ++i) {
        const auto club = clubs + i * club_stride, linked = linked_records + i * linked_stride;
        for (std::uint32_t at = 0; at < club_stride; at += 4) c.put32(club + at, 0);
        c.put32(club, words[local.below(words.size())]);
        c.put32(club + 0x2c, local.below(10) == 0 ? 0 : linked);
        c.put32(club + 0x30, local.below(4) == 0 ? 0 : linked);
        c.put32(club + 0x90, local.below(3) == 0 ? 7 : 0);
        c.put16(club + 0x1ec, std::uint32_t(ratings[local.below(ratings.size())]));
        c.put8(club + 0x244, local.pick<std::uint32_t>({0, 1, 1, 1, 0x80, 0xff}));
        c.put32(linked + 0x68, local.below(2) ? player_nation : nation_values[local.below(5)]);
        c.put8(linked + 0x119, local.below(2) ? std::uint32_t(category) : local.below(40));
        c.put8(linked + 0x120, local.pick<std::uint32_t>({0, 1, 1, 1, 0x80}));
        // The lists: empty slots, or entries (a full list when the case wants none made).
        const std::uint32_t fill = local.below(3);
        for (const auto [offset, slots] : {std::pair{0x160u, 5u}, {0x174u, 7u}, {0x190u, 3u}})
            for (std::uint32_t k = 0; k < slots; ++k)
                c.put32(club + offset + 4 * k, fill == 0 || local.below(fill == 1 ? 3 : 8) == 0 ? entries + 4 * k : 0);
    }
    // The people table: random bytes, some role 8 rows owned by a club that lists them (mode 0), one that does not
    // (mode 1, the assert of the first pass).
    for (std::uint32_t i = 0; i < person_count && i < 150; ++i) {
        const auto row = people + i * 0x70;
        for (std::uint32_t at = 0; at < 0x70; at += 4) c.put32(row + at, local.next());
        const bool role8 = local.below(3) == 0;
        c.put8(row + 0x62, role8 ? 8 : local.below(12));
        c.put32(row + 0x24, 0);
        if (role8 && local.below(3) != 0) {
            const auto club = clubs + local.below(8) * club_stride;
            c.put32(row + 0x24, club);
            bool listed = false;
            for (std::uint32_t k = 0; k < 5 && !listed; ++k)
                if (c.u32(club + 0x160 + 4 * k) == 0 || c.u32(club + 0x160 + 4 * k) == entries + 4 * k) {
                    c.put32(club + 0x160 + 4 * k, row);
                    listed = true;
                }
            if (!listed) c.put32(row + 0x24, 0);
            if (mode == 1 && listed && local.below(4) == 0)
                for (std::uint32_t k = 0; k < 5; ++k)
                    if (c.u32(club + 0x160 + 4 * k) == row) c.put32(club + 0x160 + 4 * k, 0);
        }
    }
    if (mode == 2) for (std::uint32_t i = 0; i < 8; ++i) c.put32(clubs + i * club_stride + 0x90, 0);
}
}

std::vector<Fixture> people_extra_fixtures() {
    std::vector<Fixture> out;
    Mix mix{0x3db0};
    constexpr std::array<std::uint32_t, 4> person_counts{0, 17, 40, 100};
    for (unsigned n = 0; n < 900; ++n) {
        const auto count_choice = n % 4;
        const auto person_count = person_counts[count_choice];
        // A table too short for 128 new people gets at most three clubs.
        const auto club_count = person_count < 100 ? mix.below(4) : mix.below(9);
        const auto mode = mix.below(5) == 0 ? 1u : mix.below(4) == 0 ? 2u : 0u;
        const auto seed = mix.next();
        const auto draws = script(mix, 90);
        // Every few cases a stub sets r31 so that the n-th generated person trips the 20,000-people assert.
        const bool trip = n % 5 == 4;
        const auto trip_at = mix.below(10);
        std::vector<Answer> answers{
            {0x10739890, {0}, 5}, {0x10768810, {data + 0x7000}, 1}, {0x1077b720, {0}, 8}, {0x10769b60, {0}, 3},
            {0x107158a0, {0}, 8}};
        auto f = make(0x100a3db0, "extra staff people " + std::to_string(person_count) + " clubs " +
                                      std::to_string(club_count) + " mode " + std::to_string(mode) + (trip ? " trip" : ""),
                      draws, [=](Cpu& c) { world(c, seed, club_count, person_count, mode); }, n % 3 == 0, answers);
        if (trip) {
            auto inner = f.stub;
            f.stub = [inner, trip_at](State& s) {
                if (s.pc != 0x107158a0) return inner(s);
                Cpu c(s);
                const bool fire = c.u32(answer_counters + 4 * 4) == trip_at;
                const auto done = inner(s);
                if (fire) Cpu(s).set(31, 0x5207);
                return done;
            };
        }
        out.push_back(std::move(f));
    }
    return out;
}
}
