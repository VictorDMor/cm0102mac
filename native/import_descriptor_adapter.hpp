#pragma once
#include "import_descriptor.hpp"

namespace cm::aot {
enum class ImportDescriptorOutcome { tailed, fault, budget_stop, context_transfer };
struct ImportDescriptorBoundaryObserver {
    virtual ~ImportDescriptorBoundaryObserver() = default;
    virtual void enter(const State&, std::uint32_t, bool original) = 0;
    virtual void leave(State&, std::uint32_t, bool original, ImportDescriptorOutcome) = 0;
};
bool try_modern_import_descriptor(State&, std::uint32_t);
void reset_import_descriptor_diagnostics(State&);
std::string import_descriptor_call_summary(const State&);
}
