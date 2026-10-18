#include "club_shortlist_storage_detail.hpp"

namespace cm::aot::club_shortlist_storage {
using detail::Call;
namespace {
void choose_missing_career(Call& a) {
    a.at(0x10356bac);a.load(3,2,-0x7b04);a.load(22,3,0);a.compare(22,0);
    if(a.equal()){a.value(17,0);a.jump(0x10356d10);}
    else {
        a.at(0x10356bc4);a.load(3,2,-0x7b88);a.load(4,27,0);a.load(0,3,0);a.offset(3,4,2);a.compare_registers(3,0,false);a.store(27,0,3);
        if(!a.nonpositive()){a.store(27,0,13);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(26,0,0);}
        a.at(0x10356bf0);a.value(3,0x10000);a.offset(0,3,-1);a.compare_registers(22,0);bool wide=a.positive();
        if(!wide){a.value(3,0xffff0000);a.offset(0,3,1);a.compare_registers(22,0);wide=!a.at_least();}
        if(wide){
            a.at(0x10356c10);a.value(0,0x43300000);a.exclusive_flags(3,22,0x80000000);a.store(1,0x9c,3);a.value(3,0x80010000);a.load(7,27,0);a.value(4,0x10000);a.store(1,0x98,0);a.offset(6,3,-0x7fff);
            a.floating_load(2,25,8);a.multiply(3,22,10);a.floating_load(1,1,0x98);a.offset(5,4,-1);a.floating_load(0,25,0);a.floating_subtract(1,1,2);a.multiply_high(4,6,3);a.load<2>(8,7,0);a.load(7,26,0);a.floating_difference(1,1,0,true);
            a.store(1,0x90,0);a.add(0,8,7);a.multiply_high(6,6,0);a.add(3,4,3);a.add(4,6,0);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.shift_right(3,3,15);a.add(4,4,6);a.product(5,4,5);a.mask(4,3,1,false,1);a.add(3,3,4);
            a.subtract(0,5,0);a.exclusive_flags(0,0,0x80000000);a.store(1,0x94,0);a.floating_load(0,1,0x90);a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x88);a.load(17,1,0x8c);a.call(sample_shortlist_range_entry);
            a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,22,-1);a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(17,17,3);a.compare_registers(17,0);
            if(!a.nonpositive()){a.move(17,0);a.jump(0x10356d10);}
        }else{
            a.at(0x10356ce4);a.load(3,27,0);a.compare(22,0);a.load(0,26,0);a.load<2>(3,3,0);a.add(3,3,0);a.divide(0,3,22);a.product(0,0,22);a.subtract(0,0,3);a.negate(17,0);if(!a.nonpositive())a.move(17,0);
        }
    }
    a.at(0x10356d10);a.multiply(0,17,0x128);a.load(3,14,0);a.add(0,3,0);a.store(18,12,0);
    a.load(0,20,0x98);a.load(5,18,12);a.multiply(3,0,12);a.load(4,20,0x18);a.load(5,5,0x68);a.offset(0,3,4);a.indexed_store(5,4,0);
    a.load(0,20,0x98);a.load(5,18,12);a.multiply(3,0,12);a.load(4,20,0x58);a.load(5,5,0x68);a.offset(0,3,4);a.indexed_store(5,4,0);
}
void append_staff_category(Call& a) {
    a.at(0x10356adc);a.add(20,15,21);a.load(0,15,0xd8);a.load(3,20,0x98);a.compare_registers(3,0);
    if(!a.less())detail::fatal_allocation(a,0x10356af0,31,30,29,0x48,0x38,0x11e);
    a.at(0x10356b48);a.load(0,20,0x98);a.load(3,20,0x18);a.multiply(0,0,12);a.indexed_store(19,3,0);
    a.load(0,20,0x98);a.load(3,20,0x58);a.multiply(0,0,12);a.indexed_store(19,3,0);a.load(3,18,12);a.compare(3,0,false);
    if(!a.equal()){
        a.load(0,20,0x98);a.load(5,3,0x68);a.multiply(3,0,12);a.load(4,20,0x18);a.offset(0,3,4);a.indexed_store(5,4,0);
        a.load(0,20,0x98);a.load(5,18,12);a.multiply(3,0,12);a.load(4,20,0x58);a.load(5,5,0x68);a.offset(0,3,4);a.indexed_store(5,4,0);a.jump(0x10356d58);
    }else choose_missing_career(a);
    a.at(0x10356d58);a.load(0,20,0x98);a.move(4,18);a.load(6,18,8);a.multiply(3,0,12);a.load(5,20,0x18);a.offset(0,3,8);a.indexed_store(6,5,0);a.load(3,23,0);a.call(0x10082f20);a.compare(3,0,false);a.store(24,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(5,3,0);a.jump(0x10356da4);}else{a.at(0x10356da0);a.value(5,0);}
    a.at(0x10356da4);a.load(3,23,0);a.move(4,18);a.call(0x10082f90);a.compare(3,0,false);
    if(!a.equal()){a.load(0,20,0x98);a.load<1>(5,3,0x44);a.multiply(3,0,12);a.load(4,20,0x58);a.sign8(5,5);a.offset(0,3,8);a.indexed_store(5,4,0);a.jump(0x10356df0);}
    else{a.at(0x10356dd8);a.load(0,20,0x98);a.value(5,100);a.load(4,20,0x58);a.multiply(3,0,12);a.offset(0,3,8);a.indexed_store(5,4,0);}
    a.at(0x10356df0);a.load(3,20,0x98);a.offset(0,3,1);a.store(20,0x98,0);
}
}
void reset_shortlist_storage(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,reset_shortlist_storage_entry,d,o);a.link();a.multiple(13);a.value(18,0);a.load(23,2,-0x7b7c);a.load(24,2,-0x7b78);a.move(15,3);a.load(14,2,-0x7b98);a.move(16,18);a.load(25,2,-0x4ddc);a.value(21,0);
    a.load(26,2,-0x7b90);a.load(13,2,-0x7b8c);a.load(27,2,-0x7b84);a.load(28,2,-0x7be4);a.load(22,2,-0x7be0);a.load(29,2,-0x7bac);a.load(30,2,-0x7ba8);a.load(31,2,-0x4dd8);a.save_link();a.frame(0xf0);
    do{
        a.at(0x10356930);a.value(20,0);a.value(17,0);
        do{a.at(0x10356938);a.add(3,18,17);a.offset(19,3,0x18);a.indexed_load(3,15,19);a.compare(3,0,false);if(!a.equal()){a.call(0x10775170);a.indexed_store(16,15,19);}
            a.at(0x10356954);a.offset(20,20,1);a.offset(17,17,4);a.compare(20,16);
        }while(a.less());
        a.offset(21,21,1);a.offset(18,18,64);a.compare(21,2);
    }while(a.less());
    a.value(0,0);a.value(18,0);a.store(15,0xd8,0);a.jump(0x103569d8);
    for(;;){
        a.at(0x103569d8);a.sign8(0,18);a.compare(0,16);if(!a.less())break;
        a.at(0x10356984);a.load(16,22,0);a.value(17,0);a.value(19,0);a.jump(0x103569b8);
        for(;;){
            a.at(0x103569b8);a.load(0,28,0);a.compare_registers(19,0);if(!a.less())break;
            a.at(0x10356994);a.move(3,15);a.move(4,16);a.move(5,18);a.call(0x10365d10);a.sign8(0,3,true);if(!a.equal())a.offset(17,17,1);
            a.at(0x103569b0);a.offset(19,19,1);a.offset(16,16,0x70);
        }
        a.at(0x103569c4);a.load(0,15,0xd8);a.compare_registers(17,0);if(!a.nonpositive())a.store(15,0xd8,17);
        a.at(0x103569d4);a.offset(18,18,1);
    }
    a.at(0x103569e4);a.value(18,0);a.move(17,18);
    do{
        a.at(0x103569ec);a.value(19,0);a.value(16,0);a.jump(0x10356a7c);
        for(;;){
            a.at(0x10356a7c);a.sign8(0,19);a.compare(0,16);if(!a.less())break;
            a.at(0x103569f8);a.load(0,15,0xd8);a.multiply(3,0,12);a.call(0x10775130);a.add(4,17,16);a.offset(0,4,0x18);a.indexed_store(3,15,0);a.indexed_load(0,15,0);a.compare(0,0,false);
            if(a.equal())detail::fatal_allocation(a,0x10356a1c,31,30,29,0x68,0x40,0x109);
            a.at(0x10356a74);a.offset(16,16,4);a.offset(19,19,1);
        }
        a.at(0x10356a88);a.offset(18,18,1);a.offset(17,17,64);a.compare(18,2);
    }while(a.less());
    a.offset(3,15,0x98);a.value(4,0);a.value(5,64);a.call(0x10778300);a.load(18,22,0);a.value(19,0);a.jump(0x10356e18);
    // The original deliberately leaves the last sixteen staff out of these
    // lists, even though the preceding capacity scan includes them.
    for(;;){
        a.at(0x10356e18);a.load(3,28,0);a.offset(0,3,-16);a.compare_registers(19,0);if(!a.less())break;
        a.at(0x10356ab4);a.value(16,0);a.value(21,0);a.jump(0x10356e04);
        for(;;){
            a.at(0x10356e04);a.sign8(0,16);a.compare(0,16);if(!a.less())break;
            a.at(0x10356ac4);a.move(3,15);a.move(4,18);a.move(5,16);a.call(0x10365d10);a.sign8(0,3,true);if(!a.equal())append_staff_category(a);
            a.at(0x10356dfc);a.offset(21,21,4);a.offset(16,16,1);
        }
        a.at(0x10356e10);a.offset(19,19,1);a.offset(18,18,0x70);
    }
    a.at(0x10356e28);a.pop(0xf0);a.multiple(13,true);a.finish();
}
}
