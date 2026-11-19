// CUP.CPP: three sort orders passed to qsort (0x10768300). Each takes two row addresses and returns the
// difference the original's `subf` gives, wrapping at 32 bits; qsort hands the registers a comparator leaves
// back to callers that read them after the sort, so each also leaves r4/r5 as the original does.
#include "cup_base_detail.hpp"

namespace cm::aot::cup_base {
namespace {
constexpr std::uint32_t state = Entrant::state, row_byte_6 = 6;
// The club's halfword +0x1ec (LEAGUE.CPP's comparator 0x104aab00 reads the same field).
constexpr std::uint32_t club_half_1ec = 0x1ec;
std::uint32_t signed_byte(Cpu& c, std::uint32_t at) { return sext8(c.u8(at)); }
std::uint32_t signed_half(Cpu& c, std::uint32_t at) { return sext16(c.u16(at)); }
// The tie-break both entrant orders share (0x10485850.., 0x10485890..): r4 the right club's halfword.
std::int32_t by_club_half(Cpu& c, std::uint32_t left, std::uint32_t right) {
    const auto right_half = signed_half(c, c.u32(right) + club_half_1ec);
    c.set(4, right_half);
    return std::int32_t(signed_half(c, c.u32(left) + club_half_1ec) - right_half);
}
}

std::int32_t compare_by_byte_6(Cpu& c, std::uint32_t left, std::uint32_t right) {
    return std::int32_t(signed_byte(c, right + row_byte_6) - signed_byte(c, left + row_byte_6));
}

std::int32_t compare_by_state_descending(Cpu& c, std::uint32_t left, std::uint32_t right) {
    // r5 the left row's byte (0x10485830..0x10485838).
    const auto left_state = signed_byte(c, left + state), difference = signed_byte(c, right + state) - left_state;
    c.set(5, left_state);
    return difference ? std::int32_t(difference) : by_club_half(c, left, right);
}

std::int32_t compare_by_state_ascending(Cpu& c, std::uint32_t left, std::uint32_t right) {
    // r5 the right row's byte (0x10485870..0x10485878).
    const auto right_state = signed_byte(c, right + state), difference = signed_byte(c, left + state) - right_state;
    c.set(5, right_state);
    return difference ? std::int32_t(difference) : by_club_half(c, left, right);
}

}
