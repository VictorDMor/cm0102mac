#pragma once
#include "transfer_synthesis.hpp"
#include "transfer_record_detail.hpp"

namespace cm::aot::transfer_synthesis::detail {
class Call final : public SemanticGuestCall {
    State& state; SemanticGuestObserver* observer;
    void step_at(std::uint32_t pc){if(observer)observer->before_step(state,pc);state.step(pc);SemanticGuestCall::at(pc+4);}
    void record(std::uint32_t value){const auto v=std::bit_cast<std::int32_t>(value);state.wr<1>(2304,(state.r<1>(2304)&0xf0)|(v<0?8:v>0?4:2)|(state.r<1>(1024)&1));}
public:
    Call(State& s,std::uint32_t entry,SemanticDependencyDispatch* d,SemanticGuestObserver* o):SemanticGuestCall(s,entry,d,o),state(s),observer(o){}
    void subtract_carry_at(std::uint32_t pc,unsigned destination,unsigned first,unsigned second){step_at(pc);const auto x=r(first),y=r(second);state.wr<1>(1027,x<=y);state.wr<4>(destination*4,y-x);}
    void exclusive_registers_at(std::uint32_t pc,unsigned destination,unsigned first,unsigned second){step_at(pc);state.wr<4>(destination*4,r(first)^r(second));}
    void negate_record_at(std::uint32_t pc,unsigned destination,unsigned source){step_at(pc);const auto value=0u-r(source);state.wr<4>(destination*4,value);record(value);}
    void compare_less_or_equal_bit_at(std::uint32_t pc){step_at(pc);const auto cr=state.r<1>(2304);state.wr<1>(2304,(cr&~2u)|((cr&10u)?2u:0u));}
};
using transfer_records::detail::diagnostic;
enum class Route { failed, finalize, price };
Route prepare_offer(Call&);
Route select_base_and_period(Call&);
bool constrain_payment_distribution(Call&);
bool refine_offer_value(Call&);
bool price_offer(Call&);
bool finalize_offer(Call&);

inline void advance_pool(Call& a,std::uint32_t pc){
    a.at(pc);a.load(3,31,0);a.load(0,30,0);a.offset(3,3,2);a.compare_registers(3,0,false);a.store(31,0,3);
    if(!a.nonpositive()){a.store(31,0,29);a.call(0x1077b770);a.mask(0,3,0xffff);a.store(28,0,0);}
    a.at(pc+0x28);
}
}
