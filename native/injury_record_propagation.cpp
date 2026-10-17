#include "injury_record.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::injury_record {
namespace {
using Call=SemanticGuestCall;
void advance_risk_pool(Call& a,std::uint32_t start,bool staff=false) {
    a.at(start);a.load(3,28,0);a.load(0,27,0);a.offset(3,3,2);
    if(staff)a.load(4,29,0);
    a.compare_registers(3,0,false);a.store(28,0,3);
    if(staff)a.add(19,4,24);
    if(!a.nonpositive()) {a.store(28,0,26);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(25,0,0);}
}

bool choose_risk_target(Call& a) {
    a.at(0x1018b720);a.load<1>(0,4,0x14);a.sign8(0,0);a.compare(0,-1);if(!a.equal())return false;
    a.load<1>(0,19,0);a.load(3,2,-0x64d4);a.sign8(0,0);a.multiply(0,0,12);a.add(3,3,0);a.load<1>(0,3,9);a.sign8(0,0,true);if(a.equal())return false;
    advance_risk_pool(a,0x1018b750);
    a.at(0x1018b778);a.load(5,28,0);a.value(3,0x66660000);a.load(4,25,0);a.offset(0,3,0x6667);a.load<2>(3,5,0);a.add(4,3,4);
    a.multiply_high(0,0,4);a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,20);a.subtract(0,0,4,true);if(!a.equal())return false;
    a.load<1>(0,19,0);a.value(24,0);a.value(21,0);a.store<1>(23,0x14,0);a.store<1>(23,0x15,24);a.load(23,31,0);a.jump(0x1018b8a4);
    for(;;) {
        a.at(0x1018b8a4);a.load(0,30,0);a.compare_registers(21,0);if(!a.less())break;
        a.at(0x1018b7cc);a.load<1>(0,23,0x14);a.sign8(0,0);a.compare(0,-1);
        if(!a.equal()) {
            advance_risk_pool(a,0x1018b7dc,true);
            a.at(0x1018b80c);a.load(4,28,0);a.value(3,0x288e0000);a.load<1>(0,23,0x15);a.offset(3,3,-0xf35);a.load<2>(5,4,0);
            a.load(4,25,0);a.multiply(0,0,5);a.load(6,19,0x14);a.add(5,5,4);a.multiply_high(3,3,5);a.compare(6,0,false);a.sign8(0,0);
            a.shift_right(3,3,4);a.mask(4,3,1,false,1);a.add(3,3,4);a.multiply(3,3,101);a.subtract(3,3,5);a.sign8(4,3);a.subtract(4,0,4);
            if(!a.equal()) {a.load<1>(0,6,0x2d);a.subtract(0,0,4);a.sign8(4,0);a.jump(0x1018b870);}
            else {a.at(0x1018b86c);a.value(4,100);}
            a.at(0x1018b870);a.sign8(3,4);a.sign8(0,20);a.compare_registers(3,0);
            if(!a.nonpositive()) {a.move(20,4);a.move(22,23);a.move(18,19);}
            a.at(0x1018b88c);a.sign8(0,20);a.compare(0,95);if(a.at_least())break;
        }
        a.at(0x1018b898);a.offset(24,24,0x70);a.offset(21,21,1);a.offset(23,23,0x24);
    }
    a.at(0x1018b8b0);a.compare(22,0,false);return !a.equal();
}

bool can_register_related_injury(Call& a) {
    a.at(0x1018b8b8);a.load(0,18,0x14);a.compare(0,0,false);if(a.equal())return false;
    a.load(3,18,0x24);a.compare(3,0,false);if(a.equal())return false;
    a.load<2>(0,3,0x1ec,true);a.compare(0,4250);if(a.nonpositive())return false;
    a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);if(!a.equal())return false;
    advance_risk_pool(a,0x1018b8f8);
    a.at(0x1018b920);a.load(5,28,0);a.value(3,0x55550000);a.load(4,25,0);a.offset(0,3,0x5556);a.load<2>(3,5,0);a.add(4,3,4);
    a.multiply_high(3,0,4);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);a.subtract(0,0,4,true);if(!a.equal())return false;
    a.compare(18,0,false);return !a.equal();
}
}

void propagate_injury_risk(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,propagation_entry,dependencies,observer);
    a.link();const auto caller=a.r(0);a.multiple(18);a.load(25,2,-0x7b90);a.move(23,4);a.load(26,2,-0x7b8c);a.move(31,3);a.load(27,2,-0x7b88);
    a.move(19,5);a.load(28,2,-0x7b84);a.value(22,0);a.load(29,2,-0x7be0);a.value(20,0);a.load(30,2,-0x7be4);a.value(18,0);a.save_link();a.frame(128);
    if(choose_risk_target(a)) {
        if(can_register_related_injury(a)) {
            a.at(0x1018b958);a.move(3,31);a.move(4,18);a.call(0x1018c2f0);a.sign8(3,3);a.offset(0,3,-11);a.compare(0,65,false);
            std::uint32_t target=0x1018b998;
            if(a.nonpositive()) {a.load(3,2,-0x545c);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);target=a.count_branch();}
            if(target==0x1018b988) {a.at(target);a.value(4,78);a.jump(0x1018b99c);}
            else if(target==0x1018b990) {a.at(target);a.value(4,79);a.jump(0x1018b99c);}
            else if(target==0x1018b998) {a.at(target);a.value(4,77);}
            else {a.resume(target,caller);return;}
            a.at(0x1018b99c);a.value(0,0);a.move(3,31);a.store(1,0x38,0);a.move(6,18);a.value(5,0);a.value(7,0);a.value(8,1);
            a.value(9,0xffffffff);a.value(10,0);a.call(registration_entry);a.jump(0x1018b9d8);
        } else {a.at(0x1018b9c8);a.value(3,0xffffffff);a.value(0,0);a.store<1>(22,0x14,3);a.store<1>(22,0x15,0);}
    }
    a.at(0x1018b9d8);a.pop(128);a.multiple(18,true);a.finish();
}
}
