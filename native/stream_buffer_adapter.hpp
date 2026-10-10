#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t,13> stream_buffer_entries{
    0x10775240,0x107752d0,0x107753b0,0x10775440,0x107754b0,
    0x107756f0,0x10775740,0x10775790,0x107757d0,0x107758a0,
    0x10775960,0x107766f0,0x107767b0};
struct StreamBufferBoundaryObserver {
    virtual ~StreamBufferBoundaryObserver() = default;
    virtual void enter(const State&,std::uint32_t,bool) = 0;
    virtual void leave(State&,std::uint32_t,bool) = 0;
    virtual void import_enter(const State&,const char*) {}
    virtual void import_leave(const State&,const char*) {}
};
bool try_modern_stream_buffer(State&,std::uint32_t);
void reset_stream_buffer_diagnostics(State&);
std::string stream_buffer_call_summary(const State&);
}
