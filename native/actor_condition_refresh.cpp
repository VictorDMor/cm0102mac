#include "actor_attributes.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::actor_attributes {
namespace {
using Call=SemanticGuestCall;
constexpr auto commit_phase=0x101880dcu;

void initialize_cached_date(Call& a,std::uint32_t start,int flag,unsigned date) {
    a.at(start);a.load<1>(0,2,flag);a.sign8(0,0,true);
    if(a.equal()) {a.move(3,date);a.call(0x10715880);a.value(0,1);a.store<1>(2,flag,0);}
}

void initialize_daily_cache(Call& a) {
    a.load<1>(0,2,-0x3947);a.sign8(0,0,true);
    if(a.equal()) {a.value(0,1);a.store(2,-0x3944,19);a.store<1>(2,-0x3947,0);}
    initialize_cached_date(a,0x10187aa8,-0x3948,14);
    initialize_cached_date(a,0x10187ac4,-0x3949,30);
    initialize_cached_date(a,0x10187ae0,-0x394a,21);
    a.at(0x10187afc);a.load<1>(0,2,-0x394c);a.sign8(0,0,true);
    if(a.equal()) {a.value(3,0);a.value(0,1);a.store<1>(2,-0x394b,3);a.store<1>(2,-0x394c,0);}
}

bool prepare_daily_update(Call& a) {
    a.at(0x10187b18);a.compare(31,0,false);if(a.equal())return false;
    if(!a.equal()) {a.load(3,2,-0x7be4);a.load(4,31,0);a.load(0,3,0);a.compare_registers(4,0);if(a.at_least())return false;}
    a.at(0x10187b38);a.load(0,31,0x14);a.compare(0,0,false);a.store(1,0x88,0);if(a.equal())return false;
    a.load(0,31,0);a.offset(20,28,2);a.load(5,18,0);a.value(4,0);a.multiply(3,0,0x24);
    a.load<2>(0,28,2);a.add(15,5,3);a.load(6,15,0);a.load(5,15,4);a.store(1,0x50,6);
    a.load<2>(3,1,0x52);a.store(1,0x38,6);a.compare_registers(3,0,false);a.store(1,0x3c,5);a.store(1,0x54,5);
    if(a.equal()) {a.load<2>(3,1,0x50,true);a.load<2>(0,28,0,true);a.compare_registers(3,0);if(a.equal())a.value(4,1);}
    a.at(0x10187b9c);a.compare(4,0);if(!a.equal())return false;
    a.load(5,1,0x38);a.move(4,28);a.load(6,1,0x3c);a.offset(3,1,0x60);a.call(0x10717dd0);
    a.load<2>(4,1,0x62);a.load(3,15,0x10);a.load(0,18,8);a.multiply(4,4,365);
    a.load<2>(5,1,0x60,true);a.compare_registers(3,0);a.add(17,5,4);
    bool invalid=a.positive();if(!invalid) {a.compare(3,-1);invalid=!a.at_least();}
    if(invalid) {a.at(0x10187be0);a.value(0,0xffffffff);a.store(15,0x10,0);}
    return true;
}

void partition_injury_days(Call& a) {
    a.multiply(0,0,0x38);a.load(5,18,4);a.move(4,28);a.offset(3,1,0x58);a.add(14,5,0);
    a.load(5,14,0x28);a.load(6,14,0x2c);a.call(0x10717dd0);
    a.load<2>(0,1,0x5a);a.load<2>(3,1,0x58,true);a.multiply(0,0,365);a.load<2>(4,14,0x32,true);
    a.add(5,3,0);a.compare_registers(5,4);
    if(!a.at_least()) {a.move(13,17);a.jump(commit_phase);return;}
    a.at(0x10187c38);a.load<2>(0,14,0x30,true);a.compare_registers(5,0);
    if(!a.at_least()) {
        a.add(0,5,4);a.compare_registers(17,0);
        if(!a.at_least()) {a.subtract(19,4,5);a.subtract(13,19,17);a.jump(commit_phase);}
        else {a.at(0x10187c5c);a.move(19,17);a.jump(commit_phase);}
        return;
    }
    a.at(0x10187c64);a.subtract(3,4,5);a.compare_registers(17,3);
    if(!a.nonpositive()) {a.subtract(16,0,17);a.subtract(19,16,3);a.add(0,16,19);a.subtract(13,0,17);a.jump(0x10187c9c);}
    else {
        a.at(0x10187c84);a.compare_registers(5,0);
        if(!a.at_least()) {a.subtract(16,0,5);a.subtract(19,16,17);a.jump(0x10187c9c);}
        else {a.at(0x10187c98);a.move(16,17);}
    }
    a.at(0x10187c9c);a.move(3,18);a.move(4,31);a.value(5,1);a.call(0x1018b9f0);a.jump(commit_phase);
}

void find_future_team_date(Call& a) {
    a.load<1>(0,31,0x6a);a.load(3,2,-0x7a64);a.mask(0,0,8);a.load(3,3,0);a.sign8(5,0);a.call(0x102ae9c0);
    a.load(4,31,0x20);a.move(5,3);a.load(3,2,-0x7a64);a.load(4,4,0x2c);a.load(3,3,0);a.call(0x102af650);
    a.load(4,3,0);a.load(0,3,4);a.store(1,0x48,4);a.load<2>(3,20,0);a.load<2>(4,1,0x4a);
    a.store(1,0x4c,0);a.compare_registers(4,3,false);
    if(!a.nonpositive()) {a.value(0,1);a.jump(0x10187d48);}
    else {
        a.at(0x10187d1c);
        if(!a.at_least()) {a.value(0,0);a.jump(0x10187d48);}
        else {
            a.at(0x10187d28);a.load<2>(4,1,0x48,true);a.load<2>(0,28,0,true);a.combine(0,4,0,false,true);
            a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);
        }
    }
    a.at(0x10187d48);a.sign8(0,0);
}

bool select_training_calendar(Call& a) {
    a.at(0x10187cb0);a.load(4,31,0x20);a.move(16,17);a.value(0,0);a.compare(4,0,false);
    if(!a.equal())find_future_team_date(a);
    a.at(0x10187d4c);a.load(3,31,0x24);a.compare(3,0,false);if(a.equal())return false;
    a.sign8(0,0,true);if(!a.equal())return false;
    a.load(0,2,-0x3944);a.compare_registers(3,0,false);bool update=!a.equal();
    if(!update) {
        a.load(0,14,0);a.value(5,0);a.load(6,14,4);a.store(1,0x40,0);a.load<2>(0,20,0);a.load<2>(4,1,0x42);
        a.store(1,0x44,6);a.compare_registers(4,0,false);
        if(a.equal()) {a.load<2>(4,1,0x40,true);a.load<2>(0,28,0,true);a.compare_registers(4,0);if(a.equal())a.value(5,1);}
        a.at(0x10187da4);a.compare(5,0);update=a.equal();
    }
    if(update) {
        a.at(0x10187dac);a.move(4,30);a.move(5,21);a.call(0x101929b0);a.sign8(3,3);a.load<2>(4,20,0);
        a.leading_zeroes(3,3);a.load<1>(0,28,4);a.mask(5,3,0x07ffffff,false,27);a.load<2>(3,28,0,true);
        a.store<1>(2,-0x394b,5);a.load(5,31,0x24);a.store<2>(14,2,4);a.store<2>(14,0,3);a.store(2,-0x3944,5);a.store<1>(14,4,0);
    }
    a.at(0x10187de8);a.load<1>(0,2,-0x394b);a.sign8(0,0,true);if(a.equal())return false;
    a.load(4,21,0);a.move(3,28);a.load(5,21,4);a.value(16,0);a.call(0x10718130);
    a.load(4,28,0);a.mask(18,3,0xffff);a.load(5,28,4);a.move(3,30);a.call(0x10718130);a.mask(14,3,0xffff);
    return true;
}

void advance_daily_random(Call& a,std::uint32_t start) {
    a.at(start);a.load(3,27,0);a.load(0,26,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(27,0,3);
    if(!a.nonpositive()) {a.store(27,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(24,0,0);}
    a.at(start+0x28);
}

void wide_training_draw(Call& a) {
    a.at(0x10187eb8);a.value(0,0x43300000);a.exclusive_flags(3,21,0x80000000);a.store(1,0x84,3);a.value(3,0x80010000);
    a.load(7,27,0);a.value(4,0x10000);a.store(1,0x80,0);a.offset(6,3,-0x7fff);a.floating_load(2,23,8);
    a.multiply(3,21,10);a.floating_load(1,1,0x80);a.offset(5,4,-1);a.floating_load(0,23,0);a.floating_subtract(1,1,2);
    a.multiply_high(4,6,3);a.load<2>(8,7,0);a.load(7,24,0);a.floating_difference(1,1,0,true);a.store(1,0x78,0);
    a.add(0,8,7);a.multiply_high(6,6,0);a.add(3,4,3);a.add(4,6,0);a.shift_right(4,4,15);a.mask(6,4,1,false,1);
    a.shift_right(3,3,15);a.add(4,4,6);a.product(5,4,5);a.mask(4,3,1,false,1);a.add(3,3,4);a.subtract(0,5,0);
    a.exclusive_flags(0,0,0x80000000);a.store(1,0x7c,0);a.floating_load(0,1,0x78);a.floating_subtract(0,0,2);
    a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x70);a.load(18,1,0x74);a.call(0x101864a0);
    a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,21,-1);a.multiply_high(3,4,3);
    a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(18,18,3);a.compare_registers(18,0);
    if(a.nonpositive()) {a.at(0x10187fb8);return;}
    a.move(18,0);a.jump(0x10187fb8);
}

void draw_training_increment(Call& a) {
    a.at(0x10187e4c);a.load<1>(0,31,0x67);a.sign8(0,0);a.multiply_high(3,30,0);a.mask(0,3,1,false,1);
    a.add(3,3,0);a.offset_record(21,3,2);
    if(a.equal()) {a.value(18,0);a.jump(0x10187fb8);return;}
    advance_daily_random(a,0x10187e70);
    a.value(3,0x10000);a.offset(0,3,-1);a.compare_registers(21,0);bool wide=a.positive();
    if(!wide) {a.value(3,0xffff0000);a.offset(0,3,1);a.compare_registers(21,0);wide=!a.at_least();}
    if(wide) {wide_training_draw(a);return;}
    a.at(0x10187f8c);a.load(3,27,0);a.compare(21,0);a.load(0,24,0);a.load<2>(3,3,0);a.add(3,3,0);
    a.divide(0,3,21);a.product(0,0,21);a.subtract(0,0,3);a.negate(18,0);
    if(!a.nonpositive())a.move(18,0);
}

void count_training_days(Call& a) {
    a.compare_registers(18,17);a.move(29,17);if(!a.at_least())a.move(29,18);
    a.at(0x10187e38);a.value(3,0x55550000);a.subtract(19,29,17);a.value(17,0);a.offset(30,3,0x5556);a.jump(0x10187fe0);
    for(;;) {
        a.at(0x10187fe0);a.compare_registers(17,29);if(a.at_least())break;
        a.load(0,31,0);a.load(4,22,0);a.multiply(3,0,100);a.offset(0,3,10);a.indexed_load<1>(0,4,0);a.compare(0,50,false);
        if(!a.less()) {a.jump(0x10188038);break;}
        draw_training_increment(a);
        a.at(0x10187fb8);a.compare(18,0);
        if(a.equal()) {a.load(0,31,0);a.load(5,22,0);a.multiply(3,0,100);a.offset(4,3,10);a.indexed_load<1>(3,5,4);a.offset(0,3,1);a.indexed_store<1>(0,5,4);}
        a.at(0x10187fdc);a.offset(17,17,1);
    }
}

void partition_training_days(Call& a) {
    a.compare_registers(18,14,false);
    if(!a.at_least())count_training_days(a);
    else {
        a.at(0x10188008);a.add(3,18,14);a.mask(0,3,1,false,1);a.add(0,0,3);a.shift_right(0,0,1);
        a.subtract(0,0,18,true);if(!a.at_least())a.value(0,0);
        a.at(0x10188024);a.compare_registers(17,0);a.move(19,0);if(!a.at_least())a.move(19,17);
        a.at(0x10188034);a.subtract(13,19,17);
    }
    a.at(0x10188038);a.compare(14,15,false);
    if(!a.at_least()) {
        a.subtract_from(16,14,15);a.subtract(19,16,19,true);if(a.at_least())return;
        a.value(19,0);a.jump(commit_phase);return;
    }
    a.at(0x10188054);a.load<2>(4,15,8,true);a.value(3,0x68dc0000);a.load<2>(0,15,10,true);a.offset(3,3,-0x7453);
    a.product(0,4,0);a.multiply_high(0,3,0);a.shift_right(0,0,12);a.mask(3,0,1,false,1);a.add(0,0,3);a.compare(0,8000);
    if(a.at_least())return;
    advance_daily_random(a,0x10188080);
    a.load(3,27,0);a.load(0,24,0);a.load<2>(3,3,0);a.add(0,3,0);a.mask(3,0,1,false,1);a.mask(0,0,1);
    a.combine(0,0,3,false,true);a.subtract(0,3,0);a.add(3,19,0);a.mask(0,3,1,false,1);a.add(0,0,3);a.shift_right(16,0,1);a.subtract(19,16,19);
}

void damp_high_condition(Call& a) {
    a.subtract(6,4,6);a.value(3,0x43300000);a.exclusive_flags(6,6,0x80000000);a.store(1,0x70,3);a.floating_load(1,23,8);
    a.store(1,0x74,6);a.floating_load(2,23,0x30);a.floating_load(0,1,0x70);a.floating_subtract(0,0,1,true);
    a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0x78);a.load(3,1,0x7c);a.sign16(3,3);a.add(6,4,3);a.jump(0x10188354);
}

void damp_middle_condition(Call& a) {
    a.subtract(3,4,6);a.value(7,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store(1,0x70,7);a.floating_load(2,23,8);
    a.store(1,0x74,3);a.floating_load(1,23,0x38);a.floating_load(0,1,0x70);a.floating_subtract(0,0,2,true);
    a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x78);a.load(3,1,0x7c);a.sign16(3,3);a.add(6,4,3);a.compare(6,9850);
    if(a.nonpositive())return;
    a.offset(3,6,-9850);a.store(1,0x70,7);a.exclusive_flags(3,3,0x80000000);a.floating_load(1,23,0x28);a.store(1,0x74,3);
    a.floating_load(0,1,0x70);a.floating_subtract(0,0,2,true);a.floating_product(0,1,0);a.floating_integer(0,0);
    a.floating_store(0,1,0x78);a.load(3,1,0x7c);a.sign16(3,3);a.offset(6,3,9850);a.jump(0x10188354);
}

void damp_lower_condition(Call& a) {
    a.compare(6,7000);if(a.nonpositive())return;
    a.offset(3,6,-7000);a.value(4,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store(1,0x70,4);a.floating_load(2,23,8);
    a.store(1,0x74,3);a.floating_load(1,23,0x40);a.floating_load(0,1,0x70);a.floating_subtract(0,0,2,true);
    a.floating_product(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x78);a.load(3,1,0x7c);a.sign16(3,3);a.offset(6,3,7000);a.compare(6,9000);
    if(!a.nonpositive()) {
        a.offset(3,6,-9000);a.store(1,0x80,4);a.exclusive_flags(3,3,0x80000000);a.floating_load(1,23,0x48);a.store(1,0x84,3);
        a.floating_load(0,1,0x80);a.floating_subtract(0,0,2,true);a.floating_product(0,1,0);a.floating_integer(0,0);
        a.floating_store(0,1,0x68);a.load(3,1,0x6c);a.sign16(3,3);a.offset(6,3,9000);
    }
    a.at(0x10188310);a.compare(6,9850);if(a.nonpositive())return;
    a.offset(4,6,-9850);a.value(3,0x43300000);a.exclusive_flags(4,4,0x80000000);a.store(1,0x68,3);a.floating_load(1,23,8);
    a.store(1,0x6c,4);a.floating_load(2,23,0x28);a.floating_load(0,1,0x68);a.floating_subtract(0,0,1,true);a.floating_product(0,2,0);
    a.floating_integer(0,0);a.floating_store(0,1,0x70);a.load(3,1,0x74);a.sign16(3,3);a.offset(6,3,9850);
}

void calculate_condition(Call& a) {
    a.at(commit_phase);a.load(3,1,0x88);a.load<1>(0,15,0x22);a.load<1>(5,3,0x34);a.load<2>(4,15,8,true);
    a.sign8(5,5);a.subtract(0,5,0);a.sign8(3,0,true);
    if(!a.at_least()) {a.value(0,0);a.jump(0x10188114);}
    else {a.at(0x10188104);a.compare(3,100);a.value(0,100);if(!a.positive())a.move(0,3);}
    a.at(0x10188114);a.sign8(0,0);a.value(3,0x66660000);a.multiply(7,0,0xfffffff4);a.offset(6,3,0x6667);
    a.multiply(0,5,25);a.offset(3,7,1000);a.sign16(3,3);a.mask(7,3,1,false,1);a.add(7,7,3);a.shift_right(7,7,1);
    a.multiply_high(0,6,0);a.sign16(7,7);a.shift_right(0,0,3);a.mask(6,0,1,false,1);a.add(6,0,6);a.offset(0,6,15);
    a.multiply(6,13,66);a.multiply(0,0,100);a.subtract(6,6,4);a.sign16(0,0);a.compare_registers(6,0);
    if(!a.at_least())a.value(6,4000);
    a.at(0x10188170);a.value(8,0x66660000);a.mask(5,5,0xfffffff0,false,4);a.offset(8,8,0x6667);a.multiply_high(5,8,5);
    a.shift_right(5,5,3);a.mask(8,5,1,false,1);a.add(5,5,8);a.offset(5,5,84);a.multiply(8,5,100);a.sign16(9,8);a.compare_registers(9,6);
    if(!a.at_least())a.sign16(9,6);
    a.at(0x101881a4);a.product(8,19,7);a.sign16(7,9);a.add(6,6,8);a.compare_registers(6,7);if(!a.nonpositive())a.move(6,7);
    a.at(0x101881bc);a.product(3,16,3);a.compare(4,9850);a.add(6,6,3);
    if(!a.nonpositive())damp_high_condition(a);
    else {a.at(0x1018820c);a.compare(4,9000);if(!a.nonpositive())damp_middle_condition(a);else {a.at(0x10188290);damp_lower_condition(a);}}
    a.at(0x10188354);a.compare(6,10000);if(!a.nonpositive())a.value(6,10000);
    a.at(0x10188360);a.multiply(3,29,55);a.subtract(6,3,6);a.compare_registers(6,0);if(!a.at_least())a.value(6,5000);
    a.at(0x10188374);a.sign16(0,5);a.compare_registers(0,6);if(!a.at_least())a.sign16(0,6);
    a.at(0x10188384);a.sign16(0,0);a.compare_registers(6,0);if(!a.nonpositive())a.move(6,0);
}

void restore_record_capacity(Call& a) {
    a.at(0x10188394);a.load<2>(0,15,12,true);a.load<2>(5,15,10,true);a.compare(0,1000);a.floating_load(3,23,0x50);
    if(!a.nonpositive()) {
        a.exclusive_flags(3,0,0x80000000);a.value(0,0x43300000);a.store(1,0x6c,3);a.floating_load(2,23,8);
        a.store(1,0x68,0);a.floating_load(3,23,0x60);a.floating_load(1,1,0x68);a.floating_load(0,23,0x58);
        a.floating_subtract(1,1,2);a.floating_negative_product_difference(3,3,1,0);
    }
    a.at(0x101883d0);a.floating_load(0,23,0x68);a.floating_compare(3,0);
    if(!a.nonpositive()) {a.compare(19,0);if(!a.nonpositive())a.floating_move(3,0);}
    a.at(0x101883e8);a.multiply(4,19,250);a.value(3,0x43300000);a.store(1,0x68,3);a.floating_load(2,23,8);
    a.multiply(0,16,500);a.store(1,0x78,3);a.exclusive_flags(3,4,0x80000000);a.store(1,0x6c,3);
    a.exclusive_flags(0,0,0x80000000);a.store(1,0x7c,0);a.floating_load(1,1,0x68);a.floating_load(0,1,0x78);
    a.floating_subtract(1,1,2);a.floating_subtract(0,0,2);a.floating_product(1,3,1);a.floating_product(0,3,0);
    a.floating_integer(1,1);a.floating_integer(0,0);a.floating_store(1,1,0x70);a.floating_store(0,1,0x80);a.load(3,1,0x74);a.load(0,1,0x84);
    a.add(5,5,3);a.add(5,5,0);a.compare(5,10000);if(!a.nonpositive())a.value(5,10000);
    a.at(0x10188454);a.store<2>(15,8,6);a.store<2>(15,10,5);a.load<2>(0,20,0);a.store<2>(15,2,0);
    a.load<2>(0,28,0,true);a.store<2>(15,0,0);a.load<1>(0,28,4);a.store<1>(15,4,0);
}
}

void refresh_condition_record(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,record_refresh_entry,dependencies,observer);
    a.link();a.multiple(13);a.load(21,2,-0x5450);a.move(18,3);a.load(30,2,-0x544c);a.move(31,4);
    a.load(14,2,-0x5448);a.value(13,0);a.load(22,2,-0x7b9c);a.value(29,0);a.load(23,2,-0x543c);
    a.value(19,0);a.load(24,2,-0x7b90);a.value(16,0);a.load(25,2,-0x7b8c);a.load(26,2,-0x7b88);
    a.load(27,2,-0x7b84);a.load(28,2,-0x7b80);a.save_link();a.frame(224);
    initialize_daily_cache(a);
    if(prepare_daily_update(a)) {
        a.at(0x10187be8);a.load(0,15,0x10);a.compare(0,-1);
        if(!a.equal())partition_injury_days(a);
        else if(select_training_calendar(a))partition_training_days(a);
        calculate_condition(a);restore_record_capacity(a);
    }
    a.at(0x10188474);a.pop(224);a.multiple(13,true);a.finish();
}
}
