#pragma once
#include "guest_abi.hpp"

namespace cm::aot::holiday_article {
std::uint32_t report_id(guest::Cpu&, std::uint32_t item);
bool publish(guest::Cpu&, std::uint32_t manager, std::uint32_t id);
// The standard renderer has already cleared the text object here.
bool render(guest::Cpu&, std::uint32_t item, std::uint32_t output, bool detail);
bool prepare_page(guest::Cpu&, std::uint32_t item, std::uint32_t output,
                  std::uint32_t screens, unsigned toolbar_y, unsigned text_width);
bool change_page(guest::Cpu&, std::uint32_t item, int action, std::uint32_t screens);
}
