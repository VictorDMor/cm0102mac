#include "movement_choice.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::movement_choice {
namespace {
using Call = SemanticGuestCall;
constexpr std::uint32_t row=0x101d5690, column=0x101d5680, side=0x10283690;
constexpr std::uint32_t action=0x101d5660, sample=0x101d3c90, role=0x101e3d50;

// Preserve the original spill/reload conversion and its live bias constant.
// Scratch aliases and floating status remain part of the caller's contract.
[[gnu::always_inline]] inline void integer_to_float(Call& a,unsigned destination,std::int32_t scratch,bool single=true) {
    a.floating_load(1,31,0x60); a.exclusive_flags(0,0,0x80000000); a.store(1,scratch+4,0);
    a.value(0,0x43300000); a.store(1,scratch,0); a.floating_load(0,1,scratch);
    a.floating_subtract(destination,0,1,single);
}
[[gnu::always_inline]] inline void finish_choice(Call& a) {
    a.at(0x101d8d44); a.load(0,1,0xc8); a.offset(1,1,0xc0); a.move(11,1);
    a.call(0x10773858); a.restore_link(); a.multiple(15,true,-32); a.finish();
}
[[gnu::always_inline]] inline bool initialize_score(Call& a) {
    a.sign8(0,24); a.mask(0,0,2); a.compare(0,0); bool jitter=a.equal();
    if(jitter){a.move(3,29);a.call(action);a.sign8(4,3);a.compare(4,0);jitter=a.equal();}
    if(jitter){
        a.move(3,29);a.call(0x101d5670);a.sign8(0,3);a.compare(0,0);
        if(a.equal()){a.value(3,0);a.jump(0x101d8d44);return false;}
        a.at(0x101d8304);a.value(3,6);a.call(sample);a.move(15,3);a.value(3,6);a.call(sample);
        a.subtract(0,3,15);integer_to_float(a,30,0x4c);
    }
    a.at(0x101d8338);a.move(3,29);a.call(side);a.sign8(0,3);a.leading_zeroes(3,0);a.mask(3,3,0x07ffffff,false,27);a.sign8(19,3);
    return true;
}
[[gnu::always_inline]] inline bool column_allowed(Call& a) {
    a.at(0x101d835c);a.sign8(0,26);a.compare(0,0);if(a.less())return false;
    a.sign8(0,26);a.compare(0,12);if(a.at_least())return false;
    a.sign8(0,24);a.mask(3,0,4);a.compare(3,0);if(a.equal())return true;
    a.move(3,29);a.call(action);a.sign8(0,3);a.compare(0,2);
    if(!a.at_least()){
        a.value(3,3);a.call(sample);a.move(15,3);a.move(3,29);a.call(side);a.move(4,3);a.move(3,23);a.move(5,26);a.call(0x101e4ea0);
        a.add(15,3,15);a.value(3,3);a.call(sample);a.subtract(0,3,15);a.sign8(22,0);a.jump(0x101d83f0);
    }else{
        a.at(0x101d83d4);a.move(3,29);a.call(side);a.move(4,3);a.move(3,23);a.move(5,26);a.call(0x101e4ea0);a.move(22,3);
    }
    a.at(0x101d83f0);a.move(3,29);a.call(entries[1]);a.sign8(4,3);a.compare(4,0);
    if(!a.equal()){
        a.move(3,29);a.call(side);a.sign8(0,3);a.leading_zeroes(0,0);a.mask(0,0,0x07ffffff,false,27);a.sign8(4,0);a.move(3,23);a.call(0x101d7d30);
        a.sign8(0,3);a.compare(0,0);if(!a.equal())a.offset(22,22,1);
    }
    a.at(0x101d8434);a.sign8(0,22);a.compare(0,2);return !a.positive();
}
[[gnu::always_inline]] inline bool candidate_allowed(Call& a) {
    a.at(0x101d844c);a.sign8(0,28);a.compare(0,0);if(a.less())return false;
    a.sign8(0,28);a.compare(0,9);if(a.at_least())return false;
    a.sign8(0,27);a.sign8(3,28);a.compare_registers(0,3);bool current=a.equal();
    if(current){a.sign8(0,20);a.sign8(3,26);a.compare_registers(0,3);current=a.equal();}
    if(current){a.sign8(0,24);a.mask(0,0,2);a.compare(0,0);if(!a.equal())return false;}
    a.at(0x101d8494);a.sign8(0,24);a.mask(0,0,2);a.compare(0,0);
    if(!a.equal()){
        a.move(3,29);a.move(4,28);a.move(5,26);a.call(0x10245050);a.sign8(0,3);a.compare(0,0);if(a.equal())return false;
    }
    a.at(0x101d84c0);a.move(3,23);a.move(4,29);a.call(role);a.sign16(0,3);a.mask(0,0,0x30);a.compare(0,0);if(a.equal())return true;
    a.move(3,23);a.move(4,29);a.call(role);a.sign16(0,3);a.mask(0,0,0x880,true);a.compare(0,0);if(a.equal())return true;
    a.move(3,23);a.move(4,28);a.call(entries[2]);a.sign8(0,3);a.compare(0,0);if(!a.equal())return true;
    a.move(3,23);a.move(4,27);a.call(entries[2]);a.sign8(0,3);a.compare(0,0);if(a.equal())return true;
    a.move(3,29);a.call(side);a.move(4,3);a.move(3,23);a.move(5,27);a.move(6,20);a.call(entries[3]);a.sign8(0,3);a.compare(0,0);if(!a.equal())return true;
    a.sign8(0,24);a.mask(0,0,1);a.compare(0,0);return !a.equal();
}
[[gnu::always_inline]] inline void lateral_penalty(Call& a) {
    a.load<1>(0,29,0x2e);a.sign8(0,0);integer_to_float(a,1,0x4c);
    a.floating_load(0,31,0xc0);a.floating_product(0,1,0);a.floating_round_single(0,0);a.floating_subtract(31,31,0,true);a.jump(0x101d8870);
}
[[gnu::always_inline]] inline void apply_lateral_preference(Call& a) {
    a.load<1>(0,29,0x2d);a.sign8(0,0);a.compare(0,1);const bool opposite=a.equal();
    if(!opposite){
        if(a.at_least())return;
        a.compare(0,0);if(!a.at_least()){a.jump(0x101d8870);return;}
    }
    a.at(opposite?0x101d8728:0x101d8650);a.load<1>(0,29,0x27);a.sign8(0,0);a.compare(0,0);const bool other=!a.equal();
    const bool increasing=other==opposite;
    a.at(opposite?(other?0x101d879c:0x101d8738):(other?0x101d86c4:0x101d8660));
    a.sign8(0,28);a.sign8(3,27);a.compare_registers(0,3);bool penalize=increasing?a.positive():a.less();
    if(!penalize){
        a.sign8(0,28);a.sign8(3,27);a.compare_registers(0,3);if(increasing?a.less():a.positive())return;
        a.sign8(0,28);a.compare(0,4);if(increasing?a.nonpositive():a.at_least())return;
    }
    a.at(opposite?(other?0x101d87c8:0x101d8764):(other?0x101d86f0:0x101d868c));lateral_penalty(a);
}
[[gnu::always_inline]] inline void count_occupants(Call& a) {
    a.at(0x101d8800);a.value(18,0);a.jump(0x101d8854);
    for(;;){
        a.at(0x101d8854);a.sign8(4,18);a.sign8(3,19);a.offset(0,3,0x58);a.indexed_load<1>(0,21,0);a.sign8(0,0);a.compare_registers(4,0);if(!a.less())break;
        a.at(0x101d8808);a.sign8(0,19);a.multiply(3,0,0x2c);a.sign8(0,18);a.mask(0,0,0xfffffffc,false,2);a.add(0,3,0);a.indexed_load(0,21,0);a.store(1,0x40,0);
        a.load(3,1,0x40);a.call(0x101d3b50);a.sign8(0,3);a.compare(0,0);
        if(!a.less()){a.load(3,1,0x40);a.load<1>(0,3,0x2c);a.sign8(0,0);a.compare(0,2);if(!a.at_least())a.offset(25,25,1);}
        a.at(0x101d8850);a.offset(18,18,1);
    }
}
[[gnu::always_inline]] inline void score_occupancy(Call& a) {
    a.sign8(0,24);a.mask(0,0,2);a.compare(0,0);
    if(a.equal())count_occupants(a);
    else{
        a.sign8(3,19);a.offset(0,3,0x58);a.indexed_load<1>(25,21,0);a.sign8(0,25);a.compare(0,0);
        if(!a.equal()){
            a.value(3,3);a.call(sample);a.move(15,3);a.load(3,29,0x70);a.load<1>(3,3,0x26);a.sign8(3,3);a.call(sample);a.compare_registers(3,15);
            if(!a.nonpositive()){
                a.sign8(3,25);a.offset(0,3,-2);a.compare(0,0);
                if(!a.at_least()){a.value(0,0);a.store(1,0x3c,0);a.jump(0x101d85f8);}
                else{a.at(0x101d85ec);a.sign8(3,25);a.offset(0,3,-2);a.store(1,0x3c,0);}
                a.at(0x101d85f8);a.load(0,1,0x3c);a.sign8(25,0);
            }
        }
        a.at(0x101d8600);a.sign8(0,25);integer_to_float(a,1,0x4c);a.floating_load(0,31,0x90);
        a.floating_subtract(0,1,0);a.floating_round_single(0,0);a.floating_subtract(31,31,0,true);apply_lateral_preference(a);
    }
    a.at(0x101d8870);a.sign8(0,25);a.compare(0,0);bool distance=a.equal();
    if(!distance){a.sign8(0,24);a.mask(0,0,2);a.compare(0,0);distance=!a.equal();}
    if(distance){
        a.at(0x101d888c);a.move(3,23);a.move(4,19);a.move(5,28);a.move(6,26);a.value(7,1);a.call(0x101e3f10);
        a.floating_move(28,1);a.floating_product_single(0,28,28);a.floating_sum_single(31,31,0);a.jump(0x101d8908);
    }else{
        a.at(0x101d88b4);a.floating_load(3,31,0x98);a.sign8(0,25);integer_to_float(a,2,0x4c);
        a.sign8(0,25);integer_to_float(a,0,0x44);a.floating_product_single(0,2,0);a.floating_product(0,3,0);a.floating_round_single(0,0);a.floating_subtract(31,31,0,true);
    }
}
[[gnu::always_inline]] inline void score_progress(Call& a) {
    a.at(0x101d8908);a.load<1>(0,29,0x27);a.sign8(0,0);a.compare(0,0);bool backwards=false;
    if(a.equal()){a.move(3,29);a.call(column);a.sign8(0,3);a.sign8(3,26);a.compare_registers(3,0);backwards=a.less();}
    if(!backwards){
        a.at(0x101d8930);a.load<1>(0,29,0x27);a.sign8(0,0);a.compare(0,1);
        if(a.equal()){a.move(3,29);a.call(column);a.sign8(0,3);a.sign8(3,26);a.compare_registers(3,0);backwards=!a.nonpositive();}
    }
    if(backwards){
        a.at(0x101d8958);a.sign8(0,24);a.mask(0,0,8);a.compare(0,0);
        if(!a.equal()){a.value(17,0);a.jump(0x101d89b4);}
        else{a.at(0x101d8970);a.value(17,1);a.floating_load(0,31,0xc8);a.floating_subtract(31,31,0);a.floating_round_single(31,31);a.jump(0x101d89b4);}
    }else{
        a.at(0x101d8984);a.value(17,0);a.sign8(0,26);a.sign8(3,20);a.compare_registers(0,3);
        if(!a.equal()){a.floating_load(0,31,0xd0);a.floating_sum(31,31,0);a.floating_round_single(31,31);a.jump(0x101d89b4);}
        else{a.at(0x101d89a8);a.floating_load(0,31,0x90);a.floating_sum(31,31,0);a.floating_round_single(31,31);}
    }
    a.at(0x101d89b4);a.sign8(0,22);a.compare(0,0);
    if(!a.nonpositive()){a.sign8(0,22);a.mask(0,0,0xfffffffc,false,2);integer_to_float(a,0,0x44);a.floating_subtract(31,31,0,true);}
}
[[gnu::always_inline]] inline void score_central_route(Call& a) {
    a.at(0x101d89f4);a.sign8(0,24);a.mask(0,0,2);a.compare(0,0);bool inspect=a.equal();
    if(!inspect){a.sign8(0,24);a.mask(0,0,1);a.compare(0,0);inspect=!a.equal();}
    if(inspect){
        a.at(0x101d8a14);a.sign8(0,26);a.compare(0,3);bool edge=a.less();
        if(!edge){a.sign8(0,26);a.compare(0,8);edge=!a.nonpositive();}
        if(edge){a.at(0x101d8a2c);a.sign8(0,28);a.compare(0,6);bool central=!a.at_least();
            if(central){a.sign8(0,28);a.compare(0,2);central=!a.nonpositive();}
            if(central){a.floating_load(0,31,0xd8);a.floating_sum(31,31,0);a.floating_round_single(31,31);}
        }
    }
    a.at(0x101d8a50);a.sign8(0,28);a.compare(0,4);bool reward=a.equal();
    if(!reward){
        a.sign8(0,28);a.compare(0,2);bool first=!a.nonpositive();
        if(first){a.move(3,29);a.call(row);a.sign8(0,3);a.compare(0,4);first=!a.at_least();}
        if(first){a.move(3,29);a.call(row);a.sign8(0,3);a.sign8(3,28);a.compare_registers(0,3);reward=a.at_least();}
        if(!reward){
            a.at(0x101d8a94);a.sign8(0,28);a.compare(0,6);bool second=!a.at_least();
            if(second){a.move(3,29);a.call(row);a.sign8(0,3);a.compare(0,4);second=!a.nonpositive();}
            if(second){a.move(3,29);a.call(row);a.sign8(0,3);a.sign8(3,28);a.compare_registers(0,3);reward=!a.positive();}
        }
    }
    if(reward){a.at(0x101d8acc);a.floating_load(0,31,0x40);a.floating_sum(31,31,0);a.floating_round_single(31,31);}
}
[[gnu::always_inline]] inline void score_wide_route(Call& a) {
    a.at(0x101d8ad8);a.sign8(0,24);a.mask(0,0,2);a.compare(0,0);
    if(a.equal()){
        a.sign8(0,27);a.compare(0,4);bool upper=!a.nonpositive();
        if(upper){a.sign8(3,27);a.offset(0,3,-4);a.sign8(3,28);a.offset(3,3,-4);a.compare_registers(0,3);upper=!a.at_least();}
        if(upper){a.sign8(3,27);a.offset(0,3,-4);a.multiply(0,0,3);integer_to_float(a,0,0x44);a.floating_subtract(31,31,0,true);a.jump(0x101d8cd0);return;}
        a.at(0x101d8b3c);a.sign8(0,27);a.compare(0,4);if(a.at_least())return;
        a.sign8(3,27);a.offset(0,3,-4);a.sign8(3,28);a.offset(3,3,-4);a.compare_registers(0,3);if(a.nonpositive())return;
        a.sign8(0,27);a.subtract_from(0,0,4);a.multiply(0,0,3);integer_to_float(a,0,0x44);a.floating_subtract(31,31,0,true);a.jump(0x101d8cd0);return;
    }
    a.at(0x101d8b90);a.move(3,23);a.move(4,27);a.call(entries[2]);a.sign8(0,3);a.compare(0,0);bool bonus=!a.equal();
    if(bonus){a.move(3,23);a.move(4,28);a.call(entries[2]);a.sign8(3,3);a.compare(3,0);bonus=!a.equal();}
    if(bonus){a.sign8(3,24);a.mask(0,3,1);a.compare(0,0);bonus=a.equal();}
    if(bonus){a.floating_load(1,29,0xe4,true);a.floating_load(0,31,0xe0);a.floating_product(0,1,0);a.floating_round_single(0,0);a.floating_sum_single(31,31,0);}
    a.at(0x101d8be4);a.sign8(0,27);a.compare(0,4);bool upper=!a.less();
    if(upper){a.sign8(3,27);a.offset(4,3,-4);a.sign8(3,28);a.offset(0,3,-4);a.compare_registers(4,0);upper=!a.at_least();}
    if(upper){
        a.floating_load(2,31,0xe8);a.sign8(15,27);a.offset(3,15,-4);a.floating_load(1,31,0x60);a.exclusive_flags(0,3,0x80000000);a.store(1,0x48,0);a.value(0,0x43300000);a.store(1,0x44,0);a.floating_load(0,1,0x44);a.floating_subtract(0,0,1);
        a.floating_product(0,2,0);a.floating_round_single(0,0);a.floating_subtract(31,31,0,true);a.jump(0x101d8c98);
    }else{
        a.at(0x101d8c40);a.sign8(0,27);a.compare(0,4);bool lower=!a.positive();
        if(lower){a.sign8(3,27);a.offset(0,3,-4);a.sign8(3,28);a.offset(3,3,-4);a.compare_registers(0,3);lower=!a.nonpositive();}
        if(lower){a.floating_load(2,31,0xe8);a.sign8(0,27);a.subtract_from(0,0,4);integer_to_float(a,0,0x44,false);a.floating_product(0,2,0);a.floating_round_single(0,0);a.floating_subtract(31,31,0,true);}
    }
    a.at(0x101d8c98);a.load(3,1,0xe0);a.load<1>(3,3,0);a.sign8(0,3);a.sign8(15,28);a.compare_registers(0,15);if(!a.equal())return;
    a.load(4,1,0xe4);a.load<1>(3,4,0);a.sign8(15,3);a.sign8(3,26);a.compare_registers(15,3);if(!a.equal())return;
    a.floating_load(0,30,0x34,true);a.floating_sum_single(31,31,0);
}
[[gnu::always_inline]] inline void evaluate_candidate(Call& a) {
    a.at(0x101d8560);a.sign8(0,28);a.multiply(3,0,0x450);a.sign8(0,26);a.multiply(0,0,0x5c);a.add(3,3,0);a.offset(21,3,0xa08);a.add(21,23,21);a.floating_move(31,30);a.value(25,0);
    score_occupancy(a);score_progress(a);a.at(0x101d89e8);a.sign8(0,17);a.compare(0,0);
    if(a.equal()){score_central_route(a);score_wide_route(a);}
    a.at(0x101d8cd0);a.floating_compare(31,29);a.compare_greater_or_equal_bit();
    if(a.equal()){a.floating_move(29,31);a.move(16,28);a.store<1>(1,0x38,26);}
}
}

[[gnu::flatten]] void choose_cell(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,entries[0],dependencies,observer);
    a.link();a.move(11,1);a.call(0x10773808);a.multiple(15,false,-32);a.save_link();a.frame(0xc0);
    a.move(23,3);a.move(29,4);a.store(1,0xe0,5);a.store(1,0xe4,6);a.move(24,7);a.load(30,2,-0x52c8);a.load(31,2,-0x52c4);
    a.value(22,0xfffffff6);a.value(16,0xffffffff);a.value(3,0xffffffff);a.store<1>(1,0x38,3);a.floating_load(30,30,0xc,true);a.floating_load(29,30,0x30,true);
    a.move(3,29);a.call(row);a.sign8(27,3);a.move(3,29);a.call(column);a.sign8(20,3);
    if(!initialize_score(a)){finish_choice(a);return;}
    a.offset(0,20,-1);a.sign8(26,0);a.jump(0x101d8d04);
    for(;;){
        a.at(0x101d8d04);a.sign8(4,26);a.offset(3,20,1);a.sign8(0,3);a.compare_registers(4,0);if(!a.nonpositive())break;
        if(column_allowed(a)){
            a.at(0x101d8440);a.offset(0,27,-1);a.sign8(28,0);a.jump(0x101d8cec);
            for(;;){
                a.at(0x101d8cec);a.sign8(0,28);a.offset(4,27,1);a.sign8(3,4);a.compare_registers(0,3);if(!a.nonpositive())break;
                if(candidate_allowed(a))evaluate_candidate(a);
                a.at(0x101d8ce8);a.offset(28,28,1);
            }
        }
        a.at(0x101d8d00);a.offset(26,26,1);
    }
    a.at(0x101d8d18);a.load<1>(0,1,0x38);a.load(4,1,0xe4);a.store<1>(4,0,0);a.load(3,1,0xe0);a.store<1>(3,0,16);a.sign8(0,16);a.compare(0,-1);
    if(!a.nonpositive()){a.value(3,1);a.jump(0x101d8d44);}else{a.at(0x101d8d40);a.value(3,0);}
    finish_choice(a);
}
void adjustment_byte(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,entries[1],dependencies,observer);a.load<1>(3,3,0x109);a.finish();
}
void outside_central_rows(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,entries[2],dependencies,observer);a.sign8(0,4);a.compare(0,2);bool outside=a.less();
    if(!outside){a.sign8(0,4);a.compare(0,6);outside=!a.nonpositive();}
    if(outside){a.at(0x101ee9d8);a.value(3,1);a.jump(0x101ee9e4);}else{a.at(0x101ee9e0);a.value(3,0);}
    a.at(0x101ee9e4);a.finish();
}
void inside_end_area(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,entries[3],dependencies,observer);a.sign8(0,4);a.compare(0,1);const bool first=a.equal();
    a.at(first?0x101eea3c:0x101eea74);a.sign8(0,5);a.compare(0,2);bool inside=!a.less();
    if(inside){a.sign8(0,5);a.compare(0,6);inside=!a.positive();}
    if(inside){a.sign8(0,6);a.compare(0,first?10:0);inside=!a.less();}
    if(inside){a.sign8(0,6);a.compare(0,first?11:1);inside=!a.positive();}
    if(inside){a.value(3,1);a.jump(0x101eeab0);}else{a.at(0x101eeaac);a.value(3,0);}
    a.at(0x101eeab0);a.finish();
}
}
