#include "awol_message_detail.hpp"

namespace cm::aot::awol {
namespace {
using namespace message_detail;

void personal_training_line(Call& a,std::uint32_t start,std::int32_t missing,std::int32_t present) {
    a.at(start);a.compare(24,-1);
    if(a.equal())format_player_line(a,start+8,29,17,missing,0x1c8,0x10008eb0);
    else {a.at(start+0x1c);a.move(3,29);a.offset(4,17,present);a.offset(5,1,0x38);a.offset(6,1,0x1c8);a.call(0x10727890);a.jump(0x10008eb0);}
}
void personal_training_text(Call& a) {
    a.at(0x10008cfc);a.compare(19,1);
    if(a.equal()){personal_training_line(a,0x10008d48,0xdf7,0xe60);return;}
    if(a.at_least()){personal_training_line(a,0x10008d7c,0xedc,0xf4a);return;}
    a.compare(19,0);
    if(a.at_least())personal_training_line(a,0x10008d14,0xd09,0xd78);
    else {a.jump(0x10008d7c);personal_training_line(a,0x10008d7c,0xedc,0xf4a);}
}
void general_training_action(Call& a,std::uint32_t start,std::int32_t token) {
    a.at(start);a.multiply(6,18,0x248);a.load(0,31,0);a.move(3,29);a.offset(4,17,token);a.add(5,0,6);a.offset(8,1,0x38);a.load<1>(5,5,0x23b);a.offset(7,6,0x221);a.add(7,0,7);a.offset(9,1,0x1c8);a.sign8(5,5);a.move(6,5);a.call(0x10727890);a.jump(0x10008eb0);
}
void general_training_text(Call& a) {
    a.at(0x10008db0);a.compare(24,-1);
    if(a.equal()) {
        a.at(0x10008e7c);a.multiply(6,18,0x248);a.load(0,31,0);a.move(3,29);a.offset(4,17,0x1228);a.add(5,0,6);a.offset(9,1,0x1c8);a.load<1>(5,5,0x23b);a.offset(8,6,0x221);a.add(8,0,8);a.value(7,100);a.sign8(5,5);a.move(6,5);a.call(0x10727890);return;
    }
    a.compare(19,1);
    if(a.equal()){general_training_action(a,0x10008e0c,0x108c);return;}
    if(a.at_least()){general_training_action(a,0x10008e44,0x115a);return;}
    a.compare(19,0);
    if(!a.at_least()){a.jump(0x10008e44);general_training_action(a,0x10008e44,0x115a);return;}
    a.at(0x10008dd0);a.multiply(6,18,0x248);a.load(0,31,0);a.move(3,29);a.offset(4,17,0xfcb);a.add(5,0,6);a.offset(9,1,0x38);a.load<1>(5,5,0x23b);a.offset(8,6,0x221);a.add(8,0,8);a.offset(10,1,0x1c8);a.sign8(5,5);a.value(7,100);a.move(6,5);a.call(0x10727890);a.jump(0x10008eb0);
}
void describe_training_full(Call& a) {
    a.at(0x10008b94);a.move(3,21);a.value(4,0);a.call(0x102cb570);
    copy_field_then_read(a,21,28,1);copy_field_then_read(a,21,27,2);copy_field_then_read(a,21,26,3);copy_field_then_read(a,21,25,4);copy_field_then_read(a,21,18,5,false);copy_field_then_read(a,21,19,6,false);
    a.move(3,21);a.value(4,7);a.call(0x102cb570);copy_field_then_read(a,21,24,8);copy_field_then_read(a,21,23,9);copy_field_then_read(a,21,22,10);
    a.move(0,3);a.move(3,27);a.move(21,0);a.move(4,26);a.move(5,25);a.move(9,28);a.offset(6,1,0x1c8);a.value(7,400);a.value(8,1);a.call(0x100cb590);a.compare(24,-1);
    if(!a.equal()){format_name(a,24,23,22,21,0x38,1);a.jump(0x10008cb4);}
    else {a.at(0x10008cac);a.value(0,0);a.store<1>(1,0x38,0);}
    if(selected_club_matches(a,0x10008cb4,16,18))personal_training_text(a);else general_training_text(a);
    a.at(0x10008eb0);a.move(3,20);a.move(4,29);a.call(0x1074b830);add_staff_link(a,0x10008ebc,16,28,30,27,26,25,20,0x1c8);
    a.at(0x10008efc);a.compare(24,-1);if(!a.equal())add_staff_link(a,0x10008f04,16,24,30,23,22,21,20,0x38);
    add_club_link(a,0x10008f44,31,18,20);a.jump(0x10009088);
}
void describe_training_brief(Call& a) {
    a.at(0x10008f64);a.move(3,21);a.value(4,0);a.call(0x102cb570);copy_field_then_read(a,21,19,1);copy_field_then_read(a,21,22,2);copy_field_then_read(a,21,23,3);copy_field_then_read(a,21,24,5);copy_field_then_read(a,21,18,6,false);a.move(16,3);format_name(a,19,22,23,24,0x1c8,5);a.compare(16,0);
    if(!a.equal())format_player_line(a,0x10008ffc,29,17,0x12c4,0x1c8,0x1000903c);
    else {
        a.at(0x10009010);a.compare(18,0);
        if(a.equal())format_player_line(a,0x10009018,29,17,0x12f9,0x1c8,0x1000903c);
        else {a.at(0x1000902c);a.move(3,29);a.offset(4,17,0x133a);a.offset(5,1,0x1c8);a.call(0x10727890);}
    }
    a.at(0x1000903c);a.move(3,20);a.move(4,29);a.call(0x1074b830);add_staff_link(a,0x10009048,16,19,30,22,23,24,20,0x1c8);
}
}

void describe_training_absence(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,training_text_entry,d,o);a.link();a.multiple(16);a.load(29,2,-0x7bb8);a.move(21,4);a.load(16,2,-0x7bb0);a.move(20,6);a.load(30,2,-0x7be0);a.load(31,2,-0x7be8);a.load(17,2,-0x5a08);a.save_link();a.sign8(0,5,true);a.frame(928);
    if(a.equal())describe_training_brief(a);else describe_training_full(a);
    a.at(0x10009088);a.pop(928);a.multiple(16,true);a.finish();
}
}
