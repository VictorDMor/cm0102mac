#include "ball_action.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_action {
void describe_execution_result(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    SemanticGuestCall a(state,result_message_phase,dependencies,observer);auto destination=state.pc;
    if(destination==result_message_phase) {
        a.sign8(0,13);a.compare(0,56,false);destination=0x10241658;
        if(!a.positive()){a.load(3,2,-0x5208);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);destination=a.count_branch();}
    }
    unsigned message=0;
    switch(destination) {
    case 0x10241474:message=0x1fea;break;case 0x102414a0:message=0x1feb;break;
    case 0x102414cc:message=0x1fec;break;case 0x102414f8:message=0x1fed;break;
    case 0x10241524:message=0x1fee;break;case 0x10241550:message=0x1ff2;break;
    case 0x1024157c:message=0x1fef;break;case 0x102415a8:message=0x1ff1;break;
    case 0x102415d4:message=0x1ff0;break;case 0x10241600:message=0x1ff3;break;
    case 0x1024162c:message=0x1ff4;break;case 0x10241658:message=0x1fe9;break;
    default:return;
    }
    a.at(destination);a.load(3,15,0x1a4);a.value(4,message);a.load(5,15,0x1a4);a.value(10,0);a.load(3,3,0x79f4);
    a.load<1>(6,15,0x2f);a.load<1>(7,15,0x27);a.load<1>(8,14,0x2f);a.load<1>(9,14,0x27);a.call(0x10201180);
    if(destination!=0x10241658)a.jump(result_resolution_phase);
    state.pc=result_resolution_phase;
}
}
