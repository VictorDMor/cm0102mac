#include "actor_encounter.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_encounter {
using Call = SemanticGuestCall;
namespace {
void scan_team_scores(Call&);
void scan_team_scores(Call& a) {
    for (;;) {
        a.at(0x101d68c8); a.compare(30,20);
        if (a.at_least()) break;
        a.at(0x101d68a4); a.load(0,31,0x6c); a.compare(0,0,false);
        if (!a.equal()) {
            a.at(0x101d68b0); a.move(3,31); a.floating_load(1,1,112,true); a.call(0x102865a0);
        }
        a.at(0x101d68bc); a.offset(30,30,0x1); a.move(29,31); a.offset(31,31,0x1c4);
    }
}
}
void scale_team_scores(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101d6860,dependencies,observer);
    a.at(0x101d6860); a.link(); a.store(1,-0x4,31); a.store(1,-0x8,30); a.store(1,-0xc,29); a.save_link();
    a.frame(80); a.store(1,0x68,3); a.store<1>(1,0x6f,4); a.floating_store_single(1,1,112); a.load(4,1,0x68);
    a.load<1>(0,1,0x6f); a.sign8(0,0); a.multiply(3,0,0x2350); a.offset(31,3,0x311c); a.add(31,4,31);
    a.value(30,0x0); a.jump(0x101d68c8);
    scan_team_scores(a);
    a.at(0x101d68d0); a.pop(80); a.load(31,1,-0x4); a.load(30,1,-0x8); a.load(29,1,-0xc); a.finish();
}
}
