#pragma once
// The MATCH_ENG.CPP checker's case lists. A block adds a `match_eng_<block>_checks.cpp`
// with one MATCH_ENG_CASES block; the checker runs each case's call as the original
// and as the C++ in forked children from a real match state, then the same call with
// mutated inputs (values the original actually read), and compares all the game can
// observe. Pointers and states come from the real match, never fabricated.
#include "aot_runtime.hpp"
#include <array>
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cm::aot::match_eng_checks {
struct Write { unsigned size; std::uint32_t address, value; };
struct Case {
    std::uint32_t entry = 0;
    std::vector<std::pair<unsigned, std::uint32_t>> registers;  // r3...
    std::vector<Write> memory;                                  // applied to both sides, undone afterwards
    std::optional<double> f1;
    std::string label = {};
    // Another module's routines answered on both sides (entry, r3) when dispatched, with no
    // body run: for paths that depend on what a match snapshot can't produce.
    std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {};
};
// A real match state: the object, its 40 players and one scratch stack area the
// routine's frame can grow into.
struct Scenario {
    State& s;
    std::uint32_t match;
    std::uint32_t player(unsigned side, unsigned index) const { return match + 0x311c + side * 0x2350 + index * 0x1c4; }
    std::uint32_t r32(std::uint32_t at) const { return std::uint32_t(s.memory.read<4>(at)); }
};
using CaseList = std::function<void(const Scenario&, std::vector<Case>&)>;
struct CaseBlock {
    CaseBlock(const char* name, CaseList list);
};
#define MATCH_ENG_CASES_JOIN2(a, b) a##b
#define MATCH_ENG_CASES_JOIN(a, b) MATCH_ENG_CASES_JOIN2(a, b)
// MATCH_ENG_CASES("leaves") { cases.push_back({.entry = 0x..., .registers = {{3, scenario.match}}}); }
#define MATCH_ENG_CASES(name) \
    static void MATCH_ENG_CASES_JOIN(match_eng_cases_, __LINE__)(const ::cm::aot::match_eng_checks::Scenario&, std::vector<::cm::aot::match_eng_checks::Case>&); \
    static ::cm::aot::match_eng_checks::CaseBlock MATCH_ENG_CASES_JOIN(match_eng_block_, __LINE__){name, MATCH_ENG_CASES_JOIN(match_eng_cases_, __LINE__)}; \
    static void MATCH_ENG_CASES_JOIN(match_eng_cases_, __LINE__)([[maybe_unused]] const ::cm::aot::match_eng_checks::Scenario& scenario, [[maybe_unused]] std::vector<::cm::aot::match_eng_checks::Case>& cases)
}
