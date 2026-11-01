#pragma once
// MATCH_ENG.CPP (CM3_MATCH_ENGINE): the match object, its players and the
// routines over them. Offsets are the original's; `Cpu&` keeps what the game can
// observe (guest_abi.hpp), including the argument registers these routines use
// as scratch, since a caller's next varargs call stores them on the stack.
#include "guest_fpu.hpp"
#include "match_eng_fields.hpp"

namespace cm::aot::match_eng {
// `bl entry` to another routine of this module from the C++: runs its hook in place
// (the link register, then the hook), or the original through the dispatcher while
// the routine is not reconstructed. Set its registers first, as the original does.
void call(Cpu&, std::uint32_t entry, std::uint32_t back);
// `bl 0x101d3c90`: the match's random number below `bound` (RNG draw), left in r3.
inline std::uint32_t draw(Cpu& c, std::uint32_t bound, std::uint32_t back) {
    c.set(3, bound);
    call(c, 0x101d3c90, back);
    return c.r(3);
}
// 0x101d5810: a message to the match's target object, `argument` in r6.
inline void post_message(Cpu& c, std::uint32_t match, std::uint32_t id, std::uint32_t argument, std::uint32_t back) {
    c.set({{3, c.u32(match + message_target)}, {4, id}, {5, match}, {6, argument}, {7, 0}});
    call(c, 0x101d5810, back);
}
// fctiwz then stfd to the stack: the low word the original reads back.
inline std::uint32_t truncated(Cpu& c, guest::Fpu& fp, std::uint32_t slot, double value) {
    c.put64(slot, fp.fctiwz(0, value));
    return c.u32(slot + 4);
}

// --- Larger call-free routines (match_eng_queries.cpp) ---
double side_rate(Cpu&, std::uint32_t record, std::uint32_t second_side);
std::uint32_t fixture_flag_40(Cpu&, std::uint32_t match, std::uint32_t second_side);
std::uint32_t fixture_team(Cpu&, std::uint32_t match, std::uint32_t second_side);
void append_marker(Cpu&, std::uint32_t player, std::uint32_t marker);
void set_pressure_adjustment(Cpu&, std::uint32_t player, std::uint32_t amount);
void set_pending_restart(Cpu&, std::uint32_t match, std::uint32_t kind, std::uint32_t player, std::uint32_t other,
                         std::uint32_t mode, std::uint32_t flag, std::uint32_t row, std::uint32_t column);
void clear_pending_restart(Cpu&, std::uint32_t match);
std::uint32_t pending_restart_side(Cpu&, std::uint32_t match);
std::uint32_t nonzero_byte_1b(Cpu&, std::uint32_t object);
std::uint32_t valid_competition_record(Cpu&, std::uint32_t record);
void adjust_score_toward(Cpu&, std::uint32_t player, double factor);
std::uint32_t second_side_player(Cpu&, std::uint32_t player);
std::uint32_t column_excess(Cpu&, std::uint32_t match, std::uint32_t side, std::uint32_t column);
std::uint32_t player_text(Cpu&, std::uint32_t record);
std::uint32_t turn_cost(Cpu&, std::uint32_t match, std::uint32_t from_row, std::uint32_t from_column,
                        std::uint32_t to_row, std::uint32_t to_column);
std::uint32_t outside_central_rows(Cpu&, std::uint32_t row);
std::uint32_t beyond_column(Cpu&, std::uint32_t side, std::uint32_t column);
std::uint32_t inside_end_area(Cpu&, std::uint32_t side, std::uint32_t row, std::uint32_t column);
void set_last_touch_side(Cpu&, std::uint32_t match, std::uint32_t side);
void clear_goalkeeper(Cpu&, std::uint32_t match, std::uint32_t side);
std::uint32_t scheduled_flag(Cpu&, std::uint32_t match);
std::uint32_t is_statistics_fixture(Cpu&, std::uint32_t match);
std::uint32_t thousands(Cpu&, std::uint32_t object);
std::uint32_t first_of_list(Cpu&, std::uint32_t list);
void copy_cell_record(Cpu&, std::uint32_t to, std::uint32_t from);
std::uint32_t past_substitution_time(Cpu&, std::uint32_t match);
}
