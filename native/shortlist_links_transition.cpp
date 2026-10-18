#include "shortlist_links.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::shortlist_links {
using Call=SemanticGuestCall;
namespace {
void remove_pair_index(Call& a,std::uint32_t pc,unsigned pointer,unsigned count,int comparator) {
    a.at(pc);a.load(4,21,pointer);a.offset(3,1,0x58);a.load(5,21,count);a.value(6,8);a.load(7,2,comparator);a.call(0x10775600);a.compare(3,0,false);
    if(a.equal()){a.at(pc+172);a.value(28,1);return;}
    a.load(0,21,pointer);a.subtract(0,0,3);a.mask(3,0,0x1fffffff,false,29);a.move(6,3);a.mask(5,0,0xfffffff8);a.jump(pc+100);
    for(;;){
        a.at(pc+100);a.load(4,21,count);a.offset(0,4,-1);a.compare_registers(6,0);if(!a.less())break;
        a.at(pc+56);a.offset(0,6,1);a.load(3,21,pointer);a.mask(0,0,0xfffffff8,false,3);a.offset(6,6,1);a.add(4,3,0);a.add(3,3,5);a.load(0,4,0);a.offset(5,5,8);a.store(3,0,0);a.load(0,4,4);a.store(3,4,0);
    }
    a.at(pc+116);a.load(3,21,pointer);a.mask(0,4,0xfffffff8,false,3);a.value(4,0);a.add(3,3,0);a.store(3,-4,4);a.load(0,21,count);a.load(3,21,pointer);a.mask(0,0,0xfffffff8,false,3);a.add(3,3,0);a.store(3,-8,4);a.load(3,21,count);a.offset(0,3,-1);a.store(21,count,0);a.jump(pc+176);
}
bool remove_cached_link(Call& a) {
    a.at(0x10365680);a.load(0,21,0);a.compare(0,0,false);
    if(a.equal()){a.offset(3,1,0x60);a.value(4,0xffffffff);a.call(0x10738c20);a.value(3,0);a.jump(0x10365c14);return false;}
    a.at(0x103656a0);a.load(0,21,4);a.compare(0,0,false);
    if(a.equal()){a.offset(3,1,0x60);a.value(4,0xffffffff);a.call(0x10738c20);a.value(3,0);a.jump(0x10365c14);return false;}
    a.at(0x103656c0);a.load(0,22,0);a.compare(23,0,false);a.store(1,0x58,0);
    if(!a.equal()){a.load(0,23,0);a.store(1,0x5c,0);a.jump(0x103656e4);}else{a.at(0x103656dc);a.value(0,0xffffffff);a.store(1,0x5c,0);}
    remove_pair_index(a,0x103656e4,0,0x10,-0x62a8);remove_pair_index(a,0x10365794,4,0x14,-0x6298);
    a.at(0x10365844);a.move(3,23);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
    bool decrement=a.equal();if(decrement){a.sign8(0,28,true);decrement=a.equal();}
    if(decrement){a.load(0,22,0);a.load(4,2,-0x7b9c);a.multiply(3,0,100);a.load(5,4,0);a.offset(6,3,12);a.indexed_load<1>(4,5,6);a.mask(3,4,15,false,28);a.offset(0,3,-1);a.insert_mask(4,0,0xf0,4);a.indexed_store<1>(4,5,6);}
    return true;
}
void withdraw_and_reassign_row(Call& a) {
    a.at(0x10365964);a.value(5,0xffffffff);a.offset(0,28,0x18);a.store(3,0x10,5);a.offset(6,28,0x14);a.load(3,27,0);a.indexed_store(5,3,0);a.load(3,27,0);a.load(4,22,0);a.indexed_load(0,3,6);a.compare_registers(4,0);if(a.equal())a.indexed_store(5,3,6);
    a.at(0x10365994);a.load(7,27,0);a.offset(4,28,0x28);a.value(5,0);a.move(3,21);a.indexed_load<1>(0,7,4);a.insert_mask(0,5,0x80,7);a.move(5,23);a.move(6,22);a.indexed_store<1>(0,7,4);a.load(0,27,0);a.add(4,0,28);a.call(0x1035e9f0);a.jump(0x10365bf0);
}
void defer_row_reference(Call& a,std::uint32_t pc,unsigned scratch,unsigned days) {
    a.at(pc);a.move(4,29);a.offset(3,1,scratch);a.value(5,days);a.call(0x10717890);
    a.load(3,27,0);a.offset(9,28,0x1c);a.load<2>(0,1,scratch+2);a.offset(6,28,0x14);a.add(9,3,9);a.offset(4,28,0x18);a.store<2>(9,2,0);a.value(7,0xffffffff);a.offset(3,28,12);a.value(5,0);a.load<2>(8,1,scratch,true);a.compare(30,0,false);a.offset(0,28,8);a.store<2>(9,0,8);a.load<1>(8,1,scratch+4);a.store<1>(9,4,8);a.load(9,22,0);a.load(8,27,0);a.indexed_store(9,8,6);a.load(6,27,0);a.indexed_store(7,6,4);a.load(4,27,0);a.indexed_store(5,4,3);a.load(3,27,0);a.indexed_store(5,3,0);
    if(!a.equal()){a.load(4,27,0);a.offset(3,28,0x28);a.load<1>(0,30,0x60);a.indexed_load<1>(3,4,3);a.mask(3,3,15,false,29);a.sign8(3,3);a.insert_mask(0,3,15,0);a.store<1>(30,0x60,0);}
    a.at(pc+152);a.value(0,100);a.store<1>(27,9,0);a.jump(0x10365bf0);
}
void expire_primary_row(Call& a) {
    a.at(0x10365b10);a.value(7,0xffffffff);a.offset(0,28,0x18);a.store(3,0x10,7);a.move(4,29);a.offset(3,1,0x40);a.value(5,120);a.load(6,27,0);a.indexed_store(7,6,0);a.call(0x10717b20);a.load(3,27,0);a.offset(4,28,0x1c);a.load<2>(0,1,0x42);a.offset(5,28,0x14);a.add(4,3,4);a.store<2>(4,2,0);a.load<2>(0,1,0x40,true);a.store<2>(4,0,0);a.load<1>(0,1,0x44);a.store<1>(4,4,0);a.load(3,27,0);a.load(4,22,0);a.indexed_load(0,3,5);a.compare_registers(4,0);
    if(!a.equal())return;a.value(0,0xffffffff);a.indexed_store(0,3,5);a.jump(0x10365bf0);
}
void expire_matching_row(Call& a) {
    a.at(0x10365b7c);a.load(0,3,0x14);a.compare_registers(4,0);if(a.equal()){a.value(0,0xffffffff);a.store(3,0x14,0);}
    a.at(0x10365b90);a.load(3,27,0);a.offset(5,28,0x10);a.load(4,22,0);a.indexed_load(0,3,5);a.compare_registers(4,0);if(!a.equal())return;
    a.value(7,0xffffffff);a.offset(0,28,0x18);a.indexed_store(7,3,5);a.move(4,29);a.offset(3,1,0x38);a.value(5,120);a.load(6,27,0);a.indexed_store(7,6,0);a.call(0x10717b20);a.load(3,27,0);a.offset(4,28,0x1c);a.load<2>(0,1,0x3a);a.add(4,3,4);a.store<2>(4,2,0);a.load<2>(0,1,0x38,true);a.store<2>(4,0,0);a.load<1>(0,1,0x3c);a.store<1>(4,4,0);
}
void transition_matching_row(Call& a) {
    a.at(0x10365934);a.sign8(0,24);a.compare(0,3);
    if(a.equal()){expire_primary_row(a);return;}
    if(a.at_least()){
        a.at(0x10365954);a.compare(0,5);if(a.equal()){defer_row_reference(a,0x10365a6c,0x48,14);return;}
        if(a.at_least())return;a.jump(0x10365b7c);expire_matching_row(a);
    }else{
        a.compare(0,1);if(a.equal()){withdraw_and_reassign_row(a);return;}
        if(a.at_least())defer_row_reference(a,0x103659c8,0x50,120);else a.jump(0x10365bf0);
    }
}
void transition_club_rows(Call& a) {
    a.at(0x10365894);a.load(0,23,0);a.move(4,22);a.load(3,2,-0x7b7c);a.move(5,23);a.multiply(0,0,12);a.load(6,21,0xdc);a.load(3,3,0);a.add(27,6,0);a.call(0x10082f90);a.move(30,3);a.sign8(31,25);a.value(26,0);a.value(28,0);a.jump(0x10365bf8);
    for(;;){
        a.at(0x10365bf8);a.sign8(0,26);a.compare(0,40);if(!a.less())break;
        a.at(0x103658cc);a.compare(31,1);bool inspect=true;
        if(a.equal()){a.load(3,27,0);a.indexed_load<1>(0,3,28);a.sign8(0,0);a.compare(0,11);inspect=!a.equal();}
        if(inspect){a.at(0x103658e8);a.sign8(0,25,true);if(a.equal()){a.load(3,27,0);a.indexed_load<1>(0,3,28);a.sign8(0,0);a.compare(0,11);inspect=a.equal();}}
        if(inspect){
            a.at(0x10365904);a.load(0,27,0);a.load(4,22,0);a.add(3,0,28);a.load(0,3,0x10);a.compare_registers(4,0);bool match=a.equal();
            if(!match){a.sign8(0,24);a.compare(0,4);if(a.equal()){a.load(0,3,0x14);a.compare_registers(4,0);match=a.equal();}}
            if(match)transition_matching_row(a);
        }
        a.at(0x10365bf0);a.offset(28,28,44);a.offset(26,26,1);
    }
    a.at(0x10365c04);a.offset(3,1,0x60);a.value(4,0xffffffff);a.call(0x10738c20);a.value(3,1);
}
}
void transition_staff_links(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,transition_staff_links_entry,d,o);a.link();a.multiple(21);a.load(29,2,-0x7b80);a.move(21,3);a.load(26,2,-0x7be8);a.move(22,4);a.move(23,5);a.move(24,6);a.move(25,7);a.save_link();a.frame(0xa0);a.offset(3,1,0x60);a.call(0x10738c00);a.compare(23,0,false);a.value(28,0);bool gather=a.equal();
    if(!gather){a.load(0,22,0x24);a.compare_registers(23,0,false);gather=a.equal();}
    if(gather){
        a.at(0x103655dc);a.sign8(0,24);a.compare(0,4);
        if(a.equal()){
            a.move(3,21);a.move(4,22);a.offset(5,1,0x60);a.value(6,0);a.value(7,3);a.value(8,0);a.call(collect_staff_links_entry);a.load(0,1,0x64);a.store(1,0x6c,0);a.jump(0x10365634);
            for(;;){a.at(0x10365634);a.offset(3,1,0x60);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
                a.at(0x10365610);a.load(0,3,4);a.move(3,21);a.load(5,26,0);a.move(4,22);a.multiply(0,0,0x248);a.value(6,1);a.value(7,2);a.add(5,5,0);a.call(transition_staff_links_entry);}
        }
    }
    a.at(0x10365644);a.compare(23,0,false);
    if(a.equal()){a.offset(3,1,0x60);a.value(4,0xffffffff);a.call(0x10738c20);a.value(3,1);a.jump(0x10365c14);}
    else{
        a.at(0x10365660);a.sign8(0,24);a.compare(0,5);bool remove=a.equal();
        if(!remove){a.compare(0,4);if(a.equal())remove=false;else{a.load(0,22,0x24);a.compare_registers(23,0,false);remove=!a.equal();}}
        if(!remove||remove_cached_link(a))transition_club_rows(a);
    }
    a.at(0x10365c14);a.pop(0xa0);a.multiple(21,true);a.finish();
}
}
