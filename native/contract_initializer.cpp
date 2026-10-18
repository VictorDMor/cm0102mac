#include "contract_storage.hpp"
#include "contract_initializer_detail.hpp"

namespace cm::aot::contract_storage {
namespace {
using detail::Call;
bool prepare_identity_and_rating(Call& a) {
    a.offset(3,1,0x64);a.call(0x10715880);a.offset(3,1,0x5c);a.value(4,10);a.value(5,6);a.value(6,2000);a.value(7,0xffffffff);a.call(0x107156f0);
    a.load<1>(3,31,0x38);a.value(6,0);a.insert_mask(3,6,1,0);a.value(0,0xffffffff);a.store<1>(31,0x38,3);a.value(5,2);a.move(3,31);a.move(4,30);a.store<1>(31,0x54,6);a.store(31,0x58,0);a.load<1>(0,31,0x60);a.insert_mask(0,5,15,0);a.store<1>(31,0x60,0);a.store<2>(31,0x42,6);a.store(31,8,6);a.store<1>(31,0x40,6);a.store(31,8,6);a.load(5,30,0x24);a.call(0x1037f050);
    a.load(28,30,0x24);a.compare(28,0,false);
    if(!a.equal()){a.move(3,28);a.offset(4,1,0x38);a.value(5,1);a.call(0x100ce320);a.compare(3,0,false);if(!a.equal()){a.load<1>(0,1,0x38);a.sign8(0,0,true);if(a.equal())a.move(28,3);}}
    a.at(0x103728a8);a.compare(24,0);
    if(!a.at_least()){a.load(3,2,-0x7b7c);a.move(4,30);a.move(5,28);a.value(17,0);a.load(3,3,0);a.value(6,0);a.value(7,0xffffffff);a.value(8,0);a.call(0x10084be0);a.move(24,3);}
    a.at(0x103728d8);a.compare(28,0,false);
    if(a.equal()){a.load(0,30,0x20);a.compare(0,0,false);if(a.equal()){a.load<1>(0,31,0x38);a.value(3,3);a.insert_mask(0,3,0xfc,2);a.store<1>(31,0x38,0);a.jump(0x10374558);return false;}}
    a.at(0x10372900);a.compare(28,0,false);a.load(21,30,0x14);
    if(!a.equal()) {
        a.value(3,0x10620000);a.load<2>(0,28,0x1ec,true);a.offset(3,3,0x4dd3);a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,1,false,1);a.add(3,0,3);a.offset(0,3,1);a.sign8(27,0);
        if(!a.equal()){a.load<1>(0,28,0x23d);a.compare(0,2);if(a.equal()){a.offset(27,27,-1);a.sign8(0,27);a.compare(0,1);if(!a.at_least())a.value(27,1);a.at(0x10372954);a.sign8(0,27);a.compare(0,4);if(!a.nonpositive()){a.value(27,4);a.jump(0x103729bc);}}}
    } else {
        a.at(0x10372968);a.load(0,30,0x1c);a.compare(0,0,false);
        if(a.equal()){a.load<1>(0,31,0x38);a.value(3,3);a.insert_mask(0,3,0xfc,2);a.store<1>(31,0x38,0);a.jump(0x10374558);return false;}
        a.at(0x10372988);a.load(4,30,0x20);a.value(3,0x10620000);a.offset(3,3,0x4dd3);a.load<2>(0,4,0x1ec,true);a.multiply_high(0,3,0);a.shift_right(0,0,5);a.mask(3,0,1,false,1);a.add(3,0,3);a.offset(0,3,1);a.sign8(27,0);a.compare(27,3);if(!a.at_least())a.value(27,3);
    }
    a.at(0x103729bc);a.compare(28,0,false);
    if(!a.equal()){a.load(0,28,0);a.store(31,4,0);a.jump(0x103729f0);}
    else {a.at(0x103729d0);a.load(3,30,0x20);a.compare(3,0,false);if(!a.equal()){a.load(0,3,0);a.store(31,4,0);a.jump(0x103729f0);}else {a.at(0x103729e8);a.value(0,0xffffffff);a.store(31,4,0);}}
    a.at(0x103729f0);a.load<1>(4,30,0x5e);a.compare(4,0,false);
    if(!a.equal()) {
        a.load(3,30,12);a.compare(3,0,false);
        if(!a.equal()) {a.load<1>(3,3,0x11f);a.value(0,0x43300000);a.store(1,0x94,0);a.sign8(3,3);a.floating_load(4,29,0xc0);a.exclusive_flags(3,3,0x80000000);a.store(1,0x90,4);a.floating_load(2,29,0xc8);a.store(1,0x98,3);a.floating_load(3,16,0,true);a.store(1,0x8c,0);a.floating_load(0,1,0x94);a.floating_load(1,1,0x8c);a.floating_subtract(4,0,4,true);a.floating_load(0,16,4,true);a.floating_subtract(1,1,2,true);a.floating_product_single(2,4,3);a.floating_product_single(0,1,0);a.floating_product_single(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0x84);a.load(3,1,0x88);a.jump(0x10372a68);}
        else {a.at(0x10372a64);a.value(3,1);}
        a.at(0x10372a68);a.sign8(0,3);a.compare(0,4);
        if(!a.nonpositive()){a.value(3,4);a.jump(0x10372a88);}else {a.at(0x10372a7c);a.sign8(0,3,true);if(a.equal())a.value(3,1);}
        a.at(0x10372a88);a.add(27,27,3);
    }
    a.at(0x10372a8c);a.load(3,30,0x14);a.compare(3,0,false);
    if(!a.equal()){a.load<2>(23,3,10,true);a.jump(0x10372ab0);}else {a.at(0x10372aa0);a.load(3,30,0x1c);a.compare(3,0,false);if(!a.equal())a.load<2>(23,3,0x2a,true);}
    return true;
}
void select_existing_start_year(Call& a) {
    if(!a.nonpositive()){a.load<2>(0,1,0x66);a.compare(0,1,false);if(!a.nonpositive()){a.load<2>(0,14,2);a.store<2>(31,0x2a,0);a.jump(0x10372c00);return;}}
    a.at(0x10372b84);a.floating_load(0,16,12,true);a.floating_compare(31,0);
    if(!a.nonpositive()){a.load(3,30,0x14);a.compare(3,0,false);if(!a.equal()){a.load<1>(0,3,0x11);a.compare(0,20);if(a.equal()){a.load<2>(0,14,2);a.store<2>(31,0x2a,0);a.jump(0x10372c00);return;}}}
    a.at(0x10372bb4);a.load<2>(0,30,0x5c,true);a.sign8(0,0);a.compare(0,24);bool prime=false;
    if(!a.less()){a.compare(0,29);prime=a.nonpositive();}
    if(!prime){a.at(0x10372bcc);a.load<2>(0,1,0x66);a.compare(0,2,false);if(!a.nonpositive()){a.load<2>(3,18,2);a.offset(0,3,-2);a.store<2>(31,0x2a,0);a.jump(0x10372c00);return;}}
    a.at(0x10372be8);a.load<2>(0,1,0x66);a.compare(0,3,false);
    if(!a.nonpositive()){a.load<2>(3,18,2);a.offset(0,3,-3);a.store<2>(31,0x2a,0);}
}
void select_dates_and_base_value(Call& a) {
    a.at(0x10372ab0);a.sign8(0,17,true);bool existing=!a.equal();
    if(existing){a.load(3,30,4);a.compare(3,0);existing=!a.equal();if(existing){a.sign16(0,23);a.multiply(0,0,15);a.compare_registers(3,0);existing=!a.positive();}}
    if(existing) {
        a.at(0x10372ad4);a.exclusive_flags(0,3,0x80000000);a.value(4,0x43300000);a.store(1,0x88,0);a.value(3,2);a.load<1>(0,31,0x38);a.insert_mask(0,3,0xfc,2);a.store(1,0x84,4);a.move(3,18);a.floating_load(1,29,0xc0);a.floating_load(0,1,0x84);a.store<1>(31,0x38,0);a.floating_subtract(31,0,1,true);a.load(4,30,0x4c);a.load(5,30,0x50);a.floating_integer(0,31);a.floating_store(0,1,0x8c);a.load(22,1,0x90);a.call(0x10718130);a.load(5,30,0x4c);a.move(4,18);a.load(6,30,0x50);a.offset(3,1,0x54);a.call(0x10717dd0);
        a.load<1>(0,1,0x58);a.load<2>(4,1,0x56);a.load<2>(3,1,0x54,true);a.store<2>(1,0x66,4);a.floating_load(0,16,8,true);a.store<2>(1,0x64,3);a.floating_compare(31,0);a.store<1>(1,0x68,0);a.load<2>(0,30,0x4e);a.store<2>(31,0x2a,0);a.load<2>(0,30,0x4c,true);a.store<2>(31,0x28,0);a.load<1>(0,30,0x50);a.store<1>(31,0x2c,0);
        select_existing_start_year(a);
        a.at(0x10372c00);a.load<2>(0,30,0x56);a.store<2>(31,0x32,0);a.load<2>(0,30,0x54,true);a.store<2>(31,0x30,0);a.load<1>(0,30,0x58);a.store<1>(31,0x34,0);a.jump(0x10372e08);return;
    }
    a.at(0x10372c1c);a.move(3,31);a.move(4,30);a.move(5,28);a.value(6,0);a.value(7,0xffffffff);a.value(8,11);a.value(9,0);a.call(0x1037b6f0);
    a.load(0,14,0);a.floating_integer(0,1);a.offset(15,18,2);a.load(3,14,4);a.store(1,0x44,0);a.load<2>(0,18,2);a.load<2>(4,1,0x46);a.floating_store(0,1,0x84);a.compare_registers(4,0,false);a.floating_load(31,16,16,true);a.store(1,0x48,3);a.load(22,1,0x88);
    if(!a.at_least()){a.value(3,1);a.jump(0x10372ca4);}else {a.at(0x10372c78);if(!a.nonpositive()){a.value(3,0);a.jump(0x10372ca4);}else {a.at(0x10372c84);a.load<2>(3,1,0x44,true);a.load<2>(5,18,0,true);a.combine(3,5,3,false,true);a.shift_right(4,3,1);a.intersect(3,3,5);a.subtract(3,3,4);a.mask(3,3,1,false,1);a.sign8(3,3);}}
    a.at(0x10372ca4);a.sign8(3,3,true);bool fallback=!a.equal();
    if(!fallback){a.compare(28,0,false);if(a.equal()){a.load(3,30,0x20);a.compare(3,0,false);fallback=a.equal();}}
    if(!fallback) {
        a.at(0x10372cc0);a.load(4,30,0x4c);a.load(3,30,0x50);a.store(1,0x3c,4);a.load<2>(4,1,0x3e);a.store(1,0x40,3);a.compare_registers(4,0,false);
        if(!a.at_least()){a.value(0,1);a.jump(0x10372d10);}else {a.at(0x10372ce4);if(!a.nonpositive()){a.value(0,0);a.jump(0x10372d10);}else {a.at(0x10372cf0);a.load<2>(0,1,0x3c,true);a.load<2>(4,18,0,true);a.combine(0,4,0,false,true);a.shift_right(3,0,1);a.intersect(0,0,4);a.subtract(0,0,3);a.mask(0,0,1,false,1);a.sign8(0,0);}}
        a.at(0x10372d10);a.sign8(0,0,true);
        if(!a.equal()) {
            a.load<2>(0,30,0x4e);a.move(4,18);a.offset(3,1,0x4c);a.store<2>(31,0x2a,0);a.load<2>(0,30,0x4c,true);a.store<2>(31,0x28,0);a.load<1>(0,30,0x50);a.store<1>(31,0x2c,0);a.load(5,31,0x28);a.load(6,31,0x2c);a.call(0x10717dd0);
            a.load<2>(0,1,0x4e);a.load<1>(3,1,0x50);a.store<2>(1,0x66,0);a.load<2>(4,1,0x4c,true);a.load<2>(0,1,0x66);a.store<2>(1,0x64,4);a.compare(0,2,false);a.store<1>(1,0x68,3);
            if(!a.nonpositive()){a.load<2>(3,15,0);a.offset(0,3,-1);a.store<2>(31,0x2a,0);a.jump(0x10372d90);}
        } else {a.at(0x10372d78);a.load<2>(0,1,0x5e);a.store<2>(31,0x2a,0);a.load<2>(0,1,0x5c,true);a.store<2>(31,0x28,0);a.load<1>(0,1,0x60);a.store<1>(31,0x2c,0);}
        a.at(0x10372d90);a.load<1>(0,31,0x38);a.value(3,2);a.insert_mask(0,3,0xfc,2);a.store<1>(31,0x38,0);a.load<2>(0,30,0x56);a.store<2>(31,0x32,0);a.load<2>(0,30,0x54,true);a.store<2>(31,0x30,0);a.load<1>(0,30,0x58);a.store<1>(31,0x34,0);a.jump(0x10372e00);
    } else {
        a.at(0x10372dbc);a.load<1>(3,31,0x38);a.value(0,2);a.insert_mask(3,0,0xfc,2);a.store<1>(31,0x38,3);a.value(0,0);a.load<2>(3,15,0);a.store<2>(31,0x2a,3);a.load<2>(3,18,0,true);a.store<2>(31,0x28,3);a.load<1>(3,18,4);a.store<1>(31,0x2c,3);a.load<2>(3,15,0);a.offset(3,3,1);a.store<2>(31,0x32,3);a.load<2>(3,18,0,true);a.store<2>(31,0x30,3);a.store(31,0x3c,0);
    }
    a.at(0x10372e00);a.value(0,0);a.store(31,8,0);
}
void finish_contract(Call& a) {
    a.at(0x103744a8);a.compare(28,0,false);
    if(!a.equal()) {
        a.load<1>(0,28,0x23d);a.sign8(0,0);a.compare(0,2);
        if(a.equal()){a.load<1>(0,31,0x38);a.value(3,1);a.insert_mask(0,3,1,0);a.store<1>(31,0x38,0);a.jump(0x10374548);}
        else {a.at(0x103744d4);a.compare(0,3);if(a.equal()) {
            a.move(3,31);a.value(4,0);a.call(0x10374b60);a.move(3,31);a.value(4,1);a.call(0x10374b60);a.move(3,31);a.value(4,2);a.call(0x10374b60);a.move(3,31);a.value(4,3);a.call(0x10374b60);a.move(3,31);a.value(4,4);a.call(0x10374b60);a.move(3,31);a.value(4,0);a.call(0x10374b80);a.move(3,31);a.value(4,1);a.call(0x10374b80);a.move(3,31);a.value(4,2);a.call(0x10374b80);a.move(3,31);a.value(4,0);a.call(0x10380700);
        }}
    }
    a.at(0x10374548);a.floating_integer(0,31);a.floating_store(0,1,0x6c);a.load(0,1,0x70);a.store(31,12,0);
}
}
void initialize_contract(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    Call a(s,initializer_entry,d,o);a.link();a.floating_store(31,1,-8);a.multiple(13,false,-16);a.move(31,3);a.load(29,2,-0x4dac);a.move(30,4);a.load(13,2,-0x7b5c);a.move(24,5);a.load(25,2,-0x7b90);a.value(17,1);a.load(20,2,-0x7b8c);a.load(19,2,-0x7b88);a.load(26,2,-0x7b84);a.load(14,2,-0x7a9c);a.load(16,2,-0x4da8);a.load(18,2,-0x7b80);a.save_link();a.frame(256);
    if(prepare_identity_and_rating(a)){select_dates_and_base_value(a);detail::select_contract_options(a);detail::apply_nation_value_rules(a);finish_contract(a);}
    a.at(0x10374558);a.load(0,1,0x108);a.offset(1,1,256);a.floating_load(31,1,-8);a.restore_link();a.multiple(13,true,-16);a.finish();
}
}
