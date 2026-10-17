#include "injury_reporting.hpp"
#include "injury_reporting_detail.hpp"

namespace cm::aot::injury_reporting {
namespace {
using namespace detail;
void describe_score(Call& a,std::uint32_t start,std::int32_t token) {
    a.at(start);a.load<1>(7,27,0x23b);a.offset(0,27,0x221);a.load<1>(5,28,0x23b);a.move(3,30);a.sign8(7,7);a.offset(4,31,token);a.sign8(5,5);a.store(1,0x38,0);a.move(6,5);a.move(8,7);a.offset(9,28,0x221);a.offset(10,1,0x40);a.call(0x10727890);
}
}
void describe_injury_match(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,match_context_entry,d,o);
    a.link();a.multiple(25);a.move(25,4);a.load(6,2,-0x7be8);a.move(26,5);a.load(29,2,-0x7bbc);a.load(30,2,-0x7bb8);a.load(31,2,-0x5438);a.save_link();a.frame(112);
    a.load(3,4,0x1c);a.load(0,4,0x18);a.load<1>(8,4,0x20);a.multiply(3,3,0x248);a.load(5,6,0);a.load<1>(7,25,0x21);a.multiply(4,0,0x248);a.sign8(0,8);a.sign8(6,7);a.add(27,5,3);a.compare_registers(0,6);a.add(28,5,4);
    if(!a.nonpositive()) {
        a.move(5,0);a.offset(3,1,0x40);a.offset(4,31,0x1bef);a.call(0x10727890);describe_score(a,0x1018fc4c,0x1c17);a.jump(0x1018fd18);
    } else {
        a.at(0x1018fc84);
        if(!a.at_least()) {
            a.move(5,6);a.move(6,0);a.offset(3,1,0x40);a.offset(4,31,0x1bef);a.call(0x10727890);describe_score(a,0x1018fc9c,0x1c87);a.jump(0x1018fd18);
        } else {
            a.at(0x1018fcd4);a.move(5,0);a.offset(3,1,0x40);a.offset(4,31,0x1bef);a.call(0x10727890);describe_score(a,0x1018fce4,0x1cfe);
        }
    }
    a.at(0x1018fd18);a.move(3,26);a.move(4,30);a.call(0x1074b830);
    a.move(3,26);a.move(6,28);a.offset(4,28,0x221);a.value(5,0x7d5);a.call(0x1074bab0);
    a.move(3,26);a.move(6,27);a.offset(4,27,0x221);a.value(5,0x7d5);a.call(0x1074bab0);
    a.load(0,25,12);a.load(3,2,-0x7b34);a.multiply(0,0,0x6c);a.load(4,29,0);a.load(3,3,0);a.add(4,4,0);a.call(0x104abf40);a.sign8(0,3,true);
    if(!a.equal()){a.load(0,25,12);a.move(3,30);a.load(6,29,0);a.offset(4,31,0x1d70);a.multiply(5,0,0x6c);a.offset(5,5,0x4d);a.add(5,6,5);a.call(0x10727890);a.move(3,26);a.move(4,30);a.call(0x1074b960);}
    a.at(0x1018fd9c);a.load(3,2,-0x7b4c);a.load<2>(0,25,4,true);a.load(3,3,0);a.load(4,25,0x24);a.mask(7,0,0xffff);a.load(5,25,0x18);a.load(6,25,0x1c);a.load<2>(8,25,6);a.call(0x1029eb20);a.sign8(0,3,true);
    if(!a.equal()){a.load(6,25,0x24);a.move(3,26);a.offset(4,1,0x40);a.value(5,0x7d3);a.call(0x1074bab0);}
    a.at(0x1018fddc);a.pop(112);a.multiple(25,true);a.finish();
}
}
