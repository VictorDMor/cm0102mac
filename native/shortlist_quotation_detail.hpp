#pragma once
#include "shortlist_quotation.hpp"
#include "semantic_guest_call.hpp"

namespace cm::aot::shortlist_quotation::detail {
class Call final : public SemanticGuestCall {
    State& state; SemanticGuestObserver* observer;
    void step_at(std::uint32_t pc) { if(observer)observer->before_step(state,pc);state.step(pc);SemanticGuestCall::at(pc+4); }
public:
    Call(State& s,std::uint32_t entry,SemanticDependencyDispatch* d,SemanticGuestObserver* o):SemanticGuestCall(s,entry,d,o),state(s),observer(o){}
    void divide_record_at(std::uint32_t pc,unsigned destination,unsigned first,unsigned second) {
        step_at(pc);const auto numerator=r(first),denominator=r(second);
        const auto overflow=denominator==0||(denominator==UINT32_MAX&&numerator==0x80000000u);
        state.wr<1>(1025,overflow);state.wr<1>(1024,state.r<1>(1024)||overflow);
        const auto result=divide_signed<4>(numerator,denominator);state.wr<4>(destination*4,result);
        const auto value=std::bit_cast<std::int32_t>(std::uint32_t(result));
        state.wr<1>(2304,(state.r<1>(2304)&0xf0)|(value<0?8:value>0?4:2)|(state.r<1>(1024)&1));
    }
    void product_difference_single_at(std::uint32_t pc,unsigned destination,unsigned first,unsigned second,unsigned subtrahend) {
        step_at(pc);const auto x=float_value<8>(state.r<8>(0x4000+first*16));const auto y=float_value<8>(state.r<8>(0x4000+second*16));const auto z=float_value<8>(state.r<8>(0x4000+subtrahend*16));
        const auto product=x*y;const auto difference=product-z;const auto result=double(float_value<4>(float_bits<4>(difference)));
        state.wr<8>(0x4000+destination*16,float_bits<8>(result));floating_result_status(result,std::isnan(x)||std::isnan(y)||std::isnan(z),true);
    }
    void negative_product_difference_single_at(std::uint32_t pc,unsigned destination,unsigned first,unsigned second,unsigned subtrahend) {
        step_at(pc);const auto x=float_value<8>(state.r<8>(0x4000+first*16));const auto y=float_value<8>(state.r<8>(0x4000+second*16));const auto z=float_value<8>(state.r<8>(0x4000+subtrahend*16));
        const auto product=x*y;const auto difference=product-z;const auto result=-double(float_value<4>(float_bits<4>(difference)));
        state.wr<8>(0x4000+destination*16,float_bits<8>(result));floating_result_status(result,std::isnan(x)||std::isnan(y)||std::isnan(z),true);
    }
};
void prepare_candidate_context(Call&);
void calculate_personal_context_factor(Call&);
void compare_context_country(Call&);
void compare_linked_competition(Call&);
void calculate_cross_competition_factor(Call&);
void apply_context_factor_and_reasons(Call&);
void check_linked_country_reason(Call&);
void adjust_context_club_services(Call&);
void apply_primary_category_fallback(Call&);
void select_secondary_category_scale(Call&);
void apply_secondary_category_fallback(Call&);
void adjust_context_history_and_quote(Call&);
void prepare_reference_quote(Call&);
void choose_profile_quote_baseline(Call&);
void choose_profile_quote_category(Call&);
void apply_quote_mode(Call&);
void check_quote_category_threshold(Call&);
void check_quote_profile_floor(Call&);
void compare_primary_profile_thresholds(Call&);
void use_primary_profile_quote(Call&);
void check_secondary_profile_thresholds(Call&);
void use_secondary_profile_quote(Call&);
void use_specialist_profile_quote(Call&);
void check_specialist_profile_thresholds(Call&);
void use_fallback_profile_quote(Call&);
void adjust_quote_club_context(Call&);
void check_quote_home_country(Call&);
void apply_quote_country_rules(Call&);
void check_quote_local_connection(Call&);
void compress_local_connection_quote(Call&);
void check_quote_country_band(Call&);
void compress_high_country_quote(Call&);
void calculate_first_country_scale(Call&);
void apply_first_country_scale(Call&);
void choose_first_country_cap(Call&);
void calculate_second_country_scale(Call&);
void apply_second_country_scale(Call&);
void calculate_third_country_scale(Call&);
void apply_third_country_scale(Call&);
void calculate_final_country_scale(Call&);
void apply_final_country_scale(Call&);
void finish_quote_contract_limits(Call&);
}
