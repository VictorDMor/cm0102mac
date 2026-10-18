#pragma once
#include "semantic_guest_call.hpp"

namespace cm::aot::contract_storage::detail {
using Call=SemanticGuestCall;
void select_contract_options(Call&);
void apply_nation_value_rules(Call&);

inline void advance_random_pool(Call& a,std::uint32_t start) {
    a.at(start);a.load(3,26,0);a.load(0,19,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(26,0,3);
    if(!a.nonpositive()){a.store(26,0,20);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(25,0,0);}
    a.at(start+0x28);
}

// Five complete original sampling bodies share this structure. Bound/result
// registers and stack scratch differ; the final bonus body has alternate
// scratch-register ordering, which is retained explicitly.
template<unsigned Bound,unsigned Result,unsigned Frame,bool Alternate=false>
void sample_contract_range(Call& a,std::uint32_t start) {
    a.at(start);a.value(3,0x10000);a.offset(0,3,-1);a.compare_registers(Bound,0);
    bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(0,3,1);a.compare_registers(Bound,0);wide=!a.at_least();}
    if(wide) {
        a.at(start+0x20);
        if constexpr(!Alternate) {
            a.value(0,0x43300000);a.exclusive_flags(3,Bound,0x80000000);a.store(1,Frame+4,3);a.value(3,0x80010000);a.load(7,26,0);a.value(4,0x10000);a.store(1,Frame,0);a.offset(6,3,-0x7fff);a.floating_load(2,29,0xc0);a.multiply(3,Bound,10);a.floating_load(1,1,Frame);a.offset(5,4,-1);a.floating_load(0,29,0);a.floating_subtract(1,1,2);a.multiply_high(4,6,3);a.load<2>(8,7,0);a.load(7,25,0);a.floating_difference(1,1,0,true);a.store(1,Frame+8,0);a.add(0,8,7);a.multiply_high(6,6,0);a.add(3,4,3);a.add(4,6,0);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.shift_right(3,3,15);a.add(4,4,6);a.product(5,4,5);a.mask(4,3,1,false,1);a.add(3,3,4);a.subtract(0,5,0);a.exclusive_flags(0,0,0x80000000);a.store(1,Frame+12,0);a.floating_load(0,1,Frame+8);a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,Frame+16);a.load(Result,1,Frame+20);
        } else {
            a.value(4,0x43300000);a.exclusive_flags(0,Bound,0x80000000);a.store(1,Frame+4,0);a.value(5,0x80010000);a.load(7,26,0);a.value(3,0x10000);a.store(1,Frame,4);a.offset(6,5,-0x7fff);a.floating_load(2,29,0xc0);a.multiply(0,Bound,10);a.floating_load(1,1,Frame);a.offset(5,3,-1);a.floating_load(0,29,0);a.floating_subtract(1,1,2);a.multiply_high(3,6,0);a.load<2>(8,7,0);a.load(7,25,0);a.floating_difference(1,1,0,true);a.store(1,Frame+8,4);a.add(7,8,7);a.multiply_high(4,6,7);a.add(0,3,0);a.add(3,4,7);a.shift_right(3,3,15);a.mask(4,3,1,false,1);a.shift_right(0,0,15);a.add(3,3,4);a.product(4,3,5);a.mask(3,0,1,false,1);a.add(3,0,3);a.subtract(0,4,7);a.exclusive_flags(0,0,0x80000000);a.store(1,Frame+12,0);a.floating_load(0,1,Frame+8);a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,Frame+16);a.load(Result,1,Frame+20);
        }
        a.call(0x10374570);a.value(4,0x66660000);a.offset(0,3,5);a.offset(3,4,0x6667);a.offset(4,Bound,-1);a.multiply_high(0,3,0);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.add(Result,Result,0);a.compare_registers(Result,4);
        if(!a.nonpositive()){a.move(Result,4);a.jump(start+0x120);}
    } else {
        a.at(start+0xf4);a.load(3,26,0);a.compare(Bound,0);a.load(0,25,0);a.load<2>(3,3,0);a.add(3,3,0);a.divide(0,3,Bound);a.product(0,0,Bound);a.subtract(0,0,3);a.negate(Result,0);
        if(!a.nonpositive())a.move(Result,0);
    }
    a.at(start+0x120);
}
}
