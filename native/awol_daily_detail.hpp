#pragma once
#include "awol_records.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::awol::daily_detail {
using Call=SemanticGuestCall;

inline void advance_pool(Call& a,std::uint32_t start,bool cached=false) {
    a.at(start);if(!cached)a.load(3,31,0);a.load(0,22,0);a.offset(3,3,2);
    if(cached){a.store(31,0,3);a.compare_registers(3,0,false);}else{a.compare_registers(3,0,false);a.store(31,0,3);}
    if(!a.nonpositive()){a.store(31,0,23);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(30,0,0);}
    a.at(start+(cached?0x24:0x28));
}

template<unsigned Bound,unsigned Result,bool Peer=false,std::int32_t Input=0x60,std::int32_t Output=0x70,bool Cached=false>
void bounded_draw(Call& a,std::uint32_t pool) {
    advance_pool(a,pool,Cached);const auto test=pool+(Cached?0x24:0x28),wide=test+0x20,small=wide+0xd4,join=small+0x2c;
    a.value(3,0x10000);a.offset(0,3,-1);a.compare_registers(Bound,0);bool scaled=a.positive();
    if(!scaled){a.value(3,0xffff0000);a.offset(0,3,1);a.compare_registers(Bound,0);scaled=!a.at_least();}
    if(scaled) {
        a.at(wide);
        if constexpr(!Peer) {
            a.value(4,0x43300000);a.exclusive_flags(0,Bound,0x80000000);a.store(1,Input+4,0);a.value(5,0x80010000);a.load(7,31,0);a.value(3,0x10000);a.store(1,Input,4);a.offset(6,5,-0x7fff);a.floating_load(2,29,8);a.multiply(0,Bound,10);a.floating_load(1,1,Input);a.offset(5,3,-1);a.floating_load(0,29,0);a.floating_subtract(1,1,2);a.multiply_high(3,6,0);a.load<2>(8,7,0);a.load(7,30,0);a.floating_difference(1,1,0,true);a.store(1,0x68,4);a.add(7,8,7);a.multiply_high(4,6,7);a.add(0,3,0);a.add(3,4,7);a.shift_right(3,3,15);a.mask(4,3,1,false,1);a.shift_right(0,0,15);a.add(3,3,4);a.product(4,3,5);a.mask(3,0,1,false,1);a.add(3,0,3);a.subtract(0,4,7);
        }else {
            a.value(0,0x43300000);a.exclusive_flags(3,Bound,0x80000000);a.store(1,Input+4,3);a.value(3,0x80010000);a.load(7,31,0);a.value(4,0x10000);a.store(1,Input,0);a.offset(6,3,-0x7fff);a.floating_load(2,29,8);a.multiply(3,Bound,10);a.floating_load(1,1,Input);a.offset(5,4,-1);a.floating_load(0,29,0);a.floating_subtract(1,1,2);a.multiply_high(4,6,3);a.load<2>(8,7,0);a.load(7,30,0);a.floating_difference(1,1,0,true);a.store(1,0x68,0);a.add(0,8,7);a.multiply_high(6,6,0);a.add(3,4,3);a.add(4,6,0);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.shift_right(3,3,15);a.add(4,4,6);a.product(5,4,5);a.mask(4,3,1,false,1);a.add(3,3,4);a.subtract(0,5,0);
        }
        a.exclusive_flags(0,0,0x80000000);a.store(1,0x6c,0);a.floating_load(0,1,0x68);a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,Output);a.load(Result,1,Output+4);a.call(sampling_entry);
        a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,Bound,-1);a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(Result,Result,3);a.compare_registers(Result,0);
        if(!a.nonpositive()){a.move(Result,0);a.jump(join);}
    }else {
        a.at(small);a.load(3,31,0);a.compare(Bound,0);a.load(0,30,0);a.load<2>(3,3,0);a.add(3,3,0);a.divide(0,3,Bound);a.product(0,0,Bound);a.subtract(0,0,3);a.negate(Result,0);if(!a.nonpositive())a.move(Result,0);
    }
    a.at(join);
}

template<unsigned Bound,unsigned Result,bool Peer=false,std::int32_t Input=0x60,std::int32_t Output=0x70,bool Cached=false>
void draw_or_zero(Call& a,std::uint32_t pool) {
    if(a.equal()){a.value(Result,0);a.jump(pool+(Cached?0x144:0x148));}
    else bounded_draw<Bound,Result,Peer,Input,Output,Cached>(a,pool);
}

void return_from_leave(Call&);
void return_from_unapproved_absence(Call&);
}
