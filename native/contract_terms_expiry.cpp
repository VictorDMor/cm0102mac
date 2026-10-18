#include "contract_terms.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::contract_terms {
namespace {
using Call=SemanticGuestCall;
void compare_day(Call& a,int scratch){a.load<2>(4,1,scratch,true);a.load<2>(6,3,0x30,true);a.combine(4,6,4,false,true);a.shift_right(5,4,1);a.intersect(4,4,6);a.subtract(4,4,5);a.mask(4,4,1,false,1);a.sign8(4,4);}
bool before_end(Call& a,std::uint32_t start,int slot,int scratch,int cache){
    a.at(start);a.load(4,2,slot);a.load(5,4,0);a.load(4,4,4);a.store(1,scratch,5);a.load<2>(6,1,scratch+2);a.store(1,cache,5);a.compare_registers(6,0,false);a.store(1,cache+4,4);a.store(1,scratch+4,4);
    if(!a.at_least()){a.value(4,1);a.jump(start+0x5c);}else{a.at(start+0x30);if(!a.nonpositive()){a.value(4,0);a.jump(start+0x5c);}else{a.at(start+0x3c);compare_day(a,scratch);}}
    a.at(start+0x5c);a.sign8(4,4,true);return !a.equal();
}
void cached_band(Call& a,std::uint32_t start,int cache,int scratch,int base){
    a.at(start);a.load(4,1,cache);a.value(5,0);a.load(6,1,cache+4);a.store(1,scratch,4);a.load<2>(4,1,scratch+2);a.store(1,scratch+4,6);a.compare_registers(4,0,false);
    if(a.equal()){a.load<2>(4,1,scratch,true);a.load<2>(0,3,0x30,true);a.compare_registers(4,0);if(a.equal())a.value(5,1);}
    a.at(start+0x34);a.negate(0,5);a.combine(0,0,5);a.shift_right(3,0,31);a.offset(3,3,base);a.finish();
}
void final_band(Call& a){
    a.at(0x10380b74);a.load(4,1,-0xa0);a.value(6,0);a.load(5,1,-0x9c);a.store(1,-0x68,4);a.load<2>(4,1,-0x66);a.store(1,-0x64,5);a.compare_registers(4,0,false);
    if(a.equal()){a.load<2>(5,1,-0x68,true);a.load<2>(4,3,0x30,true);a.compare_registers(5,4);if(a.equal())a.value(6,1);}
    a.at(0x10380ba8);a.compare(6,0);if(!a.equal()){a.value(3,11);a.finish();return;}
    a.at(0x10380bb8);a.load(6,2,-0x799c);a.value(5,0);a.load(4,6,0);a.load(6,6,4);a.store(1,-0x70,4);a.load<2>(4,1,-0x6e);a.store(1,-0x6c,6);a.compare_registers(4,0,false);
    if(a.equal()){a.load<2>(4,1,-0x70,true);a.load<2>(0,3,0x30,true);a.compare_registers(4,0);if(a.equal())a.value(5,1);}
    a.at(0x10380bf0);a.negate(0,5);a.combine(0,0,5);a.mask(3,0,1,false,1);a.offset(3,3,12);a.finish();
}
}
void expiry_band(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    Call a(s,expiry_band_entry,d,o);a.load<1>(0,3,0x38);a.mask(0,0,0xfe000000,false,24);a.shift_right(7,0,26);a.sign8(0,7);a.compare(0,1);
    if(a.equal()){a.value(3,14);a.finish();return;}
    a.at(0x10380730);a.load(4,2,-0x7b80);a.load<2>(0,3,0x32);a.load(5,4,0);a.load(4,4,4);a.store(1,-8,5);a.load<2>(5,1,-6);a.store(1,-4,4);a.compare_registers(5,0,false);
    if(!a.at_least()){a.value(4,1);a.jump(0x10380788);}else{a.at(0x1038075c);if(!a.nonpositive()){a.value(4,0);a.jump(0x10380788);}else{a.at(0x10380768);compare_day(a,-8);}}
    a.at(0x10380788);a.sign8(4,4,true);if(a.equal()){a.value(3,14);a.finish();return;}
    a.at(0x10380798);a.sign8(4,7);a.compare(4,2);if(!a.equal()){a.value(3,0);a.finish();return;}
    if(before_end(a,0x103807ac,-0x79b4,-0x10,-0x78)){a.value(3,0);a.finish();return;}
    if(before_end(a,0x10380818,-0x79a8,-0x18,-0x90)){
        if(before_end(a,0x1038087c,-0x79ac,-0x20,-0x88)){
            if(before_end(a,0x103808e0,-0x79b0,-0x28,-0x80))cached_band(a,0x10380944,-0x78,-0x30,2);
            else cached_band(a,0x1038098c,-0x80,-0x38,4);
        }else cached_band(a,0x103809d4,-0x88,-0x40,6);
    }else {
        if(before_end(a,0x10380a1c,-0x79a0,-0x48,-0xa0)){
            if(before_end(a,0x10380a80,-0x79a4,-0x50,-0x98))cached_band(a,0x10380ae4,-0x90,-0x58,8);
            else cached_band(a,0x10380b2c,-0x98,-0x60,10);
        }else final_band(a);
    }
}
}
