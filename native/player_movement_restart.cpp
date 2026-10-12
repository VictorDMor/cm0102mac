#include "player_movement.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;
void advance_restart_random(Call& a,std::uint32_t site) {
    a.at(site);a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}a.at(site+0x28);
}
void describe_restart_candidate(Call& a,std::uint32_t site,std::uint32_t code,std::uint32_t branch=0) {
    a.at(site);a.load(3,15,0x1a4);a.value(4,code);a.load<1>(0,15,0x27);a.value(8,UINT32_MAX);a.load(3,3,0x79f4);
    a.value(9,UINT32_MAX);a.load<1>(6,13,0x2f);a.sign8(7,0);a.load(5,15,0x1a4);a.value(10,0);a.call(0x10201180);
    if(branch)a.jump(branch);
}
void describe_restart_fallback(Call& a,std::uint32_t site,bool branch) {
    a.at(site);a.load(3,15,0x1a4);a.move(4,17);a.load<1>(0,15,0x27);a.value(6,UINT32_MAX);a.load(3,3,0x79f4);
    a.value(8,UINT32_MAX);a.load(5,15,0x1a4);a.sign8(7,0);a.value(9,UINT32_MAX);a.value(10,0);a.call(0x10201180);
    if(branch)a.jump(0x10231390);
}
bool restart_qualified(Call& a) {a.move(3,15);a.call(0x10272150);a.sign8(0,3,true);return !a.equal();}
void choose_restart_recipient(Call& a) {
    a.sign16(0,17);a.compare(0,0x1f7a);if(a.equal())return;
    a.load(4,15,0x1a4);a.value(6,0);a.load(3,15,0x1a4);a.load<1>(5,4,0x7928);a.load<1>(4,15,0x27);
    a.call(0x101f3a30);a.move(13,3,true);
    if(a.equal()){describe_restart_fallback(a,0x102311e0,true);return;}
    a.at(0x10231210);a.load<1>(0,1,0x47);a.compare(0,7);
    if(a.equal())describe_restart_candidate(a,0x1023121c,0x21e2,0x10231278);else describe_restart_fallback(a,0x1023124c,false);
    a.at(0x10231278);a.sign16(0,17);a.compare(0,0x1f79);if(!a.equal())return;
    a.load<1>(0,1,0x40);a.sign8(0,0,true);if(!a.equal())return;
    a.load<1>(0,1,0x47);a.compare(0,7);
    if(a.equal()){
        if(!restart_qualified(a))return;
        a.load<1>(0,15,0x27);a.move(3,15);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);
        a.sign8(4,0);a.call(0x10239d10);a.sign8(0,3,true);if(!a.equal())return;
        describe_restart_candidate(a,0x102312d0,0x21e1,0x10231390);return;
    }
    a.at(0x10231300);a.load<1>(4,15,0x105);a.move(3,15);a.call(0x102720a0);a.sign8(0,3,true);
    if(!a.equal() && restart_qualified(a)){describe_restart_candidate(a,0x10231324,0x21df,0x10231390);return;}
    a.at(0x10231354);if(restart_qualified(a))describe_restart_candidate(a,0x10231364,0x21e0);
}
void describe_restart_opponent(Call& a,std::uint32_t site,std::uint32_t code,bool branch=false) {
    a.at(site);a.load<1>(0,15,0x27);a.value(4,code);a.load(3,15,0x1a4);a.value(6,UINT32_MAX);a.sign8(0,0);
    a.load(5,15,0x1a4);a.leading_zeroes(0,0);a.load(3,3,0x79f4);a.mask(0,0,0x07ffffff,false,27);
    a.value(8,UINT32_MAX);a.sign8(7,0);a.value(9,UINT32_MAX);a.value(10,0);a.call(0x10201180);
    if(branch)a.jump(0x10231668);
}
bool contact_has_restart_comment(Call& a,std::uint32_t site) {
    a.at(site);a.load<1>(3,1,0x47);a.offset(0,3,-3);a.mask(0,0,0xff);a.compare(0,3,false);
    if(a.nonpositive())return true;a.sign16(0,17);a.compare(0,0x1f7a);return a.equal();
}
bool even_restart_sample(Call& a,std::uint32_t site) {
    advance_restart_random(a,site);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(0,3,0);
    a.mask(3,0,1,false,1);a.mask(0,0,1);a.combine(0,0,3,false,true);a.subtract(0,3,0,true);return a.equal();
}
void describe_flagged_restart(Call& a) {
    a.load(3,15,0x1a4);a.value(4,0x20dd);a.load<1>(7,15,0x27);a.value(8,UINT32_MAX);a.load(3,3,0x79f4);
    a.value(9,UINT32_MAX);a.load<1>(6,15,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);
    a.load<1>(0,1,0x47);a.compare(0,4);bool mark=a.equal();
    if(!mark){a.load<1>(0,1,0x47);a.compare(0,5);mark=a.equal();}if(mark){a.at(0x102313f0);a.value(0,1);a.store<1>(22,0x16,0);}
    if(contact_has_restart_comment(a,0x102313f8) && even_restart_sample(a,0x10231418))describe_restart_opponent(a,0x10231464,0x20d9,true);
}
void describe_unflagged_restart(Call& a) {
    advance_restart_random(a,0x102314a0);a.load(4,30,0);a.value(3,0x38e40000);a.load(0,27,0);a.offset(3,3,-0x71c7);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);
    a.add(0,0,3);a.multiply(0,0,9);a.subtract(0,0,4,true);if(!a.equal())return;
    describe_restart_opponent(a,0x102314fc,0x20df);
    advance_restart_random(a,0x10231534);a.load(4,30,0);a.value(3,0x55550000);a.load(0,27,0);a.offset(3,3,0x5556);
    a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(3,3,4);a.mask(0,3,1,false,1);a.add(0,3,0);
    a.multiply(0,0,3);a.subtract(0,0,4,true);if(a.equal())describe_restart_opponent(a,0x1023158c,0x20e0);
    if(contact_has_restart_comment(a,0x102315c4) && even_restart_sample(a,0x102315e4))describe_restart_opponent(a,0x10231630,0x20d9);
}
void apply_contact_restart(Call& a) {
    a.at(0x10231390);a.load(3,15,0x1a4);a.load<1>(4,1,0x46);a.call(0x101ee530);a.sign8(0,3,true);
    if(a.equal()){
        a.load<1>(0,1,0x40);a.sign8(0,0,true);
        if(!a.equal())describe_flagged_restart(a);else describe_unflagged_restart(a);
        a.at(0x10231668);a.load(3,15,0x1a4);a.move(4,22);a.floating_load(1,1,0x4c,true);a.call(0x101d6780);
        a.move(3,22);a.value(4,0);a.call(0x102884d0);
    }
    a.at(0x10231684);a.load<1>(4,15,0x105);a.value(0,0);a.load(3,15,0x1a4);a.store<1>(3,0x791d,4);
    a.load<1>(4,15,0x106);a.load(3,15,0x1a4);a.store<1>(3,0x791e,4);a.load<1>(4,15,0x27);a.load(3,15,0x1a4);
    a.store<1>(3,0x7924,4);a.load(3,15,0x1a4);a.offset(3,3,0x10000);a.store(3,-0x2000,0);
    a.load<2>(3,22,0x36,true);a.offset(0,3,-25);a.store<2>(22,0x36,0);a.jump(common_return_phase);
}
}
void continue_carrier_restart(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,carrier_restart_selection_phase,dependencies,observer);choose_restart_recipient(a);apply_contact_restart(a);state.pc=common_return_phase;
}
}
