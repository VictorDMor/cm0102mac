#include "shortlist_category.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::shortlist_category {
using Call=SemanticGuestCall;
void count_and_prune_club_rows(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,count_and_prune_club_rows_entry,d,o);a.link();a.multiple(23);a.load(29,2,-0x7b80);a.move(23,4);a.load(30,2,-0x7b40);a.move(24,5);a.load(31,2,-0x7be0);a.value(27,0);a.value(25,0);a.value(28,0);a.save_link();a.frame(0x70);a.load(0,4,0);a.load(3,3,0xdc);a.multiply(0,0,12);a.add(26,3,0);a.jump(0x1036622c);
    for(;;){
        a.at(0x1036622c);a.sign8(0,25);a.compare(0,40);if(!a.less())break;
        a.at(0x103660f8);a.load(0,26,0);a.add(4,0,28);a.load(0,4,0x10);a.compare(0,-1);bool keep=!a.nonpositive();
        if(keep){a.load(0,4,0x18);a.compare(0,-1);keep=!a.nonpositive();}
        if(keep){
            a.sign8(3,24);a.compare(3,-1);bool type=a.equal();
            if(!type){a.load<1>(0,4,0);a.sign8(0,0);a.compare_registers(3,0);type=a.equal();}
            if(type){
                a.at(0x10366134);a.load<1>(0,26,4);a.compare(0,2);bool expire=a.equal();
                if(expire){a.move(3,23);a.value(4,1);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);expire=!a.equal();}
                if(expire){
                    a.move(4,29);a.offset(3,1,0x40);a.value(5,20);a.call(0x10717b20);a.load(0,1,0x40);a.load(3,23,0x90);a.store(1,0x38,0);a.load(0,1,0x44);a.load<2>(5,1,0x3a);a.load<2>(4,3,0x4e);a.store(1,0x3c,0);a.compare_registers(5,4,false);
                    if(!a.nonpositive()){a.value(0,1);a.jump(0x103661c8);}
                    else{a.at(0x1036619c);if(!a.at_least()){a.value(0,0);a.jump(0x103661c8);}
                    else{a.at(0x103661a8);a.load<2>(4,1,0x38,true);a.load<2>(0,3,0x4c,true);a.combine(0,4,0,false,true);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);}}
                    a.at(0x103661c8);a.sign8(0,0,true);expire=!a.equal();
                }
                bool remove=false;
                if(expire){a.load(3,26,0);a.offset(0,28,0x10);a.load(4,31,0);a.value(5,0);a.indexed_load(0,3,0);a.value(6,0);a.load(3,30,0);a.multiply(0,0,0x70);a.add(4,4,0);a.call(0x103eaea0);a.compare(3,0);remove=a.equal();}
                if(remove){a.load(4,26,0);a.offset(3,28,0x10);a.value(5,0xffffffff);a.offset(0,28,0x18);a.indexed_store(5,4,3);a.load(3,26,0);a.indexed_store(5,3,0);a.jump(0x10366224);}
                else{a.at(0x10366220);a.offset(27,27,1);}
            }
        }
        a.at(0x10366224);a.offset(28,28,44);a.offset(25,25,1);
    }
    a.at(0x10366238);a.move(3,27);a.pop(0x70);a.multiple(23,true);a.finish();
}
void refresh_unassigned_rows(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,refresh_unassigned_rows_entry,d,o);a.link();a.multiple(20);a.load(27,2,-0x7b7c);a.move(21,3);a.load(28,2,-0x7b78);a.move(22,4);a.load(29,2,-0x7be4);a.move(23,5);a.load(30,2,-0x7be0);a.value(24,0);a.load(31,2,-0x7be8);a.value(26,0);a.save_link();a.frame(0x70);a.jump(0x10366390);
    for(;;){
        a.at(0x10366390);a.sign8(0,24);a.compare(0,40);if(!a.less())break;
        a.at(0x10366290);a.load(5,23,0);a.offset(6,26,0x10);a.load(3,29,0);a.indexed_load(4,5,6);a.offset(0,3,-1);a.compare_registers(4,0);if(!a.nonpositive()){a.value(0,0xffffffff);a.indexed_store(0,5,6);}
        a.at(0x103662b4);a.load(3,23,0);a.offset(0,26,0x10);a.indexed_load(0,3,0);a.compare(0,-1);
        if(!a.nonpositive()){
            a.multiply(0,0,0x70);a.load(4,30,0);a.load(3,27,0);a.value(20,0);a.add(25,4,0);a.move(4,25);a.call(0x10082f20);a.compare(3,0,false);a.store(28,0,3);
            if(!a.equal()){a.load<1>(0,3,0x38);a.mask(0,0,0xfe000000,false,24);a.shift_right(0,0,26);a.sign8(0,0);a.compare(0,5);if(!a.equal())a.value(20,1);}
            a.at(0x1036630c);a.compare(20,0);
            if(!a.equal()){a.load(3,28,0);a.load(4,31,0);a.load(0,3,4);a.multiply(0,0,0x248);a.add(0,4,0);a.jump(0x10366330);}else{a.at(0x1036632c);a.value(0,0);}
            a.at(0x10366330);a.compare_registers(0,22,false);
            if(!a.equal()){
                a.load(0,25,0x24);a.compare_registers(0,22,false);
                if(!a.equal()){
                    a.load(0,23,0);a.add(3,0,26);a.load(0,3,0x18);a.compare(0,-1);
                    if(a.equal()){a.load<1>(0,3,0);a.move(3,21);a.move(4,25);a.move(5,22);a.sign8(8,0);a.value(6,3);a.subtract_from(7,8,11);a.offset(0,8,-11);a.combine(0,7,0);a.mask(0,0,1,false,1);a.sign8(7,0);a.call(0x10365590);}
                }
            }
        }
        a.at(0x10366388);a.offset(26,26,44);a.offset(24,24,1);
    }
    a.at(0x1036639c);a.pop(0x70);a.multiple(20,true);a.finish();
}
namespace {
void compare_pairs(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o,std::uint32_t entry,int key,int tie) {
    Call a(s,entry,d,o);a.compare(3,0,false);
    if(a.equal()){a.value(3,0xffffffff);a.finish();return;}
    a.at(entry+16);a.compare(4,0,false);if(a.equal()){a.value(3,1);a.finish();return;}
    a.at(entry+32);a.load(5,3,key);a.load(0,4,key);a.compare_registers(5,0);
    if(!a.equal()){a.at(entry+88);a.subtract(3,0,5);a.finish();return;}
    a.load(3,3,tie);a.compare(3,-1);bool absent=a.equal();
    if(!absent){a.load(0,4,tie);a.compare(0,-1);absent=a.equal();}
    if(absent){a.at(entry+72);a.value(3,0);a.finish();return;}
    a.at(entry+80);a.subtract(3,0,3);a.finish();
}
}
void compare_pairs_by_second(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){compare_pairs(s,d,o,compare_pairs_by_second_entry,4,0);}
void compare_pairs_by_first(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){compare_pairs(s,d,o,compare_pairs_by_first_entry,0,4);}
}
