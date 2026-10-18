#include "transfer_synthesis_terms.hpp"

namespace cm::aot::transfer_synthesis::detail {
namespace {
void finish_cursor_advance(Call& a){if(!a.nonpositive()){a.store(31,0,29);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(28,0,0);}}

bool mandatory_components(Call& a){
    a.at(0x1041389c);a.compare(18,0,false);bool eligible=!a.equal();if(eligible){a.load<1>(0,18,0);a.compare(0,11);eligible=a.equal();}
    if(eligible){a.load<1>(0,18,0x28);a.mask(0,0,15,false,29);a.compare(0,5,false);eligible=a.equal();if(!eligible){a.compare(0,4,false);bool young=a.equal();if(young){a.load<2>(0,16,0x5c,true);a.sign8(0,0);a.compare(0,28);young=a.less();}if(young)eligible=true;else{a.at(0x104138d8);a.compare(14,0);eligible=!a.equal();}}}
    if(!eligible){a.at(0x10414174);a.value(3,0);a.jump(0x10415e34);return false;}
    a.at(0x104138e0);a.load(3,16,0xc);a.value(0,1);a.store<1>(1,0xbc,0);a.load<2>(5,3,0xac,true);a.compare(5,5000);bool established=!a.nonpositive();if(established){a.load<1>(0,16,0x5e);a.compare(0,0,false);established=a.equal();}
    if(established){
        a.load(4,16,0x14);a.value(3,0x43300000);a.exclusive_flags(0,5,0x80000000);a.store(1,0x8c,3);a.load<2>(4,4,8,true);a.floating_load(2,25,0xc0);a.exclusive_flags(4,4,0x80000000);a.floating_load(0,25,0x88);a.store(1,0x90,4);a.floating_load(1,1,0x8c);a.store(1,0x98,0);a.floating_subtract(1,1,2);a.store(1,0x94,3);a.floating_difference(1,1,0,true);a.floating_load(0,1,0x94);a.floating_subtract(0,0,2);a.floating_compare(1,0);bool contingent=a.positive();
        if(!contingent){a.load(3,16,0x20);a.compare(3,0,false);if(!a.equal()){a.call(0x100ce250);a.sign8(0,3,true);contingent=!a.equal();}}
        if(contingent){
            a.at(0x10413964);a.load(4,15,0x20);a.value(3,0x43300000);a.exclusive_flags(0,14,0x80000000);a.store(1,0x8c,3);a.exclusive_flags(4,4,0x80000000);a.floating_load(3,25,0xc0);a.store(1,0x90,4);a.floating_load(4,25,0x18);a.floating_load(0,1,0x8c);a.store(1,0x98,0);a.floating_subtract(2,0,3);a.floating_load(1,25,0x90);a.store(1,0x94,3);a.floating_load(0,1,0x94);a.floating_product(2,4,2);a.floating_subtract(0,0,3);a.floating_product(0,1,0);a.floating_compare(2,0);if(!a.nonpositive())a.jump(0x104139b8);else{a.at(0x104139b4);a.floating_move(2,0);}
            a.at(0x104139b8);a.floating_integer(0,2);a.value(0,10);a.floating_store(0,1,0x9c);a.load(3,1,0xa0);a.store(15,0x48,3);a.store<1>(15,0x4c,0);
            advance_pool(a,0x104139d0);choose_ordinary_period(a,0x104139f8,true,3);advance_pool(a,0x10413a30);calculate_ordinary_amount(a,0x10413a58,5,false);advance_pool(a,0x10413b04);choose_percentage_pair(a,0x10413b2c,false,true,true,0x10414ec8);
        }else{advance_pool(a,0x10413c70);choose_ordinary_period(a,0x10413c98,false,1);advance_pool(a,0x10413cd4);calculate_ordinary_amount(a,0x10413cfc,10,false);advance_pool(a,0x10413da8);choose_percentage_pair(a,0x10413dd0,false,true,true,0x10414ec8);}
    }else{advance_pool(a,0x10413f14);choose_ordinary_period(a,0x10413f3c,true,3);advance_pool(a,0x10413f74);calculate_ordinary_amount(a,0x10413f9c,5,false);advance_pool(a,0x10414048);choose_percentage_pair(a,0x10414070,true,true,false,0x10414ec8);}
    return true;
}

void choose_optional_contingent(Call& a){
    a.at(0x104141d4);a.load<2>(5,3,0xac,true);a.compare(5,5000);bool eligible=!a.nonpositive();if(eligible){a.load<1>(0,16,0x5e);a.compare(0,5,false);eligible=!a.at_least();}
    if(eligible){
        a.load(3,16,0x14);a.exclusive_flags(0,5,0x80000000);a.store(1,0x8c,4);a.load<2>(3,3,8,true);a.floating_load(1,25,0x88);a.exclusive_flags(3,3,0x80000000);a.store(1,0x98,0);a.store(1,0x90,3);a.floating_load(0,1,0x8c);a.store(1,0x94,4);a.floating_subtract(2,0,3);a.floating_load(0,1,0x94);a.floating_difference(1,2,1,true);a.floating_subtract(0,0,3);a.floating_compare(1,0);bool suitable=a.positive();
        if(!suitable){a.load(3,16,0x20);a.compare(3,0,false);if(!a.equal()){a.call(0x100ce250);a.sign8(0,3,true);suitable=!a.equal();}}
        eligible=suitable;
    }
    if(!eligible)return;
    advance_pool(a,0x10414244);a.load(3,31,0);a.load<2>(5,16,0x5c,true);a.load<2>(4,3,0);a.load(0,28,0);a.sign8(5,5);a.add(4,4,0);a.mask(0,4,0xc0000000,false,30);a.mask(4,4,1,false,1);a.subtract(0,4,0);a.mask(0,0,0xffffffff,false,2);a.add(4,0,4);a.offset(0,4,25);a.compare_registers(5,0);if(a.at_least())return;
    a.load<1>(4,16,0x5e);a.value(0,1);a.store<1>(1,0xbc,0);a.compare(4,0,false);bool short_condition=a.equal();
    if(short_condition){
        a.load(0,30,0);a.offset(3,3,2);a.store(31,0,3);a.compare_registers(3,0,false);finish_cursor_advance(a);a.at(0x104142dc);a.load(3,31,0);a.value(4,0x66660000);a.load(5,28,0);a.offset(0,4,0x6667);a.load<2>(4,3,0);a.add(5,4,5);a.multiply_high(0,0,5);a.shift_right(0,0,1);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,5);a.subtract(0,0,5,true);short_condition=a.equal();
    }
    if(short_condition){
        a.load(0,30,0);a.offset(3,3,2);a.store(31,0,3);a.compare_registers(3,0,false);finish_cursor_advance(a);
        a.at(0x10414334);a.load(4,31,0);a.value(6,0x43300000);a.value(3,0x2aab0000);a.load(0,15,0x20);a.load<2>(5,4,0);a.offset(3,3,-0x5555);a.load(4,28,0);a.exclusive_flags(0,0,0x80000000);a.store(1,0x90,0);a.value(0,1);a.add(5,5,4);a.floating_load(3,25,0xc0);a.multiply_high(4,3,5);a.store(1,0x8c,6);a.floating_load(0,25,0x98);a.floating_load(1,1,0x8c);a.store(1,0x94,6);a.floating_subtract(2,1,3);a.mask(3,4,1,false,1);a.add(3,4,3);a.multiply(3,3,6);a.subtract(3,3,5);a.offset(3,3,5);a.exclusive_flags(3,3,0x80000000);a.store(1,0x98,3);a.floating_load(1,1,0x94);a.floating_subtract(1,1,3);a.floating_product(1,2,1);a.floating_difference(0,1,0,true);a.floating_integer(0,0);a.floating_store(0,1,0x9c);a.load(3,1,0xa0);a.store(15,0x48,3);a.store<1>(15,0x4c,0);a.jump(0x10414474);
    }else{
        advance_pool(a,0x104143c0);a.load(4,31,0);a.value(6,0x43300000);a.value(3,0x2e8c0000);a.load(0,15,0x20);a.load<2>(5,4,0);a.offset(3,3,-0x5d17);a.load(4,28,0);a.exclusive_flags(0,0,0x80000000);a.store(1,0x90,0);a.value(0,10);a.add(5,5,4);a.floating_load(3,25,0xc0);a.multiply_high(3,3,5);a.store(1,0x8c,6);a.floating_load(0,25,0x98);a.floating_load(1,1,0x8c);a.store(1,0x94,6);a.floating_subtract(2,1,3);a.shift_right(3,3,1);a.mask(4,3,1,false,1);a.add(3,3,4);a.multiply(3,3,11);a.subtract(3,3,5);a.offset(3,3,5);a.exclusive_flags(3,3,0x80000000);a.store(1,0x98,3);a.floating_load(1,1,0x94);a.floating_subtract(1,1,3);a.floating_product(1,2,1);a.floating_difference(0,1,0,true);a.floating_integer(0,0);a.floating_store(0,1,0x9c);a.load(3,1,0xa0);a.store(15,0x48,3);a.store<1>(15,0x4c,0);
    }
}

void sample_eighth_of_base(Call& a,std::uint32_t pc){
    a.at(pc);a.load(0,15,0x20);a.shift_right(0,0,3);a.add_carry(14,0,true);if(!a.equal()){advance_pool(a,pc+0x18);sample_payment_range(a,pc+0x40);}else{a.value(13,0);a.jump(pc+0x160);}
    a.at(pc+0x160);
}

void choose_optional_ordinary(Call& a){
    sample_eighth_of_base(a,0x10414474);a.load(0,15,0x48);a.compare_registers(0,13);bool eligible=!a.at_least();if(eligible){a.compare(18,0,false);eligible=!a.equal();}if(eligible){a.load<1>(0,18,0x28);a.mask(0,0,15,false,29);a.compare(0,5,false);eligible=a.equal();}if(!eligible)return;
    advance_pool(a,0x104145f8);a.load(4,31,0);a.load(0,28,0);a.load<2>(3,4,0);a.add(0,3,0);a.mask(3,0,1,false,1);a.mask(0,0,1);a.exclusive_registers_at(0x10414638,0,0,3);a.subtract(0,3,0,true);if(a.equal())return;
    a.load(3,30,0);a.offset(4,4,2);a.value(0,1);a.store(31,0,4);a.compare_registers(4,3,false);a.store<1>(1,0xbc,0);finish_cursor_advance(a);choose_ordinary_period(a,0x10414670,false,1);advance_pool(a,0x104146ac);calculate_ordinary_amount(a,0x104146d4,5,true);
}

void choose_optional_percentage(Call& a){
    sample_eighth_of_base(a,0x10414780);a.load(3,15,0x48);a.load(0,15,0x40);a.add(0,3,0);a.compare_registers(0,13);bool eligible=!a.at_least();if(eligible){a.compare(18,0,false);eligible=!a.equal();}if(!eligible)return;
    a.load<1>(0,18,0x28);a.mask(0,0,15,false,29);a.compare(0,5,false);bool tagged=a.equal();
    if(!tagged){advance_pool(a,0x1041490c);a.load(4,31,0);a.value(3,0x66660000);a.load<2>(0,16,0x5c,true);a.offset(3,3,0x6667);a.load<2>(5,4,0);a.load(4,28,0);a.sign8(0,0);a.add(5,5,4);a.multiply_high(3,3,5);a.shift_right(3,3,3);a.mask(4,3,1,false,1);a.add(3,3,4);a.multiply(3,3,20);a.subtract(3,3,5);a.subtract_from(3,3,28);a.compare_registers(0,3);if(a.at_least())return;}
    a.at(0x10414978);a.load(3,31,0);a.value(0,1);a.store<1>(1,0xbc,0);a.offset(3,3,2);a.load(0,30,0);a.store(31,0,3);a.compare_registers(3,0,false);finish_cursor_advance(a);choose_percentage_pair(a,0x104149a8,true,false,false,0);
}

void ensure_remaining_component(Call& a){
    a.at(0x10414a54);a.load(0,15,0x48);a.compare(0,0);if(!a.equal())return;a.load(0,15,0x40);a.compare(0,0);if(!a.equal())return;a.load<1>(0,15,0x50);a.sign8(0,0,true);if(!a.equal())return;a.compare(18,0,false);if(a.equal())return;
    a.load(14,18,0xc);a.compare(14,0);if(!a.equal()){advance_pool(a,0x10414a98);sample_source_component(a);}else{a.value(0,0);a.store(1,0xc8,0);a.jump(0x10414bf0);}
    a.at(0x10414bf0);a.load(3,18,4);a.value(0,0x43300000);a.store(1,0x8c,0);a.exclusive_flags(0,3,0x80000000);a.floating_load(1,25,0xc0);a.store(1,0x90,0);a.floating_load(2,25,0xa8);a.floating_load(0,1,0x8c);a.floating_subtract(0,0,1);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0x94);a.load(14,1,0x98);a.compare(14,0);if(!a.equal()){advance_pool(a,0x10414c34);sample_payment_range(a,0x10414c5c);}else{a.value(13,0);a.jump(0x10414d7c);}
    a.at(0x10414d7c);a.load(0,1,0xc8);a.compare_registers(0,13);if(a.nonpositive())return;
    a.load(3,31,0);a.value(0,1);a.store<1>(1,0xbc,0);a.offset(3,3,2);a.load(0,30,0);a.store(31,0,3);a.compare_registers(3,0,false);finish_cursor_advance(a);choose_ordinary_period(a,0x10414db8,false,1);advance_pool(a,0x10414df4);calculate_ordinary_amount(a,0x10414e1c,5,true);
}

void optional_components(Call& a){
    a.at(0x1041417c);a.value(3,0x20000);a.load(4,15,0x20);a.offset(0,3,-0x7960);a.compare_registers(4,0);if(a.less())return;
    a.exclusive_flags(0,22,0x80000000);a.value(4,0x43300000);a.store(1,0x90,0);a.floating_load(3,25,0xc0);a.store(1,0x8c,4);a.floating_load(0,1,0x8c);a.floating_subtract(0,0,3);a.floating_compare(1,0);if(a.at_least())return;a.compare(18,0,false);if(a.equal())return;a.load<1>(0,18,0);a.compare(0,11);if(!a.equal())return;a.load(3,16,0xc);a.compare(3,0,false);if(a.equal())return;
    choose_optional_contingent(a);choose_optional_ordinary(a);choose_optional_percentage(a);ensure_remaining_component(a);
}
}

bool constrain_payment_distribution(Call& a){
    a.at(0x104137d0);a.compare(19,-1);if(a.nonpositive())return true;a.load(3,15,0x20);a.compare_registers(19,3);if(a.at_least())return true;a.load(0,1,0xc0);a.compare(0,-1);
    if(!a.nonpositive()){
        a.compare_registers(0,3);if(!a.at_least())a.move(3,0);a.at(0x104137fc);a.store(15,0x20,3);a.value(0,0);a.store<1>(15,0x51,0);a.load(0,15,0x20);a.compare_registers(0,19);if(!a.nonpositive()){a.store(15,0x20,19);a.jump(0x10414ec8);}return true;
    }
    a.at(0x1041381c);a.store(15,0x20,19);a.value(3,0x43300000);a.exclusive_flags(0,22,0x80000000);a.floating_load(3,25,0xc0);a.load(4,15,0x20);a.store(1,0x8c,3);a.exclusive_flags(4,4,0x80000000);a.floating_load(2,25,0x78);a.store(1,0x90,4);a.floating_load(0,1,0x8c);a.store(1,0x98,0);a.floating_subtract(1,0,3);a.store(1,0x94,3);a.floating_load(0,1,0x94);a.floating_product(1,2,1);a.floating_subtract(0,0,3);a.floating_compare(1,0);bool mandatory=a.less();
    if(!mandatory){
        a.store(1,0x90,4);a.floating_load(2,25,0x80);a.store(1,0x8c,3);a.floating_load(0,1,0x8c);a.store(1,0x98,0);a.floating_subtract(1,0,3);a.store(1,0x94,3);a.floating_load(0,1,0x94);a.floating_product(1,2,1);a.floating_subtract(0,0,3);a.floating_compare(1,0);bool optional=a.at_least();if(!optional){a.compare(14,0);optional=a.equal();}mandatory=!optional;
    }
    if(mandatory)return mandatory_components(a);
    optional_components(a);return true;
}
}
