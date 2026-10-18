#include "awol_event_random.hpp"

namespace cm::aot::awol {
namespace {
using namespace event_detail;
using Pool=RandomPool<23,28,29,30,31>;

void append_fitness_event(Call& a) {
    a.at(0x100090fc);a.load(3,25,0);a.offset(4,1,0x38);a.load<1>(0,21,4);a.value(5,20);a.store(1,0x38,3);a.load<2>(6,21,2);a.load(7,26,0);a.load<2>(3,21,0,true);a.store(1,0x3c,7);a.store<2>(1,0x42,6);a.store<2>(1,0x40,3);a.store<1>(1,0x44,0);a.load(3,24,8);a.call(0x10738d10);
}

template<bool AfterNotice>
void review_fitness_discipline(Call& a) {
    constexpr auto start=AfterNotice?0x10009758u:0x10009148u;
    constexpr auto first=AfterNotice?0x100097a4u:0x10009194u,second=AfterNotice?0x1000996cu:0x1000935cu;
    a.at(start);a.move(3,26);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);if(!a.equal())return;
    a.load(3,26,0x90);a.compare(3,0,false);if(a.equal())return;
    a.load(3,3,0x1c);a.compare(3,0,false);if(a.equal())return;
    a.load<1>(22,3,0x34);a.sign8(22,22,true);
    Pool::draw_or_zero<22,24,AfterNotice?0x130:0x140,0x138,AfterNotice?0x140:0x130,true>(a,first);
    Pool::advance(a,first+0x148);a.load(5,31,0);a.value(3,0x66660000);a.load(4,28,0);a.offset(0,3,0x6667);a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(0,0,4);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,10);a.subtract(0,0,4);a.compare_registers(24,0);if(a.nonpositive())return;
    a.load(3,26,0x90);a.load(3,3,0x1c);a.load<1>(4,3,0x39);a.call(0x100d2f50);a.sign8(22,3,true);Pool::draw_or_zero<22,23,0x130,0x138,0x140,true>(a,second);
    Pool::advance(a,second+0x148);a.load(5,31,0);a.value(3,0x66660000);a.load(4,28,0);a.offset(0,3,0x6667);a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(0,0,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,5);a.subtract(0,0,4);a.compare_registers(23,0);
    if(!a.nonpositive()){a.load(3,27,0);a.move(4,25);a.value(5,1);a.value(6,1);a.call(0x1010a080);a.jump(0x10009b40);}
    else {a.at(AfterNotice?0x10009b2c:0x1000951c);a.load(3,27,0);a.move(4,25);a.value(5,2);a.value(6,1);a.call(0x1010a080);if constexpr(!AfterNotice)a.jump(0x10009b40);}
}

void select_fitness_reporter(Call& a) {
    a.at(0x10009534);a.value(3,0x66660000);a.offset(24,3,0x6667);
    for(;;) {
        Pool::advance(a,0x1000953c);a.load(3,31,0);a.load(4,28,0);a.load<2>(5,3,0);a.offset(3,3,2);a.load(0,30,0);a.add(5,5,4);a.multiply_high(4,24,5);a.compare_registers(3,0,false);a.shift_right(0,4,1);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,5);a.subtract(0,0,5);a.mask(4,0,0xfffffffc,false,2);a.offset(0,4,0x160);a.indexed_load(21,26,0);a.store(31,0,3);
        if(!a.nonpositive()){a.store(31,0,29);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(28,0,0);}
        a.at(0x100095bc);a.load(3,31,0);a.load(0,28,0);a.load<2>(3,3,0);a.add(4,3,0);a.multiply_high(0,24,4);a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,20);a.subtract(0,0,4,true);if(a.equal())return;
        a.compare(21,0,false);if(!a.equal())return;
    }
}

void fill_fitness_notice(Call& a) {
    a.at(0x100095f0);a.offset(3,1,0x4c);a.value(4,0x138f);a.value(5,0);a.call(0x102bdf90);
    a.load(5,25,0);a.offset(3,1,0x4c);a.value(4,0);a.call(0x102cb4b0);a.load(5,26,0);a.offset(3,1,0x4c);a.value(4,1);a.call(0x102cb4b0);
    Pool::advance(a,0x10009620);a.load(5,31,0);a.offset(3,1,0x4c);a.load(0,28,0);a.value(4,2);a.load<2>(5,5,0);a.add(0,5,0);a.mask(5,0,1,false,1);a.mask(0,0,1);a.combine(0,0,5,false,true);a.subtract(5,5,0);a.call(0x102cb4b0);
    for(unsigned n=0;n<3;++n){a.at(0x10009674+n*20);a.load(5,25,int(0x28+n*4));a.offset(3,1,0x4c);a.value(4,n+3);a.load(5,5,0x20);a.call(0x102cb4b0);}
    a.at(0x100096b0);a.compare(21,0,false);
    if(!a.equal()) {
        a.load(5,21,0);a.offset(3,1,0x4c);a.value(4,6);a.call(0x102cb4b0);
        for(unsigned n=0;n<3;++n){a.at(0x100096c8+n*20);a.load(5,21,int(0x28+n*4));a.offset(3,1,0x4c);a.value(4,n+7);a.load(5,5,0x20);a.call(0x102cb4b0);}
        a.at(0x10009704);a.jump(0x10009748);
    }else {
        for(unsigned n=0;n<4;++n){a.at(0x10009708+n*16);a.offset(3,1,0x4c);a.value(4,n+6);a.value(5,0xffffffff);a.call(0x102cb4b0);}
    }
    a.at(0x10009748);a.load(3,22,0);a.move(5,26);a.offset(4,1,0x4c);a.call(0x102cc140);
}
}

void process_fitness_concern(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,fitness_event_entry,d,o);a.link();a.multiple(21);a.load(27,2,-0x7ba4);a.move(24,3);a.load(22,2,-0x7bf0);a.move(25,4);a.load(23,2,-0x5a0c);a.move(26,5);a.load(28,2,-0x7b90);a.load(29,2,-0x7b8c);a.load(30,2,-0x7b88);a.load(31,2,-0x7b84);a.load(21,2,-0x7b80);a.save_link();a.frame(384);a.offset(0,1,0x40);a.move(3,0);a.call(0x10715880);a.compare(25,0,false);
    if(!a.equal()) {
        a.compare(26,0,false);
        if(!a.equal()) {
            append_fitness_event(a);a.load(3,22,0);a.move(4,26);a.call(0x102cdbf0);a.sign8(0,3,true);
            if(a.equal())review_fitness_discipline<false>(a);
            else {select_fitness_reporter(a);fill_fitness_notice(a);review_fitness_discipline<true>(a);}
        }else a.jump(0x10009b40);
    }
    a.at(0x10009b40);a.pop(384);a.multiple(21,true);a.finish();
}
}
