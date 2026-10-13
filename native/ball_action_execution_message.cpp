#include "ball_action.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_action {
void describe_execution(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    SemanticGuestCall a(state,execution_message_phase,dependencies,observer);
    // A table may transfer directly to another named phase or dependency.
    // Preserve that continuation for the coordinator instead of inventing a
    // default message. Direct message entrances do not replay the table read.
    auto destination=state.pc;
    if(destination==execution_message_phase) {
        a.load<1>(0,1,0x48);a.sign8(0,0);a.compare(0,46,false);destination=0x10240150;
        if(!a.positive()){a.load(3,2,-0x5204);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);destination=a.count_branch();}
    }
    unsigned message=0;
    switch(destination) {
    case 0x1023ff6c:message=0x1fea;break;case 0x1023ff98:message=0x1feb;break;
    case 0x1023ffc4:message=0x1fec;break;case 0x1023fff0:message=0x1fed;break;
    case 0x1024001c:message=0x1fee;break;case 0x10240048:message=0x1ff2;break;
    case 0x10240074:message=0x1fef;break;case 0x102400a0:message=0x1ff1;break;
    case 0x102400cc:message=0x1ff0;break;case 0x102400f8:message=0x1ff3;break;
    case 0x10240124:message=0x1ff4;break;case 0x10240150:message=0x1fe9;break;
    default:return;
    }
    a.at(destination);a.load(3,15,0x1a4);a.value(4,message);a.load(5,15,0x1a4);a.value(10,0);a.load(3,3,0x79f4);
    a.load<1>(6,14,0x2f);a.load<1>(7,14,0x27);a.load<1>(8,15,0x2f);a.load<1>(9,15,0x27);a.call(0x10201180);
    if(destination!=0x10240150)a.jump(receiver_resolution_phase);
    state.pc=receiver_resolution_phase;
}
}
