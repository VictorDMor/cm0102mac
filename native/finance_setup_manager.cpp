// FINANCE.CPP: the finance manager's constructor (0x100f84e0).
//
// It waits for the game CD: the name of the game folder is built from two strings of the program (every second
// byte of a name made of both), and until the file system finds it the "insert CD" dialog is shown. Then
// the accounts array is allocated with one account per club (0x10773150 runs the element constructor and
// destructor vectors the game keeps in its table of contents), read from the saved game when there is a file, else
// each account is started from its club (0x100f1050).
#include "finance_setup.hpp"
#include "guest_abi.hpp"

namespace cm::aot::finance {
using guest::Frame;

namespace {
constexpr std::int32_t first_name = -0x55c8, second_name = -0x55c4;  // two Pascal strings of the program
constexpr std::uint32_t make_file_spec = 0x1077fee0, copy_text = 0x10727890, show_message = 0x10769b60,
                        allocate = 0x107735f0, construct_array = 0x10773150;
constexpr std::uint32_t insert_title = 0x11f, insert_text = 0x129;  // in the string pool
constexpr CheckSite no_array{0x100f8658, 0x50, 0x38, 0xf13};
}

std::uint32_t create_manager(Cpu& c, std::uint32_t manager, std::uint32_t file) {
    Frame frame(c, 0x100f84e0, 0x390, {.first_gpr = 22});
    const auto count_cell = c.toc(toc::club_count), pool = c.toc(toc::pool), message = c.toc(toc::message);
    const auto first = c.toc(first_name), second = c.toc(second_name);
    // r22 walks the copy of the first name and is not set again until the allocation; r26 is 0 until the file
    // system finds the folder. The other registers hold these words from here to the end.
    c.set({{24, manager}, {25, file}, {26, 0}, {27, count_cell}, {28, first}, {29, second}, {30, message}, {31, pool}});
    for (;;) {
        // Both names are copied to the frame (16 and 17 bytes); the loop merges them, one byte of each in turn,
        // until a zero byte of the second. That name has 16 characters and its zero lies past the copy, so the
        // loop ends on whatever byte the frame holds at +0x81 (read as it is).
        const auto first_copy = frame.at(0x40), second_copy = frame.at(0x70), merged = frame.at(0xcc);
        for (std::uint32_t at = 0; at < 0x10; at += 4) c.put32(second_copy + at, c.u32(second + at));
        c.put8(second_copy + 0x10, c.u8(second + 0x10));
        for (std::uint32_t at = 0; at < 0x10; at += 4) c.put32(first_copy + at, c.u32(first + at));
        std::uint32_t walked = 0;
        for (; std::int8_t(c.u8(second_copy + 1 + walked)) != 0; ++walked) {
            c.put8(merged + 1 + 2 * walked, c.u8(second_copy + 1 + walked));
            c.put8(merged + 2 + 2 * walked, c.u8(first_copy + 1 + walked));
        }
        c.put8(merged, c.u8(second_copy) + c.u8(first_copy));
        c.set(22, first_copy + 1 + walked);
        // The words of the second name are still in r8-r10 and its length in r7 when the file system is asked.
        c.set({{10, c.u32(second)}, {9, c.u32(second + 4)}, {8, c.u32(second + 8)}, {7, c.u8(second_copy)}});
        // The merged name is looked up in the current folder; the file system's answer is 0 when found.
        const auto status = std::int16_t(invoke(c, make_file_spec, 0x100f85b4, {0, 0, merged, frame.at(0x84)}));
        c.set(2, c.u32(frame.at(0x14)));
        if (status == 0) {
            c.set(26, 1);
            break;
        }
        invoke(c, copy_text, 0x100f85d8, {frame.at(0x294), pool + insert_title});
        invoke(c, copy_text, 0x100f85e4, {frame.at(0x1cc), pool + insert_text});
        invoke(c, show_message, 0x100f85f4, {frame.at(0x294), frame.at(0x1cc), 1});
    }
    const auto count = c.u32(count_cell);
    c.set({{26, count}, {22, 0}});
    std::uint32_t accounts = invoke(c, allocate, 0x100f8610, {count * account::size + 0x10});
    c.set(23, accounts);
    if (accounts != 0) {
        c.set(22, 1);
        accounts = invoke(c, construct_array, 0x100f8630,
                          {accounts, c.toc(toc::element_constructor), c.toc(toc::element_destructor), account::size, count});
        c.set(23, accounts);
    }
    c.put32(manager + manager::accounts, accounts);
    if (c.u32(manager + manager::accounts) == 0) {
        report_check(c, frame, no_array);
        c.set(3, manager);
        return manager;
    }
    if (file != 0) {
        c.set({{3, manager}, {4, file}});
        c.link(0x100f86b4);
        if (std::int8_t(read_accounts(c, manager, file)) != 0) return manager;
    }
    const auto clubs = c.u32(c.toc(toc::clubs));
    const auto accounts_base = c.u32(manager + manager::accounts);
    c.set({{23, 0}, {25, clubs}, {26, accounts_base}});
    for (std::int32_t index = 0; index < std::int32_t(c.u32(count_cell)); ++index) {
        const auto account_here = accounts_base + std::uint32_t(index) * account::size;
        const auto club_here = clubs + std::uint32_t(index) * club_size;
        c.set({{23, std::uint32_t(index)}, {25, club_here}, {26, account_here}, {3, account_here}, {4, club_here}});
        c.link(0x100f86e4);
        start_account(c, account_here, club_here);
    }
    return manager;
}
}
