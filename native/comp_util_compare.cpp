// COMP_UTIL.CPP, block `comparators`: the unit's qsort comparators (entries 0x10463580, 0x104647a0, 0x104647c0,
// 0x104647e0, 0x10464830, 0x10477cd0, 0x104777c0). Every one is called by QSORT4X.CPP's sort through the pointer
// glue with the addresses of two elements and answers a three-way result in r3, the only register the sort reads
// afterwards (qsort4x.cpp `precedes`); what else they leave in r4-r7 is kept as the original has it, because
// the checker compares r4-r10 at return. The loads happen in the original's order, and tie behaviour is the
// original's (the sort's call order is the original's, so the comparators decide where equal elements end).
#include "comp_util_detail.hpp"

namespace cm::aot::comp_util {
namespace {
// A 32-bit wrapping difference, as the PowerPC's `subf`.
std::int32_t difference(std::uint32_t minuend, std::uint32_t subtrahend) { return std::int32_t(minuend - subtrahend); }
}

std::int32_t compare_score_descending(Cpu& c, std::uint32_t first, std::uint32_t second) {
    const auto a = c.f64(first + scored::score);
    const auto b = c.f64(second + scored::score);
    c.set_f(1, a);
    c.set_f(0, b);
    // fcmpo sets FL/FG/FE and cr0 (the checker compares FPSCR); the sort wants a > b first, a < b last.
    Fpu(c).fcmp(0, a, b);
    if (a > b) return -1;
    return a < b ? 1 : 0;  // unordered operands compare equal
}

std::int32_t compare_club_reputation(Cpu& c, std::uint32_t first_slot, std::uint32_t second_slot) {
    const auto first = c.u32(first_slot);
    const auto second = c.u32(second_slot);
    const std::int32_t first_reputation = c.i16(first + club::reputation);
    const std::int32_t second_reputation = c.i16(second + club::reputation);
    c.set({{4, std::uint32_t(first_reputation)}, {5, first}});
    return second_reputation - first_reputation;
}

std::int32_t compare_tag_ascending(Cpu& c, std::uint32_t first, std::uint32_t second) {
    const auto second_tag = c.u8(second + tagged::tag);
    const auto first_tag = c.u8(first + tagged::tag);
    c.set(4, second_tag);
    return std::int32_t(std::int8_t(first_tag)) - std::int32_t(std::int8_t(second_tag));
}

std::int32_t compare_club_reputation_nulls_last(Cpu& c, std::uint32_t first_slot, std::uint32_t second_slot) {
    const auto first = c.u32(first_slot);
    const auto second = c.u32(second_slot);
    c.set(4, second);
    if (first == 0 && second == 0) return 0;
    if (first == 0) return 1;
    if (second == 0) return -1;
    const std::int32_t first_reputation = c.i16(first + club::reputation);
    const std::int32_t second_reputation = c.i16(second + club::reputation);
    return second_reputation - first_reputation;
}

std::int32_t compare_club_reputation_ascending(Cpu& c, std::uint32_t first_slot, std::uint32_t second_slot) {
    const auto first = c.u32(first_slot);
    const auto second = c.u32(second_slot);
    c.set(5, first);
    if (first == 0 && second == 0) return 0;
    if (first == 0) return 1;
    if (second == 0) return -1;
    const std::int32_t second_reputation = c.i16(second + club::reputation);
    const std::int32_t first_reputation = c.i16(first + club::reputation);
    return first_reputation - second_reputation;
}

std::int32_t compare_club_division(Cpu& c, std::uint32_t first_slot, std::uint32_t second_slot) {
    const auto first = c.u32(first_slot);
    const auto second = c.u32(second_slot);
    c.set({{6, first}, {7, second}});
    const auto first_record = c.u32(first + club::division_record);
    c.set(4, first_record);
    if (first_record == 0) {
        // The original loads the second club's record here only to test it.
        return c.u32(second + club::division_record) == 0 ? 0 : 1;
    }
    const auto second_record = c.u32(second + club::division_record);
    c.set(5, second_record);
    if (second_record == 0) return -1;
    const std::int32_t first_reputation = c.i16(first_record + comp::record::reputation);
    const std::int32_t second_reputation = c.i16(second_record + comp::record::reputation);
    if (second_reputation != first_reputation) return second_reputation - first_reputation;
    const auto first_id = c.u32(first_record + comp::record::id);
    const auto second_id = c.u32(second_record + comp::record::id);
    if (second_id != first_id) return difference(second_id, first_id);
    const std::int32_t second_tiebreak = c.i8(second + club::tiebreak);
    const std::int32_t first_tiebreak = c.i8(first + club::tiebreak);
    return first_tiebreak - second_tiebreak;
}

std::int32_t compare_screen_rows(Cpu& c, std::uint32_t first, std::uint32_t second) {
    Frame frame(c, 0x104777c0, 0x40, {});  // only LR is saved: the original's prologue still writes the caller's +8
    // The keys in order; r5 keeps the second row's value of the key that decided.
    const std::int32_t second_group = c.i8(second + screen_row::group);
    const std::int32_t first_group = c.i8(first + screen_row::group);
    c.set(5, std::uint32_t(second_group));
    if (first_group != second_group) return first_group - second_group;
    for (const auto key : {screen_row::key_32, screen_row::key_34, screen_row::key_30}) {
        const std::int32_t other = c.i16(second + key);
        const std::int32_t own = c.i16(first + key);
        c.set(5, std::uint32_t(other));
        if (own != other) return own - other;
    }
    // Equal keys: the clubs' names, through UTILS (r5 1: exact case, r6 0: no length limit). The original's
    // `neg/or/srawi/and` after the call leaves a nonzero result as it is.
    c.set({{3, c.u32(first + screen_row::club) + club::name}, {4, c.u32(second + screen_row::club) + club::name}, {5, 1}, {6, 0}});
    c.call(0x107465f0, 0x10477850);
    return std::int32_t(c.r(3));
}
}
