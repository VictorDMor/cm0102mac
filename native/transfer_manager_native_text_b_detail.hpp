#pragma once
// TRANSFER_MANAGER.CPP native-first block "text_b": what the three news-message texts share (private to the block).
#include "guest_abi.hpp"
#include "transfer_manager_native_detail.hpp"
#include <cstdint>

namespace cm::aot::transfer_manager_native::text_b {
using guest::Cpu;

// Callees (NEWS.CPP, DATABASE.CPP, LANGLIB.CPP and the output buffer routines).
inline constexpr std::uint32_t news_field_fn = 0x102cb570;     // (item, slot) -> the item's field
inline constexpr std::uint32_t date_text_fn = 0x100cb590;      // (day, month, year, buffer, size, style, person) writes a date
inline constexpr std::uint32_t date_known_fn = 0x100cbde0;     // (person record, day, month, year, 0) -> byte
inline constexpr std::uint32_t format_fn = 0x10727890;         // (buffer, format, values...): sprintf
inline constexpr std::uint32_t begin_text_fn = 0x1074b830;     // (output, text) starts the message text
inline constexpr std::uint32_t link_fn = 0x1074bab0;           // (output, text, kind, record) adds a link to the message
inline constexpr std::uint32_t paragraph_fn = 0x1074b960;      // (output, text) adds a paragraph
inline constexpr std::uint32_t link_club = 0x7d5, link_date = 0x7d7;

// The original's cells (Cpu::toc).
inline constexpr std::int32_t text_buffer_cell = -0x7bb8,  // the shared text buffer
                              text_base_cell = -0x4b9c,    // base of the format texts
                              session_cell = -0x7bb0;      // the game session (current player slot at +0x2c58)
inline constexpr std::uint32_t club_name = 0x221, club_name_length = 0x23b;
inline constexpr std::uint32_t date_buffer_size = 300;

inline std::int32_t news_field(Cpu& c, std::uint32_t item, std::uint32_t slot, std::uint32_t back) {
    return std::int32_t(invoke(c, news_field_fn, back, {item, slot}));
}
inline std::uint32_t club_at(Cpu& c, std::int32_t index) {
    return c.u32(c.toc(toc::clubs)) + std::uint32_t(index) * club_size;
}
inline std::uint32_t person_at(Cpu& c, std::int32_t index) {
    return c.u32(c.toc(toc::people)) + std::uint32_t(index) * person_size;
}
inline std::int32_t club_name_len(Cpu& c, std::uint32_t club) { return std::int32_t(c.i8(club + club_name_length)); }
}
