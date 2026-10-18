#include "injury_maintenance.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::injury_maintenance {
namespace {
using Call=SemanticGuestCall;
void select_minutes(Call& a) {
    a.at(0x10187050);a.load<1>(0,17,0x4c);a.sign8(0,0,true);
    if(!a.equal()){a.sign8(0,22);a.compare(0,11);if(!a.at_least()){a.value(6,90);a.jump(0x10187114);return;}}
    a.at(0x10187070);a.sign8(0,22);a.compare(0,11);
    if(!a.at_least()) {
        a.load<1>(0,23,0x1c8);a.sign8(0,0);a.compare(0,-1);if(!a.nonpositive()){a.move(6,0);a.jump(0x10187114);return;}
        a.at(0x10187094);a.load<1>(0,23,0x1c4);a.sign8(0,0);a.compare(0,-1);if(!a.nonpositive()){a.move(6,0);a.jump(0x10187114);return;}
        a.at(0x101870ac);a.load<2>(0,17,0x4e,true);a.sign8(6,0);a.jump(0x10187114);return;
    }
    a.at(0x101870b8);a.load<1>(3,23,0x1c6);a.sign8(0,3);a.compare(0,-1);
    if(a.equal()){a.value(6,0);a.jump(0x10187114);return;}
    a.at(0x101870d0);a.load<1>(4,23,0x1c8);a.sign8(0,4);a.compare(0,-1);if(!a.nonpositive()){a.subtract(0,3,4);a.sign8(6,0);a.jump(0x10187114);return;}
    a.at(0x101870ec);a.load<1>(4,23,0x1c4);a.sign8(0,4);a.compare(0,-1);if(!a.nonpositive()){a.subtract(0,3,4);a.sign8(6,0);a.jump(0x10187114);return;}
    a.at(0x10187108);a.load<2>(0,17,0x4e,true);a.subtract(0,3,0);a.sign8(6,0);
}
void adjust_match_penalty(Call& a) {
    a.at(0x10187114);a.load<2>(4,23,0x1ca,true);a.value(3,0x51ec0000);a.load<1>(0,16,0x40);a.offset(5,3,-0x7ae1);a.subtract_from(3,4,10000);a.sign8(4,6);a.multiply_high(3,5,3);a.sign8(0,0,true);a.shift_right(0,3,5);a.mask(3,0,1,false,1);a.add(0,0,3);a.add(0,4,0);a.sign16(21,0);
    if(a.equal()){a.value(0,2);a.divide(0,21,0);a.sign16(21,0);a.jump(0x10187268);return;}
    a.at(0x1018715c);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(0x10187184);a.load(3,30,0);a.value(4,0x55550000);a.load<2>(0,19,0x5c,true);a.offset(4,4,0x5556);a.load<2>(6,3,0);a.load(5,27,0);a.sign8(0,0);a.add(6,6,5);a.multiply_high(5,4,6);a.mask(4,5,1,false,1);a.add(4,5,4);a.multiply(4,4,3);a.subtract(4,4,6);a.offset(4,4,33);a.compare_registers(0,4);
    if(!a.nonpositive()){a.exclusive_flags(3,21,0x80000000);a.value(0,0x43300000);a.store(1,0x54,3);a.floating_load(1,26,8);a.store(1,0x50,0);a.floating_load(2,26,0x20);a.floating_load(0,1,0x50);a.floating_subtract(0,0,1);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0x48);a.load(21,1,0x4c);a.jump(0x10187268);return;}
    a.at(0x101871f8);a.load(0,29,0);a.offset(3,3,2);a.store(30,0,3);a.compare_registers(3,0,false);if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(0x1018721c);a.load(4,30,0);a.value(3,0x66660000);a.load<2>(0,19,0x5c,true);a.offset(3,3,0x6667);a.load<2>(5,4,0);a.load(4,27,0);a.sign8(0,0);a.add(5,5,4);a.multiply_high(3,3,5);a.shift_right(3,3,1);a.mask(4,3,1,false,1);a.add(3,3,4);a.multiply(3,3,5);a.subtract(3,3,5);a.subtract_from(3,3,23);a.compare_registers(0,3);if(!a.at_least()){a.mask(0,21,0xfffffffe,false,1);a.sign16(21,0);}
}
void suppress_early_date_penalty(Call& a) {
    a.at(0x10187268);a.load(3,14,0);a.load(0,14,4);a.store(1,0x40,3);a.load<2>(3,31,2);a.load<2>(4,1,0x42);a.store(1,0x44,0);a.compare_registers(4,3,false);
    if(!a.at_least()){a.value(0,1);a.jump(0x101872bc);}
    else {a.at(0x10187290);if(!a.nonpositive()){a.value(0,0);a.jump(0x101872bc);}else {a.at(0x1018729c);a.load<2>(0,1,0x40,true);a.load<2>(4,31,0,true);a.combine(0,4,0,false,true);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);}}
    a.at(0x101872bc);a.sign8(0,0,true);if(!a.equal())a.value(21,0);
}
void process_pending_match_injuries(Call& a) {
    a.at(0x101872dc);a.load<1>(0,23,0x1b3);a.sign8(0,0,true);if(a.equal())return;
    a.load(3,15,0x28);a.compare(3,0,false);if(a.equal())return;
    a.load(0,3,4);a.store(3,12,0);a.jump(0x101873f8);
    while(true) {
        a.at(0x101873f8);a.load(3,15,0x28);a.call(0x10738e00);a.compare(3,0,false);if(a.equal())break;
        a.at(0x10187300);a.load(0,3,0);a.compare_registers(0,20);if(!a.equal())continue;
        a.load<1>(4,3,4);a.compare(4,7);
        if(a.equal()){a.load(5,19,0x14);a.load<1>(0,5,15);a.sign8(0,0);a.compare(0,15);if(a.less())continue;}
        a.at(0x1018732c);a.load<1>(0,3,5);a.sign8(0,0);a.compare(0,95);
        if(!a.nonpositive()){a.value(18,2);a.jump(0x10187360);}
        else {a.at(0x10187344);a.compare(0,65);if(!a.nonpositive()){a.value(18,1);a.jump(0x10187360);}else {a.at(0x10187354);a.compare(0,50);if(!a.nonpositive())a.value(18,0);}}
        a.at(0x10187360);a.move(3,15);a.move(5,18);a.move(6,19);a.call(0x10188930);a.move(21,3);a.sign8(0,21);a.compare(0,-1);if(a.nonpositive())continue;
        a.move(3,15);a.move(4,19);a.value(5,0);a.call(0x1018b9f0);a.load(0,16,0);a.compare(0,-2);
        if(a.equal()){a.value(0,0);a.move(3,15);a.store(1,0x38,0);a.move(4,21);a.move(6,19);a.value(5,0);a.value(7,0);a.value(8,1);a.value(9,0xffffffff);a.value(10,0);a.call(0x10188cc0);a.jump(0x101873f8);}
        else {a.at(0x101873cc);a.value(0,0);a.move(3,15);a.store(1,0x38,0);a.move(4,21);a.move(6,19);a.move(7,16);a.value(5,0);a.value(8,1);a.value(9,0xffffffff);a.value(10,0);a.call(0x10188cc0);}
    }
}
}
void apply_match_physical_state(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,apply_match_entry,d,o);a.link();a.multiple(13);a.load(26,2,-0x543c);a.move(15,3);a.load(27,2,-0x7b90);a.move(16,4);a.load(28,2,-0x7b8c);a.move(17,5);a.load(29,2,-0x7b88);a.value(24,0);a.load(30,2,-0x7b84);a.load(13,2,-0x7be0);a.load(31,2,-0x7a9c);a.load(14,2,-0x7b80);a.save_link();a.value(0,0);a.frame(176);a.store<1>(1,0x58,0);a.jump(0x1018742c);
    while(true) {
        a.at(0x1018742c);a.load<1>(0,1,0x58);a.sign8(0,0);a.compare(0,2);if(!a.less())break;
        a.at(0x1018700c);a.value(22,0);a.value(25,0);a.jump(0x10187410);
        while(true) {
            a.at(0x10187410);a.sign8(0,22);a.compare(0,20);if(!a.less())break;
            a.at(0x10187018);a.add(23,24,25);a.add(23,17,23);a.load(20,23,0x194);a.compare(20,-1);bool active=!a.equal();
            if(active){a.multiply(0,20,0x70);a.load(3,13,0);a.add_record(19,3,0);if(!a.equal()){a.load(3,2,-0x7be4);a.load(4,19,0);a.load(0,3,0);a.compare_registers(4,0);active=!a.at_least();}}
            if(active){select_minutes(a);adjust_match_penalty(a);suppress_early_date_penalty(a);a.at(0x101872c8);a.load<2>(5,23,0x1ca,true);a.move(3,15);a.move(4,19);a.move(6,21);a.call(apply_fitness_entry);process_pending_match_injuries(a);}
            a.at(0x10187408);a.offset(25,25,0x4c);a.offset(22,22,1);
        }
        a.at(0x1018741c);a.load<1>(3,1,0x58);a.offset(24,24,0x664);a.offset(3,3,1);a.store<1>(1,0x58,3);
    }
    a.at(0x1018743c);a.pop(176);a.multiple(13,true);a.finish();
}
}
