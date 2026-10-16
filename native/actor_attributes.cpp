#include "actor_attributes.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_attributes {
namespace {
using Call = SemanticGuestCall;

void advance_pool(Call& a, std::uint32_t start) {
    a.at(start); a.load(3,31,0); a.load(0,26,0); a.offset(3,3,2);
    a.compare_registers(3,0,false); a.store(31,0,3);
    if (!a.nonpositive()) {
        a.store(31,0,27); a.call(0x1077b770); a.mask(0,3,0xffff); a.store(29,0,0);
    }
    a.at(start+0x28);
}

template<unsigned Bound, unsigned Result, bool First = false>
void wide_sample(Call& a, std::uint32_t start, std::uint32_t join) {
    constexpr unsigned interval = First ? 0x48 : 0x38, converted = First ? 0x38 : 0x48;
    a.at(start); a.exclusive_flags(3,Bound,0x80000000); a.value(0,0x43300000);
    a.store(1,interval+4,3); a.value(6,0x80010000); a.load(7,31,0); a.value(4,0x43300000);
    a.store(1,interval,0); a.value(5,0x10000); a.floating_load(2,30,0x80); a.value(3,0x80010000);
    a.floating_load(1,1,interval); a.offset(6,6,-0x7fff); a.floating_load(0,30,0); a.offset(5,5,-1);
    a.floating_subtract(1,1,2); a.load<2>(8,7,0); a.load(7,29,0); a.multiply(0,Bound,10);
    a.offset(3,3,-0x7fff); a.store(1,0x40,4); a.floating_difference(2,1,0,true);
    a.add(7,8,7); a.multiply_high(4,6,7); a.floating_load(1,30,0x80); a.add(4,4,7);
    a.shift_right(4,4,15); a.mask(6,4,1,false,1); a.add(4,4,6); a.product(4,4,5);
    a.subtract(Result,4,7); a.exclusive_flags(4,Result,0x80000000); a.store(1,0x44,4);
    a.multiply_high(3,3,0); a.floating_load(0,1,0x40); a.floating_subtract(0,0,1);
    a.add(0,3,0); a.shift_right(0,0,15); a.floating_product(0,2,0); a.mask(3,0,1,false,1);
    a.add(3,0,3); a.floating_integer(0,0); a.floating_store(0,1,converted); a.load(Result,1,converted+4);
    a.call(0x10226520); a.value(4,0x66660000); a.offset(3,3,5); a.offset(4,4,0x6667);
    a.offset(0,Bound,-1); a.multiply_high(3,4,3); a.shift_right(3,3,2); a.mask(4,3,1,false,1);
    a.add(3,3,4); a.add(Result,Result,3); a.compare_registers(Result,0);
    if (!a.nonpositive()) { a.offset(Result,Bound,-1); a.jump(join); }
}

// Each draw retains its own source sites and live pool accesses. In particular,
// the second draw of a pair reloads the trait after the first draw's callbacks.
template<unsigned Bound, unsigned Result, bool First = false>
void draw(Call& a, std::uint32_t start) {
    const auto join = start+0x15c;
    if (a.equal()) { a.value(Result,0); a.jump(join); return; }
    advance_pool(a,start);
    a.value(3,0x10000); a.offset(3,3,-1); a.compare_registers(Bound,3);
    bool wide = a.positive();
    if (!wide) {
        a.value(3,0xffff0000); a.offset(3,3,1); a.compare_registers(Bound,3);
        wide = !a.at_least();
    }
    if (wide) { wide_sample<Bound,Result,First>(a,start+0x48,join); return; }
    a.at(start+0x12c); a.load(3,31,0); a.compare(Bound,0); a.load(0,29,0);
    a.load<2>(3,3,0); a.add(3,3,0); a.divide(0,3,Bound); a.product(0,0,Bound);
    a.subtract(Result,0,3); a.negate(0,Result);
    if (!a.nonpositive()) a.move(0,Result);
    a.at(join-4); a.move(Result,0);
}

template<unsigned Bound, unsigned Result>
void trait_draw(Call& a, std::uint32_t start, unsigned field) {
    a.at(start); a.load<1>(Bound,28,field); a.sign8(Bound,Bound,true);
    draw<Bound,Result>(a,start+0x14);
}

void paired_trait(Call& a, std::uint32_t start, unsigned source, unsigned destination) {
    trait_draw<24,23>(a,start,source);
    trait_draw<25,24>(a,start+0x170,source);
    a.at(start+0x2e0); a.move(3,23); a.move(4,24); a.call(maximum_entry);
    a.store<1>(28,destination,3);
}

void single_trait(Call& a, std::uint32_t start, unsigned source, unsigned destination) {
    trait_draw<24,23>(a,start,source);
    a.at(start+0x170); a.store<1>(28,destination,23);
}

void condition_adjusted_pair(Call& a) {
    a.floating_load(1,3,0x50,true); a.load<1>(0,3,0x111);
    a.floating_difference(0,1,0,true,true); a.sign8(0,0); a.floating_integer(0,0);
    a.floating_store(0,1,0x48); a.load(3,1,0x4c); a.sign16(3,3);
    a.subtract_from(3,3,25); a.subtract(23,3,0,true);
    draw<23,24,true>(a,0x10235f04);
    a.at(0x10236060); a.floating_load(1,28,0x50,true); a.floating_load(0,25,0xa8,true);
    a.load<1>(0,28,0x111); a.floating_difference(0,1,0,true,true); a.sign8(0,0);
    a.floating_integer(0,0); a.floating_store(0,1,0x38); a.load(3,1,0x3c);
    a.sign16(3,3); a.subtract_from(3,3,25); a.subtract(25,3,0,true);
    draw<25,23>(a,0x10236098);
    a.at(0x102361f4); a.move(3,24); a.move(4,23); a.call(maximum_entry); a.store<1>(28,0x107,3);
}
}

// Keep the named phases while exposing their constant source sites, offsets and
// register selections to the optimizer across all fourteen sampling locations.
template<bool Observed>
[[gnu::flatten]] void refresh_body(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,refresh_entry,dependencies,Observed ? observer : nullptr);
    a.link(); a.multiple(23); a.load(25,2,-0x51e0); a.move(28,3); a.load(30,2,-0x51e8);
    a.floating_load(0,25,0xa8,true); a.load(29,2,-0x7b90); a.load(27,2,-0x7b8c);
    a.load(26,2,-0x7b88); a.load(31,2,-0x7b84); a.save_link(); a.frame(0x80);
    condition_adjusted_pair(a);
    paired_trait(a,0x10236204,0x113,0x10d);
    single_trait(a,0x102364f4,0x114,0x10a);
    single_trait(a,0x10236668,0x115,0x10b);
    single_trait(a,0x102367dc,0x116,0x10c);
    paired_trait(a,0x10236950,0x117,0x10e);
    paired_trait(a,0x10236c40,0x112,0x109);
    single_trait(a,0x10236f30,0x118,0x108);
    single_trait(a,0x102370a4,0x119,0x110);
    trait_draw<23,24>(a,0x10237218,0x11a);
    a.at(0x10237388); a.store<1>(28,0x10f,24); a.pop(0x80); a.multiple(23,true); a.finish();
}

void refresh(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    if (observer) refresh_body<true>(state, dependencies, observer);
    else refresh_body<false>(state, dependencies, nullptr);
}

void signed_maximum(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,maximum_entry,dependencies,observer);
    a.compare_registers(3,4); if (!a.nonpositive()) a.move(4,3);
    a.at(0x102373ac); a.move(3,4); a.finish();
}
}
