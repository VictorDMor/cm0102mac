#include "awol_message_detail.hpp"

namespace cm::aot::awol {
namespace {
using namespace message_detail;

void describe_fitness_brief(Call& a) {
    a.at(0x10009c4c);format_name(a,29,28,27,26,0x420,5);a.compare(20,0);
    if(!a.equal()){a.offset(3,1,0x808);a.offset(4,18,0x136e);a.offset(6,1,0x420);a.value(5,100);a.call(0x10727890);a.jump(0x10009c9c);}
    else {a.at(0x10009c8c);a.offset(3,1,0x808);a.offset(4,18,0x13a1);a.offset(5,1,0x420);a.call(0x10727890);}
    a.at(0x10009c9c);a.move(3,21);a.offset(4,1,0x808);a.call(0x1074b830);
    if(add_staff_link(a,0x10009ca8,18,29,30,28,27,26,21,0x420))a.jump(0x10009ed4);
}

void describe_fitness_full(Call& a) {
    a.at(0x10009cec);format_name(a,29,28,27,26,0x420,1);a.compare(25,-1);
    if(!a.nonpositive()) {
        format_name(a,25,24,23,22,0x38,1);a.compare(20,0);
        if(!a.equal()) {
            a.multiply(6,19,0x248);a.load(0,31,0);a.offset(3,1,0x808);a.offset(4,18,0x13d4);a.add(5,0,6);a.offset(9,1,0x38);a.load<1>(5,5,0x23b);a.offset(8,6,0x221);a.add(8,0,8);a.offset(10,1,0x420);a.sign8(5,5);a.value(7,100);a.move(6,5);a.call(0x10727890);a.jump(0x10009e24);
        }else {
            a.at(0x10009d78);a.multiply(6,19,0x248);a.load(0,31,0);a.offset(3,1,0x808);a.offset(4,18,0x14a6);a.add(5,0,6);a.offset(8,1,0x38);a.load<1>(5,5,0x23b);a.offset(7,6,0x221);a.add(7,0,7);a.offset(9,1,0x420);a.sign8(5,5);a.move(6,5);a.call(0x10727890);a.jump(0x10009e24);
        }
    }else {
        a.at(0x10009db0);a.compare(20,0);
        if(!a.equal()) {
            a.multiply(6,19,0x248);a.load(0,31,0);a.offset(3,1,0x808);a.offset(4,18,0x157f);a.add(5,0,6);a.offset(9,1,0x420);a.load<1>(5,5,0x23b);a.offset(8,6,0x221);a.add(8,0,8);a.value(7,100);a.sign8(5,5);a.move(6,5);a.call(0x10727890);a.jump(0x10009e24);
        }else {
            a.at(0x10009df0);a.multiply(7,19,0x248);a.load(0,31,0);a.offset(3,1,0x808);a.offset(4,18,0x164d);a.add(5,0,7);a.offset(8,1,0x420);a.load<1>(6,5,0x23b);a.offset(9,7,0x221);a.add(9,0,9);a.value(5,100);a.sign8(6,6);a.move(7,6);a.call(0x10727890);
        }
    }
    a.at(0x10009e24);a.move(3,21);a.offset(4,1,0x808);a.call(0x1074b830);
    add_staff_link(a,0x10009e30,18,29,30,28,27,26,21,0x420);
    add_club_link(a,0x10009e70,31,19,21,0x221);a.compare(25,-1);
    if(!a.nonpositive())add_staff_link(a,0x10009e94,18,25,30,24,23,22,21,0x38);
}
}

void describe_fitness_concern(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,fitness_text_entry,d,o);a.link();a.multiple(18);a.move(23,4);a.load(30,2,-0x7be0);a.load(31,2,-0x7be8);a.move(22,5);a.load(18,2,-0x5a08);a.move(21,6);a.move(3,23);a.value(4,0);a.save_link();a.frame(7120);a.call(0x102cb570);
    copy_field_then_read(a,23,29,1);copy_field_then_read(a,23,19,2,false);copy_field_then_read(a,23,20,3,false);copy_field_then_read(a,23,28,4);copy_field_then_read(a,23,27,5);copy_field_then_read(a,23,26,6);copy_field_then_read(a,23,25,7);copy_field_then_read(a,23,24,8);copy_field_then_read(a,23,23,9);a.sign8(0,22,true);a.move(22,3);
    if(a.equal())describe_fitness_brief(a);else describe_fitness_full(a);
    a.at(0x10009ed4);a.pop(7120);a.multiple(18,true);a.finish();
}
}
