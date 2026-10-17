#include "injury_progression.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::injury_progression {
namespace {
using Call=SemanticGuestCall;

void review_active_injury(Call& a) {
    a.at(0x1018b318);bool valid=a.equal();
    if(!valid) {a.load(3,26,0);a.load(0,29,0);a.compare_registers(3,0);valid=!a.at_least();}
    if(!valid)return;
    a.at(0x1018b32c);a.load(0,26,0);a.load(4,31,0);a.multiply(3,0,0x24);a.offset(0,3,0x10);a.indexed_load(0,4,0);a.compare(0,-1);
    if(a.equal())return;
    a.multiply(4,0,0x38);a.load(5,31,4);a.load(6,30,0);a.offset(3,1,0x50);a.load(0,30,4);
    a.add(21,5,4);a.load<2>(5,21,0x32,true);a.offset(4,21,0x28);a.store(1,0x40,6);a.store(1,0x44,0);a.call(0x10717890);
    a.load<2>(3,1,0x42);a.value(4,0);a.load<2>(0,1,0x52);a.compare_registers(3,0,false);
    if(a.equal()) {
        a.load<2>(3,1,0x40,true);a.load<2>(0,1,0x50,true);a.compare_registers(3,0);
        if(a.equal())a.value(4,1);
    }
    a.at(0x1018b39c);a.compare(4,0);
    if(!a.equal()) {
        a.load<2>(0,21,0x32,true);a.compare(0,5);
        if(!a.nonpositive()) {
            a.load(0,26,0);a.load(4,31,0);a.multiply(3,0,0x24);a.offset(5,3,0x20);a.indexed_load<1>(0,4,5);a.sign8(0,0,true);
            if(a.equal()) {
                a.value(0,1);a.move(3,31);a.indexed_store<1>(0,4,5);a.move(4,26);a.value(5,0);a.value(7,0);
                a.load<2>(8,21,0x32,true);a.load<2>(0,21,0x30,true);a.load<1>(6,21,0);a.subtract(0,8,0);a.sign16(8,0);a.call(notice_entry);
            }
        }
    }
    a.at(0x1018b3fc);a.move(3,31);a.move(4,26);a.call(0x10187a40);
}

void review_recent_clubs(Call& a) {
    for(;;) {
        a.at(0x1018b2d8);a.load(0,29,0);a.load(4,28,0);a.subtract(3,24,0);a.offset(0,3,-1);a.multiply(3,0,0x70);a.offset(0,3,0x24);
        a.indexed_load(23,4,0);a.compare(23,0,false);
        if(!a.equal()) {
            a.value(22,0);a.value(27,0);a.jump(0x1018b410);
            for(;;) {
                a.at(0x1018b410);a.sign8(0,22);a.compare(0,50);if(!a.less())break;
                a.at(0x1018b308);a.offset(0,27,0x98);a.indexed_load(26,23,0);a.compare(26,0,false);
                if(!a.equal())review_active_injury(a);
                a.at(0x1018b408);a.offset(27,27,4);a.offset(22,22,1);
            }
        }
        a.at(0x1018b41c);a.offset(24,24,1);a.compare(24,16);if(!a.less())break;
    }
}

void locate_first_equal(Call& a,std::uint32_t previous,std::uint32_t check,std::uint32_t forward,std::uint32_t comparator) {
    a.jump(check);
    for(;;) {
        a.at(check);a.load(0,31,0x10);a.compare_registers(21,0,false);
        if(!a.positive()) {a.jump(forward);return;}
        a.at(previous);a.offset(21,21,-24);a.offset(3,1,0x58);a.move(4,21);a.call(comparator);a.compare(3,0);
        if(a.equal())continue;
        a.offset(21,21,24);a.jump(forward);return;
    }
}

void start_queued_injuries(Call& a) {
    a.at(0x1018b434);a.offset(3,1,0x58);a.call(0x10715880);a.offset(3,1,0x60);a.call(0x10715880);
    a.load<1>(0,30,4);a.offset(3,1,0x58);a.load<2>(5,30,2);a.value(6,24);a.load<2>(4,30,0,true);
    a.store<2>(1,0x5a,5);a.load(7,2,-0x5458);a.store<2>(1,0x58,4);a.store<1>(1,0x5c,0);a.store<2>(1,0x62,5);
    a.store<2>(1,0x60,4);a.store<1>(1,0x64,0);a.load(4,31,0x10);a.load(5,31,0x18);a.call(0x10775600);a.move(21,3,true);
    if(a.equal())return;
    locate_first_equal(a,0x1018b48c,0x1018b4ac,0x1018b524,start_compare_entry);
    for(;;) {
        a.at(0x1018b524);a.load(3,31,0x18);a.load(4,31,0x10);a.offset(0,3,-1);a.multiply(0,0,24);a.add(0,4,0);a.compare_registers(21,0,false);
        if(!a.nonpositive())return;
        a.at(0x1018b4bc);a.move(4,21);a.offset(3,1,0x58);a.call(start_compare_entry);a.compare(3,0);if(!a.equal())return;
        a.load(0,21,0x10);a.offset(3,1,0x48);a.load(7,28,0);a.offset(4,21,8);a.multiply(0,0,0x70);
        a.load(5,21,0);a.load(6,21,4);a.add(22,7,0);a.call(0x10717dd0);a.load<2>(0,1,0x4a);a.move(3,31);
        a.load<2>(8,1,0x48,true);a.move(4,22);a.multiply(0,0,365);a.load<1>(6,21,0x15);a.load<1>(7,21,0x14);a.value(5,0);
        a.add(0,8,0);a.sign16(8,0);a.call(notice_entry);a.offset(21,21,24);
    }
}

void end_queued_injuries(Call& a) {
    a.at(0x1018b540);a.load(4,31,0x10);a.offset(3,1,0x58);a.load(5,31,0x18);a.value(6,24);a.load(7,2,-0x5454);a.call(0x10775600);a.move(21,3,true);
    if(a.equal())return;
    a.value(25,1);locate_first_equal(a,0x1018b568,0x1018b588,0x1018b5d8,end_compare_entry);
    for(;;) {
        a.at(0x1018b5d8);a.load(3,31,0x18);a.load(4,31,0x10);a.offset(0,3,-1);a.multiply(0,0,24);a.add(0,4,0);a.compare_registers(21,0,false);
        if(!a.nonpositive())return;
        a.at(0x1018b598);a.move(4,21);a.offset(3,1,0x58);a.call(end_compare_entry);a.compare(3,0);if(!a.equal())return;
        a.load(0,21,0x10);a.move(3,31);a.load(4,28,0);a.value(5,1);a.multiply(0,0,0x70);a.load<1>(6,21,0x15);a.load<1>(7,21,0x14);
        a.value(8,0);a.add(4,4,0);a.call(notice_entry);a.offset(21,21,24);
    }
}

void compact_expired_queue(Call& a) {
    a.at(0x1018b5f4);a.sign8(0,25,true);if(a.equal())return;
    a.value(21,0);a.value(8,0);a.value(6,0);a.jump(0x1018b680);
    for(;;) {
        a.at(0x1018b680);a.load(7,31,0x18);a.compare_registers(8,7);if(!a.less())break;
        a.at(0x1018b60c);a.load(3,30,0);a.offset(5,6,8);a.load(0,31,0x10);a.store(1,0x38,3);a.add(5,0,5);a.load(0,30,4);
        a.load<2>(4,1,0x3a);a.load<2>(3,5,2);a.store(1,0x3c,0);a.compare_registers(4,3,false);
        if(!a.nonpositive()) {a.value(0,1);a.jump(0x1018b66c);}
        else {
            a.at(0x1018b640);
            if(!a.at_least()) {a.value(0,0);a.jump(0x1018b66c);}
            else {a.at(0x1018b64c);a.load<2>(4,1,0x38,true);a.load<2>(0,5,0,true);a.combine(0,4,0,false,true);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);}
        }
        a.at(0x1018b66c);a.sign8(0,0,true);if(a.equal())break;
        a.offset(21,21,1);a.offset(6,6,24);a.offset(8,8,1);
    }
    a.at(0x1018b68c);a.sign16(0,21,true);
    if(!a.equal()) {
        a.sign16(3,21);a.compare_registers(3,7);
        if(!a.equal()) {
            a.subtract(0,3,7);a.load(4,31,0x10);a.multiply(3,3,24);a.multiply(5,0,24);a.add(3,4,3);a.call(0x1077fc28);a.load(2,1,0x14);
        }
    }
    a.at(0x1018b6bc);a.load(0,31,0x18);a.sign16(3,21);a.subtract(0,3,0);a.store(31,0x18,0);
}
}

void process_daily_injuries(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,daily_entry,dependencies,observer);
    a.link();a.multiple(21);a.load(28,2,-0x7be0);a.move(31,3);a.load(29,2,-0x7be4);a.value(25,0);a.load(30,2,-0x7b80);a.value(24,0);a.save_link();a.frame(160);
    review_recent_clubs(a);
    a.at(0x1018b428);a.load(0,31,0x10);a.compare(0,0,false);
    if(!a.equal()) {start_queued_injuries(a);end_queued_injuries(a);}
    compact_expired_queue(a);
    a.at(0x1018b6cc);a.pop(160);a.multiple(21,true);a.finish();
}
}
