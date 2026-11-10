#pragma once
// MANAGER_MANAGER.CPP weekly pass: the record fields it reads. Offsets are the original's; meanings
// are named as far as the code shows them (an offset name alone where only the use is known).
#include <cstdint>

namespace cm::aot::manager_manager::weekly_detail {
namespace toc {
constexpr std::int32_t clubs = -0x7be8, club_count = -0x7b50, people = -0x7be0, date = -0x7b80, leagues = -0x7b5c,
                       object_a = -0x7a64, game = -0x7b94, news = -0x5370, pool = -0x536c;
}
// The frame's slots (sp+...): the loop's, and the two flags that stop a kind of event for the week.
namespace slot {
constexpr std::int32_t club_pointer = 0xec, club_index = 0xe0, hired_flag = 0xdd, other_flag = 0xdc;
// Conversion words (high 0x43300000, low value ^ 0x80000000) and the fctiwz store.
constexpr std::int32_t conv_a = 0xcc, conv_b = 0xd4, conv_c = 0xc4, stage_half = 0xe8, record = 0xe4;
// The date the first test builds and the dates it compares.
constexpr std::int32_t date_in = 0x74, date_out = 0x7c, date_copy = 0x54;
constexpr std::int32_t date_in_b = 0x64, date_out_b = 0x6c, date_copy_b = 0x4c;
}
constexpr std::uint32_t high_word = 0x43300000;
// The club (0x248 bytes). The record at +0x90 is its manager's (inferred), +0x2c its region and
// +0x30 its league's.
namespace club {
constexpr std::uint32_t region = 0x2c, league = 0x30, manager = 0x90, reputation = 0x1ec;
}
namespace region {
constexpr std::uint32_t monthly = 0x74;
}
// The board's row of a club (0x4c bytes): three halfwords the week moves.
namespace row {
constexpr std::uint32_t value = 0x6, target = 0x8, spread = 0x16;
}
// The original's labels a section can end at: where the club's body goes on.
namespace label {
constexpr std::uint32_t club_end = 0x101a8a78, own_end = 0x101a7d28, hub = 0x101a3ad4, rows_end = 0x101a7864, quiet = 0x101a7840, wide = 0x101a6958;
}
}
