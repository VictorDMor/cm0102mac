#include "ball_action.hpp"
#include "aot_runtime.hpp"
#include "guest_context_transfer.hpp"

namespace cm::aot::ball_action {
void act(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    const auto caller=std::uint32_t(state.r<4>(4128));
    GuestNativeScope scope(state,entry);
    prepare(state,dependencies,observer);
    scope.frame(std::uint32_t(state.r<4>(4)));
    while(state.pc!=caller) {
        if(state.pc<entry || state.pc>=end) {
            // Computed guest tables retain their live destination, arguments
            // and existing LR when they transfer outside this coordinator.
            const auto continuation=std::uint32_t(state.r<4>(4128));
            if(dependencies)dependencies->call(state,state.pc,continuation);
            else run_until(state,continuation);
            continue;
        }
        switch(state.pc) {
        case entry:act(state,dependencies,observer);break;
        case orientation_phase:orient(state,dependencies,observer);break;
        case action_bias_phase:face_action(state,dependencies,observer);break;
        case area_selection_phase:select_area_action(state,dependencies,observer);break;
        case ball_state_selection_phase:select_elevated_action(state,dependencies,observer);break;
        case low_ball_selection_phase:select_low_ball_action(state,dependencies,observer);break;
        case selected_action_phase:sample_selected_action(state,dependencies,observer);break;
        case contact_selection_phase:prepare_contact(state,dependencies,observer);break;
        case contact_resolution_phase:resolve_contact(state,dependencies,observer);break;
        case contact_outcome_phase:complete_contact(state,dependencies,observer);break;
        case action_execution_phase:prepare_execution(state,dependencies,observer);break;
        case 0x1023ff6c:case 0x1023ff98:case 0x1023ffc4:case 0x1023fff0:case 0x1024001c:case 0x10240048:case 0x10240074:case 0x102400a0:case 0x102400cc:case 0x102400f8:case 0x10240124:case 0x10240150:case execution_message_phase:describe_execution(state,dependencies,observer);break;
        case receiver_resolution_phase:resolve_receiver(state,dependencies,observer);break;
        case receiver_outcome_phase:complete_receiver_contact(state,dependencies,observer);break;
        case execution_result_phase:prepare_execution_result(state,dependencies,observer);break;
        case 0x10241474:case 0x102414a0:case 0x102414cc:case 0x102414f8:case 0x10241524:case 0x10241550:case 0x1024157c:case 0x102415a8:case 0x102415d4:case 0x10241600:case 0x1024162c:case 0x10241658:case result_message_phase:describe_execution_result(state,dependencies,observer);break;
        case result_resolution_phase:resolve_execution_result(state,dependencies,observer);break;
        case result_outcome_phase:complete_execution_result(state,dependencies,observer);break;
        case unopposed_result_phase:complete_unopposed_execution(state,dependencies,observer);break;
        case no_receiver_phase:prepare_final_action(state,dependencies,observer);break;
        case final_effects_phase:complete_special_action(state,dependencies,observer);break;
        case regular_effects_phase:complete_regular_action(state,dependencies,observer);break;
        case success_reaction_phase:react_to_completed_action(state,dependencies,observer);break;
        case unsuccessful_action_phase:prepare_failed_action(state,dependencies,observer);break;
        case 0x10243690:case 0x102436bc:case 0x102436e8:case 0x10243714:case 0x10243740:case 0x1024376c:case 0x10243798:case 0x102437c4:case 0x102437f0:case 0x1024381c:case 0x10243848:case 0x10243874:case failed_message_phase:describe_failed_action(state,dependencies,observer);break;
        case 0x1024391c:case 0x10243958:case 0x10243994:case failed_comment_phase:describe_failed_comment(state,dependencies,observer);break;
        case 0x102439f4:case 0x10243a14:case 0x10243a40:case 0x10243a6c:case 0x10243a98:case 0x10243b50:case 0x10243b7c:case 0x10243ac4:case failed_fallback_phase:describe_failed_fallback(state,dependencies,observer);break;
        case failed_outcome_phase:prepare_receiver_outcome(state,dependencies,observer);break;
        case receiver_sampling_phase:sample_receiver_counter(state,dependencies,observer);break;
        case receiver_marker_phase:handle_receiver_marker(state,dependencies,observer);break;
        case receiver_position_phase:position_receiver(state,dependencies,observer);break;
        case receiver_assessment_phase:assess_receiver(state,dependencies,observer);break;
        case receiver_effects_phase:complete_receiver_outcome(state,dependencies,observer);break;
        case receiver_final_reaction_phase:react_to_receiver_outcome(state,dependencies,observer);break;
        case missing_receiver_phase:report_missing_receiver(state,dependencies,observer);break;
        case return_phase:return_action_frame(state,dependencies,observer);break;
        default:throw std::runtime_error("Unsupported ball-action continuation");
        }
    }
}
}
