#include "player_movement.hpp"
#include "guest_context_transfer.hpp"

namespace cm::aot::player_movement {
void move(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    const auto caller=std::uint32_t(state.r<4>(4128));
    // Keep the original function/frame identity alive across all named phases.
    // A nonlocal transfer abandons this invocation without running its return.
    GuestNativeScope scope(state,entry);
    if(prepare_movement(state,dependencies,observer)==Preparation::returned)return;
    scope.frame(std::uint32_t(state.r<4>(4)));
    while(state.pc!=caller) {
        if(state.pc<entry || state.pc>=end) {
            // An altered live table can branch to an external dependency. Its
            // existing LR/arguments are retained; no additional call instruction
            // or own-body dispatch is introduced.
            const auto continuation=std::uint32_t(state.r<4>(4128));
            if(dependencies)dependencies->call(state,state.pc,continuation);
            else run_until(state,continuation);
            continue;
        }
        switch(state.pc) {
        case entry:move(state,dependencies,observer);break;
        case noncarrier_phase:continue_noncarrier(state,dependencies,observer);break;
        case carrier_phase:continue_carrier_sampling(state,dependencies,observer);break;
        case carrier_decision_phase:continue_carrier_environment(state,dependencies,observer);break;
        case carrier_interaction_phase:continue_carrier_interaction(state,dependencies,observer);break;
        case carrier_contact_phase:continue_carrier_contact(state,dependencies,observer);break;
        case carrier_failed_contact_phase:case carrier_resolved_contact_phase:continue_carrier_contact_outcome(state,dependencies,observer);break;
        case 0x10230420:case 0x1023044c:case 0x10230478:case 0x102304a4:case 0x102304d0:case 0x102304fc:case 0x10230528:case 0x10230554:case 0x10230580:case 0x102305ac:case 0x102305d8:case 0x10230604:
        case carrier_contact_decision_phase:continue_carrier_contact_action(state,dependencies,observer);break;
        case carrier_contact_followup_phase:continue_carrier_contact_followup(state,dependencies,observer);break;
        case carrier_restart_selection_phase:continue_carrier_restart(state,dependencies,observer);break;
        case carrier_no_contact_phase:continue_carrier_possession(state,dependencies,observer);break;
        case carrier_play_decision_phase:continue_carrier_play_preparation(state,dependencies,observer);break;
        case carrier_play_ready_phase:continue_carrier_play_bias(state,dependencies,observer);break;
        case carrier_kind17_phase:continue_carrier_kind17_contact(state,dependencies,observer);break;
        case carrier_kind17_clear_phase:continue_carrier_kind17_clear(state,dependencies,observer);break;
        case carrier_kind18_phase:continue_carrier_kind18(state,dependencies,observer);break;
        case carrier_kind15_phase:case carrier_other_kind_phase:continue_carrier_simple_play(state,dependencies,observer);break;
        case carrier_repeated_contact_phase:continue_carrier_repeated_contact(state,dependencies,observer);break;
        case 0x10233b1c:case 0x10233b48:case 0x10233b74:case 0x10233ba0:case 0x10233bcc:case 0x10233bf8:case 0x10233c24:case 0x10233c50:case 0x10233c7c:case 0x10233ca8:case 0x10233cd4:case 0x10233d00:case 0x10233d9c:case 0x10233dd8:case 0x10233e14:continue_carrier_repeated_table(state,dependencies,observer);break;
        case carrier_transfer_phase:continue_carrier_transfer(state,dependencies,observer);break;
        case carrier_distance_phase:case 0x102346e4:continue_carrier_distance(state,dependencies,observer);break;
        case carrier_statistics_phase:continue_carrier_statistics(state,dependencies,observer);break;
        case carrier_ratings_phase:continue_carrier_ratings(state,dependencies,observer);break;
        case carrier_null_contact_phase:continue_carrier_null_contact(state,dependencies,observer);break;
        case common_return_phase:return_movement_frame(state,dependencies,observer);break;
        default:throw std::runtime_error("Unsupported movement continuation");
        }
    }
}
}
