#include "injury_random.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::injury_random {
namespace {
using Call=SemanticGuestCall;
void advance_pool(Call& a,std::uint32_t start,bool cached) {
    a.at(start);if(!cached)a.load(3,29,0);a.load(0,28,0);a.offset(3,3,2);
    if(cached){a.store(29,0,3);a.compare_registers(3,0,false);}else {a.compare_registers(3,0,false);a.store(29,0,3);}
    if(!a.nonpositive()){a.store(29,0,27);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(26,0,0);}
}
bool wide_bound(Call& a,unsigned bound,std::uint32_t start) {
    a.at(start);a.value(3,0x10000);a.offset(0,3,-1);a.compare_registers(bound,0);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(0,3,1);a.compare_registers(bound,0);wide=!a.at_least();}return wide;
}
void small_draw(Call& a,unsigned bound,unsigned result,std::uint32_t start) {
    a.at(start);a.load(3,29,0);a.compare(bound,0);a.load(0,26,0);a.load<2>(3,3,0);a.add(3,3,0);a.divide(0,3,bound);a.product(0,0,bound);a.subtract(0,0,3);a.negate(result,0);if(!a.nonpositive())a.move(result,0);
}
template<unsigned Bound,unsigned ChildBound,unsigned Result,bool First>
void scaled_draw(Call& a,std::uint32_t start) {
    constexpr unsigned input=First?0x48:0x38,output=First?0x38:0x48;
    a.at(start);a.value(9,0x43300000);a.exclusive_flags(0,Bound,0x80000000);a.store(1,input+4,0);a.value(5,0x80010000);a.load(3,29,0);a.value(4,0x10000);a.store(1,input,9);a.offset(6,5,-0x7fff);a.floating_load(2,25,8);a.multiply(0,Bound,10);a.floating_load(1,1,input);a.offset(5,4,-1);a.floating_load(0,25,0);a.floating_subtract(1,1,2);a.multiply_high(4,6,0);a.load<2>(8,3,0);a.load(7,26,0);a.floating_difference(1,1,0,true);a.store(1,0x40,9);a.add(7,8,7);a.multiply_high(6,6,7);a.add(0,4,0);a.add(4,6,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.shift_right(0,0,15);a.add(4,4,6);a.product(5,4,5);a.mask(4,0,1,false,1);a.add_record(ChildBound,0,4);a.subtract(0,5,7);a.exclusive_flags(0,0,0x80000000);a.store(1,0x44,0);a.floating_load(0,1,0x40);a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,output);a.load(Result,1,output+4);
}
void correction(Call& a,unsigned bound,unsigned result,std::uint32_t start,std::uint32_t join) {
    a.at(start);a.value(4,0x66660000);a.offset(0,3,5);a.offset(3,4,0x6667);a.offset(4,bound,-1);a.multiply_high(0,3,0);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.add(result,result,0);a.compare_registers(result,4);if(!a.nonpositive()){a.move(result,4);a.jump(join);}
}
void deepest_draw(Call& a) {
    advance_pool(a,0x1018679c,true);
    if(wide_bound(a,20,0x101867c0)) {
        a.at(0x101867e0);a.value(4,0x43300000);a.exclusive_flags(0,20,0x80000000);a.store(1,0x3c,0);a.value(5,0x80010000);a.load(7,29,0);a.value(3,0x10000);a.store(1,0x38,4);a.offset(6,5,-0x7fff);a.floating_load(2,25,8);a.multiply(0,20,10);a.floating_load(1,1,0x38);a.offset(5,3,-1);a.floating_load(0,25,0);a.floating_subtract(1,1,2);a.multiply_high(3,6,0);a.load<2>(8,7,0);a.load(7,26,0);a.floating_difference(1,1,0,true);a.store(1,0x40,4);a.add(7,8,7);a.multiply_high(4,6,7);a.add(0,3,0);a.add(3,4,7);a.shift_right(3,3,15);a.mask(4,3,1,false,1);a.shift_right(0,0,15);a.add(3,3,4);a.product(4,3,5);a.mask(3,0,1,false,1);a.add(3,0,3);a.subtract(0,4,7);a.exclusive_flags(0,0,0x80000000);a.store(1,0x44,0);a.floating_load(0,1,0x40);a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x48);a.load(25,1,0x4c);a.call(entry);correction(a,20,25,0x10186880,0x101868e0);
    }else small_draw(a,20,25,0x101868b4);
}
void third_draw(Call& a) {
    advance_pool(a,0x101866b0,true);
    if(wide_bound(a,21,0x101866d4)) {
        scaled_draw<21,20,23,false>(a,0x101866f4);
        if(a.equal()){a.value(25,0);a.jump(0x101868e0);}else deepest_draw(a);
        a.at(0x101868e0);a.value(3,0x66660000);a.offset(0,25,5);a.offset(3,3,0x6667);a.offset(4,21,-1);a.multiply_high(0,3,0);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.add(23,23,0);a.compare_registers(23,4);if(!a.nonpositive()){a.move(23,4);a.jump(0x10186940);}
    }else small_draw(a,21,23,0x10186914);
}
void second_draw(Call& a) {
    advance_pool(a,0x101865c4,true);
    if(wide_bound(a,22,0x101865e8)) {
        scaled_draw<22,21,24,false>(a,0x10186608);
        if(a.equal()){a.value(23,0);a.jump(0x10186940);}else third_draw(a);
        a.at(0x10186940);a.value(3,0x66660000);a.offset(0,23,5);a.offset(3,3,0x6667);a.offset(4,22,-1);a.multiply_high(0,3,0);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.add(24,24,0);a.compare_registers(24,4);if(!a.nonpositive()){a.move(24,4);a.jump(0x101869a0);}
    }else small_draw(a,22,24,0x10186974);
}
}
void draw_signed_bounded(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,entry,d,o);a.link();a.multiple(20);a.move(30,3,true);a.load(25,2,-0x543c);a.load(26,2,-0x7b90);a.load(27,2,-0x7b8c);a.load(28,2,-0x7b88);a.load(29,2,-0x7b84);a.save_link();a.frame(128);
    if(a.equal()){a.value(3,0);a.jump(0x10186a04);}
    else {
        advance_pool(a,0x101864d4,false);
        if(wide_bound(a,30,0x101864fc)) {
            scaled_draw<30,22,31,true>(a,0x1018651c);
            if(a.equal()){a.value(24,0);a.jump(0x101869a0);}else second_draw(a);
            a.at(0x101869a0);a.value(3,0x66660000);a.offset(4,24,5);a.offset(3,3,0x6667);a.offset(0,30,-1);a.multiply_high(3,3,4);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(31,31,3);a.compare_registers(31,0);if(!a.nonpositive())a.move(31,0);a.at(0x101869d0);a.move(3,31);a.jump(0x10186a04);
        }else small_draw(a,30,3,0x101869d8);
    }
    a.at(0x10186a04);a.pop(128);a.multiple(20,true);a.finish();
}
}
