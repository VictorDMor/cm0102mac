#include "injury_reporting.hpp"
#include "injury_reporting_detail.hpp"
#include "injury_readouts.hpp"

namespace cm::aot::injury_reporting {
namespace {
using namespace detail;
void person_description(Call& a,std::uint32_t start,std::int32_t buffer,unsigned form) {
    a.at(start);a.move(3,28);a.move(4,27);a.move(5,26);a.move(9,29);a.offset(6,1,buffer);a.value(7,0x99);a.value(8,form);a.call(0x100cb590);
}
void format_notice(Call& a,std::uint32_t start,std::int32_t token,bool other_name=false,bool diagnostic=false) {
    a.at(start);a.move(3,30);a.offset(4,31,token);a.offset(5,1,0x178);if(other_name)a.offset(6,1,0xdc);a.call(diagnostic?0x1077b720:0x10727890);
}
void append_person_link(Call& a,std::uint32_t start,bool jump) {
    a.at(start);a.multiply(16,29,0x70);a.load(0,24,0);a.move(4,28);a.move(5,27);a.move(6,26);a.offset(7,1,0x38);a.add(3,0,16);a.call(0x100cbde0);a.sign8(0,3,true);
    if(a.equal())return;
    a.load(0,24,0);a.move(3,25);a.offset(4,1,0x178);a.value(5,0x7d7);a.add(6,0,16);a.call(0x1074bab0);if(jump)a.jump(0x1018fbc4);
}
void general_description(Call& a,std::uint32_t start,std::int32_t buffer,std::int32_t token) {
    a.at(start);a.compare(3,6);
    if(!a.equal()) {a.move(3,22);a.offset(5,1,buffer);a.value(6,0x32);a.call(injury_readouts::category_text_entry);a.jump(start+0x40);}
    else {a.at(start+0x1c);a.move(3,22);a.move(4,18);a.offset(5,1,buffer);a.value(6,0x32);a.value(7,0);a.call(injury_readouts::type_text_entry);a.move(3,22);a.move(4,18);a.call(injury_readouts::type_form_entry);}
    a.at(start+0x40);a.sign8(5,3);a.move(3,30);a.offset(4,31,token);a.offset(7,1,0x178);a.move(6,5);a.offset(8,1,buffer);a.call(0x10727890);
}
void initial_special_text(Call& a) {
    a.at(0x1018f638);a.compare(18,81);
    if(a.equal()){format_notice(a,0x1018f640,0xa1b);a.jump(0x1018f7a4);return;}
    a.at(0x1018f654);a.compare(18,80);
    if(a.equal()){format_notice(a,0x1018f65c,0xa83);a.jump(0x1018f7a4);return;}
    a.at(0x1018f670);a.compare(18,77);
    if(a.equal()){format_notice(a,0x1018f678,0xb7e);a.jump(0x1018f7a4);return;}
    a.at(0x1018f68c);a.compare(18,78);
    if(a.equal()){format_notice(a,0x1018f694,0xbf4);a.jump(0x1018f7a4);return;}
    a.at(0x1018f6a8);a.compare(18,79);
    if(a.equal()){format_notice(a,0x1018f6b0,0xc79);a.jump(0x1018f7a4);return;}
    format_notice(a,0x1018f6c4,0xd2e,false,true);a.jump(0x1018f7a4);
}
void initial_ordinary_text(Call& a) {
    a.at(0x1018f6e0);a.sign8(0,18,true);
    if(a.equal()){format_notice(a,0x1018f6e8,0xd91);a.jump(0x1018f7a4);return;}
    a.at(0x1018f6fc);a.compare(18,1);
    if(a.equal()){format_notice(a,0x1018f704,0xde5);a.jump(0x1018f7a4);return;}
    a.at(0x1018f718);a.compare(18,2);
    if(a.equal()){format_notice(a,0x1018f720,0xe40);a.jump(0x1018f7a4);return;}
    format_notice(a,0x1018f734,0xe9f);a.jump(0x1018f7a4);
}
void progress_special_text(Call& a) {
    a.at(0x1018f928);a.compare(18,81);
    if(a.equal()) {
        a.compare(21,2);if(a.equal()){format_notice(a,0x1018f938,0x1057);a.jump(0x1018fb78);return;}
        a.at(0x1018f94c);a.compare(21,3);if(a.equal()){format_notice(a,0x1018f954,0x110a);a.jump(0x1018fb78);return;}
        format_notice(a,0x1018f968,0xa1b);a.jump(0x1018fb78);return;
    }
    a.at(0x1018f97c);a.compare(18,77);
    if(a.equal()) {
        a.compare(21,-1);if(a.equal()){format_notice(a,0x1018f98c,0x115e);a.jump(0x1018fb78);return;}
        a.at(0x1018f9a0);a.compare(21,1);if(a.equal()){format_notice(a,0x1018f9a8,0x1277);a.jump(0x1018fb78);return;}
        a.at(0x1018f9bc);a.compare(20,0);if(a.equal()){format_notice(a,0x1018f9c4,0x139e);a.jump(0x1018fb78);return;}
        format_notice(a,0x1018f9d8,0x1467);a.jump(0x1018fb78);return;
    }
    a.at(0x1018f9ec);a.compare(18,80);if(a.equal()){format_notice(a,0x1018f9f4,0x1522);a.jump(0x1018fb78);return;}
    a.at(0x1018fa08);a.compare(18,78);
    if(a.equal()) {
        a.compare(20,0);if(a.equal()){format_notice(a,0x1018fa18,0x1617);a.jump(0x1018fb78);return;}
        format_notice(a,0x1018fa2c,0x1696,true);a.jump(0x1018fb78);return;
    }
    a.at(0x1018fa44);a.compare(18,79);
    if(a.equal()) {
        a.compare(20,2);
        if(a.equal()) {
            a.compare(21,1);if(a.equal()){format_notice(a,0x1018fa5c,0x1796);a.jump(0x1018fb78);return;}
            format_notice(a,0x1018fa70,0x1866);a.jump(0x1018fb78);return;
        }
        format_notice(a,0x1018fa84,0x18cc);a.jump(0x1018fb78);return;
    }
    format_notice(a,0x1018fa98,0x1994);a.jump(0x1018fb78);
}
void progress_ordinary_text(Call& a) {
    a.at(0x1018fab4);a.sign8(0,18,true);
    if(a.equal()){format_notice(a,0x1018fabc,0x1a18);a.jump(0x1018fb78);return;}
    a.at(0x1018fad0);a.compare(18,1);if(a.equal()){format_notice(a,0x1018fad8,0x1a68);a.jump(0x1018fb78);return;}
    a.at(0x1018faec);a.compare(18,2);if(a.equal()){format_notice(a,0x1018faf4,0x1abf);a.jump(0x1018fb78);return;}
    format_notice(a,0x1018fb08,0x1b1a);a.jump(0x1018fb78);
}
}

void compose_injury_notice(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,notice_text_entry,d,o);
    a.link();a.multiple(16);a.move(16,4);a.move(22,3);a.load(30,2,-0x7bb8);a.move(17,5);a.load(19,2,-0x64d4);a.move(25,6);a.load(24,2,-0x7be0);a.move(3,16);a.load(31,2,-0x5438);a.value(4,0);a.save_link();a.frame(608);a.call(0x102cb570);a.sign8(21,3);
    a.move(3,16);a.value(4,1);a.call(0x102cb570);a.sign8(18,3);a.move(3,16);a.value(4,2);a.call(0x102cb570);
    for(auto [destination,argument]:{std::pair{29u,3u},std::pair{28u,4u},std::pair{27u,5u},std::pair{26u,7u}}){a.move(0,3);a.move(3,16);a.move(destination,0);a.value(4,argument);a.call(0x102cb570);}
    a.mask(20,3,0xffff);a.move(3,16);a.value(4,6);a.call(0x102cb570);a.mask(0,3,0xffff);a.move(5,20);a.sign16(4,0);a.offset(3,1,0x38);a.call(0x107156b0);
    a.move(3,16);a.value(4,8);a.call(0x102cb570);a.sign16(23,3);a.move(3,16);a.value(4,9);a.call(0x102cb570);a.move(20,3);a.move(3,16);a.value(4,10);a.call(0x102cb570);a.sign8(0,21,true);a.move(21,3);
    if(a.equal()) {
        a.sign8(0,17,true);
        if(a.equal()) {
            person_description(a,0x1018f580,0x178,5);format_notice(a,0x1018f5a0,0x9e0);a.move(3,25);a.move(4,30);a.call(0x1074b830);append_person_link(a,0x1018f5bc,true);
        } else {
            person_description(a,0x1018f600,0x178,1);a.multiply(0,18,12);a.add(3,19,0);a.load<1>(4,3,1);a.sign8(3,4);a.compare(3,12);
            if(a.equal())initial_special_text(a);
            else {a.at(0x1018f6d8);a.sign8(0,4,true);if(a.equal())initial_ordinary_text(a);else general_description(a,0x1018f748,0xa8,0xf05);}
            a.at(0x1018f7a4);a.move(3,25);a.move(4,30);a.call(0x1074b830);a.move(3,23);a.offset(4,1,0x74);a.value(5,0x32);a.value(6,1);a.call(0x1030e880);
            a.move(3,30);a.offset(4,31,0xf7c);a.offset(5,1,0x74);a.call(0x10727890);a.move(3,25);a.move(4,30);a.call(0x1074b960);append_person_link(a,0x1018f7e0,true);
        }
    } else {
        a.at(0x1018f824);a.sign8(0,17,true);
        if(a.equal()) {
            person_description(a,0x1018f82c,0x178,5);a.compare(18,81);bool special=false;if(a.equal()){a.compare(21,3);special=a.equal();}
            if(special){format_notice(a,0x1018f85c,0xfd2);a.jump(0x1018f880);}else format_notice(a,0x1018f870,0x101d);
            a.at(0x1018f880);a.move(3,25);a.move(4,30);a.call(0x1074b830);append_person_link(a,0x1018f88c,true);
        } else {
            person_description(a,0x1018f8d0,0x178,1);person_description(a,0x1018f8f0,0xdc,5);a.multiply(0,18,12);a.add(3,19,0);a.load<1>(4,3,1);a.sign8(3,4);a.compare(3,12);
            if(a.equal())progress_special_text(a);
            else {a.at(0x1018faac);a.sign8(0,4,true);if(a.equal())progress_ordinary_text(a);else general_description(a,0x1018fb1c,0x40,0x1b7c);}
            a.at(0x1018fb78);a.move(3,25);a.move(4,30);a.call(0x1074b830);append_person_link(a,0x1018fb84,false);
        }
    }
    a.at(0x1018fbc4);a.pop(608);a.multiple(16,true);a.finish();
}
}
