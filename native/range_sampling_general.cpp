#include "range_sampling.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::range_sampling {
namespace {
using Call = SemanticGuestCall;
void advance_general_pool(Call&);
void scaled_general_range(Call&);
void first_general_correction(Call&);
void first_general_scaled_correction(Call&);
void second_general_correction(Call&);
void second_general_scaled_correction(Call&);
void third_general_correction(Call&);
void third_general_scaled_correction(Call&);
void signed_general_range(Call&);

// Inlined correction levels keep their original captured slots and scratch.
// Recomputing a bound or reloading a cursor earlier would change alias behavior.
void signed_general_range(Call& a) {
    a.at(0x10712a18); a.load(3,29,0x0); a.compare(30,0); a.load(0,26,0x0); a.load<2>(3,3,0x0); a.add(3,3,0);
    a.divide(0,3,30); a.product(0,0,30); a.subtract(0,0,3); a.negate(3,0);
    if (a.positive()) {
        a.at(0x10712a40); a.move(3,0);
    }
    a.at(0x10712a44);
}
void third_general_scaled_correction(Call& a) {
    a.at(0x10712820); a.value(4,0x43300000); a.exclusive_flags(0,20,0x80000000); a.store(1,0x3c,0);
    a.value(5,0x80010000); a.load(7,29,0x0); a.value(3,0x10000); a.store(1,0x38,4); a.offset(6,5,-0x7fff);
    a.floating_load(2,25,0x8); a.multiply(0,20,0xa); a.floating_load(1,1,0x38); a.offset(5,3,-0x1);
    a.floating_load(0,25,0x0); a.floating_difference(1,1,2); a.multiply_high(3,6,0); a.load<2>(8,7,0x0);
    a.load(7,26,0x0); a.floating_difference(1,1,0,true); a.store(1,0x40,4); a.add(7,8,7); a.multiply_high(4,6,7);
    a.add(0,3,0); a.add(3,4,7); a.shift_right(3,3,0xf); a.mask(4,3,0x1,false,1); a.shift_right(0,0,0xf);
    a.add(3,3,4); a.product(4,3,5); a.mask(3,0,0x1,false,1); a.add(3,0,3); a.subtract(0,4,7);
    a.exclusive_flags(0,0,0x80000000); a.store(1,0x44,0); a.floating_load(0,1,0x40); a.floating_difference(0,0,2);
    a.floating_product(0,1,0); a.floating_integer(0,0); a.floating_store(0,1,0x48); a.load(25,1,0x4c);
    a.call(0x107124e0); a.value(4,0x66660000); a.offset(0,3,0x5); a.offset(3,4,0x6667); a.offset(4,20,-0x1);
    a.multiply_high(0,3,0); a.shift_right(0,0,0x2); a.mask(3,0,0x1,false,1); a.add(0,0,3); a.add(25,25,0);
    a.compare_registers(25,4);
    if (a.positive()) {
        a.at(0x107128ec); a.move(25,4); a.jump(0x10712920);
    }
    a.at(0x10712920);
}
void third_general_correction(Call& a) {
    a.at(0x107127dc); a.load(0,28,0x0); a.offset(3,3,0x2); a.store(29,0x0,3); a.compare_registers(3,0,false);
    if (a.positive()) {
        a.at(0x107127f0); a.store(29,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(26,0x0,0);
    }
    a.at(0x10712800); a.value(3,0x10000); a.offset(0,3,-0x1); a.compare_registers(20,0);
    if (a.positive()) {
        a.at(0x10712820);
        third_general_scaled_correction(a);
    } else {
        a.at(0x10712810); a.value(3,0xffff0000); a.offset(0,3,0x1); a.compare_registers(20,0);
        if (a.at_least()) {
            a.at(0x107128f4); a.load(3,29,0x0); a.compare(20,0); a.load(0,26,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,20); a.product(0,0,20); a.subtract(0,0,3); a.negate(25,0);
            if (a.positive()) {
                a.at(0x1071291c); a.move(25,0);
            }
            a.at(0x10712920);
        } else {
            a.at(0x10712820);
            third_general_scaled_correction(a);
        }
        a.at(0x10712920);
    }
    a.at(0x10712920);
}
void second_general_scaled_correction(Call& a) {
    a.at(0x10712734); a.value(9,0x43300000); a.exclusive_flags(0,21,0x80000000); a.store(1,0x3c,0);
    a.value(5,0x80010000); a.load(3,29,0x0); a.value(4,0x10000); a.store(1,0x38,9); a.offset(6,5,-0x7fff);
    a.floating_load(2,25,0x8); a.multiply(0,21,0xa); a.floating_load(1,1,0x38); a.offset(5,4,-0x1);
    a.floating_load(0,25,0x0); a.floating_difference(1,1,2); a.multiply_high(4,6,0); a.load<2>(8,3,0x0);
    a.load(7,26,0x0); a.floating_difference(1,1,0,true); a.store(1,0x40,9); a.add(7,8,7); a.multiply_high(6,6,7);
    a.add(0,4,0); a.add(4,6,7); a.shift_right(4,4,0xf); a.mask(6,4,0x1,false,1); a.shift_right(0,0,0xf);
    a.add(4,4,6); a.product(5,4,5); a.mask(4,0,0x1,false,1); a.add_record(20,0,4); a.subtract(0,5,7);
    a.exclusive_flags(0,0,0x80000000); a.store(1,0x44,0); a.floating_load(0,1,0x40); a.floating_difference(0,0,2);
    a.floating_product(0,1,0); a.floating_integer(0,0); a.floating_store(0,1,0x48); a.load(23,1,0x4c);
    if (!a.equal()) {
        a.at(0x107127dc);
        third_general_correction(a);
    } else {
        a.at(0x107127d4); a.value(25,0x0); a.jump(0x10712920);
    }
    a.at(0x10712920); a.value(3,0x66660000); a.offset(0,25,0x5); a.offset(3,3,0x6667); a.offset(4,21,-0x1);
    a.multiply_high(0,3,0); a.shift_right(0,0,0x2); a.mask(3,0,0x1,false,1); a.add(0,0,3); a.add(23,23,0);
    a.compare_registers(23,4);
    if (a.positive()) {
        a.at(0x1071294c); a.move(23,4); a.jump(0x10712980);
    }
    a.at(0x10712980);
}
void second_general_correction(Call& a) {
    a.at(0x107126f0); a.load(0,28,0x0); a.offset(3,3,0x2); a.store(29,0x0,3); a.compare_registers(3,0,false);
    if (a.positive()) {
        a.at(0x10712704); a.store(29,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(26,0x0,0);
    }
    a.at(0x10712714); a.value(3,0x10000); a.offset(0,3,-0x1); a.compare_registers(21,0);
    if (a.positive()) {
        a.at(0x10712734);
        second_general_scaled_correction(a);
    } else {
        a.at(0x10712724); a.value(3,0xffff0000); a.offset(0,3,0x1); a.compare_registers(21,0);
        if (a.at_least()) {
            a.at(0x10712954); a.load(3,29,0x0); a.compare(21,0); a.load(0,26,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,21); a.product(0,0,21); a.subtract(0,0,3); a.negate(23,0);
            if (a.positive()) {
                a.at(0x1071297c); a.move(23,0);
            }
            a.at(0x10712980);
        } else {
            a.at(0x10712734);
            second_general_scaled_correction(a);
        }
        a.at(0x10712980);
    }
    a.at(0x10712980);
}
void first_general_scaled_correction(Call& a) {
    a.at(0x10712648); a.value(9,0x43300000); a.exclusive_flags(0,22,0x80000000); a.store(1,0x3c,0);
    a.value(5,0x80010000); a.load(3,29,0x0); a.value(4,0x10000); a.store(1,0x38,9); a.offset(6,5,-0x7fff);
    a.floating_load(2,25,0x8); a.multiply(0,22,0xa); a.floating_load(1,1,0x38); a.offset(5,4,-0x1);
    a.floating_load(0,25,0x0); a.floating_difference(1,1,2); a.multiply_high(4,6,0); a.load<2>(8,3,0x0);
    a.load(7,26,0x0); a.floating_difference(1,1,0,true); a.store(1,0x40,9); a.add(7,8,7); a.multiply_high(6,6,7);
    a.add(0,4,0); a.add(4,6,7); a.shift_right(4,4,0xf); a.mask(6,4,0x1,false,1); a.shift_right(0,0,0xf);
    a.add(4,4,6); a.product(5,4,5); a.mask(4,0,0x1,false,1); a.add_record(21,0,4); a.subtract(0,5,7);
    a.exclusive_flags(0,0,0x80000000); a.store(1,0x44,0); a.floating_load(0,1,0x40); a.floating_difference(0,0,2);
    a.floating_product(0,1,0); a.floating_integer(0,0); a.floating_store(0,1,0x48); a.load(24,1,0x4c);
    if (!a.equal()) {
        a.at(0x107126f0);
        second_general_correction(a);
    } else {
        a.at(0x107126e8); a.value(23,0x0); a.jump(0x10712980);
    }
    a.at(0x10712980); a.value(3,0x66660000); a.offset(0,23,0x5); a.offset(3,3,0x6667); a.offset(4,22,-0x1);
    a.multiply_high(0,3,0); a.shift_right(0,0,0x2); a.mask(3,0,0x1,false,1); a.add(0,0,3); a.add(24,24,0);
    a.compare_registers(24,4);
    if (a.positive()) {
        a.at(0x107129ac); a.move(24,4); a.jump(0x107129e0);
    }
    a.at(0x107129e0);
}
void first_general_correction(Call& a) {
    a.at(0x10712604); a.load(0,28,0x0); a.offset(3,3,0x2); a.store(29,0x0,3); a.compare_registers(3,0,false);
    if (a.positive()) {
        a.at(0x10712618); a.store(29,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(26,0x0,0);
    }
    a.at(0x10712628); a.value(3,0x10000); a.offset(0,3,-0x1); a.compare_registers(22,0);
    if (a.positive()) {
        a.at(0x10712648);
        first_general_scaled_correction(a);
    } else {
        a.at(0x10712638); a.value(3,0xffff0000); a.offset(0,3,0x1); a.compare_registers(22,0);
        if (a.at_least()) {
            a.at(0x107129b4); a.load(3,29,0x0); a.compare(22,0); a.load(0,26,0x0); a.load<2>(3,3,0x0);
            a.add(3,3,0); a.divide(0,3,22); a.product(0,0,22); a.subtract(0,0,3); a.negate(24,0);
            if (a.positive()) {
                a.at(0x107129dc); a.move(24,0);
            }
            a.at(0x107129e0);
        } else {
            a.at(0x10712648);
            first_general_scaled_correction(a);
        }
        a.at(0x107129e0);
    }
    a.at(0x107129e0);
}
void scaled_general_range(Call& a) {
    a.at(0x1071255c); a.value(9,0x43300000); a.exclusive_flags(0,30,0x80000000); a.store(1,0x4c,0);
    a.value(5,0x80010000); a.load(3,29,0x0); a.value(4,0x10000); a.store(1,0x48,9); a.offset(6,5,-0x7fff);
    a.floating_load(2,25,0x8); a.multiply(0,30,0xa); a.floating_load(1,1,0x48); a.offset(5,4,-0x1);
    a.floating_load(0,25,0x0); a.floating_difference(1,1,2); a.multiply_high(4,6,0); a.load<2>(8,3,0x0);
    a.load(7,26,0x0); a.floating_difference(1,1,0,true); a.store(1,0x40,9); a.add(7,8,7); a.multiply_high(6,6,7);
    a.add(0,4,0); a.add(4,6,7); a.shift_right(4,4,0xf); a.mask(6,4,0x1,false,1); a.shift_right(0,0,0xf);
    a.add(4,4,6); a.product(5,4,5); a.mask(4,0,0x1,false,1); a.add_record(22,0,4); a.subtract(0,5,7);
    a.exclusive_flags(0,0,0x80000000); a.store(1,0x44,0); a.floating_load(0,1,0x40); a.floating_difference(0,0,2);
    a.floating_product(0,1,0); a.floating_integer(0,0); a.floating_store(0,1,0x38); a.load(31,1,0x3c);
    if (!a.equal()) {
        a.at(0x10712604);
        first_general_correction(a);
    } else {
        a.at(0x107125fc); a.value(24,0x0); a.jump(0x107129e0);
    }
    a.at(0x107129e0); a.value(3,0x66660000); a.offset(4,24,0x5); a.offset(3,3,0x6667); a.offset(0,30,-0x1);
    a.multiply_high(3,3,4); a.shift_right(3,3,0x2); a.mask(4,3,0x1,false,1); a.add(3,3,4); a.add(31,31,3);
    a.compare_registers(31,0);
    if (a.positive()) {
        a.at(0x10712a0c); a.move(31,0);
    }
    a.at(0x10712a10); a.move(3,31); a.jump(0x10712a44);
}
void advance_general_pool(Call& a) {
    a.at(0x10712514); a.load(3,29,0x0); a.load(0,28,0x0); a.offset(3,3,0x2); a.compare_registers(3,0,false);
    a.store(29,0x0,3);
    if (a.positive()) {
        a.at(0x1071252c); a.store(29,0x0,27); a.call(0x1077b770); a.mask(0,3,0xffff,false,0); a.store(26,0x0,0);
    }
    a.at(0x1071253c);
}
}

void sample_general(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,0x107124e0,dependencies,observer);
    a.at(0x107124e0); a.link(); a.multiple(20); a.move(30,3,true); a.load(25,2,-0x3ff8); a.load(26,2,-0x7b90);
    a.load(27,2,-0x7b8c); a.load(28,2,-0x7b88); a.load(29,2,-0x7b84); a.save_link(); a.frame(128);
    if (!a.equal()) {
        a.at(0x10712514);
        advance_general_pool(a);
        a.value(3,0x10000); a.offset(0,3,-0x1); a.compare_registers(30,0);
        if (a.positive()) {
            a.at(0x1071255c);
            scaled_general_range(a);
        } else {
            a.at(0x1071254c); a.value(3,0xffff0000); a.offset(0,3,0x1); a.compare_registers(30,0);
            if (a.at_least()) {
                a.at(0x10712a18);
                signed_general_range(a);
            } else {
                a.at(0x1071255c);
                scaled_general_range(a);
            }
            a.at(0x10712a44);
        }
        a.at(0x10712a44);
    } else {
        a.at(0x1071250c); a.value(3,0x0); a.jump(0x10712a44);
    }
    a.at(0x10712a44); a.pop(128); a.multiple(20,true); a.finish();
}
}
