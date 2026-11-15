// HUMAN_MANAGER.CPP checker cases for the table's lifecycle (human_manager_lifecycle.hpp).
//
// The table and its records are the career's. Cases that build a table use one in scratch memory (the
// routine fills it) and the real allocator and constructors. The CD folder lookup goes through the
// FSMakeFSSpec glue, which is reached directly (it loads the import cell at TOC -0x7de0), so no answer can
// stand in for it: the cell is pointed at a descriptor in scratch memory of a game routine that returns 0
// (found), or of today's-year getter (0x1048e29c), which fails until a poke on the dialog zeroes the year.
// UTILS.CPP's folder path (0x10746300, inside 0x10145fd0) is answered: it resolves an alias.
#include "human_manager_checks.hpp"
#include "guest_abi.hpp"
#include <string>

namespace cm::aot::human_manager_checks {
namespace {
constexpr std::uint32_t record_size = 0xa65c;
constexpr std::uint32_t build_table = 0x10145af0, destroy_record = 0x10145d70, destroy_tactic = 0x10145dd0,
                        construct_record = 0x10145e10, construct_tactic = 0x10145e70, destroy_table = 0x10145ea0,
                        start_human = 0x10145fd0;
constexpr std::uint32_t read_file = 0x1014a300, open_zip = 0x107475b0, allocate = 0x107735f0, construct_array = 0x10773150,
                        release = 0x107735a0, destroy_offer = 0x10411f10, folder_path = 0x10746300;
constexpr std::uint32_t scratch_table = scratch_base + 0x1000, scratch_file = scratch_base + 0x2000,
                        scratch_offers = scratch_base + 0x3000, descriptor = scratch_base + 0x9100,
                        import_cell = 0x108bba90u - 0x7de0u;
}

HUMAN_MANAGER_CASES("lifecycle") {
    if (scenario.match) return;
    const auto humans = scenario.global(-0x7a54);
    const auto records = scenario.r32(humans), count = scenario.r32(humans + 4);
    const auto staff_total = scenario.people();
    const auto game = scenario.toc(-0x7bb0);
    const auto current = std::uint32_t(std::int32_t(std::int16_t(scenario.s.memory.read<2>(game + 0x2c58))));
    const auto current_staff_cell = game + current * 0x2c0 + 4;
    const auto current_staff = scenario.r32(current_staff_cell);
    const auto today = scenario.toc(-0x7b80);
    const auto human_record = [&](std::uint32_t staff) {
        return records + (scenario.r32(staff) - staff_total + 16) * record_size;
    };
    using Memory = std::vector<Write>;
    using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
    // The file system answers "found" (0), or fails while today's year is not zero.
    const auto file_system = [&](std::uint32_t code = 0x10002390) {
        return Memory{{4, descriptor, code}, {4, descriptor + 4, 0x108bba90}, {4, import_cell, descriptor}};
    };
    const auto joined = [](Memory first, const Memory& second) { first.insert(first.end(), second.begin(), second.end()); return first; };

    // UTILS.CPP's folder path resolves an alias, which a snapshot cannot serve: answered (its buffers stay as the
    // frame left them on both sides).
    const Answers folder_answer{{folder_path, 0}};
    // --- 0x10145fd0: one human's record. The staff to ask about, against the table's own states.
    std::vector<std::pair<std::uint32_t, std::string>> staff{{0, "current"}, {current_staff, "current given"}};
    for (std::uint32_t slot = 0; slot < 16 && staff_total >= 16 - slot; ++slot)
        staff.emplace_back(scenario.person(staff_total - 16 + slot), "slot " + std::to_string(slot));
    if (staff_total > 17) staff.emplace_back(scenario.person(staff_total - 17), "below the table");
    staff.emplace_back(scenario.person(0), "ordinary staff");
    const std::vector<std::pair<Memory, std::string>> settings{
        {{}, ""},
        {{{4, current_staff_cell, 0}}, ", no current human"},
        {{{4, humans + 4, 0}}, ", empty table"},
        {{{4, humans + 4, 1}}, ", table of one"},
        {{{4, humans + 4, 0x80000000u}}, ", table count INT_MIN"},
    };
    for (const auto& [setting_memory, setting_label] : settings)
        for (const auto& [staff_record, staff_label] : staff)
            cases.push_back({.entry = start_human, .registers = {{3, humans}, {4, staff_record}},
                             .memory = setting_memory, .label = "start " + staff_label + setting_label, .answers = folder_answer});
    // The counters and the old state of the record: holiday and away bytes (tested as signed bytes), offers to
    // destroy (a scratch object each, answered), the counters at +8 / +0xc of the table.
    for (const auto staff_record : {current_staff, scenario.person(staff_total - 16), scenario.person(staff_total - 1)}) {
        if (scenario.r32(staff_record) - staff_total + 16 >= count) continue;
        const auto human = human_record(staff_record);
        for (const std::uint32_t holiday : {0u, 1u, 0x80u, 0x100u})
            for (const std::uint32_t away : {0u, 1u, 0x80u, 0xffu}) {
                Memory memory{{1, human + 0x9a65, holiday}, {1, human + 0x9984, away}, {1, human + 0x9985, 1}};
                cases.push_back({.entry = start_human, .registers = {{3, humans}, {4, staff_record}},
                                 .memory = memory,
                                 .label = "start holiday " + std::to_string(holiday) + " away " + std::to_string(away),
                                 .answers = folder_answer});
            }
        for (const std::uint32_t counter : {0u, 5u})
            cases.push_back({.entry = start_human, .registers = {{3, humans}, {4, staff_record}},
                             .memory = {{1, human + 0x9a65, 1}, {1, human + 0x9984, 1}, {4, humans + 8, counter}, {4, humans + 12, counter}},
                             .label = "start counters " + std::to_string(counter), .answers = folder_answer});
        for (unsigned offers = 1; offers < 4; ++offers) {
            Memory memory{{4, scratch_offers, 0x1234}, {4, scratch_offers + 8, 0x5678}};
            memory.push_back({4, human + 0x99f0, offers & 1 ? scratch_offers : 0});
            memory.push_back({4, human + 0x99f4, offers & 2 ? scratch_offers + 8 : 0});
            cases.push_back({.entry = start_human, .registers = {{3, humans}, {4, staff_record}},
                             .memory = memory, .label = "start offers " + std::to_string(offers),
                             .answers = {{destroy_offer, 0}, {folder_path, 0}}});
        }
        // Leftovers of an old setup: the fields it clears.
        cases.push_back({.entry = start_human, .registers = {{3, humans}, {4, staff_record}},
                         .memory = {{4, human + 0x99fc, 0xdeadbeef}, {1, human + 0x9a00, 3}, {1, human + 0x9a01, 0x41},
                                    {1, human + 0x9a33, 0x42}, {4, human + 0xa650, 0x77}, {4, human + 0xa658, 9}},
                         .label = "start over used record", .answers = folder_answer});
    }

    // --- 0x10145ea0: the table's destructor, on the career's table (the records and their array are the game's).
    for (const std::uint32_t flag : {0u, 1u, 0xffffffffu, 0x10000u, 0x8000u, 0x10001u})
        cases.push_back({.entry = destroy_table, .registers = {{3, humans}, {4, flag}},
                         .label = "destroy table flag " + std::to_string(std::int32_t(flag)), .answers = {{release, 0}}});
    cases.push_back({.entry = destroy_table, .registers = {{3, 0}, {4, 1}}, .label = "destroy no table"});
    cases.push_back({.entry = destroy_table, .registers = {{3, humans}, {4, 1}}, .memory = {{4, humans, 0}},
                     .label = "destroy table without records", .answers = {{release, 0}}});
    cases.push_back({.entry = destroy_table, .registers = {{3, humans}, {4, 0}}, .memory = {{4, humans, 0}},
                     .label = "destroy table without records, kept"});
    for (const std::uint32_t entries : {0u, 1u, 3u, 0x80000000u, 0xffffffffu})
        cases.push_back({.entry = destroy_table, .registers = {{3, humans}, {4, 0}}, .memory = {{4, humans + 4, entries}},
                         .label = "destroy table count " + std::to_string(std::int32_t(entries))});
    // Offers of some records (scratch objects whose destruction is answered). The sorted entries are the game's: two
    // records of the career hold real ones, which the real release frees; a planted pointer cannot be answered
    // (a call 0x10775170 makes from the original's body is not answered like the rewrite's).
    for (unsigned which = 1; which < 4; ++which) {
        Memory memory{{4, scratch_offers, 0x1234}, {4, scratch_offers + 8, 0x5678}};
        for (std::uint32_t index : {0u, 7u, count - 1}) {
            const auto human = records + index * record_size;
            memory.push_back({4, human + 0x99f0, which & 1 ? scratch_offers : 0});
            memory.push_back({4, human + 0x99f4, which & 2 ? scratch_offers + 8 : 0});
        }
        cases.push_back({.entry = destroy_table, .registers = {{3, humans}, {4, 0}}, .memory = memory,
                         .label = "destroy table with objects " + std::to_string(which),
                         .answers = {{destroy_offer, 0}}});
    }

    // --- The record's and the tactic slot's constructors and destructors, on the career's records.
    const auto human = records;
    cases.push_back({.entry = construct_record, .registers = {{3, human}}, .label = "construct record"});
    cases.push_back({.entry = construct_record, .registers = {{3, records + 5 * record_size}}, .label = "construct another record"});
    cases.push_back({.entry = construct_tactic, .registers = {{3, human}}, .label = "construct tactic slot"});
    cases.push_back({.entry = construct_tactic, .registers = {{3, human + 0x1eb4 * 3}}, .label = "construct another tactic slot"});
    for (const std::uint32_t flag : {0u, 0xffffffffu, 0x10000u, 0x8000u, 1u, 0x7fffu, 0x10001u}) {
        const auto label = std::to_string(std::int32_t(flag));
        cases.push_back({.entry = destroy_record, .registers = {{3, human}, {4, flag}}, .label = "destroy record " + label,
                         .answers = {{release, 0}}});
        cases.push_back({.entry = destroy_tactic, .registers = {{3, human}, {4, flag}}, .label = "destroy tactic slot " + label,
                         .answers = {{release, 0}}});
    }
    cases.push_back({.entry = destroy_record, .registers = {{3, 0}, {4, 1}}, .label = "destroy no record"});
    cases.push_back({.entry = destroy_tactic, .registers = {{3, 0}, {4, 1}}, .label = "destroy no tactic slot"});

    // --- 0x10145af0: building a table, in scratch memory, with the real allocator and constructors.
    const auto build = [&](const std::string& label, std::uint32_t file, Answers answers = {}, Memory more = {},
                           std::uint32_t code = 0x10002390, std::vector<Case::Poke> pokes = {},
                           std::vector<Case::NthAnswer> nth = {}, std::uint32_t table = scratch_table) {
        answers.emplace_back(folder_path, 0);
        cases.push_back({.entry = build_table, .registers = {{3, table}, {4, file}}, .memory = joined(file_system(code), more),
                         .label = "build " + label, .answers = answers, .pokes = pokes, .nth_answers = nth});
    };
    build("new game", 0);
    build("over the game's table", 0, {}, {}, 0x10002390, {}, {}, humans);
    build("new game after one dialog", 0, {}, {}, 0x1048e29c, {{0x10769b60, {{2, today + 2, 0}}}});
    build("no memory", 0, {{allocate, 0}});
    build("the array fails", 0, {{construct_array, 0}});
    for (const std::uint32_t answer : {0u, 1u, 0x100u, 0x80u, 0xffu, 0xffffff00u, 0x7fu})
        // In a run that checks 0x1014a300 too, its answer does not apply (a routine under check is never
        // answered): the open is answered as failing, so the read returns 0 instead of reaching the file system.
        build("saved game read answer " + std::to_string(answer), scratch_file, {{read_file, answer}, {open_zip, 0}});
    build("saved game, file layer answered", scratch_file, {{open_zip, 0}});
    build("saved game and no memory", scratch_file, {{read_file, 0}, {allocate, 0}, {open_zip, 0}});
    // The byte at sp+0x81 of the constructor's frame ends the name merge: the real frame byte is read on both sides.
    const auto frame = std::uint32_t(scenario.s.r<4>(guest::reg::gpr(1))) - 0x390;
    for (const std::uint32_t at_81 : {0u, 0x41u, 0xffu})
        for (const std::uint32_t at_82 : {0u, 0x20u})
            build("dead stack at +0x81: " + std::to_string(at_81) + "," + std::to_string(at_82), 0, {},
                  {{1, frame + 0x81, at_81}, {1, frame + 0x82, at_82}, {1, frame + 0x83, 0}});
}
}
