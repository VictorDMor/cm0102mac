#include "match_target.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_target {
namespace {
using Call=SemanticGuestCall;
bool closer_ready_actor(Call& a){
    a.at(0x101e3230);a.move(3,31);a.call(0x101d8110);a.sign8(0,3);a.compare(0,0);
    if(a.less())return false;
    a.compare_registers(30,31,false);if(a.equal())return false;
    a.load<1>(0,31,0x2c);a.sign8(0,0);a.compare(0,0);if(!a.equal())return false;
    a.move(3,31);a.move(4,25);a.move(5,26);a.call(0x1022e760);a.floating_move(30,1);
    a.floating_compare(30,31);if(a.less())return true;
    a.move(3,30);a.call(0x10289560);a.sign16(0,3);a.compare(0,1);if(!a.equal())return false;
    a.floating_compare(30,31);return a.equal();
}
void restore_nearest_frame(Call& a){
    a.at(0x101e32b8);a.load(0,1,0x78);a.offset(1,1,0x70);
    a.floating_load(31,1,-8);a.floating_load(30,1,-16);a.restore_link();a.multiple(25,true,-16);a.finish();
}
}
void is_nearest_candidate(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer){
    Call a(state,entries[4],dependencies,observer);
    a.link();a.floating_store(31,1,-8);a.floating_store(30,1,-16);a.multiple(25,false,-16);a.save_link();a.frame(0x70);
    a.store(1,0x88,3);a.move(30,4);a.move(25,5);a.move(26,6);a.move(3,30);a.call(0x10283690);a.move(28,3);
    a.move(3,30);a.move(4,25);a.move(5,26);a.call(0x1022e760);a.floating_move(31,1);
    a.load(3,2,-0x52c4);a.floating_load(0,3,0x50);a.floating_compare(0,31);
    bool scan=!a.equal();
    if(!scan){
        a.move(3,30);a.call(0x10289560);a.sign16(0,3);a.compare(0,1);scan=a.equal();
        if(!scan){a.value(3,1);a.jump(0x101e32b8);}
    }
    if(scan){
        a.at(0x101e3214);a.load(4,1,0x88);a.sign8(0,28);a.multiply(3,0,0x2350);a.offset(31,3,0x311c);a.add(31,4,31);
        a.value(29,0);a.jump(0x101e32a8);bool rejected=false;
        for(;;){
            a.at(0x101e32a8);a.sign8(0,29);a.compare(0,20);if(!a.less())break;
            if(closer_ready_actor(a)){a.at(0x101e3294);a.value(3,0);a.jump(0x101e32b8);rejected=true;break;}
            a.at(0x101e329c);a.offset(29,29,1);a.move(27,31);a.offset(31,31,0x1c4);
        }
        if(!rejected){a.at(0x101e32b4);a.value(3,1);}
    }
    restore_nearest_frame(a);
}
}
