#include "target_acceptance.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::target_acceptance {
using Call = SemanticGuestCall;
namespace {
bool scan_cell_candidates(Call&,State&);
bool scan_cell_candidates(Call& a,[[maybe_unused]] State& state) {
    for (;;) {
        a.at(0x101f0a94); a.sign8(3,26); a.offset(0,3,0x58); a.indexed_load<1>(0,28,0); a.sign8(0,0);
        a.compare_registers(30,0);
        if (a.at_least()) return true;
        a.at(0x101f0a0c); a.sign8(0,26); a.multiply(3,0,0x2c); a.mask(0,30,0xfffffffc,false,2); a.add(0,3,0);
        a.indexed_load(31,28,0); a.load<1>(0,31,0x2c); a.sign8(0,0); a.compare(0,1);
        if (a.positive()) {
            a.at(0x101f0a90); a.offset(30,30,0x1);
            continue;
        } else {
            a.at(0x101f0a30); a.load<1>(0,31,0x19); a.sign8(0,0); a.compare(0,0);
            if (a.less()) {
                a.at(0x101f0a90); a.offset(30,30,0x1);
                continue;
            } else {
                a.at(0x101f0a40); a.load<1>(0,1,0x87); a.sign8(0,0); a.compare(0,2);
                if (!a.equal()) {
                    a.at(0x101f0a58); a.move(3,31); a.load<2>(4,1,0x7a,true); a.load<1>(5,1,0x7f);
                    a.floating_load(1,1,116,true); a.load<1>(7,1,0x83); a.call(0x10239f10); a.sign8(0,3);
                    a.compare(0,0);
                    if (a.equal()) {
                        a.at(0x101f0a90); a.offset(30,30,0x1);
                        continue;
                    } else {
                        a.at(0x101f0a7c); a.value(0,0x0); a.store<1>(27,0x1bf,0); a.store<1>(27,0x1be,0);
                        a.value(3,0x1); a.jump(0x101f0ab0);
                        return false;
                    }
                } else {
                    a.at(0x101f0a50); a.offset(29,29,0x1); a.jump(0x101f0a90); a.offset(30,30,0x1);
                    continue;
                }
            }
        }
    }
}
}
void cell_occupants(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,0x101f09d0,dependencies,observer);
    a.at(0x101f09d0); a.link(); a.multiple(26,false,0); a.save_link(); a.frame(80); a.move(27,3); a.move(26,4);
    a.move(28,5); a.floating_store_single(1,1,116); a.store<2>(1,0x7a,7); a.store<1>(1,0x7f,8);
    a.store<1>(1,0x83,9); a.store<1>(1,0x87,10); a.value(29,0x0); a.value(30,0x0); a.jump(0x101f0a94);
    if (scan_cell_candidates(a,state)) {
        a.at(0x101f0aac); a.move(3,29);
    }
    a.at(0x101f0ab0); a.load(0,1,0x58); a.offset(1,1,0x50); a.restore_link(); a.multiple(26,true,0); a.finish();
}
}
