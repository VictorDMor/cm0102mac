#include "actor_order.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_order {
namespace {
using Call = SemanticGuestCall;
constexpr std::uint32_t sample_side = 0x101d3c90, current_position = 0x101d3b50, sort = 0x10768300;

[[gnu::always_inline, gnu::flatten]] inline void append_active_actor(Call& a) {
    a.move(3,30); a.call(current_position); a.sign8(0,3); a.compare(0,0);
    if (a.less()) return;
    a.load<1>(3,31,0x792e); a.offset(0,3,1); a.store<1>(31,0x792e,0);
    a.sign8(0,3); a.mask(3,0,0xfffffffc,false,2); a.offset(0,3,0x79f8); a.indexed_store(30,31,0);
}

[[gnu::always_inline, gnu::flatten]] inline bool restart_priority(Call& a, unsigned actor) {
    a.load(3,2,-0x3910); a.load<1>(0,3,0x7928); a.sign8(0,0); a.compare(0,0);
    if (a.equal()) return false;
    a.load(3,2,-0x3910); a.offset(3,3,0x10000); a.load(0,3,-0x2000);
    a.compare_registers(actor,0,false); return a.equal();
}

[[gnu::always_inline, gnu::flatten]] inline void weighted_rating(Call& a, unsigned actor, unsigned result, std::int32_t scratch) {
    a.move(3,actor); a.call(entries[1]); a.sign8(26,3);
    a.move(3,actor); a.call(entries[0]); a.sign8(0,3); a.add(0,0,26);
    a.floating_load(1,29,0x60); a.exclusive_flags(0,0,0x80000000); a.store(1,scratch+4,0);
    a.value(0,0x43300000); a.store(1,scratch,0); a.floating_load(0,1,scratch); a.floating_subtract(31,0,1,true);
    a.move(3,actor); a.call(entries[4]); a.floating_product_single(3,1,31);
    a.floating_load(2,29,0x90); a.load<2>(0,actor,0x3c,true); a.floating_load(1,29,0x60);
    a.exclusive_flags(0,0,0x80000000); a.store(1,scratch-4,0); a.value(0,0x43300000);
    a.store(1,scratch-8,0); a.floating_load(0,1,scratch-8); a.floating_subtract(1,0,1);
    a.floating_load(0,29,0xf0); a.floating_difference(0,1,0,true); a.floating_sum(0,2,0);
    a.floating_round_single(0,0); a.floating_product_single(0,0,3); a.floating_integer(0,0);
    a.floating_store(0,1,scratch-16); a.load(result,1,scratch-12);
}

[[gnu::always_inline, gnu::flatten]] inline void finish_comparison(Call& a) {
    a.at(0x101d9384); a.load(0,1,0x98); a.offset(1,1,0x90); a.floating_load(31,1,-8);
    a.restore_link(); a.multiple(26,true,-16); a.finish();
}
}

[[gnu::flatten]] void secondary_rating(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[0],dependencies,observer); a.load<1>(3,3,0x108); a.finish();
}
[[gnu::flatten]] void primary_rating(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[1],dependencies,observer); a.load<1>(3,3,0x107); a.finish();
}
[[gnu::flatten]] void current_fitness(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[4],dependencies,observer); a.floating_load(1,3,0x50,true); a.finish();
}

[[gnu::flatten]] void build_queue(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[2],dependencies,observer);
    a.link(); a.save(31); a.save(30); a.save(29); a.save(28); a.save_link(); a.frame(0x50);
    a.move(31,3); a.value(0,0); a.store<1>(31,0x792e,0); a.value(29,0); a.jump(0x101d915c);
    for (;;) {
        a.at(0x101d915c); a.sign8(0,29); a.compare(0,20); if (!a.less()) break;
        a.at(0x101d90b0); a.value(3,2); a.call(sample_side); a.sign8(28,3);
        a.multiply(3,28,0x2350); a.sign8(0,29); a.multiply(0,0,0x1c4); a.add(3,3,0);
        a.offset(30,3,0x311c); a.add(30,31,30); append_active_actor(a);
        a.at(0x101d9104); a.sign8(0,28); a.leading_zeroes(0,0); a.mask(0,0,0x07ffffff,false,27);
        a.multiply(3,0,0x2350); a.sign8(0,29); a.multiply(0,0,0x1c4); a.add(3,3,0);
        a.offset(30,3,0x311c); a.add(30,31,30); append_active_actor(a);
        a.at(0x101d9158); a.offset(29,29,1);
    }
    a.at(0x101d9168); a.store(2,-0x3910,31); a.offset(3,31,0x79f8); a.load<1>(4,31,0x792e);
    a.sign8(4,4); a.value(5,4); a.load(6,2,-0x6420); a.call(sort);
    a.pop(0x50); a.restore(31); a.restore(30); a.restore(29); a.restore(28); a.finish();
}

// Preserve the original ordered priority checks, including equal pointers.
// Its callback result is the wrapping difference of two truncated scores;
// replacing it with a host strict ordering would change comparison behavior.
[[gnu::flatten]] void compare_actors(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[3],dependencies,observer);
    a.link(); a.floating_store(31,1,-8); a.multiple(26,false,-16); a.save_link(); a.frame(0x90);
    a.store(1,0xa8,3); a.store(1,0xac,4); a.load(29,2,-0x52c4);
    a.load(3,1,0xa8); a.load(31,3,0); a.load(3,1,0xac); a.load(30,3,0);
    if (restart_priority(a,31)) { a.value(3,0xffffffff); a.jump(0x101d9384); finish_comparison(a); return; }
    a.at(0x101d9210);
    if (restart_priority(a,30)) { a.value(3,1); a.jump(0x101d9384); finish_comparison(a); return; }
    a.at(0x101d9240); a.load(3,2,-0x3910); a.load(0,3,0x7c68); a.compare_registers(0,31,false);
    if (a.equal()) { a.value(3,0xffffffff); a.jump(0x101d9384); finish_comparison(a); return; }
    a.at(0x101d9258); a.load(3,2,-0x3910); a.load(0,3,0x7c68); a.compare_registers(0,30,false);
    if (a.equal()) { a.value(3,1); a.jump(0x101d9384); finish_comparison(a); return; }
    a.at(0x101d9270); weighted_rating(a,31,28,0x60); weighted_rating(a,30,27,0x48);
    a.subtract(3,28,27); finish_comparison(a);
}
}
