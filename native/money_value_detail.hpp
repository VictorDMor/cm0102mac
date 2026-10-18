#pragma once
#include "money_value.hpp"
#include "semantic_guest_call.hpp"
namespace cm::aot::money_value::detail {
using Call=SemanticGuestCall;
// Align the scaled signed pair using the original half-quantum construction,
// signed quotient and wrapped low64-bit product. Register assignments differ
// for the conversion and the two editing callers.
inline void align_scaled_pair(Call& a,unsigned high,unsigned low,unsigned flags,unsigned quantum_high,unsigned quantum_low,bool separate_result=false) {
    a.move(5,flags);a.call(amount_quantum_entry);a.move(quantum_high,3);a.move(quantum_low,4);
    a.move(0,quantum_high);a.mask(3,4,0xffffffff,false,31);a.insert_mask(0,quantum_low,1,0);
    a.move(5,quantum_high);a.insert_mask(3,quantum_high,0x80000000,31);a.move(6,quantum_low);
    a.shift_right(0,0,1);a.add_carry(3,3);a.add_carry(0,0);
    a.add_register_carry(4,low,3);a.add_extended(3,high,0);a.call(0x10773960);
    a.unsigned_multiply_high(0,quantum_low,4);a.product(5,quantum_high,4);a.product(separate_result?6:low,quantum_low,4);
    a.add(4,0,5);a.product(0,quantum_low,3);
    if(separate_result){a.move(low,6);a.add(0,4,0);a.move(high,0);}else a.add(high,4,0);
}
void report_invalid_preset(Call&,bool standard);
}
