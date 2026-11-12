#pragma once
// DISCIPLINE.CPP news block: the ban story of news message type 2002 (NEWS.CPP renders it through 0x100e0b70).
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::discipline {
// 0x100e0b70: renders the ban story of a news `message` (the record post_ban_news builds, read field by field
// with 0x102cb570 and the two aggregate decoders) into the text object `text`: the headline when `body` is 0
// (the person's name, the length of the ban and, for a ban by days, its date) or the body text (the name, the
// bodies that ban, the fixture the person was sent off in and its result, then the ban's length and end date), then
// the links of the person, the clubs and the fixture. `owner` is the discipline manager (0x100e7490 takes it).
// Which of ~150 pool texts is formatted depends on the length of the ban (a date, one match, several), the ban's
// kind, whether the fixture's competition is named, the two clubs' scores, the flag of the message's last fields
// and the end date; each is one row of a table in discipline_news.cpp.
void ban_news_text(guest::Cpu& c, std::uint32_t owner, std::uint32_t message, std::int8_t body, std::uint32_t text);
}
