#include "contract_initializer_detail.hpp"

namespace cm::aot::contract_storage::detail {
namespace {
bool matches_nation(Call& a,int slot) {
    a.load(4,2,slot);a.load(4,4,0);a.compare_registers(0,4);return a.equal();
}
template<unsigned Factor>
void apply_rating_floor(Call& a,std::uint32_t start) {
    a.at(start);a.sign8(3,27);a.value(0,0x43300000);a.product(3,3,3);a.store(1,0x6c,0);a.floating_load(1,29,0xc0);
    if constexpr(Factor==4)a.mask(3,3,0xfffffffc,false,2);else a.multiply(3,3,Factor);
    a.exclusive_flags(3,3,0x80000000);a.store(1,0x70,3);a.floating_load(0,1,0x6c);a.floating_subtract(0,0,1,true);a.floating_compare(31,0);
    if(!a.at_least()){a.store(1,0x78,3);a.store(1,0x74,0);a.floating_load(0,1,0x74);a.floating_subtract(31,0,1,true);}
    a.at(start+0x40);
}
void apply_scale_curve(Call& a,std::uint32_t start,int cap,int scale) {
    a.at(start);a.floating_load(2,29,cap);a.floating_compare(31,2);
    if(!a.nonpositive()){a.floating_subtract(0,31,2);a.floating_load(1,29,scale);a.floating_product_sum(31,1,0,2);a.floating_round_single(31,31);a.jump(0x103744a8);}
}
void no_club_floor(Call& a) {
    a.at(0x10374468);a.sign8(3,27);a.value(0,0x43300000);a.product(3,3,3);a.store(1,0x6c,0);a.floating_load(1,29,0xc0);a.multiply(3,3,10);a.exclusive_flags(3,3,0x80000000);a.store(1,0x70,3);a.floating_load(0,1,0x6c);a.floating_subtract(0,0,1,true);a.floating_compare(31,0);
    if(!a.at_least()){a.store(1,0x70,3);a.store(1,0x6c,0);a.floating_load(0,1,0x6c);a.floating_subtract(31,0,1,true);}
}
void apply_default_nation_curve(Call& a) {
    // This path retains r3 (the live nation pointer), r0 and r6. Its
    // apparently contradictory later equality remains an original branch.
    a.at(0x10374304);a.sign8(5,27);a.value(4,0x43300000);a.product(5,5,5);a.store(1,0x6c,4);a.floating_load(1,29,0xc0);a.multiply(5,5,10);a.exclusive_flags(5,5,0x80000000);a.store(1,0x70,5);a.floating_load(0,1,0x6c);a.floating_subtract(0,0,1,true);a.floating_compare(31,0);
    if(!a.at_least()){a.store(1,0x78,5);a.store(1,0x74,4);a.floating_load(0,1,0x74);a.floating_subtract(31,0,1,true);}
    a.at(0x10374344);a.compare_registers(0,6);
    if(!a.equal()){apply_scale_curve(a,0x10374448,0xb8,0x48);return;}
    a.load(5,28,0x30);a.compare(5,0,false);
    if(!a.equal()){a.load(4,2,-0x6778);a.load(5,5,0);a.load(0,4,0);a.compare_registers(5,0);if(a.equal()){apply_scale_curve(a,0x1037436c,0x68,0x78);return;}}
    a.at(0x1037438c);a.load(0,30,12);a.compare_registers(0,3,false);
    if(!a.equal()){apply_scale_curve(a,0x10374428,0xb0,0x48);return;}
    a.load<2>(0,30,0x5c,true);a.sign8(0,0);a.compare(0,31);
    if(!a.nonpositive()){apply_scale_curve(a,0x103743a8,0x98,0xa0);return;}
    a.at(0x103743c8);a.compare(0,24);
    if(!a.at_least()){a.load(3,30,0x14);a.compare(3,0,false);if(!a.equal()){a.load<2>(0,3,12,true);a.compare(0,0xea6);if(!a.at_least()){apply_scale_curve(a,0x103743e8,0x98,0xa0);return;}}}
    apply_scale_curve(a,0x10374408,0xa8,0xa0);
}
}
void apply_nation_value_rules(Call& a) {
    a.at(0x10373de4);a.sign8(0,17,true);if(!a.equal())return;
    a.floating_load(0,16,16,true);a.floating_compare(0,31);if(a.equal())return;
    a.compare(28,0,false);if(a.equal()){no_club_floor(a);return;}
    a.load(3,28,0x2c);a.compare(3,0,false);if(a.equal()){no_club_floor(a);return;}
    a.load(4,2,-0x7a1c);a.load(0,3,0x68);a.load(4,4,0);a.compare_registers(0,4);
    if(a.equal()) {
        apply_rating_floor<5>(a,0x10373e20);a.load(0,28,8);a.compare(0,15000);
        if(!a.at_least()){apply_scale_curve(a,0x10373e6c,0x40,0x48);return;}
        a.at(0x10373e8c);a.compare(0,27500);
        if(!a.at_least()){apply_scale_curve(a,0x10373e94,0x50,0x48);return;}
        apply_scale_curve(a,0x10373eb4,0x58,0x48);return;
    }
    a.at(0x10373ed4);
    if(matches_nation(a,-0x7590)){apply_rating_floor<10>(a,0x10373ee4);apply_scale_curve(a,0x10373f24,0x58,0x48);return;}
    a.at(0x10373f44);
    if(matches_nation(a,-0x7af4) || matches_nation(a,-0x7a00) || matches_nation(a,-0x7a28)){apply_rating_floor<10>(a,0x10373f74);apply_scale_curve(a,0x10373fb4,0x60,0x48);return;}
    a.at(0x10373fd4);
    if(matches_nation(a,-0x7514) || matches_nation(a,-0x7650)) {
        apply_rating_floor<4>(a,0x10373ff4);a.load<1>(0,28,0x23d);a.compare(0,1);
        if(a.equal())apply_scale_curve(a,0x10374060,0x40,0x48);else apply_scale_curve(a,0x10374040,0x68,0x48);return;
    }
    a.at(0x10374080);
    if(matches_nation(a,-0x757c) || matches_nation(a,-0x7594) || matches_nation(a,-0x7574) || matches_nation(a,-0x7578)) {
        apply_rating_floor<5>(a,0x103740c0);a.load<1>(0,28,0x23d);a.compare(0,1);
        if(a.equal())apply_scale_curve(a,0x1037412c,0x80,0x78);else apply_scale_curve(a,0x1037410c,0x70,0x78);return;
    }
    a.at(0x1037414c);
    if(matches_nation(a,-0x750c) || matches_nation(a,-0x75a8) || matches_nation(a,-0x7504)) {
        apply_rating_floor<5>(a,0x1037417c);a.load<1>(0,28,0x23d);a.compare(0,1);
        if(a.equal())apply_scale_curve(a,0x103741e8,0x90,0x88);else apply_scale_curve(a,0x103741c8,0x68,0x88);return;
    }
    a.at(0x10374208);
    if(matches_nation(a,-0x7510) || matches_nation(a,-0x79fc)) {
        apply_rating_floor<5>(a,0x10374228);a.load<1>(0,28,0x23d);a.compare(0,1);
        if(a.equal())apply_scale_curve(a,0x10374294,0x40,0x78);else apply_scale_curve(a,0x10374274,0x68,0x78);return;
    }
    a.at(0x103742b4);
    if(matches_nation(a,-0x7a24) || matches_nation(a,-0x7a10) || matches_nation(a,-0x7a14) || matches_nation(a,-0x7a18))return;
    a.load(4,2,-0x7a20);a.load(6,4,0);a.compare_registers(0,6);if(a.equal())return;
    apply_default_nation_curve(a);
}
}
