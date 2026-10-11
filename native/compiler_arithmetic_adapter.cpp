#include "compiler_arithmetic_adapter.hpp"
#include "compiler_arithmetic.hpp"
#include "match_turbo.hpp"
#include <algorithm>
#include <sstream>
#include <string_view>

namespace cm::aot {
namespace {
using Operation = void (*)(State&);
constexpr std::array<Operation, 8> operations{
    compiler_arithmetic::double_to_unsigned_word, compiler_arithmetic::unsigned_quotient,
    compiler_arithmetic::signed_quotient, compiler_arithmetic::unsigned_remainder,
    compiler_arithmetic::signed_remainder, compiler_arithmetic::signed_pair_to_double,
    compiler_arithmetic::signed_pair_to_single, compiler_arithmetic::double_to_signed_pair};
// For the seven ordinary semantic entry charges, a conservative original-body
// bound preserves every tight-budget stopping prefix. Divider loops contain at
// most 65 iterations of 11 instructions; the other instructions execute once.
// The unsigned quotient instead retains its pre-existing exact full-call charge.
constexpr std::array<std::uint64_t, 8> original_bounds{20, 0, 780, 760, 771, 42, 43, 49};
void leave(State& s, unsigned index, bool original, bool returned) {
    if (!original) ++(returned ? s.compiler_arithmetic_returns[index] : s.compiler_arithmetic_faults[index]);
    if (s.compiler_arithmetic_observer)
        s.compiler_arithmetic_observer->leave(s, compiler_arithmetic_entries[index], original, returned);
}
void execute(State& s, unsigned index, bool original, std::uint64_t instruction_charge) {
    const auto entry = compiler_arithmetic_entries[index], caller = std::uint32_t(s.r<4>(4128));
    if (s.compiler_arithmetic_observer) s.compiler_arithmetic_observer->enter(s, entry, original);
    try {
        if (original) {
            s.compiler_arithmetic_original_resume = entry;
            try { run_until(s, caller); }
            catch (...) { s.compiler_arithmetic_original_resume = 0; throw; }
            s.compiler_arithmetic_original_resume = 0;
        } else if (index == 1) {
            s.executed += instruction_charge;
            operations[index](s);
        } else {
            s.step(entry); operations[index](s);
        }
    } catch (const std::runtime_error& error) {
        if (std::string_view(error.what()).starts_with("Uncompiled original address:")) ++s.compiler_arithmetic_dispatch_escapes;
        leave(s, index, original, false); throw;
    } catch (...) { leave(s, index, original, false); throw; }
    leave(s, index, original, s.pc == caller);
}
}
bool try_modern_compiler_arithmetic(State& s, std::uint32_t entry) {
    s.pc = entry;
    if (s.compiler_arithmetic_original_resume == entry) { s.compiler_arithmetic_original_resume = 0; return false; }
    const auto found = std::find(compiler_arithmetic_entries.begin(), compiler_arithmetic_entries.end(), entry);
    if (found == compiler_arithmetic_entries.end()) throw std::runtime_error("Unknown compiler-arithmetic entry");
    const auto index = unsigned(found - compiler_arithmetic_entries.begin());
    std::uint64_t instruction_charge = 1;
    bool original = s.compiler_arithmetic_original || s.compiler_arithmetic_entry_original[index];
    if (original) ++s.compiler_arithmetic_original_calls[index];
    else if (match_pc_profile_enabled()) { ++s.compiler_arithmetic_profile_original_calls[index]; original = true; }
    else {
        const auto required = index == 1 ? compiler_arithmetic::unsigned_quotient_instruction_count(s) : original_bounds[index];
        if (s.executed > s.budget || required > s.budget - s.executed) {
            ++s.compiler_arithmetic_budget_original_calls[index]; original = true;
        } else { ++s.compiler_arithmetic_calls[index]; if (index == 1) instruction_charge = required; }
    }
    if (original && !s.compiler_arithmetic_observer) return false;
    execute(s, index, original, instruction_charge); return true;
}
void invoke_compiler_arithmetic(State& s, std::uint32_t entry) {
    const auto caller = std::uint32_t(s.r<4>(4128));
    if (try_modern_compiler_arithmetic(s, entry)) return;
    s.compiler_arithmetic_original_resume = entry;
    try { run_until(s, caller); }
    catch (...) { s.compiler_arithmetic_original_resume = 0; throw; }
    s.compiler_arithmetic_original_resume = 0;
}
void reset_compiler_arithmetic_diagnostics(State& s) {
    s.compiler_arithmetic_original = false; s.compiler_arithmetic_entry_original.fill(false);
    s.compiler_arithmetic_calls.fill(0); s.compiler_arithmetic_original_calls.fill(0);
    s.compiler_arithmetic_returns.fill(0); s.compiler_arithmetic_faults.fill(0);
    s.compiler_arithmetic_budget_original_calls.fill(0); s.compiler_arithmetic_profile_original_calls.fill(0);
    s.compiler_arithmetic_original_resume = 0; s.compiler_arithmetic_dispatch_escapes = 0;
    s.compiler_arithmetic_observer = nullptr;
}
std::string compiler_arithmetic_call_summary(const State& s) {
    std::ostringstream out;
    for (unsigned i = 0; i < compiler_arithmetic_entries.size(); ++i)
        out << "compiler_arithmetic.entry=" << hex_address(compiler_arithmetic_entries[i]) << " mode="
            << (s.compiler_arithmetic_original || s.compiler_arithmetic_entry_original[i] ? "original" : "modern")
            << " modern=" << s.compiler_arithmetic_calls[i] << " original=" << s.compiler_arithmetic_original_calls[i]
            << " returns=" << s.compiler_arithmetic_returns[i] << " faults=" << s.compiler_arithmetic_faults[i]
            << " budget_original=" << s.compiler_arithmetic_budget_original_calls[i]
            << " profile_original=" << s.compiler_arithmetic_profile_original_calls[i] << '\n';
    out << "compiler_arithmetic.dispatch_escapes=" << s.compiler_arithmetic_dispatch_escapes << '\n';
    return out.str();
}
}
