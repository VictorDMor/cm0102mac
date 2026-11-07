#pragma once
// DATABASE.CPP 0x100ce320's code table, extracted from its instructions (see
// database_records_ce320.cpp), for the routine and its fixtures.
//
// The original tests a record's code word against 82 pairs of words twice: in
// the first stage a code equal to a pair's `code` word maps to the index in
// its `index` word; in the second stage the roles swap (a code equal to an
// `index` word maps to the `code` word). Each word is a TOC slot; the
// original loads the first 58 pairs' slots as it goes and keeps the rest in
// registers loaded at entry (so its register state differs by position).
#include <array>
#include <cstdint>

namespace cm::aot::database::records::club_code {
struct Pair {
    std::int32_t code, index;  // TOC offsets of the two words
};
inline constexpr unsigned pair_count = 82, loaded_pairs = 58;
inline constexpr Pair pairs[pair_count] = {
    {-0x76ec, -0x782c}, {-0x75cc, -0x75d0}, {-0x76f0, -0x7830}, {-0x76f4, -0x7834}, {-0x76f8, -0x7838},
    {-0x76fc, -0x783c}, {-0x7700, -0x7840}, {-0x7704, -0x7844}, {-0x7708, -0x7848}, {-0x770c, -0x784c},
    {-0x7710, -0x7850}, {-0x7714, -0x7854}, {-0x7718, -0x7858}, {-0x771c, -0x785c}, {-0x7720, -0x7860},
    {-0x7724, -0x7864}, {-0x7728, -0x7868}, {-0x75d4, -0x7614}, {-0x772c, -0x786c}, {-0x7730, -0x7870},
    {-0x7734, -0x7874}, {-0x7738, -0x7878}, {-0x773c, -0x787c}, {-0x7740, -0x7880}, {-0x7744, -0x7884},
    {-0x7748, -0x7888}, {-0x774c, -0x788c}, {-0x7750, -0x7890}, {-0x7754, -0x7894}, {-0x7758, -0x7898},
    {-0x775c, -0x789c}, {-0x7760, -0x78a0}, {-0x7764, -0x78a4}, {-0x7768, -0x78a8}, {-0x776c, -0x78ac},
    {-0x7770, -0x78b0}, {-0x7774, -0x78b4}, {-0x7778, -0x78b8}, {-0x777c, -0x78bc}, {-0x7780, -0x78c0},
    {-0x7784, -0x78c4}, {-0x7788, -0x78c8}, {-0x778c, -0x78cc}, {-0x7790, -0x78d0}, {-0x7794, -0x78d4},
    {-0x7798, -0x78d8}, {-0x779c, -0x78dc}, {-0x77a0, -0x78e0}, {-0x77a4, -0x78e4}, {-0x77a8, -0x78e8},
    {-0x77ac, -0x78ec}, {-0x77b0, -0x78f0}, {-0x77b4, -0x78f4}, {-0x77b8, -0x78f8}, {-0x77bc, -0x78fc},
    {-0x77c0, -0x7900}, {-0x77c4, -0x7904}, {-0x77c8, -0x7908}, {-0x77cc, -0x790c}, {-0x77d0, -0x7910},
    {-0x77d4, -0x7914}, {-0x77d8, -0x7918}, {-0x77dc, -0x791c}, {-0x77e0, -0x7920}, {-0x77e4, -0x7924},
    {-0x77e8, -0x7928}, {-0x77ec, -0x792c}, {-0x77f0, -0x7930}, {-0x77f4, -0x7934}, {-0x77f8, -0x7938},
    {-0x77fc, -0x793c}, {-0x7800, -0x7940}, {-0x7804, -0x7944}, {-0x7808, -0x7948}, {-0x780c, -0x794c},
    {-0x7810, -0x7950}, {-0x7814, -0x7954}, {-0x7818, -0x7958}, {-0x781c, -0x795c}, {-0x7820, -0x7960},
    {-0x7824, -0x7964}, {-0x7828, -0x7968},
};

// The TOC word the original keeps in register `r` (r15-r31, r12-r7, r3 are
// loaded at entry from consecutive words).
constexpr std::int32_t toc_of_register(unsigned r) {
    if (r >= 15) return -0x7968 + 4 * std::int32_t(r - 15);
    if (r == 3) return -0x790c;
    return -0x7910 - 4 * std::int32_t(r - 7);  // r7-r12
}
// The register that holds pair `position`'s index slot (positions from 58 on):
// r3, r7-r12, then r31 down to r15.
constexpr unsigned register_of_pair(unsigned position) {
    const unsigned k = position - loaded_pairs;
    return k == 0 ? 3 : k <= 6 ? 6 + k : 32 - (k - 6);
}

// For fixtures that only enumerate the TOC offsets the routine reads (as
// database_checks_search.cpp does): each stage's tests as flat lists, derived
// from `pairs`. The routine does not use them.
enum class Kind : std::uint8_t { member_toc, member_reg, map_toc_toc, map_toc_r3, map_toc_reg, map_reg_toc };
struct Step {
    Kind kind;
    std::int32_t a, b;
};
namespace detail {
constexpr std::array<Step, pair_count> steps(Kind kind, bool second) {
    std::array<Step, pair_count> out{};
    for (unsigned i = 0; i < pair_count; ++i)
        out[i] = {kind, second ? pairs[i].index : pairs[i].code, second ? pairs[i].code : pairs[i].index};
    return out;
}
}
inline constexpr auto first_members = detail::steps(Kind::member_toc, false);
inline constexpr auto first_map = detail::steps(Kind::map_toc_toc, false);
inline constexpr auto second_members = detail::steps(Kind::member_toc, true);
inline constexpr auto second_map = detail::steps(Kind::map_toc_toc, true);
}
