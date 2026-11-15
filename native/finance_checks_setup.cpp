// FINANCE.CPP checker cases for the setup block (finance_setup.hpp): the draw below a limit, a club's
// account at the start of a game, the manager's constructor and the accounts file.
//
// The sample table, its cursor and its base are the game's own; a case that needs a given draw writes the
// cursor, the base and the halfwords the next draws read (`samples`). A redraw (the cursor past the last
// entry) calls the generator for real on both sides. The file layer, the allocator's failure and the file
// system's answer are other modules' routines, answered on both sides (Case::answers, nth_answers).
#include "finance_checks.hpp"
#include "guest_abi.hpp"
#include <climits>
#include <string>
#include <tuple>

namespace cm::aot::finance_checks {
namespace {
constexpr std::uint32_t random_below = 0x100f2100, start_account = 0x100f1050, create_manager = 0x100f84e0,
                        read_accounts = 0x100f9b00, write_accounts = 0x100f9c50, days_since = 0x100f9d80;
constexpr std::uint32_t file_open = 0x107475b0, file_read = 0x10747a80, file_write = 0x10747cc0, file_close = 0x10747900,
                        club_inactive = 0x100c7d00, allocate = 0x107735f0, construct_array = 0x10773150, row_index = 0x10191fd0, month_of = 0x10718360,
                        days_between = 0x10718130, wealth = 0x100f55b0;
constexpr std::uint32_t scratch_manager = scratch_base + 0x1000, scratch_flag = scratch_base + 0x2000,
                        link_area = scratch_base + 0x3000, table_area = scratch_base + 0x4000,
                        fake_file = 0x1234;
std::string number(std::uint32_t value) { return std::to_string(std::int32_t(value)); }

struct Memory {
    std::vector<Write> writes;
    void put(std::uint32_t at, std::uint32_t value, unsigned size = 4) { writes.push_back({size, at, value}); }
    void append(const std::vector<Write>& more) { writes.insert(writes.end(), more.begin(), more.end()); }
};

// The next draws read these halfwords on top of `base`: the cursor at the first entry, entry n+1 for draw n.
std::vector<Write> samples(const Scenario& scenario, std::uint32_t base, const std::vector<std::uint32_t>& halfwords) {
    Memory memory;
    const auto first = scenario.toc(-0x7b8c);
    memory.put(scenario.toc(-0x7b84), first);
    memory.put(scenario.toc(-0x7b90), base);
    for (std::size_t n = 0; n < halfwords.size(); ++n) memory.put(first + 2 + 2 * std::uint32_t(n), halfwords[n], 2);
    return memory.writes;
}
// The cursor `back` entries before the end: the draw `back` draws on redraws.
std::vector<Write> near_end(const Scenario& scenario, std::uint32_t back) {
    return {{4, scenario.toc(-0x7b84), scenario.global(-0x7b88) - 2 * (back - 1)}};
}
}

FINANCE_CASES("setup draw") {
    if (scenario.match) return;
    // The limits: zero, small, the edges of one draw (+-0xffff) and of the scaled draw, a tenth-scale limit
    // that is itself past 0xffff (not reachable: the product wraps), words of both signs.
    const std::uint32_t limits[] = {0u, 1u, 0xffffffffu, 2u, 7u, 100u, 0xffffff9cu, 0xfffeu, 0xffffu, 0x10000u, 0x10001u,
                                    0xffff0001u, 0xffff0000u, 0xfffeffffu, 0x12345u, 0xfff8b9cbu, 3000000u, 0xff4143e0u,
                                    0x7fffffffu, 0x80000000u, 0x80000001u, 0x40000000u, 0xc0000000u, 655350u, 654000u};
    for (const auto limit : limits) {
        cases.push_back({.entry = random_below, .registers = {{3, limit}}, .label = "draw below " + number(limit)});
        cases.push_back({.entry = random_below, .registers = {{3, limit}}, .memory = near_end(scenario, 1),
                         .label = "draw below " + number(limit) + ", first draw redraws"});
        cases.push_back({.entry = random_below, .registers = {{3, limit}}, .memory = near_end(scenario, 2),
                         .label = "draw below " + number(limit) + ", second draw redraws"});
    }
    // The sum of the scaled sample and a tenth of the second draw reaches the limit (and is held to limit - 1).
    for (const std::uint32_t limit : {104856u, 127894u, 2500000u, 0xfffe6ee8u})
        cases.push_back({.entry = random_below, .registers = {{3, limit}}, .memory = samples(scenario, 0, {65534, 15}),
                         .label = "draw below " + number(limit) + " held to the limit"});
    // Chosen samples. The first draw's sample is `h1 + base`: its remainder by 0xffff scales the limit; the second
    // draw below the tenth-scale limit adds a tenth of itself, and the sum is held to limit - 1.
    const std::uint32_t bases[] = {0, 0xffff, 0xfffffff0u, 0x80000000u};
    const std::uint32_t firsts[] = {0, 1, 0xfffe, 0xffff, 0x7fff};
    const std::uint32_t seconds[] = {0, 9, 0xffff, 0x8000, 31};
    for (const std::uint32_t limit : {0x10000u, 0x12345u, 3000000u, 0x7fffffffu, 0xffff0001u, 0x80000000u, 0xff4143e0u, 100u, 0xffffff9cu})
        for (const auto base : bases)
            for (const auto h1 : firsts)
                for (const auto h2 : seconds)
                    cases.push_back({.entry = random_below, .registers = {{3, limit}}, .memory = samples(scenario, base, {h1, h2}),
                                     .label = "draw below " + number(limit) + " samples " + number(base) + "+" + number(h1) + "," +
                                              number(h2)});
}

FINANCE_CASES("setup account") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ab8);
    const auto accounts = scenario.r32(manager);
    const auto count = scenario.global(-0x7b50);
    const auto account_of = [&](std::uint32_t index) { return accounts + index * 0x168; };
    const std::uint32_t sample[] = {0, 1, 2, 57, count / 7, count / 3, count / 2, count - 1};

    // Every club of a spread, as the game has it.
    for (std::uint32_t index = 0; index < count; index += count / 40 + 1)
        cases.push_back({.entry = start_account, .registers = {{3, account_of(index)}, {4, scenario.club(index)}},
                         .label = "start account of club " + std::to_string(index)});
    cases.push_back({.entry = start_account, .registers = {{3, account_of(5)}, {4, 0}}, .label = "start account of no club"});
    cases.push_back({.entry = start_account, .registers = {{3, account_of(5)}, {4, scenario.club(5)}},
                     .label = "start account of an inactive club", .answers = {{club_inactive, 1}}});

    const auto cell_a = scenario.global(-0x7a10), cell_b = scenario.global(-0x7a14), cell_c = scenario.global(-0x7a18);
    const auto name_text = scenario.toc(-0x5598) + 0x1f;  // "Rushden"
    const auto table_cell = scenario.toc(-0x7b5c);
    const auto link_object = [&](Memory& memory, std::uint32_t at, std::uint32_t index) { memory.put(at, index); };

    // The funds, the reputation, the name, the division and the status of the club, one by one.
    const std::int32_t funds_list[] = {0, 1, 9, 10, 90, 500000, 654000, 655350, 5000000, 2000000000, -1, -100, -500000, -655350, -2000000000,
                                       INT_MIN};
    const std::int32_t reputations[] = {0, 1, 400, 499, 500, 999, 1000, 1499, 2600, 5000, 5400, 0x128e, 0x128f, 0x1482, 0x1483, 0x1676, 0x1677,
                                        9000, 32767, -1, -500, -32768};
    const std::uint32_t club_index = sample[3];
    const auto club = scenario.club(club_index);
    const auto account = account_of(club_index);
    const auto division = scenario.r32(club + 0x2c);
    for (const auto funds : funds_list)
        for (const auto reputation : {0, 1000, 5400, 32767}) {
            Memory memory;
            memory.put(club + 4, std::uint32_t(funds));
            memory.put(club + 0x1ec, std::uint32_t(reputation) & 0xffff, 2);
            cases.push_back({.entry = start_account, .registers = {{3, account}, {4, club}}, .memory = memory.writes,
                             .label = "account funds " + std::to_string(funds) + " reputation " + std::to_string(reputation)});
        }
    // The club named "Rushden": the same jitter whatever the funds.
    for (const auto funds : {0, 100, 70000, 3000000, -70000}) {
        Memory memory;
        memory.put(club + 4, std::uint32_t(funds));
        memory.put(club + 0x1ec, 20, 2);
        for (std::uint32_t at = 0; at < 8; ++at) memory.put(club + 0x1ee + at, 0, 1);
        memory.put(club + 0x1ee, 0x52757368, 4);  // "Rush"
        memory.put(club + 0x1f2, 0x6465, 2);      // "de"
        cases.push_back({.entry = start_account, .registers = {{3, account}, {4, club}}, .memory = memory.writes,
                         .label = "account of Rushden funds " + std::to_string(funds)});
    }
    (void)name_text;
    // The table by reputation: every band, with and without a division, in each status, with every size/rating.
    for (const auto reputation : reputations)
        for (const std::uint32_t status : {0u, 2u, 3u})
            for (const bool with_division : {false, true}) {
                Memory memory;
                memory.put(club + 4, 0, 4);
                memory.put(club + 0x1ec, std::uint32_t(reputation) & 0xffff, 2);
                memory.put(club + 0x23d, status, 1);
                memory.put(club + 0x2c, with_division ? division : 0);
                cases.push_back({.entry = start_account, .registers = {{3, account}, {4, club}}, .memory = memory.writes,
                                 .label = "account by table reputation " + std::to_string(reputation) + " status " + std::to_string(status) +
                                          (with_division ? " with division" : " no division")});
            }
    if (division) {
        for (const std::int32_t rating : {-5, 0, 1, 2, 3, 4, 127})
            for (const std::int32_t size : {0, 1, -1, 5, 127, -128})
                for (const std::uint32_t parity : {0u, 1u}) {
                    Memory memory;
                    memory.append(samples(scenario, 0, {parity, 1234, 77, 4321}));
                    memory.put(club + 4, 0, 4);
                    memory.put(club + 0x1ec, 3000, 2);
                    memory.put(club + 0x23d, 3, 1);
                    memory.put(division + 0x11c, std::uint32_t(rating), 1);
                    memory.put(division + 0x11f, std::uint32_t(size), 1);
                    cases.push_back({.entry = start_account, .registers = {{3, account}, {4, club}}, .memory = memory.writes,
                                     .label = "account division rating " + std::to_string(rating) + " size " + std::to_string(size) +
                                              " parity " + std::to_string(parity)});
                }
        // Negative table values (low reputation) with the parity of the draw, and the wide and narrow draws below f31.
        for (const std::uint32_t parity : {0u, 1u, 0xffffu, 0x7ffeu})
            for (const std::uint32_t base : {0u, 0xfffffff0u, 0x10000u})
                for (const std::int32_t reputation : {100, 700, 1400, 1700, 5200, 8000, 12000}) {
                    Memory memory;
                    memory.append(samples(scenario, base, {parity, 40000, 3, 51234, 17}));
                    memory.put(club + 4, 0, 4);
                    memory.put(club + 0x1ec, std::uint32_t(reputation), 2);
                    memory.put(club + 0x23d, 0, 1);
                    cases.push_back({.entry = start_account, .registers = {{3, account}, {4, club}}, .memory = memory.writes,
                                     .label = "account negative band reputation " + std::to_string(reputation) + " parity " +
                                              std::to_string(parity) + " base " + number(base)});
                }
    }
    // The scaled draws held to their limit: samples 65534 then 15 for the first draw (and its nested draw), others for the second.
    for (const std::int32_t funds : {1048560, 1278940, -1048560}) {
        Memory memory;
        memory.append(samples(scenario, 0, {65534, 15, 200, 7}));
        memory.put(club + 4, std::uint32_t(funds), 4);
        memory.put(club + 0x1ec, 32767, 2);
        cases.push_back({.entry = start_account, .registers = {{3, account}, {4, club}}, .memory = memory.writes,
                         .label = "account scaled draws held to the limit, funds " + std::to_string(funds)});
    }
    if (division)
        for (const std::int32_t size : {9, 10, 12, 127, -9})
            for (const std::int32_t rating : {1, 2}) {
                Memory memory;
                memory.append(samples(scenario, 0, {65534, 15, 200, 7}));
                memory.put(club + 4, 0, 4);
                memory.put(club + 0x1ec, 5000, 2);
                memory.put(club + 0x23d, 0, 1);
                memory.put(division + 0x11c, std::uint32_t(rating), 1);
                memory.put(division + 0x11f, std::uint32_t(size), 1);
                cases.push_back({.entry = start_account, .registers = {{3, account}, {4, club}}, .memory = memory.writes,
                                 .label = "account whole-number draws held to the limit, size " + std::to_string(size) + " rating " +
                                          std::to_string(rating)});
            }
    // Redraws at each draw of each path.
    for (const std::uint32_t back : {1u, 2u, 3u, 4u, 5u})
        for (const std::int32_t funds : {5000, 5000000, 0}) {
            Memory memory;
            memory.append(near_end(scenario, back));
            memory.put(club + 4, std::uint32_t(funds), 4);
            memory.put(club + 0x1ec, funds == 0 ? 2600 : 9000, 2);
            memory.put(club + 0x23d, 3, 1);
            cases.push_back({.entry = start_account, .registers = {{3, account}, {4, club}}, .memory = memory.writes,
                             .label = "account redraw at draw " + std::to_string(back) + " funds " + std::to_string(funds)});
        }
    // The links of a club in status 3: a table and a link object in scratch memory.
    for (const std::uint32_t first_entry : {0u, 1u})
        for (const std::uint32_t second_entry : {0u, 1u})
            for (const std::uint32_t shape : {0u, 1u, 2u, 3u}) {
                Memory memory;
                memory.put(table_area, 0);
                memory.put(table_area + 4, 1);
                memory.put(table_area + 8, 0);
                memory.put(table_cell, shape == 3 ? 0 : table_area);
                link_object(memory, link_area, first_entry);
                link_object(memory, link_area + 0x10, second_entry + 1);
                memory.put(club + 0x23d, 3, 1);
                memory.put(club + 0x30, shape == 1 ? 0 : link_area);
                memory.put(club + 0x38, shape == 2 ? 0 : link_area + 0x10);
                memory.put(club + 4, 0, 4);
                memory.put(club + 0x1ec, 2600, 2);
                cases.push_back({.entry = start_account, .registers = {{3, account}, {4, club}}, .memory = memory.writes,
                                 .label = "account links " + std::to_string(first_entry) + std::to_string(second_entry) + " shape " +
                                          std::to_string(shape)});
            }
    // A status other than 3 does not look at the links.
    for (const std::uint32_t status : {1u, 4u, 0x83u, 0xffu}) {
        Memory memory;
        memory.put(table_area, 1);
        memory.put(table_cell, table_area);
        link_object(memory, link_area, 0);
        memory.put(club + 0x23d, status, 1);
        memory.put(club + 0x30, link_area);
        memory.put(club + 0x38, link_area);
        memory.put(club + 4, 0, 4);
        memory.put(club + 0x1ec, 2600, 2);
        cases.push_back({.entry = start_account, .registers = {{3, account}, {4, club}}, .memory = memory.writes,
                         .label = "account links of status " + std::to_string(status)});
    }
    // The attendance figure: the division's kind against the three cells, the reputation bands, the wealth band.
    if (division) {
        for (const auto kind_cell : {cell_a, cell_b, cell_c, cell_a + 1})
            for (const std::int32_t reputation : {0x128e, 0x128f, 0x1300, 0x1482, 0x1483, 0x1676, 0x1677, 0x2000, 0x4000})
                for (const std::int32_t funds : {0, 5000000, 10000000, 12000000, 15000000, 20000000, 40000000, 200000000, -9000000})
                    for (const std::uint32_t scale : {0u, 1u}) {
                        Memory memory;
                        memory.put(division + 0x68, kind_cell);
                        memory.put(club + 4, std::uint32_t(funds), 4);
                        memory.put(club + 0x1ec, std::uint32_t(reputation), 2);
                        memory.put(club + 0x23e, 1, 1);
                        memory.put(club + 0x241, scale, 1);
                        cases.push_back({.entry = start_account, .registers = {{3, account}, {4, club}}, .memory = memory.writes,
                                         .label = "account attendance kind " + number(kind_cell) + " reputation " +
                                                  std::to_string(reputation) + " funds " + std::to_string(funds) + " scale " +
                                                  std::to_string(scale)});
                    }
    }
    // A club in trouble (wealth band below 0) that asks for its band: the balance far below the reputation's limit.
    for (const std::int32_t funds : {-2000000000, -50000000, -3000000, -500000})
        for (const std::int32_t reputation : {0, 1000, 5400, 32767}) {
            Memory memory;
            memory.put(club + 4, std::uint32_t(funds), 4);
            memory.put(club + 0x1ec, std::uint32_t(reputation), 2);
            memory.put(club + 0x23e, 1, 1);
            cases.push_back({.entry = start_account, .registers = {{3, account}, {4, club}}, .memory = memory.writes,
                             .label = "account in trouble funds " + std::to_string(funds) + " reputation " + std::to_string(reputation)});
        }
    // The wealth band answered, for the interval byte (every band and the flag that asks for it).
    for (const std::uint32_t band : {0u, 1u, 2u, 3u, 0xffu, 0xfeu, 0x7fu})
        for (const std::uint32_t uses : {0u, 1u}) {
            Memory memory;
            memory.put(club + 0x23e, uses, 1);
            memory.put(club + 4, 0, 4);
            cases.push_back({.entry = start_account, .registers = {{3, account}, {4, club}}, .memory = memory.writes,
                             .label = "account interval band " + std::to_string(band) + " uses " + std::to_string(uses),
                             .answers = {{wealth, band}}});
        }
}

FINANCE_CASES("setup manager") {
    if (scenario.match) return;
    const auto count = scenario.global(-0x7b50);
    const auto count_cell = scenario.toc(-0x7b50);
    const auto stack = std::uint32_t(scenario.s.r<4>(guest::reg::gpr(1)));
    const auto frame = stack - 0x390;
    // FSMakeFSSpec's glue is reached directly by the original (it loads the import cell at TOC -0x7de0: lwz r12,-0x7de0(r2)
    // at 0x1077fee0), so no answer can stand in for it. The cell is pointed at a descriptor in scratch memory of a game
    // routine that returns 0 (0x10002390), or of today's-year getter (0x1048e29c) for the retry cases, where a poke on the
    // dialog (0x10769b60) zeroes the year so that the next try succeeds. Same trick as INJURY.CPP's constructor cases.
    const std::uint32_t import_cell = 0x108bba90u - 0x7de0u, descriptor = scratch_base + 0x9100;
    const auto today = scenario.toc(-0x7b80);
    using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
    const auto add = [&](const std::string& label, std::uint32_t clubs, std::uint32_t file, Answers answers = {},
                         std::vector<Case::NthAnswer> nth = {}, std::uint32_t code = 0x10002390, std::vector<Case::Poke> pokes = {},
                         std::vector<Write> more = {}) {
        Memory memory;
        memory.put(descriptor, code);
        memory.put(descriptor + 4, 0x108bba90);
        memory.put(import_cell, descriptor);
        memory.put(count_cell, clubs);
        memory.append(more);
        cases.push_back({.entry = create_manager, .registers = {{3, scratch_manager}, {4, file}}, .memory = memory.writes,
                         .label = "create manager " + label, .answers = answers, .pokes = pokes, .nth_answers = nth});
    };
    Memory year_zero;
    year_zero.put(today + 2, 0, 2);
    const std::vector<Case::Poke> retry{{0x10769b60, year_zero.writes}};
    const Answers file_ok{{file_open, fake_file}, {file_read, 0}, {file_close, 0}};
    for (const std::uint32_t clubs : {0u, 1u, 3u}) {
        const auto n = std::to_string(clubs) + " clubs, ";
        add(n + "new game", clubs, 0);
        add(n + "new game after one dialog", clubs, 0, {}, {}, 0x1048e29c, retry);
        add(n + "no memory", clubs, 0, {}, {{allocate, 1, 0}});
        add(n + "the array fails", clubs, 0, {}, {{construct_array, 1, 0}});
        add(n + "saved game does not open", clubs, fake_file, {{file_open, 0}});
        add(n + "saved game read", clubs, fake_file, {{file_open, fake_file}, {file_read, clubs}, {file_close, 0}});
        add(n + "saved game read short", clubs, fake_file, {{file_open, fake_file}, {file_read, clubs + 1}, {file_close, 0}});
        add(n + "saved game and no memory", clubs, fake_file, file_ok, {{allocate, 1, 0}});
    }
    add("all clubs, new game", count, 0);
    add("all clubs, saved game read", count, fake_file, {{file_open, fake_file}, {file_read, count}, {file_close, 0}});
    add("3 clubs, count changes in the loop", 3, 0, {}, {}, 0x10002390, {{club_inactive, {{4, count_cell, 2}}}});
    // The byte at sp+0x81 of the constructor's frame ends the merge loop: the real frame byte is read on both sides.
    for (const std::uint32_t at_81 : {0u, 0x41u, 0xffu})
        for (const std::uint32_t at_82 : {0u, 0x20u}) {
            Memory dead;
            dead.put(frame + 0x81, at_81, 1);
            dead.put(frame + 0x82, at_82, 1);
            dead.put(frame + 0x83, 0, 1);
            add("1 club, dead stack at +0x81: " + std::to_string(at_81) + "," + std::to_string(at_82), 1, 0, {}, {}, 0x10002390, {},
                dead.writes);
        }
}

FINANCE_CASES("setup file") {
    if (scenario.match) return;
    const auto manager = scenario.global(-0x7ab8);
    const auto count = scenario.global(-0x7b50);
    for (const auto [entry, transfer, name] : {std::tuple{read_accounts, file_read, "read"}, std::tuple{write_accounts, file_write, "write"}}) {
        const std::string label = name;
        cases.push_back({.entry = entry, .registers = {{3, manager}, {4, 0}}, .label = label + " accounts of no file"});
        cases.push_back({.entry = entry, .registers = {{3, manager}, {4, fake_file}}, .label = label + " accounts, open fails",
                         .answers = {{file_open, 0}}});
        for (const std::uint32_t moved : {count, count + 1, 0u, count - 1, 0xffffffffu})
            cases.push_back({.entry = entry, .registers = {{3, manager}, {4, fake_file}},
                             .label = label + " accounts, " + std::to_string(std::int32_t(moved)) + " of " + std::to_string(count),
                             .answers = {{file_open, fake_file}, {transfer, moved}, {file_close, 0}}});
        cases.push_back({.entry = entry, .registers = {{3, manager}, {4, fake_file}}, .memory = {{4, scenario.toc(-0x7b50), 3}},
                         .label = label + " accounts, count 3",
                         .answers = {{file_open, fake_file}, {transfer, 3}, {file_close, 0}}});
    }
}

FINANCE_CASES("setup days") {
    if (scenario.match) return;
    const auto rows = scenario.toc(-0x7b44);
    const auto today = scenario.toc(-0x7b80);
    const auto today_year = scenario.s.memory.read<2>(today + 2);
    const auto year_cell = scenario.toc(-0x7a9c);
    const auto this_year = scenario.s.memory.read<2>(year_cell + 2);
    (void)today_year;
    const auto row = [&](std::int32_t index) { return rows + std::uint32_t(index * 0x4c); };
    cases.push_back({.entry = days_since, .registers = {{3, 0}, {4, 5}, {5, scratch_flag}}, .label = "days since, no such item",
                     .answers = {{row_index, 0xffffffffu}}});
    cases.push_back({.entry = days_since, .registers = {{3, 0}, {4, 5}, {5, scratch_flag}}, .memory = {{1, scratch_flag, 7}},
                     .label = "days since, item index 0x80", .answers = {{row_index, 0x80}}});
    // The item's row: its end-year flag, kind byte and date, against today's year and the month answers.
    for (const std::uint32_t flag : {0u, 1u})
        for (const std::uint32_t year_now : {2000u, 2001u, 2002u, 2003u, 0u, 0xffffu})
            for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 0x7fu, 0x80u, 0xffu, 0xfeu})
                for (const std::uint32_t same_year : {0u, 1u})
                    for (const std::uint32_t month : {0u, 1u, 5u, 11u, 12u, 13u, 14u, 0xffu, 0x180u, 0xfffffff3u})
                        for (const std::uint32_t days : {0u, 5u, 0xfffffffbu}) {
                            Memory memory;
                            memory.put(row(0) + 0x18, flag, 1);
                            memory.put(row(0) + 0x36, kind, 1);
                            memory.put(row(0) + 0x2e, same_year ? year_now : year_now + 1, 2);
                            memory.put(today + 2, year_now, 2);
                            memory.put(year_cell + 2, this_year, 2);
                            cases.push_back({.entry = days_since, .registers = {{3, 0}, {4, 5}, {5, scratch_flag}}, .memory = memory.writes,
                                             .label = "days since flag " + std::to_string(flag) + " year " + std::to_string(year_now) +
                                                      " kind " + std::to_string(kind) + " same " + std::to_string(same_year) + " month " +
                                                      number(month) + " days " + number(days),
                                             .answers = {{row_index, 0}, {month_of, month}, {days_between, days}}});
                        }
    // The real month and days functions, on rows dated around today.
    for (const std::uint32_t kind : {0u, 1u, 2u, 3u})
        for (const std::uint32_t offset : {0u, 1u, 30u, 400u}) {
            Memory memory;
            memory.put(row(1) + 0x18, 0, 1);
            memory.put(row(1) + 0x36, kind, 1);
            memory.put(row(1) + 0x2c, std::uint32_t(scenario.s.memory.read<2>(today)) - offset, 2);
            memory.put(row(1) + 0x2e, scenario.s.memory.read<2>(today + 2), 2);
            cases.push_back({.entry = days_since, .registers = {{3, 0}, {4, 5}, {5, scratch_flag}}, .memory = memory.writes,
                             .label = "days since real, kind " + std::to_string(kind) + " offset " + std::to_string(offset),
                             .answers = {{row_index, 1}}});
        }
}
}
