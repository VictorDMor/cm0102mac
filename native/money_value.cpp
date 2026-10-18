#include "money_value_detail.hpp"
namespace cm::aot::money_value {
void to_display_amount(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,to_display_amount_entry,d,o);
    a.link();a.store(1,-4,31);a.load(5,2,-0x59d4);a.store(1,-8,30);a.floating_load(2,5,0);
    a.store(1,-12,29);a.store(1,-16,28);a.move(28,4);a.save_link();a.value(0,0x43300000);a.frame(0x50);
    a.load(3,3,0);a.store(1,0x38,0);a.exclusive_flags(3,3,0x80000000);a.floating_load(0,2,-0x3aec);
    a.store(1,0x3c,3);a.floating_load(1,1,0x38);a.floating_subtract(1,1,2);a.floating_product(1,1,0);a.call(0x10773de0);
    a.mask(0,28,0x20,true);a.move(30,4);a.move(31,3);
    if(a.equal())detail::align_scaled_pair(a,31,30,28,29,28,true);
    a.at(0x10009fe0);a.load(0,1,0x58);a.move(4,30);a.move(3,31);a.offset(1,1,0x50);a.restore_link();
    a.load(31,1,-4);a.load(30,1,-8);a.load(29,1,-12);a.load(28,1,-16);a.finish();
}
void from_display_amount(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,from_display_amount_entry,d,o);
    a.link();a.store(1,-4,31);a.move(31,3);a.move(3,4);a.save_link();a.move(4,5);a.frame(0x50);a.call(0x10773c80);
    a.floating_load(0,2,-0x3aec);a.floating_difference(0,1,0,true);a.floating_integer(0,0);a.floating_store(0,1,0x38);
    a.load(0,1,0x3c);a.store(31,0,0);a.pop(0x50);a.load(31,1,-4);a.finish();
}
void increase_amount(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,increase_amount_entry,d,o);
    a.link();a.multiple(26);a.move(30,4);a.load(4,2,-0x59d4);a.move(29,3);a.floating_load(2,4,0);a.save_link();
    a.value(0,0x43300000);a.frame(0x60);a.load(3,3,0);a.store(1,0x38,0);a.exclusive_flags(3,3,0x80000000);
    a.floating_load(0,2,-0x3aec);a.store(1,0x3c,3);a.floating_load(1,1,0x38);a.floating_subtract(1,1,2);
    a.floating_product(1,1,0);a.call(0x10773de0);a.mask(0,30,0x20,true);a.move(31,4);a.move(26,3);
    if(a.equal())detail::align_scaled_pair(a,26,31,30,28,27);
    a.at(0x1000a110);a.value(3,1);a.value(0,0);a.add_register_carry(4,31,3);a.move(5,30);a.add_extended(3,26,0);a.call(amount_quantum_entry);
    a.add_register_carry(4,31,4);a.add_extended(3,26,3);a.call(0x10773c80);a.floating_load(0,2,-0x3aec);
    a.floating_difference(0,1,0,true);a.floating_integer(0,0);a.floating_store(0,1,0x38);a.load(0,1,0x3c);a.store(29,0,0);a.load(3,29,0);
    a.pop(0x60);a.multiple(26,true);a.finish();
}
void decrease_amount(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,decrease_amount_entry,d,o);
    a.link();a.multiple(26);a.move(31,4);a.load(4,2,-0x59d4);a.move(30,3);a.floating_load(2,4,0);a.save_link();
    a.value(0,0x43300000);a.frame(0x60);a.load(3,3,0);a.store(1,0x38,0);a.exclusive_flags(3,3,0x80000000);
    a.floating_load(0,2,-0x3aec);a.store(1,0x3c,3);a.floating_load(1,1,0x38);a.floating_subtract(1,1,2);
    a.floating_product(1,1,0);a.call(0x10773de0);a.mask(0,31,0x20,true);a.move(26,4);a.move(27,3);
    if(a.equal())detail::align_scaled_pair(a,27,26,31,29,28);
    a.at(0x1000a220);a.move(4,26);a.move(3,27);a.move(5,31);a.call(amount_quantum_entry);
    a.subtract_register_carry(4,4,26);a.subtract_extended(3,3,27);a.call(0x10773c80);a.floating_load(0,2,-0x3aec);
    a.floating_difference(0,1,0,true);a.floating_integer(0,0);a.floating_store(0,1,0x38);a.load(0,1,0x3c);a.store(30,0,0);a.load(3,30,0);
    a.pop(0x60);a.multiple(26,true);a.finish();
}
}
