#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t,9> stream_io_entries{
    0x10775ad0,0x10775c10,0x10775c80,0x10775d80,0x10775fe0,
    0x10776000,0x10776360,0x10776380,0x1077fa50};
struct StreamIoBoundaryObserver {
    virtual ~StreamIoBoundaryObserver() = default;
    virtual void enter(const State&,std::uint32_t,bool) = 0;
    virtual void leave(State&,std::uint32_t,bool) = 0;
    virtual void import_enter(const State&,const char*) {}
    virtual void import_leave(const State&,const char*) {}
};
bool try_modern_stream_io(State&,std::uint32_t);
void reset_stream_io_diagnostics(State&);
std::string stream_io_call_summary(const State&);
}
