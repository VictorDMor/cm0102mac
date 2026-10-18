#pragma once
#include "awol_records.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::awol::event_detail {
using Call=SemanticGuestCall;

// Register locations vary between the original event operations. Keep the
// shared pool's ordered reads, cursor publication and reseed effects explicit.
template<unsigned Constants,unsigned Salt,unsigned Base,unsigned End,unsigned Cursor>
struct RandomPool {
    static void advance(Call& a,std::uint32_t start) {
        a.at(start);a.load(3,Cursor,0);a.load(0,End,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(Cursor,0,3);
        if(!a.nonpositive()){a.store(Cursor,0,Base);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(Salt,0,0);}
        a.at(start+0x28);
    }
    template<unsigned Bound,unsigned Result>
    static void draw(Call& a,std::uint32_t pool) {
        advance(a,pool);const auto wide=pool+0x48,small=pool+0x11c,join=pool+0x148;
        a.value(3,0x10000);a.offset(0,3,-1);a.compare_registers(Bound,0);bool scaled=a.positive();
        if(!scaled){a.value(3,0xffff0000);a.offset(0,3,1);a.compare_registers(Bound,0);scaled=!a.at_least();}
        if(scaled) {
            a.at(wide);a.value(0,0x43300000);a.exclusive_flags(3,Bound,0x80000000);a.store(1,0x50,3);a.value(3,0x80010000);a.load(7,Cursor,0);a.value(4,0x10000);a.store(1,0x4c,0);a.offset(6,3,-0x7fff);a.floating_load(2,Constants,8);a.multiply(3,Bound,10);a.floating_load(1,1,0x4c);a.offset(5,4,-1);a.floating_load(0,Constants,0);a.floating_subtract(1,1,2);a.multiply_high(4,6,3);a.load<2>(8,7,0);a.load(7,Salt,0);a.floating_difference(1,1,0,true);a.store(1,0x54,0);a.add(0,8,7);a.multiply_high(6,6,0);a.add(3,4,3);a.add(4,6,0);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.shift_right(3,3,15);a.add(4,4,6);a.product(5,4,5);a.mask(4,3,1,false,1);a.add(3,3,4);a.subtract(0,5,0);a.exclusive_flags(0,0,0x80000000);a.store(1,0x58,0);a.floating_load(0,1,0x54);a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x5c);a.load(Result,1,0x60);a.call(sampling_entry);
            a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,Bound,-1);a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(Result,Result,3);a.compare_registers(Result,0);if(!a.nonpositive()){a.move(Result,0);a.jump(join);}
        }else {
            a.at(small);a.load(3,Cursor,0);a.compare(Bound,0);a.load(0,Salt,0);a.load<2>(3,3,0);a.add(3,3,0);a.divide(0,3,Bound);a.product(0,0,Bound);a.subtract(0,0,3);a.negate(Result,0);if(!a.nonpositive())a.move(Result,0);
        }
        a.at(join);
    }
    template<unsigned Bound,unsigned Result>
    static void draw_or_zero(Call& a,std::uint32_t pool) {
        if(a.equal()){a.value(Result,0);a.jump(pool+0x148);}else draw<Bound,Result>(a,pool);
    }
};
}
