#pragma once
// The media.cpp checker's case lists. A block adds a `media_<block>_checks.cpp` with one
// MEDIA_CASES block; the checker runs each case's call as the original and as the C++
// in forked children from a real career state, then the same call with mutated inputs
// (values the original actually read), and compares all the game can observe. Pointers and
// states come from the real world, never fabricated.
#include "aot_runtime.hpp"
#include <functional>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cm::aot::media_checks {
// A mapped area outside the game's memory for the rows a case writes.
inline constexpr std::uint32_t scratch_base = 0x60000000, scratch_size = 0x100000;
struct Write { unsigned size; std::uint32_t address, value; };
struct Case {
    std::uint32_t entry = 0;
    std::vector<std::pair<unsigned, std::uint32_t>> registers;  // r3...
    std::vector<Write> memory;                                  // applied to both sides, undone afterwards
    std::optional<double> f1;
    std::string label = {};
    // Another module's routines answered on both sides (entry, r3) when dispatched, with no
    // body run: for paths that depend on what the snapshot can't produce.
    std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {};
};
// A real career: the world's tables through their TOC slots.
struct Scenario {
    State& s;
    std::uint32_t r32(std::uint32_t at) const { return std::uint32_t(s.memory.read<4>(at)); }
    std::uint32_t toc(int slot) const { return r32(0x108bba90 + std::uint32_t(slot)); }
    std::uint32_t global(int slot) const { return r32(toc(slot)); }
    // The world's records: club (0x248), competition (0x6c), person (0x128) tables.
    std::uint32_t club(unsigned index) const { return global(-0x7be8) + index * 0x248; }
    std::uint32_t competition(unsigned index) const { return global(-0x7bbc) + index * 0x6c; }
    std::uint32_t person(unsigned index) const { return global(-0x7b98) + index * 0x128; }
};
using CaseList = std::function<void(const Scenario&, std::vector<Case>&)>;
struct CaseBlock {
    CaseBlock(const char* name, CaseList list);
};
#define MEDIA_CASES_JOIN2(a, b) a##b
#define MEDIA_CASES_JOIN(a, b) MEDIA_CASES_JOIN2(a, b)
// MEDIA_CASES("leaves") { cases.push_back({.entry = 0x..., .registers = {{3, ...}}}); }
#define MEDIA_CASES(name) \
    static void MEDIA_CASES_JOIN(media_cases_, __LINE__)(const ::cm::aot::media_checks::Scenario&, std::vector<::cm::aot::media_checks::Case>&); \
    static ::cm::aot::media_checks::CaseBlock MEDIA_CASES_JOIN(media_block_, __LINE__){name, MEDIA_CASES_JOIN(media_cases_, __LINE__)}; \
    static void MEDIA_CASES_JOIN(media_cases_, __LINE__)([[maybe_unused]] const ::cm::aot::media_checks::Scenario& scenario, [[maybe_unused]] std::vector<::cm::aot::media_checks::Case>& cases)
}
