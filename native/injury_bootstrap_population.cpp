#include "injury_bootstrap.hpp"
#include "injury_bootstrap_detail.hpp"

namespace cm::aot::injury_bootstrap {
using namespace detail;
namespace {
void refresh_random_seed(Call& a) {
    a.store(25,0,23);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(22,0,0);
}
void choose_initial_risk_type(Call& a) {
    a.at(0x10188714);a.load(3,25,0);a.load(0,24,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(25,0,3);if(!a.nonpositive())refresh_random_seed(a);
    a.at(0x1018873c);a.load(3,25,0);a.value(4,0x63e70000);a.load(5,22,0);a.offset(0,4,0x63f);a.load<2>(4,3,0);a.add(5,4,5);a.multiply_high(0,0,5);a.shift_right(0,0,5);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,82);a.subtract(0,0,5);a.sign8(16,0);a.multiply(4,16,12);a.offset(0,4,9);a.indexed_load<1>(0,21,0);a.sign8(0,0,true);
    if(!a.equal()) {
        a.load(0,24,0);a.offset(3,3,2);a.store(25,0,3);a.compare_registers(3,0,false);if(!a.nonpositive())refresh_random_seed(a);
        a.at(0x101887a8);a.load(5,25,0);a.value(3,0x66660000);a.load(4,22,0);a.offset(0,3,0x6667);a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(0,0,4);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,10);a.subtract(0,0,4,true);
        if(a.equal()){a.store<1>(19,0x14,16);a.offset(31,31,1);}
    }
}
void choose_initial_injury(Call& a) {
    a.at(0x101887e4);a.load(3,25,0);a.load(0,24,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(25,0,3);if(!a.nonpositive())refresh_random_seed(a);
    a.at(0x1018880c);a.load(5,25,0);a.value(3,0x51ec0000);a.load(4,22,0);a.offset(0,3,-0x7ae1);a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(0,0,4);a.shift_right(0,0,5);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,100);a.subtract(0,0,4,true);
    if(a.equal()) {
        a.load(0,24,0);a.offset(3,5,2);a.store(25,0,3);a.compare_registers(3,0,false);if(!a.nonpositive())refresh_random_seed(a);
        a.at(0x10188864);a.load(5,25,0);a.value(3,0x2aab0000);a.load(4,22,0);a.offset(0,3,-0x5555);a.load<2>(7,5,0);a.move(3,29);a.move(6,18);a.value(5,0xffffffff);a.add(7,7,4);a.multiply_high(0,0,7);a.shift_right(0,0,1);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,12);a.subtract(0,0,7);a.sign8(4,0);a.call(0x10188930);
        a.value(0,0);a.move(4,3);a.store(1,0x38,0);a.move(3,29);a.move(6,18);a.value(5,1);a.value(7,0);a.value(8,1);a.value(9,0xffffffff);a.value(10,0);a.call(0x10188cc0);a.offset(30,30,1);
    }
}
}
void populate_staff_records(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,population_entry,d,o);
    a.link();a.multiple(16);a.load(26,2,-0x7be4);a.move(29,3);a.load(20,2,-0x7bb8);a.value(31,0);a.load(21,2,-0x64d4);a.value(30,0);a.load(22,2,-0x7b90);a.load(23,2,-0x7b8c);a.load(24,2,-0x7b88);a.load(25,2,-0x7b84);a.load(27,2,-0x7b80);a.load(16,2,-0x7ba8);a.load(28,2,-0x5438);a.save_link();a.frame(176);
    a.load(0,26,0);a.multiply(3,0,0x24);a.call(0x10775130);a.store(29,0,3);a.load(0,29,0);a.compare(0,0,false);
    if(a.equal()) {
        report_allocation_failure(a,0x10188650,28,16,0x48,0x40,0x7b9);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);a.jump(0x10188910);
    } else {
        a.at(0x101886a8);a.load(3,2,-0x7be0);a.move(19,0);a.value(17,0);a.load(18,3,0);a.jump(0x101888e4);
        while(true) {
            a.at(0x101888e4);a.load(0,26,0);a.compare_registers(17,0);if(!a.less())break;
            a.at(0x101886bc);a.load<2>(0,27,2);a.value(5,10000);a.value(4,0);a.value(3,0xffffffff);a.store<2>(19,2,0);a.value(0,50);a.load<2>(6,27,0,true);a.store<2>(19,0,6);a.load<1>(6,27,4);a.store<1>(19,4,6);a.store<2>(19,8,5);a.store<2>(19,10,5);a.store<2>(19,12,4);a.store(19,16,3);a.store(19,24,3);a.store(19,28,3);a.store<1>(19,20,3);a.store<1>(19,33,3);a.store<1>(19,34,0);a.load(0,18,20);a.compare(0,0,false);
            if(!a.equal()){choose_initial_risk_type(a);choose_initial_injury(a);}
            a.at(0x101888d8);a.offset(17,17,1);a.offset(18,18,0x70);a.offset(19,19,0x24);
        }
        a.at(0x101888f0);a.move(3,20);a.move(5,31);a.offset(4,28,0x7c);a.call(0x1077b720);a.move(3,20);a.move(5,30);a.offset(4,28,0x9f);a.call(0x1077b720);
    }
    a.at(0x10188910);a.pop(176);a.multiple(16,true);a.finish();
}
}
