#pragma once
#include "utils.hpp"
#include "guest_values.hpp"

namespace cm::aot::utils::detail {
using namespace guest;
// Table of contents: UTILS.CPP's string pool, its 256-row character table
// (plain, upper, lower), the shared path and ordinal buffers.
inline constexpr std::int32_t strings_slot = -0x3e34, characters_slot = -0x3e3c, path_slot = -0x3e38,
                              ordinal_slot = -0x3e40, message_slot = -0x7ba8;
// Offsets into the string pool.
inline constexpr std::uint32_t assertion_format = 0xa, assertion_title = 0x17, no_text = 0xef;

// Runtime routines of other modules.
inline constexpr std::uint32_t string_length = 0x1077dbd0, string_copy = 0x1077dbf0, string_copy_limited = 0x1077dc10,
    string_append = 0x1077dc60, string_find = 0x1077dda0, string_compare_nocase = 0x107393f0,
    format_text = 0x1077b720, localized_text = 0x10727890, current_language = 0x10727880,
    split_path = 0x10739890, make_path = 0x10739b40, folder_location = 0x107390e0,
    open_file = 0x10739de0, close_file = 0x107766f0, file_position = 0x10777cc0, seek_file = 0x10777f10,
    read_file = 0x10775fe0, write_file = 0x10776360, read_character = 0x10775c10, remove_file = 0x10776690,
    allocate = 0x10775130, release = 0x10775170;

// `bl target`, whose next instruction is `back`, with the arguments in r3 on.
inline std::uint32_t call(Cpu& c, std::uint32_t target, std::uint32_t back,
                          std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned reg = 3;
    for (const auto value : arguments) c.set(reg++, value);
    c.call(target, back);
    return c.r(3);
}
// The same through CFM glue: the caller reloads its TOC from sp+20.
inline std::uint32_t carbon_call(Cpu& c, std::uint32_t target, std::uint32_t back,
                                 std::initializer_list<std::uint32_t> arguments = {}) {
    const auto value = call(c, target, back, arguments);
    c.set(2, c.u32(c.sp() + 0x14));
    return value;
}
// The assertion every UTILS.CPP routine reports for a bad argument.
inline void report(Cpu& c, std::uint32_t at, const Frame& frame, std::int32_t name, std::int32_t extension,
                   std::uint32_t line) {
    const auto strings = c.toc(strings_slot);
    report_assertion(c, {at, strings, strings + assertion_format, strings + assertion_title,
                         frame.at(name), frame.at(extension), line});
}
}
