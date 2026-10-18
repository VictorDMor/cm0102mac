#include "transfer_synthesis_detail.hpp"

namespace cm::aot::transfer_synthesis::detail {
namespace {
void priority_contract_club(Call& a,std::uint32_t pc,unsigned result){
    a.at(pc);a.load(3,27,0);a.move(4,16);a.call(0x10082f20);a.load(4,2,-0x7b78);a.compare(3,0,false);a.store(4,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(result,3,0);a.jump(pc+0x38);}else{a.at(pc+0x34);a.value(result,0);}
    a.at(pc+0x38);
}
void select_priority_flag(Call& a,std::uint32_t pc,bool copied){
    const auto clear=pc+(copied?0x17c:0x180),join=clear+8;
    a.at(pc);a.load(0,16,0x14);a.compare(0,0,false);bool eligible=!a.equal();
    if(eligible){a.compare(18,0,false);eligible=!a.equal();}if(eligible){a.load<1>(0,18,0x28);a.mask(0,0,15,false,29);a.compare(0,4,false);eligible=!a.at_least();}
    if(eligible){advance_pool(a,pc+0x24);a.load(4,31,0);a.value(3,0x05760000);a.load(5,16,0x14);a.offset(0,3,0x19f1);a.load<2>(4,4,0);a.load(3,28,0);a.load<2>(5,5,10,true);a.add(4,4,3);a.multiply_high(0,0,4);a.shift_right(0,0,6);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,3000);a.subtract(3,0,4);a.offset(0,3,4000);a.compare_registers(5,0);eligible=!a.nonpositive();}
    if(eligible){priority_contract_club(a,pc+0x90,0);a.compare(0,0,false);eligible=!a.equal();}
    if(eligible){priority_contract_club(a,pc+0xd0,3);a.load(3,3,0x30);a.load(0,17,0x30);a.compare_registers(3,0,false);eligible=a.equal();}
    if(eligible){advance_pool(a,pc+0x118);a.load(5,31,0);a.value(3,copied?0x55550000:0x66660000);a.load(4,28,0);a.offset(0,3,copied?0x5556:0x6667);a.load<2>(3,5,0);a.add(4,3,4);
        if(copied){a.multiply_high(3,0,4);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);}else{a.multiply_high(0,0,4);a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,20);}
        a.subtract(0,0,4,true);eligible=a.equal();}
    if(eligible){a.value(0,1);a.store<1>(15,0x35,0);a.jump(join);}else{a.at(clear);a.value(0,0);a.store<1>(15,0x35,0);}
    a.at(join);
}
template<unsigned Size=4>void copy_field(Call& a,unsigned offset,bool sign=false){a.load<Size>(0,13,offset,sign);a.store<Size>(15,offset,0);}
void copy_offer_fields(Call& a){
    a.at(0x10412a38);copy_field(a,0);copy_field(a,4);copy_field(a,8);copy_field(a,12);copy_field<1>(a,0x10);copy_field<1>(a,0x11);copy_field<2>(a,0x16);copy_field<2>(a,0x14,true);copy_field<1>(a,0x18);copy_field<1>(a,0x1c);copy_field(a,0x20);copy_field<1>(a,0x24);copy_field(a,0x28);copy_field(a,0x2c);copy_field(a,0x30);copy_field<1>(a,0x34);copy_field<1>(a,0x35);copy_field<1>(a,0x36);copy_field<1>(a,0x37);copy_field<1>(a,0x38);copy_field(a,0x3c);copy_field(a,0x40);copy_field<1>(a,0x44);copy_field(a,0x48);copy_field<1>(a,0x4c);copy_field<1>(a,0x50);copy_field<1>(a,0x51);
}
void retain_eligible_exchange(Call& a){
    a.at(0x10412c94);a.value(4,0);a.value(0,0xffffffff);a.store<1>(15,0x10,4);a.store<1>(15,0x34,4);a.store<1>(15,0x36,4);a.load<1>(3,15,0x3c);a.insert_mask(3,4,0xf0,4);a.store<1>(15,0x3c,3);a.load<1>(3,15,0x38);a.insert_mask(3,4,0xf0,4);a.store<1>(15,0x38,3);a.store(15,0x30,0);a.store(15,0x2c,0);a.store<1>(15,0x37,4);a.store(15,0,4);a.load(0,15,4);a.compare(0,0,false);
    if(a.equal())return;
    a.offset(3,1,0x5c);a.call(0x10738c00);a.value(3,16);a.call(0x10773520);a.move(14,3,true);if(!a.equal())a.call(0x10738c00);
    a.at(0x10412cf8);a.store(15,4,14);a.load(3,13,4);a.load(0,3,4);a.store(3,12,0);a.jump(0x10412e80);
    for(;;){a.at(0x10412e80);a.load(3,13,4);a.call(0x10738e00);a.move(18,3,true);if(a.equal())break;
        a.at(0x10412d0c);a.load(0,18,0);a.load(4,22,0);a.multiply(0,0,0x70);a.load(3,27,0);a.add(14,4,0);a.move(4,14);a.call(0x10082f20);a.load(4,2,-0x7b78);a.compare(3,0,false);a.store(4,0,3);
        if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(0,3,0);a.jump(0x10412d54);}else{a.at(0x10412d50);a.value(0,0);}
        a.at(0x10412d54);a.compare_registers(0,17,false);if(!a.equal())continue;
        a.load(3,27,0);a.move(4,14);a.call(0x10083110);a.compare(3,0,false);if(!a.equal())continue;
        a.load(3,27,0);a.move(4,14);a.call(0x10082f20);a.load(4,2,-0x7b78);a.compare(3,0,false);a.store(4,0,3);
        if(!a.equal()){a.load<1>(0,3,0x38);a.mask(0,0,0xfe000000,false,24);a.shift_right(0,0,26);a.sign8(0,0);a.subtract_from(0,0,5);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.jump(0x10412db0);}else{a.at(0x10412dac);a.value(0,0);}
        a.at(0x10412db0);a.compare(0,0);if(!a.equal())continue;
        a.load<1>(0,14,0x62);a.compare(0,11);if(!a.equal())continue;
        a.load(3,26,0);a.move(4,14);a.value(5,0);a.value(6,0);a.call(0x103eaea0);a.compare(3,0);bool blocked=!a.equal();
        if(!blocked){a.load(3,26,0);a.move(4,14);a.offset(5,1,0x5c);a.call(0x103eb020);a.compare(3,0);blocked=!a.equal();}
        if(blocked){a.at(0x10412df8);a.load<2>(0,1,0x5c,true);a.value(14,0);a.compare(0,1);
            if(a.equal()){a.load(0,1,0x60);a.offset(3,1,0x5c);a.store(1,0x68,0);a.call(0x10738e00);a.move(4,3);a.load(3,26,0);a.load(4,4,0);a.call(0x103eb440);a.load(3,3,0);a.load(4,16,0);a.load(0,3,12);a.compare_registers(4,0);if(a.equal())a.value(14,1);}
            a.at(0x10412e40);a.sign8(0,14,true);if(a.equal())continue;}
        a.at(0x10412e48);a.load(3,18,0);a.value(0,0);a.offset(4,1,0x4c);a.value(5,8);a.store(1,0x4c,3);a.load<1>(3,18,4);a.store<1>(1,0x50,3);a.load<1>(3,18,5);a.store<1>(1,0x51,3);a.load<1>(3,18,6);a.store<1>(1,0x52,3);a.store<1>(1,0x50,0);a.load(3,15,4);a.call(0x10738d10);
    }
    a.at(0x10412e90);a.load(3,15,4);a.load<2>(0,3,0,true);a.sign16(0,0,true);if(a.equal()){a.value(4,1);a.call(0x10738c20);a.value(0,0);a.store(15,4,0);}
    a.at(0x10412eb0);a.offset(3,1,0x5c);a.value(4,0xffffffff);a.call(0x10738c20);a.jump(0x10415b2c);
}
}
Route prepare_offer(Call& a){
    bool valid=!a.equal();if(valid){a.compare(17,0,false);valid=!a.equal();}
    if(!valid){diagnostic(a,0x10412748,13,23,0x6c,0x54,0x154);a.load(4,2,-0x7bac);a.value(0,0);a.value(3,0);a.store<1>(4,0,0);a.jump(0x10415e34);return Route::failed;}
    select_priority_flag(a,0x104127a4,false);
    a.value(0,0);a.move(4,16);a.store(15,0,0);a.store(15,4,0);a.load(0,17,0);a.store(15,8,0);a.load(0,16,0);a.store(15,12,0);a.load(3,27,0);a.call(0x10082f20);a.compare(18,0,false);a.store<1>(15,0x37,21);a.move(21,3);
    if(!a.equal()){a.load<1>(0,18,0);a.sign8(0,0);a.jump(0x10412974);}else{a.at(0x10412970);a.value(0,11);}
    a.at(0x10412974);a.store<1>(15,0x11,0);a.value(5,0);a.value(0,0);a.value(4,10);a.store<1>(15,0x24,5);a.value(3,0xffffffff);a.compare(20,0,false);a.store(15,0x40,5);a.store<1>(15,0x44,4);a.store(15,0x48,5);a.store<1>(15,0x4c,4);a.store<1>(15,0x50,5);a.store<1>(15,0x51,5);a.store(15,0x30,3);a.store(15,0x2c,3);a.store<1>(15,0x34,5);a.store<1>(15,0x10,5);a.store<1>(15,0x36,5);a.store<1>(1,0xbc,0);a.load<1>(0,15,0x3c);a.insert_mask(0,5,0xf0,4);a.store<1>(15,0x3c,0);a.load<1>(0,15,0x38);a.insert_mask(0,5,0xf0,4);a.store<1>(15,0x38,0);a.store(15,0x20,5);if(!a.equal())a.store(20,0,5);
    a.at(0x104129e4);a.move(3,17);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);if(a.equal())return Route::price;
    a.load(3,26,0);a.move(4,16);a.move(5,17);a.value(6,1);a.call(0x103f5d20);a.move(13,3,true);bool copy=!a.equal();if(copy){a.load<1>(0,13,0x24);a.sign8(0,0,true);copy=a.equal();}if(copy){a.load<1>(0,15,0x37);a.sign8(0,0,true);copy=a.equal();}
    if(copy){copy_offer_fields(a);select_priority_flag(a,0x10412b10,true);retain_eligible_exchange(a);return Route::finalize;}
    a.at(0x10412ec0);a.load(0,16,8);a.move(4,17);a.load(3,2,-0x7ab8);a.store(15,0x20,0);a.load(3,3,0);a.call(0x10103510);a.move(4,16);a.value(5,0);a.value(6,0);a.value(7,1);a.value(8,0);a.call(0x100f5770);a.load(0,15,0x20);a.compare_registers(0,4);if(!a.nonpositive()){a.store(15,0x20,4);a.jump(0x10415b2c);}a.at(0x10415b2c);return Route::finalize;
}
}
