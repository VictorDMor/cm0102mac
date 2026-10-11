#pragma once
#include <cstdint>

namespace cm::aot {
struct State;
class GuestDispatchScope;
class GuestNativeScope;
// A host control transfer is deliberately separate from guest faults and host
// std::exception handlers. No C++ guest-call epilogue runs while it propagates.
struct GuestContextTransfer {
    State* state;
    GuestDispatchScope* destination;
};
class GuestDispatchScope {
public:
    explicit GuestDispatchScope(State&) noexcept;
    ~GuestDispatchScope();
    GuestDispatchScope(const GuestDispatchScope&) = delete;
    GuestDispatchScope& operator=(const GuestDispatchScope&) = delete;
    bool resumes(const GuestContextTransfer& transfer) const noexcept;
private:
    State* state_;
    std::uint32_t entry_stack_, function_ = 0, frame_ = 0;
    GuestDispatchScope* previous_;
    GuestDispatchScope* previous_state_;
    GuestNativeScope* caller_;
    friend class GuestNativeScope;
    friend void record_context_function(State&, std::uint32_t) noexcept;
    friend void record_context_frame(State&, std::uint32_t) noexcept;
    friend void complete_context_transfer(State&);
    friend GuestDispatchScope* guest_dispatch_scope(State&) noexcept;
};
class GuestNativeScope {
public:
    explicit GuestNativeScope(State&) noexcept;
    GuestNativeScope(State&, std::uint32_t function) noexcept;
    ~GuestNativeScope();
    GuestNativeScope(const GuestNativeScope&) = delete;
    GuestNativeScope& operator=(const GuestNativeScope&) = delete;
    void frame(std::uint32_t address) noexcept { frame_ = address; allocated_ = true; }
private:
    State* state_;
    std::uint32_t function_, frame_;
    bool allocated_ = false;
    GuestNativeScope* previous_;
    GuestDispatchScope* dispatcher_;
    friend class GuestDispatchScope;
    friend void complete_context_transfer(State&);
};
// Record values already loaded by the guest operations; never add memory reads
// for host routing. Calls outside a dispatch scope leave architectural state
// untouched and retain the component helpers' explicit destination boundary.
void record_context_function(State&, std::uint32_t) noexcept;
void record_context_frame(State&, std::uint32_t) noexcept;
void complete_context_transfer(State&);
}
