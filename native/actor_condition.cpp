#include "actor_attributes.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_attributes {
namespace {
using Call = SemanticGuestCall;

void periodic_pool_draw(Call& a) {
    a.load(3,28,0); a.load(0,27,0); a.offset(3,3,2);
    a.compare_registers(3,0,false); a.store(28,0,3);
    if (!a.nonpositive()) {
        a.store(28,0,26); a.call(0x1077b770); a.mask(0,3,0xffff); a.store(25,0,0);
    }
    a.at(0x10239418); a.load(4,28,0); a.value(3,0x2e8c0000); a.load(0,25,0);
    a.offset(3,3,-0x5d17); a.load<2>(4,4,0); a.add(4,4,0); a.multiply_high(0,3,4);
    a.shift_right(0,0,1); a.mask(3,0,1,false,1); a.add(0,0,3); a.multiply(0,0,11);
    a.subtract(0,0,4,true);
}

void adjust_exertion_byte(Call& a) {
    a.load<1>(0,29,0x103); a.load(4,29,0x70); a.sign8(3,0); a.load<1>(5,29,0x102);
    a.mask(0,3,1,false,1); a.load<1>(4,4,0x25); a.add(0,0,3); a.sign8(5,5);
    a.sign8(3,4); a.shift_right(0,0,1); a.add(3,3,0); a.offset(0,3,10);
    a.shift_right(0,0,2); a.add_carry(0,0); a.compare_registers(5,0);
    if (!a.nonpositive()) { a.load<1>(3,29,0x102); a.offset(0,3,-1); a.store<1>(29,0x102,0); }
}

void apply_condition_drift(Call& a) {
    a.at(0x10239498); a.floating_load(2,29,0x50,true); a.floating_load(0,29,0x5c,true);
    a.floating_load(1,31,0x34,true); a.floating_sum_single(0,2,0); a.floating_store_single(0,29,0x50);
    a.floating_load(0,29,0x5c,true); a.floating_compare(1,0);
    if (a.equal()) {
        a.floating_load(0,31,0,true); a.floating_store_single(0,29,0x5c); a.jump(0x102394d8);
    } else {
        a.at(0x102394c4); a.floating_load(1,29,0x5c,true); a.floating_load(0,30,0x28);
        a.floating_product(0,1,0); a.floating_round_single(0,0); a.floating_store_single(0,29,0x5c);
    }
}

void apply_high_load_cost(Call& a) {
    a.at(0x102394d8); a.load<1>(0,29,1); a.compare(0,50,false);
    if (a.nonpositive()) return;
    a.load<1>(3,29,1); a.value(0,0x43300000); a.store(1,0x38,0); a.offset(0,3,-50);
    a.floating_load(1,30,0x80); a.exclusive_flags(0,0,0x80000000); a.floating_load(2,30,0x168);
    a.store(1,0x3c,0); a.floating_load(0,1,0x38); a.floating_subtract(1,0,1); a.call(0x10780fc0);
    a.load(2,1,0x14); a.floating_round_single(1,1); a.floating_load(2,31,0x70,true);
    a.floating_load(0,30,0x2d8); a.floating_product_single(1,2,1); a.floating_compare(1,0);
    if (!a.nonpositive()) a.floating_load(1,31,0xb4,true);
    a.at(0x10239530); a.floating_load(0,29,0x50,true); a.floating_subtract(0,0,1,true);
    a.floating_store_single(0,29,0x50);
}

void clamp_periodic_condition(Call& a) {
    a.at(0x1023953c); a.floating_load(1,30,0x10); a.floating_load(0,29,0x50,true); a.floating_compare(1,0);
    if (!a.nonpositive()) { a.floating_load(0,30,0x10); a.jump(0x10239570); }
    else {
        a.at(0x10239554); a.floating_load(1,30,0x2e0); a.floating_load(0,29,0x50,true); a.floating_compare(1,0);
        if (!a.at_least()) { a.floating_load(0,30,0x2e0); a.jump(0x10239570); }
        else { a.at(0x1023956c); a.floating_load(0,29,0x50,true); }
    }
    a.at(0x10239570); a.floating_round_single(0,0); a.floating_store_single(0,29,0x50);
}

void movement_condition_cost(Call& a) {
    a.floating_load(2,3,0x50,true); a.floating_load(0,6,0xb8,true); a.floating_compare(2,0);
    std::uint32_t start=0x102395b4; unsigned factor=0x64;
    if (a.nonpositive()) {
        a.at(0x102395d4); a.floating_load(2,3,0x50,true); a.floating_load(0,6,0xc0,true); a.floating_compare(2,0);
        start=0x102395e4; factor=0x5c;
        if (a.nonpositive()) {
            a.at(0x10239604); a.floating_load(2,3,0x50,true); a.floating_load(0,6,0xc4,true); a.floating_compare(2,0);
            start=0x10239614; factor=0xc8;
            if (a.nonpositive()) { start=0x10239634; factor=0xcc; }
        }
    }
    a.at(start); a.floating_load(2,6,factor,true); a.floating_load(0,6,0xbc,true);
    a.floating_product_single(1,2,1); a.floating_load(2,3,0x50,true); a.floating_difference(0,1,0,true,true);
    a.floating_subtract(0,2,0,true); a.floating_store_single(0,3,0x50);
    if (start!=0x10239634) a.jump(0x10239650);
    a.at(0x10239650); a.floating_load(1,3,0x50,true); a.floating_load(0,6,0,true); a.floating_compare(1,0);
    if (a.finish_if_at_least()) return;
    a.floating_load(0,6,0,true); a.floating_store_single(0,3,0x50); a.finish();
}

void action_condition_cost(Call& a,std::uint32_t start,unsigned factor) {
    a.at(start); a.floating_load(2,7,0x2e8); a.floating_load(1,3,0xd8,true); a.floating_load(0,7,0x38);
    a.floating_load(3,7,factor); a.floating_negative_product_difference(2,2,1,0);
    a.floating_load(4,7,0x1f0); a.floating_load(1,7,0x2f0); a.floating_load(5,3,0x50,true); a.floating_load(0,6,0,true);
    a.floating_product(2,3,2); a.floating_product(2,4,2); a.floating_difference(1,2,1,true); a.floating_round_single(1,1);
    a.floating_subtract(1,5,1,true); a.floating_store_single(1,3,0x50); a.floating_load(1,3,0x50,true); a.floating_compare(1,0);
    if (a.finish_if_at_least()) return;
    a.floating_load(0,6,0,true); a.floating_store_single(0,3,0x50); a.finish();
}
}

void refresh_condition(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,periodic_entry,dependencies,observer);
    a.link(); a.multiple(25); a.load(30,2,-0x51e8); a.move(29,3); a.load(25,2,-0x7b90);
    a.load(26,2,-0x7b8c); a.load(27,2,-0x7b88); a.load(28,2,-0x7b84); a.load(31,2,-0x51e0);
    a.save_link(); a.frame(0x60); a.call(refresh_entry); periodic_pool_draw(a);
    if (a.equal()) { adjust_exertion_byte(a); apply_condition_drift(a); apply_high_load_cost(a); clamp_periodic_condition(a); }
    a.at(0x10239578); a.pop(0x60); a.multiple(25,true); a.finish();
}

void consume_condition(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,consumption_entry,dependencies,observer);
    a.sign16(0,5); a.load(7,2,-0x51e8); a.compare(0,-1); a.load(6,2,-0x51e0);
    if (a.equal()) { movement_condition_cost(a); return; }
    a.at(0x1023966c); a.sign16(4,5); a.offset(0,4,-19); a.compare(0,27,false);
    auto target=0x10239738u;
    if (!a.positive()) { a.load(4,2,-0x5200); a.mask(0,0,0xfffffffc,false,2); a.indexed_load(4,4,0); a.count_destination(4); target=a.count_branch(); }
    switch (target) {
    case 0x10239690: action_condition_cost(a,target,0x28); break;
    case 0x102396e4: action_condition_cost(a,target,0x170); break;
    case 0x10239738: action_condition_cost(a,target,0x2f8); break;
    default:
        // A changed guest table retains its destination and LR. This is a
        // dependency transfer, not a remapping to a preferred action formula.
        a.transfer(target);
        break;
    }
}
}
