#pragma once
// The press story about a staff member, a match or a former star (media.cpp, block text_c).
#include "media.hpp"

namespace cm::aot::media {
// Writes the story of a log `record` into the text object `output`; `full` asks for the long version (two
// paragraphs) instead of one line. Links for the names in it follow.
void press_story_text(guest::Cpu&, std::uint32_t record, std::uint32_t full, std::uint32_t output);
}
