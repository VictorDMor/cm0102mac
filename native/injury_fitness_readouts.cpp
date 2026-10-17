#include "injury_fitness.hpp"
#include "injury_fitness_detail.hpp"

namespace cm::aot::injury_fitness {
using namespace detail;
void classify_injury_recovery(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,recovery_phase_entry,d,o);
    a.link();a.save(31);a.load(31,2,-0x5438);a.save(30);a.load(30,2,-0x7ba8);a.save(29);a.move(29,4,true);a.save(28);a.move(28,3);a.save_link();a.frame(128);
    bool finished=false;
    if(a.equal()){report_missing_staff(a);finished=true;}
    else {
        a.at(0x1018770c);if(!a.equal()){a.load(3,2,-0x7be4);a.load(4,29,0);a.load(0,3,0);a.compare_registers(4,0);if(!a.less()){a.value(3,0);a.jump(0x101877bc);finished=true;}}
        if(!finished) {
            a.at(0x1018772c);a.move(3,28);a.move(4,29);a.call(refresh_entry);a.load(0,29,0);a.load(3,28,0);a.multiply(0,0,0x24);a.add(3,3,0);a.load(0,3,0x10);a.compare(0,-1);
            if(!a.equal()) {
                a.multiply(0,0,0x38);a.load(5,28,4);a.load(4,2,-0x7b80);a.offset(3,1,0x38);a.add(30,5,0);a.load(5,30,0x28);a.load(6,30,0x2c);a.call(0x10717dd0);a.load<2>(0,1,0x3a);a.load<2>(4,1,0x38,true);a.multiply(3,0,365);a.load<2>(0,30,0x32,true);a.add(3,4,3);a.compare_registers(3,0);
                if(!a.at_least()){a.value(3,2);a.jump(0x101877bc);}
                else {a.at(0x10187798);a.load<2>(4,30,0x30,true);a.combine(0,4,3,false,true);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(3,0);a.jump(0x101877bc);}
            }else {a.at(0x101877b8);a.value(3,0);}
        }
    }
    a.at(0x101877bc);a.pop(128);a.restore(31);a.restore(30);a.restore(29);a.restore(28);a.finish();
}
void read_staff_fitness(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,fitness_read_entry,d,o);
    a.link();a.save(31);a.move(31,5);a.save(30);a.move(30,4,true);a.save(29);a.move(29,3);a.save_link();a.frame(96);
    bool finished=false;
    if(a.equal()){a.value(3,0);a.jump(0x101878b4);finished=true;}
    else {
        a.at(0x10187810);if(!a.equal()) {a.load(3,2,-0x7be4);a.load(4,30,0);a.load(0,3,0);a.compare_registers(4,0);if(!a.less()){a.sign8(0,31,true);a.value(3,10000);if(!a.equal()){a.value(3,100);a.jump(0x101878b4);}finished=true;}}
        if(!finished) {
            a.at(0x1018783c);a.move(3,29);a.move(4,30);a.call(refresh_entry);a.sign8(0,31,true);
            if(!a.equal()) {a.load(3,30,0);a.value(0,0x43300000);a.load(4,2,-0x543c);a.multiply(5,3,0x24);a.load(6,29,0);a.load(3,2,-0x5444);a.store(1,0x40,0);a.add(5,6,5);a.floating_load(2,4,8);a.load<2>(0,5,10,true);a.floating_load(0,3,0,true);a.exclusive_flags(0,0,0x80000000);a.store(1,0x44,0);a.floating_load(1,1,0x40);a.floating_subtract(1,1,2,true);a.floating_difference(0,1,0,true,true);a.floating_integer(0,0);a.floating_store(0,1,0x38);a.load(3,1,0x3c);a.jump(0x101878b4);}
            else {a.at(0x101878a0);a.load(0,30,0);a.load(3,29,0);a.multiply(0,0,0x24);a.add(3,3,0);a.load<2>(3,3,10,true);}
        }
    }
    a.at(0x101878b4);a.pop(96);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
void read_effective_fitness(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,effective_fitness_entry,d,o);
    a.link();a.save(31);a.move(31,5);a.save(30);a.move(30,4,true);a.save(29);a.move(29,3);a.save_link();a.frame(96);
    bool finished=false;
    if(a.equal()){a.value(3,0);a.jump(0x10187a00);finished=true;}
    else {
        a.at(0x10187900);if(!a.equal()){a.load(3,2,-0x7be4);a.load(4,30,0);a.load(0,3,0);a.compare_registers(4,0);if(!a.less()){a.sign8(0,31,true);a.value(3,10000);if(!a.equal()){a.value(3,100);a.jump(0x10187a00);}finished=true;}}
        if(!finished) {
            a.at(0x1018792c);a.move(3,29);a.move(4,30);a.call(recovery_phase_entry);a.load(5,30,0);a.value(4,0x68dc0000);a.sign8(0,3);a.load(6,29,0);a.multiply(3,5,0x24);a.offset(5,4,-0x7453);a.compare(0,1);a.add(4,6,3);a.load<2>(3,4,8,true);a.load<2>(0,4,10,true);a.product(0,3,0);a.multiply_high(0,5,0);a.shift_right(0,0,12);a.mask(3,0,1,false,1);a.add(0,0,3);a.sign16(3,0);
            if(a.equal()){a.compare(3,9000);a.value(0,9000);if(!a.at_least())a.move(0,3);a.at(0x1018798c);a.sign16(3,0);}
            a.at(0x10187990);a.sign16(0,3);a.value(3,100);a.compare(0,100);if(!a.nonpositive())a.move(3,0);
            a.at(0x101879a4);a.sign8(0,31,true);a.sign16(3,3);
            if(!a.equal()) {a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.load(4,2,-0x543c);a.store(1,0x44,3);a.floating_load(2,4,8);a.store(1,0x40,0);a.load(3,2,-0x5444);a.floating_load(1,1,0x40);a.floating_load(0,3,0,true);a.floating_subtract(1,1,2,true);a.floating_load(2,4,0x28);a.floating_difference(0,1,0,true,true);a.floating_sum(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0x38);a.load(3,1,0x3c);a.sign16(0,3);a.compare(0,100);if(!a.nonpositive())a.value(3,100);}
        }
    }
    a.at(0x10187a00);a.pop(96);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
}
