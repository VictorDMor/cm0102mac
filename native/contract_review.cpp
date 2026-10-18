#include "contract_review_detail.hpp"

namespace cm::aot::contract_review {
namespace {
using detail::Call;
void clear_present_condition(Call& a,std::uint32_t pc,std::uint32_t mask,bool wide=false) {
    a.at(pc);a.load(0,28,0x50);a.mask(0,0,mask,true);
    if(!a.equal()) {
        if(wide){a.value(4,0x10000);a.move(3,28);a.offset(4,4,0);}
        else {a.move(3,28);a.value(4,mask);}
        a.call(clear_review_condition_entry);
    }
}
bool review_can_clear(Call& a) {
    a.at(0x1037f638);a.load(0,30,0x14);a.compare(0,0,false);if(a.equal())return false;
    a.load(3,28,4);a.load(0,4,0);a.compare_registers(3,0);if(!a.equal())return false;
    a.load<1>(0,28,0x5c);a.mask(0,0,8,true);if(a.equal())return false;
    a.load<1>(0,28,0x54);a.sign8(0,0,true);if(!a.equal())return false;
    a.move(3,28);a.call(has_review_trigger_entry);a.sign8(0,3,true);if(!a.equal())return false;
    a.load<1>(0,28,0x60);a.mask(0,0,15);a.compare(0,7,false);if(a.equal())return false;
    a.load(3,2,-0x7b40);a.move(4,30);a.value(5,0);a.load(3,3,0);a.call(0x103ea290);a.sign8(0,3,true);return a.equal();
}
void complete_review_clear(Call& a) {
    a.at(0x1037f6a8);a.load(3,31,0);a.move(4,28);a.move(5,29);a.call(0x1008f620);
    a.load<1>(0,28,0x5c);a.mask(0,0,0xfffffff7);a.store<1>(28,0x5c,0);
    a.load<1>(3,30,0x65);a.load<1>(0,30,0x67);a.add(3,3,0);a.offset(0,3,5);a.store<1>(28,0x4c,0);
    a.load<1>(0,28,0x45);a.compare(0,0,false);if(!a.equal()){a.value(0,0);a.store<1>(28,0x45,0);}
    a.at(0x1037f6ec);a.load<1>(0,28,0x44);a.sign8(0,0);a.compare(0,15);if(!a.at_least()){a.value(0,15);a.store<1>(28,0x44,0);}
    clear_present_condition(a,0x1037f704,0x10);clear_present_condition(a,0x1037f71c,1);
    clear_present_condition(a,0x1037f734,4);clear_present_condition(a,0x1037f74c,8);
    clear_present_condition(a,0x1037f764,0x100);clear_present_condition(a,0x1037f77c,0x80);
    clear_present_condition(a,0x1037f794,0x10000,true);
}
bool status_needs_followup(Call& a) {
    a.at(0x1037f930);a.load(0,29,0);a.load(3,30,0);a.multiply(0,0,0x70);a.add(3,3,0);a.load<1>(0,3,0x62);a.sign8(0,0);
    a.compare(0,12);if(a.equal())return false;a.compare(0,13);if(a.equal())return false;a.compare(0,15);if(a.equal())return false;
    a.load<1>(3,29,0x4c);a.sign8(0,3,true);if(a.equal())return false;
    a.sign8(0,3);a.mask(0,0,0x10000,true);if(!a.equal())return false;
    a.load<1>(0,29,0x54);a.sign8(0,0,true);if(!a.equal())return false;
    a.move(3,29);a.call(has_review_trigger_entry);a.sign8(0,3,true);if(a.equal())return false;
    a.load(0,29,0);a.value(5,0);a.load(4,30,0);a.multiply(0,0,0x70);a.load(3,31,0);a.add(4,4,0);a.call(0x103f43b0);a.sign8(0,3,true);if(!a.equal())return false;
    a.load<1>(3,29,0x5c);a.sign8(3,3);a.mask(0,3,8,true);if(!a.equal())return false;
    a.mask(0,3,1,true);if(!a.equal())return false;
    a.load(0,29,0);a.load(4,30,0);a.multiply(0,0,0x70);a.load(3,31,0);a.add(4,4,0);a.call(0x103e6470);a.sign8(0,3,true);if(!a.equal())return false;
    a.load(0,29,0);a.load(4,30,0);a.multiply(3,0,0x70);a.offset(0,3,0x5c);a.indexed_load<2>(0,4,0,true);a.sign8(0,0);a.compare(0,20);
    if(a.positive())return true;
    a.add(3,4,3);a.load(3,3,0x14);a.load<2>(0,3,10,true);a.compare(0,4250);return !a.nonpositive();
}
}
void reset_review_state(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,reset_review_state_entry,d,o);a.link();a.save(31);a.move(31,5);a.save(30);a.move(30,4);a.value(4,0);a.save(29);a.move(29,3);a.save_link();a.value(0,0);a.frame(80);a.store<1>(3,0x4c,0);a.store<1>(3,0x5c,0);
    a.call(clear_review_conditions_entry);a.move(3,29);a.call(reset_status_bits_entry);a.value(3,4);a.value(0,0);a.store<1>(29,0x5c,3);a.compare(31,0,false);a.store<1>(29,0x46,0);a.store<1>(29,0x45,0);
    if(!a.equal()){a.load(5,31,0x2c);a.compare(5,0,false);if(!a.equal()){
        a.load(3,2,-0x7b3c);a.move(4,30);a.load<1>(6,5,0x119);a.move(5,31);a.load(3,3,0);a.value(7,0);a.call(0x103092f0);a.sign8(0,3);a.value(3,10);a.compare(0,10);if(!a.positive())a.move(3,0);
        a.at(0x1037f0e8);a.load<1>(0,29,0x60);a.insert_mask(0,3,0xf0,4);a.store<1>(29,0x60,0);
    }}
    a.at(0x1037f0f4);a.pop(80);a.restore(31);a.restore(30);a.restore(29);a.finish();
}
void refresh_review_conditions(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,refresh_review_conditions_entry,d,o);a.link();a.multiple(26);a.load(30,2,-0x7be0);a.move(29,4);a.load(31,2,-0x7b7c);a.move(28,3);a.load(26,2,-0x7b78);a.value(27,0);a.save_link();a.frame(80);
    a.load(0,3,0);a.load(4,30,0);a.multiply(0,0,0x70);a.load(3,31,0);a.add(4,4,0);a.call(0x10082f20);a.compare(3,0,false);a.store(26,0,3);
    if(!a.equal()){a.load<1>(0,3,0x38);a.mask(0,0,0xfe000000,false,24);a.shift_right(0,0,26);a.sign8(0,0);a.compare(0,5);if(!a.equal())a.value(27,1);}
    a.at(0x1037f5f8);a.compare(27,0);if(!a.equal()){a.load(3,26,0);a.load(4,2,-0x7be8);a.load(0,3,4);a.load(3,4,0);a.multiply(0,0,0x248);a.add(4,3,0);a.jump(0x1037f620);}else{a.at(0x1037f61c);a.value(4,0);}
    a.at(0x1037f620);a.load(0,28,0);a.compare(4,0,false);a.load(3,30,0);a.multiply(0,0,0x70);a.add(30,3,0);
    if(!a.equal()&&review_can_clear(a))complete_review_clear(a);
    a.at(0x1037f7b0);a.pop(80);a.multiple(26,true);a.finish();
}
void clear_review_condition(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,clear_review_condition_entry,d,o);a.link();a.save(31);a.move(31,4);a.save(30);a.move(30,3);a.save_link();a.frame(64);a.load(4,3,0x50);a.intersect(0,4,31,true);a.store(3,0x50,0);a.call(refresh_review_conditions_entry);
    a.load(0,30,0);a.move(4,31);a.load(3,2,-0x7be0);a.multiply(0,0,0x70);a.load(3,3,0);a.add(3,3,0);a.call(0x100de090);a.pop(64);a.restore(31);a.restore(30);a.finish();
}
void clear_review_conditions(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,clear_review_conditions_entry,d,o);a.link();a.save(31);a.move(31,4);a.save(30);a.move(30,3);a.save_link();a.value(0,0);a.frame(64);a.store(3,0x50,0);a.load(3,2,-0x7be0);a.load(0,30,0);a.load(3,3,0);a.multiply(0,0,0x70);a.add(3,3,0);a.call(0x100ddf30);a.sign8(0,31,true);
    if(!a.equal()){a.move(3,30);a.value(4,0);a.call(refresh_review_conditions_entry);}
    a.at(0x1037f880);a.pop(64);a.restore(31);a.restore(30);a.finish();
}
void add_status_bits(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,add_status_bits_entry,d,o);a.link();a.multiple(26);a.move(26,4);a.load(30,2,-0x7be0);a.compare(26,0x1000);a.load(31,2,-0x7b40);a.load(27,2,-0x7ba8);a.move(29,3);a.load(28,2,-0x4da4);a.save_link();a.frame(128);
    if(!a.nonpositive()){detail::report_diagnostic(a,0x1037f8d0,28,27,0x1e9b);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);}
    a.at(0x1037f924);a.load(0,29,0x48);a.combine(0,0,26);a.store(29,0x48,0);
    if(status_needs_followup(a)){a.at(0x1037fa24);a.load(3,2,-0x7b7c);a.move(4,29);a.load(3,3,0);a.call(0x1008e350);}
    a.at(0x1037fa34);a.pop(128);a.multiple(26,true);a.finish();
}
void clear_status_bits(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,clear_status_bits_entry,d,o);a.load(0,3,0x48);a.intersect(0,0,4,true);a.store(3,0x48,0);a.finish();}
void reset_status_bits(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,reset_status_bits_entry,d,o);a.value(0,0);a.store(3,0x48,0);a.finish();}
void status_bits(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,status_bits_entry,d,o);a.load(3,3,0x48);a.finish();}
}
