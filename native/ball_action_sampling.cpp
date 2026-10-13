#include "ball_action_detail.hpp"

namespace cm::aot::ball_action::detail {
void advance_action_random(SemanticGuestCall& a,std::uint32_t start) {
    a.at(start);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(start+0x28);
}

void sample_action_interval(SemanticGuestCall& a,std::uint32_t start,unsigned interval,unsigned result) {
    const auto done=start+0x15c;
    a.at(start);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(start+0x28);a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(interval,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(interval,3);wide=!a.at_least();}
    if(wide) {
        a.at(start+0x48);a.exclusive_flags(3,interval,0x80000000);a.value(0,0x43300000);a.store(1,0x7c,3);a.value(6,0x80010000);
        a.load(7,30,0);a.value(4,0x43300000);a.store(1,0x78,0);a.value(5,0x10000);a.floating_load(2,26,0x80);
        a.value(3,0x80010000);a.floating_load(1,1,0x78);a.offset(6,6,-0x7fff);a.floating_load(0,26,0);
        a.offset(5,5,-1);a.floating_subtract(1,1,2);a.load<2>(8,7,0);a.load(7,27,0);a.multiply(0,interval,10);
        a.offset(3,3,-0x7fff);a.store(1,0x80,4);a.floating_difference(2,1,0,true);a.add(7,8,7);a.multiply_high(4,6,7);
        a.floating_load(1,26,0x80);a.add(4,4,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.add(4,4,6);
        a.product(4,4,5);a.subtract(result,4,7);a.exclusive_flags(4,result,0x80000000);a.store(1,0x84,4);
        a.multiply_high(3,3,0);a.floating_load(0,1,0x80);a.floating_subtract(0,0,1);a.add(0,3,0);a.shift_right(0,0,15);
        a.floating_product(0,2,0);a.mask(3,0,1,false,1);a.add(3,0,3);a.floating_integer(0,0);a.floating_store(0,1,0x88);
        a.load(result,1,0x8c);a.call(0x10226520);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);
        a.offset(0,interval,-1);a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);
        a.add(result,result,3);a.compare_registers(result,0);
        if(!a.nonpositive()){a.offset(result,interval,-1);a.jump(done);}
    } else {
        a.at(start+0x12c);a.load(3,30,0);a.compare(interval,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(3,3,0);
        a.divide(0,3,interval);a.product(0,0,interval);a.subtract(result,0,3);a.negate(0,result);
        if(!a.nonpositive())a.move(0,result);
        a.at(start+0x158);a.move(result,0);
    }
}

void area_coordinates(SemanticGuestCall& a,std::uint32_t start,std::uint32_t join,
                      bool right,unsigned coordinate,unsigned truth,unsigned result,unsigned actor) {
    a.at(start);a.load<1>(coordinate,actor,0x105);a.value(truth,0);a.sign8(coordinate,coordinate);a.compare(coordinate,2);
    bool inside=!a.less();
    if(inside){a.load<1>(coordinate,actor,0x105);a.sign8(coordinate,coordinate);a.compare(coordinate,6);inside=!a.positive();}
    if(inside){a.load<1>(coordinate,actor,0x106);a.sign8(coordinate,coordinate,!right);
        if(right)a.compare(coordinate,10);
        inside=!a.less();}
    if(inside){a.load<1>(coordinate,actor,0x106);a.sign8(coordinate,coordinate);a.compare(coordinate,right?11:1);inside=!a.positive();}
    if(inside)a.value(truth,1);
    a.at(start+(right?0x48:0x44));a.sign8(result,truth);
    if(right)a.jump(join);
}
}
