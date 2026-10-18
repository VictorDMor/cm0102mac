#include "profile_qualification_detail.hpp"

namespace cm::aot::profile_qualification {
void combine_qualification_masks(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,combine_qualification_masks_entry,d,o);
    a.multiple(27);a.sign16(9,3);a.load(10,2,-0x5520);a.sign16(8,4);a.load(7,2,-0x5524);a.offset(6,1,-0x2c);a.value(27,0);
    a.load(28,10,0);a.load(29,10,4);a.load(30,10,8);a.load(31,10,12);a.load(12,7,0);a.load(11,7,4);a.load<2>(10,7,8);a.load<2>(7,7,10);
    a.store(1,-0x2c,28);a.store(1,-0x28,29);a.store(1,-0x24,30);a.store(1,-0x20,31);a.store(1,-0x38,12);a.store(1,-0x34,11);a.store<2>(1,-0x30,10);a.store<2>(1,-0x2e,7);a.jump(0x101173e0);
    // The last matching table index wins. Absent bits retain the original
    // incoming index registers; initializing them would change the contract.
    while(true) {
        a.at(0x101173e0);a.load<2>(10,6,0,true);a.compare(10,-1);
        if(a.equal())break;
        a.at(0x101173c0);a.intersect_record(7,9,10);if(!a.equal())a.move(0,27);
        a.at(0x101173cc);a.intersect_record(7,8,10);if(!a.equal())a.move(5,27);
        a.at(0x101173d8);a.offset(6,6,2);a.offset(27,27,1);
    }
    a.sign16(10,3);a.sign16(9,4);a.offset(7,1,-0x38);a.offset(6,1,-0x38);a.value(12,0);a.jump(0x10117428);
    while(true) {
        a.at(0x10117428);a.load<2>(11,6,0,true);a.compare(11,-1);
        if(a.equal())break;
        a.at(0x10117408);a.intersect_record(8,10,11);if(!a.equal())a.move(3,12);
        a.at(0x10117414);a.intersect_record(8,9,11);if(!a.equal())a.move(4,12);
        a.at(0x10117420);a.offset(6,6,2);a.offset(12,12,1);
    }
    a.sign8(6,0);a.sign8(0,5);a.add(6,6,0);a.sign8(3,3);a.sign8(0,4);a.offset(4,1,-0x2c);
    a.mask(5,6,1,false,1);a.add(3,3,0);a.add(5,5,6);a.mask(0,3,1,false,1);a.shift_right(5,5,1);a.add(0,0,3);a.shift_right(0,0,1);
    a.mask(3,5,0xfffffffe,false,1);a.mask(0,0,0xfffffffe,false,1);a.indexed_load<2>(3,4,3,true);a.indexed_load<2>(0,7,0,true);a.combine(0,3,0);a.sign16(3,0);a.multiple(27,true);a.finish();
}
}
