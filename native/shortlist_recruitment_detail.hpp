#pragma once
#include "semantic_guest_call.hpp"

namespace cm::aot::shortlist_recruitment::detail {
using Call=SemanticGuestCall;
inline void expand_role_qualifications(Call& a,std::uint32_t pc,unsigned destination) {
    a.at(pc);a.mask(0,3,2,true);a.move(destination,3);
    if(!a.equal()){a.flags(0,destination,4);a.sign16(destination,0);}
    // These are qualification implications retained from the original rules.
    constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,4> implications{{{8,16},{32,16},{128,256},{2048,1024}}};
    for(unsigned n=0;n<implications.size();++n){
        a.at(pc+20+n*16);a.mask(0,destination,implications[n].first,true);
        if(!a.equal()){a.flags(0,destination,implications[n].second);a.sign16(destination,0);}
    }
    a.at(pc+84);
}
inline void clamp_interest_ratio(Call& a,std::uint32_t pc,unsigned constants) {
    a.at(pc);a.floating_compare(4,0);
    if(!a.nonpositive())a.jump(pc+32);
    else{a.at(pc+12);a.floating_load(4,constants,0x140);a.floating_compare(4,0);if(!a.at_least())a.jump(pc+32);else{a.at(pc+28);a.floating_move(4,0);}}
    a.at(pc+32);a.floating_load(0,constants,0x80);a.floating_compare(4,0);
    if(!a.nonpositive()){a.floating_load(0,constants,0x140);a.floating_load(1,constants,0x138);a.floating_difference(0,4,0,true);a.floating_sum(4,1,0);}
    a.at(pc+60);
}
}
