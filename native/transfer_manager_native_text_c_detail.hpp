#pragma once
// TRANSFER_MANAGER.CPP native-first block "text_c": what its news-message texts share (private to the block).
#include "transfer_manager_native_text_b_detail.hpp"
#include <initializer_list>

namespace cm::aot::transfer_manager_native::text_c {
using namespace text_b;
using guest::Cpu;

inline constexpr std::uint32_t text_page = 0x10000;
inline constexpr std::uint32_t first_value_register = 5;

// The formatter (sprintf): destination, format and up to six values in r5-r10; a register the
// original does not set at that call keeps what the previous callee left (so is not touched here).
inline void format_text(Cpu& c, std::uint32_t back, std::uint32_t destination, std::uint32_t format,
                        std::initializer_list<std::uint32_t> values = {}) {
    c.set(3, destination);
    c.set(4, format);
    unsigned n = first_value_register;
    for (const auto value : values) c.set(n++, value);
    c.call(format_fn, back);
}
// The club's name length byte as the formatter takes it (sign-extended).
inline std::uint32_t name_length(Cpu& c, std::uint32_t club) { return std::uint32_t(club_name_len(c, club)); }
inline std::uint32_t name_of(std::uint32_t club) { return club + club_name; }
}
