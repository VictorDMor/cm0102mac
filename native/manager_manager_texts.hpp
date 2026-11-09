#pragma once
// MANAGER_MANAGER.CPP manager texts block: three text builders the message dispatcher at 0x102c5900 calls.
#include "guest_abi.hpp"
#include <cstdint>

namespace cm::aot::manager_manager {
// All three take the module object (r3), a message record (r4: its fields come from 0x102cb570 by index), a
// flag byte (r5) and the text sink the news line goes to (r6, 0x1074b830 and the posts 0x1074bab0). The caller
// (0x102c5914, 0x102c593c, 0x102c5af0) ignores r3 afterwards. The meanings below are inferred from the code.

// 0x101b76b0: the text of a message about a club (field 0 is the club, field 1 a halfword of flags). Flag clear:
// the text depends on the club's place against the number of clubs and the flags; flag set: the text is the
// module's own (0x1019d320). The text goes to the sink, then the club is posted (kind 0x7d5).
void post_text(guest::Cpu& c, std::uint32_t object, std::uint32_t message, std::uint32_t flag, std::uint32_t sink);

// 0x101b7960: the text of a message about a club and the people it names. A 0x3c-byte list of person ids is
// copied from the unit's table (TOC -0x5404) to the frame. Flag clear: the club's own text by the club flag;
// flag set: the module's texts (0x101980a0, 0x1019c590, which fills the ids and answers how many). The names
// of the ids found are appended and posted (kind 0x7db), then the club (kind 0x7d5).
void post_people_text(guest::Cpu& c, std::uint32_t object, std::uint32_t message, std::uint32_t flag, std::uint32_t sink);

// 0x101b5370: the text of a message about a person and up to two clubs. The flag picks one of two field layouts
// (flag clear: person, three words, a club and a kind; flag set: person, three words, two clubs, the kind and a
// level), a text for the person (by name or by the three words), and a sentence by the kind (1, 2 or 3; none for the others) and the
// clubs. The sink gets the text, the person (kinds 0x7da, 0x7dc, 0x7d7) and each club (kind 0x7d5).
void appointment_text(guest::Cpu& c, std::uint32_t object, std::uint32_t message, std::uint32_t flag, std::uint32_t sink);
}
