#include "stdlib_core_adapter.hpp"
#include "stdlib_core_fixtures.hpp"
#include "match_turbo.hpp"
#include <numeric>

namespace {
using namespace cm::stdlib_core_checks;
enum class Path { adapter, generated, cfm };
enum class Mode { modern, entry, global, unrelated, profile };
unsigned cases = 0, exact_stops = 0, mapping_faults = 0, rejected = 0, mixed_resumes = 0;
std::array<unsigned, 4> adapter{}, generated{}, cfm{}, selected{};
std::uint64_t total(const auto& values) { return std::accumulate(values.begin(), values.end(), UINT64_C(0)); }
struct Boundary final : StdlibCoreBoundaryObserver {
    struct Event { std::uint32_t entry; bool original, leave; StdlibCoreOutcome outcome;
        bool operator==(const Event&) const = default; };
    std::vector<Event> events; bool corrupt = false;
    void enter(const State&, std::uint32_t entry, bool original) override {
        events.push_back({entry, original, false, StdlibCoreOutcome::fault});
    }
    void leave(State& s, std::uint32_t entry, bool original, StdlibCoreOutcome outcome) override {
        events.push_back({entry, original, true, outcome});
        if (corrupt && !original) s.wr<4>(12, s.r<4>(12) ^ 1);
    }
};
void prepare(State& s, unsigned index, Path path) {
    s.pc = entries[index];
    if (path == Path::cfm) {
        constexpr auto indirect = fixture + 0x900;
        s.memory.write<4>(indirect, s.pc); s.memory.write<4>(indirect + 4, s.r<4>(8));
        s.wr<4>(48, indirect); s.pc = 0x10781320;
    }
}
std::string execute(State& s, unsigned index, Path path) {
    try {
        if (path == Path::adapter) require(try_modern_stdlib_core(s, entries[index]), "Missing direct stdlib adapter");
        else run_until(s, caller);
    } catch (const std::runtime_error& error) { return error.what(); }
    return {};
}
void compare(unsigned index, const State& initial, Path path, bool turbo, Mode mode = Mode::modern,
        bool budget = false, bool negative = false, const Mutation& mutation = {}, bool guest_comparator = false) {
    auto a = clone(initial), b = clone(initial); prepare(*a, index, path); prepare(*b, index, path);
    a->stdlib_core_original = true;
    if (mode == Mode::entry) b->stdlib_core_entry_original[index] = true;
    if (mode == Mode::global) b->stdlib_core_original = true;
    if (mode == Mode::unrelated) b->stdlib_core_entry_original[(index + 1) % 4] = true;
    Monitor x, y; Comparator old_callback, new_callback; Boundary old, candidate; candidate.corrupt = negative;
    x.effects.state = a.get(); y.effects.state = b.get();
    if (mutation) { mutation(x, old_callback); mutation(y, new_callback); }
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    a->code_fragment_observer = &old_callback; b->code_fragment_observer = &new_callback;
    a->stdlib_core_observer = &old; b->stdlib_core_observer = &candidate;
    set_match_turbo_enabled(false); const auto ax = execute(*a, index, path);
    set_match_turbo_enabled(turbo); const auto bx = execute(*b, index, path);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr;
    a->stdlib_core_observer = b->stdlib_core_observer = nullptr;
    const auto label = hex_address(entries[index]) + "/path=" + std::to_string(unsigned(path)) +
        "/available=" + std::to_string(initial.budget - initial.executed);
    require(ax == bx, label + " outcome original=" + ax + " semantic=" + bx);
    if (negative) {
        bool detected = false; try { Image(*a).compare(*b, label); } catch (const std::runtime_error&) { detected = true; }
        require(detected, label + " own boundary negative accepted"); ++rejected;
    } else Image(*a).compare(*b, label);
    require(x.effects.bytes == y.effects.bytes && old_callback.arguments == new_callback.arguments &&
        old_callback.boundaries == new_callback.boundaries, label + " ordered accesses and import boundaries");
    require(!b->stdlib_core_original_body && !b->stdlib_core_dispatch_escapes, label + " original guard resets and no dispatch escape");
    if (ax.empty() || ax == "Native instruction budget exhausted" || index < 3)
        require(a->executed == b->executed, label + " exact successful/budget instruction accounting");
    const bool modern = total(b->stdlib_core_calls) != 0;
    const bool resumed = total(b->stdlib_core_budget_resumes) != 0;
    if (budget) {
        require(ax.empty() || ax == "Native instruction budget exhausted", label + " bounded outcome " + ax);
        require(!total(b->stdlib_core_original_calls) && !total(b->stdlib_core_profile_original_calls), label + " separate budget path");
        if (!ax.empty()) ++exact_stops;
        mixed_resumes += resumed;
    } else if (mode == Mode::modern || mode == Mode::unrelated) {
        require(b->stdlib_core_calls[index] == 1 && total(b->stdlib_core_calls) == 1 && !resumed &&
            !total(b->stdlib_core_original_calls) && !total(b->stdlib_core_budget_original_calls) &&
            !total(b->stdlib_core_profile_original_calls), label + " one dedicated modern entry");
    } else {
        require(!modern && !resumed, label + " selected original became semantic");
        if (mode == Mode::profile) require(b->stdlib_core_profile_original_calls[index] == 1 &&
            !total(b->stdlib_core_original_calls), label + " profile-only selection");
        else { require(b->stdlib_core_original_calls[index] == 1 && total(b->stdlib_core_original_calls) == 1,
            label + " individual original selection"); ++selected[index]; }
    }
    require(b->stdlib_core_returns[index] == unsigned(modern && !resumed && ax.empty()) &&
        b->stdlib_core_budget_returns[index] == unsigned(modern && resumed && ax.empty()) &&
        b->stdlib_core_faults[index] == unsigned(modern && !ax.empty()) &&
        !total(b->stdlib_core_transfers), label + " returned/faulted/resumed classifications");
    if (guest_comparator) require(ax.empty() && b->r<4>(12) == table + 60 && b->string_calls[9] == 4 &&
        !b->string_original_calls[9] && new_callback.arguments.empty(),
        label + " real semantic string comparator, chosen element and four actual guest calls");
    // A too-small outer CFM budget may stop before reaching the root hook.
    if (!budget || old.events.size() || candidate.events.size()) {
        require(old.events.size() == 2 && candidate.events.size() == 2, label + " one full root boundary");
        require(candidate.events[0].entry == entries[index] && candidate.events[1].entry == entries[index] &&
            !candidate.events[0].leave && candidate.events[1].leave, label + " exact observed root");
    }
    if (ax.starts_with("Unmapped data access at ")) ++mapping_faults;
    if (ax.empty() && !budget && !negative && mode == Mode::modern) {
        if (path == Path::adapter) ++adapter[index]; else if (path == Path::generated) ++generated[index]; else ++cfm[index];
    }
    ++cases;
}
void roots() {
    for (unsigned index = 0; index < 4; ++index) {
        auto s = fixture_state(index);
        for (bool turbo : {false, true}) {
            for (auto path : {Path::adapter, Path::generated, Path::cfm}) compare(index, *s, path, turbo);
            compare(index, *s, Path::generated, turbo, Mode::entry);
            compare(index, *s, Path::cfm, turbo, Mode::global);
            compare(index, *s, Path::generated, turbo, Mode::unrelated);
            if (index == 3) { auto fault = without_byte(*s, descriptor + 7); compare(index, *fault, Path::generated, turbo); }
            if (index == 2) { auto fault = clone(*s); fault->wr<4>(16, 0); compare(index, *fault, Path::generated, turbo); }
        }
        compare(index, *s, Path::adapter, true, Mode::modern, false, true);
        const auto evidence = check(index, *s, "production budget inventory");
        for (std::uint64_t available = 0; available <= evidence.original_steps + 6; ++available) {
            s->budget = s->executed + available;
            for (bool turbo : {false, true}) for (auto path : {Path::generated, Path::cfm})
                compare(index, *s, path, turbo, Mode::modern, true);
        }
    }
    auto s = fixture_state(2);
    for (std::int32_t a : {-17, 0, 17, INT32_MIN}) for (std::int32_t b : {-3, 3, INT32_MIN}) {
        s->wr<4>(12, std::uint32_t(a)); s->wr<4>(16, std::uint32_t(b)); compare(2, *s, Path::generated, true);
    }
    s = fixture_state(3);
    const Mutation expensive = [](Monitor&, Comparator& c) {
        c.body = [](State& out, unsigned) { for (unsigned n = 0; n < 7; ++n) out.step(0x12340000 + n * 4); out.wr<4>(12, 0); };
    };
    for (std::uint64_t available = 10; available < 55; ++available) {
        s->budget = s->executed + available;
        compare(3, *s, Path::generated, true, Mode::modern, true, false, expensive);
    }
    s = fixture_state(3); s->wr<4>(24, 12); s->memory.write<4>(descriptor, 0x1077dc90);
    s->memory.copy_in(key, {'f', 'f', 0});
    for (unsigned n = 0; n < 9; ++n) s->memory.copy_in(table + n * 12,
        {std::uint8_t('a' + n), std::uint8_t('a' + n), 0});
    for (bool turbo : {false, true}) for (auto path : {Path::generated, Path::cfm})
        compare(3, *s, path, turbo, Mode::modern, false, false, {}, true);
}
void reset() {
    auto s = fixture_state(3); const Image before(*s); Boundary observer;
    s->stdlib_core_original = true; s->stdlib_core_entry_original.fill(true);
    s->stdlib_core_calls.fill(1); s->stdlib_core_original_calls.fill(2); s->stdlib_core_returns.fill(3);
    s->stdlib_core_faults.fill(4); s->stdlib_core_transfers.fill(5); s->stdlib_core_budget_original_calls.fill(6);
    s->stdlib_core_profile_original_calls.fill(7); s->stdlib_core_budget_resumes.fill(8); s->stdlib_core_budget_returns.fill(9);
    s->stdlib_core_original_body = 33; s->stdlib_core_dispatch_escapes = 44; s->stdlib_core_observer = &observer;
    s->register_context_calls[28] = 55; s->allocator_original = true;
    reset_stdlib_core_diagnostics(*s); before.compare(*s, "Independent stdlib reset architecture");
    require(!s->stdlib_core_original && s->stdlib_core_entry_original == std::array<bool, 4>{} &&
        !total(s->stdlib_core_calls) && !total(s->stdlib_core_original_calls) && !total(s->stdlib_core_returns) &&
        !total(s->stdlib_core_transfers) && !total(s->stdlib_core_faults) && !total(s->stdlib_core_budget_original_calls) &&
        !total(s->stdlib_core_profile_original_calls) && !total(s->stdlib_core_budget_resumes) && !total(s->stdlib_core_budget_returns) &&
        !s->stdlib_core_original_body && !s->stdlib_core_dispatch_escapes && !s->stdlib_core_observer &&
        s->register_context_calls[28] == 55 && s->allocator_original, "Independent stdlib diagnostics reset"); ++cases;
}
}
int main(int argc, char** argv) {
    try {
        const bool profile = argc == 2 && std::string_view(argv[1]) == "--profile";
        require(argc == 1 || profile, "Usage: stdlib core integration checks [--profile]");
        if (profile) {
            require(match_pc_profile_enabled(), "Profiling must be explicitly enabled");
            for (unsigned index = 0; index < 4; ++index) for (bool turbo : {false, true}) {
                auto s = fixture_state(index); compare(index, *s, Path::generated, turbo, Mode::profile);
            }
        } else {
            require(!match_pc_profile_enabled(), "Normal acceptance requires profiling disabled"); roots(); reset();
            for (unsigned index = 0; index < 4; ++index) {
                require(adapter[index] >= 2 && generated[index] >= 2 && cfm[index] >= 2 && selected[index] == 4,
                    "Missing dedicated individual production evidence");
                std::cout << hex_address(entries[index]) << ": adapter=" << adapter[index] << " generated=" << generated[index]
                    << " CFM=" << cfm[index] << " selected-original=" << selected[index] << '\n';
            }
            require(rejected == 4 && mapping_faults == 2 && mixed_resumes > 0, "Missing negative, fault or search-continuation evidence");
        }
        std::cout << cases << " production comparisons; " << exact_stops << " exact budget stops; " << mapping_faults
            << " mapping faults; " << rejected << " rejected negatives; " << mixed_resumes << " budget continuation cases\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
