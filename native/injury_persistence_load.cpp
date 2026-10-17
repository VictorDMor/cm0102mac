#include "injury_persistence.hpp"
#include "injury_persistence_detail.hpp"

namespace cm::aot::injury_persistence {
using namespace detail;
namespace {
bool load_legacy_records(Call& a) {
    a.at(0x10184840);a.value(27,0);a.value(25,0);a.jump(0x10184940);
    while(true) {
        a.at(0x10184940);a.load(0,26,8);a.compare_registers(27,0);if(!a.less())break;
        a.at(0x10184850);a.load(0,26,4);a.value(4,0x34);a.load(6,1,0x3c);a.value(5,1);a.add(3,0,25);a.call(0x10747a80);a.compare(3,1);
        if(!a.equal()){fail_load(a,0x10184870,0x1f8,0xa0,0x17f);return false;}
        a.at(0x101848d0);a.load(0,26,4);a.offset(3,1,0x48);a.add(4,0,25);a.load<2>(5,4,0x30,true);a.offset(4,4,0x28);a.call(0x10717890);
        a.load(4,28,0);a.offset(3,1,0x48);a.load(5,28,4);a.call(0x10718130);a.load(4,26,4);a.offset(0,25,0x34);a.indexed_store<2>(3,4,0);
        a.offset(3,1,0x40);a.load(0,26,4);a.add(4,0,25);a.load<2>(5,4,0x32,true);a.offset(4,4,0x28);a.call(0x10717890);
        a.load(4,28,0);a.offset(3,1,0x40);a.load(5,28,4);a.call(0x10718130);a.load(4,26,4);a.offset(0,25,0x36);a.offset(27,27,1);a.offset(25,25,0x38);a.indexed_store<2>(3,4,0);
    }
    a.at(0x1018494c);a.jump(0x101849d0);return true;
}
}
void load_controller(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,load_entry,d,o);
    a.link();a.multiple(25);a.compare(4,0,false);a.load(27,2,-0x7be4);a.load(28,2,-0x7b80);a.move(26,3);a.load(29,2,-0x7bac);a.load(30,2,-0x7ba8);a.load(31,2,-0x5438);a.save_link();a.frame(768);
    if(a.equal()){fail_load(a,0x10184560,0x2b8,0xd0,0x148,false);return;}
    a.at(0x101845b8);a.move(3,4);a.offset(4,31,0x71);a.value(5,0);a.value(6,0x16);a.value(7,4);a.call(0x107475b0);a.compare(3,0,false);a.store(1,0x3c,3);
    if(a.equal()){fail_load(a,0x101845dc,0x298,0xc8,0x150,false);return;}
    a.at(0x10184634);a.load(0,27,0);a.multiply(3,0,0x24);a.call(0x10775130);a.store(26,0,3);a.load(3,26,0);a.compare(3,0,false);
    if(a.equal()){fail_load(a,0x10184650,0x278,0xc0,0x158);return;}
    a.at(0x101846b0);a.load(5,27,0);a.value(4,0x24);a.load(6,1,0x3c);a.call(0x10747a80);a.load(0,27,0);a.compare_registers(0,3);
    if(!a.equal()){fail_load(a,0x101846cc,0x258,0xb8,0x160);return;}
    a.at(0x1018472c);a.load(6,1,0x3c);a.offset(3,26,8);a.value(4,4);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
    if(!a.equal()){fail_load(a,0x10184748,0x238,0xb0,0x168);return;}
    a.at(0x101847a8);a.load(0,26,8);a.multiply(3,0,0x38);a.call(0x10775130);a.store(26,4,3);a.load(0,26,4);a.compare(0,0,false);
    if(a.equal()){fail_load(a,0x101847c4,0x218,0xa8,0x171);return;}
    a.at(0x10184824);a.load(3,2,-0x7b74);a.load(3,3,0);a.call(0x10775550);a.value(4,0x40000);a.offset(0,4,-0x1fdc);a.compare_registers(3,0);
    if(!a.at_least()) {if(!load_legacy_records(a))return;}
    else {
        a.at(0x10184950);a.load(3,26,4);a.value(4,0x38);a.load(5,26,8);a.load(6,1,0x3c);a.call(0x10747a80);a.load(0,26,8);a.compare_registers(0,3);
        if(!a.equal()){fail_load(a,0x10184970,0x1d8,0x98,0x18e);return;}
    }
    a.at(0x101849d0);a.load(6,1,0x3c);a.offset(3,26,0x14);a.value(4,4);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
    if(!a.equal()){fail_load(a,0x101849ec,0x1b8,0x90,0x197);return;}
    a.at(0x10184a4c);a.load(0,26,0x14);a.compare(0,0);
    if(!a.equal()) {
        a.store(26,0x18,0);a.load(0,26,0x14);a.multiply(3,0,0x18);a.call(0x10775130);a.store(26,0x10,3);a.load(3,26,0x10);a.compare(3,0,false);
        if(a.equal()){fail_load(a,0x10184a78,0x198,0x88,0x1a6);return;}
        a.at(0x10184ad8);a.load(5,26,0x14);a.value(4,0x18);a.load(6,1,0x3c);a.call(0x10747a80);a.load(0,26,0x14);a.compare_registers(0,3);
        if(!a.equal()){fail_load(a,0x10184af4,0x178,0x80,0x1ae);return;}
    }
    a.at(0x10184b54);a.load(6,1,0x3c);a.offset(3,26,0x20);a.value(4,4);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
    if(!a.equal()){fail_load(a,0x10184b70,0x158,0x78,0x1b7);return;}
    a.at(0x10184bd0);a.load(0,26,0x20);a.compare(0,0);
    if(!a.equal()) {
        a.store(26,0x24,0);a.load(0,26,0x20);a.multiply(3,0,0x14);a.call(0x10775130);a.store(26,0x1c,3);a.load(3,26,0x1c);a.compare(3,0,false);
        if(a.equal()){fail_load(a,0x10184bfc,0x138,0x70,0x1c7);return;}
        a.at(0x10184c5c);a.load(5,26,0x20);a.value(4,0x14);a.load(6,1,0x3c);a.call(0x10747a80);a.load(0,26,0x20);a.compare_registers(0,3);
        if(!a.equal()){fail_load(a,0x10184c78,0x118,0x68,0x1d0);return;}
    }
    a.at(0x10184cd8);a.load(6,1,0x3c);a.offset(3,1,0x38);a.value(4,2);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
    if(!a.equal()){fail_load(a,0x10184cf4,0xf8,0x60,0x1d9);return;}
    a.at(0x10184d54);a.load<2>(0,1,0x38,true);a.compare(0,0);
    if(!a.equal()) {
        a.load(3,26,0x28);a.compare(3,0,false);if(!a.equal()){a.value(4,1);a.call(0x10738c20);}
        a.at(0x10184d74);a.value(3,0x10);a.call(0x10773520);a.move(25,3,true);if(!a.equal())a.call(0x10738c00);
        a.at(0x10184d88);a.store(26,0x28,25);a.value(25,0);a.jump(0x10184e28);
        while(true) {
            a.at(0x10184e28);a.load<2>(0,1,0x38,true);a.sign16(3,25);a.compare_registers(3,0);if(!a.less())break;
            a.at(0x10184d94);a.load(6,1,0x3c);a.offset(3,1,0x58);a.value(4,8);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
            if(!a.equal()){fail_load(a,0x10184db0,0xd8,0x50,0x1f0);return;}
            a.at(0x10184e14);a.load(3,26,0x28);a.offset(4,1,0x58);a.value(5,8);a.call(0x10738d10);a.offset(25,25,1);
        }
    }
    a.at(0x10184e38);a.offset(3,1,0x3c);a.call(0x10747900);a.value(3,1);finish_load(a);
}
}
