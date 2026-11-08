#pragma once
// MANAGER_MANAGER.CPP post-match manager update (0x1019e8f0, 0x1019e990, 0x101c15f0): the record
// fields they read. Offsets are the original's; meanings are named as far as the code shows them.
#include <cstdint>

namespace cm::aot::manager_manager::update_detail {
// The module object (TOC -0x79f0): `table` points at the board's rows (0x4c bytes per club, by the
// club's id); `pending` is the list at +0x10 of clubs waiting for a departure decision.
namespace object {
constexpr std::uint32_t table = 0x0, pending = 0x10;
}
// The board's row of a club: the value and three figures the results move and clamp to 1..10000,
// `trust`, which only drops (unclamped), and a flag halfword.
namespace row {
constexpr std::uint32_t size = 0x4c, value = 0x6, figure_b = 0x8, figure_a = 0xa, figure_c = 0xc, flags = 0xe,
                        trust = 0x16;
constexpr std::int32_t figure_max = 0x2710, figure_min = 1;
constexpr std::uint32_t flag_cup_run = 0x4, flag_closed = 0x400;  // bits of the halfword at +0xe
}
// The club record (0x248 bytes).
namespace club {
constexpr std::uint32_t id = 0x0, tie_link = 0x14, region = 0x2c, league = 0x30, group = 0x3c, rival_a = 0x74,
                        rival_b = 0x78, rival_c = 0x7c, manager = 0x90, reputation = 0x1ec;
}
namespace person {
constexpr std::uint32_t career = 0x14, home_region = 0xc, size = 0x70;
}
namespace manager {
constexpr std::uint32_t staff = 0x1c, edge = 0x69;
}
namespace staff {
constexpr std::uint32_t trait = 0x3d;
}
// The fixture (0x6c bytes of a competition's list): the two clubs, the competition and the
// recorded result. The result bytes are signed; -1 means none.
namespace fixture {
constexpr std::uint32_t id = 0x0, competition = 0x14, venue = 0x18, home = 0x1c, away = 0x20, round = 0x30,
                        number = 0x32, flags = 0x38, kind = 0x3a, played = 0x40, goals_home = 0x43,
                        goals_away = 0x44, total_home = 0x47, total_away = 0x48, leg_home = 0x49, leg_away = 0x4a,
                        penalties_home = 0x4b, penalties_away = 0x4c;
constexpr std::uint32_t league_kind = 8, cup_kind = 5;
// Bits of the halfword at +0x38.
constexpr std::uint32_t flag_closed = 0x10, flag_hidden = 0x800, flag_friendly = 0x2000, flag_replay = 0x4;
}
// The competition (0x6c bytes) as the update reads it: its level (a halfword).
namespace competition {
constexpr std::uint32_t level = 0x18;
}
// The fixture's venue (+0x18) and the club's group (+0x3c) are the same kind of record: its
// leader at +0x10, whose region (+0x18) the standing compares with the clubs', and whose two
// coordinates (doubles at +0, +8) give the rivals' distance.
namespace venue {
constexpr std::uint32_t leader = 0x10;
}
namespace leader {
constexpr std::uint32_t region = 0x18, x = 0x0, y = 0x8;
}
// The league entry (the leagues table at TOC -0x7b5c, by a club's link) and its owner at +8.
namespace league_entry {
constexpr std::uint32_t owner = 0x8;
}
// The results record 0x10459b50 returns: the length of the current run and whether it is live.
namespace results {
constexpr std::uint32_t streak = 0x75, live = 0x77;
}
// A club's row of the draw table at TOC -0x7984 (8 bytes by id): a signed mark at +4.
namespace draw_row {
constexpr std::uint32_t shift = 3, mark = 0x4;
}
// The match record: two sides 0x664 apart, each with 20 slots of 0x4c bytes at +0x194, a
// strength pair at +0x128 of the side, and a flag byte and a spare halfword of the whole.
namespace match {
constexpr std::uint32_t side_size = 0x664, slot_size = 0x4c, slot_count = 20, person = 0x194, strength_a = 0x128,
                        strength_b = 0x129, neutral = 0x4c, spare = 0x4e;
// Slot bytes (from the slot's start): three marks and a weight.
constexpr std::uint32_t mark_other = 0x1c4, mark_low = 0x1c6, mark_high = 0x1c8, weight = 0x1c9;
}
// The outcome flags the update builds in r21 (a halfword) and reads back through r14.
namespace outcome {
constexpr std::uint32_t margin_a = 0x1, margin_b = 0x2, margin_c = 0x4, margin_d = 0x8, margin_e = 0x10,
                        margin_f = 0x20, near_rival = 0x40, rival = 0x80, tie_lost = 0x100, tie_won = 0x200,
                        drawn = 0x400, won = 0x800, lost = 0x1000;
}
// The int-to-double idiom's high word: the compiler builds a double from
// 0x43300000 | (value ^ 0x80000000) on the stack and subtracts the bias (pool +0x38).
constexpr std::uint32_t conversion_high = 0x43300000;
// The module's pool (TOC -0x536c): doubles, named by what this routine uses them for. Two rules
// share a slot: figure_c_gain is the outsider slot (0x60), figure_a_gain the tier_b one (0x20).
namespace pool {
constexpr std::uint32_t tier_a = 0x10, tier_b = 0x20, tier_c = 0x28, bias = 0x38, base = 0x48, one = 0x50,
                        no_competition = 0x58, outsider = 0x60, cup_home = 0x68, cup_away = 0x70,
                        league_intercept = 0x78, league_slope = 0x80, level_gap = 0x88, round_factor = 0x90,
                        strong = 0x98, unplayed = 0xa8, strength_a = 0xb0, strength_b = 0xb8, drift = 0xc0,
                        score_scale = 0xc8, score_offset = 0xd0, distance_scale = 0xd8, distance_limit = 0xe0,
                        near_rival = 0xe8, value_gain = 0xf0, figure_b_gain = 0xf8, figure_c_gain = 0x60,
                        figure_a_gain = 0x20;
}
}
