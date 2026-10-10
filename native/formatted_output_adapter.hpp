#pragma once
#include "aot_runtime.hpp"
namespace cm::aot {
inline constexpr std::array<std::uint32_t,15> formatted_output_entries{
    0x10778db0,0x10779620,0x10779880,0x10779b70,0x10779ed0,
    0x10779ff0,0x1077a000,0x1077a690,0x1077b4c0,0x1077b520,
    0x1077b590,0x1077b610,0x1077b680,0x1077b6f0,0x1077b720};
struct FormattedOutputBoundaryObserver {
    virtual ~FormattedOutputBoundaryObserver()=default;
    virtual void enter(const State&,std::uint32_t,bool original)=0;
    virtual void leave(State&,std::uint32_t,bool original)=0;
};
bool try_modern_formatted_output(State&,std::uint32_t entry);
bool try_modern_formatted_output_shared_tail(State&);
void reset_formatted_output_diagnostics(State&);
std::string formatted_output_call_summary(const State&);
}
