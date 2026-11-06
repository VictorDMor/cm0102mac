#include "match_pl_execute_ball_action_detail.hpp"
#include "match_pl.hpp"

namespace cm::aot::match_pl::ball_action_detail {
using namespace guest;

void return_action(Cpu& c) {
    const auto caller = c.sp() + 0x150;
    c.s.pc = ball_action::return_phase;
    c.set(0, c.u32(caller + 8));
    c.set(1, caller);
    c.s.wr<8>(reg::fpr(31), c.u64(caller - 8));
    c.s.wr<8>(reg::fpr(30), c.u64(caller - 0x10));
    c.s.wr<8>(reg::fpr(29), c.u64(caller - 0x18));
    c.set_lr(c.r(0));
    auto saved = caller - 0x6c;
    for (unsigned reg = 13; reg != 32; ++reg, saved += 4) {
        c.s.wr<4>(2432, saved);
        c.set(reg, c.u32(saved));
    }
    c.s.wr<4>(2432, saved);
    c.finish();
}

using ActionPhase = void (*)(Cpu&);

static ActionPhase ball_action_phase(std::uint32_t pc) {
    using namespace ball_action;
    switch (pc) {
    case orientation_phase: return orient;
    case action_bias_phase: return face;
    case area_selection_phase: return select_area;
    case ball_state_selection_phase: return select_elevated;
    case low_ball_selection_phase: return select_low;
    case selected_action_phase: return sample_selected;
    case contact_selection_phase: return prepare_contact;
    case contact_resolution_phase: return resolve_contact;
    case contact_outcome_phase: return complete_contact;
    case action_execution_phase: return prepare_execution;
    case execution_message_phase:
    case 0x1023ff6c: case 0x1023ff98: case 0x1023ffc4: case 0x1023fff0:
    case 0x1024001c: case 0x10240048: case 0x10240074: case 0x102400a0:
    case 0x102400cc: case 0x102400f8: case 0x10240124: case 0x10240150:
        return describe_execution;
    case receiver_resolution_phase: return resolve_receiver;
    case receiver_outcome_phase: return complete_receiver;
    case execution_result_phase: return prepare_result;
    case result_message_phase:
    case 0x10241474: case 0x102414a0: case 0x102414cc: case 0x102414f8:
    case 0x10241524: case 0x10241550: case 0x1024157c: case 0x102415a8:
    case 0x102415d4: case 0x10241600: case 0x1024162c: case 0x10241658:
        return describe_result;
    case result_resolution_phase: return resolve_result;
    case result_outcome_phase: return complete_result;
    case unopposed_result_phase: return complete_unopposed;
    case no_receiver_phase: return prepare_final;
    case final_effects_phase: return complete_special;
    case regular_effects_phase: return complete_regular;
    case success_reaction_phase: return react_to_success;
    case unsuccessful_action_phase: return prepare_failed;
    case failed_message_phase:
    case 0x10243690: case 0x102436bc: case 0x102436e8: case 0x10243714:
    case 0x10243740: case 0x1024376c: case 0x10243798: case 0x102437c4:
    case 0x102437f0: case 0x1024381c: case 0x10243848: case 0x10243874:
        return describe_failed;
    case failed_comment_phase:
    case 0x1024391c: case 0x10243958: case 0x10243994:
        return describe_failed_comment;
    case failed_fallback_phase:
    case 0x102439f4: case 0x10243a14: case 0x10243a40: case 0x10243a6c:
    case 0x10243a98: case 0x10243ac4: case 0x10243b50: case 0x10243b7c:
        return describe_failed_fallback;
    case failed_outcome_phase: return prepare_failed_outcome;
    case receiver_sampling_phase: return sample_receiver;
    case receiver_marker_phase: return handle_marker;
    case receiver_position_phase: return position_receiver;
    case receiver_assessment_phase: return assess_receiver;
    case receiver_effects_phase: return complete_receiver_effects;
    case receiver_final_reaction_phase: return react_to_receiver;
    case missing_receiver_phase: return report_missing_receiver;
    case return_phase: return return_action;
    default: return nullptr;
    }
}

void act(Cpu& c) {
    const auto caller = c.lr();
    c.s.step(ball_action::entry);
    GuestNativeScope scope(c.s, ball_action::entry);
    prepare(c, scope);
    while (c.s.pc != caller) {
        const auto pc = c.s.pc;
        if (const auto phase = ball_action_phase(pc)) phase(c);
        else if (pc == ball_action::entry) act(c);
        else if (pc > ball_action::entry && pc < ball_action::end)
            throw std::runtime_error("Unsupported ball-action continuation " + hex_address(pc));
        else {
            // A live table can transfer outside the routine. The branch
            // leaves LR intact, including a return address inside this frame.
            run_until(c.s, c.lr());
        }
    }
}
}

namespace cm::aot::match_pl {
void execute_ball_action(Cpu& c) { ball_action_detail::act(c); }
}
