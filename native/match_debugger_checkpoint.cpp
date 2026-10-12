#include "match_debugger_checkpoint.hpp"
#include <algorithm>

namespace cm::aot::match_debugger {
Checkpoint::Checkpoint(const State& state, const Checkpoint* previous)
    : registers_(state.registers), unique_(state.unique), allocated_(state.allocated), available_(state.available),
      pc_(state.pc), heap_end_(state.heap_end), executed_(state.executed), budget_(state.budget),
      native_calls_(state.native_calls), memory_error_(state.memory_error), platform_(state.platform) {
    for (const auto& [start, length] : state.memory.mapped_ranges()) {
        const auto end = std::uint64_t(start) + length;
        for (std::uint64_t address = start; address < end;) {
            const auto size = std::min(end - address, 4096 - address % 4096);
            auto bytes = state.memory.copy_out(std::uint32_t(address), std::size_t(size));
            const Page* prior = nullptr;
            if (previous) {
                const auto found = std::lower_bound(previous->pages_.begin(), previous->pages_.end(), address,
                    [](const Page& page, std::uint64_t value) { return page.address < value; });
                if (found != previous->pages_.end() && found->address == address && *found->bytes == bytes) prior = &*found;
            }
            pages_.push_back({std::uint32_t(address), prior ? prior->bytes : std::make_shared<const std::vector<std::uint8_t>>(std::move(bytes))});
            address += size;
        }
    }
}

std::unique_ptr<State> Checkpoint::restore() const {
    auto state = std::make_unique<State>();
    state->registers = registers_; state->unique = unique_; state->allocated = allocated_; state->available = available_;
    state->pc = pc_; state->heap_end = heap_end_; state->executed = executed_; state->budget = budget_;
    state->native_calls = native_calls_; state->memory_error = memory_error_; state->platform = platform_;
    for (const auto& page : pages_) {
        state->memory.map(page.address, page.bytes->size()); state->memory.copy_in(page.address, *page.bytes);
    }
    return state;
}

std::size_t Checkpoint::newly_owned_bytes(const Checkpoint* previous) const {
    std::size_t bytes = sizeof(*this) + pages_.size() * sizeof(Page);
    for (const auto& page : pages_) {
        bool shared = false;
        if (previous) {
            const auto found = std::lower_bound(previous->pages_.begin(), previous->pages_.end(), page.address,
                [](const Page& value, std::uint32_t address) { return value.address < address; });
            shared = found != previous->pages_.end() && found->address == page.address && found->bytes == page.bytes;
        }
        if (!shared) bytes += page.bytes->size();
    }
    return bytes;
}
}
