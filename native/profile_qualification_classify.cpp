#include "profile_qualification_detail.hpp"

namespace cm::aot::profile_qualification::detail {
void report_missing_profile(Call& a) {
    a.at(0x101174bc);a.offset(3,31,0xe);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(4,0);a.value(5,0);
    a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.move(3,30);a.offset(4,31,0x1c);a.offset(6,1,0x40);a.offset(7,1,0x38);a.value(8,0xe79);
    a.call(0x1077b720);a.move(4,30);a.offset(3,31,0x29);a.value(5,0);a.call(0x10769b60);a.load(4,2,-0x7bac);a.value(0,0);a.value(3,0);a.store<1>(4,0,0);a.jump(0x10117670);
}
void scan_primary_qualifications(Call& a) {
    a.at(0x10117518);a.value(4,15);a.flags(0,0,0);
    // Stop descending as soon as any primary qualification is present.
    // The three central fields are deliberately tested in priority order.
    while(true) {
        a.at(0x10117520);a.load<1>(6,3,15);a.sign8(7,4);a.sign8(6,6);a.compare_registers(7,6);
        if(!a.positive()){a.flags(0,0,1);a.sign16(0,0);}
        a.at(0x1011753c);a.load<1>(7,3,17);a.sign8(6,4);a.sign8(7,7);a.compare_registers(7,6);
        if(!a.less()){a.flags(0,0,4);a.sign16(0,0);}
        a.at(0x10117558);a.load<1>(7,3,16);a.sign8(6,4);a.sign8(7,7);a.compare_registers(7,6);
        if(!a.less()){a.flags(0,0,2);a.sign16(0,0);}
        a.at(0x10117574);a.load<1>(6,3,18);a.sign8(7,4);a.sign8(6,6);a.compare_registers(6,7);
        if(!a.less()){a.flags(0,0,8);a.sign16(0,0);a.jump(0x101175c8);}
        else {
            a.at(0x10117594);a.load<1>(6,3,20);a.sign8(6,6);a.compare_registers(6,7);
            if(!a.less()){a.flags(0,0,32);a.sign16(0,0);a.jump(0x101175c8);}
            else {
                a.at(0x101175b0);a.load<1>(6,3,19);a.sign8(6,6);a.compare_registers(6,7);
                if(!a.less()){a.flags(0,0,16);a.sign16(0,0);}
            }
        }
        a.at(0x101175c8);a.load<1>(7,3,21);a.sign8(6,4);a.sign8(7,7);a.compare_registers(7,6);
        if(!a.less()){a.flags(0,0,64);a.sign16(0,0);}
        a.at(0x101175e4);a.offset(4,4,-1);a.sign8(6,4);a.compare(6,10);
        if(a.less())break;
        a.sign16(6,0,true);if(!a.equal())break;
    }
}
void scan_secondary_qualifications(Call& a) {
    a.at(0x101175fc);a.value(6,15);
    while(true) {
        a.at(0x10117600);a.load<1>(4,3,23);a.sign8(4,4);a.compare(4,15);
        if(!a.less()){a.flags(0,0,0x800);a.value(5,1);a.sign16(0,0);}
        a.at(0x1011761c);a.load<1>(4,3,24);a.sign8(4,4);a.compare(4,15);
        if(!a.less()){a.flags(0,0,0x80);a.value(5,1);a.sign16(0,0);}
        a.at(0x10117638);a.load<1>(4,3,25);a.sign8(4,4);a.compare(4,15);
        if(!a.less()){a.flags(0,0,0x200);a.value(5,1);a.sign16(0,0);}
        a.at(0x10117654);a.offset(6,6,-1);a.sign8(4,6);a.compare(4,10);
        if(a.less())break;
        a.sign8(4,5,true);if(!a.equal())break;
    }
    a.at(0x1011766c);a.move(3,0);
}
}
namespace cm::aot::profile_qualification {
void classify_profile_qualifications(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    detail::Call a(s,classify_profile_qualifications_entry,d,o);
    a.link();a.store(1,-4,31);a.compare(3,0,false);a.load(31,2,-0x550c);a.store(1,-8,30);a.value(5,0);a.load(30,2,-0x7ba8);a.store(1,8,0);a.value(0,0);a.frame(0x70);
    if(a.equal())detail::report_missing_profile(a);
    else {detail::scan_primary_qualifications(a);detail::scan_secondary_qualifications(a);}
    a.at(0x10117670);a.pop(0x70);a.load(31,1,-4);a.load(30,1,-8);a.finish();
}
}
