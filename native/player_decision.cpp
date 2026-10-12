#include "player_decision.hpp"
#include "semantic_guest_call.hpp"

// Named decision phases preserve the complete original guest contract. The
// explicit call ABI also preserves interrupted prefixes and dependency order.
namespace cm::aot::player_decision {
namespace {
using Call = SemanticGuestCall;
void enter_decision(Call& a) {
    a.link(); a.floating_store(31, 1, -8); a.floating_store(30, 1, -16); a.multiple(13, false, -16);
    a.move(31, 3); a.load(13, 2, -0x7ba8); a.value(20, 0); a.load(14, 2, -0x6424);
    a.value(18, 0); a.load(26, 2, -0x51e8); a.value(16, 0); a.load(29, 2, -0x7b90);
    a.load(25, 2, -0x7b8c); a.load(23, 2, -0x7b88); a.load(30, 2, -0x7b84);
    a.load(15, 2, -0x51e4); a.load(24, 2, -0x51e0); a.save_link(); a.frame(0x100);
}
void finish_decision(Call& a) {
    a.at(0x1028572c); a.load(0, 1, 0x108); a.offset(1, 1, 0x100);
    a.floating_load(31, 1, -8); a.floating_load(30, 1, -16); a.restore_link(); a.multiple(13, true, -16); a.finish();
}
void move_to_ball(Call& a) {
    // Repeated live owner reads and byte arguments are part of this branch's
    // ABI. Movement remains a distinct original dependency, not this body.
    a.load(5, 31, 0x1a4); a.move(3, 31); a.load(4, 31, 0x1a4); a.value(6, 0x100);
    a.load<1>(5, 5, 0x791e); a.value(7, 0); a.load<1>(4, 4, 0x791d); a.value(8, 0); a.call(0x1022e7e0);
    a.value(0, 0x100); a.store<2>(31, 0x19c, 0); a.jump(0x1028572c);
}
bool choose_dead_ball_priority(Call& a) {
    a.load(3, 3, 0x1a4); a.load<1>(0, 3, 0x7924); a.sign8(0, 0); a.compare(0, -1);
    if (!a.equal()) { a.at(0x10283814); return false; }
    a.load(3, 31, 0x1a4); a.offset(3, 3, 0x10000); a.load(0, 3, -0x2008); a.compare_registers(0, 31, false);
    if (a.equal()) {
        // The original stream advances before the bound test and reloads its
        // cursor/seed afterward. The signed trait plus remainder decides whether
        // the designated receiver pursues the ball.
        a.load(3, 30, 0); a.load(0, 23, 0); a.offset(3, 3, 2); a.load<1>(17, 31, 0x10a);
        a.compare_registers(3, 0, false); a.store(30, 0, 3);
        if (!a.nonpositive()) { a.store(30, 0, 25); a.call(0x1077b770); a.mask(0, 3, 0xffff); a.store(29, 0, 0); }
        a.at(0x10283740); a.load(5, 30, 0); a.value(3, 0x66660000); a.load(4, 29, 0); a.offset(3, 3, 0x6667);
        a.load<2>(5, 5, 0); a.sign8(0, 17); a.add(5, 5, 4); a.multiply_high(3, 3, 5); a.shift_right(3, 3, 3);
        a.mask(4, 3, 1, false, 1); a.add(3, 3, 4); a.multiply(3, 3, 20); a.subtract(3, 3, 5); a.add_record(0, 0, 3);
        if (!a.equal()) { move_to_ball(a); return true; }
    }
    a.at(0x102837ac); a.load(3, 31, 0x1a4); a.offset(3, 3, 0x10000); a.load(0, 3, -0x2008); a.compare(0, 0, false);
    if (!a.equal()) { a.at(0x10283814); return false; }
    a.load(3, 31, 0x1a4); a.move(4, 31); a.call(0x101f5280); a.sign8(0, 3, true);
    if (a.equal()) { a.at(0x10283814); return false; }
    a.load(3, 31, 0x1a4); a.load<1>(0, 3, 0x1de); a.sign8(0, 0, true);
    if (!a.equal()) { a.at(0x10283814); return false; }
    move_to_ball(a); return true;
}
bool read_tactical_context(Call& a) {
    a.at(0x10283814);a.load<1>(0,31,0x27);a.load(5,31,0x1a4);a.sign8(0,0);a.load<1>(4,31,0x19);
    a.multiply(3,0,0x18e0);a.offset(3,3,0x7c6c);a.add(3,5,3);a.call(0x101149f0);a.mask(0,3,4,true);
    if(!a.equal())a.value(18,1);
    a.at(0x10283840);a.value(0,0xffffffff);a.move(4,31);a.store<2>(31,0x19c,0);a.load(3,31,0x1a4);a.load(5,31,0x1a4);
    a.load<1>(27,3,0x791d);a.load<1>(28,5,0x791e);a.load(3,31,0x1a4);a.call(0x101e3d50);a.load(4,31,0x1a4);a.move(22,3);
    a.load<1>(0,4,0x791f);a.sign8(0,0);a.compare(0,-1);if(!a.equal())return false;
    a.load(3,31,0x1a4);a.load<1>(0,3,0x7924);a.sign8(0,0);a.compare(0,-1);if(a.equal())return false;
    a.load(3,31,0x1a4);a.move(4,31);a.offset(5,1,0x39);a.offset(6,1,0x38);a.call(0x101e25a0);
    a.value(0,0x68);a.move(3,31);a.store<2>(31,0x19c,0);a.load<1>(4,1,0x39);a.load<1>(5,1,0x38);a.call(0x102374f0);a.jump(0x1028572c);return true;
}
void draw_defensive_traits(Call& a,std::uint32_t start) {
    a.at(start);a.load(3,30,0);a.load(0,23,0);a.offset(3,3,2);a.load<1>(17,31,0x108);a.load<1>(19,31,0x10d);
    a.compare_registers(3,0,false);a.load<1>(21,31,0x109);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
    a.at(start+0x34);a.load(3,30,0);a.value(5,0x51ec0000);a.load(6,29,0);a.offset(5,5,-0x7ae1);a.load<2>(7,3,0);
    a.sign8(3,19);a.sign8(0,21);a.sign8(4,17);a.add(6,7,6);a.multiply_high(5,5,6);a.product(0,3,0);
    a.shift_right(3,5,3);a.mask(5,3,1,false,1);a.add(3,3,5);a.multiply(3,3,25);a.product(0,4,0);a.subtract(3,3,6);a.compare_registers(0,3);
}
void defensive_step(Call& a,bool away) {
    a.value(0,0x7a);a.move(3,31);a.store<2>(31,0x19c,0);a.value(6,0xffffffff);a.value(7,0);a.value(8,0);
    a.load(5,31,0x1a4);a.load<1>(4,31,0x105);a.load<1>(5,5,away?0x7925:0x7926);a.offset(0,5,away?-2:2);a.sign8(5,0);
    a.call(0x1022e7e0);a.jump(0x1028572c);
}
void draw_defensive_pair(Call& a,std::uint32_t start) {
    a.at(start);a.load(3,30,0);a.load(0,23,0);a.offset(3,3,2);a.load<1>(17,31,0x10e);a.load<1>(19,31,0x109);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
    a.at(start+0x30);a.load(3,30,0);a.value(4,0x51ec0000);a.load(5,29,0);a.offset(4,4,-0x7ae1);a.load<2>(6,3,0);a.sign8(3,17);a.sign8(0,19);
    a.add(6,6,5);a.multiply_high(4,4,6);a.shift_right(4,4,5);a.mask(5,4,1,false,1);a.add(4,4,5);a.multiply(4,4,100);a.product(0,3,0);a.subtract(4,4,6);a.compare_registers(0,4);
}
bool choose_defensive_line(Call& a) {
    a.at(0x102838c0);a.load<1>(0,31,0x27);a.sign8(0,0,true);const bool away=!a.equal();
    draw_defensive_traits(a,away?0x10283b74:0x102838cc);if(a.nonpositive())return false;
    a.load(3,31,0x1a4);a.offset(3,3,0x10000);a.load(0,3,-0x2000);a.compare(0,0,false);if(a.equal())return false;
    a.sign16(0,22);a.compare(0,1);if(a.equal())return false;
    a.load<1>(0,31,0x106);a.sign8(3,28);a.sign8(0,0);a.compare_registers(3,0);
    if(away?a.at_least():a.nonpositive())return false;
    a.load<1>(0,31,0x106);a.sign8(0,0);a.compare(0,away?5:6);if(away?a.nonpositive():a.at_least())return false;
    a.load(5,31,0x1a4);a.load(4,31,0x1a4);a.offset(3,5,0x10000);a.load(0,3,away?-0x1fec:-0x1fe8);a.load<1>(4,4,away?0x7925:0x7926);a.compare(0,0,false);
    if(!a.equal()){a.offset(3,5,0x10000);a.load(3,3,away?-0x1fec:-0x1fe8);a.load<1>(0,3,0x106);a.jump(away?0x10283c68:0x102839c0);}
    else{a.at(away?0x10283c64:0x102839bc);a.value(0,away?0xffffffff:9);}
    a.at(away?0x10283c68:0x102839c0);a.sign8(3,0);a.sign8(0,4);a.compare_registers(3,0);if(away?a.less():a.positive())return false;
    a.load<1>(0,31,0x27);a.load(3,31,0x1a4);a.sign8(0,0);a.multiply(0,0,0x18e0);a.offset(3,3,0x10000);a.add(3,3,0);a.load(0,3,-0x7e0c);a.mask(0,0,0x400,true);
    bool strict=a.equal();
    if(!strict){
        a.load(3,31,0x1a4);a.load<1>(0,31,0x106);a.load<1>(away?4:3,3,away?0x7925:0x7926);
        a.sign8(away?3:0,0);a.sign8(away?0:3,away?4:3);a.subtract(0,3,0);a.compare(0,2);strict=a.at_least();
        if(!strict){
            a.load(3,31,0x1a4);a.load<1>(0,31,0x106);a.load<1>(away?4:3,3,away?0x7925:0x7926);
            a.sign8(away?3:0,0);a.sign8(away?0:3,away?4:3);a.subtract(0,3,0);a.compare(0,-2);strict=a.nonpositive();
            if(!strict){defensive_step(a,away);return true;}
        }
    }
    draw_defensive_pair(a,away?0x10283d10:0x10283a68);if(a.nonpositive())return false;
    a.load(3,31,0x1a4);a.load<1>(0,31,0x106);a.load<1>(away?4:3,3,away?0x7925:0x7926);
    a.sign8(away?3:0,0);a.sign8(away?0:3,away?4:3);a.subtract(0,3,0);a.compare(0,1);if(!a.equal())return false;
    a.load(5,31,0x1a4);a.load(4,31,0x1a4);a.offset(3,5,0x10000);a.load(0,3,away?-0x1fec:-0x1fe8);a.load<1>(4,4,away?0x7925:0x7926);a.compare(0,0,false);
    if(!a.equal()){a.offset(3,5,0x10000);a.load(3,3,away?-0x1fec:-0x1fe8);a.load<1>(0,3,0x106);a.jump(away?0x10283dd4:0x10283b2c);}
    else{a.at(away?0x10283dd0:0x10283b28);a.value(0,away?0xffffffff:9);}
    a.at(away?0x10283dd4:0x10283b2c);a.sign8(3,0);a.sign8(0,4);a.offset(3,3,away?-1:1);a.compare_registers(3,0);if(away?a.less():a.positive())return false;
    defensive_step(a,away);return true;
}
// Signed integer and scaled-float ranges are distinct original paths. Their
// live stream reads, double bias, jitter dependency and narrowing stay ordered.
void draw_pressure_strength(Call& a) {
    a.compare(17,0);
    if(a.equal()){a.value(19,0);a.jump(0x10284168);return;}
    a.at(0x1028400c);a.load(3,30,0);a.load(0,23,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
    a.at(0x10284034);a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(17,3);bool large=a.positive();
    if(!large){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(17,3);large=!a.at_least();}
    if(large){
        a.at(0x10284054);a.exclusive_flags(3,17,0x80000000);a.value(0,0x43300000);a.store(1,0x98,3);a.value(6,0x80010000);a.load(3,30,0);a.value(5,0x10000);
        a.store(1,0x94,0);a.value(0,0x43300000);a.floating_load(2,26,0x80);a.value(4,0x80010000);a.floating_load(1,1,0x94);a.offset(6,6,-0x7fff);
        a.floating_load(0,26,0);a.offset(5,5,-1);a.floating_subtract(1,1,2);a.load<2>(8,3,0);a.load(7,29,0);a.multiply(3,17,10);a.store(1,0x8c,0);a.offset(4,4,-0x7fff);
        a.floating_difference(2,1,0,true);a.add(0,8,7);a.multiply_high(6,6,0);a.floating_load(1,26,0x80);a.add(6,6,0);a.shift_right(6,6,15);a.mask(7,6,1,false,1);
        a.add(6,6,7);a.product(5,6,5);a.subtract(19,5,0);a.exclusive_flags(0,19,0x80000000);a.store(1,0x90,0);a.multiply_high(0,4,3);a.floating_load(0,1,0x8c);
        a.floating_subtract(0,0,1);a.add(0,0,3);a.shift_right(0,0,15);a.floating_product(0,2,0);a.mask(3,0,1,false,1);a.add(3,0,3);a.floating_integer(0,0);
        a.floating_store(0,1,0x84);a.load(19,1,0x88);a.call(0x10226520);a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,17,-1);
        a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.add(19,19,3);a.compare_registers(19,0);
        if(!a.nonpositive()){a.offset(19,17,-1);a.jump(0x10284168);}
    }else{
        a.at(0x10284138);a.load(3,30,0);a.compare(17,0);a.load(0,29,0);a.load<2>(3,3,0);a.add(3,3,0);a.divide(0,3,17);a.product(0,0,17);a.subtract(19,0,3);a.negate(0,19);
        if(!a.nonpositive())a.move(0,19);a.at(0x10284164);a.move(19,0);
    }
}
void draw_pressure_threshold(Call& a) {
    a.at(0x10284168);a.floating_load(1,24,0xc8,true);a.floating_load(0,24,0,true);a.floating_product_single(1,1,31);a.floating_compare(1,0);
    if(a.equal()){a.value(17,0);a.jump(0x10284380);return;}
    a.at(0x10284184);a.load(3,30,0);a.load(0,23,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
    a.at(0x102841ac);a.floating_load(0,24,0xc8,true);a.value(3,0x10000);a.offset(3,3,-1);a.floating_product_single(0,0,31);a.floating_integer(0,0);a.floating_store(0,1,0x84);a.load(0,1,0x88);a.compare_registers(0,3);bool large=a.positive();
    if(!large){a.floating_load(0,24,0xc8,true);a.value(3,0xffff0000);a.offset(3,3,1);a.floating_product_single(0,0,31);a.floating_integer(0,0);a.floating_store(0,1,0x84);a.load(0,1,0x88);a.compare_registers(0,3);large=!a.at_least();}
    if(large){
        a.at(0x102841f4);a.floating_load(0,24,0xc8,true);a.value(0,0x43300000);a.load(3,30,0);a.value(7,0x80010000);a.floating_product_single(1,0,31);a.floating_load(0,24,0xc8,true);
        a.store(1,0x8c,0);a.value(5,0x43300000);a.floating_product_single(0,0,31);a.value(6,0x10000);a.floating_integer(1,1);a.value(4,0x80010000);a.floating_integer(0,0);
        a.load<2>(8,3,0);a.load(0,29,0);a.offset(7,7,-0x7fff);a.floating_store(1,1,0x84);a.add(0,8,0);a.floating_load(3,26,0x80);a.multiply_high(7,7,0);a.load(3,1,0x88);
        a.offset(6,6,-1);a.store(1,0x94,5);a.offset(4,4,-0x7fff);a.exclusive_flags(3,3,0x80000000);a.store(1,0x90,3);a.add(5,7,0);a.floating_load(2,26,0);
        a.shift_right(5,5,15);a.floating_store(0,1,0x74);a.mask(7,5,1,false,1);a.floating_load(0,1,0x8c);a.add(5,5,7);a.product(5,5,6);a.floating_load(1,26,0x80);
        a.floating_subtract(0,0,3);a.load(3,1,0x78);a.floating_difference(2,0,2,true);a.subtract(17,5,0);a.exclusive_flags(0,17,0x80000000);a.store(1,0x98,0);a.multiply(0,3,10);
        a.floating_load(0,1,0x94);a.floating_subtract(0,0,1);a.multiply_high(3,4,0);a.floating_product(0,2,0);a.add(0,3,0);a.shift_right(0,0,15);a.floating_integer(0,0);a.mask(3,0,1,false,1);
        a.add(3,0,3);a.floating_store(0,1,0x7c);a.load(17,1,0x80);a.call(0x10226520);a.floating_load(0,24,0xc8,true);a.value(4,0x66660000);a.offset(0,3,5);
        a.floating_product_single(0,0,31);a.offset(4,4,0x6667);a.multiply_high(0,4,0);a.floating_integer(0,0);a.floating_store(0,1,0x6c);a.shift_right(0,0,2);a.mask(4,0,1,false,1);
        a.load(3,1,0x70);a.add(0,0,4);a.add(17,17,0);a.offset(0,3,-1);a.compare_registers(17,0);
        if(!a.nonpositive()){a.floating_load(0,24,0xc8,true);a.floating_product_single(0,0,31);a.floating_integer(0,0);a.floating_store(0,1,0x64);a.load(3,1,0x68);a.offset(17,3,-1);a.jump(0x10284380);}
    }else{
        a.at(0x10284328);a.floating_load(0,24,0xc8,true);a.load(3,30,0);a.floating_product_single(1,0,31);a.floating_load(0,24,0xc8,true);a.load<2>(3,3,0);a.load(0,29,0);
        a.floating_product_single(0,0,31);a.floating_integer(1,1);a.add(5,3,0);a.floating_integer(0,0);a.floating_store(1,1,0x64);a.load(4,1,0x68);a.floating_store(0,1,0x6c);
        a.divide(3,5,4);a.load(0,1,0x70);a.compare(0,0);a.product(0,3,4);a.subtract(17,0,5);a.negate(0,17);if(!a.nonpositive())a.move(0,17);a.at(0x1028437c);a.move(17,0);
    }
}
bool choose_ball_pursuit(Call& a) {
    a.at(0x10283e1c);a.load<1>(3,31,0x105);a.sign16(0,22);a.compare(0,1);a.store<1>(1,0x39,3);a.load<1>(0,31,0x106);a.store<1>(1,0x38,0);if(a.equal())return false;
    a.load(3,31,0x1a4);a.load<1>(0,3,0x7924);a.sign8(0,0);a.compare(0,-1);
    if(!a.equal()){
        a.load(3,31,0x1a4);a.offset(3,3,0x10000);a.load(0,3,-0x2000);a.compare(0,0,false);if(a.equal())return false;
        a.load(3,31,0x1a4);a.load<1>(0,31,0x27);a.offset(3,3,0x10000);a.load(4,3,-0x2000);a.sign8(3,0);a.load<1>(0,4,0x27);a.sign8(0,0);a.compare_registers(3,0);if(a.equal())return false;
        a.load(3,31,0x1a4);a.load<1>(0,31,0x27);a.offset(4,3,0x10000);a.load(3,31,0x1a4);a.load(4,4,-0x2000);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x7ffffff,false,27);
        a.load<1>(5,4,0x106);a.sign8(4,0);a.call(0x101e4ea0);a.sign8(0,3,true);if(a.less())return false;
    }
    a.at(0x10283eb8);a.load(3,31,0x1a4);a.offset(3,3,0x10000);a.load(0,3,-0x2008);a.compare(0,0,false);
    if(!a.equal()){
        a.load(3,31,0x1a4);a.load<1>(0,31,0x27);a.offset(3,3,0x10000);a.load(4,3,-0x2008);a.sign8(3,0);a.load<1>(0,4,0x27);a.sign8(0,0);a.compare_registers(3,0);if(a.equal())return false;
    }
    a.at(0x10283ef0);a.load(3,31,0x1a4);a.move(4,31);a.call(0x101f5280);a.sign8(0,3,true);if(!a.equal()){move_to_ball(a);return true;}
    a.at(0x10283f34);a.move(3,31);a.call(0x10289560);a.mask(0,3,0x200,true);bool permitted=!a.equal();
    if(!permitted){a.move(3,31);a.call(0x10289560);a.mask(0,3,0x100,true);permitted=!a.equal();}
    if(!permitted){a.move(3,31);a.call(0x10289560);a.mask(0,3,0x400,true);permitted=!a.equal();}
    if(!permitted)return false;
    a.at(0x10283f64);a.move(3,31);a.call(0x10289560);a.mask(0,3,8,true);permitted=!a.equal();
    if(!permitted){a.move(3,31);a.call(0x10289560);a.mask(0,3,16,true);permitted=!a.equal();}
    if(!permitted){a.move(3,31);a.call(0x10289560);a.mask(0,3,32,true);permitted=!a.equal();}
    if(!permitted)return false;
    a.at(0x10283f94);a.load(3,31,0x1a4);a.load(4,31,0x1a4);a.load<1>(3,3,0x791d);a.load<1>(0,31,0x105);a.load<1>(17,4,0x791e);a.sign8(3,3);a.sign8(0,0);a.load<1>(21,31,0x110);
    a.subtract(3,3,0);a.call(0x10775540);a.load<1>(0,31,0x106);a.sign8(4,17);a.sign8(17,3);a.sign8(0,0);a.subtract(3,4,0);a.call(0x10775540);
    a.floating_load(0,31,0xe0,true);a.sign8(3,3);a.sign8(0,17);a.floating_integer(0,0);a.multiply(3,3,0x24);a.mask(0,0,0xfffffffc,false,2);a.floating_store(0,1,0x94);a.add(0,3,0);
    a.floating_indexed_single(31,14,0);a.load(17,1,0x98);draw_pressure_strength(a);draw_pressure_threshold(a);
    a.at(0x10284380);a.sign8(0,21);a.add(0,0,19);a.compare_registers(0,17);if(a.nonpositive())return false;
    move_to_ball(a);return true;
}
bool mark_geometry_allowed(Call& a) {
    a.load<1>(0,31,0x27);a.load(3,31,0x1a4);a.sign8(0,0);a.multiply(0,0,0x18e0);a.offset(3,3,0x10000);a.add(3,3,0);a.load(0,3,-0x7e0c);a.mask(0,0,0x400,true);if(a.equal())return true;
    a.load(3,31,0x1a4);a.offset(3,3,0x10000);a.load(0,3,-0x2000);a.compare(0,0,false);if(a.equal())return true;
    a.load<1>(0,31,0x27);a.sign8(0,0,true);bool away=!a.equal();
    if(!away){
        a.load<1>(0,31,0x106);a.sign8(0,0);a.compare(0,6);away=a.at_least();
        if(!away){
            a.load<1>(0,31,0x106);a.sign8(3,28);a.offset(3,3,-1);a.sign8(0,0);a.compare_registers(3,0);away=a.less();
            if(!away){a.load(4,31,0x1b0);a.load(3,31,0x1a4);a.load<1>(0,4,0x106);a.load<1>(3,3,0x7925);a.sign8(0,0);a.sign8(3,3);a.compare_registers(3,0);if(a.nonpositive())return true;}
        }
    }
    a.at(0x10284458);a.load<1>(0,31,0x27);a.compare(0,1);if(!a.equal())return false;
    a.load<1>(0,31,0x106);a.sign8(0,0);a.compare(0,5);if(a.nonpositive())return false;
    a.load<1>(0,31,0x106);a.sign8(3,28);a.offset(3,3,1);a.sign8(0,0);a.compare_registers(3,0);if(a.positive())return true;
    a.load(4,31,0x1b0);a.load(3,31,0x1a4);a.load<1>(0,4,0x106);a.load<1>(3,3,0x7926);a.sign8(0,0);a.sign8(3,3);a.compare_registers(3,0);return !a.less();
}
bool choose_mark_following(Call& a) {
    a.at(0x102843c0);a.load(0,31,0x1b0);a.compare(0,0,false);if(a.equal())return false;
    if(!mark_geometry_allowed(a))return false;
    a.at(0x102844ac);a.load(3,31,0x1b0);a.load<1>(0,3,0x19);a.sign8(0,0);a.compare(0,-4);
    bool invalid=false;
    if(!a.equal()){
        if(a.at_least()){a.at(0x102844d0);a.compare(0,0);invalid=!a.at_least();}
        else{a.compare(0,-5);invalid=a.at_least();if(!invalid)a.jump(0x102844e4);}
    }
    if(invalid){a.at(0x102844d8);a.value(0,0);a.store(31,0x1b0,0);a.jump(0x1028468c);return false;}
    a.at(0x102844e4);a.load<1>(0,31,0x108);a.sign8(0,0,true);
    if(!a.equal()){
        a.load(3,31,0x1a4);a.load(0,31,0x1b0);a.offset(3,3,0x10000);a.load(3,3,-0x2008);a.compare_registers(3,0,false);
        if(a.equal()){
            a.value(0,0x105);a.move(3,31);a.store<2>(31,0x19c,0);a.move(4,27);a.move(5,28);a.value(6,0xffffffff);a.value(7,0);a.value(8,0);a.call(0x1022e7e0);a.jump(0x1028572c);return true;
        }
    }
    a.at(0x10284530);a.value(0,0x105);a.store<2>(31,0x19c,0);a.load<1>(0,31,0x27);a.load(3,31,0x1b0);a.sign8(0,0);a.load(4,31,0x1b0);a.leading_zeroes(0,0);
    a.load<1>(14,3,0x106);a.mask(0,0,0x7ffffff,false,27);a.load<1>(13,4,0x105);a.load(3,31,0x1a4);a.move(5,14);a.sign8(4,0);a.call(0x101ee9f0);a.sign8(0,3,true);
    bool fixed=!a.equal();if(!fixed){a.sign8(0,18,true);fixed=!a.equal();}
    if(!fixed){
        a.load(3,30,0);a.load(0,23,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
        if(!a.nonpositive()){a.store(30,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
        a.at(0x102845a0);a.load(5,30,0);a.value(4,0x30c30000);a.value(0,0x43300000);a.load(3,31,0x1b0);a.load<2>(6,5,0);a.offset(4,4,0xc31);a.load(5,29,0);a.store(1,0x64,0);
        a.add(5,6,5);a.floating_load(2,3,0xb0,true);a.multiply_high(0,4,5);a.floating_load(1,26,0x80);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,21);
        a.subtract(0,0,5);a.exclusive_flags(0,0,0x80000000);a.store(1,0x68,0);a.floating_load(0,1,0x64);a.floating_subtract(0,0,1,true);a.floating_compare(2,0);fixed=a.at_least();
        if(!fixed){
            a.load<1>(0,31,0x27);a.sign8(0,0,true);bool adjust=!a.equal();
            if(!adjust){a.sign8(0,14,true);adjust=a.nonpositive();if(!adjust){a.offset(14,14,-1);a.jump(0x10284634);}}
            if(adjust){
                a.at(0x10284618);a.load<1>(0,31,0x27);a.compare(0,1);
                if(a.equal()){a.sign8(0,14);a.compare(0,11);if(!a.at_least())a.offset(14,14,1);}
            }
            a.at(0x10284634);a.sign8(0,13);a.compare(0,2);bool toward_center=a.positive();
            if(!toward_center){a.sign8(0,13);a.compare(0,8);toward_center=a.at_least();if(!toward_center){a.offset(13,13,1);a.jump(0x1028466c);}}
            if(toward_center){
                a.at(0x10284654);a.sign8(0,13);a.compare(0,6);
                if(!a.less()){a.sign8(0,13,true);if(!a.nonpositive())a.offset(13,13,-1);}
            }
        }
    }
    a.at(0x1028466c);a.move(3,31);a.move(4,13);a.move(5,14);a.value(6,0xffffffff);a.value(7,0);a.value(8,0);a.call(0x1022e7e0);a.jump(0x1028572c);return true;
}
void draw_destination_radius(Call& a) {
    a.floating_load(1,26,0x28);a.floating_load(0,31,0xe4,true);a.floating_sum(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x64);a.load(17,1,0x68);a.compare(17,0);
    if(a.equal()){a.value(18,0);a.jump(0x1028482c);return;}
    a.at(0x102846d0);a.load(3,30,0);a.load(0,23,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
    a.at(0x102846f8);a.value(3,0x10000);a.offset(3,3,-1);a.compare_registers(17,3);bool large=a.positive();
    if(!large){a.value(3,0xffff0000);a.offset(3,3,1);a.compare_registers(17,3);large=!a.at_least();}
    if(large){
        a.at(0x10284718);a.exclusive_flags(3,17,0x80000000);a.value(0,0x43300000);a.store(1,0x68,3);a.value(6,0x80010000);a.load(7,30,0);a.value(4,0x43300000);a.store(1,0x64,0);
        a.value(5,0x10000);a.floating_load(2,26,0x80);a.value(3,0x80010000);a.floating_load(1,1,0x64);a.offset(6,6,-0x7fff);a.floating_load(0,26,0);a.offset(5,5,-1);
        a.floating_subtract(1,1,2);a.load<2>(8,7,0);a.load(7,29,0);a.multiply(0,17,10);a.offset(3,3,-0x7fff);a.store(1,0x6c,4);a.floating_difference(2,1,0,true);
        a.add(7,8,7);a.multiply_high(4,6,7);a.floating_load(1,26,0x80);a.add(4,4,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.add(4,4,6);a.product(4,4,5);a.subtract(18,4,7);
        a.exclusive_flags(4,18,0x80000000);a.store(1,0x70,4);a.multiply_high(3,3,0);a.floating_load(0,1,0x6c);a.floating_subtract(0,0,1);a.add(0,3,0);a.shift_right(0,0,15);
        a.floating_product(0,2,0);a.mask(3,0,1,false,1);a.add(3,0,3);a.floating_integer(0,0);a.floating_store(0,1,0x74);a.load(18,1,0x78);a.call(0x10226520);
        a.value(4,0x66660000);a.offset(3,3,5);a.offset(4,4,0x6667);a.offset(0,17,-1);a.multiply_high(3,4,3);a.shift_right(3,3,2);a.mask(4,3,1,false,1);
        a.add(3,3,4);a.add(18,18,3);a.compare_registers(18,0);if(!a.nonpositive()){a.offset(18,17,-1);a.jump(0x1028482c);}
    }else{
        a.at(0x102847fc);a.load(3,30,0);a.compare(17,0);a.load(0,29,0);a.load<2>(3,3,0);a.add(3,3,0);a.divide(0,3,17);a.product(0,0,17);a.subtract(18,0,3);a.negate(0,18);
        if(!a.nonpositive())a.move(0,18);a.at(0x10284828);a.move(18,0);
    }
}
void scale_destination_limit(Call& a) {
    a.at(0x1028482c);a.compare(18,0);
    if(a.equal()){a.floating_load(30,26,0x88);a.jump(0x10284964);return;}
    a.at(0x1028483c);a.load(3,30,0);a.load(0,23,0);a.offset(3,3,2);a.load<1>(17,31,0x110);a.compare_registers(3,0,false);a.floating_load(30,26,0x90);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
    a.at(0x1028486c);a.load(5,30,0);a.value(3,0x66660000);a.load(4,29,0);a.offset(3,3,0x6667);a.load<2>(5,5,0);a.sign8(0,17);a.add(5,5,4);
    a.multiply_high(3,3,5);a.shift_right(3,3,2);a.mask(4,3,1,false,1);a.add(3,3,4);a.multiply(3,3,10);a.subtract(3,3,5);a.compare_registers(0,3);
    if(!a.nonpositive()){a.floating_load(0,26,0x3a0);a.floating_product(30,30,0);}
    a.at(0x102848b0);a.load<1>(0,31,0x109);a.sign8(0,0,true);if(a.equal()){a.floating_load(0,26,0x3a0);a.floating_product(30,30,0);}
    a.at(0x102848c4);a.load<1>(0,31,0x10d);a.sign8(0,0,true);if(a.equal()){a.floating_load(0,26,0x3a0);a.floating_product(30,30,0);}
    a.at(0x102848d8);a.load(3,30,0);a.load(0,23,0);a.offset(3,3,2);a.floating_load(31,31,0x1a8,true);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
    a.at(0x10284904);a.load(5,30,0);a.value(3,0x1b4f0000);a.value(0,0x43300000);a.load(4,29,0);a.load<2>(5,5,0);a.offset(3,3,-0x7e4b);a.store(1,0x64,0);a.add(4,5,4);
    a.floating_load(1,26,0x80);a.multiply_high(0,3,4);a.shift_right(0,0,4);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,150);a.subtract(3,0,4);a.offset(0,3,30);
    a.exclusive_flags(0,0,0x80000000);a.store(1,0x68,0);a.floating_load(0,1,0x64);a.floating_subtract(0,0,1,true);a.floating_compare(31,0);
    if(!a.nonpositive()){a.floating_load(0,26,0x3a0);a.floating_product(30,30,0);}
}
void fallback_destination(Call& a,std::uint32_t start) {
    a.at(start);a.sign8(0,16,true);if(!a.equal())return;
    a.load(3,31,0x1a4);a.move(4,31);a.offset(5,1,0x3b);a.offset(6,1,0x3a);a.value(7,1);a.call(0x101e28b0);
    a.load<1>(4,1,0x3b);a.move(3,31);a.call(0x102857c0);a.store<1>(1,0x3b,3);if(start!=0x10284d50)a.jump(0x10284d80);
}
void use_ball_destination(Call& a,std::uint32_t start) {
    a.at(start);a.store<1>(1,0x3b,27);a.value(20,1);a.store<1>(1,0x3a,28);a.jump(0x10284d80);
}
bool tactical_ball_destination(Call& a) {
    a.load<1>(0,31,0x27);a.load(5,31,0x1a4);a.sign8(0,0);a.load<1>(4,31,0x19);a.multiply(3,0,0x18e0);a.offset(3,3,0x7c6c);a.add(3,5,3);a.call(0x10113e10);
    a.value(6,0x10000);a.value(7,0);a.offset(6,6,-0x8000);a.value(5,0);a.intersect(6,4,6);a.intersect(4,3,7);a.value(0,0);a.combine(3,6,5,false,true);a.combine(0,4,0,false,true);a.combine(0,3,0,true);
    if(!a.equal())return true;
    a.load<1>(0,31,0x27);a.load(5,31,0x1a4);a.sign8(0,0);a.load<1>(4,31,0x19);a.multiply(3,0,0x18e0);a.offset(3,3,0x7c6c);a.add(3,5,3);a.call(0x10113e10);
    a.value(5,0x2000);a.value(0,0);a.intersect(6,4,5);a.value(4,0);a.intersect(5,3,0);a.value(0,0);a.combine(3,6,4,false,true);a.combine(0,5,0,false,true);a.combine(0,3,0,true);
    if(!a.equal()){
        a.load<1>(0,31,0x27);a.load(4,31,0x1a4);a.sign8(0,0);a.multiply(3,0,0x18e0);a.offset(3,3,0x7c6c);a.add(3,4,3);a.load(0,3,0x588);a.mask(0,0,0x100,true);if(!a.equal())return true;
    }
    a.at(0x10284a68);a.load<1>(0,31,0x27);a.load(4,31,0x1a4);a.sign8(0,0);a.multiply(3,0,0x18e0);a.offset(3,3,0x7c6c);a.add(3,4,3);a.load(0,3,0x588);a.mask(0,0,0x4000,true);return !a.equal();
}
void draw_opponent_count(Call& a) {
    a.load<1>(0,31,0x27);a.load(3,31,0x1a4);a.sign8(0,0);a.leading_zeroes(0,0);a.mask(0,0,0x7ffffff,false,27);a.sign8(0,0);a.add(3,3,0);a.load<1>(18,3,0x77bc);a.sign8(0,18,true);
    if(a.equal()){a.value(17,0);a.jump(0x10284c64);return;}
    a.at(0x10284aec);a.load(3,30,0);a.load(0,23,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
    a.at(0x10284b14);a.value(3,0x10000);a.sign8(0,18);a.offset(3,3,-1);a.compare_registers(0,3);bool large=a.positive();
    if(!large){a.value(3,0xffff0000);a.sign8(0,18);a.offset(3,3,1);a.compare_registers(0,3);large=!a.at_least();}
    if(large){
        a.at(0x10284b3c);a.sign8(3,18);a.value(0,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store(1,0x64,0);a.load(7,30,0);a.value(6,0x80010000);a.store(1,0x68,3);
        a.value(4,0x43300000);a.floating_load(2,26,0x80);a.value(5,0x10000);a.floating_load(1,1,0x64);a.sign8(0,18);a.load<2>(8,7,0);a.value(3,0x80010000);a.floating_subtract(1,1,2);
        a.floating_load(0,26,0);a.load(7,29,0);a.offset(6,6,-0x7fff);a.store(1,0x6c,4);a.offset(5,5,-1);a.floating_difference(2,1,0,true);a.add(7,8,7);a.multiply_high(4,6,7);
        a.floating_load(1,26,0x80);a.offset(3,3,-0x7fff);a.add(4,4,7);a.shift_right(4,4,15);a.mask(6,4,1,false,1);a.add(4,4,6);a.product(4,4,5);a.subtract(17,4,7);
        a.exclusive_flags(4,17,0x80000000);a.store(1,0x70,4);a.multiply(0,0,10);a.floating_load(0,1,0x6c);a.multiply_high(3,3,0);a.floating_subtract(0,0,1);a.floating_product(0,2,0);
        a.add(0,3,0);a.shift_right(0,0,15);a.mask(3,0,1,false,1);a.floating_integer(0,0);a.add(3,0,3);a.floating_store(0,1,0x74);a.load(17,1,0x78);a.call(0x10226520);
        a.value(4,0x66660000);a.offset(0,3,5);a.offset(4,4,0x6667);a.sign8(3,18);a.multiply_high(4,4,0);a.offset(0,3,-1);a.shift_right(3,4,2);a.mask(4,3,1,false,1);a.add(3,3,4);
        a.add(17,17,3);a.compare_registers(17,0);if(!a.nonpositive()){a.sign8(3,18);a.offset(17,3,-1);a.jump(0x10284c64);}
    }else{
        a.at(0x10284c30);a.load(5,30,0);a.sign8(3,18);a.load(4,29,0);a.sign8(0,18,true);a.load<2>(0,5,0);a.add(4,0,4);a.divide(0,4,3);a.product(0,0,3);a.subtract(17,0,4);a.negate(0,17);
        if(!a.nonpositive())a.move(0,17);a.at(0x10284c60);a.move(17,0);
    }
}
void select_destination(Call& a) {
    a.at(0x1028468c);a.load(3,31,0x1a4);a.load<1>(0,3,0x7924);a.sign8(0,0);a.compare(0,-1);if(a.equal()){fallback_destination(a,0x10284d50);return;}
    a.mask(0,22,1,true);if(!a.equal()){fallback_destination(a,0x10284d50);return;}
    draw_destination_radius(a);scale_destination_limit(a);
    a.at(0x10284964);a.load<1>(0,31,0x105);a.sign8(3,27);a.sign8(0,0);a.subtract(3,3,0);a.call(0x10775540);a.load<1>(0,31,0x106);a.sign8(4,28);a.sign8(17,3);a.sign8(0,0);a.subtract(3,4,0);a.call(0x10775540);
    a.sign8(3,3);a.sign8(0,17);a.multiply(3,3,0x24);a.mask(0,0,0xfffffffc,false,2);a.add(0,3,0);a.floating_indexed_single(0,14,0);a.floating_compare(0,30);
    if(a.at_least()){fallback_destination(a,0x10284d1c);return;}
    if(tactical_ball_destination(a)){use_ball_destination(a,0x10284a8c);return;}
    a.at(0x10284a9c);a.move(3,31);a.call(0x10289560);a.mask(0,3,2,true);bool fallback=!a.equal();
    if(!fallback){a.move(3,31);a.call(0x10289560);a.mask(0,3,4,true);fallback=!a.equal();}
    if(fallback){fallback_destination(a,0x10284ce8);return;}
    draw_opponent_count(a);
    a.at(0x10284c64);a.load(3,30,0);a.load(0,23,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(30,0,3);
    if(!a.nonpositive()){a.store(30,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
    a.at(0x10284c8c);a.load(4,30,0);a.value(3,0x51ec0000);a.load(0,29,0);a.offset(3,3,-0x7ae1);a.load<2>(4,4,0);a.add(4,4,0);a.multiply_high(0,3,4);a.shift_right(0,0,3);
    a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,25);a.subtract(0,0,4);a.compare_registers(17,0);
    if(!a.nonpositive()){a.load(3,31,0x1a4);a.move(4,31);a.call(0x101f0dd0);a.sign8(0,3,true);if(a.equal()){use_ball_destination(a,0x10284cd8);return;}}
    fallback_destination(a,0x10284ce8);
}
bool goal_area(Call& a,std::uint32_t start) {
    a.at(start);a.load<1>(0,31,0x27);a.compare(0,1);const bool home=!a.equal();
    a.at(start+(home?0x5c:0xc));a.load<1>(0,31,0x105);a.value(3,0);a.sign8(0,0);a.compare(0,2);bool outside=a.less();
    if(!outside){a.load<1>(0,31,0x105);a.sign8(0,0);a.compare(0,6);outside=a.positive();}
    if(!outside){a.load<1>(0,31,0x106);a.sign8(0,0,home);if(home)outside=a.less();else{a.compare(0,10);outside=a.less();}}
    if(!outside){a.load<1>(0,31,0x106);a.sign8(0,0);a.compare(0,home?1:11);outside=a.positive();}
    if(!outside)a.value(3,1);
    a.at(start+(home?0xa0:0x54));a.sign8(0,3);if(!home)a.jump(start+0xa4);
    a.at(start+0xa4);a.sign8(0,0,true);return !a.equal();
}
void missing_keeper_diagnostic(Call& a) {
    a.offset(3,15,0x8f);a.offset(6,1,0x44);a.offset(7,1,0x3c);a.value(4,0);a.value(5,0);a.call(0x10739890);a.call(0x10768810);a.move(5,3);a.move(3,13);a.offset(4,15,0x9c);
    a.offset(6,1,0x44);a.offset(7,1,0x3c);a.value(8,0x540c);a.call(0x1077b720);a.move(4,13);a.offset(3,15,0xa9);a.value(5,0);a.call(0x10769b60);a.load(4,2,-0x7bac);
    a.value(0,0);a.value(3,0xffffffff);a.store<1>(4,0,0);a.call(0x1076f7d0);
}
bool choose_keeper_escape(Call& a) {
    a.at(0x10284d80);a.sign16(0,22);a.compare(0,1);if(a.equal())return false;
    a.load(3,31,0x1a4);a.load<1>(0,3,0x7924);a.sign8(0,0);a.compare(0,-1);if(a.equal())return false;
    if(!goal_area(a,0x10284da0))return false;
    a.load(3,31,0x1a4);a.move(5,27);a.load<1>(4,31,0x27);a.move(6,28);a.call(0x101eea30);a.sign8(0,3,true);if(a.equal())return false;
    a.load(3,31,0x1a4);a.load<1>(4,31,0x27);a.call(0x101f1e10);a.move(16,3,true);if(a.equal())missing_keeper_diagnostic(a);
    a.at(0x10284ed8);a.load<1>(0,16,0x2c);a.sign8(0,0);a.compare(0,1);if(a.nonpositive())return false;
    a.load<1>(5,31,0x27);a.value(0,11);a.move(3,31);a.value(4,4);a.sign8(5,5);a.value(6,0xffffffff);a.leading_zeroes(5,5);a.value(7,0);a.mask(5,5,1,false,27);
    a.value(8,0);a.negate(5,5);a.intersect(0,0,5,true);a.sign8(5,0);a.call(0x1022e7e0);a.value(0,0x78);a.store<2>(31,0x19c,0);a.jump(0x1028572c);return true;
}
void keeper_destination(Call& a,std::uint32_t start) {
    a.at(start);a.value(0,0x71);a.move(3,31);a.store<2>(31,0x19c,0);a.value(6,0xffffffff);a.value(7,0);a.value(8,0);a.load<1>(4,1,0x3b);a.load<1>(5,1,0x3a);a.call(0x1022e7e0);a.jump(0x1028572c);
}
void keeper_pursuit(Call& a) {
    a.value(0,0x103);a.move(3,31);a.store<2>(31,0x19c,0);a.move(4,27);a.move(5,28);a.value(6,0x100);a.value(7,0);a.value(8,0);a.call(0x1022e7e0);a.jump(0x1028572c);
}
bool valid_ball_position(Call& a,std::uint32_t start) {
    a.at(start);a.sign8(0,27,true);if(a.less())return false;a.sign8(0,27);a.compare(0,9);if(a.at_least())return false;
    a.sign8(0,28,true);if(a.less())return false;a.sign8(0,28);a.compare(0,12);return !a.at_least();
}
void keeper_capture(Call& a) {
    a.load<1>(5,31,0x27);a.value(0,11);a.load<1>(4,31,0x105);a.move(3,31);a.sign8(6,5);a.load<1>(5,31,0x106);a.leading_zeroes(7,6);a.value(6,4);
    a.mask(7,7,1,false,27);a.negate(7,7);a.intersect(0,0,7);a.sign8(7,0);a.call(0x10237830);
    a.load(3,31,0x1a4);a.value(4,0x1f46);a.load<1>(7,31,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);a.value(9,0xffffffff);a.load<1>(6,31,0x2f);a.value(10,0);
    a.load(5,31,0x1a4);a.call(0x10201180);a.move(3,31);a.call(0x1022e3a0);a.load<2>(3,31,0x36,true);a.value(0,0x103);a.offset(3,3,4);a.store<2>(31,0x36,3);a.store<2>(31,0x19c,0);a.jump(0x1028572c);
}
bool choose_keeper_action(Call& a) {
    a.at(0x10284f2c);a.sign16(0,22);a.compare(0,1);if(!a.equal())return false;
    a.load<1>(0,31,0x105);a.sign8(3,27);a.sign8(0,0);a.compare_registers(3,0);bool displaced=!a.equal();
    if(!displaced){a.load<1>(0,31,0x106);a.sign8(3,28);a.sign8(0,0);a.compare_registers(3,0);displaced=!a.equal();}
    if(!displaced){
        a.load(3,31,0x1a4);a.load<1>(0,3,0x7924);a.sign8(0,0);a.compare(0,-1);if(!a.equal())return true;
        a.load(3,31,0x1a4);a.load<1>(0,3,0x791f);a.sign8(0,0);a.compare(0,5);if(a.at_least())return true;
        a.load(3,31,0x1a4);a.load<1>(0,3,0x1de);a.sign8(0,0,true);if(!a.equal())return true;
        keeper_capture(a);return true;
    }
    a.at(0x10285014);a.load(3,31,0x1a4);a.move(4,31);a.call(0x101f5280);a.sign8(0,3,true);bool reposition=a.equal();
    if(!reposition){
        a.load<1>(0,31,0x105);a.sign8(3,27);a.sign8(0,0);a.subtract(3,3,0);a.call(0x10775540);a.load<1>(0,31,0x106);a.sign8(4,28);a.sign8(13,3);a.sign8(0,0);a.subtract(3,4,0);a.call(0x10775540);
        a.sign8(3,3);a.sign8(0,13);a.multiply(3,3,0x24);a.floating_load(0,26,0x90);a.mask(0,0,0xfffffffc,false,2);a.add(0,3,0);a.floating_indexed_single(1,14,0);a.floating_compare(1,0);a.compare_less_or_equal_bit();reposition=!a.equal();
    }
    if(reposition){keeper_destination(a,0x102851c4);return true;}
    a.load(3,31,0x1a4);a.move(5,27);a.load<1>(4,31,0x27);a.move(6,28);a.call(0x101eea30);a.sign8(0,3,true);
    if(!a.equal()){
        a.load(3,31,0x1a4);a.load<1>(0,3,0x7924);a.sign8(0,0);a.compare(0,-1);bool pursue=a.equal();
        if(!pursue){a.load<1>(0,31,0x109);a.sign8(0,0,true);pursue=a.equal();if(pursue){a.load<1>(0,31,0x10d);a.sign8(0,0,true);pursue=a.equal();}}
        if(pursue){a.at(0x102850c4);keeper_pursuit(a);return true;}
    }
    a.at(0x102850ec);a.load(3,31,0x1a4);a.load<1>(0,3,0x7924);a.sign8(0,0);a.compare(0,-1);if(!a.equal()){keeper_destination(a,0x1028519c);return true;}
    a.load<1>(0,31,0x27);a.move(5,27);a.load(3,31,0x1a4);a.move(6,28);a.sign8(0,0);a.value(7,1);a.leading_zeroes(0,0);a.mask(0,0,0x7ffffff,false,27);a.sign8(4,0);a.call(0x101e3f10);
    a.floating_load(0,26,0xc0);a.floating_compare(1,0);bool pursue=a.positive();
    if(!pursue){a.load<1>(0,31,0x109);a.sign8(0,0,true);pursue=a.equal();if(pursue){a.load<1>(0,31,0x10d);a.sign8(0,0,true);pursue=a.equal();}}
    if(pursue){if(valid_ball_position(a,0x1028514c)){a.at(0x10285174);keeper_pursuit(a);}return true;}
    keeper_destination(a,0x1028519c);return true;
}
bool attack_half(Call& a,std::uint32_t start,bool live_kind_allows) {
    a.at(start);a.load(3,31,0x1a4);a.load<1>(0,3,0x7924);a.sign8(0,0);a.compare(0,-1);if(!a.equal())return live_kind_allows;
    a.load<1>(0,31,0x27);a.compare(0,1);bool home=!a.equal();bool inside=false;
    if(!home){a.load<1>(0,31,0x106);a.sign8(0,0);a.compare(0,6);inside=a.at_least();}
    if(!inside){
        a.at(start+0x30);a.load<1>(0,31,0x27);a.sign8(0,0,true);
        if(a.equal()){a.load<1>(0,31,0x106);a.sign8(0,0);a.compare(0,6);inside=!a.at_least();}
    }
    if(inside){a.at(start+0x4c);a.value(0,1);a.jump(start+0x58);}
    else{a.at(start+0x54);a.value(0,0);}
    a.at(start+0x58);a.sign8(0,0,true);return !a.equal();
}
void outfield_destination(Call& a,std::uint32_t start) {
    a.at(start);a.sign8(0,20,true);a.value(0,0x68);if(!a.equal())a.value(0,0x100);
    a.at(start+0x10);a.store<2>(31,0x19c,0);a.move(3,31);a.value(6,0x100);a.value(7,0);a.load<1>(4,1,0x3b);a.value(8,0);a.load<1>(5,1,0x3a);a.call(0x1022e7e0);a.jump(0x1028572c);
}
void queue_capture_event(Call& a,std::uint32_t start,std::uint32_t message) {
    a.at(start);a.load(3,31,0x1a4);a.value(4,message);a.load<1>(7,31,0x27);a.value(8,0xffffffff);a.load(3,3,0x79f4);a.value(9,0xffffffff);
    a.load<1>(6,31,0x2f);a.value(10,0);a.load(5,31,0x1a4);a.call(0x10201180);
}
bool special_capture_event(Call& a) {
    a.load(3,31,0x1a4);a.load<1>(6,31,0x106);a.load<1>(0,3,0x1e6);a.load<1>(5,31,0x105);a.sign8(0,0,true);a.load<1>(4,31,0x27);bool away=a.equal();
    if(!away){
        a.sign8(0,4,true);away=!a.equal();
        if(!away){a.sign8(0,6);a.compare(0,10);away=a.less();}
        if(!away){a.sign8(0,5);a.compare(0,5);away=a.positive();}
        if(!away){a.sign8(0,5);a.compare(0,3);away=a.less();}
        if(!away){a.value(0,1);a.jump(0x1028549c);a.at(0x1028549c);a.sign8(0,0,true);return !a.equal();}
    }
    a.at(0x10285454);a.load<1>(0,3,0x1e6);a.sign8(0,0,true);bool no_event=a.equal();
    if(!no_event){a.sign8(0,4);a.compare(0,1);no_event=!a.equal();}
    if(!no_event){a.sign8(0,6);a.compare(0,1);no_event=a.positive();}
    if(!no_event){a.sign8(0,5);a.compare(0,5);no_event=a.positive();}
    if(!no_event){a.sign8(0,5);a.compare(0,3);no_event=a.less();}
    if(!no_event){a.value(0,1);a.jump(0x1028549c);}else{a.at(0x10285498);a.value(0,0);}
    a.at(0x1028549c);a.sign8(0,0,true);return !a.equal();
}
void outfield_capture(Call& a) {
    a.value(3,0x100);a.value(0,11);a.store<2>(31,0x19c,3);a.move(3,31);a.value(6,4);a.load<2>(4,31,0x36,true);a.offset(4,4,4);a.store<2>(31,0x36,4);
    a.load<1>(5,31,0x27);a.load<1>(4,31,0x105);a.sign8(7,5);a.load<1>(5,31,0x106);a.leading_zeroes(7,7);a.mask(7,7,1,false,27);a.negate(7,7);a.intersect(0,0,7);a.sign8(7,0);a.call(0x10237830);
    if(special_capture_event(a)){queue_capture_event(a,0x102854a4,0x2113);a.jump(0x102854f8);}else queue_capture_event(a,0x102854d0,0x1f46);
    a.at(0x102854f8);a.move(3,31);a.call(0x1022e3a0);a.jump(0x1028572c);
}
void outfield_idle_or_move(Call& a) {
    if(goal_area(a,0x1028556c)){
        a.load<1>(0,31,0x105);a.load<1>(3,31,0x106);a.sign8(4,0);a.load<1>(0,31,0x27);a.sign8(3,3);a.load(5,31,0x1a4);a.multiply(4,4,0x450);a.sign8(0,0);
        a.leading_zeroes(0,0);a.mask(0,0,0x7ffffff,false,27);a.multiply(3,3,0x5c);a.add(4,5,4);a.sign8(0,0);a.add(3,4,3);a.add(3,3,0);a.load<1>(0,3,0xa60);a.sign8(0,0,true);
        bool crowded=!a.equal();
        if(crowded){
            a.load<1>(0,31,0x105);a.load<1>(3,31,0x106);a.sign8(4,0);a.load<1>(0,31,0x27);a.sign8(3,3);a.load(5,31,0x1a4);a.multiply(4,4,0x450);a.sign8(0,0);a.multiply(3,3,0x5c);
            a.add(4,5,4);a.add(3,4,3);a.add(3,3,0);a.load<1>(0,3,0xa60);a.compare(0,1);crowded=a.equal();
            if(crowded){a.load<1>(0,31,0x10d);a.sign8(0,0,true);crowded=!a.equal();}
            if(crowded){a.value(0,0x104);a.store<2>(31,0x19c,0);a.jump(0x1028572c);return;}
        }
    }
    a.at(0x102856b4);a.load<1>(3,1,0x3b);a.load<1>(0,31,0x105);a.sign8(3,3);a.sign8(0,0);a.compare_registers(3,0);bool move=!a.equal();
    if(!move){a.load<1>(3,1,0x3a);a.load<1>(0,31,0x106);a.sign8(3,3);a.sign8(0,0);a.compare_registers(3,0);move=!a.equal();}
    if(move){outfield_destination(a,0x102856e4);return;}
    a.at(0x10285718);a.sign8(0,20,true);a.value(0,0x76);if(!a.equal())a.value(0,0x100);a.at(0x10285728);a.store<2>(31,0x19c,0);
}
void choose_outfield_action(Call& a) {
    a.at(0x102851ec);a.load(3,31,0x1a4);a.move(4,31);a.call(0x101e4e10);a.sign8(0,3,true);bool available=!a.less();
    if(available){a.load<1>(0,31,0x10a);a.sign8(0,0,true);available=!a.equal();}
    if(available){a.move(3,31);a.value(4,0);a.call(0x10239e50);a.value(0,0x6b);a.store<2>(31,0x19c,0);a.jump(0x1028572c);return;}
    a.at(0x10285224);a.load(3,31,0x1a4);a.move(4,31);a.call(0x101f5280);a.sign8(0,3,true);
    if(a.equal()){outfield_idle_or_move(a);return;}
    bool pursue=attack_half(a,0x10285238,false);
    if(!pursue){
        const bool traits=attack_half(a,0x10285298,true);
        if(traits){a.at(0x102852f8);a.load<1>(0,31,0x110);a.sign8(0,0,true);pursue=!a.equal();}
        if(!pursue){
            a.at(0x10285304);a.load<1>(0,31,0x110);a.sign8(0,0,true);
            if(a.equal()){outfield_idle_or_move(a);return;}
            a.load(3,31,0x1a4);a.move(4,27);a.load<1>(6,1,0x3b);a.move(5,28);a.load<1>(7,1,0x3a);a.call(0x101ef8d0);a.floating_load(0,24,0x2c,true);a.floating_compare(1,0);
            if(a.at_least()){outfield_idle_or_move(a);return;}
        }
    }
    if(!valid_ball_position(a,0x10285334)){outfield_destination(a,0x10285538);return;}
    a.load<1>(0,31,0x105);a.sign8(3,27);a.sign8(0,0);a.compare_registers(3,0);bool displaced=!a.equal();
    if(!displaced){a.load<1>(0,31,0x106);a.sign8(3,28);a.sign8(0,0);a.compare_registers(3,0);displaced=!a.equal();}
    if(displaced){a.at(0x10285510);a.value(0,0x100);a.move(3,31);a.store<2>(31,0x19c,0);a.move(4,27);a.move(5,28);a.value(6,0x100);a.value(7,0);a.value(8,0);a.call(0x1022e7e0);a.jump(0x1028572c);return;}
    a.load(3,31,0x1a4);a.load<1>(0,3,0x7924);a.sign8(0,0);a.compare(0,-1);bool hold=!a.equal();
    if(!hold){a.load(3,31,0x1a4);a.load<1>(0,3,0x791f);a.sign8(0,0);a.compare(0,5);hold=a.at_least();}
    if(!hold){a.load(3,31,0x1a4);a.load<1>(0,3,0x1de);a.sign8(0,0,true);hold=!a.equal();}
    if(hold){a.at(0x10285504);a.value(0,0x104);a.store<2>(31,0x19c,0);a.jump(0x1028572c);return;}
    outfield_capture(a);
}
}
void decide(State& state,DependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call call(state,entry,dependencies,observer);enter_decision(call);
    if(!choose_dead_ball_priority(call) && !read_tactical_context(call) && !choose_defensive_line(call) &&
        !choose_ball_pursuit(call) && !choose_mark_following(call)){
        select_destination(call);
        if(!choose_keeper_escape(call) && !choose_keeper_action(call))choose_outfield_action(call);
    }
    finish_decision(call);
}
}
