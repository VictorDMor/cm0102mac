#include "awol_records.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::awol {
namespace {
using Call=SemanticGuestCall;
void allocate_lists(Call& a,std::uint32_t start) {
    for(unsigned slot=0;slot<3;++slot) {
        a.at(start+slot*24);a.value(3,16);a.call(0x10773520);a.move(31,3,true);
        if(!a.equal())a.call(0x10738c00);
        a.at(start+slot*24+20);a.store(30,int(slot*4),31);
    }
}
}
void construct_records(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,constructor_entry,d,o);a.link();a.save(31);a.compare(4,0,false);a.load(31,2,-0x7be4);a.save(30);a.move(30,3);a.save_link();a.value(0,0);a.frame(64);
    a.store(3,12,0);a.store(3,0,0);a.store(3,4,0);a.store(3,8,0);
    if(!a.equal()) {
        a.call(load_entry);a.sign8(0,3,true);
        if(a.equal()) {
            a.load(3,30,12);a.compare(3,0,false);
            if(!a.equal()){a.call(0x10775170);a.value(0,0);a.store(30,12,0);}
            a.at(0x1000320c);a.load(3,30,0);a.compare(3,0,false);
            if(!a.equal()){a.value(4,1);a.call(0x10738c20);a.value(0,0);a.store(30,0,0);}
            a.at(0x10003228);a.load(3,31,0);a.value(4,1);a.call(0x10775200);a.store(30,12,3);
            allocate_lists(a,0x10003238);a.at(0x10003280);a.jump(0x100032dc);
        }
    } else {
        a.at(0x10003284);a.load(3,31,0);a.value(4,1);a.call(0x10775200);a.store(30,12,3);allocate_lists(a,0x10003294);
    }
    a.at(0x100032dc);a.move(3,30);a.pop(64);a.restore(31);a.restore(30);a.finish();
}
void destroy_records(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,destructor_entry,d,o);a.link();a.save(31);a.move(31,4);a.save(30);a.move(30,3,true);a.save_link();a.frame(64);
    if(!a.equal()) {
        a.load(3,30,12);a.compare(3,0,false);
        if(!a.equal()){a.call(0x10775170);a.value(0,0);a.store(30,12,0);}
        for(unsigned slot=0;slot<3;++slot) {
            a.at(0x10003338+slot*28);a.load(3,30,int(slot*4));a.compare(3,0,false);
            if(!a.equal()){a.value(4,1);a.call(0x10738c20);a.value(0,0);a.store(30,int(slot*4),0);}
        }
        a.at(0x1000338c);a.sign16(0,31,true);
        if(!a.nonpositive()){a.move(3,30);a.call(0x107735a0);}
    }
    a.at(0x1000339c);a.move(3,30);a.pop(64);a.restore(31);a.restore(30);a.finish();
}
}
