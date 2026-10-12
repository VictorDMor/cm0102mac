#include "player_movement.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::player_movement {
namespace {
using Call = SemanticGuestCall;

void advance_interaction_random(Call& a,std::uint32_t start) {
    a.at(start);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
    a.at(start+0x28);
}

void query_neighbour(Call& a) {
    a.offset(0,1,0x43);a.sign16(3,22);a.store(1,0x38,0);a.value(0,0);a.move(4,15);a.add(21,21,3);
    a.store(1,0x3c,0);a.sign8(6,18);a.offset(7,1,0x45);a.offset(8,1,0x44);a.load(3,15,0x1a4);
    a.offset(9,1,0x48);a.load<1>(5,15,0x27);a.offset(10,1,0x46);a.call(0x101eaf60);a.move(22,3,true);
    if(!a.equal()){a.load<1>(7,1,0x46);a.move(4,15);a.load(3,15,0x1a4);a.move(5,18);a.move(6,22);
        a.sign8(7,7);a.value(8,0);a.call(0x101f5350);}
}

bool commentary_allowed(Call& a) {
    for(auto [site,kind]:{std::pair{0x1022fabcu,18},std::pair{0x1022fac8u,17},std::pair{0x1022fad4u,16},std::pair{0x1022fae0u,15}}) {
        a.at(site);a.load<2>(0,15,0x19c,true);a.compare(0,kind);if(a.equal())return false;
    }
    a.move(3,15);a.call(0x10289560);a.sign16(0,3);a.compare(0,1);if(a.equal())return false;
    advance_interaction_random(a,0x1022fb00);a.load(4,30,0);a.value(3,0x66660000);a.load(0,27,0);
    a.offset(3,3,0x6667);a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,3);
    a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,20);a.subtract(0,0,4,true);return a.equal();
}

void scaled_commentary_strength(Call& a,std::uint32_t site,std::int32_t spill,unsigned result) {
    a.at(site);a.move(3,15);a.call(0x10245160);a.floating_load(2,31,0x8c,true);a.value(3,0x66660000);
    a.floating_load(0,26,0xc0);a.offset(3,3,0x6667);a.floating_product_single(1,2,1);a.floating_sum(0,0,1);
    a.floating_integer(0,0);a.floating_store(0,1,spill);a.load(0,1,spill+4);a.sign8(0,0);
    a.multiply_high(0,3,0);a.shift_right(0,0,1);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(result,0,5);
}

void queue_commentary(Call& a,std::uint32_t site,std::uint32_t code,bool branch=true) {
    a.at(site);a.load(3,15,0x1a4);a.sign8(10,23);a.load<1>(7,15,0x27);a.value(4,code);a.load(3,3,0x79f4);
    a.value(8,UINT32_MAX);a.load<1>(6,15,0x2f);a.value(9,UINT32_MAX);a.load(5,15,0x1a4);a.call(0x10201180);
    if(branch)a.jump(carrier_contact_phase);
}

// Mirrored flank tests retain every original load and each conditional step.
bool in_flank(Call& a,std::uint32_t site,bool left) {
    a.at(site);a.load<1>(0,15,0x27);a.sign8(0,0,true);
    if(a.equal()){a.load<1>(0,15,0x105);a.sign8(0,0);a.compare(0,left?2:6);
        if(left?a.less():a.positive())return true;}
    a.at(site+0x1c);a.load<1>(0,15,0x27);a.compare(0,1);if(!a.equal())return false;
    a.load<1>(0,15,0x105);a.sign8(0,0);a.compare(0,left?6:2);return left?!a.nonpositive():!a.at_least();
}

bool in_forward_zone(Call& a) {
    a.load<1>(0,15,0x27);a.sign8(23,3);a.sign8(0,0,true);
    if(a.equal()){a.load<1>(0,15,0x106);a.sign8(0,0);a.compare(0,7);if(a.positive())return true;}
    a.at(0x1022fc14);a.load<1>(0,15,0x27);a.compare(0,1);if(!a.equal())return false;
    a.load<1>(0,15,0x106);a.sign8(0,0);a.compare(0,4);return !a.at_least();
}

bool in_backward_zone(Call& a) {
    a.at(0x1022fd48);a.load<1>(0,15,0x27);a.compare(0,1);
    if(a.equal()){a.load<1>(0,15,0x106);a.sign8(0,0);a.compare(0,7);if(a.positive())return true;}
    a.at(0x1022fd64);a.load<1>(0,15,0x27);a.sign8(0,0,true);if(!a.equal())return false;
    a.load<1>(0,15,0x106);a.sign8(0,0);a.compare(0,4);return !a.at_least();
}

void positional_commentary(Call& a) {
    scaled_commentary_strength(a,0x1022fb5c,0xbc,0);a.compare(0,1);
    if(!a.at_least()){a.value(3,1);a.jump(0x1022fbf4);}
    else scaled_commentary_strength(a,0x1022fbb0,0xc4,3);
    a.at(0x1022fbf4);
    if(in_forward_zone(a)) {
        if(in_flank(a,0x1022fc30,true)){queue_commentary(a,0x1022fc68,0x2160);return;}
        if(in_flank(a,0x1022fc94,false)){queue_commentary(a,0x1022fccc,0x2162);return;}
        a.at(0x1022fcf8);a.load<1>(0,15,0x27);a.move(3,15);a.sign8(0,0);a.leading_zeroes(0,0);
        a.mask(0,0,0x07ffffff,false,27);a.sign8(4,0);a.call(0x10239d10);a.sign8(0,3,true);if(!a.equal())return;
        queue_commentary(a,0x1022fd1c,0x2161);return;
    }
    if(in_backward_zone(a)) {
        if(in_flank(a,0x1022fd80,true)){queue_commentary(a,0x1022fdb8,0x215a);return;}
        if(in_flank(a,0x1022fde4,false)){queue_commentary(a,0x1022fe1c,0x215c);return;}
        queue_commentary(a,0x1022fe48,0x215b);return;
    }
    if(in_flank(a,0x1022fe74,true)){queue_commentary(a,0x1022feac,0x215d);return;}
    if(in_flank(a,0x1022fed8,false)){queue_commentary(a,0x1022ff10,0x215f);return;}
    a.at(0x1022ff3c);a.load<1>(0,15,0x105);a.sign8(3,0);a.offset(3,3,-4);a.call(0x10775540);
    a.compare(3,2);if(a.at_least())return;a.load<1>(0,15,0x106);a.compare(0,5);
    if(!a.equal()){a.load<1>(0,15,0x106);a.compare(0,6);if(!a.equal())return;}
    advance_interaction_random(a,0x1022ff6c);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(0,3,0);
    a.mask(3,0,1,false,1);a.mask(0,0,1);a.combine(0,0,3,false,true);a.subtract(0,3,0,true);if(a.equal())return;
    queue_commentary(a,0x1022ffb8,0x215e,false);
}
}

void continue_carrier_interaction(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call call(state,carrier_interaction_phase,dependencies,observer);query_neighbour(call);
    if(commentary_allowed(call))positional_commentary(call);state.pc=carrier_contact_phase;
}
}
