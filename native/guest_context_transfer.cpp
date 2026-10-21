#include "guest_context_transfer.hpp"
#include "aot_runtime.hpp"
#include <algorithm>

namespace cm::aot {
// Each State keeps its own scope chains, innermost first; scopes are stack
// objects, so construction and destruction stay last-in, first-out.
GuestDispatchScope* guest_dispatch_scope(State& state) noexcept { return state.dispatch_scope; }
GuestDispatchScope::GuestDispatchScope(State& state) noexcept : GuestDispatchScope(state, return_sentinel) {}
GuestDispatchScope::GuestDispatchScope(State& state, std::uint32_t return_pc) noexcept : state_(&state),
    entry_stack_(std::uint32_t(state.r<4>(4))), return_pc_(return_pc), previous_(state.dispatch_scope),
    previous_state_(state.dispatch_scope), caller_(state.native_scope) {
    state.dispatch_scope = this;
}
GuestDispatchScope::~GuestDispatchScope() { state_->dispatch_scope = previous_; }
bool GuestDispatchScope::resumes(const GuestContextTransfer& transfer) const noexcept {
    return transfer.state == state_ && transfer.destination == this;
}
GuestNativeScope::GuestNativeScope(State& state) noexcept : GuestNativeScope(state, state.pc) {}
GuestNativeScope::GuestNativeScope(State& state, std::uint32_t function) noexcept : state_(&state),
    function_(function), frame_(std::uint32_t(state.r<4>(4))), previous_(state.native_scope),
    dispatcher_(state.dispatch_scope) { state.native_scope = this; }
GuestNativeScope::~GuestNativeScope() { state_->native_scope = previous_; }
// A capture without a State is inert: it is never linked or found.
GuestScalarContextCapture::GuestScalarContextCapture(State* state) noexcept
    : state_(state), previous_(state ? state->scalar_capture : nullptr) { if (state) state->scalar_capture = this; }
GuestScalarContextCapture::~GuestScalarContextCapture() { if (state_) state_->scalar_capture = previous_; }
void scalar_context_capture_link(State& state, std::uint32_t value) noexcept {
    if (auto* capture = state.scalar_capture) { capture->link_ = value; capture->has_link_ = true; }
}
void scalar_context_capture_stack(State& state, std::uint32_t value) noexcept {
    if (auto* capture = state.scalar_capture) { capture->stack_ = value; capture->has_stack_ = true; }
}
void GuestScalarContextCapture::commit() {
    if (!state_ || !has_link_ || !has_stack_) return;
    auto* owner = guest_dispatch_scope(*state_);
    if (!owner) return;
    // Capturing the return of a semantic call requires abandoning that C++
    // caller when it is later resumed. A generated callee in the same frame
    // has a different continuation and remains in the current dispatcher.
    if (owner->caller_ && owner->caller_->dispatcher_ &&
        owner->return_pc_ == link_ && owner->caller_->frame_ == stack_)
        owner = owner->caller_->dispatcher_;
    const std::array target{link_, stack_};
    if (std::find(owner->scalar_contexts_.begin(), owner->scalar_contexts_.end(), target) == owner->scalar_contexts_.end())
        owner->scalar_contexts_.push_back(target);
}
void complete_scalar_context_transfer(State& state, std::uint64_t& unknown) {
    auto* current = guest_dispatch_scope(state);
    if (!current) return;
    const std::array target{std::uint32_t(state.r<4>(4128)), std::uint32_t(state.r<4>(4))};
    for (auto* owner = current; owner; owner = owner->previous_state_)
        if (std::find(owner->scalar_contexts_.begin(), owner->scalar_contexts_.end(), target) != owner->scalar_contexts_.end())
            throw GuestContextTransfer{&state, owner};
    // Foreign or manually constructed contexts retain the architectural stack
    // fallback, counted separately from a captured live continuation.
    ++unknown;
    auto* destination = current;
    while (destination->previous_state_ && target[1] >= destination->entry_stack_)
        destination = destination->previous_state_;
    throw GuestContextTransfer{&state, destination};
}
void record_context_function(State& state, std::uint32_t function) noexcept {
    if (auto* scope = guest_dispatch_scope(state)) scope->function_ = function;
}
void record_context_frame(State& state, std::uint32_t frame) noexcept {
    if (auto* scope = guest_dispatch_scope(state)) scope->frame_ = frame;
}
void complete_context_transfer(State& state) {
    auto* current = guest_dispatch_scope(state);
    if (!current) return;
    const auto function = current->function_, frame = current->frame_;
    current->function_ = current->frame_ = 0;
    // A handler in an active semantic function abandons that C++ invocation.
    // Frame identity distinguishes recursive invocations of the same function.
    if (function) for (auto* scope = state.native_scope; scope; scope = scope->previous_)
        if (scope->function_ == function && scope->frame_ == frame)
            throw GuestContextTransfer{&state, scope->dispatcher_};
    // Generated callees can share their semantic caller's frame. A known,
    // different function in that frame is still inside this callback dispatch;
    // its handler must finish before the surviving semantic caller resumes.
    if (function && current->caller_ && current->caller_->allocated_ &&
        current->caller_->frame_ == frame && current->caller_->function_ != function)
        throw GuestContextTransfer{&state, current};
    // Use the unwound frame, not restored r1: the original can retain a deeper
    // stack address while replacing its backchain with the recovered caller's.
    auto* destination = current;
    while (destination->previous_state_ && frame >= destination->entry_stack_)
        destination = destination->previous_state_;
    throw GuestContextTransfer{&state, destination};
}
}
