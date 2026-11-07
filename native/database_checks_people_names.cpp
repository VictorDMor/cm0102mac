// Fixtures for 0x100b3650 (names) and 0x100b2b50 (the passes in order): the person table whose rows point at
// club-like records, with the real name pickers (0x100c4400, 0x100c4cf0), the unmarked-record picker (0x100c53e0)
// and the sample pool behind them, as the generation block's fixtures build them.
#include "database_checks_people_detail.hpp"

namespace cm::aot::database_checks {
namespace {
using namespace people_checks;
constexpr std::uint32_t table_cell = data + 0x100, count_cell = data + 0x110, table = data + 0x4000;
// The pickers' world (database_checks_generation.cpp's category picker, unmarked record and pool).
constexpr std::uint32_t rows = data + 0x1000, names = data + 0x8000, club_table = data + 0x10000,
                        index_cells = data + 0x1400, club_count_cell = data + 0x1300, club_table_cell = data + 0x1304,
                        unmarked_table = data + 0x14000, pool_start = data + 0x2000, pool_end_cell = data + 0x1f00,
                        cursor_cell = data + 0x1f10, salt_cell = data + 0x1f20;
constexpr std::int32_t category_slots[] = {-0x7584, -0x7588, -0x74d8, -0x74e0, -0x7a28, -0x7a10, -0x7594, -0x7a00,
                                           -0x7af4, -0x7598, -0x759c, -0x75a0, -0x7a14, -0x7a24, -0x75a8, -0x79fc,
                                           -0x75ac, -0x7504, -0x7a18, -0x75b0, -0x75b4, -0x75b8, -0x75bc, -0x75c0,
                                           -0x7514, -0x75c4, -0x750c, -0x74e8, -0x75a4};
constexpr unsigned clubs = 10, nations = 8;

void pickers_world(Cpu& c, Mix& local, std::int32_t category) {
    constexpr std::array<std::int32_t, 10> counts{0, 3, 5, 6, 10, 20, 50, 100, 101, 200};
    // Name pickers.
    c.put32(c.r(2) - 0x7580, data + 0x100 + 0x40);
    c.put32(data + 0x140, rows);
    for (const std::int32_t slot : {-0x7550, -0x754c}) c.put32(c.r(2) + std::uint32_t(slot), data + 0x150);
    c.put32(data + 0x150, local.below(400));
    c.put32(c.r(2) - 0x752c, data + 0x160);
    c.put32(c.r(2) - 0x7530, data + 0x168);
    c.put32(data + 0x168, names);
    c.put32(data + 0x160, names);
    c.put32(c.r(2) - 0x7b04, club_count_cell);
    c.put32(club_count_cell, 1 + local.below(clubs));
    c.put32(c.r(2) - 0x7b98, club_table_cell);
    c.put32(club_table_cell, club_table);
    c.put32(c.r(2) - 0x7590, data + 0x1380);
    c.put32(c.r(2) - 0x758c, data + 0x1384);
    c.put32(data + 0x1380, local.below(clubs));
    c.put32(data + 0x1384, local.below(clubs));
    unsigned k = 0;
    for (const auto slot : category_slots) {
        c.put32(c.r(2) + std::uint32_t(slot), index_cells + 4 * k);
        c.put32(index_cells + 4 * k, local.below(clubs));
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
        // 0x100c4b00 reads the record's first bytes as text: short, with or without a dot.
        const auto length = local.below(6);
        for (std::uint32_t j = 0; j < length; ++j) c.put8(names + 0x2c * i + j, local.below(5) == 0 ? '.' : 'a' + local.below(5));
        c.put8(names + 0x2c * i + length, 0);
    }
    for (std::uint32_t i = 1; i <= 100; ++i) c.put8(names - 0x2c * i + 0x28, local.below(256));
    for (unsigned i = 0; i < clubs; ++i) {
        c.put32(club_table + i * 0x128 + 0x68, local.below(nations));
        c.put8(club_table + i * 0x128 + 0x119, local.below(2) ? std::uint32_t(category) : local.below(40));
    }
    // The unmarked-record picker (a count, the table, the static flag and cached index) and the sample pool.
    const std::int32_t unmarked_counts[] = {0, 1, 2, 30, 100, -1, -7};
    c.put32(c.r(2) - 0x7554, data + 0x180);
    c.put32(data + 0x180, std::uint32_t(unmarked_counts[local.below(7)]));
    c.put32(c.r(2) - 0x7534, data + 0x184);
    c.put32(data + 0x184, unmarked_table);
    for (std::uint32_t i = 0; i < 100; ++i) c.put8(unmarked_table + 0x2c * i, 1 + local.below(255));
    if (local.below(2) && c.u32(data + 0x180) > 0 && c.u32(data + 0x180) < 100)
        c.put8(unmarked_table + 0x2c * local.below(c.u32(data + 0x180)), 0);
    const auto states = local.below(4);
    c.put8(c.r(2) - 0x39e9, states & 1);
    c.put32(c.r(2) - 0x39e8, (states >> 1) ? local.below(20) : 0xffffffffu);
    c.put32(c.r(2) - 0x7b84, cursor_cell);
    c.put32(c.r(2) - 0x7b90, salt_cell);
    c.put32(c.r(2) - 0x7b88, pool_end_cell);
    c.put32(c.r(2) - 0x7b8c, pool_start);
    c.put32(pool_end_cell, pool_start + 0x80);
    const std::array<std::uint32_t, 4> cursors{pool_start, pool_start + 0x7c, pool_start + 0x7e, pool_start + 0x80};
    c.put32(cursor_cell, cursors[local.below(4)]);
    c.put32(salt_cell, local.below(3) == 0 ? 0xfffffff0u : local.below(0x8000));
    for (std::uint32_t i = 0; i < 0x50; ++i) c.put16(pool_start + 2 * i, local.below(0x10000));
}

// The person table: `count` rows, each with a nation word (0 skips the row; otherwise a pointer to a club
// record, or a word the pickers read as a person) and the name fields as 0xa5.
void table_world(Cpu& c, std::int32_t count, const std::vector<std::uint32_t>& nations) {
    c.put32(c.r(2) - 0x7be0, table_cell);
    c.put32(c.r(2) - 0x7be4, count_cell);
    c.put32(table_cell, table);
    c.put32(count_cell, std::uint32_t(count));
    for (std::uint32_t i = 0; i < nations.size(); ++i) c.put32(table + 0x70 * i + 0xc, nations[i]);
}
// The passes in order, all doing real work (the worlds of the shuffle, swap and counter fixtures merged over
// the names' world): `count` rows (1 to 4) with player, staff and detail records, club-like records at +0xc and
// +0x10, dates, counter bytes and closeness; a squad table whose two clubs list those people; the gates' globals
// and nation rows; the contract manager's lookup, today's date and the settling hook scripted by the fixture.
constexpr std::uint32_t nation_rows = data + 0x3000, gate_cells = data + 0x3800, manager_cell = data + 0x3900,
                        contract = data + 0x3a00, date_words = data + 0x3a80, date_record = data + 0x3b00,
                        hook_cell = data + 0x3b40, squad_clubs = data + 0x16000, squad_cell = data + 0x3b80,
                        squad_count_cell = data + 0x3b84, players = data + 0x5000, details = data + 0x5800,
                        staff_record = data + 0x5a00;
constexpr std::int32_t gate_slots[] = {-0x74f4, -0x7af4, -0x7a14, -0x750c, -0x75c4, -0x7504, -0x75c8, -0x7a10, -0x7a1c,
                                      -0x757c, -0x7578, -0x7574, -0x7650, -0x7654, -0x7658, -0x79fc};
void passes_world(Cpu& c, Mix& mix, std::int32_t count) {
    const auto people = std::uint32_t(std::clamp(count, 0, 4));
    const auto club_of = [&] { return club_table + 0x128 * mix.below(clubs); };
    // Gates: each global points at a cell with a small nation number (or one above 0x80000000), the nation rows.
    for (unsigned n = 0; n < std::size(gate_slots); ++n) {
        c.put32(c.r(2) + std::uint32_t(gate_slots[n]), gate_cells + 4 * n);
        c.put32(gate_cells + 4 * n, (n % 5 == 4 ? 0x80000000u : 0u) + n % nations);
    }
    c.put32(c.r(2) - 0x7a70, gate_cells + 0x80);
    c.put32(gate_cells + 0x80, 3);
    c.put32(c.r(2) - 0x7520, gate_cells + 0x84);
    c.put32(gate_cells + 0x84, nation_rows);
    c.put32(c.r(2) - 0x7b7c, manager_cell);
    c.put32(manager_cell, data + 0x3c00);
    c.put32(contract + 0x28, mix.pick<std::uint32_t>({0, 1, 0x7ce, 0xffff, 0x8000}));
    c.put32(contract + 0x2a, mix.pick<std::uint32_t>({0, 1, 0x7ce, 0xffff, 0x8000}));
    c.put16(date_words, mix.pick<std::uint32_t>({0, 1, 0x7ce, 0x8000}));
    c.put16(date_words + 2, mix.below(2) ? c.u16(contract + 0x2a) : mix.pick<std::uint32_t>({0, 1, 0x7ce}));
    c.put32(date_words + 4, 0x1234abcd);
    for (unsigned n = 0; n < clubs; ++n) {
        c.put8(club_table + 0x128 * n + 0x11d, mix.pick<std::uint32_t>({0, 1, 2, 2, 3}));
        c.put32(club_table + 0x128 * n + 0x74, mix.below(2) ? nation_rows + 3 * 0xc8 : 0);
    }
    c.put16(date_record, 2000);
    c.put16(date_record + 2, 300);
    c.put32(c.r(2) - 0x7b80, date_record);
    c.put32(c.r(2) - 0x7a7c, hook_cell);
    c.put32(hook_cell, 0);
    // The people table.
    c.put32(c.r(2) - 0x7be0, table_cell);
    c.put32(c.r(2) - 0x7be4, count_cell);
    c.put32(table_cell, table);
    c.put32(count_cell, std::uint32_t(count));
    for (std::uint32_t k = 0; k < 4; ++k) {
        const auto row = table + 0x70 * k, player = players + 0x200 * k, detail = details + 0x40 * k;
        for (std::uint32_t at = 0; at < 0x70; at += 4) c.put32(row + at, mix.next() * 40503u);
        for (std::uint32_t at = 0; at < 0x200; ++at) {
            const auto byte = mix.next() & 0xff;
            c.put8(player + at, byte == 0x81 ? 0x80 : byte);  // -127 would keep the settling routines from ending
        }
        c.put32(row + 0xc, club_of());  // the shuffle's gate reads this club's status: never null
        c.put32(row + 0x10, mix.below(3) == 0 ? 0 : club_of());
        c.put32(row + 0x14, mix.below(8) == 0 ? 0 : player);
        c.put32(row + 0x1c, mix.below(2) ? staff_record : 0);
        c.put32(row + 0x24, mix.below(8) == 0 ? 0 : detail);
        c.put32(detail + 0x2c, mix.below(8) == 0 ? 0 : club_of());
        c.put32(detail + 0x30, mix.below(2) ? 0 : mix.below(3));
        const auto counter = mix.pick<std::uint32_t>({0, 0, 1, 2, 5, 10, 11, 40, 128, 255});
        c.put8(row + 0x5e, counter);
        c.put8(row + 0x5f, mix.pick<std::uint32_t>({0, 1, 3, 6, (counter - 1) & 0xff, counter, 255}));
        c.put16(row + 0x5c, mix.pick<std::uint32_t>({0x14, 0x15, 0x16, 0x30, 0x80, 0, 0x1020, 0xff14, 0x18}));
        c.put16(row + 0x34, 2000 + mix.pick<std::int32_t>({0, 0, 1, -1, 3}));
        c.put16(row + 0x36, 300 - mix.pick<std::int32_t>({-3, 0, 10, 19, 20, 21, 25, 26, 29, 30, 31, 45, 100, 200, 24, 27}));
        c.put16(player + 4, mix.below(3) ? 100 + mix.below(15) : mix.below(0x10000));
        c.put16(player + 8, mix.pick<std::uint32_t>({0x128d, 0x128e, 0x128f, 0x8000, 0, 0xffff, 0x2000}));
        c.put8(player + 0x11, mix.below(3) == 0 ? 0x14 : mix.below(256));
        c.put8(player + 0x14, mix.below(2) ? 0x14 : mix.below(256));
        c.put8(player + 0x15, mix.below(2) ? 0x14 : mix.below(256));
        for (std::uint32_t b = 0; b < 0x10; ++b) c.put8(player + b, mix.below(256));
    }
    // The squad table of the swap pass: three clubs of 50 slots listing those people.
    c.put32(c.r(2) - 0x7be8, squad_cell);
    c.put32(c.r(2) - 0x7b50, squad_count_cell);
    c.put32(squad_cell, squad_clubs);
    c.put32(squad_count_cell, 1 + mix.below(3));
    for (std::uint32_t club = 0; club < 3; ++club)
        for (std::uint32_t slot = 0; slot < 50; ++slot)
            c.put32(squad_clubs + 0x248 * club + 0x98 + 4 * slot,
                    mix.below(8) == 0 || people == 0 ? 0 : table + 0x70 * mix.below(std::max(people, 1u)));
    // The pool's salts stay below 16 bits (a negative sum would index outside a squad).
    c.put32(salt_cell, mix.below(0x8000));
}
}

std::vector<Fixture> people_names_fixtures() {
    std::vector<Fixture> out;
    Mix mix{0x70e0a11e};
    for (unsigned n = 0; n < 500; ++n) {
        const std::int32_t count = n < 6 ? std::int32_t(n) - 1 : std::int32_t(mix.below(5));
        std::vector<std::uint32_t> nations;
        for (std::int32_t i = 0; i < std::max(count, 0); ++i)
            nations.push_back(mix.below(4) == 0 ? 0 : club_table + 0x128 * mix.below(clubs));
        const auto draws = script(mix, 12);
        const auto world = mix.next();
        const auto category = std::int32_t(mix.below(36)) - 2;
        out.push_back(make(0x100b3650, "names count " + std::to_string(count), draws, [=](Cpu& c) {
            Mix local{world};
            pickers_world(c, local, category);
            table_world(c, count, nations);
        }, n % 3 == 0));
    }
    const std::vector<Answer> passes_answers{{0x10082f20, {contract, 0}, 2}, {0x107156f0, {date_words}, 5}, {0x103d3f70, {0}, 3}};
    for (unsigned n = 0; n < 300; ++n) {
        const std::int32_t count = n < 5 ? std::int32_t(n) - 1 : 1 + std::int32_t(mix.below(4));
        std::vector<std::uint32_t> nations;
        for (std::int32_t i = 0; i < std::max(count, 0); ++i)
            nations.push_back(mix.below(4) == 0 ? 0 : club_table + 0x128 * mix.below(clubs));
        const auto draws = script(mix, 30);
        const auto world = mix.next();
        out.push_back(make(0x100b2b50, "passes count " + std::to_string(count), draws, [=](Cpu& c) {
            Mix local{world};
            pickers_world(c, local, std::int32_t(local.below(36)) - 2);
            table_world(c, count, nations);
            passes_world(c, local, count);
        }, n % 3 == 0, passes_answers));
    }
    return out;
}
}
