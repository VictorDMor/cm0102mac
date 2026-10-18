#include "shortlist_links.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::shortlist_links {
using Call=SemanticGuestCall;
namespace {
bool percentage_difference_exceeds_contract(Call& a) {
    a.at(0x103631d4);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.load<1>(31,31,0x4c);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(0x10363200);a.load(7,30,0);a.value(3,0x51ec0000);a.load(5,27,0);a.offset(4,3,-0x7ae1);a.load<2>(6,7,0);a.offset(3,7,2);a.load(0,29,0);a.add(5,6,5);a.store(30,0,3);a.multiply_high(4,4,5);a.compare_registers(3,0,false);a.shift_right(0,4,5);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,100);a.subtract(29,0,5);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(0x10363254);a.load(5,30,0);a.value(3,0x51ec0000);a.load(4,27,0);a.offset(0,3,-0x7ae1);a.load<2>(3,5,0);a.sign8(5,31);a.add(4,3,4);a.multiply_high(0,0,4);a.shift_right(0,0,5);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,100);a.subtract(0,0,4);a.subtract(0,0,29);a.compare_registers(5,0);return a.nonpositive();
}
void update_contract_marker(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o,std::uint32_t entry,std::uint32_t mask,bool set,bool normalized) {
    Call a(s,entry,d,o);a.link();a.load(3,2,-0x7b7c);a.save_link();a.frame(0x40);a.load(3,3,0);a.call(0x10082f20);a.compare(3,0,false);
    if(normalized){
        if(a.equal()){a.value(3,0);a.jump(entry+56);}else{a.at(entry+40);a.load<1>(0,3,0x5c);a.mask(0,0,mask);a.store<1>(3,0x5c,0);a.value(3,1);}
        a.at(entry+56);
    }else{if(!a.equal()){a.load<1>(0,3,0x5c);if(set)a.flags(0,0,mask);else a.mask(0,0,mask);a.store<1>(3,0x5c,0);}a.at(entry+44);}
    a.pop(0x40);a.finish();
}
}
void mark_primary_contract(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,mark_primary_contract_entry,d,o);a.link();a.multiple(23);a.move(24,4);a.move(23,3);a.load(26,2,-0x7b7c);a.move(25,5);a.load(27,2,-0x7b90);a.move(3,24);a.load(28,2,-0x7b8c);a.value(4,0);a.load(29,2,-0x7b88);a.load(30,2,-0x7b84);a.save_link();a.frame(0x60);a.call(0x100d3040);a.sign8(0,3,true);bool apply=a.equal();
    if(apply){a.load(3,26,0);a.move(4,24);a.call(0x10082f20);a.move(31,3,true);apply=!a.equal();}
    if(apply){
        a.load<1>(0,31,0x5c);a.value(6,7);a.load(3,2,-0x7b40);a.move(4,24);a.flags(0,0,1);a.value(5,0);a.store<1>(31,0x5c,0);a.load<1>(0,31,0x60);a.insert_mask(0,6,15,0);a.store<1>(31,0x60,0);a.load(3,3,0);a.call(0x103f43b0);a.sign8(0,3,true);apply=a.equal();
        if(apply){
            a.load(4,24,0x24);a.move(3,24);a.call(0x100da0b0);a.sign8(0,3,true);bool react=!a.equal();
            if(!react)react=!percentage_difference_exceeds_contract(a);
            if(react){a.at(0x10363294);a.load(4,24,0x24);a.move(3,24);a.value(5,1);a.load(4,4,0x90);a.call(0x100d9b80);}
            a.at(0x103632a8);a.sign8(0,25,true);if(!a.equal()){a.move(3,23);a.move(4,24);a.value(5,1);a.call(0x10363ca0);a.load(3,26,0);a.move(4,24);a.load(5,24,0x24);a.call(0x1008cd20);}
        }
    }
    a.at(0x103632d0);a.pop(0x60);a.multiple(23,true);a.finish();
}
void clear_primary_contract_flag_one(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){update_contract_marker(s,d,o,clear_primary_contract_flag_one_entry,0xfffffffe,false,true);}
void set_primary_contract_flag_two(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){update_contract_marker(s,d,o,set_primary_contract_flag_two_entry,2,true,false);}
void clear_primary_contract_flag_two(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){update_contract_marker(s,d,o,clear_primary_contract_flag_two_entry,0xfffffffd,false,true);}
}
