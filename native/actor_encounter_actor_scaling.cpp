#include "actor_encounter.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_encounter {
using Call = SemanticGuestCall;
namespace {
}
void scale_actor_score(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x102865a0,dependencies,observer);
    a.at(0x102865a0); a.floating_load(0,3,424,true); a.value(0,0x43300000); a.load(4,2,-0x51e8);
    a.floating_product_single(0,0,1); a.store(1,-0x8,0); a.floating_load(1,4,128);
    a.floating_store_single(0,3,424); a.load(4,3,0x6c); a.floating_load(2,3,424,true); a.load<1>(0,4,0x69);
    a.sign8(0,0); a.mask(0,0,0xfffffffe,false,1); a.subtract_from(0,0,0x32); a.exclusive_flags(0,0,0x80000000);
    a.store(1,-0x4,0); a.floating_load(0,1,-8); a.floating_subtract(0,0,1,true); a.floating_compare(2,0);
    if (a.nonpositive()) {
        a.at(0x102865f4); a.load(5,3,0x6c); a.value(0,0x43300000); a.load(4,2,-0x51e8); a.load<1>(5,5,0x69);
        a.store(1,-0x10,0); a.sign8(0,5); a.floating_load(1,4,128); a.mask(0,0,0xfffffffe,false,1);
        a.subtract_from(0,0,0x32); a.exclusive_flags(0,0,0x80000000); a.store(1,-0xc,0); a.floating_load(0,1,-16);
        a.floating_subtract(0,0,1,true);
    } else {
        a.at(0x102865ec); a.floating_load(0,3,424,true); a.jump(0x10286628);
    }
    a.at(0x10286628); a.floating_store_single(0,3,424); a.load(4,2,-0x51e8); a.floating_load(1,3,424,true);
    a.floating_load(0,4,936); a.floating_compare(1,0);
    // Conditional ABI return consumes only its original branch step.
    if (a.nonpositive()) {state.pc=std::uint32_t(state.r<4>(4128));return;}
    a.load(4,2,-0x51e0); a.floating_load(0,4,28,true); a.floating_store_single(0,3,424); a.finish();
}
}
