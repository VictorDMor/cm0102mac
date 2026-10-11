#include "time_runtime_adapter.hpp"
#include "time_runtime_fixtures.hpp"
#include "match_turbo.hpp"
#include <numeric>

namespace {
using namespace cm::time_runtime_checks;
enum class Path { adapter, generated, cfm };
enum class Mode { modern, entry, global, unrelated, profile };
std::array<unsigned, 8> adapters{}, generated{}, indirect{}, selected{};
unsigned cases = 0, rejected = 0, budgets = 0, resumed = 0;
std::uint64_t total(const auto& values) { return std::accumulate(values.begin(), values.end(), UINT64_C(0)); }
struct Boundary final : TimeRuntimeBoundaryObserver {
    unsigned corrupt = 8; bool child = false;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& s, std::uint32_t entry, bool original, TimeRuntimeOutcome outcome) override {
        if (!original && corrupt < 8 && entry == time_runtime_entries[corrupt] && outcome == TimeRuntimeOutcome::returned) {
            if (child) s.memory.write<4>(fields + 12, s.memory.read<4>(fields + 12) ^ 1);
            else s.wr<4>(12, s.r<4>(12) ^ 1);
        }
    }
};
void prepare(State& s, unsigned index, Path path) {
    s.pc = time_runtime_entries[index];
    if (path == Path::cfm) {
        s.memory.write<4>(fixture + 0x900, s.pc); s.memory.write<4>(fixture + 0x904, s.r<4>(8));
        s.wr<4>(48, fixture + 0x900); s.pc = 0x10781320;
    }
}
std::string execute(State& s, unsigned index, Path path) {
    try {
        if (path == Path::adapter) require(try_modern_time_runtime(s, time_runtime_entries[index]), "Direct time-runtime adapter");
        else run_until(s, caller);
    } catch (const std::runtime_error& e) { return e.what(); } return {};
}
void compare(unsigned index, const State& initial, Path path, bool turbo, Mode mode = Mode::modern,
             bool budget = false, bool negative = false, const Mutation& mutation = {}, bool child_negative = false) {
    auto a = clone(initial), b = clone(initial); prepare(*a, index, path); prepare(*b, index, path);
    a->time_runtime_original = true;
    if (mode == Mode::entry) b->time_runtime_entry_original[index] = true;
    if (mode == Mode::global) b->time_runtime_original = true;
    if (mode == Mode::unrelated) b->time_runtime_entry_original[0] = index != 0;
    Monitor x, y; Imports old, modern; Boundary observer;
    x.effects.state = a.get(); y.effects.state = b.get();
    if (mutation) { mutation(x, old); mutation(y, modern); }
    if (negative) observer.corrupt = index;
    if (child_negative) { observer.corrupt = 3; observer.child = true; }
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    a->code_fragment_observer = &old; b->code_fragment_observer = &modern; b->time_runtime_observer = &observer;
    set_match_turbo_enabled(false); const auto ax = execute(*a, index, path);
    set_match_turbo_enabled(turbo); const auto bx = execute(*b, index, path);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr; b->time_runtime_observer = nullptr;
    const auto label = hex_address(time_runtime_entries[index]) + "/path=" + std::to_string(unsigned(path)) +
        "/available=" + std::to_string(initial.budget - initial.executed);
    require(ax == bx, label + " outcomes original=" + ax + " modern=" + bx);
    if (negative || child_negative) {
        bool detected = false; try { Image(*a).compare(*b, label); } catch (const std::runtime_error&) { detected = true; }
        require(detected, label + " boundary negative accepted"); ++rejected;
    } else {
        Image(*a).compare(*b, label);
        require(x.effects.bytes == y.effects.bytes && old.arguments == modern.arguments && old.boundaries == modern.boundaries,
            label + " memory/import effects");
    }
    if (ax.empty() || ax == "Native instruction budget exhausted") require(a->executed == b->executed, label + " exact successful/budget accounting");
    require(!b->time_runtime_original_body && !b->time_runtime_dispatch_escapes, label + " guard reset/escape");
    if (budget) {
        require(ax.empty() || ax == "Native instruction budget exhausted", label + " bounded outcome");
        require(!total(b->time_runtime_original_calls) && !total(b->time_runtime_profile_original_calls), label + " separate budget diagnostics");
        budgets += !ax.empty(); resumed += total(b->time_runtime_budget_resumes) != 0;
    } else if (mode == Mode::modern || mode == Mode::unrelated) {
        require(b->time_runtime_calls[index] == 1 && !total(b->time_runtime_original_calls) &&
            !total(b->time_runtime_budget_original_calls) && !total(b->time_runtime_profile_original_calls) &&
            !total(b->time_runtime_budget_resumes), label + " successful dedicated root and semantic dependencies");
    } else {
        require(b->time_runtime_calls[index] == 0, label + " selected root became semantic");
        if (mode == Mode::profile) require(b->time_runtime_profile_original_calls[index] == 1 &&
            !total(b->time_runtime_original_calls) && !total(b->time_runtime_calls), label + " separate profile selection");
        else { require(b->time_runtime_original_calls[index] == 1, label + " selected original root"); ++selected[index]; }
    }
    if (ax.empty() && !budget && !negative && !child_negative && mode == Mode::modern) {
        require(b->time_runtime_returns[index] == 1 && !total(b->time_runtime_faults) && !total(b->time_runtime_transfers), label + " normal semantic return");
        if (path == Path::adapter) ++adapters[index]; else if (path == Path::generated) ++generated[index]; else ++indirect[index];
    }
    ++cases;
}
void roots() {
    for (unsigned n = 0; n < 8; ++n) {
        auto s = fixture_state(n);
        for (bool turbo : {false, true}) {
            for (auto path : {Path::adapter, Path::generated, Path::cfm}) compare(n, *s, path, turbo);
            compare(n, *s, Path::generated, turbo, Mode::entry);
            compare(n, *s, Path::cfm, turbo, Mode::global);
            compare(n, *s, Path::generated, turbo, Mode::unrelated);
        }
        compare(n, *s, Path::generated, false, Mode::modern, false, true);
        if (n != 0) { auto bad = without_byte(*s, stack + 11); compare(n, *bad, Path::generated, false); }
    }
    auto s = fixture_state(5); s->wr<4>(12, 0);
    compare(5, *s, Path::generated, false, Mode::modern, false, false, {}, true);
    // Original parent/semantic children and the opposite direction retain the same live ABI.
    compare(5, *s, Path::generated, false, Mode::entry);
    compare(5, *s, Path::generated, false, Mode::modern);
}
void budgets_and_faults() {
    for (unsigned n = 0; n < 8; ++n) {
        auto s = fixture_state(n);
        if (n == 2) s->wr<4>(12, 0x7c558180);
        if (n == 5) s->wr<4>(12, 0);
        for (unsigned available = 0; available < 120; ++available) {
            s->budget = s->executed + available;
            for (auto path : {Path::generated, Path::cfm}) compare(n, *s, path, available % 2, Mode::modern, true);
        }
    }
    for (unsigned n : {6u, 7u}) {
        auto s = fixture_state(n); compare(n, *s, Path::generated, false, Mode::modern, false, false,
            [](Monitor&, Imports& imports) { imports.body = [](State&, std::string_view) { throw std::runtime_error("Controlled clock import failure"); }; });
    }
}
void resets() {
    auto s = fixture_state(0); s->time_runtime_original = true; s->time_runtime_entry_original.fill(true);
    s->time_runtime_calls.fill(17); s->time_runtime_returns.fill(19); s->time_runtime_original_body = 42;
    s->stdlib_core_calls[0] = 33; s->register_context_calls[0] = 34; reset_time_runtime_diagnostics(*s);
    require(!s->time_runtime_original && !total(s->time_runtime_calls) && !total(s->time_runtime_returns) &&
        !s->time_runtime_original_body && s->stdlib_core_calls[0] == 33 && s->register_context_calls[0] == 34, "Independent reset");
}
}
int main(int argc, char**) {
    try {
        if (argc > 1) {
            require(match_pc_profile_enabled(), "Explicit profiling mode");
            for (unsigned n = 0; n < 8; ++n) compare(n, *fixture_state(n), Path::generated, false, Mode::profile);
            std::cout << cases << " separate time-runtime profile selections\n"; return 0;
        }
        require(!match_pc_profile_enabled(), "Normal acceptance without profiling");
        roots(); budgets_and_faults(); resets();
        for (unsigned n = 0; n < 8; ++n) {
            require(adapters[n] && generated[n] && indirect[n] && selected[n], "Individual successful dispatch coverage");
            std::cout << "time_runtime.production entry=" << hex_address(time_runtime_entries[n]) << " adapter=" << adapters[n]
                << " generated=" << generated[n] << " cfm=" << indirect[n] << " selected=" << selected[n] << '\n';
        }
        std::cout << cases << " time-runtime production comparisons; " << budgets << " budget stops; " << resumed
            << " diagnostic continuations; " << rejected << " own/child negatives\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
