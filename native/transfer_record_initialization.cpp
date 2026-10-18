#include "transfer_record_detail.hpp"

namespace cm::aot::transfer_records {
using detail::Call;
void initialize_zero_fee_offer(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,initialize_zero_fee_offer_entry,d,o);a.link();a.save(31);a.compare(4,0,false);a.load(31,2,-0x4b88);a.save(30);a.load(30,2,-0x7ba8);a.save_link();a.frame(112);
    bool valid=!a.equal();if(valid){a.compare(5,0,false);valid=!a.equal();}
    if(!valid){detail::diagnostic(a,0x104163fc,31,30,0x40,0x38,0x485);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);a.jump(0x104164cc);}
    else {a.at(0x10416454);a.value(9,0);a.value(8,11);a.store<1>(3,0x35,9);a.value(7,1);a.value(6,10);a.value(0,0xffffffff);a.store(3,0,9);a.store<1>(3,0x11,8);a.store(3,4,9);a.load(5,5,0);a.store(3,8,5);a.load(4,4,0);a.store(3,12,4);a.store(3,0x20,9);a.store<1>(3,0x24,7);a.store(3,0x40,9);a.store<1>(3,0x44,6);a.store(3,0x48,9);a.store<1>(3,0x4c,6);a.store<1>(3,0x50,9);a.store<1>(3,0x51,9);a.store(3,0x30,0);a.store(3,0x2c,0);a.store<1>(3,0x34,9);a.store<1>(3,0x10,9);a.store<1>(3,0x36,9);a.store<1>(3,0x37,9);a.load<1>(0,3,0x38);a.insert_mask(0,9,0xf0,4);a.store<1>(3,0x38,0);}
    a.at(0x104164cc);a.pop(112);a.restore(31);a.restore(30);a.finish();
}
void initialize_priced_offer(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,initialize_priced_offer_entry,d,o);a.link();a.multiple(21);a.move(27,4,true);a.load(21,2,-0x7b7c);a.load(22,2,-0x7b78);a.move(26,3);a.load(23,2,-0x7be8);a.move(28,5);a.load(30,2,-0x7b90);a.move(29,6);a.load(31,2,-0x7b84);a.load(24,2,-0x7ba8);a.load(25,2,-0x4b88);a.save_link();a.frame(144);
    bool valid=!a.equal();if(valid){a.compare(28,0,false);valid=!a.equal();}
    if(!valid){detail::diagnostic(a,0x10416538,25,24,0x40,0x38,0x4b1);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);a.jump(0x104166fc);}
    else {
        a.at(0x10416590);a.load(3,21,0);a.call(0x10082f20);a.compare(3,0,false);a.store(22,0,3);
        if(!a.equal()){a.load(0,3,4);a.load(3,23,0);a.multiply(0,0,0x248);a.add(0,3,0);a.jump(0x104165bc);}else{a.at(0x104165b8);a.value(0,0);}
        a.at(0x104165bc);a.compare(0,0,false);bool same=!a.equal();
        if(same){a.load(3,21,0);a.move(4,27);a.call(0x10082f20);a.compare(3,0,false);a.store(22,0,3);
            if(!a.equal()){a.load(0,3,4);a.load(3,23,0);a.multiply(0,0,0x248);a.add(3,3,0);a.jump(0x104165f4);}else{a.at(0x104165f0);a.value(3,0);}
            a.at(0x104165f4);a.load(3,3,0x30);a.load(0,28,0x30);a.compare_registers(3,0,false);same=a.equal();}
        bool marked=false;
        if(same){a.at(0x10416604);a.load(3,2,-0x7b88);a.load(4,31,0);a.load(0,3,0);a.offset(3,4,2);a.compare_registers(3,0,false);a.store(31,0,3);
            if(!a.nonpositive()){a.load(0,2,-0x7b8c);a.store(31,0,0);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(30,0,0);}
            a.at(0x10416634);a.load(5,31,0);a.value(3,0x66660000);a.load(4,30,0);a.offset(0,3,0x6667);a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(0,0,4);a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,20);a.subtract(0,0,4,true);
            if(a.equal()){a.load(3,27,0x14);a.load<2>(0,3,10,true);a.compare(0,7000);if(!a.nonpositive()){a.value(0,1);a.store<1>(26,0x35,0);a.jump(0x1041668c);marked=true;}}
        }
        if(!marked){a.at(0x10416684);a.value(0,0);a.store<1>(26,0x35,0);}
        a.at(0x1041668c);a.value(6,0);a.value(4,11);a.store(26,0,6);a.value(3,10);a.value(0,0xffffffff);a.store(26,4,6);a.load(5,28,0);a.store(26,8,5);a.load(5,27,0);a.store(26,12,5);a.store<1>(26,0x11,4);a.store(26,0x20,29);a.store<1>(26,0x24,6);a.store(26,0x40,6);a.store<1>(26,0x44,3);a.store(26,0x48,6);a.store<1>(26,0x4c,3);a.store<1>(26,0x50,6);a.store<1>(26,0x51,6);a.store(26,0x30,0);a.store(26,0x2c,0);a.store<1>(26,0x34,6);a.store<1>(26,0x10,6);a.store<1>(26,0x36,6);a.store<1>(26,0x37,6);a.load<1>(0,26,0x38);a.insert_mask(0,6,0xf0,4);a.store<1>(26,0x38,0);
    }
    a.at(0x104166fc);a.pop(144);a.multiple(21,true);a.finish();
}
}
