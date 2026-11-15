// DISPLAY.CPP checker cases for the network screen transport (display_network_object.cpp): 0x10723a30
// (the reply, or the plain send) and 0x10723cb0 (a GUI object's screen command). No capture reaches them,
// so the cases build the global output stream in the scratch area and give the table's GUI objects fields.
// NETWORK 0x1073a4c0 (the socket send) is answered on both sides; everything else runs.
#include "display_checks.hpp"
#include "display_detail.hpp"
#include <cstdio>
#include <string>

namespace cm::aot::display_checks {
namespace {
using u32 = std::uint32_t;
constexpr u32 ready = 0x10723a30, gui_object = 0x10723cb0, network_send = 0x1073a4c0;
std::string hex(u32 value) { char text[16]; std::snprintf(text, sizeof text, "%x", value); return text; }

// The global output stream (TOC -0x5c04: bytes pointer, position, length) pointing at the scratch area.
std::vector<Write> stream(u32 out, u32 length) {
    return {{4, out, scratch_base + 0x4000}, {4, out + 4, 0x7777}, {4, out + 8, length}};
}
// A GUI object (0x17c bytes at table + 0xba864 + index * 0x17c) with distinctive fields, bits set in the
// high halves (byte order and sign extension), the area index -1 (no area lookup) and the given link.
std::vector<Write> object(u32 table, int index, u32 seed, u32 link, const std::string& text, bool area = false) {
    const u32 base = table + 0xba864 + u32(index) * 0x17c;
    std::vector<Write> w;
    const auto word = [&](u32 offset, u32 value) { w.push_back({4, base + offset, value}); };
    const auto half = [&](u32 offset, u32 value) { w.push_back({2, base + offset, value}); };
    const u32 s = seed * 0x01010101u;
    word(0x0, table + 4);  // the areas array 10725dc0 indexes when the object has an area
    word(0x8, 0x80000001u ^ s);
    for (u32 offset : {0xcu, 0x10u, 0x14u, 0x18u, 0x1cu, 0x20u, 0x24u}) word(offset, 0x80008000u + offset * 0x101 + s);
    word(0x38, 0xfedc0000u ^ s); word(0x3c, 0x12348765u ^ s); word(0x40, 0x9abc1357u ^ s); word(0x48, 0x2468ace1u ^ s);
    word(0x54, 0x5555aaaau ^ s); word(0x58, 0xdeadbeefu ^ s);
    half(0x6a, 0x8123u ^ seed); half(0x6c, 0x7fedu ^ seed); half(0x6e, 0x9001u ^ seed); half(0x70, 0xc0deu ^ seed);
    half(0x72, area ? 0u : 0xffffu);
    if (area) w.push_back({2, table + 4 + 0x20e, 0});  // the area's field: not -1, so the string starts at +0x7d
    half(0x76, link);
    for (unsigned i = 0; i < 8; ++i) w.push_back({1, base + 0x60 + i, 0x81u + i * 7 + seed});  // the position copied for a link
    for (std::size_t i = 0; i <= text.size(); ++i) w.push_back({1, base + (area ? 0x7du : 0x78u) + u32(i), i < text.size() ? u32(text[i]) : 0u});
    return w;
}
}

DISPLAY_CASES("network-2") {
    if (scenario.match) return;
    const auto game = scenario.toc(-0x7bb0), out = scenario.toc(-0x5c04), count_cell = scenario.toc(-0x5c24);
    const u32 tables[] = {game + 0x2c68, game + 0x12d6b0};
    const auto concat = [](std::vector<Write> a, const std::vector<Write>& b) { a.insert(a.end(), b.begin(), b.end()); return a; };
    const auto name = [](const std::string& what, unsigned t) { return what + (t ? ", second table" : ", first table"); };
    const std::vector<std::pair<u32, u32>> sent = {{network_send, 1}}, failed = {{network_send, 0}}, odd = {{network_send, 0x100}}, odd_set = {{network_send, 0x180}};

    for (unsigned t = 0; t < 2; ++t) {
        const auto table = tables[t];
        // 0x10723a30: the announcement (reply flag clear) and the plain send (set), each with the send
        // answered both ways; and the stream cut at every size the two appended bytes can fail on.
        for (const u32 flag : {0u, 1u, 0xffu})
            for (const auto& answer : {sent, failed, odd, odd_set})
                for (const u32 length : {0u, 1u, 2u, 3u, 0x1000u})
                    cases.push_back({.entry = ready, .registers = {{3, table}, {4, 0x1234abcd}},
                                     .memory = concat(stream(out, length), {{1, table + display::table::reply_wanted, flag}, {2, count_cell, 0}}),
                                     .label = name("ready, flag " + hex(flag) + ", send " + hex(answer[0].second) + ", length " + std::to_string(length), t),
                                     .answers = answer});
        cases.push_back({.entry = ready, .registers = {{3, table}, {4, 0x1}}, .memory = concat(stream(out, 0x10), {{1, table + display::table::reply_wanted, 0}}),
                         .label = name("ready, preset screen_set", t), .answers = sent});
        // 0x10723cb0: link -1 (no second record), the link to the object itself, and a link to another one.
        const auto run = [&](const std::string& what, unsigned index, u32 index_word, const std::vector<Write>& records, u32 length,
                             const std::vector<std::pair<u32, u32>>& answers) {
            cases.push_back({.entry = gui_object, .registers = {{3, table}, {4, 0xfeed0001u}, {5, index_word}},
                             .memory = concat(stream(out, length), records),
                             .label = name(what + ", length " + std::to_string(length), t), .answers = answers});
            (void)index;
        };
        run("object, no link", 2, 2, object(table, 2, 1, 0xffff, "net"), 0x1000, sent);
        run("object, no link, send fails", 2, 2, object(table, 2, 2, 0xffff, "net"), 0x1000, failed);
        run("object, empty string", 2, 2, object(table, 2, 3, 0xffff, ""), 0x1000, sent);
        run("object, high bits in the index", 3, 0x12340003u, object(table, 3, 4, 0xffff, "high"), 0x1000, sent);
        // Index -1 is the 0x17c bytes before the first GUI object (inside the areas' tail).
        run("object, index -1", 0xffff, 0xffff, object(table, -1, 5, 0xffff, "x"), 0x1000, sent);
        run("object, area string", 1, 1, object(table, 1, 6, 0xffff, "area text", true), 0x1000, sent);
        run("object, linked to itself", 2, 2, object(table, 2, 7, 2, "self"), 0x1000, sent);
        run("object, linked to itself, send fails", 2, 2, object(table, 2, 8, 2, "self"), 0x1000, failed);
        run("object, linked", 2, 2, concat(object(table, 2, 9, 5, "first"), object(table, 5, 10, 0xffff, "second")), 0x1000, sent);
        run("object, linked, send fails", 2, 2, concat(object(table, 2, 11, 5, "first"), object(table, 5, 12, 0xffff, "second")), 0x1000, failed);
        run("object, linked, answer with high bits", 2, 2, concat(object(table, 2, 13, 5, "first"), object(table, 5, 14, 0xffff, "second")), 0x1000, odd);
        run("object, linked, answer with a set low byte", 2, 2, concat(object(table, 2, 13, 5, "first"), object(table, 5, 14, 0xffff, "second")), 0x1000, odd_set);
        run("object, linked to index -2", 2, 2, concat(object(table, 2, 15, 0xfffe, "first"), object(table, -2, 23, 0xffff, "before")), 0x1000, sent);
        run("object, linked, long strings", 2, 2,
            concat(object(table, 2, 16, 4, std::string(60, 'a')), object(table, 4, 17, 0xffff, std::string(70, 'b'))), 0x1000, sent);
        // The stream cut at every length: each site's check fails in turn (the later, smaller appends still
        // fit and are written); the whole linked command is 1+2+... bytes, the plain one fewer.
        if (t == 0) {
            const auto linked = concat(object(table, 2, 18, 5, "ab"), object(table, 5, 19, 0xffff, "cd"));
            for (u32 length = 0; length < 150; ++length) run("object, linked, cut", 2, 2, linked, length, sent);
            const auto single = object(table, 2, 20, 0xffff, "ab");
            for (u32 length = 0; length < 70; ++length) run("object, plain, cut", 2, 2, single, length, failed);
        } else {
            for (const u32 length : {0u, 5u, 40u, 77u, 90u})
                run("object, linked, cut", 2, 2, concat(object(table, 2, 21, 5, "ab"), object(table, 5, 22, 0xffff, "cd")), length, sent);
        }
    }
}
}
