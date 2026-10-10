#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
inline constexpr std::array<std::uint32_t,27> allocator_entries{
    0x107740f0,0x10774190,0x10774270,0x10774360,0x107743e0,0x10774430,
    0x107744d0,0x10774570,0x10774630,0x10774680,0x107746d0,0x10774760,
    0x10774840,0x10774920,0x107749b0,0x10774ae0,0x10774b10,0x10774b60,
    0x10774d20,0x10774e80,0x10774ee0,0x10774f40,0x107750e0,0x10775130,
    0x10775170,0x107751b0,0x10775200};
// Private, process-local observations; neither heap metadata nor save data.
struct AllocatorBoundaryObserver {
    virtual ~AllocatorBoundaryObserver() = default;
    virtual void enter(const State&,std::uint32_t,bool) = 0;
    virtual void leave(State&,std::uint32_t,bool) = 0;
    virtual void import_enter(const State&,const char*) {}
    virtual void import_leave(const State&,const char*) {}
};
bool try_modern_allocator(State&,std::uint32_t entry);
void reset_allocator_diagnostics(State&);
std::string allocator_call_summary(const State&);
}
