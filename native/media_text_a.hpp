#pragma once
// The press's long column about a player and the manager's reply to it (or, in the short form, its headline).
#include "media.hpp"

namespace cm::aot::media {
void press_column_story(guest::Cpu&, std::uint32_t self, std::uint32_t item, std::uint32_t full, std::uint32_t output);
}
