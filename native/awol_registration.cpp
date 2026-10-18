#include "awol_event_random.hpp"

namespace cm::aot::awol {
namespace {
using namespace event_detail;
using Pool=RandomPool<22,23,24,25,26>;

bool eligible_registration(Call& a) {
    a.at(0x10006944);a.compare(29,0,false);if(a.equal())return false;
    a.load(0,29,0x14);a.compare(0,0,false);if(a.equal()){a.jump(0x100070ec);return false;}
    a.at(0x1000695c);a.compare(29,0,false);
    if(!a.equal()){a.load(3,2,-0x7be4);a.load(4,29,0);a.load(0,3,0);a.compare_registers(4,0);if(a.at_least())return false;}
    a.at(0x10006978);a.load(5,28,12);a.load(4,29,0);a.indexed_load<1>(3,5,4);a.mask(0,3,1,true);if(!a.equal())return false;
    a.flags(0,3,1);a.indexed_store<1>(0,5,4);return true;
}

void reduce_profile_metric(Call& a,std::uint32_t pool,std::int32_t metric,bool first=false) {
    Pool::advance(a,pool);
    const auto input=first?0x5c:0x4c,output=first?0x4c:0x5c;
    a.load(5,26,0);a.value(6,0x43300000);a.value(4,0x63e70000);a.load(3,29,0x14);a.load<2>(5,5,0);a.offset(0,4,0x63f);a.load(4,23,0);a.load<2>(7,3,metric,true);a.add(5,5,4);a.store(1,0x54,6);a.multiply_high(0,0,5);a.exclusive_flags(4,7,0x80000000);a.floating_load(4,22,8);a.floating_load(0,22,0x18);a.store(1,input+4,4);a.floating_load(2,22,0x10);a.shift_right(0,0,4);a.store(1,input,6);a.mask(4,0,1,false,1);a.add(0,0,4);a.floating_load(1,1,input);a.multiply(0,0,41);a.floating_subtract(3,1,4);a.subtract(0,0,5);a.exclusive_flags(0,0,0x80000000);a.store(1,0x58,0);a.floating_load(1,1,0x54);a.floating_subtract(1,1,4);a.floating_difference(0,1,0,true);a.floating_subtract(0,2,0);a.floating_product(0,3,0);a.floating_integer(0,0);a.floating_store(0,1,output);a.load(0,1,output+4);a.store<2>(3,metric,0);
}

void affect_peer_relationship(Call& a) {
    a.at(0x10006c0c);a.load(3,29,0x24);a.offset(0,21,0x98);a.indexed_load(19,3,0);a.compare(19,0,false);if(a.equal())return;
    a.compare_registers(19,29,false);if(a.equal())return;
    a.load<1>(17,19,0x65);a.sign8(17,17,true);Pool::draw_or_zero<17,16>(a,0x10006c3c);
    a.at(0x10006d84);a.compare(16,0);if(a.equal())return;
    a.load<1>(17,19,0x67);a.sign8(17,17,true);Pool::draw_or_zero<17,16>(a,0x10006da0);
    a.at(0x10006ee8);a.compare(16,0);if(!a.equal())return;
    a.move(3,19);a.move(4,29);a.value(5,1);a.call(0x100d9b80);
}

void affect_club_relationships(Call& a) {
    a.at(0x10006bb0);a.load(3,29,0x24);a.compare(3,0,false);if(a.equal())return;
    a.load(4,3,0x90);a.move(3,29);a.value(5,1);a.call(0x100d9b80);
    a.load(3,29,0x24);a.value(4,1);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
    if(a.equal()){a.load(3,29,0x24);a.move(4,29);a.value(5,1);a.load(3,3,0x90);a.call(0x100d9b80);}
    a.at(0x10006c00);a.value(18,0);a.value(21,0);a.flags(0,0,0);
    for(;;){affect_peer_relationship(a);a.at(0x10006f00);a.offset(18,18,1);a.offset(21,21,4);a.compare(18,50);if(!a.less())break;}
    a.load(3,29,0x24);a.move(4,29);a.value(5,1);a.load(3,3,0x80);a.call(0x100d9b80);
}

void append_absence_record(Call& a) {
    a.at(0x10006f24);a.sign8(0,31);a.load(5,29,0);a.load<1>(3,27,4);a.compare(0,4);a.load<2>(4,27,2);a.load<2>(0,27,0,true);a.store(1,0x40,5);a.store<2>(1,0x46,4);a.store<2>(1,0x44,0);a.store<1>(1,0x48,3);
    if(a.equal()) {
        Pool::advance(a,0x10006f50);a.load(5,26,0);a.value(3,0x84210000);a.load(4,23,0);a.offset(0,3,0x843);a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(0,0,4);a.add(0,0,4);a.shift_right(0,0,4);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,31);a.subtract(3,0,4);a.offset(0,3,2);a.sign16(16,0);a.jump(0x10007018);
    }else {
        Pool::advance(a,0x10006fb8);a.load(5,26,0);a.value(3,0x1b4f0000);a.load(4,23,0);a.offset(0,3,-0x7e4b);a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(0,0,4);a.shift_right(0,0,4);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,150);a.subtract(3,0,4);a.offset(0,3,21);a.sign16(16,0);
    }
    a.at(0x10007018);a.move(4,20);a.offset(3,1,0x38);a.sign16(5,16);a.call(0x10717890);a.load<1>(0,1,0x3c);a.offset(4,1,0x40);a.load<2>(6,1,0x3a);a.value(5,12);a.load<2>(3,1,0x38,true);a.store<2>(1,0x46,6);a.store<2>(1,0x44,3);a.store<1>(1,0x48,0);a.load(3,28,0);a.call(0x10738d10);
}

void choose_notice_club(Call& a,std::uint32_t start) {
    a.at(start);a.compare(30,0,false);a.move(3,28);a.move(4,29);
    if(!a.equal()){a.move(5,30);a.jump(start+0x1c);}else {a.at(start+0x18);a.load(5,29,0x24);}
    a.at(start+0x1c);
}
void notify_registration(Call& a) {
    a.at(0x10007050);a.sign8(0,31);a.compare(0,4);
    if(a.equal()){choose_notice_club(a,0x1000706c);a.move(7,16);a.value(6,4);a.call(notice_entry);a.jump(0x100070ec);return;}
    if(!a.at_least()) {
        a.compare(0,1);
        if(a.equal()){choose_notice_club(a,0x10007098);a.value(6,2);a.value(7,0xffffffff);a.call(notice_entry);a.jump(0x100070ec);return;}
        a.jump(0x100070c4);
    }
    choose_notice_club(a,0x100070c4);a.value(6,1);a.value(7,0xffffffff);a.call(notice_entry);
}
}

void register_primary_absence(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,registration_entry,d,o);a.link();a.multiple(16);a.load(22,2,-0x5a0c);a.move(28,3);a.load(23,2,-0x7b90);a.move(29,4);a.load(24,2,-0x7b8c);a.move(30,5);a.load(25,2,-0x7b88);a.move(31,6);a.load(26,2,-0x7b84);a.load(27,2,-0x7b80);a.save_link();a.frame(176);a.offset(20,1,0x44);a.move(3,20);a.call(0x10715880);
    if(eligible_registration(a)){reduce_profile_metric(a,0x10006994,10,true);reduce_profile_metric(a,0x10006a48,8);reduce_profile_metric(a,0x10006afc,12);affect_club_relationships(a);append_absence_record(a);notify_registration(a);}
    a.at(0x100070ec);a.pop(176);a.multiple(16,true);a.finish();
}
}
