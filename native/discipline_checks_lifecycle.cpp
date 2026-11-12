// DISCIPLINE.CPP lifecycle block cases (discipline_checks.hpp): the manager's constructor, destructor and the
// creation of its bodies (the load and save are in discipline_checks_lifecycle_file.cpp). The career snapshot's
// manager, bodies and competitions are the real ones; the file layer, the folder lookup and the history file
// are scripted (discipline_checks_lifecycle_service.hpp), the allocators answer failures where a case needs
// them, and Case::memory sets what the branches read.
#include "discipline_checks_lifecycle_service.hpp"
#include <string>

namespace {
using namespace cm::aot::discipline_checks;
using lifecycle::Role;
using lifecycle::Script;
constexpr std::uint32_t construct = 0x100de410, destroy = 0x100de660, create_bodies = 0x100e7cd0;
constexpr std::uint32_t zeroed_allocate = 0x10775200, super_cup_check = 0x101926b0, clock_value = 0x10775550;
Write word(std::uint32_t at, std::uint32_t value) { return {4, at, value}; }
Write byte(std::uint32_t at, std::uint32_t value) { return {1, at, value}; }
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;

// The TOC cells of the competitions whose activity decides each body (0x100e7cd0, in the original's order).
constexpr std::int32_t competition_cells[] = {-0x7508, -0x7510, -0x7b68, -0x750c, -0x7a10, -0x7594, -0x7a00, -0x7af4,
                                              -0x7a28, -0x7574, -0x7a14, -0x7a24, -0x7504, -0x79fc, -0x7a1c, -0x7a18,
                                              -0x7514, -0x7a20, -0x75a8, -0x7598, -0x7590, -0x7650, -0x7578, -0x75b0,
                                              -0x758c, -0x757c};

// The constructor on a new (zeroed) object in the scratch area: the folder lookup, the history file object,
// then the load or the build.
void construct_cases(const Scenario& scenario, std::vector<Case>& cases) {
    const auto people = scenario.people();
    const auto fresh = [&](std::string label, Script script, std::uint32_t file, Answers answers = {}) {
        script.manager = lifecycle::new_manager;
        script.people = people;
        cases.push_back({.entry = construct, .registers = {{3, lifecycle::new_manager}, {4, file}}, .memory = lifecycle::memory(script),
                         .label = "lifecycle: construct " + label, .answers = std::move(answers), .service = lifecycle::service(script)});
    };
    fresh("new game, folder found at once", {}, 0);
    {
        Script once; once.spec_failures = 1;
        fresh("new game, folder missing once (the CD dialog)", once, 0);
        Script twice; twice.spec_failures = 2;
        fresh("new game, folder missing twice", twice, 0);
    }
    {
        // The history file object cannot be allocated: the report, and nothing after it.
        Script none; none.fail_object = 1;
        fresh("history object not allocated", none, 0);
        fresh("history object not allocated, with a file", none, lifecycle::file_argument);
    }
    {
        fresh("from a file, the whole load", {}, lifecycle::file_argument);
        Script no_open; no_open.open_fails = true;
        fresh("from a file that does not open: built as new", no_open, lifecycle::file_argument);
        Script short_table; short_table.fail = Role::table;
        fresh("from a file with a short table: built as new", short_table, lifecycle::file_argument);
        Script no_counts; no_counts.fail = Role::record_count;
        fresh("from a file without the record count: built as new", no_counts, lifecycle::file_argument);
        // The table's allocation fails inside the load; the build then allocates it as well and fails again.
        Script no_table; no_table.fail_size = people * 0x2c;
        fresh("from a file, table not allocated: built as new", no_table, lifecycle::file_argument);
    }
    {
        Script no_table; no_table.fail_size = people * 0x2c;
        fresh("new game, person table not allocated", no_table, 0);
        fresh("new game, bodies not allocated", {}, 0, {{zeroed_allocate, 0}});
        Script third; third.fail_object = 3;
        fresh("new game, the second body not allocated", third, 0);
    }
}

void destroy_cases(const Scenario& scenario, std::vector<Case>& cases) {
    const auto manager = scenario.global(-0x7aa4);
    const auto bodies = scenario.r32(manager);
    const auto add = [&](std::string label, std::uint32_t object, std::uint32_t deleting, std::vector<Write> memory = {}) {
        cases.push_back({.entry = destroy, .registers = {{3, object}, {4, deleting}}, .memory = std::move(memory),
                         .label = "lifecycle: destroy " + label});
    };
    add("the manager, kept", manager, 0);
    add("the manager, deleted", manager, 1);
    add("the manager, flag -1", manager, 0xffffffffu);
    add("the manager, flag 0x10001 (halfword 1)", manager, 0x10001);
    add("the manager, flag 0x10000 (halfword 0)", manager, 0x10000);
    add("the manager, flag 0x8000 (halfword -32768)", manager, 0x8000);
    add("no object", 0, 1);
    add("no ban records", manager, 1, {word(manager + 4, 0)});
    add("no sorted index", manager, 1, {word(manager + 0xc, 0)});
    add("no person table", manager, 1, {word(manager + 0x14, 0)});
    add("no history file", manager, 1, {word(manager + 0x18, 0)});
    add("no buffers, no history file", manager, 0, {word(manager + 4, 0), word(manager + 0xc, 0), word(manager + 0x14, 0), word(manager + 0x18, 0)});
    add("empty body slots", manager, 1, {word(bodies, 0), word(bodies + 4 * 41, 0), word(bodies + 4 * 20, 0)});
    std::vector<Write> no_bodies;
    for (unsigned i = 0; i < 42; ++i) no_bodies.push_back(word(bodies + 4 * i, 0));
    add("no bodies", manager, 1, no_bodies);
    // No case with the body array itself null (a failed build): the loop would read the slots at address 0, which the
    // game's memory does not map; both sides stop there.
}

void bodies_cases(const Scenario& scenario, std::vector<Case>& cases) {
    const auto manager = scenario.global(-0x7aa4);
    const auto competitions = scenario.global(-0x7b98);
    const auto flag_at = [&](std::int32_t cell) { return competitions + scenario.r32(scenario.toc(cell)) * 0x128 + 0x120; };
    const auto base = [&](std::string label, std::uint32_t file, std::vector<Write> memory, Answers answers, Script script) {
        script.manager = manager;
        script.people = scenario.people();
        cases.push_back({.entry = create_bodies, .registers = {{3, manager}, {4, file}}, .memory = std::move(memory),
                         .label = "lifecycle: bodies " + label, .answers = std::move(answers), .service = lifecycle::service(script)});
    };
    const Answers neither{{super_cup_check, 0}, {clock_value, 0x3e043}};
    // As the snapshot has it, a new game (the constructors told there is no file) and a load (told the file).
    base("as the snapshot, new game", 0, {}, {{clock_value, 0x3e044}}, {});
    base("as the snapshot, from a file", lifecycle::handle_value, {}, {{clock_value, 0x3e044}}, {});
    base("as the snapshot, clock below the cutoff, from a file", lifecycle::handle_value, {}, {{clock_value, 0x3e043}}, {});
    base("as the snapshot, clock at the cutoff", 0, {}, {{clock_value, 0x3e044}}, {});
    base("as the snapshot, clock far above the cutoff", 0, {}, {{clock_value, 0x7fffffff}}, {});
    base("as the snapshot, clock negative", 0, {}, {{clock_value, 0xffffffffu}}, {});
    // Every competition inactive, then each group alone active, then each alone inactive.
    std::vector<Write> none;
    for (auto cell : competition_cells) none.push_back(byte(flag_at(cell), 0));
    base("no competition active, no super cup", 0, none, neither, {});
    base("no competition active, super cup", 0, none, {{super_cup_check, 1}, {clock_value, 0x3e043}}, {});
    base("no competition active, from a file", lifecycle::handle_value, none, neither, {});
    std::vector<Write> all;
    for (auto cell : competition_cells) all.push_back(byte(flag_at(cell), 1));
    base("every competition active, super cup", 0, all, {{super_cup_check, 1}, {clock_value, 0x3e044}}, {});
    for (std::size_t n = 0; n < std::size(competition_cells); ++n) {
        std::vector<Write> only = none;
        only[n] = byte(flag_at(competition_cells[n]), 0x80);  // a nonzero signed byte
        base("only group " + std::to_string(n) + " active", 0, only, neither, {});
        std::vector<Write> but = all;
        but[n] = byte(flag_at(competition_cells[n]), 0);
        base("every group but " + std::to_string(n) + " active", 0, but, {{super_cup_check, 1}, {clock_value, 0x3e043}}, {});
    }
    // Allocation failures: the body array, every object, and a single object (the first ones, a middle one, a late one).
    base("body array not allocated", 0, {}, {{zeroed_allocate, 0}, {clock_value, 0x3e044}}, {});
    for (std::uint32_t clock : {0x3e043u, 0x3e044u}) {
        Script every; every.fail_object = lifecycle::every_object;
        base("every object allocation fails, clock " + std::to_string(clock), 0, {}, {{clock_value, clock}}, every);
    }
    for (unsigned k : {1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 10u, 12u, 20u, 38u}) {
        Script script; script.fail_object = k;
        base("object allocation " + std::to_string(k) + " fails", 0, {}, {{clock_value, 0x3e044}}, script);
    }
}

DISCIPLINE_CASES("lifecycle") {
    if (scenario.match) return;
    lifecycle::install(scenario);
    construct_cases(scenario, cases);
    destroy_cases(scenario, cases);
    bodies_cases(scenario, cases);
}
}
