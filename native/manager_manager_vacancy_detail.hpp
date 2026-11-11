#pragma once
// Pieces the vacancy block shares: TOC cells and callees outside the block.
#include "manager_manager_detail.hpp"
#include <cstdint>

namespace cm::aot::manager_manager::vacancy {
namespace toc_cell {
constexpr std::int32_t date = -0x7b80, news_desk = -0x7bf0, today_copy = -0x7a9c, pool = -0x5368,
                       text_buffer = -0x7bb8, people = -0x7be0, clubs = -0x7be8, rows = -0x7b98;
}
namespace callee {
constexpr std::uint32_t file_message = 0x102cc140, file_message_c = 0x102cc400;
}
}
