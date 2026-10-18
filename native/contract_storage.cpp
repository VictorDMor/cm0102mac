#include "contract_storage.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::contract_storage {
namespace {
using Call=SemanticGuestCall;
bool reuse_empty_record(Call& a) {
    a.load(4,31,0);a.value(3,0);a.jump(0x10078f50);
    while(true) {
        a.at(0x10078f50);a.load(0,31,8);a.compare_registers(3,0);
        if(!a.less())return false;
        a.at(0x10078f38);a.load(0,4,0);a.compare(0,-1);
        if(a.equal()){a.jump(0x10079034);return true;}
        a.at(0x10078f48);a.offset(3,3,1);a.offset(4,4,100);
    }
}
void report_allocation_failure(Call& a) {
    a.offset(3,30,0x37);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(4,0);a.value(5,0);a.call(0x10739890);
    a.call(0x10768810);a.move(5,3);a.move(3,29);a.offset(4,30,0x4c);a.offset(6,1,0x40);a.offset(7,1,0x38);a.offset(9,30,0x1e0);a.value(8,0x172);a.call(0x1077b720);
    a.move(3,28);a.offset(4,30,0xb8);a.call(0x10727890);
    a.move(3,27);a.move(5,29);a.offset(4,30,0xd0);a.call(0x10727890);
    a.move(3,28);a.move(4,27);a.value(5,0);a.call(0x10769b60);
    a.value(3,0xffffffff);a.jump(0x10079034);
}
void append_record(Call& a) {
    a.at(0x10078f5c);a.load(0,31,8);a.load(3,31,4);a.compare_registers(0,3);
    if(a.equal()) {
        a.offset(0,3,200);a.store(31,4,0);a.load(3,31,0);a.compare(3,0,false);
        if(!a.equal()) {
            a.load(0,31,4);a.multiply(4,0,100);a.call(0x107751b0);a.store(31,0,3);a.jump(0x10078fa4);
        } else {
            a.at(0x10078f94);a.load(0,31,4);a.multiply(3,0,100);a.call(0x10775130);a.store(31,0,3);
        }
        a.at(0x10078fa4);a.load(0,31,0);a.compare(0,0,false);
        if(a.equal()){report_allocation_failure(a);return;}
    }
    a.at(0x10079020);a.load(3,31,8);a.offset(0,3,1);a.store(31,8,0);a.load(3,31,8);a.offset(3,3,-1);
}
}

void reserve_record(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,reserve_entry,d,o);a.link();a.multiple(27);a.load(27,2,-0x79e0);a.move(31,3);
    a.load(28,2,-0x79dc);a.load(29,2,-0x7ba8);a.load(30,2,-0x5718);a.save_link();a.sign8(0,4,true);a.frame(128);
    bool reused=false;if(a.equal())reused=reuse_empty_record(a);
    if(!reused)append_record(a);
    a.at(0x10079034);a.pop(128);a.multiple(27,true);a.finish();
}

void bind_primary(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,bind_primary_entry,d,o);a.link();a.save(31);a.move(31,5);a.save(30);a.move(30,4);a.value(4,1);
    a.save(29);a.move(29,3);a.save_link();a.frame(80);a.call(reserve_entry);a.compare(3,0);
    if(!a.less()) {
        a.load(0,30,0);a.multiply(7,3,100);a.load(6,2,-0x7b9c);a.move(4,30);a.move(5,31);a.multiply(0,0,100);
        a.load(6,6,0);a.indexed_store(3,6,0);a.load(0,30,0);a.load(3,29,0);a.indexed_store(0,3,7);
        a.load(0,29,0);a.add(3,0,7);a.call(initializer_entry);
    }
    a.at(0x10078edc);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
}
