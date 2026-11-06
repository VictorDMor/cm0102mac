#pragma once

#include "guest_abi.hpp"

namespace cm::aot::match_pl::shot_event_detail {

void report_shot_event(guest::Cpu& c, std::uint32_t actor, unsigned code, std::uint32_t back,
                       bool launch_parameter = false);
bool report_available_miss_message(guest::Cpu& c, std::uint32_t actor, std::uint32_t first_query_return);
void report_power_miss(guest::Cpu& c, std::uint32_t actor, unsigned event, std::uint32_t back);
bool has_shot_pressure(guest::Cpu& c, std::uint32_t actor, std::uint32_t stack, unsigned threshold = 0x350);
void apply_shot_morale(guest::Cpu& c, std::uint32_t player, unsigned range, std::uint32_t refill_return,
                       std::uint32_t morale_return, std::uint32_t refresh_return);
void draw_miss_context(guest::Cpu& c, std::uint32_t refill_back, unsigned base = 4);
bool report_single_shot_context(guest::Cpu& c, std::uint32_t iterator_return);
void report_double_shot_context(guest::Cpu& c, std::uint32_t refill_return);
void continue_shot_retry(guest::Cpu& c, std::uint32_t actor, std::uint32_t stack, std::uint32_t refresh_return);
void restore_and_retry_shot(guest::Cpu& c, std::uint32_t actor, std::uint32_t stack, std::uint32_t back);
void place_alternate_shot_outside_pitch(guest::Cpu& c, std::uint32_t actor, std::uint32_t stack,
                                        std::uint32_t diagnostic_return, unsigned diagnostic_context);
void close_alternate_shot(guest::Cpu& c, std::uint32_t actor, std::uint32_t back);
bool in_goal_area_for_side(guest::Cpu& c, std::uint32_t player, bool away);
bool in_shot_goal_area(guest::Cpu& c, std::uint32_t player);
struct BlockMessageTable {
    std::int32_t toc_offset;
    std::uint32_t read_pc, first, continuation;
};
void report_blocked_shot(guest::Cpu& c, std::uint32_t actor, unsigned code, std::uint32_t back);
void report_blocking_action(guest::Cpu& c, std::uint32_t actor, std::uint32_t stack, BlockMessageTable table);
void credit_shot_assistant(guest::Cpu& c, std::uint32_t actor);
void advance_shot_stream(guest::Cpu& c, std::uint32_t back);
void draw_narrow_shot_sample(guest::Cpu& c, std::int32_t bound, unsigned destination, std::uint32_t back);
void draw_shot_modulo_fifty(guest::Cpu& c, std::uint32_t back);
void call_shot_owner(guest::Cpu& c, std::uint32_t entry, std::uint32_t back);
std::uint32_t draw_shot_modulo_three(guest::Cpu& c, std::uint32_t back);

}
