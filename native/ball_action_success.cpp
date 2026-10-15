#include "ball_action.hpp"
#include "ball_action_detail.hpp"

namespace cm::aot::ball_action {
namespace {
using Call=SemanticGuestCall;

void emit_actor(Call& a,std::uint32_t start,unsigned message) {
    a.at(start);a.load(3,15,0x1a4);a.value(4,message);a.load<1>(7,15,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);
    a.value(9,0xffffffff);a.load<1>(6,15,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);
}

void emit_pair(Call& a,std::uint32_t start,unsigned message,bool jump=true) {
    a.at(start);a.load(3,15,0x1a4);a.value(4,message);a.load(5,15,0x1a4);a.value(10,0);a.load(3,3,0x79f4);
    a.load<1>(6,15,0x2f);a.load<1>(7,15,0x27);a.load<1>(8,14,0x2f);a.load<1>(9,14,0x27);a.call(0x10201180);
    if(jump)a.jump(0x10242934);
}

bool has_both_profiles(Call& a,std::uint32_t start,std::uint32_t join) {
    a.at(start);a.load(0,15,0x70);a.compare(0,0,false);if(a.equal())return false;
    a.load(0,14,0x70);a.compare(0,0,false);if(a.equal())return false;
    a.at(join);return true;
}

void describe_special_receiver(Call& a) {
    if(has_both_profiles(a,0x102426dc,0x102426f4)) {
        detail::advance_action_random(a,0x102426f4);a.load(5,30,0);a.value(4,0x55550000);a.load(3,15,0x70);a.offset(4,4,0x5556);
        a.load<2>(6,5,0);a.load(5,27,0);a.load<1>(0,3,0x2e);a.add(6,6,5);a.load(3,14,0x70);a.multiply_high(5,4,6);
        a.sign8(4,0);a.load<1>(0,3,0x2e);a.sign8(0,0);a.mask(3,5,1,false,1);a.add(3,5,3);a.multiply(3,3,3);
        a.subtract(3,3,6);a.add(3,4,3);a.offset(3,3,3);a.compare_registers(3,0);
        if(!a.at_least()){emit_pair(a,0x10242770,0x2164);return;}
    }
    if(has_both_profiles(a,0x1024279c,0x102427b4)) {
        a.move(3,14);a.call(0x10289560);a.sign16(0,3);a.compare(0,1);
        if(!a.equal()) {
            detail::advance_action_random(a,0x102427c8);a.load(4,30,0);a.value(5,0x55550000);a.load(0,27,0);a.offset(5,5,0x5556);
            a.load<2>(6,4,0);a.load(3,14,0x70);a.add(6,6,0);a.load(4,15,0x70);a.multiply_high(5,5,6);
            a.load<1>(0,3,0x2e);a.load<1>(3,4,0x2e);a.sign8(0,0);a.sign8(4,3);a.mask(3,5,1,false,1);a.add(3,5,3);
            a.multiply(3,3,3);a.subtract(3,3,6);a.add(3,0,3);a.offset(0,3,5);a.compare_registers(4,0);
            if(!a.nonpositive()){emit_pair(a,0x10242844,0x21cb);return;}
        }
    }
    a.at(0x10242870);a.move(3,14);a.call(0x10289560);a.sign16(0,3);a.compare(0,1);
    if(a.equal()) {
        detail::advance_action_random(a,0x10242884);a.load(4,30,0);a.value(3,0x66660000);a.load(0,27,0);a.offset(3,3,0x6667);
        a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);
        a.add(0,0,3);a.multiply(0,0,5);a.subtract(0,0,4,true);
        if(!a.equal()){emit_pair(a,0x102428e0,0x214f);return;}
    }
    emit_pair(a,0x1024290c,0x1f5d,false);
}

bool special_actor_area(Call& a) {
    a.at(0x102429b4);a.load<1>(0,15,0x27);a.sign8(0,0,true);const bool right=a.equal();
    a.at(right?0x102429c0:0x10242a18);a.load<1>(0,15,0x106);
    if(right)a.compare(0,11);else a.sign8(0,0,true);
    bool direct=a.equal();
    if(direct){a.load<1>(0,15,0x105);a.sign8(0,0);a.compare(0,3);direct=!a.less();}
    if(direct){a.load<1>(0,15,0x105);a.sign8(0,0);a.compare(0,5);direct=!a.positive();}
    if(direct){a.value(3,1);a.jump(0x10242a6c);}
    else {
        a.at(right?0x102429f4:0x10242a4c);a.floating_load(1,31,0,true);a.move(3,15);a.floating_load(2,31,0,true);
        a.value(4,4);a.value(5,right?11:0);a.value(6,0);a.value(9,0);a.call(0x10244c60);if(right)a.jump(0x10242a6c);
    }
    a.at(0x10242a6c);a.sign8(0,3,true);return !a.equal();
}

void react_special_receiver(Call& a) {
    a.load<1>(3,15,9);a.offset(0,3,1);a.store<1>(15,9,0);a.load<2>(3,15,0x36,true);a.offset(0,3,300);a.store<2>(15,0x36,0);
    a.load<1>(0,14,0x17);a.sign8(0,0);a.multiply(3,0,30);a.offset_record(16,3,70);
    if(a.equal()){a.value(13,0);a.jump(0x10242c04);}else detail::sample_action_interval(a,0x10242aa8,16,13);
    a.at(0x10242c04);a.compare(13,0);if(!a.equal())return;
    detail::advance_action_random(a,0x10242c0c);a.load(4,30,0);a.move(3,14);a.load(0,27,0);a.load<2>(4,4,0);a.add(0,4,0);
    a.mask(4,0,1,false,1);a.mask(0,0,1);a.combine(0,0,4,false,true);a.subtract(0,4,0);a.sign8(4,0);a.call(0x10288f30);a.sign8(0,3,true);
    if(a.equal())return;
    a.move(3,14);a.value(4,0);a.call(0x1022ba10);
}

void apply_special_receiver(Call& a) {
    describe_special_receiver(a);a.at(0x10242934);a.value(0,123);a.move(3,14);a.store<2>(15,0x19c,0);
    a.load<1>(4,14,7);a.offset(0,4,1);a.store<1>(14,7,0);a.call(0x10289560);a.sign16(0,3);a.value(5,std::uint32_t(-600));a.compare(0,1);
    if(!a.equal())a.value(5,std::uint32_t(-10));
    a.at(0x10242964);a.load<2>(4,14,0x36,true);a.sign16(0,5);a.move(3,14);a.add(0,4,0);a.store<2>(14,0x36,0);
    a.load<1>(4,15,7);a.offset(0,4,1);a.store<1>(15,7,0);a.load<1>(4,15,8);a.offset(0,4,1);a.store<1>(15,8,0);
    a.load<2>(4,15,0x36,true);a.offset(0,4,-10);a.store<2>(15,0x36,0);a.load<2>(4,15,0x36,true);a.offset(0,4,50);a.store<2>(15,0x36,0);
    a.call(0x10270ba0);a.sign8(0,3,true);if(a.equal())return;
    if(special_actor_area(a))react_special_receiver(a);
}

void adjust_actor_balance(Call& a,std::uint32_t start,bool jump_after_lower_clamp=true) {
    // The last original lower clamp falls through; the earlier copies branch.
    a.at(start);a.load<2>(3,1,0x4a,true);a.load<2>(0,15,0x1a0,true);a.subtract(3,3,17);a.offset(3,3,-200);a.sign16(3,3);a.add(0,0,3);
    a.store<2>(15,0x1a0,0);a.load<2>(0,15,0x1a0,true);a.compare(0,200);
    if(!a.nonpositive()){a.value(0,200);a.store<2>(15,0x1a0,0);a.jump(success_reaction_phase);return;}
    a.at(start+0x34);a.load<2>(0,15,0x1a0,true);a.compare(0,-200);if(a.at_least())return;
    a.value(0,std::uint32_t(-200));a.store<2>(15,0x1a0,0);if(jump_after_lower_clamp)a.jump(success_reaction_phase);
}

bool special_regular_message(Call& a) {
    a.at(0x10242d38);a.sign16(0,16);a.compare(0,-500);
    if(!a.at_least()){emit_actor(a,0x10242d44,0x21cc);a.jump(0x10242ea8);return true;}
    a.at(0x10242d70);a.move(3,15);a.call(0x10272050);a.sign8(0,3,true);
    if(!a.equal()) {
        detail::advance_action_random(a,0x10242d80);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(0,3,0);
        a.mask(3,0,1,false,1);a.mask(0,0,1);a.combine(0,0,3,false,true);a.subtract(0,3,0,true);
        if(!a.equal()){emit_actor(a,0x10242dcc,0x21db);a.jump(0x10242ea8);return true;}
    }
    a.at(0x10242df8);a.move(3,15);a.call(0x102721a0);a.sign8(0,3,true);
    if(!a.equal()) {
        detail::advance_action_random(a,0x10242e08);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(0,3,0);
        a.mask(3,0,1,false,1);a.mask(0,0,1);a.combine(0,0,3,false,true);a.subtract(0,3,0,true);
        if(!a.equal()){emit_actor(a,0x10242e54,0x21dc);a.jump(0x10242ea8);return true;}
    }
    emit_actor(a,0x10242e80,0x1f47);return true;
}
}

void prepare_final_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,no_receiver_phase,dependencies,observer);a.compare(14,0,false);
    if(!a.equal()){a.load<2>(0,1,0x4a,true);a.sign16(3,17);a.compare_registers(3,0);if(a.nonpositive()){state.pc=unsuccessful_action_phase;return;}}
    a.at(0x1024260c);a.move(3,15);a.call(0x1022e3a0);a.sign16(0,17);a.value(4,0x43300000);a.exclusive_flags(3,0,0x80000000);
    a.sign16(0,17);a.store(1,0x7c,3);a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.sign8(5,13);a.store(1,0x78,4);
    a.compare(14,0,false);a.floating_load(2,26,0x80);a.floating_load(1,1,0x78);a.floating_load(0,26,0x330);a.floating_subtract(1,1,2);
    a.store<2>(15,0x19c,5);a.floating_load(2,26,0x80);a.store(1,0x8c,3);a.floating_difference(3,1,0,true);a.load<2>(3,15,0x36,true);
    a.store(1,0x88,0);a.offset(0,3,4);a.floating_load(0,26,0x330);a.floating_load(1,1,0x88);a.floating_subtract(1,1,2);
    a.store<2>(15,0x36,0);a.floating_integer(2,3);a.load(4,15,0x1a4);a.floating_difference(0,1,0,true);a.floating_store(2,1,0x80);
    a.load(3,4,0x3108);a.load(0,1,0x84);a.add(0,3,0);a.store(4,0x3108,0);a.floating_integer(0,0);a.load(4,15,0x1a4);
    a.load(3,4,0x310c);a.floating_store(0,1,0x90);a.load(0,1,0x94);a.add(0,3,0);a.store(4,0x310c,0);
    if(!a.equal()){a.load<1>(3,14,0x2c);a.load<1>(0,1,0x46);a.add(0,3,0);a.store<1>(14,0x2c,0);}state.pc=final_effects_phase;
}

void complete_special_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,final_effects_phase,dependencies,observer);a.sign8(0,22);a.compare(0,4);
    if(!a.equal()){state.pc=regular_effects_phase;return;}
    a.compare(14,0,false);if(!a.equal())apply_special_receiver(a);
    adjust_actor_balance(a,0x10242c74);state.pc=success_reaction_phase;
}

void complete_regular_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,regular_effects_phase,dependencies,observer);a.sign8(0,23,true);
    if(!a.equal()){emit_actor(a,0x10242cc8,0x216e);a.jump(success_reaction_phase);state.pc=success_reaction_phase;return;}
    a.at(0x10242cf4);a.compare_registers(24,15,false);bool eligible=a.equal();
    if(eligible){a.sign8(0,22);a.compare(0,2);eligible=!a.at_least();}
    if(eligible) {
        a.load<1>(0,15,0x106);a.sign8(0,0,true);bool edge=a.equal();
        if(!edge){a.load<1>(0,15,0x106);a.compare(0,11);edge=a.equal();}
        if(edge){a.at(0x10242d20);a.load<1>(0,15,0x105);a.sign8(0,0);a.subtract_from(3,0,4);a.call(0x10775540);a.compare(3,1);eligible=!a.positive();}
        if(eligible&&special_regular_message(a)){adjust_actor_balance(a,0x10242ea8);state.pc=success_reaction_phase;return;}
    }
    adjust_actor_balance(a,0x10242ef4,false);state.pc=success_reaction_phase;
}

void react_to_completed_action(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,success_reaction_phase,dependencies,observer);a.sign8(0,22);a.compare(0,4);bool trigger=a.equal();
    if(!trigger) {
        a.load<2>(14,15,0x1a0,true);a.compare(14,0);
        if(a.equal()){a.value(13,0);a.jump(0x102430b8);}else detail::sample_action_interval(a,0x10242f5c,14,13);
        detail::advance_action_random(a,0x102430b8);a.load(4,30,0);a.value(3,0x51ec0000);a.load(0,27,0);a.offset(3,3,-0x7ae1);
        a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,6);a.mask(3,0,1,false,1);
        a.add(0,0,3);a.multiply(0,0,200);a.subtract(0,0,4);a.compare_registers(13,0);trigger=!a.less();
    }
    if(trigger) {
        a.at(0x10243118);a.sign8(0,22);a.compare(0,4);
        if(!a.at_least()){a.load(3,15,0x1a4);a.value(0,1);a.store<1>(3,0x1c0,0);a.load(4,15,0x1a4);a.load(3,4,0x3108);
            a.offset(0,3,100);a.store(4,0x3108,0);a.load(4,15,0x1a4);a.load(3,4,0x310c);a.offset(0,3,100);a.store(4,0x310c,0);}
        a.at(0x10243150);a.move(3,15);a.call(0x102775e0);
    }
    a.at(0x10243158);a.load<1>(0,15,0x27);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.sign8(0,0);a.compare(0,1);
    const bool right=a.equal();detail::area_coordinates(a,right?0x10243174:0x102431c4,0x1024320c,right,0,3,0);
    a.at(0x1024320c);a.sign8(0,0,true);if(!a.equal()){state.pc=return_phase;return;}
    a.load<2>(13,15,0x32,true);a.compare(13,0);
    if(a.equal()){a.value(14,0);a.jump(0x10243384);}else detail::sample_action_interval(a,0x10243228,13,14);
    a.at(0x10243384);a.compare(14,-10);if(a.at_least()){state.pc=return_phase;return;}
    a.floating_load(1,31,0x18,true);a.move(3,15);a.call(0x102865a0);
    a.load<1>(0,15,0x27);a.value(4,0x211d);a.load(3,15,0x1a4);a.value(6,0xffffffff);a.sign8(0,0);a.load(5,15,0x1a4);
    a.leading_zeroes(0,0);a.load(3,3,0x79f4);a.mask(0,0,0x07ffffff,false,27);a.value(8,0xffffffff);a.sign8(7,0);
    a.value(9,0xffffffff);a.value(10,0);a.call(0x10201180);a.jump(return_phase);state.pc=return_phase;
}
}
