#include "contract_review_detail.hpp"

namespace cm::aot::contract_review {
namespace {
using detail::Call;
// Three complete 82-instruction samplers differ only in local relocation,
// conversion scratch layout and result register. The source audit proves that
// normalization; the independent reference still retains every instruction.
template<unsigned Result,bool FirstScratch=false>
void sample_response_range(Call& a,std::uint32_t start) {
    const auto join=start+0x148;
    a.at(start);a.load(3,26,0);a.load(0,25,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(26,0,3);
    if(!a.nonpositive()){a.store(26,0,24);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(23,0,0);}
    a.at(start+0x28);a.value(3,0x10000);a.offset(0,3,-1);a.compare_registers(21,0);bool wide=a.positive();
    if(!wide){a.value(3,0xffff0000);a.offset(0,3,1);a.compare_registers(21,0);wide=!a.at_least();}
    if(wide){
        a.at(start+0x48);a.value(4,0x43300000);a.exclusive_flags(0,21,0x80000000);a.store(1,FirstScratch?0x4c:0x3c,0);a.value(5,0x80010000);a.load(7,26,0);a.value(3,0x10000);a.store(1,FirstScratch?0x48:0x38,4);a.offset(6,5,-0x7fff);
        a.floating_load(2,22,0xc0);a.multiply(0,21,10);a.floating_load(1,1,FirstScratch?0x48:0x38);a.offset(5,3,-1);a.floating_load(0,22,0);a.floating_subtract(1,1,2);a.multiply_high(3,6,0);a.load<2>(8,7,0);a.load(7,23,0);a.floating_difference(1,1,0,true);a.store(1,0x40,4);a.add(7,8,7);a.multiply_high(4,6,7);a.add(0,3,0);a.add(3,4,7);a.shift_right(3,3,15);a.mask(4,3,1,false,1);a.shift_right(0,0,15);a.add(3,3,4);a.product(4,3,5);a.mask(3,0,1,false,1);a.add(3,0,3);a.subtract(0,4,7);a.exclusive_flags(0,0,0x80000000);a.store(1,0x44,0);a.floating_load(0,1,0x40);a.floating_subtract(0,0,2);a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,FirstScratch?0x38:0x48);a.load(Result,1,FirstScratch?0x3c:0x4c);
        a.call(0x10374570);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,21,-1);a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(Result,Result,3);a.compare_registers(Result,0);
        if(!a.nonpositive()){a.move(Result,0);a.jump(join);}
    }else{
        a.at(start+0x11c);a.load(3,26,0);a.compare(21,0);a.load(0,23,0);a.load<2>(3,3,0);a.add(3,3,0);a.divide(0,3,21);a.product(0,0,21);a.subtract(0,0,3);a.negate(Result,0);if(!a.nonpositive())a.move(Result,0);
    }
    a.at(join);
}
bool review_notification_needed(Call& a) {
    a.at(0x1037fb40);a.load<1>(0,27,0x4c);a.sign8(0,0);a.mask(0,0,0x10000,true);if(!a.equal())return false;
    a.load<1>(0,27,0x54);a.sign8(0,0,true);if(!a.equal())return false;
    a.move(3,27);a.call(has_review_trigger_entry);a.sign8(0,3,true);if(a.equal())return false;
    a.load(0,27,0);a.value(5,0);a.load(4,31,0);a.multiply(0,0,0x70);a.load(3,21,0);a.add(4,4,0);a.call(0x103f43b0);a.sign8(0,3,true);if(!a.equal())return false;
    a.load<1>(3,27,0x5c);a.sign8(3,3);a.mask(0,3,8,true);if(!a.equal())return false;
    a.mask(0,3,1,true);if(!a.equal())return false;
    a.load(0,27,0);a.load(4,31,0);a.multiply(0,0,0x70);a.load(3,21,0);a.add(4,4,0);a.call(0x103e6470);a.sign8(0,3,true);if(!a.equal())return false;
    a.load(0,27,0);a.load(4,31,0);a.multiply(3,0,0x70);a.offset(0,3,0x5c);a.indexed_load<2>(0,4,0,true);a.sign8(0,0);a.compare(0,20);if(a.positive())return true;
    a.add(3,4,3);a.load(3,3,0x14);a.load<2>(0,3,10,true);a.compare(0,4250);return !a.nonpositive();
}
void notify_review_if_needed(Call& a) {
    a.at(0x1037fb08);a.load(4,30,0x90);a.compare(4,0,false);bool exempt=false;
    if(!a.equal()){a.load(3,27,0);a.load(0,4,0);a.compare_registers(3,0);exempt=a.equal();}
    if(!exempt){a.at(0x1037fb24);a.load(4,30,0x94);a.compare(4,0,false);if(!a.equal()){a.load(3,27,0);a.load(0,4,0);a.compare_registers(3,0);exempt=a.equal();}}
    if(!exempt&&review_notification_needed(a)){a.at(0x1037fbfc);a.load(3,2,-0x7b7c);a.move(4,27);a.load(3,3,0);a.call(0x1008e350);}
}
void adjust_primary_staff_trait(Call& a) {
    a.at(0x1037fc2c);a.value(3,0x10000);a.offset(0,3,0x6288);a.intersect_record(0,28,0);if(a.equal())return;
    a.load(3,30,0x90);a.load<1>(0,3,0x65);a.sign8(0,0);a.compare(0,1);if(a.nonpositive())return;
    a.load(3,26,0);a.load(0,25,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(26,0,3);if(!a.nonpositive()){a.store(26,0,24);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(23,0,0);}
    a.at(0x1037fc78);a.load(5,26,0);a.value(3,0x66660000);a.load(4,23,0);a.offset(0,3,0x6667);a.load<2>(3,5,0);a.add(4,3,4);a.multiply_high(0,0,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,5);a.subtract(0,0,4,true);
    if(!a.equal())return;
    a.load(4,30,0x90);a.load<1>(3,4,0x65);a.offset(0,3,-1);a.store<1>(4,0x65,0);a.jump(0x10380170);
}
void compare_primary_relationship_draws(Call& a) {
    a.at(0x1037fcc0);a.move(3,30);a.value(4,1);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);if(!a.equal())return;
    a.value(3,0x10000);a.offset(0,3,0x6288);a.intersect_record(0,28,0);if(a.equal())return;
    a.load(3,30,0x90);a.compare(3,0,false);if(a.equal())return;a.load(0,3,0x1c);a.compare(0,0,false);if(a.equal())return;
    a.load<1>(21,3,0x69);a.sign8(21,21,true);if(a.equal()){a.value(20,0);a.jump(0x1037fe64);}else sample_response_range<20,true>(a,0x1037fd1c);
    a.at(0x1037fe64);a.compare(20,0);if(!a.equal()){
        a.load(3,30,0x90);a.load(3,3,0x1c);a.load<1>(21,3,0x34);a.sign8(21,21,true);
        if(a.equal()){a.value(20,0);a.jump(0x1037ffd0);}else sample_response_range<20>(a,0x1037fe88);
        a.at(0x1037ffd0);a.load<1>(0,27,0x4c);a.value(3,0x88890000);a.offset(3,3,-0x7777);a.sign8(0,0);a.multiply_high(3,3,0);a.add(0,3,0);a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(3,0,3);a.offset_record(21,3,10);
        if(a.equal()){a.value(22,0);a.jump(0x1038014c);}else sample_response_range<22>(a,0x10380004);
        a.at(0x1038014c);a.compare_registers(20,22);if(a.nonpositive())return;
    }
    a.at(0x10380154);a.load(0,27,0);a.value(5,1);a.load(4,31,0);a.multiply(0,0,0x70);a.load(3,30,0x90);a.add(4,4,0);a.call(0x100d9b80);
}
void compare_secondary_relationship_draw(Call& a) {
    a.at(0x10380170);a.value(3,0x20000);a.offset(0,3,1);a.intersect_record(0,28,0);if(a.equal())return;
    a.load(3,30,0x80);a.compare(3,0,false);if(a.equal())return;a.load(0,3,0x1c);a.compare(0,0,false);if(a.equal())return;
    a.load(3,26,0);a.load(0,25,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(26,0,3);if(!a.nonpositive()){a.store(26,0,24);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(23,0,0);}
    a.at(0x103801c0);a.load(5,26,0);a.value(4,0x88890000);a.load(3,30,0x80);a.offset(0,4,-0x7777);a.load<2>(5,5,0);a.load(4,23,0);a.load(6,3,0x1c);a.add(5,5,4);a.multiply_high(0,0,5);a.load<1>(4,6,0x36);a.sign8(6,4);a.add(0,0,5);a.shift_right(0,0,4);a.mask(4,0,1,false,1);a.add(0,0,4);a.multiply(0,0,30);a.subtract(0,0,5);a.compare_registers(6,0);
    if(!a.nonpositive()){a.load(0,27,0);a.value(5,1);a.load(4,31,0);a.multiply(0,0,0x70);a.add(4,4,0);a.call(0x100d9b80);}
}
}
void apply_review_conditions(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,apply_review_conditions_entry,d,o);a.link();a.multiple(20);a.move(29,5);a.load(31,2,-0x7be0);a.move(27,3);a.move(28,4);a.load(5,2,-0x7be8);a.load(21,2,-0x7b40);a.load(22,2,-0x4dac);a.load(23,2,-0x7b90);a.load(24,2,-0x7b8c);a.load(25,2,-0x7b88);a.load(26,2,-0x7b84);a.save_link();a.frame(128);
    a.load(0,3,0);a.load(4,3,4);a.multiply(0,0,0x70);a.load(3,31,0);a.load(5,5,0);a.add(3,3,0);a.load<1>(0,3,0x62);a.multiply(3,4,0x248);a.sign8(0,0);a.compare(0,11);a.add(30,5,3);bool active=a.equal();if(!active){a.compare(0,15);active=a.equal();if(!active){a.compare(0,13);active=a.equal();}}
    if(active){
        a.at(0x1037fafc);a.load(0,27,0x50);a.combine(0,0,28);a.store(27,0x50,0);notify_review_if_needed(a);
        a.at(0x1037fc0c);a.move(3,30);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
        if(!a.equal())adjust_primary_staff_trait(a);else compare_primary_relationship_draws(a);
        compare_secondary_relationship_draw(a);
        a.at(0x10380224);a.compare(29,0,false);if(!a.equal()){a.mask(0,28,0xe0000000,true);if(!a.equal()){a.load(0,27,0);a.move(4,29);a.load(3,31,0);a.move(5,28);a.multiply(0,0,0x70);a.add(3,3,0);a.call(0x100ddfc0);}}
    }
    a.at(0x10380250);a.pop(128);a.multiple(20,true);a.finish();
}
}
