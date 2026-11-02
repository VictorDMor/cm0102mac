#pragma once
// The press's board and fans reactions about staff (media.cpp, block text_b).
#include "media.hpp"

namespace cm::aot::media {
// The story text for the board's or the fans' reaction to a staff member's (or the manager's) behaviour:
// `owner` is the caller's object (passed on to the story-kind tests), `record` the story record, `full`
// whether the long version is wanted, `output` the text object that takes the story.
void staff_reaction_text(guest::Cpu&, std::uint32_t owner, std::uint32_t record, std::uint32_t full, std::uint32_t output);
}
