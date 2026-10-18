#include "awol_daily_detail.hpp"

namespace cm::aot::awol::daily_detail {
namespace {
void remove_leave_record(Call& a) {
    a.at(0x100047ec);a.move(3,20);a.move(4,21);a.call(remove_secondary_item_entry);a.jump(0x100054bc);
}
void remove_absence_record(Call& a) {
    a.at(0x10005498);a.move(3,20);a.move(4,21);a.call(remove_secondary_item_entry);a.jump(0x100054bc);
}
void review_peer_reactions(Call&);
}

void return_from_leave(Call& a) {
    a.at(0x100041f4);a.load(0,21,0);a.load(3,18,0);a.multiply(0,0,0x70);a.add(14,3,0);a.load(0,14,0x14);a.compare(0,0,false);
    if(a.equal()){remove_leave_record(a);return;}
    a.load<1>(15,14,0x67);a.sign8(15,15,true);draw_or_zero<15,16,false,0x70,0x60>(a,0x10004224);
    a.at(0x1000436c);a.compare(16,0);if(!a.equal()){remove_leave_record(a);return;}
    a.load<1>(15,14,0x69);a.sign8(15,15,true);draw_or_zero<15,16>(a,0x10004388);
    a.at(0x100044d0);a.compare(16,0);if(!a.equal()){remove_leave_record(a);return;}
    a.load<1>(3,14,0x67);a.sign8(0,3);a.compare(0,1);
    if(!a.nonpositive()) {
        a.sign8(15,3,true);draw_or_zero<15,16>(a,0x100044f8);
        a.at(0x10004640);a.compare(16,0);if(a.equal()){a.load<1>(3,14,0x67);a.offset(0,3,-1);a.store<1>(14,0x67,0);}
    }
    a.at(0x10004654);a.load<1>(3,14,0x69);a.sign8(0,3);a.compare(0,1);
    if(!a.nonpositive()) {
        a.sign8(15,3,true);draw_or_zero<15,16>(a,0x10004674);
        a.at(0x100047bc);a.compare(16,0);if(a.equal()){a.load<1>(3,14,0x69);a.offset(0,3,-1);a.store<1>(14,0x69,0);}
    }
    a.at(0x100047d0);a.move(3,20);a.move(4,21);a.call(remove_secondary_item_entry);a.move(3,20);a.move(4,14);a.call(0x10007a50);a.jump(0x100054bc);
}

namespace {
void consider_peer_contract(Call& a) {
    a.at(0x1000488c);a.call(0x1037fa70);a.mask(0,3,2,true);if(!a.equal())return;
    a.load<1>(3,25,0x67);a.sign8(0,3);a.compare(0,15);if(a.nonpositive())return;
    a.load<1>(0,25,0x63);a.sign8(0,0);a.compare(0,10);if(a.nonpositive())return;
    a.sign8(16,3,true);draw_or_zero<16,15,true>(a,0x100048c8);
    advance_pool(a,0x10004a10);
    a.load(5,31,0);a.value(3,0x88890000);a.load(4,30,0);a.offset(0,3,-0x7777);a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(0,0,4);a.add(0,0,4);a.shift_right(0,0,4);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,30);a.subtract(0,0,4);a.compare_registers(15,0);if(a.nonpositive())return;
    a.load<1>(0,17,0x60);a.mask(0,0,15);a.compare(0,1);
    if(!a.equal()){a.sign8(0,0);a.compare(0,2);if(!a.equal())return;}
    a.at(0x10004a90);a.load<2>(0,25,0x5c,true);a.sign8(0,0);a.compare(0,23);if(a.nonpositive())return;
    a.load(3,25,0x14);a.load<2>(0,3,10,true);a.compare(0,5250);if(a.nonpositive())return;
    a.move(3,17);a.value(4,2);a.call(0x1037f8a0);
}

void consider_young_peer_traits(Call& a) {
    a.at(0x10004abc);a.load<2>(0,25,0x5c,true);a.sign8(0,0);a.compare(0,21);if(a.at_least())return;
    a.load(3,25,0x14);a.load<2>(0,3,10,true);a.compare(0,5250);if(a.nonpositive())return;
    a.load<1>(3,25,0x67);a.sign8(0,3);a.compare(0,1);
    if(!a.nonpositive()) {
        a.sign8(15,3,true);draw_or_zero<15,16,true>(a,0x10004afc);
        a.at(0x10004c44);a.compare(16,0);if(a.equal()){a.load<1>(3,25,0x67);a.offset(0,3,-1);a.store<1>(25,0x67,0);}
    }
    a.at(0x10004c58);a.load(3,25,0x14);a.load<1>(0,3,0x44);a.sign8(0,0);a.compare(0,1);
    if(!a.nonpositive()) {
        // This draw uses the staff trait again, rather than profile offset 0x44.
        a.load<1>(15,25,0x67);a.sign8(15,15,true);draw_or_zero<15,16,true>(a,0x10004c80);
        a.at(0x10004dc8);a.compare(16,0);if(a.equal()){a.load(4,25,0x14);a.load<1>(3,4,0x44);a.offset(0,3,-1);a.store<1>(4,0x44,0);}
    }
    a.at(0x10004de0);a.load<1>(3,25,0x69);a.sign8(0,3);a.compare(0,1);
    if(!a.nonpositive()) {
        a.sign8(15,3,true);draw_or_zero<15,16,true>(a,0x10004e00);
        a.at(0x10004f48);a.compare(16,0);if(a.equal()){a.load<1>(3,25,0x69);a.offset(0,3,-1);a.store<1>(25,0x69,0);}
    }
}

void review_peer_reactions(Call& a) {
    a.at(0x10004840);a.value(24,0);a.value(26,0);a.jump(0x10004f64);
    for(;;) {
        a.at(0x10004f64);a.compare(24,50);if(a.at_least())break;a.compare(27,0,false);if(a.equal())break;
        a.at(0x10004850);a.offset(0,26,0x98);a.indexed_load(25,27,0);a.compare(25,0,false);bool eligible=!a.equal();
        if(eligible){a.compare_registers(25,28,false);eligible=!a.equal();}
        if(eligible){a.load(0,25,0x14);a.compare(0,0,false);eligible=!a.equal();}
        if(eligible) {
            a.load(3,14,0);a.move(4,28);a.move(5,27);a.call(0x10082f90);a.move(17,3,true);
            if(!a.equal()){consider_peer_contract(a);consider_young_peer_traits(a);}
        }
        a.at(0x10004f5c);a.offset(26,26,4);a.offset(24,24,1);
    }
}
}

void return_from_unapproved_absence(Call& a) {
    a.at(0x100047fc);a.load(0,21,0);a.load(4,18,0);a.multiply(0,0,0x70);a.load(3,14,0);a.add(28,4,0);a.move(4,28);a.call(0x10082f20);a.compare(3,0,false);a.store(15,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(27,3,0);a.jump(0x10004840);}
    else {a.at(0x1000483c);a.value(27,0);}
    review_peer_reactions(a);
    a.at(0x10004f74);a.load(0,28,0x14);a.compare(0,0,false);if(a.equal()){remove_absence_record(a);return;}
    a.load<1>(14,28,0x67);a.sign8(14,14,true);draw_or_zero<14,15>(a,0x10004f94);
    advance_pool(a,0x100050dc);
    a.load(3,31,0);a.value(4,0x88890000);a.load(5,30,0);a.offset(0,4,-0x7777);a.load<2>(4,3,0);a.sign8(6,15);a.add(5,4,5);a.multiply_high(0,0,5);a.add(0,0,5);a.shift_right(0,0,3);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,15);a.subtract(4,0,5);a.offset(0,4,1);a.sign8(0,0);a.compare_registers(6,0);
    if(a.at_least()){remove_absence_record(a);return;}
    a.load<1>(4,28,0x67);a.sign8(0,4);a.compare(0,1);
    if(!a.nonpositive()) {
        a.sign8(14,4,true);draw_or_zero<14,15,false,0x60,0x70,true>(a,0x1000516c);
        // Unlike the leave and peer paths, this decrement requires a nonzero draw.
        a.at(0x100052b0);a.compare(15,0);if(!a.equal()){a.load<1>(3,28,0x67);a.offset(0,3,-1);a.store<1>(28,0x67,0);}
    }
    a.at(0x100052c4);a.load(3,28,0x14);a.load<1>(3,3,0x44);a.sign8(0,3);a.compare(0,1);
    if(!a.nonpositive()) {
        a.sign8(14,3,true);draw_or_zero<14,15>(a,0x100052e8);
        a.at(0x10005430);a.compare(15,0);if(!a.equal()){a.load(4,28,0x14);a.load<1>(3,4,0x44);a.offset(0,3,-1);a.store<1>(4,0x44,0);}
    }
    a.at(0x10005448);a.move(3,20);a.move(4,21);a.call(remove_secondary_item_entry);a.move(3,20);a.move(4,28);a.call(0x10007a50);a.compare(27,0,false);if(a.equal())return;
    a.load(0,28,0x14);a.compare(0,0,false);if(a.equal())return;
    a.load(3,2,-0x7b94);a.move(4,28);a.move(5,27);a.value(6,8);a.load(3,3,0);a.value(7,0);a.value(8,0);a.call(0x10649fb0);a.jump(0x100054bc);
}
}
