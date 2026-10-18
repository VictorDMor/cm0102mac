#include "club_shortlist_storage_detail.hpp"

namespace cm::aot::club_shortlist_storage {
using detail::Call;
namespace {
void initialize_club_rows(Call& a) {
    // Forty 44-byte records, with the original partial-byte flag writes and
    // repeated table reads retained across the date-construction boundary.
    do {
        a.at(0x1035662c);a.load(0,21,0xdc);a.value(4,11);a.offset(5,26,2);a.value(7,0);a.indexed_load(3,25,0);
        a.offset(18,26,4);a.offset(19,26,8);a.offset(20,26,12);a.indexed_store<1>(4,3,26);a.offset(11,26,16);a.value(6,0xffffffff);a.offset(10,26,20);
        a.load(3,21,0xdc);a.offset(0,26,0x28);a.value(12,255);a.offset(9,26,24);a.indexed_load(17,25,3);a.offset(8,26,0x24);
        a.offset(3,1,0x38);a.value(4,1);a.indexed_store<2>(7,17,5);a.value(5,1997);
        a.load(17,21,0xdc);a.indexed_load(17,25,17);a.indexed_store(7,17,18);
        a.load(18,21,0xdc);a.indexed_load(18,25,18);a.indexed_store(7,18,19);
        a.load(19,21,0xdc);a.indexed_load(19,25,19);a.indexed_store(7,19,20);
        a.load(20,21,0xdc);a.indexed_load(20,25,20);a.indexed_store(6,20,11);
        a.load(11,21,0xdc);a.indexed_load(11,25,11);a.indexed_store(6,11,10);
        a.load(10,21,0xdc);a.indexed_load(11,25,10);a.indexed_load<1>(10,11,0);a.insert_mask(10,12,0x78,3);a.indexed_store<1>(10,11,0);
        a.load(10,21,0xdc);a.indexed_load(10,25,10);a.indexed_store(6,10,9);
        a.load(9,21,0xdc);a.indexed_load(10,25,9);a.indexed_load<1>(9,10,0);a.insert_mask(9,7,0x80,7);a.indexed_store<1>(9,10,0);
        a.load(9,21,0xdc);a.indexed_load(9,25,9);a.indexed_store<1>(6,9,8);
        a.load(6,21,0xdc);a.indexed_load(8,25,6);a.indexed_load<1>(6,8,0);a.insert_mask(6,7,4,2);a.indexed_store<1>(6,8,0);a.call(0x107156b0);
        a.load(3,21,0xdc);a.offset(4,26,0x1c);a.offset(22,22,1);a.load<2>(0,1,0x3a);a.indexed_load(3,25,3);a.compare(22,40);a.offset(26,26,44);a.add(4,3,4);
        a.store<2>(4,2,0);a.load<2>(0,1,0x38,true);a.store<2>(4,0,0);a.load<1>(0,1,0x3c);a.store<1>(4,4,0);
    } while(a.less());
}
}
void construct_shortlist_storage(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,construct_shortlist_storage_entry,d,o);a.link();a.multiple(17);a.load(31,2,-0x7b50);a.move(21,3);a.load(27,2,-0x7be8);a.move(17,4);a.load(28,2,-0x7bac);a.load(29,2,-0x7ba8);a.load(30,2,-0x4dd8);a.save_link();a.frame(0xd0);
    a.load(0,31,0);a.multiply(3,0,12);a.call(0x10775130);a.store(21,0xdc,3);a.load(0,21,0xdc);a.compare(0,0,false);
    if(a.equal())detail::fatal_allocation(a,0x10356458,30,29,28,0x70,0x48,0x51);
    // Keep the original constant guards: structural controls can alter them.
    a.at(0x103564b0);a.value(0,0);a.compare(0,16);if(!a.at_least()){a.value(0,0);for(unsigned off=0x18;off<0x58;off+=4)a.store(21,off,0);}
    a.at(0x10356500);a.value(0,0);a.compare(0,16);if(!a.at_least()){a.value(0,0);for(unsigned off=0x58;off<0x98;off+=4)a.store(21,off,0);}
    a.at(0x10356550);a.compare(17,0,false);
    if(!a.equal()){a.move(3,21);a.move(4,17);a.call(load_shortlist_storage_entry);a.move(3,21);a.jump(0x10356794);}
    else {
        a.at(0x1035656c);a.value(23,0);a.move(25,23);a.move(24,23);a.jump(0x1035675c);
        for(;;){
            a.at(0x1035675c);a.load(0,31,0);a.compare_registers(23,0);if(!a.less())break;
            a.at(0x1035657c);a.value(3,0x6e0);a.call(0x10775130);a.load(4,21,0xdc);a.indexed_store(3,4,25);a.load(3,21,0xdc);a.indexed_load(0,3,25);a.compare(0,0,false);
            if(a.equal())detail::fatal_allocation(a,0x1035659c,30,29,28,0x50,0x40,0x6c);
            a.at(0x103565f4);a.load(3,21,0xdc);a.value(26,0);a.offset(0,25,4);a.offset(5,24,0x1ec);a.indexed_store<1>(26,3,0);a.offset(3,25,6);a.offset(0,25,9);a.value(22,0);
            a.load(6,27,0);a.load(4,21,0xdc);a.indexed_load<2>(5,6,5,true);a.indexed_store<2>(5,4,3);a.load(3,21,0xdc);a.indexed_store<1>(26,3,0);
            initialize_club_rows(a);a.offset(25,25,12);a.offset(24,24,0x248);a.offset(23,23,1);
        }
        a.at(0x10356768);a.value(0,0);a.move(3,21);a.store(21,8,0);a.value(4,1);a.store(21,12,0);a.store(21,16,0);a.store(21,20,0);a.store(21,0,0);a.store(21,4,0);a.call(reset_shortlist_storage_entry);a.move(3,21);
    }
    a.at(0x10356794);a.pop(0xd0);a.multiple(17,true);a.finish();
}
void destroy_shortlist_storage(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,destroy_shortlist_storage_entry,d,o);a.link();a.multiple(24);a.move(30,3,true);a.load(28,2,-0x7b50);a.move(31,4);a.save_link();a.frame(0x60);
    if(!a.equal()){
        a.load(0,30,0xdc);a.compare(0,0,false);
        if(!a.equal()){
            a.value(27,0);a.value(26,0);a.move(29,27);a.jump(0x1035680c);
            for(;;){a.at(0x1035680c);a.load(0,28,0);a.compare_registers(26,0);if(!a.less())break;
                a.at(0x103567f0);a.load(3,30,0xdc);a.indexed_load(3,3,27);a.call(0x10775170);a.load(3,30,0xdc);a.offset(26,26,1);a.indexed_store(29,3,27);a.offset(27,27,12);
            }
            a.at(0x10356818);a.load(3,30,0xdc);a.call(0x10775170);a.value(0,0);a.store(30,0xdc,0);
        }
        a.at(0x10356828);a.value(28,0);a.value(25,0);a.move(29,28);
        do{a.at(0x10356834);a.value(24,0);a.value(27,0);a.flags(0,0,0);
            do{a.at(0x10356840);a.add(3,28,27);a.offset(26,3,0x18);a.indexed_load(3,30,26);a.compare(3,0,false);if(!a.equal()){a.call(0x10775170);a.indexed_store(29,30,26);}
                a.at(0x1035685c);a.offset(24,24,1);a.offset(27,27,4);a.compare(24,16);
            }while(a.less());
            a.offset(25,25,1);a.offset(28,28,64);a.compare(25,2);
        }while(a.less());
        a.at(0x1035687c);a.load(3,30,0);a.compare(3,0,false);if(!a.equal()){a.call(0x10775170);a.value(0,0);a.store(30,0,0);}
        a.at(0x10356894);a.load(3,30,4);a.compare(3,0,false);if(!a.equal()){a.call(0x10775170);a.value(0,0);a.store(30,4,0);}
        a.at(0x103568ac);a.sign16(0,31,true);if(!a.nonpositive()){a.move(3,30);a.call(0x107735a0);}
    }
    a.at(0x103568bc);a.move(3,30);a.pop(0x60);a.multiple(24,true);a.finish();
}
void initialize_auxiliary_record(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,initialize_auxiliary_record_entry,d,o);a.link();a.save(31);a.move(31,3);a.save_link();a.offset(3,31,0x1c);a.frame(64);a.call(0x10715880);a.move(3,31);a.pop(64);a.restore(31);a.finish();
}
}
