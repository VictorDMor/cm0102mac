#include "transfer_synthesis_terms.hpp"

namespace cm::aot::transfer_synthesis::detail {
namespace {
void release_output(Call& a,std::uint32_t pc){
    a.at(pc);a.compare(20,0,false);if(!a.equal()){a.load(3,20,0);a.call(0x107735a0);a.value(0,0);a.store(20,0,0);}
}
void refuse(Call& a,std::uint32_t pc){a.at(pc);a.value(3,0);a.jump(0x10415e34);}

void choose_final_components(Call& a){
    a.at(0x10415080);a.load(3,16,0xc);a.load<2>(5,3,0xac,true);a.compare(5,5000);bool established=!a.nonpositive();if(established){a.load<1>(0,16,0x5e);a.compare(0,0,false);established=a.equal();}
    if(established){
        a.load(4,16,0x14);a.value(3,0x43300000);a.exclusive_flags(0,5,0x80000000);a.store(1,0x8c,3);a.load<2>(4,4,8,true);a.floating_load(2,25,0xc0);a.exclusive_flags(4,4,0x80000000);a.floating_load(0,25,0x88);a.store(1,0x90,4);a.floating_load(1,1,0x8c);a.store(1,0x98,0);a.floating_subtract(1,1,2);a.store(1,0x94,3);a.floating_difference(1,1,0,true);a.floating_load(0,1,0x94);a.floating_subtract(0,0,2);a.floating_compare(1,0);bool contingent=a.positive();if(!contingent){a.load(3,16,0x20);a.compare(3,0,false);if(!a.equal()){a.call(0x100ce250);a.sign8(0,3,true);contingent=!a.equal();}}
        if(contingent){
            a.at(0x104150fc);a.load(3,15,0x20);a.value(0,0x43300000);a.store(1,0x8c,0);a.value(0,10);a.exclusive_flags(3,3,0x80000000);a.floating_load(1,25,0xc0);a.store(1,0x90,3);a.floating_load(2,25,0x18);a.floating_load(0,1,0x8c);a.floating_subtract(0,0,1);a.floating_product(0,2,0);a.floating_integer(0,0);a.floating_store(0,1,0x94);a.load(3,1,0x98);a.store(15,0x48,3);a.store<1>(15,0x4c,0);
            advance_pool(a,0x1041513c);choose_ordinary_period(a,0x10415164,false,3);advance_pool(a,0x104151a0);calculate_ordinary_amount(a,0x104151c8,5,true);advance_pool(a,0x10415274);choose_percentage_pair(a,0x1041529c,false,false,false,0x10415760);
        }else{advance_pool(a,0x1041533c);choose_ordinary_period(a,0x10415364,false,1);advance_pool(a,0x104153a0);calculate_ordinary_amount(a,0x104153c8,10,true);advance_pool(a,0x10415474);choose_percentage_pair(a,0x1041549c,false,false,false,0x10415760);}
    }else{advance_pool(a,0x1041553c);choose_ordinary_period(a,0x10415564,true,3);advance_pool(a,0x1041559c);calculate_ordinary_amount(a,0x104155c4,5,true);advance_pool(a,0x10415670);choose_percentage_pair(a,0x10415698,true,false,false,0x10415760);}
    refuse(a,0x10415760);
}

void revise_toward_target(Call& a){
    a.at(0x10415770);a.compare_registers(0,19);bool revise=!a.at_least();if(revise){a.compare(18,0,false);revise=!a.equal();}if(revise){a.load(5,18,4);a.compare(5,0);revise=!a.equal();}if(revise){a.load<1>(0,1,0xbc);a.sign8(0,0,true);revise=a.equal();}if(revise){a.compare(18,0,false);revise=!a.equal();}if(revise){a.load<1>(0,18,0);a.compare(0,11);revise=a.equal();}
    if(revise){
        a.load(4,18,0xc);a.value(3,0x43300000);a.exclusive_flags(0,5,0x80000000);a.store(1,0x8c,3);a.exclusive_flags(4,4,0x80000000);a.floating_load(3,25,0xc0);a.store(1,0x90,4);a.floating_load(0,25,0xb0);a.floating_load(1,1,0x8c);a.store(1,0x98,0);a.floating_subtract(2,1,3,true);a.store(1,0x94,3);a.floating_load(1,1,0x94);a.floating_subtract(1,1,3,true);a.floating_difference(31,2,1,true,true);a.floating_compare(31,0);if(!a.at_least()){a.load(3,2,-0x4b90);a.floating_load(31,3,0,true);}
        a.at(0x104157f8);a.floating_load(0,25,0x18);a.floating_compare(31,0);if(!a.nonpositive()){a.load(3,2,-0x4b90);a.floating_load(31,3,4,true);}
        advance_pool(a,0x1041580c);a.load(3,31,0);a.load(0,28,0);a.load<2>(3,3,0);a.add(0,3,0);a.mask(3,0,1,false,1);a.mask(0,0,1);a.exclusive_registers_at(0x1041584c,0,0,3);a.subtract(0,3,0,true);
        if(!a.equal()){
            a.load(3,2,-0x4b90);a.floating_load(1,3,8,true);a.floating_difference(0,1,31,true,true);a.floating_difference(0,0,1,true,true);a.floating_integer(0,0);a.floating_store(0,1,0x8c);a.load(0,1,0x90);a.sign8(0,0);a.multiply(3,0,10);a.compare(3,20);
            if(!a.at_least()){a.value(0,20);a.jump(0x1041589c);}else{a.at(0x1041588c);a.compare(3,100);a.value(0,100);if(!a.positive())a.move(0,3);}
            a.at(0x1041589c);a.store<1>(15,0x44,0);a.value(3,0x66660000);a.offset(3,3,0x6667);a.load<1>(0,15,0x44);a.sign8(0,0);a.multiply_high(0,3,0);a.shift_right(0,0,2);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(3,0,10);a.compare(3,10);
            if(!a.at_least()){a.value(0,10);a.jump(0x104158e4);}else{a.at(0x104158d4);a.compare(3,50);a.value(0,50);if(!a.positive())a.move(0,3);}
            a.at(0x104158e4);a.store<1>(15,0x44,0);a.value(3,0x43300000);a.floating_load(2,25,0xc0);a.load(4,15,0x20);a.load<1>(0,15,0x44);a.subtract(4,4,19);a.store(1,0x94,3);a.sign8(0,0);a.exclusive_flags(4,4,0x80000000);a.store(1,0x9c,3);a.multiply(0,0,30000);a.store(1,0x98,4);a.exclusive_flags(0,0,0x80000000);a.floating_load(0,1,0x94);a.store(1,0xa0,0);a.floating_subtract(1,0,2,true);a.floating_load(0,1,0x9c);a.floating_product_single(1,31,1);a.floating_subtract(0,0,2,true);a.floating_compare(1,0);
            if(!a.at_least())a.jump(0x1041594c);else{a.at(0x1041593c);a.store(1,0xa8,0);a.store(1,0xa4,3);a.floating_load(0,1,0xa4);a.floating_subtract(1,0,2,true);}
            a.at(0x1041594c);a.floating_integer(0,1);a.floating_store(0,1,0xac);a.load(0,1,0xb0);a.store(15,0x40,0);a.jump(0x104159a4);
        }else{
            a.at(0x10415960);a.load(3,15,0x20);a.value(0,0x43300000);a.store(1,0x8c,0);a.subtract(0,3,19);a.load(3,2,-0x4b90);a.exclusive_flags(0,0,0x80000000);a.floating_load(2,25,0xc0);a.store(1,0x90,0);a.floating_load(0,3,0xc,true);a.floating_load(1,1,0x8c);a.floating_subtract(1,1,2,true);a.floating_product_single(1,31,1);a.floating_product_single(0,1,0);a.floating_integer(0,0);a.floating_store(0,1,0x94);a.load(0,1,0x98);a.add(23,23,0);
        }
        a.at(0x104159a4);a.compare_registers(23,22);if(!a.nonpositive())a.move(23,22);
    }
    a.at(0x104159b0);a.store(15,0x20,23);a.jump(0x10415a34);
}

bool check_target_and_existing_price(Call& a){
    a.at(0x10414f14);a.load(0,15,0x20);a.compare_registers(23,0);if(!a.positive()){
        a.value(5,0x43300000);a.exclusive_flags(4,0,0x80000000);a.store(1,0x98,4);a.exclusive_flags(6,23,0x80000000);a.floating_load(3,25,0xc0);a.store(1,0x94,5);a.floating_load(1,25,0x30);a.floating_load(0,1,0x94);a.store(1,0x90,6);a.floating_subtract(0,0,3,true);a.store(1,0x8c,5);a.floating_load(2,1,0x8c);a.floating_product(0,1,0);a.floating_subtract(1,2,3);a.floating_compare(1,0);a.compare_greater_or_equal_bit();
        if(a.equal()){
            a.exclusive_flags(0,3,0x80000000);a.store(1,0x94,5);a.floating_load(1,25,0x80);a.store(1,0x98,0);a.floating_load(0,1,0x94);a.store(1,0x90,4);a.floating_subtract(0,0,3);a.store(1,0x8c,5);a.floating_load(2,1,0x8c);a.floating_product(0,1,0);a.floating_subtract(1,2,3);a.floating_compare(1,0);if(a.at_least())return true;release_output(a,0x10414f98);refuse(a,0x10414fb0);return false;
        }
    }
    a.at(0x10414fb8);a.value(6,0x43300000);a.exclusive_flags(5,0,0x80000000);a.store(1,0x98,5);a.exclusive_flags(7,23,0x80000000);a.floating_load(3,25,0xc0);a.store(1,0x94,6);a.floating_load(1,25,0x18);a.floating_load(0,1,0x94);a.store(1,0x90,7);a.floating_subtract(0,0,3);a.store(1,0x8c,6);a.floating_load(2,1,0x8c);a.floating_product(0,1,0);a.floating_subtract(1,2,3);a.floating_compare(1,0);a.compare_less_or_equal_bit_at(0x10414ff4);bool components=a.equal();
    if(components){a.load<1>(4,1,0xbc);a.sign8(4,4,true);components=a.equal();}if(components){a.compare(18,0,false);components=!a.equal();}if(components){a.load<1>(4,18,0);a.compare(4,11);components=a.equal();}
    if(components){
        a.store(1,0x98,5);a.floating_load(1,25,0x28);a.store(1,0x94,6);a.floating_load(0,1,0x94);a.store(1,0x90,7);a.floating_subtract(0,0,3);a.store(1,0x8c,6);a.floating_load(2,1,0x8c);a.floating_product(0,1,0);a.floating_subtract(1,2,3);a.floating_compare(1,0);a.compare_greater_or_equal_bit();bool eligible=a.equal();if(eligible){a.compare(18,0,false);eligible=!a.equal();}if(eligible){a.load<1>(0,18,0x28);a.mask(0,0,15,false,29);a.compare(0,5,false);eligible=a.equal();if(!eligible){a.compare(0,4,false);eligible=a.equal();if(eligible){a.load<2>(0,16,0x5c,true);a.sign8(0,0);a.compare(0,28);eligible=!a.at_least();}}}
        if(eligible)choose_final_components(a);else{release_output(a,0x10415748);refuse(a,0x10415760);}return false;
    }
    a.at(0x10415768);a.compare_registers(23,0);if(!a.at_least()){revise_toward_target(a);return true;}
    a.at(0x104159b8);a.value(4,0x43300000);a.exclusive_flags(3,3,0x80000000);a.store(1,0x98,3);a.exclusive_flags(0,0,0x80000000);a.floating_load(3,25,0xc0);a.store(1,0x94,4);a.floating_load(1,25,0x80);a.floating_load(0,1,0x94);a.store(1,0x90,0);a.floating_subtract(0,0,3);a.store(1,0x8c,4);a.floating_load(2,1,0x8c);a.floating_product(1,1,0);a.floating_subtract(0,2,3);a.floating_compare(0,1);if(a.at_least())return true;
    a.floating_integer(0,1);a.floating_store(0,1,0x8c);a.load(0,1,0x90);a.store(15,0x20,0);a.load(0,15,0x20);a.compare_registers(0,23);if(a.nonpositive())return true;release_output(a,0x10415a14);refuse(a,0x10415a2c);return false;
}

bool check_finance_and_clause(Call& a){
    a.at(0x10415a34);a.load(3,2,-0x7ab8);a.move(4,17);a.load(3,3,0);a.call(0x10103510);a.move(4,16);a.value(5,0);a.value(6,0);a.value(7,1);a.value(8,0);a.call(0x100f5770);a.load(6,15,0x20);a.exclusive_flags(5,3,0x80000000);a.shift_right(0,6,31);a.exclusive_flags(3,0,0x80000000);a.subtract_carry_at(0x10415a6c,0,6,4);a.subtract_extended(3,3,5);a.subtract_extended(3,5,5);a.negate_record_at(0x10415a78,3,3);if(!a.equal()){refuse(a,0x10415a80);return false;}
    a.at(0x10415a88);a.compare(21,0,false);bool clause=!a.equal();if(clause){a.move(3,21);a.call(0x10380520);a.sign8(0,3,true);clause=!a.equal();}
    if(clause){
        a.load(4,15,0x20);a.value(3,0x43300000);a.load(0,21,0x24);a.exclusive_flags(4,4,0x80000000);a.store(1,0x8c,3);a.exclusive_flags(0,0,0x80000000);a.floating_load(2,25,0xc0);a.store(1,0x90,4);a.floating_load(3,25,0xb8);a.floating_load(0,1,0x8c);a.store(1,0x98,0);a.floating_subtract(1,0,2);a.store(1,0x94,3);a.floating_load(0,1,0x94);a.floating_product(1,3,1);a.floating_subtract(0,0,2);a.floating_compare(1,0);if(!a.nonpositive()){a.value(0,0);a.store(15,0x40,0);a.store(15,0x48,0);a.store<1>(15,0x50,0);a.load(0,21,0x24);a.store(15,0x20,0);a.jump(0x10415b2c);}
    }else{
        a.at(0x10415b04);a.compare(18,0,false);bool rebalance=a.equal();if(!rebalance){a.load<1>(0,18,0);a.compare(0,11);rebalance=a.equal();}if(rebalance){a.at(0x10415b18);a.move(3,15);a.move(4,16);a.move(5,24);a.move(6,17);a.call(0x104172f0);}
    }
    a.at(0x10415b2c);return true;
}
}

bool refine_offer_value(Call& a){
    a.at(0x10414ec8);a.load(3,26,0);a.move(4,16);a.move(5,17);a.call(0x103f4240);a.compare(3,0,false);bool prior=!a.equal();if(prior){a.load<1>(0,3,0x34);a.compare(0,2);prior=a.equal();}if(prior){a.load<1>(0,1,0xc4);a.sign8(0,0,true);prior=a.equal();}
    if(prior){a.value(4,0);a.value(5,0);a.value(6,0);a.value(7,1);a.call(0x10416710);a.jump(0x10414f14);}else{a.at(0x10414f10);a.value(3,0xffffffff);}
    return check_target_and_existing_price(a)&&check_finance_and_clause(a);
}

bool price_offer(Call& a){const auto route=select_base_and_period(a);if(route==Route::failed)return false;if(route==Route::finalize)return true;return constrain_payment_distribution(a)&&refine_offer_value(a);}
}
