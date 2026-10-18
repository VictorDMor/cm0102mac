#include "transfer_valuation_detail.hpp"

namespace cm::aot::transfer_valuation {
using detail::Call;
void positive_installment_total(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,positive_installment_total_entry,d,o);a.load(0,3,0x40);a.value(4,0);a.compare(0,0);if(!a.nonpositive())a.move(4,0);a.at(0x10416d44);a.load(0,3,0x48);a.compare(0,0);if(!a.nonpositive())a.add(4,4,0);a.at(0x10416d54);a.move(3,4);a.finish();
}
void adjusted_offer_value(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,adjusted_offer_value_entry,d,o);a.link();a.floating_store(31,1,-8);a.floating_store(30,1,-16);a.multiple(16,false,-16);a.move(22,3);a.load(18,2,-0x4b90);a.move(23,4);a.load(19,2,-0x4b8c);a.move(24,5);a.load(20,2,-0x7be0);a.move(25,6);a.load(21,2,-0x7b7c);a.move(26,7);a.load(29,2,-0x7b78);a.move(27,8);a.floating_load(31,19,0);a.value(16,0);a.floating_load(30,18,0x10,true);a.value(30,0);a.value(17,0);a.save_link();a.frame(176);a.load(0,3,12);a.load(3,20,0);a.multiply(0,0,0x70);a.add(28,3,0);a.load(3,21,0);a.move(4,28);a.call(0x10082f20);a.compare(3,0,false);a.store(29,0,3);
    if(!a.equal()){a.load<1>(0,3,0x38);a.mask(0,0,0xfe000000,false,24);a.shift_right(0,0,26);a.sign8(0,0);a.compare(0,5);if(!a.equal())a.value(17,1);}
    a.at(0x104168dc);a.compare(17,0);
    if(!a.equal()){a.load(3,29,0);a.load(4,2,-0x7be8);a.load(0,3,4);a.load(3,4,0);a.multiply(0,0,0x248);a.add(17,3,0);a.jump(0x10416904);}else{a.at(0x10416900);a.value(17,0);}
    a.at(0x10416904);a.load<1>(0,22,0x11);a.load<2>(3,28,0x5c,true);a.compare(0,11);a.load(31,22,0x20);a.sign8(29,3);
    bool statistics=a.equal();if(statistics){a.load(5,28,0x24);a.compare(5,0,false);statistics=!a.equal();}
    if(statistics){a.load(6,28,0x14);a.move(4,28);a.load(3,5,0x90);a.load<2>(7,6,10,true);a.value(6,2);a.call(0x10219e40);a.sign16(30,3);a.load(5,28,0x24);a.multiply(0,30,50);a.move(4,28);a.load(3,5,0x90);a.value(6,1);a.sign16(7,0);a.call(0x10219e40);a.sign16(16,3);}
    a.at(0x10416964);a.compare(17,0,false);bool adjusted=!a.equal();if(adjusted){a.sign8(0,26,true);adjusted=!a.equal();}if(adjusted){a.load<1>(0,22,0x51);a.sign8(0,0,true);adjusted=!a.nonpositive();}
    if(adjusted){a.load(3,2,-0x7ab8);a.move(4,17);a.load(3,3,0);a.call(0x10103510);a.call(0x100f55b0);a.sign8(0,3,true);
        if(a.equal()){
            a.at(0x104169d4);a.load<1>(0,22,0x51);a.sign8(0,0);a.compare(0,7);
            if(!a.at_least()){a.floating_load(30,18,0x1c,true);a.jump(0x10416a18);}else{a.at(0x104169ec);a.compare(0,13);if(!a.at_least()){a.floating_load(30,18,0x20,true);a.jump(0x10416a18);}else{a.at(0x104169fc);a.floating_load(30,18,0x24,true);a.jump(0x10416a18);}}
        }else if(a.at_least()){
            a.at(0x104169b0);a.compare(0,2);if(!a.at_least()){a.jump(0x10416a04);a.load<1>(0,22,0x51);a.sign8(0,0);a.compare(0,12);if(!a.nonpositive())a.floating_load(30,18,0x1c,true);}
        }else{
            a.at(0x104169a0);a.compare(0,-2);if(a.equal()){a.at(0x104169bc);a.floating_load(30,18,0x14,true);a.floating_load(31,19,0xc8);a.jump(0x10416a18);}else if(a.at_least()){a.at(0x104169c8);a.floating_load(30,18,0x18,true);a.floating_load(31,19,0x18);a.jump(0x10416a18);}else a.jump(0x10416a18);
        }
    }
    a.at(0x10416a18);a.sign8(0,23,true);bool extras=a.equal();if(extras){a.load(0,28,0x14);a.compare(0,0,false);extras=!a.equal();}
    if(extras){a.load(4,22,0x40);a.compare(4,0);
        if(!a.nonpositive()){a.load<1>(0,22,0x44);a.value(3,0x43300000);a.exclusive_flags(4,4,0x80000000);a.store(1,0x50,3);a.sign8(0,0);a.floating_load(4,19,0xc0);a.exclusive_flags(0,0,0x80000000);a.store(1,0x5c,4);a.floating_load(2,19,0xd0);a.store(1,0x54,0);a.floating_load(0,19,0);a.floating_load(1,1,0x50);a.store(1,0x58,3);a.floating_subtract(1,1,4);a.floating_load(3,1,0x58);a.floating_negative_product_difference(0,2,1,0);a.floating_subtract(1,3,4);a.floating_product(0,1,0);a.floating_product(0,31,0);a.floating_integer(0,0);a.floating_store(0,1,0x48);a.load(0,1,0x4c);a.add(31,31,0);}
        a.at(0x10416a94);a.load(0,22,0x48);a.compare(0,0);bool contingent=!a.nonpositive();if(contingent){a.load<1>(0,28,0x5e);a.compare(0,0,false);contingent=a.equal();}if(contingent){a.compare(29,28);contingent=!a.at_least();}if(contingent){a.sign16(0,16);a.load(3,28,12);a.multiply(0,0,65);a.load<2>(3,3,0xac,true);a.compare_registers(0,3);contingent=!a.nonpositive();}
        if(contingent){a.subtract(0,3,0);a.value(4,0x43300000);a.exclusive_flags(3,0,0x80000000);a.offset(0,29,-24);a.exclusive_flags(0,0,0x80000000);a.store(1,0x4c,3);a.load<1>(3,22,0x4c);a.store(1,0x48,4);a.floating_load(7,19,0xc0);a.sign8(3,3);a.floating_load(0,1,0x48);a.exclusive_flags(3,3,0x80000000);a.store(1,0x5c,0);a.floating_subtract(3,0,7);a.floating_load(2,19,0xe0);a.store(1,0x58,4);a.floating_load(1,19,0xe8);a.floating_load(0,1,0x58);a.floating_difference(6,3,2,true);a.floating_load(8,19,0xd8);a.store(1,0x54,3);a.floating_load(5,19,0xd0);a.store(1,0x50,4);a.floating_load(3,19,0);a.floating_subtract(2,0,7);a.floating_load(4,1,0x50);a.floating_sum(8,8,6);a.floating_load(0,19,0x18);a.floating_subtract(4,4,7);a.absolute_at(0x10416b40,6,2);a.floating_negative_product_difference(2,5,4,3);a.floating_difference(1,6,1,true);a.floating_product(8,8,2);a.floating_subtract(1,3,1);a.floating_product(8,8,1);a.floating_compare(8,0);if(!a.nonpositive())a.floating_move(8,0);
            a.at(0x10416b64);a.load(3,22,0x48);a.value(0,0x43300000);a.store(1,0x40,0);a.exclusive_flags(0,3,0x80000000);a.floating_load(1,19,0xc0);a.store(1,0x44,0);a.floating_load(0,1,0x40);a.floating_subtract(0,0,1);a.floating_product(0,0,8);a.floating_product(0,31,0);a.floating_integer(0,0);a.floating_store(0,1,0x38);a.load(0,1,0x3c);a.add(31,31,0);}
        a.at(0x10416b9c);a.load<1>(6,22,0x50);a.sign8(0,6,true);bool percent=!a.nonpositive();if(percent){a.sign8(0,27,true);percent=!a.equal();}
        if(percent){a.sign16(5,16);a.value(4,0x43300000);a.exclusive_flags(0,5,0x80000000);a.sign16(3,30);a.store(1,0x3c,0);a.exclusive_flags(0,3,0x80000000);a.floating_load(3,19,0xc0);a.store(1,0x38,4);a.floating_load(2,19,0x30);a.floating_load(0,1,0x38);a.store(1,0x44,0);a.floating_subtract(1,0,3);a.store(1,0x40,4);a.floating_load(0,1,0x40);a.floating_product(1,2,1);a.floating_subtract(0,0,3);a.floating_compare(1,0);
            if(!a.nonpositive()){a.subtract(0,3,5);a.store(1,0x38,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(1,19,0xf0);a.store(1,0x3c,0);a.sign8(0,6);a.load(3,28,8);a.floating_load(0,1,0x38);a.product(0,3,0);a.store(1,0x40,4);a.floating_subtract(2,0,3);a.floating_load(0,19,0x98);a.floating_difference(2,2,1,true);a.exclusive_flags(0,0,0x80000000);a.store(1,0x44,0);a.floating_load(1,1,0x40);a.floating_subtract(1,1,3);a.floating_product(1,2,1);a.floating_product(1,31,1);a.floating_difference(0,1,0,true);a.floating_integer(0,0);a.floating_store(0,1,0x48);a.load(0,1,0x4c);a.add(31,31,0);}
        }
    }
    a.at(0x10416c58);a.load(3,22,4);a.compare(3,0,false);bool exchange=!a.equal();if(exchange){a.sign8(0,24,true);exchange=a.equal();}
    if(exchange){a.load(0,3,4);a.store(3,12,0);a.jump(0x10416ccc);
        for(;;){a.at(0x10416ccc);a.load(3,22,4);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
            a.at(0x10416c7c);a.sign8(0,25,true);if(!a.equal()){a.load(0,3,0);a.value(5,0);a.load(4,20,0);a.value(6,0);a.multiply(0,0,0x70);a.load(3,21,0);a.value(7,0xffffffff);a.value(8,0);a.add(4,4,0);a.call(0x10084be0);a.add(31,31,3);a.jump(0x10416ccc);}else{a.at(0x10416cb4);a.load(0,3,0);a.load(4,20,0);a.multiply(3,0,0x70);a.offset(0,3,8);a.indexed_load(0,4,0);a.add(31,31,0);}
        }
    }
    a.at(0x10416cdc);a.exclusive_flags(3,31,0x80000000);a.value(0,0x43300000);a.store(1,0x3c,3);a.floating_load(1,19,0xc0);a.store(1,0x38,0);a.floating_load(0,1,0x38);a.floating_subtract(0,0,1,true);a.floating_product_single(0,0,30);a.floating_integer(0,0);a.floating_store(0,1,0x40);a.load(3,1,0x44);a.load(0,1,0xb8);a.offset(1,1,176);a.floating_load(31,1,-8);a.floating_load(30,1,-16);a.restore_link();a.multiple(16,true,-16);a.finish();
}
}
