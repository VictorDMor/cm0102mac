#pragma once
// Pieces the vacancy block shares: TOC cells, callees outside the block, and the news message buffer.
#include "manager_manager_detail.hpp"
#include <cstdint>

namespace cm::aot::manager_manager::vacancy {
namespace toc_cell {
constexpr std::int32_t date = -0x7b80, news_desk = -0x7bf0, today_copy = -0x7a9c, pool = -0x5368,
                       text_buffer = -0x7bb8, people = -0x7be0, clubs = -0x7be8, rows = -0x7b98;
}
namespace callee {
constexpr std::uint32_t clear_message = 0x102bdf90, set_field = 0x102cb4b0, file_message = 0x102cc140,
                        file_message_c = 0x102cc400;
}
inline std::uint32_t sext(std::int8_t value) { return std::uint32_t(std::int32_t(value)); }  // extsb

// Fills field `index` of the message buffer at `buffer` (0x102cb4b0); `back` is the address after the `bl`.
inline void set_field(guest::Cpu& c, std::uint32_t buffer, std::uint32_t index, std::uint32_t value, std::uint32_t back) {
    c.set({{3, buffer}, {4, index}, {5, value}});
    c.call(callee::set_field, back);
}
inline void clear_message(guest::Cpu& c, std::uint32_t buffer, std::uint32_t kind, std::uint32_t back) {
    c.set({{3, buffer}, {4, kind}, {5, 0}});
    c.call(callee::clear_message, back);
}
}
