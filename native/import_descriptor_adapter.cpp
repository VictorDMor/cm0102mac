#include "import_descriptor_adapter.hpp"
#include "guest_context_transfer.hpp"
#include "match_turbo.hpp"
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
struct OriginalBody {
    State& state;
    std::uint32_t previous;
    OriginalBody(State& s, std::uint32_t entry) : state(s), previous(s.import_descriptor_original_body) {
        state.import_descriptor_original_body = entry;
    }
    ~OriginalBody() { state.import_descriptor_original_body = previous; }
};
void original_tail(State& s, std::uint32_t entry) {
    OriginalBody selected(s, entry);
    if (!dispatch_page(s)) {
        ++s.import_descriptor_dispatch_escapes;
        throw std::runtime_error("Missing original import-descriptor body");
    }
    // Only this body spans a generated page boundary. Stop immediately after
    // its tail; the destination may itself equal an interior instruction.
    if (entry == 0x10780ff0) {
        if (s.pc != 0x10781000 || !dispatch_page(s)) {
            ++s.import_descriptor_dispatch_escapes;
            throw std::runtime_error("Missing original import-descriptor continuation");
        }
    }
}
void leave(State& s, unsigned index, bool original, ImportDescriptorOutcome outcome) {
    if (!original) {
        if (outcome == ImportDescriptorOutcome::tailed) ++s.import_descriptor_tails[index];
        else if (outcome == ImportDescriptorOutcome::budget_stop) ++s.import_descriptor_budget_stops[index];
        else if (outcome == ImportDescriptorOutcome::context_transfer) ++s.import_descriptor_transfers[index];
        else ++s.import_descriptor_faults[index];
    }
    if (s.import_descriptor_observer)
        s.import_descriptor_observer->leave(s, import_descriptor::entries[index], original, outcome);
}
}
bool try_modern_import_descriptor(State& s, std::uint32_t entry) {
    s.pc = entry;
    const auto index = import_descriptor::entry_index(entry);
    if (s.import_descriptor_original_body == entry) return false;
    bool original = s.import_descriptor_original || s.import_descriptor_entry_original[index];
    if (original) ++s.import_descriptor_original_calls[index];
    else if (match_pc_profile_enabled()) { original = true; ++s.import_descriptor_profile_original_calls[index]; }
    else {
        const auto required = index == import_descriptor::import_count ? 5u : 6u;
        if (s.executed > s.budget || required > s.budget - s.executed) {
            original = true; ++s.import_descriptor_budget_original_calls[index];
        } else ++s.import_descriptor_calls[index];
    }
    if (s.import_descriptor_observer) s.import_descriptor_observer->enter(s, entry, original);
    try {
        if (original) original_tail(s, entry);
        else import_descriptor::invoke(s, index);
    } catch (const GuestContextTransfer&) {
        leave(s, index, original, ImportDescriptorOutcome::context_transfer); throw;
    } catch (const std::runtime_error& error) {
        leave(s, index, original, std::string_view(error.what()) == "Native instruction budget exhausted"
            ? ImportDescriptorOutcome::budget_stop : ImportDescriptorOutcome::fault); throw;
    } catch (...) { leave(s, index, original, ImportDescriptorOutcome::fault); throw; }
    leave(s, index, original, ImportDescriptorOutcome::tailed);
    return true;
}
void reset_import_descriptor_diagnostics(State& s) {
    s.import_descriptor_original = false; s.import_descriptor_entry_original.fill(false);
    s.import_descriptor_calls.fill(0); s.import_descriptor_original_calls.fill(0);
    s.import_descriptor_tails.fill(0); s.import_descriptor_faults.fill(0);
    s.import_descriptor_budget_stops.fill(0); s.import_descriptor_transfers.fill(0);
    s.import_descriptor_budget_original_calls.fill(0); s.import_descriptor_profile_original_calls.fill(0);
    s.import_descriptor_original_body = 0; s.import_descriptor_dispatch_escapes = 0;
    s.import_descriptor_observer = nullptr;
}
std::string import_descriptor_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned n = 0; n < import_descriptor::entry_count; ++n)
        out << "import_descriptor.entry=" << hex_address(import_descriptor::entries[n]) << " modern=" << s.import_descriptor_calls[n]
            << " original=" << s.import_descriptor_original_calls[n] << " tails=" << s.import_descriptor_tails[n]
            << " faults=" << s.import_descriptor_faults[n] << " budget_stops=" << s.import_descriptor_budget_stops[n]
            << " transfers=" << s.import_descriptor_transfers[n] << " budget_original=" << s.import_descriptor_budget_original_calls[n]
            << " profile_original=" << s.import_descriptor_profile_original_calls[n] << '\n';
    out << "import_descriptor.dispatch_escapes=" << s.import_descriptor_dispatch_escapes << '\n';
    return out.str();
}
}
