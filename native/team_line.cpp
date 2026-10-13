#include "team_line.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::team_line {
namespace {
using Call = SemanticGuestCall;
constexpr std::uint32_t column=0x101d5680, row=0x101d5690;
constexpr std::uint32_t side=0x10283690, position=0x101d8110, role=0x10289560;

void finish_lane(Call& a) {
    a.at(0x101e5050); a.pop(0x60); a.multiple(25,true); a.finish();
}

// An active actor occupying the exact cell always blocks it, even a goalkeeper.
// Other actors block only the same row in the selected direction. Repeated
// queries are intentional: a dependency can change live fields between calls.
bool blocks_lane(Call& a) {
    a.at(0x101e4f50); a.move(3,31); a.call(position); a.sign8(0,3); a.compare(0,0);
    if(a.less()) return false;
    a.move(3,31); a.call(row); a.sign8(3,3); a.sign8(0,27); a.compare_registers(0,3);
    bool exact=a.equal();
    if(exact) {
        a.move(3,31); a.call(column); a.sign8(3,3); a.sign8(0,30); a.compare_registers(0,3);
        exact=a.equal();
    }
    if(exact) { a.value(3,0); a.jump(0x101e5050); return true; }
    a.at(0x101e4f9c); a.move(3,31); a.call(role); a.sign16(0,3); a.compare(0,1);
    if(a.equal()) return false;
    a.sign8(0,28); a.compare(0,1); bool ahead=false;
    if(a.equal()) {
        a.move(3,31); a.call(column); a.sign8(3,3); a.sign8(0,30); a.compare_registers(0,3);
        ahead=a.positive();
    }
    if(!ahead) {
        a.at(0x101e4fd4); a.sign8(0,28); a.compare(0,0);
        if(!a.equal()) return false;
        a.move(3,31); a.call(column); a.sign8(3,3); a.sign8(0,30); a.compare_registers(0,3);
        if(a.at_least()) return false;
    }
    a.at(0x101e4ff8); a.move(3,31); a.call(row); a.sign8(4,3); a.sign8(3,27); a.offset(0,3,1); a.compare_registers(0,4);
    if(a.nonpositive()) return false;
    a.move(3,31); a.call(row); a.sign8(4,3); a.sign8(3,27); a.offset(0,3,-1); a.compare_registers(0,4);
    if(a.at_least()) return false;
    a.value(3,0); a.jump(0x101e5050); return true;
}
}

void tactical_flags(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[0],dependencies,observer); a.load(3,3,0x588); a.finish();
}

void line_adjustment_enabled(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[1],dependencies,observer);
    a.link(); a.save_link(); a.frame(0x40); a.store(1,0x58,3); a.store<1>(1,0x5f,4);
    a.load(4,1,0x58); a.load<1>(0,1,0x5f); a.sign8(0,0); a.multiply(3,0,0x18e0);
    a.offset(3,3,0x7c6c); a.add(3,4,3); a.call(entries[0]); a.mask(0,3,0x400); a.compare(0,0);
    if(!a.equal()) { a.value(3,1); a.jump(0x101d7d78); }
    else { a.at(0x101d7d74); a.value(3,0); }
    a.at(0x101d7d78); a.pop(0x40); a.finish();
}

void actor_column_excess(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[2],dependencies,observer);
    a.link(); a.save(31); a.save(30); a.save(29); a.save_link(); a.frame(0x50);
    a.move(31,3); a.move(29,4); a.move(3,29); a.call(column); a.move(30,3);
    a.offset(3,31,0x10000); a.load(0,3,-0x2000); a.compare_registers(29,0,false);
    if(a.equal()) { a.value(3,0); a.jump(0x101e4e84); }
    else {
        a.at(0x101e4e54); a.move(3,29); a.call(side); a.sign8(0,3); a.compare(0,1);
        if(a.equal()) { a.load<1>(0,31,0x7926); a.subtract(0,30,0); a.sign8(3,0); a.jump(0x101e4e84); }
        else { a.at(0x101e4e78); a.load<1>(0,31,0x7925); a.subtract(0,0,30); a.sign8(3,0); }
    }
    a.at(0x101e4e84); a.pop(0x50); a.restore(31); a.restore(30); a.restore(29); a.finish();
}

void column_excess(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[3],dependencies,observer); a.sign8(0,4); a.compare(0,1);
    if(a.equal()) { a.load<1>(0,3,0x7926); a.subtract(0,5,0); a.sign8(3,0); a.jump(0x101e4ec8); }
    else { a.at(0x101e4ebc); a.load<1>(0,3,0x7925); a.subtract(0,0,5); a.sign8(3,0); }
    a.at(0x101e4ec8); a.finish();
}

void row_lane_clear(State& state, SemanticDependencyDispatch* dependencies, SemanticGuestObserver* observer) {
    Call a(state,entries[4],dependencies,observer);
    a.link(); a.multiple(25); a.save_link(); a.frame(0x60); a.store(1,0x78,3); a.move(28,4); a.move(27,5); a.move(30,6);
    a.sign8(0,28); a.leading_zeroes(0,0); a.mask(0,0,0x07ffffff,false,27); a.sign8(26,0);
    a.sign8(0,28); a.compare(0,0);
    if(a.equal()) {
        a.sign8(0,30); a.compare(0,8);
        if(!a.nonpositive()) { a.value(3,0); a.jump(0x101e5050); finish_lane(a); return; }
    } else {
        a.at(0x101e4f20); a.sign8(0,30); a.compare(0,3);
        if(!a.at_least()) { a.value(3,0); a.jump(0x101e5050); finish_lane(a); return; }
    }
    a.at(0x101e4f34); a.load(4,1,0x78); a.sign8(0,26); a.multiply(3,0,0x2350);
    a.offset(31,3,0x311c); a.add(31,4,31); a.value(29,0); a.jump(0x101e5044);
    for(;;) {
        a.at(0x101e5044); a.compare(29,20); if(!a.less()) break;
        if(blocks_lane(a)) { finish_lane(a); return; }
        a.at(0x101e5038); a.offset(29,29,1); a.move(25,31); a.offset(31,31,0x1c4);
    }
    a.at(0x101e504c); a.value(3,1); finish_lane(a);
}
}
