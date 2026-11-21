// COMP_UTIL.CPP, block `comp_screens_list`: 0x10477490, which orders the rows of a COMP_SCREENS list for the
// screens. The list is a utility object of NETWORK.CPP (a count at +0, the first row's address at +4, a cursor
// at +0xc) of 0x50-byte rows. The routine copies the rows out through the object's iterator (0x10738e00), sorts
// the copy with the screen-row comparator (0x104777c0, through its transition vector) by QSORT4X, empties the
// object (0x10738ca0) and adds the sorted rows back one by one (0x10738d10), then frees the copy. Three
// preconditions report the game's assertion (a missing list: line 0x2781, no rows: 0x278c, no memory: 0x2794)
// and answer 0 with the list as it was.
//
// Each callee is the original's, called with the arguments and the non-volatile registers it sees there (r26
// the row offset and r29 the row number in the last loop, r28 the copy, r27 the list, r30 the count).
#include "comp_util_detail.hpp"

namespace cm::aot::comp_util {
namespace {
// The utility object.
namespace list {
constexpr std::uint32_t count = 0x00 /* halfword */, first_row = 0x04, cursor = 0x0c;
}
constexpr std::uint32_t iterate = 0x10738e00, clear = 0x10738ca0, append_row = 0x10738d10, sort = 0x10768300;
struct Site { std::uint32_t at; std::int32_t name, extension; std::uint32_t line; };
void report(Cpu& c, const Frame& frame, const Site& site) {
    const auto pool = c.toc(toc::pool);
    guest::report_assertion(c, {site.at, pool, pool + 0xe, pool + 0x1b, frame.at(site.name), frame.at(site.extension), site.line});
}
}

bool sort_screen_list(Cpu& c, std::uint32_t object) {
    Frame frame(c, 0x10477490, 0xd0, {.first_gpr = 26});
    c.set({{27, object}, {26, c.toc(toc::success_flag)}, {31, c.toc(toc::message_buffer)}, {29, c.toc(toc::pool)}});
    if (object == 0) {
        report(c, frame, {0x104774c8, 0x90, 0x48, 0x2781});
        return false;
    }
    const std::int32_t rows = c.i16(object + list::count);
    c.set(30, std::uint32_t(rows));
    if (rows == 0) {
        report(c, frame, {0x1047752c, 0x70, 0x40, 0x278c});
        return false;
    }
    const auto copy = invoke(c, callee::allocate, 0x10477578, {std::uint32_t(rows) * screen_row::size});
    c.set(28, copy);
    if (copy == 0) {
        report(c, frame, {0x10477594, 0x50, 0x38, 0x2794});
        return false;
    }
    c.put32(object + list::cursor, c.u32(object + list::first_row));
    c.set({{29, 0}, {31, 0}});
    // Copy the rows out in the object's order.
    for (std::int32_t row = 0; row < rows; ++row) {
        const auto source = invoke(c, iterate, 0x104775f4, {object});
        const auto target = copy + std::uint32_t(row) * screen_row::size;
        c.set(4, target);
        for (std::uint32_t at = 0; at < screen_row::size; at += 4) c.put32(target + at, c.u32(source + at));
        c.set({{31, std::uint32_t(row + 1) * screen_row::size}, {29, std::uint32_t(row + 1)}});
    }
    invoke(c, sort, 0x10477764, {copy, std::uint32_t(rows), screen_row::size, c.toc(toc::screens_vector)});
    invoke(c, clear, 0x1047776c, {object});
    // Add the sorted rows back; r26 counts the bytes, r29 the rows.
    for (std::int32_t row = 0; row < rows; ++row) {
        c.set({{29, std::uint32_t(row)}, {26, std::uint32_t(row) * screen_row::size}});
        invoke(c, append_row, 0x10477788, {object, copy + std::uint32_t(row) * screen_row::size, screen_row::size});
    }
    c.set({{29, std::uint32_t(rows)}, {26, std::uint32_t(rows) * screen_row::size}});
    invoke(c, callee::release, 0x104777a0, {copy});
    return true;
}
}
