#pragma once
// TRANSFER_MANAGER.CPP native-first block "text_e": what its five news-message texts share (private to the block).
#include "transfer_manager_native_text_b_detail.hpp"
#include <initializer_list>
#include <optional>

namespace cm::aot::transfer_manager_native::text_e {
using namespace text_b;
using guest::Cpu;

inline constexpr std::uint32_t cash_value_fn = 0x10009f20, cash_text_fn = 0x1000b410, person_name_fn = 0x100caeb0;
inline constexpr std::uint32_t stack_argument = 0x38;

// The formatter (sprintf) with the original's argument layout: destination, format, then the values:
// the first six in r5-r10, the rest in the caller's parameter area from sp+0x38. A register the original
// does not set at that call keeps what the previous callee left, so the value list may stop short.
inline void say(Cpu& c, const guest::Frame& frame, std::uint32_t back, std::uint32_t destination, std::uint32_t format,
                std::initializer_list<std::uint32_t> values) {
    unsigned n = 5, stacked = 0;
    for (const auto value : values) {
        if (n <= 10) c.set(n++, value);
        else c.put32(frame.at(stack_argument + 4 * stacked++), value);
    }
    c.set(3, destination);
    c.set(4, format);
    c.call(format_fn, back);
}
// The club's name length byte, sign-extended as the formatter takes it, and its name's address.
inline std::uint32_t name_length(Cpu& c, std::uint32_t club) { return std::uint32_t(club_name_len(c, club)); }
inline std::uint32_t name_of(std::uint32_t club) { return club + club_name; }
// An amount written as cash text: the amount goes to `word`, the text to `buffer` (the original's two calls).
inline void cash_amount_text(Cpu& c, std::uint32_t word, std::uint32_t buffer, std::uint32_t size, std::uint32_t amount,
                             std::uint32_t back_value, std::uint32_t back_text) {
    c.put32(word, amount);
    invoke(c, cash_value_fn, back_value, {word, 6});
    invoke(c, cash_text_fn, back_text, {buffer, size, c.r(3), c.r(4), 6});
}
}
