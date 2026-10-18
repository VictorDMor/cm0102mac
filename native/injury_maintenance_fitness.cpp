#include "injury_maintenance.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::injury_maintenance {
namespace {
using Call=SemanticGuestCall;
void draw_condition_increment(Call& a) {
    a.at(0x10186b38);a.sign16(28,6,true);
    if(a.equal()){a.value(29,0);a.jump(0x10186c98);return;}
    a.at(0x10186b48);a.load(3,2,-0x7b88);a.load(4,31,0);a.load(0,3,0);a.offset(3,4,2);a.compare_registers(3,0,false);a.store(31,0,3);
    if(!a.nonpositive()){a.load(0,2,-0x7b8c);a.store(31,0,0);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(30,0,0);}
    a.at(0x10186b78);a.value(3,0x10000);a.offset(0,3,-1);a.compare_registers(28,0);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(0,3,1);a.compare_registers(28,0);wide=!a.at_least();}
    if(wide) {
        a.at(0x10186b98);a.value(4,0x43300000);a.exclusive_flags(0,28,0x80000000);a.store(1,0x3c,0);a.value(5,0x80010000);a.load(7,31,0);a.value(3,0x10000);a.store(1,0x38,4);a.offset(6,5,-0x7fff);a.floating_load(2,29,8);a.multiply(0,28,10);a.floating_load(1,1,0x38);a.offset(5,3,-1);a.floating_load(0,29,0);a.floating_subtract(1,1,2);a.multiply_high(3,6,0);a.load<2>(8,7,0);a.load(7,30,0);a.floating_difference(1,1,0,true);a.store(1,0x40,4);a.add(7,8,7);a.multiply_high(4,6,7);a.add(0,3,0);a.add(3,4,7);a.shift_right(3,3,15);a.mask(4,3,1,false,1);a.shift_right(0,0,15);a.add(3,3,4);a.product(4,3,5);a.mask(3,0,1,false,1);a.add(3,0,3);a.subtract(0,4,7);a.exclusive_flags(0,0,0x80000000);a.store(1,0x44,0);a.floating_load(0,1,0x40);a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x48);a.load(29,1,0x4c);a.call(0x101864a0);a.value(4,0x66660000);a.offset(0,3,5);a.offset(3,4,0x6667);a.offset(4,28,-1);a.multiply_high(0,3,0);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.add(29,29,0);a.compare_registers(29,4);if(!a.nonpositive()){a.move(29,4);a.jump(0x10186c98);}
    }else {a.at(0x10186c6c);a.load(3,31,0);a.compare(28,0);a.load(0,30,0);a.load<2>(3,3,0);a.add(3,3,0);a.divide(0,3,28);a.product(0,0,28);a.subtract(0,0,3);a.negate(29,0);if(!a.nonpositive())a.move(29,0);}
}
}
void apply_staff_fitness(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,apply_fitness_entry,d,o);a.link();a.multiple(26);a.move(27,4,true);a.load(29,2,-0x543c);a.load(30,2,-0x7b90);a.move(26,3);a.load(31,2,-0x7b84);a.save_link();a.frame(112);bool active=!a.equal();
    if(active){a.at(0x10186a48);if(!a.equal()){a.load(3,2,-0x7be4);a.load(4,27,0);a.load(0,3,0);a.compare_registers(4,0);active=!a.at_least();}}
    if(active) {
        a.at(0x10186a60);a.load(0,27,0);a.load(3,26,0);a.multiply(0,0,0x24);a.add(3,3,0);a.store<2>(3,10,5);a.load(0,27,0);a.load(3,26,0);a.multiply(0,0,0x24);a.add(7,3,0);a.load<2>(5,7,12,true);a.compare(5,-1000);
        if(!a.at_least()) {
            a.value(3,0x43300000);a.exclusive_flags(0,5,0x80000000);a.store(1,0x4c,0);a.sign16(0,6);a.exclusive_flags(0,0,0x80000000);a.floating_load(3,29,8);a.store(1,0x48,3);a.floating_load(4,29,0x10);a.floating_load(0,1,0x48);a.store(1,0x44,0);a.floating_subtract(2,0,3);a.floating_load(1,29,0x18);a.store(1,0x40,3);a.floating_load(0,1,0x40);a.floating_product_difference(1,4,2,1);a.floating_subtract(0,0,3);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x38);a.load(0,1,0x3c);a.add(0,5,0);a.store<2>(7,12,0);a.jump(0x10186cbc);
        }else {
            a.at(0x10186aec);a.load(3,27,0x14);a.load<1>(4,27,0x67);a.load<1>(0,3,0x44);a.load<1>(3,3,0x3c);a.sign8(4,4);a.sign8(0,0);a.sign8(3,3);a.add(0,0,4);a.add(0,3,0);a.sign16(0,0);a.subtract_from(0,0,1200);a.compare_registers(5,0);
            if(!a.at_least()){a.sign16(0,6);a.add(0,5,0);a.store<2>(7,12,0);a.jump(0x10186cbc);}
            else {a.at(0x10186b30);a.compare(5,1250);if(!a.at_least()){draw_condition_increment(a);a.at(0x10186c98);a.load(3,27,0);a.offset(0,29,1);a.load(4,26,0);a.sign16(0,0);a.multiply(3,3,0x24);a.add(4,4,3);a.load<2>(3,4,12,true);a.add(0,3,0);a.store<2>(4,12,0);}}
        }
    }
    a.at(0x10186cbc);a.pop(112);a.multiple(26,true);a.finish();
}
}
