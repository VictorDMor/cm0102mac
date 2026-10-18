#include "awol_records.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::awol {
namespace {
using Call=SemanticGuestCall;
bool valid_notice(Call& a) {
    if(a.equal())return false;
    a.load(0,31,0x24);a.compare(0,0,false);if(a.equal())return false;
    a.load(0,31,0x14);a.compare(0,0,false);if(a.equal())return false;
    a.compare(26,0,false);return !a.equal();
}
void fill_notice(Call& a) {
    a.offset(3,1,0x38);a.value(4,0xbc5);a.value(5,0);a.call(0x102bdf90);
    a.load(5,31,0);a.offset(3,1,0x38);a.value(4,0);a.call(0x102cb4b0);
    for(unsigned field=0;field<3;++field){a.at(0x1000716c+field*20);a.load(5,31,int(0x28+field*4));a.offset(3,1,0x38);a.value(4,field+1);a.load(5,5,0x20);a.call(0x102cb4b0);}
    a.at(0x100071a8);a.compare(26,0,false);
    if(!a.equal()){a.load(5,26,0);a.offset(3,1,0x38);a.value(4,4);a.call(0x102cb4b0);a.jump(0x100071d8);}
    else {a.at(0x100071c4);a.offset(3,1,0x38);a.value(29,0xffffffff);a.value(4,4);a.value(5,0xffffffff);a.call(0x102cb4b0);}
    a.at(0x100071d8);a.load(3,31,0x24);a.compare(3,0,false);
    if(!a.equal()){a.load(5,3,0);a.offset(3,1,0x38);a.value(4,5);a.call(0x102cb4b0);a.jump(0x1000720c);}
    else {a.at(0x100071f8);a.offset(3,1,0x38);a.value(29,0xffffffff);a.value(4,5);a.value(5,0xffffffff);a.call(0x102cb4b0);}
    a.at(0x1000720c);a.offset(3,1,0x38);a.sign8(5,27);a.value(4,6);a.call(0x102cb4b0);a.offset(3,1,0x38);a.sign16(5,28);a.value(4,7);a.call(0x102cb4b0);
    a.load(3,31,0x24);a.compare(3,0,false);bool has_manager=!a.equal();
    if(has_manager){a.load(3,3,0x2c);a.compare(3,0,false);has_manager=!a.equal();}
    if(has_manager){a.load(5,3,0x68);a.offset(3,1,0x38);a.value(4,8);a.call(0x102cb4b0);a.jump(0x10007268);}
    else {a.at(0x10007258);a.move(5,29);a.offset(3,1,0x38);a.value(4,8);a.call(0x102cb4b0);}
}
void send_notice(Call& a) {
    a.at(0x10007268);a.load(3,31,0x14);a.load<2>(0,3,12,true);a.compare(0,7500);
    if(!a.nonpositive()){a.load(3,30,0);a.offset(4,1,0x38);a.call(0x102cc8e0);a.jump(0x100072cc);return;}
    a.at(0x10007288);a.load(3,31,0x24);a.compare(3,0,false);bool has_manager=!a.equal();
    if(has_manager){a.load(5,3,0x2c);a.compare(5,0,false);has_manager=!a.equal();}
    if(has_manager){a.load(3,30,0);a.offset(4,1,0x38);a.call(0x102cc400);}
    a.at(0x100072ac);a.load(3,31,0x24);a.load(5,31,12);a.load(0,3,0x2c);a.compare_registers(0,5,false);
    if(!a.equal()){a.load(3,30,0);a.offset(4,1,0x38);a.call(0x102cc400);}
}
}
void notify_absence_change(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,notice_entry,d,o);a.link();a.multiple(26);a.move(31,4,true);a.load(30,2,-0x7bf0);a.move(26,5);a.move(27,6);a.move(28,7);a.value(29,0xffffffff);a.save_link();a.frame(320);
    if(valid_notice(a)){a.at(0x1000714c);fill_notice(a);send_notice(a);}
    a.at(0x100072cc);a.pop(320);a.multiple(26,true);a.finish();
}
}
