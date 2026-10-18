#include "awol_event_random.hpp"

namespace cm::aot::awol {
namespace {
using namespace event_detail;
using Pool=RandomPool<0,27,28,29,30>;

bool eligible_training_notice(Call& a) {
    if(a.equal())return false;
    a.load(0,22,0x14);a.compare(0,0,false);if(a.equal())return false;
    a.compare(23,0,false);if(a.equal())return false;
    a.load(0,23,0x90);a.compare(0,0,false);return !a.equal();
}

void fill_training_context(Call& a) {
    a.at(0x100087c8);a.offset(3,1,0x38);a.value(4,0xbc9);a.value(5,0);a.call(0x102bdf90);
    a.load(5,22,0);a.offset(3,1,0x38);a.value(4,0);a.call(0x102cb4b0);
    for(unsigned n=0;n<3;++n){a.at(0x100087e8+n*20);a.load(5,22,int(0x28+n*4));a.offset(3,1,0x38);a.value(4,n+1);a.load(5,5,0x20);a.call(0x102cb4b0);}
    a.at(0x10008824);a.load(5,23,0);a.offset(3,1,0x38);a.value(4,4);a.call(0x102cb4b0);
    Pool::advance(a,0x10008834);a.load(4,30,0);a.value(3,0x55550000);a.load(5,27,0);a.offset(0,3,0x5556);a.load<2>(6,4,0);a.offset(3,1,0x38);a.value(4,5);a.add(6,6,5);a.multiply_high(5,0,6);a.mask(0,5,1,false,1);a.add(0,5,0);a.multiply(0,0,3);a.subtract(5,0,6);a.call(0x102cb4b0);
    a.move(3,23);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(5,3);a.offset(3,1,0x38);a.value(4,6);a.call(0x102cb4b0);
}

void fill_manager_reporter(Call& a) {
    a.at(0x100088e8);a.load(5,23,0x90);a.offset(3,1,0x38);a.value(4,7);a.load(5,5,0);a.call(0x102cb4b0);
    for(unsigned n=0;n<3;++n){a.at(0x100088fc+n*24);a.load(5,23,0x90);a.offset(3,1,0x38);a.value(4,n+8);a.load(5,5,int(0x28+n*4));a.load(5,5,0x20);a.call(0x102cb4b0);}
    a.at(0x10008944);a.jump(0x10008af4);
}

void select_coach_reporter(Call& a) {
    a.at(0x100089ac);a.value(3,0x66660000);a.value(24,0);a.offset(21,3,0x6667);
    for(;;) {
        Pool::advance(a,0x100089b8);a.load(3,30,0);a.offset(24,24,1);a.load(0,27,0);a.compare(24,1000);a.load<2>(3,3,0);a.add(4,3,0);a.multiply_high(0,21,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,5);a.subtract(4,0,4);
        if(a.at_least())break;
        a.mask(3,4,0xfffffffc,false,2);a.offset(0,3,0x160);a.indexed_load(0,23,0);a.compare(0,0,false);if(!a.equal())break;
    }
    a.at(0x10008a28);a.mask(0,4,0xfffffffc,false,2);a.add(3,23,0);a.load(3,3,0x160);a.compare(3,0,false);if(!a.equal())a.load(25,3,0);
}

void choose_alternate_reporter(Call& a) {
    Pool::advance(a,0x10008948);a.load(3,30,0);a.load(0,27,0);a.load<2>(3,3,0);a.add(0,3,0);a.mask(3,0,1,false,1);a.mask(0,0,1);a.combine(0,0,3,false,true);a.subtract(0,3,0,true);
    if(a.equal()) {
        a.at(0x10008998);a.load(3,23,0x94);a.compare(3,0,false);
        if(!a.equal()){a.load(25,3,0);a.jump(0x10008a40);}
    }else {a.jump(0x100089ac);select_coach_reporter(a);}
    a.at(0x10008a40);a.compare(25,-1);
    if(a.equal()) {
        for(unsigned n=0;n<4;++n){a.at(0x10008a48+n*16);a.offset(3,1,0x38);a.value(4,n+7);a.value(5,0xffffffff);a.call(0x102cb4b0);}
        a.at(0x10008a88);a.jump(0x10008af4);
    }else {
        a.at(0x10008a8c);a.move(5,25);a.offset(3,1,0x38);a.value(4,7);a.call(0x102cb4b0);a.multiply(21,25,0x70);
        for(unsigned n=0;n<3;++n){a.at(0x10008aa0+n*28);a.load(0,31,0);a.offset(3,1,0x38);a.value(4,n+8);a.add(5,0,21);a.load(5,5,int(0x28+n*4));a.load(5,5,0x20);a.call(0x102cb4b0);}
    }
}

void choose_training_reporter(Call& a) {
    a.at(0x100088bc);a.load(0,23,0x90);a.compare(0,0,false);bool alternative=a.equal();
    if(!alternative){a.move(3,23);a.value(4,1);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);alternative=!a.equal();}
    if(alternative)choose_alternate_reporter(a);else fill_manager_reporter(a);
}

void send_training_notice(Call& a) {
    a.at(0x10008af4);a.load(3,26,0);a.move(4,23);a.call(0x102cdbf0);a.sign8(0,3,true);
    if(a.equal()) {
        a.load(5,23,0x2c);a.compare(5,0,false);if(a.equal())return;
        a.load(3,22,0x14);a.load<2>(0,3,10,true);a.compare(0,7250);if(a.nonpositive())return;
        a.load(3,26,0);a.offset(4,1,0x38);a.call(0x102cc400);a.jump(0x10008b44);
    }else {a.at(0x10008b34);a.load(3,26,0);a.offset(4,1,0x38);a.load(5,22,0x24);a.call(0x102cc140);}
}
}

void notify_training_absence(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,training_notice_entry,d,o);a.link();a.multiple(21);a.move(22,4,true);a.load(26,2,-0x7bf0);a.load(27,2,-0x7b90);a.move(23,5);a.load(28,2,-0x7b8c);a.value(25,0xffffffff);a.load(29,2,-0x7b88);a.load(30,2,-0x7b84);a.load(31,2,-0x7be0);a.save_link();a.frame(336);
    if(eligible_training_notice(a)){fill_training_context(a);choose_training_reporter(a);send_training_notice(a);}
    a.at(0x10008b44);a.pop(336);a.multiple(21,true);a.finish();
}
}
