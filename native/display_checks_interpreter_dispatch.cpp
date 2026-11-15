// DISPLAY.CPP checker cases for the interpreter's commands 0-5 (display_interpreter.cpp): byte streams built
// in scratch memory against the career's two display tables.
#include "display_checks.hpp"
#include "display_detail.hpp"
#include <string>
#include <vector>

namespace cm::aot::display_checks {
namespace {
constexpr std::uint32_t interpret = 0x1071de80;
constexpr std::uint32_t stream_at = scratch_base + 0x100, bytes_at = scratch_base + 0x200;
constexpr std::uint32_t high = 0x130000;

// A command's bytes (the stream's halfwords and words are little-endian, as the original's PC form) and the offsets where each field starts.
struct Bytes {
    std::vector<std::uint8_t> data;
    std::vector<std::uint32_t> cuts;
    Bytes& u8(std::uint32_t v) { cuts.push_back(std::uint32_t(data.size())); data.push_back(std::uint8_t(v)); return *this; }
    Bytes& u16(std::uint32_t v) { cuts.push_back(std::uint32_t(data.size())); data.push_back(std::uint8_t(v)); data.push_back(std::uint8_t(v >> 8)); return *this; }
    Bytes& u32(std::uint32_t v) {
        cuts.push_back(std::uint32_t(data.size()));
        for (int shift = 0; shift <= 24; shift += 8) data.push_back(std::uint8_t(v >> shift));
        return *this;
    }
    // The game's strings end with a zero byte, and the copy (0x10739ef0) runs to it.
    Bytes& text(const std::string& s) { for (const auto ch : s) data.push_back(std::uint8_t(ch)); data.push_back(0); return *this; }
};

struct Setup {
    std::vector<Write> memory;  // extra writes (the table's counts and flags)
    std::uint32_t r4 = 0x1234, r5 = 0xabcd;
    std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {};
};

void add(std::vector<Case>& cases, const std::string& label, std::uint32_t table, const Bytes& bytes, std::uint32_t length,
         const Setup& setup = {}) {
    Case entry{.entry = interpret, .registers = {{3, table}, {4, setup.r4}, {5, setup.r5}, {6, stream_at}, {21, 0x42},
                              // the registers a failed read leaves as the caller had them: benign values
                              {13, 0}, {17, 0}, {20, 0}, {22, 4}, {23, 4}, {24, 0}},
               .memory = {{4, stream_at, bytes_at}, {4, stream_at + 4, 0}, {4, stream_at + 8, length}}, .label = label, .answers = setup.answers};
    for (std::uint32_t i = 0; i < bytes.data.size(); ++i) entry.memory.push_back({1, bytes_at + i, bytes.data[i]});
    for (const auto& w : setup.memory) entry.memory.push_back(w);
    cases.push_back(std::move(entry));
}
// The cuts that make each field's read fail in turn, then the whole stream.
void add_cuts(std::vector<Case>& cases, const std::string& label, std::uint32_t table, const Bytes& bytes, const Setup& setup,
              std::uint32_t first_cut = 0) {
    for (const auto cut : bytes.cuts)
        if (cut >= first_cut) add(cases, label + ", cut at " + std::to_string(cut), table, bytes, cut + 1, setup);
    add(cases, label + ", whole", table, bytes, std::uint32_t(bytes.data.size()), setup);
}
// One GUI object of command 1: the flag word and the fields its bits select, in the original's order.
// `area` is the value of the 0x40000 field (the area index, -1 for none); `text` is the 0x4000 string.
Bytes& guio(Bytes& b, std::uint32_t flags, std::uint32_t area = 0, const std::string& text = "", std::uint32_t first_word = 0) {
    b.u32(flags);
    if (flags & 0x1) b.u32(first_word);
    if (flags & 0x2) b.u32(0x11);
    if (flags & 0x4) b.u32(0x22);
    if (flags & 0x8) b.u16(5);
    if (flags & 0x10) b.u16(6);
    if (flags & 0x20) b.u16(7);
    if (flags & 0x40) b.u16(8);
    if (flags & 0x80) b.u8(9);
    if (flags & 0x100) b.u16(10);
    if (flags & 0x200) b.u16(11);
    if (flags & 0x400) b.u16(12);
    if (flags & 0x800) b.u32(13);
    if (flags & 0x1000) b.u8(14);
    if (flags & 0x2000) b.u16(15);
    if (flags & 0x4000) { b.u16(std::uint32_t(text.size()) + 1); b.text(text); }
    if (flags & 0x8000) b.u16(16);
    if (flags & 0x10000) b.u32(17);
    if (flags & 0x20000) b.u32(18);
    if (flags & 0x40000) b.u16(area);
    if (flags & 0x80000) b.u16(0);
    return b;
}
}

DISPLAY_CASES("interpreter-1") {
    if (scenario.match) return;
    const auto game = scenario.toc(-0x7bb0);
    const auto output = scenario.toc(-0x5c04);
    const auto global_cell = scenario.toc(-0x5bfc);  // the global output stream {bytes, position, length}
    for (const auto table : {game + 0x2c68, game + 0x12d6b0}) {
        const auto top = table + high;
        const auto name = [&](const std::string& what) { return what + (table == game + 0x2c68 ? " first" : " second"); };
        // Drawing and file routines the snapshot can't run (a window, a file spec) are answered.
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> answered = {{0x10718fc0, 0}, {0x1071c2d0, 0}, {0x10723850, 0}, {0x10723650, 0}, {0x1073ede0, 0},
                                                                                    {0x10769b60, 0}, {0x1076f7d0, 0}};
        const auto quiet = [&](std::uint32_t reply) -> Setup {
            return {.memory = {{1, table + display::table::screen_set, 0}, {1, table + display::table::reply_wanted, reply}}, .answers = answered};
        };
        // Command 4: -2. Command 3: with and without the screen already set.
        {
            Bytes b;
            b.u8(4);
            add(cases, name("command 4"), table, b, 1);
            Bytes three;
            three.u8(3);
            add(cases, name("command 3"), table, three, 1, quiet(0));
            add(cases, name("command 3, screen set"), table, three, 1, {.memory = {{1, table + display::table::screen_set, 1}}, .answers = answered});
            add(cases, name("command 3, flag already set"), table, three, 1, {.memory = {{1, table + display::table::screen_set, 0}, {1, table + display::table::unsent, 1}}, .answers = answered});
            // Command 5: the byte, and a stream too short for it.
            for (const std::uint32_t value : {0u, 1u, 7u, 0x80u}) {
                Bytes five;
                five.u8(5).u8(value);
                add(cases, name("command 5, value " + std::to_string(value)), table, five, 2, {.answers = answered});
            }
            Bytes five;
            five.u8(5).u8(1);
            add(cases, name("command 5, short"), table, five, 1, {.answers = answered});
        }
        // Command 0: header. Whole streams answering or appending to the output stream, and every cut.
        for (const std::uint32_t reply : {0u, 1u}) {
            Bytes b;
            b.u8(0).u16(5).text("Test").u8(2).u16(3).u16(5).u8(1).u16(6);
            Setup setup = quiet(reply);
            add_cuts(cases, name(reply ? "command 0 reply" : "command 0 append"), table, b, setup);
        }
        {
            Bytes b;
            b.u8(0).u16(0).u8(1).u16(0).u16(0).u8(0).u16(0);
            add(cases, name("command 0, empty string"), table, b, std::uint32_t(b.data.size()), quiet(0));
            // The output stream with no room: the first append fails and falls through, then the second fails.
            for (const std::uint32_t room : {0u, 1u, 2u, 3u}) {
                Setup setup = quiet(0);
                setup.memory.push_back({4, output + 8, room});
                add(cases, name("command 0, output room " + std::to_string(room)), table, b, std::uint32_t(b.data.size()), setup);
            }
        }
        // Command 1: GUI objects.
        {
            // The second table's areas are empty in the snapshot: its objects name no area (-1, which makes
            // the original file the index instead), so 0x107144c0 isn't asked about an area that isn't there.
            const bool second = table != game + 0x2c68;
            const auto g = [&](Bytes& b, std::uint32_t flags, std::uint32_t area = 0, const std::string& text = "",
                               std::uint32_t first_word = 0) -> Bytes& {
                if (second && !(flags & 0x100000)) { flags |= 0x40000; area = 0xffff; }
                return guio(b, flags, area, text, first_word);
            };
            const auto count_of = [&](std::uint32_t n, std::uint32_t reply, std::vector<Write> more = {}) {
                Setup setup = quiet(reply);
                setup.memory.push_back({2, table + display::table::announced_guio_count, n});
                // The pointer the final copy and the 0x100000 block use is null in a snapshot: point it at scratch.
                setup.memory.push_back({4, global_cell, scratch_base + 0x20000});
                for (const auto& w : more) setup.memory.push_back(w);
                return setup;
            };
            const auto whole = [&](const std::string& label, const Bytes& b, const Setup& setup) {
                add(cases, name(label), table, b, std::uint32_t(b.data.size()), setup);
            };
            for (const std::uint32_t reply : {0u, 1u}) {
                Bytes none; none.u8(1); g(none, 0); g(none, 0);
                whole(reply ? "command 1, two empty, reply" : "command 1, two empty", none, count_of(2, reply));
            }
            Bytes none_one; none_one.u8(1); g(none_one, 0);
            Bytes zero; zero.u8(1);
            whole("command 1, no objects", zero, count_of(0, 0));
            whole("command 1, screen set", none_one, {.memory = {{1, table + display::table::screen_set, 1}, {2, table + display::table::announced_guio_count, 1}}, .answers = answered});
            // Each bit alone and the combinations the original treats specially.
            for (const std::uint32_t flags : {0x1u, 0x2u, 0x4u, 0x8u, 0x10u, 0x20u, 0x40u, 0x80u, 0x100u, 0x200u, 0x400u, 0x800u, 0x1000u,
                                              0x2000u, 0x8000u, 0x10000u, 0x20000u, 0x40000u, 0x80000u, 0x200u | 0x400u | 0x2000u,
                                              0x200u | 0x80000u, 0x7ffffu & ~0x4000u & ~0x80000u}) {
                Bytes b; b.u8(1); g(b, flags);
                whole("command 1, flags " + std::to_string(flags), b, count_of(1, 0));
            }
            {  // area index -1: the index list; with the list full: the assertion
                Bytes b; b.u8(1); g(b, 0x40000, 0xffff);
                whole("command 1, no area", b, count_of(1, 0));
                whole("command 1, no area, list full", b, count_of(1, 0, {{2, table + display::table::guio_limit, 0x4b0}}));
                whole("command 1, no area, list one short", b, count_of(1, 0, {{2, table + display::table::guio_limit, 0x4af}}));
                // the table full: the overflow flag
                whole("command 1, table full", b, count_of(1, 0, {{2, top - 0x625a, 0x4af}}));
                whole("command 1, table nearly full", b, count_of(1, 0, {{2, top - 0x625a, 0x4ae}}));
            }
            {  // the string, with and without the 0x400 bit of the first word (which calls 0x10723650)
                Bytes b; b.u8(1); g(b, 0x4000 | 0x1, 0, "Test", 0);
                whole("command 1, string", b, count_of(1, 0));
                Bytes d; d.u8(1); g(d, 0x4000 | 0x1, 0, "", 0x400);
                whole("command 1, string, path", d, count_of(1, 0));
                Bytes e; e.u8(1); g(e, 0x4000, 0, "ab");
                whole("command 1, string without word", e, count_of(1, 0));
            }
            if (!second) {  // the 0x100000 block: the fields come from 0x10725dc0, which reads a GUI record from the
                // global buffer: here the table's own first records
                const std::vector<Write> in_table = {{4, global_cell, table + 0xba864}};
                Bytes b; b.u8(1); g(b, 0x100000);
                whole("command 1, block", b, count_of(1, 0, in_table));
                Bytes c1; c1.u8(1); g(c1, 0x100000 | 0x8 | 0x40000, 0);
                whole("command 1, block with fields", c1, count_of(1, 0, in_table));
                Bytes two; two.u8(1); g(two, 0x100000); g(two, 0x100000 | 0x1);
                whole("command 1, two blocks", two, count_of(2, 0, in_table));
            }
            if (!second) {  // every cut of a full object, then of a string and a record copy (the second table's areas are empty: a cut that drops the area field would ask about area 0)
                Bytes b; b.u8(1); g(b, 0x7ffffu & ~0x80000u & ~0x4000u);
                add_cuts(cases, name("command 1, full"), table, b, count_of(1, 0));
                Bytes t; t.u8(1); g(t, 0x4000 | 0x8000, 0, "ab");
                add_cuts(cases, name("command 1, string full"), table, t, count_of(1, 0));
                Bytes r; r.u8(1); g(r, 0x80000 | 0x1);
                add_cuts(cases, name("command 1, record"), table, r, count_of(1, 0));
                for (const std::uint32_t room : {0u, 1u}) whole("command 1, output room " + std::to_string(room), r, count_of(1, 0, {{4, output + 8, room}}));
            }
        }
        // Command 2: areas.
        for (const std::uint32_t reply : {0u, 1u}) {
            for (const std::uint32_t count : {0u, 1u, 2u}) {
                Bytes b;
                b.u8(2);
                for (std::uint32_t i = 0; i < count; ++i) {
                    b.u16(10 + i).u16(20).u16(300).u16(200).u8(3).u8(1).u16(4).text("abc").u8(4).u8(1).u16(3).text("de")
                        .u32(0x01020304).u16(0x0506).u16(7);
                }
                Setup setup = quiet(reply);
                setup.memory.push_back({2, table + display::table::announced_area_count, count});
                add(cases, name("command 2, areas " + std::to_string(count) + (reply ? " reply" : " append")), table, b,
                    std::uint32_t(b.data.size()), setup);
            }
        }
        {  // fills instead of strings; every cut of one area
            Bytes b;
            b.u8(2).u16(10).u16(20).u16(300).u16(200).u8(3).u8(0).u8(4).u8(0).u32(0x01020304).u16(0x0506).u16(7);
            Setup setup = quiet(0);
            setup.memory.push_back({2, table + display::table::announced_area_count, 1});
            add_cuts(cases, name("command 2, fills"), table, b, setup, 1);
            Bytes strings;
            strings.u8(2).u16(10).u16(20).u16(300).u16(200).u8(3).u8(1).u16(4).text("abc").u8(4).u8(1).u16(3).text("de")
                .u32(0x01020304).u16(0x0506).u16(7);
            add_cuts(cases, name("command 2, strings"), table, strings, setup, 1);
            add(cases, name("command 2, screen set"), table, strings, std::uint32_t(strings.data.size()),
                {.memory = {{1, table + display::table::screen_set, 1}, {2, table + display::table::announced_area_count, 1}}, .answers = answered});
            for (const std::uint32_t room : {0u, 1u}) {
                Setup full = setup;
                full.memory.push_back({4, output + 8, room});
                add(cases, name("command 2, output room " + std::to_string(room)), table, strings,
                    std::uint32_t(strings.data.size()), full);
            }
        }
    }
}
}
