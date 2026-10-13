#include "pitch_distance.hpp"
#include "semantic_guest_call.hpp"
#include "stdlib_core_adapter.hpp"
#include "match_turbo.hpp"

namespace cm::aot::pitch_distance {
namespace {
using Call = SemanticGuestCall;
constexpr std::uint32_t absolute = 0x10775540;

struct AbsoluteDispatch final : SemanticDependencyDispatch {
    void call(State& s, std::uint32_t target, std::uint32_t continuation) override {
        // This four-instruction leaf touches registers only. With enough budget
        // and no observer/original/profile selection it cannot call out, capture
        // a context or transfer control. Its existing adapter owns accounting
        // and diagnostics. Every observable/interruptible call keeps the shared
        // dispatcher and its nonlocal continuation scope.
        if (target == absolute && !s.stdlib_core_observer && !s.stdlib_core_original &&
            !s.stdlib_core_entry_original[0] && !s.stdlib_core_original_body &&
            !match_pc_profile_enabled() && s.executed <= s.budget && s.budget-s.executed >= 4) {
            if (try_modern_stdlib_core(s, target)) return;
        }
        run_until(s, continuation);
    }
};

[[gnu::always_inline, gnu::flatten]] inline void restore_query(Call& a) {
    a.pop(0x40); a.restore(31); a.restore(30); a.finish();
}

bool bounded_delta(Call& a, unsigned source, std::int32_t maximum) {
    a.sign8(0,source); a.compare(0,0);
    if (a.less()) return false;
    a.sign8(0,source); a.compare(0,maximum);
    return !a.positive();
}
}

// Keep each guest checkpoint visible while inlining these short, hot queries.
// The original table is live data, not a host Euclidean-distance calculation.
[[gnu::flatten]] void point_to_point(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    AbsoluteDispatch absolute_dispatch;
    Call a(state,entries[0],dependencies ? dependencies : &absolute_dispatch,observer);
    a.link(); a.save(31); a.save(30); a.save_link(); a.frame(0x40);
    a.store<1>(1,0x5f,4); a.store<1>(1,0x63,5); a.store<1>(1,0x67,6); a.store<1>(1,0x6b,7);
    a.load<1>(0,1,0x67); a.sign8(3,0); a.load<1>(0,1,0x5f); a.sign8(0,0); a.subtract(3,3,0); a.call(absolute);
    a.sign8(31,3);
    a.load<1>(0,1,0x6b); a.sign8(3,0); a.load<1>(0,1,0x63); a.sign8(0,0); a.subtract(3,3,0); a.call(absolute);
    a.sign8(30,3);
    bool in_range = bounded_delta(a,31,8);
    if (in_range) {
        a.sign8(0,30); a.compare(0,0); in_range = !a.less();
        if (in_range) { a.sign8(0,30); a.compare(0,11); in_range = a.nonpositive(); }
    }
    if (!in_range) {
        a.at(0x101ef95c); a.load(3,2,-0x52c8); a.floating_load(1,3,0xdc,true); a.jump(0x101ef984);
    } else {
        a.at(0x101ef968); a.load(4,2,-0x6424); a.sign8(0,30); a.multiply(3,0,0x24);
        a.sign8(0,31); a.mask(0,0,0xfffffffc,false,2); a.add(0,3,0); a.floating_indexed_single(1,4,0);
    }
    a.at(0x101ef984); restore_query(a);
}

[[gnu::flatten]] void actor_to_cell(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    AbsoluteDispatch absolute_dispatch;
    Call a(state,entries[1],dependencies ? dependencies : &absolute_dispatch,observer);
    a.link(); a.save(31); a.move(31,5); a.save(30); a.move(30,3); a.save_link(); a.frame(0x40);
    a.load<1>(0,3,0x105); a.sign8(3,4); a.sign8(0,0); a.subtract(3,3,0); a.call(absolute);
    a.load<1>(0,30,0x106); a.sign8(4,31); a.sign8(31,3); a.sign8(0,0); a.subtract(3,4,0); a.call(absolute);
    a.sign8(3,3); a.sign8(0,31); a.multiply(3,3,0x24); a.load(4,2,-0x6424);
    a.mask(0,0,0xfffffffc,false,2); a.add(0,3,0); a.floating_indexed_single(1,4,0);
    restore_query(a);
}

[[gnu::flatten]] void actor_to_attacking_goal(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    AbsoluteDispatch absolute_dispatch;
    Call a(state,entries[2],dependencies ? dependencies : &absolute_dispatch,observer);
    a.link(); a.save(31); a.load(31,2,-0x6424); a.save(30); a.move(30,3); a.save_link(); a.frame(0x40);
    a.load<1>(0,3,0x27); a.sign8(0,0,true);
    if (a.equal()) {
        a.load<1>(0,30,0x105); a.sign8(3,0); a.offset(3,3,-4); a.call(absolute);
        a.load<1>(0,30,0x106); a.sign8(30,3); a.sign8(3,0); a.offset(3,3,-11); a.call(absolute);
        a.sign8(3,3); a.sign8(0,30); a.multiply(3,3,0x24);
        a.mask(0,0,0xfffffffc,false,2); a.add(0,3,0); a.floating_indexed_single(1,31,0); a.jump(0x10245204);
    } else {
        a.at(0x102451c8); a.load<1>(0,30,0x105); a.sign8(3,0); a.offset(3,3,-4); a.call(absolute);
        a.move(0,3); a.load<1>(3,30,0x106); a.sign8(30,0); a.sign8(3,3); a.call(absolute);
        a.sign8(3,3); a.sign8(0,30); a.multiply(3,3,0x24);
        a.mask(0,0,0xfffffffc,false,2); a.add(0,3,0); a.floating_indexed_single(1,31,0);
    }
    a.at(0x10245204); restore_query(a);
}
}
