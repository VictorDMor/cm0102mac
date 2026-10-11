#include "guest_context_transfer.hpp"
#include "aot_runtime.hpp"

namespace cm::aot {
namespace {
thread_local GuestDispatchScope* dispatch_scope = nullptr;
thread_local GuestNativeScope* native_scope = nullptr;
}
GuestDispatchScope* guest_dispatch_scope(State& state) noexcept {
    for (auto* scope = dispatch_scope; scope; scope = scope->previous_)
        if (scope->state_ == &state) return scope;
    return nullptr;
}
GuestDispatchScope::GuestDispatchScope(State& state) noexcept : state_(&state),
    entry_stack_(std::uint32_t(state.r<4>(4))), previous_(dispatch_scope),
    previous_state_(guest_dispatch_scope(state)), caller_(native_scope) {
    while (caller_ && caller_->state_ != &state) caller_ = caller_->previous_;
    dispatch_scope = this;
}
GuestDispatchScope::~GuestDispatchScope() { dispatch_scope = previous_; }
bool GuestDispatchScope::resumes(const GuestContextTransfer& transfer) const noexcept {
    return transfer.state == state_ && transfer.destination == this;
}
GuestNativeScope::GuestNativeScope(State& state) noexcept : GuestNativeScope(state, state.pc) {}
GuestNativeScope::GuestNativeScope(State& state, std::uint32_t function) noexcept : state_(&state),
    function_(function), frame_(std::uint32_t(state.r<4>(4))), previous_(native_scope),
    dispatcher_(guest_dispatch_scope(state)) { native_scope = this; }
GuestNativeScope::~GuestNativeScope() { native_scope = previous_; }
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
    if (function) for (auto* scope = native_scope; scope; scope = scope->previous_)
        if (scope->state_ == &state && scope->function_ == function && scope->frame_ == frame)
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
