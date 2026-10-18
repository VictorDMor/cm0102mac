#pragma once
#include "money_configuration.hpp"
#include "semantic_guest_call.hpp"
namespace cm::aot::money_configuration::detail {
using Call=SemanticGuestCall;
void apply_profile(Call&,std::uint32_t destination);
// The five suffix buffers are cached before the first copy. Keep their writes
// ordered: ordinary units, thousands, millions, billions, then trillions.
inline void copy_suffixes(Call& a,std::int32_t units,std::int32_t thousands,std::int32_t millions,std::int32_t billions,std::int32_t trillions){
    a.move(3,28);a.offset(4,30,units);a.call(0x1077dbf0);
    a.move(3,27);a.offset(4,30,thousands);a.call(0x1077dbf0);
    a.move(3,26);a.offset(4,30,millions);a.call(0x1077dbf0);
    a.move(3,25);a.offset(4,30,billions);a.call(0x1077dbf0);
    a.move(3,24);a.offset(4,30,trillions);a.call(0x1077dbf0);
}
inline void shared_separator(Call& a,unsigned character){
    a.value(4,character);a.value(0,0);a.store<1>(2,-0x3aff,4);a.value(3,1);
    a.store<1>(2,-0x3b00,4);a.store<1>(2,-0x3b01,0);a.store<1>(2,-0x3b02,31);
}
inline void distinct_separators(Call& a,unsigned grouping,unsigned decimal,unsigned four_digit){
    a.value(3,grouping);a.value(4,decimal);a.value(0,four_digit);a.store<1>(2,-0x3aff,3);a.value(3,1);
    a.store<1>(2,-0x3b00,4);a.store<1>(2,-0x3b01,0);a.store<1>(2,-0x3b02,31);
}
}
