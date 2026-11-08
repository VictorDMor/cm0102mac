#pragma once
// MANAGER_MANAGER.CPP manager news text (0x101b9b50, 0x101bfd10): the record fields and frame slots they use.
// Offsets are the original's; meanings are named as far as the code shows them.
#include <cstdint>

namespace cm::aot::manager_manager::news_detail {
// TOC slots: the unit's string pool (-0x5368), the leagues table by league id, the cups list holder, the news line
// the posted item is formatted into, and the people table.
namespace toc {
constexpr std::int32_t pool = -0x5368, leagues = -0x7b5c, cups = -0x7b54, news_line = -0x7bb8, people = -0x7be0;
}
namespace callee {
// The formatter: destination, format, then the arguments in r5-r10 and the frame's words from 0x38.
constexpr std::uint32_t formatter = 0x10727890, club_flag = 0x100c7d00, nation_name = 0x10168a00,
                        cup_flag = 0x10494d20, virtual_glue = 0x10781320, person_name = 0x100caeb0, post = 0x1077b720;
}
namespace club {
constexpr std::uint32_t id = 0x0, nation = 0x2c, league = 0x30, name = 0x1ee, name_class = 0x242;
}
namespace competition {
constexpr std::uint32_t name = 0x4d, name_class = 0x67;
}
// The module object: the board's rows (0x4c bytes per club, by id) and the row's flag halfword.
namespace object {
constexpr std::uint32_t table = 0x0;
}
namespace row {
constexpr std::uint32_t size = 0x4c, flags = 0xe;
}
// The league record the club's league (club +0x30, whose id is at +0) selects in the leagues table: a halfword
// at +0x36, a byte at +0xc3 and a virtual method at +0xa4 of its vtable.
namespace league {
constexpr std::uint32_t id = 0x0, level = 0x36, margin = 0xc3, ask = 0xa4;
constexpr std::uint32_t flag_asks = 0x20;  // the row's flag that makes the league's method decide
}
// compose's own frame (0xd70 bytes): the text buffers, the formatter's extra argument words from 0x38.
namespace slot {
constexpr std::uint32_t arguments = 0x38, nation = 0x40, club_text = 0xa4, comment = 0x16c, news_text = 0x748,
                        toc_save = 0x14;
}
// 0x101bfd10's frame (0x2c0 bytes): the buffer the person's name is formatted into.
namespace comment_slot {
constexpr std::uint32_t name = 0x1c8;
}
// The caller's stack arguments, from its sp: two words and five bytes (low bytes of words).
namespace argument {
constexpr std::uint32_t people = 0x38, mode = 0x3c, kind = 0x47, expected = 0x4b, actual = 0x4f, limit = 0x53,
                        commented = 0x57;
}
}
