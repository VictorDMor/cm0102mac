#include "transfer_offer_detail.hpp"

namespace cm::aot::transfer_offer_queries {
namespace {
using detail::Call;
void squad_reason(Call& a) {
    a.at(0x103ea3d4);a.load<1>(0,3,0x60);a.mask(0,0,15);a.compare(0,4);
    if(a.equal()){a.at(0x103ea41c);a.value(3,0);return;}
    if(!a.at_least()){
        a.compare(0,1);if(a.equal()){a.at(0x103ea404);a.value(3,0x200);a.jump(0x103ea420);return;}
        if(a.at_least()){a.at(0x103ea40c);a.value(3,0x400);a.jump(0x103ea420);return;}
        a.jump(0x103ea41c);
    }else{a.at(0x103ea3f8);a.compare(0,6);if(!a.at_least()){a.jump(0x103ea414);a.value(3,0x2000);a.jump(0x103ea420);return;}}
    a.at(0x103ea41c);a.value(3,0);
}
bool rival_reason_allowed(Call& a) {
    a.at(0x103ea508);a.sign16(0,30);a.compare(0,0x800);if(!a.equal())return true;
    a.compare(29,0,false);if(a.equal())return true;a.compare(31,0,false);if(a.equal())return true;
    a.move(3,31);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
    if(!a.equal()){a.move(3,29);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);if(!a.equal()){a.value(3,1);a.jump(0x103ea5e4);return false;}}
    a.at(0x103ea56c);a.move(3,29);a.move(4,31);a.call(0x10464160);a.sign8(0,3,true);
    if(!a.equal()){a.value(3,1);a.jump(0x103ea5e4);return false;}
    a.at(0x103ea588);a.load(0,29,0x74);a.compare_registers(0,31,false);bool rival=a.equal();
    if(!rival){a.load(0,29,0x78);a.compare_registers(0,31,false);rival=a.equal();}
    if(!rival){a.load(0,29,0x7c);a.compare_registers(0,31,false);rival=a.equal();}
    if(!rival){a.load(0,31,0x74);a.compare_registers(0,29,false);rival=a.equal();}
    if(!rival){a.load(0,31,0x78);a.compare_registers(0,29,false);rival=a.equal();}
    if(!rival){a.load(0,31,0x7c);a.compare_registers(0,29,false);rival=a.equal();}
    if(rival){a.at(0x103ea5d0);a.value(3,1);a.jump(0x103ea5e4);}else{a.at(0x103ea5d8);a.value(3,0);a.jump(0x103ea5e4);}return false;
}
void emit_reason(Call& a,std::uint32_t pc,unsigned offset,bool empty_argument=false) {
    a.at(pc);a.move(3,5);a.offset(4,31,offset);if(empty_argument)a.offset(5,31,0x18e8);a.call(0x10727890);
    if(pc!=0x103ea738)a.jump(0x103ea744);
}
void describe_reason(Call& a) {
    a.at(0x103ea67c);a.sign16(0,4);a.compare(0,0x400);if(a.equal()){emit_reason(a,0x103ea6f8,0x1912);return;}
    if(!a.at_least()){
        a.compare(0,10);if(a.equal()){emit_reason(a,0x103ea728,0x19a0);return;}
        if(!a.at_least()){a.compare(0,0);if(a.equal()){emit_reason(a,0x103ea6d4,0x18ae,true);return;}a.jump(0x103ea744);return;}
        a.at(0x103ea6a4);a.compare(0,0x200);if(a.equal()){emit_reason(a,0x103ea6e8,0x18e9);return;}a.jump(0x103ea744);return;
    }
    a.at(0x103ea6b0);a.compare(0,0x1000);if(a.equal()){emit_reason(a,0x103ea718,0x196d);return;}
    if(!a.at_least()){a.compare(0,0x800);if(a.equal()){emit_reason(a,0x103ea738,0x19c2);return;}a.jump(0x103ea744);return;}
    a.at(0x103ea6c8);a.compare(0,0x2000);if(a.equal())emit_reason(a,0x103ea708,0x1940);else a.jump(0x103ea744);
}
}
void rejection_reason_from_squad_status(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,rejection_reason_from_squad_status_entry,d,o);a.link();a.save(31);a.load(3,2,-0x7be0);a.save(30);a.load(30,2,-0x7b7c);a.save(29);a.load(31,2,-0x7b78);a.save_link();a.frame(80);
    a.load(0,4,12);a.load(3,3,0);a.multiply(0,0,0x70);a.add(29,3,0);a.load(3,30,0);a.move(4,29);a.call(0x10082f20);a.compare(3,0,false);a.store(31,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(5,3,0);a.jump(0x103ea3b8);}else{a.at(0x103ea3b4);a.value(5,0);}
    a.at(0x103ea3b8);a.load(3,30,0);a.move(4,29);a.call(0x10082f90);a.compare(3,0,false);
    if(a.equal()){a.value(3,0);a.jump(0x103ea420);}else squad_reason(a);
    a.at(0x103ea420);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
void rejection_reason_allowed(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,rejection_reason_allowed_entry,d,o);a.link();a.multiple(27);a.move(27,5);a.load(5,2,-0x7be0);a.move(30,4);a.load(3,2,-0x7b7c);a.load(31,2,-0x7b78);a.load(29,2,-0x7be8);a.save_link();a.frame(80);
    a.load(0,27,12);a.load(4,5,0);a.multiply(0,0,0x70);a.load(3,3,0);a.add(28,4,0);a.move(4,28);a.call(0x10082f20);a.compare(3,0,false);a.store(31,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,29,0);a.multiply(0,0,0x248);a.add(31,3,0);a.jump(0x103ea4a8);}else{a.at(0x103ea4a4);a.value(31,0);}
    a.at(0x103ea4a8);a.load(3,27,8);a.sign16(0,30);a.compare(0,0x2000);a.load(4,29,0);a.multiply(0,3,0x248);a.add(29,4,0);bool rejected=false;
    if(a.equal()){a.load<2>(0,28,0x5c,true);a.sign8(0,0);a.compare(0,24);if(!a.less()){a.value(3,0);a.jump(0x103ea5e4);rejected=true;}}
    if(!rejected){a.at(0x103ea4dc);a.sign16(0,30);a.compare(0,0x1000);if(a.equal()){a.compare(31,0,false);if(!a.equal()){a.load(3,31,0x30);a.load(0,29,0x30);a.compare_registers(3,0,false);if(!a.equal()){a.value(3,0);a.jump(0x103ea5e4);rejected=true;}}}}
    if(!rejected&&rival_reason_allowed(a)){a.at(0x103ea5e0);a.value(3,1);}
    a.at(0x103ea5e4);a.pop(80);a.multiple(27,true);a.finish();
}
void describe_rejection_reason(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,describe_rejection_reason_entry,d,o);a.link();a.save(31);a.compare(6,50);a.load(31,2,-0x4b9c);a.save(30);a.load(30,2,-0x7ba8);a.save_link();a.frame(112);
    if(!a.at_least()){detail::diagnostic(a,0x103ea624,31,30,0x40,0x38,0x163c);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);a.jump(0x103ea744);}else describe_reason(a);
    a.at(0x103ea744);a.pop(112);a.restore(31);a.restore(30);a.finish();
}
void squad_status_for_rejection_reason(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,squad_status_for_rejection_reason_entry,d,o);a.link();a.save(31);a.load(31,2,-0x7b78);a.save(30);a.load(30,2,-0x7b7c);a.save(29);a.move(29,5);a.save(28);a.move(28,4);a.save_link();a.frame(80);
    a.load(3,30,0);a.call(0x10082f20);a.compare(3,0,false);a.store(31,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(5,3,0);a.jump(0x103ea7bc);}else{a.at(0x103ea7b8);a.value(5,0);}
    a.at(0x103ea7bc);a.load(3,30,0);a.move(4,28);a.call(0x10082f90);a.compare(3,0,false);
    if(a.equal()){a.value(3,0);a.jump(0x103ea81c);}
    else {a.at(0x103ea7d8);a.sign16(0,29);a.load<1>(3,3,0x60);a.compare(0,0x400);a.mask(3,3,15);
        if(a.equal()){a.at(0x103ea810);a.value(3,2);a.jump(0x103ea81c);}
        else if(!a.at_least()){a.compare(0,0x200);if(a.equal()){a.at(0x103ea808);a.value(3,1);a.jump(0x103ea81c);}else a.jump(0x103ea81c);}
        else{a.at(0x103ea7fc);a.compare(0,0x2000);if(a.equal()){a.at(0x103ea818);a.value(3,5);}else a.jump(0x103ea81c);}}
    a.at(0x103ea81c);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.restore(28);a.finish();
}
}
