// DISPLAY.CPP checker cases for the interpreter's commands 6, 7 and 8 (display_interpreter_build.cpp):
// byte streams built in scratch memory, run against both display tables of the real state.
#include "display_checks.hpp"
#include "display_detail.hpp"
#include <algorithm>
#include <string>
#include <vector>

namespace cm::aot::display_checks {
namespace {
constexpr std::uint32_t interpret = 0x1071de80;

// A command's byte stream (little-endian values, as the game's network form stores them).
struct Bytes {
    std::vector<std::uint8_t> data;
    std::vector<std::uint32_t> ends;  // the length after each value: where a truncated stream cuts a read
    Bytes& u8(std::uint32_t v) { data.push_back(std::uint8_t(v)); ends.push_back(std::uint32_t(data.size())); return *this; }
    Bytes& u16(std::uint32_t v) { u8(v); u8(v >> 8); ends.pop_back(); ends.back() = std::uint32_t(data.size()); return *this; }
    Bytes& u32(std::uint32_t v) { for (int n = 0; n < 4; ++n) data.push_back(std::uint8_t(v >> (8 * n))); ends.push_back(std::uint32_t(data.size())); return *this; }
    Bytes& text(const std::string& t) { u16(std::uint32_t(t.size())); for (const char ch : t) data.push_back(std::uint8_t(ch)); ends.back() = std::uint32_t(data.size()); return *this; }
};

constexpr std::uint32_t stream_address = scratch_base + 0x100, bytes_address = scratch_base + 0x200,
                        target_address = scratch_base + 0x2000;

Case make(std::uint32_t table, const Bytes& bytes, std::uint32_t length, std::string label,
          std::vector<std::pair<unsigned, std::uint32_t>> registers = {}, std::vector<Write> memory = {}, std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
    Case item{.entry = interpret, .registers = {{3, table}, {4, 0x1234}, {5, 0xabcd}, {6, stream_address}, {21, 0x42}}};
    for (const auto& r : registers) item.registers.push_back(r);
    item.memory = {{4, stream_address, bytes_address}, {4, stream_address + 4, 9}, {4, stream_address + 8, length}};
    for (std::size_t n = 0; n < bytes.data.size(); ++n) item.memory.push_back({1, bytes_address + std::uint32_t(n), bytes.data[n]});
    for (const auto& w : memory) item.memory.push_back(w);
    item.answers = std::move(answers);
    item.label = std::move(label);
    return item;
}

// Command 6: one GUI object, then (optionally) a second linked to it.
Bytes command_6(std::int32_t index, std::uint32_t flags, std::uint32_t text_target, const std::string& first_text,
                std::int32_t link, const std::string& second_text) {
    Bytes b;
    b.u8(6).u16(index).u32(flags).u16(10).u16(20).u16(110).u16(50).u8(3).u16(7).u32(0x11).u16(0x22).u16(0x33)
        .u32(0x44).u8(0xfe).u16(0x55).text(first_text).u16(9).u32(0x66).u32(text_target).u16(0x1234).u32(text_target);
    b.u16(link);
    if (link == -1 || link == index) return b;
    b.u16(0x7777).u32(0x1001).u32(0x1002).u32(0x1003).u32(0x1004);
    b.u32(flags).u16(11).u16(21).u16(111).u16(51).u8(0x83).u16(8).u32(0x12).u16(0x23).u16(0x34).u32(0x45).u8(1)
        .u16(0x56).text(second_text).u16(0xa).u32(0x67).u32(0x68).u16(0x4321).u32(text_target);
    return b;
}
}

DISPLAY_CASES("interpreter-2") {
    if (scenario.match) return;
    const auto game = scenario.toc(-0x7bb0);
    const std::vector<std::uint32_t> tables{game + 0x2c68, game + 0x12d6b0};
    const std::vector<std::pair<unsigned, std::uint32_t>> sane{{13, 7}, {17, 0}, {20, 0}, {22, 0x1234}};
    for (const auto table : tables) {
        const bool first = table == tables[0];
        const auto tag = std::string(first ? "table 1" : "table 2");
        // Command 6: complete streams, with and without the second object, the text copy and its flag.
        for (const auto& [flags, target] : std::vector<std::pair<std::uint32_t, std::uint32_t>>{{0, 0}, {8, 0}, {8, target_address}, {0, target_address}})
            for (const auto& [link, name] : std::vector<std::pair<std::int32_t, std::string>>{{-1, "no link"}, {3, "link to itself"}, {4, "linked"}, {9, "linked"}})
                for (const auto& text : {std::string(), std::string("Hello")}) {
                    const auto bytes = command_6(3, flags, target, text, link, text);
                    cases.push_back(make(table, bytes, std::uint32_t(bytes.data.size()),
                                         "command 6 " + tag + ", flags " + std::to_string(flags) + ", target " + std::to_string(target) +
                                             ", " + name + ", text '" + text + "'", sane));
                }
        // Truncated streams: every read of both blocks fails in turn (and the later, shorter ones succeed).
        // Flags 0: a stale text pointer (the cut that skips its read) is then never copied to.
        const auto whole = command_6(3, 0, 0, "ab", 4, "cd");
        std::vector<std::uint32_t> cuts;
        for (const auto end : whole.ends) { cuts.push_back(end - 1); cuts.push_back(end); }
        for (std::uint32_t length = 1; length <= whole.data.size(); ++length)
            if (first || std::find(cuts.begin(), cuts.end(), length) != cuts.end())
                cases.push_back(make(table, whole, length, "command 6 " + tag + ", cut at " + std::to_string(length), sane));
        // A text length that is stale after a failed read: cut right before it, with r17/r20 from the caller.
        {
            const auto empty = command_6(3, 0, 0, "", 4, "");
            cases.push_back(make(table, empty, 1, "command 6 " + tag + ", empty after the command", sane));
        }
    }
    // Command 7: two words and a text; the human row's flag byte decides on the second call.
    const auto row_flag = game + 0x2c61;
    // 0x1076e5f0 and 0x1076e920 draw to the window (GetPort), which a snapshot has not: answered.
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> text_answers{{0x1076e5f0, 0}, {0x1076e920, 0}};
    for (const auto table : tables) {
        const auto tag = std::string(table == tables[0] ? "table 1" : "table 2");
        for (const std::uint32_t flag : {0u, 1u, 0x80u})
            for (const auto& text : {std::string(), std::string("Hello world")}) {
                Bytes b;
                b.u8(7).u32(5).u32(6).text(text);
                cases.push_back(make(table, b, std::uint32_t(b.data.size()), "command 7 " + tag + ", flag " + std::to_string(flag) +
                                     ", text '" + text + "'", sane, {{1, row_flag, flag}}, text_answers));
            }
        Bytes whole;
        whole.u8(7).u32(0x10).u32(0x20).text("abc");
        for (std::uint32_t length = 1; length <= whole.data.size(); ++length)
            cases.push_back(make(table, whole, length, "command 7 " + tag + ", cut at " + std::to_string(length), sane, {{1, row_flag, 1}}, text_answers));
    }
    // Command 8: a sub-command byte, the object's index and the sub-command's values.
    const auto field = [](std::uint32_t table, std::uint32_t at) { return table + 0x130000 + at; };
    const auto sub_stream = [](std::uint32_t sub, std::int32_t index) {
        Bytes b;
        b.u8(8).u16(index).u8(sub);
        switch (sub) {
        case 0: b.u32(0x01020304); break;
        case 1: b.u16(11).u16(-22).u16(33).u16(-44); break;
        case 2: case 5: case 9: case 10: b.u32(0xa0b0c0d0 + sub); break;
        case 3: case 4: case 7: b.u16(0xbeef); break;
        case 6: b.u8(0x85); break;
        case 8: b.text("Sub text"); break;
        default: break;
        }
        return b;
    };
    for (const auto table : tables) {
        const auto tag = std::string(table == tables[0] ? "table 1" : "table 2");
        const std::vector<Write> open_gate{{2, table + display::table::announced_area_count, 0}};
        for (std::uint32_t sub = 0; sub <= 10; ++sub) {
            const auto b = sub_stream(sub, 5);
            const auto length = std::uint32_t(b.data.size());
            cases.push_back(make(table, b, length, "command 8 " + tag + ", sub-command " + std::to_string(sub), sane, open_gate));
            // The gate: counts that match (the stream goes on) and ones that do not (it ends).
            const std::vector<Write> match{{2, table + display::table::announced_area_count, 1}, {2, field(table, 0u - 0x625c), 1},
                                           {2, table + display::table::announced_guio_count, 2}, {2, field(table, 0u - 0x625a), 2}};
            auto wrong_area = match, wrong_guio = match;
            wrong_area[1].value = 9;
            wrong_guio[3].value = 9;
            cases.push_back(make(table, b, length, "command 8 " + tag + ", sub-command " + std::to_string(sub) + ", gate matches", sane, match));
            cases.push_back(make(table, b, length, "command 8 " + tag + ", sub-command " + std::to_string(sub) + ", area count differs", sane, wrong_area));
            cases.push_back(make(table, b, length, "command 8 " + tag + ", sub-command " + std::to_string(sub) + ", object count differs", sane, wrong_guio));
            // Cut at every length: the failing read leaves the slot as it was (the index slot too, so the
            // register for the sub-command is set by hand where its byte is missing).
            for (std::uint32_t cut = 1; cut < length; ++cut) {
                auto registers = sane;
                registers[0] = {13, sub};
                cases.push_back(make(table, b, cut, "command 8 " + tag + ", sub-command " + std::to_string(sub) + ", cut at " + std::to_string(cut),
                                     registers, open_gate));
            }
        }
        for (const std::uint32_t sub : {11u, 12u, 0x7fu, 0x80u, 0xffu}) {
            const auto b = sub_stream(sub, 5);
            cases.push_back(make(table, b, std::uint32_t(b.data.size()), "command 8 " + tag + ", sub-command " + std::to_string(sub), sane, open_gate));
        }
    }
}
}
