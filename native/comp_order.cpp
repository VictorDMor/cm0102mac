// COMP.CPP: the two sort orders whose vectors ARG_PRM.CPP and ARG_SECOND.CPP pass to the sort (TOC -0x6060
// and -0x6058). Both put larger values first and break ties by the row's id, ascending (the word difference,
// wrapping, as `subf` leaves it).
#include "comp_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::comp {

std::int32_t compare_by_value(Cpu& c, std::uint32_t left, std::uint32_t right) {
    // lfs, lfs, fcmpo cr0: FPSCR's FL/FG/FE as the original leaves them. An unordered pair falls through
    // both branches (ble, bge) to the ids.
    guest::Fpu fpu(c);
    const double a = c.f32(left + 4), b = c.f32(right + 4);
    fpu.fcmp(0, a, b);
    if (a > b) return -1;
    if (a < b) return 1;
    return std::int32_t(c.u32(left) - c.u32(right));
}

std::int32_t compare_by_column(Cpu& c, std::uint32_t left, std::uint32_t right) {
    // The column is a signed byte the sort's caller sets; outside 0..3 every pair is equal.
    const std::int32_t column = c.i8(c.toc(toc::sort_column));
    if (column < 0 || column > 3) return 0;
    const auto at = 0xa + std::uint32_t(column) * 4;
    const auto a = c.i16(left + at), b = c.i16(right + at);
    if (a > b) return -1;
    if (a < b) return 1;
    return std::int32_t(c.u32(left) - c.u32(right));
}

}
