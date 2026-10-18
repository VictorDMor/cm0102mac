#include "injury_daily_detail.hpp"

namespace cm::aot::injury_daily::detail {
namespace {
void chance_three(Call& a,std::uint32_t pool,bool alternate=false) {
    advance_pool(a,pool);
    if(alternate){a.load(3,29,0);a.value(4,0x55550000);a.load(5,26,0);a.offset(0,4,0x5556);a.load<2>(4,3,0);a.add(5,4,5);a.multiply_high(4,0,5);a.mask(0,4,1,false,1);a.add(0,4,0);a.multiply(0,0,3);a.subtract(0,0,5,true);}
    else {a.load(5,29,0);a.value(3,0x55550000);a.load(4,26,0);a.offset(0,3,0x5556);a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(3,0,4);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);a.subtract(0,0,4,true);}
}
bool existing_injury_matches(Call& a,std::uint32_t start) {
    a.at(start);a.load(3,15,0);a.offset(0,22,0x14);a.indexed_load<1>(4,3,0);a.sign8(0,4);a.compare(0,-1);if(a.nonpositive())return false;
    a.move(3,15);a.call(0x1018c180);a.sign8(3,3);a.sign8(0,16);a.compare_registers(0,3);return a.equal();
}
void decrement_attribute(Call& a,std::uint32_t start,unsigned field,bool jump) {
    a.at(start);a.load(4,18,0x14);a.load<1>(3,4,field);a.offset(0,3,-1);a.store<1>(4,field,0);if(jump)a.jump(0x10186440);
}
void random_decrement_three(Call& a,std::uint32_t pool,unsigned field,std::uint32_t join) {
    advance_pool(a,pool);a.load(5,29,0);a.value(4,0x55550000);a.load(3,18,0x14);a.offset(4,4,0x5556);a.load<2>(6,5,0);a.load(5,26,0);a.load<1>(0,3,field);a.add(6,6,5);a.multiply_high(5,4,6);a.mask(4,5,1,false,1);a.add(4,5,4);a.multiply(4,4,3);a.subtract(4,4,6);a.offset(4,4,1);a.sign8(4,4);a.subtract(0,4,0);a.store<1>(3,field,0);a.jump(join);
}
void reduce_primary_and_secondary(Call& a) {
    a.at(0x10185f48);a.load(3,18,0x14);a.load<1>(0,3,0x1b);a.sign8(0,0);a.compare(0,20);
    if(a.equal()) {
        if(existing_injury_matches(a,0x10185f5c))random_decrement_three(a,0x10185f8c,0x1b,0x10186124);
        else {
            chance_three(a,0x10185ffc,true);
            if(!a.equal()) {
                advance_pool(a,0x10186054,true);a.load(4,29,0);a.load(3,18,0x14);a.load<2>(5,4,0);a.load(4,26,0);a.load<1>(0,3,0x1b);a.add(4,5,4);a.mask(5,4,1,false,1);a.mask(4,4,1);a.combine(4,4,5,false,true);a.subtract(4,5,4);a.offset(4,4,1);a.sign8(4,4);a.subtract(0,4,0);a.store<1>(3,0x1b,0);a.jump(0x10186124);
            }
        }
    }else {a.at(0x101860b4);a.compare(0,15);if(!a.nonpositive()){chance_three(a,0x101860bc);if(a.equal())decrement_attribute(a,0x10186114,0x1b,false);}}
    a.at(0x10186124);a.load(3,18,0x14);a.load<1>(0,3,0x36);a.compare(0,20);
    if(a.equal()) {
        if(existing_injury_matches(a,0x10186134))random_decrement_three(a,0x10186164,0x36,0x10186440);
        else {chance_three(a,0x101861d4);if(!a.equal())decrement_attribute(a,0x1018622c,0x36,true);}
    }else {a.at(0x10186240);a.load<1>(0,3,0x1b);a.sign8(0,0);a.compare(0,17);if(!a.nonpositive()){chance_three(a,0x10186250);if(a.equal())decrement_attribute(a,0x101862a8,0x1b,true);}}
}
}
bool apply_attribute_effects(Call& a,std::uint32_t target,std::uint32_t caller) {
    switch(target) {
    case 0x10185e9c:
        if(existing_injury_matches(a,target)){a.load(3,18,0x14);a.load<1>(0,3,0x1b);a.compare(0,20);if(a.equal()){chance_three(a,0x10185edc);if(a.equal())decrement_attribute(a,0x10185f34,0x1b,true);}}break;
    case 0x10185f48:reduce_primary_and_secondary(a);break;
    case 0x101862bc:
        a.at(target);a.load(3,18,0x14);a.load<1>(4,3,0x1d);a.sign8(0,4);a.compare(0,20);
        if(a.equal()){a.offset(0,4,-1);a.store<1>(3,0x1d,0);a.jump(0x10186440);}
        else {a.at(0x101862dc);a.compare(0,15);if(!a.nonpositive()){chance_three(a,0x101862e4);if(a.equal())decrement_attribute(a,0x1018633c,0x1d,true);}}break;
    case 0x10186350:
        a.at(target);a.load(3,18,0x14);a.load<1>(0,3,0x1d);a.sign8(0,0);a.compare(0,20);
        if(a.equal()){chance_three(a,0x10186364);if(!a.equal())decrement_attribute(a,0x101863bc,0x1d,true);}
        else {a.at(0x101863d0);a.compare(0,15);if(!a.nonpositive()){chance_three(a,0x101863d8);if(a.equal())decrement_attribute(a,0x10186430,0x1d,false);}}break;
    case 0x10186440:break;
    default:a.resume(target,caller);return false;
    }
    return true;
}
}
