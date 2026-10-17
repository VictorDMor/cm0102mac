#include "actor_attributes.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_attributes {
namespace {
using Call=SemanticGuestCall;

void merge_first_update(Call& a) {
    a.load<1>(4,3,4);a.load<1>(0,30,0);a.sign8(4,4);a.sign8(0,0);a.compare_registers(4,0);
    if(a.equal()) {
        a.load<1>(5,3,5);a.load<1>(4,31,0);a.sign8(0,5);a.sign8(4,4);a.add(0,0,4);a.compare(0,100);
        if(!a.nonpositive()) {a.value(0,100);a.store<1>(3,5,0);a.jump(0x10186da8);}
        else {a.at(0x10186d78);a.add(0,5,4);a.store<1>(3,5,0);a.jump(0x10186da8);}
    } else {
        a.at(0x10186d84);a.load<1>(5,31,0);a.load<1>(0,3,5);a.sign8(4,5);a.sign8(0,0);a.compare_registers(4,0);
        if(!a.nonpositive()) {a.store<1>(3,5,5);a.load<1>(0,30,0);a.store<1>(3,4,0);}
    }
    // Read back each retained byte after the preceding store. The caller may
    // pass pointers into this same item, including its identifier and padding.
    a.at(0x10186da8);a.load<1>(0,3,4);a.store<1>(30,0,0);a.load<1>(0,3,5);a.store<1>(31,0,0);a.jump(0x10186df8);
}
}

void merge_condition_update(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,queue_merge_entry,dependencies,observer);
    a.link();a.multiple(27);a.move(28,3);a.move(29,4);a.move(30,5);a.move(31,6);a.save_link();a.frame(96);
    a.load(0,3,0x28);a.compare(0,0,false);
    if(a.equal()) {
        a.value(3,16);a.call(0x10773520);a.move(27,3,true);if(!a.equal())a.call(0x10738c00);
        a.at(0x10186d10);a.store(28,0x28,27);
    }
    // A failed allocation still reaches the original null queue dereference.
    a.at(0x10186d14);a.load(3,28,0x28);a.load(0,3,4);a.store(3,12,0);a.jump(0x10186dbc);
    a.flags(0,0,0);
    bool matched=false;
    for(;;) {
        a.at(0x10186dc0);a.load(3,28,0x28);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
        a.at(0x10186d28);a.load(4,3,0);a.load(0,29,0);a.compare_registers(4,0);
        if(a.equal()) {merge_first_update(a);matched=true;break;}
    }
    if(!matched) {
        a.at(0x10186dd0);a.load(0,29,0);a.offset(4,1,0x38);a.value(5,8);a.store(1,0x38,0);
        a.load<1>(0,31,0);a.store<1>(1,0x3d,0);a.load<1>(0,30,0);a.store<1>(1,0x3c,0);
        // The final two stack bytes are deliberately not initialized.
        a.load(3,28,0x28);a.call(0x10738d10);
    }
    a.at(0x10186df8);a.pop(96);a.multiple(27,true);a.finish();
}

void find_condition_update(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,queue_lookup_entry,dependencies,observer);
    a.link();a.save(31);a.move(31,4);a.save(30);a.move(30,3);a.save_link();a.frame(64);
    a.load(3,3,0x28);a.compare(3,0,false);
    if(a.equal()) {a.value(3,0);a.jump(0x10186e7c);}
    else {
        a.at(0x10186e40);a.load(0,3,4);a.store(3,12,0);a.jump(0x10186e64);a.flags(0,0,0);
        for(;;) {
            a.at(0x10186e68);a.load(3,30,0x28);a.call(0x10738e00);a.compare(3,0,false);
            if(a.equal()) {a.value(3,0);break;}
            a.at(0x10186e50);a.load(4,3,0);a.load(0,31,0);a.compare_registers(4,0);
            if(a.equal()) {a.jump(0x10186e7c);break;}
        }
    }
    a.at(0x10186e7c);a.pop(64);a.restore(31);a.restore(30);a.finish();
}
}
