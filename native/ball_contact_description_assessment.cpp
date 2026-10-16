#include "ball_contact.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_contact {
void assess_description_weight(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    SemanticGuestCall a(state,description_assessment_phase,dependencies,observer);
    a.sign8(0,31);a.mask(7,0,0xfffffffe,false,1);a.offset(6,1,0x3c);a.indexed_load<2>(5,6,7,true);a.compare(5,5);
    if(a.equal()) {
        a.sign8(24,31);a.mask(0,24,0xfffffffc,false,2);a.offset(6,1,0x48);a.indexed_load(7,6,0);
        a.floating_load(1,7,0x7c,true);a.floating_load(0,30,0x98);a.floating_compare(1,0);
        if(!a.nonpositive()) {
            a.sign8(0,31);a.mask(0,0,0xfffffffc,false,2);a.offset(8,1,0x48);a.indexed_load(7,8,0);
            a.floating_load(27,7,0x7c,true);a.jump(0x101f5ef8);
        } else {a.at(0x101f5ef4);a.floating_load(27,30,0x98);}
        a.at(0x101f5ef8);a.sign8(3,31);a.mask(4,3,0xfffffffc,false,2);a.offset(3,1,0x40);a.floating_indexed_single(2,3,4);
        a.floating_load(1,28,0x28,true);a.floating_round_single(0,27);a.floating_difference(0,1,0,true,true);
        a.floating_product_single(0,2,0);a.floating_indexed_store_single(0,3,4);
    }
    a.at(0x101f5f1c);a.value(3,0x10000);a.offset(3,3,-0x3cb0);a.call(0x101d3c90);a.move(23,3);
    a.floating_load(1,28,0x28,true);a.sign8(5,31);a.mask(0,5,0xfffffffc,false,2);a.offset(7,1,0x40);a.floating_indexed_single(0,7,0);
    a.floating_product_single(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x50);a.load(6,1,0x54);a.compare_registers(23,6);
    state.pc=a.at_least()?description_next_phase:description_reaction_phase;
}
}
