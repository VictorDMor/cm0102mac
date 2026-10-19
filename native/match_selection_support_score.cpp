#include "match_selection_support.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_selection_support {
void actor_selection_score(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    SemanticGuestCall a(state,entries[2],dependencies,observer);
    a.compare(4,0,false);a.load(5,2,-0x51e8);
    bool ordinary=a.equal();
    if(!ordinary){a.at(0x102875fc);a.load(0,3,0x1ac);a.compare_registers(4,0,false);ordinary=!a.equal();}
    if(ordinary){
        a.at(0x10287660); a.floating_load(1,3,424,true); a.value(0,0x43300000); a.floating_load(0,5,936);
        a.load<1>(3,3,0x102); a.floating_difference(0,1,0,true); a.floating_load(1,5,48); a.sign8(3,3);
        a.store(1,-0x10,0); a.exclusive_flags(0,3,0x80000000); a.floating_load(4,5,128); a.store(1,-0xc,0);
        a.floating_sum(2,1,0); a.floating_load(1,5,24); a.floating_load(3,1,-16); a.floating_load(0,5,40);
        a.floating_subtract(3,3,4); a.floating_product_sum(1,3,2,1); a.floating_product(0,1,0);
        a.floating_integer(0,0); a.floating_store(0,1,-8); a.load(3,1,-0x4); a.finish();
    }else{
        a.at(0x10287608); a.floating_load(1,3,424,true); a.value(0,0x43300000); a.floating_load(0,5,936);
        a.load<1>(3,3,0x102); a.floating_difference(0,1,0,true); a.floating_load(1,5,48); a.sign8(3,3);
        a.store(1,-0x8,0); a.offset(0,3,0x5); a.floating_load(2,5,128); a.exclusive_flags(0,0,0x80000000);
        a.floating_load(3,5,192); a.store(1,-0x4,0); a.floating_sum(0,1,0); a.floating_load(1,1,-8);
        a.floating_subtract(1,1,2); a.floating_product(1,3,1); a.floating_product(0,1,0);
        a.floating_integer(0,0); a.floating_store(0,1,-16); a.load(3,1,-0xc); a.finish();
    }
}
}
