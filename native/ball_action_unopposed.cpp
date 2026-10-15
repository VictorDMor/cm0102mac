#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
void complete_unopposed_execution(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    SemanticGuestCall a(state,unopposed_result_phase,dependencies,observer);a.load<1>(0,1,0x42);a.sign8(0,0,true);
    if(!a.equal()) {
        a.load<1>(0,15,0x27);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0);a.compare(0,1);
        const bool right=a.equal();detail::area_coordinates(a,right?0x102424d4:0x10242524,0x1024256c,right,0,3,0);
        a.at(0x1024256c);a.sign8(0,0,true);bool mark=!a.equal();
        if(!mark){a.load<1>(0,1,0x45);a.sign8(0,0,true);mark=!a.equal();}
        if(mark){a.at(0x10242580);a.value(0,1);a.store<1>(1,0xd0,0);}
    }
    a.at(0x10242588);a.load<1>(3,1,0x48);a.sign8(0,22);a.compare(0,4);a.sign8(0,3);a.store<2>(14,0x19c,0);
    if(a.equal()){state.pc=no_receiver_phase;return;}
    a.load<1>(3,14,4);a.move(4,14);a.offset(0,3,1);a.store<1>(14,4,0);a.load(3,15,0x1a4);a.call(0x101e3d50);a.mask(0,3,0x30,true);
    bool smaller_loss=false;
    if(!a.equal()) {
        a.load(3,15,0x1a4);a.move(4,14);a.call(0x101e3d50);a.sign16(0,3);a.mask(0,0,0x880,true);smaller_loss=!a.equal();
    }
    if(smaller_loss){a.load<2>(3,14,0x36,true);a.offset(0,3,-59);a.store<2>(14,0x36,0);a.jump(no_receiver_phase);}
    else {a.at(0x102425e8);a.load<2>(3,14,0x36,true);a.offset(0,3,-70);a.store<2>(14,0x36,0);}
    state.pc=no_receiver_phase;
}
}
