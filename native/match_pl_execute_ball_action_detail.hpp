#pragma once
#include "ball_action.hpp"
#include "guest_abi.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::match_pl::ball_action_detail {
using guest::Cpu;
using guest::Fpu;

inline constexpr unsigned actor_register = 15;
inline constexpr std::uint32_t actor_match = 0x1a4;
inline constexpr std::uint32_t actor_column = 0x105;
inline constexpr std::uint32_t actor_row = 0x106;
inline constexpr std::uint32_t actor_side = 0x27;
inline constexpr std::uint32_t ball_row = 0x791f;
inline constexpr std::uint32_t contact_column_slot = 0x44;
inline constexpr std::uint32_t contact_row_slot = 0x43;
inline constexpr std::uint32_t no_comment = 0xffffffffu;
inline constexpr std::uint32_t match_status = 0x7928;
inline constexpr std::uint32_t match_commentary = 0x79f4;
inline constexpr unsigned recipient_register = 14;
inline constexpr std::uint32_t execution_contact_slot = 0x40;
inline constexpr std::uint32_t execution_blocked_slot = 0x45;
inline constexpr std::uint32_t execution_kind_slot = 0x48;
inline constexpr std::uint32_t execution_rating_slot = 0x4a;
inline constexpr std::uint32_t execution_marker_slot = 0x42;
inline constexpr std::uint32_t execution_feedback_slot = 0x4c;
inline constexpr std::uint32_t actor_balance = 0x1a0;

std::uint32_t truncated_interval(std::uint32_t shifted);
void advance_stream(Cpu& c, std::uint32_t back);
void prepare(Cpu& c, GuestNativeScope& scope);
std::uint32_t signed_half(std::uint32_t value);
std::uint32_t signed_byte(std::uint32_t value);
std::uint32_t subtract_from(Cpu& c, std::uint32_t value, std::uint32_t limit);
void orient(Cpu& c);
void call_owned(Cpu& c, std::uint32_t entry, std::uint32_t back);
void check_area_coordinates(Cpu& c, std::uint32_t actor, bool away,
                            unsigned coordinate, unsigned inside_register, unsigned result);
void sample_narrow_interval(Cpu& c, std::int32_t interval, unsigned result);
void draw_narrow_interval(Cpu& c, std::int32_t interval, unsigned result, std::uint32_t refill_back);
void face(Cpu& c);
void select_area(Cpu& c);
void select_elevated(Cpu& c);
std::uint32_t high_product(std::uint32_t value, std::int32_t multiplier);
std::uint32_t shift_signed_with_carry(Cpu& c, std::uint32_t value, unsigned bits);
bool aligns_with_goal(Cpu& c, std::uint32_t actor, std::uint32_t home_back, std::uint32_t away_back);
void select_low(Cpu& c);
void sample_selected(Cpu& c);
void prepare_contact(Cpu& c);
void comment_on_player(Cpu& c, std::uint32_t actor, std::uint32_t subject, unsigned message, std::uint32_t back);
void comment_on_actor(Cpu& c, std::uint32_t actor, unsigned message, std::uint32_t back);
void comment_on_opposing_side_of(Cpu& c, std::uint32_t actor, std::uint32_t subject, unsigned message, std::uint32_t back);
void comment_on_opposing_side(Cpu& c, std::uint32_t actor, unsigned message, std::uint32_t back);
void resolve_contact(Cpu& c);
void comment_on_recipient(Cpu& c, std::uint32_t actor, std::uint32_t recipient, unsigned message, std::uint32_t back);
void comment_on_side(Cpu& c, std::uint32_t actor, unsigned message, std::uint32_t back);
bool compare_signed_result(Cpu& c);
std::uint32_t draw_opposition_reaction(Cpu& c, unsigned modulus, std::uint32_t multiplier,
                                      unsigned shift, std::uint32_t refill_back);
void maybe_comment_on_opposition_reaction(Cpu& c, std::uint32_t actor, std::uint32_t subject,
                                          std::uint32_t back);
void complete_contact(Cpu& c);
void resolve_receiver(Cpu& c);
void resolve_result(Cpu& c);
std::uint32_t draw_modulo_three(Cpu& c, std::uint32_t refill_back);
void prepare_execution(Cpu& c);
std::uint32_t live_commentary_target(Cpu& c, std::uint32_t index, unsigned last,
                                      std::int32_t toc_slot, std::uint32_t fallback);
void describe_execution(Cpu& c);
void describe_result(Cpu& c);
void describe_failed(Cpu& c);
void prepare_result(Cpu& c);
void comment_on_ordered_pair(Cpu& c, std::uint32_t actor, std::uint32_t first, std::uint32_t second,
                             unsigned message, std::uint32_t back);
void comment_on_pair(Cpu& c, std::uint32_t actor, std::uint32_t recipient, unsigned message, std::uint32_t back);
bool draw_major_contact_parity(Cpu& c, std::uint32_t back);
void complete_receiver(Cpu& c);
void complete_result(Cpu& c);
void complete_unopposed(Cpu& c);
void prepare_final(Cpu& c);
void complete_regular(Cpu& c);
void react_to_success(Cpu& c);
void add_player_counter(Cpu& c, std::uint32_t player, std::uint32_t offset, std::uint32_t change);
void complete_special(Cpu& c);
void prepare_failed(Cpu& c);
void describe_failed_comment(Cpu& c);
void describe_failed_fallback(Cpu& c);
void prepare_failed_outcome(Cpu& c);
void sample_receiver(Cpu& c);
void handle_marker(Cpu& c);
void position_receiver(Cpu& c);
void assess_receiver(Cpu& c);
void complete_receiver_effects(Cpu& c);
void react_to_receiver(Cpu& c);
void report_missing_receiver(Cpu& c);
void return_action(Cpu& c);
void act(Cpu& c);
}
