#include "match_random.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::match_random {
namespace {
using Call = SemanticGuestCall;

void advance_pool(Call& a) {
    a.at(0x101d3cbc);
    a.load(3,29,0); a.offset(4,3,2); a.store(29,0,4);
    a.load(3,2,-0x7b88); a.load(0,3,0); a.compare_registers(4,0,false);
    if (!a.nonpositive()) {
        // The base is reloaded only on wrap; salt is refreshed after installing
        // the cursor. Slot aliases therefore affect the following sample.
        a.load(0,2,-0x7b8c); a.store(29,0,0); a.call(0x1077b770);
        a.mask(0,3,0xffff); a.store(28,0,0);
    }
}

void signed_small_range(Call& a) {
    a.at(0x101d3e00);
    a.load(3,29,0); a.load<2>(3,3,0); a.load(0,28,0); a.add(3,3,0);
    a.divide(0,3,30); a.product(0,0,30); a.subtract(31,0,3);
    a.compare(30,0);
    if (!a.nonpositive()) { a.move(27,31); a.jump(0x101d3e30); }
    else { a.at(0x101d3e2c); a.negate(27,31); }
    a.at(0x101d3e30); a.move(3,27);
}

void scaled_wide_range(Call& a) {
    a.at(0x101d3d0c);
    a.load(3,29,0); a.load<2>(3,3,0); a.load(0,28,0); a.add(4,3,0);
    a.value(3,0x80010000); a.offset(3,3,-0x7fff); a.multiply_high(0,3,4);
    a.add(0,0,4); a.shift_right(0,0,15); a.mask(3,0,1,false,1); a.add(0,0,3);
    a.value(3,0x10000); a.offset(3,3,-1); a.product(0,0,3); a.subtract(31,0,4);
    // Preserve the original double conversion through the live TOC table and
    // stack scratch, including fctiwz/FPSCR and faults at each ordered access.
    a.load(3,2,-0x52c4); a.floating_load(1,3,0x60);
    a.exclusive_flags(0,30,0x80000000); a.store(1,0x4c,0); a.value(0,0x43300000);
    a.store(1,0x48,0); a.floating_load(0,1,0x48); a.floating_difference(1,0,1);
    a.load(3,2,-0x52c4); a.floating_load(0,3,0x70); a.floating_difference(2,1,0,true);
    a.load(3,2,-0x52c4); a.floating_load(1,3,0x60);
    a.exclusive_flags(0,31,0x80000000); a.store(1,0x44,0); a.value(0,0x43300000);
    a.store(1,0x40,0); a.floating_load(0,1,0x40); a.floating_difference(0,0,1);
    a.floating_product(0,2,0); a.floating_integer(0,0); a.floating_store(0,1,0x38); a.load(31,1,0x3c);
    a.multiply(0,30,10); a.value(3,0x80010000); a.offset(3,3,-0x7fff);
    a.multiply_high(3,3,0); a.add(0,3,0); a.shift_right(0,0,15);
    a.mask(3,0,1,false,1); a.add(3,0,3); a.call(entry);
    // The correction consumes its own pool draw. Integer multiplication before
    // recursion wraps at 32 bits, even for extreme signed range arguments.
    a.offset(0,3,5); a.value(3,0x66660000); a.offset(3,3,0x6667);
    a.multiply_high(0,3,0); a.shift_right(0,0,2); a.mask(3,0,1,false,1);
    a.add(0,0,3); a.add(31,31,0); a.offset(0,30,-1); a.compare_registers(31,0);
    if (!a.nonpositive()) a.offset(31,30,-1);
    a.at(0x101d3df8); a.move(3,31); a.jump(0x101d3e34);
}
}

void sample(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entry,dependencies,observer);
    a.link(); a.multiple(27); a.save_link(); a.frame(112); a.move(30,3);
    a.load(28,2,-0x7b90); a.load(29,2,-0x7b84); a.compare(30,0);
    if (a.equal()) { a.value(3,0); a.jump(0x101d3e34); }
    else {
        advance_pool(a);
        a.at(0x101d3cec); a.value(3,0x10000); a.offset(3,3,-1); a.compare_registers(30,3);
        bool wide = a.positive();
        if (!wide) {
            a.value(3,0xffff0000); a.offset(3,3,1); a.compare_registers(30,3);
            wide = !a.at_least();
        }
        if (wide) scaled_wide_range(a); else signed_small_range(a);
    }
    a.at(0x101d3e34); a.pop(112); a.multiple(27,true); a.finish();
}
}
