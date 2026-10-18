#include "preferences_actions.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::preferences_actions {
namespace {
using Call = SemanticGuestCall;

// Each stage reads the current event again. A zero low halfword selects the
// controller's fallback action; -1 is the special event index without a row.
bool action_is(Call& a, std::uint32_t stage, unsigned scratch, unsigned action) {
    a.at(stage);
    a.sign16(0,26);a.compare(0,-1);
    if (a.equal()) {
        a.at(stage+32);a.value(0,0);
    } else {
        a.at(stage+12);
        a.multiply(0,0,0x17c);a.offset(scratch,3,0xc0000);a.add(scratch,scratch,0);
        a.load(0,scratch,-0x5794);a.jump(stage+36);
    }
    a.at(stage+36);a.sign16(0,0,true);
    if (a.equal()) {
        a.at(stage+48);a.offset(scratch,30,0x260000);a.load<2>(0,scratch,-0x7e62,true);
    } else {
        a.at(stage+44);a.jump(stage+56);
    }
    a.at(stage+56);a.sign16(0,0);a.compare(0,std::int32_t(action));
    return a.equal();
}

void event_value(Call& a, std::uint32_t stage, unsigned destination) {
    a.at(stage);a.sign16(0,26);a.compare(0,-1);
    if (a.equal()) {
        a.at(stage+32);a.value(destination,0);
    } else {
        a.at(stage+12);a.multiply(0,0,0x17c);a.offset(3,3,0xc0000);a.add(3,3,0);
        a.load(destination,3,-0x5754);a.jump(stage+36);
    }
    a.at(stage+36);
}

void select_action(Call& a) {
    if (a.equal()) {
        a.at(0x1033f8d4);a.move(3,27);a.call(0x1013f570);a.value(3,0xfffffff5);a.jump(0x1033fc4c);return;
    }
    if (action_is(a,0x1033f8e4,4,0x28)) {
        a.at(0x1033f928);a.load(3,2,-0x7ac8);a.load(3,3,0);a.call(0x1013feb0);
        a.move(3,27);a.value(4,1);a.call(0x1013f4d0);a.value(3,0xfffffff7);a.jump(0x1033fc4c);return;
    }
    if (action_is(a,0x1033f948,4,0x29)) {
        a.at(0x1033f98c);a.move(3,27);a.call(0x1013feb0);a.move(3,27);a.value(4,1);
        a.call(0x1013f4d0);a.value(3,0xfffffff7);a.jump(0x1033fc4c);return;
    }
    if (action_is(a,0x1033f9a8,4,0x2a)) {
        event_value(a,0x1033f9ec,0);a.sign8(4,0);a.move(3,27);a.call(0x1013fb60);a.compare(31,0,false);
        if (a.equal()) {
            a.at(0x1033fa2c);a.load(3,2,-0x7b98);a.load(0,3,0);a.compare(0,0,false);
            if (!a.equal()) {a.at(0x1033fa3c);a.call(0x10343200);}
        } else {
            a.at(0x1033fa24);a.compare(31,1,false);
            if (a.equal()) {
                a.at(0x1033fa2c);a.load(3,2,-0x7b98);a.load(0,3,0);a.compare(0,0,false);
                if (!a.equal()) {a.at(0x1033fa3c);a.call(0x10343200);}
            }
        }
        a.at(0x1033fa40);a.value(3,0xfffffff5);a.jump(0x1033fc4c);return;
    }
    if (action_is(a,0x1033fa48,4,0x3d)) {
        event_value(a,0x1033fa8c,4);a.move(3,27);a.call(0x101406f0);
        a.value(3,0xfffffff5);a.jump(0x1033fc4c);return;
    }
    if (action_is(a,0x1033fac0,4,0x2b)) {
        event_value(a,0x1033fb04,0);a.move(3,27);a.sign8(4,0);a.call(0x1013fc50);
        a.value(3,0xfffffff5);a.jump(0x1033fc4c);return;
    }
    if (action_is(a,0x1033fb3c,4,0x2c)) {
        event_value(a,0x1033fb80,4);a.move(3,27);a.call(0x1013f680);
        a.value(3,0xfffffff5);a.jump(0x1033fc4c);return;
    }
    if (action_is(a,0x1033fbb4,3,0x2d)) {
        a.at(0x1033fbf8);a.move(3,27);a.value(4,0x2000);a.call(0x1013f680);
        // Selecting the option can change the active view and event payload.
        // Reload both before reading the signed interval; keep the cached context.
        a.load<2>(3,29,0,true);a.sign16(0,26);a.compare(0,-1);a.multiply(3,3,0x2c0);a.indexed_load(3,28,3);
        if (a.equal()) {
            a.at(0x1033fc30);a.value(0,0);
        } else {
            a.at(0x1033fc1c);a.multiply(0,0,0x17c);a.offset(3,3,0xc0000);a.add(3,3,0);
            a.load(0,3,-0x5754);a.jump(0x1033fc34);
        }
        a.at(0x1033fc34);a.move(3,27);a.sign16(4,0);a.call(0x1013fe00);
        a.value(3,0xfffffff5);a.jump(0x1033fc4c);return;
    }
    a.at(0x1033fc48);a.value(3,0);
}
}

void dispatch_action(State& s, SemanticDependencyDispatch* d, SemanticGuestObserver* o) {
    Call a(s,dispatch_action_entry,d,o);
    a.link();a.multiple(26);a.load(30,2,-0x7bb0);a.move(26,3);a.value(4,0);a.move(3,30);
    a.store(1,8,0);a.frame(0x50);a.call(0x1073ef70);a.move(0,3);a.move(3,30);a.move(27,0);
    a.value(4,1);a.call(0x1073ef70);a.offset(29,30,0x2c58);a.sign16(0,26);a.load<2>(4,30,0x2c58,true);
    a.compare(0,-1);a.offset(28,30,0x24);a.mask(31,3,0xff);a.multiply(3,4,0x2c0);a.indexed_load(3,28,3);
    if (a.equal()) {
        a.at(0x1033f8b0);a.value(0,0);
    } else {
        a.at(0x1033f89c);a.multiply(0,0,0x17c);a.offset(4,3,0xc0000);a.add(4,4,0);
        a.load(0,4,-0x5794);a.jump(0x1033f8b4);
    }
    a.at(0x1033f8b4);a.sign16(0,0,true);
    if (a.equal()) {
        a.at(0x1033f8c0);a.offset(4,30,0x260000);a.load<2>(0,4,-0x7e62,true);
    } else {
        a.at(0x1033f8bc);a.jump(0x1033f8c8);
    }
    a.at(0x1033f8c8);a.sign16(0,0);a.compare(0,0x27);select_action(a);
    a.at(0x1033fc4c);a.load(0,1,0x58);a.offset(1,1,0x50);a.restore_link();a.multiple(26,true);a.finish();
}
}
