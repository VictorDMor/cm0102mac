#include "register_context_adapter.hpp"
#include "register_context_fixtures.hpp"
#include "match_turbo.hpp"
#include <numeric>

namespace {
using namespace cm::register_context_checks;
enum class Path { native, generated, cfm };
enum class Mode { modern, entry, global, mixed, profile };
unsigned cases = 0, stops = 0, mapping_faults = 0, rejected = 0;
std::array<unsigned, 30> native{}, generated{}, cfm{}, selected{};
std::uint64_t total(const auto& values) { return std::accumulate(values.begin(), values.end(), UINT64_C(0)); }
struct Boundary final : RegisterContextBoundaryObserver {
    struct Event { std::uint32_t entry; bool original, leave; RegisterContextOutcome outcome;
        bool operator==(const Event&) const = default; };
    std::vector<Event> events;
    bool corrupt = false;
    void enter(const State&, std::uint32_t entry, bool original) override {
        events.push_back({entry, original, false, RegisterContextOutcome::fault});
    }
    void leave(State& s, std::uint32_t entry, bool original, RegisterContextOutcome outcome) override {
        events.push_back({entry, original, true, outcome});
        if (corrupt && !original) s.wr<4>(12, s.r<4>(12) ^ 1);
    }
};
void prepare(State& s, unsigned index, Path path) {
    s.pc = entries[index].address;
    if (path == Path::cfm) {
        constexpr auto descriptor = fixture + 0xc0;
        s.memory.write<4>(descriptor, s.pc); s.memory.write<4>(descriptor + 4, s.r<4>(8));
        s.wr<4>(48, descriptor); s.pc = 0x10781320;
    }
}
std::string execute(State& s, unsigned index, Path path) {
    try {
        if (path == Path::native) invoke_register_context(s, entries[index].address);
        else run_until(s, index == 29 ? restored_link : incoming_link);
    } catch (const std::runtime_error& error) { return error.what(); }
    return {};
}
void compare(unsigned index, const State& initial, Path path, bool turbo, Mode mode = Mode::modern,
             bool negative = false, bool boundaries = true, bool budget = false) {
    auto a = clone(initial), b = clone(initial); prepare(*a, index, path); prepare(*b, index, path);
    a->register_context_original = true;
    if (mode == Mode::entry) b->register_context_entry_original[index] = true;
    if (mode == Mode::global) b->register_context_original = true;
    if (mode == Mode::mixed) b->register_context_entry_original[(index + 1) % 30] = true;
    Monitor x, y; x.effects.state = a.get(); y.effects.state = b.get();
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    Boundary old, candidate; candidate.corrupt = negative;
    if (boundaries) { a->register_context_observer = &old; b->register_context_observer = &candidate; }
    set_match_turbo_enabled(false); const auto ax = execute(*a, index, path);
    set_match_turbo_enabled(turbo); const auto bx = execute(*b, index, path);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->register_context_observer = b->register_context_observer = nullptr;
    const auto label = hex_address(entries[index].address) + "/path=" + std::to_string(unsigned(path)) +
        "/available=" + std::to_string(initial.budget - initial.executed);
    require(ax == bx, label + " outcome original=" + ax + " modern=" + bx);
    if (negative) {
        bool detected = false; try { Image(*a).compare(*b, label); } catch (const std::runtime_error&) { detected = true; }
        require(detected, label + " boundary negative accepted"); ++rejected;
    } else Image(*a).compare(*b, label);
    require(x.effects.bytes == y.effects.bytes, label + " ordered effects");
    require(!b->register_context_original_body && !b->register_context_dispatch_escapes, label + " original family suppression resets");
    const bool modern = total(b->register_context_calls) != 0;
    if (!modern) require(a->executed == b->executed, label + " original stopping/accounting prefix");
    if (budget) {
        require(ax.empty() || ax == "Native instruction budget exhausted", label + " bounded outcome");
        require(total(b->register_context_original_calls) == 0 && total(b->register_context_profile_original_calls) == 0,
            label + " separate budget selection");
        if (!ax.empty()) { require(!modern, label + " partial call ran modern body"); ++stops; }
    } else if (mode == Mode::modern || mode == Mode::mixed) {
        require(b->register_context_calls[index] == 1 && total(b->register_context_calls) == 1 &&
            !total(b->register_context_original_calls) && !total(b->register_context_budget_original_calls) &&
            !total(b->register_context_profile_original_calls), label + " dedicated modern root");
        require(b->register_context_returns[index] == unsigned(ax.empty() && index != 29) &&
            b->register_context_transfers[index] == unsigned(ax.empty() && index == 29) &&
            b->register_context_faults[index] == unsigned(!ax.empty()), label + " distinct return/transfer/fault");
    } else {
        require(!modern, label + " diagnostic selection became modern in a shared suffix");
        if (mode == Mode::profile) require(b->register_context_profile_original_calls[index] == 1 &&
            !total(b->register_context_original_calls), label + " profile selection");
        else { require(b->register_context_original_calls[index] == 1 && total(b->register_context_original_calls) == 1,
            label + " individual original root"); ++selected[index]; }
    }
    if (boundaries) {
        require(old.events.size() == 2 && candidate.events.size() == 2, label + " one complete boundary");
        require(candidate.events[0].entry == entries[index].address && candidate.events[1].entry == entries[index].address &&
            !candidate.events[0].leave && candidate.events[1].leave, label + " exact root observer");
    }
    require(b->register_context_unknown_transfers == unsigned(index == 29 && path != Path::native && ax.empty()),
        label + " manufactured contexts use separately counted routing fallback");
    if (ax.starts_with("Unmapped data access at ")) ++mapping_faults;
    if (!budget && mode == Mode::modern && ax.empty() && !negative) {
        if (path == Path::native) ++native[index]; else if (path == Path::generated) ++generated[index]; else ++cfm[index];
    }
    ++cases;
}
void roots() {
    for (unsigned index = 0; index < 30; ++index) {
        auto s = fixture_state(index);
        for (bool turbo : {false, true}) {
            for (auto path : {Path::native, Path::generated, Path::cfm}) compare(index, *s, path, turbo);
            compare(index, *s, Path::generated, turbo, Mode::entry);
            compare(index, *s, Path::cfm, turbo, Mode::global, false, false);
            compare(index, *s, Path::generated, turbo, Mode::mixed);
            const auto missing = index < 28 ? stack - (32 - entries[index].first) * 8 : buffer;
            auto fault = without_byte(*s, missing); compare(index, *fault, Path::generated, turbo);
        }
        compare(index, *s, Path::native, true, Mode::modern, true);
        for (std::uint64_t available = 0; available <= entries[index].instructions + 1; ++available) {
            s->budget = s->executed + available;
            for (bool turbo : {false, true}) compare(index, *s, Path::generated, turbo, Mode::modern, false, true, true);
        }
    }
}
void reset() {
    auto s = fixture_state(28); const Image before(*s); Boundary observer;
    s->register_context_original = true; s->register_context_entry_original.fill(true);
    s->register_context_calls.fill(1); s->register_context_original_calls.fill(2);
    s->register_context_returns.fill(3); s->register_context_transfers.fill(4); s->register_context_faults.fill(5);
    s->register_context_budget_original_calls.fill(6); s->register_context_profile_original_calls.fill(7);
    s->register_context_original_body = 99; s->register_context_dispatch_escapes = s->register_context_unknown_transfers = 8;
    s->register_context_observer = &observer; s->compiler_arithmetic_calls[0] = 33; s->allocator_original = true;
    reset_register_context_diagnostics(*s); before.compare(*s, "Reset architecture");
    require(!s->register_context_original && !total(s->register_context_calls) && !total(s->register_context_original_calls) &&
        !total(s->register_context_returns) && !total(s->register_context_transfers) && !total(s->register_context_faults) &&
        !total(s->register_context_budget_original_calls) && !total(s->register_context_profile_original_calls) &&
        s->register_context_entry_original == std::array<bool, 30>{} && !s->register_context_original_body &&
        !s->register_context_dispatch_escapes && !s->register_context_unknown_transfers && !s->register_context_observer &&
        s->compiler_arithmetic_calls[0] == 33 && s->allocator_original, "Independent diagnostic reset"); ++cases;
}
}
int main(int argc, char** argv) {
    try {
        const bool profile = argc == 2 && std::string_view(argv[1]) == "--profile";
        require(argc == 1 || profile, "Usage: register context integration checks [--profile]");
        if (profile) {
            require(match_pc_profile_enabled(), "Profile acceptance requires explicit profiling");
            for (unsigned i = 0; i < 30; ++i) for (bool turbo : {false, true}) {
                auto s = fixture_state(i); s->budget = UINT64_MAX;
                s->executed = match_pc_sample_threshold(0) - 2;
                compare(i, *s, Path::generated, turbo, Mode::profile);
            }
        } else {
            require(!match_pc_profile_enabled(), "Normal acceptance requires profiling disabled"); roots(); reset();
            for (unsigned i = 0; i < 30; ++i) {
                require(native[i] == 2 && generated[i] == 2 && cfm[i] == 2 && selected[i] == 4, "Missing individual dispatch evidence");
                std::cout << hex_address(entries[i].address) << ": native=" << native[i] << " generated=" << generated[i]
                    << " CFM=" << cfm[i] << " original=" << selected[i] << '\n';
            }
            require(rejected == 30 && mapping_faults == 60, "Missing root negatives or mapping faults");
        }
        std::cout << cases << " production comparisons; " << stops << " exact budget stops; " << mapping_faults
            << " mapping faults; " << rejected << " rejected negatives\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
