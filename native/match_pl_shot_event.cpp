#include "match_pl_shot_event_detail.hpp"
#include "match_pl_shot_event.hpp"

namespace cm::aot::match_pl::shot_event_detail {
using namespace guest;

void open(guest::Cpu&, GuestNativeScope&);
void assess_opening(guest::Cpu&);
void prepare_shot(guest::Cpu&);
void choose_shot_opposition(guest::Cpu&);
void begin_goal_statistics(guest::Cpu&);
void settle_goal_statistics(guest::Cpu&);
void select_goal_announcement(guest::Cpu&);
void record_direct_goal(guest::Cpu&);
void resolve_blocked_shot(guest::Cpu&);
void react_to_shot_blocker(guest::Cpu&);
void record_alternate_goal(guest::Cpu&);
void account_goal_keeper(guest::Cpu&);
void finish_shot_blocker(guest::Cpu&);
void route_shot_after_block(guest::Cpu&);
void publish_blocker_followup(guest::Cpu&);
void describe_blocker_restart(guest::Cpu&);
void conclude_blocker_followup(guest::Cpu&);
void describe_shot_deflection(guest::Cpu&);
void apply_deflection_effects(guest::Cpu&);
void describe_shot_attempt(guest::Cpu&);
void route_shot_restart(guest::Cpu&);
void prepare_shot_release(guest::Cpu&);
void classify_shot_flight(guest::Cpu&);
void account_shot_flight(guest::Cpu&);
void prepare_goalkeeper_followthrough(guest::Cpu&);
void publish_shot_strength(guest::Cpu&);
void resolve_alternate_shot(guest::Cpu&);
void describe_missed_shot(guest::Cpu&);
void describe_open_play_miss(guest::Cpu&);
void route_missed_shot_outcome(guest::Cpu&);
void finish_shot_out_of_play(guest::Cpu&);
void choose_close_miss(guest::Cpu&);
void choose_miss_narration(guest::Cpu&);
void describe_simple_miss(guest::Cpu&);
void describe_single_context_miss(guest::Cpu&);
void describe_double_context_miss(guest::Cpu&);
void describe_power_miss(guest::Cpu&);
void finish_close_miss(guest::Cpu&);
void describe_power_miss_recipient(guest::Cpu&);
void choose_low_power_miss(guest::Cpu&);
void describe_low_power_single_miss(guest::Cpu&);
void describe_low_power_fallback(guest::Cpu&);
void describe_low_power_miss_recipient(guest::Cpu&);
void describe_low_power_double_miss(guest::Cpu&);
void choose_target_shot_narration(guest::Cpu&);
void describe_on_target_miss(guest::Cpu&);
void finish_on_target_miss(guest::Cpu&);
void evaluate_shot_goal_threshold(guest::Cpu&);
void describe_direct_goal_celebration(guest::Cpu&);
void describe_goal_celebration(guest::Cpu&);
void choose_goal_provider_context(guest::Cpu&);
void describe_provider_goal(guest::Cpu&);
void describe_goal_provider_pair(guest::Cpu&);
void describe_goal_celebration_style(guest::Cpu&);
void evaluate_goal_incident(guest::Cpu&);
void choose_goal_incident_retry(guest::Cpu&);
void describe_goal_roles(guest::Cpu&);
void describe_goal_distance(guest::Cpu&);
void describe_goal_milestones(guest::Cpu&);
void describe_goal_keeper_note(guest::Cpu&);
void select_goal_confirmation(guest::Cpu&);
void describe_goal_score_change(guest::Cpu&);
void resolve_saved_shot(guest::Cpu&);
void resolve_final_saved_shot(guest::Cpu&);
void report_goal_confirmation(guest::Cpu&);
void assess_shot_blocker(guest::Cpu&);
void announce_goal(guest::Cpu&);
void publish_goal(guest::Cpu&);
void return_shot(guest::Cpu&);
void react_to_goal_margin(guest::Cpu&);
void react_to_close_goal(guest::Cpu&);
void reset_after_goal(guest::Cpu&);

namespace {
constexpr auto entry = shot_event_entry;
constexpr auto end = 0x1026a1b0;
constexpr auto epilogue = 0x1026a190;

bool run_phase(Cpu& c) {
    switch (c.s.pc) {
    case 0x102578dc:
        assess_opening(c); return true;
    case 0x10257a78:
        prepare_shot(c); return true;
    case 0x10257d30:
        choose_shot_opposition(c); return true;
    case 0x10258004:
        assess_shot_blocker(c); return true;
    case 0x10258334:
        resolve_blocked_shot(c); return true;
    case 0x102586d0:
        react_to_shot_blocker(c); return true;
    case 0x10258b64:
        publish_blocker_followup(c); return true;
    case 0x10258f2c:
        describe_blocker_restart(c); return true;
    case 0x102591a0:
        conclude_blocker_followup(c); return true;
    case 0x102594a4: case 0x10259580:
        finish_shot_blocker(c); return true;
    case 0x102595e0:
        route_shot_after_block(c); return true;
    case 0x10259958:
        describe_shot_deflection(c); return true;
    case 0x1025a11c: case 0x1025a908:
        apply_deflection_effects(c); return true;
    case 0x1025a968:
        describe_shot_attempt(c); return true;
    case 0x1025ad44:
        route_shot_restart(c); return true;
    case 0x1025aef8:
        prepare_shot_release(c); return true;
    case 0x1025b18c:
        classify_shot_flight(c); return true;
    case 0x1025b4b8:
        account_shot_flight(c); return true;
    case 0x1025b8a4:
        prepare_goalkeeper_followthrough(c); return true;
    case 0x1025bdf0:
        publish_shot_strength(c); return true;
    case 0x1025bff0:
        resolve_alternate_shot(c); return true;
    case 0x1025c28c:
        describe_missed_shot(c); return true;
    case 0x1025c60c:
        describe_open_play_miss(c); return true;
    case 0x1025d164:
        route_missed_shot_outcome(c); return true;
    case 0x1025d26c:
        finish_shot_out_of_play(c); return true;
    case 0x1025d4d4:
        choose_close_miss(c); return true;
    case 0x1025d778:
        choose_miss_narration(c); return true;
    case 0x1025d9d0:
        describe_single_context_miss(c); return true;
    case 0x1025dec0:
        describe_double_context_miss(c); return true;
    case 0x1025e630: case 0x1025e65c: case 0x1025e6a4:
        describe_simple_miss(c); return true;
    case 0x1025e6d0:
        describe_power_miss(c); return true;
    case 0x1025e998:
        describe_power_miss_recipient(c); return true;
    case 0x1025ec74:
        choose_low_power_miss(c); return true;
    case 0x1025ee9c: case 0x1025fb88:
        describe_low_power_single_miss(c); return true;
    case 0x1025f0c8:
        describe_low_power_miss_recipient(c); return true;
    case 0x1025f38c: case 0x1025fdb4:
        describe_low_power_double_miss(c); return true;
    case 0x1025f830: case 0x10260258:
        describe_low_power_fallback(c); return true;
    case 0x1025f858:
        finish_close_miss(c); return true;
    case 0x1025f914:
        choose_target_shot_narration(c); return true;
    case 0x10260284:
        describe_on_target_miss(c); return true;
    case 0x10262154:
        finish_on_target_miss(c); return true;
    case 0x10262198:
        evaluate_shot_goal_threshold(c); return true;
    case 0x102626bc:
        begin_goal_statistics(c); return true;
    case 0x10262900:
        record_direct_goal(c); return true;
    case 0x10262b60:
        record_alternate_goal(c); return true;
    case 0x10262e08:
        account_goal_keeper(c); return true;
    case 0x10263464:
        settle_goal_statistics(c); return true;
    case 0x10263618:
        select_goal_announcement(c); return true;
    case 0x102636d8:
        announce_goal(c); return true;
    case 0x10263700:
        describe_direct_goal_celebration(c); return true;
    case 0x102637b4:
        describe_goal_celebration(c); return true;
    case 0x10263d20:
        choose_goal_provider_context(c); return true;
    case 0x10263ed8:
        describe_goal_provider_pair(c); return true;
    case 0x10264180:
        describe_goal_roles(c); return true;
    case 0x10264a50: case 0x10264a94:
        describe_provider_goal(c); return true;
    case 0x10264bb0:
        describe_goal_celebration_style(c); return true;
    case 0x10264c1c: case 0x102651bc: case 0x10265618: case 0x10265f10: case 0x10266350: case 0x10266d68:
        evaluate_goal_incident(c); return true;
    case 0x10265000: case 0x102651b8:
        choose_goal_incident_retry(c); return true;
    case 0x102655a0:
        describe_goal_distance(c); return true;
    case 0x102659fc:
        describe_goal_milestones(c); return true;
    case 0x102662f4:
        describe_goal_keeper_note(c); return true;
    case 0x10266734: case 0x10266d44:
        describe_goal_score_change(c); return true;
    case 0x102670f4:
        select_goal_confirmation(c); return true;
    case 0x10267154:
        publish_goal(c); return true;
    case 0x102671c4:
        report_goal_confirmation(c); return true;
    case 0x10267640:
        react_to_goal_margin(c); return true;
    case 0x10267794:
        react_to_close_goal(c); return true;
    case 0x10267b28:
        reset_after_goal(c); return true;
    case 0x10267b6c: case 0x10267f9c: case 0x10268178: case 0x102686f0:
        resolve_saved_shot(c); return true;
    case 0x102687e8: case 0x1026a184:
        resolve_final_saved_shot(c); return true;
    case 0x102586cc: case 0x1025c234: case 0x1025d240:
        c.s.pc = epilogue;
        return true;
    default:
        return false;
    }
}
}

void execute_shot_activation(Cpu& c);

static void run_shot_phases(Cpu& c, std::uint32_t stop) {
    while (c.s.pc != stop) {
        const auto pc = c.s.pc;
        if (pc == epilogue) {
            return_shot(c);
            return;
        }
        if (pc == entry) execute_shot_activation(c);
        else if (!run_phase(c)) {
            if (pc > entry && pc < end)
                throw std::runtime_error("Unsupported shot continuation " + hex_address(pc));
            // A mutable guest table can transfer outside this routine with
            // the link register still pointing into the live activation.
            run_until(c.s, c.lr());
        }
    }
}

void execute_shot_activation(Cpu& c) {
    const auto caller = c.lr();
    GuestNativeScope scope(c.s, entry);
    open(c, scope);
    run_shot_phases(c, caller);
}

void resume_shot_activation(Cpu& c, std::uint32_t stop) {
    GuestNativeScope scope(c.s, entry);
    scope.frame(c.sp());
    run_shot_phases(c, stop);
}
}

namespace cm::aot::match_pl {
void execute_shot_event(guest::Cpu& c) {
    shot_event_detail::execute_shot_activation(c);
}

void run_shot_event_until(State& state, std::uint32_t stop) {
    if (state.pc == shot_event_entry || !shot_event_uses_cpp(state)) {
        run_until(state, stop);
        return;
    }
    guest::Cpu cpu(state);
    shot_event_detail::resume_shot_activation(cpu, stop);
}
}
