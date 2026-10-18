#pragma once
#include "shortlist_policy.hpp"
#include "semantic_guest_call.hpp"
namespace cm::aot::shortlist_policy::detail {
class Call final:public SemanticGuestCall {
    State& state;SemanticGuestObserver* observer;
    void step_at(std::uint32_t pc){if(observer)observer->before_step(state,pc);state.step(pc);SemanticGuestCall::at(pc+4);}
public:
    Call(State& s,std::uint32_t root,SemanticDependencyDispatch* d,SemanticGuestObserver* o):SemanticGuestCall(s,root,d,o),state(s),observer(o){}
    void subtract_carry_at(std::uint32_t pc,unsigned destination,unsigned first,unsigned second){
        step_at(pc);const auto x=r(first),y=r(second);state.wr<1>(1027,y>=x);state.wr<4>(destination*4,y-x);
    }
    void negate_record_at(std::uint32_t pc,unsigned destination,unsigned source){
        step_at(pc);const auto value=0u-r(source);state.wr<4>(destination*4,value);const auto sign=std::bit_cast<std::int32_t>(value);
        state.wr<1>(2304,(state.r<1>(2304)&0xf0)|(sign<0?8:sign>0?4:2)|(state.r<1>(1024)&1));
    }
};

void prepare_offer_policy(Call&);
void check_offer_contract_mode(Call&);
void check_requested_offer_mode(Call&);
void check_requested_profile_limits(Call&);
void compare_requested_profile_limits(Call&);
void reject_requested_offer(Call&);
void resolve_existing_offer_value(Call&);
void resolve_offer_value_pair(Call&);
void compare_offer_value_limits(Call&);
void compare_offer_value_ratio(Call&);
void prepare_country_policy(Call&);
void prepare_row_contract_gate(Call&);
}
