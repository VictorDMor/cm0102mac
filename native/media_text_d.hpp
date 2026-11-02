#pragma once
// The press story writer (media.cpp, block text_d).
#include "media.hpp"

namespace cm::aot::media {
// Writes the story that the log record describes into `output`: `full` selects the long version (body, the
// manager's quote and the links) over the short one (a headline over the club).
void press_feature_text(guest::Cpu&, std::uint32_t record, std::uint32_t full, std::uint32_t output);
}
