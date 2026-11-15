// HUMAN_MANAGER.CPP checker cases for each human's sorted list of 0x20-byte entries
// (human_manager_lists.hpp).
//
// The lists are built in the scratch area: every record of the career's table gets one (entries
// pointer, capacity, count and the entries: key word, text from +4), so any staff record the table
// covers reaches it. The keys and texts sit in scratch too.
#include "human_manager_checks.hpp"
#include <string>

namespace cm::aot::human_manager_checks {
namespace {
constexpr std::uint32_t record_size = 0xa65c, entries_at = 0xa650, capacity_at = 0xa654, count_at = 0xa658;
constexpr std::uint32_t remove_everywhere = 0x1014b690, add_entry = 0x1014b810, remove_entry = 0x1014bb50,
                        entry_text = 0x1014bdc0, has_entry = 0x1014bf80, set_entry_text = 0x1014c0e0, comparator = 0x1014d520;
constexpr std::uint32_t allocate = 0x10775130;
constexpr std::uint32_t key_cell = scratch_base + 0x100;
constexpr std::uint32_t area = scratch_base + 0x1000, area_stride = 0x800;
// Texts: short, 26 characters (allowed), 27 (too long), empty.
constexpr std::uint32_t text_short = scratch_base + 0x200, text_26 = scratch_base + 0x240, text_27 = scratch_base + 0x280,
                        text_empty = scratch_base + 0x2c0;

struct Shape {
    std::string name;
    std::vector<std::uint32_t> keys;
    std::uint32_t capacity;
    bool null_list;  // entries pointer null (count and capacity as given)
};

void text_writes(std::vector<Write>& out, std::uint32_t at, const std::string& text) {
    for (std::size_t i = 0; i <= text.size(); ++i) out.push_back({1, at + std::uint32_t(i), i < text.size() ? std::uint32_t(std::uint8_t(text[i])) : 0u});
}
void entry_writes(std::vector<Write>& out, std::uint32_t address, const std::vector<std::uint32_t>& keys) {
    for (std::size_t i = 0; i < keys.size(); ++i) {
        out.push_back({4, address + std::uint32_t(i) * 0x20, keys[i]});
        text_writes(out, address + std::uint32_t(i) * 0x20 + 4, "e" + std::to_string(i));
    }
}
// Gives record `index` the list, at its own scratch region.
void list_writes(std::vector<Write>& out, std::uint32_t records, std::uint32_t index, const Shape& shape) {
    const auto record = records + index * record_size, address = area + index * area_stride;
    out.push_back({4, record + entries_at, shape.null_list ? 0 : address});
    out.push_back({4, record + capacity_at, shape.capacity});
    out.push_back({4, record + count_at, std::uint32_t(shape.keys.size())});
    if (!shape.null_list) entry_writes(out, address, shape.keys);
}
}

HUMAN_MANAGER_CASES("lists") {
    if (scenario.match) return;
    const auto humans = scenario.global(-0x7a54);
    const auto records = scenario.r32(humans), count = scenario.r32(humans + 4);
    const auto staff_total = scenario.people();
    const auto game = scenario.toc(-0x7bb0);
    const auto current = std::uint32_t(std::int32_t(std::int16_t(scenario.s.memory.read<2>(game + 0x2c58))));
    const auto current_staff_cell = game + current * 0x2c0 + 4;
    const auto current_staff = scenario.r32(current_staff_cell);
    if (count == 0 || count > 16 || staff_total < 17) return;
    const auto current_index = scenario.r32(current_staff) - staff_total + 16;
    if (current_index >= count) return;
    const auto other_staff = scenario.person(staff_total - 16 + (current_index == 0 ? 1 : 0));

    const std::vector<Shape> shapes{
        {"empty", {}, 10, false},
        {"one", {20}, 10, false},
        {"many", {10, 20, 30, 40, 50}, 10, false},
        {"duplicates", {10, 20, 20, 30}, 10, false},
        {"wide keys", {1, 0x40000000, 0x80000001u, 0xc0000000u}, 10, false},
        {"full", {10, 20, 30, 40, 50}, 5, false},
        {"null list", {}, 0, true},
    };
    const std::pair<std::uint32_t, std::string> lookups[] = {{10, "first"}, {30, "middle"}, {50, "last"}, {20, "twenty"},
                                                             {25, "absent"}, {5, "below"}, {99, "above"}, {0, "zero"},
                                                             {0x80000001u, "wide"}};
    const auto with_key = [&](std::vector<Write> memory, std::uint32_t key) {
        memory.push_back({4, key_cell, key});
        return memory;
    };
    const auto all_lists = [&](const Shape& shape) {
        std::vector<Write> out;
        for (std::uint32_t index = 0; index < count; ++index) list_writes(out, records, index, shape);
        return out;
    };
    // One list per routine and staff: the current human (null and given), another human.
    const std::pair<std::uint32_t, std::string> people[] = {{0, "current"}, {current_staff, "current given"}, {other_staff, "other"}};
    for (const auto& shape : shapes) {
        for (const auto& [key, key_name] : lookups) {
            const auto memory = with_key(all_lists(shape), key);
            const auto label = shape.name + ", key " + key_name;
            for (const auto& [staff, staff_name] : people) {
                if (shape.null_list && staff == other_staff) continue;
                cases.push_back({.entry = remove_entry, .registers = {{3, humans}, {4, key_cell}, {5, staff}}, .memory = memory, .label = "remove, " + label + ", " + staff_name});
                cases.push_back({.entry = entry_text, .registers = {{3, humans}, {4, key_cell}, {5, staff}}, .memory = memory, .label = "text of, " + label + ", " + staff_name});
                cases.push_back({.entry = has_entry, .registers = {{3, humans}, {4, key_cell}, {5, staff}}, .memory = memory, .label = "has, " + label + ", " + staff_name});
                for (const auto text : {text_short, text_26, text_empty})
                    cases.push_back({.entry = set_entry_text, .registers = {{3, humans}, {4, key_cell}, {5, text}, {6, staff}}, .memory = memory,
                                     .label = "set text, " + label + ", " + staff_name});
            }
            cases.push_back({.entry = remove_everywhere, .registers = {{3, humans}, {4, key_cell}}, .memory = memory, .label = "remove everywhere, " + label});
        }
    }
    // A key pointer of null: every routine returns before reading anything.
    for (const auto entry : {remove_entry, entry_text, has_entry})
        cases.push_back({.entry = entry, .registers = {{3, humans}, {4, 0}, {5, 0}}, .memory = all_lists(shapes[2]), .label = "no key"});
    cases.push_back({.entry = set_entry_text, .registers = {{3, humans}, {4, 0}, {5, text_short}, {6, 0}}, .memory = all_lists(shapes[2]), .label = "set text, no key"});
    cases.push_back({.entry = add_entry, .registers = {{3, humans}, {4, 0}, {5, text_short}, {6, 0}}, .memory = all_lists(shapes[2]), .label = "add, no key"});
    cases.push_back({.entry = remove_everywhere, .registers = {{3, humans}, {4, 0}}, .memory = all_lists(shapes[2]), .label = "remove everywhere, no key"});

    // Text too long for set_entry_text (27 characters) is refused before the search.
    {
        auto memory = with_key(all_lists(shapes[2]), 30);
        text_writes(memory, text_short, "AB");
        text_writes(memory, text_26, std::string(26, 'z'));
        text_writes(memory, text_27, std::string(27, 'y'));
        for (const auto& [staff, staff_name] : people)
            for (const auto text : {text_26, text_27})
                cases.push_back({.entry = set_entry_text, .registers = {{3, humans}, {4, key_cell}, {5, text}, {6, staff}}, .memory = memory,
                                 .label = "set text long, " + staff_name});
        // Adding: every list shape against new, present and duplicate keys, texts of each length.
        for (const auto& shape : shapes) {
            for (const std::uint32_t key : {5u, 20u, 25u, 55u, 0u, 0xffffffffu, 0x80000000u}) {
                auto added = with_key(all_lists(shape), key);
                text_writes(added, text_short, "AB");
                text_writes(added, text_26, std::string(26, 'z'));
                text_writes(added, text_27, std::string(27, 'y'));
                for (const auto text : {text_short, text_26, text_27, text_empty})
                    for (const auto& [staff, staff_name] : people) {
                        // A full list in scratch cannot be reallocated by the game's allocator: those are
                        // reached through a null list below.
                        if ((shape.null_list && staff == other_staff) || (shape.capacity == shape.keys.size() && !shape.null_list)) continue;
                        cases.push_back({.entry = add_entry, .registers = {{3, humans}, {4, key_cell}, {5, text}, {6, staff}}, .memory = added,
                                         .label = "add, " + shape.name + ", key " + std::to_string(key) + ", " + staff_name});
                    }
            }
            // The allocator refusing a new list (answered; the game's realloc is not answered by the checker).
            if (shape.null_list) {
                auto added = with_key(all_lists(shape), 25);
                text_writes(added, text_short, "AB");
                cases.push_back({.entry = add_entry, .registers = {{3, humans}, {4, key_cell}, {5, text_short}, {6, 0}}, .memory = added,
                                 .label = "add, " + shape.name + ", allocation fails", .answers = {{allocate, 0}}});
            }
        }
        // The realloc failure (0x1014ba8c, line 0x9c6, then the fatal exit): the facade 0x107751b0 runs in
        // place, but its heap resize 0x10774f40 is dispatched on both sides, so that is answered with null.
        {
            auto grown = with_key(all_lists(shapes[6]), 25);
            text_writes(grown, text_short, "AB");
            grown.push_back({4, records + current_index * record_size + count_at, 10});
            cases.push_back({.entry = add_entry, .registers = {{3, humans}, {4, key_cell}, {5, text_short}, {6, 0}}, .memory = grown,
                             .label = "add, null list holding 10, realloc fails", .answers = {{0x10774f40, 0}}});
        }
        // A new list where the allocator is the game's own; with a count equal to the capacity the
        // allocation sets (10) the list then grows by the game's realloc too.
        for (const std::uint32_t listed : {0u, 3u, 10u}) {
            auto grown = with_key(all_lists(shapes[6]), 25);
            text_writes(grown, text_short, "AB");
            grown.push_back({4, records + current_index * record_size + count_at, listed});
            cases.push_back({.entry = add_entry, .registers = {{3, humans}, {4, key_cell}, {5, text_short}, {6, 0}}, .memory = grown,
                             .label = "add, null list holding " + std::to_string(listed) + ", real allocation"});
        }
    }

    // The staff and table checks of every routine with a staff argument.
    const std::vector<std::pair<std::vector<Write>, std::string>> settings{
        {{}, ""},
        {{{4, current_staff_cell, 0}}, ", no current human"},
        {{{4, humans + 4, 0}}, ", empty table"},
        {{{4, humans + 4, 1}}, ", table of one"},
        {{{4, humans + 4, 0x80000000u}}, ", table count INT_MIN"},
    };
    std::vector<std::pair<std::uint32_t, std::string>> who{{0, "current"}, {current_staff, "current given"},
                                                           {scenario.person(staff_total - 17), "below the table"},
                                                           {scenario.person(0), "ordinary staff"}};
    for (std::uint32_t slot : {0u, 15u}) who.emplace_back(scenario.person(staff_total - 16 + slot), "slot " + std::to_string(slot));
    for (const auto& [setting, setting_label] : settings)
        for (const auto& [staff, who_label] : who) {
            auto memory = with_key(all_lists(shapes[2]), 30);
            text_writes(memory, text_short, "AB");
            for (const auto& write : setting) memory.push_back(write);
            const auto label = who_label + setting_label;
            cases.push_back({.entry = remove_entry, .registers = {{3, humans}, {4, key_cell}, {5, staff}}, .memory = memory, .label = "remove, " + label});
            cases.push_back({.entry = entry_text, .registers = {{3, humans}, {4, key_cell}, {5, staff}}, .memory = memory, .label = "text of, " + label});
            cases.push_back({.entry = has_entry, .registers = {{3, humans}, {4, key_cell}, {5, staff}}, .memory = memory, .label = "has, " + label});
            cases.push_back({.entry = set_entry_text, .registers = {{3, humans}, {4, key_cell}, {5, text_short}, {6, staff}}, .memory = memory, .label = "set text, " + label});
            cases.push_back({.entry = add_entry, .registers = {{3, humans}, {4, key_cell}, {5, text_short}, {6, staff}}, .memory = memory, .label = "add, " + label});
        }

    // Removing a key from every list: partial tables, a record without a list in the middle (the
    // routine returns from inside its loop), and a table of none (the sort goes to the register's value).
    for (const std::uint32_t table_count : {1u, 2u, count}) {
        for (const auto& shape : {shapes[2], shapes[3], shapes[1]})
            for (const std::uint32_t key : {20u, 30u, 99u}) {
                auto memory = with_key(all_lists(shape), key);
                memory.push_back({4, humans + 4, table_count});
                cases.push_back({.entry = remove_everywhere, .registers = {{3, humans}, {4, key_cell}}, .memory = memory,
                                 .label = "remove everywhere, table of " + std::to_string(table_count) + ", " + shape.name});
            }
        for (std::uint32_t hole = 0; hole < table_count; hole += (table_count > 3 ? 5 : 1)) {
            auto memory = with_key(all_lists(shapes[2]), 30);
            memory.push_back({4, humans + 4, table_count});
            memory.push_back({4, records + hole * record_size + entries_at, 0});
            cases.push_back({.entry = remove_everywhere, .registers = {{3, humans}, {4, key_cell}}, .memory = memory,
                             .label = "remove everywhere, no list at " + std::to_string(hole)});
        }
    }
    {
        auto memory = with_key(all_lists(shapes[2]), 30);
        memory.push_back({4, humans + 4, 0});
        // r29 at entry stands for the caller's: a record whose list the routine sorts.
        cases.push_back({.entry = remove_everywhere, .registers = {{3, humans}, {4, key_cell}, {29, records + current_index * record_size}},
                         .memory = memory, .label = "remove everywhere, empty table"});
        auto negative = with_key(all_lists(shapes[2]), 30);
        negative.push_back({4, humans + 4, 0xfffffff0u});
        cases.push_back({.entry = remove_everywhere, .registers = {{3, humans}, {4, key_cell}, {29, records + current_index * record_size}},
                         .memory = negative, .label = "remove everywhere, negative count"});
    }

    // The comparator: rows in scratch, null rows, equal keys and keys far apart (the difference wraps).
    constexpr std::uint32_t row_a = scratch_base + 0x3000, row_b = scratch_base + 0x3020;
    cases.push_back({.entry = comparator, .registers = {{3, 0}, {4, 0}}, .label = "compare, both null"});
    cases.push_back({.entry = comparator, .registers = {{3, 0}, {4, row_b}}, .label = "compare, first null"});
    cases.push_back({.entry = comparator, .registers = {{3, row_a}, {4, 0}}, .label = "compare, second null"});
    for (const std::uint32_t first : {0u, 1u, 10u, 0x7fffffffu, 0x80000000u, 0x80000001u, 0xffffffffu})
        for (const std::uint32_t second : {0u, 1u, 10u, 0x7fffffffu, 0x80000000u, 0x80000001u, 0xffffffffu})
            cases.push_back({.entry = comparator, .registers = {{3, row_a}, {4, row_b}},
                             .memory = {{4, row_a, first}, {4, row_b, second}},
                             .label = "compare " + std::to_string(first) + " " + std::to_string(second)});
}
}
