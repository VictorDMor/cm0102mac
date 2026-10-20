#include "semantic_io_checks.hpp"
#include "native_sort8.hpp"
#include "native_drawing.hpp"
#include "import_descriptor_adapter.hpp"
#include "shortlist_category_adapter.hpp"
#include "match_speed.hpp"
#include "match_turbo.hpp"
#include <random>

namespace {
using namespace cm::semantic_io_checks;
constexpr std::uint32_t base = 0x30000000, entry = 0x10768660;
struct PassiveReference : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
};
unsigned comparisons = 0, faults = 0, overflows = 0;

std::unique_ptr<State> fixture(unsigned count, unsigned kind, unsigned comparator, std::mt19937& random) {
    auto s = std::make_unique<State>();
    s->memory.map(base, 0x10000);
    s->memory.map(0x108b0000, 0x10000);
    for (auto& byte : s->registers) byte = random();
    for (auto& byte : s->unique) byte = random();
    s->pc = entry;
    s->budget = UINT64_MAX;
    s->wr<4>(4, base + 0xf000);
    s->wr<4>(8, 0x108bba90);
    s->wr<4>(12, base + 0x1000);
    s->wr<4>(16, count);
    s->wr<4>(20, 8);
    s->wr<4>(24, base + 0x8000);
    s->wr<4>(4128, return_sentinel);
    if (kind == 4) s->wr<4>(12, base + 0x1001);
    if (kind == 5) s->wr<4>(12, base + 0xeff0); // Array overlaps saved frame.
    if (kind == 6) s->wr<4>(24, base + 0x1000); // Descriptor overlaps sortable records.
    if (kind == 7) s->wr<4>(24, base + 0xeff8); // Descriptor overwritten by the prologue.
    if (kind == 8) s->wr<4>(24, base + 0xfffc); // Valid entry word, missing TOC word.
    if (kind == 9) s->wr<4>(4, base + 0x10); // Partial multiple-store fault.
    if (kind == 10) s->wr<4>(4, 0x80000000);
    if (kind == 11) s->wr<4>(12, base + 0xfffc);
    if (kind == 12) s->wr<4>(12, 0x80000000);
    if (kind == 13) s->wr<4>(4, base + 0xf001);
    if (kind == 14) s->wr<4>(24, base + 0x8001);
    for (unsigned n = 0; n < count; ++n) {
        const auto address = std::uint32_t(s->r<4>(12)) + n * 8;
        if (address < base || std::uint64_t(address) + 8 > base + 0x10000) continue;
        for (unsigned word = 0; word < 2; ++word) {
            const auto value = kind == 0 ? random() : kind == 1 ? 0u : kind == 2 ?
                unsigned(n % 3) - 1u : std::array{0u, 1u, 0x7fffffffu, 0x80000000u, 0xffffffffu}[random() % 5];
            s->memory.write<4>(address + word * 4, value);
        }
    }
    const auto descriptor = std::uint32_t(s->r<4>(24));
    s->memory.write<4>(descriptor, comparator);
    if (descriptor != base + 0xfffc) s->memory.write<4>(descriptor + 4, s->r<4>(8));
    return s;
}

std::string execute(State& s) {
    try { run_until(s, return_sentinel); }
    catch (const std::exception& error) { return error.what(); }
    return {};
}

void diagnostics(const State& a, const State& b, const std::string& label) {
    require(a.import_descriptor_calls == b.import_descriptor_calls &&
        a.import_descriptor_original_calls == b.import_descriptor_original_calls &&
        a.import_descriptor_tails == b.import_descriptor_tails &&
        a.import_descriptor_faults == b.import_descriptor_faults &&
        a.import_descriptor_budget_stops == b.import_descriptor_budget_stops &&
        a.import_descriptor_budget_original_calls == b.import_descriptor_budget_original_calls &&
        a.import_descriptor_profile_original_calls == b.import_descriptor_profile_original_calls &&
        a.import_descriptor_transfers == b.import_descriptor_transfers &&
        a.import_descriptor_original_body == b.import_descriptor_original_body &&
        a.import_descriptor_dispatch_escapes == b.import_descriptor_dispatch_escapes,
        label + " descriptor diagnostics differ");
    const auto& x = shortlist_category_diagnostics(a);
    const auto& y = shortlist_category_diagnostics(b);
    require(x.calls == y.calls && x.original_calls == y.original_calls && x.returns == y.returns &&
        x.faults == y.faults && x.budget_stops == y.budget_stops && x.transfers == y.transfers &&
        x.terminations == y.terminations && x.profile_original_calls == y.profile_original_calls &&
        x.original_body == y.original_body && x.dispatch_escapes == y.dispatch_escapes,
        label + " comparator diagnostics differ");
}

void compare(const State& input, bool must_return = false) {
    auto a = clone(input), b = clone(input);
    // A general observer selects ordinary dispatch without changing the guest.
    // Both sides retain identical unlimited budgets, including overflow cases.
    PassiveReference observer;
    a->memory.observer = &observer;
    const auto before = native_sort8_calls();
    const auto expected = execute(*a), actual = execute(*b);
    a->memory.observer = nullptr;
    const auto label = "Sort8 case=" + std::to_string(comparisons);
    require(expected == actual, label + " faults differ: " + expected + " / " + actual);
    require(!must_return || expected.empty(), label + " expected a complete sort: " + expected);
    Image(*a).compare(*b, label);
    require(a->executed == b->executed, label + " original accounting differs");
    diagnostics(*a, *b, label);
    require(native_sort8_calls() == before + 1, label + " fused path was not entered once");
    ++comparisons;
    faults += !expected.empty();
    overflows += expected == "Native instruction budget exhausted";
}

void equivalence() {
    std::mt19937 random(0x18f3);
    for (const auto comparator : {0x103663b0u, 0x10366410u}) {
        for (unsigned seed = 0; seed < 8; ++seed) {
            for (const unsigned count : {2u, 3u, 7u, 16u, 65u}) {
                for (unsigned kind = 0; kind < 15; ++kind)
                    compare(*fixture(count, kind, comparator, random), kind < 5 || kind >= 13);
            }
        }
        auto input = fixture(7, 3, comparator, random);
        auto reference = clone(*input);
        PassiveReference observer;
        reference->memory.observer = &observer;
        const auto outcome = execute(*reference);
        require(outcome.empty(), "Overflow reference must return: " + outcome);
        const auto steps = reference->executed;
        // Every original fault prefix, including descriptor budget fallback,
        // comparator entry/return and the outer epilogue.
        for (std::uint64_t cut = 0; cut < steps; ++cut) {
            input->executed = UINT64_MAX - cut;
            compare(*input);
        }
    }
}

struct DescriptorObserver : ImportDescriptorBoundaryObserver {
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool, ImportDescriptorOutcome) override {}
};
struct CategoryObserver : ShortlistCategoryBoundaryObserver {
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool, ShortlistCategoryOutcome) override {}
};

void fallbacks_and_drawing(bool profile) {
    std::mt19937 random(114);
    auto s = fixture(16, 0, 0x10366410, random);
    const auto decline = [&](const std::string& label) {
        const Image image(*s);
        const auto steps = s->executed, calls = native_sort8_calls();
        require(!try_native_sort8(*s), label + " accepted fusion");
        image.compare(*s, label);
        require(s->executed == steps && native_sort8_calls() == calls, label + " changed accounting");
    };
    if (profile) { decline("Original-PC profiling"); return; }
    s->budget = UINT64_MAX - 1; decline("Finite budget"); s->budget = UINT64_MAX;
    for (unsigned count : {0u, 1u}) { s->wr<4>(16, count); decline("Trivial count"); }
    s->wr<4>(16, 16);
    for (unsigned width : {0u, 4u, 16u}) { s->wr<4>(20, width); decline("Other record width"); }
    s->wr<4>(20, 8);
    s->memory.write<4>(base + 0x8000, 0x103660b0); decline("Other comparator");
    s->memory.write<4>(base + 0x8000, 0x10366410);
    s->wr<4>(24, 0x80000000); decline("Unmapped descriptor"); s->wr<4>(24, base + 0x8000);
    PassiveReference observer;
    s->memory.observer = &observer; decline("General write observer"); s->memory.observer = nullptr;
    s->memory.date_observer = &observer; decline("Read observer"); s->memory.date_observer = nullptr;
    std::vector<std::pair<std::uint32_t, unsigned>> trace;
    s->memory.read_trace = &trace; decline("Read trace"); s->memory.read_trace = nullptr;
    DescriptorObserver descriptor;
    s->import_descriptor_observer = &descriptor; decline("Descriptor observer"); s->import_descriptor_observer = nullptr;
    for (bool* flag : {&s->import_descriptor_original, &s->import_descriptor_entry_original[import_descriptor::import_count]}) {
        *flag = true; decline("Original descriptor policy"); *flag = false;
    }
    s->import_descriptor_original_body = 0x10781320; decline("Original descriptor body"); s->import_descriptor_original_body = 0;
    auto& d = shortlist_category_diagnostics(*s);
    CategoryObserver category;
    d.observer = &category; decline("Category observer"); d.observer = nullptr;
    for (bool* flag : {&d.original, &d.entry_original[3], &d.entry_original[4]}) {
        *flag = true; decline("Original category policy"); *flag = false;
    }
    for (auto body : {0x103663b0u, 0x10366410u}) {
        d.original_body = body; decline("Original comparator body");
    }
    d.original_body = 0;
    auto a = clone(*s), b = clone(*s);
    NativeDrawingObserver x, y;
    x.register_surface(*a, base + 0x1000, 8, 8, 16, 16);
    y.register_surface(*b, base + 0x1000, 8, 8, 16, 16);
    a->budget = UINT64_MAX - 1;
    const auto before = native_sort8_calls();
    require(execute(*a).empty() && execute(*b).empty(), "Drawing sort must return");
    Image(*a).compare(*b, "Sort surface updates");
    require(a->executed == b->executed && native_sort8_calls() == before + 1, "Drawing fusion coverage");
    diagnostics(*a, *b, "Sort drawing");
    require(x.snapshot(*a, base + 0x1000)->background == y.snapshot(*b, base + 0x1000)->background,
        "Sorted surface pixels differ");
    a->memory.observer = b->memory.observer = nullptr;
}
}

int main(int argc, char** argv) {
    try {
        initialize_native_match_speed();
        const bool profile = argc == 2 && std::string(argv[1]) == "--profile";
        require((argc == 1 || profile) && profile == match_pc_profile_enabled(), "Unexpected sort profile policy");
        if (!profile) equivalence();
        fallbacks_and_drawing(profile);
        std::cout << "PASS sort8 complete comparisons=" << comparisons << " exact-faults=" << faults
                  << " overflow-prefixes=" << overflows << " policy/profile/drawing checks\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
