// COMP_UTIL.CPP, block `round_codes`: cases for 0x1046f1f0 (a round code and a kind byte to a small class, under
// the preferences record's language byte) and 0x10476ec0 (a competition record's name class).
//
// 0x1046f1f0 is a function of (code halfword, kind byte, language byte) and the three are registers or one byte of
// memory, so the cases are exhaustive over what it distinguishes: every code from -2 to 0x500 under every language
// that has tables (2, 3, 4, 5, 9) with kind 3 and with kinds 0 and 4 (the kind byte only tells 3 from the rest),
// the 16-bit extremes and a code with garbage in the register's upper half (extsh ignores it), every kind byte
// value (0..255) at one code of each stretch, and the languages without a table (0, 1, 6, 7, 8, 10, 255) over every
// stretch's edges. The language byte is poked into the preferences record (undone after each case). Both sides
// compare r3, r5 and the other registers the routine leaves (r4-r10 are compared: no volatile-free exemption).
// 0x10476ec0 reads a competition record: each real record, a null one, with its two gender bytes poked to
// distinct values, every pair of (first, second) codes that is zero or not (also as a halfword that is zero in a
// register that is not), every kind. Labels start with "round_codes:" (CM_COMP_UTIL_BLOCK=round_codes).
#include "comp_util_checks.hpp"
#include <set>
#include <string>

namespace cm::aot::comp_util_checks {
namespace {
constexpr std::uint32_t gender_of_code = 0x1046f1f0, gender_of_competition = 0x10476ec0;
constexpr int preferences_slot = -0x7ac8;
constexpr std::uint32_t language_offset = 8;
Write w8(std::uint32_t at, std::uint32_t value) { return {1, at, value & 0xff}; }
std::string number(std::int64_t value) { return std::to_string(value); }

// The edges of every stretch of codes the tables have (and the gaps between them).
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
Case code_case(const Scenario& scenario, std::uint32_t language, std::uint32_t code_register, std::uint32_t kind_register, const std::string& what) {
    return {.entry = gender_of_code, .registers = {{3, code_register}, {4, kind_register}},
            .memory = {w8(scenario.global(preferences_slot) + language_offset, language)},
            .label = "round_codes: " + what + " language " + number(language) + " code " + number(std::int16_t(code_register)) + " kind " + number(std::int8_t(kind_register))};
}
}

COMP_UTIL_CASES("round_codes") {
    // Every code from -2 to 0x500, under each language with tables, for kind 3, 0 and 4.
    for (const std::uint32_t language : {2u, 3u, 4u, 5u, 9u})
        for (const std::uint32_t kind : {3u, 0u, 4u})
            for (std::int32_t code = -2; code <= 0x500; ++code)
                cases.push_back(code_case(scenario, language, std::uint32_t(code), kind, "sweep"));
}

COMP_UTIL_CASES("round_codes") {
    // The 16-bit extremes and garbage above the halfword and the byte (extsh and extsb ignore it), per language.
    for (const std::uint32_t language : {2u, 3u, 4u, 5u, 9u})
        for (const std::uint32_t kind : {3u, 0u, 0x103u, 0xffffff03u, 0x80u})
            for (const std::uint32_t code : {0x7fffu, 0x8000u, 0xffffu, 0x10000u, 0x10005u, 0xabcd0005u, 0xffff0064u, 0x8000fffeu, 0x7fff0000u, 0x0001038fu})
                cases.push_back(code_case(scenario, language, code, kind, "extremes"));
}

COMP_UTIL_CASES("round_codes") {
    // Every kind byte value at each stretch's edges: only 3 differs from the others, in every language that has tables.
    for (const std::uint32_t language : {2u, 3u, 4u, 5u, 9u})
        for (std::uint32_t kind = 0; kind < 256; ++kind)
            for (const std::int32_t code : {0, 1, 44, 45, 50, 99, 100, 160, 374, 375, 1000, 1155, 1156, -1})
                cases.push_back(code_case(scenario, language, std::uint32_t(code), kind, "kind byte"));
}

COMP_UTIL_CASES("round_codes") {
    // Languages without a table: nothing is read of the code, 0 is answered, r5 stays; and the byte just beside each
    // language that has one (a language is the low byte only).
    for (const std::uint32_t language : {0u, 1u, 6u, 7u, 8u, 10u, 11u, 0x82u, 0xffu})
        for (const std::uint32_t kind : {3u, 0u})
            for (const auto code : edges())
                cases.push_back(code_case(scenario, language, std::uint32_t(code), kind, "other language"));
}

COMP_UTIL_CASES("round_codes") {
    // The competition record's class: its two gender bytes at distinct values so a wrong one is visible; the
    // first code (r5) wins over the second (r4), a code whose halfword is zero counts as none, and without codes
    // kind 1 takes the long name's byte, any other kind the short name's, no record the constant 1.
    std::vector<std::uint32_t> records{0};
    for (unsigned index : {0u, 1u, 5u, 50u}) records.push_back(scenario.competition(index));
    const std::uint32_t none = 0, five = 5, big = 0x10000, odd = 0x1ffff, wide = 0xabcd0037u, negative = 0xffffu;
    for (const auto record : records)
        for (const std::uint32_t first : {none, five, big, odd, wide, negative})
            for (const std::uint32_t second : {none, five, big, 0x2cu, wide})
                for (const std::uint32_t kind : {0u, 1u, 2u, 3u, 0xffu, 0x101u}) {
                    Case item{.entry = gender_of_competition, .registers = {{3, record}, {4, second}, {5, first}, {6, kind}},
                              .label = "round_codes: competition record " + number(record == 0 ? -1 : (record - scenario.competition(0)) / 0x6c) + " first " + number(std::int32_t(first)) + " second " + number(std::int32_t(second)) + " kind " + number(kind)};
                    if (record) item.memory = {w8(record + 0x67, 0x41), w8(record + 0x6a, 0x52)};
                    cases.push_back(std::move(item));
                }
    // The routine read under the language that makes the code path run (a language byte with tables).
    for (const std::uint32_t language : {2u, 3u, 5u, 9u, 0u})
        for (const std::uint32_t first : {five, 50u, 160u, 1000u, 1156u})
            cases.push_back({.entry = gender_of_competition, .registers = {{3, scenario.competition(1)}, {4, 0}, {5, first}, {6, 1}},
                             .memory = {w8(scenario.global(preferences_slot) + language_offset, language)},
                             .label = "round_codes: competition under language " + number(language) + " first " + number(first)});
}
}
