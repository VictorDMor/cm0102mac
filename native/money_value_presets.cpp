#include "money_value_detail.hpp"
namespace cm::aot::money_value {
namespace detail {
void report_invalid_preset(Call& a,bool standard) {
    a.move(3,31);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(4,0);a.value(5,0);a.call(0x10739890);a.call(0x10768810);
    a.move(5,3);a.move(3,30);a.offset(4,31,9);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(8,standard?170:192);a.call(0x1077b720);
    a.move(4,30);a.offset(3,31,0x16);a.value(5,0);a.call(0x10769b60);a.load(3,2,-0x7bac);a.value(0,0);a.store<1>(3,0,0);
    a.load(3,29,0);a.jump(standard?0x1000a310:0x1000a424);
}
}
void select_standard_amount(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,select_standard_amount_entry,d,o);
    a.link();a.store(1,-4,31);a.load(31,2,-0x59d8);a.store(1,-8,30);a.load(30,2,-0x7ba8);a.store(1,-12,29);
    a.move(29,3);a.save_link();a.sign8(0,4,true);a.frame(0x70);bool valid=false;
    if(!a.less()){a.sign8(0,4);a.compare(0,7);valid=a.less();}
    if(!valid){a.at(0x1000a2a8);detail::report_invalid_preset(a,true);}
    else{a.at(0x1000a304);a.load(5,2,-0x59dc);a.value(6,7);a.call(select_table_amount_entry);}
    a.at(0x1000a310);a.pop(0x70);a.load(31,1,-4);a.load(30,1,-8);a.load(29,1,-12);a.finish();
}
void select_table_amount(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,select_table_amount_entry,d,o);
    a.link();a.store(1,-4,31);a.compare(5,0,false);a.load(31,2,-0x59d8);a.store(1,-8,30);a.load(30,2,-0x7ba8);
    a.store(1,-12,29);a.move(29,3);a.save_link();a.frame(0x80);bool valid=false;
    if(!a.equal()){a.sign8(3,4);a.sign16(0,6);a.compare_registers(3,0);if(!a.at_least()){a.sign8(0,4,true);valid=a.at_least();}}
    if(!valid){a.at(0x1000a374);detail::report_invalid_preset(a,false);}
    else {
        a.at(0x1000a3d0);a.mask(3,3,0xfffffffc,false,2);a.value(0,0x43300000);a.indexed_load(3,5,3);a.load(4,2,-0x59d4);
        a.exclusive_flags(3,3,0x80000000);a.store(1,0x68,0);a.floating_load(1,4,0);a.store(1,0x6c,3);a.floating_load(2,2,-0x3af4);
        a.floating_load(0,1,0x68);a.floating_subtract(0,0,1);a.floating_product(1,2,0);a.call(0x10773de0);a.call(0x10773c80);
        a.floating_load(0,2,-0x3aec);a.floating_difference(0,1,0,true);a.floating_integer(0,0);a.floating_store(0,1,0x60);a.load(0,1,0x64);a.store(29,0,0);a.load(3,29,0);
    }
    a.at(0x1000a424);a.pop(0x80);a.load(31,1,-4);a.load(30,1,-8);a.load(29,1,-12);a.finish();
}
}
