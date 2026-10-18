#include "awol_event_random.hpp"

namespace cm::aot::awol {
namespace {
using namespace event_detail;
using Pool=RandomPool<28,29,25,24,30>;

bool eligible_training_event(Call& a) {
    a.at(0x10007aa0);a.compare(31,0,false);if(a.equal())return false;
    a.load(0,31,0x14);a.compare(0,0,false);if(a.equal()){a.jump(0x1000875c);return false;}
    a.at(0x10007ab8);a.compare(31,0,false);
    if(!a.equal()){a.load(3,2,-0x7be4);a.load(4,31,0);a.load(0,3,0);a.compare_registers(4,0);if(a.at_least())return false;}
    a.at(0x10007ad4);a.load(3,23,0);a.move(4,31);a.call(0x10082f20);a.compare(3,0,false);a.store(21,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,20,0);a.multiply(0,0,0x248);a.add(27,3,0);a.jump(0x10007b04);}else {a.at(0x10007b00);a.value(27,0);}
    a.at(0x10007b04);a.compare(27,0,false);if(a.equal())return false;
    a.load(3,26,12);a.load(0,31,0);a.indexed_load<1>(0,3,0);a.mask(0,0,2,true);return a.equal();
}

template<bool Second>
bool convert_to_primary_absence(Call& a) {
    constexpr auto start=Second?0x10007e38u:0x10007b44u;
    a.at(start);a.load(0,19,0x50);a.mask(0,0,Second?0x800:0x2000,true);if(a.equal())return false;
    a.load<1>(0,31,0x67);a.sign8(3,0);a.offset_record(16,3,10);
    if constexpr(Second)Pool::draw_or_zero<16,15,0x48,0x50,0x58>(a,0x10007e5c);
    else Pool::draw_or_zero<16,15,0x58,0x50,0x48>(a,0x10007b68);
    a.compare(15,0);if(!a.equal())return false;
    a.load<1>(0,31,0x65);a.sign8(3,0);a.offset_record(16,3,10);
    Pool::draw_or_zero<16,15,0x48,0x50,0x58>(a,Second?0x10007fc4:0x10007cd0);
    a.compare(15,0);if(!a.equal())return false;
    a.move(3,26);a.move(4,31);a.value(5,0);a.value(6,0);a.call(registration_entry);a.jump(0x1000875c);return true;
}
bool choose_primary_event(Call& a) {
    a.at(0x10007b20);a.load(3,23,0);a.move(4,31);a.call(0x10082f20);a.move(19,3,true);if(a.equal())return false;
    a.load<1>(0,19,0x4c);a.sign8(0,0);a.compare(0,-100);if(a.positive())return false;
    return convert_to_primary_absence<false>(a)||convert_to_primary_absence<true>(a);
}

void publish_training_flag(Call& a) {
    a.at(0x1000812c);a.load(5,26,12);a.move(4,31);a.load(3,31,0);a.indexed_load<1>(0,5,3);a.flags(0,0,2);a.indexed_store<1>(0,5,3);a.load(3,23,0);a.call(0x10082f20);a.compare(3,0,false);a.store(21,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,20,0);a.multiply(0,0,0x248);a.add(5,3,0);a.jump(0x10008170);}else {a.at(0x1000816c);a.value(5,0);}
    a.at(0x10008170);a.move(3,26);a.move(4,31);a.call(training_notice_entry);
}
void apply_training_fitness(Call& a) {
    Pool::advance(a,0x1000817c);a.load(4,30,0);a.value(3,0x66660000);a.load(5,31,0);a.offset(0,3,0x6667);a.load<2>(6,4,0);a.move(4,31);a.load(3,29,0);a.multiply(7,5,100);a.load(8,2,-0x7b9c);a.value(5,0);a.add(6,6,3);a.multiply_high(0,0,6);a.load(3,8,0);a.add(8,3,7);a.load<1>(7,8,10);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,10);a.subtract(0,0,6);a.mask(0,0,0xff);a.add(0,7,0);a.store<1>(8,10,0);a.load(3,18,0);a.call(0x101878d0);
    a.load(4,31,0x14);a.move(20,3);a.load<1>(0,4,0x34);a.sign8(0,0);a.subtract_from(0,0,21);a.multiply(0,0,25);a.sign16(19,0,true);
    Pool::draw_or_zero<19,15,0x48,0x50,0x58,true,false>(a,0x10008230);
    a.add(4,19,15);a.load(3,18,0);a.offset(0,4,100);a.move(4,31);a.sign16(0,0);a.value(6,0);a.subtract(0,0,20);a.sign16(5,0);a.call(0x10186a20);
    a.load(7,31,0);a.offset(4,1,0x38);a.load<1>(0,22,4);a.value(5,16);a.load<2>(6,22,2);a.load<2>(3,22,0,true);a.store(1,0x38,7);a.store<2>(1,0x3e,6);a.store<2>(1,0x3c,3);a.store<1>(1,0x40,0);a.load(3,26,4);a.call(0x10738d10);
}
void discipline_training_absence(Call& a) {
    a.at(0x100083cc);a.move(3,27);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);if(!a.equal())return;
    a.load(3,27,0x90);a.compare(3,0,false);if(a.equal())return;
    a.load(3,3,0x1c);a.compare(3,0,false);if(a.equal())return;
    a.load<1>(16,3,0x34);a.sign8(16,16,true);Pool::draw_or_zero<16,15,0x48,0x50,0x58,true>(a,0x10008418);a.compare(15,0);if(a.equal())return;
    a.load(3,27,0x90);a.load(3,3,0x1c);a.load<1>(4,3,0x39);a.call(0x100d2f50);a.sign8(16,3,true);Pool::draw_or_zero<16,15,0x48,0x50,0x58,true>(a,0x10008588);
    Pool::advance(a,0x100086d0);a.load(5,30,0);a.value(3,0x66660000);a.load(4,29,0);a.offset(0,3,0x6667);a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(0,0,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,5);a.subtract(0,0,4);a.compare_registers(15,0);
    if(!a.nonpositive()){a.load(3,17,0);a.move(4,31);a.value(5,1);a.value(6,1);a.call(0x1010a080);a.jump(0x1000875c);}
    else {a.at(0x10008748);a.load(3,17,0);a.move(4,31);a.value(5,2);a.value(6,1);a.call(0x1010a080);}
}
}

void process_training_absence(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,training_event_entry,d,o);a.link();a.multiple(15);a.load(17,2,-0x7ba4);a.move(26,3);a.load(18,2,-0x7ba0);a.move(31,4);a.load(28,2,-0x5a0c);a.load(29,2,-0x7b90);a.load(25,2,-0x7b8c);a.load(24,2,-0x7b88);a.load(30,2,-0x7b84);a.load(22,2,-0x7b80);a.load(23,2,-0x7b7c);a.load(21,2,-0x7b78);a.load(20,2,-0x7be8);a.save_link();a.frame(176);a.offset(0,1,0x3c);a.move(3,0);a.call(0x10715880);
    if(eligible_training_event(a)&&!choose_primary_event(a)){publish_training_flag(a);apply_training_fitness(a);discipline_training_absence(a);}
    a.at(0x1000875c);a.pop(176);a.multiple(15,true);a.finish();
}
}
