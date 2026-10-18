#include "awol_message_detail.hpp"

namespace cm::aot::awol {
namespace {
using namespace message_detail;

void general_club_line(Call& a,std::uint32_t start,std::int32_t token) {
    a.at(start);a.multiply(6,30,0x248);a.load(0,28,0);a.move(3,31);a.offset(4,21,token);a.add(5,0,6);a.offset(8,1,0x40);a.load<1>(5,5,0x23b);a.offset(7,6,0x221);a.add(7,0,7);a.sign8(5,5);a.move(6,5);a.call(0x10727890);a.jump(0x10007760);
}
void general_nation_line(Call& a,std::uint32_t start,std::int32_t token) {
    a.at(start);a.multiply(6,20,0x128);a.load(8,16,0);a.load(11,28,0);a.move(3,31);a.offset(4,21,token);a.add(5,8,6);a.load<1>(7,5,0x116);a.multiply(9,30,0x248);a.offset(0,6,0xe1);a.sign8(7,7);a.add(5,11,9);a.add(0,8,0);a.load<1>(5,5,0x23b);a.offset(9,9,0x221);a.offset(10,1,0x40);a.move(8,7);a.sign8(5,5);a.store(1,0x38,0);a.move(6,5);a.add(9,11,9);a.call(0x10727890);a.jump(0x10007760);
}
void general_transfer_line(Call& a) {
    a.at(0x1000753c);a.multiply(4,27,0x248);a.load(10,28,0);a.move(3,31);a.offset(5,10,0x23b);a.multiply(6,30,0x248);a.indexed_load<1>(7,5,4);a.offset(0,4,0x221);a.sign8(7,7);a.indexed_load<1>(5,5,6);a.add(0,10,0);a.offset(9,6,0x221);a.move(8,7);a.sign8(5,5);a.store(1,0x38,0);a.add(9,10,9);a.offset(4,21,0x3b5);a.move(6,5);a.offset(10,1,0x40);a.call(0x10727890);a.jump(0x10007760);
}
void general_primary_text(Call& a) {
    a.at(0x10007420);a.compare(18,2);
    if(a.equal()){general_transfer_line(a);return;}
    if(a.at_least()) {
        a.at(0x1000743c);a.compare(18,4);if(!a.equal()){a.jump(0x10007760);return;}
        a.at(0x10007448);a.compare(19,7);
        if(!a.at_least()){general_nation_line(a,0x10007450,0x25);return;}
        a.at(0x100074a8);a.compare(20,-1);
        if(a.equal())general_club_line(a,0x10007508,0x2a3);else general_nation_line(a,0x100074b0,0x176);
        return;
    }
    a.compare(18,0);
    if(a.equal())general_club_line(a,0x100075c0,0x583);
    else if(a.at_least())general_club_line(a,0x1000758c,0x4aa);
    else a.jump(0x10007760);
}

void personal_return_line(Call& a) {
    a.at(0x1000761c);a.compare(19,7);
    if(!a.at_least()) {
        a.multiply(6,30,0x248);a.load(8,28,0);a.load(0,16,0);a.move(3,31);a.offset(4,21,0x678);a.add(5,8,6);a.load<1>(7,5,0x23b);a.multiply(10,20,0x128);a.offset(6,6,0x221);a.sign8(7,7);a.add(5,0,10);a.add(6,8,6);a.load<1>(5,5,0x116);a.offset(10,10,0xe1);a.offset(9,1,0x40);a.move(8,7);a.sign8(5,5);a.store(1,0x38,6);a.move(6,5);a.add(10,0,10);a.call(0x10727890);a.jump(0x10007760);return;
    }
    a.at(0x1000767c);a.compare(20,-1);
    if(a.equal())format_player_line(a,0x100076b8,31,21,0x890,0x40,0x10007760);
    else {a.multiply(6,20,0x128);a.load(0,16,0);a.move(3,31);a.offset(4,21,0x79c);a.add(5,0,6);a.offset(7,1,0x40);a.load<1>(5,5,0x116);a.offset(8,6,0xe1);a.add(8,0,8);a.sign8(5,5);a.move(6,5);a.call(0x10727890);a.jump(0x10007760);}
}
void personal_transfer_line(Call& a) {
    a.at(0x100076cc);a.multiply(4,30,0x248);a.load(11,28,0);a.move(3,31);a.offset(5,11,0x23b);a.multiply(6,27,0x248);a.indexed_load<1>(7,5,4);a.offset(0,4,0x221);a.sign8(7,7);a.indexed_load<1>(5,5,6);a.add(0,11,0);a.offset(10,6,0x221);a.move(8,7);a.sign8(5,5);a.store(1,0x38,0);a.move(6,5);a.offset(4,21,0x968);a.offset(9,1,0x40);a.add(10,11,10);a.call(0x10727890);a.jump(0x10007760);
}
void personal_primary_text(Call& a) {
    a.at(0x100075f4);a.compare(18,2);
    if(a.equal()){personal_transfer_line(a);return;}
    if(a.at_least()){a.at(0x10007610);a.compare(18,4);if(a.equal())personal_return_line(a);else a.jump(0x10007760);return;}
    a.compare(18,0);
    if(a.equal()){a.at(0x10007750);a.move(3,31);a.offset(4,21,0xb1f);a.offset(5,1,0x40);a.call(0x10727890);}
    else if(a.at_least()) {
        a.at(0x1000771c);a.multiply(6,30,0x248);a.load(0,28,0);a.move(3,31);a.offset(4,21,0xa51);a.add(5,0,6);a.offset(7,1,0x40);a.load<1>(5,5,0x23b);a.offset(8,6,0x221);a.add(8,0,8);a.sign8(5,5);a.move(6,5);a.call(0x10727890);a.jump(0x10007760);
    }else a.jump(0x10007760);
}
void describe_primary_full(Call& a) {
    a.at(0x10007318);a.move(3,20);a.value(4,0);a.call(0x102cb570);copy_field_then_read(a,20,23,1);copy_field_then_read(a,20,24,2);copy_field_then_read(a,20,25,3);copy_field_then_read(a,20,26,4);copy_field_then_read(a,20,27,5,false);copy_field_then_read(a,20,30,6,false);copy_field_then_read(a,20,18,7,false);copy_field_then_read(a,20,19,8,false);a.move(20,3);format_name(a,23,24,25,26,0x40,1);
    if(selected_club_matches(a,0x100073d8,17,30))personal_primary_text(a);else general_primary_text(a);
    a.at(0x10007760);a.move(3,29);a.move(4,31);a.call(0x1074b830);add_staff_link(a,0x1000776c,16,23,22,24,25,26,29,0x40);
    a.at(0x100077ac);a.compare(27,-1);if(!a.nonpositive())add_club_link(a,0x100077b4,28,27,29);
    a.at(0x100077d0);a.compare(30,-1);if(!a.nonpositive()){add_club_link(a,0x100077d8,28,30,29);a.jump(0x10007a38);}
}

void brief_transfer_line(Call& a) {
    a.at(0x100078dc);a.value(3,0x55550000);a.offset(0,3,0x5556);a.multiply_high(3,0,16);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);a.subtract(0,0,16);a.compare(0,1);
    if(a.equal()){format_player_line(a,0x10007924,31,21,0xc57,0x40,0x100079c8);return;}
    if(!a.at_least()){a.compare(0,0);if(a.at_least()){format_player_line(a,0x10007910,31,21,0xc24,0x40,0x100079c8);return;}a.jump(0x10007938);}
    format_player_line(a,0x10007938,31,21,0xc8a,0x40,0x100079c8);
}
void brief_missing_line(Call& a) {
    a.at(0x1000794c);a.value(3,0x55550000);a.offset(0,3,0x5556);a.multiply_high(3,0,16);a.mask(0,3,1,false,1);a.add(0,3,0);a.multiply(0,0,3);a.subtract(0,0,16,true);
    if(a.equal())format_player_line(a,0x10007970,31,21,0xc57,0x40,0x100079c8);
    else {a.jump(0x10007984);format_player_line(a,0x10007984,31,21,0xc8a,0x40,0x100079c8);}
}
void brief_primary_text(Call& a) {
    a.at(0x100078a0);a.compare(20,2);
    if(a.equal()){brief_transfer_line(a);return;}
    if(a.at_least()){a.at(0x100078bc);a.compare(20,4);if(a.equal())format_player_line(a,0x100078c8,31,21,0xbe6,0x40,0x100079c8);else a.jump(0x100079c8);return;}
    a.compare(20,0);
    if(a.equal()) {
        a.at(0x10007998);a.multiply(6,23,0x248);a.load(0,28,0);a.move(3,31);a.offset(4,21,0xcb7);a.add(5,0,6);a.offset(7,1,0x40);a.load<1>(5,5,0x23b);a.offset(8,6,0x221);a.add(8,0,8);a.sign8(5,5);a.move(6,5);a.call(0x10727890);
    }else if(a.at_least())brief_missing_line(a);else a.jump(0x100079c8);
}
void describe_primary_brief(Call& a) {
    a.at(0x100077f8);a.move(3,20);a.value(4,0);a.call(0x102cb570);copy_field_then_read(a,20,16,1);copy_field_then_read(a,20,17,2);copy_field_then_read(a,20,18,3);copy_field_then_read(a,20,19,4);
    a.move(3,20);a.value(4,5);a.call(0x102cb570);copy_field_then_read(a,20,23,6,false);a.move(3,20);a.value(4,6);a.call(0x102cb570);a.move(20,3);format_name(a,16,17,18,19,0x40,5);
    brief_primary_text(a);a.at(0x100079c8);a.move(3,29);a.move(4,31);a.call(0x1074b830);add_staff_link(a,0x100079d4,16,16,22,17,18,19,29,0x40);
    a.at(0x10007a14);a.compare(23,-1);if(!a.nonpositive())add_club_link(a,0x10007a1c,28,23,29);
}
}

void describe_primary_absence(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,primary_text_entry,d,o);a.link();a.multiple(16);a.load(31,2,-0x7bb8);a.move(20,4);a.load(16,2,-0x7b98);a.move(29,6);a.load(17,2,-0x7bb0);a.load(22,2,-0x7be0);a.load(28,2,-0x7be8);a.load(21,2,-0x5a08);a.save_link();a.sign8(0,5,true);a.frame(528);
    if(a.equal())describe_primary_brief(a);else describe_primary_full(a);
    a.at(0x10007a38);a.pop(528);a.multiple(16,true);a.finish();
}
}
