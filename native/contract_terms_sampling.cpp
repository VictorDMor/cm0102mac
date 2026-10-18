#include "contract_terms.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::contract_terms {
namespace {
using Call=SemanticGuestCall;

void advance_inline_pool(Call& a,std::uint32_t start) {
    // Later inline draws advance the pointer left in r3 by the preceding draw.
    // Reloading the slot here would change alias and callback behavior.
    a.at(start);a.load(0,28,0);a.offset(3,3,2);a.store(29,0,3);a.compare_registers(3,0,false);
    if(!a.nonpositive()){a.store(29,0,27);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(26,0,0);}
}

bool wide_bound(Call& a,unsigned bound,std::uint32_t start) {
    a.at(start);a.value(3,0x10000);a.offset(0,3,-1);a.compare_registers(bound,0);
    if(a.positive())return true;
    a.value(3,0xffff0000);a.offset(0,3,1);a.compare_registers(bound,0);return !a.at_least();
}

template<unsigned Bound,unsigned Result,unsigned Child,unsigned Converted>
void scaled_inline_draw(Call& a,std::uint32_t start) {
    a.at(start);a.value(9,0x43300000);a.exclusive_flags(0,Bound,0x80000000);a.store(1,Converted==0x38?0x4c:0x3c,0);
    a.value(5,0x80010000);a.load(3,29,0);a.value(4,0x10000);a.store(1,Converted==0x38?0x48:0x38,9);
    a.offset(6,5,-0x7fff);a.floating_load(2,25,0xc0);a.multiply(0,Bound,10);a.floating_load(1,1,Converted==0x38?0x48:0x38);
    a.offset(5,4,-1);a.floating_load(0,25,0);a.floating_subtract(1,1,2);a.multiply_high(4,6,0);a.load<2>(8,3,0);
    a.load(7,26,0);a.floating_difference(1,1,0,true);a.store(1,0x40,9);a.add(7,8,7);a.multiply_high(6,6,7);
    a.add(0,4,0);a.add(4,6,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.shift_right(0,0,15);
    a.add(4,4,6);a.product(5,4,5);a.mask(4,0,1,false,1);a.add_record(Child,0,4);a.subtract(0,5,7);
    a.exclusive_flags(0,0,0x80000000);a.store(1,0x44,0);a.floating_load(0,1,0x40);a.floating_subtract(0,0,2);
    a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,Converted);a.load(Result,1,Converted+4);
}

template<unsigned Bound,unsigned Result>
void signed_inline_draw(Call& a,std::uint32_t start) {
    a.at(start);a.load(3,29,0);a.compare(Bound,0);a.load(0,26,0);a.load<2>(3,3,0);a.add(3,3,0);
    a.divide(0,3,Bound);a.product(0,0,Bound);a.subtract(0,0,3);a.negate(Result,0);
    if(!a.nonpositive())a.move(Result,0);
}

template<unsigned Bound,unsigned Result,unsigned Child,bool Recursive=false>
void add_correction(Call& a,std::uint32_t start,std::uint32_t join) {
    a.at(start);
    if constexpr(Recursive){a.value(4,0x66660000);a.offset(0,Child,5);a.offset(3,4,0x6667);}
    else {a.value(3,0x66660000);a.offset(0,Child,5);a.offset(3,3,0x6667);}
    a.offset(4,Bound,-1);a.multiply_high(0,3,0);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);
    a.add(Result,Result,0);a.compare_registers(Result,4);if(!a.nonpositive()){a.move(Result,4);a.jump(join);}
}

void recursive_correction_draw(Call& a) {
    a.at(0x103748b0);a.value(4,0x43300000);a.exclusive_flags(0,20,0x80000000);a.store(1,0x3c,0);
    a.value(5,0x80010000);a.load(7,29,0);a.value(3,0x10000);a.store(1,0x38,4);a.offset(6,5,-0x7fff);
    a.floating_load(2,25,0xc0);a.multiply(0,20,10);a.floating_load(1,1,0x38);a.offset(5,3,-1);a.floating_load(0,25,0);
    a.floating_subtract(1,1,2);a.multiply_high(3,6,0);a.load<2>(8,7,0);a.load(7,26,0);a.floating_difference(1,1,0,true);
    a.store(1,0x40,4);a.add(7,8,7);a.multiply_high(4,6,7);a.add(0,3,0);a.add(3,4,7);a.shift_right(3,3,15);
    a.mask(4,3,1,false,1);a.shift_right(0,0,15);a.add(3,3,4);a.product(4,3,5);a.mask(3,0,1,false,1);
    a.add(3,0,3);a.subtract(0,4,7);a.exclusive_flags(0,0,0x80000000);a.store(1,0x44,0);a.floating_load(0,1,0x40);
    a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x48);a.load(25,1,0x4c);
    a.call(sample_contract_range_entry);add_correction<20,25,3,true>(a,0x10374950,0x103749b0);
}

void third_correction_draw(Call& a) {
    scaled_inline_draw<21,23,20,0x48>(a,0x103747c4);
    if(a.equal()){a.value(25,0);a.jump(0x103749b0);}
    else {
        advance_inline_pool(a,0x1037486c);
        if(wide_bound(a,20,0x10374890))recursive_correction_draw(a);
        else signed_inline_draw<20,25>(a,0x10374984);
    }
    add_correction<21,23,25>(a,0x103749b0,0x10374a10);
}

void second_correction_draw(Call& a) {
    scaled_inline_draw<22,24,21,0x48>(a,0x103746d8);
    if(a.equal()){a.value(23,0);a.jump(0x10374a10);}
    else {
        advance_inline_pool(a,0x10374780);
        if(wide_bound(a,21,0x103747a4))third_correction_draw(a);
        else signed_inline_draw<21,23>(a,0x103749e4);
    }
    add_correction<22,24,23>(a,0x10374a10,0x10374a70);
}

void scaled_contract_draw(Call& a) {
    scaled_inline_draw<30,31,22,0x38>(a,0x103745ec);
    if(a.equal()){a.value(24,0);a.jump(0x10374a70);}
    else {
        advance_inline_pool(a,0x10374694);
        if(wide_bound(a,22,0x103746b8))second_correction_draw(a);
        else signed_inline_draw<22,24>(a,0x10374a44);
    }
    a.at(0x10374a70);a.value(3,0x66660000);a.offset(4,24,5);a.offset(3,3,0x6667);a.offset(0,30,-1);
    a.multiply_high(3,3,4);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(31,31,3);
    a.compare_registers(31,0);if(!a.nonpositive())a.move(31,0);
    a.at(0x10374aa0);a.move(3,31);a.jump(0x10374ad4);
}
}

void sample_contract_range(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,sample_contract_range_entry,dependencies,observer);
    a.link();a.multiple(20);a.move(30,3,true);a.load(25,2,-0x4dac);a.load(26,2,-0x7b90);a.load(27,2,-0x7b8c);
    a.load(28,2,-0x7b88);a.load(29,2,-0x7b84);a.save_link();a.frame(128);
    if(a.equal()){a.value(3,0);a.jump(0x10374ad4);}
    else {
        a.at(0x103745a4);a.load(3,29,0);a.load(0,28,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(29,0,3);
        if(!a.nonpositive()){a.store(29,0,27);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(26,0,0);}
        if(wide_bound(a,30,0x103745cc))scaled_contract_draw(a);
        else signed_inline_draw<30,3>(a,0x10374aa8);
    }
    a.at(0x10374ad4);a.pop(128);a.multiple(20,true);a.finish();
}
}
