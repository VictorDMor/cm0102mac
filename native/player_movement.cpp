#include "player_movement.hpp"
#include "semantic_guest_call.hpp"

// Whole movement coordinator support. These named continuation phases preserve the
// original frame, live reads and dependency ABI; no original-body replay is used.
namespace cm::aot::player_movement {
namespace {
using Call=SemanticGuestCall;

void reserve_movement_context(Call& a) {
    a.link();a.floating_store(31,1,-8);a.floating_store(30,1,-16);a.multiple(13,false,-16);
    a.move(16,4);a.move(15,3);a.value(4,0);a.load(31,2,-0x51e0);a.move(17,5);a.load(26,2,-0x51e8);
    a.move(18,6);a.load(27,2,-0x7b90);a.move(19,7);a.load(28,2,-0x7b8c);a.move(20,8);a.load(29,2,-0x7b88);
    a.value(13,0);a.load(30,2,-0x7b84);a.load(14,2,-0x51e4);a.save_link();a.value(0,0);a.frame(0x180);
    a.store<2>(1,0x48,0);a.load<1>(0,3,0x105);a.sign8(3,16);a.store<1>(1,0x116,0);
    a.load<1>(0,15,0x106);a.store<1>(1,0x115,0);a.load<1>(0,1,0x116);a.store<1>(1,0x40,4);
    a.sign8(0,0);a.compare_registers(3,0);a.load<1>(3,15,0x2c);a.offset(0,3,1);a.store<1>(15,0x2c,0);
    a.value(0,0);a.store<1>(1,0x114,0);
}

// The stationary and turning paths share the original neighbour decision,
// including its volatile register schedule and the 900-value RNG interval.
void react_stationary_neighbours(Call& a,std::uint32_t loop) {
    while(true) {
        a.at(loop+0xbc);a.load<1>(0,15,0x104);a.sign8(3,13);a.sign8(0,0);a.compare_registers(3,0);
        if(!a.less())break;
        a.at(loop);a.sign8(0,13);a.load(3,15,0x1a4);a.mask(0,0,0xfffffffc,false,2);a.load<1>(5,15,0x105);
        a.add(4,15,0);a.load<1>(6,15,0x106);a.load(4,4,0x1b4);a.call(0x101d8120);a.sign8(0,3,true);
        if(!a.equal()) {
            a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.load<1>(14,15,0x107);a.load<1>(16,15,0x10a);
            a.compare_registers(3,0,false);a.store(30,0,3);
            if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
            a.at(loop+0x58);a.load(3,30,0);a.value(4,0x91a30000);a.load(5,27,0);a.offset(4,4,-0x4c3b);a.load<2>(6,3,0);
            a.sign8(3,14);a.sign8(0,16);a.add(6,6,5);a.multiply_high(4,4,6);a.add(4,4,6);a.shift_right(4,4,9);
            a.mask(5,4,1,false,1);a.add(4,4,5);a.multiply(4,4,900);a.product(3,3,0);a.subtract(4,4,6);a.offset(0,4,50);
            a.compare_registers(3,0);
            if(!a.positive()){a.sign8(0,13);a.mask(0,0,0xfffffffc,false,2);a.add(3,15,0);a.load(3,3,0x1b4);a.call(0x102836a0);}
        }
        a.at(loop+0xb8);a.offset(13,13,1);
    }
}

void approach_requested_cell(Call& a,std::uint32_t start) {
    a.at(start);a.sign8(3,23);a.sign8(0,16);a.compare_registers(3,0);
    if(!a.at_least()){a.offset(23,23,1);a.jump(start+0x2c);}
    else {a.at(start+0x18);a.sign8(3,23);a.sign8(0,16);a.compare_registers(3,0);if(!a.nonpositive())a.offset(23,23,-1);}
    a.at(start+0x2c);a.sign8(3,24);a.sign8(0,17);a.compare_registers(3,0);
    if(!a.at_least()){a.offset(24,24,1);a.jump(start+0x58);}
    else {a.at(start+0x44);a.sign8(3,24);a.sign8(0,17);a.compare_registers(3,0);if(!a.nonpositive())a.offset(24,24,-1);}
}

Preparation prepare_movement_context(Call& a) {
    reserve_movement_context(a);
    bool unchanged=a.equal();
    if(unchanged){a.load<1>(0,1,0x115);a.sign8(3,17);a.sign8(0,0);a.compare_registers(3,0);unchanged=a.equal();}
    if(unchanged){a.sign16(0,18);a.compare(0,15);unchanged=!a.equal();}
    if(unchanged){a.sign16(0,18);a.compare(0,256);unchanged=!a.equal();}
    if(unchanged){a.value(13,0);a.jump(0x1022e968);react_stationary_neighbours(a,0x1022e8ac);a.jump(0x10235d88);return Preparation::returned;}
    a.at(0x1022e980);a.move(3,15);a.call(0x102775c0);a.sign8(0,3,true);
    if(a.equal()){a.load(3,15,0x1a4);a.move(4,15);a.move(5,18);a.value(6,0);a.value(7,0xffffffff);a.value(8,1);a.call(0x101f5350);}
    a.at(0x1022e9ac);a.move(3,15);a.call(0x102775c0);a.sign8(0,3,true);
    bool must_turn=a.equal();
    if(must_turn){a.move(3,15);a.move(4,16);a.move(5,17);a.call(0x10245050);a.sign8(0,3,true);must_turn=a.equal();}
    for(auto offset:{0x10f,0x10d,0x108})if(must_turn){a.load<1>(0,15,offset);a.sign8(0,0,true);must_turn=a.equal();}
    if(must_turn){a.load<1>(4,15,0x105);a.move(3,15);a.load<1>(5,15,0x106);a.move(6,16);a.move(7,17);a.call(0x10237830);
        a.value(13,0);a.jump(0x1022ead4);react_stationary_neighbours(a,0x1022ea18);a.value(0,111);a.store<2>(15,0x19c,0);
        a.jump(0x10235d88);return Preparation::returned;}
    a.at(0x1022eaf4);a.load<1>(23,15,0x105);a.sign8(0,16);a.load<1>(24,15,0x106);
    // The first x comparison reuses the target sign-extension above.
    a.sign8(3,23);a.compare_registers(3,0);
    if(!a.at_least()){a.offset(23,23,1);a.jump(0x1022eb28);}
    else {a.at(0x1022eb14);a.sign8(3,23);a.sign8(0,16);a.compare_registers(3,0);if(!a.nonpositive())a.offset(23,23,-1);}
    a.at(0x1022eb28);a.sign8(3,24);a.sign8(0,17);a.compare_registers(3,0);
    if(!a.at_least()){a.offset(24,24,1);a.jump(0x1022eb54);}
    else {a.at(0x1022eb40);a.sign8(3,24);a.sign8(0,17);a.compare_registers(3,0);if(!a.nonpositive())a.offset(24,24,-1);}
    a.at(0x1022eb54);a.move(3,15);a.call(0x102775c0);a.sign8(0,3,true);
    if(a.equal()) {
        a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.load<1>(21,15,0x107);a.load<1>(22,15,0x107);
        a.compare_registers(3,0,false);a.store(30,0,3);
        if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
        a.at(0x1022eb94);a.load(3,30,0);a.value(4,0x10620000);a.load(5,27,0);a.offset(4,4,0x4dd3);a.load<2>(6,3,0);
        a.sign8(3,21);a.sign8(0,22);a.add(6,6,5);a.multiply_high(4,4,6);a.shift_right(4,4,6);a.mask(5,4,1,false,1);
        a.add(4,4,5);a.multiply(4,4,1000);a.product(0,3,0);a.subtract(4,4,6);a.compare_registers(0,4);
        if(!a.nonpositive())approach_requested_cell(a,0x1022ebd8);
    }
    a.at(0x1022ec30);a.move(16,23);a.move(17,24);a.move(3,15);a.call(0x102775c0);a.sign8(0,3,true);
    return a.equal()?Preparation::noncarrier:Preparation::carrier;
}

void migrate_noncarrier_cell(Call& a) {
    a.at(0x10234e68);a.load<1>(0,15,0x28);a.sign8(0,0);a.compare(0,-1);
    if(!a.nonpositive()) {
        a.load<1>(5,15,0x105);a.sign8(3,16);a.load<1>(4,15,0x106);a.sign8(0,17);a.sign8(5,5);a.load<1>(6,15,0x27);
        a.sign8(8,4);a.load<1>(4,15,0x27);a.multiply(11,5,0x450);a.load<1>(5,15,0x27);a.sign8(9,4);a.load<1>(4,15,0x28);
        a.load(12,15,0x1a4);a.sign8(7,5);a.multiply(10,8,0x5c);a.sign8(8,6);a.sign8(5,4);a.load(4,15,0x1a4);
        a.add(10,11,10);a.mask(6,5,0xfffffffc,false,2);a.offset(5,10,0xa08);a.add(5,12,5);a.add(8,5,8);
        a.multiply(9,9,0x2c);a.load<1>(8,8,0x58);a.sign8(8,8);a.multiply(7,7,0x2c);a.add(9,5,9);a.mask(8,8,0xfffffffc,false,2);
        a.add(8,9,8);a.load(8,8,-4);a.add(6,7,6);a.multiply(9,3,0x450);a.indexed_store(8,5,6);a.load<1>(3,15,0x27);
        a.multiply(8,0,0x5c);a.sign8(0,3);a.add(7,5,0);a.load<1>(6,7,0x58);a.add(3,9,8);a.offset(3,3,0xa08);
        a.offset(0,6,-1);a.store<1>(7,0x58,0);a.add(3,4,3);a.load<1>(4,15,0x27);a.load<1>(0,15,0x28);a.sign8(4,4);
        a.sign8(0,0);a.multiply(4,4,0x2c);a.mask(0,0,0xfffffffc,false,2);a.add(0,4,0);a.indexed_load(0,5,0);a.compare(0,0,false);
        if(!a.equal()) {
            a.load<1>(4,15,0x27);a.load<1>(0,15,0x28);a.sign8(4,4);a.load<1>(6,15,0x28);a.multiply(4,4,0x2c);
            a.sign8(0,0);a.mask(0,0,0xfffffffc,false,2);a.add(0,4,0);a.indexed_load(4,5,0);a.store<1>(4,0x28,6);
        }
        a.at(0x10234f78);a.load<1>(0,15,0x27);a.sign8(0,0);a.add(4,3,0);a.load<1>(0,4,0x58);a.store<1>(15,0x28,0);
        a.load<1>(0,15,0x27);a.sign8(0,0);a.add(5,3,0);a.load<1>(4,5,0x58);a.offset(0,4,1);a.store<1>(5,0x58,0);
        a.load<1>(4,15,0x27);a.load<1>(0,15,0x28);a.sign8(4,4);a.multiply(4,4,0x2c);a.sign8(0,0);a.mask(0,0,0xfffffffc,false,2);
        a.add(0,4,0);a.indexed_store(15,3,0);a.jump(0x10235034);
    } else {
        a.at(0x10234fc8);a.sign8(0,16);a.sign8(3,17);a.multiply(4,0,0x450);a.load<1>(0,15,0x27);a.load(5,15,0x1a4);
        a.sign8(0,0);a.multiply(3,3,0x5c);a.add(3,4,3);a.offset(3,3,0xa08);a.add(3,5,3);a.add(4,3,0);a.load<1>(0,4,0x58);
        a.store<1>(15,0x28,0);a.load<1>(0,15,0x27);a.sign8(0,0);a.add(5,3,0);a.load<1>(4,5,0x58);a.offset(0,4,1);
        a.store<1>(5,0x58,0);a.load<1>(4,15,0x27);a.load<1>(0,15,0x28);a.sign8(4,4);a.sign8(0,0);a.multiply(4,4,0x2c);
        a.mask(0,0,0xfffffffc,false,2);a.add(0,4,0);a.indexed_store(15,3,0);
    }
}

void maintain_defensive_extent(Call& a) {
    a.at(0x10235034);a.store<1>(15,0x105,16);a.store<1>(15,0x106,17);a.load<1>(0,15,0x27);a.sign8(0,0,true);
    bool done=false;
    if(a.equal()) {
        a.load(4,15,0x1a4);a.offset(3,4,0x10000);a.load(0,3,-0x1fec);a.compare(0,0,false);
        if(!a.equal()) {a.offset(3,4,0x10000);a.load(3,3,-0x1fec);a.load<1>(3,3,0x106);a.jump(0x10235070);}
        else {a.at(0x1023506c);a.value(3,0xffffffff);}
        a.at(0x10235070);a.load<1>(0,15,0x106);a.sign8(3,3);a.sign8(0,0);a.compare_registers(3,0);
        if(!a.at_least()) {a.load(3,15,0x1a4);a.offset(3,3,0x10000);a.store(3,-0x1fec,15);a.jump(0x102350e8);done=true;}
    }
    if(!done) {
        a.at(0x10235094);a.load<1>(0,15,0x27);a.compare(0,1);
        if(a.equal()) {
            a.load(4,15,0x1a4);a.offset(3,4,0x10000);a.load(0,3,-0x1fe8);a.compare(0,0,false);
            if(!a.equal()) {a.offset(3,4,0x10000);a.load(3,3,-0x1fe8);a.load<1>(3,3,0x106);a.jump(0x102350c8);}
            else {a.at(0x102350c4);a.value(3,9);}
            a.at(0x102350c8);a.load<1>(0,15,0x106);a.sign8(3,3);a.sign8(0,0);a.compare_registers(3,0);
            if(!a.nonpositive()) {a.load(3,15,0x1a4);a.offset(3,3,0x10000);a.store(3,-0x1fe8,15);}
        }
    }
}

void react_neighbours(Call& a) {
    a.at(0x102350e8);a.value(18,0);a.jump(0x102351ac);
    while(true) {
        a.at(0x102351ac);a.load<1>(0,15,0x104);a.sign8(3,18);a.sign8(0,0);a.compare_registers(3,0);
        if(!a.less())break;
        a.at(0x102350f0);a.sign8(0,18);a.load(3,15,0x1a4);a.mask(0,0,0xfffffffc,false,2);a.load<1>(5,15,0x105);
        a.add(4,15,0);a.load<1>(6,15,0x106);a.load(4,4,0x1b4);a.call(0x101d8120);a.sign8(0,3,true);
        if(!a.equal()) {
            a.load(3,30,0);a.load(0,29,0);a.offset(3,3,2);a.load<1>(13,15,0x107);a.load<1>(14,15,0x10a);
            a.compare_registers(3,0,false);a.store(30,0,3);
            if(!a.nonpositive()){a.store(30,0,28);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(27,0,0);}
            a.at(0x10235148);a.load(3,30,0);a.value(4,0x91a30000);a.load(5,27,0);a.offset(4,4,-0x4c3b);a.load<2>(6,3,0);
            a.sign8(3,13);a.sign8(0,14);a.add(6,6,5);a.multiply_high(4,4,6);a.add(4,4,6);a.shift_right(4,4,9);
            a.mask(5,4,1,false,1);a.add(4,4,5);a.multiply(4,4,900);a.product(3,3,0);a.subtract(4,4,6);a.offset(0,4,50);
            a.compare_registers(3,0);
            if(!a.positive()){a.sign8(0,18);a.mask(0,0,0xfffffffc,false,2);a.add(3,15,0);a.load(3,3,0x1b4);a.call(0x102836a0);}
        }
        a.at(0x102351a8);a.offset(18,18,1);
    }
}

void select_surface_pace(Call& a) {
    a.at(0x102351cc);a.load(3,15,0x1a4);a.load<1>(0,3,1);a.sign8(0,0);a.compare(0,8);
    std::uint32_t selected;
    if(a.equal())selected=0x10235248;
    else if(!a.at_least()) {
        a.compare(0,3);
        if(a.equal())selected=0x10235268;
        else if(!a.at_least()){a.compare(0,1);if(a.equal())selected=0x10235230;else if(a.at_least())selected=0x10235238;else {a.jump(0x10235268);selected=0x10235268;}}
        else {a.at(0x10235200);a.compare(0,5);if(a.at_least())selected=0x10235268;else {a.jump(0x10235240);selected=0x10235240;}}
    } else {
        a.at(0x1023520c);a.compare(0,32);
        if(a.equal())selected=0x10235258;
        else if(!a.at_least()){a.compare(0,16);if(a.equal())selected=0x10235250;else {a.jump(0x10235268);selected=0x10235268;}}
        else {a.at(0x10235224);a.compare(0,64);if(a.equal())selected=0x10235260;else {a.jump(0x10235268);selected=0x10235268;}}
    }
    a.at(selected);
    const auto offset=selected==0x10235230?0x98:selected==0x10235238?0x9c:selected==0x10235240?0x18:
        selected==0x10235248?0xa0:selected==0x10235250?0x18:selected==0x10235258?0xa4:selected==0x10235260?0x78:0x34;
    a.floating_load(31,31,offset,true);if(selected!=0x10235268)a.jump(0x1023526c);
    a.at(0x1023526c);a.load(3,15,0x1a4);a.load<1>(0,3,0x30d8);a.sign8(0,0);a.compare(0,3);
    if(a.equal()){a.at(0x10235294);a.floating_load(0,31,0x18,true);a.floating_product_single(31,31,0);a.jump(0x102352a8);}
    else if(a.at_least()){a.at(0x10235288);a.compare(0,5);if(!a.at_least()){a.jump(0x102352a0);a.floating_load(0,31,0x78,true);a.floating_product_single(31,31,0);}}
    else a.jump(0x102352a8);
}

void apply_role_pace(Call& a) {
    a.at(0x102352a8);a.floating_load(2,15,0xd8,true);a.floating_load(1,26,0x28);a.floating_load(0,26,0xd0);a.load<1>(0,1,0x116);
    a.floating_product_sum(1,2,1,0);a.floating_load(0,26,0x48);a.sign8(3,0);a.load<1>(0,15,0x105);a.floating_load(2,26,0x18);
    a.sign8(0,0);a.floating_round_single(1,1);a.floating_load(3,26,0xc8);a.subtract(3,3,0);a.floating_difference(0,1,0,true);
    a.floating_subtract(0,2,0);a.floating_sum(30,3,0);a.floating_round_single(30,30);a.call(0x10775540);
    a.load<1>(0,1,0x115);a.sign8(13,3);a.load<1>(4,15,0x106);a.sign8(5,0);a.sign8(0,4);a.subtract(3,5,0);a.call(0x10775540);
    a.sign8(3,3);a.sign8(0,13);a.multiply(4,3,36);a.load(5,2,-0x6424);a.mask(0,0,0xfffffffc,false,2);a.move(3,15);
    a.add(0,4,0);a.floating_indexed_single(0,5,0);a.floating_product_single(0,30,0);a.floating_product_single(30,31,0);a.call(0x10289560);
    a.move(0,3);a.move(3,15);a.sign16(13,0);a.call(0x102895e0);a.sign16(0,3);a.compare_registers(0,13);
    if(!a.equal()) {
        a.move(3,15);a.call(0x1028ac50);a.sign8(0,3);a.compare(0,1);
        if(!a.nonpositive()) {a.floating_load(1,15,0xd8,true);a.floating_load(0,26,0x38);a.floating_load(2,26,0x168);
            a.floating_difference(0,1,0,true);a.floating_subtract(0,2,0);a.floating_round_single(0,0);a.floating_product_single(30,30,0);a.jump(0x102353e4);}
        else {
            a.at(0x10235388);a.move(3,15);a.call(0x10289560);a.mask(0,3,4,true);bool role=!a.equal();
            if(!role){a.move(3,15);a.call(0x102895e0);a.mask(0,3,4,true);role=!a.equal();}
            a.at(role?0x102353a8:0x102353c8);a.floating_load(1,15,0xd8,true);a.floating_load(0,26,0x38);a.floating_load(2,26,role?0x290:0x190);
            a.floating_difference(0,1,0,true);a.floating_subtract(0,2,0);a.floating_round_single(0,0);a.floating_product_single(30,30,0);
            if(role)a.jump(0x102353e4);
        }
    }
}

void read_pace_tactics(Call& a,std::uint32_t start) {
    a.at(start);a.load<1>(0,15,0x27);a.load(5,15,0x1a4);a.sign8(0,0);a.load<1>(4,15,0x19);
    a.multiply(3,0,0x18e0);a.offset(3,3,0x7c6c);a.add(3,5,3);a.call(0x10113e10);
}
void consume_noncarrier_condition(Call& a) {
    a.at(0x102351c0);a.load<1>(0,15,0x19);a.sign8(0,0,true);if(a.less())return;
    select_surface_pace(a);apply_role_pace(a);read_pace_tactics(a,0x102353e4);
    a.value(6,0x10000);a.value(7,0);a.offset(6,6,-0x8000);a.value(5,0);a.intersect(6,4,6);a.intersect(4,3,7);
    a.value(0,0);a.combine(3,6,5,false,true);a.combine(0,4,0,false,true);a.combine(0,3,0,true);bool extra=!a.equal();
    if(!extra) {
        read_pace_tactics(a,0x10235430);a.value(5,0x2000);a.value(0,0);a.intersect(6,4,5);a.value(4,0);a.intersect(5,3,0);
        a.value(0,0);a.combine(3,6,4,false,true);a.combine(0,5,0,false,true);a.combine(0,3,0,true);extra=!a.equal();
        if(extra){a.load<1>(0,15,0x27);a.load(4,15,0x1a4);a.sign8(0,0);a.multiply(3,0,0x18e0);a.offset(3,3,0x7c6c);
            a.add(3,4,3);a.load(0,3,0x588);a.mask(0,0,0x100,true);extra=!a.equal();}
    }
    if(extra){a.at(0x1023549c);a.floating_load(1,15,0xd8,true);a.floating_load(0,26,0x2a0);a.floating_load(2,26,0x298);
        a.floating_difference(0,1,0,true);a.floating_subtract(0,2,0);a.floating_round_single(0,0);a.floating_product_single(30,30,0);}
    a.at(0x102354b8);a.floating_move(1,30);a.move(3,15);a.value(5,0xffffffff);a.call(0x10239590);
}

void queue_capture_commentary(Call& a,std::uint32_t start,unsigned code) {
    a.at(start);a.load(3,15,0x1a4);a.value(4,code);a.load<1>(7,15,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);
    a.value(9,0xffffffff);a.load<1>(6,15,0x2f);a.value(10,0);a.load(5,15,0x1a4);a.call(0x10201180);
}
bool goal_area_capture(Call& a) {
    a.at(0x10235698);a.load(3,15,0x1a4);a.load<1>(6,15,0x106);a.load<1>(0,3,0x1e6);a.load<1>(5,15,0x105);
    a.sign8(0,0,true);a.load<1>(4,15,0x27);bool inside=!a.equal();
    if(inside){a.sign8(0,4,true);inside=a.equal();}
    if(inside){a.sign8(0,6);a.compare(0,10);inside=!a.less();}
    if(inside){a.sign8(0,5);a.compare(0,5);inside=!a.positive();}
    if(inside){a.sign8(0,5);a.compare(0,3);inside=!a.less();}
    if(inside){a.value(0,1);a.jump(0x10235730);}
    else {
        a.at(0x102356e8);a.load<1>(0,3,0x1e6);a.sign8(0,0,true);inside=!a.equal();
        if(inside){a.sign8(0,4);a.compare(0,1);inside=a.equal();}
        if(inside){a.sign8(0,6);a.compare(0,1);inside=a.nonpositive();}
        if(inside){a.sign8(0,5);a.compare(0,5);inside=!a.positive();}
        if(inside){a.sign8(0,5);a.compare(0,3);inside=!a.less();}
        if(inside){a.value(0,1);a.jump(0x10235730);}else {a.at(0x1023572c);a.value(0,0);}
    }
    a.at(0x10235730);a.sign8(0,0,true);return !a.equal();
}

void capture_unowned_ball(Call& a) {
    a.at(0x10235554);a.load(3,15,0x1a4);a.load<1>(0,3,0x791f);a.sign8(0,0);a.compare(0,5);if(a.at_least())return;
    a.load(3,15,0x1a4);a.load<1>(0,3,0x1de);a.sign8(0,0,true);if(!a.equal())return;
    a.load(3,15,0x1a4);a.load<1>(0,3,0x7924);a.sign8(0,0);a.compare(0,-1);if(!a.equal())return;
    a.load(3,15,0x1a4);a.load<1>(0,15,0x105);a.load<1>(4,3,0x791d);a.sign8(3,0);a.sign8(0,4);a.compare_registers(3,0);if(!a.equal())return;
    a.load(3,15,0x1a4);a.load<1>(0,15,0x106);a.load<1>(4,3,0x791e);a.sign8(3,0);a.sign8(0,4);a.compare_registers(3,0);if(!a.equal())return;
    for(auto offset:{-0x2008,-0x200c}){a.load(3,15,0x1a4);a.offset(3,3,0x10000);a.load(0,3,offset);a.compare_registers(15,0,false);if(a.equal())return;}
    a.load(3,15,0x1a4);a.offset(3,3,0x10000);a.load(0,3,-0x2008);a.compare(0,0,false);
    if(!a.equal()) {
        a.load(3,15,0x1a4);a.offset(3,3,0x10000);a.load(3,3,-0x2008);a.call(0x10283690);a.load<1>(0,15,0x27);
        a.sign8(3,3);a.sign8(0,0);a.compare_registers(0,3);if(a.equal())return;
        queue_capture_commentary(a,0x10235624,0x1f60);a.move(3,15);a.call(0x1022e3a0);a.load<2>(3,15,0x36,true);a.offset(0,3,28);
        a.store<2>(15,0x36,0);a.jump(0x102357ac);return;
    }
    a.at(0x10235664);a.load<1>(5,15,0x27);a.value(0,11);a.load<1>(4,15,0x105);a.move(3,15);a.sign8(6,5);
    a.load<1>(5,15,0x106);a.leading_zeroes(7,6);a.value(6,4);a.mask(7,7,1,false,27);a.negate(7,7);a.intersect(0,0,7);
    a.sign8(7,0);a.call(0x10237830);
    if(goal_area_capture(a)){queue_capture_commentary(a,0x10235738,0x2113);a.jump(0x1023578c);}
    else queue_capture_commentary(a,0x10235764,0x1f46);
    a.at(0x1023578c);a.load(4,15,0x1a4);a.value(0,0);a.move(3,15);a.store<1>(4,0x791f,0);a.call(0x1022e3a0);
    a.load<2>(3,15,0x36,true);a.offset(0,3,4);a.store<2>(15,0x36,0);
}

void synchronize_noncarrier_ball(Call& a) {
    a.at(0x102354c8);a.move(3,15);a.call(0x102775c0);a.sign8(0,3,true);bool owns=!a.equal();
    if(!owns){a.load<1>(0,1,0x114);a.sign8(0,0,true);owns=!a.equal();}
    if(!owns){capture_unowned_ball(a);return;}
    a.at(0x102354e4);a.load<1>(0,15,0x105);a.load(3,15,0x1a4);a.store<1>(3,0x791d,0);
    a.load<1>(0,15,0x106);a.load(3,15,0x1a4);a.store<1>(3,0x791e,0);
    a.load(3,15,0x1a4);a.load<1>(0,3,0x791d);a.sign8(0,0,true);bool invalid=a.less();
    if(!invalid){a.load(3,15,0x1a4);a.load<1>(0,3,0x791e);a.sign8(0,0,true);invalid=a.less();}
    if(!invalid){a.load(3,15,0x1a4);a.load<1>(0,3,0x791e);a.sign8(0,0);a.compare(0,12);invalid=a.at_least();}
    if(!invalid){a.load(3,15,0x1a4);a.load<1>(0,3,0x791d);a.sign8(0,0);a.compare(0,9);invalid=!a.less();}
    if(invalid){a.at(0x10235544);a.load(3,15,0x1a4);a.load<1>(4,15,0x27);a.call(0x101f0c00);a.jump(0x102357ac);}
}

// Two distinct original ABI schedules calculate the same weighted focus.
// Keep their load/register ordering while sharing the named facing operation.
void face_near_ball(Call& a,std::uint32_t start,bool toward_far_goal) {
    a.at(start);
    if(toward_far_goal) {
        a.load(4,15,0x1a4);a.value(9,0x55550000);a.load(3,15,0x1a4);a.value(7,0x55550000);
        a.load<1>(0,4,0x791e);a.sign8(5,16);a.load<1>(4,3,0x791d);a.sign8(3,17);a.sign8(0,0);a.offset(9,9,0x5556);
        a.sign8(4,4);a.offset(7,7,0x5556);a.add(8,5,4);a.add(6,3,0);a.offset(0,6,11);a.load<1>(4,15,0x105);
        a.offset(8,8,4);a.load<1>(5,15,0x106);a.multiply_high(6,9,8);a.move(3,15);a.multiply_high(7,7,0);
        a.mask(0,6,1,false,1);a.add(6,6,0);a.mask(0,7,1,false,1);a.sign8(6,6);a.add(0,7,0);a.sign8(7,0);
    } else {
        a.load(3,15,0x1a4);a.value(8,0x55550000);a.load(4,15,0x1a4);a.value(6,0x55550000);
        a.load<1>(0,3,0x791d);a.sign8(5,16);a.load<1>(7,4,0x791e);a.sign8(3,17);a.sign8(0,0);a.offset(8,8,0x5556);
        a.add(5,5,0);a.offset(6,6,0x5556);a.sign8(0,7);a.load<1>(4,15,0x105);a.offset(7,5,4);a.load<1>(5,15,0x106);
        a.add(0,3,0);a.move(3,15);a.multiply_high(8,8,7);a.multiply_high(7,6,0);a.mask(0,8,1,false,1);a.add(6,8,0);
        a.mask(0,7,1,false,1);a.sign8(6,6);a.add(0,7,0);a.sign8(7,0);
    }
    a.call(0x10237830);if(start!=0x10235d1c)a.jump(0x10235d88);
}

void face_noncarrier_context(Call& a) {
    a.at(0x102357ac);a.load<1>(0,15,0x27);a.sign8(0,0,true);const bool away=!a.equal();
    a.at(away?0x10235aa4:0x102357b8);a.move(3,15);a.call(0x102775c0);a.sign8(0,3,true);if(!a.equal())return;
    a.load<1>(0,15,0x27);a.load(3,15,0x1a4);a.sign8(0,0);a.leading_zeroes(0,0);a.load<1>(4,3,0x7924);
    a.mask(0,0,0x7ffffff,false,27);a.sign8(3,0);a.sign8(0,4);a.compare_registers(3,0);
    if(a.equal()){face_near_ball(a,away?0x10235adc:0x102357f0,away);return;}
    a.at(away?0x10235b50:0x10235860);a.load(3,15,0x1a4);a.offset(3,3,0x10000);a.load(0,3,-0x2008);a.compare_registers(15,0,false);
    if(a.equal()){face_near_ball(a,away?0x10235b64:0x10235874,!away);return;}
    a.at(away?0x10235bd4:0x102358e8);a.load(3,15,0x1a4);a.load<1>(0,15,0x27);a.load<1>(4,3,0x7924);a.sign8(3,0);a.sign8(0,4);a.compare_registers(3,0);
    bool together=a.equal();
    if(!together) {
        a.load(3,15,0x1a4);a.offset(3,3,0x10000);a.load(0,3,-0x2008);a.compare(0,0,false);
        if(!a.equal()) {a.load(3,15,0x1a4);a.load<1>(0,15,0x27);a.offset(3,3,0x10000);a.load(4,3,-0x2008);a.sign8(3,0);
            a.load<1>(0,4,0x27);a.sign8(0,0);a.compare_registers(3,0);together=a.equal();}
    }
    if(together){face_near_ball(a,away?0x10235c28:0x1023593c,!away);return;}
    a.at(away?0x10235c98:0x102359b0);a.move(3,15);a.call(0x102721a0);a.sign8(0,3,true);
    if(!a.equal())face_near_ball(a,away?0x10235ca8:0x102359c0,away);
    else face_near_ball(a,away?0x10235d1c:0x10235a30,!away);
}

void noncarrier(Call& a) {
    a.load<1>(3,15,0x105);a.sign8(0,16);a.sign8(3,3);a.compare_registers(3,0);bool moved=!a.equal();
    if(!moved){a.load<1>(3,15,0x106);a.sign8(0,17);a.sign8(3,3);a.compare_registers(3,0);moved=!a.equal();}
    if(moved)migrate_noncarrier_cell(a);
    maintain_defensive_extent(a);react_neighbours(a);consume_noncarrier_condition(a);synchronize_noncarrier_ball(a);face_noncarrier_context(a);
}
void finish_movement(Call& a) {
    a.at(0x10235d88);a.load(0,1,0x188);a.offset(1,1,0x180);a.floating_load(31,1,-8);a.floating_load(30,1,-16);
    a.restore_link();a.multiple(13,true,-16);a.finish();
}
}

void continue_noncarrier(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call call(state,noncarrier_phase,dependencies,observer);noncarrier(call);finish_movement(call);
}
void return_movement_frame(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call call(state,common_return_phase,dependencies,observer);finish_movement(call);
}
Preparation prepare_movement(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call call(state,entry,dependencies,observer);const auto phase=prepare_movement_context(call);
    if(phase==Preparation::returned)finish_movement(call);
    else state.pc=phase==Preparation::carrier?carrier_phase:noncarrier_phase;
    return phase;
}
}
