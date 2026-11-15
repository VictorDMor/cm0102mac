// FINANCE.CPP: the accounts in the saved game's file (0x100f9b00, 0x100f9c50) and the days-since question
// (0x100f9d80).
//
// The file holds the accounts array as one block of count * 0x168 bytes, opened and read or written
// through the game's file layer (0x107475b0 opens the file named "finance.dat" of the saved game's folder,
// 0x10747a80 reads, 0x10747cc0 writes, 0x10747900 closes). A failed open, or a read or write of another
// size than asked, reports the assertion and clears the success flag.
#include "finance_setup.hpp"

namespace cm::aot::finance {
using guest::Frame;

namespace {
constexpr std::uint32_t open_file = 0x107475b0, read_block = 0x10747a80, write_block = 0x10747cc0, close_file = 0x10747900;
constexpr std::uint32_t file_name = 0x156;  // "finance.dat" in the string pool
constexpr std::int32_t success_cell = toc::success;
constexpr CheckSite read_open_failed{0x100f9b6c, 0x6c, 0x44, 0x10f4}, read_short{0x100f9be0, 0x4c, 0x3c, 0x10fc},
                    no_such_item{0x100f9dd0, 0x40, 0x38, 0x1142},
                    write_open_failed{0x100f9cb4, 0x6c, 0x44, 0x111b}, write_short{0x100f9d24, 0x4c, 0x3c, 0x1123};

// The part of both routines before and after the transfer. `mode` is 0 to read and 1 to write.
bool transfer(Cpu& c, const Frame& frame, std::uint32_t manager, std::uint32_t file, std::uint32_t mode,
              std::uint32_t transfer_entry, std::uint32_t open_back, std::uint32_t transfer_back,
              std::uint32_t close_back, const CheckSite& open_failed, const CheckSite& short_transfer) {
    const auto count_cell = c.toc(toc::club_count);
    const auto pool = c.toc(toc::pool);
    const auto handle = invoke(c, open_file, open_back, {file, pool + file_name, mode, 0x16, 4});
    c.put32(frame.at(0x38), handle);
    if (handle == 0) {
        report_check(c, frame, open_failed);
        return false;
    }
    const auto moved = invoke(c, transfer_entry, transfer_back, {c.u32(manager + manager::accounts), account::size, c.u32(count_cell), handle});
    if (std::int32_t(c.u32(count_cell)) == std::int32_t(moved)) {
        invoke(c, close_file, close_back, {frame.at(0x38)});
        return true;
    }
    report_check(c, frame, short_transfer);
    return false;
}
}

bool read_accounts(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c, 0x100f9b00, 0xb0, {.first_gpr = 27});
    c.set({{27, manager}, {28, c.toc(toc::club_count)}, {29, c.toc(success_cell)}, {30, c.toc(toc::message)},
           {31, c.toc(toc::pool)}});
    if (file == 0) return false;
    return transfer(c, frame, manager, file, 0, read_block, 0x100f9b4c, 0x100f9bc0, 0x100f9c2c, read_open_failed, read_short);
}

void write_accounts(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c, 0x100f9c50, 0xb0, {.first_gpr = 27});
    c.set({{27, manager}, {28, c.toc(toc::club_count)}, {29, c.toc(success_cell)}, {30, c.toc(toc::message)},
           {31, c.toc(toc::pool)}});
    if (file == 0) return;
    transfer(c, frame, manager, file, 1, write_block, 0x100f9c94, 0x100f9d04, 0x100f9d6c, write_open_failed, write_short);
}

std::uint32_t days_since_recorded(Cpu& c, std::uint32_t item, std::uint32_t year_reached) {
    Frame frame(c, 0x100f9d80, 0x80, {.first_gpr = 26});
    const auto rows = c.toc(-0x7b44), today = c.toc(toc::today), pool = c.toc(toc::pool);
    c.set({{27, c.toc(toc::message)}, {28, year_reached}, {29, pool}, {30, rows}, {31, today}});
    c.put8(year_reached, 0);
    const auto index = std::int8_t(invoke(c, 0x10191fd0, 0x100f9db4, {item}));
    if (index < 0) {
        report_check(c, frame, no_such_item);
        return 0;
    }
    const auto row = rows + std::uint32_t(std::int32_t(index) * 0x4c);
    c.set(29, std::uint32_t(std::int32_t(index) * 0x4c));
    // The row's own end year, or the one after this year's; `year_reached` is set once today is in it.
    const std::uint32_t last_year = c.i8(row + 0x18) != 0 ? 0x7d2u : std::uint32_t(std::uint16_t(c.u16(c.toc(-0x7a9c) + 2) + 1u));
    if (c.u16(today + 2) >= last_year) c.put8(year_reached, 1);
    const std::uint32_t kind = c.u8(row + 0x36), start_year = c.u16(row + 0x2e), this_year_at = today + 2;
    c.set({{26, start_year}, {27, this_year_at}, {28, kind}});
    const auto raw = invoke(c, 0x10718360, 0x100f9e78, {today});
    const std::int32_t band = std::int8_t(kind), gap = std::int8_t(raw);
    const auto days = [&](std::uint32_t back) {
        const auto days_since = std::int32_t(invoke(c, 0x10718130, back, {row + 0x2c, c.u32(today), c.u32(today + 4)}));
        return days_since > 0 ? std::uint32_t(days_since) : 0u;
    };
    if (band < 2) {
        if (start_year == c.u16(this_year_at)) {
            if (gap <= 12 - band) return 0;
            return days(0x100f9ee0);
        }
        if (gap >= band) return 0;
        return days(0x100f9eb0);
    }
    const auto distance = std::int8_t(kind - raw);
    if (distance > 2 || distance < 0) return 0;
    return days(0x100f9f18);
}
}
