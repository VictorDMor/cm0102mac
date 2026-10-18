#include "money_text_detail.hpp"
namespace cm::aot::money_text::detail {
// Prepend each signed decimal remainder and retain the original low64-bit
// reconstructed sum. The unsigned comparison and fifteen-character cap are
// separate from the signed quotient/remainder services.
void build_decimal_digits(Call& a){
    for(;;){
        a.at(0x1000b588);a.sign8(0,18);a.compare(0,15);if(a.at_least())break;
        a.exclusive_flags(4,20,0x80000000);a.exclusive_flags(3,17,0x80000000);
        a.subtract_register_carry(0,16,19);a.subtract_extended(3,3,4);a.subtract_extended(3,4,4);a.negate_record(3,3);
        if(a.equal())break;
        a.at(0x1000b4e4);
    a.offset(0,23,0x8);
    a.move(3,17);
    a.add(6,25,0);
    a.move(4,16);
    a.load(5,6,0x0);
    a.load(6,6,0x4);
    a.call(0x10773b70);
    a.add(21,25,24);
    a.load(22,21,0x0);
    a.load(21,21,0x4);
    a.move(5,22);
    a.move(6,21);
    a.call(0x10773960);
    a.offset(0,18,0x1);
    a.sign8(5,0);
    a.jump(0x1000b53c);
        for(;;){a.at(0x1000b53c);a.sign8(0,5,true);if(!a.positive())break;
            a.at(0x1000b528);
    a.sign8(3,5);
    a.offset(5,5,-0x1);
    a.offset(0,3,-0x1);
    a.indexed_load<1>(0,31,0);
    a.indexed_store<1>(0,31,3);
        }
        a.at(0x1000b544);
    a.sign8(5,4);
    a.offset(24,24,0x8);
    a.offset(4,5,0x30);
    a.offset(23,23,0x8);
    a.store<1>(1,0xa0,4);
    a.unsigned_multiply_high(3,21,5);
    a.shift_right(0,5,31);
    a.offset(18,18,0x1);
    a.product(4,22,5);
    a.add(4,3,4);
    a.product(3,21,0);
    a.product(5,21,5);
    a.add(4,4,3);
    a.add_register_carry(0,19,5);
    a.move(19,0);
    a.add_extended(0,20,4);
    a.move(20,0);
    }
}
// The guest renderer uses a pipe as the visible placeholder for an ASCII space.
void replace_spaces(Call& a){
    a.value(6,0);a.value(4,124);a.jump(0x1000c73c);
    for(;;){a.at(0x1000c73c);a.sign16(3,6);a.indexed_load<1>(5,15,3);a.sign8(0,5,true);if(a.equal())break;
        a.at(0x1000c728);a.sign8(0,5);a.compare(0,32);if(a.equal())a.indexed_store<1>(4,15,3);
        a.at(0x1000c738);a.offset(6,6,1);
    }
    a.value(3,1);
}
}
