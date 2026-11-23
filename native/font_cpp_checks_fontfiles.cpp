// FONT.CPP, block `fontfiles` (0x1074d380, 0x1074d450, 0x1074f170, 0x1074f2c0, 0x10750d30, 0x10750f00): cases. The Toolbox glue (the Resource
// Manager, the file system, the Memory Manager) has no host behind it in a snapshot, so each case scripts its answers (Case::answers /
// nth_answers: r3 only, given on both sides where the dispatcher sees the call); what a glue call writes through a pointer (the resource
// file object, the font data record, the file spec) is written by the case's pokes, or by the routines that run for real around it.
// Data (paths, a TrueType name table image, rows) is built in the scratch area. Labels start with "fontfiles:".
#include "font_cpp_checks.hpp"
#include "font_cpp_files_detail.hpp"
#include <cstdlib>
#include <cstring>
#include <random>

namespace cm::aot::font_cpp_checks {
namespace {
using namespace cm::aot::font_cpp;
[[maybe_unused]] constexpr std::uint32_t open_entry = 0x1074d380, close_entry = 0x1074d450, find_entry = 0x1074f170, file_entry = 0x1074f2c0,
                        builder_entry = 0x10750d30, name_entry = 0x10750f00;
[[maybe_unused]] constexpr std::uint32_t object_at = scratch_base + 0x10000, spec_at = scratch_base + 0x10100, path_at = scratch_base + 0x11000,
                        font_at = scratch_base + 0x12000, buffer_at = scratch_base + 0x13000, data_at = scratch_base + 0x20000,
                        handle_at = scratch_base + 0x14000, file_at = scratch_base + 0x14200;
constexpr std::uint32_t any_file = 0x61234567;

std::vector<Write> fill(std::uint32_t at, std::uint32_t size, std::uint8_t byte) {
    std::vector<Write> memory;
    for (std::uint32_t n = 0; n < size; ++n) memory.push_back(w8(at + n, byte));
    return memory;
}
std::vector<Write> text(std::uint32_t at, const std::string& value) {
    std::vector<Write> memory;
    for (std::size_t n = 0; n < value.size(); ++n) memory.push_back(w8(at + std::uint32_t(n), std::uint8_t(value[n])));
    memory.push_back(w8(at + std::uint32_t(value.size()), 0));
    return memory;
}
using Answers = std::vector<std::pair<std::uint32_t, std::uint32_t>>;
}


// A TrueType-like image at data_at: the sfnt header, a directory of four tables ('name' at index `name_index`, or absent when negative) and a
// name table at 0x100 whose records are {name id, string}. Tables other than 'name' point at 0x200.
struct NameRecordSpec { std::uint16_t id; std::string text; std::uint16_t length_override; bool override_length; };
std::vector<Write> font_image(int name_index, const std::vector<NameRecordSpec>& records, unsigned tables = 4, int second_name_index = -1,
                              const std::vector<NameRecordSpec>& second = {}) {
    std::vector<std::uint8_t> bytes(0x400, 0);
    const auto put16 = [&](std::size_t at, std::uint32_t value) { bytes[at] = std::uint8_t(value >> 8); bytes[at + 1] = std::uint8_t(value); };
    const auto put32 = [&](std::size_t at, std::uint32_t value) { put16(at, value >> 16); put16(at + 2, value & 0xffff); };
    put32(0, 0x00010000);
    put16(4, tables);
    const char* tags[] = {"cmap", "head", "post", "OS/2"};
    const auto table_at = [&](std::size_t table_start, const std::vector<NameRecordSpec>& list) {
        put16(table_start, 0);
        put16(table_start + 2, std::uint32_t(list.size()));
        const std::size_t strings = 6 + 12 * list.size();
        put16(table_start + 4, std::uint32_t(strings));
        std::size_t at = table_start + strings;
        for (std::size_t k = 0; k < list.size(); ++k) {
            put16(table_start + 6 + 12 * k, 1);
            put16(table_start + 6 + 12 * k + 2, 0);
            put16(table_start + 6 + 12 * k + 4, 0);
            put16(table_start + 6 + 12 * k + 6, list[k].id);
            put16(table_start + 6 + 12 * k + 8, list[k].override_length ? list[k].length_override : std::uint32_t(list[k].text.size()));
            put16(table_start + 6 + 12 * k + 10, std::uint32_t(at - table_start - strings));
            for (char ch : list[k].text) bytes[at++] = std::uint8_t(ch);
        }
    };
    for (unsigned k = 0; k < 4; ++k) {
        const auto entry = 12 + 16 * k;
        const bool is_name = int(k) == name_index || int(k) == second_name_index;
        const char* tag = is_name ? "name" : tags[k];
        for (unsigned n = 0; n < 4; ++n) bytes[entry + n] = std::uint8_t(tag[n]);
        put32(entry + 8, is_name ? (int(k) == name_index ? 0x100 : 0x300) : 0x200);
        put32(entry + 12, 0x100);
    }
    if (name_index >= 0) table_at(0x100, records);
    if (second_name_index >= 0) table_at(0x300, second);
    std::vector<Write> memory;
    for (std::size_t n = 0; n < bytes.size(); ++n) memory.push_back(w8(data_at + std::uint32_t(n), bytes[n]));
    return memory;
}

FONT_CPP_CASES("fontfiles") {
    std::mt19937 random(0xf11e);
    const auto first_case = cases.size();

    // ---- 0x1074d380: the resource file object. The glue: CurResFile (the low half is stored), FSpOpenResFile (first, then second after a
    // create), FSpCreateResFile, UseResFile. Rare paths first (the call trace pairs the first cases).
    for (const std::uint32_t first : {0xffffffffu, 0x1ffffu, 5u, 0u})
        for (const std::uint32_t create : {1u, 0xffu, 0x80000001u, 0u, 0x100u})
            for (const std::uint32_t permission : {0u, 1u, 3u, 0x7fu, 0x80u, 0xffu, 0x1ffu, 0x12345680u})
                for (const std::uint32_t second : {7u, 0xffffffffu}) {
                    const bool failed = (first & 0xffff) == 0xffff;
                    if (!failed && second != 7) continue;
                    Case item{.entry = open_entry, .registers = {{3, object_at}, {4, spec_at}, {5, permission}, {6, create}}, .memory = fill(object_at, 8, 0x5a),
                              .label = "fontfiles: open resource file first " + hex(first) + " create " + hex(create) + " permission " + hex(permission) +
                                       " second " + hex(second)};
                    item.imports = {{"CarbonLib::CurResFile", 0, 0x12345678}, {"CarbonLib::FSpOpenResFile", 1, first}, {"CarbonLib::FSpOpenResFile", 0, second},
                                    {"CarbonLib::FSpCreateResFile", 0, 0}, {"CarbonLib::UseResFile", 0, 0}};
                    cases.push_back(item);
                }

    // ---- 0x1074d450: closing it, with its flag.
    for (const auto refs : std::vector<std::pair<std::uint32_t, std::uint32_t>>{{3, 5}, {3, 0xffff}, {0xfffe, 0x7fff}, {0, 0}})
        for (const std::uint32_t flag : {0u, 1u, 0xffffffffu, 0x8000u, 0x10000u, 0x18000u, 0x7fffu, 5u, 0xffff0001u}) {
            Case item{.entry = close_entry, .registers = {{3, object_at}, {4, flag}}, .memory = {w16(object_at, refs.first), w16(object_at + 2, refs.second)},
                      .label = "fontfiles: close resource file " + hex(refs.first) + "/" + hex(refs.second) + " flag " + hex(flag)};
            item.imports = {{"CarbonLib::UseResFile", 0, 0}, {"CarbonLib::CloseResFile", 0, 0}};
            item.answers = {{lib::delete_object, 0}};
            cases.push_back(item);
        }
    // With a real heap block as the object (the font record constructor allocates one, zeroed; its first two halfwords are the refnums) the delete is
    // real, so a delete made or not made shows in the heap.
    for (const std::uint32_t flag : {0u, 1u, 0xffffu, 0x8000u, 0x10000u, 0x7fffu, 0x18000u}) {
        const auto built = scenario.toc(toc::list_built);
        Case item{.entry = close_entry, .registers = {{3, object_at}, {4, flag}}, .memory = {w8(built, 1)},
                  .label = "fontfiles: close resource file, real object, flag " + hex(flag)};
        item.imports = {{"CarbonLib::UseResFile", 0, 0}, {"CarbonLib::CloseResFile", 0, 0}};
        item.prepare = [flag](State& s) {
            guest::Cpu c(s);
            c.set(3, object_at + 0x80);
            font_cpp_run_original(s, 0x1074f390);
            const auto block = c.u32(object_at + 0x80);
            c.put16(block, 3);
            c.put16(block + 2, 5);
            c.set({{3, block}, {4, flag}});
        };
        cases.push_back(item);
    }
    for (const std::uint32_t flag : {0u, 1u, 0xffffffffu})
        cases.push_back({.entry = close_entry, .registers = {{3, 0}, {4, flag}}, .label = "fontfiles: close resource file null object flag " + hex(flag),
                         .answers = {{lib::delete_object, 0}}});

    // ---- 0x1074f2c0: opening a font file: the extension tests (.ttf, .t2k), the index, fopen's answer.
    const std::vector<std::string> paths{"Mac:Data:FONTS:a.ttf", "Mac:Data:FONTS:a.TTF", "Mac:Data:FONTS:b.t2k", "Mac:Data:FONTS:b.T2K", "Mac:Data:FONTS:c.txt",
                                         "Mac:Data:FONTS:d", ".ttf", "e.tt", "e.ttfx", "e.t2", "", "Mac:Data:FONTS:f.Ttf", "g.ttf.t2k", "h.t2k.ttf"};
    for (const auto& path : paths)
        for (const std::uint32_t index : {0u, 1u, 0x10000u, 0x10001u, 0xffffu})
            for (const std::uint32_t opened : {any_file, 0u}) {
                auto memory = fill(path_at, 0x80, 0x5a);
                append(memory, text(path_at, path));
                append(memory, fill(font_at, 8, 0x77));
                Case item{.entry = file_entry, .registers = {{3, path_at}, {4, font_at}, {5, index}}, .memory = memory,
                          .label = "fontfiles: open font file '" + path + "' index " + hex(index) + " fopen " + hex(opened)};
                item.answers = {{lib::fopen, opened}};
                cases.push_back(item);
            }

    // ---- 0x10750f00: the name reader. Data through a Handle (GetHandleSize scripted, BlockMoveData real) or a FILE (fseek and fread scripted,
    // the pieces poked into the routine's stack buffers). The frame is the original's (0x90): header at +0x5c, directory entry +0x4c, name table
    // header +0x38, name record +0x40.
    const auto frame_of_name = scenario.sp() - 0x90;
    const NameRecordSpec family{1, "Family", 0, false}, full{4, "Full Name X", 0, false};
    struct Image { std::string label; std::vector<Write> memory; };
    std::vector<Image> images;
    images.push_back({"standard", font_image(2, {family, full})});
    images.push_back({"id 4 first", font_image(0, {full, family})});
    images.push_back({"name last table", font_image(3, {family, full})});
    images.push_back({"no id 4", font_image(2, {family, {2, "Regular", 0, false}})});
    images.push_back({"empty id 4", font_image(2, {family, {4, "", 0, false}, full})});
    images.push_back({"id 4 string starts 0", font_image(2, {{4, std::string("\0abc", 4), 0, false}, full})});
    images.push_back({"id 4 long", font_image(2, {{4, "A very long full font name that does not fit in a short buffer ok", 0, false}})});
    images.push_back({"id 4 length past the string", font_image(2, {{4, "Abc", 0x7000, true}})});
    images.push_back({"no name table", font_image(-1, {})});
    images.push_back({"zero tables", font_image(2, {family, full}, 0)});
    images.push_back({"two name tables", font_image(1, {family}, 4, 3, {full})});
    images.push_back({"zero records", font_image(2, {})});
    images.push_back({"many records", font_image(2, {family, family, family, {4, "", 0, false}, {3, "x", 0, false}, full})});
    const auto font_with_handle = [&](std::uint32_t size) {
        std::vector<Write> memory{w32(font_at, 0), w32(font_at + 4, handle_at), w32(handle_at, data_at)};
        (void)size;
        return memory;
    };
    const auto name_case = [&](const std::string& label, std::vector<Write> memory, std::uint32_t buffer_size, std::uint32_t handle_size) {
        append(memory, fill(buffer_at, 0x120, 0x5a));
        append(memory, font_with_handle(handle_size));
        Case item{.entry = name_entry, .registers = {{3, font_at}, {4, buffer_at}, {5, buffer_size}}, .memory = memory,
                  .label = "fontfiles: name " + label + " size " + hex(buffer_size) + " handle size " + hex(handle_size)};
        item.imports = {{"CarbonLib::GetHandleSize", 0, handle_size}};
        return item;
    };
    // The data in a FILE: one piece of each kind poked (every table directory read gets the same entry, every name record the same record).
    const auto file_case = [&](const std::string& label, std::vector<Write> memory, std::uint32_t buffer_size, std::uint32_t seek_answer,
                               std::uint32_t read_answer, bool name_entry_in_directory) {
        append(memory, fill(buffer_at, 0x120, 0x5a));
        append(memory, std::vector<Write>{w32(font_at, file_at), w32(font_at + 4, 0)});
        Case item{.entry = name_entry, .registers = {{3, font_at}, {4, buffer_at}, {5, buffer_size}}, .memory = memory,
                  .label = "fontfiles: name from file " + label + " size " + hex(buffer_size) + " seek " + hex(seek_answer) + " read " + hex(read_answer)};
        std::vector<Write> pieces;
        const auto bytes = [&](std::uint32_t at, std::initializer_list<std::uint8_t> list) {
            std::uint32_t n = 0;
            for (auto value : list) pieces.push_back(w8(at + n++, value));
        };
        bytes(frame_of_name + 0x5c, {0, 1, 0, 0, 0, 2, 0, 0, 0, 0, 0, 0});  // numTables 2
        if (name_entry_in_directory) bytes(frame_of_name + 0x4c, {'n', 'a', 'm', 'e', 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0});
        else bytes(frame_of_name + 0x4c, {'c', 'm', 'a', 'p', 0, 0, 0, 0, 0, 0, 2, 0, 0, 0, 1, 0});
        bytes(frame_of_name + 0x38, {0, 0, 0, 1, 0, 18});                        // one record, strings at 18
        bytes(frame_of_name + 0x40, {0, 1, 0, 0, 0, 0, 0, 4, 0, 5, 0, 0});     // id 4, length 5, offset 0
        for (unsigned n = 0; n < 5; ++n) pieces.push_back(w8(buffer_at + n, std::uint8_t("Hello"[n])));
        item.pokes = {{lib::fread, pieces}};
        item.answers = {{lib::fseek, seek_answer}, {lib::fread, read_answer}};
        item.original_owners = {lib::fread};
        return item;
    };
    cases.push_back(file_case("name", {}, 0x20, 0, 1, true));
    cases.push_back(file_case("no name table", {}, 0x20, 0, 1, false));
    cases.push_back(file_case("seek fails", {}, 0x20, 1, 1, true));
    cases.push_back(file_case("read fails", {}, 0x20, 0, 0, true));
    cases.push_back(file_case("read of two", {}, 0x20, 0, 2, true));
    cases.push_back(file_case("size 3", {}, 3, 0, 1, true));
    cases.push_back(file_case("size 1", {}, 1, 0, 1, true));
    {
        auto item = file_case("no file", {}, 0x20, 0, 1, true);
        item.memory.push_back(w32(font_at, 0));
        item.label = "fontfiles: name no file nor handle";
        cases.push_back(item);
    }
    for (std::size_t k = 0; k < images.size(); ++k)
        for (const std::uint32_t buffer_size : {0u, 1u, 2u, 5u, 0xffu, 0x100u})
            cases.push_back(name_case(images[k].label, images[k].memory, buffer_size, 0x400));
    for (const std::uint32_t handle_size :
         {0u, 0xbu, 0xcu, 0x1bu, 0x1cu, 0x4bu, 0x4cu, 0x105u, 0x106u, 0x111u, 0x112u, 0x11du, 0x11eu, 0x123u, 0x124u, 0x12eu, 0x12fu, 0x130u, 0xffffffffu})
        for (const std::uint32_t buffer_size : {0x20u, 6u})
            cases.push_back(name_case("standard", images[0].memory, buffer_size, handle_size));
    for (std::size_t k = 1; k < images.size(); ++k)
        for (const std::uint32_t handle_size : {0x10cu, 0x13fu, 0x2fu})
            cases.push_back(name_case(images[k].label, images[k].memory, 0x20, handle_size));

    // ---- 0x1074f170: the data of font number `index` of a file: the Resource Manager's answers scripted (the resource file object is made by
    // 0x1074d380 for real around them), the fallback of the file itself (0x1074f2c0: the extension tests real, fopen scripted).
    const std::string ttf_path = "Mac:Data:FONTS:a.ttf", txt_path = "Mac:Data:FONTS:a.txt";
    const auto find_case = [&](const std::string& label, std::uint32_t made, std::uint32_t refnum, std::uint32_t count, std::uint32_t handle, const std::string& path,
                               std::uint32_t opened, std::uint32_t index) {
        auto memory = fill(font_at, 8, 0x77);
        append(memory, fill(path_at, 0x80, 0x5a));
        append(memory, text(path_at, path));
        Case item{.entry = find_entry, .registers = {{3, path_at}, {4, font_at}, {5, index}}, .memory = memory,
                  .label = "fontfiles: find font data " + label + " made " + hex(made) + " refnum " + hex(refnum) + " count " + hex(count) + " handle " + hex(handle) +
                           " '" + path + "' fopen " + hex(opened) + " index " + hex(index)};
        item.imports = {{"CarbonLib::FSMakeFSSpec", 0, made}, {"CarbonLib::ResolveAliasFile", 0, 0}, {"CarbonLib::CurResFile", 0, 0x11112222},
                        {"CarbonLib::FSpOpenResFile", 0, refnum}, {"CarbonLib::UseResFile", 0, 0}, {"CarbonLib::CloseResFile", 0, 0},
                        {"CarbonLib::Count1Resources", 0, count}, {"CarbonLib::Get1IndResource", 0, handle}, {"CarbonLib::DetachResource", 0, 0}};
        item.answers = {{lib::fopen, opened}};
        return item;
    };
    // The file route (made != 0, no resources, or an unusable resource file) with each extension and fopen answer.
    for (const std::uint32_t made : {0u, 5u, 0x10000u, 0xffffffe4u, 0xffffu}) {
        for (const std::uint32_t refnum : {7u, 0xffffffffu, 0x1ffffu}) {
            if (made != 0 && made != 0x10000 && refnum != 7) continue;
            for (const auto* path : {&ttf_path, &txt_path})
                for (const std::uint32_t opened : {any_file, 0u})
                    for (const std::uint32_t index : {0u, 1u, 0x10000u}) {
                        const bool resources_used = (made & 0xffff) == 0 && (refnum & 0xffff) != 0xffff;
                        // With a usable resource file the count is zero: no sfnt resources, so the file is tried.
                        cases.push_back(find_case(resources_used ? "no resources" : "no resource file", made, refnum, 0, handle_at, *path, opened, index));
                    }
        }
    }
    // The resource route: the count against the index (both as signed halfwords), the handle.
    for (const std::uint32_t count : {1u, 3u, 0x10003u, 0x8000u, 0xffffffffu, 0x7fffu, 0u, 2u, 0x10000u})
        for (const std::uint32_t index : {0u, 1u, 2u, 3u, 4u, 0x10001u, 0xffffu, 0x10000u, 0x7ffeu, 0x7fffu})
            for (const std::uint32_t handle : {handle_at, 0u})
                cases.push_back(find_case("resource route", 0, 7, count, handle, ttf_path, any_file, index));
    for (const std::uint32_t refnum : {0u, 0x10007u})
        cases.push_back(find_case("refnum zero", 0, refnum, 2, handle_at, ttf_path, any_file, 1));

    // ---- 0x10750d30: the list builder. The folder walk is scripted: the first lookup (0x10739690) and each next lookup (0x107397d0), the names
    // (0x107395f0, an answered pointer per call), the font data finder (0x1074f170: for the n-th call `1` for a font and `0` for the end of a
    // file) with the font record it hands back poked into the builder's stack (the Handle of a name table image, or a FILE); the in-page name
    // reader (0x10750f00) reads that image for real. The builder's frame is at the case's stack pointer less 0xa20.
    const auto builder_base = scenario.sp() - 0xa20;
    const auto count_variable = scenario.toc(toc::list_count), table_variable = scenario.toc(toc::list_table);
    const auto name_strings = scratch_base + 0x30000;
    struct Walk {
        std::string label;
        std::uint32_t first = 0;                      // 0x10739690's answer
        std::vector<std::uint32_t> next;              // 0x107397d0's answers by call (the last repeats)
        std::vector<std::uint32_t> fonts;             // 0x1074f170's answers by call (the last repeats)
        std::vector<std::uint32_t> names;             // 0x107395f0's answers by call (the last repeats)
        std::uint32_t file = 0, handle = handle_at;   // the font record's FILE and Handle after each find
        std::uint32_t initial_count = 0, malloc_answer = 1;
        std::vector<Write> data;                      // the image
        std::uint32_t handle_size = 0x400;
        std::vector<Write> more;
    };
    const auto walk_case = [&](const Walk& walk) {
        std::vector<Write> memory = walk.data;
        append(memory, walk.more);
        append(memory, std::vector<Write>{w16(count_variable, walk.initial_count), w32(table_variable, 0)});
        append(memory, fill(buffer_at, 0x120, 0x5a));
        append(memory, std::vector<Write>{w32(handle_at, data_at)});
        Case item{.entry = builder_entry, .memory = memory, .label = "fontfiles: builder " + walk.label};
        item.answers = {{lib::make_path, 0}, {lib::find_first, walk.first}, {lib::find_close, 0}, {lib::fclose, 0}, {lib::fseek, 0}, {lib::fread, 1}};
        if (walk.malloc_answer != 1) item.answers.push_back({lib::allocate, walk.malloc_answer});
        const auto add_sequence = [&](std::uint32_t entry, const std::vector<std::uint32_t>& values) {
            for (std::size_t k = 0; k < values.size(); ++k) item.nth_answers.push_back({entry, unsigned(k + 1), values[k]});
            item.answers.push_back({entry, values.empty() ? 0 : values.back()});
        };
        add_sequence(lib::find_next, walk.next);
        add_sequence(find_entry, walk.fonts);
        add_sequence(lib::copy_name, walk.names);
        item.imports = {{"CarbonLib::GetHandleSize", 0, walk.handle_size}, {"CarbonLib::DisposeHandle", 0, 0}};
        item.pokes = {{find_entry, {w32(builder_base + 0x38, walk.file), w32(builder_base + 0x3c, walk.handle)}}};
        item.original_owners = {lib::fread};
        return item;
    };
    const auto names_of = [&](unsigned n) {
        std::vector<std::uint32_t> list;
        for (unsigned k = 0; k < n; ++k) list.push_back(name_strings + 0x40 * (k + 1));
        return list;
    };
    {
        Walk w{.label = "one file with one font"};
        w.next = {1}; w.fonts = {1, 0}; w.names = names_of(4); w.data = images[0].memory;
        cases.push_back(walk_case(w));
        w.label = "empty folder"; w.first = 1; w.fonts = {}; w.next = {};
        cases.push_back(walk_case(w));
        w.label = "first lookup fails with count preset"; w.initial_count = 7;
        cases.push_back(walk_case(w));
    }
    {
        Walk w{.label = "two files, two fonts each"};
        w.next = {0, 1}; w.fonts = {1, 1, 0, 1, 1, 0}; w.names = names_of(12); w.data = images[0].memory;
        cases.push_back(walk_case(w));
        w.label = "second font's name unreadable"; w.data = images[3].memory;
        cases.push_back(walk_case(w));
        w.label = "malloc fails"; w.data = images[0].memory; w.malloc_answer = 0;
        cases.push_back(walk_case(w));
        w.label = "file name copy is null"; w.malloc_answer = 1; w.names = {0, name_strings + 0x40, name_strings + 0x80, name_strings + 0xc0};
        cases.push_back(walk_case(w));
        w.label = "all file names null"; w.names = {0}; w.next = {0, 0, 1};
        cases.push_back(walk_case(w));
        w.label = "no fonts in the files"; w.names = names_of(6); w.fonts = {0}; w.next = {0, 1};
        cases.push_back(walk_case(w));
    }
    {
        Walk w{.label = "fonts read from a FILE"};
        w.next = {1}; w.fonts = {1, 0}; w.names = names_of(4); w.file = any_file; w.handle = 0;
        // The name reader's frame lies below the builder's (0x90 bytes); the pieces it reads are poked in with its first fread.
        const auto reader = builder_base - 0x90;
        auto item = walk_case(w);
        std::vector<Write> pieces;
        const auto bytes = [&](std::uint32_t at, std::initializer_list<std::uint8_t> list) {
            std::uint32_t n = 0;
            for (auto value : list) pieces.push_back(w8(at + n++, value));
        };
        bytes(reader + 0x5c, {0, 1, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0});
        bytes(reader + 0x4c, {'n', 'a', 'm', 'e', 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1, 0});
        bytes(reader + 0x38, {0, 0, 0, 1, 0, 18});
        bytes(reader + 0x40, {0, 1, 0, 0, 0, 0, 0, 4, 0, 5, 0, 0});
        // The string goes to the name buffer of the builder's frame (+0x40).
        for (unsigned n = 0; n < 5; ++n) pieces.push_back(w8(builder_base + 0x40 + n, std::uint8_t("Hello"[n])));
        item.pokes.push_back({lib::fread, pieces});
        item.label = "fontfiles: builder fonts read from a FILE";
        cases.push_back(item);
    }
    // The 100-row limit is tested after a file's fonts are all listed: a file of 100 fonts ends the walk (no next lookup), one of 101 does not, a
    // preset count starts the rows further along. The rows are in the builder's stack, 100 of them below the directory record.
    for (const unsigned in_file : {100u, 101u, 99u, 3u})
        for (const unsigned preset : {0u, 98u, 99u}) {
            if ((in_file == 100 || in_file == 101 || in_file == 99) && preset != 0 && in_file != 3) continue;
            Walk w{.label = "file of " + std::to_string(in_file) + " fonts, count preset " + std::to_string(preset)};
            w.next = {0, 1};
            w.fonts.assign(in_file, 1);
            w.fonts.push_back(0);
            w.fonts.push_back(0);
            w.names = {name_strings + 0x40};
            w.data = images[0].memory;
            w.initial_count = preset;
            cases.push_back(walk_case(w));
        }
    for (const unsigned preset : {98u, 99u, 100u, 0xffffu}) {
        Walk w{.label = "three fonts, count preset " + std::to_string(preset)};
        w.next = {0, 1}; w.fonts = {1, 1, 1, 0, 1, 0}; w.names = names_of(8); w.data = images[0].memory; w.initial_count = preset;
        cases.push_back(walk_case(w));
    }
    (void)random;
    (void)first_case;
}
}
