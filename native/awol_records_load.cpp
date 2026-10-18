#include "awol_records.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::awol {
namespace {
using Call=SemanticGuestCall;
void failed_read(Call& a,std::uint32_t start) {
    a.at(start);a.offset(3,1,0x40);a.call(0x10747900);a.value(3,0);a.jump(0x100036c4);
}
bool read_primary_list(Call& a) {
    a.at(0x100034c8);a.load(6,1,0x40);a.offset(3,1,0x38);a.value(4,4);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
    if(!a.equal()){failed_read(a,0x100034e4);return false;}
    a.at(0x100034f4);a.value(3,16);a.call(0x10773520);a.move(30,3,true);if(!a.equal())a.call(0x10738c00);
    a.at(0x10003508);a.store(31,0,30);a.value(30,0);a.jump(0x10003558);
    while(true) {
        a.at(0x10003558);a.load(0,1,0x38);a.compare_registers(30,0);if(!a.less())break;
        a.at(0x10003514);a.load(6,1,0x40);a.offset(3,1,0x54);a.value(4,12);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
        if(!a.equal()){failed_read(a,0x10003530);return false;}
        a.at(0x10003544);a.load(3,31,0);a.offset(4,1,0x54);a.value(5,12);a.call(0x10738d10);a.offset(30,30,1);
    }
    return true;
}
bool read_secondary_list(Call& a) {
    a.at(0x10003564);a.load(6,1,0x40);a.offset(3,1,0x38);a.value(4,4);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
    if(!a.equal()){failed_read(a,0x10003580);return false;}
    a.at(0x10003590);a.value(3,16);a.call(0x10773520);a.move(30,3,true);if(!a.equal())a.call(0x10738c00);
    a.at(0x100035a4);a.store(31,4,30);a.value(30,0);a.jump(0x100035f4);
    while(true) {
        a.at(0x100035f4);a.load(0,1,0x38);a.compare_registers(30,0);if(!a.less())break;
        a.at(0x100035b0);a.load(6,1,0x40);a.offset(3,1,0x44);a.value(4,16);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
        if(!a.equal()){failed_read(a,0x100035cc);return false;}
        a.at(0x100035e0);a.load(3,31,4);a.offset(4,1,0x44);a.value(5,16);a.call(0x10738d10);a.offset(30,30,1);
    }
    return true;
}
bool read_pending_list(Call& a) {
    a.at(0x10003600);a.value(3,16);a.call(0x10773520);a.move(30,3,true);if(!a.equal())a.call(0x10738c00);
    a.at(0x10003614);a.store(31,8,30);a.load(3,2,-0x7b74);a.load(3,3,0);a.call(0x10775550);a.value(4,0x40000);a.offset(0,4,-0x1ffb);a.compare_registers(3,0);
    if(a.nonpositive())return true;
    a.load(6,1,0x40);a.offset(3,1,0x38);a.value(4,4);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
    if(!a.equal()){failed_read(a,0x10003650);return false;}
    a.at(0x10003660);a.value(30,0);a.jump(0x100036ac);
    while(true) {
        a.at(0x100036ac);a.load(0,1,0x38);a.compare_registers(30,0);if(!a.less())break;
        a.at(0x10003668);a.load(6,1,0x40);a.offset(3,1,0x60);a.value(4,20);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
        if(!a.equal()){failed_read(a,0x10003684);return false;}
        a.at(0x10003698);a.load(3,31,8);a.offset(4,1,0x60);a.value(5,20);a.call(0x10738d10);a.offset(30,30,1);
    }
    return true;
}
bool read_archive(Call& a) {
    a.at(0x100033f4);a.compare(30,0,false);
    if(a.equal()){a.value(3,0);a.jump(0x100036c4);return false;}
    a.at(0x10003404);a.load(4,2,-0x5a08);a.move(3,30);a.value(5,0);a.value(6,22);a.value(7,4);a.call(0x107475b0);a.compare(3,0,false);a.store(1,0x40,3);
    if(a.equal()){a.value(3,0);a.jump(0x100036c4);return false;}
    a.at(0x10003430);a.move(6,3);a.offset(3,1,0x3c);a.value(4,4);a.value(5,1);a.call(0x10747a80);a.compare(3,1);
    if(!a.equal()){failed_read(a,0x1000344c);return false;}
    a.at(0x1000345c);a.load(3,1,0x3c);a.compare(3,0);
    if(a.equal()){failed_read(a,0x10003468);return false;}
    a.at(0x10003478);a.call(0x10775130);a.store(31,12,3);a.load(3,31,12);a.compare(3,0,false);
    if(a.equal()){failed_read(a,0x1000348c);return false;}
    a.at(0x1000349c);a.load(5,1,0x3c);a.value(4,1);a.load(6,1,0x40);a.call(0x10747a80);a.load(0,1,0x3c);a.compare_registers(0,3);
    if(!a.equal()){failed_read(a,0x100034b8);return false;}
    if(!read_primary_list(a)||!read_secondary_list(a)||!read_pending_list(a))return false;
    a.at(0x100036b8);a.offset(3,1,0x40);a.call(0x10747900);a.value(3,1);return true;
}
}
void load_records(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,load_entry,d,o);a.link();a.save(31);a.move(31,3);a.save(30);a.move(30,4);a.save_link();a.frame(128);
    a.offset(3,1,0x58);a.call(0x10715880);a.offset(3,1,0x48);a.call(0x10715880);a.offset(3,1,0x68);a.call(0x10715880);
    read_archive(a);a.at(0x100036c4);a.pop(128);a.restore(31);a.restore(30);a.finish();
}
}
