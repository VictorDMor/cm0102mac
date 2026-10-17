#include "injury_readouts.hpp"
#include "injury_readouts_detail.hpp"

namespace cm::aot::injury_readouts {
namespace {
using namespace detail;
bool valid_risk_actor(Call& a,std::uint32_t profile) {
    a.compare(4,0,false);if(a.equal())return false;
    bool valid=a.equal();
    if(!valid){a.load(5,2,-0x7be4);a.load(6,4,0);a.load(0,5,0);a.compare_registers(6,0);valid=a.less();}
    if(!valid)return false;
    a.at(profile);a.load(0,4,0x14);a.compare(0,0,false);return !a.equal();
}
}

void find_history_record(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,history_entry,d,o);
    a.link();a.save(31);a.compare(4,0,false);a.load(31,2,-0x5438);a.save(30);a.load(30,2,-0x7ba8);a.save_link();a.frame(112);
    if(a.equal()) {
        report_invalid(a,0x1018c1c4,31,30,0x40,0x38,0xd71);a.load(4,2,-0x7bac);a.value(0,0);a.value(3,0);a.store<1>(4,0,0);a.jump(0x1018c254);
    } else {
        a.at(0x1018c220);a.load(5,3,12);a.compare(5,0,false);
        if(!a.equal()) {
            a.load(0,4,0);a.load(4,3,0);a.move(3,5);a.multiply(0,0,0x24);a.add(5,4,0);a.load(4,5,0x18);a.load(5,5,0x1c);a.call(0x10749fd0);a.jump(0x1018c254);
        } else {a.at(0x1018c250);a.value(3,0);}
    }
    a.at(0x1018c254);a.pop(112);a.restore(31);a.restore(30);a.finish();
}

void risk_multiplier(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,risk_multiplier_entry,d,o);
    if(valid_risk_actor(a,0x1018c290)) {
        a.at(0x1018c29c);a.load(0,4,0);a.load(3,3,0);a.multiply(0,0,0x24);a.add(3,3,0);a.load<1>(0,3,0x14);a.sign8(0,0);a.compare(0,-1);
        if(!a.nonpositive()) {
            a.load<1>(0,3,0x15);a.value(3,1);a.sign8(0,0);a.compare(0,1);if(!a.nonpositive())a.move(3,0);
            a.at(0x1018c2d4);a.sign8(3,3);a.finish();return;
        }
    }
    a.at(0x1018c2dc);a.value(3,0);a.finish();
}

void risk_type(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,risk_type_entry,d,o);
    if(valid_risk_actor(a,0x1018c310)){a.at(0x1018c31c);a.load(0,4,0);a.load(3,3,0);a.multiply(0,0,0x24);a.add(3,3,0);a.load<1>(3,3,0x14);a.finish();return;}
    a.at(0x1018c334);a.value(3,0xffffffff);a.finish();
}

void describe_current_injury(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,current_text_entry,d,o);
    a.link();a.multiple(25);a.move(26,4,true);a.load(30,2,-0x7ba8);a.load(31,2,-0x5438);a.move(25,3);a.move(27,5);a.move(28,6);a.move(29,7);a.save_link();a.frame(128);
    if(a.equal()) {
        report_invalid(a,0x1018c480,31,30,0x40,0x38,0xe4c);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);a.jump(0x1018c54c);
    } else {
        a.at(0x1018c4d8);bool valid=a.equal();
        if(!valid){a.load(3,2,-0x7be4);a.load(4,26,0);a.load(0,3,0);a.compare_registers(4,0);valid=a.less();}
        if(valid) {
            a.at(0x1018c4f0);a.move(3,25);a.move(4,26);a.call(0x10187a40);a.load(0,26,0);a.load(3,25,0);a.multiply(0,0,0x24);a.add(3,3,0);a.load(0,3,0x10);a.compare(0,-1);
            if(a.equal()){a.move(3,27);a.offset(4,31,0xd7);a.offset(5,31,0xf6);a.call(0x10727890);a.jump(0x1018c54c);}
            else {a.at(0x1018c52c);a.multiply(0,0,0x38);a.load(4,25,4);a.move(3,25);a.move(5,27);a.indexed_load<1>(4,4,0);a.move(6,28);a.move(7,29);a.call(type_text_entry);}
        }
    }
    a.at(0x1018c54c);a.pop(128);a.multiple(25,true);a.finish();
}

void describe_remaining_time(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,remaining_text_entry,d,o);
    a.link();a.multiple(24);a.move(25,4,true);a.load(26,2,-0x7ba8);a.load(27,2,-0x5438);a.move(24,3);a.move(28,5);a.move(29,6);a.move(30,7);a.move(31,8);a.save_link();a.frame(144);
    if(a.equal()) {
        report_invalid(a,0x1018d984,27,26,0x48,0x40,0x1056);a.load(4,2,-0x7bac);a.value(0,0);a.value(3,0);a.store<1>(4,0,0);a.jump(0x1018da94);
    } else {
        a.at(0x1018d9e0);bool valid=a.equal();
        if(!valid){a.load(3,2,-0x7be4);a.load(4,25,0);a.load(0,3,0);a.compare_registers(4,0);valid=a.less();}
        if(!valid){a.value(3,0);a.jump(0x1018da94);}
        else {
            a.at(0x1018da00);a.move(3,24);a.move(4,25);a.call(0x10187a40);a.load(0,25,0);a.load(3,24,0);a.multiply(0,0,0x24);a.add(3,3,0);a.load(0,3,0x10);a.compare(0,-1);
            if(a.equal()){a.value(0,0);a.value(3,0);a.store<1>(28,0,0);a.jump(0x1018da94);}
            else {
                a.at(0x1018da38);a.multiply(0,0,0x38);a.load(5,24,4);a.load(4,2,-0x7b80);a.offset(3,1,0x38);a.add(26,5,0);a.load(5,26,0x28);a.load(6,26,0x2c);a.call(0x10717dd0);
                a.load<2>(3,1,0x3a);a.sign8(0,30,true);a.load<2>(4,1,0x38,true);a.multiply(0,3,365);a.add(3,4,0);
                if(a.equal()){a.load<2>(0,26,0x32,true);a.subtract(3,3,0);a.jump(0x1018da84);}
                else{a.at(0x1018da7c);a.load<2>(0,26,0x30,true);a.subtract(3,3,0);}
                a.at(0x1018da84);a.move(4,28);a.move(5,29);a.move(6,31);a.call(0x1030e880);
            }
        }
    }
    a.at(0x1018da94);a.pop(144);a.multiple(24,true);a.finish();
}
}
