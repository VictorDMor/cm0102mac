// HUMAN_MANAGER.CPP checker cases for a human's record and its fields (human_manager_records.hpp).
//
// The table is the game's own (global -0x7a54): one 0xa65c-byte record per human, indexed by the
// staff id less the staff count plus 16. Humans are the career's; a case changes the fields a
// routine reads, the current human, the table's count, or a club's manager.
#include "human_manager_checks.hpp"
#include <string>

namespace cm::aot::human_manager_checks {
namespace {
constexpr std::uint32_t record_size = 0xa65c;
constexpr std::uint32_t current_tactic_slot = 0x101466c0, short_setting = 0x10147bc0, set_short_setting = 0x10147d20,
                        away = 0x10147e70, holiday = 0x10149170, runs_club = 0x101481d0, runs_nation = 0x101484b0,
                        setting = 0x10148bf0, set_setting = 0x10148d50, flag = 0x10149df0, folder = 0x10147150,
                        runs = 0x1014c2e0, current_human_runs = 0x1014c490;
// UTILS.CPP: the folder path of a name (FSMakeFSSpec, which a snapshot cannot serve).
constexpr std::uint32_t folder_path = 0x10746300;
constexpr std::uint32_t tactic_fields[] = {0x10146a20, 0x10146b90, 0x10146d00, 0x10146e70, 0x10146fe0};
constexpr std::uint32_t record_fields[] = {0x101487d0, 0x10148930, 0x10148a90, 0x10148eb0, 0x10149010, 0x10149c90, 0x1014a1a0};
// Routines taking (table, staff); the others put the staff in r5.
constexpr std::uint32_t staff_in_r4[] = {current_tactic_slot, short_setting, away, holiday, runs_club, setting,
                                         0x10146a20, 0x10146b90, 0x10146d00, 0x10146e70, 0x10146fe0,
                                         0x101487d0, 0x10148930, 0x10148a90, 0x10148eb0, 0x10149010, 0x10149c90, 0x1014a1a0};
}

HUMAN_MANAGER_CASES("records") {
    if (scenario.match) return;
    const auto humans = scenario.global(-0x7a54);
    const auto records = scenario.r32(humans), count = scenario.r32(humans + 4);
    const auto staff_total = scenario.people();
    const auto game = scenario.toc(-0x7bb0);
    const auto current = std::uint32_t(std::int32_t(std::int16_t(scenario.s.memory.read<2>(game + 0x2c58))));
    const auto current_staff_cell = game + current * 0x2c0 + 4;
    const auto current_staff = scenario.r32(current_staff_cell);
    const auto human_record = [&](std::uint32_t staff) {
        return records + (scenario.r32(staff) - staff_total + 16) * record_size;
    };
    // The staff to ask about: the current human (null and given), every staff id the table covers,
    // the first id below it and an ordinary member of staff (both outside the table).
    std::vector<std::pair<std::uint32_t, std::string>> staff{{0, "current"}, {current_staff, "current given"}};
    for (std::uint32_t slot = 0; slot < 16 && staff_total >= 16 - slot; ++slot)
        staff.emplace_back(scenario.person(staff_total - 16 + slot), "slot " + std::to_string(slot));
    if (staff_total > 17) staff.emplace_back(scenario.person(staff_total - 17), "below the table");
    staff.emplace_back(scenario.person(0), "ordinary staff");
    // Changes of the table and the game record that move a routine onto its failure paths.
    const std::vector<std::pair<std::vector<Write>, std::string>> settings{
        {{}, ""},
        {{{4, current_staff_cell, 0}}, ", no current human"},
        {{{4, humans + 4, 0}}, ", empty table"},
        {{{4, humans + 4, 1}}, ", table of one"},
        {{{4, humans + 4, 0x80000000u}}, ", table count INT_MIN"},
    };
    const auto both = [&](std::uint32_t entry, std::uint32_t r3, std::uint32_t value_register, std::uint32_t value,
                          std::uint32_t staff_record, const std::vector<Write>& memory, const std::string& label) {
        Case item{.entry = entry, .registers = {{3, r3}}, .memory = memory, .label = label};
        if (value_register) item.registers.emplace_back(value_register, value);
        item.registers.emplace_back(value_register == 4 ? 5 : 4, staff_record);
        cases.push_back(std::move(item));
    };
    for (const auto& [setting_memory, setting_label] : settings)
        for (const auto& [staff_record, staff_label] : staff) {
            const auto label = staff_label + setting_label;
            for (const auto entry : staff_in_r4) both(entry, humans, 0, 0, staff_record, setting_memory, label);
            both(set_short_setting, humans, 4, 0x8001, staff_record, setting_memory, label);
            both(set_setting, humans, 4, 0xdeadbeef, staff_record, setting_memory, label);
            both(flag, humans, 4, 3, staff_record, setting_memory, label);
            both(runs_nation, humans, 5, scratch_base + 0x10, staff_record, setting_memory, label);
            both(runs_nation, humans, 5, 0, staff_record, setting_memory, label + ", no refusal byte");
            cases.push_back({.entry = folder, .registers = {{3, humans}, {4, staff_record}, {5, 1}}, .memory = setting_memory,
                             .label = "folder, " + label, .answers = {{folder_path, 0}}});
        }

    // The values the fields hold, on the current human and on a human of another slot.
    for (const auto staff_record : {current_staff, scenario.person(staff_total - 1)}) {
        const auto human = human_record(staff_record);
        if (scenario.r32(staff_record) - staff_total + 16 >= count) continue;
        for (const std::uint32_t value : {0u, 1u, 0x7fu, 0x80u, 0xffu}) {
            for (const auto [entry, offset] : {std::pair{away, 0x9984u}, {holiday, 0x9a65u}, {runs_club, 0x9985u}})
                cases.push_back({.entry = entry, .registers = {{3, humans}, {4, staff_record}},
                                 .memory = {{1, human + offset, value}}, .label = "byte field " + std::to_string(value)});
            cases.push_back({.entry = runs_nation, .registers = {{3, humans}, {4, staff_record}, {5, scratch_base + 0x10}},
                             .memory = {{1, human + 0x9986, value}}, .label = "nation option " + std::to_string(value)});
            // The current tactic slot, sign-extended (the original reads whatever byte is there).
            for (const auto entry : tactic_fields)
                cases.push_back({.entry = entry, .registers = {{3, humans}, {4, staff_record}},
                                 .memory = {{1, human + 0x9a00, value}}, .label = "tactic slot " + std::to_string(value)});
            cases.push_back({.entry = current_tactic_slot, .registers = {{3, humans}, {4, staff_record}},
                             .memory = {{1, human + 0x9a00, value}}, .label = "tactic slot " + std::to_string(value)});
        }
        for (const std::uint32_t which : {0u, 1u, 7u, 0x7fu, 0x80u, 0xffu, 0x100u})
            cases.push_back({.entry = flag, .registers = {{3, humans}, {4, which}, {5, staff_record}},
                             .label = "flag " + std::to_string(which)});
        for (const std::uint32_t value : {0u, 1u, 0x7fffu, 0x8000u, 0xffffu, 0x18000u})
            cases.push_back({.entry = short_setting, .registers = {{3, humans}, {4, staff_record}},
                             .memory = {{2, human + 0x9a66, value & 0xffff}}, .label = "short setting " + std::to_string(value)}),
            cases.push_back({.entry = set_short_setting, .registers = {{3, humans}, {4, value}, {5, staff_record}},
                             .label = "set short setting " + std::to_string(value)});
        for (const std::uint32_t value : {0u, 1u, 0x80000000u, 0xffffffffu}) {
            cases.push_back({.entry = setting, .registers = {{3, humans}, {4, staff_record}},
                             .memory = {{4, human + 0x99fc, value}}, .label = "setting " + std::to_string(value)});
            cases.push_back({.entry = set_setting, .registers = {{3, humans}, {4, value}, {5, staff_record}},
                             .label = "set setting " + std::to_string(value)});
        }
        for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 0x7fu, 0x80u, 0xffu, 0x102u})
            cases.push_back({.entry = folder, .registers = {{3, humans}, {4, staff_record}, {5, kind}},
                             .label = "folder kind " + std::to_string(kind), .answers = {{folder_path, 0}}});
        // A human whose club's competition rates below 400 never runs a national side; at 400 the option counts.
        if (const auto job = scenario.r32(staff_record + 0x20)) {
            const auto competition = scenario.r32(job + 0x2c);
            for (const std::uint32_t rating : {0u, 399u, 400u, 0x80000000u, 0x7fffffffu})
                for (const std::uint32_t option : {0u, 1u})
                    for (const std::uint32_t refused : {0u, scratch_base + 0x10})
                        cases.push_back({.entry = runs_nation, .registers = {{3, humans}, {4, staff_record}, {5, refused}},
                                         .memory = {{4, competition + 0x6c, rating}, {1, human + 0x9986, option}, {1, scratch_base + 0x10, 0x55}},
                                         .label = "nation rating " + std::to_string(rating)});
        }
        // The same with a job given to the human (a club whose nation's rating word +0x6c is set), so the
        // rating test runs whatever the career's humans hold at +0x20.
        {
            const auto club = scenario.club(5), nation = scenario.r32(club + 0x2c);
            if (nation)
                for (const std::uint32_t rating : {0u, 399u, 400u, 0xffffffffu})
                    for (const std::uint32_t refused : {0u, scratch_base + 0x10})
                        cases.push_back({.entry = runs_nation, .registers = {{3, humans}, {4, staff_record}, {5, refused}},
                                         .memory = {{4, staff_record + 0x20, club}, {4, nation + 0x6c, rating},
                                                    {1, human + 0x9986, 1}, {1, scratch_base + 0x10, 0x55}},
                                         .label = "nation, given job, rating " + std::to_string(rating)});
        }
        for (const auto entry : record_fields)
            cases.push_back({.entry = entry, .registers = {{3, humans}, {4, staff_record}}, .label = "record field"});
    }

    // Whether a human runs a club: sampled clubs, the clubs humans manage and the national sides at the
    // end of the table, with each flag word (a low byte of zero counts as false) and each manager.
    const auto clubs = scenario.global(-0x7b50);
    std::vector<std::uint32_t> sample;
    for (std::uint32_t index = 0; index < clubs; index += clubs / 40 + 1) sample.push_back(scenario.club(index));
    for (std::uint32_t index = clubs > 4 ? clubs - 4 : 0; index < clubs; ++index) sample.push_back(scenario.club(index));
    std::vector<std::uint32_t> managed;
    for (std::uint32_t index = 0; index < clubs; ++index)
        if (const auto manager = scenario.r32(scenario.club(index) + 0x90);
            manager && std::int32_t(scenario.r32(manager)) >= std::int32_t(staff_total - 16))
            managed.push_back(scenario.club(index));
    const std::uint32_t flags[] = {0u, 1u, 0x100u};
    cases.push_back({.entry = runs, .registers = {{3, 0}, {4, 0}, {5, 0}, {6, 0}, {7, 0}}, .label = "runs, no club"});
    cases.push_back({.entry = current_human_runs, .registers = {{3, 0}, {4, 0}, {5, 0}}, .label = "current runs, no club"});
    for (const auto club : sample)
        for (const auto r4 : flags) for (const auto r5 : flags) {
            cases.push_back({.entry = current_human_runs, .registers = {{3, club}, {4, r4}, {5, r5}}, .label = "current runs, club"});
            for (const auto r7 : {0u, 1u})
                for (const auto manager : {0u, current_staff})
                    cases.push_back({.entry = runs, .registers = {{3, club}, {4, r4}, {5, r5}, {6, manager}, {7, r7}}, .label = "runs, club"});
        }
    // A human's own club with every option and absence: the current human's, then the other humans'.
    for (const auto club : managed) {
        const auto manager = scenario.r32(club + 0x90);
        if (scenario.r32(manager) - staff_total + 16 >= count) continue;
        const auto human = human_record(manager);
        for (unsigned bits = 0; bits < 16; ++bits) {
            const std::vector<Write> options{{1, human + 0x9a65, bits & 1}, {1, human + 0x9984, bits >> 1 & 1},
                                             {1, human + 0x9985, bits >> 2 & 1}, {1, human + 0x9986, bits >> 3 & 1}};
            for (const auto r4 : {0u, 1u}) for (const auto r5 : {0u, 1u}) {
                cases.push_back({.entry = current_human_runs, .registers = {{3, club}, {4, r4}, {5, r5}}, .memory = options,
                                 .label = "current runs, own club " + std::to_string(bits)});
                auto as_current = options;
                as_current.push_back({4, current_staff_cell, manager});
                cases.push_back({.entry = current_human_runs, .registers = {{3, club}, {4, r4}, {5, r5}}, .memory = as_current,
                                 .label = "current runs, own club as current " + std::to_string(bits)});
                for (const auto r7 : {0u, 1u})
                    for (const auto given : {0u, manager, scenario.person(0)})
                        cases.push_back({.entry = runs, .registers = {{3, club}, {4, r4}, {5, r5}, {6, given}, {7, r7}}, .memory = options,
                                         .label = "runs, own club " + std::to_string(bits)});
            }
        }
        // The club's manager below the humans' ids: never asked.
        cases.push_back({.entry = runs, .registers = {{3, club}, {4, 1}, {5, 1}, {6, 0}, {7, 1}},
                         .memory = {{4, manager, staff_total - 17}}, .label = "runs, manager below the humans"});
        // A national side managed by the human: the national question, then the nation's team.
        if (clubs >= 2) {
            const auto national = scenario.club(clubs - 1);
            for (const auto r5 : {0u, 1u})
                cases.push_back({.entry = runs, .registers = {{3, national}, {4, 1}, {5, r5}, {6, 0}, {7, 0}},
                                 .memory = {{4, national + 0x90, manager}, {1, human + 0x9986, 1}}, .label = "runs, national side"}),
                cases.push_back({.entry = current_human_runs, .registers = {{3, national}, {4, 1}, {5, r5}},
                                 .memory = {{4, national + 0x90, manager}, {4, current_staff_cell, manager}},
                                 .label = "current runs, national side"});
        }
    }
}
}
