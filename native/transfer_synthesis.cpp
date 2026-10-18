#include "transfer_synthesis_detail.hpp"

namespace cm::aot::transfer_synthesis {
void synthesize_offer(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){
    detail::Call a(s,synthesize_offer_entry,d,o);
    a.link();a.floating_store(31,1,-8);a.multiple(13,false,-16);a.move(16,4,true);a.load(25,2,-0x4b8c);a.move(15,3);a.load(22,2,-0x7be0);a.move(17,5);a.load(26,2,-0x7b40);a.move(18,6);a.load(27,2,-0x7b7c);a.move(19,7);a.load(28,2,-0x7b90);a.move(21,8);a.load(29,2,-0x7b8c);a.move(20,9);a.load(30,2,-0x7b88);a.value(14,0);a.load(31,2,-0x7b84);a.load(23,2,-0x7ba8);a.load(13,2,-0x4b88);a.save_link();a.value(0,0);a.frame(304);a.store<1>(1,0xc4,0);a.value(0,0xffffffff);a.store(1,0xc0,0);
    const auto route=detail::prepare_offer(a);
    if(route!=detail::Route::failed && (route==detail::Route::finalize || detail::price_offer(a)))detail::finalize_offer(a);
    a.at(0x10415e34);a.load(0,1,0x138);a.offset(1,1,304);a.floating_load(31,1,-8);a.restore_link();a.multiple(13,true,-16);a.finish();
}
}
