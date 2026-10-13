#include "team_spatial.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::team_spatial {
namespace {
using Call = SemanticGuestCall;
constexpr std::uint32_t current_position = 0x101d8110, current_role = 0x10289560;
constexpr std::uint32_t distance_to_cell = 0x1022e760, current_column = 0x101d5680;

void team_actor_base(Call& a, unsigned actor) {
    a.load(4,1,0x78); a.load<1>(0,1,0x7f); a.sign8(0,0);
    a.multiply(3,0,0x2350); a.offset(actor,3,0x311c); a.add(actor,4,actor);
}

bool eligible_distance_actor(Call& a, unsigned actor, bool role_mask) {
    a.move(3,actor); a.call(current_position); a.sign8(0,3); a.compare(0,0);
    if (a.less()) return false;
    a.move(3,actor); a.call(current_role); a.sign16(0,3);
    if (role_mask) a.mask(0,0,1);
    a.compare(0,role_mask ? 0 : 1);
    const bool skip = role_mask ? !a.equal() : a.equal();
    if (skip) return false;
    a.load<1>(0,1,0x8b); a.sign8(0,0); a.compare(0,0);
    if (!a.equal()) return true;
    a.load<1>(0,actor,0x2c); a.sign8(0,0); a.compare(0,0);
    return a.equal();
}

// The nearest-actor query permits state 1; the distance/weight queries require
// state 0. All three use the caller's signed low-byte override.
[[gnu::always_inline, gnu::flatten]] inline bool eligible_nearest_actor(Call& a) {
    a.move(3,31); a.call(current_position); a.sign8(0,3); a.compare(0,0);
    if (a.less()) return false;
    a.move(3,31); a.call(current_role); a.sign16(0,3); a.compare(0,1);
    if (a.equal()) return false;
    a.sign8(0,28); a.compare(0,0);
    if (!a.equal()) return true;
    a.load<1>(0,31,0x2c); a.sign8(0,0); a.compare(0,1);
    return !a.positive();
}

bool consider_nearest_actor(Call& a, std::uint32_t start) {
    if (!eligible_nearest_actor(a)) return false;
    a.at(start+0x44); a.move(3,31); a.move(4,26); a.move(5,27); a.call(distance_to_cell);
    a.floating_move(31,1); a.floating_load(0,29,0x50); a.floating_compare(0,31);
    if (a.equal()) { a.move(3,31); a.jump(0x101e3d34); return true; }
    a.at(start+0x6c); a.floating_compare(31,30);
    if (!a.at_least()) { a.move(30,31); a.floating_move(30,31); }
    return false;
}

void restore_distance_frame(Call& a, unsigned first, bool two_floats) {
    a.load(0,1,0x68); a.offset(1,1,0x60); a.floating_load(31,1,-8);
    if (two_floats) a.floating_load(30,1,-16);
    a.restore_link(); a.multiple(first,true,-16); a.finish();
}

[[gnu::always_inline, gnu::flatten]] inline void consider_column_limit(Call& a, std::uint32_t start, bool away, unsigned slot) {
    // Unrolled original slots use two different scratch-register allocations.
    // Preserve them even when a candidate is excluded before its column read.
    const unsigned scratch = (away ? slot < 12 : slot >= 8) ? 3 : 0;
    a.move(3,31); a.call(current_position); a.sign8(scratch,3); a.compare(scratch,0);
    if (a.less()) return;
    a.move(3,31); a.call(current_role); a.sign16(scratch,3); a.compare(scratch,1);
    if (a.equal()) return;
    a.move(3,31); a.call(current_column);
    const unsigned candidate = !away && slot >= 8 ? 0 : 3, limit = candidate == 0 ? 3 : 0;
    a.sign8(candidate,3); a.sign8(limit,30); a.compare_registers(candidate,limit);
    const bool skip = away ? a.nonpositive() : a.at_least();
    if (!skip) {
        // A winning column is read again; a callback may have changed it.
        a.at(start+0x40); a.move(3,31); a.call(current_column); a.move(30,3);
    }
}

void scan_column_limits(Call& a, std::uint32_t first, bool away) {
    for (unsigned slot=0; slot<20; ++slot) {
        const auto start = first + slot*0x50;
        a.at(start); consider_column_limit(a,start,away,slot);
        if (slot != 19) { a.at(start+0x4c); a.offset(31,31,0x1c4); }
    }
}
}

// Keep guest checkpoints and fixed register operands visible to the optimizer.
// Outlined per-instruction helper calls otherwise dominate these short scans.
[[gnu::flatten]] void nearest_actor(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[0],dependencies,observer);
    a.link(); a.floating_store(31,1,-8); a.floating_store(30,1,-16); a.multiple(26,false,-16); a.save_link(); a.frame(0x60);
    a.store(1,0x78,3); a.store<1>(1,0x7f,4); a.move(26,5); a.move(27,6); a.move(28,7);
    a.load(29,2,-0x52c4); a.value(30,0); a.load(3,2,-0x52c8); a.floating_load(30,3,0xc8,true);
    team_actor_base(a,31);
    bool exact = false;
    for (unsigned slot=0; slot<20; ++slot) {
        const auto start = 0x101e3334 + slot*0x80;
        a.at(start);
        if (consider_nearest_actor(a,start)) { exact=true; break; }
        if (slot != 19) { a.at(start+0x7c); a.offset(31,31,0x1c4); }
    }
    if (!exact) { a.at(0x101e3d30); a.move(3,30); }
    a.at(0x101e3d34); restore_distance_frame(a,26,true);
}

[[gnu::flatten]] void nearest_distance(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[1],dependencies,observer);
    a.link(); a.floating_store(31,1,-8); a.floating_store(30,1,-16); a.multiple(27,false,-16); a.save_link(); a.frame(0x60);
    a.store(1,0x78,3); a.store<1>(1,0x7f,4); a.store<1>(1,0x83,5); a.store<1>(1,0x87,6); a.store<1>(1,0x8b,7);
    a.load(31,2,-0x52c8); a.load(30,2,-0x52c4); a.floating_load(31,31,0xc8,true);
    team_actor_base(a,29); a.value(28,0); a.jump(0x101e3ff4);
    bool exact = false;
    for (;;) {
        a.at(0x101e3ff4); a.sign8(0,28); a.compare(0,20); if (!a.less()) break;
        a.at(0x101e3f68);
        if (eligible_distance_actor(a,29,true)) {
            a.at(0x101e3fb4); a.move(3,29); a.load<1>(4,1,0x83); a.load<1>(5,1,0x87); a.call(distance_to_cell);
            a.floating_move(30,1); a.floating_load(0,30,0x50); a.floating_compare(0,30);
            if (a.equal()) { a.floating_load(1,31,0xc,true); a.jump(0x101e4004); exact=true; break; }
            a.at(0x101e3fdc); a.floating_compare(30,31);
            if (!a.at_least()) a.floating_move(31,30);
        }
        a.at(0x101e3fe8); a.offset(28,28,1); a.move(27,29); a.offset(29,29,0x1c4);
    }
    if (!exact) { a.at(0x101e4000); a.floating_move(1,31); }
    a.at(0x101e4004); restore_distance_frame(a,27,true);
}

[[gnu::flatten]] void nearby_weight(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[2],dependencies,observer);
    a.link(); a.floating_store(31,1,-8); a.multiple(27,false,-16); a.save_link(); a.frame(0x60);
    a.store(1,0x78,3); a.store<1>(1,0x7f,4); a.store<1>(1,0x83,5); a.store<1>(1,0x87,6); a.store<1>(1,0x8b,7);
    a.load(31,2,-0x52c4); a.value(29,0); team_actor_base(a,30); a.value(28,0); a.jump(0x101e4100);
    for (;;) {
        a.at(0x101e4100); a.sign8(0,28); a.compare(0,20); if (!a.less()) break;
        a.at(0x101e4070);
        if (eligible_distance_actor(a,30,false)) {
            a.at(0x101e40b8); a.move(3,30); a.load<1>(4,1,0x83); a.load<1>(5,1,0x87); a.call(distance_to_cell);
            a.floating_move(31,1); a.floating_load(0,31,0x50); a.floating_compare(0,31);
            if (a.equal()) { a.offset(29,29,3); a.jump(0x101e40f4); }
            else {
                a.at(0x101e40e0); a.floating_load(0,31,0x1b8); a.floating_compare(31,0); a.compare_less_or_equal_bit();
                if (a.equal()) a.offset(29,29,1);
            }
        }
        a.at(0x101e40f4); a.offset(28,28,1); a.move(27,30); a.offset(30,30,0x1c4);
    }
    a.at(0x101e410c); a.move(3,29); restore_distance_frame(a,27,false);
}

[[gnu::flatten]] void refresh_column_limits(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[3],dependencies,observer);
    a.link(); a.save(31); a.save(30); a.save(29); a.save_link(); a.frame(0x50); a.move(29,3);
    a.value(30,6); a.offset(31,29,0x546c); scan_column_limits(a,0x101e4154,true);
    a.at(0x101e4790); a.offset(0,30,1); a.sign8(0,0); a.store<1>(29,0x7925,0);
    a.value(30,5); a.offset(31,29,0x311c); scan_column_limits(a,0x101e47a4,false);
    a.at(0x101e4de0); a.offset(0,30,-1); a.sign8(3,0); a.store<1>(29,0x7926,3);
    a.pop(0x50); a.restore(31); a.restore(30); a.restore(29); a.finish();
}
}
