#include "actor_attributes.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_attributes {
namespace {
using Call=SemanticGuestCall;
constexpr auto finish_body=0x10239cd0u;

void select_restoration_scale(Call& a) {
    a.load(4,28,0x6c); a.load(3,29,0); a.load(4,4,0); a.call(record_scale_entry);
    a.sign16(0,3); a.compare(0,1000);
    if (a.nonpositive()) return;
    a.load(4,28,0x6c); a.load(3,29,0); a.load(4,4,0); a.call(record_scale_entry);
    a.value(0,0x43300000); a.sign16(3,3); a.store(1,0x40,0); a.exclusive_flags(0,3,0x80000000);
    a.floating_load(3,30,0x80); a.store(1,0x44,0); a.floating_load(1,30,0x308);
    a.floating_load(2,1,0x40); a.floating_load(0,30,0x300); a.floating_subtract(2,2,3);
    a.floating_negative_product_difference(31,1,2,0);
}

void restore_primary_role(Call& a) {
    a.load(3,28,0x70); a.value(0,0x43300000); a.store(1,0x40,0); a.load<1>(0,3,0x34);
    a.floating_load(3,30,0x80); a.sign8(0,0); a.floating_load(1,28,0x50,true);
    a.multiply(3,0,10); a.floating_load(0,30,0x2e0); a.offset(0,3,150); a.exclusive_flags(0,0,0x80000000);
    a.store(1,0x44,0); a.floating_load(2,1,0x40); a.floating_subtract(2,2,3);
    a.floating_product_sum(1,31,2,1); a.floating_round_single(1,1); a.floating_store_single(1,28,0x50);
    a.floating_load(1,28,0x50,true); a.floating_compare(1,0);
    if (!a.nonpositive()) { a.floating_load(0,31,0xd0,true); a.floating_store_single(0,28,0x50); a.jump(finish_body); }
}

void restore_available_actor(Call& a) {
    a.at(0x10239898); a.load<1>(0,28,0x19); a.sign8(0,0,true); if (a.less()) return;
    a.load<1>(0,28,0x1f); a.sign8(0,0); a.compare(0,-1); const bool returning=a.equal();
    a.at(returning?0x102398b4:0x102398f8);
    a.load(3,28,0x70); a.value(0,0x43300000); a.store(1,0x40,0); a.load<1>(0,3,0x34);
    a.floating_load(2,30,0x80); a.sign8(0,0); a.floating_load(0,28,0x50,true);
    a.multiply(3,0,returning?12:10); a.offset(0,3,returning?325:150); a.exclusive_flags(0,0,0x80000000);
    a.store(1,0x44,0); a.floating_load(1,1,0x40); a.floating_subtract(1,1,2);
    a.floating_product_sum(0,31,1,0); a.floating_round_single(0,0); a.floating_store_single(0,28,0x50);
    if (returning) a.jump(0x10239938);
    a.at(0x10239938); a.floating_load(1,28,0x50,true); a.floating_load(0,30,0x2e0); a.floating_compare(1,0);
    if (!a.nonpositive()) { a.floating_load(0,31,0xd0,true); a.floating_store_single(0,28,0x50); a.jump(finish_body); }
}

void project_condition(Call& a,std::uint32_t start) {
    a.at(start); a.load(3,28,0x70); a.value(0,0x43300000); a.store(1,0x38,0); a.load<1>(0,3,0x34);
    a.floating_load(2,30,0x80); a.sign8(0,0); a.floating_load(0,28,0x50,true);
    a.multiply(3,0,12); a.offset(0,3,85); a.exclusive_flags(0,0,0x80000000); a.store(1,0x3c,0);
    a.floating_load(1,1,0x38); a.floating_subtract(1,1,2); a.floating_product_sum(0,31,1,0);
    a.floating_round_single(0,0); a.floating_store_single(0,28,0x54);
}

void interpolate_projected_condition(Call& a,std::uint32_t start,unsigned field,unsigned first,unsigned second) {
    a.at(start); a.load<1>(5,28,field); a.value(4,0x43300000); a.load(3,28,0x70); a.value(0,0x43300000);
    a.sign8(5,5); a.store(1,first,4); a.exclusive_flags(4,5,0x80000000); a.load<1>(3,3,0x34);
    a.store(1,first+4,4); a.sign8(3,3); a.floating_load(1,30,0x80); a.multiply(3,3,12);
    a.floating_load(0,1,first); a.floating_load(2,30,0x310); a.floating_subtract(1,0,1);
    a.floating_load(0,30,0x318); a.offset(3,3,85); a.exclusive_flags(3,3,0x80000000); a.store(1,second,0);
    a.floating_product_difference(0,2,1,0); a.store(1,second+4,3); a.floating_load(2,30,0x80);
    a.floating_load(1,1,second); a.floating_round_single(3,0); a.floating_load(0,28,0x50,true);
    a.floating_subtract(1,1,2,true); a.floating_product_single(1,3,1); a.floating_product_sum(0,31,1,0);
    a.floating_round_single(0,0); a.floating_store_single(0,28,0x54); a.jump(start+0xbc);
}

void enforce_available_projection(Call& a,std::uint32_t start) {
    a.at(start); a.load<1>(0,28,0x1c); a.sign8(0,0,true); if (!a.equal()) return;
    a.load(0,28,0x6c); a.compare(0,0,false); if (a.equal()) return;
    a.load(3,29,0); a.value(5,0); a.load(4,28,0x6c); a.call(effective_condition_entry);
    a.sign16(0,3); a.compare(0,6500); if (!a.nonpositive()) a.value(3,6500);
    a.at(start+0x38); a.sign16(3,3); a.value(0,0x43300000); a.exclusive_flags(3,3,0x80000000);
    a.store(1,0x38,0); a.floating_load(1,30,0x80); a.store(1,0x3c,3); a.floating_load(2,28,0x54,true);
    a.floating_load(0,1,0x38); a.floating_subtract(0,0,1,true); a.floating_compare(2,0);
    if (!a.at_least()) { a.floating_load(0,31,0xd4,true); a.floating_store_single(0,28,0x54); a.jump(finish_body); }
}

void restore_projection(Call& a) {
    a.at(0x10239954); a.load<1>(0,28,0x20); a.sign8(0,0); a.compare(0,-1);
    if (!a.nonpositive()) {
        a.load<1>(0,28,0x20); a.sign8(0,0); a.compare(0,45); bool interpolate=!a.nonpositive();
        if (interpolate) { a.load<1>(0,28,0x20); a.sign8(0,0); a.compare(0,65); interpolate=!a.at_least(); }
        if (interpolate) interpolate_projected_condition(a,0x10239984,0x20,0x40,0x38);
        else project_condition(a,0x10239a00);
        enforce_available_projection(a,0x10239a40); return;
    }
    a.at(0x10239ab0); a.load<1>(0,28,0x21); a.sign8(0,0); a.compare(0,-1);
    if (!a.nonpositive()) {
        a.load<1>(0,28,0x21); a.sign8(0,0); a.compare(0,45); bool interpolate=!a.nonpositive();
        if (interpolate) { a.load<1>(0,28,0x21); a.sign8(0,0); a.compare(0,65); interpolate=!a.at_least(); }
        if (interpolate) interpolate_projected_condition(a,0x10239ae0,0x21,0x38,0x40);
        else project_condition(a,0x10239b5c);
        enforce_available_projection(a,0x10239b9c); return;
    }
    a.at(0x10239c0c); a.load<1>(0,28,0x19); a.sign8(0,0,true);
    if (a.less()) { a.at(0x10239cc8); a.floating_load(0,28,0x50,true); a.floating_store_single(0,28,0x54); }
    else { project_condition(a,0x10239c18); enforce_available_projection(a,0x10239c58); }
}
}

void restore_condition(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,restoration_entry,dependencies,observer);
    a.link(); a.floating_store(31,1,-8); a.multiple(27,false,-16); a.move(28,3); a.load(30,2,-0x51e8);
    a.move(27,4); a.load(29,2,-0x7ba0); a.load(31,2,-0x51e0); a.floating_load(31,30,0x10); a.save_link(); a.frame(0x70);
    a.load(0,3,0x70); a.compare(0,0,false);
    if (!a.equal()) {
        select_restoration_scale(a); a.at(0x10239820); a.sign8(0,27,true);
        if (a.equal()) {
            a.move(3,28); a.call(0x10289560); a.sign16(0,3); a.compare(0,1);
            if (a.equal()) restore_primary_role(a); else restore_available_actor(a);
        } else restore_projection(a);
        a.at(finish_body); a.floating_load(2,28,0x54,true); a.floating_load(1,28,0x5c,true); a.floating_load(0,30,0x2e0);
        a.floating_sum_single(1,2,1); a.floating_store_single(1,28,0x54); a.floating_load(1,28,0x54,true); a.floating_compare(1,0);
        if (!a.nonpositive()) { a.floating_load(0,31,0xd0,true); a.floating_store_single(0,28,0x54); }
    }
    a.at(0x10239cf8); a.load(0,1,0x78); a.offset(1,1,0x70); a.floating_load(31,1,-8);
    a.restore_link(); a.multiple(27,true,-16); a.finish();
}
}
