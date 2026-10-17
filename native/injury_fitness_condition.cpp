#include "injury_fitness.hpp"
#include "injury_fitness_detail.hpp"

namespace cm::aot::injury_fitness {
using namespace detail;
void update_staff_condition(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,condition_update_entry,d,o);
    a.link();a.multiple(24);a.load(5,2,-0x7be4);a.move(24,3);a.load(27,2,-0x7b90);a.load(28,2,-0x7b8c);a.load(29,2,-0x7b88);a.load(30,2,-0x7b84);a.load(31,2,-0x7be0);a.save_link();a.frame(96);
    a.load(0,5,0);a.compare_registers(4,0);
    if(!a.at_least()) {
        a.multiply(26,4,0x24);a.load(0,24,0);a.add(5,0,26);a.load<2>(3,5,12,true);a.compare(3,-1000);
        if(!a.at_least()){a.offset(0,3,-1);a.store<2>(5,12,0);}
        a.at(0x101874a8);a.multiply(25,4,0x70);a.load(3,31,0);a.offset(0,25,0x5c);a.indexed_load<2>(0,3,0,true);a.sign8(0,0);a.compare(0,19);
        bool done=false;
        if(!a.at_least()) {
            a.load(0,24,0);a.add(3,0,26);a.load<2>(0,3,12,true);a.compare(0,0);
            if(!a.at_least()){a.value(0,0);a.store<2>(3,12,0);a.jump(0x101875f0);done=true;}
        }
        if(!done) {
            a.at(0x101874e4);a.load(0,24,0);a.add(3,0,26);a.load<2>(0,3,12,true);a.compare(0,-500);bool random_decrease=a.less();
            if(!random_decrease) {
                advance_random_input(a,0x101874f8,27,28,29,30);a.load(5,30,0);a.value(3,0x51ec0000);a.load(4,27,0);a.offset(0,3,-0x7ae1);a.load<2>(3,5,0);a.offset(5,25,0x5c);a.load(6,31,0);a.add(4,3,4);a.multiply_high(0,0,4);a.indexed_load<2>(3,6,5,true);a.sign8(5,3);a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,25);a.subtract(0,0,4);a.compare_registers(5,0);random_decrease=!a.at_least();
            }
            if(random_decrease) {
                advance_random_input(a,0x10187568,27,28,29,30);a.load(5,30,0);a.value(3,0x4ec50000);a.load(0,24,0);a.offset(4,3,-0x13b1);a.load<2>(6,5,0);a.load(5,27,0);a.add(3,0,26);a.load<2>(0,3,12,true);a.add(6,6,5);a.multiply_high(4,4,6);a.shift_right(4,4,2);a.mask(5,4,1,false,1);a.add(4,4,5);a.multiply(4,4,13);a.subtract(4,4,6);a.sign16(4,4);a.subtract(0,4,0);a.store<2>(3,12,0);a.jump(0x101875f0);
            } else {a.at(0x101875dc);a.load(0,24,0);a.add(4,0,26);a.load<2>(3,4,12,true);a.offset(0,3,-13);a.store<2>(4,12,0);}
        }
        a.at(0x101875f0);a.load(0,31,0);a.add(3,0,25);a.load(3,3,0x14);a.compare(3,0,false);
        if(!a.equal()) {a.load<1>(0,3,15);a.compare(0,20);if(a.equal()){a.load(0,24,0);a.value(3,1050);a.add(4,0,26);a.load<2>(0,4,12,true);a.compare(0,1050);if(!a.at_least())a.move(3,0);a.at(0x1018762c);a.store<2>(4,12,3);}}
    }
    a.at(0x10187630);a.pop(96);a.multiple(24,true);a.finish();
}
void read_staff_condition(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,condition_read_entry,d,o);a.load(5,2,-0x7be4);a.load(0,5,0);a.compare_registers(4,0);
    if(!a.less()){a.value(0,0);a.jump(0x10187678);}else {a.at(0x10187668);a.multiply(0,4,0x24);a.load(3,3,0);a.add(3,3,0);a.load<2>(0,3,12,true);}
    a.at(0x10187678);a.sign16(3,0);a.finish();
}
}
