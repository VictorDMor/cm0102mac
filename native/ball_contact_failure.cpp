#include "ball_contact.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_contact {
namespace {
using Call=SemanticGuestCall;
void advance_random(Call& a,std::uint32_t start,bool capture_trait=false) {
    a.at(start);a.load(3,31,0);a.load(0,27,0);a.offset(3,3,2);if(capture_trait)a.load<1>(24,30,0x109);
    a.compare_registers(3,0,false);a.store(31,0,3);
    if(!a.nonpositive()){a.store(31,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
    a.at(start+(capture_trait?0x2c:0x28));
}
void wide_interval(Call& a,std::uint32_t start,std::uint32_t join,bool later) {
    const int interval=later?0x38:0x40,remainder=later?0x40:0x48,converted=later?0x48:0x38;
    a.at(start);a.exclusive_flags(3,24,0x80000000);a.value(0,0x43300000);a.store(1,interval+4,3);a.value(6,0x80010000);
    a.load(7,31,0);a.value(4,0x43300000);a.store(1,interval,0);a.value(5,0x10000);a.floating_load(2,26,0x80);a.value(3,0x80010000);
    a.floating_load(1,1,interval);a.offset(6,6,-0x7fff);a.floating_load(0,26,0);a.offset(5,5,-1);a.floating_subtract(1,1,2);
    a.load<2>(8,7,0);a.load(7,29,0);a.multiply(0,24,10);a.offset(3,3,-0x7fff);a.store(1,remainder,4);a.floating_difference(2,1,0,true);
    a.add(7,8,7);a.multiply_high(4,6,7);a.floating_load(1,26,0x80);a.add(4,4,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);
    a.add(4,4,6);a.product(4,4,5);a.subtract(25,4,7);a.exclusive_flags(4,25,0x80000000);a.store(1,remainder+4,4);
    a.multiply_high(3,3,0);a.floating_load(0,1,remainder);a.floating_subtract(0,0,1);a.add(0,3,0);a.shift_right(0,0,15);
    a.floating_product(0,2,0);a.mask(3,0,1,false,1);a.add(3,0,3);a.floating_integer(0,0);a.floating_store(0,1,converted);a.load(25,1,converted+4);
    a.call(0x10226520);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,24,-1);a.multiply_high(3,4,3);
    a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(25,25,3);a.compare_registers(25,0);
    if(!a.nonpositive()){a.offset(25,24,-1);a.jump(join);}
}
void sample_interval(Call& a,std::uint32_t start,bool later) {
    const auto join=later?0x10270acc:0x10270528;
    advance_random(a,start);a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(24,3);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(24,3);wide=!a.at_least();}
    if(wide){wide_interval(a,start+0x48,join,later);return;}
    a.at(later?0x10270a9c:0x102704f8);a.load(3,31,0);a.compare(24,0);a.load(0,29,0);a.load<2>(3,3,0);a.add(3,3,0);a.divide(0,3,24);
    a.product(0,0,24);a.subtract(25,0,3);a.negate(0,25);if(!a.nonpositive())a.move(0,25);a.at(later?0x10270ac8:0x10270524);a.move(25,0);
}
void message(Call& a,std::uint32_t start,unsigned id) {
    a.at(start);a.load(3,30,0x1a4);a.value(4,id);a.load<1>(7,30,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);a.value(9,0xffffffff);
    a.load<1>(6,30,0x2f);a.value(10,0);a.load(5,30,0x1a4);a.call(0x10201180);
}
void remainder_ten(Call& a) {
    a.load(4,31,0);a.value(3,0x66660000);a.load(0,29,0);a.offset(3,3,0x6667);a.load<2>(4,4,0);a.add(4,4,0);
    a.multiply_high(0,3,4);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,10);a.subtract(0,0,4,true);
}
void remainder_two_hundred(Call& a) {
    a.load(4,31,0);a.value(3,0x51ec0000);a.load(0,29,0);a.offset(3,3,-0x7ae1);a.load<2>(4,4,0);a.add(4,4,0);
    a.multiply_high(0,3,4);a.shift_right(0,0,6);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,200);a.subtract(0,0,4,true);
}
void request(Call& a,std::uint32_t start,unsigned kind,bool active,bool self) {
    a.at(start);a.load(3,30,0x1a4);
    if(self){a.move(10,30);a.load<1>(4,30,0x105);a.value(6,kind);a.load<1>(5,30,0x106);a.value(8,0);a.load<1>(7,30,0x27);a.value(9,1);}
    else {a.value(6,kind);a.load<1>(4,30,0x105);a.value(8,unsigned(active));a.load<1>(5,30,0x106);a.value(9,1);a.load<1>(7,30,0x27);a.value(10,0);}
    a.call(0x101f1100);
}
void choose_request(Call& a,bool later) {
    const auto join=later?0x10270944:0x10270b4c;
    advance_random(a,later?0x10270818:0x10270660);remainder_ten(a);
    if(!a.equal()){request(a,later?0x10270874:0x102706bc,later?1:3,true,false);a.jump(join);return;}
    advance_random(a,later?0x1027089c:0x102706e4);remainder_two_hundred(a);
    if(!a.equal()){request(a,later?0x102708f8:0x10270740,later?1:3,false,false);a.jump(join);return;}
    request(a,later?0x10270920:0x10270768,later?1:3,false,true);if(!later)a.jump(join);
}
void react_three(Call& a,bool final) {
    advance_random(a,final?0x10270ad4:0x102707b8);
    a.load(3,31,0);a.value(4,0x55550000);a.load(0,29,0);a.offset(4,4,0x5556);a.load<2>(5,3,0);a.move(3,30);a.add(5,5,0);
    a.multiply_high(4,4,5);a.mask(0,4,1,false,1);a.add(0,4,0);a.multiply(0,0,3);a.subtract(0,0,5);if(final)a.negate(0,0);a.sign8(4,0);
    a.call(final?0x10288f30:0x10285ed0);
}
void reduced_balance_reaction(Call& a) {
    a.at(0x10270944);a.load<2>(3,30,0x36,true);a.offset(0,3,-250);a.store<2>(30,0x36,0);a.load(3,30,0x6c);a.load<1>(0,3,0x66);
    a.sign8(0,0);a.multiply(3,0,30);a.offset_record(24,3,20);
    if(a.equal()){a.value(25,0);a.jump(0x10270acc);}else sample_interval(a,0x10270970,true);
    a.at(0x10270acc);a.compare(25,0);if(!a.equal())return;
    react_three(a,true);a.sign8(0,3,true);if(a.equal())return;a.move(3,30);a.value(4,0);a.call(0x1022ba10);
}
void resolve_failed_contact(Call& a) {
    a.at(0x10270380);a.load(3,30,0x70);a.value(0,0x43300000);a.store(1,0x48,0);a.load<1>(0,3,0x21);a.floating_load(1,26,0x80);
    a.sign8(0,0);a.floating_load(2,30,0xa8,true);a.exclusive_flags(0,0,0x80000000);a.store(1,0x4c,0);a.floating_load(0,1,0x48);
    a.floating_subtract(0,0,1,true);a.floating_sum_single(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0x40);a.load(24,1,0x44);a.compare(24,0);
    if(a.equal()){a.value(25,0);a.jump(0x10270528);}else sample_interval(a,0x102703cc,false);
    advance_random(a,0x10270528);a.load(4,31,0);a.value(3,0x66660000);a.load(0,29,0);a.offset(3,3,0x6667);a.load<2>(4,4,0);a.add(4,4,0);
    a.multiply_high(0,3,4);a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,20);a.subtract(0,0,4);a.compare_registers(25,0);
    if(a.at_least()){message(a,0x10270b54,0x1fe2);a.value(3,1);return;}
    advance_random(a,0x10270588,true);a.load(4,31,0);a.sign8(0,24);a.load(3,29,0);a.load<2>(4,4,0);a.add(4,4,3);
    a.mask(3,4,0xe0000000,false,29);a.mask(4,4,1,false,1);a.subtract(3,4,3);a.mask(3,3,0xffffffff,false,3);a.add(3,3,4);a.compare_registers(0,3);
    if(!a.nonpositive()) {
        message(a,0x102705e4,0x1fe3);advance_random(a,0x1027060c);a.load(4,31,0);a.move(3,30);a.load(0,29,0);a.load<2>(4,4,0);a.add(0,4,0);
        a.mask(4,0,1,false,1);a.mask(0,0,1);a.combine(0,0,4,false,true);a.subtract(0,4,0);a.sign8(4,0);a.call(0x10285ed0);choose_request(a,false);
    } else {message(a,0x10270790,0x216f);react_three(a,false);choose_request(a,true);reduced_balance_reaction(a);}
    a.at(0x10270b4c);a.value(3,0);a.jump(0x10270b80);
}
}
void react_to_failure(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,failure_entry,dependencies,observer);a.link();a.multiple(24);a.load(26,2,-0x51e8);a.move(30,3);a.load(29,2,-0x7b90);a.load(28,2,-0x7b8c);
    a.load(27,2,-0x7b88);a.load(31,2,-0x7b84);a.save_link();a.sign16(0,4);a.compare(0,1000);a.frame(0x70);
    if(!a.nonpositive()){message(a,0x10270354,0x1fe2);a.jump(0x10270b7c);a.at(0x10270b7c);a.value(3,1);}
    else resolve_failed_contact(a);
    a.at(0x10270b80);a.load(0,1,0x78);a.offset(1,1,0x70);a.restore_link();a.multiple(24,true);a.finish();
}
}
