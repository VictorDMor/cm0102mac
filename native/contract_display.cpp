#include "contract_display.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::contract_display {
namespace {
using Call=SemanticGuestCall;
void select_period_value(Call& a) {
    a.load(0,3,12);a.load(4,30,0);a.store(1,0x38,0);a.load(4,4,4);
    a.mask(0,4,0x200000,true);
    if(!a.equal()) {
        a.load(0,3,12);a.value(3,8);a.store(1,0x38,0);a.jump(0x10372604);
    } else {
        a.at(0x103725d8);a.mask(0,4,0x400000,true);
        if(!a.equal()) {
            a.load(0,3,12);a.value(3,8);a.mask(0,0,0xfffffffc,false,2);
            a.store(1,0x38,0);a.jump(0x10372604);
        } else {
            a.at(0x103725f4);a.load(0,3,12);a.value(3,4);
            a.multiply(0,0,52);a.store(1,0x38,0);
        }
    }
}
void append_period_label(Call& a,bool abbreviated) {
    const auto begin=abbreviated?0x1037265cu:0x103726b8u;
    a.at(begin);a.load(3,30,0);a.load(3,3,4);a.mask(0,3,0x200000,true);
    if(!a.equal()) {
        a.move(3,28);a.offset(4,31,abbreviated?0x2a:0x69);
        a.offset(5,1,0x3c);a.call(0x10727890);a.jump(0x10372710);
    } else {
        a.at(begin+0x24);a.mask(0,3,0x400000,true);
        if(!a.equal()) {
            a.move(3,28);a.offset(4,31,abbreviated?0x3f:0x83);
            a.offset(5,1,0x3c);a.call(0x10727890);a.jump(0x10372710);
        } else {
            a.at(begin+0x40);a.mask(0,3,0x800000,true);
            if(!a.equal()) {
                a.move(3,28);a.offset(4,31,abbreviated?0x54:0x9e);
                a.offset(5,1,0x3c);a.call(0x10727890);
                if(abbreviated)a.jump(0x10372710);
            }
        }
    }
}
}
void format_wage_text(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,format_wage_text_entry,d,o);
    a.link();a.multiple(27);a.load(30,2,-0x7ac8);a.move(28,4);
    a.load(31,2,-0x4da4);a.move(29,5);a.save_link();a.frame(0x120);
    select_period_value(a);
    a.at(0x10372604);a.sign8(3,3);a.sign8(0,6);a.combine(27,3,0);
    a.offset(3,1,0x38);a.sign8(4,27);a.call(0x10009f20);
    a.move(5,3);a.move(6,4);a.offset(3,1,0x3c);a.sign8(7,27);
    a.value(4,200);a.call(0x1000b410);a.sign8(0,29,true);
    if(a.equal()) {
        a.move(3,28);a.offset(4,31,0x27);a.offset(5,1,0x3c);
        a.call(0x1077b720);a.jump(0x10372710);
    } else {
        a.at(0x10372650);a.sign8(0,29);a.compare(0,2);
        append_period_label(a,a.equal());
    }
    a.at(0x10372710);a.pop(0x120);a.multiple(27,true);a.finish();
}
void format_contract_end(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,format_contract_end_entry,d,o);
    a.link();a.load(5,2,-0x4da4);a.save_link();a.frame(0x40);
    a.load<1>(0,3,0x38);a.mask(0,0,0xfe000000,false,24);
    a.shift_right(0,0,26);a.sign8(0,0);a.compare(0,3);
    if(a.equal()) {
        a.move(3,4);a.offset(4,5,0xb9);a.call(0x10727890);a.jump(0x103727ac);
    } else {
        a.at(0x10372768);a.compare(0,4);
        if(a.equal()) {
            a.move(3,4);a.offset(4,5,0xca);a.offset(5,5,0xf2);
            a.call(0x10727890);a.jump(0x103727ac);
        } else {
            a.at(0x10372784);a.compare(0,1);
            if(a.equal()) {
                a.move(3,4);a.offset(4,5,0xf3);a.call(0x10727890);a.jump(0x103727ac);
            } else {
                a.at(0x1037279c);a.offset(3,3,0x30);a.value(5,200);a.value(6,255);a.call(0x10715e00);
            }
        }
    }
    a.at(0x103727ac);a.pop(0x40);a.finish();
}
}
