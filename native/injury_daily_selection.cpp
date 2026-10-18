#include "injury_daily_detail.hpp"

namespace cm::aot::injury_daily::detail {
bool eligible_staff(Call& a) {
    a.at(0x10185710);a.multiply(21,17,0x70);a.load(0,30,0);a.add(18,0,21);a.load(0,18,0x14);a.compare(0,0,false);
    if(a.equal()){a.offset(19,19,-1);a.jump(0x10186470);return false;}
    a.at(0x10185730);a.load<1>(0,18,0x62);a.sign8(0,0);a.compare(0,11);bool kind=a.equal();
    if(!kind){a.compare(0,15);kind=a.equal();if(!kind){a.compare(0,13);if(!a.equal())return false;}}
    a.at(0x10185750);a.load(3,2,-0x7b24);a.move(4,18);a.value(5,0);a.load(3,3,0);a.call(0x10003dc0);a.sign8(0,3,true);if(!a.equal())return false;
    a.load(3,18,0x14);a.load<1>(0,3,15);a.compare(0,20);
    if(a.equal()) {
        advance_pool(a,0x1018577c);a.load(7,29,0);a.value(3,0x66660000);a.load(5,26,0);a.offset(4,3,0x6667);a.load<2>(6,7,0);a.offset(3,7,2);a.load(0,28,0);a.add(5,6,5);a.store(29,0,3);a.multiply_high(4,4,5);a.compare_registers(3,0,false);a.shift_right(0,4,1);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,5);a.subtract(16,0,5);
        if(!a.nonpositive()){a.store(29,0,27);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(26,0,0);}
        a.at(0x101857f8);a.load(3,29,0);a.load(0,26,0);a.load<2>(3,3,0);a.add(0,3,0);a.mask(3,0,1,false,1);a.mask(0,0,1);a.combine(0,0,3,false,true);a.subtract(0,3,0);a.compare_registers(16,0);if(!a.nonpositive())return false;
    }
    a.at(0x10185820);a.multiply(22,17,0x24);a.load(0,15,0);a.add(3,0,22);a.load(0,3,0x10);a.compare(0,-1);if(!a.equal())return false;
    a.load<1>(0,3,0x14);a.sign8(0,0);a.compare(0,-1);
    if(!a.nonpositive()) {
        advance_pool(a,0x10185848);a.load(6,29,0);a.value(4,0x51ec0000);a.load(3,18,0x14);a.offset(5,4,-0x7ae1);a.load<2>(6,6,0);a.load(0,26,0);a.load<1>(4,3,0x2d);a.add(0,6,0);a.load(3,15,0);a.multiply_high(6,5,0);a.sign8(5,4);a.add(4,3,22);a.load<1>(3,4,0x15);a.load<1>(4,4,0x22);a.sign8(3,3);a.shift_right(6,6,5);a.sign8(4,4);a.multiply(5,5,5);a.mask(7,6,1,false,1);a.add(6,6,7);a.multiply(6,6,100);a.add(3,3,5);a.add(3,4,3);a.mask(3,3,0xfffffffe,false,1);a.subtract(4,6,0);a.offset(0,3,-30);a.compare_registers(4,0);if(!a.nonpositive())return false;a.jump(0x10185970);
    }else {
        advance_pool(a,0x101858e4);a.load(5,29,0);a.value(4,0x66660000);a.load(3,18,0x14);a.offset(6,4,0x6667);a.load<2>(5,5,0);a.offset(4,22,0x22);a.load(0,26,0);a.load<1>(3,3,0x2d);a.add(0,5,0);a.load(5,15,0);a.multiply_high(6,6,0);a.sign8(3,3);a.indexed_load<1>(4,5,4);a.sign8(4,4);a.shift_right(5,6,6);a.mask(6,5,1,false,1);a.multiply(3,3,5);a.add(5,5,6);a.multiply(5,5,160);a.add(3,4,3);a.mask(3,3,0xfffffffe,false,1);a.subtract(4,5,0);a.offset(0,3,-30);a.compare_registers(4,0);if(!a.nonpositive())return false;
    }
    advance_pool(a,0x10185970);a.load(3,29,0);a.value(4,0x55550000);a.load(5,26,0);a.offset(0,4,0x5556);a.load<2>(4,3,0);a.add(5,4,5);a.multiply_high(4,0,5);a.mask(0,4,1,false,1);a.add(0,4,0);a.multiply(0,0,3);a.subtract(0,0,5,true);if(!a.equal())return false;
    a.load(0,15,0);a.add(4,0,22);a.load<1>(0,4,0x22);a.sign8(0,0);a.compare(0,20);return !a.less();
}
void choose_injury_type(Call& a) {
    a.at(0x101859e0);a.load<1>(0,4,0x14);a.sign8(0,0);a.compare(0,-1);
    if(!a.nonpositive()) {
        a.load<1>(0,4,0x15);a.sign8(4,0);a.offset_record(16,4,2);if(a.equal()){a.value(17,0);a.jump(0x10185b4c);}else bounded_draw<16,false,true>(a,0x10185a08);
        advance_pool(a,0x10185b4c);a.load(5,29,0);a.value(3,0x55550000);a.load(4,26,0);a.offset(0,3,0x5556);a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(3,0,4);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);a.subtract(0,0,4);a.compare_registers(17,0);
        if(!a.nonpositive()){a.load(4,15,0);a.offset(0,22,0x14);a.move(3,15);a.indexed_load<1>(4,4,0);a.call(0x1018c180);a.sign8(16,3);a.jump(0x10185dfc);return;}
    }
    a.at(0x10185bc4);a.value(3,0x2aab0000);a.offset(31,3,-0x5555);a.flags(0,0,0);
    while(true) {
        advance_pool(a,0x10185bd0);a.load(3,29,0);a.load(0,26,0);a.load<2>(4,3,0);a.add(5,4,0);a.multiply_high(0,31,5);a.shift_right(0,0,1);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,12);a.subtract(0,0,5);a.sign8(16,0,true);
        if(a.equal()){advance_pool(a,0x10185c28,true);a.load(5,29,0);a.value(3,0x51ec0000);a.load(4,26,0);a.offset(0,3,-0x7ae1);a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(0,0,4);a.shift_right(0,0,5);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,100);a.subtract(0,0,4,true);if(!a.equal())continue;}
        a.at(0x10185c80);a.multiply(0,16,3);a.add(3,24,0);a.load<1>(0,3,2);a.load<1>(4,3,1);a.sign8(3,0);a.sign8(4,4);a.offset(0,3,2);a.subtract(23,4,0,true);
        if(a.equal()){a.value(17,0);a.jump(0x10185df4);}else bounded_draw<23>(a,0x10185cac);
        a.at(0x10185df4);a.compare(17,0);if(!a.equal())return;
    }
}
std::uint32_t classify_candidate(Call& a) {
    a.at(0x10185dfc);a.load(3,18,0x14);a.load<1>(0,3,15);a.sign8(0,0);a.compare(0,15);
    if(!a.at_least()){a.sign8(0,16);a.compare(0,7);if(a.equal()){a.offset(19,19,-1);a.jump(0x10186470);return 0x10186470;}}
    a.at(0x10185e24);a.move(3,15);a.move(4,18);a.call(0x10188490);a.sign8(0,3,true);if(!a.equal())return 0x10186470;
    a.compare_registers(19,14);
    if(!a.at_least()){a.load(0,30,0);a.move(3,15);a.move(4,16);a.value(5,0xffffffff);a.add(6,0,21);a.call(0x10188930);a.jump(0x10185e74);}
    else {a.at(0x10185e5c);a.load(0,30,0);a.move(3,15);a.move(4,16);a.value(5,0);a.add(6,0,21);a.call(0x10188930);}
    a.at(0x10185e74);a.sign8(4,3);a.move(17,3);a.offset(0,4,-12);a.compare(0,48,false);if(!a.nonpositive())return 0x10186440;
    a.load(3,2,-0x5440);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);return a.count_branch();
}
}
