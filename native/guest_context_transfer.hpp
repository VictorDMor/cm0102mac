#pragma once
#include <cstdint>
#include <array>
#include <vector>

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
    GuestDispatchScope(State&, std::uint32_t return_pc) noexcept;
    ~GuestDispatchScope();
    GuestDispatchScope(const GuestDispatchScope&) = delete;
    GuestDispatchScope& operator=(const GuestDispatchScope&) = delete;
    bool resumes(const GuestContextTransfer& transfer) const noexcept;
    std::uint32_t return_pc() const noexcept { return return_pc_; }
private:
    State* state_;
    std::uint32_t entry_stack_, return_pc_, function_ = 0, frame_ = 0;
    std::vector<std::array<std::uint32_t, 2>> scalar_contexts_;
    GuestDispatchScope* previous_;
    GuestDispatchScope* previous_state_;
    GuestNativeScope* caller_;
    friend class GuestNativeScope;
    friend void record_context_function(State&, std::uint32_t) noexcept;
    friend void record_context_frame(State&, std::uint32_t) noexcept;
    friend void complete_context_transfer(State&);
    friend GuestDispatchScope* guest_dispatch_scope(State&) noexcept;
    friend class GuestScalarContextCapture;
    friend void complete_scalar_context_transfer(State&, std::uint64_t&);
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
    friend class GuestScalarContextCapture;
};
// Routing associations live only as long as their dispatch owner. Guest storage
// remains the original 248-byte buffer; copied buffers retain the same target.
// Values are observed before the existing stores, with no additional reads.
class GuestScalarContextCapture {
public:
    explicit GuestScalarContextCapture(State*) noexcept;
    ~GuestScalarContextCapture();
    GuestScalarContextCapture(const GuestScalarContextCapture&) = delete;
    GuestScalarContextCapture& operator=(const GuestScalarContextCapture&) = delete;
    void commit();
private:
    State* state_;
    GuestScalarContextCapture* previous_;
    std::uint32_t link_ = 0, stack_ = 0;
    bool has_link_ = false, has_stack_ = false;
    friend void scalar_context_capture_link(State&, std::uint32_t) noexcept;
    friend void scalar_context_capture_stack(State&, std::uint32_t) noexcept;
};
void scalar_context_capture_link(State&, std::uint32_t) noexcept;
void scalar_context_capture_stack(State&, std::uint32_t) noexcept;
void complete_scalar_context_transfer(State&, std::uint64_t& unknown);
// Record values already loaded by the guest operations; never add memory reads
// for host routing. Calls outside a dispatch scope leave architectural state
// untouched and retain the component helpers' explicit destination boundary.
void record_context_function(State&, std::uint32_t) noexcept;
void record_context_frame(State&, std::uint32_t) noexcept;
void complete_context_transfer(State&);
}
