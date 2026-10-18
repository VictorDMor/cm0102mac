#include "shortlist_recruitment.hpp"
#include "shortlist_recruitment_detail.hpp"

namespace cm::aot::shortlist_recruitment {
using detail::Call;
namespace {
bool prepare_requested_interest(Call& a) {
    a.load(3,28,0);a.call(0x10082f20);a.load(4,2,-0x7b78);a.compare(3,0,false);a.store(4,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(25,3,0);a.jump(0x10364d74);}else{a.at(0x10364d70);a.value(25,0);}
    a.at(0x10364d74);a.load(0,17,0);a.load(4,15,0xdc);a.multiply(0,0,12);a.load(3,16,0x14);a.add(20,4,0);a.call(0x10117490);detail::expand_role_qualifications(a,0x10364d8c,19);
    a.load<1>(0,20,4);a.compare(0,1);bool active=a.equal();if(active){a.load<1>(0,20,9);a.compare(0,100,false);active=a.less();}
    if(!active){a.at(0x10364df8);a.value(3,0);a.jump(0x10365574);return false;}
    a.at(0x10364e00);a.load(0,15,0);a.compare(0,0,false);
    if(a.equal()){a.value(0,0);a.jump(0x10364e64);}
    else{
        a.at(0x10364e14);a.load(0,16,0);a.compare(17,0,false);a.store(1,0x48,0);
        if(!a.equal()){a.load(0,17,0);a.store(1,0x4c,0);a.jump(0x10364e38);}else{a.at(0x10364e30);a.value(0,0xffffffff);a.store(1,0x4c,0);}
        a.at(0x10364e38);a.load(4,15,0);a.offset(3,1,0x48);a.load(5,15,0x10);a.value(6,8);a.load(7,2,-0x62a8);a.call(0x10775600);a.compare(3,0,false);
        if(!a.equal()){a.value(0,1);a.jump(0x10364e64);}else{a.at(0x10364e60);a.value(0,0);}
    }
    a.at(0x10364e64);a.sign8(0,0,true);if(!a.equal()){a.value(3,1);a.jump(0x10365574);return false;}
    a.at(0x10364e74);a.load(3,28,0);a.move(4,16);a.call(0x10082f20);a.load(4,2,-0x7b78);a.compare(3,0,false);a.store(4,0,3);
    if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(5,3,0);a.jump(0x10364eac);}else{a.at(0x10364ea8);a.value(5,0);}
    a.at(0x10364eac);a.move(3,15);a.move(4,16);a.move(6,17);a.call(0x1036f590);a.sign8(0,3,true);if(a.equal()){a.value(3,0);a.jump(0x10365574);return false;}
    a.at(0x10364ecc);a.load(3,2,-0x7b40);a.move(4,16);a.move(5,17);a.value(6,0);a.load(3,3,0);a.value(7,1);a.call(0x103f83c0);a.sign8(0,3,true);if(a.equal()){a.value(3,0);a.jump(0x10365574);return false;}
    a.at(0x10364ef8);a.value(23,0);a.load(22,20,0);return true;
}
bool requested_row_available(Call& a) {
    a.at(0x10364f00);a.load<1>(0,22,0x28);a.mask(0,0,1,true,25);if(!a.equal())return false;
    a.load<1>(0,22,0);a.compare(0,11);if(!a.equal())return false;
    a.load(0,22,0x18);a.compare(0,-1);if(!a.equal())return false;
    a.load<2>(3,22,2,true);a.sign16(0,19);a.intersect(0,3,0);a.compare_registers(3,0);if(!a.equal())return false;
    a.load(3,22,0x10);a.load(0,22,0x14);a.compare_registers(3,0);if(a.equal()){a.compare(3,-1);if(!a.equal())return false;}
    a.at(0x10364f50);a.load(3,22,0x1c);a.load(0,22,0x20);a.store(1,0x40,3);a.load<2>(3,27,2);a.load<2>(4,1,0x42);a.store(1,0x44,0);a.compare_registers(4,3,false);
    if(!a.at_least()){a.value(0,1);a.jump(0x10364fa4);}
    else{a.at(0x10364f78);if(!a.nonpositive()){a.value(0,0);a.jump(0x10364fa4);}else{a.at(0x10364f84);a.load<2>(0,1,0x40,true);a.load<2>(4,27,0,true);a.combine(0,4,0,false,true);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);}}
    a.at(0x10364fa4);a.sign8(0,0,true);return !a.equal();
}
bool requested_category_policy(Call& a) {
    a.at(0x10364fac);a.load<1>(0,22,0x28);a.mask(3,0,15,false,29);a.compare(3,5,false);
    if(a.equal()){a.load<2>(0,16,0x5c,true);a.sign8(0,0);a.compare(0,24);if(a.positive())return false;}
    a.at(0x10364fcc);a.compare(3,5,false);bool foreign=a.equal();
    if(foreign){a.load(3,17,0x2c);a.load(0,16,12);a.compare_registers(3,0,false);foreign=!a.equal();}
    if(foreign){a.load(4,16,0x24);a.compare(4,0,false);if(!a.equal()){a.load(0,4,0x2c);a.compare_registers(0,3,false);foreign=!a.equal();}}
    if(foreign){
        a.at(0x10364ffc);a.load(3,16,0x14);a.load<2>(0,3,10,true);a.compare(0,5000);if(a.less())return false;
        a.load(3,2,-0x7b88);a.load(4,30,0);a.load(0,3,0);a.offset(3,4,2);a.compare_registers(3,0,false);a.store(30,0,3);
        if(!a.nonpositive()){a.load(0,2,-0x7b8c);a.store(30,0,0);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(13,0,0);}
        a.at(0x1036503c);a.load(4,30,0);a.value(3,0x10620000);a.load(6,13,0);a.offset(0,3,0x4dd3);a.load<2>(7,4,0);a.move(3,16);a.load(4,17,0x90);a.value(5,0);a.add(7,7,6);a.multiply_high(0,0,7);a.shift_right(0,0,6);a.mask(6,0,1,false,1);a.add(0,0,6);a.multiply(0,0,1000);a.subtract(20,0,7);a.call(0x100cdef0);a.sign16(3,3);a.offset(0,20,500);a.compare_registers(3,0);if(a.less())return false;
    }
    a.at(0x1036508c);a.load(3,2,-0x7b40);a.move(4,16);a.move(5,17);a.value(6,1);a.load(3,3,0);a.call(0x103ecef0);a.sign8(0,3,true);
    if(!a.equal()){a.load<1>(0,22,0x28);a.mask(0,0,15,false,29);a.compare(0,1,false);if(!a.equal()){a.compare(0,2,false);if(!a.equal()){a.compare(0,3,false);if(!a.equal())return false;}}}
    return true;
}
bool requested_previous_contribution(Call& a) {
    a.at(0x103650cc);a.load(0,22,0x10);a.compare(0,-1);
    if(!a.equal()){
        a.load(3,2,-0x7be0);a.multiply(0,0,0x70);a.load(20,22,4);a.move(4,17);a.load(5,3,0);a.move(3,15);a.move(6,22);a.add(21,5,0);a.move(8,20);a.move(5,21);a.offset(7,1,0x3c);a.value(9,1);a.call(0x10361490);a.move(31,3);a.load(3,28,0);a.move(4,21);a.call(0x10082f20);a.load(4,2,-0x7b78);a.compare(3,0,false);a.store(4,0,3);
        if(!a.equal()){a.load(0,3,4);a.load(3,2,-0x7be8);a.multiply(0,0,0x248);a.load(3,3,0);a.add(24,3,0);a.jump(0x10365148);}else{a.at(0x10365144);a.value(24,0);}
        a.at(0x10365148);a.compare(24,0,false);
        if(a.equal()){a.value(3,0);a.jump(0x103651b0);}
        else{
            a.at(0x10365158);a.move(3,24);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
            if(!a.equal()){a.load(3,28,0);a.move(4,24);a.move(5,17);a.move(6,21);a.call(0x10089a40);a.jump(0x103651b0);}
            else{a.at(0x10365190);a.load(3,28,0);a.move(4,24);a.move(5,17);a.move(6,21);a.value(7,0);a.value(8,1);a.value(9,1);a.call(0x10085860);}
        }
        a.at(0x103651b0);a.load<2>(0,17,0x1ec,true);a.value(5,0x43300000);a.value(4,0x66660000);a.subtract(6,31,3);a.product(0,0,0);a.store(1,0x68,5);a.offset(3,4,0x6667);a.floating_load(2,29,8);a.exclusive_flags(4,6,0x80000000);a.store(1,0x60,5);a.multiply_high(0,3,0);a.store(1,0x6c,4);a.floating_load(3,29,0x80);a.floating_load(0,1,0x68);a.floating_load(4,29,0x148);a.floating_subtract(1,0,2);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.exclusive_flags(0,0,0x80000000);a.store(1,0x64,0);a.floating_load(0,1,0x60);a.floating_subtract(0,0,2);a.floating_difference(0,1,0,true);a.floating_subtract(0,3,0);detail::clamp_interest_ratio(a,0x10365214,29);
        a.load(3,1,0x3c);a.value(0,0x43300000);a.store(1,0x58,0);a.exclusive_flags(0,3,0x80000000);a.floating_load(1,29,8);a.store(1,0x5c,0);a.floating_load(0,1,0x58);a.floating_subtract(0,0,1);a.floating_product(0,0,4);a.floating_integer(0,0);a.floating_store(0,1,0x50);a.load(3,1,0x54);a.store(1,0x3c,3);a.load(0,22,4);a.compare_registers(3,0);if(a.at_least())return false;
        a.compare_registers(3,20);a.move(0,20);if(!a.nonpositive())a.move(0,3);
        a.at(0x103652a0);a.store(1,0x3c,0);a.move(4,21);a.load(3,26,0);a.call(0x1018c340);a.compare(3,0,false);
        if(!a.equal()){a.load<2>(3,3,0x34,true);a.compare(3,15);if(!a.nonpositive()){a.offset(3,3,-15);a.load(0,1,0x3c);a.product(3,3,3);a.subtract(0,3,0);a.store(1,0x3c,0);a.jump(0x103652e8);}}
        return true;
    }
    a.at(0x103652dc);a.load(20,22,4);a.value(21,0);a.store(1,0x3c,20);return true;
}
void requested_candidate_contribution(Call& a) {
    a.at(0x103652e8);a.move(3,15);a.move(4,17);a.move(5,16);a.move(6,22);a.move(8,20);a.offset(7,1,0x38);a.value(9,0);a.call(0x10361490);a.compare(25,0,false);a.move(20,3);bool adjust=!a.equal();
    if(adjust){a.load(3,14,0);a.move(4,25);a.call(0x10103510);a.compare(3,0,false);adjust=!a.equal();}
    if(adjust){a.load(3,14,0);a.move(4,25);a.call(0x10103510);a.load<1>(0,3,0x165);a.compare(0,1);adjust=a.equal();}
    if(!adjust)return;
    a.move(3,25);a.value(4,0);a.value(5,1);a.value(6,0);a.value(7,0);a.call(0x1014c2e0);a.sign8(0,3,true);
    if(!a.equal()){
        a.load(3,28,0);a.move(4,16);a.move(5,25);a.value(6,0);a.value(7,0xffffffff);a.value(8,0);a.call(0x10084be0);a.compare_registers(20,3);
        if(!a.at_least()){a.move(3,20);a.jump(0x103653a8);}else{a.at(0x1036538c);a.load(3,28,0);a.move(4,16);a.move(5,25);a.value(6,0);a.value(7,0xffffffff);a.value(8,0);a.call(0x10084be0);}
        a.at(0x103653a8);a.move(20,3);a.jump(0x103653c4);
    }else{a.at(0x103653b0);a.load(0,16,8);a.compare_registers(20,0);if(!a.at_least())a.move(0,20);a.at(0x103653c0);a.move(20,0);}
}
bool finish_requested_row(Call& a) {
    a.at(0x103653c4);a.load<2>(0,17,0x1ec,true);a.value(4,0x43300000);a.value(3,0x66660000);a.subtract(5,20,18);a.product(0,0,0);a.store(1,0x50,4);a.offset(3,3,0x6667);a.floating_load(2,29,8);a.exclusive_flags(5,5,0x80000000);a.store(1,0x58,4);a.multiply_high(0,3,0);a.store(1,0x54,5);a.floating_load(3,29,0x80);a.floating_load(0,1,0x50);a.floating_load(4,29,0x148);a.floating_subtract(1,0,2);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.exclusive_flags(0,0,0x80000000);a.store(1,0x5c,0);a.floating_load(0,1,0x58);a.floating_subtract(0,0,2);a.floating_difference(0,1,0,true);a.floating_subtract(0,3,0);detail::clamp_interest_ratio(a,0x10365428,29);
    a.load(3,1,0x38);a.value(0,0x43300000);a.store(1,0x60,0);a.move(4,16);a.exclusive_flags(0,3,0x80000000);a.floating_load(1,29,8);a.store(1,0x64,0);a.load(3,26,0);a.floating_load(0,1,0x60);a.floating_subtract(0,0,1);a.floating_product(0,0,4);a.floating_integer(0,0);a.floating_store(0,1,0x68);a.load(0,1,0x6c);a.store(1,0x38,0);a.call(0x1018c340);a.compare(3,0,false);
    if(!a.equal()){a.load<2>(3,3,0x34,true);a.compare(3,15);if(!a.nonpositive()){a.offset(3,3,-15);a.load(0,1,0x38);a.product(3,3,3);a.subtract(0,3,0);a.store(1,0x38,0);}}
    a.at(0x103654cc);a.load(3,1,0x38);a.load(0,1,0x3c);a.compare_registers(3,0);if(a.less())return false;
    a.load<1>(0,22,0x28);a.move(3,15);a.move(4,16);a.move(5,17);a.mask(0,0,15,false,29);a.value(6,1);a.sign8(8,0);a.value(7,0);a.value(9,0);a.call(0x10369080);a.floating_load(0,29,0x2c0);a.floating_compare(1,0);a.compare_greater_or_equal_bit();if(!a.equal())return false;
    a.load<1>(0,1,0xeb);a.sign8(0,0,true);
    if(!a.equal()){
        a.load(0,16,0);a.compare(21,0,false);a.store(22,0x10,0);
        if(!a.equal()){a.move(3,15);a.move(4,21);a.move(5,17);a.value(6,3);a.value(7,0);a.call(0x10365590);}
        a.at(0x10365548);a.move(3,15);a.move(4,16);a.move(5,17);a.call(0x103632f0);
    }
    a.at(0x10365558);a.value(3,1);a.jump(0x10365574);return true;
}
}
void consider_requested_club_interest(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,consider_requested_club_interest_entry,d,o);a.link();a.multiple(13);a.load(28,2,-0x7b7c);a.move(15,3);a.load(26,2,-0x7ba0);a.move(16,4);a.load(27,2,-0x7b80);a.move(17,5);a.load(14,2,-0x7ab8);a.move(18,6);a.load(29,2,-0x4ddc);a.load(13,2,-0x7b90);a.load(30,2,-0x7b84);a.save_link();a.frame(0xc0);a.store<1>(1,0xeb,7);
    if(prepare_requested_interest(a)){
        for(;;){
            if(requested_row_available(a)&&requested_category_policy(a)&&requested_previous_contribution(a)){requested_candidate_contribution(a);if(finish_requested_row(a))break;}
            a.at(0x10365560);a.offset(23,23,1);a.offset(22,22,44);a.compare(23,40);if(!a.less()){a.value(3,0);break;}
        }
    }
    a.at(0x10365574);a.pop(0xc0);a.multiple(13,true);a.finish();
}
}
