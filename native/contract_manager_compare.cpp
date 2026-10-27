#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
// The signed 16-bit day comparison in the reference leaves these multiply-
// free bitwise intermediates in volatile registers. The ordering itself is
// expressed on named dates; keep this residue separate from that result.
void leave_day_comparison(Cpu& c, std::int16_t selected, std::int16_t other) {
    const auto bits = extended(selected) ^ extended(other);
    const auto half = std::int32_t(bits) >> 1;
    c.set(5, std::uint32_t(half));
    c.s.wr<1>(guest::reg::xer_ca, std::int32_t(bits) < 0 && (bits & 1));
}
}

std::uint32_t compare_scheduled(Cpu& c, std::uint32_t first, std::uint32_t second) {
    const auto stack = c.sp();
    const auto date_word = c.u32(second + 4), flag_word = c.u32(second + 8);
    c.set(5, date_word);
    c.set(0, flag_word);
    c.put32(stack - 8, date_word);
    const auto first_year = c.u16(first + 6), second_year = c.u16(stack - 6);
    c.set(7, first_year);
    c.set(6, second_year);
    c.put32(stack - 0x18, date_word);
    const auto by_year = compare_logical(c, second_year, first_year);
    c.put32(stack - 0x14, flag_word);
    c.put32(stack - 4, flag_word);
    bool second_after = by_year > 0;
    if (by_year == 0) {
        const auto second_day = c.i16(stack - 8), first_day = c.i16(first + 4);
        c.set(6, extended(second_day));
        leave_day_comparison(c, second_day, first_day);
        second_after = second_day > first_day;
    }
    c.set(0, second_after);
    if (nonzero(c, second_after)) return result(c, 0xffffffff);

    // The original keeps a second local copy. Re-read it rather than
    // caching values across writes: callers can pass aliased stack records.
    const auto saved_date = c.u32(stack - 0x18), saved_flag = c.u32(stack - 0x14);
    c.set(5, saved_date);
    c.set(0, saved_flag);
    c.put32(stack - 0x10, saved_date);
    const auto repeated_year = c.u16(stack - 0xe);
    c.set(5, repeated_year);
    c.put32(stack - 0xc, saved_flag);
    const auto repeated_order = compare_logical(c, repeated_year, first_year);
    bool second_before = repeated_order < 0;
    if (repeated_order == 0) {
        const auto second_day = c.i16(stack - 0x10), first_day = c.i16(first + 4);
        c.set(6, extended(first_day));
        leave_day_comparison(c, first_day, second_day);
        second_before = second_day < first_day;
    }
    c.set(0, second_before);
    if (nonzero(c, second_before)) return result(c, 1);
    const auto second_kind = c.u8(second + 12);
    const auto first_kind = c.i8(first + 12);
    c.set(4, second_kind);
    c.set(0, std::uint32_t(std::int32_t(first_kind)));
    return result(c, std::uint32_t(std::int32_t(first_kind) - std::int32_t(std::int8_t(second_kind))));
}
}
