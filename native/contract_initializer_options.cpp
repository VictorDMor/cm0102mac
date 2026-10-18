#include "contract_initializer_detail.hpp"

namespace cm::aot::contract_storage::detail {
namespace {
bool supports_release_option(Call& a,std::uint32_t start,int method) {
    a.at(start);a.compare(28,0,false);if(a.equal())return false;
    a.load(3,28,0x30);a.compare(3,0,false);if(a.equal())return false;
    a.load(0,3,0);a.load(3,13,0);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.compare(3,0,false);if(a.equal())return false;
    a.load<1>(0,3,0x43);a.compare(0,2);if(!a.equal())return false;
    a.load(12,3,0);a.load(12,12,method);a.call(0x10781320);a.load(2,1,0x14);a.sign8(0,3,true);return !a.equal();
}
bool first_release_eligible(Call& a) {
    a.at(0x10372f00);a.load<1>(0,30,0x62);a.compare(0,11);if(!a.equal())return false;
    a.sign16(0,23);a.compare(0,0xabe);if(a.nonpositive())return false;
    advance_random_pool(a,0x10372f18);
    a.load(4,26,0);a.value(6,0x51ec0000);a.value(3,0x66660000);a.load<1>(8,30,0x65);a.load<2>(5,4,0);a.sign8(4,27);a.load(0,25,0);a.sign16(7,23);a.offset(6,6,-0x7ae1);a.offset(3,3,0x6667);a.multiply_high(6,6,7);a.add(7,5,0);a.sign8(0,8);a.load<1>(5,30,0x63);a.sign8(8,5);a.shift_right(6,6,4);a.multiply_high(5,3,7);a.mask(3,8,0xfffffffe,false,1);a.mask(8,6,1,false,1);a.add(8,6,8);a.shift_right(5,5,3);a.mask(6,5,1,false,1);a.add(5,5,6);a.multiply(5,5,20);a.subtract(5,5,7);a.multiply(6,0,3);a.add(0,8,5);a.subtract(5,6,0);a.multiply(0,4,12);a.add(3,3,5);a.compare_registers(3,0);if(a.nonpositive())return false;
    return supports_release_option(a,0x10372fc0,0xa0);
}
void first_release_option(Call& a) {
    if(first_release_eligible(a)) {
        a.shift_right(0,24,2);a.add_carry(14,0,true);
        if(a.equal()){a.value(15,0);a.jump(0x1037316c);}else {advance_random_pool(a,0x10373024);sample_contract_range<14,15,0x84>(a,0x1037304c);}
        a.at(0x1037316c);a.value(3,0x40000);a.add(14,24,15);a.offset(0,3,-0x2f70);a.compare_registers(14,0);if(!a.at_least())a.move(14,0);
        a.at(0x10373184);a.move(3,31);a.value(4,0);a.call(0x10374af0);a.move(3,31);a.move(4,14);a.call(0x10380700);a.jump(0x103731ac);
    }else {a.at(0x103731a0);a.move(3,31);a.value(4,0);a.call(0x10374b60);}
}
bool second_release_eligible(Call& a) {
    a.at(0x103731ac);a.load<1>(0,30,0x62);a.compare(0,11);if(!a.equal())return false;
    a.sign16(0,23);a.compare(0,0x1964);if(a.nonpositive())return false;
    advance_random_pool(a,0x103731c4);
    a.load(3,26,0);a.value(6,0x66660000);a.value(5,0x51ec0000);a.load<1>(0,30,0x65);a.load<2>(10,3,0);a.sign8(4,27);a.load(9,25,0);a.sign8(7,0);a.offset(8,6,0x6667);a.sign16(6,23);a.add(0,10,9);a.offset(5,5,-0x7ae1);a.multiply_high(8,8,0);a.mask(7,7,0xfffffffc,false,2);a.shift_right(8,8,3);a.multiply_high(5,5,6);a.mask(6,8,1,false,1);a.add(6,8,6);a.multiply(8,6,20);a.shift_right(5,5,4);a.mask(6,5,1,false,1);a.add(5,5,6);a.subtract(6,8,0);a.subtract(5,7,5);a.multiply(0,4,14);a.subtract(4,6,5);a.compare_registers(4,0);return !a.nonpositive();
}
void second_release_option(Call& a) {
    if(second_release_eligible(a)) {
        a.shift_right(0,24,2);a.add_carry(14,0,true);
        if(a.equal()){a.value(15,0);a.jump(0x103733b4);}else {
            a.at(0x10373270);a.load(0,19,0);a.offset(3,3,2);a.store(26,0,3);a.compare_registers(3,0,false);
            if(!a.nonpositive()){a.store(26,0,20);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(25,0,0);}
            sample_contract_range<14,15,0x84>(a,0x10373294);
        }
        a.at(0x103733b4);a.add(14,24,15);a.value(3,0x43300000);a.exclusive_flags(0,14,0x80000000);a.store(1,0x84,3);a.mask(4,14,0xfffffffe,false,1);a.floating_load(3,29,0xc0);a.store(1,0x88,0);a.exclusive_flags(0,4,0x80000000);a.floating_load(4,29,0x10);a.floating_load(0,1,0x84);a.store(1,0x90,0);a.floating_subtract(2,0,3);a.floating_load(1,29,8);a.store(1,0x8c,3);a.floating_load(0,1,0x8c);a.floating_product_sum(1,4,2,1);a.floating_subtract(0,0,3);a.floating_compare(1,0);
        if(!a.nonpositive())a.jump(0x10373414);else {a.at(0x10373404);a.store(1,0x98,0);a.store(1,0x94,3);a.floating_load(0,1,0x94);a.floating_subtract(1,0,3);}
        a.at(0x10373414);a.floating_integer(0,1);a.value(3,0xb70000);a.offset(0,3,0x1b00);a.floating_store(0,1,0x7c);a.load(4,1,0x80);a.compare_registers(4,0);
        if(!a.nonpositive()){a.exclusive_flags(3,4,0x80000000);a.value(0,0x43300000);a.store(1,0x78,3);a.floating_load(2,29,0xc0);a.store(1,0x74,0);a.floating_load(3,29,0x20);a.floating_load(1,1,0x74);a.floating_load(0,29,0x18);a.floating_subtract(1,1,2);a.floating_product_sum(0,3,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x6c);a.load(4,1,0x70);}
        a.at(0x10373464);a.value(3,0x40000);a.add(14,14,4);a.offset(0,3,-0x2f70);a.compare_registers(14,0);if(!a.at_least())a.move(14,0);
        a.at(0x1037347c);a.move(3,31);a.value(4,2);a.call(0x10374af0);a.move(3,31);a.move(4,14);a.call(0x10380700);a.jump(0x103734a4);
    }else {a.at(0x10373498);a.move(3,31);a.value(4,2);a.call(0x10374b60);}
}
bool third_release_eligible(Call& a) {
    a.at(0x103734a4);a.load<1>(0,30,0x62);a.compare(0,11);if(!a.equal())return false;
    a.sign16(0,23);a.compare(0,0x6d6);if(a.nonpositive())return false;
    advance_random_pool(a,0x103734bc);
    a.load(3,26,0);a.value(6,0x66660000);a.value(5,0x51ec0000);a.load<1>(0,30,0x63);a.load<2>(10,3,0);a.sign8(3,27);a.load(9,25,0);a.sign8(4,0);a.offset(8,6,0x6667);a.load<1>(7,30,0x65);a.add(0,10,9);a.sign16(6,23);a.multiply_high(8,8,0);a.offset(5,5,-0x7ae1);a.sign8(7,7);a.mask(7,7,0xfffffffe,false,1);a.shift_right(8,8,3);a.multiply_high(5,5,6);a.mask(6,8,1,false,1);a.add(6,8,6);a.shift_right(5,5,4);a.multiply(8,6,20);a.mask(6,5,1,false,1);a.add(5,5,6);a.multiply(4,4,3);a.subtract(6,8,0);a.subtract(0,7,5);a.add(4,0,4);a.multiply(0,3,10);a.subtract(3,6,4);a.compare_registers(3,0);if(a.nonpositive())return false;
    return supports_release_option(a,0x10373564,0xa4);
}
void scale_third_release(Call& a) {
    a.at(0x1037372c);a.load<1>(3,30,0x65);a.sign8(0,3);a.compare(0,5);bool half=false;
    if(!a.at_least()){a.load<1>(0,30,0x63);a.sign8(0,0);a.compare(0,10);half=!a.nonpositive();}
    if(half){a.exclusive_flags(3,13,0x80000000);a.value(0,0x43300000);a.store(1,0x70,3);a.floating_load(1,29,0xc0);a.store(1,0x6c,0);a.floating_load(2,29,0x28);a.floating_load(0,1,0x6c);a.floating_subtract(0,0,1);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0x74);a.load(13,1,0x78);a.jump(0x10373800);return;}
    a.at(0x10373780);a.load<1>(0,30,0x63);a.sign8(0,0);a.compare(0,10);
    if(!a.nonpositive()){a.exclusive_flags(3,13,0x80000000);a.value(0,0x43300000);a.store(1,0x70,3);a.floating_load(1,29,0xc0);a.store(1,0x6c,0);a.floating_load(2,29,0x30);a.floating_load(0,1,0x6c);a.floating_subtract(0,0,1);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0x74);a.load(13,1,0x78);a.jump(0x10373800);return;}
    a.at(0x103737c4);a.sign8(0,3);a.compare(0,15);
    if(!a.at_least()){a.exclusive_flags(3,13,0x80000000);a.value(0,0x43300000);a.store(1,0x70,3);a.floating_load(1,29,0xc0);a.store(1,0x6c,0);a.floating_load(2,29,0x38);a.floating_load(0,1,0x6c);a.floating_subtract(0,0,1);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0x74);a.load(13,1,0x78);}
}
void third_release_option(Call& a) {
    if(third_release_eligible(a)) {
        a.mask(0,24,1,false,1);a.add(0,0,24);a.shift_right(13,0,1,true);
        if(a.equal()){a.value(14,0);a.jump(0x10373714);}else {advance_random_pool(a,0x103735cc);sample_contract_range<13,14,0x6c>(a,0x103735f4);}
        a.at(0x10373714);a.value(3,0x10000);a.subtract(13,14,24);a.offset(0,3,-0x3cb0);a.compare_registers(13,0);if(!a.at_least())a.move(13,0);
        scale_third_release(a);a.at(0x10373800);a.move(3,31);a.value(4,3);a.call(0x10374af0);a.move(3,31);a.move(4,13);a.call(0x10380700);a.jump(0x10373828);
    }else {a.at(0x1037381c);a.move(3,31);a.value(4,3);a.call(0x10374b60);}
}
template<unsigned Index,bool Alternate=false>
void profile_bonus(Call& a,std::uint32_t start) {
    a.at(start);a.value(3,0x66660000);a.offset(0,3,0x6667);a.multiply_high(0,0,22);a.shift_right(0,0,4);a.mask(3,0,1,false,1);a.add_record(13,0,3);
    if(a.equal()){a.value(14,0);a.jump(start+0x16c);}else {advance_random_pool(a,start+0x24);sample_contract_range<13,14,0x6c,Alternate>(a,start+0x4c);}
    a.at(start+0x16c);a.value(4,0x66660000);a.move(3,31);a.offset(0,4,0x6667);a.value(4,Index);a.multiply_high(0,0,22);a.shift_right(0,0,3);a.mask(5,0,1,false,1);a.add(0,0,5);a.subtract(5,14,0);a.offset(5,5,25);a.call(0x10374b40);a.jump(start+0x1a8);
}
}
void select_contract_options(Call& a) {
    a.at(0x10372e08);a.move(3,31);a.value(4,0);a.call(0x10380700);a.move(3,31);a.value(4,4);a.call(0x10374b60);a.move(3,31);a.value(4,1);a.call(0x10374b60);a.load(0,30,0x14);a.compare(0,0,false);
    if(!a.equal()) {
        a.move(3,31);a.value(4,4);a.call(0x10374b60);a.load<2>(0,30,0x5c,true);a.sign8(0,0);a.compare(0,32);bool age_option=!a.nonpositive();
        if(age_option){a.sign16(3,23);a.compare(3,0xcb2);age_option=!a.nonpositive();}
        if(age_option){a.value(4,0x55550000);a.load(3,30,0);a.offset(4,4,0x5556);a.multiply_high(5,4,3);a.mask(4,5,1,false,1);a.add(4,5,4);a.multiply(4,4,3);a.subtract(4,4,3,true);age_option=a.equal();}
        if(age_option){a.value(4,0x66660000);a.load<1>(6,30,0x64);a.offset(5,4,0x6667);a.load<1>(4,30,0x67);a.multiply_high(7,5,3);a.load<1>(5,30,0x63);a.sign8(6,6);a.sign8(4,4);a.sign8(5,5);a.add(4,4,6);a.shift_right(6,7,2);a.mask(7,6,1,false,1);a.add(6,6,7);a.multiply(6,6,10);a.subtract(3,6,3);a.add(3,4,3);a.add(3,3,0);a.offset(0,3,-28);a.add(0,5,0);a.compare(0,45);age_option=!a.nonpositive();}
        if(age_option){a.move(3,31);a.value(4,4);a.call(0x10374af0);a.jump(0x10372f00);}
        else {a.at(0x10372ee8);a.load(0,30,0x1c);a.compare(0,0,false);if(!a.equal()){a.move(3,31);a.value(4,4);a.call(0x10374af0);}}
        first_release_option(a);second_release_option(a);third_release_option(a);
        a.at(0x10373828);a.load<1>(0,21,0x15);a.sign8(0,0);a.compare(0,15);
        if(!a.less())profile_bonus<0>(a,0x10373838);else {a.at(0x103739d4);a.move(3,31);a.value(4,0);a.call(0x10374b80);}
        a.at(0x103739e0);a.load<1>(0,21,0x13);a.sign8(0,0);a.compare(0,15);bool second=a.at_least();
        if(!second){a.load<1>(0,21,0x14);a.sign8(0,0);a.compare(0,15);second=a.at_least();}
        if(!second){a.load<1>(0,21,0x15);a.sign8(0,0);a.compare(0,15);second=!a.less();}
        if(second)profile_bonus<1>(a,0x10373a10);else {a.at(0x10373bac);a.move(3,31);a.value(4,1);a.call(0x10374b80);}
        a.at(0x10373bb8);a.load<1>(0,21,15);a.compare(0,20);
        if(a.equal())profile_bonus<2,true>(a,0x10373bc4);else {a.at(0x10373d60);a.move(3,31);a.value(4,2);a.call(0x10374b80);}
    }
    a.at(0x10373d6c);a.load(0,30,0x14);a.compare(0,0,false);
    if(a.equal()) {
        for(unsigned n=0;n<5;++n){a.move(3,31);a.value(4,n);a.call(0x10374b60);}
        for(unsigned n=0;n<3;++n){a.move(3,31);a.value(4,n);a.call(0x10374b80);}
        a.move(3,31);a.value(4,0);a.call(0x10380700);
    }
}
}
