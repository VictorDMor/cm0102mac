#include "ball_contact.hpp"
#include "aot_runtime.hpp"
#include "guest_context_transfer.hpp"

namespace cm::aot::ball_contact {
void describe(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    const auto caller=std::uint32_t(state.r<4>(4128));
    GuestNativeScope scope(state,description_entry);
    prepare_description(state,dependencies,observer);
    scope.frame(std::uint32_t(state.r<4>(4)));
    while(state.pc!=caller) {
        if(state.pc<description_entry||state.pc>=0x101f6940) {
            const auto continuation=std::uint32_t(state.r<4>(4128));
            if(dependencies)dependencies->call(state,state.pc,continuation);
            else run_until(state,continuation);
            continue;
        }
        switch(state.pc) {
        case description_entry:describe(state,dependencies,observer);break;
        case description_exit_phase:select_description_exit(state,dependencies,observer);break;
        case description_players_phase:prepare_description_players(state,dependencies,observer);break;
        case description_weight_phase:
        case 0x101f5ab8:case 0x101f5afc:case 0x101f5b40:case 0x101f5b9c:case 0x101f5be0:case 0x101f5c3c:case 0x101f5c80:
        case 0x101f5d10:case 0x101f5d54:case 0x101f5d98:case 0x101f5ddc:case 0x101f5e20:case 0x101f5e64:
            adjust_description_weight(state,dependencies,observer);break;
        case description_assessment_phase:assess_description_weight(state,dependencies,observer);break;
        case description_reaction_phase:
        case 0x101f5fa4:case 0x101f5fd4:case 0x101f6004:case 0x101f6034:case 0x101f6064:case 0x101f6094:case 0x101f60c4:
        case 0x101f60f4:case 0x101f6124:case 0x101f6168:case 0x101f61c4:case 0x101f61f4:case 0x101f6224:case 0x101f6254:case 0x101f6284:
        case 0x101f62b4:case 0x101f62f4:case 0x101f6324:case 0x101f6354:case 0x101f6384:case 0x101f63b0:case 0x101f63e0:
        case 0x101f642c:case 0x101f6470:case 0x101f64b4:case 0x101f64f8:case 0x101f653c:case 0x101f6580:case 0x101f65c4:case 0x101f6608:case 0x101f664c:
        case 0x101f6690:case 0x101f66b4:case 0x101f66f8:case 0x101f673c:case 0x101f6780:case 0x101f67c4:case 0x101f6808:case 0x101f684c:case 0x101f6890:case 0x101f68d4:
            select_description_reaction(state,dependencies,observer);break;
        case description_next_phase:case description_test_phase:next_description_player(state,dependencies,observer);break;
        case description_return_phase:return_description_frame(state,dependencies,observer);break;
        default:throw std::runtime_error("Unsupported contact-description continuation");
        }
    }
}
}
