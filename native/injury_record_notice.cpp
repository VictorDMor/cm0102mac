#include "injury_record.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::injury_record {
namespace {
using Call=SemanticGuestCall;

void append_club_context(Call& a,std::uint32_t start,std::int32_t field,unsigned index) {
    a.at(start);a.load(3,24,field);a.compare(3,0,false);
    if(!a.equal()) {a.load(5,3,0x20);a.jump(start+0x18);}
    else {a.at(start+0x14);a.value(5,0);}
    a.at(start+0x18);a.offset(3,1,0xb4);a.value(4,index);a.call(0x102cb4b0);
}

template<unsigned Destination>
void current_club(Call& a,std::uint32_t start) {
    a.at(start);a.load(3,28,0);a.move(4,24);a.call(0x10082f20);a.compare(3,0,false);a.store(29,0,3);
    if(!a.equal()) {a.load(0,3,4);a.load(3,31,0);a.multiply(0,0,0x248);a.add(Destination,3,0);a.jump(start+0x30);}
    else {a.at(start+0x2c);a.value(Destination,0);}
}

void route_short_notice(Call& a) {
    a.at(0x1018b1a0);a.compare(24,0,false);if(a.equal())return;
    a.load(4,24,0x24);a.compare(4,0,false);if(a.equal())return;
    a.load(3,30,0);a.call(0x102cdbf0);a.sign8(0,3,true);if(a.equal())return;
    a.load(3,30,0);a.offset(4,1,0xb4);a.load(5,24,0x24);a.call(0x102cc140);
    current_club<0>(a,0x1018b1d4);a.at(0x1018b204);a.compare(0,0,false);
    if(!a.equal()) {
        current_club<3>(a,0x1018b20c);a.at(0x1018b23c);a.load(0,24,0x24);a.compare_registers(3,0,false);
        if(!a.equal()) {current_club<5>(a,0x1018b248);a.at(0x1018b278);a.load(3,30,0);a.offset(4,1,0xb4);a.call(0x102cc140);}
    }
    a.at(0x1018b284);a.load(5,24,0x20);a.compare(5,0,false);if(a.equal())return;
    a.load(3,30,0);a.offset(4,1,0xb4);a.call(0x102cc140);
}

void route_long_notice(Call& a) {
    a.at(0x1018b12c);a.load(3,24,0x14);a.compare(3,0,false);
    if(!a.equal()) {
        a.load<2>(0,3,12,true);a.compare(0,8000);
        if(!a.nonpositive()) {a.load(3,30,0);a.offset(4,1,0xb4);a.call(0x102cc8e0);a.jump(0x1018b29c);return;}
    }
    a.at(0x1018b154);a.value(0,0);a.offset(25,1,0x3c);a.store<1>(1,0x38,0);a.sign8(24,27);a.jump(0x1018b18c);
    for(;;) {
        a.at(0x1018b18c);a.load<1>(3,1,0x38);a.sign8(0,3);a.compare_registers(0,24);if(!a.less())break;
        a.at(0x1018b168);a.sign8(0,3);a.load(3,30,0);a.mask(0,0,0xfffffffc,false,2);a.offset(4,1,0xb4);a.indexed_load(5,25,0);a.call(0x102cc140);
        a.load<1>(3,1,0x38);a.offset(0,3,1);a.store<1>(1,0x38,0);
    }
    a.jump(0x1018b29c);
}
}

void prepare_injury_notice(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,notice_entry,dependencies,observer);
    a.link();a.multiple(24);a.load(3,2,-0x7b40);a.move(24,4);a.load(28,2,-0x7b7c);a.move(25,5);a.load(29,2,-0x7b78);
    a.move(26,6);a.load(30,2,-0x7bf0);a.load(31,2,-0x7be8);a.save_link();a.frame(448);a.load(3,3,0);a.compare(3,0,false);
    if(!a.equal()) {a.offset(6,1,0x3c);a.value(5,90);a.value(7,30);a.call(0x103fc710);a.jump(0x1018af24);}
    else {a.at(0x1018af20);a.value(3,0);}
    a.at(0x1018af24);a.load(5,24,0x20);a.move(27,3);a.compare(5,0,false);
    if(!a.equal()) {
        a.sign8(0,3);a.compare(0,30);
        if(!a.at_least()) {a.mask(0,0,0xfffffffc,false,2);a.offset(4,1,0x3c);a.indexed_store(5,4,0);a.offset(27,3,1);}
    }
    a.at(0x1018af50);a.offset(3,1,0xb4);a.value(4,2000);a.value(5,0);a.call(0x102bdf90);
    a.load(5,24,0);a.offset(3,1,0xb4);a.value(4,0);a.call(0x102cb4b0);
    append_club_context(a,0x1018af70,0x28,1);append_club_context(a,0x1018af94,0x2c,2);append_club_context(a,0x1018afb8,0x30,3);
    a.at(0x1018afdc);a.load<1>(5,25,0);a.offset(3,1,0xb4);a.value(4,4);a.sign8(5,5);a.call(0x102cb4b0);
    a.load<2>(5,25,0x28,true);a.offset(3,1,0xb4);a.value(4,5);a.call(0x102cb4b0);
    a.load<2>(5,25,0x2a);a.offset(3,1,0xb4);a.value(4,6);a.call(0x102cb4b0);
    a.load<2>(5,25,0x30,true);a.offset(3,1,0xb4);a.value(4,7);a.call(0x102cb4b0);
    a.load<2>(5,24,0x34,true);a.offset(3,1,0xb4);a.value(4,8);a.call(0x102cb4b0);
    a.load<2>(5,24,0x36);a.offset(3,1,0xb4);a.value(4,9);a.call(0x102cb4b0);
    a.load<1>(0,25,0x20);a.sign8(0,0);a.compare(0,-1);
    if(!a.equal()) {
        for(auto [field,index]:{std::pair{0x18,10},std::pair{0x1c,11},std::pair{12,12}}) {a.load(5,25,field);a.offset(3,1,0xb4);a.value(4,index);a.call(0x102cb4b0);}
        a.jump(0x1018b0b4);
    } else {
        a.at(0x1018b084);
        for(unsigned index=10;index<=12;++index) {a.offset(3,1,0xb4);a.value(4,index);a.value(5,0xffffffff);a.call(0x102cb4b0);}
    }
    a.at(0x1018b0b4);a.move(5,26);a.offset(3,1,0xb4);a.value(4,13);a.call(0x102cb4b0);a.load(3,24,0x24);a.compare(3,0,false);
    if(!a.equal()) {a.load(5,3,0);a.offset(3,1,0xb4);a.value(4,14);a.call(0x102cb4b0);a.jump(0x1018b0f4);}
    else {a.at(0x1018b0e4);a.offset(3,1,0xb4);a.value(4,14);a.value(5,0xffffffff);a.call(0x102cb4b0);}
    a.at(0x1018b0f4);a.value(4,0x40000);a.offset(3,1,0xb4);a.offset(5,4,0x20e9);a.value(4,15);a.call(0x102cb4b0);
    a.value(0,16);a.offset(3,25,4);a.store<1>(1,0x38,0);a.offset(4,1,0xb4);a.offset(5,1,0x38);a.call(0x1030dcd0);
    a.load<2>(0,25,0x30,true);a.compare(0,100);
    if(!a.nonpositive())route_long_notice(a);else route_short_notice(a);
    a.at(0x1018b29c);a.pop(448);a.multiple(24,true);a.finish();
}
}
