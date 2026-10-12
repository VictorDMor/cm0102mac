#include "actor_construction_abi.hpp"

namespace cm::aot::actor_construction {
namespace {
void report_invalid_actor(ConstructionCall& a) {
    a.at(0x1020e93c); a.load(3, 2, -0x7bb8); a.offset(4, 31, 0x3f); a.sign8(5, 27); a.call(0x1077b720);
    a.move(3, 31); a.offset(6, 1, 0x40); a.offset(7, 1, 0x38); a.value(4, 0); a.value(5, 0);
    a.call(0x10739890); a.call(0x10768810); a.move(5, 3); a.move(3, 30); a.offset(4, 31, 0xe);
    a.offset(6, 1, 0x40); a.offset(7, 1, 0x38); a.value(8, 0xc37); a.call(0x1077b720);
    a.move(4, 30); a.offset(3, 31, 0x1b); a.value(5, 0); a.call(0x10769b60);
    a.load(4, 2, -0x7bac); a.value(0, 0); a.value(3, 0xffffffff); a.store<1>(4, 0, 0); a.call(0x1076f7d0);
}
void prepare_body(ConstructionCall& a) {
    a.link(); a.multiple(26); a.move(26, 3); a.move(27, 4); a.load(30, 2, -0x7ba8);
    a.move(28, 5); a.load(31, 2, -0x5274); a.move(29, 6); a.value(3, 0); a.save_link();
    a.sign8(0, 27, true); a.frame(0x80);
    bool valid_index = !a.less();
    if (valid_index) { a.sign8(0, 27); a.compare(0, 19); valid_index = !a.positive(); }
    if (valid_index) a.value(3, 1);
    a.at(0x1020e934); a.compare(3, 0);
    if (a.equal()) report_invalid_actor(a);
    a.at(0x1020e9a8); a.compare(28, 0, false);
    if (a.equal()) { a.value(3, 0); a.jump(0x1020e9e4); return; }
    a.at(0x1020e9b8); a.sign8(0, 27); a.load(3, 26, 0x28); a.multiply(0, 0, 0x1c4);
    a.load(4, 26, 0x14); a.load<1>(6, 26, 0x1a); a.move(5, 28); a.move(7, 27); a.move(8, 29);
    a.add(3, 3, 0); a.call(entries[1]); a.value(3, 1);
}
}
void prepare_actor(State& state, DependencyDispatch* dependencies) {
    ConstructionCall a(state, entries[0], dependencies); prepare_body(a);
    a.at(0x1020e9e4); a.pop(0x80); a.multiple(26, true); a.finish();
}
}
