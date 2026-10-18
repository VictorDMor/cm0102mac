#pragma once
#include "shortlist_assessment.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::shortlist_assessment::detail {
class Call final : public SemanticGuestCall {
    State& state; SemanticGuestObserver* observer;
    void step_at(std::uint32_t pc) { if(observer)observer->before_step(state,pc);state.step(pc);SemanticGuestCall::at(pc+4); }
public:
    Call(State& s,std::uint32_t entry,SemanticDependencyDispatch* d,SemanticGuestObserver* o):SemanticGuestCall(s,entry,d,o),state(s),observer(o){}
    void absolute_at(std::uint32_t pc,unsigned destination,unsigned source) {
        step_at(pc);state.wr<8>(0x4000+destination*16,state.r<8>(0x4000+source*16)&UINT64_C(0x7fffffffffffffff));
    }
    void negative_product_difference_single_at(std::uint32_t pc,unsigned destination,unsigned first,unsigned second,unsigned subtrahend) {
        step_at(pc);const auto x=float_value<8>(state.r<8>(0x4000+first*16));const auto y=float_value<8>(state.r<8>(0x4000+second*16));const auto z=float_value<8>(state.r<8>(0x4000+subtrahend*16));
        const auto product=x*y;const auto difference=product-z;const auto result=-double(float_value<4>(float_bits<4>(difference)));
        state.wr<8>(0x4000+destination*16,float_bits<8>(result));floating_result_status(result,std::isnan(x)||std::isnan(y)||std::isnan(z),true);
    }
};
void prepare_contribution_context(Call&);
void choose_contribution_qualifications(Call&);
void calculate_contribution_threshold(Call&);
void adjust_for_existing_interest(Call&);
void choose_contribution_coefficients(Call&);
void calculate_contribution_baseline(Call&);
void apply_staff_value_curve(Call&);
void apply_contribution_country_scales(Call&);
void finish_contribution_value(Call&);
void prepare_interest_context(Call&);
void select_profile_context_scale(Call&);
void apply_profile_context_scale(Call&);
void adjust_interest_relationships(Call&);
void compare_interest_country_context(Call&);
void compare_linked_club_context(Call&);
void calculate_linked_club_scale(Call&);
void apply_linked_club_scale(Call&);
void apply_club_pair_scale(Call&);
void adjust_interest_club_services(Call&);
void adjust_interest_contract_preferences(Call&);
void check_additional_country_scale(Call&);
void adjust_interest_request_options(Call&);
void calculate_interest_contribution(Call&);
void calculate_interest_financial_context(Call&);
void compare_interest_country_fields(Call&);
void calculate_country_field_difference(Call&);
void adjust_interest_contract_flags(Call&);
void compare_interest_reference_values(Call&);
void calculate_primary_reference_difference(Call&);
void choose_interest_reference_scale(Call&);
void calculate_lower_reference_scale(Call&);
void limit_interest_reference_scale(Call&);
void calculate_fallback_reference_difference(Call&);
void finish_interest_score(Call&);
}
