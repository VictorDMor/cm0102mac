#pragma once
#include "aot_runtime.hpp"

namespace cm::aot::stdlib_core {
// Accounting is owned by the dispatcher. Components can run with no accounting
// policy; production uses exact search phase charges around guest callbacks.
struct SearchExecution {
    std::uint64_t budget_resumes = 0;
    void (*resume_original)(State&, std::uint32_t caller) = nullptr;
};
void absolute_word(State&);
void absolute_long_word(State&);
void floor_remainder(State&, bool account = false);
void binary_search(State&, SearchExecution* = nullptr);
}
