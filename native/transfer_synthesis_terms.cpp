#include "transfer_synthesis_terms.hpp"

namespace cm::aot::transfer_synthesis::detail {
void choose_ordinary_period(Call& a,std::uint32_t pc,bool thirds,unsigned increment){
    a.at(pc);a.load(5,31,0);a.value(3,thirds?0x55550000:0x66660000);a.offset(0,3,thirds?0x5556:0x6667);a.load(4,28,0);a.load<2>(3,5,0);a.add(4,3,4);
    if(thirds){a.multiply_high(3,0,4);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);}
    else{a.multiply_high(0,0,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,5);}
    a.subtract(3,0,4);a.offset(0,3,increment);a.multiply(0,0,10);a.store<1>(15,0x44,0);
}

void calculate_ordinary_amount(Call& a,std::uint32_t pc,unsigned increment,bool alternate){
    a.at(pc);
    if(!alternate){
        a.load<1>(3,15,0x44);a.value(0,0x43300000);a.load(5,31,0);a.value(4,0x2e8c0000);a.sign8(3,3);a.load(6,15,0x20);a.exclusive_flags(3,3,0x80000000);a.store(1,0x9c,0);a.exclusive_flags(6,6,0x80000000);a.floating_load(4,25,0xc0);a.store(1,0xa0,3);a.offset(3,4,-0x5d17);a.floating_load(0,25,0x38);a.floating_load(1,1,0x9c);a.load<2>(5,5,0);a.floating_subtract(1,1,4);a.load(4,28,0);a.store(1,0x90,6);a.add(4,5,4);a.floating_load(2,25,0x98);a.floating_difference(0,1,0,true);a.multiply_high(3,3,4);a.store(1,0x8c,0);a.floating_load(1,1,0x8c);a.store(1,0x94,0);a.shift_right(0,3,1);
    }else{
        a.load<1>(0,15,0x44);a.value(5,0x43300000);a.load(4,31,0);a.value(3,0x2e8c0000);a.sign8(0,0);a.load(6,15,0x20);a.exclusive_flags(0,0,0x80000000);a.store(1,0x9c,5);a.exclusive_flags(6,6,0x80000000);a.floating_load(4,25,0xc0);a.store(1,0xa0,0);a.offset(0,3,-0x5d17);a.floating_load(0,25,0x38);a.floating_load(1,1,0x9c);a.load<2>(4,4,0);a.floating_subtract(1,1,4);a.load(3,28,0);a.store(1,0x90,6);a.add(4,4,3);a.floating_load(2,25,0x98);a.floating_difference(0,1,0,true);a.multiply_high(0,0,4);a.store(1,0x8c,5);a.floating_load(1,1,0x8c);a.store(1,0x94,5);a.shift_right(0,0,1);
    }
    a.mask(3,0,1,false,1);a.floating_subtract(3,1,4);a.add(0,0,3);a.floating_subtract(0,2,0);a.multiply(0,0,11);a.subtract(3,0,4);a.offset(0,3,increment);a.exclusive_flags(0,0,0x80000000);a.store(1,0x98,0);a.floating_load(1,1,0x94);a.floating_subtract(1,1,4);a.floating_product(1,3,1);a.floating_difference(0,1,0,true);a.floating_integer(0,0);a.floating_store(0,1,0xa4);a.load(0,1,0xa8);a.store(15,0x40,0);
}

namespace {
void rollover_after_draw(Call& a){if(!a.nonpositive()){a.store(31,0,29);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(28,0,0);}}
void adjust_ordinary_for_delta(Call& a,std::uint32_t pc,std::uint32_t join){
    a.at(pc);a.compare(22,10000);a.value(0,10000);a.load<1>(4,15,0x44);a.load(3,15,0x40);a.sign8(4,4);a.product(5,3,14);a.multiply(4,4,30000);a.add(3,22,4);if(!a.nonpositive())a.move(0,22);
    a.at(pc+0x28);a.divide(0,5,0);a.compare_registers(3,0);if(!a.at_least())a.jump(pc+0x4c);else{a.at(pc+0x38);a.compare(22,10000);a.value(0,10000);if(!a.nonpositive())a.move(0,22);a.at(pc+0x48);a.divide(3,5,0);}
    a.at(pc+0x4c);a.store(15,0x40,3);a.jump(join);
}
}

void choose_percentage_pair(Call& a,std::uint32_t pc,bool fifths,bool compare_delta,bool adjust_ordinary,std::uint32_t join){
    // The first draw advances the cursor before deciding whether it wrapped.
    // Do not replace this with two calls to advance_pool: the reseed call is
    // observably interleaved with the first percentage's arithmetic.
    a.at(pc);a.load(7,31,0);a.value(3,fifths?0x66660000:0x2aab0000);a.load(5,28,0);a.offset(4,3,fifths?0x6667:-0x5555);a.load<2>(6,7,0);a.offset(3,7,2);a.load(0,30,0);a.add(5,6,5);a.store(31,0,3);a.multiply_high(4,4,5);a.compare_registers(3,0,false);
    if(fifths){a.shift_right(0,4,1);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,5);a.subtract(3,0,5);a.offset(0,3,1);}
    else{a.mask(0,4,1,false,1);a.add(0,4,0);a.multiply(0,0,6);a.subtract(0,0,5);}
    a.multiply(0,0,5);a.sign8(13,0);rollover_after_draw(a);
    const auto second=pc+(fifths?0x60:0x58);a.at(second);
    if(compare_delta){
        a.load(4,31,0);a.value(3,0x43300000);a.value(5,fifths?0x66660000:0x2aab0000);a.exclusive_flags(0,22,0x80000000);a.load<2>(7,4,0);a.exclusive_flags(4,14,0x80000000);a.load(6,28,0);a.offset(5,5,fifths?0x6667:-0x5555);a.store(1,0x98,0);a.add(6,7,6);a.floating_load(2,25,0xc0);a.multiply_high(fifths?0:5,5,6);a.store(1,0x94,3);a.floating_load(1,25,0x18);a.floating_load(0,1,0x94);a.store(1,0x90,4);a.floating_subtract(0,0,2);
        if(fifths){a.shift_right(0,0,1);a.store(1,0x8c,3);a.mask(3,0,1,false,1);a.floating_product(0,1,0);a.add(0,0,3);a.floating_load(1,1,0x8c);a.multiply(0,0,5);a.floating_subtract(1,1,2);a.subtract(3,0,6);a.offset(0,3,1);a.floating_compare(1,0);a.multiply(0,0,5);a.sign8(0,0);}
        else{a.store(1,0x8c,3);a.mask(0,5,1,false,1);a.add(0,5,0);a.floating_product(0,1,0);a.multiply(0,0,6);a.floating_load(1,1,0x8c);a.floating_subtract(1,1,2);a.subtract(0,0,6);a.multiply(0,0,5);a.floating_compare(1,0);a.sign8(0,0);}
        const auto branch=second+(fifths?0x78:0x70);const auto maximum=branch+4;const auto minimum=branch+(adjust_ordinary?0x68:0x18);
        if(!a.nonpositive()){
            a.at(maximum);a.compare_registers(13,0);if(!a.nonpositive())a.move(0,13);a.at(maximum+0xc);a.store<1>(15,0x50,0);
            if(adjust_ordinary)adjust_ordinary_for_delta(a,maximum+0x10,join);else a.jump(join);
        }else{a.at(minimum);a.compare_registers(13,0);if(!a.at_least())a.move(0,13);a.at(minimum+0xc);a.store<1>(15,0x50,0);a.jump(join);}
    }else{
        a.load(5,31,0);a.value(3,fifths?0x66660000:0x2aab0000);a.offset(0,3,fifths?0x6667:-0x5555);a.load(4,28,0);a.load<2>(3,5,0);a.add(4,3,4);
        if(fifths){a.multiply_high(0,0,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,5);a.subtract(3,0,4);a.offset(0,3,1);}
        else{a.multiply_high(3,0,4);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,6);a.subtract(0,0,4);}
        a.multiply(0,0,5);a.sign8(0,0);a.compare_registers(13,0);if(!a.at_least())a.move(0,13);
        a.at(second+(fifths?0x48:0x40));a.store<1>(15,0x50,0);if(join)a.jump(join);
    }
}

void sample_payment_range(Call& a,std::uint32_t pc){
    a.at(pc);a.value(3,0x10000);a.offset(0,3,-1);a.compare_registers(14,0);bool wide=a.positive();if(!wide){a.value(3,0xffff0000);a.offset(0,3,1);a.compare_registers(14,0);wide=!a.at_least();}
    if(wide){
        a.at(pc+0x20);a.value(0,0x43300000);a.exclusive_flags(3,14,0x80000000);a.store(1,0x90,3);a.value(3,0x80010000);a.load(7,31,0);a.value(4,0x10000);a.store(1,0x8c,0);a.offset(6,3,-0x7fff);a.floating_load(2,25,0xc0);a.multiply(3,14,10);a.floating_load(1,1,0x8c);a.offset(5,4,-1);a.floating_load(0,25,0xa0);a.floating_subtract(1,1,2);a.multiply_high(4,6,3);a.load<2>(8,7,0);a.load(7,28,0);a.floating_difference(1,1,0,true);a.store(1,0x94,0);a.add(0,8,7);a.multiply_high(6,6,0);a.add(3,4,3);a.add(4,6,0);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.shift_right(3,3,15);a.add(4,4,6);a.product(5,4,5);a.mask(4,3,1,false,1);a.add(3,3,4);a.subtract(0,5,0);a.exclusive_flags(0,0,0x80000000);a.store(1,0x98,0);a.floating_load(0,1,0x94);a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x9c);a.load(13,1,0xa0);a.call(0x10415e50);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,14,-1);a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(13,13,3);a.compare_registers(13,0);if(!a.nonpositive()){a.move(13,0);a.jump(pc+0x120);}
    }else{a.at(pc+0xf4);a.load(3,31,0);a.compare(14,0);a.load(0,28,0);a.load<2>(3,3,0);a.add(3,3,0);a.divide(0,3,14);a.product(0,0,14);a.subtract(0,0,3);a.negate(13,0);if(!a.nonpositive())a.move(13,0);}
    a.at(pc+0x120);
}

void sample_source_component(Call& a){
    constexpr auto pc=0x10414ac0u;
    a.at(pc);a.value(3,0x10000);a.offset(0,3,-1);a.compare_registers(14,0);bool wide=a.positive();if(!wide){a.value(3,0xffff0000);a.offset(0,3,1);a.compare_registers(14,0);wide=!a.at_least();}
    if(wide){
        a.at(pc+0x20);a.value(0,0x43300000);a.exclusive_flags(3,14,0x80000000);a.store(1,0x90,3);a.value(3,0x80010000);a.load(7,31,0);a.offset(6,3,-0x7fff);a.store(1,0x8c,0);a.value(4,0x10000);a.floating_load(2,25,0xc0);a.multiply(3,14,10);a.floating_load(1,1,0x8c);a.offset(5,4,-1);a.floating_load(0,25,0xa0);a.floating_subtract(1,1,2);a.load<2>(8,7,0);a.multiply_high(4,6,3);a.load(7,28,0);a.store(1,0x94,0);a.floating_difference(1,1,0,true);a.add(0,8,7);a.multiply_high(6,6,0);a.add(3,4,3);a.add(4,6,0);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.shift_right(3,3,15);a.add(4,4,6);a.product(5,4,5);a.mask(4,3,1,false,1);a.add(3,3,4);a.subtract(0,5,0);a.exclusive_flags(0,0,0x80000000);a.store(1,0x98,0);a.floating_load(0,1,0x94);a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x9c);a.load(0,1,0xa0);a.store(1,0xc8,0);a.call(0x10415e50);a.value(4,0x66660000);a.offset(0,3,5);a.offset(4,4,0x6667);a.offset(3,14,-1);a.multiply_high(0,4,0);a.shift_right(0,0,2);a.mask(4,0,1,false,1);a.add(4,0,4);a.load(0,1,0xc8);a.add(0,0,4);a.compare_registers(0,3);a.store(1,0xc8,0);if(!a.nonpositive()){a.store(1,0xc8,3);a.jump(0x10414bf0);}
    }else{a.at(0x10414bc0);a.load(3,31,0);a.compare(14,0);a.load(0,28,0);a.load<2>(3,3,0);a.add(3,3,0);a.divide(0,3,14);a.product(0,0,14);a.subtract(3,0,3);a.negate(0,3);a.store(1,0xc8,0);if(!a.nonpositive())a.store(1,0xc8,3);}
    a.at(0x10414bf0);
}
}
