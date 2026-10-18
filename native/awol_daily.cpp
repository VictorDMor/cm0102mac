#include "awol_daily_detail.hpp"

namespace cm::aot::awol {
namespace {
using namespace daily_detail;

bool review_primary_records(Call& a) {
    a.at(0x100040a8);a.load(3,3,0);a.compare(3,0,false);
    if(a.equal())return true;
    a.load(0,3,4);a.value(16,0);a.store(3,12,0);a.jump(0x10004144);
    for(;;) {
        a.at(0x10004144);a.load(3,20,0);a.load<2>(0,3,0,true);a.compare_registers(16,0);if(!a.less())break;
        a.at(0x100040c8);a.call(0x10738e00);a.move(4,3,true);if(a.equal())return false;
        a.load(3,19,0);a.load(0,19,4);a.store(1,0x48,3);a.load<2>(3,4,6);a.load<2>(5,1,0x4a);a.store(1,0x4c,0);a.compare_registers(5,3,false);
        if(!a.nonpositive()){a.value(0,1);a.jump(0x10004128);}
        else {
            a.at(0x100040fc);
            if(!a.at_least()){a.value(0,0);a.jump(0x10004128);}
            else {a.at(0x10004108);a.load<2>(5,1,0x48,true);a.load<2>(0,4,4,true);a.combine(0,5,0,false,true);a.shift_right(3,0,1);a.intersect(0,0,5);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);}
        }
        a.at(0x10004128);a.sign8(0,0,true);
        if(!a.equal()){a.move(3,20);a.call(remove_primary_item_entry);a.jump(0x10004154);return true;}
        a.at(0x10004140);a.offset(16,16,1);
    }
    return true;
}

bool past_adjusted_date(Call& a,std::uint32_t start,std::int32_t current,std::int32_t adjusted) {
    a.at(start);a.load<2>(3,1,current+2);a.load<2>(0,1,adjusted+2);a.compare_registers(3,0,false);
    if(!a.nonpositive()){a.value(0,1);a.jump(start+0x44);}
    else {
        a.at(start+0x18);
        if(!a.at_least()){a.value(0,0);a.jump(start+0x44);}
        else {a.at(start+0x24);a.load<2>(4,1,current,true);a.load<2>(0,1,adjusted,true);a.combine(0,4,0,false,true);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);}
    }
    a.at(start+0x44);a.sign8(0,0,true);return !a.equal();
}

bool review_secondary_records(Call& a) {
    a.at(0x10004154);a.load(3,20,4);a.compare(3,0,false);if(a.equal())return true;
    a.load(0,3,4);a.value(16,0);a.store(3,12,0);a.jump(0x100054ac);
    for(;;) {
        a.at(0x100054ac);a.load(3,20,4);a.load<2>(0,3,0,true);a.compare_registers(16,0);if(!a.less())break;
        a.at(0x10004170);a.call(0x10738e00);a.move(21,3,true);if(a.equal())return false;
        a.load(4,19,0);a.offset(3,1,0x58);a.load(0,19,4);a.value(5,1);a.store(1,0x40,4);a.offset(4,21,4);a.store(1,0x44,0);a.call(0x10717a10);
        if(past_adjusted_date(a,0x1000419c,0x40,0x58)) {
            a.at(0x100041e8);a.load<1>(0,21,12);a.sign8(0,0,true);
            if(!a.equal())return_from_leave(a);else return_from_unapproved_absence(a);
            return true;
        }
        a.at(0x100054a8);a.offset(16,16,1);
    }
    return true;
}

// A successful pending record removal ends the entire daily review. Failed
// recovery checks keep scanning the remaining records with their live count.
bool recover_pending_leave(Call& a) {
    a.at(0x1000555c);a.load(0,26,0);a.load(3,18,0);a.multiply(0,0,0x70);a.add(24,3,0);a.load(0,24,0x14);a.compare(0,0,false);
    if(!a.equal()) {
        a.at(0x1000558c);a.load<1>(14,24,0x67);a.sign8(14,14,true);draw_or_zero<14,15>(a,0x100055a0);
        a.at(0x100056e8);a.compare(15,0);if(!a.equal())return false;
        a.load<1>(14,24,0x69);a.sign8(14,14,true);draw_or_zero<14,15>(a,0x10005704);
        a.at(0x1000584c);a.compare(15,0);if(!a.equal())return false;
        a.load<1>(3,24,0x67);a.sign8(0,3);a.compare(0,1);
        if(!a.nonpositive()) {
            a.sign8(14,3,true);draw_or_zero<14,15>(a,0x10005874);
            a.at(0x100059bc);a.compare(15,0);if(a.equal()){a.load<1>(3,24,0x67);a.offset(0,3,-1);a.store<1>(24,0x67,0);}
        }
        a.at(0x100059d0);a.load<1>(3,24,0x69);a.sign8(0,3);a.compare(0,1);
        if(!a.nonpositive()) {
            a.sign8(14,3,true);draw_or_zero<14,15>(a,0x100059f0);
            a.at(0x10005b38);a.compare(15,0);if(a.equal()){a.load<1>(3,24,0x69);a.offset(0,3,-1);a.store<1>(24,0x69,0);}
        }
        a.at(0x10005b4c);a.load(3,20,8);a.move(4,26);a.call(0x10738e60);a.jump(0x10006060);
    }else {a.load(3,20,8);a.move(4,26);a.call(0x10738e60);a.jump(0x10006060);}
    return true;
}

bool recover_pending_absence(Call& a) {
    a.at(0x10005b60);a.load(0,26,0);a.load(3,18,0);a.multiply(0,0,0x70);a.add(14,3,0);a.load<1>(15,14,0x67);a.sign8(15,15,true);draw_or_zero<15,16>(a,0x10005b84);
    advance_pool(a,0x10005ccc);
    a.load(3,31,0);a.value(4,0x66660000);a.load(5,30,0);a.offset(0,4,0x6667);a.load<2>(4,3,0);a.sign8(6,16);a.add(5,4,5);a.multiply_high(0,0,5);a.shift_right(0,0,1);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,5);a.subtract(4,0,5);a.offset(0,4,1);a.sign8(0,0);a.compare_registers(6,0);if(a.at_least())return false;
    a.load<1>(4,14,0x67);a.sign8(0,4);a.compare(0,1);
    if(!a.nonpositive()) {
        a.sign8(15,4,true);draw_or_zero<15,16,false,0x60,0x70,true>(a,0x10005d58);
        a.at(0x10005e9c);a.compare(16,0);if(a.equal()){a.load<1>(3,14,0x67);a.offset(0,3,-1);a.store<1>(14,0x67,0);}
    }
    a.at(0x10005eb0);a.load(3,14,0x14);a.compare(3,0,false);
    if(!a.equal()) {
        a.load<1>(3,3,0x44);a.sign8(0,3);a.compare(0,1);
        if(!a.nonpositive()) {
            a.sign8(15,3,true);draw_or_zero<15,16>(a,0x10005edc);
            a.at(0x10006024);a.compare(16,0);if(a.equal()){a.load(4,14,0x14);a.load<1>(3,4,0x44);a.offset(0,3,-1);a.store<1>(4,0x44,0);}
        }
    }
    a.at(0x1000603c);a.load(3,20,8);a.move(4,26);a.call(0x10738e60);a.jump(0x10006060);return true;
}

void review_pending_records(Call& a) {
    a.at(0x100054bc);a.load(3,20,8);a.compare(3,0,false);if(a.equal())return;
    a.load(0,3,4);a.value(21,0);a.store(3,12,0);a.jump(0x10006050);
    for(;;) {
        a.at(0x10006050);a.load(3,20,8);a.load<2>(0,3,0,true);a.compare_registers(21,0);if(!a.less())return;
        a.at(0x100054d8);a.call(0x10738e00);a.move(26,3,true);if(a.equal())return;
        a.load(4,19,0);a.offset(3,1,0x50);a.load(0,19,4);a.value(5,1);a.store(1,0x38,4);a.offset(4,26,8);a.store(1,0x3c,0);a.call(0x10717a10);
        if(past_adjusted_date(a,0x10005504,0x38,0x50)) {
            a.at(0x10005550);a.load<1>(0,26,16);a.sign8(0,0,true);
            const bool removed=!a.equal()?recover_pending_leave(a):recover_pending_absence(a);if(removed)return;
        }
        a.at(0x1000604c);a.offset(21,21,1);
    }
}
}

void process_daily_absences(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    using namespace daily_detail;
    Call a(s,daily_entry,d,o);a.link();a.multiple(14);a.move(20,3);a.load(29,2,-0x5a0c);a.load(30,2,-0x7b90);a.load(23,2,-0x7b8c);a.load(22,2,-0x7b88);a.load(31,2,-0x7b84);a.load(18,2,-0x7be0);a.load(19,2,-0x7b80);a.load(14,2,-0x7b7c);a.load(15,2,-0x7b78);a.save_link();a.frame(192);
    if(review_primary_records(a)&&review_secondary_records(a))review_pending_records(a);
    a.at(0x10006060);a.pop(192);a.multiple(14,true);a.finish();
}
}
