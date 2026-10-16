#include "ball_contact.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_contact {
namespace {
using Call=SemanticGuestCall;
std::uint32_t selected_actor_reaction(Call& a) {
    a.load<1>(0,1,0x3a);a.sign8(0,0);a.compare(0,3);if(a.equal())return 0x101f58cc;
    if(a.at_least()){a.at(0x101f58a0);a.compare(0,5);if(a.equal())return 0x101f5920;if(a.at_least())return 0x101f593c;a.jump(0x101f58e8);return 0x101f58e8;}
    a.compare(0,1);if(a.equal())return 0x101f58b0;if(a.at_least())return 0x101f5904;a.jump(0x101f593c);return 0x101f593c;
}
std::uint32_t random_actor_reaction(Call& a) {
    a.at(0x101f593c);a.value(3,11);a.call(0x101d3c90);a.move(0,3);a.compare(0,8);
    if(a.at_least()){a.at(0x101f5970);a.compare(0,10);if(a.equal())return 0x101f59f0;if(a.at_least())return 0x101f5a08;a.jump(0x101f59d4);return 0x101f59d4;}
    a.compare(0,3);if(a.at_least()){a.at(0x101f5964);a.compare(0,6);if(a.at_least())return 0x101f59b8;a.jump(0x101f599c);return 0x101f599c;}
    a.compare(0,0);if(a.at_least())return 0x101f5980;a.jump(0x101f5a08);return 0x101f5a08;
}
void actor_reaction(Call& a,std::uint32_t start,unsigned kind,std::uint32_t join) {
    a.at(start);a.move(3,29);a.value(4,kind);a.load<1>(5,1,0x38);a.move(6,27);a.value(7,0);a.call(0x101f6940);if(start!=0x101f59f0)a.jump(join);
}
}
void select_description_exit(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,description_exit_phase,dependencies,observer);a.compare(27,0,false);
    if(!a.equal()) {
        a.floating_load(1,27,0x50,true);a.floating_load(0,28,0xd8,true);a.floating_compare(1,0);
        if(!a.at_least()){a.move(3,29);a.value(4,0xffffffff);a.value(5,0);a.move(6,27);a.value(7,0);a.call(0x101f6940);a.jump(description_return_phase);state.pc=description_return_phase;return;}
    }
    a.at(0x101f5834);a.load<1>(6,1,0xef);a.sign8(0,6);a.compare(0,0);if(a.equal()){state.pc=description_players_phase;return;}
    a.compare(27,0,false);if(a.equal()){state.pc=description_players_phase;return;}
    a.value(3,0x10000);a.offset(3,3,-0x3cb0);a.call(0x101d3c90);a.move(23,3);a.floating_load(1,28,0x28,true);a.floating_load(0,1,0x40,true);
    a.floating_product_single(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x58);a.load(6,1,0x5c);a.compare_registers(23,6);
    if(!a.at_least()) {
        auto target=selected_actor_reaction(a);bool fallback=target==0x101f593c;if(fallback)target=random_actor_reaction(a);
        unsigned kind=0;
        switch(target){case 0x101f58b0:case 0x101f5980:kind=1;break;case 0x101f58cc:case 0x101f599c:kind=3;break;case 0x101f58e8:case 0x101f59b8:kind=4;break;case 0x101f5904:case 0x101f59d4:kind=2;break;case 0x101f5920:case 0x101f59f0:kind=5;break;default:break;}
        if(kind){actor_reaction(a,target,kind,fallback?0x101f5a08:description_return_phase);if(!fallback){state.pc=description_return_phase;return;}}
    }
    a.at(0x101f5a08);a.jump(description_return_phase);state.pc=description_return_phase;
}
void prepare_description_players(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,description_players_phase,dependencies,observer);a.store(1,0x48,27);a.store(1,0x4c,26);a.load<2>(0,1,0xe2,true);a.store<2>(1,0x3c,0);
    a.load<2>(7,1,0xea,true);a.store<2>(1,0x3e,7);a.value(31,0);a.jump(description_test_phase);state.pc=description_test_phase;
}
void next_description_player(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,state.pc,dependencies,observer);if(state.pc==description_next_phase)a.offset(31,31,1);
    a.at(description_test_phase);a.sign8(7,31);a.compare(7,2);state.pc=a.less()?description_weight_phase:description_return_phase;
}
void return_description_frame(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,description_return_phase,dependencies,observer);a.load(0,1,0xc8);a.offset(1,1,0xc0);a.move(11,1);a.call(0x10773854);a.restore_link();a.multiple(21,true,-0x30);a.finish();
}
}
