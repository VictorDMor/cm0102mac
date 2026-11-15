// DISPLAY.CPP checker cases for the network block, sending side (display_network_send.cpp). No capture
// reaches a network game, so the real display tables of the snapshot (empty) get the areas and GUI
// objects each case builds, the stream gets a buffer in the scratch area whose length decides which
// checked write reports, and the network send (0x1073a4c0) is answered on both sides.
#include "display_checks.hpp"
#include "display_detail.hpp"
#include <optional>
#include <string>

namespace cm::aot::display_checks {
namespace {
using u32 = std::uint32_t;
constexpr u32 send_part = 0x1071c2d0, send_screen = 0x1071c1d0, localize = 0x10723650, reply = 0x10723850;
constexpr u32 send_entry = 0x1073a4c0;
constexpr u32 high = 0x130000;
constexpr u32 field(u32 table, std::int32_t displacement) { return table + high + u32(displacement); }
// Scratch layout.
constexpr u32 peer = scratch_base + 0x100, text_area = scratch_base + 0x1000, buffer = scratch_base + 0x2000,
              second_stream = scratch_base + 0x30000, copy_target = scratch_base + 0x40000;

std::string hex(u32 value) { char text[16]; std::snprintf(text, sizeof text, "%x", value); return text; }

// One GUI object as the getter 0x10725dc0 reads it (record offsets).
struct Object {
    u32 a = 0, d = 0, e = 0, f = 0, g = 0, h = 0, i = 0, b = 0, j = 0, k = 0, l = 0, m = 0, c = 0;
    u32 h76 = 0, h74 = 0, h72 = 0, s7a = 0, s78 = 0xffff;
    std::string text;
};
void put_object(std::vector<Write>& memory, u32 table, u32 record, const Object& o) {
    const std::vector<std::pair<u32, u32>> words = {{0, table + 4}, {0x8, o.l}, {0xc, o.a}, {0x10, o.d}, {0x14, o.e}, {0x18, o.f},
                                                    {0x1c, o.g}, {0x20, o.h}, {0x24, o.i}, {0x38, o.b}, {0x3c, o.j}, {0x40, o.k},
                                                    {0x48, o.m}, {0x54, o.c}};
    for (const auto& [at, value] : words) memory.push_back({4, record + at, value});
    for (const auto& [at, value] : std::vector<std::pair<u32, u32>>{{0x6a, o.h76}, {0x6c, o.h74}, {0x6e, o.s7a}, {0x70, o.h72}, {0x72, o.s78}})
        memory.push_back({2, record + at, value});
    // The text is at +0x78, or +0x7d when the object names an area (the getter's choice).
    const u32 where = record + (o.s78 == 0xffff ? 0x78 : 0x7d);
    for (u32 n = 0; n <= o.text.size(); ++n) memory.push_back({1, where + n, n < o.text.size() ? std::uint8_t(o.text[n]) : 0u});
}
Object rich(u32 seed) {
    Object o;
    o.a = 0x11 + seed, o.d = 0x1002 + seed, o.e = 0x2003, o.f = 0x3004, o.g = 0x4005, o.h = 0x16, o.i = 0x5006, o.b = 0x70000 + seed;
    o.j = 0x80000, o.k = 0x9007, o.l = 0xa0000, o.m = 0xb0000, o.c = 0xc0000 + seed, o.h76 = 7, o.h74 = 8, o.h72 = 9, o.s7a = 10, o.s78 = 0xffff;
    o.text = "OK " + std::to_string(seed);
    return o;
}
std::vector<Write> stream_memory(u32 stream, u32 capacity) {
    return {{4, stream, buffer}, {4, stream + 4, 0}, {4, stream + 8, capacity}};
}
}

DISPLAY_CASES("network-1") {
    if (scenario.match) return;
    const auto game = scenario.toc(-0x7bb0);
    const auto stream = scenario.toc(-0x5c04), stream2 = scenario.toc(-0x5c08), shared_variable = scenario.toc(-0x5bfc);
    const u32 tables[] = {game + 0x2c68, game + 0x12d6b0};
    const auto row = std::int16_t(scenario.s.memory.read<2>(game + 0x2c58));
    const u32 copy_row = copy_target;  // the copy of the human's last send
    const u32 shared = copy_row - u32(row) * 0x6f544;
    const auto label = [](const std::string& what, unsigned t) { return what + (t ? ", second table" : ", first table"); };
    const std::vector<std::pair<u32, u32>> sent = {{send_entry, 1}}, refused = {{send_entry, 0}};

    for (unsigned t = 0; t < 2; ++t) {
        const u32 table = tables[t];
        const u32 guio_records = table + 0xba864;
        // ---- 0x1071c2d0 mode 0: the header; the capacity decides which write reports.
        for (u32 capacity = 0; capacity <= 72; capacity += (t ? 5 : 1)) {
            auto memory = stream_memory(stream, capacity);
            memory.insert(memory.end(), {{2, field(table, -0x625c), 3}, {2, field(table, -0x625a), 9}, {1, table + display::table::width_flag, 1}, {2, table + display::table::height_flag, 0x1234}});
            cases.push_back({.entry = send_part, .registers = {{3, table}, {4, 0x1234}, {5, 0x2345}, {6, 0}}, .memory = memory, .answers = sent,
                             .label = label("header, capacity " + std::to_string(capacity), t)});
        }
        cases.push_back({.entry = send_part, .registers = {{3, table}, {4, 1}, {5, 2}, {6, 0}}, .memory = stream_memory(stream, 0x100), .answers = refused,
                         .label = label("header, send fails", t)});
        // ---- other values of the part byte: the stream, a part nothing handles.
        for (const u32 part : {4u, 0x80u, 0xffu, 0x102u, 0x7fu})
            cases.push_back({.entry = send_part, .registers = {{3, table}, {4, 1}, {5, 2}, {6, part}}, .memory = stream_memory(stream, 0x100), .answers = sent,
                             .label = label("part " + hex(part), t)});
        cases.push_back({.entry = send_part, .registers = {{3, table}, {4, 1}, {5, 2}, {6, 4}}, .memory = stream_memory(stream, 0), .answers = sent,
                         .label = label("part 4, no room", t)});
        // ---- mode 2: the command stream goes out as it is.
        for (const auto& answers : {sent, refused}) {
            auto memory = stream_memory(stream, 0x100);
            memory.insert(memory.end(), {{4, stream2, second_stream}, {4, stream2 + 4, 0x37}, {4, stream2 + 8, 0x100}});
            cases.push_back({.entry = send_part, .registers = {{3, table}, {4, 0x77}, {5, 0x88}, {6, 2}}, .memory = memory, .answers = answers,
                             .label = label(answers[0].second ? "command stream" : "command stream, send fails", t)});
        }
        cases.push_back({.entry = send_part, .registers = {{3, table}, {4, 0x77}, {5, 0x88}, {6, 2}}, .memory = stream_memory(stream, 0), .answers = sent,
                         .label = label("command stream, no room for the part byte", t)});
        // ---- mode 3: each area with an id, then the closing message.
        for (const u32 areas : {0u, 1u, 3u}) {
            auto memory = stream_memory(stream, 0x100);
            memory.push_back({2, field(table, -0x625c), areas});
            const u32 ids[] = {5, 0xffffffffu, 0x01020304u};
            for (u32 n = 0; n < areas; ++n) memory.push_back({4, table + 4 + n * 0xbf0 + 0x200, ids[n]});
            cases.push_back({.entry = send_part, .registers = {{3, table}, {4, 0x77}, {5, 0x88}, {6, 3}}, .memory = memory, .answers = sent,
                             .label = label("areas, count " + std::to_string(areas), t)});
            if (areas == 3) {
                for (const unsigned nth : {1u, 2u, 3u})
                    cases.push_back({.entry = send_part, .registers = {{3, table}, {4, 0x77}, {5, 0x88}, {6, 3}}, .memory = memory, .answers = sent,
                                     .nth_answers = {{send_entry, nth, 0}}, .label = label("areas, send " + std::to_string(nth) + " fails", t)});
                for (u32 capacity = 0; capacity <= 8; ++capacity) {
                    auto small = memory;
                    small[2] = {4, stream + 8, capacity};
                    cases.push_back({.entry = send_part, .registers = {{3, table}, {4, 0x77}, {5, 0x88}, {6, 3}}, .memory = small, .answers = sent,
                                     .label = label("areas, capacity " + std::to_string(capacity), t)});
                }
            }
        }
        // ---- mode 1: GUI objects.
        struct Gui {
            std::string name;
            std::vector<Object> list;
            u32 capacity = 0x1000;
            bool mirrored = false;
            std::vector<Object> copy = {};
            std::optional<u32> copy_count = std::nullopt;
            std::vector<std::pair<u32, u32>> answers = sent;
            std::vector<Case::NthAnswer> nth = {};
        };
        const auto gui_case = [&](const Gui& g) {
            auto memory = stream_memory(stream, g.capacity);
            memory.push_back({2, field(table, -0x625a), u32(g.list.size())});
            for (u32 n = 0; n < g.list.size(); ++n) put_object(memory, table, guio_records + n * 0x17c, g.list[n]);
            memory.push_back({1, field(table, -0x56c1), g.mirrored});
            if (g.mirrored) {
                memory.push_back({4, shared_variable, shared});
                for (u32 n = 0; n < g.copy.size(); ++n) put_object(memory, table, copy_row + n * 0x17c, g.copy[n]);
                memory.push_back({2, copy_row + 0x6f540, g.copy_count.value_or(u32(g.copy.size()))});
            }
            cases.push_back({.entry = send_part, .registers = {{3, table}, {4, 0x77}, {5, 0x88}, {6, 1}}, .memory = memory, .answers = g.answers,
                             .nth_answers = g.nth, .label = label("objects, " + g.name, t)});
        };
        Object bare;
        Object named = rich(1);
        named.s78 = 0;  // names area 0: the text moves to +0x7d
        Object blank_text = rich(2);
        blank_text.text.clear();
        gui_case({.name = "none", .list = {}});
        gui_case({.name = "one blank", .list = {bare}});
        gui_case({.name = "one rich", .list = {rich(0)}});
        gui_case({.name = "named area", .list = {named}});
        gui_case({.name = "empty text", .list = {blank_text}});
        gui_case({.name = "send fails", .list = {rich(0)}, .answers = refused});
        // An identical object is replaced by its index; one 11 back is out of the window of ten.
        Object twin = rich(0);
        std::vector<Object> twins;
        for (u32 n = 0; n < 13; ++n) twins.push_back(n == 12 || n == 6 ? twin : rich(100 + n));
        gui_case({.name = "duplicates", .list = twins});
        std::vector<Object> far_twins;
        for (u32 n = 0; n < 12; ++n) far_twins.push_back(n == 11 || n == 0 ? twin : rich(200 + n));
        gui_case({.name = "twin outside the window", .list = far_twins});
        if (t == 0)
            for (u32 capacity = 0; capacity <= 110; ++capacity)
                gui_case({.name = "capacity " + std::to_string(capacity), .list = {rich(0), rich(1)}, .capacity = capacity});
        // The copy of the last send: identical, changed in a few members, changed in many, a changed text.
        Object changed = rich(0);
        changed.a += 1;
        changed.h76 = 99;
        Object everything = rich(5);
        everything.a = 1, everything.b = 2, everything.c = 3, everything.d = 4, everything.e = 5, everything.f = 6, everything.g = 7, everything.h = 8,
        everything.i = 9, everything.j = 10, everything.k = 11, everything.l = 12, everything.m = 13, everything.h76 = 14, everything.h74 = 15,
        everything.h72 = 16, everything.s7a = 17, everything.s78 = 0xffff;
        everything.text = "something else entirely";
        Object other_text = rich(0);
        other_text.text = "OK 9";
        Object longer = rich(0);
        longer.text = "OK 0, with more";
        gui_case({.name = "mirrored, same", .list = {rich(0), rich(1)}, .mirrored = true, .copy = {rich(0), rich(1)}});
        gui_case({.name = "mirrored, a few members differ", .list = {changed}, .mirrored = true, .copy = {rich(0)}});
        gui_case({.name = "mirrored, everything differs", .list = {rich(0)}, .mirrored = true, .copy = {everything}});
        gui_case({.name = "mirrored, text differs", .list = {rich(0)}, .mirrored = true, .copy = {other_text}});
        gui_case({.name = "mirrored, text longer", .list = {rich(0)}, .mirrored = true, .copy = {longer}});
        gui_case({.name = "mirrored, copy without text", .list = {rich(0)}, .mirrored = true, .copy = {blank_text}});
        gui_case({.name = "mirrored, copy shorter", .list = {rich(0), rich(1), rich(2)}, .mirrored = true, .copy = {rich(0), rich(1), rich(2)}, .copy_count = 1u});
        gui_case({.name = "mirrored, copy empty", .list = {rich(0)}, .mirrored = true, .copy = {}, .copy_count = 0u});
        gui_case({.name = "mirrored, new object blank", .list = {bare}, .mirrored = true, .copy = {rich(0)}});
        gui_case({.name = "mirrored, both blank", .list = {bare}, .mirrored = true, .copy = {bare}});
        gui_case({.name = "mirrored, send fails", .list = {rich(0)}, .mirrored = true, .copy = {rich(0)}, .answers = refused});
        if (t == 0)
            for (u32 capacity = 0; capacity <= 40; ++capacity)
                gui_case({.name = "mirrored, capacity " + std::to_string(capacity), .list = {changed}, .mirrored = true, .copy = {rich(0)}, .capacity = capacity});
        {  // No copy buffer at all (the shared pointer is null).
            auto memory = stream_memory(stream, 0x1000);
            memory.push_back({2, field(table, -0x625a), 1});
            put_object(memory, table, guio_records, rich(0));
            memory.insert(memory.end(), {{1, field(table, -0x56c1), 1}, {4, shared_variable, 0}});
            cases.push_back({.entry = send_part, .registers = {{3, table}, {4, 0x77}, {5, 0x88}, {6, 1}}, .memory = memory, .answers = sent,
                             .label = label("objects, mirrored without a copy buffer", t)});
        }

        // ---- 0x1071c1d0: the whole screen.
        for (const u32 wanted : {0u, 1u}) {
            auto memory = stream_memory(stream, 0x400);
            memory.insert(memory.end(), {{1, peer + 0xc35b8, 1}, {1, table + display::table::reply_wanted, wanted}, {2, field(table, -0x625c), 2}, {2, field(table, -0x625a), 2},
                                         {4, stream2, second_stream}, {4, stream2 + 4, 5}, {4, stream2 + 8, 0x100}});
            for (u32 n = 0; n < 2; ++n) {
                put_object(memory, table, guio_records + n * 0x17c, rich(n));
                memory.push_back({4, table + 4 + n * 0xbf0 + 0x200, 10 + n});
            }
            cases.push_back({.entry = send_screen, .registers = {{3, table}, {4, peer}, {5, 3}}, .memory = memory, .answers = sent,
                             .label = label(wanted ? "screen, reply wanted" : "screen", t)});
            if (!wanted)
                for (const unsigned nth : {1u, 2u, 3u, 4u})
                    cases.push_back({.entry = send_screen, .registers = {{3, table}, {4, peer}, {5, 3}}, .memory = memory, .answers = sent,
                                     .nth_answers = {{send_entry, nth, 0}}, .label = label("screen, send " + std::to_string(nth) + " fails", t)});
        }
        cases.push_back({.entry = send_screen, .registers = {{3, table}, {4, peer}, {5, 3}}, .memory = {{1, peer + 0xc35b8, 0}}, .answers = sent,
                         .label = label("screen, peer not set up", t)});
        cases.push_back({.entry = send_screen, .registers = {{3, table}, {4, 0}, {5, 3}}, .answers = sent, .label = label("screen, no peer", t)});

        // ---- 0x10723850: the reply {5, byte}.
        for (u32 capacity = 0; capacity <= 3; ++capacity)
            for (const auto& answers : {sent, refused})
                cases.push_back({.entry = reply, .registers = {{4, 0x11}, {5, 0x22}, {6, 0x1ab}}, .memory = stream_memory(stream, capacity), .answers = answers,
                                 .label = label("reply, capacity " + std::to_string(capacity) + (answers[0].second ? "" : ", send fails"), t)});
    }

    // ---- 0x10723650: a path rewritten to the game's own folders.
    const auto path_case = [&](const std::string& path, u32 pictures_first, const std::string& name, std::vector<std::pair<u32, u32>> answers = {{0x10739d60, 0}}) {
        std::vector<Write> memory;
        for (u32 n = 0; n < 4; ++n) memory.push_back({1, text_area + 0x200 + n, std::uint32_t(std::uint8_t("x:y"[n]))});  // what an answered 0x10746280 returns
        for (u32 n = 0; n <= path.size(); ++n) memory.push_back({1, text_area + n, n < path.size() ? std::uint8_t(path[n]) : 0u});
        // 0x10739d60 (a file check) and 0x10739640 (a path normalizer) reach the file system: answered.
        answers.insert(answers.end(), {{0x10739640, 0}});
        cases.push_back({.entry = localize, .registers = {{3, text_area}, {4, pictures_first}}, .memory = memory, .answers = std::move(answers),
                         .label = "path " + name});
    };
    for (const u32 first : {0u, 1u, 0x100u}) {
        const std::string suffix = ", flag " + hex(first);
        path_case("C:\\Games\\pictures\\faces\\a.bmp", first, "pictures" + suffix);
        path_case("C:\\Games\\sounds\\crowd\\b.wav", first, "sounds" + suffix, {{0x10739d60, 0}, {0x10746280, text_area + 0x200}});
        path_case("C:\\Games\\data\\c.dat", first, "data" + suffix);
        path_case("C:\\Games\\other\\d.dat", first, "no folder" + suffix);
        path_case("pictures\\sounds\\data\\e", first, "all three" + suffix, {{0x10739d60, 0}, {0x10746280, text_area + 0x200}});
        path_case("x\\pictures\\y\\pictures\\z", first, "pictures twice" + suffix);
        path_case("", first, "empty" + suffix);
        path_case("C:\\Games\\pictures\\a.bmp", first, "already a file" + suffix, {{0x10739d60, 1}});
    }
}
}
