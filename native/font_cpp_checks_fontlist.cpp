// FONT.CPP, block `fontlist` (0x10750a00..0x10750d28): cases for the font list's folder finder, its four wrappers (the loader, the
// selection setter, the count and the name getter) and their lazy-build prelude. The state is the save-info snapshot's: the list is
// built there (the flag is set), so the prelude's other branches are made by poking the flag, the count, the table and the
// selection variables, with the generated list builder 0x10750d30 (an in-page callee of the original, which cannot be answered)
// kept deterministic by answering the routines it calls on other pages (the directory iteration 0x10739690, the path maker
// 0x10739b40, the cleanup 0x107156a0) so its loop is skipped. The builder's result is then its count test: the count variable
// poked to zero makes it return 0 (the list stays unbuilt) and a nonzero count makes it allocate a table and copy 12 bytes a row
// of its own stack's leftovers, so those cases fill the dead stack with a pattern first (both sides then read the same bytes).
// Rows of the font table and the names they point to are built in the scratch area. Labels start with "fontlist:".
#include "font_cpp_checks.hpp"
#include "font_cpp_detail.hpp"
#include <cstdlib>
#include <random>

namespace cm::aot::font_cpp_checks {
namespace {
using namespace cm::aot::font_cpp;
constexpr std::uint32_t load_list = 0x10750a00, select = 0x10750a60, count_entry = 0x10750b00, name_entry = 0x10750b60, folder_entry = 0x10750ca0;
constexpr std::uint32_t directory_first = 0x10739690, path_maker = 0x10739b40, cleanup = 0x107156a0, localized = 0x10727890, copier = 0x1077dc10;
constexpr std::uint32_t rows_at = scratch_base + 0x30000, names_at = scratch_base + 0x31000, buffer_at = scratch_base + 0x32000;
constexpr std::uint32_t buffer_span = 0x120;
}

FONT_CPP_CASES("fontlist") {
    std::mt19937 random(0xf0a7);
    const auto built = scenario.toc(toc::list_built), count = scenario.toc(toc::list_count), table = scenario.toc(toc::list_table),
               selection = scenario.toc(toc::selection), folder = scenario.toc(toc::folder_path);
    const auto real_count = scenario.u16(count);
    const std::uint32_t real_table = scenario.r32(table);
    if (std::getenv("CM_FONT_CPP_SHOW_STATE"))
        std::fprintf(stderr, "fontlist state: built %u count %u table %x selection %x folder '%s'\n", scenario.u8(built), real_count, real_table,
                     scenario.r32(selection), [&] { static char text[0x101]; for (unsigned n = 0; n < 0x100; ++n) text[n] = char(scenario.u8(folder + n)); text[0x100] = 0; return text; }());

    // A table of `n` rows in the scratch area: row k's name "name-k", its alias "alias-k" when `alias(k)`; the count variable and
    // the table variable point at them.
    const auto rows = [&](unsigned n, auto alias) {
        std::vector<Write> memory{w16(count, n), w32(table, rows_at)};
        for (unsigned k = 0; k < n; ++k) {
            const auto name = names_at + k * 0x40, other = name + 0x20;
            const char* text = "name-";
            for (unsigned i = 0; text[i]; ++i) memory.push_back(w8(name + i, std::uint8_t(text[i])));
            memory.push_back(w8(name + 5, std::uint8_t('0' + k))), memory.push_back(w8(name + 6, 0));
            for (unsigned i = 0; i < 6; ++i) memory.push_back(w8(other + i, std::uint8_t("alias-"[i])));
            memory.push_back(w8(other + 6, std::uint8_t('0' + k))), memory.push_back(w8(other + 7, 0));
            memory.push_back(w32(rows_at + k * font_row::size + font_row::name, name));
            memory.push_back(w32(rows_at + k * font_row::size + font_row::alias, alias(k) ? other : 0));
            memory.push_back(w32(rows_at + k * font_row::size + 8, 0x01020304 + k));
        }
        return memory;
    };
    const auto fill = [](std::uint32_t at, std::uint32_t size, std::uint8_t byte) {
        std::vector<Write> memory;
        for (std::uint32_t n = 0; n < size; ++n) memory.push_back(w8(at + n, byte));
        return memory;
    };
    // The dead stack below the case's stack pointer, filled with a pattern (the generated builder copies leftover words of its own frame
    // into the table when its count is nonzero).
    const auto stack_pattern = [&] {
        std::vector<Write> memory;
        for (std::uint32_t n = 4; n <= 0x1800; n += 4) memory.push_back(w32(scenario.sp() - n, 0x5a5a5a5au));
        return memory;
    };
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> builder_answers{{directory_first, 1}, {path_maker, 0}, {cleanup, 0}};

    const auto first_case = cases.size();
    // ---- The prelude's branches, through each wrapper: built (the snapshot), not built with the builder returning 0 and 1.
    for (const auto entry : {load_list, select, count_entry, name_entry}) {
        std::vector<std::pair<unsigned, std::uint32_t>> registers{{3, entry == select ? std::uint32_t(-1) : entry == name_entry ? std::uint32_t(-2) : 0x1234u},
                                                                   {4, buffer_at}, {5, 32}};
        const auto label = std::string("fontlist: ") + hex(entry);
        const auto base = [&](std::vector<Write> memory, const char* what) {
            Case item{.entry = entry, .registers = registers, .memory = std::move(memory), .label = label + " " + what};
            return item;
        };
        // Rare sites first (the call trace pairs only the first cases of an entry): the builder paths.
        {
            auto memory = rows(3, [](unsigned k) { return k % 2 == 1; });
            memory.push_back(w8(built, 0));
            auto item = base(memory, "unbuilt, builder returns 0 (count 0)");
            item.memory.push_back(w16(count, 0));
            item.answers = builder_answers;
            cases.push_back(item);
            auto stacked = base(rows(3, [](unsigned k) { return k % 2 == 1; }), "unbuilt, builder returns 1 (count 3)");
            stacked.memory.push_back(w8(built, 0));
            append(stacked.memory, stack_pattern());
            stacked.answers = builder_answers;
            cases.push_back(stacked);
        }
        cases.push_back(base({w8(built, 0xff)}, "built flag 0xff"));
        cases.push_back(base({w8(built, 1)}, "built flag 1"));
        cases.push_back(base({}, "snapshot state"));
    }

    // ---- 0x10750ca0: the folder finder, with the buffer prefilled (the last byte's terminator) and the real data path.
    cases.push_back({.entry = folder_entry, .memory = fill(folder, folder_size, 0x5a), .label = "fontlist: folder prefilled"});
    cases.push_back({.entry = folder_entry, .label = "fontlist: folder snapshot"});
    cases.push_back({.entry = folder_entry, .memory = fill(folder, folder_size, 0), .label = "fontlist: folder zeroed"});
    cases.push_back({.entry = folder_entry, .memory = fill(folder, folder_size, 0xff), .label = "fontlist: folder 0xff"});
    // A path of 0x100 bytes or more (the data path's static buffer filled before the copy): only the explicit terminator ends the copy.
    {
        const auto path = scenario.toc(-0x3e38);
        std::vector<Write> longest;
        for (std::uint32_t n = 0; n < folder_size; ++n) longest.push_back(w8(path + n, 'A'));
        Case item{.entry = folder_entry, .memory = fill(folder, folder_size, 0x5a), .label = "fontlist: folder long path"};
        item.pokes = {{copier, longest}};
        cases.push_back(item);
        auto exact = item;
        exact.pokes = {{copier, {w8(path + 0xfe, 'A'), w8(path + 0xff, 0)}}};
        exact.label = "fontlist: folder path of 0xff bytes";
        cases.push_back(exact);
    }
    // The data path's static buffer is a TOC word that is never zero: with it poked to zero (and the page at address 0 mapped, as
    // --null-page does) both lookups return null and the folder is emptied.
    if (std::getenv("CM_FONT_CPP_NULL_PAGE")) {
        auto memory = fill(folder, folder_size, 0x5a);
        memory.push_back(w32(0x108bba90 - 0x3e38, 0));
        cases.push_back({.entry = folder_entry, .memory = memory, .label = "fontlist: folder, no data path (null page)"});
    }

    // ---- 0x10750a60: the selection setter. Indices around the edges of the two ranges and of the count.
    for (const unsigned n : {0u, 1u, 3u, 5u, 300u, 0xffffu})
        for (const std::int32_t index : {-1000, -4, -3, -2, -1, 0, 1, 2, 3, 4, 5, 299, 300, 301, 0xffff, 0x10000, 0x7fffffff, INT32_MIN}) {
            std::vector<Write> memory{w16(count, n), w32(selection, 0x7777)};
            cases.push_back({.entry = select, .registers = {{3, std::uint32_t(index)}}, .memory = memory,
                             .label = "fontlist: select " + std::to_string(index) + " of " + std::to_string(n)});
        }
    cases.push_back({.entry = select, .registers = {{3, std::uint32_t(0)}}, .label = "fontlist: select 0 snapshot"});
    cases.push_back({.entry = select, .registers = {{3, std::uint32_t(-1)}}, .label = "fontlist: select -1 snapshot"});

    // ---- 0x10750b00: the count.
    for (const unsigned n : {0u, 1u, 7u, 0xffffu, 0x8000u}) cases.push_back({.entry = count_entry, .memory = {w16(count, n)}, .label = "fontlist: count " + std::to_string(n)});
    cases.push_back({.entry = count_entry, .label = "fontlist: count snapshot"});

    // ---- 0x10750b60: the name getter. Built-in styles (the localized text real and answered), listed fonts with and without alias,
    // empty and missing table, indexes at the edges, sizes around the buffer.
    for (const std::int32_t index : {-1, -2}) {
        for (const std::uint32_t size : {0u, 1u, 2u, 5u, 33u, 100u, 0x100u}) {
            for (const bool answered : {false, true}) {
                auto memory = fill(buffer_at, buffer_span, 0x5a);
                Case item{.entry = name_entry, .registers = {{3, std::uint32_t(index)}, {4, buffer_at}, {5, size}}, .memory = memory,
                          .label = "fontlist: name " + std::to_string(index) + " size " + std::to_string(size) + (answered ? " localizer answered" : " localizer real")};
                if (answered) item.answers = {{localized, 0x11}};
                cases.push_back(item);
            }
        }
        cases.push_back({.entry = name_entry, .registers = {{3, std::uint32_t(index)}, {4, buffer_at}, {5, 64}}, .memory = fill(buffer_at, buffer_span, 0x5a),
                         .label = "fontlist: name " + std::to_string(index) + " copier answered", .answers = {{copier, 0}}});
    }
    for (const unsigned n : {0u, 1u, 2u, 5u})
        for (const std::int32_t index : {-1000, -4, -3, 0, 1, 2, 4, 5, 6, 0x7fffffff, INT32_MIN})
            for (const std::uint32_t size : {0u, 1u, 3u, 8u, 0x40u, 0x100u}) {
                auto memory = rows(n, [](unsigned k) { return k % 2 == 1; });
                append(memory, fill(buffer_at, buffer_span, 0x5a));
                cases.push_back({.entry = name_entry, .registers = {{3, std::uint32_t(index)}, {4, buffer_at}, {5, size}}, .memory = memory,
                                 .label = "fontlist: name row " + std::to_string(index) + " of " + std::to_string(n) + " size " + std::to_string(size)});
            }
    // The table variable null with a nonzero count, and the snapshot's own table with each of its rows.
    for (const std::int32_t index : {0, 1, 3}) {
        auto memory = fill(buffer_at, buffer_span, 0x5a);
        memory.push_back(w32(table, 0));
        memory.push_back(w16(count, 4));
        cases.push_back({.entry = name_entry, .registers = {{3, std::uint32_t(index)}, {4, buffer_at}, {5, 40}}, .memory = memory,
                         .label = "fontlist: name null table " + std::to_string(index)});
    }
    for (std::int32_t index = -3; index <= std::int32_t(real_count) + 1; ++index)
        cases.push_back({.entry = name_entry, .registers = {{3, std::uint32_t(index)}, {4, buffer_at}, {5, 0x40}}, .memory = fill(buffer_at, buffer_span, 0x5a),
                         .label = "fontlist: name snapshot row " + std::to_string(index)});
    // Random: arbitrary size and index, alias pattern and buffer contents.
    for (unsigned n = 0; n < 60; ++n) {
        const unsigned rows_count = random() % 6;
        const auto alias_bits = std::uint32_t(random());
        auto memory = rows(rows_count, [&](unsigned k) { return alias_bits >> k & 1; });
        for (std::uint32_t at = 0; at < buffer_span; ++at) memory.push_back(w8(buffer_at + at, std::uint8_t(random())));
        cases.push_back({.entry = name_entry, .registers = {{3, std::uint32_t(std::int32_t(random() % 9) - 3)}, {4, buffer_at}, {5, 1 + random() % 0x80}},
                         .memory = memory, .label = "fontlist: name random " + std::to_string(n)});
    }
    // The snapshot's list is not built (the flag is clear and the count zero), so every case that is not about the prelude sets the flag:
    // its wrapper then goes straight to the list's variables. The prelude's cases above keep the flag they set or the snapshot's.
    for (auto at = cases.begin() + std::ptrdiff_t(first_case); at != cases.end(); ++at)
        if (at->entry != folder_entry && at->label.find("unbuilt") == std::string::npos && at->label.find("built flag") == std::string::npos &&
            at->label.find("snapshot state") == std::string::npos)
            at->memory.insert(at->memory.begin(), w8(built, 1));
}
}
