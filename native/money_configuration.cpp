#include "money_configuration_detail.hpp"
namespace cm::aot::money_configuration {
void configure_currency(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    detail::Call a(s,configure_currency_entry,d,o);
    a.link();a.multiple(24);a.move(31,3);a.load(24,2,-0x59f0);a.load(25,2,-0x59ec);a.load(26,2,-0x59e8);
    a.load(27,2,-0x59e4);a.load(28,2,-0x59e0);a.load(29,2,-0x7ba8);a.load(30,2,-0x59d8);a.load(5,2,-0x59d4);
    a.load(3,2,-0x59d0);a.store(1,0x8,0);a.sign8(0,31);a.compare(0,24,false);a.frame(0x80);
    if(a.positive()){
        a.at(0x1000b0dc);
    a.move(3,30);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(4,0x0);a.value(5,0x0);a.call(0x10739890);
    a.call(0x10768810);a.move(5,3);a.move(3,29);a.offset(4,30,0x9);a.offset(6,1,0x40);a.offset(7,1,0x38);
    a.value(8,0x252);a.call(0x1077b720);a.move(4,29);a.offset(3,30,0x16);a.value(5,0x0);a.call(0x10769b60);
    a.load(4,2,-0x7bac);a.value(0,0x0);a.value(3,0x0);a.store<1>(4,0x0,0);
    }else{
    a.load(4,2,-0x59f4);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(4,4,0);a.count_destination(4);
        // One bounded selection from the original immutable dispatch bank.
        detail::apply_profile(a,a.count_branch());
    }
    a.at(0x1000b134);
    a.load(0,1,0x88);a.offset(1,1,0x80);a.restore_link();a.multiple(24,true);a.finish();
}
}
