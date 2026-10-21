#include "ball_contact.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_contact {
namespace {
using Call=SemanticGuestCall;
void sample_description_byte(Call& a,bool second) {
    a.at(second?0x101f5414:0x101f53b0);a.value(3,102);a.call(0x101d3c90);a.move(7,3);a.move(3,7);a.call(0x101d3c90);
    if(second){a.move(6,3);a.sign8(8,6);a.store<1>(1,0x39,8);a.load<1>(7,1,0x39);a.sign8(7,7);a.compare(7,50);}
    else {a.move(0,3);a.sign8(0,0);a.store<1>(1,0x38,0);a.load<1>(7,1,0x38);a.sign8(8,7);a.compare(8,50);}
    if(a.nonpositive())return;
    a.value(3,3);a.call(0x101d3c90);a.move(second?0:7,3);a.compare(second?0:7,0);if(a.equal())return;
    a.value(3,76);a.call(0x101d3c90);a.move(second?7:0,3);a.move(3,second?7:0);a.call(0x101d3c90);
    if(second){a.move(6,3);a.sign8(5,6);a.store<1>(1,0x39,5);}else {a.move(5,3);a.sign8(6,5);a.store<1>(1,0x38,6);}
}
void adjust_fitness_weight(Call& a,bool second) {
    const auto start=second?0x101f56a4:0x101f54e0,weight=second?0x44:0x40;const unsigned player=second?26:27,divisor=second?28:29;
    a.at(start);a.floating_load(2,player,0x50,true);a.floating_load(1,28,0x54,true);a.floating_load(0,28,0xc8,true);a.floating_product_sum(1,2,1,0);
    a.floating_load(0,28,0xdc,true);a.floating_compare(1,0);
    if(!a.nonpositive()){a.floating_load(2,player,0x50,true);a.floating_load(1,28,0x54,true);a.floating_load(0,28,0xc8,true);a.floating_product_sum(divisor,2,1,0);a.jump(start+0x34);}
    else {a.at(start+0x30);a.floating_load(divisor,28,0xdc,true);}
    a.at(start+0x34);a.floating_load(1,1,weight,true);a.floating_load(0,28,0xcc,true);a.floating_difference(0,0,divisor,true,true);
    a.floating_product_single(0,1,0);a.floating_store_single(0,1,weight);
}
void adjust_profile_weight(Call& a,bool second) {
    const auto start=second?0x101f56ec:0x101f5528;const int weight=second?0x44:0x40;const unsigned player=second?26:27;
    // The receiver's gate deliberately reads the initiating player's profile.
    // A null initiating player may fault here even with a nonnull receiver.
    a.at(start);a.load(second?7:8,27,0x70);a.load<1>(second?6:7,second?7:8,0x2d);a.sign8(second?0:3,second?6:7);a.compare(second?0:3,10);
    if(!a.nonpositive()) {
        a.floating_load(2,1,weight,true);a.load(second?6:4,player,0x70);a.load<1>(second?0:3,second?6:4,0x2d);a.sign8(second?0:23,second?0:3);
        a.floating_load(1,30,0x60);a.exclusive_flags(second?7:5,second?0:23,0x80000000);a.store(1,0x5c,second?7:5);a.value(0,0x43300000);a.store(1,0x58,0);
        a.floating_load(0,1,0x58);a.floating_subtract(0,0,1,true);a.floating_product_single(1,2,0);a.floating_load(0,28,0x28,true);
        a.floating_difference(0,1,0,true,true);a.floating_store_single(0,1,weight);a.jump(start+0x9c);return;
    }
    a.at(start+0x54);a.floating_load(3,1,weight,true);a.floating_load(2,28,0x10,true);a.load(7,player,0x70);a.load<1>(second?5:6,7,0x2d);
    a.sign8(second?6:7,second?5:6);a.offset(0,second?6:7,-10);a.floating_load(1,30,0x60);a.exclusive_flags(7,0,0x80000000);a.store(1,0x5c,7);
    a.value(second?6:0,0x43300000);a.store(1,0x58,second?6:0);a.floating_load(0,1,0x58);a.floating_subtract(1,0,1,true);
    a.floating_load(0,28,0x28,true);a.floating_difference(0,1,0,true,true);a.floating_sum_single(0,2,0);a.floating_product_single(0,3,0);a.floating_store_single(0,1,weight);
}
void adjust_context_weight(Call& a,bool second) {
    const auto start=second?0x101f5788:0x101f55c4;const int weight=second?0x44:0x40;const unsigned player=second?26:27,factor=second?30:31;
    a.at(start);a.load(3,25,0);a.load(4,player,0x6c);a.call(0x1018c270);a.move(7,3);a.sign8(second?0:6,7);a.compare(second?0:6,0);if(a.equal())return;
    a.load(3,25,0);a.load(4,player,0x6c);a.call(0x1018c270);a.move(second?0:5,3);a.sign8(second?8:0,second?0:5);a.floating_load(1,30,0x60);
    a.exclusive_flags(second?7:0,second?8:0,0x80000000);a.store(1,0x5c,second?7:0);a.value(second?5:6,0x43300000);a.store(1,0x58,second?5:6);
    a.floating_load(0,1,0x58);a.floating_subtract(2,0,1);a.floating_load(1,30,0x90);a.floating_load(0,30,0x1b8);a.floating_product_sum(factor,1,2,0);
    a.floating_round_single(factor,factor);a.floating_load(0,30,0x40);a.floating_compare(factor,0);if(!a.nonpositive())a.floating_load(factor,28,0x108,true);
    a.at(start+0x6c);a.floating_load(0,1,weight,true);a.floating_product_single(0,0,factor);a.floating_store_single(0,1,weight);
}
void prepare_player_weight(Call& a,bool second) {
    const auto start=second?0x101f563c:0x101f5478;const int weight=second?0x44:0x40;const unsigned player=second?26:27;
    a.at(start);a.compare(player,0,false);if(a.equal())return;
    a.load(3,25,0);a.load(4,player,0x6c);a.call(0x1018c340);a.move(24,3);a.compare(24,0,false);
    if(!a.equal()) {
        a.floating_load(1,28,0x11c,true);a.floating_load(0,1,weight,true);a.floating_product_single(0,1,0);a.floating_store_single(0,1,weight);
        a.load(3,25,0);a.load<1>(4,24,0);a.call(0x1018c180);a.move(0,3);a.store<1>(1,second?0x3b:0x3a,0);
        a.value(3,25);a.call(0x101d3c90);a.move(second?7:6,3);a.compare(second?7:6,0);
        if(!a.equal()){a.load<1>(second?0:7,1,second?0x39:0x38);a.mask(second?7:0,second?0:7,0xfffffffe,false,1);a.sign8(0,second?7:0);a.store<1>(1,second?0x39:0x38,0);}
    }
    adjust_fitness_weight(a,second);adjust_profile_weight(a,second);adjust_context_weight(a,second);
}
}
// Keep fixed register selections visible to the optimizer across both players.
[[gnu::flatten]] void prepare_description(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,description_entry,dependencies,observer);a.link();a.move(11,1);a.call(0x10773804);a.multiple(21,false,-0x30);a.save_link();a.frame(0xc0);
    a.move(29,3);a.move(27,4);a.store<2>(1,0xe2,5);a.move(26,6);a.store<2>(1,0xea,7);a.store<1>(1,0xef,8);
    a.load(25,2,-0x7ba0);a.load(28,2,-0x52c8);a.load(30,2,-0x52c4);a.load<2>(0,2,-0x3914);a.store<2>(1,0x3a,0);a.offset(22,1,0x3a);
    a.load(5,2,-0x52dc);a.load(6,5,0);a.load(0,5,4);a.store(1,0x40,6);a.store(1,0x44,0);a.offset(21,1,0x40);
    sample_description_byte(a,false);sample_description_byte(a,true);prepare_player_weight(a,false);prepare_player_weight(a,true);state.pc=description_exit_phase;
}
}
