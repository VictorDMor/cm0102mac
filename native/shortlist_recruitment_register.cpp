#include "shortlist_recruitment.hpp"
#include "shortlist_recruitment_detail.hpp"
#include "match_turbo.hpp"
#include "club_control.hpp"

namespace cm::aot::shortlist_recruitment {
using detail::Call;
namespace {
bool club_policy_allows_interest(Call& a) {
    a.move(3,27);a.value(13,0);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
    if(!a.equal()){a.value(3,0);a.jump(0x10364ce8);return false;}
    a.at(0x10363fcc);a.move(3,27);a.value(4,1);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
    if(!a.equal()){
        a.move(3,27);a.call(0x100c96e0);a.move(4,3);a.load(3,14,0);a.call(0x10147e70);a.sign8(0,3,true);
        if(!a.equal()){
            a.move(3,27);a.call(0x100c96e0);a.move(5,3);a.load(3,15,0);a.move(4,26);a.call(0x1030fbf0);a.sign8(0,3,true);if(!a.equal())a.value(13,1);
            a.at(0x1036402c);a.move(3,27);a.call(0x100c96e0);a.move(5,3);a.load(3,14,0);a.value(4,4);a.call(0x10149df0);a.sign8(0,3);a.compare(0,2);
            if(a.equal()){
                a.at(0x10364068);a.move(3,27);a.call(0x100c96e0);a.move(5,3);a.load(3,15,0);a.move(4,26);a.call(0x1030fbf0);a.sign8(0,3,true);
                if(a.equal()){a.value(3,0);a.jump(0x10364ce8);return false;}
            }else if(!a.at_least()){
                a.compare(0,1);if(a.at_least()){a.at(0x10364060);a.value(3,0);a.jump(0x10364ce8);return false;}a.jump(0x10364090);
            }
        }
    }
    return true;
}
bool prepare_registration(Call& a) {
    a.at(0x10364090);a.load(0,27,0);a.move(4,26);a.load(5,25,0xdc);a.multiply(0,0,12);a.load(3,20,0);a.add(15,5,0);a.call(0x10082f20);a.move(16,3,true);
    if(!a.equal()){a.load(0,16,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(30,3,0);a.jump(0x103640d0);}else{a.at(0x103640cc);a.value(30,0);}
    a.at(0x103640d0);a.load<1>(0,15,4);a.compare(0,1);bool active=a.equal();
    if(active){a.load<1>(0,15,9);a.compare(0,100,false);active=a.less();}
    if(!active){a.at(0x103640e8);a.value(3,0);a.jump(0x10364ce8);return false;}
    a.at(0x103640f0);a.load(0,25,0);a.compare(0,0,false);
    if(a.equal()){a.value(0,0);a.jump(0x10364154);}
    else{
        a.at(0x10364104);a.load(0,26,0);a.compare(27,0,false);a.store(1,0x54,0);
        if(!a.equal()){a.load(0,27,0);a.store(1,0x58,0);a.jump(0x10364128);}else{a.at(0x10364120);a.value(0,0xffffffff);a.store(1,0x58,0);}
        a.at(0x10364128);a.load(4,25,0);a.offset(3,1,0x54);a.load(5,25,0x10);a.value(6,8);a.load(7,2,-0x62a8);a.call(0x10775600);a.compare(3,0,false);
        if(!a.equal()){a.value(0,1);a.jump(0x10364154);}else{a.at(0x10364150);a.value(0,0);}
    }
    a.at(0x10364154);a.sign8(0,0,true);if(!a.equal()){a.value(3,1);a.jump(0x10364ce8);return false;}
    a.at(0x10364164);a.load(3,20,0);a.move(4,26);a.call(0x10082f20);a.load(4,2,-0x7b78);a.compare(3,0,false);a.store(4,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(5,3,0);a.jump(0x1036419c);}else{a.at(0x10364198);a.value(5,0);}
    a.at(0x1036419c);a.move(3,25);a.move(4,26);a.move(6,27);a.call(0x1036f590);a.sign8(0,3,true);if(a.equal()){a.value(3,0);a.jump(0x10364ce8);return false;}
    a.at(0x103641bc);a.load(3,2,-0x7b40);a.move(4,26);a.move(5,27);a.value(6,0);a.load(3,3,0);a.value(7,0);a.call(0x103f83c0);a.sign8(0,3,true);if(a.equal()){a.value(3,0);a.jump(0x10364ce8);return false;}
    a.at(0x103641e8);a.load(3,2,-0x7ab8);a.move(4,27);a.load(3,3,0);a.call(0x10103510);a.compare(3,0,false);if(a.equal()){a.value(3,0);a.jump(0x10364ce8);return false;}
    a.at(0x10364208);a.value(4,0);a.value(5,0);a.value(6,0);a.value(7,1);a.value(8,0);a.call(0x100f5770);a.move(14,4,true);if(!a.at_least())a.value(14,0);
    a.at(0x1036422c);a.load<2>(3,27,0x1ec,true);a.value(0,0x43300000);a.store(1,0xe0,0);a.exclusive_flags(0,3,0x80000000);a.floating_load(2,21,8);a.store(1,0xe4,0);a.floating_load(0,21,0x118);a.floating_load(1,1,0xe0);a.floating_load(31,21,0x80);a.floating_subtract(1,1,2);a.floating_difference(0,1,0,true);a.floating_compare(0,31);if(!a.nonpositive())a.floating_move(31,0);
    a.at(0x10364264);a.value(17,0);a.load(15,15,0);a.flags(0,0,0);return true;
}
bool row_is_available(Call& a) {
    a.at(0x10364270);a.load<1>(0,15,0x28);a.value(28,0xffffffff);a.mask(0,0,1,true,25);if(!a.equal())return false;
    a.load<1>(0,15,0);a.compare(0,11);if(!a.equal())return false;
    a.load(0,15,0x18);a.compare(0,-1);if(!a.equal())return false;
    a.load<2>(3,15,2,true);a.sign16(0,3,true);if(a.equal())return false;
    a.load<2>(0,1,0x176,true);a.intersect(0,3,0);a.compare_registers(3,0);if(!a.equal())return false;
    a.move(3,25);a.move(4,15);a.move(5,16);a.move(6,27);a.offset(7,1,0x40);a.call(0x1036fbf0);a.compare(16,0,false);a.move(23,3);
    if(!a.equal()){
        a.sign8(0,23,true);
        if(a.equal()){
            a.load<2>(0,27,0x1ec,true);a.compare(0,7000);
            if(!a.at_least()){
                a.load(4,15,8);a.value(3,0x43300000);a.load(0,26,8);a.exclusive_flags(4,4,0x80000000);a.store(1,0xe0,3);a.exclusive_flags(0,0,0x80000000);a.floating_load(2,21,8);a.store(1,0xe4,4);a.floating_load(3,21,0x70);a.floating_load(0,1,0xe0);a.store(1,0xdc,0);a.floating_subtract(1,0,2);a.store(1,0xd8,3);a.floating_load(0,1,0xd8);a.floating_product(1,3,1);a.floating_subtract(0,0,2);a.floating_compare(1,0);
                if(!a.at_least()){
                    a.compare(30,0,false);if(a.equal())return false;
                    a.move(3,30);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);if(a.equal())return false;
                    a.load(3,26,8);a.load(0,15,8);a.offset(3,3,-0x170000);a.offset(3,3,0x1ca0);a.compare_registers(3,0);if(a.at_least())return false;
                    a.move(3,26);a.value(4,0);a.call(0x100d3040);a.sign8(0,3,true);if(a.equal())return false;
                }
            }
        }
    }
    a.at(0x10364388);a.load(3,15,0x10);a.load(0,15,0x14);a.compare_registers(3,0);
    if(a.equal()){a.compare(3,-1);if(!a.equal())return false;}
    a.at(0x103643a0);a.load(3,15,0x1c);a.load(0,15,0x20);a.store(1,0x4c,3);a.load(3,2,-0x7b80);a.load<2>(4,1,0x4e);a.load<2>(3,3,2);a.store(1,0x50,0);a.compare_registers(4,3,false);
    if(!a.at_least()){a.value(0,1);a.jump(0x103643fc);}
    else{a.at(0x103643cc);if(!a.nonpositive()){a.value(0,0);a.jump(0x103643fc);}else{a.at(0x103643d8);a.load(3,2,-0x7b80);a.load<2>(0,1,0x4c,true);a.load<2>(4,3,0,true);a.combine(0,4,0,false,true);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);}}
    a.at(0x103643fc);a.sign8(0,0,true);return !a.equal();
}
bool category_policy_allows_row(Call& a) {
    a.at(0x10364404);a.load<1>(0,15,0x28);a.mask(3,0,15,false,29);a.compare(3,5,false);
    if(a.equal()){a.load<2>(0,26,0x5c,true);a.sign8(0,0);a.compare(0,24);if(a.positive())return false;}
    a.at(0x10364424);a.compare(3,5,false);bool foreign=a.equal();
    if(foreign){a.sign8(0,13,true);foreign=a.equal();}
    if(foreign){a.load(3,27,0x2c);a.load(0,26,12);a.compare_registers(3,0,false);foreign=!a.equal();}
    if(foreign){a.load(4,26,0x24);a.compare(4,0,false);if(!a.equal()){a.load(0,4,0x2c);a.compare_registers(0,3,false);foreign=!a.equal();}}
    if(foreign){
        a.at(0x1036445c);a.load(3,26,0x14);a.load<2>(0,3,10,true);a.compare(0,1000);if(a.less())return false;
        a.load(3,2,-0x7b88);a.load(4,22,0);a.load(0,3,0);a.offset(3,4,2);a.compare_registers(3,0,false);a.store(22,0,3);
        if(!a.nonpositive()){a.load(0,2,-0x7b8c);a.store(22,0,0);a.call(0x1077b770);a.mask(0,3,0xffff);a.load(3,2,-0x7b90);a.store(3,0,0);}
        a.at(0x103644a0);a.load(5,22,0);a.value(4,0x10620000);a.load(3,2,-0x7b90);a.offset(0,4,0x4dd3);a.load<2>(7,5,0);a.value(5,0);a.load(6,3,0);a.move(3,26);a.load(4,27,0x90);a.add(7,7,6);a.multiply_high(0,0,7);a.shift_right(0,0,7);a.mask(6,0,1,false,1);a.add(0,0,6);a.multiply(0,0,2000);a.subtract(18,0,7);a.call(0x100cdef0);a.sign16(3,3);a.offset(0,18,1000);a.compare_registers(3,0);if(a.less())return false;
    }
    a.at(0x103644f4);a.load(3,2,-0x7b40);a.move(4,26);a.move(5,27);a.value(6,1);a.load(3,3,0);a.call(0x103ecef0);a.sign8(0,3,true);
    if(!a.equal()){a.load<1>(0,15,0x28);a.mask(0,0,15,false,29);a.compare(0,1,false);if(!a.equal()){a.compare(0,2,false);if(!a.equal()){a.compare(0,3,false);if(!a.equal())return false;}}}
    return true;
}
void adjust_row_threshold(Call& a) {
    a.at(0x10364534);a.value(4,0x43300000);a.exclusive_flags(3,14,0x80000000);a.store(1,0xe4,3);a.load(5,15,8);a.store(1,0xe0,4);a.exclusive_flags(0,5,0x80000000);a.floating_load(4,21,8);a.floating_load(0,1,0xe0);a.store(1,0xdc,0);a.floating_subtract(0,0,4);a.floating_load(1,21,0x58);a.store(1,0xd8,4);a.floating_load(2,1,0xd8);a.floating_product(0,1,0);a.floating_subtract(1,2,4);a.floating_compare(1,0);a.compare_less_or_equal_bit();bool adjust=a.equal();
    if(adjust){a.sign8(0,23,true);adjust=a.equal();}
    if(adjust){
        a.load(0,15,4);a.move(28,5);a.store(1,0xd8,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(2,21,0x2c8);a.store(1,0xdc,0);a.floating_load(1,21,0xf8);a.floating_load(0,1,0xd8);a.store(1,0xe4,0);a.floating_subtract(0,0,4,true);a.store(1,0xe0,4);a.floating_product(3,2,0);a.floating_load(0,1,0xe0);a.store(1,0xc4,3);a.floating_subtract(2,0,4,true);a.store(1,0xc0,4);a.floating_round_single(3,3);a.floating_load(0,1,0xc0);a.store(1,0xc8,4);a.floating_product_single(2,3,2);a.floating_subtract(0,0,4);a.floating_integer(2,2);a.floating_product(1,1,0);a.floating_store(2,1,0xd0);a.load(0,1,0xd4);a.store(15,8,0);a.load(0,15,8);a.exclusive_flags(0,0,0x80000000);a.store(1,0xcc,0);a.floating_load(0,1,0xc8);a.floating_subtract(0,0,4);a.floating_compare(0,1);
        if(!a.nonpositive()){a.floating_integer(0,1);a.floating_store(0,1,0xc0);a.load(0,1,0xc4);a.store(15,8,0);}
    }
    a.at(0x10364618);a.load(0,15,8);a.compare(0,1);if(!a.nonpositive()){a.move(29,0);a.jump(0x10364630);}else{a.at(0x1036462c);a.value(29,1);}
}
bool previous_staff_contribution(Call& a) {
    a.at(0x10364630);a.load(0,15,0x10);a.compare(0,-1);
    if(!a.equal()){
        a.load(3,2,-0x7be0);a.multiply(0,0,0x70);a.load(3,3,0);a.add(18,3,0);a.load(0,18,0x14);a.compare(0,0,false);
        if(!a.equal()){
            a.load(31,15,4);a.move(3,25);a.move(4,27);a.move(5,18);a.move(6,15);a.move(8,31);a.offset(7,1,0x48);a.value(9,1);a.call(0x10361490);a.move(24,3);a.load(3,20,0);a.move(4,18);a.call(0x10082f20);a.load(4,2,-0x7b78);a.compare(3,0,false);a.store(4,0,3);
            if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(19,3,0);a.jump(0x103646b8);}else{a.at(0x103646b4);a.value(19,0);}
            a.at(0x103646b8);a.compare(19,0,false);
            if(a.equal()){a.value(3,0);a.jump(0x10364720);}
            else{
                a.at(0x103646c8);a.move(3,19);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
                if(!a.equal()){a.load(3,20,0);a.move(4,19);a.move(5,27);a.move(6,18);a.call(0x10089a40);a.jump(0x10364720);}
                else{a.at(0x10364700);a.load(3,20,0);a.move(4,19);a.move(5,27);a.move(6,18);a.value(7,0);a.value(8,1);a.value(9,1);a.call(0x10085860);}
            }
            a.at(0x10364720);a.value(4,0x43300000);a.exclusive_flags(0,29,0x80000000);a.store(1,0xcc,0);a.subtract(0,24,3);a.exclusive_flags(0,0,0x80000000);a.floating_load(2,21,8);a.store(1,0xc8,4);a.floating_load(3,21,0x80);a.floating_load(0,1,0xc8);a.store(1,0xc4,0);a.floating_subtract(0,0,2);a.floating_load(4,21,0x148);a.store(1,0xc0,4);a.floating_load(1,1,0xc0);a.floating_product(0,31,0);a.floating_subtract(1,1,2);a.floating_difference(0,1,0,true);a.floating_subtract(0,3,0);detail::clamp_interest_ratio(a,0x10364768,21);
            a.load(3,1,0x48);a.value(0,0x43300000);a.store(1,0xd0,0);a.exclusive_flags(0,3,0x80000000);a.floating_load(1,21,8);a.store(1,0xd4,0);a.floating_load(0,1,0xd0);a.floating_subtract(0,0,1);a.floating_product(0,0,4);a.floating_integer(0,0);a.floating_store(0,1,0xd8);a.load(3,1,0xdc);a.store(1,0x48,3);a.load(0,15,4);a.compare_registers(3,0);if(a.at_least())return false;
            a.compare_registers(3,31);a.move(0,31);if(!a.nonpositive())a.move(0,3);
            a.at(0x103647f4);a.load(3,2,-0x7ba0);a.move(4,18);a.store(1,0x48,0);a.load(3,3,0);a.call(0x1018c340);a.compare(3,0,false);
            if(!a.equal()){a.load<2>(3,3,0x34,true);a.compare(3,15);if(!a.nonpositive()){a.offset(3,3,-15);a.load(0,1,0x48);a.product(3,3,3);a.subtract(0,3,0);a.store(1,0x48,0);a.jump(0x10364848);}}
            return true;
        }
    }
    a.at(0x10364834);a.value(0,0xffffffff);a.value(18,0);a.store(15,0x10,0);a.load(31,15,4);a.store(1,0x48,31);return true;
}
void candidate_staff_contribution(Call& a) {
    a.at(0x10364848);a.move(3,25);a.move(4,27);a.move(5,26);a.move(6,15);a.move(8,31);a.offset(7,1,0x44);a.value(9,1);a.call(0x10361490);a.compare(30,0,false);a.move(24,3);bool adjust=!a.equal();
    if(adjust){a.load(3,2,-0x7ab8);a.move(4,30);a.load(3,3,0);a.call(0x10103510);a.compare(3,0,false);adjust=!a.equal();}
    if(adjust){a.load(3,2,-0x7ab8);a.move(4,30);a.load(3,3,0);a.call(0x10103510);a.load<1>(0,3,0x165);a.compare(0,1);adjust=a.equal();}
    if(!adjust)return;
    a.move(3,30);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
    if(!a.equal()){
        a.load(3,20,0);a.move(4,26);a.move(5,30);a.value(6,0);a.value(7,0xffffffff);a.value(8,0);a.call(0x10084be0);a.compare_registers(24,3);
        if(!a.at_least()){a.move(3,24);a.jump(0x10364910);}else{a.at(0x103648f4);a.load(3,20,0);a.move(4,26);a.move(5,30);a.value(6,0);a.value(7,0xffffffff);a.value(8,0);a.call(0x10084be0);}
        a.at(0x10364910);a.move(24,3);a.jump(0x1036492c);
    }else{a.at(0x10364918);a.load(0,26,8);a.compare_registers(24,0);if(!a.at_least())a.move(0,24);a.at(0x10364928);a.move(24,0);}
}
bool candidate_contract_allows_row(Call& a) {
    a.at(0x1036492c);a.compare(30,0,false);bool linked=!a.equal();
    if(linked){a.sign8(0,23,true);linked=a.equal();}
    if(!linked){a.at(0x1036493c);a.value(19,0);a.jump(0x103649a4);}
    else{
        a.at(0x10364944);a.move(3,30);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
        if(!a.equal()){a.load(3,20,0);a.move(4,30);a.move(5,27);a.move(6,26);a.call(0x10089a40);a.move(19,3);a.jump(0x103649a4);}
        else{a.at(0x10364980);a.load(3,20,0);a.move(4,30);a.move(5,27);a.move(6,26);a.value(7,0);a.value(8,1);a.value(9,1);a.call(0x10085860);a.move(19,3);}
    }
    a.at(0x103649a4);a.load(3,1,0x40);a.compare(3,0);bool bound=!a.equal();if(bound){a.sign8(0,23,true);bound=!a.equal();}
    if(bound){a.compare_registers(3,19);a.move(0,19);if(!a.at_least())a.move(0,3);a.at(0x103649c8);a.move(19,0);}
    a.at(0x103649cc);a.compare(30,0,false);bool offer=!a.equal();if(offer){a.sign8(0,23,true);offer=a.equal();}
    if(offer){
        a.shift_right(6,24,31);a.value(0,0);a.store(1,0x38,0);a.shift_right(9,19,31);a.move(3,25);a.move(4,30);a.move(5,26);a.move(7,24);a.move(8,27);a.move(10,19);a.call(0x1036f170);a.sign8(0,3);a.compare(0,2);
        if(a.equal()){a.compare(28,-1);if(!a.equal()){a.store(15,8,28);a.jump(0x10364cd4);}return false;}
    }
    return true;
}
bool complete_registration(Call& a) {
    a.at(0x10364a24);a.value(3,0x43300000);a.exclusive_flags(0,29,0x80000000);a.store(1,0xcc,0);a.subtract(0,24,19);a.exclusive_flags(0,0,0x80000000);a.floating_load(2,21,8);a.store(1,0xc8,3);a.floating_load(3,21,0x80);a.floating_load(0,1,0xc8);a.store(1,0xc4,0);a.floating_subtract(0,0,2);a.floating_load(4,21,0x148);a.store(1,0xc0,3);a.floating_load(1,1,0xc0);a.floating_product(0,31,0);a.floating_subtract(1,1,2);a.floating_difference(0,1,0,true);a.floating_subtract(0,3,0);detail::clamp_interest_ratio(a,0x10364a6c,21);
    a.load(3,1,0x44);a.value(0,0x43300000);a.store(1,0xd0,0);a.move(4,26);a.exclusive_flags(0,3,0x80000000);a.load(3,2,-0x7ba0);a.store(1,0xd4,0);a.floating_load(1,21,8);a.floating_load(0,1,0xd0);a.load(3,3,0);a.floating_subtract(0,0,1);a.floating_product(0,0,4);a.floating_integer(0,0);a.floating_store(0,1,0xd8);a.load(0,1,0xdc);a.store(1,0x44,0);a.call(0x1018c340);a.compare(3,0,false);
    if(!a.equal()){a.load<2>(3,3,0x34,true);a.compare(3,15);if(!a.nonpositive()){a.offset(3,3,-15);a.load(0,1,0x44);a.product(3,3,3);a.subtract(0,3,0);a.store(1,0x44,0);}}
    a.at(0x10364b14);a.load(3,1,0x44);a.load(0,1,0x48);a.compare_registers(3,0);
    if(a.less()){a.compare(28,-1);if(!a.equal()){a.store(15,8,28);a.jump(0x10364cd4);}return false;}
    a.at(0x10364b34);a.load<1>(0,15,0x28);a.move(3,25);a.move(4,26);a.move(5,27);a.mask(0,0,15,false,29);a.value(6,1);a.sign8(8,0);a.value(7,0);a.value(9,0);a.call(0x10369080);a.floating_load(0,21,0x78);a.floating_compare(1,0);a.compare_greater_or_equal_bit();
    if(!a.equal()){a.at(0x10364cc8);a.compare(28,-1);if(!a.equal())a.store(15,8,28);return false;}
    a.load<1>(3,15,0x28);a.mask(0,3,1,true,25);
    if(a.equal()){
        a.mask(0,3,15,false,29);a.move(4,26);a.move(5,27);a.offset(3,1,0x5c);a.sign8(7,0);a.value(6,0);a.value(8,11);a.value(9,0);a.call(0x1037b6f0);a.load<1>(0,15,0x28);a.floating_move(31,1);a.move(3,25);a.move(4,26);a.mask(0,0,15,false,29);a.move(5,27);a.sign8(8,0);a.value(6,0);a.value(7,0);a.value(9,0);a.call(0x10369080);a.floating_load(0,21,0x48);a.floating_compare(0,1);
        if(!a.nonpositive())a.jump(0x10364c04);
        else{a.at(0x10364bd8);a.load<1>(0,15,0x28);a.move(3,25);a.move(4,26);a.move(5,27);a.mask(0,0,15,false,29);a.value(6,0);a.sign8(8,0);a.value(7,0);a.value(9,0);a.call(0x10369080);a.floating_move(0,1);}
        a.at(0x10364c04);a.floating_round_single(1,0);a.floating_load(2,21,0x160);a.floating_load(0,21,0x168);a.floating_difference(1,2,1,true);a.floating_round_single(1,1);a.floating_compare(1,0);
        if(!a.at_least()){a.load(3,2,-0x4de8);a.floating_load(1,3,8,true);}
        a.at(0x10364c28);a.load(3,2,-0x7ab8);a.floating_product_single(31,1,31);a.move(4,27);a.load(3,3,0);a.call(0x10103510);a.load<1>(0,15,0x28);a.move(4,26);a.mask(0,0,15,false,29);a.sign8(5,0);a.call(0x100f2680);a.exclusive_flags(3,3,0x80000000);a.value(0,0x43300000);a.store(1,0xc4,3);a.floating_load(2,21,8);a.store(1,0xc0,0);a.floating_load(0,21,0x170);a.floating_load(1,1,0xc0);a.floating_subtract(1,1,2,true);a.floating_product(0,0,1);a.floating_compare(31,0);
        if(!a.nonpositive()){a.value(3,0);a.jump(0x10364ce8);return true;}
    }
    a.at(0x10364c84);a.load(0,26,0);a.compare(18,0,false);a.store(15,0x10,0);
    if(!a.equal()){a.move(3,25);a.move(4,18);a.move(5,27);a.value(6,3);a.value(7,0);a.call(0x10365590);}
    a.at(0x10364cac);a.move(3,25);a.move(4,26);a.move(5,27);a.call(0x103632f0);a.value(3,1);a.jump(0x10364ce8);return true;
}
}
// The app and the replays: no observer, no instruction budget, no read
// observers, no PC profile, and no host code watching a PC of the routine
// (the conditions under which SemanticGuestCall counts in batches).
bool plain_run(const State& s,SemanticGuestObserver* o) {
    return !o&&s.budget==UINT64_MAX&&!s.memory.date_observer&&!s.memory.read_trace&&!match_pc_profile_enabled()&&
           !routine_watches_pc(register_club_interest_entry);
}
// Most calls end before the routine's first effect on game state: the
// club's recruitment entry (12 bytes per club id from base+0xdc) is inactive,
// 74% of 1.76M calls on a 26-country day. Until that check the original
// writes only stack: its frame, a local contract draft (0x103721f0) and the
// frames of two control queries (0x1014c2e0, with 0x100ce250). The game reads
// stack it never initialised and copies some of it into heap records, so for
// clubs where both queries take their plain path (club_control::
// plainly_uncontrolled: no human manager, not among the last nation-count
// clubs), plain runs (the app and the replays) write that same stack image
// here and return 0 as the original does. The scratch registers and the
// instruction count are not reproduced. Every other club runs the original.
bool settles_without_effect(State& s,SemanticGuestObserver* o) {
    if(!plain_run(s,o))return false;
    const auto r=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
    const auto word=[&](std::uint32_t address){return std::uint32_t(s.memory.read<4>(address));};
    const auto base=r(3),club=r(5),sp=r(1),link=std::uint32_t(s.r<4>(4128));
    std::uint32_t control_r31=0;
    try {
        if(!club_control::plainly_uncontrolled(s,club))return false;
        control_r31=club_control::human_table_slot(s);
        const auto entry=word(base+0xdc)+word(club)*12;
        if(s.memory.read<1>(entry+4)==1&&s.memory.read<1>(entry+9)<100)return false;
        s.memory.read<1>(sp-0x150-0x80-0x70);s.memory.read<1>(sp+0x27);   // the stack the image covers
    } catch(const std::runtime_error&) { return false; }
    const auto frame=sp-0x150,draft=frame+0x5c,inner=frame-0x80;
    // 0x10363f60 prologue.
    s.memory.write<8>(sp-8,s.r<8>(0x4000+31*16));
    for(unsigned n=13;n<32;++n)s.memory.write<4>(sp-0x5c+(n-13)*4,r(n));
    s.memory.write<4>(sp+8,link);
    s.memory.write<4>(frame,sp);
    s.memory.write<2>(sp+0x26,r(6));
    // 0x103721f0 on the draft at frame+0x5c, with its two dates (0x10715880).
    s.memory.write<4>(frame-4,r(31));s.memory.write<4>(frame+8,0x10363fa0);s.memory.write<4>(frame-0x40,frame);
    for(const auto date:{draft+0x28,draft+0x30}){s.memory.write<2>(date,0x28);s.memory.write<2>(date+2,0x7b4);s.memory.write<1>(date+4,1);}
    s.memory.write<4>(draft+4,0xffffffffu);s.memory.write<1>(draft+0x54,0);s.memory.write<4>(draft+0x58,0xffffffffu);
    s.memory.write<2>(draft+0x42,0);s.memory.write<1>(draft+0x5c,0);s.memory.write<4>(draft+0x48,0);
    s.memory.write<4>(draft+0x50,0);s.memory.write<4>(draft+8,0);
    // Two 0x1014c2e0 calls (r24-r31 as the routine holds them then), each
    // calling 0x100ce250 from its frame.
    const std::array<std::uint32_t,8> saved{r(24),base,r(4),club,r(28),r(29),r(30),r(31)};
    for(const auto returned:{0x10363fbcu,0x10363fe4u}) {
        for(unsigned n=0;n<8;++n)s.memory.write<4>(frame-0x20+n*4,saved[n]);
        s.memory.write<4>(frame+8,returned);s.memory.write<4>(inner,frame);
        s.memory.write<4>(inner-4,control_r31);s.memory.write<4>(inner-8,0);
        s.memory.write<4>(inner+8,0x1014c44c);s.memory.write<4>(inner-0x70,inner);
    }
    s.wr<4>(0,link);s.wr<4>(12,0);s.pc=link;              // li r3,0 ... lwz r0,0x158(r1); blr
    return true;
}

void register_club_interest(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    if(settles_without_effect(s,o))return;
    Call a(s,register_club_interest_entry,d,o);a.link();a.floating_store(31,1,-8);a.multiple(13,false,-16);a.move(25,3);a.load(15,2,-0x7b1c);a.move(26,4);a.load(14,2,-0x7a54);a.move(27,5);a.load(20,2,-0x7b7c);a.load(21,2,-0x4ddc);a.load(22,2,-0x7b84);a.save_link();a.frame(0x150);a.store<2>(1,0x176,6);a.offset(3,1,0x5c);a.call(0x103721f0);
    if(club_policy_allows_interest(a)&&prepare_registration(a)){
        for(;;){
            bool returned=false;
            if(row_is_available(a)&&category_policy_allows_row(a)){
                adjust_row_threshold(a);
                if(previous_staff_contribution(a)){candidate_staff_contribution(a);if(candidate_contract_allows_row(a))returned=complete_registration(a);}
            }
            if(returned)break;
            a.at(0x10364cd4);a.offset(17,17,1);a.offset(15,15,44);a.compare(17,40);if(!a.less()){a.value(3,0);break;}
        }
    }
    a.at(0x10364ce8);a.load(0,1,0x158);a.offset(1,1,0x150);a.floating_load(31,1,-8);a.restore_link();a.multiple(13,true,-16);a.finish();
}
}
