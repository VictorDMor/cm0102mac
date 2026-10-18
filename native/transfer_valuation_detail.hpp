#pragma once
#include "transfer_valuation.hpp"
#include "transfer_record_detail.hpp"

namespace cm::aot::transfer_valuation::detail {
class Call final : public SemanticGuestCall {
    State& state;SemanticGuestObserver* observer;
    void step_at(std::uint32_t pc){if(observer)observer->before_step(state,pc);state.step(pc);SemanticGuestCall::at(pc+4);}
    void record(std::uint32_t value){const auto signed_result=std::bit_cast<std::int32_t>(value);state.wr<1>(2304,(state.r<1>(2304)&0xf0)|(signed_result<0?8:signed_result>0?4:2)|(state.r<1>(1024)&1));}
public:
    Call(State& s,std::uint32_t entry,SemanticDependencyDispatch* d,SemanticGuestObserver* o):SemanticGuestCall(s,entry,d,o),state(s),observer(o){}
    void absolute_at(std::uint32_t pc,unsigned destination,unsigned source){step_at(pc);state.wr<8>(0x4000+destination*16,state.r<8>(0x4000+source*16)&UINT64_C(0x7fffffffffffffff));}
    void offset_carry_at(std::uint32_t pc,unsigned destination,unsigned source,std::int32_t value){step_at(pc);const auto sum=std::uint64_t(r(source))+std::uint32_t(value);state.wr<1>(1027,sum>UINT32_MAX);state.wr<4>(destination*4,std::uint32_t(sum));}
    void subtract_record_at(std::uint32_t pc,unsigned destination,unsigned first,unsigned second){step_at(pc);const auto sum=std::uint64_t(~r(first))+r(second)+state.r<1>(1027);state.wr<1>(1027,sum>UINT32_MAX);const auto value=std::uint32_t(sum);state.wr<4>(destination*4,value);record(value);}
    void subtract_carry_at(std::uint32_t pc,unsigned destination,unsigned first,unsigned second){step_at(pc);const auto a=r(first),b=r(second);state.wr<1>(1027,a<=b);state.wr<4>(destination*4,b-a);}
    void negate_record_at(std::uint32_t pc,unsigned destination,unsigned source){step_at(pc);const auto value=0u-r(source);state.wr<4>(destination*4,value);record(value);}
};
using transfer_records::detail::diagnostic;
}
