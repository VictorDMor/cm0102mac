#include "transfer_valuation_detail.hpp"

namespace cm::aot::transfer_valuation {
using detail::Call;
void rebalance_offer_terms(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    // Both signed magic-multiply sequences divide the profile skill by 500.
    // The finance gate compares against sign extension of a wrapped 32-bit double.
    Call a(s,rebalance_offer_terms_entry,d,o);a.link();a.save(31);a.load(31,2,-0x4b8c);a.save(30);a.move(30,6);a.save(29);a.move(29,4);a.save(28);a.move(28,3);a.save_link();a.frame(112);a.load(4,4,0x14);a.compare(4,0,false);bool eligible=!a.equal();
    if(eligible){a.load<2>(3,4,4,true);a.load<2>(0,4,6,true);a.subtract(0,3,0);a.compare(0,25);eligible=!a.less();}
    if(eligible){a.load<1>(0,28,0x50);a.sign8(0,0,true);eligible=a.equal();}
    if(eligible){a.load(0,28,0x48);a.compare(0,0);eligible=a.equal();}
    if(eligible){a.load(0,28,0x40);a.compare(0,0);eligible=a.equal();if(!eligible)a.jump(0x10417570);}
    if(eligible){a.at(0x10417364);a.load(3,2,-0x7ab8);a.move(4,30);a.load(3,3,0);a.call(0x10103510);a.value(4,0);a.value(5,0);a.value(6,0);a.value(7,1);a.value(8,0);a.call(0x100f5770);a.load(0,28,0x20);a.exclusive_flags(5,3,0x80000000);a.mask(7,0,0xfffffffe,false,1);a.shift_right(3,7,31);a.exclusive_flags(6,3,0x80000000);a.subtract_carry_at(0x104173a0,3,4,7);a.subtract_extended(5,5,6);a.subtract_extended(5,6,6);a.negate_record_at(0x104173ac,5,5);eligible=a.equal();}
    if(eligible){a.load<2>(6,30,0x1ec,true);a.compare(6,15);bool first=!a.at_least();
        if(first){a.load(4,29,0x14);a.value(3,0x10620000);a.offset(5,3,0x4dd3);a.load<2>(3,4,10,true);a.multiply_high(3,5,3);a.shift_right(3,3,5);a.mask(4,3,1,false,1);a.add(3,3,4);a.subtract(3,6,3);a.compare(3,2);first=!a.nonpositive();}
        if(first){a.exclusive_flags(0,0,0x80000000);a.value(3,0x43300000);a.store(1,0x54,0);a.value(0,20);a.floating_load(1,31,0xc0);a.store(1,0x50,3);a.floating_load(2,31,0x30);a.floating_load(0,1,0x50);a.floating_subtract(0,0,1);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0x48);a.load(3,1,0x4c);a.store(28,0x20,3);a.store<1>(28,0x50,0);a.jump(0x10417570);}
        else{a.at(0x1041742c);a.load(3,29,12);a.compare(3,0,false);bool second=!a.equal();if(second){a.load<2>(3,3,0xac,true);a.compare(3,5000);second=!a.nonpositive();}if(second){a.load(3,29,0x14);a.compare(3,0,false);second=!a.equal();}if(second){a.load<2>(3,3,10,true);a.compare(3,4250);second=!a.nonpositive();}if(second){a.load<1>(3,29,0x5e);a.compare(3,0,false);second=a.equal();}
            if(second){a.value(3,0x43300000);a.exclusive_flags(0,0,0x80000000);a.store(1,0x4c,0);a.value(0,10);a.floating_load(2,31,0xc0);a.store(1,0x48,3);a.floating_load(3,31,0xd8);a.floating_load(0,1,0x48);a.store(1,0x40,3);a.floating_subtract(0,0,2);a.floating_load(1,31,0x30);a.floating_product(0,3,0);a.floating_integer(0,0);a.floating_store(0,1,0x50);a.load(3,1,0x54);a.store(28,0x48,3);a.store<1>(28,0x4c,0);a.load(0,28,0x20);a.exclusive_flags(0,0,0x80000000);a.store(1,0x44,0);a.floating_load(0,1,0x40);a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x38);a.load(0,1,0x3c);a.store(28,0x20,0);a.jump(0x10417570);}
            else{a.at(0x104174d8);a.load(4,29,0x14);a.value(3,0x10620000);a.offset(5,3,0x4dd3);a.load<2>(3,4,10,true);a.multiply_high(3,5,3);a.shift_right(3,3,5);a.mask(4,3,1,false,1);a.add(3,3,4);a.subtract(3,6,3);a.compare(3,-3);
                if(!a.at_least()){a.value(3,0x43300000);a.exclusive_flags(0,0,0x80000000);a.store(1,0x3c,0);a.value(0,20);a.floating_load(2,31,0xc0);a.store(1,0x38,3);a.floating_load(3,31,0xd8);a.floating_load(0,1,0x38);a.store(1,0x48,3);a.floating_subtract(0,0,2);a.floating_load(1,31,0x30);a.floating_product(0,3,0);a.floating_integer(0,0);a.floating_store(0,1,0x40);a.load(3,1,0x44);a.store(28,0x40,3);a.store<1>(28,0x44,0);a.load(0,28,0x20);a.exclusive_flags(0,0,0x80000000);a.store(1,0x4c,0);a.floating_load(0,1,0x48);a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x50);a.load(0,1,0x54);a.store(28,0x20,0);}
            }
        }
    }
    a.at(0x10417570);a.pop(112);a.restore(31);a.restore(30);a.restore(29);a.restore(28);a.finish();
}
}
