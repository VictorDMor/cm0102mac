#include "injury_maintenance.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::injury_maintenance {
using Call=SemanticGuestCall;
void merge_pending_injury(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,merge_pending_entry,d,o);a.link();a.multiple(27);a.move(28,3);a.move(29,4);a.move(30,5);a.move(31,6);a.save_link();a.frame(96);
    a.load(0,3,0x28);a.compare(0,0,false);
    if(a.equal()){a.value(3,16);a.call(0x10773520);a.move(27,3,true);if(!a.equal())a.call(0x10738c00);a.at(0x10186d10);a.store(28,0x28,27);}
    a.at(0x10186d14);a.load(3,28,0x28);a.load(0,3,4);a.store(3,12,0);a.jump(0x10186dbc);
    a.flags(0,0,0);bool found=false;
    while(true) {
        a.at(0x10186dc0);a.load(3,28,0x28);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
        a.at(0x10186d28);a.load(4,3,0);a.load(0,29,0);a.compare_registers(4,0);if(!a.equal())continue;
        a.load<1>(4,3,4);a.load<1>(0,30,0);a.sign8(4,4);a.sign8(0,0);a.compare_registers(4,0);
        if(a.equal()) {
            a.load<1>(5,3,5);a.load<1>(4,31,0);a.sign8(0,5);a.sign8(4,4);a.add(0,0,4);a.compare(0,100);
            if(!a.nonpositive()){a.value(0,100);a.store<1>(3,5,0);a.jump(0x10186da8);}
            else {a.at(0x10186d78);a.add(0,5,4);a.store<1>(3,5,0);a.jump(0x10186da8);}
        }else {
            a.at(0x10186d84);a.load<1>(5,31,0);a.load<1>(0,3,5);a.sign8(4,5);a.sign8(0,0);a.compare_registers(4,0);
            if(!a.nonpositive()){a.store<1>(3,5,5);a.load<1>(0,30,0);a.store<1>(3,4,0);}
        }
        a.at(0x10186da8);a.load<1>(0,3,4);a.store<1>(30,0,0);a.load<1>(0,3,5);a.store<1>(31,0,0);a.jump(0x10186df8);found=true;break;
    }
    if(!found){a.at(0x10186dd0);a.load(0,29,0);a.offset(4,1,0x38);a.value(5,8);a.store(1,0x38,0);a.load<1>(0,31,0);a.store<1>(1,0x3d,0);a.load<1>(0,30,0);a.store<1>(1,0x3c,0);a.load(3,28,0x28);a.call(0x10738d10);}
    a.at(0x10186df8);a.pop(96);a.multiple(27,true);a.finish();
}
void find_pending_injury(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,find_pending_entry,d,o);a.link();a.save(31);a.move(31,4);a.save(30);a.move(30,3);a.save_link();a.frame(64);a.load(3,3,0x28);a.compare(3,0,false);
    if(a.equal()){a.value(3,0);a.jump(0x10186e7c);}
    else {
        a.at(0x10186e40);a.load(0,3,4);a.store(3,12,0);a.jump(0x10186e64);a.flags(0,0,0);
        while(true) {
            a.at(0x10186e68);a.load(3,30,0x28);a.call(0x10738e00);a.compare(3,0,false);
            if(a.equal()){a.value(3,0);break;}
            a.at(0x10186e50);a.load(4,3,0);a.load(0,31,0);a.compare_registers(4,0);if(!a.equal())continue;a.jump(0x10186e7c);break;
        }
    }
    a.at(0x10186e7c);a.pop(64);a.restore(31);a.restore(30);a.finish();
}
void reset_staff_physical_state(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,reset_staff_entry,d,o);a.link();a.multiple(27);a.move(27,3);a.load(30,2,-0x7be0);a.load(31,2,-0x7b80);a.move(28,4);a.save_link();a.frame(80);a.load(3,3,0x28);a.compare(3,0,false);
    if(!a.equal()) {
        a.load(0,3,4);a.store(3,12,0);a.jump(0x10186eec);
        while(true){a.at(0x10186eec);a.load(3,27,0x28);a.call(0x10738e00);a.move(4,3,true);if(a.equal())break;a.at(0x10186ed8);a.load(0,4,0);a.compare_registers(0,28);if(!a.equal())continue;a.load(3,27,0x28);a.call(0x10738e60);}
    }
    a.at(0x10186efc);a.multiply(0,28,0x24);a.load(3,27,0);a.add(29,3,0);a.load(0,29,0x10);a.compare(0,-1);
    if(!a.equal()){a.multiply(0,28,0x70);a.load(4,30,0);a.move(3,27);a.value(5,0);a.add(4,4,0);a.call(0x1018b9f0);}
    a.at(0x10186f2c);a.multiply(0,28,0x70);a.load(4,30,0);a.move(3,27);a.add(4,4,0);a.call(0x1018fdf0);a.load<2>(0,31,2);a.multiply(3,28,100);a.value(6,10000);a.load(4,2,-0x7b9c);a.store<2>(29,2,0);a.value(5,0);a.value(0,0xffffffff);a.load<2>(7,31,0,true);a.offset(3,3,13);a.store<2>(29,0,7);a.load<1>(7,31,4);a.store<1>(29,4,7);a.store<2>(29,8,6);a.store<2>(29,10,6);a.store<2>(29,12,5);a.store(29,0x10,0);a.store<1>(29,0x14,0);a.store(29,0x18,0);a.store(29,0x1c,0);a.store<1>(29,0x21,0);a.load(4,4,0);a.indexed_load<1>(0,4,3);a.insert_mask(0,5,0x80,7);a.indexed_store<1>(0,4,3);a.pop(80);a.multiple(27,true);a.finish();
}
}
