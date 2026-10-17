#include "injury_bootstrap.hpp"
#include "injury_bootstrap_detail.hpp"

namespace cm::aot::injury_bootstrap {
using namespace detail;
void initialize_club_thresholds(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,club_thresholds_entry,d,o);
    a.link();a.multiple(25);a.load(28,2,-0x7b04);a.move(25,3);a.load(29,2,-0x7b50);a.load(30,2,-0x7be4);a.load(31,2,-0x7b80);a.save_link();a.frame(112);
    a.offset(3,1,0x40);a.call(0x10715880);a.offset(3,1,0x38);a.call(0x10715880);a.load(3,2,-0x7be8);a.value(27,0);a.load(26,3,0);a.jump(0x101844fc);
    while(true) {
        a.at(0x101844fc);a.load(3,28,0);a.load(0,29,0);a.mask(3,3,0xfffffffe,false,1);a.subtract(0,3,0);a.compare_registers(27,0);if(!a.less())break;
        a.at(0x10184414);a.move(3,26);a.offset(4,1,0x40);a.offset(5,1,0x38);a.call(0x101929b0);a.sign8(0,3,true);
        if(a.equal()) {
            a.load(4,31,0);a.offset(3,1,0x40);a.load(5,31,4);a.call(0x10718130);a.compare(3,80);if(!a.nonpositive())a.value(3,40);
            a.at(0x10184448);a.compare(3,40);if(!a.nonpositive()){a.subtract_from(0,3,80);a.sign8(3,0);a.jump(0x10184460);}
        }else{a.at(0x1018445c);a.value(3,0);}
        a.at(0x10184460);a.offset(5,26,0x98);a.value(4,0);a.jump(0x101844e8);
        while(true) {
            a.at(0x101844e8);a.sign8(0,4);a.compare(0,50);if(!a.less())break;
            a.at(0x10184470);a.load(6,5,0);a.compare(6,0,false);
            if(!a.equal()) {
                a.load(8,6,0);a.load(0,30,0);a.compare_registers(8,0);
                if(!a.at_least()) {
                    a.load(6,6,0x14);a.load<1>(0,6,0x34);a.sign8(6,0);a.mask(0,6,1,false,1);a.add(0,0,6);a.shift_right(0,0,1);a.subtract(6,0,3);a.compare(6,1);
                    if(!a.at_least()){a.value(0,1);a.jump(0x101844c8);}
                    else {a.at(0x101844b8);a.compare(6,40);a.value(0,40);if(!a.positive())a.move(0,6);}
                    a.at(0x101844c8);a.multiply(0,0,100);a.load(7,25,0);a.multiply(6,8,0x24);a.subtract_from(8,0,10000);a.offset(0,6,8);a.indexed_store<2>(8,7,0);
                }
            }
            a.at(0x101844e0);a.offset(4,4,1);a.offset(5,5,4);
        }
        a.at(0x101844f4);a.offset(27,27,1);a.offset(26,26,0x248);
    }
    a.at(0x10184514);a.pop(112);a.multiple(25,true);a.finish();
}

void staff_availability_gate(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,availability_entry,d,o);
    a.link();a.save(31);a.load(31,2,-0x7b80);a.save(30);a.move(30,4);a.save(29);a.load(29,2,-0x7a64);a.save(28);a.value(28,0);a.save_link();a.frame(96);
    a.offset(3,1,0x48);a.call(0x10715880);a.offset(3,1,0x40);a.call(0x10715880);a.compare(30,0,false);
    if(a.equal()){a.value(3,1);a.jump(0x101885d0);}
    else {
        a.at(0x101884dc);a.load(4,30,0x20);a.compare(4,0,false);
        if(!a.equal()) {
            a.load<1>(0,30,0x6a);a.load(3,29,0);a.mask(0,0,8);a.sign8(5,0);a.call(0x102ae9c0);a.load(4,30,0x20);a.move(5,3);a.load(3,29,0);a.load(4,4,0x2c);a.call(0x102af650);
            a.load(4,3,0);a.load(0,3,4);a.store(1,0x38,4);a.load<2>(3,31,2);a.load<2>(4,1,0x3a);a.store(1,0x3c,0);a.compare_registers(4,3,false);
            if(!a.nonpositive()){a.value(0,1);a.jump(0x10188564);}
            else {a.at(0x10188538);if(!a.at_least()){a.value(0,0);a.jump(0x10188564);}
                else {a.at(0x10188544);a.load<2>(4,1,0x38,true);a.load<2>(0,31,0,true);a.combine(0,4,0,false,true);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);}}
            a.at(0x10188564);a.sign8(28,0);
        }
        a.at(0x10188568);a.load(3,30,0x24);a.compare(3,0,false);bool available=false;
        if(!a.equal()){a.sign8(0,28,true);if(a.equal()){a.load(0,3,0x14);a.compare(0,-1);if(a.equal()) {
            a.offset(4,1,0x48);a.offset(5,1,0x40);a.call(0x101929b0);a.sign8(0,3);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0,true);
            if(!a.equal()){a.load(4,1,0x40);a.move(3,31);a.load(5,1,0x44);a.call(0x10718130);a.mask(0,3,0xffff);a.compare(0,40,false);if(!a.at_least()){a.value(3,1);a.jump(0x101885d0);available=true;}}
        }}}
        if(!available){a.at(0x101885cc);a.value(3,0);}
    }
    a.at(0x101885d0);a.pop(96);a.restore(31);a.restore(30);a.restore(29);a.restore(28);a.finish();
}
}
