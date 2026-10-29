// Full returned comparisons for the implemented MATCH_PL block. Dependencies
// not yet reconstructed are scripted identically at their original boundaries.
#include "match_pl_adapter.hpp"
#include "match_pl.hpp"
#include "actor_action_adapter.hpp"
#include "actor_attributes_adapter.hpp"
#include "actor_effects_adapter.hpp"
#include "actor_encounter_adapter.hpp"
#include "actor_flags_adapter.hpp"
#include "actor_motion_adapter.hpp"
#include "actor_policy_adapter.hpp"
#include "marking_pressure_adapter.hpp"
#include "match_selection_support_adapter.hpp"
#include "match_target_adapter.hpp"
#include "pitch_distance_adapter.hpp"
#include "range_sampling_adapter.hpp"
#include "spatial_role_adapter.hpp"
#include "target_acceptance_adapter.hpp"
#include "ball_contact_adapter.hpp"
#include "player_decision_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include "application_exit.hpp"
#include <cstdlib>
#include <iostream>
#include <map>
#include <cmath>
#include <limits>

#if defined(CM_MATCH_PL_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
namespace cm::aot { bool match_pl_reference(State&); }
using namespace cm::aot;
using guest::Cpu;
namespace {
constexpr std::uint32_t data = 0x60000000, player = data + 0x1000,
    match = data + 0x10000, other_match = data + 0x20000,
    trace_at = data + 0x38000, stack = 0x71010000, sentinel = 0xfffffff0;
// The probe stops at a dispatcher service boundary, not an observed PPC
// instruction. This assertion needs no extra generated instruction boundary.
constexpr std::uint32_t fatal_boundary = 0x1076f7d0;
struct SecondaryInputs {
    int match_side = 0, phase = 0, other = 0;
    int ball_column = 4, ball_row = 5, target_column = 4, target_row = 5;
    int trait = 0, effort = 0, mobility = 0, count = 0, condition = 6000, profile = 0;
    int line = -1, row_adjust = -1, crowd = 0, pursuit = 0, moving = 0;
    int linked = 0, linked_column = 4, linked_row = 5;
    float linked_metric = 0;
    unsigned formation = 0, offer = 0, offer_after = 0;
};
struct ContextInputs : SecondaryInputs {
    int main = 0, main_side = 1, linked_slot = 4;
    std::uint32_t team_flags = 0;
    int line0 = 5, line1 = 5, limit0 = -1, limit1 = 9;
    bool peer0 = false, peer1 = false;
    int trait108 = 0, trait109 = 0, trait10e = 0, status110 = 0, opposition = 0;
    float bound_e0 = 20;
    double metric = 0;
    int keeper_status = 0;
    bool keeper_null = false, expected_exit = false, returning_exit = false, goal_flag = false, locked = false;
    std::uint32_t context_query = 0, reseed = 0, role_later = UINT32_MAX;
    bool custom_coefficient = false;
    float coefficient = 1, coefficient_after = 0;
    std::uint32_t coefficient_back = 0;
    int own_count = 0, opposing_count = 0;
};
struct PrimaryInputs : ContextInputs {
    int restart = 3;
    bool target = false;
    int query_score = 2000, query_kind = 1, query_flag = 0;
    int query_column = 3, query_row = 7;
    int shot_quality = 10, shot_duration = 100;
    float attribute_a4 = 0;
    int team_shape = 0, neighbor_count = 0, balance = 0, population = 0, support = 0;
    int policy_query = 0, edge_query = 0, shot_mode = 1;
    int action = 0, keeper_column = 4, keeper_row = INT32_MIN, keeper_role = -1;
    float attribute = 10, bound_e4 = 10;
    bool publish = false, custom_distance = false;
    float distance = 1;
    int list_items = 0, population_count = 0;
    int tactic = 0, rating = 0, staff67 = 10, staff68 = 10, run_trait = 1;
    int event_count = 0, condition_penalty = 0;
    std::array<int, 5> balance_outputs{};
    bool previous_peer = false;
    bool previous_null = false;
    int previous_action = 54, turn_state = 0, heading = 0;
    std::uint32_t opposing_flags = 0xa5a5a5a5;
    bool actual_population = false;
    bool custom_attributes = false;
    float attribute98 = 10, attribute9c = 10;
};
struct ActionInputs {
    int trajectory_result = 0, core_flag = 1, receiver = 0;
    int destination_column = 3, destination_row = 7, force = 0, heading = 0;
    int phase = 0, restart = 0, opposition = 20;
    int defender = 0, defender_side = 1, defender_column = 4, defender_row = 5;
    int target = 1, target_quality = 0, action = 19, foul = 0;
    int contact = 0, severity = 0, caution = 0, target_delay = 0, actor_delay = 0;
    int protected_action = 0, request_restart = 0;
    int actor_bias = 50, actor_skill = 50, actor_temper = 0, target_temper = 0;
    int target_trait0 = 10, target_trait1 = 10, receiver_trait0 = 0, receiver_trait1 = 0;
    int target_attempts = 4, target_successes = 2, target_condition = 7000;
    int time = 0, actor_action = 19, diagnostic = 0, receiver_delay = 0;
    int core_ball_column = INT32_MIN, core_ball_row = INT32_MIN;
    bool receiver_busy_on_reseed = false;
    bool receiver_busy_on_seed_read = false;
    bool split_attribute = false;
    float second_attribute = 0;
    int free_taker = 1, controller = 0;
    int context_contact = -1, temporal_flag = 0;
    float actor_metric = 10, target_metric = 10;
    float attribute = 10000, distance = 4;
    bool custom_distance = false, custom_coefficient = false, possession = true;
    float coefficient = 1;
    float core_capacity = 20;
    int core_weather = 0, core_wait = 0;
    bool custom_core_coefficients = false;
    float core_distance_weight = 1, core_flight_weight = 1, core_work_weight = 1;
    bool core_queue = false;
    int core_queue_gate = 0, core_queue_entries = 0, core_team_policy = 0;
    float core_profile_result = 1.25f;
    int core_previous = 0, core_ball_bias = 0, core_staff_trait = 0, core_goal_restart = 0;
    bool core_has_club = true, core_wait_split = false;
    int core_wait_after = 5;
    int core_phase_after_tally = INT32_MIN;
    int core_recoveries = 2, core_feedback_count = 1, core_feedback_done = 0;
    int core_receiver_side = INT32_MIN, core_receiver_column = INT32_MIN, core_receiver_row = INT32_MIN;
    int core_reseed = INT32_MIN, core_goal_query = INT32_MIN;
    int core_actor_morale = 0, core_target_morale = 0;
    bool core_null_match_at_profile = false;
    bool core_reset_pool_at_tally = false, core_negative_penalty_draw = false;
    int defender_restart_after_query = INT32_MIN;
    bool parent_sampling_reset = false, receiver_busy_after_core_seed = false;
};
constexpr bool action_entry(std::uint32_t entry) {
    return entry == 0x10245220 || entry == 0x10245870 || entry == 0x10274170;
}
constexpr bool core_entry(std::uint32_t entry) { return entry == 0x10247030; }
struct Case {
    std::uint32_t entry;
    int side = 0, column = 4, row = 5;
    std::uint32_t argument = 0, first_role = 1, second_role = 4;
    bool null_player = false, null_match = false, mutate = false;
    std::uint32_t strategy_low = 0, strategy_high = 0x13572468;
    float intensity = 50.0f;
    double factor = 1.25;
    std::uint32_t random_seed = 0;
    bool last_is_player = false, clear_staff_after_scale = false, nonuniform_pool = false, foreign_table = false;
    bool no_feedback = false, feedback_alias = false;
    unsigned pool_position = 0;
    bool force_exit = false;
    bool custom_pool = false;
    std::array<std::uint16_t, 8> pool_words{};
    bool mutate_choice = false, mutate_after_step = false;
    std::uint32_t context_flags = 0;
    SecondaryInputs secondary{};
    ContextInputs context{};
    PrimaryInputs primary{};
    ActionInputs action{};
};
Case active;
// Exercise the original's separate reads without introducing an import or
// replacing either computation. Forward all writes to the comparison observer.
struct ActionReads final : MemoryObserver {
    State& s; MemoryObserver* previous; unsigned count = 0, wait_count = 0;
    bool receiver_changed = false, publication_pending = false, published = false, ball_changed = false;
    bool enabled = (action_entry(active.entry) || core_entry(active.entry)) &&
        (active.action.split_attribute || active.action.receiver_busy_on_seed_read || active.action.core_wait_split || active.action.core_ball_column != INT32_MIN || active.action.receiver_busy_after_core_seed);
    explicit ActionReads(State& state) : s(state), previous(s.memory.date_observer) {
        if (enabled) s.memory.date_observer = this;
    }
    ~ActionReads() { if (enabled) s.memory.date_observer = previous; }
    void mapped(std::uint32_t at, std::uint64_t size) override { if (previous) previous->mapped(at, size); }
    void written(std::uint32_t at, const std::uint8_t* bytes, std::size_t size) override {
        if (previous) previous->written(at, bytes, size);
        if (at == data + 0x328 && size == 4) publication_pending = bytes[3] == 1;
        if (publication_pending && at == match + 0x791e && size == 1) published = true;
    }
    void cleared(std::uint32_t at, std::uint64_t size) override { if (previous) previous->cleared(at, size); }
    void read(std::uint32_t at, const std::uint8_t* bytes, unsigned size) override {
        if (previous) previous->read(at, bytes, size);
        if (active.action.split_attribute && at == player + 0x50 && size == 4 && ++count == 2)
            s.memory.write<4>(at, std::bit_cast<std::uint32_t>(active.action.second_attribute));
        if (active.action.receiver_busy_on_seed_read && at == data + 0x100 && size == 4) {
            s.memory.write<1>(data + 0x902c, 1); receiver_changed = true;
        }
        if (active.action.receiver_busy_after_core_seed && at == data + 0x100 && size == 4 && s.r<4>(4) == stack - 0xc0) {
            s.memory.write<1>(data + 0x902c, 1); receiver_changed = true;
        }
        if (active.action.core_wait_split && at == match + 0x7920 && size == 1 && ++wait_count == 2)
            s.memory.write<1>(at, std::uint8_t(active.action.core_wait_after));
        // Exercise the caller's fresh ball-position reads after the actual core
        // publishes it. The core itself runs to its normal epilogue.
        if (published && !ball_changed && active.action.core_ball_column != INT32_MIN && at == match + 0x791d && size == 1 &&
            active.entry == 0x10245870 && s.r<4>(4) == stack - 0xe0) {
            ball_changed = true;
            s.memory.write<1>(at, std::uint8_t(active.action.core_ball_column));
            s.memory.write<1>(match + 0x791e, std::uint8_t(active.action.core_ball_row));
        }
    }
    void require_reads() const {
        cm::semantic_io_checks::require(!active.action.split_attribute || count >= 2, "Both direct-action attribute reads must execute");
        cm::semantic_io_checks::require(!active.action.receiver_busy_on_seed_read || receiver_changed, "Receiver seed-read mutation must execute");
        cm::semantic_io_checks::require(!active.action.receiver_busy_after_core_seed || receiver_changed, "Post-core receiver seed-read mutation must execute");
        cm::semantic_io_checks::require(!active.action.core_wait_split || wait_count >= 2, "Both core timing wait reads must execute");
        cm::semantic_io_checks::require(active.action.core_ball_column == INT32_MIN || ball_changed, "Core publication ball-position mutation must execute");
    }
};
struct PopulationState {
    bool exercised = false;
    std::uint32_t list = 0, tail = 0, heap_end = 0;
    std::map<std::uint32_t, std::uint32_t> allocated, available;
} population_state;
bool reference_mode = false, generated_mode = false;
constexpr std::uint32_t production_entries[]{
#define MATCH_PL_ROUTINE(entry, ...) entry,
#define MATCH_PL_DRAFT(...)
#include "match_pl_routines.inc"
#undef MATCH_PL_DRAFT
#undef MATCH_PL_ROUTINE
};

bool scripted(State& s) {
    Cpu c(s);
    const auto target = s.pc;
    // Owned action, pass and core bodies execute on both routes, including
    // calls from the primary selector and the root continuation.
    if (action_entry(target) || core_entry(target)) return false;
    // Primary fixtures exercise the actual list destructor and allocator.
    if (active.entry == 0x10278220 && target == 0x107735a0) return false;
    if (active.entry == 0x10278220 && active.primary.actual_population) {
        if (target == 0x101d7dc0) {
            population_state = {true, c.r(6), 0, s.heap_end, s.allocated, s.available};
            return false;
        }
        if (target == 0x10738c20 && population_state.exercised)
            population_state.tail = c.u32(population_state.list + 8);
    }
    constexpr std::array primary_services{0x101e8950u, 0x10245220u, 0x10252080u, 0x102576e0u,
        0x101ee770u, 0x101e4020u, 0x101f21d0u, 0x101d7dc0u, 0x101ef3c0u,
        0x101fd000u, 0x101d8d70u, 0x101d69c0u, 0x101ee9c0u,
        0x10274170u, 0x10245870u, 0x10775130u, 0x10775170u, 0x1077fc28u};
    const bool core_allocation = core_entry(active.entry) && (target == 0x10775130 || target == 0x10775170 || target == 0x1077fc28);
    if (active.entry != 0x10278220 && active.entry != 0x102775e0 && !core_allocation && std::ranges::find(primary_services, target) != primary_services.end()) return false;
    const bool selection_entry = active.entry == 0x10281af0 || active.entry == 0x102836a0 || active.entry == 0x10278220;
    const auto& selection = active.entry == 0x10278220 ? static_cast<const SecondaryInputs&>(active.primary) : active.entry == 0x102836a0 ? static_cast<const SecondaryInputs&>(active.context) : active.secondary;
    if (target == active.entry && match_pl_lists(target)) return false;
    const std::array services{0x107735a0u,
                             0x1022e7e0u, 0x101e3d50u, 0x101e3de0u,
                             0x101e3e70u, 0x10113e10u, 0x101149f0u, 0x101e4e10u,
                             0x101f0dd0u, 0x1077b770u, 0x10780fc0u, 0x10187650u, 0x101878d0u,
                             0x10739890u, 0x10768810u, 0x1077b720u, 0x10769b60u, 0x1076f7d0u,
                             0x101ee610u, 0x101ef8d0u, 0x10114b10u,
                             0x100ca010u, 0x10082f90u, 0x100c7d00u, 0x10718130u,
                             0x10201180u, 0x101f51e0u, 0x101f1530u, 0x101eeac0u, 0x101eeeb0u,
                             0x101f1100u, 0x1022ba10u, 0x101d6860u, 0x101f5050u, 0x101d6780u, 0x101eea30u,
                             0x10717b20u, 0x1069e700u, 0x10658370u,
                             0x101d8120u,
                             0x101e25a0u, 0x101d8260u, 0x101e28b0u, 0x101f5280u,
                             0x101ee680u, 0x101ee9f0u, 0x101e4ea0u, 0x101f1e10u, 0x101e3f10u,
                             0x101e8950u, 0x10245220u, 0x10252080u, 0x102576e0u,
                             0x101ee770u, 0x101e4020u, 0x101f21d0u, 0x101d7dc0u,
                             0x101ef3c0u, 0x101fd000u, 0x101d8d70u, 0x101d69c0u, 0x101ee9c0u,
                             0x10274170u, 0x10245870u, 0x10775130u, 0x10775170u, 0x1077fc28u};
    constexpr std::array action_services{0x101f5230u, 0x101d68f0u, 0x101f0c00u,
        0x101eaf60u, 0x101f5350u, 0x101f5110u, 0x101f3a30u, 0x101ee530u,
        0x10303560u, 0x101f0090u, 0x1026a1b0u, 0x1026d660u};
    const bool action_context = action_entry(active.entry) || core_entry(active.entry) || active.entry == 0x10278220 || active.entry == 0x102775e0;
    const bool core_team_query = action_context && target == 0x1014c2e0;
    if (std::ranges::find(services, target) == services.end() && !core_team_query &&
        (!action_context || std::ranges::find(action_services, target) == action_services.end())) return false;
    auto used = c.u32(trace_at);
    const auto append = [&](std::uint32_t word) { c.put32(trace_at + 4 + used, word); used += 4; };
    append(target); append(c.lr());
    for (unsigned reg = 3; reg <= 10; ++reg) append(c.r(reg));
    for (unsigned reg = 13; reg < 32; ++reg) append(c.r(reg));
    for (unsigned field = 0; field < 8; ++field) append(c.cr(field));
    for (unsigned reg = 1; reg <= 2; ++reg) {
        const auto bits = std::bit_cast<std::uint64_t>(c.f(reg));
        append(bits >> 32); append(bits);
    }
    for (unsigned reg = 14; reg < 32; ++reg) {
        const auto bits = std::bit_cast<std::uint64_t>(c.f(reg)); append(bits >> 32); append(bits);
    }
    c.put32(trace_at, used);
    if (target == 0x10113e10) {
        c.set(3, active.strategy_high); c.set(4, active.strategy_low);
        c.set_cr(0, active.strategy_low & 1 ? 4 : 2);
    } else if (target == 0x101e3d50) {
        if (active.mutate) c.put32(player + 0x1a4, other_match);
        if (active.entry == 0x10289d50 && (active.context_flags & 64)) c.put32(player + 0x70, data + 0xd100);
        c.set(3, active.entry == 0x10278220 && c.r(4) == data + 0xf000 && active.primary.keeper_role >= 0 ? std::uint32_t(active.primary.keeper_role) : active.entry == 0x102836a0 && c.lr() != 0x10283864 && active.context.role_later != UINT32_MAX ? active.context.role_later : active.first_role);
    } else if (target == 0x101e3de0 || target == 0x101e3e70) c.set(3, active.second_role);
    else if (target == 0x101149f0 || target == 0x101e4e10) {
        c.set(3, selection_entry ? target == 0x101149f0 ? selection.formation : std::uint32_t(selection.line) : active.argument);
    }
    else if (target == 0x101f0dd0 || target == 0x1077b770) {
        c.set(3, target == 0x101f0dd0 && selection_entry ? std::uint32_t(selection.moving) : target == 0x1077b770 && active.entry == 0x102836a0 ? active.context.reseed : target == 0x1077b770 && active.entry == 0x10278220 ? active.primary.reseed : active.first_role);
        if (target == 0x1077b770 && active.entry == 0x102836a0 && active.context.coefficient_back && c.lr() == active.context.coefficient_back) {
            c.put_f32(c.toc(-0x51e0) + 0xc8, active.context.coefficient_after);
            c.put32(data + 0x31c, c.u32(data + 0x31c) + 1);
        }
        if (target == 0x1077b770 && active.entry == 0x10281af0) {
            if (active.context_flags & 2) c.put8(player + 0x10a, 255);
            if (active.context_flags & 4) c.put32(player + 0x1a4, other_match);
        }
        if (target == 0x1077b770 && active.entry == 0x102775e0) {
            if (active.context_flags & 64) { c.put8(player + 0x107, 127); c.put8(player + 0x10a, 127); }
            if (active.context_flags & 128) c.put32(player + 0x1a4, other_match);
        }
        if (target == 0x1077b770 && active.entry == 0x10289d50) {
            const auto back = c.lr();
            if ((active.context_flags & 128) && back == 0x1028a9c0) c.put32(player + 0x1a4, 0);
            if (active.context_flags & 8) c.put32(player + 0x1a4, other_match);
            if (back == 0x1028a490 || back == 0x1028a594 || back == 0x1028ab24) {
                if (active.context_flags & 16) c.put16(c.u32(player + 0x1a4) + 0x7948, 0);
                if (active.context_flags & 32) c.put8(player + 0x1f, 100);
            }
        }
    }
    else if (target == 0x10780fc0) {
        c.put32(c.sp() + 0x14, c.r(2)); c.set_f(1, std::pow(c.f(1), c.f(2)));
        if (active.entry == 0x10286650 && active.mutate) {
            c.put_f32(player + 0x1a8, 3); c.put32(player + 0x1ac, c.r(5));
        }
    }
    else if (target == 0x10187650) {
        c.set(3, active.strategy_low);
        if (active.clear_staff_after_scale) c.put32(player + 0x6c, 0);
    }
    else if (target == 0x101878d0) c.set(3, active.strategy_high);
    else if (target == 0x101ee610) c.set(3, c.lr() == 0x1028183c ? active.strategy_high : active.first_role);
    else if (target == 0x101ef8d0) c.set_f(1, active.entry == 0x102836a0 ? active.context.metric : active.factor);
    else if (target == 0x10114b10) c.set(3, data + 0xc000);
    else if (target == 0x100ca010) {
        c.set(3, active.first_role);
        if (active.mutate) {
            c.put8(player + 0x27, c.u8(player + 0x27) == 0 ? 1 : 0);
            c.put32(player + 0x1a4, other_match);
            c.put32(player + 0x6c, data + 0xc100); c.put32(player + 0x70, data + 0xd100);
        }
    }
    else if (target == 0x10082f90) c.set(3, active.null_player ? 0 : data + 0xe000);
    else if (target == 0x100c7d00) c.set(3, active.strategy_high);
    else if (target == 0x10718130) c.set(3, active.second_role);
    else if (target == 0x101f51e0) {
        c.put8(c.r(5), active.first_role);
        if (active.entry == 0x102884d0 && active.mutate) {
            c.put32(player + 0x1a4, other_match); c.put8(player + 0x27, 1 - active.side);
        }
    }
    else if (target == 0x10201180) {
        if (active.entry == 0x10288230 && active.mutate && c.r(4) != 0x20dd) c.put8(c.sp() + 0x38, active.argument);
        if (active.entry == 0x102884d0 && active.mutate_after_step && c.r(4) == 0x21b9) {
            c.put32(player + 0x1a4, other_match); c.put8(player + 0x27, 1 - active.side);
            c.put8(other_match + 0x7928, 7); c.put8(other_match + 0x791e, 7);
        }
        if (core_entry(active.entry) && active.mutate_after_step && c.u32(data + 0x31c) == 0) {
            c.put32(player + 0x1a4, other_match); c.put8(player + 0x27, 1 - active.side); c.put8(player + 0x2f, 99);
            c.put8(other_match + 0x7928, 7); c.put32(data + 0x31c, 1);
        }
    }
    else if (target == 0x101f1530) {}
    else if (target == 0x101eeac0) c.set(3, active.strategy_low);
    else if (target == 0x101eeeb0) c.set(3, active.strategy_high);
    else if (target == 0x101f1100) {
        if (active.mutate) {
            c.put32(player + 0x1a4, other_match); c.put8(player + 0x2f, 99);
            c.put8(player + 0x27, c.u8(player + 0x27) == 0 ? 1 : 0);
        }
    }
    else if (target == 0x1022ba10) c.set(3, 0);
    else if (target == 0x101d6860) {
        if (active.entry == 0x102876c0 && active.mutate) {
            c.put32(player + 0x1a4, other_match);
            c.put32(data + 0x9000 + 0x6c, data + 0xe200); c.put8(data + 0xe269, 25);
            c.put8(data + 0x9000 + 0x27, active.side);
        }
    }
    else if (target == 0x101f5050) {
        c.put8(c.r(8), 3); c.put8(c.r(9), 7); c.put8(c.r(10), 1);
        c.set(3, active.strategy_high & 1);
    }
    else if (target == 0x101d6780) {
        if (active.entry == 0x102884d0 && active.mutate_choice) c.put8(c.sp() + 0x38, active.row);
    }
    else if (target == 0x101eea30) {
        c.set(3, active.entry == 0x102836a0 ? active.context.context_query : active.second_role);
        if (active.entry == 0x102775e0 && (active.context_flags & 256)) c.put32(player + 0x1a4, other_match);
    }
    else if (target == 0x101d8120) {
        c.set(3, active.context_flags & 1 ? 0 : active.context_flags & 2 ? 255 : 1);
        if (active.context_flags & 4) c.put8(player + 0x104, 0);
        if (active.context_flags & 65536) c.put8(player + 0x104, 3);
        if (active.context_flags & 131072) c.put32(player + 0x1b8, data + 0xaa00);
        if (active.context_flags & 8) c.put32(player + 0x1b4, data + 0xa800);
        if (active.context_flags & 16) { c.put8(player + 0x105, 3); c.put8(player + 0x106, 7); }
        if (active.context_flags & 32) c.put32(player + 0x1a4, other_match);
    }
    else if (target == 0x101e25a0 || target == 0x101e28b0 || target == 0x101d8260) {
        c.put8(c.r(5), selection.target_column); c.put8(c.r(6), selection.target_row);
        if (target == 0x101d8260) {
            const auto count = c.u32(data + 0x314) + 1; c.put32(data + 0x314, count);
            c.set(3, count > selection.offer_after ? selection.offer : 0);
        }
        if (active.entry == 0x10281af0 && (active.context_flags & 8)) {
            c.put32(player + 0x1a4, other_match); c.put8(player + 0x27, 1 - active.side);
        }
        if (active.entry == 0x10281af0 && (active.context_flags & 16)) c.put16(player + 0x36, 32765);
        if (active.entry == 0x10281af0 && (active.context_flags & 64) && target == 0x101d8260) c.put8(player + 0x10a, 0);
    }
    else if (target == 0x101e8950) {
        c.put8(c.r(6), active.primary.query_kind); c.put8(c.r(7), active.primary.query_flag);
        c.put8(c.r(8), active.primary.query_column); c.put8(c.r(9), active.primary.query_row);
        c.put32(c.r(10), active.primary.query_score); c.set(3, active.primary.target ? data + 0x9000 : 0);
        if (active.mutate) { c.put32(player + 0x1a4, other_match); c.put8(player + 0x27, active.side == 0 ? 1 : 0); }
    }
    else if (target == 0x10252080) {
        c.put8(c.r(4), active.primary.query_kind); c.put16(c.r(5), active.primary.shot_duration);
        c.set(3, active.primary.shot_quality); c.set_f(1, active.factor);
    }
    else if (target == 0x10245220 || target == 0x102576e0 || target == 0x10274170 || target == 0x10245870) { c.set(3, 9); c.set_f(1, active.factor); }
    else if (target == 0x101f5280) c.set(3, selection.pursuit);
    else if (target == 0x101ee680) c.set(3, selection.crowd);
    else if (target == 0x101ee9f0) c.set(3, selection.moving);
    else if (target == 0x101e4ea0) c.set(3, selection.row_adjust);
    else if (target == 0x101f1e10) c.set(3, (active.entry == 0x10278220 ? active.primary.keeper_null : active.context.keeper_null) ? 0 : data + 0xf000);
    else if (target == 0x101e3f10) c.set_f(1, active.entry == 0x10278220 ? active.primary.metric : active.context.metric);
    else if (target == 0x101ee770) c.set(3, active.primary.team_shape);
    else if (target == 0x101e4020) c.set(3, active.primary.neighbor_count);
    else if (target == 0x101f21d0) {
        for (unsigned reg = 5; reg <= 9; ++reg) c.put8(c.r(reg), active.primary.balance_outputs[reg - 5]);
        c.set(3, active.primary.balance);
    }
    else if (target == 0x10775130) c.set(3, allocate(s, c.r(3), false));
    else if (target == 0x10775170) deallocate(s, c.r(3));
    else if (target == 0x1077fc28) {
        c.put32(c.sp() + 0x14, c.r(2));
        cm::semantic_io_checks::require(c.r(5) <= 0x1c4, "Bounded primary candidate copy");
        s.memory.copy_in(c.r(4), s.memory.copy_out(c.r(3), c.r(5)));
    }
    else if (target == 0x101d7dc0) {
        const auto out = c.r(5), list = c.r(6), service_back = c.lr(), service_stack = c.sp();
        population_state = {true, list, 0, s.heap_end, s.allocated, s.available};
        for (int index = 0; index < active.primary.list_items; ++index) {
            c.set(3, list); c.set(4, player); c.set(5, 0x1c4);
            c.call(0x10738d10, 0xffffffe0);
            cm::semantic_io_checks::require(c.r(3) != 0 && c.sp() == service_stack,
                                           "Primary candidate-list append failed");
        }
        population_state.tail = c.u32(list + 8);
        c.set_lr(service_back);
        c.put16(out, active.primary.population_count); c.set(3, active.primary.population);
    }
    else if (target == 0x101ef3c0) c.set(3, active.primary.support);
    else if (target == 0x101fd000 || target == 0x101d8d70) c.set(3, active.primary.policy_query);
    else if (target == 0x101d69c0) c.set(3, active.primary.shot_mode);
    else if (target == 0x101ee9c0) c.set(3, active.primary.edge_query);
    else if (target == 0x10717b20) {
        const auto out = c.r(3), days = c.r(5);
        c.put16(out, std::uint32_t(-30000) + days); c.put16(out + 2, 60000 + days); c.put8(out + 4, 0x80 + days);
        if (active.entry == 0x10289d50 && (active.context_flags & 256) && days == 7) c.put32(data + 0xc000, 100);
    }
    else if (target == 0x1069e700) {
        c.set(3, c.r(7) == active.strategy_high ? (active.context_flags & 512 ? 255 : 1) : 0);
        if (active.context_flags & 4) { c.put32(player + 0x6c, data + 0xc200); c.put32(player + 0xfc, data + 0xb200); }
    }
    else if (target == 0x10658370) c.set(3, active.last_is_player);
    else if (target == 0x10768810) c.set(3, 17);
    else if (target == 0x10739890 || target == 0x1077b720 || target == 0x10769b60 || target == 0x1076f7d0) {
        if (target == 0x1076f7d0 && active.entry == 0x102836a0 && active.context.expected_exit) throw ApplicationExit{};
        if (target == 0x1076f7d0 && core_entry(active.entry) && active.force_exit) throw ApplicationExit{};
    }
    else if (target == 0x107735a0) {
        c.put32(data + 0x300, c.r(3));
        if (active.force_exit) throw ApplicationExit{};
    }
    else {
        c.put32(data + 0x304, target);
        c.put32(data + 0x308, c.r(4)); c.put32(data + 0x30c, c.r(5));
        c.set(3, active.first_role);
    }
    // Root continuation cases now execute the real primary selector. Pending
    // action dependencies may change its live actor before it returns.
    std::uint32_t action_parent = target == 0x101eaf60 && c.lr() == 0x10274978 ? c.u32(c.sp() + 0x128) : 0;
    if (target == 0x101eaf60 && c.lr() == 0x102486f4) {
        const auto core_parent = c.u32(c.sp() + 0x158);
        if (core_parent == 0x1024529c) action_parent = c.u32(c.sp() + 0x218);
        else if (core_parent == 0x10246b30) action_parent = c.u32(c.sp() + 0x238);
    }
    const bool pending_primary_action = c.lr() >= 0x10278220 && c.lr() < 0x10281780 &&
        (target == 0x102576e0 || target == 0x1022e7e0);
    if (active.entry == 0x102775e0 && (pending_primary_action || (action_parent >= 0x10278220 && action_parent < 0x10281780))) {
        const auto live_match = c.u32(player + 0x1a4);
        if (active.context_flags & 1024) c.put32(live_match + 0xe000, 0);
        if (active.context_flags & 2048) c.put16(player + 0x19c, 109);
        if (active.context_flags & 4096) c.put8(player + 0x2c, 1);
        if (active.context_flags & 8192) c.put8(live_match + 0x7928, 1);
        if (active.context_flags & 16384) c.put8(player + 0x10a, 0);
        if (active.context_flags & 32768) c.put32(player + 0x1a4, other_match);
    }
    if (action_context) {
        auto in = active.action;
        if (!action_entry(active.entry) && !core_entry(active.entry)) { in.trajectory_result = 1; in.target = 0; in.defender = 0; }
        switch (target) {
        case 0x101f0dd0:
            if (core_entry(active.entry) && in.core_goal_query != INT32_MIN) c.set(3, in.core_goal_query);
            break;
        case 0x101f5230:
            c.set(3, in.defender == 2 ? player : in.defender ? data + 0x9200 : 0);
            if (in.defender_restart_after_query != INT32_MIN) {
                c.put8(c.u32(player + 0x1a4) + 0x7928, in.defender_restart_after_query); c.put32(data + 0x32c, 1);
            }
            break;
        case 0x101eaf60:
            c.put8(c.r(7), in.actor_delay); c.put8(c.r(8), in.target_delay); c.put16(c.r(9), in.target_quality);
            c.put8(c.r(10), in.action); c.put8(c.u32(c.sp() + 0x38), in.context_contact < 0 ? in.contact : in.context_contact);
            c.set(3, in.target == 2 ? player : in.target ? data + 0x9000 : 0); break;
        case 0x101f5050:
            c.put8(c.r(8), in.caution); c.put8(c.r(9), in.severity); c.put8(c.r(10), in.contact); c.set(3, in.foul); break;
        case 0x101f5110: c.put_f32(c.r(10), 1.25f); break;
        case 0x101f3a30: c.set(3, in.free_taker ? data + 0x9400 : 0); break;
        case 0x101ee530: c.set(3, in.protected_action); break;
        case 0x101f1100: c.put8(c.u32(player + 0x1a4) + 0x7928, in.request_restart); break;
        case 0x1077b770:
            if (core_entry(active.entry) && in.core_reseed != INT32_MIN) c.set(3, in.core_reseed);
            if (core_entry(active.entry) && in.core_null_match_at_profile && c.u32(player + 0x1a4) == 0) {
                c.put32(player + 0x1a4, match); c.put32(data + 0x324, 2);
            }
            if (in.receiver_busy_on_reseed) c.put8(data + 0x902c, 1);
            break;
        case 0x10303560:
            c.set_f(1, in.core_profile_result);
            if (core_entry(active.entry) && in.core_negative_penalty_draw) {
                c.put32(c.toc(-0x7b90), 0x80000001); c.put32(c.toc(-0x7b84), c.toc(-0x7b8c));
            }
            if (core_entry(active.entry) && in.core_null_match_at_profile) {
                c.put32(player + 0x1a4, 0); c.put32(data + 0x324, 1);
                c.put32(c.toc(-0x7b84), c.u32(c.toc(-0x7b88)));
            }
            break;
        case 0x1014c2e0: c.set(3, in.core_team_policy); break;
        case 0x101f0090:
            c.set(3, in.trajectory_result);
            if (in.core_ball_column != INT32_MIN) c.put32(data + 0x328, 1);
            if (in.parent_sampling_reset) {
                const auto position = active.pool_position;
                const auto offset = position >= 3 ? (position - 2) * 2 : position & 1 ? 14u : position & 2 ? 12u : 0u;
                c.put32(c.toc(-0x7b90), active.random_seed); c.put32(c.toc(-0x7b84), c.toc(-0x7b8c) + offset);
            }
            break;
        case 0x101f5350:
            if (core_entry(active.entry) && in.core_reset_pool_at_tally) {
                c.put32(c.toc(-0x7b90), 0); c.put32(c.toc(-0x7b84), c.toc(-0x7b8c));
            }
            if (core_entry(active.entry) && in.core_phase_after_tally != INT32_MIN) {
                c.put8(c.u32(player + 0x1a4) + 0x791f, in.core_phase_after_tally);
                c.put32(data + 0x320, 1);
            }
            break;
        case 0x1026a1b0: case 0x1026d660: c.put8(c.u32(player + 0x1a4) + 0x7928, in.core_goal_restart); break;
        }
    }
    // These services leave arguments intact, making caller preparation visible.
    c.finish();
    return true;
}
bool probe(State& s) {
    // Cpu::call runs lowered leaves before the probe. Keep the reference's
    // dispatch path symmetric, particularly the two-instruction side getter.
    if (run_leaf(s)) return true;
    if (scripted(s)) return true;
    if (reference_mode && match_pl_lists(s.pc)) return match_pl_reference(s);
    if (reference_mode && s.pc >= 0x102238a0 && s.pc < 0x1028ad60)
        return match_pl_reference(s);
    return false;
}

void initialize_selection(Cpu& c, const SecondaryInputs& in) {
    c.put32(data + 0x314, 0);
    c.put32(c.r(2) - 0x7bac, data + 0x400); c.put32(c.r(2) - 0x7ba8, data + 0x500);
    c.put32(player + 0x70, data + 0xd000); c.put32(player + 0x1b0, in.linked ? data + 0x9000 : 0);
    c.put8(player + 0x19, 4); c.put8(player + 0x104, in.count); c.put8(player + 0x10a, in.trait);
    c.put8(player + 0x10c, in.mobility); c.put8(player + 0x10d, in.effort); c.put8(player + 0x28, 255);
    c.put16(player + 0x36, in.condition);
    c.put_f32(player + 0xe4, float(active.factor) - c.f32(c.toc(-0x51e0) + 0x5c));
    for (auto field : {0x17u, 0x18u, 0x19u}) c.put8(data + 0xd000 + field, 0);
    c.put8(data + 0xd01a, in.profile);
    c.put32(match + 0xe000, 0); c.put32(match + 0xdff8, in.other == 1 ? player : in.other == 2 ? data + 0x9000 : 0);
    c.put8(match + 0x7924, in.match_side); c.put8(match + 0x791f, in.phase);
    c.put8(match + 0x791d, in.ball_column); c.put8(match + 0x791e, in.ball_row); c.put8(match + 0x1de, active.context_flags & 32 ? 1 : 0);
    c.put32(match + 0x81f4 + unsigned(active.side) * 0x18e0, 0);
    c.put8(data + 0x9019, in.linked == 2 ? -1 : in.linked == 3 ? -5 : in.linked == 4 ? -2 : 4);
    c.put8(data + 0x9027, 1 - active.side); c.put8(data + 0x9105, in.linked_column); c.put8(data + 0x9106, in.linked_row);
    c.put_f32(data + 0x90b0, in.linked_metric);
    for (unsigned column = 0; column < 9; ++column) for (unsigned row = 0; row < 12; ++row) {
        const auto cell = match + 0xa08 + column * 0x450 + row * 0x5c;
        c.put8(cell + 0x58, 0); c.put8(cell + 0x59, 0);
    }
    c.s.memory.copy_in(other_match, c.s.memory.copy_out(match, 0x10000));
}

void initialize_actions(Cpu& c) {
    const auto& in = active.action; const auto distances = data + 0x4000, directions = data + 0x8000;
    c.put32(data + 0x32c, 0);
    c.put32(c.r(2) - 0x6424, distances); c.put32(c.r(2) - 0x641c, directions);
    for (int offset = -2048; offset < 8192; offset += 4) c.put_f32(distances + std::uint32_t(offset), in.custom_distance ? in.distance : float(std::abs(offset) / 16.0));
    if (!in.custom_distance) for (unsigned row = 0; row < 16; ++row) for (unsigned col = 0; col < 9; ++col)
        c.put_f32(distances + row * 36 + col * 4, float(std::hypot(double(row), double(col))));
    for (int col = -8; col <= 8; ++col) for (int row = -11; row <= 11; ++row) {
        auto angle = int(std::atan2(double(row), double(col)) * 180.0 / std::acos(-1.0)); if (angle < 0) angle += 360;
        c.put16(directions + 0x1c8 + std::uint32_t(col * 48 + row * 2), angle);
    }
    c.put32(c.r(2) - 0x7bac, data + 0x400); c.put32(c.r(2) - 0x7ba8, data + 0x500);
    c.put32(c.r(2) - 0x7b90, data + 0x100); c.put32(data + 0x100, active.random_seed);
    c.put32(c.r(2) - 0x7b8c, data + 0x200); c.put32(c.r(2) - 0x7b88, data + 0x104); c.put32(data + 0x104, data + 0x20e);
    const auto position = active.pool_position;
    c.put32(c.r(2) - 0x7b84, data + 0x108); c.put32(data + 0x108, data + 0x200 + (position >= 3 ? (position - 2) * 2 : position & 1 ? 14 : position & 2 ? 12 : 0));
    for (unsigned i = 0; i < 8; ++i) c.put16(data + 0x200 + i * 2, active.custom_pool ? active.pool_words[i] : active.strategy_low + (active.nonuniform_pool ? i * 7919 : 0));
    for (auto address : {player, data + 0x9000, data + 0x9200, data + 0x9400}) {
        c.s.memory.copy_in(address, std::vector<std::uint8_t>(0x1c4, 0));
        c.put32(address + 0x1a4, match); c.put32(address + 0x70, data + 0xd000); c.put32(address + 0x6c, data + 0xc000);
        c.put8(address + 0x27, address == player ? active.side : 1 - active.side); c.put8(address + 0x2f, address == player ? 7 : 8);
        c.put8(address + 0x105, active.column); c.put8(address + 0x106, active.row); c.put8(address + 0x28, 255);
        c.put16(address + 0x36, in.target_condition); c.put16(address + 0x19c, 19); c.put_f32(address + 0x50, in.attribute); c.put_f32(address + 0x1a8, 50);
        c.put_f32(address + 0xb0, 10); c.put_f32(address + 0xa8, 10); c.put_f32(address + 0xe0, 20); c.put_f32(address + 0xe4, 20);
        c.put32(address + 0xfc, data + 0xb000); c.put8(address + 0x19, 4);
    }
    c.put8(data + 0x9227, in.defender_side); c.put8(data + 0x9305, in.defender_column); c.put8(data + 0x9306, in.defender_row);
    c.put8(data + 0x9004, in.target_attempts); c.put8(data + 0x9005, in.target_successes);
    c.put8(data + 0x9007, in.target_attempts); c.put8(data + 0x9008, in.target_successes); c.put8(data + 0x9006, 2); c.put8(data + 0x9009, 1);
    c.put16(data + 0x9032, in.target_temper); c.put8(data + 0x910f, in.target_trait0); c.put8(data + 0x9110, in.target_trait1);
    c.put8(data + 0x910a, in.receiver_trait0); c.put8(data + 0x9108, in.receiver_trait1); c.put8(data + 0x902c, in.receiver_delay);
    c.put16(player + 0x1a0, in.actor_bias); c.put16(player + 0x13e, in.actor_skill); c.put16(player + 0x32, in.actor_temper); c.put16(player + 0x19e, in.heading);
    c.put8(player + 0x10e, active.second_role); c.put8(player + 0x2c, 0);
    c.put16(player + 0x19c, in.actor_action);
    c.put8(match + 0x791d, active.column); c.put8(match + 0x791e, active.row); c.put8(match + 0x791f, in.phase);
    c.put8(match + 0x7928, in.restart); c.put8(match + 0x7931, in.diagnostic); c.put8(match + 0x77bc, in.opposition); c.put8(match + 0x77bd, in.opposition);
    c.put32(match + 0xe000, in.possession ? player : 0); c.put32(match + 0xdff4, 0); c.put32(match + 0xdff8, 0);
    c.put16(match + 0x7948, in.time); c.put32(match + 0x3118, data + 0xe000);
    c.put32(data + 0xe014, 0);  // Valid scene with no optional clock-event list.
    c.put32(data + 0xe01c, data + 0xb000); c.put32(data + 0xe020, data + 0xb100);
    c.put32(data + 0xb02c, in.controller ? data + 0xb800 : 0); c.put32(data + 0xb12c, in.controller ? data + 0xb800 : 0);
    c.put8(data + 0xb919, in.controller == 2 ? 8 : 7);
    c.put8(match + 0xe03d, in.temporal_flag); c.put8(match + 0xe073, in.temporal_flag);
    c.put_f32(player + 0xb0, in.actor_metric); c.put_f32(data + 0x90b0, in.target_metric);
    if (in.custom_coefficient) c.put_f32(c.toc(-0x51e0) + 0x2c, in.coefficient);
    c.s.memory.copy_in(other_match, c.s.memory.copy_out(match, 0x10000));
    c.set(3, player); c.set(4, active.argument); c.set(5, active.entry == 0x10245870 ? in.force : in.receiver == 2 ? player : in.receiver ? data + 0x9000 : 0);
    c.set(6, in.core_flag); c.set(7, in.destination_column); c.set(8, in.destination_row);
    if (core_entry(active.entry)) {
        c.put_f32(player + 0xc8, in.core_capacity);
        c.put16(player + 0x2a, in.core_ball_bias); c.put32(player + 0xfc, in.core_has_club ? data + 0xb000 : 0);
        c.put32(data + 0x31c, 0);
        c.put32(data + 0x320, 0);
        c.put32(data + 0x324, 0);
        c.put8(player + 0x17, in.core_actor_morale); c.put8(data + 0x9017, in.core_target_morale);
        c.put8(data + 0x9006, in.core_recoveries); c.put8(data + 0x9009, in.core_feedback_count);
        c.put8(data + 0x9100, in.core_feedback_done);
        if (in.receiver == 1) {
            if (in.core_receiver_side != INT32_MIN) c.put8(data + 0x9027, in.core_receiver_side);
            if (in.core_receiver_column != INT32_MIN) c.put8(data + 0x9105, in.core_receiver_column);
            if (in.core_receiver_row != INT32_MIN) c.put8(data + 0x9106, in.core_receiver_row);
        }
        if (in.custom_core_coefficients) {
            c.put_f32(c.toc(-0x51e0) + 0x84, in.core_distance_weight);
            c.put_f32(c.toc(-0x51e0) + 0x5c, in.core_flight_weight);
            c.put_f32(c.toc(-0x51e0) + 0x8c, in.core_work_weight);
        }
        for (auto offset : {0x12au, 0x12cu, 0x12eu, 0x130u, 0x134u, 0x15au, 0x15cu, 0x15eu, 0x186u, 0x18au, 0x198u})
            c.put16(player + offset, in.actor_skill);
        c.put32(c.r(2) - 0x7b3c, data + 0x700); c.put32(data + 0x700, data + 0x740);
        c.put32(c.r(2) - 0x7be4, data + 0x704); c.put32(data + 0x704, 100);
        c.put32(match + 0x3114, 0); c.put16(match + 0x30d8, 0); c.put8(match + 0x30d8, in.core_weather);
        c.put8(match + 0x7920, in.core_wait); c.put8(match + 0x1c0, in.core_previous);
        c.put8(data + 0xc066, in.core_staff_trait); c.put8(data + 0xc067, 0); c.put32(data + 0xc000, 1); c.put32(data + 0xb000, 2);
        if (in.core_queue) {
            constexpr auto list = data + 0x10000 - 0x100;
            c.put16(list, 0); c.put32(list + 4, 0); c.put32(list + 8, 0); c.put32(list + 12, 0);
            c.put32(match + 0x3114, in.core_queue_gate == 1 ? 0 : list);
            c.put32(player + 0x6c, in.core_queue_gate == 14 ? 0 : data + 0xc000); c.put32(data + 0xc000, in.core_queue_gate == 6 ? 2 : 0);
            c.put32(data + 0x906c, in.core_queue_gate == 4 ? 0 : data + 0xc200); c.put32(data + 0xc200, 1);
            c.put32(data + 0x704, in.core_queue_gate == 5 ? 1 : 2);
            c.put32(data + 0x90fc, in.core_queue_gate == 7 ? 0 : data + 0xb000);
            c.put8(data + 0xc267, in.core_queue_gate == 9 ? 127 : 0); c.put8(data + 0xc266, 0);
            c.put8(data + 0xd01c, in.core_queue_gate == 10 ? 0 : 127);
            c.put8(data + 0xd025, in.core_queue_gate == 11 ? 0 : 127);
            c.put16(data + 0xd00a, in.core_queue_gate == 12 ? -32768 : 32767);
        }
        c.put8(data + 0x300, in.core_flag);
        c.s.memory.copy_in(other_match, c.s.memory.copy_out(match, 0x10000));
        c.set(4, active.column); c.set(5, active.row); c.set(6, active.argument); c.set(7, data + 0x300);
        c.set(8, in.receiver == 2 ? player : in.receiver ? data + 0x9000 : 0);
        c.set(9, in.destination_column); c.set(10, in.destination_row);
    }
}

void initialize_context(Cpu& c, const ContextInputs& in) {
    initialize_selection(c, in); c.put32(data + 0x31c, 0);
    c.put32(match + 0xe000, in.main == 1 ? player : in.main == 2 ? data + 0x9000 : 0);
    c.put8(data + 0x9027, in.main_side); c.put32(data + 0x91a4, match); c.put8(data + 0x9019, in.linked_slot);
    c.put32(match + 0xdff4, 0); c.put32(match + 0x79f4, data + 0x8000);
    c.put8(match + 0x1de, in.locked); c.put8(match + 0x1e6, in.goal_flag);
    c.put32(match + 0xe014, in.peer0 ? data + 0xa000 : 0); c.put32(match + 0xe018, in.peer1 ? data + 0xa200 : 0);
    c.put8(data + 0xa106, in.limit0); c.put8(data + 0xa306, in.limit1);
    c.put8(match + 0x7925, in.line0); c.put8(match + 0x7926, in.line1);
    c.put32(match + 0x81f4 + unsigned(active.side) * 0x18e0, in.team_flags);
    c.put8(match + 0x77bc, in.opposition); c.put8(match + 0x77bd, in.opposition);
    c.put8(player + 0x108, in.trait108); c.put8(player + 0x109, in.trait109); c.put8(player + 0x10e, in.trait10e); c.put8(player + 0x110, in.status110);
    c.put_f32(player + 0xe0, in.bound_e0); c.put_f32(player + 0xe4, float(active.factor - c.f64(c.toc(-0x51e8) + 0x28)));
    c.put_f32(player + 0x1a8, active.intensity); c.put_f32(player + 0x50, 50);
    c.put8(player + 0x10f, 10); c.put8(player + 0x2f, 7); c.put8(player + 0x2c, 0); c.put8(player + 0x2d, 0);
    c.put16(player + 0x19e, 0); c.put8(data + 0xd030, 10); c.put8(data + 0xd03b, 10);
    c.put8(data + 0xf02c, in.keeper_status);
    if (in.custom_coefficient) c.put_f32(c.toc(-0x51e0) + 0xc8, in.coefficient);
    const auto cell = match + 0xa08 + std::uint32_t(active.column * 0x450 + active.row * 0x5c);
    c.put8(cell + 0x58 + active.side, in.own_count); c.put8(cell + 0x58 + unsigned(active.side == 0), in.opposing_count);
    if (in.returning_exit) { c.s.memory.map(0, 0x1000); c.put8(0x2c, 0); }
    c.s.memory.copy_in(other_match, c.s.memory.copy_out(match, 0x10000));
}

void initialize_case(Cpu& c) {
    population_state = {};
    c.s.memory.map(data, 0x40000); c.s.memory.map(stack - 0x10000, 0x20000);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x40000, 0xa5));
    c.s.memory.copy_in(stack - 0x10000, std::vector<std::uint8_t>(0x20000, 0x5a));
    for (unsigned reg = 0; reg < 32; ++reg) c.set(reg, 0x51000000 + reg * 0x101);
    c.set(1, stack); c.set(2, 0x108bba90); c.set_lr(sentinel);
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, field + 1);
    c.set_f(1, active.factor); c.set_f(2, -2.5);
    c.put32(trace_at, 0);
    if (active.entry == 0x10278220 && active.primary.list_items) {
        // Reserve a free arena so successful destruction restores both heap
        // tables exactly, including heap_end, rather than just removing owners.
        const auto arena = allocate(c.s, unsigned(active.primary.list_items) * 0x400, true);
        cm::semantic_io_checks::require(arena != 0, "Primary list arena allocation failed");
        deallocate(c.s, arena);
    }
    c.put32(player + 0x1a4, active.null_match ? 0 : match);
    c.put8(player + 0x27, active.side);
    c.put8(player + 0x105, active.column); c.put8(player + 0x106, active.row);
    c.put8(match + 0xe03d, active.row); c.put8(match + 0xe073, active.column);
    c.put8(match + 0x791e, active.row);
    c.put8(match + 0x7925, active.row); c.put8(match + 0x7926, active.row);
    c.put8(match + 0x791f, active.row);
    c.put8(player + 0x19, active.column); c.put8(player + 0x2c, active.row);
    c.put16(player + 0x19e, active.row); c.put16(player + 0x1c2, active.row);
    c.put8(player + 0x10b, 31); c.put8(player + 0x10d, 47);
    const bool geometry = active.entry == 0x1022e760 || active.entry == 0x10245160 ||
        active.entry == 0x10270ba0 || active.entry == 0x10245050 || active.entry == 0x10237830 || active.entry == 0x10281780 || active.entry == 0x10244c60 || active.entry == 0x10237420 || active.entry == 0x102775e0 || active.entry == 0x10281af0 || active.entry == 0x102836a0 || active.entry == 0x10278220;
    if (geometry) {
        const auto distances = data + 0x4000, directions = data + 0x8000;
        c.put32(c.r(2) - 0x6424, distances); c.put32(c.r(2) - 0x641c, directions);
        for (int offset = -2048; offset < 8192; offset += 4) c.put_f32(distances + std::uint32_t(offset), float(std::abs(offset) / 16.0));
        for (unsigned row = 0; row < 16; ++row) for (unsigned column = 0; column < 9; ++column)
            c.put_f32(distances + row * 36 + column * 4, float(std::hypot(double(row), double(column))));
        for (int column = -8; column <= 8; ++column) for (int row = -11; row <= 11; ++row) {
            auto angle = int(std::atan2(double(row), double(column)) * 180.0 / std::acos(-1.0));
            if (angle < 0) angle += 360;
            c.put16(directions + 0x1c8 + std::uint32_t(column * 48 + row * 2), angle);
        }
        c.put16(player + 0x19e, active.first_role); c.put8(player + 0x10f, active.strategy_high);
    }
    if (active.entry == 0x102865a0 || active.entry == 0x102875f0 || active.entry == 0x10235db0 ||
        active.entry == 0x10235ea0 || active.entry == 0x102393c0 || active.entry == 0x10239590 || active.entry == 0x10239790) {
        c.put_f32(player + 0x1a8, active.intensity); c.put_f32(player + 0x50, active.intensity);
        c.put32(player + 0x6c, data + 0xc000); c.put8(data + 0xc069, active.row);
        c.put8(player + 0x102, active.row); c.put32(player + 0x1ac, data + 0x9000);
    }
    if (active.entry == 0x10235ea0 || active.entry == 0x102393c0) {
        for (unsigned field = 0x111; field <= 0x11a; ++field)
            c.put8(player + field, active.column == 1 ? (field % 3 == 0 ? 0 : field % 3 == 1 ? active.row : -active.row) : active.row);
        c.put32(player + 0x70, data + 0xd000); c.put8(data + 0xd025, active.strategy_high);
        c.put8(player + 0x103, active.column); c.put8(player + 0x102, active.first_role);
        c.put8(player + 1, active.argument); c.put_f32(player + 0x5c, float(active.factor));
    }
    if (active.entry == 0x10239590) {
        c.put_f32(player + 0xd8, float(active.row));
        if (active.foreign_table) c.put32(c.toc(-0x5200) + (active.argument - 19) * 4, sentinel);
    }
    if (active.entry == 0x10239790) {
        c.put32(c.r(2) - 0x7ba0, data + 0x100); c.put32(data + 0x100, data + 0x200);
        c.put32(data + 0xc000, 12345);
        c.put32(player + 0x70, active.null_player ? 0 : data + 0xd000);
        c.put8(data + 0xd034, active.side);
        c.put8(player + 0x20, active.row); c.put8(player + 0x21, active.second_role);
        c.put8(player + 0x1f, active.mutate ? -1 : 0); c.put8(player + 0x1c, active.argument >> 8);
        c.put_f32(player + 0x54, active.intensity); c.put_f32(player + 0x5c, float(active.factor));
    }
    if (active.entry == 0x10235db0 || active.entry == 0x10226520 || active.entry == 0x10235ea0 || active.entry == 0x102393c0 || active.entry == 0x10237830 || active.entry == 0x10288230 || active.entry == 0x102857c0 || active.entry == 0x10288f30 || active.entry == 0x10244c60 || active.entry == 0x10270320 || active.entry == 0x10286650 || active.entry == 0x10289620 || active.entry == 0x102876c0 || active.entry == 0x102884d0 || active.entry == 0x10289d50 || active.entry == 0x102775e0 || active.entry == 0x10281af0 || active.entry == 0x102836a0 || active.entry == 0x10278220) {
        c.put32(c.r(2) - 0x7b90, data + 0x100);
        c.put32(data + 0x100, active.entry == 0x10237830 || active.entry == 0x102876c0 || active.entry == 0x102884d0 || active.entry == 0x10289d50 || active.entry == 0x102775e0 || active.entry == 0x10281af0 || active.entry == 0x102836a0 || active.entry == 0x10278220 ? active.random_seed : active.entry == 0x10244c60 ? 0 : active.second_role);
        c.put32(c.r(2) - 0x7b8c, data + 0x200);
        const auto span = active.entry == 0x102393c0 && active.mutate ? 0x3eu : 0x0eu;
        c.put32(c.r(2) - 0x7b88, data + 0x104); c.put32(data + 0x104, data + 0x200 + span);
        const auto position = active.entry == 0x10235db0 ? active.argument : active.entry == 0x10288230 ? active.strategy_high : active.entry == 0x10244c60 || active.entry == 0x10270320 || active.entry == 0x10286650 || active.entry == 0x10289620 || active.entry == 0x102876c0 || active.entry == 0x102884d0 || active.entry == 0x10289d50 || active.entry == 0x102775e0 || active.entry == 0x10281af0 || active.entry == 0x102836a0 || active.entry == 0x10278220 ? active.pool_position : unsigned(active.side);
        const auto offset = position >= 3 ? (position - 2) * 2 : position & 1 ? span : position & 2 ? span - 2 : 0;
        c.put32(c.r(2) - 0x7b84, data + 0x108); c.put32(data + 0x108, data + 0x200 + offset);
        for (unsigned at = 0; at <= span; at += 2)
            c.put16(data + 0x200 + at, active.custom_pool ? active.pool_words[(at / 2) % active.pool_words.size()] :
                active.strategy_low + (active.nonuniform_pool ? (at / 2) * 7919 : 0));
    }
    if (active.entry == 0x10237830) {
        c.put32(player + 0x70, data + 0xd000); c.put8(data + 0xd030, 10);
        c.put8(data + 0xd03b, 10 + int(active.intensity));
        c.put8(player + 0x108, active.first_role); c.put8(player + 0x10f, active.strategy_high);
    }
    if (active.entry == 0x10281780) c.put8(player + 0x10f, active.strategy_low);
    if (active.entry == 0x102818f0) {
        c.put16(player + 0x19e, active.row); c.put32(player + 0x70, data + 0xd000);
        c.put8(data + 0xd030, active.column); c.put8(data + 0xd03b, active.strategy_high);
        c.put8(player + 0x2d, active.second_role); c.put_f32(player + 0xe8, active.intensity);
    }
    if (active.entry == 0x10285ef0) {
        const auto other = active.side == 0 ? 1u : 0u;
        const auto team = match + other * 0x18e0 + 0x7c6c;
        c.put32(team + 0x1880, active.strategy_low & 1 ? 0x10000000 : 0);
        c.put8(match + other + 0xaf, active.row); c.put8(player + 0x109, active.strategy_high);
        for (unsigned i = 0; i < 20; ++i) {
            const auto opponent = match + 0x311c + other * 0x2350 + i * 0x1c4;
            c.put8(opponent + 0x19, active.strategy_low & 2 ? -1 : 0);
            c.put32(opponent + 0x6c, i == unsigned(active.column) ? data + 0xc000 : 0);
            c.put32(opponent + 0x1a4, match);
        }
    }
    if (active.entry == 0x10227140) {
        c.put32(player + 0x6c, data + 0xc000); c.put32(player + 0x70, data + 0xd000);
        c.put32(match + 0x3118, data + 0x8000); c.put32(data + 0x801c, data + 0x9000); c.put32(data + 0x8020, data + 0xa000);
        c.put16(data + 0x91ec, active.row); c.put16(data + 0xa1ec, std::uint32_t(active.row) + 20);
        c.put16(data + 0xd00c, active.column); c.put8(data + 0xd02f, int(active.factor));
        c.put8(data + 0xe04c, active.strategy_low); c.put8(data + 0xe060, active.argument);
        c.put8(data + 0xc05e, int(active.intensity)); c.put32(data + 0xc04c, 111); c.put32(data + 0xc050, 222);
        c.put32(c.r(2) - 0x7b7c, data + 0x100); c.put32(data + 0x100, data + 0x200);
        c.put32(c.r(2) - 0x7b80, data + 0x300);
        c.put16(data + 0xd10c, 185); c.put8(data + 0xd12f, 10);
        c.put8(data + 0xc15e, 75); c.put32(data + 0xc14c, 333); c.put32(data + 0xc150, 444);
    }
    if (active.entry == 0x10288230) {
        c.put8(player + 0x2f, active.column); c.put32(match + 0x79f4, data + 0x9000);
        c.put32(match + 0x3118, data + 0x8000); c.put32(data + 0x801c, data + 0xa000); c.put32(data + 0x8020, data + 0xb000);
        c.put32(data + 0xa02c, active.argument ? 0 : data + 0xe000); c.put32(data + 0xb02c, active.argument ? 0 : data + 0xe000);
        c.put8(data + 0xe119, active.row);
    }
    if (active.entry == 0x102857c0) {
        c.put32(player + 0x70, data + 0xd000); c.put8(player + 0x10d, active.column);
        c.put8(data + 0xd018, active.row);
        c.put8(data + 0xd019, active.row + (active.nonuniform_pool ? 7 : 0));
        c.put8(data + 0xd017, active.row - (active.nonuniform_pool ? 3 : 0));
    }
    if (active.entry == 0x10288f30) {
        c.put8(player + 0x17, active.column); c.put16(player + 0x36, active.row);
        c.put32(match + 0x3100, active.strategy_high & 1 ? UINT32_MAX : 0);
    }
    if (active.entry == 0x10244c60) c.put8(player + 0x10e, active.random_seed);
    if (active.entry == 0x10270320) {
        c.put32(player + 0x70, data + 0xd000); c.put8(data + 0xd021, active.row);
        c.put32(player + 0x6c, data + 0xc000); c.put8(data + 0xc066, active.first_role);
        c.put_f32(player + 0xa8, active.intensity); c.put8(player + 0x109, active.strategy_high);
        c.put8(player + 0x17, 10); c.put16(player + 0x36, 7600);
        c.put32(match + 0x3100, 0); c.put32(match + 0x79f4, data + 0x9000); c.put8(player + 0x2f, 7);
    }
    if (active.entry == 0x102876c0) {
        constexpr auto peer = data + 0x9000, ineligible = data + 0xa000;
        c.put8(player + 0x105, 4); c.put8(player + 0x106, 5);
        c.put32(player + 0x6c, data + 0xc000); c.put32(data + 0xc00c, data + 0xc100);
        c.put8(data + 0xc219, active.strategy_low); c.put32(player + 0x70, data + 0xd000);
        c.put32(player + 0x1ac, active.strategy_high & 4 ? peer : 0);
        c.put16(player + 0x19c, active.second_role); c.put_f32(player + 0x1a8, active.intensity);
        c.put32(peer + 0x1a4, match); c.put8(peer + 0x27, 1 - active.side);
        c.put8(peer + 0x105, 4); c.put8(peer + 0x106, 5); c.put8(peer + 0x2f, 8);
        c.put32(peer + 0x6c, active.strategy_high & 8 ? 0 : data + 0xe000);
        c.put32(peer + 0x70, active.strategy_high & 16 ? 0 : data + 0xe100);
        c.put32(data + 0xe00c, data + 0xc100); c.put8(data + 0xe069, active.column);
        c.put_f32(peer + 0x1a8, float(active.factor)); c.put32(peer + 0x1ac, 0);
        c.put32(ineligible + 0x6c, 0);
        const auto cell = match + 0xa08 + 4 * 0x450 + 5 * 0x5c;
        const auto count = (active.strategy_high >> 8) & 3;
        c.put8(cell + 0x58 + active.side, 1); c.put32(cell + active.side * 0x2c, player);
        c.put8(cell + 0x58 + (active.side == 0), count);
        for (unsigned i = 0; i < count; ++i)
            c.put32(cell + (active.side == 0) * 0x2c + i * 4,
                    (active.strategy_high & 32 ? i == 0 : i == count - 1) ? peer : i == 0 ? 0 : ineligible);
        c.put8(match + 0x1bc, active.row); c.put32(match + 0x79f4, data + 0x8000);
        for (int i = 0; i < active.row; ++i) {
            c.put32(match + 0xb4 + i * 12, active.strategy_high & 2 ? player : peer);
            c.put32(match + 0xb8 + i * 12, peer); c.put8(match + 0xbc + i * 12, 250);
        }
        for (unsigned team = 0; team < 2; ++team) for (unsigned slot = 0; slot < 20; ++slot) {
            const auto record = match + 0x311c + team * 0x2350 + slot * 0x1c4;
            c.put8(record + 0x19, 255); c.put8(record + 0x2c, 0); c.put8(record + 0x27, team);
            c.put32(record + 0x1a4, match); c.put_f32(record + 0x1a8, 100);
            c.put8(record + 0x105, 4); c.put8(record + 0x106, 5);
            c.put32(record + 0x6c, 0); c.put32(record + 0x70, 0);
            if (active.strategy_high & 64) {
                if (slot == 1 || slot == 4) {
                    c.put8(record + 0x19, 4); c.put_f32(record + 0x1a8, slot == 1 ? 110 : 100);
                    c.put8(record + 0x28, 255); c.put8(record + 0x105, 3); c.put8(record + 0x106, 4);
                }
                if (slot == 2) c.put8(record + 0x19, 11);
                if (slot == 3) { c.put8(record + 0x19, 4); c.put8(record + 0x2c, 2); }
            }
        }
        if (active.strategy_high & 128) {
            const auto root = match + 0x311c;
            c.s.memory.copy_in(root, c.s.memory.copy_out(player, 0x1c4));
            c.put8(root + 0x19, 4); c.put8(root + 0x2c, 0);
            c.put32(cell + active.side * 0x2c, root);
        }
    }
    if (active.entry == 0x102884d0) {
        const auto scene = data + 0xe000, reference = data + 0xe100, node = data + 0xe200;
        c.put_f32(player + 0x1a8, active.intensity); c.put_f32(player + 0x78, float(active.factor));
        c.put32(player + 0x1ac, 0); c.put8(player + 0x2f, 7);
        c.put32(match + 0x79f4, data + 0x8000); c.put32(match + 0x3118, scene);
        c.put8(match + 0x7924, active.strategy_high & 1 ? unsigned(active.side == 0) : unsigned(active.side));
        c.put8(match + 0x791d, 4); c.put8(match + 0x791e, active.row); c.put8(match + 0x7928, active.strategy_low);
        c.put32(scene + 0x14, active.strategy_high & 2 ? 0 : reference);
        c.put32(reference + 8, active.strategy_high & 4 ? 0 : node);
        c.put32(node + 0x68, active.strategy_high & 8 ? 12346 : 12345);
        c.put32(c.r(2) - 0x7a10, data + 0x700); c.put32(data + 0x700, 12345);
        c.put32(scene + 0x1c, data + 0xe400); c.put32(scene + 0x20, data + 0xe500);
        c.put32(data + 0xe42c, active.strategy_high & 16 ? 0 : data + 0xe600);
        c.put32(data + 0xe52c, active.strategy_high & 16 ? 0 : data + 0xe700);
        c.put8(data + 0xe719, active.column); c.put8(data + 0xe819, active.column);
    }
    if (active.entry == 0x10281af0) initialize_selection(c, active.secondary);
    if (active.entry == 0x102836a0) initialize_context(c, active.context);
    if (active.entry == 0x10278220) {
        if (active.primary.restart == 0 || active.primary.restart == 1 || active.primary.restart == 8) {
            initialize_context(c, active.primary);
            c.put32(player + 0x6c, data + 0xc000); c.put8(data + 0xc067, active.primary.staff67); c.put8(data + 0xc068, active.primary.staff68);
            c.put8(player + 0x11, 0); c.put8(player + 0x17, active.primary.event_count); c.put8(player + 0x2d, active.primary.turn_state); c.put8(player + 0x2e, active.primary.run_trait);
            c.put16(player + 0x19c, active.primary.action); c.put16(player + 0x1a0, active.primary.condition_penalty);
            c.put16(player + 0x19e, active.primary.heading);
            for (auto offset : {0x98u, 0x9cu, 0xa0u, 0xacu, 0xb0u, 0xb4u, 0xe4u, 0xe8u}) c.put_f32(player + offset, active.primary.attribute);
            if (active.primary.custom_attributes) { c.put_f32(player + 0x98, active.primary.attribute98); c.put_f32(player + 0x9c, active.primary.attribute9c); }
            c.put_f32(player + 0xe4, active.primary.bound_e4);
            c.put32(data + 0xf1a4, match); c.put32(data + 0xf06c, data + 0xc000); c.put32(data + 0xf070, data + 0xd000);
            c.put8(data + 0xf027, 1 - active.side); c.put8(data + 0xf105, active.primary.keeper_column); c.put8(data + 0xf106, active.primary.keeper_row == INT32_MIN ? active.side == 0 ? 11 : 0 : active.primary.keeper_row);
            c.put8(data + 0xf019, 4); c.put16(data + 0xf19e, 0); c.put8(data + 0xf10f, 10);
            c.put8(match + 0x1c0, active.primary.publish); c.put32(match + 0xe024, active.primary.previous_peer && !active.primary.previous_null ? data + 0x9000 : 0); c.put32(match + 0xe028, active.primary.previous_peer ? player : 0);
            c.put16(data + 0x919c, active.primary.previous_action); c.put16(data + 0x9036, 6000);
            c.put32(match + 0x3108, 0); c.put16(match + 0x7948, active.primary.rating); c.put8(match + 0x1c4 + active.side, active.primary.tactic);
            c.put32(match + 0x81f4 + unsigned(active.side == 0) * 0x18e0, active.primary.opposing_flags);
            for (unsigned index = 0; index < 2; ++index) {
                const auto peer = data + 0xa400 + index * 0x200;
                c.s.memory.copy_in(peer, c.s.memory.copy_out(player, 0x1c4));
                c.put8(peer + 0x27, 1 - active.side); c.put8(peer + 0x24, index);
                if (active.primary.actual_population) c.put8(peer + 0x10d, 20);
            }
            const auto cell = match + 0xa08 + unsigned(active.column * 0x450 + active.row * 0x5c);
            for (unsigned side = 0; side < 2; ++side) for (unsigned index = 0; index < 11; ++index)
                c.put32(cell + side * 0x2c + index * 4, data + 0xa400 + (index % 2) * 0x200);
        } else {
            // Restart cases now reach the real ball core through pass helpers.
            // Supply the staff/attribute records that their former modeled
            // boundary did not read, preserving restart and geometry inputs.
            c.put32(player + 0x6c, data + 0xc000); c.put32(player + 0x70, data + 0xd000);
            c.put32(player + 0xfc, data + 0xb000); c.put8(data + 0xc066, 0);
            c.put_f32(player + 0x50, 50); c.put_f32(player + 0xc8, 20);
            c.put16(player + 0x32, 0); c.put16(player + 0x1a0, 0); c.put16(player + 0x2a, 0);
            c.put8(player + 0x17, 1); c.put16(player + 0x36, 6000);
            for (auto offset : {0x12au, 0x12cu, 0x12eu, 0x130u, 0x134u, 0x15au, 0x15cu, 0x15eu, 0x186u, 0x18au, 0x198u})
                c.put16(player + offset, 50);
        }
        c.put8(match + 0x7928, active.primary.restart); c.put8(match + 0x791d, active.primary.ball_column); c.put8(match + 0x791e, active.primary.ball_row);
        c.put8(match + 0x7925, active.primary.line0); c.put8(match + 0x7926, active.primary.line1);
        c.put8(player + 0x10c, active.primary.mobility); c.put_f32(player + 0xa4, active.primary.attribute_a4);
        if (active.primary.custom_distance) {
            const auto column = std::abs(active.column - 4), row = std::abs(active.row - (active.side == 0 ? 11 : 0));
            c.put_f32(c.toc(-0x6424) + unsigned(row * 36 + column * 4), active.primary.distance);
        }
    }
    if (active.entry == 0x102775e0) {
        initialize_context(c, active.context);
        // Low flags select main actor, secondary actor/side, phase5 and +2c.
        // High bytes are the two signed peer traits; strategy_high holds
        // signed match context and the signed +110 status in its low bytes.
        const auto flags = active.strategy_low;
        // The reconstructed secondary selector now runs as a real child of
        // dispatch_action; retain the same root cases with valid dependencies.
        c.put32(data + 0x314, 0); c.put32(player + 0x70, data + 0xd000); c.put32(player + 0x1b0, 0);
        for (auto field : {0x17u, 0x18u, 0x19u, 0x1au}) c.put8(data + 0xd000 + field, 0);
        c.put8(player + 0x19, 4); c.put8(player + 0x10c, 0); c.put8(player + 0x10d, 0);
        c.put_f32(player + 0xe4, 20); c.put8(match + 0x1de, 0);
        c.put32(match + 0x81f4 + unsigned(active.side) * 0x18e0, 0);
        c.put32(match + 0xe000, flags & 1 ? player : 0);
        c.put32(match + 0xdff8, flags & 2 ? data + 0x9000 : 0);
        c.put8(data + 0x9027, flags & 4 ? 1 - active.side : active.side);
        // All owned selectors run as actual children, including primary.
        for (auto actor : {player, data + 0x9000, data + 0x91c4, data + 0x9388, data + 0xa800, data + 0xaa00}) {
            c.put32(actor + 0x6c, data + 0xc000); c.put32(actor + 0xfc, data + 0xb000);
            c.put_f32(actor + 0xc8, 20); c.put16(actor + 0x32, 0); c.put16(actor + 0x2a, 0);
            for (auto field : {0x98u, 0x9cu, 0xa0u, 0xacu, 0xb0u, 0xb4u, 0xe8u}) c.put_f32(actor + field, 10);
            c.put_f32(actor + 0xa4, 0); c.put8(actor + 0x11, 0); c.put8(actor + 0x17, 0); c.put8(actor + 0x2d, 0); c.put8(actor + 0x2e, 1);
            c.put16(actor + 0x1a0, 0);
        }
        c.put8(data + 0xc066, 0); c.put8(data + 0xc067, 10); c.put8(data + 0xc068, 10);
        c.put32(data + 0xf1a4, match); c.put32(data + 0xf06c, data + 0xc000); c.put32(data + 0xf070, data + 0xd000);
        c.put8(data + 0xf027, 1 - active.side); c.put8(data + 0xf105, 4); c.put8(data + 0xf106, active.side == 0 ? 11 : 0);
        c.put8(data + 0xf019, 4); c.put16(data + 0xf19e, 0); c.put8(data + 0xf10f, 10);
        c.put32(match + 0xe024, 0); c.put32(match + 0xe028, 0); c.put32(match + 0x3108, 0); c.put16(match + 0x7948, 0);
        if (active.context_flags & 2048) { active.primary.target = true; active.primary.query_score = 1000000; active.primary.query_kind = 7; }
        c.put8(match + 0x7924, active.strategy_high);
        c.put8(match + 0x791d, 4); c.put8(match + 0x791e, 5);
        c.put8(match + 0x791f, active.context_flags & 512 ? -1 : 0);
        c.put8(match + 0x7928, 0); c.put8(match + 0x1c0, 99);
        c.put8(match + 0x1c4 + std::uint32_t(std::int32_t(std::int8_t(active.side))), flags & 8 ? 5 : 0);
        c.put8(player + 0x2c, flags & 16 ? 1 : 0);
        c.put8(player + 0x104, active.argument); c.put8(player + 0x107, flags >> 16);
        c.put8(player + 0x10a, flags >> 24); c.put8(player + 0x110, active.strategy_high >> 8);
        c.put_f32(player + 0x50, active.intensity); c.put_f32(player + 0x80, float(active.factor));
        c.put16(player + 0x36, 6000); c.put16(player + 0x19c, 19);
        for (unsigned index = 0; index < 127; ++index)
            c.put32(player + 0x1b4 + index * 4, data + 0x9000 + index * 0x1c4);
        c.s.memory.copy_in(other_match, c.s.memory.copy_out(match, 0x10000));
        c.put32(other_match + 0xe000, 0); c.put8(other_match + 0x791f, 0);
        // Each pending decision target is now a real MATCH_PLAYER record.
        // Replacement targets do not overlap the original three peers.
        for (auto peer : {data + 0x9000, data + 0x91c4, data + 0x9388, data + 0xa800, data + 0xaa00}) {
            c.put32(peer + 0x1a4, match); c.put32(peer + 0x70, data + 0xd000); c.put32(peer + 0x6c, data + 0xc000);
            c.put8(peer + 0x19, 4); c.put8(peer + 0x27, 1 - active.side); c.put8(peer + 0x28, 255); c.put8(peer + 0x2f, 7);
            c.put8(peer + 0x105, 0); c.put8(peer + 0x106, 0); c.put8(peer + 0x104, 0);
            for (auto field : {0x108u, 0x109u, 0x10au, 0x10cu, 0x10du, 0x10eu, 0x110u, 0x2cu, 0x2du}) c.put8(peer + field, 0);
            c.put8(peer + 0x10f, 10); c.put16(peer + 0x36, 6000); c.put16(peer + 0x19c, 19); c.put16(peer + 0x19e, 0);
            c.put32(peer + 0x1b0, 0); c.put32(peer + 0x1ac, 0);
            c.put_f32(peer + 0xe0, 20); c.put_f32(peer + 0xe4, 20); c.put_f32(peer + 0x50, 50); c.put_f32(peer + 0x1a8, 50);
        }
        c.put8(data + 0x9027, flags & 4 ? 1 - active.side : active.side);
    }
    if (active.entry == 0x10289d50) {
        const auto flags = active.strategy_low;
        c.put32(c.r(2) - 0x7b80, data + 0x800); c.put32(c.r(2) - 0x7b94, data + 0x900);
        c.put32(data + 0x900, data + 0xa000); c.put32(c.r(2) - 0x7be4, data + 0xa00); c.put32(data + 0xa00, 100);
        c.put32(player + 0x6c, flags & 32 ? 0 : data + 0xc000);
        c.put32(data + 0xc000, flags & 64 ? 100 : 1); c.put32(data + 0xc200, 2);
        c.put32(data + 0xc04c, 0x11223344); c.put32(data + 0xc050, 0x55667788);
        c.put32(match + 0x3118, data + 0xe000);
        c.put32(data + 0xe01c, data + 0xb000); c.put32(data + 0xe020, data + 0xb100);
        for (unsigned team = 0; team < 3; ++team) c.put32(data + 0xb02c + team * 0x100, data + 0xb800 + team * 0x100);
        c.put32(player + 0xfc, data + 0xb000 + active.side * 0x100); c.put32(player + 0x70, data + 0xd000);
        c.put8(data + 0xd011, int(active.intensity)); c.put8(data + 0xd111, 0);
        c.put16(player + 0x36, int(active.factor)); c.put16(player + 0x3a, active.context_flags & 2 ? 250 : 251);
        c.put8(player + 0xc, flags & 1 ? 1 : 0); c.put8(player + 0x10, flags & 2 ? 1 : 0);
        c.put8(player + 3, flags & 4 ? 3 : 0);
        c.put8(player + 0x31, active.context_flags & 1024 ? 255 : flags & 8 ? 1 : 0);
        c.put8(player + 0x17, flags >> 8); c.put8(player + 0x14, flags >> 16); c.put8(player + 0xa, flags >> 24);
        c.put8(player + 0x1f, active.column); c.put16(match + 0x7948, active.row);
        c.put32(match + 0xe028, flags & 128 ? 0 : player);
        c.put32(match + 0xe024, active.context_flags & 1 ? 0 : data + 0x9000);
        const auto event = std::int16_t(active.argument);
        if (event >= 8000 && event <= 8676)
            c.put8(match + std::uint32_t(std::int8_t(active.side) * 1000 + event - 7432), flags & 16 ? 1 : 0);
    }
    if (active.entry == 0x10289620) {
        c.put16(player + 0x3c, active.row);
        if (active.foreign_table) c.put_f64(c.toc(-0x51e8) + 0x3e8, active.factor);
    }
    if (active.entry == 0x10286650) {
        c.put_f32(player + 0x1a8, active.intensity); c.put32(player + 0x1ac, data + 0x9000);
        c.set_f(31, -3.5);
        const auto kind = std::int8_t(active.argument);
        if (active.foreign_table && kind >= 2 && kind <= 5)
            c.put_f64(c.toc(-0x51e8) + 0x3b0 + unsigned(kind - 2) * 8, active.factor);
    }
    if (active.entry == 0x102374f0) {
        c.put32(c.r(2) - 0x7bac, data + 0x400); c.put32(c.r(2) - 0x7ba8, data + 0x500);
        const auto cell = [&](int x, int y) { return match + 0xa08 + std::uint32_t(x * 0x450 + y * 0x5c); };
        const auto previous = cell(active.column, active.row), destination = cell(std::int8_t(active.argument), std::int8_t(active.first_role));
        c.put8(previous + 0x58 + active.side, active.strategy_low & 255);
        c.put8(destination + 0x58 + active.side, active.strategy_low >> 8);
        c.put8(player + 0x28, active.strategy_high);
        c.put32(match + 0xe000, active.mutate ? player : 0);
        for (unsigned i = 0; i < 11; ++i) c.put32(previous + active.side * 0x2c + i * 4, data + 0x3000 + i * 0x100);
        if (active.null_player) c.put32(previous + active.side * 0x2c + ((active.strategy_low & 255) - 1) * 4, 0);
        if (active.last_is_player) c.put32(previous + active.side * 0x2c + ((active.strategy_low & 255) - 1) * 4, player);
    }
    if (active.entry == 0x10239e50 && std::int8_t(active.argument) != 0) {
        const auto target_row = std::int8_t(active.row + (active.side == 1 ? 1 : -1));
        const auto destination = match + 0xa08 + std::uint32_t(4 * 0x450 + target_row * 0x5c);
        c.put8(player + 0x28, -1);
        c.put8(destination + 0x58 + std::uint32_t(std::int32_t(std::int8_t(active.side))), 0);
    }
    if (active.entry == 0x102775c0) c.put32(match + 0xe000, active.argument ? player : 0);
    if (active.entry == 0x102864f0) {
        const auto team = std::uint32_t(std::int32_t(std::int8_t(active.side)));
        c.put32(match + 0x81f4 + team * 0x18e0, active.argument & 1 ? 0x1000 : 0);
        c.put8(match + team + 6, active.row);
    }
    if (active.entry == 0x10283540 || active.entry == 0x10283590) {
        c.put8(player + 0x104, active.row);
        for (unsigned index = 0; index < 127; ++index) c.put32(player + 0x1b4 + index * 4, index == unsigned(active.column) ? active.argument : data + 0x5000 + index * 4);
    }
    if (active.entry == 0x1022e3a0) {
        const auto flags = active.argument;
        const auto previous = data + 0x3000, receiver = data + 0x5000;
        c.put32(previous + 0x1a4, match);
        c.put32(match + 0xe000, flags & 1 ? previous : 0);
        c.put32(match + 0xdff4, flags & 2 ? receiver : 0);
        c.put8(receiver + 0x27, flags & 4 ? 1 - active.side : active.side);
        c.put8(match + 0x1be + std::uint32_t(std::int32_t(std::int8_t(active.side))), flags & 8 ? 1 : 0);
        c.put8(match + 0x77bc, flags & 16 ? 40 : 0);
        c.put8(match + 0x77bd, flags & 16 ? 40 : 0);
        c.put8(match + 0x7921, flags & 32 ? active.side : 1 - active.side);
        if (flags & 64) c.put32(match + 0xe000, player);
        c.put8(receiver + 2, flags & 128 ? 255 : 1);
        c.put16(receiver + 0x36, flags & 128 ? 32760 : 6000);
        if (flags & 256) c.put8(match + 0x77bc + std::uint32_t(active.side == 0), active.second_role);
        c.s.memory.copy_in(other_match, c.s.memory.copy_out(match, 0x10000));
    }
    if (active.entry == 0x102238a0 && active.argument == 1) {
        c.put32(c.r(2) - 0x51e0, player + 0x70); c.put32(player + 0x70, 0x3f800000);
    }
    c.set(3, active.null_player && active.entry != 0x10239790 ? 0 : active.entry == 0x102238a0 && active.argument == 2 ? stack - 0x180 : player);
    c.set(4, active.argument);
    c.set(5, std::uint32_t(active.column)); c.set(6, std::uint32_t(active.row));
    if (active.entry == 0x10239dc0) c.set(4, std::uint32_t(active.side));
    if (active.entry == 0x102373a0) c.set(3, std::uint32_t(active.row));
    if (geometry) c.set(5, active.second_role);
    if (active.entry == 0x10226520) c.set(3, active.argument);
    if (active.entry == 0x10239590) c.set(5, active.argument);
    if (active.entry == 0x10237830) { c.set(5, std::uint32_t(active.row)); c.set(6, std::uint32_t(active.column)); c.set(7, active.second_role); }
    if (active.entry == 0x102374f0) { c.set(3, player); c.set(5, active.first_role); }
    if (active.entry == 0x10227140) c.set(3, player);
    if (active.entry == 0x10281780) c.set(5, active.second_role);
    if (active.entry == 0x102818f0) c.set(10, data + 0x300);
    if (active.entry == 0x10244c60) {
        c.set(5, active.second_role); c.set(6, active.side);
        c.set(9, active.no_feedback ? 0 : active.feedback_alias ? player + 0x10e : data + 0x300);
        c.set_f(2, active.intensity);
    }
    if (active.entry == 0x10286650) c.set(5, active.strategy_high == 0 ? 0 : active.strategy_high == 1 ? data + 0x9000 : data + 0x9004);
    if (active.entry == 0x102876c0 && (active.strategy_high & 128)) c.set(3, match + 0x311c);
    if ((active.mutate || active.mutate_after_step || (active.entry == 0x10289d50 && (active.context_flags & 8))) && active.entry != 0x1022e3a0) c.s.memory.copy_in(other_match, c.s.memory.copy_out(match, 0x10000));
    if (active.entry == 0x10289d50 && (active.context_flags & 8)) c.put16(other_match + 0x7948, active.row + 110);
    if (active.entry == 0x102836a0 && active.mutate) c.put8(other_match + 0x791e, (active.context.ball_row + 1) % 12);
    if (action_entry(active.entry) || core_entry(active.entry)) initialize_actions(c);
}

void require_return(const State& s) {
    cm::semantic_io_checks::require(s.pc == sentinel && s.r<4>(4) == stack,
                                   "MATCH_PL comparison did not return normally");
    if (active.action.defender_restart_after_query != INT32_MIN)
        cm::semantic_io_checks::require(s.memory.read<4>(data + 0x32c) == 1, "Post-core defender restart mutation did not execute");
    if (core_entry(active.entry) && active.action.core_queue) {
        constexpr auto list = data + 0x10000 - 0x100;
        const auto entries = unsigned(active.action.core_queue_entries);
        cm::semantic_io_checks::require(s.memory.read<2>(list) == entries && s.memory.read<4>(list + 12) == 0,
                                       "Core disciplinary queue count/cursor differs");
        if (entries) {
            const auto node = std::uint32_t(s.memory.read<4>(list + 4)), payload = std::uint32_t(s.memory.read<4>(node));
            cm::semantic_io_checks::require(s.memory.read<4>(list + 8) == node && s.memory.read<2>(node + 4) == 28 &&
                s.memory.read<4>(node + 8) == 0 && s.memory.read<4>(node + 12) == 0 &&
                s.allocated.contains(node) && s.allocated.contains(payload), "Core disciplinary queue ownership differs");
            const std::vector<std::uint8_t> expected{
                0,0,0,1, 0,0,0,2, 0,0,0,0, 0,std::uint8_t(active.side),0,0x5a,
                0xff,0xff,0xff,0xff, 0,0x82,0x5a,0x5a, 0,0,0,0};
            cm::semantic_io_checks::require(s.memory.copy_out(payload, 28) == expected,
                                           "Core disciplinary queue payload/padding differs");
        } else cm::semantic_io_checks::require(s.memory.read<4>(list + 4) == 0 && s.memory.read<4>(list + 8) == 0,
                                              "Core disciplinary queue gate appended unexpectedly");
    }
    if (core_entry(active.entry) && active.mutate_after_step)
        cm::semantic_io_checks::require(s.memory.read<4>(data + 0x31c) == 1, "Core commentary state mutation did not execute");
    if (core_entry(active.entry) && active.action.core_phase_after_tally != INT32_MIN)
        cm::semantic_io_checks::require(s.memory.read<4>(data + 0x320) == 1, "Core tally phase mutation did not execute");
    if (core_entry(active.entry) && active.action.core_null_match_at_profile)
        cm::semantic_io_checks::require(s.memory.read<4>(data + 0x324) == 2 && s.memory.read<4>(player + 0x1a4) == match,
                                       "Core temporary null match was not restored at the reseed boundary");
    if (active.entry == 0x102836a0 && active.context.coefficient_back)
        cm::semantic_io_checks::require(s.memory.read<4>(data + 0x31c) != 0, "MATCH_PL coefficient mutation boundary was not exercised");
    if (active.entry == 0x10278220 && (active.primary.list_items || active.primary.actual_population)) {
        cm::semantic_io_checks::require(population_state.exercised &&
            s.heap_end == population_state.heap_end && s.allocated == population_state.allocated && s.available == population_state.available,
            "Primary candidate-list ownership was not completely released");
        cm::semantic_io_checks::require(s.memory.read<2>(population_state.list) == unsigned(active.primary.list_items) &&
            s.memory.read<4>(population_state.list + 4) == 0 && s.memory.read<4>(population_state.list + 8) == population_state.tail &&
            s.memory.read<4>(population_state.list + 12) == 0,
            "Primary candidate-list destructor fields differ from the original contract");
    }
}
template<class Action> void returned_only(Action action, const State* state = nullptr) {
    try { action(); }
    catch (const std::exception& error) {
        if (state && (active.entry == 0x10278220 || active.entry == 0x102775e0 || action_entry(active.entry) || core_entry(active.entry))) throw std::runtime_error(std::string(error.what()) + " pc=" + hex_address(state->pc) + " lr=" + hex_address(std::uint32_t(state->r<4>(4128))));
        throw;
    }
    catch (...) { throw std::runtime_error("MATCH_PL comparison left through non-returning control flow"); }
}
void original(State& s, std::uint32_t entry) {
    ActionReads reads(s);
    reference_mode = true;
    returned_only([&] { s.pc = entry; run_until(s, sentinel); require_return(s); }, &s);
    reads.require_reads();
}
void rewrite(State& s, std::uint32_t entry) {
    ActionReads reads(s);
    reference_mode = false;
    returned_only([&] {
        if (generated_mode) { s.pc = entry; run_until(s, sentinel); }
        else match_pl_run_rewrite(s, entry);
        require_return(s);
    }, &s);
    reads.require_reads();
#if defined(CM_MATCH_PL_COVERAGE)
    __llvm_profile_write_file();
#endif
}
void corrupt(State& s, std::uint32_t entry) {
    rewrite(s, entry); Cpu c(s); c.set(3, c.r(3) ^ 1);
}
void corrupt_memory(State& s, std::uint32_t entry) {
    rewrite(s, entry); Cpu c(s); c.put8(player + 0x19, c.u8(player + 0x19) ^ 1);
}
void corrupt_core_rng(State& s, std::uint32_t entry) {
    rewrite(s, entry); Cpu c(s); const auto seed_cell = c.toc(-0x7b90); c.put32(seed_cell, c.u32(seed_cell) ^ 1);
}
void explain_primary_trace(const State& prepared) {
    auto a = cm::semantic_io_checks::clone(prepared), b = cm::semantic_io_checks::clone(prepared);
    try { original(*a, active.entry); rewrite(*b, active.entry); }
    catch (const std::exception&) { return; }
    Cpu expected(*a), actual(*b);
    constexpr unsigned record_size = 77 * 4;
    const auto used = std::min({expected.u32(trace_at), actual.u32(trace_at), 0x8000u});
    for (unsigned offset = 0; offset + record_size <= used; offset += record_size) {
        const auto record = trace_at + 4 + offset;
        for (unsigned slot = 0; slot < 77; ++slot) if (expected.u32(record + slot * 4) != actual.u32(record + slot * 4)) {
            std::cout << "  first service difference target=" << hex_address(expected.u32(record))
                      << " back=" << hex_address(expected.u32(record + 4)) << " slot=" << slot
                      << " original=" << hex_address(expected.u32(record + slot * 4))
                      << " rewrite=" << hex_address(actual.u32(record + slot * 4)) << '\n';
            return;
        }
    }
}

// These wrappers stop at the expected fatal service without changing guest
// PC/SP. They compare caller ABI, CR/FPSCR and memory separately from guest returns.
void expected_fatal_prefix(State& s, std::uint32_t entry, bool original_route) {
    reference_mode = original_route;
    bool exited = false;
    try {
        if (original_route || generated_mode) { s.pc = entry; run_until(s, sentinel); }
        else match_pl_run_rewrite(s, entry);
    } catch (const ApplicationExit&) { exited = true; }
    catch (const std::exception&) { throw; }
    catch (...) { throw std::runtime_error("Unexpected control flow at MATCH_PL fatal prefix"); }
    Cpu c(s);
    const auto size = core_entry(entry) ? 0x150u : 0x100u, back = core_entry(entry) ? 0x102505ecu : 0x10284ed8u;
    cm::semantic_io_checks::require(exited && s.pc == fatal_boundary && c.sp() == stack - size && c.lr() == back,
                                   "MATCH_PL fatal prefix did not reach the expected ApplicationExit boundary");
#if defined(CM_MATCH_PL_COVERAGE)
    if (!original_route) __llvm_profile_write_file();
#endif
}
void fatal_original(State& s, std::uint32_t entry) { expected_fatal_prefix(s, entry, true); }
void fatal_rewrite(State& s, std::uint32_t entry) { expected_fatal_prefix(s, entry, false); }
void corrupt_fatal_prefix(State& s, std::uint32_t entry) { fatal_rewrite(s, entry); Cpu c(s); c.set(3, c.r(3) ^ 1); }
Case fatal_case(int side) {
    Case test{0x102836a0}; test.side = side; test.row = side == 0 ? 0 : 11;
    test.first_role = 0; test.strategy_low = 0; test.strategy_high = 0;
    test.factor = 20; test.custom_pool = true; test.pool_words.fill(1);
    test.context.context_query = 1; test.context.keeper_null = true; test.context.expected_exit = true;
    return test;
}
Case core_fatal_case(int side, int kind = 32) {
    Case test{0x10247030, side, 4, 5, std::uint32_t(kind)};
    test.action.target = 0; test.action.receiver = 1; test.action.trajectory_result = 0; test.force_exit = true; return test;
}

std::uint64_t external_budget(std::uint32_t entry) {
    // Pass selection executes two original abs leaves (8 instructions), the
    // core's direction abs (4), and two original wide-column queries (16).
    // Its original-body negative control must still exhaust this allowance.
    return entry == 0x10278220 ? 128 : entry == 0x10245870 ? 32 : entry == 0x10281780 ? 12 : entry == 0x10247030 || entry == 0x10245220 || entry == 0x1022e760 || entry == 0x10245160 || entry == 0x10270ba0 ? 8 :
        entry == 0x1028ac50 || entry == 0x10245050 || entry == 0x10244c60 || entry == 0x10237420 ? 4 : 0;
}
Case route_case(std::uint32_t entry) {
    Case test{entry};
    if (entry == 0x102374f0) { test.strategy_high = UINT32_MAX; test.argument = 4; test.first_role = 6; }
    if (entry == 0x10244c60) { test.argument = 4; test.first_role = 1; test.second_role = 5; }
    if (entry == 0x10247030 || entry == 0x10245220) { test.argument = 54; test.action.target = 0; test.action.receiver = 0; }
    return test;
}

void require_production_routes(const State& base) {
    reference_mode = false;
    for (const auto entry : production_entries) {
        active = route_case(entry);
        auto state = cm::semantic_io_checks::clone(base); Cpu c(*state); initialize_case(c);
        if (action_entry(entry) || core_entry(entry) || entry == 0x10278220) {
            auto original_state = cm::semantic_io_checks::clone(base); Cpu original_cpu(*original_state); initialize_case(original_cpu);
            original_state->budget = external_budget(entry); original_state->pc = entry; bool exhausted = false;
            try { match_pl_run_original(*original_state, entry); }
            catch (const std::runtime_error& error) {
                if (std::string_view(error.what()) != "Native instruction budget exhausted") throw;
                exhausted = true;
            }
            cm::semantic_io_checks::require(exhausted, "Original action body fits the C++ route's external budget");
            std::cout << "ok original action budget rejected " << hex_address(entry) << '\n';
        }
        // Hooks precede original instruction accounting. An original fallback
        // cannot return with this budget; distance queries permit only their
        // four-instruction external abs dependencies. Primary also executes
        // actual external list construction/destruction. This is routing evidence,
        // not a requirement to preserve the original's diagnostic counts.
        state->budget = external_budget(entry);
        state->pc = entry;
        cm::semantic_io_checks::Effects observer;
        observer.state = state.get(); state->memory.date_observer = &observer;
        run_until(*state, sentinel);
        state->memory.date_observer = nullptr;
        require_return(*state);
        std::cout << "ok production C++ route " << hex_address(entry) << '\n';
    }
    // Legacy original/profile switches must still reach original instructions
    // after a family loses its production hook to MATCH_PL.
    const auto check_legacy = [&](const auto& entries, auto run) {
        for (const auto entry : entries) {
            if (std::ranges::find(production_entries, entry) == std::end(production_entries)) continue;
            active = route_case(entry);
            auto state = cm::semantic_io_checks::clone(base); Cpu c(*state); initialize_case(c);
            state->budget = external_budget(entry); state->pc = entry;
            bool exhausted = false;
            try { run(*state, entry); }
            catch (const std::runtime_error& error) {
                if (std::string_view(error.what()) != "Native instruction budget exhausted") throw;
                exhausted = true;
            }
            cm::semantic_io_checks::require(exhausted, "Legacy original switch entered MATCH_PL C++");
            auto restored = cm::semantic_io_checks::clone(base); Cpu next(*restored); initialize_case(next);
            restored->budget = external_budget(entry); restored->pc = entry;
            run_until(*restored, sentinel); require_return(*restored);
            std::cout << "ok legacy original route " << hex_address(entry) << '\n';
        }
    };
    check_legacy(actor_action::entries, [](State& s, std::uint32_t entry) { actor_action_diagnostics(s).original = true; try_modern_actor_action(s, entry); });
    check_legacy(actor_attributes::entries, [](State& s, std::uint32_t entry) { actor_attributes_diagnostics(s).original = true; try_modern_actor_attributes(s, entry); });
    check_legacy(actor_effects::entries, [](State& s, std::uint32_t entry) { actor_effects_diagnostics(s).original = true; try_modern_actor_effects(s, entry); });
    check_legacy(actor_encounter::entries, [](State& s, std::uint32_t entry) { actor_encounter_diagnostics(s).original = true; try_modern_actor_encounter(s, entry); });
    check_legacy(actor_flags::entries, [](State& s, std::uint32_t entry) { actor_flags_diagnostics(s).original = true; try_modern_actor_flags(s, entry); });
    check_legacy(actor_motion::entries, [](State& s, std::uint32_t entry) { actor_motion_diagnostics(s).original = true; try_modern_actor_motion(s, entry); });
    check_legacy(actor_policy::entries, [](State& s, std::uint32_t entry) { actor_policy_diagnostics(s).original = true; try_modern_actor_policy(s, entry); });
    check_legacy(marking_pressure::entries, [](State& s, std::uint32_t entry) { marking_pressure_diagnostics(s).original = true; try_modern_marking_pressure(s, entry); });
    check_legacy(match_selection_support::entries, [](State& s, std::uint32_t entry) { match_selection_support_diagnostics(s).original = true; try_modern_match_selection_support(s, entry); });
    check_legacy(match_target::entries, [](State& s, std::uint32_t entry) { match_target_diagnostics(s).original = true; try_modern_match_target(s, entry); });
    check_legacy(pitch_distance::entries, [](State& s, std::uint32_t entry) { pitch_distance_diagnostics(s).original = true; try_modern_pitch_distance(s, entry); });
    check_legacy(range_sampling::entries, [](State& s, std::uint32_t entry) { range_sampling_diagnostics(s).original = true; try_modern_range_sampling(s, entry); });
    check_legacy(spatial_role::entries, [](State& s, std::uint32_t entry) { spatial_role_diagnostics(s).original = true; try_modern_spatial_role(s, entry); });
    check_legacy(target_acceptance::entries, [](State& s, std::uint32_t entry) { target_acceptance_diagnostics(s).original = true; try_modern_target_acceptance(s, entry); });
    check_legacy(ball_contact::entries, [](State& s, std::uint32_t entry) { ball_contact_diagnostics(s).original = true; try_modern_ball_contact(s, entry); });
    check_legacy(player_decision::entries, [](State& s, std::uint32_t entry) { player_decision_diagnostics(s).original = true; try_modern_player_decision(s, entry); });
}

std::vector<Case> cases() {
    std::vector<Case> result;
    const std::array queries{0x10237b10u, 0x10239d10u, 0x10239dc0u, 0x10251240u,
        0x10272050u, 0x102720a0u, 0x102720d0u, 0x10272100u, 0x10272150u,
        0x102721a0u, 0x102721f0u, 0x10283620u};
    // Every signed-byte outcome is exercised; unrelated coordinates stay valid.
    for (auto entry : queries) for (int side : {0, 1, 2, 255}) {
        for (int value : {-128, -1, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 127}) {
            Case test{entry, side, 4, value, std::uint32_t(side)};
            if (entry == 0x102720a0 || entry == 0x102720d0) {
                test.column = value; test.argument = std::uint32_t(value);
            } else if (entry == 0x102721f0) test.argument = std::uint32_t(value);
            result.push_back(test);
        }
    }
    for (auto entry : {0x10239d10u, 0x10239dc0u})
        for (int side : {0, 1}) for (int column : {-128, -1, 0, 1, 2, 3, 6, 7, 127})
            for (int row : {-1, 0, 1, 2, 9, 10, 11, 12})
                result.push_back({entry, side, column, row, std::uint32_t(side)});
    for (auto entry : {0x102238a0u, 0x1022e710u}) result.push_back({entry});
    result.push_back({0x102238a0, 0, 4, 5, 1});
    result.push_back({0x102238a0, 0, 4, 5, 2});
    for (int release : {-32768, -1, 0, 1, 32767, 65536}) for (bool null : {false, true}) {
        Case test{0x102270f0}; test.argument = std::uint32_t(release); test.null_player = null;
        result.push_back(test);
    }
    for (int side : {0, 1, 2, 255}) for (int column : {2, 3, 5, 6}) for (int row : {0, 1, 11, 255})
        result.push_back({0x10237420, side, column, row});
    for (int side : {0, 1, 2, 255}) for (int row : {-128, -1, 0, 1, 10, 11, 12, 127})
        for (auto immediate : {0u, 1u, 255u, 256u}) result.push_back({0x10239e50, side, 4, row, immediate});
    for (std::uint32_t flags = 0; flags < 128; ++flags)
        result.push_back({0x1028acf0, 0, 4, 5, flags});
    for (auto entry : {0x1028abc0u, 0x1028ac50u})
        for (auto first : {0u, 1u, 4u, 8u, 16u, 32u, 64u, 0x80u, 0x800u, 0x880u, UINT32_MAX})
            for (auto second : {0u, 1u, 4u, 8u, 16u, 32u, 64u, 0x800u, 0x880u}) {
                Case test{entry}; test.first_role = first; test.second_role = second;
                result.push_back(test);
                test.mutate = true; result.push_back(test);
            }
    for (auto entry : {0x1028abc0u, 0x1028ac50u}) { Case test{entry}; test.null_match = true; result.push_back(test); }
    for (int side : {0, 1, 255}) for (int row : {0, 1, 5, 6, 10, 11})
        for (auto flags : {0u, 1u, 2u, 3u, 6u, 10u, 18u, 34u})
            for (auto role : {1u, 4u}) {
                Case test{0x1022e3a0, side, 4, row, flags}; test.first_role = role; result.push_back(test);
            }
    for (int side : {0, 1}) for (auto flags : {2u, 3u, 66u, 130u}) {
        Case test{0x1022e3a0, side, 4, 5, flags};
        test.mutate = true; result.push_back(test);
    }
    for (auto count : {0u, 1u, 9u, 10u, 127u, 128u, 255u}) {
        Case test{0x1022e3a0, 0, 4, 5, 256}; test.second_role = count; result.push_back(test);
    }
    for (auto entry : {0x102373a0u, 0x10237b00u, 0x10283690u, 0x10285790u,
                       0x10285ed0u, 0x10285ee0u, 0x10289550u})
        for (int value : {-32768, -129, -128, -1, 0, 1, 127, 128, 255, 256, 32767}) {
            Case test{entry, value, 4, value, std::uint32_t(value + 1)}; result.push_back(test);
        }
    for (auto first : {INT32_MIN, -1, 0, 1, INT32_MAX}) for (auto second : {INT32_MIN, -1, 0, 1, INT32_MAX})
        result.push_back({0x102373a0, 0, 4, first, std::uint32_t(second)});
    for (auto entry : {0x102373c0u, 0x10285750u, 0x10289560u, 0x102895a0u, 0x102895e0u})
        for (bool absent : {false, true}) for (auto value : {0u, 1u, 2u, 255u, 0xffffu, UINT32_MAX}) {
            Case test{entry, 1, 19, 5, value, value, value};
            if (entry != 0x102373c0) test.null_match = absent;
            test.mutate = !absent; result.push_back(test);
        }
    for (auto value : {0u, 1u}) result.push_back({0x102775c0, 0, 4, 5, value});
    for (auto entry : {0x10283540u, 0x10283590u})
        for (int count : {-128, -1, 0, 1, 3, 127}) for (int found : {-1, 0, 1, 2, 126})
            result.push_back({entry, 0, found, count, data + 0x7000});
    for (auto entry : {0x10283540u, 0x10283590u}) result.push_back({entry, 0, 1, 3, data + 0x5000});
    for (auto entry : {0x102862b0u, 0x10286320u, 0x10286390u, 0x10286400u,
                       0x10286470u, 0x102864b0u, 0x102864f0u, 0x102860c0u, 0x102861b0u})
        for (auto mask : {0u, 1u << 17, 1u << 19, 1u << 21, 1u << 23, 1u << 25,
                          1u << 27, 1u << 29, 1u << 30, 1u << 31, UINT32_MAX})
            for (auto role : {0u, 1u, 2u}) for (int side : {0, 1}) {
                Case test{entry, side, 19, 255, 0, role}; test.strategy_low = mask;
                result.push_back(test);
                if (entry == 0x102864f0) { test.argument = 1; result.push_back(test); }
            }
    for (auto entry : {0x1022e760u, 0x10245160u, 0x10270ba0u, 0x10245050u})
        for (int side : {0, 1}) for (int column : {0, 4, 8}) for (int row : {0, 5, 11})
            for (auto target_row : {0u, 5u, 11u}) for (auto role : {0u, 90u, 180u, 359u}) {
                Case test{entry, side, column, row, 4, role, target_row};
                test.strategy_high = 10; result.push_back(test);
            }
    for (auto entry : {0x102865a0u, 0x102875f0u})
        for (int trait : {-128, -1, 0, 1, 24, 25, 100, 127})
            for (float intensity : {-100.0f, 0.0f, 1.0f, 49.0f, 50.0f, 100.0f, 200.0f})
                for (auto opponent : {0u, data + 0x9000, data + 0x9004}) {
                    Case test{entry, 0, 4, trait, opponent}; test.intensity = intensity;
                    result.push_back(test);
                }
    for (auto entry : {0x102865a0u, 0x102875f0u})
        for (auto value : {0.0f, -0.0f, std::numeric_limits<float>::denorm_min(),
                           std::numeric_limits<float>::max(), std::numeric_limits<float>::infinity(),
                           std::numeric_limits<float>::quiet_NaN()})
            for (auto factor : {0.0, -1.0, 2.0, std::numeric_limits<double>::infinity(),
                                std::numeric_limits<double>::quiet_NaN()}) {
                Case test{entry, 0, 4, 127, data + 0x9000}; test.intensity = value; test.factor = factor;
                result.push_back(test);
            }
    for (int side : {0, 1}) for (auto position : {std::pair{0,0}, {8,11}, {1,5}, {7,6}, {4,5}, {4,10}})
        for (int trait : {0, 1, 19, 20, 21, 127}) for (float attribute : {-1.f, 0.f, 10.f, 100000.f, std::numeric_limits<float>::quiet_NaN()})
            for (int opponents : {0, 1, 2}) {
                Case test{0x10278220, side, position.first, position.second}; auto& in = test.primary;
                in.restart = 0; in.trait109 = trait; in.trait10e = trait; in.attribute = attribute; in.bound_e4 = attribute;
                in.own_count = 1; in.opposing_count = opponents; in.event_count = opponents ? -128 : 127;
                in.condition_penalty = opponents ? -10 : 10; in.mobility = opponents ? 10 : 0; in.keeper_role = 1; in.keeper_status = 1;
                in.keeper_column = position.first; in.keeper_row = position.second; in.team_shape = 2; in.neighbor_count = 1;
                in.list_items = 2; in.population = 1; in.population_count = 1; in.support = 1;
                in.offer = opponents & 1; in.query_kind = 7; in.target = opponents & 1; in.query_score = 2000;
                in.balance = 1; in.balance_outputs = {1, 5, 0, 0, 0}; in.policy_query = 1; in.edge_query = 1;
                test.first_role = opponents ? 0x8c1 : 0; test.second_role = opponents ? 0x880 : 0; test.strategy_high = UINT32_MAX;
                test.strategy_low = UINT32_MAX; test.pool_position = 7; result.push_back(test);
            }
    for (auto seed : {0u, 1u, 65535u, 0x7fffffffu, 0x80000000u, UINT32_MAX})
        for (auto value : {0u, 1u, 65535u}) for (auto wrap : {0u, 1u, 2u}) for (int pressure : {0, 1, -1}) {
            Case test{0x10235db0, 0, 4, pressure, wrap, 0x12345678, seed};
            test.strategy_low = value; result.push_back(test);
        }
    for (auto bound : {0u, 1u, 2u, 65535u, 65536u, 0xffff0001u, 0xffff0000u,
                        0x7fffffffu, 0x80000000u, UINT32_MAX, 0x33333333u, 0x1999999au})
        for (auto seed : {0u, 65535u, 0x7fffffffu, 0x80000000u, UINT32_MAX})
            for (auto word : {0u, 1u, 32768u, 65535u}) for (auto position : {0u, 1u, 2u}) {
                Case test{0x10226520, int(position), 4, 5, bound, 0x12345678, seed};
                test.strategy_low = word; result.push_back(test);
            }
    for (auto entry : {0x10235ea0u, 0x102393c0u})
        for (int trait : {-128, -13, -12, -11, -1, 0, 1, 25, 127}) for (int position : {0, 1, 2, 3})
            for (auto word : {0u, 1u, 11u, 65535u}) for (auto load : {0u, 50u, 51u, 255u}) {
                Case test{entry, position, trait & 1, trait, load, 0, 0};
                test.strategy_low = word; test.strategy_high = std::uint32_t(trait); result.push_back(test);
            }
    for (auto seed : {0x80000000u, 0x80000001u, 0xfffffff0u}) for (auto word : {0u, 1u, 11u, 65535u}) {
        Case test{0x102393c0, 0, 0, -12, 255, 0, seed}; test.mutate = true;
        test.strategy_low = word; test.factor = 1; result.push_back(test);
    }
    { Case test{0x102393c0}; test.strategy_low = 11; test.factor = 1; result.push_back(test); }
    for (auto entry : {0x10235ea0u, 0x102393c0u}) for (auto intensity : {0.0f, 10000.0f, -10000.0f,
                    std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
        Case test{entry, 1, 0, 25, 255, 0, 0}; test.intensity = intensity; result.push_back(test);
    }
    for (int action = -1; action <= 47; ++action)
        for (int resistance : {0, 50, 100}) for (float intensity : {-1.0f, 0.0f, 1.0f, 5000.0f, 8000.0f, 9000.0f, 10000.0f}) {
            Case test{0x10239590, 0, 4, resistance, std::uint32_t(action)}; test.intensity = intensity;
            result.push_back(test);
        }
    for (auto action : {UINT32_MAX, 0xffffu, 0x10000u, 19u, 30u, 46u, 47u})
        for (float condition : {0.0f, -0.0f, std::numeric_limits<float>::denorm_min(),
             std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity(),
             std::numeric_limits<float>::quiet_NaN()})
            for (double work : {0.0, std::numeric_limits<double>::denorm_min(),
                 std::numeric_limits<double>::infinity(), -std::numeric_limits<double>::infinity(),
                 std::numeric_limits<double>::quiet_NaN()}) {
                Case test{0x10239590, 0, 4, 100, action}; test.intensity = condition; test.factor = work;
                result.push_back(test);
            }
    for (int mode : {0, 1, 257}) for (int minute : {-1, 0, 45, 46, 64, 65, 90})
        for (auto role : {1u, 4u}) for (int slot : {-1, 4}) for (auto scale : {0u, 1000u, 1001u, 65535u}) {
            Case test{0x10239790, 25, slot, minute, std::uint32_t(mode), role, std::uint32_t(minute)};
            test.strategy_low = scale; test.strategy_high = 6400; test.intensity = 6200;
            result.push_back(test); test.mutate = true; result.push_back(test);
            if (minute >= 0) { test.row = -1; result.push_back(test); }
        }
    for (float value : {0.0f, -100.0f, 10000.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
        for (auto mode : {0u, 1u}) for (auto effective : {0u, 6500u, 6501u, 65535u}) {
            Case test{0x10239790, -128, 4, 50, mode, 4, 60}; test.intensity = value;
            test.strategy_low = 2000; test.strategy_high = effective; result.push_back(test);
        }
    { Case test{0x10239790}; test.null_player = true; result.push_back(test); }
    { Case test{0x10239790, 0, 4, 50, 1}; test.clear_staff_after_scale = true; test.strategy_low = 1000; result.push_back(test); }
    { Case test{0x10285750}; test.null_match = true; result.push_back(test); }
    { Case test{0x10285790}; test.null_match = true; result.push_back(test); }
    { Case test{0x10226520, 0, 4, 5, 1000000, 0, 0x80000000}; test.strategy_low = 123; result.push_back(test); }
    { Case test{0x10239590, 0, 4, 100, 19}; test.foreign_table = true; result.push_back(test); }
    for (const auto [x, y] : {std::pair{4, 11}, std::pair{4, 0}, std::pair{4, 6}, std::pair{5, 6}})
        for (auto trait : {0u, 20u, 255u}) for (int position : {0, 1, 2})
            for (auto seed : {0u, 65535u, 0x80000000u}) for (float bias : {-100.0f, 0.0f, 100.0f}) {
                Case test{0x10237830, position, x, y, 4, trait, std::uint32_t(y)};
                test.strategy_high = trait; test.second_role = std::uint32_t(y); test.strategy_low = seed & 65535;
                test.intensity = bias; test.random_seed = seed; result.push_back(test);
            }
    for (int side : {0, 1}) for (int member : {-1, 0, 1}) for (auto count : {0u, 1u, 2u, 11u})
        for (auto destination_count : {0u, 10u, 11u}) for (bool possession : {false, true}) {
            if (member >= 0 && count > 0 && unsigned(member) >= count) continue;
            Case test{0x102374f0, side, 4, 6, 5, 6}; test.strategy_low = count | (destination_count << 8);
            test.strategy_high = std::uint32_t(member); test.mutate = possession; result.push_back(test);
            if (member >= 0 && count > 0) { test.null_player = true; result.push_back(test); }
        }
    for (auto target : {std::pair{4, 6}, std::pair{4, 7}, std::pair{5, 6}}) {
        Case test{0x102374f0, 0, 4, 6, std::uint32_t(target.first), std::uint32_t(target.second)};
        test.strategy_low = 1; test.strategy_high = 0; test.last_is_player = true; result.push_back(test);
    }
    for (int side : {0, 1}) for (auto bearing : {0u, 9u, 10u, 11u, 180u, 65535u})
        for (auto trait : {0u, 20u, 127u, 255u}) for (double radius : {0.0, 3.0, 100.0, std::numeric_limits<double>::quiet_NaN()}) {
            Case test{0x10281780, side, 4, 5, 4, bearing, 0}; test.strategy_low = trait;
            test.strategy_high = 0; test.factor = radius; result.push_back(test);
        }
    for (auto target : {std::pair{4, 5}, std::pair{5, 5}}) {
        Case test{0x10281780, 0, 4, 5, std::uint32_t(target.first), 0, std::uint32_t(target.second)};
        test.strategy_low = 20; test.factor = 1; result.push_back(test);
    }
    for (int action : {3, 51, 52, 53, 54, 55, 57, 61, 90, 91, 92, 0, 19, 93})
        for (int heading : {-32768, -1, 0, 90, 180, 359, 32767}) for (auto target : {0u, 90u, 180u, 359u, 65535u})
            for (auto duration : {0.0, 1.0, 100.0, std::numeric_limits<double>::quiet_NaN()}) {
                Case test{0x102818f0, 0, 20, heading, std::uint32_t(action), target, 2};
                test.strategy_high = 5; test.factor = duration; test.intensity = 0; result.push_back(test);
            }
    for (int side : {0, 1}) for (int index : {-1, 0, 19}) for (auto flags : {0u, 4u, 20u})
        for (auto state : {0u, 1u, 2u}) for (auto trait : {0u, 1u}) for (auto role : {1u, 4u, 64u}) for (int pressure : {0, 2}) {
            Case test{0x10285ef0, side, index, pressure, flags, 1, role};
            test.strategy_low = state; test.strategy_high = trait; result.push_back(test);
        }
    for (auto entry : {0x10226520u, 0x10235db0u, 0x10235ea0u, 0x102393c0u, 0x10237830u})
        for (int position : {0, 1, 2, 7}) for (auto seed : {0u, 65535u, 0x80000000u}) {
            Case test{entry, position, 5, 6, entry == 0x10226520 ? 0x7fffffffu : 4u, 0,
                entry == 0x10237830 ? 6u : seed};
            test.random_seed = seed; test.strategy_low = 17; test.nonuniform_pool = true;
            test.intensity = 6200; test.factor = 1; result.push_back(test);
        }
    for (int side : {0, 1}) for (auto category : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, 8u, 9u, 255u})
        for (auto affinity : {0u, 20u, 255u}) for (auto history : {false, true}) for (auto national : {0u, 1u}) {
            Case test{0x10227140, side, 180, 175, category, affinity, 35};
            test.null_player = !history; test.strategy_low = 75; test.strategy_high = national; test.factor = 20;
            result.push_back(test);
        }
    for (const auto [height, club_height] : {std::pair{-32768, 32767}, std::pair{32767, -32768}, std::pair{175, 175}})
        for (auto trait : {-128, -1, 0, 127}) {
            Case test{0x10227140, 0, height, club_height, 0, 127, 255};
            test.strategy_low = std::uint32_t(trait); test.factor = trait; result.push_back(test);
        }
    for (auto age : {0u, 999u, 1000u, 1001u, 0x80000000u, 0x7fffffffu}) for (bool mutate : {false, true}) {
        Case test{0x10227140, 0, 180, 175, 2, 25, age}; test.mutate = mutate;
        test.strategy_low = 75; test.factor = 20; result.push_back(test);
    }
    for (int side : {0, 1}) for (auto status : {0u, 1u, 2u, 3u, 4u, 8u, 9u, 255u})
        for (auto word : {0u, 1u, 3u, 8u, 65535u}) for (auto position : {0u, 1u, 2u})
            for (auto country : {0, 8}) for (auto absent : {0u, 1u}) {
                Case test{0x10288230, side, 7, country, absent, status, 0};
                test.strategy_low = word; test.strategy_high = position; test.nonuniform_pool = true; result.push_back(test);
            }
    for (auto role : {0u, 1u, 0x80u, 0x100u, 0x200u, 0x400u, 0x800u})
        for (int trait : {-128, -1, 0, 1, 20, 127}) for (auto bias : {0u, 4u, 255u, 0x100u})
            for (int side : {0, 1}) for (int position : {0, 1, 2}) {
                Case test{0x102857c0, side, -1, trait, bias, role, 0};
                test.strategy_low = 17; test.nonuniform_pool = true;
                if (position) test.side = position == 1 ? 1 : 2;
                result.push_back(test);
            }
    for (auto change : {0xffffff80u, 0xfffffffeu, UINT32_MAX, 0u, 1u, 2u, 127u, 255u, 256u})
        for (int morale : {-128, -1, 0, 1, 9, 10, 20, 127}) for (int score : {7499, 7500, 7501, 10000})
            for (auto closed : {0u, 1u}) for (int position : {0, 1, 2}) {
                Case test{0x10288f30, position, morale, score, change, 0x12345678, 0};
                test.strategy_high = closed; test.strategy_low = 17; test.nonuniform_pool = true; result.push_back(test);
            }
    for (int gate : {0, 127}) for (auto bias : {255u, 256u}) {
        Case test{0x102857c0, 0, gate, 20, bias, 0, 0}; test.strategy_low = 0; result.push_back(test);
    }
    for (int trait : {-7, 3, -128}) for (auto seed : {0u, 0x80000000u, UINT32_MAX}) {
        Case test{0x102857c0, 0, -128, trait, 256, 0, seed}; test.nonuniform_pool = true;
        test.strategy_low = 17; result.push_back(test);
    }
    { Case test{0x102857c0, 0, -128, -1, 256, 0, 0x80000000}; result.push_back(test); }
    for (auto status : {2u, 3u, 9u}) for (auto replacement : {0u, 3u, 4u}) {
        Case test{0x10288230, 0, 7, 8, replacement, status, 0}; test.mutate = true;
        test.strategy_low = 17; test.nonuniform_pool = true; result.push_back(test);
    }
    for (auto word : {0u, 1u, 2u, 3u, 7u, 8u, 17u, 65535u}) for (bool positive : {false, true}) {
        Case test{0x10288f30, 0, positive ? 10 : -128, 10000, positive ? 2u : 0xfffffffeu, 0, 0x80000000};
        test.strategy_low = word; result.push_back(test);
    }
    for (int adaptive : {0, 1}) for (auto heading : {0u, 90u, 180u, 359u})
        for (auto trait : {0u, 1u, 20u, 127u, 255u}) for (auto feedback : {0u, 1u, 2u, 127u, 128u, 255u})
            for (auto approaching : {0u, 1u}) for (bool absent : {false, true}) {
                Case test{0x10244c60, adaptive, 4, 5, 4, heading, 5}; test.random_seed = trait;
                test.strategy_low = approaching; test.strategy_high = feedback; test.no_feedback = absent;
                test.nonuniform_pool = true; test.intensity = 3; result.push_back(test);
            }
    for (auto [x, y] : {std::pair{4, 0}, std::pair{5, 6}, std::pair{8, 11}})
        for (auto time : {0.0, 5.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
            for (auto position : {0u, 1u, 2u}) {
                Case test{0x10244c60, 1, 4, 5, std::uint32_t(x), 0, std::uint32_t(y)};
                test.random_seed = 20; test.factor = time; test.strategy_high = 128; test.pool_position = position;
                test.nonuniform_pool = true; result.push_back(test);
            }
    for (auto heading : {0x8000u, 0x7fffu, 359u}) for (auto feedback : {0u, 1u, 128u, 255u}) {
        Case test{0x10244c60, 1, 4, 5, 4, heading, 5}; test.feedback_alias = true;
        test.random_seed = 128; test.strategy_high = feedback; test.nonuniform_pool = true; result.push_back(test);
    }
    for (auto trait : {0u, 128u}) for (auto time : {0.0, 1.0}) {
        Case test{0x10244c60, 1, 4, 5, 4, 359, 5}; test.random_seed = trait;
        test.strategy_high = 1; test.intensity = 0; test.factor = time; result.push_back(test);
    }
    for (auto quality : {0u, 1000u, 1001u, 65535u}) for (auto value : {-1000000.0f, -65536.0f, -1.0f, 0.0f,
                 1.0f, 20.0f, 65536.0f, 1000000.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()})
        for (auto position : {0u, 1u, 2u}) for (auto seed : {0u, 65535u, 0x80000000u}) {
            Case test{0x10270320, 0, 4, 0, quality, 0x12345678, seed};
            test.intensity = value; test.strategy_low = 17; test.pool_position = position;
            test.nonuniform_pool = true; result.push_back(test);
        }
    for (int side : {0, 1}) for (auto trait : {0u, 7u, 255u}) for (auto staff : {0u, 1u, 127u, 255u})
        for (auto request : {0u, 1u, 2u}) for (bool mutate : {false, true}) for (bool recover : {false, true}) {
            Case test{0x10270320, side, 4, 1, 0, staff, 0}; test.intensity = 0;
            test.strategy_high = trait; test.custom_pool = true; test.pool_words = {0, 0, 1, 0, 0, 0, 0, 0};
            test.pool_words[5] = request == 0 ? 1 : 0; test.pool_words[6] = request == 1 ? 1 : 0;
            test.pool_words[7] = recover ? 1 : 0;
            test.mutate = mutate; result.push_back(test);
        }
    for (auto kind : {1u, 2u, 3u, 4u, 5u}) for (float intensity : {0.0f, 0.5f, 1.0f, 2.0f, 5.0f, 10.0f, 50.0f, 100.0f})
        for (auto opponent : {0u, 1u, 2u}) for (auto position : {0u, 1u, 2u}) for (auto seed : {0u, 65535u, 0x80000000u}) {
            Case test{0x10286650, 0, 4, 5, kind, 0x12345678, seed}; test.intensity = intensity;
            test.strategy_high = opponent; test.pool_position = position; test.strategy_low = 17;
            test.nonuniform_pool = true; result.push_back(test);
        }
    for (auto kind : {0u, 1u, 6u, 128u, 255u, 256u, 257u})
        for (float intensity : {110.0f, 127.0f, 128.0f, -1.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
            Case test{0x10286650, 0, 4, 5, kind}; test.intensity = intensity; result.push_back(test);
        }
    // Mutated source constants characterize the general signed sampler too;
    // the game's ordinary positive coefficients cannot reach its negative arm.
    for (auto kind : {2u, 3u, 4u, 5u}) for (double coefficient : {0.0, -1000.0, -1000000.0}) {
        Case test{0x10286650, 0, 4, 5, kind}; test.foreign_table = true;
        test.factor = coefficient; test.intensity = 1; test.second_role = 0; result.push_back(test);
    }
    for (auto kind : {2u, 3u, 4u, 5u}) {
        Case test{0x10286650, 0, 4, 5, kind}; test.intensity = 1; test.second_role = 0;
        test.custom_pool = true; test.pool_words = {0, 65534, 304, 0, 0, 0, 0, 0};
        result.push_back(test);
    }
    for (auto kind : {1u, 2u, 3u, 4u, 5u}) {
        Case test{0x10286650, 0, 4, 5, kind}; test.mutate = true; test.strategy_high = 2;
        test.nonuniform_pool = true; result.push_back(test);
    }
    for (float intensity : {-129.0f, -128.0f, -17.0f, -0.5f, 109.999f, 111.0f, 255.0f, 256.0f}) {
        Case test{0x10286650, 0, 4, 5, 1}; test.intensity = intensity;
        test.strategy_high = 1; result.push_back(test);
    }
    for (auto kind : {2u, 3u, 4u, 5u}) {
        Case test{0x10286650, 0, 4, 5, kind}; test.foreign_table = true;
        test.factor = -1000000; test.intensity = 1; test.second_role = 0x80000000;
        result.push_back(test);
    }
    for (auto kind : {2u, 3u, 4u, 5u})
        for (float intensity : {1000000.0f, std::numeric_limits<float>::infinity(), std::numeric_limits<float>::quiet_NaN()}) {
            Case test{0x10286650, 0, 4, 5, kind}; test.intensity = intensity;
            test.nonuniform_pool = true; result.push_back(test);
        }
    for (int readiness : {-32768, -1, 0, 1, 284, 285, 32767})
        for (auto trait : {0u, 1u, 2u, 20u, 32767u, 32768u, 65535u, 65536u})
            for (auto position : {0u, 1u, 2u}) for (auto seed : {0u, 65535u, 0x80000000u}) {
                Case test{0x10289620, 0, 4, readiness, trait, 0x12345678, seed};
                test.pool_position = position; test.strategy_low = 17;
                test.nonuniform_pool = true; result.push_back(test);
            }
    for (double coefficient : {0.0, 1.0, -1.0, 1000000.0, -1000000.0})
        for (auto seed : {0u, 0x80000000u}) {
            Case test{0x10289620, 0, 4, 300, 20, 0, seed};
            test.custom_pool = true; test.pool_words = {0, 299, 0, 65534, 300, 0, 0, 0};
            test.foreign_table = true; test.factor = coefficient; result.push_back(test);
        }
    // Reseeding inside the nested correction exposes its incoming F1/F2.
    {
        Case test{0x10289620, 0, 4, 300, 20, 0, 0}; test.pool_position = 6;
        test.custom_pool = true; test.pool_words = {300, 0, 0, 0, 0, 299, 0, 65534};
        test.foreign_table = true; test.factor = 1000000; result.push_back(test);
    }
    for (double coefficient : {1000000.0, -1000000.0}) {
        Case test{0x10289620, 0, 4, 32767, 20, 0, 0x80000000};
        test.custom_pool = true; test.pool_words = {0, 2, 0, 65534, 304, 0, 0, 0};
        test.foreign_table = true; test.factor = coefficient; result.push_back(test);
    }
    {
        Case test{0x10289620, 0, 4, 32767, 20, 0, 0x80000000};
        test.custom_pool = true; test.pool_words = {0, 2, 0, 65534, 0, 0, 0, 0};
        test.pool_words[4] = (2147483648u - 13) % 3051;
        test.foreign_table = true; test.factor = 1000000; result.push_back(test);
    }
    {
        // wrap32(bound*10)=-65534; the negative arithmetic shift is exact.
        Case test{0x10286650, 0, 4, 5, 2}; test.intensity = 1;
        test.foreign_table = true; test.factor = 1717980365.0; test.second_role = 0;
        result.push_back(test);
        Case scaled{0x10289620, 0, 4, 300, 20, 0, 0};
        scaled.custom_pool = true; scaled.pool_words = {0, 299, 0, 65534, 304, 0, 0, 0};
        scaled.foreign_table = true; scaled.factor = 1717980365.0 / 20;
        result.push_back(scaled);
        Case gate{0x10289620, 0, 4, 0, 20, 0, 0x80000063};
        gate.custom_pool = true; result.push_back(gate);
    }
    constexpr std::array<std::array<std::uint16_t, 8>, 8> peer_pools{{
        {1, 0, 1, 1, 1, 1, 1, 1}, {1, 1, 0, 0, 1, 1, 1, 1},
        {1, 1, 0, 20, 0, 1, 1, 1}, {1, 1, 0, 20, 5, 1, 1, 1},
        {1, 1, 0, 20, 9, 1, 1, 1}, {1, 1, 1, 0, 0, 1, 1, 1},
        {1, 1, 1, 0, 1, 1, 1, 1}, {1, 1, 1, 1, 1, 1, 1, 1}}};
    for (int side : {0, 1}) for (int depth : {-41, -40, -1, 0, 1, 2, 9, 10, 127})
        for (int count : {0, 1, 4, 5}) for (auto words : peer_pools) for (auto accepted : {0u, 1u}) {
            Case test{0x102876c0, side, 10, count, std::uint32_t(depth), 0, 95};
            test.intensity = 100; test.factor = 50; test.strategy_low = 8;
            test.strategy_high = 0x100 | accepted; test.custom_pool = true; test.pool_words = words;
            result.push_back(test);
        }
    for (auto flags : {0u, 0x100u, 0x108u, 0x110u, 0x300u, 0x303u}) {
        Case test{0x102876c0, 0, 10, 2, 10, 0, 96}; test.strategy_high = flags;
        test.intensity = 100; test.factor = 50; test.custom_pool = true;
        result.push_back(test);
    }
    for (auto words : {peer_pools[0], peer_pools[1], peer_pools[2], peer_pools[5]})
        for (int trait : {0, 10, 25, 127, 128, 255})
            for (double intensity : {-1000.0, 0.0, 30.0, 1000.0, 1000000.0,
                     std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()}) {
                Case test{0x102876c0, 0, trait, 0, 10, 0, 96}; test.strategy_high = 0x100;
                test.intensity = 100; test.factor = intensity; test.custom_pool = true; test.pool_words = words;
                result.push_back(test);
            }
    for (auto trait : {8u, 7u, 255u}) {
        Case test{0x102876c0, 0, 10, 5, 10, 0, 95}; test.strategy_high = 0x100;
        test.intensity = 100; test.factor = 50; test.strategy_low = trait; test.pool_position = 7;
        test.custom_pool = true; test.pool_words = {1, 1, 1, 1, 1, 1, 1, 0}; result.push_back(test);
    }
    for (auto seed : {0xfffffff8u, 0xffffffffu}) {
        Case test{0x102876c0, 0, 10, 5, 10, 0, 95}; test.strategy_high = 0x100;
        test.intensity = 100; test.factor = 50; test.strategy_low = 7; test.random_seed = seed;
        const auto bias = 0u - seed; test.custom_pool = true;
        test.pool_words = {0, std::uint16_t(bias + 1), std::uint16_t(bias), std::uint16_t(bias + 20), 0, 0, 0, 0};
        result.push_back(test);
    }
    for (auto flags : {0x320u, 0x103u, 0x140u, 0x180u, 0x1c0u})
        for (auto words : peer_pools) for (auto position : {0u, 1u, 2u}) {
            Case test{0x102876c0, 0, 10, 2, 10, 0, 96}; test.strategy_high = flags;
            test.intensity = 100; test.factor = 50; test.custom_pool = true; test.pool_words = words;
            test.pool_position = position; result.push_back(test);
        }
    for (bool mutate : {false, true}) {
        Case test{0x102876c0, 0, 10, 2, 10, 0, 96}; test.strategy_high = 0x101;
        test.intensity = 100; test.factor = 50; test.custom_pool = true; test.mutate = mutate;
        result.push_back(test);
    }
    constexpr std::array<std::array<std::uint16_t, 8>, 6> incident_pools{{
        {1, 0, 0, 1, 1, 1, 1, 1}, {1, 1, 0, 0, 1, 1, 1, 1},
        {1, 1, 1, 0, 1, 1, 1, 1}, {1, 1, 1, 1, 0, 1, 1, 1},
        {1, 1, 1, 1, 1, 0, 1, 1}, {1, 1, 1, 1, 1, 1, 1, 1}}};
    for (int side : {0, 1}) for (auto action : {0u, 1u, 2u, 3u, 4u, 5u, 9u, 128u, 255u})
        for (auto words : incident_pools) for (double resistance : {0.0, 4.0, 5.0, 20.0, 21.0, -1000000.0,
                     1000000.0, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN()})
            for (auto position : {0u, 1u, 2u}) {
                Case test{0x102884d0, side, 8, 5, 123, action, std::uint32_t(side == 0)};
                test.strategy_high = 1; test.strategy_low = 6; test.intensity = 100; test.factor = resistance;
                test.custom_pool = true; test.pool_words = words; test.pool_position = position;
                result.push_back(test);
            }
    for (auto flags : {0u, 1u, 3u, 5u, 9u, 17u}) for (auto phase : {5u, 6u, 7u, 8u})
        for (int row : {0, 1, 10, 11, 128}) for (auto action : {0u, 3u, 9u}) {
            Case test{0x102884d0, 0, 7, row, 0, action, 1};
            test.strategy_high = flags; test.strategy_low = phase; test.intensity = 100; test.factor = 5;
            test.custom_pool = true; result.push_back(test);
        }
    for (double resistance : {-65516.0, -65515.0, 65555.0, 65556.0})
        for (auto words : {incident_pools[0], incident_pools[1]}) for (auto position : {0u, 1u, 2u}) {
            Case test{0x102884d0, 0, 8, 5, 1, 3, 1};
            test.strategy_high = 1; test.strategy_low = 6; test.intensity = 100; test.factor = resistance;
            test.custom_pool = true; test.pool_words = words; test.pool_position = position;
            result.push_back(test);
        }
    for (bool request : {false, true}) for (bool choice : {false, true}) for (bool step : {false, true}) {
        Case test{0x102884d0, 0, 8, 9, 255, 3, 1};
        test.strategy_high = 1; test.strategy_low = 6; test.intensity = 100; test.factor = 5;
        test.custom_pool = true; test.mutate = request; test.mutate_choice = choice; test.mutate_after_step = step;
        result.push_back(test);
    }
    for (int side : {0, 1}) for (int trait : {7, 8, 255}) for (auto flags : {1u, 17u})
        for (auto notice : {0u, 1u}) {
            Case test{0x102884d0, side, trait, 5, 1, 3, 1};
            test.strategy_high = flags; test.strategy_low = 6; test.intensity = 100; test.factor = 0;
            test.custom_pool = true; test.pool_words = {0, 0, 1, 1, std::uint16_t(notice), 0, 0, 0};
            result.push_back(test);
        }
    {
        Case test{0x102884d0, 0, 8, 5, 1, 3, 1};
        test.strategy_high = 1; test.strategy_low = 6; test.intensity = 100; test.factor = 0;
        test.custom_pool = true; test.pool_position = 2; result.push_back(test);
        test.pool_position = 7; test.factor = -1000000;
        test.pool_words = {300, 0, 0, 0, 0, 0, 0, 65534}; result.push_back(test);
        test.pool_position = 0; test.random_seed = 0xffffffe0;
        test.pool_words = {0, 32, 65534, 19, 0, 0, 0, 0}; result.push_back(test);
    }
    for (unsigned event = 0x219e; event <= 0x21b8; ++event)
        for (int side : {0, 1}) for (int condition : {0, 6500, 7000, 8000, 9000})
            for (auto criterion : {0u, 29u, 39u, 51u, 52u, 59u, 82u, UINT32_MAX}) {
                Case test{0x10289d50, side, 0, 1000, event, 4, 100};
                test.strategy_low = 0x05030008; test.strategy_high = criterion;
                test.intensity = 20; test.factor = condition; test.custom_pool = true;
                test.pool_words = {0, 0, 0, 1, 0, 0, 0, 0}; result.push_back(test);
            }
    for (auto event : {0u, 7999u, 8000u, 8605u, 8633u, 8676u, 8677u, 32768u, 65535u, 0x1234219eu, 0xffff21b8u}) {
        Case test{0x10289d50, 0, 0, 1000, event, 4, 100};
        test.strategy_low = 0x05030008; test.strategy_high = 0; test.intensity = 20; test.factor = 6000;
        test.custom_pool = true; test.pool_words = {0, 0, 0, 1, 0, 0, 0, 0}; result.push_back(test);
    }
    const auto event_case = [](std::uint32_t event) {
        Case test{0x10289d50, 0, 0, 1000, event, 4, 100};
        test.strategy_low = 0x05030008; test.strategy_high = UINT32_MAX;
        test.intensity = 20; test.factor = 6000; test.custom_pool = true;
        test.pool_words = {0, 0, 0, 1, 0, 0, 0, 0}; return test;
    };
    for (auto event : {0x219eu, 0x21a3u, 0x21b1u, 0x21b2u, 0x21b7u})
        for (auto flags : {0u, 1u, 2u, 4u, 8u, 16u, 32u, 64u, 0x900u})
            for (int time : {-32768, -1, 0, 110, 605, 715, 1000, 32767}) {
                auto test = event_case(event); test.strategy_low = (test.strategy_low & ~8u) | flags;
                test.row = time; test.strategy_high = 0; result.push_back(test);
            }
    for (auto event : {0x219fu, 0x21a0u, 0x21b0u, 0x21b3u, 0x21b4u, 0x21b5u, 0x21b6u})
        for (auto seed : {0u, 0xfffffc19u, 0xfffff831u, 0x80000000u})
            for (auto position : {0u, 1u, 2u}) for (int condition : {6499, 6500, 7000, 7250, 8000, 9000}) {
                auto test = event_case(event); test.factor = condition; test.random_seed = seed;
                test.pool_position = position; test.strategy_low |= 1; test.last_is_player = true;
                test.strategy_high = 52; test.second_role = 90; result.push_back(test);
            }
    for (unsigned divisor : {0u, 1u, 2u, 3u, 4u, 5u, 255u})
        for (auto event : {0x21a1u, 0x21a2u}) for (int time : {-32768, -1, 0, 110, 649, 660, 682, 880, 1000, 32767})
            for (auto context : {0u, 1u, 2u, 8u, 16u, 32u}) {
                auto test = event_case(event); test.strategy_low = (test.strategy_low & 0x00ffffff) | (divisor << 24);
                test.row = time; test.strategy_high = 82; test.context_flags = context; test.pool_position = 1;
                result.push_back(test);
            }
    for (auto event : {0x21a1u, 0x21a2u}) {
        auto test = event_case(event); test.strategy_low |= 128; result.push_back(test);
    }
    for (int trait : {-128, 0, 15, 16, 18, 20, 127}) for (auto role : {0u, 4u})
        for (auto morale : {0u, 8u, 128u, 255u}) for (auto seed : {0u, 0x80000000u}) {
            auto test = event_case(0x21b8); test.intensity = trait; test.first_role = role;
            test.strategy_low = (test.strategy_low & ~0xff00u) | (morale << 8);
            test.random_seed = seed; result.push_back(test);
        }
    for (auto flags : {0x05000008u, 0x05030009u}) {
        auto test = event_case(0x21b8); test.strategy_low = flags; result.push_back(test);
    }
    for (auto context : {0u, 8u, 16u, 32u, 48u, 64u, 128u}) {
        auto test = event_case(0x21b8); test.context_flags = context;
        test.pool_position = context == 128 || context == 64 ? 1 : 6;
        test.pool_words = {0, 0, 0, 0, 0, 0, 0, 1};
        test.mutate = context == 64; result.push_back(test);
    }
    for (auto context : {0u, 4u, 256u, 512u, 1024u}) {
        auto test = event_case(0x219e); test.strategy_high = 39; test.context_flags = context;
        result.push_back(test);
        test = event_case(0x21b5); test.context_flags = context; test.factor = 9000; result.push_back(test);
    }
    for (auto days : {89u, 90u, 0xffffffffu}) {
        auto test = event_case(0x21a0); test.second_role = days; test.last_is_player = true;
        test.factor = 9000; result.push_back(test);
    }
    {
        auto test = event_case(0x21b6); test.strategy_low |= 1; test.factor = 9000;
        test.context_flags = 8; test.pool_position = 1; test.row = 660; result.push_back(test);
    }
    {
        auto test = event_case(0x21b7); test.strategy_low |= 1; result.push_back(test);
        test = event_case(0x21b6); test.strategy_low |= 1; test.factor = 9000; test.row = 0;
        result.push_back(test);
        test.row = 1000; test.strategy_low &= ~8u; result.push_back(test);
    }
    const auto action_case = [] {
        Case test{0x102775e0};
        test.first_role = 0; test.second_role = 0; test.strategy_high = 1;
        test.strategy_low = 0x05050000; test.intensity = 100000; test.factor = 20;
        test.custom_pool = true; test.pool_words = {1, 1, 1, 1, 1, 1, 1, 1}; return test;
    };
    for (int side : {0, 1}) for (auto role : {0u, 1u, 4u})
        for (auto flags : {0u, 1u, 2u, 6u, 8u, 16u, 17u}) for (auto context : {0u, 1u, 2u, 255u})
            for (auto count : {0u, 2u, 255u}) {
                auto test = action_case(); test.side = side; test.first_role = role;
                test.strategy_low |= flags; test.strategy_high = context; test.argument = count;
                result.push_back(test);
            }
    for (auto position : {0u, 1u, 2u, 6u}) for (auto seed : {0u, 0xfffffff0u, 0x80000000u})
        for (float intensity : {-100000.f, 0.f, 5.f, 100000.f, std::numeric_limits<float>::quiet_NaN()}) {
            auto test = action_case(); test.pool_position = position; test.random_seed = seed;
            test.intensity = intensity; test.argument = 2; result.push_back(test);
        }
    for (auto path : {0u, 1u, 2u, 3u, 4u}) for (auto context : {0u, 1u, 2u, 4u, 8u, 16u, 32u, 64u, 128u, 65536u, 131072u})
        for (int trait : {-128, -1, 0, 1, 127}) {
            auto test = action_case(); test.argument = 2; test.context_flags = context;
            test.strategy_low = (std::uint32_t(std::uint8_t(trait)) << 24) | (5u << 16);
            test.pool_position = 1;
            if (path == 0) test.intensity = -100000;
            if (path == 1) { test.strategy_high = 1; test.pool_words.fill(0); }
            if (path == 2) { test.strategy_low |= 8; test.column = 4; test.row = 11; test.strategy_high |= 0xff00; test.pool_words.fill(1); }
            if (path == 3) { test.first_role = 1; test.strategy_low |= 16; }
            if (path == 4) test.strategy_low |= 1;
            result.push_back(test);
        }
    {
        auto test = action_case(); test.pool_words.fill(0); test.strategy_high = 0x101;
        result.push_back(test); // Zero /20 draw with nonzero signed +110.
        test = action_case(); test.strategy_low |= 8; test.column = 4; test.row = 11;
        test.strategy_high = 0x101; result.push_back(test); // Equal +110 and /10 draw.
    }
    for (auto returned : {0u, 1u, 255u}) for (auto flags : {0u, 256u, 512u, 768u}) {
        auto test = action_case(); test.strategy_high = 2; test.second_role = returned;
        test.context_flags = flags; test.first_role = 1; result.push_back(test);
    }
    for (double bound : {0., 1., -1., 20., -20., 65535., -65535., 65536., -65536., 2147483520., -2147483648., std::numeric_limits<double>::quiet_NaN()})
        for (auto position : {0u, 1u, 2u, 6u}) for (auto flag : {0u, 2048u, 4096u, 8192u, 16384u, 32768u}) {
            auto test = action_case(); test.strategy_low |= 1; test.context_flags = 1024 | flag;
            test.intensity = -100000; test.factor = bound; test.pool_position = position;
            result.push_back(test);
        }
    const auto secondary_case = [] {
        Case test{0x10281af0}; test.first_role = 1; test.strategy_high = 0;
        test.strategy_low = 0; test.factor = 20; test.custom_pool = true; test.pool_words.fill(1); return test;
    };
    for (int side : {0, 1}) for (auto role : {0u, 1u, 0x40u, 0x88u, 0x808u, 0x200u, 0x208u, 0x204u})
        for (auto strategy : {0u, 1u << 21, 1u << 23}) for (auto alternate : {0u, 4u})
            for (int trait : {-1, 0, 5, 127}) for (auto position : {std::pair{4,5}, {0,0}, {8,11}, {2,2}, {6,8}}) {
                auto test = secondary_case(); test.side = side; test.first_role = role;
                test.strategy_low = strategy; test.strategy_high = alternate;
                test.column = position.first; test.row = position.second;
                test.secondary.trait = trait; test.secondary.count = 2; result.push_back(test);
            }
    for (int phase : {-1, 0}) for (int context : {-1, 0, 1}) for (int other : {0, 1, 2})
        for (int pursuit : {0, 1}) for (int trait : {0, 5}) {
            auto test = secondary_case(); test.secondary.phase = phase; test.secondary.match_side = context;
            test.secondary.other = other; test.secondary.pursuit = pursuit; test.secondary.trait = trait;
            result.push_back(test);
        }
    for (int linked : {0, 1, 2, 3, 4}) for (int effort : {0, 1}) for (int side : {0, 1})
        for (float metric : {-1.f, 50.f, std::numeric_limits<float>::quiet_NaN()}) for (int column : {0, 4, 8}) {
            auto test = secondary_case(); test.side = side; test.secondary.linked = linked;
            test.secondary.effort = effort; test.secondary.linked_metric = metric;
            test.secondary.linked_column = column; test.secondary.linked_row = side == 0 ? 0 : 11;
            result.push_back(test);
        }
    for (auto bound : {0., 1., -1., 20., -20., 65535., -65535., 65536., -65536., 2147483520., -2147483648., std::numeric_limits<double>::quiet_NaN()})
        for (auto position : {0u, 1u, 2u, 6u}) for (int mobility : {-128, 0, 127}) {
            auto test = secondary_case(); test.column = 3; test.factor = bound; test.pool_position = position;
            test.secondary.mobility = mobility; result.push_back(test);
        }
    for (int side : {0, 1}) for (int profile : {-128, -1, 0, 1, 127})
        for (auto seed : {0u, 0x80000000u}) for (auto offer : {0u, 1u, 255u}) {
            auto test = secondary_case(); test.side = side; test.random_seed = seed;
            test.strategy_low = 1u << 21; test.secondary.profile = profile;
            test.secondary.offer = offer; test.secondary.trait = 5; test.secondary.count = 2;
            test.pool_position = 1; result.push_back(test);
        }
    for (int line : {-128, -1, 0, 1, 127}) for (int adjust : {-128, -1, 0, 1, 127})
        for (auto context : {0u, 2u, 4u, 8u, 16u}) {
            auto test = secondary_case(); test.secondary.line = line; test.secondary.row_adjust = adjust;
            test.secondary.trait = 5; test.secondary.count = 2; test.context_flags = context;
            test.secondary.offer = 1; test.pool_position = 1; result.push_back(test);
        }
    for (int side : {0, 1}) for (auto role : {0x80u, 0x88u, 0x800u, 0x808u})
        for (int column : {-1, 0, 1, 2, 6, 7}) for (int row : {-1, 0, 1, 9, 10, 11, 12}) {
            auto test = secondary_case(); test.side = side; test.first_role = role; test.strategy_high = 4;
            test.secondary.ball_column = column; test.secondary.ball_row = row; result.push_back(test);
        }
    for (int side : {0, 1}) for (auto formation : {0u, 4u}) for (auto moving : {0, 1, 255})
        for (int row : {0, 5, 11}) for (int column : {0, 4, 8}) {
            auto test = secondary_case(); test.side = side; test.secondary.linked = 1;
            test.secondary.effort = 1; test.secondary.linked_column = column; test.secondary.linked_row = row;
            test.secondary.linked_metric = -1; test.secondary.moving = moving; test.secondary.formation = formation;
            result.push_back(test);
        }
    for (int side : {0, 1}) for (int mobility : {0, 1}) for (int crowd : {0, 1, 255}) {
        auto test = secondary_case(); test.side = side; test.row = side == 0 ? 8 : 3;
        test.secondary.ball_row = side == 0 ? 9 : 2; test.secondary.mobility = mobility;
        test.secondary.crowd = crowd; result.push_back(test);
    }
    for (int side : {0, 1}) for (int column : {-128, -127, 4}) {
        auto test = secondary_case(); test.side = side; test.strategy_low = 1u << 23;
        test.secondary.trait = 127; test.secondary.target_column = column; result.push_back(test);
    }
    for (int line : {-128, -1, 0, 1, 127}) for (int adjust : {-128, -1, 0, 1, 127})
        for (int trait : {-128, 0, 5}) {
            auto test = secondary_case(); test.secondary.line = line; test.secondary.row_adjust = adjust;
            test.secondary.trait = trait; test.secondary.count = 2; result.push_back(test);
        }
    for (int side : {0, 1}) for (int row : {-1, 5, 11}) {
        auto test = secondary_case(); test.side = side; test.column = side == 0 ? 0 : 8;
        test.row = side == 0 ? 11 : 0; test.secondary.ball_row = row;
        test.secondary.ball_column = test.column; result.push_back(test);
    }
    for (int column : {3, 4}) for (auto after : {0u, 1u, 2u, 3u}) for (auto context : {0u, 16u}) {
        auto test = secondary_case(); test.column = column; test.first_role = 0;
        test.secondary.trait = 5; test.secondary.count = 2; test.secondary.mobility = 127;
        test.secondary.offer = 1; test.secondary.offer_after = after; test.context_flags = context;
        test.factor = 0; result.push_back(test);
    }
    for (auto seed : {0x80000000u, 0xffffffe0u, 0xffffffd8u}) for (auto position : {0u, 2u}) {
        auto test = secondary_case(); test.random_seed = seed; test.pool_position = position;
        test.strategy_low = 1u << 21; test.secondary.profile = -1; test.pool_words.fill(0);
        test.secondary.trait = -128; result.push_back(test);
    }
    {
        auto test = secondary_case(); test.secondary.match_side = -1; test.secondary.other = 1;
        test.pool_words.fill(0); result.push_back(test);
        test.secondary.other = 0; test.secondary.pursuit = 1; test.context_flags = 32; result.push_back(test);
    }
    for (int side : {0, 1}) {
        auto test = secondary_case(); test.side = side; test.row = side == 0 ? 11 : 0;
        test.secondary.trait = 5; test.secondary.count = 2; test.secondary.offer = 1; result.push_back(test);
        test = secondary_case(); test.side = side; test.secondary.trait = 5; test.secondary.row_adjust = 1;
        result.push_back(test);
    }
    {
        auto test = secondary_case(); test.first_role = 0; test.secondary.effort = 1; result.push_back(test);
    }
    for (auto role : {0u, 2u, 4u}) for (int column : {3, 4}) {
        auto test = secondary_case(); test.first_role = role; test.column = column;
        test.secondary.count = 2; test.secondary.mobility = 127; test.factor = 0; result.push_back(test);
        test.secondary.trait = 5; test.context_flags = 64; result.push_back(test);
    }
    const auto context_case = [] {
        Case test{0x102836a0}; test.first_role = 0; test.strategy_low = 0; test.strategy_high = 0;
        test.factor = 20; test.custom_pool = true; test.pool_words.fill(1); return test;
    };
    for (int side : {0, 1}) for (auto role : {0u, 1u, 0x208u, 0x110u, 0x420u, 0x800u})
        for (int phase : {-1, 0, 1}) for (int other : {0, 1, 2}) for (int main : {0, 1, 2})
            for (int pursuit : {0, 1}) for (auto query : {0u, 1u}) for (int target : {3, 4}) {
                auto test = context_case(); test.side = side; test.first_role = role;
                test.context.match_side = phase; test.context.other = other; test.context.main = main;
                test.context.main_side = 1 - side; test.context.pursuit = pursuit; test.context.context_query = query;
                test.context.target_column = target; result.push_back(test);
            }
    for (int side : {0, 1}) for (auto flags : {0u, 0x400u}) for (int line : {2, 3, 4, 5, 7, 8, 9, 10})
        for (bool peer : {false, true}) for (int limit : {-1, 2, 3, 8, 9, 11}) for (int trait : {0, 5}) {
            auto test = context_case(); test.side = side; test.row = side == 0 ? 4 : 7;
            test.context.main = 2; test.context.team_flags = flags; test.context.ball_row = side == 0 ? 6 : 5;
            test.context.trait108 = 5; test.context.trait109 = 5; test.context.effort = 5; test.context.trait10e = trait;
            test.context.line0 = line; test.context.line1 = line; test.context.peer0 = peer; test.context.peer1 = peer;
            test.context.limit0 = limit; test.context.limit1 = limit; result.push_back(test);
        }
    for (float bound : {0.f, 1.f, -1.f, 20.f, -20.f, 65535.f, -65535.f, 65536.f, -65536.f, 2147483520.f, -2147483648.f, std::numeric_limits<float>::quiet_NaN()})
        for (float coefficient : {0.1f, 1.f, 100000.f, std::numeric_limits<float>::quiet_NaN()})
            for (auto position : {0u, 1u, 2u, 6u, 8u}) for (auto ball : {std::pair{4,5}, {3,4}, {8,11}}) {
                auto test = context_case(); test.first_role = 0x208; test.context.match_side = -1;
                test.context.bound_e0 = bound; test.context.custom_coefficient = true; test.context.coefficient = coefficient;
                test.context.status110 = 127; test.context.ball_column = ball.first; test.context.ball_row = ball.second;
                test.pool_position = position; result.push_back(test);
            }
    for (int side : {0, 1}) for (int slot : {-6, -5, -4, -3, -1, 0}) for (auto flags : {0u, 0x400u})
        for (int main : {0, 2}) for (int other : {0, 2}) for (int moving : {0, 1})
            for (float metric : {-1.f, 50.f, std::numeric_limits<float>::quiet_NaN()}) {
                auto test = context_case(); test.side = side; test.row = side == 0 ? 4 : 7;
                test.context.linked = 1; test.context.linked_slot = slot; test.context.team_flags = flags;
                test.context.main = main; test.context.other = other; test.context.moving = moving;
                test.context.linked_metric = metric; result.push_back(test);
            }
    for (int side : {0, 1}) {
        auto test = context_case(); test.side = side; test.row = side == 0 ? 0 : 11;
        test.context.context_query = 1; test.context.keeper_null = true; test.context.returning_exit = true;
        result.push_back(test); // Explicit alternative where the fatal boundary returns.
    }
    {
        auto test = context_case(); test.first_role = 0x208; test.context.match_side = -1;
        test.context.bound_e0 = 0; test.context.custom_coefficient = true; test.context.coefficient = 1;
        test.context.coefficient_back = 0x102841a4; test.context.coefficient_after = 0;
        test.pool_position = 8; test.context.ball_column = 3; test.context.ball_row = 4; result.push_back(test);
    }
    for (int side : {0, 1}) for (auto role : {0u, 1u}) for (int phase : {-1, 0, 5})
        for (int match_side : {-1, 0, 1}) for (auto formation : {0u, 4u}) for (bool locked : {false, true}) {
            auto test = context_case(); test.side = side; test.first_role = role;
            test.context.phase = phase; test.context.match_side = match_side; test.context.other = 2;
            test.context.main_side = side; test.context.formation = formation; test.context.locked = locked;
            test.pool_position = locked ? 1 : 0; result.push_back(test);
        }
    for (int side : {0, 1}) for (auto role : {0u, 1u}) for (int main : {0, 1}) for (int row : {4, 6})
        for (int delta : {-1, 0, 1}) for (auto position : {0u, 1u, 8u}) {
            auto test = context_case(); test.side = side; test.first_role = role; test.row = side == 0 ? row : row + 1;
            test.context.main = main; test.context.ball_row = test.row + delta;
            test.context.trait108 = 5; test.context.trait109 = 5; test.context.effort = 5; test.context.trait10e = 5;
            test.context.peer0 = true; test.context.peer1 = true; test.context.limit0 = 9; test.context.limit1 = 2;
            test.context.line0 = 8; test.context.line1 = 3; test.pool_position = position; result.push_back(test);
        }
    for (double bound : {0., 1., -1., -20., 65535., -65535., 65536., -65536., 2147483520., -2147483648., std::numeric_limits<double>::quiet_NaN()})
        for (auto position : {0u, 1u, 6u, 7u, 8u}) for (auto seed : {0u, 0x80000000u, 0xfffffff0u}) {
            auto test = context_case(); test.factor = bound; test.pool_position = position; test.random_seed = seed;
            test.context.status110 = position & 1 ? 127 : 0; test.context.effort = position & 1;
            test.context.trait109 = (position >> 1) & 1; test.intensity = seed == 0 ? -1 : 50; result.push_back(test);
        }
    for (int side : {0, 1}) for (auto role : {0u, 2u, 4u}) for (auto policy : {0u, 0x8000u, 0x2000u, 0xa000u})
        for (auto flags : {0u, 0x100u, 0x4000u, 0x4100u}) for (int opponents : {-128, -1, 0, 1, 20, 127}) for (int moving : {0, 1}) {
            auto test = context_case(); test.side = side; test.first_role = role; test.strategy_low = policy;
            test.context.team_flags = flags; test.context.opposition = opponents; test.context.moving = moving;
            test.pool_position = unsigned(opponents) & 1 ? 1 : 8; result.push_back(test);
        }
    for (int side : {0, 1}) for (int phase : {-1, 0})
        for (auto ball : {std::pair{3,5}, {4,4}, {0,0}, {8,11}, {-1,5}, {4,-1}, {9,5}, {4,12}})
            for (int pursuit : {0, 1}) for (auto query : {0u, 1u}) for (auto traits : {0u, 1u, 2u, 3u})
                for (double metric : {0., 100., std::numeric_limits<double>::quiet_NaN()}) {
                    auto test = context_case(); test.side = side; test.first_role = 1;
                    test.context.match_side = phase; test.context.other = 2; test.context.main_side = side;
                    test.context.ball_column = ball.first; test.context.ball_row = ball.second; test.context.pursuit = pursuit;
                    test.context.context_query = query; test.context.effort = traits & 1; test.context.trait109 = traits >> 1;
                    test.context.metric = metric; result.push_back(test);
                }
    for (int side : {0, 1}) for (int row : {0, 1, 5, 6, 10, 11}) for (int phase : {-1, 0})
        for (int status : {0, 1}) for (double metric : {-1., 100., std::numeric_limits<double>::quiet_NaN()})
            for (bool on_ball : {false, true}) for (bool goal : {false, true}) for (auto policy : {0u, 0x8000u}) {
                auto test = context_case(); test.side = side; test.row = row; test.strategy_low = policy;
                test.context.match_side = phase; test.context.other = 2; test.context.main_side = side;
                test.context.status110 = status; test.context.metric = metric; test.context.pursuit = 1;
                test.context.goal_flag = goal; test.context.ball_column = on_ball ? 4 : 3; test.context.ball_row = row;
                test.context.trait109 = 1; test.context.effort = 1; test.intensity = -1;
                result.push_back(test);
            }
    for (int side : {0, 1}) for (int own : {0, 1, 2}) for (int opponents : {0, 1, 128}) for (int effort : {0, 1})
        for (auto query : {0u, 1u}) for (int keeper : {0, 1, 2}) {
            auto test = context_case(); test.side = side; test.row = side == 0 ? 0 : 11;
            test.context.own_count = own; test.context.opposing_count = opponents; test.context.effort = effort;
            test.context.context_query = query; test.context.keeper_status = keeper; result.push_back(test);
        }
    for (auto role : {0x200u, 0x100u, 0x400u}) for (int row : {4, 5, 6}) {
        auto test = context_case(); test.first_role = role; test.context.match_side = -1;
        test.context.ball_column = 3; test.context.ball_row = row; result.push_back(test);
    }
    for (float coefficient : {-1.f, -100000.f}) for (auto position : {0u, 1u, 6u, 8u}) {
        auto test = context_case(); test.first_role = 0x208; test.context.match_side = -1;
        test.context.custom_coefficient = true; test.context.coefficient = coefficient; test.context.bound_e0 = 0;
        test.context.ball_column = 3; test.context.ball_row = 4; test.pool_position = position; result.push_back(test);
    }
    {
        auto test = context_case(); test.context.match_side = -1; test.context.other = 1;
        test.pool_words.fill(0); test.pool_position = 1; result.push_back(test);
        test.context.other = 0; test.context.pursuit = 1; test.context.locked = true; result.push_back(test);
    }
    for (int side : {0, 1}) for (int row : {0, 4, 5, 6, 7, 11}) for (int ball_row : {0, 5, 11}) for (int line : {0, 5, 11}) {
        auto test = context_case(); test.side = side; test.row = row; test.context.ball_row = ball_row;
        test.context.linked = 1; test.context.linked_slot = 0; test.context.main = 2; test.context.team_flags = 0x400;
        test.context.line0 = line; test.context.line1 = line; test.context.linked_metric = -1; result.push_back(test);
    }
    for (int side : {0, 1}) for (int row : {0, 5, 11}) for (int column : {0, 4, 8})
        for (auto formation : {0u, 4u}) for (int moving : {0, 1}) {
            auto test = context_case(); test.side = side; test.context.linked = 1; test.context.linked_slot = 0;
            test.context.linked_row = row; test.context.linked_column = column; test.context.linked_metric = -1;
            test.context.formation = formation; test.context.moving = moving; test.pool_position = 1; result.push_back(test);
        }
    for (int side : {0, 1}) for (int other : {0, 2}) {
        auto test = context_case(); test.side = side; test.context.linked = 1; test.context.linked_slot = 0;
        test.context.other = other; test.context.trait108 = 1; result.push_back(test);
    }
    for (auto location : {std::array{-1,5,0,5}, {9,5,8,5}, {4,-1,4,0}, {4,12,4,11}})
        for (int side : {0, 1}) for (auto role : {0u, 1u}) for (auto policy : {0u, 0x8000u}) {
            auto test = context_case(); test.side = side; test.first_role = role; test.strategy_low = policy;
            test.column = location[2]; test.row = location[3]; test.context.ball_column = location[0]; test.context.ball_row = location[1];
            test.context.other = 2; test.context.main_side = side; test.context.pursuit = 1; test.context.status110 = 1;
            test.context.match_side = role == 1 ? -1 : 0; test.context.metric = 100; test.intensity = -1; result.push_back(test);
        }
    for (int line : {-1, 0, 1, 127}) for (int trait : {0, 1}) {
        auto test = context_case(); test.context.line = line; test.context.trait = trait; result.push_back(test);
    }
    for (int side : {0, 1}) for (int column : {1, 2, 6, 7}) for (int row : {-1, 0, 1, 10, 11, 12}) {
        auto test = context_case(); test.side = side; test.column = column; test.row = row; result.push_back(test);
    }
    for (int side : {0, 1}) for (int column : {2, 6}) {
        auto test = context_case(); test.side = side; test.column = column; test.row = side == 0 ? 10 : 1;
        test.context.match_side = -1; test.context.other = 2; test.context.main_side = side;
        test.context.pursuit = 1; test.context.status110 = 1; test.context.metric = -1; test.context.goal_flag = true;
        test.context.ball_column = column; test.context.ball_row = test.row; result.push_back(test);
    }
    for (auto initial : {0u, 1u, 0x208u}) for (auto later : {0u, 1u, 0x208u}) for (bool mutate : {false, true})
        for (int phase : {-1, 0}) for (auto position : {0u, 7u}) {
            auto test = context_case(); test.first_role = initial; test.context.role_later = later;
            test.context.match_side = phase; test.mutate = mutate; test.pool_position = position;
            test.context.ball_column = 3; test.context.ball_row = 4; result.push_back(test);
        }
    for (bool first_bound : {false, true}) for (float after : {0.f, -1.f, 100000.f, std::numeric_limits<float>::quiet_NaN()}) {
        auto test = context_case(); test.first_role = 0x208; test.context.match_side = -1;
        test.context.ball_column = 3; test.context.ball_row = 4;
        test.context.bound_e0 = first_bound ? 65536 : 0; test.context.custom_coefficient = true;
        test.context.coefficient = 100000; test.context.coefficient_after = after;
        test.context.coefficient_back = 0x10226574; test.pool_position = 7; result.push_back(test);
    }
    for (int restart : {2, 3, 4, 5, 6, 7}) for (int side : {0, 1, 2, 255})
        for (bool target : {false, true}) for (int score : {1999, 2000}) for (auto pool : {0u, 7u}) {
            Case test{0x10278220, side, 4, side == 0 ? 10 : 1}; test.primary.restart = restart;
            test.primary.target = target; test.primary.query_score = score; test.pool_position = pool;
            test.strategy_low = pool ? 99 : 100; result.push_back(test);
        }
    for (int side : {0, 1}) for (int column : {0, 1, 2, 6, 7, 8}) for (int row : {0, 1, 10, 11})
        for (bool mutate : {false, true}) {
            Case test{0x10278220, side, column, row}; test.primary.restart = 2; test.mutate = mutate; test.pool_position = 7;
            result.push_back(test);
        }
    for (float bound : {0.f, -1.f, 65535.f, 65536.f, 1e9f, std::numeric_limits<float>::quiet_NaN()})
        for (int side : {0, 1}) for (int trait : {-128, 0, 127}) for (auto pool : {0u, 7u}) {
            Case test{0x10278220, side, 4, side == 0 ? 10 : 1}; test.primary.restart = 6;
            test.primary.attribute_a4 = bound; test.primary.mobility = trait; test.pool_position = pool;
            test.strategy_low = 65535; test.random_seed = UINT32_MAX; result.push_back(test);
        }
    for (int side : {0, 1}) for (int ball_column : {0, 8}) for (int ball_row : {0, 11}) {
        Case test{0x10278220, side, 4, side == 0 ? 10 : 1}; test.primary.restart = 2;
        test.primary.ball_column = ball_column; test.primary.ball_row = ball_row; result.push_back(test);
    }
    for (int side : {0, 1}) for (int column : {1, 4, 7}) for (int row : {0, 11}) {
        Case test{0x10278220, side, column, row}; test.primary.restart = 6; result.push_back(test);
    }
    for (float bound : {-20.f, -21.f, -65556.f, 65516.f}) for (int side : {0, 1})
        for (int trait : {0, 127}) for (auto seed : {0u, 0x80000000u, UINT32_MAX}) for (auto word : {0u, 100u, 101u, 65535u}) {
            Case test{0x10278220, side, 4, side == 0 ? 10 : 1}; test.primary.restart = 6;
            test.primary.attribute_a4 = bound; test.primary.mobility = trait; test.random_seed = seed;
            test.strategy_low = word; test.pool_position = 7; result.push_back(test);
        }
    for (int restart : {3, 4, 7}) for (int side : {0, 1}) for (bool target : {false, true})
        for (int score : {-1, 0, 1999, 2000, INT32_MAX}) {
            Case test{0x10278220, side, 4, side == 0 ? 10 : 1}; test.primary.restart = restart;
            test.primary.target = target; test.primary.query_score = score; test.primary.query_kind = 255;
            test.primary.query_flag = 128; test.mutate = true; test.pool_position = 7; result.push_back(test);
        }
    for (int side : {0, 1}) for (auto role : {0u, 1u}) for (auto mode : {0u, 4u})
        for (int mobility : {0, 10}) for (int trait : {0, 1}) {
            Case test{0x10278220, side, 4, 5, mode}; test.primary.restart = 0; test.first_role = role;
            test.primary.mobility = mobility; test.primary.trait109 = trait; test.pool_position = 7;
            result.push_back(test);
        }
    unsigned primary_index = 0;
    for (int side : {0, 1}) for (auto mode : {0u, 4u}) for (int phase : {-1, 0, 1, 2, 3, 4, 5})
        for (auto role : {0u, 1u, 0x40u, 0x880u, 0x38u, 0x81u}) for (auto position : {std::pair{4, 0}, {4, 1}, {1, 5}, {4, 5}, {7, 5}, {4, 10}, {4, 11}}) {
            Case test{0x10278220, side, position.first, position.second, mode}; const auto index = primary_index++;
            test.primary.restart = 0; test.primary.phase = phase; test.first_role = role;
            test.primary.mobility = index % 2 ? 0 : 10; test.primary.trait109 = index % 3 ? 1 : 0; test.primary.trait10e = index % 4 ? 0 : 10;
            test.primary.target = index & 1; test.primary.query_score = index % 4 == 0 ? -2000 : index % 4 == 1 ? 0 : index % 4 == 2 ? 2000 : 5000;
            test.primary.query_kind = index % 3 ? 7 : 94; test.primary.offer = index % 3 ? 0 : 1;
            test.primary.team_shape = index % 3; test.primary.neighbor_count = index % 4; test.primary.support = index % 3;
            test.primary.metric = index % 3 ? 0 : 4; test.primary.row_adjust = index % 2 ? -1 : 0;
            test.primary.policy_query = index % 4 ? 0 : 1; test.primary.publish = index % 3 == 0;
            test.primary.keeper_status = index % 2; test.primary.keeper_column = index % 3 ? 4 : 2;
            test.primary.keeper_row = index % 4 == 0 ? 5 : index % 4 == 1 ? side == 0 ? 11 : 0 : side == 0 ? 10 : 1;
            test.primary.keeper_role = index % 3 ? 1 : 0; test.pool_position = index % 2 ? 7 : 0;
            test.strategy_low = index % 3 ? 0 : 65535; test.random_seed = index % 5 ? 0 : 0x80000000;
            test.strategy_high = (index % 7) << 25; result.push_back(test);
        }
    for (float distance : {0.f, .01f, .5f, 1.f, 1.5f, 3.f, 6.f, 20.f, 1000.f, 100000.f, std::numeric_limits<float>::quiet_NaN()})
        for (int side : {0, 1}) for (auto role : {0u, 1u, 0x40u}) for (int trait : {-128, -10, 0, 1, 20, 127}) {
            Case test{0x10278220, side, 4, 5}; test.primary.restart = 0; test.first_role = role;
            test.primary.custom_distance = true; test.primary.distance = distance; test.primary.mobility = trait;
            test.primary.trait109 = trait; test.primary.trait10e = trait; test.primary.keeper_role = 1; test.primary.keeper_status = 1;
            test.primary.offer = 1; test.primary.metric = 4; test.primary.bound_e4 = trait; test.pool_position = 7;
            test.strategy_low = 65535; test.random_seed = UINT32_MAX; result.push_back(test);
        }
    for (int side : {0, 1}) for (int items : {1, 2}) for (int count : {-1, 0, 1, 20, 100, 32767})
        for (int population : {-128, -1, 1, 2, 127}) {
            Case test{0x10278220, side, 4, 5}; test.primary.restart = 0;
            test.primary.list_items = items; test.primary.population_count = count; test.primary.population = population;
            test.primary.offer = 1; test.primary.target = true; test.primary.query_score = 5000;
            test.primary.mobility = 10; test.primary.keeper_status = 1;
            test.pool_position = 7; test.strategy_low = 65535; test.random_seed = UINT32_MAX; result.push_back(test);
            test.strategy_low |= 0x2a000000; result.push_back(test);
        }
    for (int restart : {0, 1, 8}) for (int side : {0, 1}) for (auto role : {0u, 1u, 0x8c1u})
        for (int phase : {-1, 0, 3, 5}) for (auto position : {std::pair{0,0}, {8,11}, {4,5}, {4,10}})
            for (float attribute : {-100000.f, -1.f, 0.f, .01f, 10.f, 100000.f, std::numeric_limits<float>::quiet_NaN()}) {
                const auto bits = 0x9e3779b9u * ++primary_index;
                Case test{0x10278220, side, position.first, position.second, bits & 1 ? 0u : 4u};
                auto& in = test.primary; in.restart = restart; in.phase = phase; test.first_role = role;
                in.attribute = attribute; in.bound_e4 = attribute; in.tactic = bits & 2 ? 5 : 0;
                in.mobility = bits & 4 ? 10 : 0; in.effort = bits & 8 ? 1 : 0;
                in.trait109 = std::array{-128, -10, 0, 1, 19, 20, 21, 127}[(bits >> 4) & 7];
                in.trait10e = bits & 128 ? 10 : 0; in.target = bits & 256; in.offer = (bits >> 10) & 1;
                in.query_score = std::array{-20000, 0, 2000, 10000}[(bits >> 11) & 3];
                in.query_kind = std::array{6, 7, 8, 94}[(bits >> 13) & 3];
                in.query_column = bits & 32768 ? -1 : 9; in.query_row = bits & 65536 ? -1 : 12;
                in.target_column = bits & 131072 ? -1 : 9; in.target_row = bits & 262144 ? -1 : 12;
                in.team_shape = (bits >> 19) % 3; in.neighbor_count = (bits >> 21) % 3;
                in.crowd = bits & 8388608 ? 0 : 1; in.moving = bits & 16777216 ? 0 : 1;
                in.edge_query = bits & 33554432 ? 0 : 1; in.policy_query = bits & 67108864 ? 0 : 1;
                in.staff67 = bits & 134217728 ? -128 : 0; in.staff68 = bits & 268435456 ? 0 : 127;
                in.rating = bits & 536870912 ? 2000 : -1; in.balance = bits & 1073741824 ? 1 : 2;
                in.balance_outputs = bits & 0x80000000 ? std::array{10, 1, 2, 3, 4} : std::array{-1, 5, 4, 3, 2};
                in.run_trait = std::int8_t(bits >> 16); in.publish = bits & 1; in.action = std::array{2, 16, 17, 18, 59}[(bits >> 7) % 5];
                in.turn_state = bits & 1024 ? 2 : 0; in.opposing_flags = bits & 2048 ? 0x400 : 0;
                in.metric = std::array{-100000., 0., 4., std::numeric_limits<double>::quiet_NaN()}[(bits >> 9) & 3];
                in.keeper_status = bits & 2; in.keeper_role = bits & 4 ? 0 : 1;
                in.keeper_column = bits & 8 ? position.first : 4; in.keeper_row = bits & 16 ? position.second : side == 0 ? 11 : 0;
                in.previous_peer = true; in.previous_null = (bits & 4096) != 0; in.previous_action = std::array{52, 54, 55, 54}[(bits >> 13) & 3];
                test.strategy_high = 0xffffffff; test.strategy_low = bits & 32 ? 0 : 65535;
                test.random_seed = bits & 64 ? UINT32_MAX : 0x80000000; test.pool_position = 7;
                if (phase != -1 && test.argument != 4 && (bits & 128)) {
                    in.list_items = 1; in.population = 1; in.population_count = bits & 256 ? 1 : 20;
                }
                result.push_back(test);
                test.strategy_low |= 0x2a000000; test.second_role = 0x880; result.push_back(test);
            }
    for (int side : {0, 1}) for (float distance : {std::nextafter(4.f, 0.f), 4.f, std::nextafter(4.f, 5.f), 20.f})
        for (int trait : {74, 75, 76}) for (int quality : {-32768, 0, 32767})
            for (auto seed : {0u, 65536u, 0x80000000u, UINT32_MAX}) {
                Case test{0x10278220, side, 4, 5}; auto& in = test.primary;
                in.restart = 0; in.custom_distance = true; in.distance = distance; in.mobility = 127;
                in.trait109 = trait; in.shot_quality = quality; in.keeper_status = 1; in.metric = 3;
                test.first_role = 1; test.pool_position = 7; test.strategy_low = 65535; test.random_seed = seed;
                result.push_back(test); test.pool_position = 0; result.push_back(test);
            }
    for (int side : {0, 1}) for (float distance : {0.f, -.01f, .01f, 100000.f, std::numeric_limits<float>::quiet_NaN()})
        for (auto keeper : {std::pair{0,5}, {4,10}, {3,11}, {4,11}}) for (int status : {0, 1})
            for (auto seed : {0u, 0x80000000u}) {
                Case test{0x10278220, side, 4, side == 0 ? 8 : 3}; auto& in = test.primary;
                in.restart = 0; in.custom_distance = true; in.distance = distance;
                in.keeper_column = keeper.first; in.keeper_row = side == 0 ? keeper.second : 11 - keeper.second;
                in.keeper_status = status; in.keeper_role = 1; in.trait109 = 20; in.trait10e = -10;
                in.metric = 3; test.strategy_low = 65535; test.pool_position = 7; test.random_seed = seed; result.push_back(test);
            }
    for (int side : {0, 1}) for (int column : {1, 4, 7}) for (int row : {-1, 0, 3, 8, 11, 12})
        for (auto roles : {std::pair{0x8b8u, 0x880u}, {0x8b8u, 0u}, {0x38u, 0x880u}}) {
            Case test{0x10278220, side, column, row}; auto& in = test.primary;
            in.restart = 0; in.keeper_column = column; in.keeper_row = row; in.keeper_role = 1;
            in.metric = 3; in.target = false; in.offer = 0; in.action = 2; in.mobility = 10;
            in.trait109 = 1; in.edge_query = 1; in.balance = 1; in.rating = 2000;
            test.first_role = roles.first; test.second_role = roles.second; test.strategy_high = 4;
            test.strategy_low = 0x2a00ffff; test.pool_position = 7; result.push_back(test);
            test.strategy_high = 0; in.target = true; in.query_kind = 7; in.query_score = 2000; result.push_back(test);
        }
    for (int side : {0, 1}) for (float distance : {-100000.f, 5.f, 20.f, 5500.f, 110000.f})
        for (double metric : {-107., -100000., 100000.}) for (auto role : {0u, 0x40u}) for (auto pool : {0u, 1u, 2u, 7u}) {
            Case test{0x10278220, side, 4, side == 0 ? 8 : 3}; auto& in = test.primary;
            in.restart = 0; in.custom_distance = true; in.distance = distance; in.metric = metric; in.moving = 1;
            in.keeper_column = 0; in.keeper_row = 5; in.keeper_status = 1; in.keeper_role = 1; in.support = -2;
            test.first_role = role; test.custom_pool = true; test.pool_words = {65534,65535,65534,65535,65534,65535,65534,65535};
            test.pool_position = pool; result.push_back(test);
        }
    for (int side : {0, 1}) for (int phase : {-1, 0, 4}) for (auto mode : {0u, 4u})
        for (float attribute : {-100000.f, -1.f, 0.f, 10.f, 100000.f}) for (float distance : {0.f, 1.f, 6.f}) {
            Case test{0x10278220, side, 4, side == 0 ? 11 : 0, mode}; auto& in = test.primary;
            in.restart = 0; in.phase = phase; in.action = 16; in.custom_distance = true; in.distance = distance;
            in.attribute = attribute; in.bound_e4 = attribute; in.mobility = 10; in.trait109 = 1; in.team_shape = 2;
            in.keeper_role = 1; in.offer = 1; test.pool_position = 0; result.push_back(test);
        }
    for (int side : {0, 1}) for (int row : {-1, 0, 1, 10, 11, 12}) for (bool absent : {false, true}) {
        Case test{0x10278220, side, 4, row}; auto& in = test.primary;
        in.restart = 0; in.phase = absent ? 5 : 0; in.keeper_null = absent; in.keeper_role = 1; in.keeper_row = row;
        in.bound_e4 = 0; in.trait109 = 0; in.trait10e = 0; test.first_role = 0x40; result.push_back(test);
    }
    for (int side : {0, 1}) for (auto word : {1u, 2u, 3u, 9u}) for (int balance : {-1, 0, 1, 2, 3})
        for (auto role : {0u, 1u, 0x40u}) for (int column : {3, 4, 5}) {
            Case test{0x10278220, side, 4, side == 0 ? 6 : 5}; auto& in = test.primary;
            in.restart = 0; in.rating = 2000; in.staff68 = 0; in.policy_query = 1; in.balance = balance;
            in.query_score = 0; in.target_column = column; in.target = false; in.mobility = 0; in.trait109 = 1;
            in.keeper_role = 1; in.metric = 0; test.strategy_low = word; test.pool_position = 0; test.first_role = role; result.push_back(test);
        }
    for (int side : {0, 1}) for (int score : {-20001, -20000, -7, 1}) {
        Case test{0x10278220, side, 4, side == 0 ? 6 : 5}; auto& in = test.primary;
        in.restart = 0; in.opposing_flags = 0x400; in.trait109 = 20; in.query_score = score;
        test.strategy_low = 5; test.pool_position = 0; result.push_back(test);
    }
    for (int side : {0, 1}) for (int opponents : {0, 1, 2, 3}) for (auto role : {0u, 0x40u}) {
        Case test{0x10278220, side, 4, 5}; auto& in = test.primary;
        in.restart = 0; in.actual_population = true; in.list_items = std::min(opponents, 2); in.opposing_count = opponents;
        in.mobility = 10; in.trait109 = 1; test.first_role = role; test.pool_position = 0; result.push_back(test);
    }
    for (int side : {0, 1}) for (auto word : {0u, 1u, 9u}) for (int score : {20000, 1000000, INT32_MAX})
        for (int row : {3, 5, 6, 8}) {
            Case test{0x10278220, side, 4, row}; auto& in = test.primary;
            in.restart = 0; in.phase = -1; in.action = 16; in.target = false; in.query_score = score;
            test.strategy_low = word; test.pool_position = 0; result.push_back(test);
        }
    for (int side : {0, 1}) for (auto mode : {0u, 4u}) for (auto role : {0u, 1u, 0x8b8u})
        for (bool target : {false, true}) for (int phase : {0, 3, 5}) {
            Case test{0x10278220, side, 4, 5, mode}; auto& in = test.primary;
            in.restart = 0; in.phase = phase; in.target = target; in.offer = 1; in.mobility = 10; in.trait109 = 20;
            in.keeper_role = 1; in.keeper_status = 1; test.first_role = role; test.second_role = 0x880;
            test.mutate = true; test.pool_position = 7; test.strategy_low = 0x2a00ffff; result.push_back(test);
        }
    for (int side : {0, 1}) for (int column : {1, 3, 4, 7}) for (int relative_row : {2, 5, 6, 9})
        for (int population : {0, 1}) for (int mobility : {0, 127}) {
            Case test{0x10278220, side, column, side == 0 ? relative_row : 11 - relative_row}; auto& in = test.primary;
            in.restart = 0; in.rating = 2000; in.staff68 = 0; in.policy_query = 1; in.balance = 1;
            in.query_score = 0; in.mobility = mobility; in.trait109 = 1; in.keeper_role = 1;
            if (mobility == 127) in.shot_quality = 32767;
            in.actual_population = population; in.opposing_count = population; in.list_items = population;
            test.first_role = 0; test.strategy_low = 1; test.pool_position = 0; result.push_back(test);
        }
    for (int side : {0, 1}) for (int quality : {-32768, -3, -2, -1, 0, 1}) for (auto word : {0u, 1u, 65535u}) {
        Case test{0x10278220, side, 4, 5}; auto& in = test.primary;
        in.restart = 0; in.action = 59; in.custom_distance = true; in.distance = 4; in.mobility = 127;
        in.trait109 = 75; in.shot_quality = quality; in.keeper_role = 1;
        test.strategy_low = word; test.pool_position = 0; result.push_back(test);
    }
    for (int side : {0, 1}) for (float attribute : {-1.f, 0.f, 1.f, 20.f}) for (auto word : {0u, 1u, 49u}) {
        Case test{0x10278220, side, 4, side == 0 ? 5 : 6}; auto& in = test.primary;
        in.restart = 0; in.custom_attributes = true; in.attribute98 = attribute; in.attribute9c = 100000;
        in.heading = side == 0 ? 90 : 270;
        in.bound_e4 = 0; in.custom_distance = true; in.distance = .5f; in.metric = 4; in.mobility = 0;
        in.keeper_role = 1; in.action = 0; test.strategy_low = word; test.pool_position = 0; result.push_back(test);
    }
    for (int side : {0, 1}) for (int mobility : {0, 10}) {
        Case test{0x10278220, side, 4, side == 0 ? 6 : 5}; auto& in = test.primary;
        in.restart = 0; in.mobility = mobility; in.condition_penalty = 32767; in.keeper_role = 1;
        in.heading = side == 0 ? 90 : 270;
        test.strategy_low = 0; test.custom_pool = true; test.pool_words.fill(1); test.pool_position = 0; result.push_back(test);
    }
    // A single coherent fixture block covers pass destination selection, pass
    // completion and direct contests. Newly owned children execute normally.
    for (auto entry : {0x10245220u, 0x10245870u, 0x10274170u}) {
        Case test{entry}; test.action.trajectory_result = 1; test.action.possession = false; result.push_back(test);
    }
    for (int side : {0, 1}) for (int kind : {9, 54, 62, 91}) for (int receiver : {0, 1, 2})
        for (int flag : {0, 1}) for (int restart : {0, 7}) {
            Case test{0x10245220, side, 4, 5, std::uint32_t(kind)}; test.action.receiver = receiver;
            test.action.core_flag = flag; test.action.restart = restart; result.push_back(test);
        }
    for (int side : {0, 1}) for (int heading : {0, 41, 90, 131, 180, 229, 270, 311, 359})
        for (int row : {1, 5, 10}) for (int column : {1, 4, 7}) for (auto mode : {0u, 4u, UINT32_MAX})
            for (int force : {0, 1}) {
                Case test{0x10245870, side, column, row, mode}; test.action.heading = heading; test.action.force = force;
                test.custom_pool = true; test.pool_words = {0, 1, 2, 3, 5, 7, 11, 17}; test.pool_position = 7;
                result.push_back(test);
            }
    for (auto entry : {0x10245220u, 0x10245870u}) for (int defender : {1, 2}) for (int side : {0, 1})
        for (float distance : {0.f, .01f, 1.f, 65536.f, -65536.f}) for (auto position : {0u, 7u}) {
            Case test{entry, side}; test.action.defender = defender; test.action.defender_side = 1 - side;
            test.action.custom_distance = true; test.action.distance = distance; test.pool_position = position;
            test.action.receiver = 1; test.strategy_low = 1; result.push_back(test);
        }
    for (int side : {0, 1}) for (int target : {0, 1, 2}) for (int quality : {-300, 0, 300})
        for (int phase : {-1, 0, 4}) for (auto word : {0u, 1u, 17u}) {
            Case test{0x10274170, side}; test.action.target = target; test.action.target_quality = quality;
            test.action.phase = phase; test.strategy_low = word; result.push_back(test);
        }
    for (int side : {0, 1}) for (int foul : {1, 2, 3, 4, 5, 6, 7}) for (int contact : {0, 1})
        for (int row : {0, 5, 11}) for (auto word : {1u, 30u}) {
            Case test{0x10274170, side, 4, row}; test.action.foul = foul; test.action.contact = contact;
            test.action.severity = 7; test.action.target_quality = 300; test.strategy_low = word; result.push_back(test);
        }
    for (int bias : {-32768, 0, 32767}) for (int skill : {-32768, 0, 32767})
        for (float attribute : {-32768.f, 0.f, 9999.f, 10000.f, 32767.f}) for (auto seed : {0u, 0x80000000u}) {
            Case test{0x10274170}; test.action.actor_bias = bias; test.action.actor_skill = skill;
            test.action.attribute = attribute; test.random_seed = seed; test.pool_position = 7; test.strategy_low = 65535;
            result.push_back(test);
        }
    for (int side : {0, 1}) for (int phase : {-1, 0}) for (auto word : {0u, 1u, 65535u}) {
        Case test{0x10245870, side, 4, 5}; test.second_role = 0; test.action.phase = phase;
        test.strategy_low = word; test.pool_position = 7; result.push_back(test);
    }
    for (auto entry : {0x10245220u, 0x10245870u}) for (auto seed : {0u, 0x80000000u})
        for (float coefficient : {-100000.f, -1.f, .01f, 1.f}) {
            Case test{entry}; test.action.defender = 1; test.action.custom_coefficient = true;
            test.action.coefficient = coefficient; test.action.custom_distance = true; test.action.distance = 10;
            test.random_seed = seed; test.strategy_low = 65535; result.push_back(test);
        }
    for (int opposite : {0, 1, 20, 255}) for (int delay : {0, 1}) {
        Case test{0x10245220}; test.action.receiver = 1; test.action.opposition = opposite;
        test.action.receiver_delay = delay; test.action.receiver_trait0 = 127; test.action.receiver_trait1 = 127;
        test.action.receiver_busy_on_reseed = true; test.pool_position = 7; result.push_back(test);
    }
    for (auto position : {std::pair{-1, 5}, {4, -1}, {9, 5}, {4, 12}}) {
        Case test{0x10245870}; test.action.core_ball_column = position.first; test.action.core_ball_row = position.second;
        test.action.trajectory_result = 0;
        result.push_back(test);
    }
    for (int side : {0, 1}) for (int action = -1; action <= 47; ++action) for (int foul : {0, 3, 7}) {
        Case test{0x10274170, side}; test.action.target_quality = 300; test.action.action = action;
        test.action.foul = foul; test.strategy_low = 1; result.push_back(test);
    }
    for (int side : {0, 1}) for (int mode : {0, 1}) for (int temper : {0, 100})
        for (int protected_action : {0, 1}) for (int row : {0, 5, 11}) {
            Case test{0x10274170, side, 4, row, unsigned(mode)};
            test.action.actor_temper = temper; test.action.target_temper = temper;
            test.action.protected_action = protected_action; test.action.target_quality = 300;
            test.action.diagnostic = 1; test.action.contact = 1;
            test.random_seed = 0x80000000; test.strategy_low = 65535; result.push_back(test);
        }
    for (int side : {0, 1}) for (int target : {0, 1, 2}) for (int action : {16, 19})
        for (int temper : {0, 100}) for (auto role : {0u, 1u, 0x40u}) {
            Case test{0x10274170, side}; test.action.target = target; test.action.actor_action = action;
            test.action.actor_temper = temper; test.action.target_quality = -32768;
            test.action.actor_bias = 0; test.action.actor_skill = 0; test.random_seed = 0x80000000;
            test.first_role = role; test.strategy_low = 65535; result.push_back(test);
        }
    for (int side : {0, 1}) for (int row : {0, 5, 11}) for (int foul : {1, 7})
        for (int protected_action : {0, 1}) for (auto word : {0u, 1u, 4u, 9u, 30u}) {
            Case test{0x10274170, side, 4, row}; test.action.target_quality = 300; test.action.foul = foul;
            test.action.severity = 10; test.action.contact = 1; test.action.protected_action = protected_action;
            test.action.diagnostic = 1; test.strategy_low = word; result.push_back(test);
        }
    for (int side : {0, 1}) for (int kind : {19, 21, 33, 45}) for (int time : {0, 990, -990})
        for (int attempts : {0, 10}) for (int request : {0, 7}) {
            Case test{0x10274170, side, 4, 5}; test.action.target_quality = 300; test.action.action = kind;
            test.action.phase = 4; test.action.time = time; test.action.target_attempts = attempts;
            test.action.target_successes = attempts / 2; test.action.request_restart = request;
            test.strategy_low = 1; test.pool_position = 7; result.push_back(test);
        }
    for (int phase : {-128, -1, 0, 127}) for (int sign : {-1, 1}) for (auto word : {0u, 1u, 65535u}) {
        Case test{0x10274170}; test.action.actor_bias = sign * 32767; test.action.actor_skill = sign * 32767;
        test.action.phase = phase; test.strategy_low = word; test.pool_position = 7; result.push_back(test);
    }
    for (int target : {0, 1, 2}) for (int quality : {-300, 300}) {
        Case test{0x10274170}; test.action.target = target; test.action.target_quality = quality;
        test.mutate = true; test.strategy_low = 1; test.pool_position = 7; result.push_back(test);
    }
    for (int side : {0, 1}) for (int column : {1, 4, 7}) for (int row : {-1, 0, 1, 10, 11, 12})
        for (int foul : {0, 1}) {
            Case test{0x10274170, side, column, row}; test.action.target_quality = foul ? 300 : -300;
            test.action.foul = foul; test.action.contact = 1; test.strategy_low = 0; result.push_back(test);
        }
    for (int side : {0, 1}) for (int action : {19, 21}) for (int attempts : {0, 10})
        for (int successes : {0, 5, 8, 10}) for (int condition : {6000, 7000}) {
            Case test{0x10274170, side}; test.action.action = action; test.action.phase = 4;
            test.action.target_quality = 300; test.action.time = 990; test.action.target_attempts = attempts;
            test.action.target_successes = successes; test.action.target_condition = condition;
            test.pool_position = 7; test.strategy_low = 1; result.push_back(test);
        }
    for (int side : {0, 1}) for (int action : {0, 1, 12, 19, 21, 33, 34, 41, 42, 43, 44, 45, 46, 47})
        for (auto choice : {0u, 1u, 2u, 11u}) {
            Case test{0x10274170, side, 4, side == 0 ? 11 : 0}; test.action.action = action;
            test.action.target_quality = 300; test.action.protected_action = 1;
            test.custom_pool = true; test.pool_words = {0, 0, 0, std::uint16_t(choice), 0, 1, 2, 3};
            result.push_back(test);
        }
    for (int side : {0, 1}) for (int action : {44, 47}) for (int foul : {2, 3})
        for (int row : {0, 5, 11}) {
            Case test{0x10274170, side, 4, row}; test.action.action = action; test.action.target_quality = 300;
            test.action.contact = 1; test.action.foul = foul; test.strategy_low = 1; result.push_back(test);
        }
    for (int side : {0, 1}) for (int foul : {1, 7}) for (int column : {1, 4, 7})
        for (int row : {2, 3, 8, 9}) for (auto word : {0u, 1u, 4u, 9u, 18u}) {
            Case test{0x10274170, side, column, row}; test.action.target_quality = 300;
            test.action.foul = foul; test.action.severity = 50; test.action.free_taker = int(word & 1);
            test.action.controller = int(word % 3); test.strategy_low = word;
            result.push_back(test);
        }
    for (int side : {0, 1}) for (auto role : {0u, 2u, 4u, 0x8b0u}) for (int restart : {0, 7}) {
        Case test{0x10274170, side, 7, 10, 1}; test.action.target_quality = 300; test.first_role = role;
        test.action.phase = 0; test.action.request_restart = restart; test.strategy_low = 1; result.push_back(test);
    }
    for (int side : {-1, 0, 1}) for (int opposition : {0, 20}) {
        Case test{0x10245220, side}; test.action.receiver = 1; test.action.opposition = opposition;
        test.action.receiver_trait0 = 127; test.action.receiver_trait1 = 127;
        test.action.receiver_busy_on_reseed = true; test.pool_position = 1; result.push_back(test);
    }
    for (auto entry : {0x10245220u, 0x10245870u}) {
        Case test{entry}; test.action.defender = 1; test.pool_position = 1; test.strategy_low = 1;
        test.action.custom_coefficient = true; test.action.coefficient = -100000;
        result.push_back(test);
    }
    for (auto pair : {std::pair{0.f, 20000.f}, {20000.f, 0.f}, {-32768.f, 32767.f}}) {
        Case test{0x10274170}; test.action.split_attribute = true; test.action.attribute = pair.first;
        test.action.second_attribute = pair.second; test.strategy_low = 65535; result.push_back(test);
    }
    for (auto entry : {0x10245220u, 0x10245870u, 0x10274170u}) for (float value : {
        std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::infinity(), -std::numeric_limits<float>::infinity()}) {
        Case test{entry}; test.action.attribute = value; test.action.custom_distance = true; test.action.distance = value;
        test.action.defender = 1; test.strategy_low = 1; result.push_back(test);
    }
    for (int side : {0, 1}) for (int controller : {0, 1, 2}) for (int contact : {0, 1}) {
        Case test{0x10274170, side}; test.action.target_quality = 300; test.action.foul = 3;
        test.action.severity = 50; test.action.controller = controller; test.action.contact = contact;
        test.action.context_contact = 1; test.strategy_low = 24; result.push_back(test);
    }
    for (int side : {0, 1}) for (int contact : {0, 1}) for (auto word : {0u, 9u, 18u}) {
        Case test{0x10274170, side}; test.action.target_quality = 300; test.action.foul = 3;
        test.action.contact = contact; test.action.context_contact = 1; test.strategy_low = word; result.push_back(test);
    }
    for (int side : {0, 1}) for (auto word : {1u, 2u}) {
        Case test{0x10274170, side, 4, side == 0 ? 11 : 0}; test.action.target_quality = 300;
        test.action.foul = 1; test.action.severity = 0; test.strategy_low = word; result.push_back(test);
    }
    for (int side : {0, 1}) for (int temper : {0, 100}) for (auto word : {0u, 1u}) {
        Case test{0x10274170, side, 4, side == 0 ? 11 : 0}; test.action.target_quality = 300;
        test.action.diagnostic = 1; test.action.target_temper = temper; test.strategy_low = word; result.push_back(test);
    }
    for (int side : {-1, 0, 1}) for (int restart : {0, 7}) for (int contact : {0, 1}) {
        Case test{0x10274170, side, 7, 5, 1}; test.action.target_quality = restart ? 300 : -300;
        test.action.request_restart = restart; test.action.context_contact = 0; test.action.contact = contact;
        test.action.opposition = 0; test.strategy_low = 1; result.push_back(test);
    }
    for (int side : {0, 1}) for (int column : {1, 7}) {
        Case test{0x10274170, side, column, side == 0 ? 11 : 0}; test.action.target_quality = 300;
        test.action.protected_action = 1; test.custom_pool = true; test.pool_words.fill(0); result.push_back(test);
        test.column = 4; test.random_seed = 0x80000000; result.push_back(test);
    }
    for (int side : {0, 1}) for (int action : {1, 21, 46}) {
        Case test{0x10274170, side}; test.action.action = action; test.action.actor_metric = 1000;
        test.action.target_quality = 2000; test.action.phase = 4; test.strategy_low = 1; result.push_back(test);
    }
    for (int side : {0, 1}) for (int flag : {0, 1}) {
        Case test{0x10274170, side, 4, side == 0 ? 0 : 11}; test.action.target_quality = 300;
        test.action.phase = 4; test.action.temporal_flag = flag; test.action.time = 990;
        test.strategy_low = 1; result.push_back(test);
    }
    for (auto word : {0u, 50u}) {
        Case test{0x10245220}; test.action.receiver = 1; test.action.receiver_busy_on_seed_read = true;
        test.random_seed = 0x80000000; test.strategy_low = word; result.push_back(test);
    }
    for (auto seed : {0u, 0x80000000u}) {
        Case test{0x10245870}; test.second_role = 0; test.random_seed = seed;
        test.strategy_low = 0; result.push_back(test);
    }
    {
        Case test{0x10245870, 0, 4, -3, 4}; test.action.force = 1; test.action.heading = 270;
        test.custom_pool = true; test.pool_words = {0,1,0,0,0,0,0,0}; result.push_back(test);
        test.row = 5; test.action.restart = 7; result.push_back(test);
    }
    {
        Case test{0x10245220}; test.action.defender = 1; test.action.custom_distance = true;
        test.action.distance = 1000; test.random_seed = std::uint32_t(-65534); result.push_back(test);
    }
    {
        Case test{0x10245870}; test.second_role = 0; test.action.heading = 90;
        test.random_seed = 0x80000000; test.strategy_low = 2; result.push_back(test);
    }
    for (int side : {0, 1}) {
        Case test{0x10274170, side, 4, side == 0 ? 11 : 0}; test.action.target_quality = 300;
        test.action.foul = 1; test.action.severity = -128; test.random_seed = 0x80000000;
        test.strategy_low = 1; result.push_back(test);
    }
    for (int side : {0, 1}) {
        Case test{0x10274170, side}; test.action.target_quality = 300; test.action.foul = 3;
        test.action.context_contact = 1; test.custom_pool = true; test.pool_words = {0,1,1,1,9,1,0,1};
        result.push_back(test);
    }
    for (int side : {0, 1}) {
        Case test{0x10274170, side, 4, side == 0 ? 11 : 0}; test.action.target_quality = 300;
        test.action.contact = 1; test.action.request_restart = 7; test.strategy_low = 1; result.push_back(test);
    }
    for (int side : {0, 1}) for (int kind : {6, 7, 8, 9, 11, 30, 31, 52, 54, 55, 60, 61, 62, 90, 91, 92, 93, 94}) {
        Case test{0x10247030, side, 4, 5, unsigned(kind)};
        test.action.target = 0; test.action.receiver = 1; test.action.trajectory_result = 0;
        result.push_back(test);
    }
    const auto core_case = [](int side, int kind) {
        Case test{0x10247030, side, 4, 5, unsigned(kind)};
        test.action.target = 0; test.action.receiver = 1; test.action.trajectory_result = 0; return test;
    };
    constexpr std::array core_kinds{6, 7, 8, 9, 11, 30, 31, 52, 54, 55, 60, 61, 62, 90, 91, 92, 93, 94};
    constexpr std::array<std::array<int, 4>, 8> trajectories{{
        {4,5,3,7}, {0,1,8,10}, {8,10,0,1}, {4,10,4,12},
        {4,1,4,-1}, {0,5,-1,5}, {8,5,9,5}, {4,5,4,5}}};
    for (auto kind : core_kinds) for (int side : {0, 1}) for (auto seed : {0u, 0x80000000u})
        for (auto word : {0u, 1u, 7u, 65535u}) for (const auto& coordinates : trajectories) {
            auto test = core_case(side, kind); test.random_seed = seed; test.strategy_low = word;
            test.column = coordinates[0]; test.row = coordinates[1];
            test.action.destination_column = coordinates[2]; test.action.destination_row = coordinates[3]; result.push_back(test);
        }
    for (auto kind : core_kinds) for (int side : {0, 1}) for (int phase : {-128, -1, 0, 1, 2, 3, 4, 5, 127})
        for (int weather : {-1, 0, 1, 2, 3, 4, 5}) {
            auto test = core_case(side, kind); test.action.phase = phase; test.action.core_weather = weather;
            test.factor = 4; test.strategy_low = 7; test.action.possession = false; result.push_back(test);
        }
    for (auto kind : core_kinds) for (int side : {0, 1}) for (double metric : {0.0, 2.0, 3.0, 15.0, 40.0, 200.0})
        for (auto word : {0u, 1u, 7u, 65535u}) {
            auto test = core_case(side, kind); test.factor = metric; test.strategy_low = word;
            test.first_role = word == 7 ? 4 : 1; test.action.attribute = word == 65535 ? 6000 : 10000; result.push_back(test);
        }
    for (auto kind : core_kinds) for (float value : {-0.0f, 0.0f, std::numeric_limits<float>::denorm_min(),
        -std::numeric_limits<float>::infinity(), std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN(), std::numeric_limits<float>::max()}) {
        auto test = core_case(0, kind); test.factor = value; result.push_back(test);
        test = core_case(1, kind); test.action.attribute = value; result.push_back(test);
    }
    for (int kind : {6, 11, 31, 54, 61, 93}) for (int side : {0, 1}) for (float value : {-1.0f, 0.0f, 0.001f, 1.0f, 100000.0f}) {
        auto test = core_case(side, kind); test.action.custom_core_coefficients = true;
        test.action.core_distance_weight = value; test.action.core_flight_weight = value;
        test.action.core_work_weight = value; test.factor = 2.5; test.strategy_low = 1;
        test.action.core_capacity = value; result.push_back(test);
    }
    for (int kind : {6, 7, 8, 9, 52, 54, 55, 62, 90, 91, 92, 93, 94}) for (int receiver : {0, 2}) {
        auto test = core_case(0, kind); test.action.receiver = receiver; test.strategy_low = 7; result.push_back(test);
    }
    for (int side : {0, 1}) for (int action : {1, 12, 19, 21, 33, 34, 41, 42, 43, 44, 45, 46, 47})
        for (int foul : {0, 1, 2, 3, 4, 5, 6, 7, 8, -1}) for (int contact : {0, 1}) for (auto word : {0u, 1u, 7u}) {
            auto test = core_case(side, 31); test.action.target = 1; test.action.target_quality = 300; test.action.action = action;
            test.action.foul = foul; test.action.contact = contact; test.action.caution = word == 7 ? 10 : 5;
            test.action.severity = word == 1 ? -128 : 20; test.action.context_contact = 1;
            test.strategy_low = word; result.push_back(test);
        }
    for (int side : {0, 1}) for (int row : {0, 5, 11}) for (int foul : {0, 2, 3, 4, 5, 6, 7})
        for (int protect : {0, 1}) for (int restart : {0, 7}) {
            auto test = core_case(side, 31); test.row = row; test.action.target = 1; test.action.target_quality = 300;
            test.action.foul = foul; test.action.contact = 1; test.action.protected_action = protect;
            test.action.context_contact = 1; test.action.request_restart = restart; test.action.diagnostic = 1;
            test.action.controller = 2; test.action.caution = 10; test.action.severity = 20; test.strategy_low = 1; result.push_back(test);
        }
    for (int side : {0, 1}) for (int gate = 0; gate <= 13; ++gate) {
        auto test = core_case(side, 31); test.action.target = 1; test.action.target_quality = 300;
        test.action.foul = gate == 2 ? 3 : 4; test.action.caution = gate == 3 ? 0 : 10; test.action.contact = 1;
        test.action.protected_action = 1; test.action.context_contact = 1; test.action.core_queue = true;
        test.action.core_queue_gate = gate; test.action.core_queue_entries = gate == 0 ? 1 : 0;
        test.action.core_profile_result = gate == 13 ? 0 : 10; test.action.core_team_policy = gate == 8 ? 1 : 0;
        test.strategy_low = 1; result.push_back(test);
    }
    for (int kind : {6, 7, 8, 94}) for (int side : {0, 1}) for (int row : {0, 10, 11})
        for (auto role : {1u, 0x880u, 0x8b8u}) for (auto secondary : {0u, 0x880u}) for (auto flags : {0u, 4u})
            for (int policy : {0, 1, 2}) {
                auto test = core_case(side, kind); test.row = row; test.first_role = role; test.second_role = secondary;
                test.strategy_high = flags; test.action.core_team_policy = policy == 1; test.action.core_has_club = policy != 2;
                test.strategy_low = 7; test.action.core_flag = 0; result.push_back(test);
            }
    for (int kind : {9, 11, 30, 31, 52, 54, 55, 60, 61, 91, 93}) for (int side : {0, 1})
        for (int column : {0, 3, 4, 7, 8}) for (int row : {0, 3, 5, 6, 8, 11}) {
            auto test = core_case(side, kind); test.column = column; test.row = row; test.factor = 15;
            test.first_role = 4; test.action.heading = 4; test.strategy_low = 3; test.action.core_flag = 0;
            test.action.destination_column = column; test.action.destination_row = row; result.push_back(test);
        }
    for (int kind : {6, 7, 8, 9, 11, 30, 31, 52, 54, 55, 60, 61, 91, 92, 93, 94})
        for (int control : {0, 1}) for (int previous : {0, 1}) for (int query : {0, 1}) for (int column : {0, 4, 8}) {
            auto test = core_case(0, kind); test.action.core_flag = control; test.action.core_previous = previous;
            test.secondary.moving = query; test.second_role = query; test.action.destination_column = column;
            test.action.destination_row = 5; test.strategy_low = 7; result.push_back(test);
        }
    for (int side : {0, 1}) for (int row : {0, 4, 8, 11}) for (int column : {0, 4, 8}) for (int word : {0, 1, 2, 7}) {
        auto test = core_case(side, 31); test.column = column; test.row = row;
        test.action.target = 1; test.action.target_quality = 300; test.action.protected_action = 1;
        test.strategy_low = unsigned(word); test.action.actor_bias = -200; test.action.actor_skill = -200; result.push_back(test);
    }
    for (int side : {0, 1}) for (int action : {1, 12, 19, 21, 33, 34, 41, 42, 43, 44, 45, 46, 47})
        for (int word : {0, 1, 7}) {
            auto test = core_case(side, 31); test.row = side == 0 ? 11 : 0; test.action.target = 1;
            test.action.target_quality = 300; test.action.action = action; test.action.protected_action = 1;
            test.custom_pool = true; for (unsigned i = 0; i < 8; ++i) test.pool_words[i] = (i & 1) ? 0 : std::uint16_t(word);
            result.push_back(test);
        }
    for (int side : {0, 1}) for (int target : {1, 2}) for (int quality : {-32768, 249, 250, 700, 32767})
        for (auto role : {1u, 2u, 4u, 0x30u, 0x880u, 0x8b8u}) {
            auto test = core_case(side, 31); test.action.target = target; test.action.target_quality = quality;
            test.first_role = role; test.action.core_flag = 0; test.strategy_low = 1; result.push_back(test);
        }
    for (int side : {0, 1}) for (int column : {0, 4, 8}) for (int row : {0, 2, 5, 9, 11})
        for (int action : {19, 21, 33, 45}) for (int time : {0, 990, -990}) {
            auto test = core_case(side, 31); test.column = column; test.row = row; test.action.target = 1;
            test.action.action = action; test.action.target_quality = 800; test.action.time = time;
            test.first_role = 4; test.action.target_attempts = 20; test.action.target_successes = 19;
            test.action.core_ball_bias = 1; test.strategy_low = 1; result.push_back(test);
        }
    for (int side : {0, 1}) for (int kind : {6, 7, 11, 31, 54, 61, 94}) for (auto seed : {0u, 0x80000000u})
        for (int bias : {-32768, -200, 0, 32767}) {
            auto test = core_case(side, kind); test.action.core_ball_bias = bias; test.action.possession = false;
            test.random_seed = seed; test.strategy_low = 31; test.action.core_flag = 0; test.action.actor_temper = -100;
            result.push_back(test);
        }
    for (int side : {0, 1}) for (int kind : {9, 11, 30, 31, 60, 61, 91, 93}) for (int row : {-1, 0, 5, 11, 12})
        for (int query : {0, 1}) for (int restart : {0, 7}) {
            auto test = core_case(side, kind); test.row = row; test.action.trajectory_result = query; test.action.restart = restart;
            test.action.destination_column = 4; test.action.destination_row = row; test.action.core_goal_restart = restart;
            test.strategy_low = 7; result.push_back(test);
        }
    for (int side : {0, 1}) for (int foul : {4, 5}) for (int caution : {5, 10, 11})
        for (auto seed : {0u, 0x80000000u}) for (unsigned position : {0u, 1u, 2u, 3u}) for (auto word : {0u, 1u, 65535u}) {
            auto test = core_case(side, 31); test.action.target = 1; test.action.target_quality = 300;
            test.action.foul = foul; test.action.caution = caution; test.action.contact = 1; test.action.protected_action = 1;
            test.action.context_contact = 1; test.action.core_queue = true; test.action.core_queue_entries = 1;
            test.action.core_profile_result = 10; test.random_seed = seed; test.pool_position = position; test.strategy_low = word;
            result.push_back(test);
        }
    for (int side : {0, 1}) for (int controller : {0, 1, 2}) for (int contact : {0, 1}) {
        auto test = core_case(side, 31); test.action.target = 1; test.action.target_quality = 300;
        test.action.foul = 4; test.action.severity = 127; test.action.context_contact = 1; test.action.contact = contact;
        test.action.controller = controller; test.action.action = 44; test.custom_pool = true;
        test.pool_words = {1,1,1,0,100,1,0,1}; result.push_back(test);
    }
    for (int side : {0, 1}) for (int row : {0, 11}) for (int foul : {1, 7, 8})
        for (int word : {0, 1, 10}) for (int taker : {0, 1}) {
            auto test = core_case(side, 31); test.row = row; test.action.target = 1; test.action.target_quality = 300;
            test.action.foul = foul; test.action.severity = 0; test.action.context_contact = 1;
            test.action.free_taker = taker; test.strategy_low = unsigned(word); test.action.core_staff_trait = 10; result.push_back(test);
        }
    for (const auto& pair : std::array<std::array<float, 2>, 4>{{{32767,-32768}, {-32768,32767}, {10000,0}, {0,10000}}}) {
        auto test = core_case(0, 31); test.action.attribute = pair[0]; test.action.second_attribute = pair[1];
        test.action.split_attribute = true; test.strategy_low = 7; result.push_back(test);
    }
    for (int side : {-1, 2}) for (int counter : {0, -1}) {
        auto test = core_case(side, 54); test.action.receiver = 0; test.action.opposition = counter; test.action.core_flag = 0;
        test.strategy_low = 7; result.push_back(test);
    }
    for (int kind : {52, 55}) for (int wait : {-128, 0, 5, 127}) {
        auto test = core_case(0, kind); test.action.core_wait_split = true; test.action.core_wait_after = wait;
        test.strategy_low = 1; result.push_back(test);
    }
    for (int kind : {6, 11, 31, 54, 61, 93}) {
        auto test = core_case(0, kind); test.mutate = true; test.strategy_low = 7; result.push_back(test);
        test = core_case(1, kind); test.mutate_after_step = true; test.strategy_low = 7; result.push_back(test);
    }
    for (int kind : {31, 61}) for (unsigned position : {0u, 1u}) {
        auto test = core_case(0, kind); test.factor = 2.5; test.action.custom_core_coefficients = true;
        test.action.core_distance_weight = -100000; test.action.core_flight_weight = -100000;
        test.first_role = 1; test.action.heading = 1; test.pool_position = position; test.strategy_low = 7; result.push_back(test);
    }
    for (int kind : {6, 31, 61}) for (double metric : {0.0, std::numeric_limits<double>::quiet_NaN()}) {
        auto test = core_case(0, kind); test.action.phase = 5; test.factor = metric; test.action.core_weather = 1; result.push_back(test);
    }
    constexpr std::array<std::array<int, 4>, 7> comment_paths{{
        {4,5,4,5}, {4,5,0,5}, {4,5,8,5}, {4,5,4,7}, {4,5,4,3}, {4,10,0,10}, {4,10,8,10}}};
    for (int kind : {11, 30, 31, 60, 61, 93}) for (int side : {0, 1}) for (int phase : {0, 1})
        for (int control : {0, 1}) for (int previous : {0, 1}) for (const auto& path : comment_paths) {
            auto test = core_case(side, kind); test.column = path[0]; test.row = side ? 11 - path[1] : path[1];
            test.action.destination_column = path[2]; test.action.destination_row = side ? 11 - path[3] : path[3];
            test.action.phase = phase; test.action.core_flag = control; test.action.core_previous = previous;
            test.first_role = 4; test.second_role = 0; test.action.heading = 4; test.strategy_low = 7; result.push_back(test);
        }
    for (int kind : {6, 7, 8, 94}) for (int side : {0, 1}) for (int from : {0, 8}) for (int to : {0, 4, 8})
        for (int previous : {0, 1}) {
            auto test = core_case(side, kind); test.column = from; test.action.destination_column = to;
            test.action.destination_row = test.row; test.action.core_previous = previous; test.first_role = 4;
            test.action.heading = 4; test.strategy_low = 7; result.push_back(test);
        }
    for (int kind : {9, 91}) for (int side : {0, 1}) for (int column : {0, 8}) for (int row : {-1, 12}) for (int word : {0, 1}) {
        auto test = core_case(side, kind); test.action.destination_column = column; test.action.destination_row = row;
        test.strategy_low = unsigned(word); test.first_role = 4; test.action.heading = 4; result.push_back(test);
    }
    for (int kind : {52, 55}) for (int side : {0, 1}) for (int query : {0, 1}) for (double metric : {1.25, 4.0}) {
        auto test = core_case(side, kind); test.second_role = unsigned(query); test.factor = metric; result.push_back(test);
    }
    // Contextual reports require a long duration and a separate one-in-three
    // draw. Vary the pitch metric independently of the initial flight metric.
    for (int side : {0, 1}) for (int column : {0, 2, 3, 4, 6, 8}) for (int row : {0, 4, 5, 6, 7, 8, 9, 11})
        for (unsigned word : {0u, 3u}) for (bool constant_distance : {false, true}) {
            auto test = core_case(side, 31); test.column = column; test.row = row; test.factor = 40;
            test.first_role = 4; test.action.heading = 4; test.strategy_low = word;
            test.action.core_reseed = 0;
            test.action.custom_distance = constant_distance; test.action.distance = 40; result.push_back(test);
        }
    for (int action : {15, 16, 17, 18}) {
        auto test = core_case(0, 31); test.action.actor_action = action; test.factor = 40;
        test.first_role = 4; test.strategy_low = 0; result.push_back(test);
    }
    constexpr std::array<std::array<std::uint16_t, 8>, 3> accuracy_words{{
        {0,1,7,31,0,1,100,7}, {31,0,100,1,7,65535,0,7}, {65535,1,31,0,1,100,7,0}}};
    for (int side : {0, 1}) for (int kind : {6, 7, 8, 31, 54, 94}) for (int row : {1, 5, 7, 10})
        for (unsigned role : {1u, 4u}) for (int receiver : {0, 1}) for (const auto& words : accuracy_words) {
            // The original kind31 commentary dereferences its recipient.
            // Null recipients belong to the crossing/unaddressed action kinds.
            if (kind == 31 && receiver == 0) continue;
            auto test = core_case(side, kind); test.row = row; test.first_role = role; test.factor = 15;
            test.action.receiver = receiver; test.action.actor_skill = -100; test.action.heading = 4;
            test.action.actor_bias = -200;
            test.custom_pool = true; test.pool_words = words; result.push_back(test);
        }
    for (int side : {0, 1}) for (int action : {12, 21, 34, 41, 42, 44, 46, 47})
        for (int gate = 0; gate < 8; ++gate) {
            auto test = core_case(side, 31); test.action.target = 1; test.action.target_quality = 800;
            test.action.action = action; test.action.time = 990; test.first_role = 4;
            test.action.target_attempts = gate == 1 ? 0 : 20; test.action.target_successes = gate == 2 ? 1 : 19;
            test.action.core_recoveries = gate == 3 ? 0 : 2; test.action.core_feedback_count = gate == 4 ? 0 : 1;
            test.action.target_condition = gate == 5 ? 6000 : 7000; test.action.temporal_flag = gate == 6;
            test.action.core_feedback_done = gate == 7; test.strategy_low = 1; result.push_back(test);
        }
    for (int side : {0, 1}) for (int phase : {-1, 4, 7}) for (int quality : {300, 800}) {
        auto test = core_case(side, 31); test.action.target = 1; test.action.target_quality = quality;
        test.action.core_phase_after_tally = phase; test.first_role = 4; test.strategy_low = 1; result.push_back(test);
    }
    for (int side : {0, 1}) for (unsigned position : {0u, 1u, 2u, 3u}) for (int quality : {250, 300, 800}) {
        auto test = core_case(side, 31); test.row = side ? 11 : 0; test.action.target = 1;
        test.action.target_quality = quality; test.action.core_ball_bias = 1; test.action.target_metric = 0;
        test.action.actor_metric = 100; test.action.action = 21; test.pool_position = position;
        test.strategy_low = 1; result.push_back(test);
    }
    for (int side : {0, 1}) for (int row : {0, 11}) for (int receiver_row : {0, 11}) for (double metric : {1.0, 4.0}) {
        auto test = core_case(side, 11); test.row = row; test.action.core_receiver_side = side;
        test.action.core_receiver_column = 4; test.action.core_receiver_row = receiver_row;
        test.factor = metric; test.first_role = 4; test.action.heading = 4; test.strategy_low = 0; result.push_back(test);
    }
    for (int side : {0, 1}) for (int kind : {31, 61}) for (double metric : {1.0, 20.0}) {
        auto test = core_case(side, kind); test.action.destination_column = 4; test.action.destination_row = side ? -1 : 12;
        test.factor = metric; test.first_role = 4; test.action.heading = 4; test.strategy_low = 0; result.push_back(test);
    }
    for (int side : {0, 1}) for (int column : {1, 4, 7}) for (int row : {0, 3, 5, 8, 11})
        for (int foul : {0, 1, 2, 3, 7}) for (int contact : {0, 1}) for (unsigned word : {0u, 1u}) {
            auto test = core_case(side, 31); test.column = column; test.row = row; test.action.target = 1;
            test.action.target_quality = 300; test.action.foul = foul; test.action.contact = contact;
            test.action.context_contact = int(word); test.action.action = 44; test.action.diagnostic = 1;
            test.action.core_reseed = int(word); test.action.core_target_morale = 1; test.strategy_low = word;
            test.action.severity = -128; result.push_back(test);
        }
    for (int side : {0, 1}) for (int quality : {0, 1, 199, 800}) for (int phase : {0, 7})
        for (int query : {0, 1}) for (unsigned position = 0; position < 8; ++position) {
            auto test = core_case(side, 31); test.action.target = 1; test.action.target_quality = quality;
            test.action.actor_bias = -200; test.action.request_restart = 7; test.action.core_phase_after_tally = phase;
            test.action.core_goal_query = query; test.action.core_ball_bias = 1; test.pool_position = position;
            test.strategy_low = 1; result.push_back(test);
        }
    for (int side : {0, 1}) for (int old_morale : {1, 10}) {
        auto test = core_case(side, 31); test.action.core_actor_morale = old_morale; test.action.core_target_morale = old_morale;
        test.action.core_reseed = 0; test.strategy_low = 0; result.push_back(test);
        test.row = side ? 0 : 11; test.action.target = 1; test.action.target_quality = 300;
        test.action.foul = 1; test.action.severity = -128; test.action.context_contact = 1; result.push_back(test);
    }
    for (int side : {0, 1}) {
        auto test = core_case(side, 31); test.action.target = 1; test.action.target_quality = 300;
        test.action.foul = 4; test.action.caution = 10; test.action.contact = 1; test.action.context_contact = 1;
        test.action.core_queue = true; test.action.core_queue_gate = 14; test.action.core_team_policy = 1;
        test.action.core_profile_result = 10; test.strategy_low = 1; result.push_back(test);
        test = core_case(side, 31); test.row = side ? 0 : 11; test.action.target = 1; test.action.target_quality = 300;
        test.action.foul = 1; test.action.severity = -128; test.action.context_contact = 1;
        test.action.core_null_match_at_profile = true; test.action.core_reseed = 0; test.strategy_low = 1; result.push_back(test);
    }
    for (int side : {0, 1}) for (int from : {0, 8}) for (int to : {1, 7}) {
        auto test = core_case(side, 9); test.column = from; test.row = side ? 2 : 9;
        test.action.destination_column = to; test.action.destination_row = test.row;
        test.primary.edge_query = 1; test.first_role = 4; test.action.heading = 4;
        test.action.core_reseed = 0; test.strategy_low = 0; result.push_back(test);
    }
    for (int side : {0, 1}) for (unsigned role : {1u, 4u}) {
        auto test = core_case(side, 54); test.first_role = role; test.secondary.moving = 1; result.push_back(test);
        test = core_case(side, 31); test.action.actor_bias = -200; test.first_role = role; test.factor = 15;
        test.action.core_staff_trait = 1; test.action.core_actor_morale = 1; test.action.core_reset_pool_at_tally = true;
        test.custom_pool = true; test.pool_words = {0,1,0,0,0,1,0,0}; result.push_back(test);
    }
    for (int side : {0, 1}) {
        auto test = core_case(side, 31); test.row = side ? 0 : 11; test.action.target = 1; test.action.target_quality = 300;
        test.action.foul = 1; test.action.severity = -128; test.action.context_contact = 1;
        test.action.core_negative_penalty_draw = true; test.strategy_low = 0; result.push_back(test);
        test = core_case(side, 31); test.action.target = 1; test.action.target_quality = 800;
        test.action.possession = false; test.action.core_ball_bias = 1; test.first_role = 4; test.strategy_low = 1; result.push_back(test);
    }
    // Earlier caller fixtures used a modeled core with an unspecified kind.
    // Give those default completion cases a valid unaddressed action now that
    // the full core executes; invalid kinds have dedicated fatal-prefix checks.
    for (auto& test : result) if (test.entry == 0x10245220 && test.argument == 0) test.argument = 9;
    for (auto entry : {0x10245220u, 0x10245870u}) {
        Case test{entry, 0, 4, 5, 9}; test.action.defender_restart_after_query = 7; result.push_back(test);
    }
    for (int side : {0, 1}) for (auto seed : {0u, 0x80000000u, 0xffffffb0u}) for (unsigned position : {0u, 1u}) {
        Case test{0x10245220, side, 4, 5, 9}; test.action.receiver = 1;
        test.action.receiver_trait0 = 127; test.action.receiver_trait1 = 127;
        test.action.parent_sampling_reset = true; test.action.receiver_busy_after_core_seed = true;
        test.random_seed = seed; test.pool_position = position; test.strategy_low = 1; result.push_back(test);
    }
    for (auto entry : {0x10245220u, 0x10245870u}) for (auto seed : {0xffff0000u, 0xffff0001u}) {
        Case test{entry, 0, 4, 5, 9}; test.action.defender = 1; test.action.receiver = 1;
        test.action.parent_sampling_reset = true; test.action.custom_coefficient = true; test.action.coefficient = 100000;
        test.action.custom_distance = true; test.action.distance = 10; test.random_seed = seed;
        test.strategy_low = 1; result.push_back(test);
    }
    return result;
}
}

int main(int argc, char** argv) {
    if (argc < 2 || argc > 3) return 2;
    try {
        auto base = std::make_unique<State>(); load_program(*base, argv[1]); dispatch_probe = probe;
        if (argc == 3 && std::string_view(argv[2]) == "--routes-only") {
            generated_mode = true; require_production_routes(*base); return 0;
        }
        generated_mode = argc == 3 && std::string_view(argv[2]) == "--production";
        auto tests = cases();
        if (argc == 3 && !generated_mode) {
            const std::string_view option(argv[2]);
            if (option == "--actions-block") std::erase_if(tests, [](const Case& test) { return !action_entry(test.entry); });
            else {
                if (!option.starts_with("--entry=")) throw std::runtime_error("Unknown MATCH_PL checker option");
                const auto entry = std::stoul(std::string(option.substr(8)), nullptr, 0);
                std::erase_if(tests, [entry](const Case& test) { return test.entry != entry; });
            }
            if (tests.empty()) throw std::runtime_error("No fixture cases for selected MATCH_PL entry");
        }
        if (generated_mode) std::erase_if(tests, [](const Case& test) {
            return std::ranges::find(production_entries, test.entry) == std::end(production_entries);
        });
        unsigned failures = 0, completed = 0;
        std::map<std::uint32_t, unsigned> compared;
        for (const auto& test : tests) {
            active = test;
            auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize_case(c);
            const auto found = compare_routine_call(*state, active.entry, original, rewrite,
                RoutineChecker::result_r3 | RoutineChecker::result_f1);
            if (found.empty()) { ++completed; ++compared[active.entry]; }
            else {
                std::cout << "DIFFERS " << hex_address(active.entry) << " case=" << completed + failures << " side=" << active.side
                          << " column=" << active.column << " row=" << active.row << " argument=" << active.argument << '\n';
                if (active.entry == 0x10278220) std::cout << "  primary restart=" << active.primary.restart << " role=" << active.first_role << " mobility=" << active.primary.mobility << " trait109=" << active.primary.trait109 << " pool=" << active.pool_position << '\n';
                for (const auto& line : found) std::cout << "  " << line << '\n';
                if (active.entry == 0x10278220 || action_entry(active.entry) || core_entry(active.entry)) explain_primary_trace(*state);
                if (++failures == 5) break;
            }
        }
        if (!failures && std::ranges::any_of(tests, [](const Case& test) { return test.entry == 0x102836a0; })) {
            unsigned prefixes = 0;
            for (int side : {0, 1}) {
                active = fatal_case(side);
                auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize_case(c);
                const auto found = compare_routine_call(*state, active.entry, fatal_original, fatal_rewrite,
                    RoutineChecker::result_r3 | RoutineChecker::result_f1);
                if (found.empty()) ++prefixes;
                else { ++failures; for (const auto& line : found) std::cout << "FATAL PREFIX " << line << '\n'; }
            }
            active = fatal_case(0);
            auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize_case(c);
            const auto spoiled = compare_routine_call(*state, active.entry, fatal_original, corrupt_fatal_prefix,
                RoutineChecker::result_r3 | RoutineChecker::result_f1);
            if (!std::ranges::any_of(spoiled, [](const auto& line) { return line.starts_with("r3 "); })) {
                ++failures; std::cout << "FAIL fatal prefix corruption was not detected\n";
            } else std::cout << "ok fatal prefix result corruption detected\n";
            std::cout << prefixes << "/2 expected fatal-boundary prefix comparisons (excluded from guest returns)\n";
        }
        if (!failures) {
            if (std::ranges::any_of(tests, [](const Case& test) { return core_entry(test.entry); })) {
                unsigned prefixes = 0;
                for (int side : {0, 1}) for (int kind : {10, 32, 127}) {
                    active = core_fatal_case(side, kind);
                    auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize_case(c);
                    const auto found = compare_routine_call(*state, active.entry, fatal_original, fatal_rewrite,
                        RoutineChecker::result_r3 | RoutineChecker::result_f1);
                    if (found.empty()) ++prefixes;
                    else { ++failures; for (const auto& line : found) std::cout << "CORE FATAL PREFIX " << line << '\n'; }
                }
                active = core_fatal_case(0);
                auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize_case(c);
                const auto spoiled = compare_routine_call(*state, active.entry, fatal_original, corrupt_fatal_prefix,
                    RoutineChecker::result_r3 | RoutineChecker::result_f1);
                if (!std::ranges::any_of(spoiled, [](const auto& line) { return line.starts_with("r3 "); })) {
                    ++failures; std::cout << "FAIL core fatal prefix corruption was not detected\n";
                } else std::cout << "ok core fatal prefix corruption detected\n";
                std::cout << prefixes << "/6 core fatal-boundary prefix comparisons (excluded from guest returns)\n";
                active = {0x10247030, 0, 4, 5, 31}; active.action.target = 0; active.action.receiver = 1; active.action.trajectory_result = 0;
                auto rng_state = cm::semantic_io_checks::clone(*base); Cpu rng_cpu(*rng_state); initialize_case(rng_cpu);
                const auto changed = compare_routine_call(*rng_state, active.entry, original, corrupt_core_rng,
                    RoutineChecker::result_r3 | RoutineChecker::result_f1);
                if (!std::ranges::any_of(changed, [](const auto& line) { return line.starts_with("byte "); })) {
                    ++failures; std::cout << "FAIL core RNG corruption was not detected\n";
                } else std::cout << "ok core RNG corruption detected\n";
            }
        }
        if (!failures) {
            active = {0x102270f0, 0, 4, 5, 1}; active.force_exit = true;
            auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize_case(c);
            const auto found = compare_routine_call(*state, active.entry, original, rewrite,
                RoutineChecker::result_r3 | RoutineChecker::result_f1);
            const auto rejected = std::ranges::count_if(found, [](const auto& line) {
                return line.find("MATCH_PL comparison left through non-returning control flow") != std::string::npos;
            });
            if (rejected != 2) { ++failures; std::cout << "FAIL non-returning control was counted as a return\n"; }
            else std::cout << "ok matching non-returning controls rejected\n";
        }
        if (!failures) for (bool memory : {false, true}) {
            active = {memory ? 0x102238a0u : 0x1028acf0u};
            auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize_case(c);
            const auto found = compare_routine_call(*state, active.entry, original,
                memory ? corrupt_memory : corrupt, RoutineChecker::result_r3 | RoutineChecker::result_f1);
            const auto prefix = memory ? "byte " : "r3 ";
            const bool detected = std::ranges::any_of(found, [&](const auto& line) { return line.starts_with(prefix); });
            if (!detected) { ++failures; std::cout << "FAIL negative control\n"; }
            else std::cout << "ok " << (memory ? "memory" : "result") << " corruption detected\n";
        }
        if (!failures && generated_mode) require_production_routes(*base);
        for (const auto& [entry, count] : compared)
            std::cout << hex_address(entry) << " returned " << count << '\n';
        std::cout << completed << '/' << tests.size() << " returned comparisons, " << failures << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
