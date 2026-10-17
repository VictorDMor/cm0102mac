#include "injury_progression.hpp"
#include "injury_progression_random.hpp"

namespace cm::aot::injury_progression {
namespace {
using namespace detail;

void clear_contract_flag(Call& a,std::uint32_t start,std::uint32_t bit,std::uint32_t pool,std::uint32_t join,std::uint32_t draw) {
    a.at(start);a.load(0,26,0x50);a.mask(0,0,bit,true);if(a.equal())return;
    a.load<1>(17,31,0x67);a.sign8(17,17,true);
    if(a.equal()){a.value(18,0);a.jump(join);}
    else if(pool==0x1018deac)bounded_notice_draw<0x12c>(a,pool);else bounded_notice_draw(a,pool);
    advance_notice_pool(a,join);
    a.at(draw);a.load(5,30,0);a.value(3,0x51ec0000);a.load(4,29,0);a.offset(0,3,-0x7ae1);a.load<2>(3,5,0);a.add(4,3,4);
    a.multiply_high(0,0,4);a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,25);a.subtract(0,0,4);a.compare_registers(18,0);
    if(!a.nonpositive()) {
        if(bit==0x20000){a.value(4,0x20000);a.move(3,26);a.offset(4,4,0);}else{a.move(3,26);a.value(4,bit);}
        a.call(0x1037f7d0);a.value(23,1);
    }
}

void improve_contract_reaction(Call& a) {
    a.at(0x1018e418);a.sign8(0,23,true);bool improve=!a.equal();
    if(!improve) {
        draw_notice_trait(a,0x1018e420,0x67,0x1018e434,0x1018e57c);a.compare(18,0);improve=!a.equal();
    }
    if(!improve)return;
    a.at(0x1018e584);a.load<1>(4,26,0x4c);a.sign8(3,4);a.offset(0,3,15);a.compare(0,100);
    if(!a.nonpositive()){a.value(0,100);a.store<1>(26,0x4c,0);a.jump(0x1018e5c0);return;}
    a.at(0x1018e5a4);a.compare(0,-100);
    if(!a.at_least()){a.value(0,0xffffff9c);a.store<1>(26,0x4c,0);a.jump(0x1018e5c0);return;}
    a.at(0x1018e5b8);a.offset(0,4,15);a.store<1>(26,0x4c,0);
}

void change_player_relationship(Call& a,std::uint32_t start,std::uint32_t predicate,std::uint32_t update,std::uint32_t join) {
    a.at(start);a.compare(28,0,false);if(a.equal())return;
    a.load(4,28,0x90);a.compare(4,0,false);if(a.equal())return;
    a.move(3,31);a.call(predicate);a.sign8(0,3,true);if(a.equal())return;
    a.load(4,28,0x90);a.move(3,31);a.call(update);a.at(join);
}

bool update_contract_relationships(Call& a) {
    a.at(0x1018e5c0);a.sign8(0,23,true);
    if(!a.equal()) {
        a.compare(28,0,false);a.value(26,2);
        if(!a.equal()) {
            a.load(4,28,0x90);a.compare(4,0,false);
            if(!a.equal()) {a.move(3,31);a.call(0x100d9580);a.sign8(0,3,true);if(!a.equal()){a.load(4,28,0x90);a.move(3,31);a.call(0x100d9e90);}}
        }
        a.at(0x1018e5fc);a.compare(28,0,false);
        if(!a.equal()) {
            a.load(0,28,0x90);a.compare(0,0,false);
            if(!a.equal()) {
                draw_notice_trait(a,0x1018e610,0x65,0x1018e624,0x1018e76c);a.compare(18,5);
                if(!a.nonpositive()){a.load(4,28,0x90);a.move(3,31);a.value(5,1);a.call(0x100d9960);a.jump(0x1018e978);}
            }
        }
    } else {
        a.at(0x1018e788);a.load(0,21,0);a.compare(0,0,false);bool escalate=!a.equal();
        if(escalate){a.load<1>(3,31,0x65);a.sign8(0,3);a.compare(0,12);escalate=!a.at_least();}
        if(escalate) {
            a.sign8(17,3,true);
            if(a.equal()){a.value(18,0);a.jump(0x1018e8fc);}else bounded_notice_draw(a,0x1018e7b4);
            advance_notice_pool(a,0x1018e8fc);
            a.at(0x1018e924);a.load(5,30,0);a.value(3,0x66660000);a.load(4,29,0);a.offset(0,3,0x6667);a.load<2>(3,5,0);a.add(4,3,4);
            a.multiply_high(0,0,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,5);a.subtract(0,0,4);a.compare_registers(18,0);
            if(!a.at_least()) {a.load(3,21,0);a.move(4,31);a.move(5,28);a.value(6,4);a.call(0x10006900);a.jump(0x1018f328);return true;}
        }
        a.at(0x1018e974);a.value(26,0);
    }
    return false;
}

void adjust_contract_traits(Call& a) {
    a.at(0x1018eb60);a.load(0,24,0);a.offset(3,5,2);a.store(30,0,3);a.compare_registers(3,0,false);
    if(!a.nonpositive()){a.store(30,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
    a.at(0x1018eb84);a.load(3,30,0);a.load(0,29,0);a.load<2>(3,3,0);a.add(3,3,0);a.mask(0,3,0xc0000000,false,30);
    a.mask(3,3,1,false,1);a.subtract(0,3,0);a.mask(0,0,0xffffffff,false,2);a.add(0,0,3);a.compare(0,1);
    if(a.equal()) {
        a.at(0x1018ec28);a.load<1>(3,31,0x63);a.sign8(0,3);a.compare(0,20);if(!a.at_least()){a.offset(0,3,1);a.store<1>(31,0x63,0);}
        a.at(0x1018ec40);a.load<1>(3,31,0x65);a.sign8(0,3);a.compare(0,1);if(a.nonpositive())return;
        a.offset(0,3,-1);a.store<1>(31,0x65,0);a.jump(0x1018f2c8);return;
    }
    const bool upper=a.at_least();
    if(!upper) {a.compare(0,0);if(a.at_least()) {
        a.at(0x1018ebc0);a.load<1>(3,31,0x63);a.sign8(0,3);a.compare(0,20);if(!a.at_least()){a.offset(0,3,1);a.store<1>(31,0x63,0);}
        a.at(0x1018ebd8);a.load<1>(3,31,0x69);a.sign8(0,3);a.compare(0,1);if(!a.nonpositive()){a.offset(0,3,-1);a.store<1>(31,0x69,0);}
        a.at(0x1018ebf0);a.load(4,31,0x14);a.load<1>(3,4,0x44);a.sign8(0,3);a.compare(0,1);if(!a.nonpositive()){a.offset(0,3,-1);a.store<1>(4,0x44,0);}
        a.at(0x1018ec0c);a.load<1>(3,31,0x65);a.sign8(0,3);a.compare(0,1);if(a.nonpositive())return;
        a.offset(0,3,-1);a.store<1>(31,0x65,0);a.jump(0x1018f2c8);return;
    }a.jump(0x1018ec5c);}
    a.at(0x1018ec5c);a.load<1>(3,31,0x69);a.sign8(0,3);a.compare(0,1);if(!a.nonpositive()){a.offset(0,3,-1);a.store<1>(31,0x69,0);}
    a.at(0x1018ec74);a.load(4,31,0x14);a.load<1>(3,4,0x44);a.sign8(0,3);a.compare(0,1);if(a.nonpositive())return;
    a.offset(0,3,-1);a.store<1>(4,0x44,0);a.jump(0x1018f2c8);
}

void assess_contract_traits(Call& a) {
    a.at(0x1018e978);a.load(3,31,0x14);a.value(23,1);a.load<1>(5,31,0x67);a.load<1>(4,31,0x65);a.load<2>(0,3,10,true);
    a.sign8(5,5);a.sign8(3,4);a.add(3,5,3);a.compare(0,7250);a.sign16(3,3);if(!a.nonpositive())a.offset(3,3,-5);
    a.at(0x1018e9a8);a.sign16(17,3,true);
    if(a.equal()){a.value(18,0);a.jump(0x1018eb00);}else bounded_notice_draw<0x11c,false>(a,0x1018e9b8);
    advance_notice_pool(a,0x1018eb00);
    a.at(0x1018eb28);a.load(5,30,0);a.value(3,0x66660000);a.load(4,29,0);a.offset(0,3,0x6667);a.load<2>(3,5,0);a.add(4,3,4);
    a.multiply_high(0,0,4);a.shift_right(0,0,3);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,20);a.subtract(0,0,4);a.compare_registers(18,0);
    if(!a.at_least())adjust_contract_traits(a);
}

bool process_contract_injury(Call& a) {
    a.at(0x1018de5c);a.load(3,2,-0x7b7c);a.move(4,31);a.value(23,0);a.load(3,3,0);a.call(0x10082f20);a.move(26,3,true);bool eligible=!a.equal();
    if(eligible){a.load(0,31,0x14);a.compare(0,0,false);eligible=!a.equal();}
    if(eligible){a.compare(28,0,false);eligible=!a.equal();}
    if(!eligible){a.at(0x1018ec94);a.value(23,1);a.value(26,0);a.jump(0x1018f2c8);return false;}
    clear_contract_flag(a,0x1018de8c,0x20000,0x1018deac,0x1018dff4,0x1018e01c);
    clear_contract_flag(a,0x1018e068,0x20,0x1018e088,0x1018e1d0,0x1018e1f8);
    clear_contract_flag(a,0x1018e240,0x40,0x1018e260,0x1018e3a8,0x1018e3d0);
    improve_contract_reaction(a);if(update_contract_relationships(a))return true;
    assess_contract_traits(a);return false;
}

void clear_persistent_risk(Call& a,std::uint32_t start) {
    a.at(start);a.load(0,31,0);a.value(5,0xffffffff);a.load(3,17,0);a.value(4,0);a.multiply(0,0,0x24);a.value(26,2);a.add(3,3,0);a.store<1>(3,0x14,5);
    a.load(0,31,0);a.load(3,17,0);a.multiply(0,0,0x24);a.add(3,3,0);a.store<1>(3,0x15,4);
}

void update_risk_start(Call& a) {
    a.at(0x1018eca8);a.compare(4,77);
    if(!a.equal()){a.at(0x1018ed6c);a.value(26,0xffffffff);a.jump(0x1018ef40);return;}
    a.sign16(0,5,true);
    if(a.equal()) {
        clear_persistent_risk(a,0x1018ecb8);a.load<1>(0,31,0x65);a.sign8(0,0);a.compare(0,10);if(a.nonpositive())return;
        a.value(23,1);a.jump(0x1018ef40);return;
    }
    a.at(0x1018ed04);a.compare(5,5);
    if(!a.at_least()) {
        a.load(0,31,0);a.value(4,0);a.load(3,17,0);a.value(26,1);a.multiply(0,0,0x24);a.add(3,3,0);a.store<1>(3,0x15,4);
        a.load<1>(0,31,0x65);a.sign8(0,0);a.compare(0,15);if(a.nonpositive())return;
        a.load<1>(0,31,0x63);a.sign8(0,0);a.compare(0,10);if(a.nonpositive())return;
        a.value(23,1);a.jump(0x1018ef40);return;
    }
    a.at(0x1018ed50);a.load<1>(0,31,0x69);a.value(26,0);a.sign8(0,0);a.compare(0,5);if(a.at_least())return;
    a.value(23,0xffffffff);a.jump(0x1018ef40);
}

void zero_risk_counter(Call& a,std::uint32_t start) {
    a.at(start);a.load(0,31,0);a.value(4,0);a.load(3,17,0);a.value(26,1);a.multiply(0,0,0x24);a.add(3,3,0);a.store<1>(3,0x15,4);
}

void update_risk_end(Call& a) {
    a.at(0x1018ed74);a.compare(4,79);
    if(a.equal()) {
        a.compare(5,30000);
        if(!a.at_least()){clear_persistent_risk(a,0x1018ed84);a.jump(0x1018ef1c);}
        else {
            a.at(0x1018edbc);a.load(0,24,0);a.offset(3,3,2);a.store(30,0,3);a.compare_registers(3,0,false);
            if(!a.nonpositive()){a.store(30,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
            a.at(0x1018ede0);a.load(5,30,0);a.value(3,0x66660000);a.load(4,29,0);a.offset(0,3,0x6667);a.load<2>(3,5,0);a.add(4,3,4);
            a.multiply_high(0,0,4);a.shift_right(0,0,1);a.mask(3,0,1,false,1);a.add(0,0,3);a.multiply(0,0,5);a.subtract(0,0,4,true);
            if(!a.equal()){zero_risk_counter(a,0x1018ee14);a.jump(0x1018ef1c);}
            else {
                a.at(0x1018ee34);a.load(0,24,0);a.offset(3,5,2);a.store(30,0,3);a.value(26,3);a.compare_registers(3,0,false);
                if(!a.nonpositive()){a.store(30,0,25);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(29,0,0);}
                a.at(0x1018ee5c);a.load(3,30,0);a.load(4,31,0);a.load<2>(3,3,0);a.load(0,29,0);a.multiply(4,4,0x24);a.load(5,17,0);
                a.add(3,3,0);a.mask(0,3,0xc0000000,false,30);a.mask(3,3,1,false,1);a.add(5,5,4);a.subtract(0,3,0);a.load<1>(4,5,0x15);
                a.mask(0,0,0xffffffff,false,2);a.add(3,0,3);a.offset(0,3,1);a.sign8(0,0);a.add(0,4,0);a.store<1>(5,0x15,0);a.jump(0x1018ef1c);
            }
        }
    } else {
        a.at(0x1018eea8);a.compare(4,78);
        if(a.equal()) {
            a.compare(5,29500);
            if(!a.at_least()){clear_persistent_risk(a,0x1018eeb8);a.jump(0x1018ef1c);}
            else {
                a.at(0x1018eef0);a.compare(5,30000);
                if(!a.at_least()){zero_risk_counter(a,0x1018eef8);a.jump(0x1018ef1c);}else{a.at(0x1018ef18);a.value(26,0);}
            }
        }
    }
    a.at(0x1018ef1c);a.load(0,31,0);a.value(5,0);a.load(4,2,-0x7b9c);a.multiply(3,0,100);a.load(4,4,0);a.offset(3,3,13);
    a.indexed_load<1>(0,4,3);a.insert_mask(0,5,0x80,7);a.indexed_store<1>(0,4,3);
}

void update_risk_relationships(Call& a) {
    a.at(0x1018ef40);a.sign8(0,23);a.compare(0,1);
    if(a.equal()) {
        change_player_relationship(a,0x1018ef4c,0x100d9580,0x100d9e90,0x1018ef7c);
        a.at(0x1018ef7c);a.compare(28,0,false);if(a.equal())return;
        a.load(0,28,0x90);a.compare(0,0,false);if(a.equal())return;
        draw_notice_trait(a,0x1018ef90,0x65,0x1018efa4,0x1018f0ec);a.compare(18,5);if(a.nonpositive())return;
        a.load(4,28,0x90);a.move(3,31);a.value(5,1);a.call(0x100d9960);a.jump(0x1018f2c8);return;
    }
    a.at(0x1018f108);a.compare(0,-1);if(!a.equal())return;
    change_player_relationship(a,0x1018f110,0x100d9520,0x100d9e10,0x1018f140);
    a.at(0x1018f140);a.compare(28,0,false);if(a.equal())return;
    a.load(0,28,0x90);a.compare(0,0,false);if(a.equal())return;
    draw_notice_trait(a,0x1018f154,0x69,0x1018f168,0x1018f2b0);a.compare(18,0);if(!a.equal())return;
    a.load(4,28,0x90);a.move(3,31);a.value(5,1);a.call(0x100d9b80);
}

bool update_progress_effects(Call& a) {
    a.at(0x1018ddd0);a.multiply(0,26,12);a.load(3,2,-0x64d4);a.load(28,31,0x24);a.value(26,0xffffffff);a.value(23,0);a.add(3,3,0);a.load<1>(0,3,1);a.compare(0,12);
    if(!a.equal())return false;
    advance_notice_pool(a,0x1018ddf4);
    a.at(0x1018de1c);a.load(3,30,0);a.value(5,0x10620000);a.sign8(4,19);a.load(6,29,0);a.load<2>(7,3,0);a.offset(0,5,0x4dd3);a.compare(4,81);
    a.add(6,7,6);a.multiply_high(0,0,6);a.shift_right(0,0,11);a.mask(5,0,1,false,1);a.add(0,0,5);a.multiply(0,0,32000);a.subtract(0,0,6);a.sign16(5,0);
    if(a.equal())return process_contract_injury(a);
    a.at(0x1018eca0);a.sign8(0,18,true);
    if(a.equal())update_risk_start(a);else update_risk_end(a);
    update_risk_relationships(a);return false;
}

void route_progress_notice(Call& a) {
    a.at(0x1018f2c8);a.offset(3,1,0x38);a.sign8(5,26);a.value(4,9);a.call(0x102cb4b0);a.offset(3,1,0x38);a.sign8(5,23);a.value(4,10);a.call(0x102cb4b0);a.compare(28,0,false);
    if(!a.equal()){a.load(7,28,0x2c);a.jump(0x1018f2fc);}else{a.at(0x1018f2f8);a.load(7,31,12);}
    a.at(0x1018f2fc);a.compare(7,0,false);
    if(!a.equal()){a.load(8,7,0x74);a.jump(0x1018f310);}else{a.at(0x1018f30c);a.value(8,0);}
    a.at(0x1018f310);a.load(3,20,0);a.move(5,28);a.move(9,22);a.offset(4,1,0x38);a.value(6,0);a.call(0x102cc030);
}
}

void prepare_progress_notice(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,notice_entry,dependencies,observer);
    a.link();a.multiple(17);a.move(31,4,true);a.load(20,2,-0x7bf0);a.load(21,2,-0x7b24);a.move(17,3);a.load(27,2,-0x543c);a.move(18,5);
    a.load(29,2,-0x7b90);a.move(19,6);a.load(25,2,-0x7b8c);a.move(22,7);a.load(24,2,-0x7b88);a.move(23,8);a.load(30,2,-0x7b84);a.save_link();a.frame(384);
    bool eligible=!a.equal();
    if(eligible) {
        a.sign8(0,22,true);
        if(a.equal()) {
            a.load(4,31,0x24);a.compare(4,0,false);eligible=!a.equal();
            if(eligible){a.load(3,20,0);a.call(0x102cdbf0);a.sign8(0,3,true);if(a.equal()){a.jump(0x1018f328);eligible=false;}}
        }
    }
    if(eligible) {
        a.at(0x1018dd20);a.offset(3,1,0x38);a.value(4,2001);a.value(5,0);a.call(0x102bdf90);a.offset(3,1,0x38);a.sign8(5,18);a.value(4,0);a.call(0x102cb4b0);
        a.sign8(26,19);a.offset(3,1,0x38);a.move(5,26);a.value(4,1);a.call(0x102cb4b0);a.load(5,31,0);a.offset(3,1,0x38);a.value(4,2);a.call(0x102cb4b0);
        for(auto [field,index]:{std::pair{0x28,3},std::pair{0x2c,4},std::pair{0x30,5}}){a.load(5,31,field);a.offset(3,1,0x38);a.value(4,index);a.load(5,5,0x20);a.call(0x102cb4b0);}
        a.load<2>(5,31,0x34,true);a.offset(3,1,0x38);a.value(4,6);a.call(0x102cb4b0);a.load<2>(5,31,0x36);a.offset(3,1,0x38);a.value(4,7);a.call(0x102cb4b0);
        a.offset(3,1,0x38);a.sign16(5,23);a.value(4,8);a.call(0x102cb4b0);
        if(!update_progress_effects(a))route_progress_notice(a);
    }
    a.at(0x1018f328);a.pop(384);a.multiple(17,true);a.finish();
}
}
