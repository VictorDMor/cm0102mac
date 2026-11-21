// COMP_UTIL.CPP, block `round_text`: cases for the round's name (0x10469bb0) and the sentences built around it
// (0x10476f40, 0x104768a0, 0x104769d0, 0x10469710, 0x104698c0).
//
// 0x10469bb0 is a function of (code halfword, kind byte, size, the language the localized format answers in): every
// code from -2 to 0x500 under each kind with tables (0, 1, 3), the 16-bit extremes, garbage above the halfword,
// every kind byte at the stretches' edges, the sizes around 30 and the assertions (size below 30, a kind without a
// table), each under the snapshot's own language and a few others (the byte at TOC -0x35ac the lookup reads).
// Each case carries empty pokes of the two callees the arms use, which makes the checker trace it: the call trace
// then compares, for every code, the format call's site (the arm's own), the non-volatile registers and the
// arguments the callee reads (--argument-masks). The wrappers get real competition records and clubs from the
// state and fixture rows built in the scratch area from real values (record pointer, the two codes), every branch
// (null and short buffers, no record, no codes, one or two codes, the special competitions' marker codes, every
// kind, language 8's translation) and each assertion. The news item the sentence builders append to is a scratch
// pointer: the two appenders (0x1074b830, 0x1074bab0) are answered, so what they would copy of the caller's
// leftover stack is not run here (listed with the block's limits). Labels start with "round_text:".
#include "comp_util_checks.hpp"
#include <set>
#include <string>

namespace cm::aot::comp_util_checks {
namespace {
constexpr std::uint32_t round_text = 0x10469bb0, pair_text = 0x10476f40, fixture_text = 0x104768a0, name_text = 0x104769d0, competition_news = 0x10469710,
                        fixture_news = 0x104698c0;
constexpr std::uint32_t format = 0x10727890, sprintf_entry = 0x1077b720, copy_entry = 0x1077dbf0, begin_item = 0x1074b830, append_item = 0x1074bab0;
constexpr std::uint32_t language_byte = 0x108bba90u - 0x35ac;  // the localization lookup's current language
constexpr std::uint32_t buffer_at = scratch_base + 0xa0000, rows_at = scratch_base + 0xb0000, item_at = scratch_base + 0xc0000;
Write w8(std::uint32_t at, std::uint32_t value) { return {1, at, value & 0xff}; }
Write w16(std::uint32_t at, std::uint32_t value) { return {2, at, value & 0xffff}; }
Write w32(std::uint32_t at, std::uint32_t value) { return {4, at, value}; }
std::string number(std::int64_t value) { return std::to_string(value); }
// An empty poke on each callee a case reaches: the checker traces every case that has one.
std::vector<Case::Poke> traced() {
    return {{format, {}}, {sprintf_entry, {}}, {copy_entry, {}}};
}
std::vector<Case::Poke> traced_news() {
    return {{format, {}}, {begin_item, {}}, {append_item, {}}, {copy_entry, {}}};
}
// The language the snapshot has (no poke) and the ones a case can select by memory.
const std::vector<std::int32_t> languages = {-1 /* the snapshot's own */, 8, 1, 2, 3, 4, 5};
std::vector<Write> in_language(std::int32_t language) {
    return language < 0 ? std::vector<Write>{} : std::vector<Write>{w8(language_byte, std::uint32_t(language))};
}
std::vector<Write> buffer_filled(std::vector<Write> memory = {}) {
    memory.push_back(w32(buffer_at, 0x41424344));
    memory.push_back(w32(buffer_at + 4, 0x45464748));
    return memory;
}
std::string label(const std::string& what, std::int32_t language, std::uint32_t code, std::uint32_t kind) {
    return "round_text: " + what + " language " + number(language) + " code " + number(std::int16_t(code)) + " kind " + number(std::int8_t(kind));
}
Case name_case(std::uint32_t code, std::uint32_t kind, std::uint32_t size, std::int32_t language, const std::string& what) {
    return {.entry = round_text, .registers = {{3, buffer_at}, {4, size}, {5, code}, {6, kind}}, .memory = buffer_filled(in_language(language)),
            .label = label(what + " size " + number(std::int32_t(size)), language, code, kind), .pokes = traced()};
}
// The codes the stretches start and end at, and one past.
const std::set<std::int32_t>& edges() {
    static const std::set<std::int32_t> set = [] {
        std::set<std::int32_t> out;
        for (const std::int32_t edge : {0, 44, 50, 94, 100, 155, 160, 374, 1000, 1019, 1020, 1035, 1036, 1047, 1048, 1054, 1055, 1155})
            for (std::int32_t d = -2; d <= 2; ++d) out.insert(edge + d);
        out.insert({-32768, -32767, -1, 32767, 0x400, 0x483, 0x484, 0x485, 0x500});
        return out;
    }();
    return set;
}
// The scratch fixture rows: +0x14 the competition record, +0x30 and +0x32 the two round codes.
std::uint32_t fixture_row(std::vector<Write>& memory, unsigned slot, std::uint32_t record, std::uint32_t code_30, std::uint32_t code_32) {
    const auto row = rows_at + slot * 0x100;
    memory.push_back(w32(row + 0x14, record));
    memory.push_back(w16(row + 0x30, code_30));
    memory.push_back(w16(row + 0x32, code_32));
    return row;
}
}

COMP_UTIL_CASES("round_text") {
    // Every code from -2 to 0x500 under each kind with tables, in the snapshot's language, 8 and one more.
    for (const std::int32_t language : {-1, 8, 3})
        for (const std::uint32_t kind : {0u, 1u, 3u})
            for (std::int32_t code = -2; code <= 0x500; ++code)
                cases.push_back(name_case(std::uint32_t(code), kind, 100, language, "sweep"));
}

COMP_UTIL_CASES("round_text") {
    // The edges of every stretch under the other languages, the 16-bit extremes and garbage above the halfword and
    // the byte, the sizes around the assertion (30) and a size that is negative as a word.
    for (const std::int32_t language : languages)
        for (const std::uint32_t kind : {0u, 1u, 3u})
            for (const auto code : edges()) cases.push_back(name_case(std::uint32_t(code), kind, 100, language, "edges"));
    for (const std::int32_t language : {-1, 8})
        for (const std::uint32_t kind : {0u, 1u, 3u, 0x100u, 0x101u, 0xffffff03u})
            for (const std::uint32_t code : {0x7fffu, 0x8000u, 0xffffu, 0x10000u, 0x10005u, 0xabcd0005u, 0xffff0064u, 0x8000fffeu, 0x0001038fu})
                cases.push_back(name_case(code, kind, 100, language, "garbage"));
    for (const std::uint32_t size : {0u, 1u, 29u, 30u, 31u, 100u, 0x7fffffffu, 0x80000000u, 0xffffffffu, 1u})
        for (const std::uint32_t kind : {0u, 1u, 3u, 2u})
            for (const std::int32_t code : {0, 5, 1155, -1})
                cases.push_back(name_case(std::uint32_t(code), kind, size, -1, "size"));
}

COMP_UTIL_CASES("round_text") {
    // The kind byte at each stretch's edges: only 0, 1 and 3 have tables, every other value reports the assertion.
    for (std::uint32_t kind = 0; kind < 256; ++kind)
        for (const std::int32_t code : {0, 1, 44, 45, 160, 1155, 1156, -1})
            cases.push_back(name_case(std::uint32_t(code), kind, 100, -1, "kind byte"));
}

COMP_UTIL_CASES("round_text") {
    // 0x10476f40: one or two codes into a buffer. A code counts as nonzero by its whole register (0x10000 is a
    // nonzero register whose halfword is 0); sizes around 200; the buffer null; every kind.
    const std::uint32_t codes[] = {0, 1, 5, 44, 160, 1000, 1156, 0x10000, 0x10005, 0xffff, 70000};
    for (const std::int32_t language : {-1, 8, 2})
        for (const std::uint32_t kind : {0u, 1u, 3u, 2u})
            for (const auto first : codes)
                for (const auto second : codes)
                    cases.push_back({.entry = pair_text, .registers = {{3, first}, {4, second}, {5, buffer_at}, {6, 300}, {7, kind}},
                                     .memory = buffer_filled(in_language(language)),
                                     .label = "round_text: pair " + number(first) + " " + number(second) + " kind " + number(kind) + " language " + number(language),
                                     .pokes = traced()});
    for (const std::uint32_t size : {0u, 199u, 200u, 201u, 0x80000000u})
        for (const std::uint32_t buffer : {buffer_at, 0u})
            for (const auto first : {0u, 5u})
                for (const auto second : {0u, 160u})
                    cases.push_back({.entry = pair_text, .registers = {{3, first}, {4, second}, {5, buffer}, {6, size}, {7, 0}}, .memory = buffer_filled(),
                                     .label = "round_text: pair assertion size " + number(std::int32_t(size)) + " buffer " + number(buffer) + " codes " + number(first) + " " + number(second)});
}

COMP_UTIL_CASES("round_text") {
    // 0x104769d0: records (none and four real ones), kinds 0..3 and others, the two codes (nothing, plain, the
    // special competitions' markers 0x453 and 0x456 in the +0x32 slot, a code whose halfword is 0), the sizes around
    // 0x67, a null buffer, and language 8 (the names through 0x10728ac0 and the table).
    std::vector<std::uint32_t> records{0};
    for (const unsigned index : {0u, 1u, 7u, 120u}) records.push_back(scenario.competition(index));
    // The records the two special competitions are: the id the cell at TOC -0x6750 holds names one of them.
    const auto special_id = scenario.r32(scenario.toc(-0x6750));
    records.push_back(scenario.competition(special_id));
    const std::uint32_t codes_32[] = {0, 5, 50, 160, 1000, 1156, 0x453, 0x456, 0x10000};
    const std::uint32_t codes_30[] = {0, 5, 44, 160, 1000, 0x10001};
    for (const std::int32_t language : {-1, 8})
        for (const auto record : records)
            for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 4u, 0xffu})
                for (const auto code_32 : codes_32)
                    for (const auto code_30 : codes_30)
                        cases.push_back({.entry = name_text, .registers = {{3, record}, {4, code_32}, {5, code_30}, {6, kind}, {7, buffer_at}, {8, 600}},
                                         .memory = buffer_filled(in_language(language)),
                                         .label = "round_text: name record " + number(record == 0 ? -1 : (record - scenario.competition(0)) / 0x6c) + " codes " + number(code_32) + " " +
                                                  number(code_30) + " kind " + number(kind) + " language " + number(language),
                                         .pokes = traced()});
    // Sizes (the assertion below 0x67) and a null buffer.
    for (const std::uint32_t size : {0u, 0x66u, 0x67u, 0x68u, 0x80000000u})
        for (const std::uint32_t buffer : {buffer_at, 0u})
            for (const std::uint32_t kind : {1u, 2u})
                cases.push_back({.entry = name_text, .registers = {{3, scenario.competition(1)}, {4, 5}, {5, 160}, {6, kind}, {7, buffer}, {8, size}}, .memory = buffer_filled(),
                                 .label = "round_text: name assertion size " + number(std::int32_t(size)) + " buffer " + number(buffer)});
}

COMP_UTIL_CASES("round_text") {
    // 0x104768a0: the checked wrapper over fixture rows built in the scratch area (record pointer, the two codes).
    std::vector<Write> memory;
    const auto real = fixture_row(memory, 0, scenario.competition(1), 160, 5);
    const auto no_record = fixture_row(memory, 1, 0, 160, 5);
    const auto no_codes = fixture_row(memory, 2, scenario.competition(1), 0, 0);
    const auto only_30 = fixture_row(memory, 3, scenario.competition(7), 1000, 0);
    const auto only_32 = fixture_row(memory, 4, scenario.competition(7), 0, 160);
    const auto special = fixture_row(memory, 5, scenario.competition(scenario.r32(scenario.toc(-0x6750))), 5, 0x453);
    for (const std::int32_t language : {-1, 8})
        for (const auto row : {real, no_record, no_codes, only_30, only_32, special, 0u})
            for (const std::uint32_t kind : {0u, 1u, 2u, 3u})
                for (const std::uint32_t size : {600u, 0x67u, 0x66u})
                    for (const std::uint32_t buffer : {buffer_at, 0u}) {
                        auto written = memory;
                        for (const auto& write : buffer_filled(in_language(language))) written.push_back(write);
                        cases.push_back({.entry = fixture_text, .registers = {{3, row}, {4, kind}, {5, buffer}, {6, size}}, .memory = written,
                                         .label = "round_text: fixture row " + number(row == 0 ? -1 : (row - rows_at) / 0x100) + " kind " + number(kind) + " size " + number(size) +
                                                  " buffer " + number(buffer) + " language " + number(language),
                                         .pokes = traced()});
                    }
}

COMP_UTIL_CASES("round_text") {
    // 0x10469710: a competition-and-round sentence. The fixture's +0x30 code is rounded down to ten (below 1000,
    // negative ones too); kinds 2, 4 and -1 take the first format. The news item is a scratch pointer and its two
    // appenders are answered.
    std::vector<Write> memory;
    std::vector<std::uint32_t> rows;
    unsigned slot = 0;
    for (const std::uint32_t code_30 : {0u, 5u, 10u, 12u, 99u, 160u, 999u, 1000u, 1001u, 0xfffbu /* -5 */, 0xfff1u /* -15 */, 0x7fffu, 0x8000u})
        for (const std::uint32_t code_32 : {0u, 5u, 0x453u})
            rows.push_back(fixture_row(memory, slot++, code_32 == 0x453 ? scenario.competition(scenario.r32(scenario.toc(-0x6750))) : scenario.competition(1), code_30, code_32));
    for (const std::int32_t language : {-1, 8})
        for (const auto row : rows)
            for (const std::uint32_t kind : {2u, 4u, 0xffu, 0u, 3u, 0x102u, 0x7fu})
                for (const std::uint32_t club : {0u, 1u, 9u}) {
                    auto written = memory;
                    for (const auto& write : in_language(language)) written.push_back(write);
                    cases.push_back({.entry = competition_news, .registers = {{3, club}, {4, row}, {5, kind}, {6, item_at}}, .memory = written,
                                     .label = "round_text: competition news row " + number((row - rows_at) / 0x100) + " kind " + number(kind) + " club " + number(club) + " language " + number(language),
                                     .pokes = traced_news(), .answers = {{begin_item, 0}, {append_item, 0}}});
                }
}

COMP_UTIL_CASES("round_text") {
    // 0x104698c0: a fixture-round line. Kinds 6 and 7 and -2 (and others, which build nothing), with and without a
    // round's text (the flag is a signed byte: 0x100 is zero), the round code a whole register (garbage above the
    // halfword is rotated into the tag), real competition records.
    for (const std::int32_t language : {-1, 8})
        for (const std::uint32_t record : {scenario.competition(1), scenario.competition(7), scenario.competition(120)})
            for (const std::uint32_t kind : {6u, 7u, 0xfeu, 0u, 5u, 0x106u, 0xffu, 0x80u})
                for (const std::uint32_t with_round : {0u, 1u, 0x100u, 0xffu, 0x80u})
                    for (const std::uint32_t code : {0u, 5u, 160u, 1000u, 1156u, 0xffffu, 0x10005u, 0xabcd0160u, 0x80000000u})
                        for (const std::uint32_t club : {0u, 3u}) {
                            cases.push_back({.entry = fixture_news, .registers = {{3, club}, {4, code}, {5, record}, {6, kind}, {7, with_round}, {8, item_at}}, .memory = in_language(language),
                                             .label = "round_text: fixture news kind " + number(kind) + " round " + number(with_round) + " code " + number(code) + " club " + number(club) +
                                                      " language " + number(language),
                                             .pokes = traced_news(), .answers = {{begin_item, 0}, {append_item, 0}}});
                        }
}
}
