#include "contract_terms.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::contract_terms {
namespace {
using Call=SemanticGuestCall;
void emit_label(Call& a,std::uint32_t pc,unsigned table,int offset,std::uint32_t join){a.at(pc);a.move(3,4);a.offset(4,table,offset);a.call(0x10727890);a.jump(join);}
bool emit_month(Call& a,std::uint32_t target,std::uint32_t caller) {
    constexpr auto first=0x10374df8u,last=0x10374eb8u;
    if(target>=first && target<last && !(target&3)) {
        const auto which=(target-first)/16,part=(target-first)%16;a.at(target);
        if(part==0)a.move(3,30);
        if(part<=4)a.offset(4,31,0x11e + int(which)*17);
        if(part<=8)a.call(0x10727890);
        a.jump(0x10374f28);return true;
    }
    if(target==0x10374f28)return true;
    a.resume(target,caller);return false;
}
}
void bonus_name(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    Call a(s,bonus_name_entry,d,o);a.link();a.load(5,2,-0x4da4);a.save_link();a.sign8(0,3);a.compare(0,1);a.frame(64);
    if(a.equal())emit_label(a,0x103802b8,5,0x294,0x103802e4);
    else if(a.at_least()){a.at(0x1038029c);a.compare(0,3);if(a.at_least()){a.at(0x103802d8);a.move(3,4);a.offset(4,5,0x2b3);a.call(0x10727890);}else{a.jump(0x103802c8);emit_label(a,0x103802c8,5,0x2a1,0x103802e4);}}
    else {a.at(0x10380290);a.compare(0,0);if(a.at_least())emit_label(a,0x103802a8,5,0x289,0x103802e4);else{a.jump(0x103802d8);a.move(3,4);a.offset(4,5,0x2b3);a.call(0x10727890);}}
    a.at(0x103802e4);a.pop(64);a.finish();
}
void option_name(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    Call a(s,option_name_entry,d,o);a.link();a.load(5,2,-0x4da4);a.save_link();a.sign8(0,3);a.compare(0,2);a.frame(64);
    if(a.equal())emit_label(a,0x10380360,5,0x2fe,0x1038039c);
    else if(a.at_least()){a.at(0x10380330);a.compare(0,4);if(a.equal())emit_label(a,0x10380380,5,0x333,0x1038039c);else if(a.at_least()){a.at(0x10380390);a.move(3,4);a.offset(4,5,0x34e);a.call(0x10727890);}else{a.jump(0x10380370);emit_label(a,0x10380370,5,0x319,0x1038039c);}}
    else {a.at(0x10380320);a.compare(0,0);if(a.equal())emit_label(a,0x10380340,5,0x2c6,0x1038039c);else if(a.at_least())emit_label(a,0x10380350,5,0x2e3,0x1038039c);else{a.jump(0x10380390);a.move(3,4);a.offset(4,5,0x34e);a.call(0x10727890);}}
    a.at(0x1038039c);a.pop(64);a.finish();
}
void describe_bonus(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    Call a(s,describe_bonus_entry,d,o);a.link();a.save(31);a.move(31,7);a.save(30);a.move(30,5);a.save(29);a.move(29,4);a.save(28);a.move(28,3);a.move(3,29);a.save_link();a.frame(480);a.offset(4,1,0x3c);a.call(bonus_name_entry);a.sign8(0,31);a.compare(0,1);
    if(a.equal()){a.move(3,30);a.offset(4,1,0x3c);a.call(0x1077dbf0);a.jump(0x10374c84);}
    else {a.at(0x10374c34);a.sign8(0,29);a.offset(3,1,0x38);a.mask(0,0,0xfffffffc,false,2);a.value(4,10);a.add(5,28,0);a.load(0,5,0x10);a.store(1,0x38,0);a.call(0x10009f20);a.move(5,3);a.move(6,4);a.offset(3,1,0x104);a.value(4,200);a.value(7,10);a.call(0x1000b410);a.load(4,2,-0x4da4);a.move(3,30);a.offset(5,1,0x104);a.offset(6,1,0x3c);a.offset(4,4,0xfb);a.call(0x10727890);}
    a.at(0x10374c84);a.pop(480);a.restore(31);a.restore(30);a.restore(29);a.restore(28);a.finish();
}
void describe_option(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,describe_option_entry,d,o);a.link();a.move(3,4);a.save_link();a.move(4,5);a.frame(64);a.call(option_name_entry);a.pop(64);a.finish();}
void describe_end(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    Call a(s,describe_end_entry,d,o);a.link();const auto caller=a.r(0);a.save(31);a.load(31,2,-0x4da4);a.save(30);a.move(30,4);a.save(29);a.move(29,3);a.save_link();a.sign8(0,5);a.compare(0,1);a.frame(80);
    bool month=!a.equal();if(!month){a.load<1>(0,29,0x38);a.mask(0,0,0xfe000000,false,24);a.shift_right(0,0,26);a.sign8(0,0);a.compare(0,2);month=!a.equal();}
    if(!month){a.offset(3,29,0x30);a.call(0x10718360);a.sign8(4,3);a.load<2>(0,29,0x32);a.compare(4,11,false);a.sign16(5,0);if(a.positive()){a.at(0x10374f28);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.finish();return;}a.load(3,2,-0x4db0);a.mask(0,4,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);const auto target=a.count_branch();if(!emit_month(a,target,caller))return;}
    else {
        a.at(0x10374eb8);a.load<1>(0,29,0x38);a.mask(0,0,0xfe000000,false,24);a.shift_right(0,0,26);a.sign8(0,0);a.compare(0,3);
        if(a.equal()){a.move(3,30);a.offset(4,31,0xb9);a.call(0x10727890);a.jump(0x10374f28);}
        else {a.at(0x10374ee0);a.compare(0,4);if(a.equal()){a.move(3,30);a.offset(4,31,0xca);a.offset(5,31,0xf2);a.call(0x10727890);a.jump(0x10374f28);}else {a.at(0x10374efc);a.compare(0,1);if(a.equal()){a.move(3,30);a.offset(4,31,0xf3);a.call(0x10727890);a.jump(0x10374f28);}else {a.at(0x10374f14);a.move(4,30);a.offset(3,29,0x30);a.value(5,200);a.value(6,255);a.call(0x10715e00);}}}
    }
    a.at(0x10374f28);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
}
