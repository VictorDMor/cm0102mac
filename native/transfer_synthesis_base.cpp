#include "transfer_synthesis_detail.hpp"

namespace cm::aot::transfer_synthesis::detail {
namespace {
bool try_contract_route(Call& a){
    a.at(0x10412f04);a.load(3,26,0);a.move(4,18);a.move(5,21);a.move(6,17);a.load(3,3,0x988);a.offset(7,1,0x3c);a.call(0x1036fbf0);a.move(13,3);a.sign8(0,13,true);
    bool allow=true;if(!a.equal()){a.load(3,26,0);a.move(4,16);a.call(0x103faff0);a.sign8(0,3,true);if(a.equal()){a.value(0,0);a.jump(0x10412f4c);allow=false;}}
    if(allow){a.at(0x10412f48);a.value(0,1);}
    a.at(0x10412f4c);a.compare(18,0,false);bool possible=!a.equal();if(possible){a.sign8(0,0,true);possible=!a.equal();}
    bool shortcut=false;if(possible){a.compare(21,0,false);shortcut=a.equal();if(!shortcut){a.load<1>(0,18,0);a.compare(0,11);shortcut=!a.equal();if(!shortcut){a.compare(21,0,false);if(!a.equal()){a.load<1>(0,21,0x38);a.mask(0,0,0xfe000000,false,24);a.shift_right(0,0,26);a.sign8(0,0);a.compare(0,1);shortcut=a.equal();if(!shortcut){a.compare(0,3);shortcut=a.equal();}if(!shortcut){a.compare(0,5);shortcut=a.equal();}if(!shortcut){a.sign8(0,13,true);shortcut=!a.equal();}}}}}
    if(!shortcut)return false;
    a.at(0x10412fa8);a.compare(20,0,false);if(a.equal()){a.value(3,0);a.jump(0x10415e34);return true;}
    a.at(0x10412fb8);a.value(3,100);a.call(0x10773520);a.move(13,3,true);if(!a.equal())a.call(0x103721f0);
    a.at(0x10412fcc);a.store(20,0,13);a.move(4,16);a.floating_load(1,25,0);a.move(5,17);a.load(3,27,0);a.value(7,0xffffffff);a.load(6,20,0);a.value(10,2);a.call(0x10080d30);a.sign8(0,3,true);
    if(a.equal()){a.load(3,20,0);a.call(0x107735a0);a.value(3,0);a.jump(0x10415e34);}else{a.at(0x10413008);a.value(0,1);a.store<1>(15,0x37,0);a.jump(0x10415b2c);}
    return true;
}
void resolve_primary_club(Call& a){
    a.at(0x10413014);a.load(3,27,0);a.move(4,16);a.call(0x10082f20);a.load(4,2,-0x7b78);a.compare(3,0,false);a.store(4,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(24,3,0);a.jump(0x1041304c);}else{a.at(0x10413048);a.value(24,0);}
}
void select_initial_values(Call& a){
    a.at(0x10413074);a.compare(18,0,false);bool priced=!a.equal();if(priced){a.load<1>(0,18,0);a.compare(0,11);priced=a.equal();}if(priced){a.load(0,16,0x14);a.compare(0,0,false);priced=!a.equal();}
    if(!priced){a.at(0x10413094);a.value(22,0);a.jump(0x10413170);}
    else{
        a.at(0x1041309c);a.compare(24,0,false);bool special=!a.equal();if(special){a.move(3,24);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);special=!a.equal();}
        if(special){a.load(3,27,0);a.move(4,24);a.move(5,17);a.move(6,16);a.call(0x10089a40);a.move(0,3);a.move(3,16);a.move(22,0);a.value(4,0);a.call(0x100d3040);a.sign8(0,3,true);
            if(!a.equal()){a.value(4,0x55550000);a.offset(3,22,-0x170000);a.offset(0,4,0x5556);a.multiply_high(4,0,22);a.offset(3,3,0x1ca0);a.mask(0,4,1,false,1);a.add(0,4,0);a.compare_registers(0,3);if(!a.nonpositive())a.move(3,0);a.at(0x1041311c);a.store(1,0xc0,3);a.jump(0x10413170);}}
        else{a.at(0x10413124);a.load(3,27,0);a.move(4,24);a.move(5,17);a.move(6,16);a.value(7,0);a.value(8,1);a.value(9,1);a.call(0x10085860);a.move(0,3);a.load(3,27,0);a.move(22,0);a.move(4,24);a.move(5,17);a.move(6,16);a.value(7,0);a.value(8,1);a.value(9,0);a.call(0x10085860);a.subtract(14,3,22);}
    }
    a.at(0x10413170);a.compare(21,0,false);bool clause=!a.equal();if(clause){a.move(3,21);a.call(0x10380520);a.sign8(0,3,true);clause=!a.equal();}if(clause){a.load(4,21,0x24);a.compare_registers(22,4);clause=!a.nonpositive();}if(clause){a.load<1>(0,15,0x3c);a.value(3,1);a.insert_mask(0,3,0xf0,4);a.move(22,4);a.store<1>(15,0x3c,0);}
    a.at(0x104131a8);a.compare(18,0,false);bool target=!a.equal();if(target){a.load<1>(0,18,0);a.compare(0,11);target=a.equal();}
    if(!target){a.at(0x104131bc);a.value(23,0);a.jump(0x10413218);}else{a.at(0x104131c4);a.compare(18,0,false);if(!a.equal()){a.load(3,26,0);a.move(4,17);a.move(5,16);a.move(6,18);a.load(3,3,0x988);a.offset(7,1,0x40);a.value(8,0xffffffff);a.value(9,1);a.call(0x10361490);a.move(23,3);a.jump(0x10413218);}else{a.at(0x104131f8);a.load(3,27,0);a.move(4,16);a.move(5,17);a.value(6,0);a.value(7,0xffffffff);a.value(8,0);a.call(0x10084be0);a.move(23,3);}}
}
void value_at_primary_club(Call& a){a.load(3,27,0);a.move(4,16);a.move(5,24);a.value(6,0);a.value(7,0xffffffff);a.value(8,0);a.call(0x10084be0);}
void apply_financial_constraint(Call& a){
    a.at(0x10413218);a.compare(18,0,false);bool constrained=!a.equal();if(constrained){a.load<1>(0,18,0);a.compare(0,11);constrained=a.equal();}if(constrained){a.compare(24,0,false);constrained=!a.equal();}if(constrained){a.load(3,2,-0x7ab8);a.move(4,24);a.load(3,3,0);a.call(0x10103510);a.compare(3,0,false);constrained=!a.equal();}if(constrained){a.load(3,2,-0x7ab8);a.move(4,24);a.load(3,3,0);a.call(0x10103510);a.load<1>(0,3,0x165);a.compare(0,1);constrained=a.equal();}
    if(!constrained)return;
    a.move(3,24);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
    if(!a.equal()){
        value_at_primary_club(a);a.compare_registers(23,3);if(!a.at_least()){a.move(3,23);a.jump(0x104132d0);}else{a.at(0x104132b4);value_at_primary_club(a);}
        a.at(0x104132d0);a.compare_registers(22,3);if(!a.at_least()){a.move(3,22);a.jump(0x10413328);}else{a.at(0x104132e0);value_at_primary_club(a);a.compare_registers(23,3);if(!a.at_least()){a.move(3,23);a.jump(0x10413328);}else{a.at(0x1041330c);value_at_primary_club(a);}}
        a.at(0x10413328);a.move(23,3);a.jump(0x10413364);
    }else{a.at(0x10413330);a.load(3,16,8);a.compare_registers(23,3);a.move(0,3);if(!a.at_least())a.move(0,23);a.at(0x10413344);a.compare_registers(22,0);if(!a.at_least()){a.move(23,22);a.jump(0x10413364);}else{a.at(0x10413354);a.compare_registers(23,3);if(!a.at_least())a.move(3,23);a.at(0x10413360);a.move(23,3);}}
    a.at(0x10413364);a.value(0,1);a.store<1>(1,0xc4,0);
}
void payment_mood_factor(Call& a,std::uint32_t pc,bool buyer){
    a.at(pc);a.load(3,2,-0x7ab8);a.move(4,buyer?17:24);a.load(3,3,0);a.call(0x10103510);a.call(0x100f55b0);a.sign8(0,3,true);
    if(a.equal()){a.at(pc+0x58);a.floating_load(0,25,0x30);a.floating_product(31,31,0);a.jump(pc+0x78);}
    else if(a.at_least()){a.at(pc+0x30);a.compare(0,2);if(a.equal()){a.at(pc+0x40);a.floating_load(0,25,buyer?0x20:0x40);a.floating_product(31,31,0);a.jump(pc+0x78);}else if(!a.at_least()){a.jump(pc+0x4c);a.floating_load(0,25,buyer?0x28:0x38);a.floating_product(31,31,0);a.jump(pc+0x78);}}
    else{a.at(pc+0x20);a.compare(0,-2);if(a.equal()){a.at(pc+0x70);a.floating_load(0,25,buyer?0x40:0x20);a.floating_product(31,31,0);}else if(a.at_least()){a.at(pc+0x64);a.floating_load(0,25,buyer?0x38:0x28);a.floating_product(31,31,0);a.jump(pc+0x78);}else a.jump(pc+0x78);}
    a.at(pc+0x78);
}
struct DivisionScratch{unsigned base,target,limit,base_out,target_out,limit_out;};
void divide_period_values(Call& a,std::uint32_t pc,unsigned period,unsigned factor,DivisionScratch w,bool jump){
    a.at(pc);a.value(0,period);a.value(4,0x43300000);a.store<1>(15,0x51,0);a.exclusive_flags(5,23,0x80000000);a.exclusive_flags(0,19,0x80000000);a.floating_load(4,25,0xc0);a.load(3,15,0x20);a.store(1,w.base,4);a.exclusive_flags(3,3,0x80000000);a.floating_load(3,25,factor);a.store(1,w.base+4,3);a.floating_load(0,1,w.base);a.store(1,w.target+4,5);a.floating_subtract(0,0,4);a.store(1,w.target,4);a.floating_load(2,1,w.target);a.floating_difference(1,0,3,true);a.store(1,w.limit+4,0);a.store(1,w.limit,4);a.floating_load(0,1,w.limit);a.floating_subtract(2,2,4);a.floating_subtract(0,0,4);a.floating_integer(1,1);a.floating_difference(2,2,3,true);a.floating_store(1,1,w.base_out);a.load(0,1,w.base_out+4);a.store(15,0x20,0);a.floating_difference(0,0,3,true);a.floating_integer(1,2);a.floating_integer(0,0);a.floating_store(1,1,w.target_out);a.floating_store(0,1,w.limit_out);a.load(23,1,w.target_out+4);a.load(19,1,w.limit_out+4);if(jump)a.jump(0x104137d0);
}
void choose_payment_period(Call& a){
    a.at(0x1041336c);a.compare(24,0,false);a.store(15,0x20,22);if(a.equal())return;
    advance_pool(a,0x10413378);a.load(5,31,0);a.value(4,0x80010000);a.value(3,0x10000);a.value(0,0x43300000);a.load<2>(7,5,0);a.offset(5,4,-0x7fff);a.load(6,28,0);a.offset(4,3,-1);a.store(1,0xb4,0);a.value(3,152);a.add(6,7,6);a.floating_load(1,25,0xc0);a.multiply_high(0,5,6);a.floating_load(2,25,8);a.add(0,0,6);a.shift_right(0,0,15);a.mask(5,0,1,false,1);a.add(0,0,5);a.product(0,0,4);a.subtract(0,0,6);a.exclusive_flags(0,0,0x80000000);a.store(1,0xb8,0);a.floating_load(0,1,0xb4);a.floating_subtract(0,0,1);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0xac);a.load(13,1,0xb0);a.call(0x10415e50);a.value(5,0x66660000);a.value(4,0xf0000);a.offset(3,3,5);a.offset(5,5,0x6667);a.offset(0,4,0x423f);a.multiply_high(3,5,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(13,13,3);a.compare_registers(13,0);if(!a.nonpositive())a.move(13,0);
    a.at(0x10413448);a.load(0,15,0x20);a.compare_registers(0,13);if(a.nonpositive())return;
    a.load(4,17,0x2c);a.load(3,2,-0x7a10);a.load(4,4,0x68);a.load(0,3,0);a.floating_load(31,25,0x10);a.compare_registers(4,0);bool named=a.equal();if(!named){a.load(3,2,-0x7a1c);a.load(0,3,0);a.compare_registers(4,0);named=a.equal();}if(named){a.at(0x10413480);a.floating_load(31,25,0x18);}
    payment_mood_factor(a,0x10413484,true);payment_mood_factor(a,0x104134fc,false);
    a.floating_load(0,25,0x48);a.floating_compare(31,0);if(!a.nonpositive()){divide_period_values(a,0x10413580,24,0x50,{0xa4,0xac,0x94,0x9c,0xb4,0x8c},true);return;}
    a.at(0x1041360c);a.floating_load(0,25,0x58);a.floating_compare(31,0);if(!a.nonpositive()){divide_period_values(a,0x10413618,18,0x60,{0x9c,0x8c,0xac,0xa4,0x94,0xb4},true);return;}
    a.at(0x104136a4);a.floating_load(0,25,0x68);a.floating_compare(31,0);if(!a.nonpositive()){divide_period_values(a,0x104136b0,12,0x30,{0x9c,0x8c,0xac,0xa4,0x94,0xb4},true);return;}
    a.at(0x1041373c);a.floating_load(0,25,0);a.floating_compare(31,0);if(!a.nonpositive())divide_period_values(a,0x10413748,6,0x70,{0x9c,0x8c,0xac,0xa4,0x94,0xb4},false);
}
}
Route select_base_and_period(Call& a){
    if(try_contract_route(a))return a.r(3)==0?Route::failed:Route::finalize;
    resolve_primary_club(a);a.at(0x1041304c);a.compare(18,0,false);bool missing=!a.equal();if(missing){a.load<1>(0,18,0);a.compare(0,11);missing=a.equal();}if(missing){a.load(0,16,0x14);a.compare(0,0,false);missing=a.equal();}if(missing){a.value(3,0);a.jump(0x10415e34);return Route::failed;}
    select_initial_values(a);apply_financial_constraint(a);choose_payment_period(a);return Route::price;
}
}
