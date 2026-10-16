#include "ball_contact.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::ball_contact {
namespace {
using Call=SemanticGuestCall;
struct ActorRegisters {unsigned sample_index,sample_base,player_index,player_offset,player_base;};
struct OpponentRegisters {unsigned index,zeroes,opposite,offset,base;};
void actor_arguments(Call& a,unsigned kind,ActorRegisters r) {
    a.move(3,29);a.value(4,kind);a.sign8(r.sample_index,31);a.offset(r.sample_base,1,0x38);a.indexed_load<1>(5,r.sample_base,r.sample_index);
    a.sign8(r.player_index,31);a.mask(r.player_offset,r.player_index,0xfffffffc,false,2);a.offset(r.player_base,1,0x48);a.indexed_load(6,r.player_base,r.player_offset);
}
void opponent(Call& a,OpponentRegisters r,unsigned destination) {
    a.sign8(r.index,31);a.leading_zeroes(r.zeroes,r.index);a.mask(r.opposite,r.zeroes,0x07ffffff,false,27);
    a.mask(r.offset,r.opposite,0xfffffffc,false,2);a.offset(r.base,1,0x48);a.indexed_load(destination,r.base,r.offset);
}
void current_reaction(Call& a,unsigned kind,ActorRegisters r,bool jump=true,std::uint32_t join=description_next_phase) {
    actor_arguments(a,kind,r);a.value(7,0);a.call(0x101f6940);if(jump)a.jump(join);
}
void paired_reaction(Call& a,unsigned kind,ActorRegisters actor,OpponentRegisters other,bool jump=true,std::uint32_t join=description_next_phase) {
    actor_arguments(a,kind,actor);opponent(a,other,7);a.call(0x101f6940);if(jump)a.jump(join);
}
std::uint32_t kind_five_reaction(Call& a) {
    a.sign8(7,31);a.offset(6,1,0x3a);a.indexed_load<1>(5,6,7);a.sign8(0,5);a.compare(0,9);
    if(a.equal())return 0x101f5fa4;
    if(a.at_least()){a.at(0x101f5f98);a.compare(0,11);if(a.at_least())return 0x101f6034;a.jump(0x101f6004);return 0x101f6004;}
    a.compare(0,6);if(a.equal())return 0x101f5fd4;a.jump(0x101f6034);return 0x101f6034;
}
std::uint32_t random_kind_five_reaction(Call& a) {
    a.value(3,5);a.call(0x101d3c90);a.move(0,3);a.compare(0,3);if(a.equal())return 0x101f6094;
    if(a.at_least()){a.at(0x101f6058);a.compare(0,5);if(a.at_least())return description_next_phase;a.jump(0x101f60c4);return 0x101f60c4;}
    a.compare(0,0);if(a.at_least())return 0x101f6064;a.jump(description_next_phase);return description_next_phase;
}
std::uint32_t unpaired_reaction(Call& a) {
    a.sign8(0,31);a.offset(7,1,0x3a);a.indexed_load<1>(6,7,0);a.sign8(5,6);a.compare(5,3);if(a.equal())return 0x101f61f4;
    if(a.at_least()){a.at(0x101f61b4);a.compare(5,5);if(a.equal())return 0x101f6284;if(a.at_least())return 0x101f62b4;a.jump(0x101f6224);return 0x101f6224;}
    a.compare(5,1);if(a.equal())return 0x101f61c4;if(a.at_least())return 0x101f6254;a.jump(0x101f62b4);return 0x101f62b4;
}
std::uint32_t random_unpaired_reaction(Call& a) {
    a.value(3,11);a.call(0x101d3c90);a.compare(3,6);
    if(a.at_least()){a.at(0x101f62dc);a.compare(3,8);if(a.equal())return 0x101f63b0;if(a.less())return 0x101f6384;a.compare(3,11);if(a.at_least())return description_next_phase;a.jump(0x101f62f4);return 0x101f62f4;}
    a.compare(3,0);if(a.equal())return 0x101f62f4;if(a.less())return description_next_phase;a.compare(3,4);if(a.at_least())return 0x101f6354;a.jump(0x101f6324);return 0x101f6324;
}
std::uint32_t paired_style(Call& a) {
    opponent(a,{3,4,5,0,3},0);a.compare(0,0,false);if(a.equal())return description_next_phase;
    a.sign8(0,31);a.offset(7,1,0x3a);a.indexed_load<1>(4,7,0);a.sign8(0,4);a.compare(0,9,false);if(a.positive())return 0x101f6690;
    a.load(3,2,-0x52e4);a.mask(0,0,0xfffffffc,false,2);a.indexed_load(3,3,0);a.count_destination(3);return a.count_branch();
}
std::uint32_t random_paired_style(Call& a) {
    a.value(3,9);a.call(0x101d3c90);a.compare(3,8,false);if(a.positive())return description_next_phase;
    a.load(4,2,-0x52e8);a.mask(3,3,0xfffffffc,false,2);a.indexed_load(4,4,3);a.count_destination(4);return a.count_branch();
}
}
void select_description_reaction(State& state,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    Call a(state,description_reaction_phase,dependencies,observer);auto target=state.pc;
    if(target==description_reaction_phase) {
        a.sign8(7,31);a.mask(0,7,0xfffffffe,false,1);a.offset(7,1,0x3c);a.indexed_load<2>(0,7,0,true);a.compare(0,5);
        target=a.equal()?kind_five_reaction(a):0x101f60f4;
    }
    // These are semantic branches and live table destinations, not an instruction loop.
    if(target==0x101f6034){a.at(target);target=random_kind_five_reaction(a);}
    if(target==0x101f60f4) {
        a.at(target);a.load<1>(6,29,0x791f);a.sign8(0,6);a.compare(0,4);
        target=0x101f6168;
        if(a.equal()){opponent(a,{0,0,0,7,6},5);a.compare(5,0,false);if(!a.equal())target=0x101f6124;}
    }
    if(target==0x101f6168){a.at(target);opponent(a,{0,6,0,0,7},0);a.compare(0,0,false);target=a.equal()?unpaired_reaction(a):0x101f63e0;}
    if(target==0x101f62b4){a.at(target);target=random_unpaired_reaction(a);}
    if(target==0x101f63e0){a.at(target);target=paired_style(a);}
    if(target==0x101f6690){a.at(target);target=random_paired_style(a);}
    a.at(target);auto join=description_next_phase;
    switch(target) {
    case 0x101f5fa4:current_reaction(a,9,{0,6,24,0,7},true,join=description_return_phase);break;
    case 0x101f5fd4:current_reaction(a,6,{0,7,0,7,6},true,join=description_return_phase);break;
    case 0x101f6004:current_reaction(a,10,{0,6,0,0,7},true,join=description_return_phase);break;
    case 0x101f6064:current_reaction(a,9,{7,5,6,0,7});break;
    case 0x101f6094:current_reaction(a,6,{6,7,0,0,8});break;
    case 0x101f60c4:current_reaction(a,10,{7,5,6,0,23});break;
    case 0x101f6124:paired_reaction(a,11,{6,7,6,0,7},{0,7,8,0,7});break;
    case 0x101f61c4:current_reaction(a,1,{6,7,6,10,9},true,join=description_return_phase);break;
    case 0x101f61f4:current_reaction(a,3,{0,8,7,0,6},true,join=description_return_phase);break;
    case 0x101f6224:current_reaction(a,4,{7,6,10,9,7},true,join=description_return_phase);break;
    case 0x101f6254:current_reaction(a,2,{0,7,8,7,6},true,join=description_return_phase);break;
    case 0x101f6284:current_reaction(a,5,{0,8,7,0,6},true,join=description_return_phase);break;
    case 0x101f62f4:current_reaction(a,1,{0,5,6,7,6});break;
    case 0x101f6324:current_reaction(a,3,{0,5,0,0,9});break;
    case 0x101f6354:current_reaction(a,4,{8,5,0,0,7});break;
    case 0x101f6384:
        // The original kind-two fallback deliberately falls through to kind five.
        current_reaction(a,2,{6,5,0,8,7},false);[[fallthrough]];
    case 0x101f63b0:current_reaction(a,5,{0,5,0,0,6});break;
    case 0x101f642c:paired_reaction(a,1,{0,5,6,0,9},{8,0,0,0,7},true,join=description_return_phase);break;
    case 0x101f6470:paired_reaction(a,3,{6,5,0,7,6},{0,0,0,0,7},true,join=description_return_phase);break;
    case 0x101f64b4:paired_reaction(a,4,{6,5,10,9,6},{0,0,0,8,7},true,join=description_return_phase);break;
    case 0x101f64f8:paired_reaction(a,2,{0,5,9,0,6},{0,7,0,8,7},true,join=description_return_phase);break;
    case 0x101f653c:paired_reaction(a,5,{8,5,0,0,6},{0,7,7,0,9},true,join=description_return_phase);break;
    case 0x101f6580:paired_reaction(a,6,{8,5,0,0,6},{7,0,7,0,7},true,join=description_return_phase);break;
    case 0x101f65c4:paired_reaction(a,7,{0,5,6,0,7},{0,7,0,8,7},true,join=description_return_phase);break;
    case 0x101f6608:paired_reaction(a,8,{0,5,6,7,6},{8,7,7,0,7},true,join=description_return_phase);break;
    case 0x101f664c:paired_reaction(a,9,{6,5,24,0,6},{7,0,0,8,7},true,join=description_return_phase);break;
    case 0x101f66b4:paired_reaction(a,1,{23,5,0,7,6},{7,7,0,0,7});break;
    case 0x101f66f8:paired_reaction(a,2,{6,5,0,0,6},{24,7,0,0,7});break;
    case 0x101f673c:paired_reaction(a,3,{0,7,6,0,6},{0,7,0,0,7});break;
    case 0x101f6780:paired_reaction(a,4,{5,6,0,7,6},{7,0,0,8,7});break;
    case 0x101f67c4:paired_reaction(a,5,{5,6,0,23,6},{0,0,0,0,7});break;
    case 0x101f6808:paired_reaction(a,6,{6,5,6,7,6},{0,0,7,8,7});break;
    case 0x101f684c:paired_reaction(a,7,{0,7,0,0,6},{0,0,7,0,7});break;
    case 0x101f6890:paired_reaction(a,8,{6,5,6,7,6},{10,0,9,8,7});break;
    case 0x101f68d4:paired_reaction(a,9,{0,6,7,6,10},{9,0,7,7,8},false);break;
    default:state.pc=target;return;
    }
    state.pc=join;
}
}
