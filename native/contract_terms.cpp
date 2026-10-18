#include "contract_terms.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::contract_terms {
namespace {
using Call=SemanticGuestCall;
void read_kind(Call& a){a.load<1>(0,3,0x38);a.mask(0,0,0xfe000000,false,24);a.shift_right(0,0,26);a.sign8(0,0);}
void kind_equals(Call& a,std::uint32_t kind){read_kind(a);a.subtract_from(0,0,kind);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.sign8(3,0);a.finish();}
void adjust_value(Call& a,std::uint32_t start,std::uint32_t service) {
    a.link();a.save(31);a.load(4,2,-0x7ac8);a.move(31,3);a.save_link();a.value(0,0);a.frame(80);a.store(1,0x38,0);a.load(3,4,0);a.load(3,3,4);a.mask(0,3,0x00800000,true);
    if(!a.equal()){a.load(0,31,12);a.offset(3,1,0x38);a.value(4,4);a.multiply(0,0,52);a.store(1,0x38,0);a.call(service);a.value(3,0x4ec50000);a.load(0,1,0x38);a.offset(3,3,-0x13b1);a.multiply_high(0,3,0);a.shift_right(0,0,4);a.mask(3,0,1,false,1);a.add(0,0,3);a.store(31,12,0);a.jump(start+0xbc);}
    else {a.at(start+0x6c);a.mask(0,3,0x00400000,true);
        if(!a.equal()){a.load(0,31,12);a.offset(3,1,0x38);a.value(4,8);a.mask(0,0,0xfffffffc,false,2);a.store(1,0x38,0);a.call(service);a.load(0,1,0x38);a.shift_right(0,0,2);a.add_carry(0,0);a.store(31,12,0);a.jump(start+0xbc);}
        else {a.at(start+0xa0);a.load(0,31,12);a.offset(3,1,0x38);a.value(4,8);a.store(1,0x38,0);a.call(service);a.load(0,1,0x38);a.store(31,12,0);}}
    a.at(start+0xbc);a.pop(80);a.restore(31);a.finish();
}
}
void enable_option(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,enable_option_entry,d,o);a.sign8(0,4);a.compare(0,1);if(a.equal()){a.value(0,0);for(int n=0;n<5;++n)a.store<1>(3,0x1c+n,0);a.jump(0x10374b20);}else{a.at(0x10374b18);a.value(0,0);a.store<1>(3,0x1d,0);}a.at(0x10374b20);a.sign8(0,4);a.value(4,1);a.add(3,3,0);a.store<1>(3,0x1c,4);a.finish();}
void set_bonus(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,set_bonus_entry,d,o);a.sign8(0,4);a.mask(0,0,0xfffffffc,false,2);a.add(3,3,0);a.store(3,0x10,5);a.finish();}
void clear_option(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,clear_option_entry,d,o);a.sign8(0,4);a.value(4,0);a.add(3,3,0);a.store<1>(3,0x1c,4);a.finish();}
void clear_bonus(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,clear_bonus_entry,d,o);a.sign8(0,4);a.value(4,0xffffffff);a.mask(0,0,0xfffffffc,false,2);a.add(3,3,0);a.store(3,0x10,4);a.finish();}
void reset_terms(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,reset_terms_entry,d,o);a.value(4,0);a.value(0,0xffffffff);a.store<1>(3,0x54,4);a.store(3,0x58,0);for(int n=0;n<5;++n)a.store<1>(3,0x1c+n,4);for(int n=0;n<3;++n)a.store(3,0x10+n*4,0);a.finish();}
void is_expired(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,is_expired_entry,d,o);kind_equals(a,3);}
void is_rolling(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,is_rolling_entry,d,o);kind_equals(a,1);}
void end_date_delta(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,end_date_delta_entry,d,o);a.link();a.load(5,2,-0x7b80);a.save_link();a.offset(3,3,0x30);a.frame(64);a.load(4,5,0);a.load(5,5,4);a.call(0x10718130);a.sign16(3,3);a.pop(64);a.finish();}
void supports_bonus(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,supports_bonus_entry,d,o);a.load(3,3,0x14);a.compare(3,0,false);if(a.equal()){a.at(0x103803f8);a.value(3,0);a.finish();return;}a.load<1>(0,3,15);a.compare(0,20);if(a.equal()){a.sign8(0,4);a.subtract_from(0,0,2);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.sign8(3,0);a.finish();}else{a.at(0x103803e0);a.sign8(4,4);a.subtract_from(3,4,2);a.offset(0,4,-2);a.combine(0,3,0);a.mask(3,0,1,false,1);a.finish();}}
void supports_option(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    Call a(s,supports_option_entry,d,o);a.link();a.value(7,0);a.save_link();a.frame(64);a.load(6,4,0x30);a.compare(6,0,false);
    if(!a.equal()){a.load(4,2,-0x7b5c);a.load(0,6,0);a.load(4,4,0);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(0,4,0);a.compare(0,0,false);if(!a.equal())a.move(7,0);}
    a.at(0x1038043c);a.sign8(0,5);a.compare(0,2);
    if(a.equal()){a.at(0x103804a8);a.value(3,0);a.jump(0x10380510);}
    else {a.at(0x10380448);if(a.at_least()){a.at(0x1038045c);a.compare(0,4);if(a.equal()){a.at(0x103804e4);a.load<2>(3,3,0x5c,true);a.value(0,31);a.sign8(4,3);a.combine(0,4,0,false,true);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(3,0);a.jump(0x10380510);}else if(a.at_least()){a.at(0x1038050c);a.value(3,0);}else {a.jump(0x103804b0);a.compare(7,0,false);if(!a.equal()){a.move(3,7);a.load(12,7,0);a.load(12,12,0xa4);a.call(0x10781320);a.load(2,1,0x14);a.sign8(0,3,true);if(!a.equal()){a.value(3,1);a.jump(0x10380510);}else {a.at(0x103804dc);a.value(3,0);a.jump(0x10380510);}}else {a.at(0x103804dc);a.value(3,0);a.jump(0x10380510);}}}
        else {a.at(0x1038044c);a.compare(0,0);if(a.equal()){a.at(0x1038046c);a.compare(7,0,false);if(!a.equal()){a.move(3,7);a.load(12,7,0);a.load(12,12,0xa0);a.call(0x10781320);a.load(2,1,0x14);a.sign8(0,3,true);if(!a.equal()){a.value(3,1);a.jump(0x10380510);}else{a.at(0x10380498);a.value(3,0);a.jump(0x10380510);}}else{a.at(0x10380498);a.value(3,0);a.jump(0x10380510);}}else if(a.at_least()){a.at(0x103804a0);a.value(3,1);a.jump(0x10380510);}else {a.jump(0x1038050c);a.value(3,0);}}}
    a.at(0x10380510);a.pop(64);a.finish();
}
void has_release_terms(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,has_release_terms_entry,d,o);a.load<1>(0,3,0x1c);a.compare(0,2);bool found=a.equal();if(!found){a.load<1>(0,3,0x1d);a.compare(0,1);found=a.equal();if(!found){a.load<1>(0,3,0x1f);a.compare(0,2);found=a.equal();}}if(found){a.at(0x10380544);a.value(3,1);a.finish();}else{a.at(0x1038054c);a.value(3,0);a.finish();}}
void adjust_value_primary(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,adjust_value_primary_entry,d,o);adjust_value(a,adjust_value_primary_entry,0x1000a060);}
void adjust_value_alternate(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,adjust_value_alternate_entry,d,o);adjust_value(a,adjust_value_alternate_entry,0x1000a170);}
void set_release_amount(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){Call a(s,set_release_amount_entry,d,o);a.store(3,0x24,4);a.finish();}
}
