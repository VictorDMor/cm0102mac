// HUMAN_MANAGER.CPP checker cases for the squad lists of a tactic slot, the recent file names and the
// squad flags (human_manager_tactics.hpp).
//
// The table is the game's own (global -0x7a54), the humans the career's. A case writes the fields a
// routine reads: the current tactic byte, ids into the squad lists, names into the name lists, a
// staff record's team; or changes the current human or the table's count to reach the failure paths.
#include "human_manager_checks.hpp"
#include <string>

namespace cm::aot::human_manager_checks {
namespace {
constexpr std::uint32_t record_size = 0xa65c, tactic_size = 0x1eb4, squad = 0x1dec;
constexpr std::uint32_t recent_path = 0x10147310, remember = 0x101476e0, set_flag = 0x10149f50, squad_player = 0x1014c890,
                        set_squad_player = 0x1014caa0, remove_squad_player = 0x1014ccf0, clear_squad_slot = 0x1014cf00,
                        swap_squad_slots = 0x1014d0f0, squad_slot_of = 0x1014d300;
// UTILS.CPP's folder path and the generated path builder 0x10739b40: file-system calls a snapshot cannot serve.
constexpr std::uint32_t folder_path = 0x10746300, build_path = 0x10739b40;
constexpr std::uint32_t squad_entries = 50;

// Writes of a text (and its terminator) as words, big-endian.
void text(std::vector<Write>& out, std::uint32_t address, const std::string& value) {
    std::string bytes = value;
    bytes.push_back('\0');
    while (bytes.size() % 4) bytes.push_back('\0');
    for (std::size_t at = 0; at < bytes.size(); at += 4) {
        std::uint32_t word = 0;
        for (std::size_t n = 0; n < 4; ++n) word = word << 8 | std::uint8_t(bytes[at + n]);
        out.push_back({4, address + std::uint32_t(at), word});
    }
}
}

HUMAN_MANAGER_CASES("tactics") {
    if (scenario.match) return;
    const auto humans = scenario.global(-0x7a54);
    const auto records = scenario.r32(humans), count = scenario.r32(humans + 4);
    const auto staff_total = scenario.people();
    const auto game = scenario.toc(-0x7bb0);
    const auto current = std::uint32_t(std::int32_t(std::int16_t(scenario.s.memory.read<2>(game + 0x2c58))));
    const auto current_staff_cell = game + current * 0x2c0 + 4;
    const auto current_staff = scenario.r32(current_staff_cell);
    const auto pool = scenario.toc(-0x54a0);
    const auto human_record = [&](std::uint32_t staff) {
        return records + (scenario.r32(staff) - staff_total + 16) * record_size;
    };
    // Whom to ask about: the current human (null and given), each id the table covers, the first id
    // below it and an ordinary member of staff (both outside the table).
    std::vector<std::pair<std::uint32_t, std::string>> staff{{0, "current"}, {current_staff, "current given"}};
    for (std::uint32_t slot = 0; slot < 16 && staff_total >= 16 - slot; ++slot)
        staff.emplace_back(scenario.person(staff_total - 16 + slot), "slot " + std::to_string(slot));
    if (staff_total > 17) staff.emplace_back(scenario.person(staff_total - 17), "below the table");
    staff.emplace_back(scenario.person(0), "ordinary staff");
    const std::vector<std::pair<std::vector<Write>, std::string>> settings{
        {{}, ""},
        {{{4, current_staff_cell, 0}}, ", no current human"},
        {{{4, humans + 4, 0}}, ", empty table"},
        {{{4, humans + 4, 1}}, ", table of one"},
        {{{4, humans + 4, 0x80000000u}}, ", table count INT_MIN"},
    };
    const auto player = scenario.person(staff_total > 300 ? 123 : 5);  // a person whose id the lists hold below

    // Registers of each routine: {entry, arguments before the staff}.
    const auto ask = [&](std::uint32_t entry, std::vector<std::uint32_t> arguments, std::uint32_t staff_record,
                         std::vector<Write> memory, const std::string& label) {
        Case item{.entry = entry, .memory = std::move(memory), .label = label};
        unsigned n = 3;
        item.registers.emplace_back(n++, humans);
        for (const auto value : arguments) item.registers.emplace_back(n++, value);
        item.registers.emplace_back(n, staff_record);
        cases.push_back(std::move(item));
    };
    // The routines with their other arguments: a valid entry index and a person's staff record.
    const auto each_routine = [&](std::uint32_t staff_record, const std::vector<Write>& memory, const std::string& label) {
        ask(squad_player, {0}, staff_record, memory, label);
        ask(set_squad_player, {0, player}, staff_record, memory, label);
        ask(remove_squad_player, {player}, staff_record, memory, label);
        ask(clear_squad_slot, {0}, staff_record, memory, label);
        ask(swap_squad_slots, {0, 1}, staff_record, memory, label);
        ask(squad_slot_of, {player}, staff_record, memory, label);
        // set_flag: staff in r6, so its arguments are (which, value).
        ask(set_flag, {3, 1}, staff_record, memory, label);
        ask(set_flag, {1, 1}, staff_record, memory, label);
    };
    for (const auto& [setting_memory, setting_label] : settings)
        for (const auto& [staff_record, staff_label] : staff) each_routine(staff_record, setting_memory, staff_label + setting_label);

    // The game record's current human moved to another entry: no staff there (the career's other humans are empty), or an entry beyond them.
    for (const std::uint32_t who : {1u, 0xffu, 2u})
        each_routine(0, {{2, game + 0x2c58, who}}, "current human index " + std::to_string(who));
    // The squad lists. Each slot's list gets ids that name the slot (the same id at entries 0 and 2,
    // a different one at 1 and at the last entry), so a read or write in the wrong slot shows.
    const auto id_of_person = [&](std::uint32_t id) { return scenario.person(id); };
    const std::uint32_t base_ids[] = {40, 41, 42, 43, 44};
    for (const auto staff_record : {current_staff, scenario.person(staff_total - 1)}) {
        if (scenario.r32(staff_record) - staff_total + 16 >= count) continue;
        const auto human = human_record(staff_record);
        for (const std::uint32_t slot_byte : {0u, 1u, 2u, 3u, 4u, 5u, 0x7fu, 0x80u, 0xffu}) {
            std::vector<Write> lists{{1, human + 0x9a00, slot_byte}};
            for (std::uint32_t slot = 0; slot < 5; ++slot) {
                const auto list = human + slot * tactic_size + squad;
                lists.push_back({4, list, base_ids[slot]});
                lists.push_back({4, list + 4, 9});
                lists.push_back({4, list + 8, base_ids[slot]});
                lists.push_back({4, list + 12, 0xffffffffu});
                lists.push_back({4, list + 4 * (squad_entries - 1), 60 + slot});
            }
            const auto label = "slot byte " + std::to_string(slot_byte);
            for (const std::int32_t which : {-128, -1, 0, 1, 2, 3, 48, 49, 50, 51, 127}) {
                const auto word = std::uint32_t(which);
                ask(squad_player, {word}, staff_record, lists, label + ", entry " + std::to_string(which));
                ask(set_squad_player, {word, id_of_person(base_ids[0])}, staff_record, lists, label + ", entry " + std::to_string(which));
                ask(clear_squad_slot, {word}, staff_record, lists, label + ", entry " + std::to_string(which));
            }
            ask(set_squad_player, {0, 0}, staff_record, lists, label + ", no player");
            ask(set_squad_player, {49, id_of_person(7)}, staff_record, lists, label + ", last entry");
            for (const auto id : {40u, 41u, 42u, 43u, 44u, 9u, 60u, 64u, 0u})
                for (const auto entry : {remove_squad_player, squad_slot_of})
                    ask(entry, {id_of_person(id)}, staff_record, lists, label + ", id " + std::to_string(id));
            ask(remove_squad_player, {0}, staff_record, lists, label + ", no player");
            ask(squad_slot_of, {0}, staff_record, lists, label + ", no player");
            for (const auto [first, second] : {std::pair{0, 1}, {1, 0}, {0, 49}, {49, 0}, {3, 3}, {2, 2}, {0, 50}, {50, 0}, {-1, 0}, {0, -1},
                                               {50, 50}, {-128, 0}, {0, -128}, {127, 1}, {1, 127}, {48, 49}})
                ask(swap_squad_slots, {std::uint32_t(first), std::uint32_t(second)}, staff_record, lists,
                    label + ", swap " + std::to_string(first) + " " + std::to_string(second));
        }
    }

    // The squad flags: each flag, the values the original reads as a byte (sign-extended to test),
    // and the staff's team (kept, none, other clubs).
    std::vector<std::uint32_t> teams{0};
    const auto club_count = scenario.global(-0x7b50);
    for (const auto index : {0u, 1u, club_count / 2, club_count > 2 ? club_count - 2 : 0u, club_count > 1 ? club_count - 1 : 0u})
        teams.push_back(scenario.club(index));
    for (const auto staff_record : {current_staff, scenario.person(staff_total - 1), scenario.person(staff_total - 9)}) {
        if (scenario.r32(staff_record) - staff_total + 16 >= count) continue;
        for (const std::int32_t which : {-1, 0, 1, 2, 3, 4, 7, 8, 127, -128})
            for (const std::uint32_t value : {0u, 1u, 0x80u, 0xffu, 0x100u})
                ask(set_flag, {std::uint32_t(which), value}, staff_record, {}, "flag " + std::to_string(which) + " := " + std::to_string(value));
        for (const auto team : teams)
            ask(set_flag, {3, 1}, staff_record, {{4, staff_record + 0x24, team}}, "squad list, team " + std::to_string(team));
        ask(set_flag, {3, 0x80}, staff_record, {{4, staff_record + 0x24, teams.back()}}, "squad list, negative value");
    }

    // The recent file names. The lists hold known names (kind 1 in the current slot, 3 training, 2 and
    // the others search/print), so a copy or a shift shows where it went.
    const std::string names[] = {"Alpha", "Beta", "gamma", "Delta"};
    for (const auto staff_record : {current_staff, scenario.person(staff_total - 1)}) {
        if (scenario.r32(staff_record) - staff_total + 16 >= count) continue;
        const auto human = human_record(staff_record);
        const auto buffer = scratch_base + 0x1000, name = scratch_base + 0x100;
        for (const std::uint32_t slot_byte : {0u, 1u, 2u, 0x80u}) {
            std::vector<Write> lists{{1, human + 0x9a00, slot_byte}};
            const auto slot_list = human + std::uint32_t(std::int32_t(std::int8_t(slot_byte))) * tactic_size + 0x19eb;
            for (unsigned n = 0; n < 4; ++n) {
                // The same four names in a different order per list, so a read or shift in the wrong list shows.
                text(lists, slot_list + n * 0x100, names[n]);
                text(lists, human + 0xa140 + n * 0x100, names[(n + 1) % 4]);
                text(lists, human + 40000 + n * 0x100, names[(n + 2) % 4]);
            }
            const auto label = "slot byte " + std::to_string(slot_byte);
            // Reading a name back: each kind, entry, and the buffer and size edges.
            for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 5u, 0x7fu, 0x80u, 0xffu, 0x101u})
                for (const std::uint32_t which : {0u, 1u, 3u, 4u, 0xffu, 0x80u}) {
                    for (const std::uint32_t bare : {0u, 1u}) {
                        Case item{.entry = recent_path,
                                  .registers = {{3, humans}, {4, which}, {5, buffer}, {6, 0x100}, {7, bare}, {8, staff_record}, {9, kind}},
                                  .memory = lists,
                                  .label = label + ", read kind " + std::to_string(kind) + " entry " + std::to_string(which),
                                  .answers = {{folder_path, 0}, {build_path, 0}}};
                        cases.push_back(std::move(item));
                    }
                }
            for (const std::uint32_t bare : {0x100u, 0x80u, 0x1u})
                cases.push_back({.entry = recent_path,
                                 .registers = {{3, humans}, {4, 1}, {5, buffer}, {6, 0x100}, {7, bare}, {8, staff_record}, {9, 1}},
                                 .memory = lists, .label = label + ", bare word " + std::to_string(bare),
                                 .answers = {{folder_path, 0}, {build_path, 0}}});
            for (const std::uint32_t size : {0u, 0xffu, 0x100u, 0x1000u, 0x80000000u, 0xffffffffu})
                cases.push_back({.entry = recent_path,
                                 .registers = {{3, humans}, {4, 2}, {5, buffer}, {6, size}, {7, 0}, {8, staff_record}, {9, 2}},
                                 .memory = lists, .label = label + ", size " + std::to_string(size),
                                 .answers = {{folder_path, 0}, {build_path, 0}}});
            cases.push_back({.entry = recent_path, .registers = {{3, humans}, {4, 1}, {5, 0}, {6, 0x100}, {7, 0}, {8, staff_record}, {9, 1}},
                             .memory = lists, .label = label + ", no buffer"});
            // Putting a name first: new, present at each place, in another case, empty.
            for (const std::int32_t kind : {0, 1, 2, 3, 4, 5, -1, 0x80, 0x101}) {
                for (const std::string& value : {std::string("Alpha.tct"), std::string("Beta.tct"), std::string("gamma.tsh"), std::string("GAMMA.tsh"),
                                                 std::string("Delta"), std::string("Omega.tct"), std::string(""), std::string("a.b.c"),
                                                 std::string("Alpha"), std::string("Beta"), std::string("GAMMA"), std::string("delta")}) {
                    auto memory = lists;
                    text(memory, name, value);
                    // The lists hold the names without the extension: the compared part is the name part.
                    cases.push_back({.entry = remember, .registers = {{3, humans}, {4, name}, {5, staff_record}, {6, std::uint32_t(kind)}},
                                     .memory = std::move(memory), .label = label + ", remember '" + value + "' kind " + std::to_string(kind)});
                }
            }
            for (const std::uint32_t pool_name : {0x37u, 0x5cu, 0x49u})
                cases.push_back({.entry = remember, .registers = {{3, humans}, {4, pool + pool_name}, {5, staff_record}, {6, 1}},
                                 .memory = lists, .label = label + ", remember pool text"});
        }
    }
    // The failure paths of the name routines, for each setting and member of staff.
    for (const auto& [setting_memory, setting_label] : settings)
        for (const auto& [staff_record, staff_label] : staff) {
            const auto label = staff_label + setting_label;
            for (const std::uint32_t kind : {1u, 3u, 2u, 4u}) {
                cases.push_back({.entry = recent_path,
                                 .registers = {{3, humans}, {4, 0}, {5, scratch_base + 0x1000}, {6, 0x100}, {7, 1}, {8, staff_record}, {9, kind}},
                                 .memory = setting_memory, .label = "read, " + label, .answers = {{folder_path, 0}, {build_path, 0}}});
                cases.push_back({.entry = remember, .registers = {{3, humans}, {4, pool + 0x5c}, {5, staff_record}, {6, kind}},
                                 .memory = setting_memory, .label = "remember, " + label});
            }
        }
    // The path of a name with the builder run for real (one case per kind).
    for (const std::uint32_t kind : {1u, 2u, 3u, 5u})
        cases.push_back({.entry = recent_path,
                         .registers = {{3, humans}, {4, 0}, {5, scratch_base + 0x1000}, {6, 0x100}, {7, 0}, {8, current_staff}, {9, kind}},
                         .label = "path, real builder, kind " + std::to_string(kind), .answers = {{folder_path, 0}}});
}
}
