#include "transfer_record_detail.hpp"

namespace cm::aot::transfer_records {
using detail::Call;
void construct_empty_offer(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,construct_empty_offer_entry,d,o);a.link();a.save(31);a.move(31,3);a.save_link();a.offset(3,31,0x14);a.frame(64);a.call(0x10715880);a.value(0,0);a.move(3,31);a.store(31,0,0);a.store(31,4,0);a.pop(64);a.restore(31);a.finish();
}
void construct_generated_offer(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,construct_generated_offer_entry,d,o);a.link();a.multiple(26);a.move(26,3);a.move(27,4);a.move(28,5);a.move(29,6);a.move(30,7);a.move(31,8);a.offset(3,26,0x14);a.save_link();a.frame(80);a.call(0x10715880);a.move(3,26);a.move(4,27);a.move(5,28);a.move(6,29);a.move(7,30);a.move(8,31);a.value(9,0);a.call(0x104126d0);a.move(3,26);a.pop(80);a.multiple(26,true);a.finish();
}
void destroy_offer(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,destroy_offer_entry,d,o);a.link();a.save(31);a.move(31,4);a.save(30);a.move(30,3,true);a.save_link();a.frame(64);
    if(!a.equal()){
        a.load(3,30,0);a.compare(3,0,false);
        if(!a.equal()){a.load<2>(0,3,0,true);a.compare(0,1);if(a.equal()){a.value(4,1);a.call(0x10738c20);a.jump(0x10411f5c);}else{a.at(0x10411f54);a.offset(4,30,0x28);a.call(0x10738e60);}}
        a.at(0x10411f5c);a.load(3,30,4);a.compare(3,0,false);if(!a.equal()){a.value(4,1);a.call(0x10738c20);}
        a.at(0x10411f70);a.sign16(0,31,true);if(!a.nonpositive()){a.move(3,30);a.call(0x107735a0);}
    }
    a.at(0x10411f80);a.move(3,30);a.pop(64);a.restore(31);a.restore(30);a.finish();
}
}
