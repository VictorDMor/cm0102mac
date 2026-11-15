// DISPLAY.CPP checker cases for the interpreter's commands 9-26 (display_interpreter_controls.cpp): each
// command on byte streams built in scratch memory against both display tables of the real main object,
// with every stream read cut short in turn, the table's list and the TOC objects present and absent, and
// the other modules' routines the commands call answered where the snapshot cannot run them.
#include "display_checks.hpp"
#include "display_detail.hpp"
#include <string>
#include <vector>

namespace cm::aot::display_checks {
namespace {
constexpr std::uint32_t interpret = 0x1071de80;
using Bytes = std::vector<std::uint8_t>;
// The stream's halfwords and words are little-endian (the commands byte-swap them).
Bytes le16(std::uint32_t v) { return {std::uint8_t(v), std::uint8_t(v >> 8)}; }
Bytes le32(std::uint32_t v) { return {std::uint8_t(v), std::uint8_t(v >> 8), std::uint8_t(v >> 16), std::uint8_t(v >> 24)}; }
Bytes join(std::initializer_list<Bytes> parts) {
    Bytes all;
    for (const auto& part : parts) all.insert(all.end(), part.begin(), part.end());
    return all;
}
// A case that interprets `command` followed by `payload` with the stream cut at `length` bytes.
Case make(std::uint32_t table, std::uint32_t command, const Bytes& payload, std::size_t length, std::string label) {
    const auto stream = scratch_base + 0x100, bytes = scratch_base + 0x200;
    Case item{.entry = interpret,
              // stale values the failed reads leave: small so a copy of "r13 bytes" stays inside the frame
              .registers = {{3, table}, {4, 0x1234}, {5, 0xabcd}, {6, stream}, {13, 3}, {17, 0x55}, {20, 7}, {21, 0x42}, {24, 9}},
              .memory = {{4, stream, bytes}, {4, stream + 4, 0}, {4, stream + 8, std::uint32_t(length)}, {1, bytes, command}},
              .label = std::move(label)};
    for (std::size_t i = 0; i < payload.size(); ++i) item.memory.push_back({1, bytes + 1 + std::uint32_t(i), payload[i]});
    return item;
}
void half(Case& item, std::uint32_t at, std::uint32_t value) { item.memory.push_back({2, at, value}); }
}

DISPLAY_CASES("interpreter-3") {
    if (scenario.match) return;
    const auto game = scenario.toc(-0x7bb0);
    const auto out = scenario.toc(-0x5c04);                 // the output text buffer object {bytes, position, length}
    const auto r19_word = scenario.toc(-0x7468);            // the object commands 11-12 hand to the engine
    const auto scratch = scratch_base + 0x4000;             // a list record, its array and strings
    // The allocator runs for real on both sides (the original calls it without the dispatcher), so the list's
    // array is a real heap block (the club table) whose first words a case overwrites (undone afterwards),
    // and its elements are the people table or null.
    const auto list_array = scenario.club(0), element = scenario.global(-0x7be0);
    for (const auto table : {game + 0x2c68, game + 0x12d6b0}) {
        const auto high = table + 0x130000;
        const auto name = std::string(table == game + 0x2c68 ? "first table" : "second table");
        const auto add = [&](Case item) { item.label = name + ", " + item.label; cases.push_back(std::move(item)); };
        // The table's list record: {array, count, capacity halfword}.
        const auto with_list = [&](Case item, std::uint32_t count) {
            item.memory.push_back({4, table + display::table::string_list, scratch});
            item.memory.push_back({4, scratch, list_array});
            item.memory.push_back({4, scratch + 4, count});
            item.memory.push_back({2, scratch + 8, 9});
            for (std::uint32_t i = 0; i < 4; ++i) item.memory.push_back({4, list_array + i * 4, i == 0 ? element : 0});
            return item;
        };
        const auto with_object = [&](Case item) {  // the TOC -0x7468 word points at an engine object
            item.memory.push_back({4, r19_word, scratch_base + 0x800});
            return item;
        };
        // Command 9: the flag halfword against the table's counts, then the marks and the call.
        for (const std::uint32_t flag : {0u, 3u, 4u})
            for (const std::uint32_t guio_flag : {0u, 2u})
                for (const std::uint32_t third : {0u, 1u}) {
                    auto item = make(table, 9, {}, 1, "command 9, flag " + std::to_string(flag) + ", guio " + std::to_string(guio_flag) +
                                                     ", third " + std::to_string(third));
                    half(item, table + display::table::announced_area_count, flag);
                    half(item, table + display::table::announced_guio_count, guio_flag);
                    half(item, high - 0x625c, 3);
                    half(item, high - 0x625a, 2);
                    item.memory.push_back({1, high - 0x56c0, third});
                    item.answers.push_back({0x10718fc0, 0});
                    add(item);
                }
        // Command 10: the text message, every read cut short, the output buffer's room, the engine object.
        const auto message = join({le32(0x11223344), {5}, {6}, le16(3), {'a', 'b', 0}});
        const auto empty_text = join({le32(0x11223344), {5}, {6}, le16(1), {0}});  // 0x10723650 returns at once on "" (a name needs files)
        for (std::size_t length = 1; length <= message.size() + 1; ++length)
        {
            auto item = make(table, 10, message, length, "command 10, length " + std::to_string(length));
            item.answers.push_back({0x1073a4c0, 1});
            add(item);
        }
        for (const std::uint32_t room : {0u, 1u, 4u, 5u, 6u, 0x100u}) {
            for (const std::uint32_t handed : {1u, 0u}) {
                for (const bool engine : {false, true}) {
                    if (engine && room != 0x100) continue;
                    const auto& sent = engine ? empty_text : message;
                    auto item = make(table, 10, sent, sent.size() + 1,
                                     "command 10, room " + std::to_string(room) + ", result " + std::to_string(handed) + (engine ? ", engine" : ""));
                    item.memory.push_back({4, out + 8, room});
                    item.answers.push_back({0x1073a4c0, handed});
                    if (engine) {
                        item = with_object(item);
                        item.answers.push_back({0x10744860, 5});
                        item.answers.push_back({0x10744ca0, 0});
                    }
                    add(item);
                }
            }
        }
        // Command 11: a byte, -1 or not, with and without the engine object.
        for (const std::uint32_t value : {0xffu, 5u})
            for (const bool engine : {false, true})
                for (const std::size_t length : {1u, 2u}) {
                    auto item = make(table, 11, {std::uint8_t(value)}, length,
                                     "command 11, " + std::to_string(value) + ", length " + std::to_string(length) + (engine ? ", engine" : ""));
                    if (engine) {
                        item = with_object(item);
                        item.answers.push_back({0x10744700, 0});
                        item.answers.push_back({0x10744770, 0});
                    }
                    add(item);
                }
        // Command 12: byte and halfword.
        for (const bool engine : {false, true})
            for (std::size_t length = 1; length <= 4; ++length) {
                auto item = make(table, 12, join({{4}, le16(0xfff0)}), length,
                                 "command 12, length " + std::to_string(length) + (engine ? ", engine" : ""));
                if (engine) {
                    item = with_object(item);
                    item.answers.push_back({0x10744ca0, 0});
                }
                add(item);
            }
        // Commands 13, 14: the game object calls.
        for (std::size_t length = 1; length <= 6; ++length) {
            auto item = make(table, 13, join({le32(0x01020304), {7}}), length, "command 13, length " + std::to_string(length));
            item.answers.push_back({0x10742e60, 0});
            add(item);
        }
        for (std::size_t length = 1; length <= 2; ++length) {
            auto item = make(table, 14, {7}, length, "command 14, length " + std::to_string(length));
            item.answers.push_back({0x10742e90, 0});
            add(item);
        }
        // Command 15: the human row; the current human as it is (-1) and 0 and 2.
        for (const std::uint32_t human : {0xffffu, 0u, 2u})
            for (std::size_t length = 1; length <= 8; ++length) {
                auto item = make(table, 15, join({le16(0x1234), le32(0xdeadbeef)}), length,
                                 "command 15, human " + std::to_string(human) + ", length " + std::to_string(length));
                half(item, game + 0x2c58, human);
                add(item);
            }
        // Command 16: an area index (also negative) and a word for 0x107145f0.
        for (const std::uint32_t index : {0u, 3u, 0xffffu})
            for (std::size_t length = 1; length <= 8; ++length) {
                auto item = make(table, 16, join({le16(index), le32(0x01020304)}), length,
                                 "command 16, index " + std::to_string(index) + ", length " + std::to_string(length));
                item.answers.push_back({0x107145f0, 0});
                add(item);
            }
        // Commands 17 and 18: the bytes the TOC words -0x744c and -0x5c00 point to are cleared.
        for (const std::uint32_t command : {17u, 18u}) {
            auto item = make(table, command, {}, 1, "command " + std::to_string(command));
            item.memory.push_back({1, scenario.toc(command == 17 ? -0x744c : -0x5c00), 1});
            add(item);
        }
        // Commands 19, 20.
        for (const std::uint32_t command : {19u, 20u})
            for (std::size_t length = 1; length <= 3; ++length)
                add(make(table, command, le16(0x8123), length, "command " + std::to_string(command) + ", length " + std::to_string(length)));
        // Commands 21, 22: the engine call; 22 with and without the list.
        {
            auto item = make(table, 21, {}, 1, "command 21");
            item.answers.push_back({0x1074cd90, 0});
            add(item);
            auto none = make(table, 22, {}, 1, "command 22, no list");
            none.answers.push_back({0x1074cd90, 0});
            add(none);
            auto listed = with_list(make(table, 22, {}, 1, "command 22, list"), 2);
            listed.memory.push_back({2, scratch + 8, 0xfff0});
            listed.answers.push_back({0x1074cd90, 0});
            add(listed);
        }
        // Command 23: allocate the list (the allocator runs for real on both sides: the original calls it
        // without the dispatcher) and read its capacity.
        for (std::size_t length = 1; length <= 3; ++length)
            add(make(table, 23, le16(0x0102), length, "command 23, length " + std::to_string(length)));
        // Command 24: free the list (none, empty, some elements, negative count).
        {
            auto none = make(table, 24, {}, 1, "command 24, no list");
            add(none);
            for (const std::uint32_t count : {0u, 1u, 3u, 0x80000000u}) {
                auto item = with_list(make(table, 24, {}, 1, "command 24, count " + std::to_string(count)), count);
                add(item);
            }
        }
        // Commands 25 and 26: append a text copy or a null; a count too large for the allocator makes it refuse.
        for (const std::uint32_t count : {2u, 0x3fffffffu}) {
            for (std::size_t length = 1; length <= 6; ++length)
                add(with_list(make(table, 25, join({le16(3), {'h', 'i', 0}}), length,
                                   "command 25, count " + std::to_string(count) + ", length " + std::to_string(length)), count));
            add(with_list(make(table, 26, {}, 1, "command 26, count " + std::to_string(count)), count));
        }
        add(make(table, 25, join({le16(3), {'h', 'i', 0}}), 6, "command 25, no list"));
        add(make(table, 26, {}, 1, "command 26, no list"));
    }
}
}
