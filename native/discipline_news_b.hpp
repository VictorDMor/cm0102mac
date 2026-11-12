#pragma once
// DISCIPLINE.CPP news block B: the text of a ban story (news message type 2003), native-first.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::discipline {
// 0x100e59b0: renders message type 2003 into the text object `text`. `owner` is the manager object 0x100e7490 takes,
// `message` the news record (its fields come from 0x102cb570 by index: 0 and 12..., see the .cpp), `body_flag` a
// signed byte: 0 builds the headline (the person's name linked as a person), anything else the body text (a
// format picked by the ban's kind, whether the two clubs' ratings compare, the ban length, whether the ban names a
// date) followed by the person's, the two clubs' and the fixture's links. A message whose field 12 is not 0 is
// another story: 0x100e0b70 renders it. For a ban length below 0 the original reads r28, which its only caller
// (NEWS 0x102be040) holds as the clubs table's TOC cell address: the routine takes that value itself.
void render_ban_story(guest::Cpu& c, std::uint32_t owner, std::uint32_t message, std::uint32_t body_flag,
                      std::uint32_t text);
}
