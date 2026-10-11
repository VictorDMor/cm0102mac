#include "exception_search_fixtures.hpp"
#include "runtime_lifecycle_adapter.hpp"
#include "application_exit.hpp"
#include <numeric>

using namespace cm::semantic_io_checks;
using namespace cm::exception_checks;
namespace {
constexpr std::uint32_t record = fixture + 0x9000, specification = fixture + 0x9100,
    allowed = fixture + 0xa100, bad_type = fixture + 0xa200, callbacks = fixture + 0xe000,
    callback_toc = fixture + 0xe100, indirect_entry = fixture + 0xf800;
unsigned comparisons = 0, negatives = 0;
std::array<unsigned, 21> direct_witnesses{}, cfm_witnesses{};
struct Imports : RuntimeLifecycleBoundaryObserver {
    struct Import { std::string name; decltype(State::registers) registers; std::uint32_t pc;
        std::uint64_t calls; bool operator==(const Import&) const = default; };
    std::vector<Import> imports;
    bool returning_exit = true;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool, bool) override {}
    void import_enter(const State& s, const char* name) override { imports.push_back({name, s.registers, s.pc, s.native_calls}); }
    void import_leave(const State& s, const char* name) override { import_enter(s, name); }
    bool import_fixture(State& s, const char* name) override {
        if (std::string_view(name) != "CarbonLib::Gestalt" && !(returning_exit && std::string_view(name) == "CarbonLib::ExitToShell")) return false;
        s.wr<4>(12, 0x12345678); s.pc = std::uint32_t(s.r<4>(4128)); return true;
    }
};
struct Boundaries final : ExceptionRuntimeBoundaryObserver {
    struct Event { std::uint32_t entry; bool original, returned; ExceptionRuntimeOutcome outcome; };
    std::vector<Event> events;
    void enter(const State&, std::uint32_t entry, bool original) override { events.push_back({entry, original, false, {}}); }
    void leave(State&, std::uint32_t entry, bool original, ExceptionRuntimeOutcome outcome) override {
        events.push_back({entry, original, true, outcome});
    }
};
struct Input { std::unique_ptr<State> state; std::uint32_t stop = return_sentinel; };
Input initial(const char* program, unsigned index, bool cfm = false) {
    auto f = setup(program); Input input{std::move(f.state)}; auto& s = *input.state;
    s.memory.write<4>(stack, stack + 128); s.memory.write<4>(stack + 128, stack + 192);
    s.memory.write<4>(stack + 136, base + 0x120); s.memory.write<4>(stack + 8, base + 0x120);
    for (auto field : {0x39c, 0x394, 0x3a0, 0x3a4}) s.memory.write<4>(context + field, stack);
    for (auto [offset, value] : {std::pair{0u, descriptor}, {4u, base + 0x100}, {8u, action},
            {12u, base}, {16u, descriptors}, {20u, toc}, {24u, stack}, {28u, stack}, {32u, stack}})
        s.memory.write<4>(metadata + offset, value);
    for (unsigned n = 0; n < 4; ++n) {
        s.memory.write<4>(callbacks + n * 8, 280984548); s.memory.write<4>(callbacks + n * 8 + 4, callback_toc + n * 0x100);
    }
    for (auto [offset, value] : {std::pair{0x5abcu, callbacks + 24}, {0x3b94u, bad_type}, {0x5ab8u, 0x11223344u},
            {0x5ab4u, 0x55667788u}, {0x3b90u, allowed + 0x20}, {0x3b98u, allowed + 0x40},
            {0x3ba0u, bad_type}, {0x3ba4u, actual}, {0x3520u, callbacks}, {0x351cu, callbacks + 8}})
        s.memory.write<4>(toc - offset, value);
    text(s, bad_type, "!Bad!0!"); text(s, allowed + 0x20, "l"); text(s, allowed + 0x40, "q");
    s.memory.write<4>(record, object); s.memory.write<4>(record + 4, actual); s.memory.write<4>(record + 8, callbacks + 16);
    s.memory.write<4>(record + 0x14, specification); s.memory.write<2>(specification + 2, 0);
    constexpr auto lifecycle = fixture + 0xb000;
    for (auto [offset, value] : {std::pair{0x5a7cu, lifecycle}, {0x5a80u, lifecycle + 4}, {0x5a84u, lifecycle + 8},
            {0x3b78u, lifecycle + 0x100}, {0x3b3cu, lifecycle + 0x200}, {0x5ab0u, lifecycle + 12}})
        s.memory.write<4>(toc - offset, value);
    s.memory.write<4>(toc - 0x3514, 0);
    for (unsigned n = 0; n < 4; ++n) s.memory.write<4>(lifecycle + n * 4, 0);
    for (unsigned n = 0; n < 6; ++n) s.memory.write<4>(lifecycle + 0x200 + n * 4, 1);
    s.wr<4>(4128, return_sentinel); s.budget = 100000;
    switch (index) {
    case 0:
        s.memory.write<1>(action, 0); s.memory.write<1>(action + 4, 0); s.wr<4>(12, metadata); break;
    case 1: s.wr<4>(16, metadata); break;
    case 2: s.wr<4>(16, metadata); s.wr<4>(20, action); break;
    case 3:
        s.memory.write<2>(specification + 2, 1); s.memory.write<4>(specification + 12, expected);
        s.wr<4>(12, actual); s.wr<4>(16, specification); break;
    case 4: s.wr<4>(12, record); break;
    case 5: case 6: s.wr<4>(12, object); s.wr<4>(16, 0); break;
    case 7: s.wr<4>(16, stack); s.wr<4>(20, return_sentinel); s.wr<4>(24, 0); break;
    case 8:
        s.memory.write<1>(action, 15); s.memory.write<2>(action + 2, 0);
        s.memory.write<4>(action + 4, 0x200); s.memory.write<4>(action + 8, 0x40);
        s.wr<4>(16, metadata); s.wr<4>(20, action); input.stop = landing; break;
    case 9: input.stop = landing; break;
    case 10: case 11: case 12:
        s.wr<4>(12, actual); s.wr<4>(16, object); s.wr<4>(20, callbacks + 16); s.wr<4>(4128, base + 0x120);
        s.wr<4>(5120, 0x80000100); s.memory.write<1>(toc - 0x352a, index == 11 ? 1 : 0);
        if (index == 10) {
            s.memory.write<4>(node + 12, base + 0x900000); s.memory.write<4>(node + 4, ranges + 24);
            s.memory.write<4>(ranges + 12, 0x772ad0); s.memory.write<4>(ranges + 16, 0x34); s.memory.write<4>(ranges + 20, 0x400);
        }
        input.stop = landing; break;
    case 13: s.wr<4>(12, record); break;
    case 20: s.wr<4>(12, actual); s.wr<4>(16, expected); s.wr<4>(20, specification); break;
    default: break;
    }
    s.pc = exception_runtime_entries[index];
    if (cfm) {
        s.memory.write<4>(indirect_entry, s.pc); s.memory.write<4>(indirect_entry + 4, toc);
        s.wr<4>(48, indirect_entry); s.pc = 0x10781320;
    }
    return input;
}
template<class F> std::string outcome(F&& run) {
    try { run(); } catch (const std::exception& error) { return error.what(); }
    return {};
}
void compare(Input input, unsigned index, bool cfm, int original_entry = -1, bool fault = false,
        bool unknown = false, const std::string& suffix = {}) {
    auto a = std::move(input.state), b = clone(*a); a->exception_runtime_original = true;
    if (original_entry >= 0) b->exception_runtime_entry_original[unsigned(original_entry)] = true;
    Imports x, y; Boundaries boundaries; Effects ea, eb; ea.state = a.get(); eb.state = b.get();
    a->runtime_lifecycle_observer = &x; b->runtime_lifecycle_observer = &y; b->exception_runtime_observer = &boundaries;
    a->memory.date_observer = &ea; b->memory.date_observer = &eb;
    const auto label = hex_address(exception_runtime_entries[index]) + (cfm ? " CFM" : " direct") + suffix;
    const auto original_error = outcome([&] { run_until(*a, input.stop); });
    const auto modern_error = outcome([&] { run_until(*b, input.stop); });
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->runtime_lifecycle_observer = b->runtime_lifecycle_observer = nullptr; b->exception_runtime_observer = nullptr;
    require(original_error == modern_error && fault == !original_error.empty(), label + " original=" + original_error + " modern=" + modern_error);
    Image image(*a); image.compare(*b, label); require(ea.bytes == eb.bytes && ea.callbacks == eb.callbacks && x.imports == y.imports, label + " ordered accesses/callbacks/imports");
    require(a->exception_runtime_original_calls[index] && !a->exception_runtime_calls[index], label + " original root witness");
    require((original_entry == int(index) ? b->exception_runtime_original_calls[index] : b->exception_runtime_calls[index]) > 0, label + " selected root witness");
    require(b->exception_runtime_unknown_transfers == unsigned(unknown) && b->exception_runtime_dispatch_escapes == unsigned(fault), label + " separate unknown/escape counts");
    require(std::any_of(boundaries.events.begin(), boundaries.events.end(), [&](const auto& e) {
        return e.entry == exception_runtime_entries[index] && !e.returned && e.original == (original_entry == int(index));
    }), label + " observed root entry");
    if (original_entry < 0 && !unknown) {
        require(std::accumulate(b->exception_runtime_original_calls.begin(), b->exception_runtime_original_calls.end(), std::uint64_t(0)) == 0,
                label + " normal mode must not execute diagnostic original bodies");
        ++(cfm ? cfm_witnesses[index] : direct_witnesses[index]);
        if (!cfm) {
            b->wr<4>(12, b->r<4>(12) ^ 1); bool rejected = false;
            try { image.compare(*b, label + " corrupted root"); } catch (const std::runtime_error&) { rejected = true; }
            require(rejected, label + " corrupted production root accepted"); ++negatives;
        }
    } else if (original_entry >= 0) {
        require(b->exception_runtime_original_calls[unsigned(original_entry)] > 0, label + " selected original child/root executed");
        for (unsigned n = 0; n < 21; ++n) if (n != unsigned(original_entry)) require(!b->exception_runtime_original_calls[n], label + " unrelated original selection");
    }
    ++comparisons;
}
void dispatch_checks(const char* program) {
    for (unsigned n = 0; n < 21; ++n) {
        compare(initial(program, n), n, false);
        compare(initial(program, n, true), n, true);
        compare(initial(program, n), n, false, int(n), false, false, " observed original root");
    }
    for (auto [root, child] : {std::pair{3u, 20u}, {4u, 19u}, {10u, 12u}, {11u, 9u}, {17u, 18u}})
        compare(initial(program, root), root, false, int(child), false, false, " selected original child");
    for (auto [index, slot, target] : {std::tuple{0u, 0x3b88u, 0x107717d8u},
             {2u, 0x3b8cu, 0x1077241cu}, {9u, 0x3b9cu, 0x10772ab0u}}) {
        auto f = initial(program, index); const auto kind = index == 0 ? 0u : 12u;
        const auto table = std::uint32_t(f.state->memory.read<4>(toc - slot)); f.state->memory.write<4>(table + kind * 4, target);
        f.stop = return_sentinel; compare(std::move(f), index, false, -1, false, true, " unknown compiled continuation");
    }
    auto f = initial(program, 0); const auto table = std::uint32_t(f.state->memory.read<4>(toc - 0x3b88));
    f.state->memory.write<4>(table, 0xdead0000); compare(std::move(f), 0, false, -1, true, true, " unmapped computed destination");
}
struct ThrowingImports final : Imports {
    unsigned capture, after_throw = 0;
    bool thrown = false;
    explicit ThrowingImports(unsigned mode) : capture(mode) {}
    bool import_fixture(State& s, const char* name) override {
        if (!thrown && std::string_view(name) == "CarbonLib::Gestalt" && s.r<4>(8) == callback_toc + 0x100) {
            thrown = true;
            s.wr<4>(12, actual); s.wr<4>(16, object); s.wr<4>(20, callbacks + 16);
            const auto entry = capture == 0 ? 0x10772ca0u : capture == 1 ? 0x10772b10u : 0x10772ad0u;
            invoke_exception_runtime(s, entry); ++after_throw;
            throw std::runtime_error("Unexpected callback throw returned to interrupted import");
        }
        return Imports::import_fixture(s, name);
    }
};
Input unexpected_initial(const char* program, unsigned branch, unsigned capture) {
    auto input = initial(program, 4); auto& s = *input.state;
    s.wr<4>(4128, base + 0x120); s.wr<4>(5120, 0x80000100);
    s.memory.write<1>(toc - 0x352a, capture == 1 || capture == 3);
    // The throwing CFM callback has its own TOC. Its runtime registry and
    // dispatch slots must describe this fixture too; a zero handler slot is
    // an original null-descriptor fault, not a successful unexpected catch.
    const auto throwing_toc = callback_toc + 0x100;
    for (auto offset : {0x3b9c, 0x3b8c, 0x3b88, 0x3524, 0x3528, 0x3520})
        s.memory.write<4>(throwing_toc - offset, s.memory.read<4>(toc - offset));
    s.memory.write<1>(throwing_toc - 0x3529, 1);
    s.memory.write<1>(throwing_toc - 0x352a, capture == 1 || capture == 3);
    s.memory.write<4>(node + 12, base + 0x900000); s.memory.write<4>(node + 4, ranges + 48);
    for (auto [offset, start, length, desc] : {std::tuple{12u, 0x7724c0u, 0xdcu, 0x300u},
             {24u, 0x772ad0u, 0x34u, 0x400u}, {36u, 0x772ee0u, 0x28u, 0x500u}}) {
        s.memory.write<4>(ranges + offset, start); s.memory.write<4>(ranges + offset + 4, length);
        s.memory.write<4>(ranges + offset + 8, desc);
    }
    const auto own = descriptors + 0x300;
    // After catch restoration, r1 can retain the deeper callback stack. The
    // unexpected function's records use its saved r31 frame instead.
    s.memory.write<2>(own, (6u << 11) | 0x10);
    for (auto [offset, value] : {std::pair{2u, 4u}, {4u, 0x30u}, {6u, 0x80u},
             {8u, 0x34u}, {10u, 0xd8u}, {12u, 0xa0u}}) s.memory.write<2>(own + offset, value);
    s.memory.write<1>(own + 0x80, 15); s.memory.write<2>(own + 0x82, 0);
    s.memory.write<4>(own + 0x84, 0x34); s.memory.write<4>(own + 0x88, 0x40);
    s.memory.write<1>(own + 0xa0, 0x8d); s.memory.write<2>(own + 0xa2, 0x40);
    s.memory.write<2>(specification + 2, 1); s.memory.write<4>(specification + 12, allowed);
    text(s, allowed, branch == 1 ? "i" : branch == 2 ? "l" : branch == 3 ? "q" : "x");
    if (branch >= 2) text(s, expected, "!Bad!");
    input.stop = branch ? landing : base + 0x120;
    return input;
}
void unexpected_checks(const char* program) {
    for (unsigned n = 0; n < 6; ++n) {
        const auto branch = n < 4 ? n : n == 4 ? 0u : 2u, capture = n < 4 ? n % 2 : n - 2;
        auto input = unexpected_initial(program, branch, capture); auto a = std::move(input.state), b = clone(*a);
        a->exception_runtime_original = true;
        ThrowingImports x(capture), y(capture); Boundaries boundary; Effects ea, eb; ea.state = a.get(); eb.state = b.get();
        a->runtime_lifecycle_observer = &x; b->runtime_lifecycle_observer = &y; b->exception_runtime_observer = &boundary;
        a->memory.date_observer = &ea; b->memory.date_observer = &eb;
        const auto label = "Nested unexpected branch=" + std::to_string(branch) + " capture=" + std::to_string(capture);
        const auto ax = outcome([&] { run_until(*a, input.stop); }), bx = outcome([&] { run_until(*b, input.stop); });
        a->memory.date_observer = b->memory.date_observer = nullptr;
        a->runtime_lifecycle_observer = b->runtime_lifecycle_observer = nullptr; b->exception_runtime_observer = nullptr;
        require(ax.empty() && bx.empty(), label + " original=" + ax + " pc=" + hex_address(a->pc) + " toc=" + hex_address(a->r<4>(8)) +
            " modern=" + bx + " pc=" + hex_address(b->pc) + " toc=" + hex_address(b->r<4>(8)));
        Image image(*a); image.compare(*b, label);
        require(ea.bytes == eb.bytes && ea.callbacks == eb.callbacks && x.imports == y.imports, label + " ordered access/import prefix");
        require(x.thrown && y.thrown && !x.after_throw && !y.after_throw, label + " interrupted callback never resumes");
        require(b->exception_runtime_calls[4] == 1 && b->exception_runtime_transfers[4] == 1 && !b->exception_runtime_returns[4] &&
            b->exception_runtime_landings == 1 && !b->exception_runtime_original_landings, label + " owning entry and separate continuation witnesses");
        require(std::accumulate(b->exception_runtime_original_calls.begin(), b->exception_runtime_original_calls.end(), std::uint64_t(0)) == 0 &&
            !b->exception_runtime_unknown_transfers && !b->exception_runtime_dispatch_escapes, label + " all semantic, no unknown transfer/escape");
        if (!branch) require(b->pc == base + 0x120 && b->r<4>(4) == stack && b->exception_runtime_landing_returns == 1 &&
            !b->exception_runtime_landing_transfers, label + " returning continuation restores original caller");
        else {
            const auto published = branch == 1 ? object : stack - 112 + (branch == 2 ? 0x3c : 0x38);
            require(b->pc == landing && b->memory.read<4>(stack + 0x40) == published && b->exception_runtime_landing_transfers == 1 &&
                !b->exception_runtime_landing_returns, label + " continuation rethrow/bad-exception publication");
            if (branch >= 2) require(b->memory.read<4>(published) == 0x11223344, label + " bad-exception final vtable");
        }
        if (!n) {
            b->wr<4>(12, b->r<4>(12) ^ 1); bool rejected = false;
            try { image.compare(*b, label + " corrupted landing"); } catch (const std::runtime_error&) { rejected = true; }
            require(rejected, "Corrupted production continuation accepted"); ++negatives;
        }
        ++comparisons;
    }
}
void reset_checks(const char* program) {
    auto input = initial(program, 0); auto& s = *input.state; Boundaries observer;
    s.exception_runtime_original = true; s.exception_runtime_entry_original.fill(true);
    s.exception_runtime_calls.fill(11); s.exception_runtime_original_calls.fill(12); s.exception_runtime_returns.fill(13);
    s.exception_runtime_transfers.fill(14); s.exception_runtime_terminations.fill(15); s.exception_runtime_original_resume = 123;
    s.exception_runtime_dispatch_escapes = 16; s.exception_runtime_unknown_transfers = 17; s.exception_runtime_landings = 18;
    s.exception_runtime_original_landings = 19; s.exception_runtime_landing_returns = 20; s.exception_runtime_landing_transfers = 21;
    s.exception_runtime_landing_terminations = 22; s.exception_runtime_observer = &observer;
    s.code_fragment_original = true; s.code_fragment_calls.fill(91); s.cpp_object_storage_calls.fill(92);
    Image image(s); reset_exception_runtime_diagnostics(s); image.compare(s, "Exception reset architectural/heap isolation");
    require(!s.exception_runtime_original && !s.exception_runtime_original_resume && !s.exception_runtime_observer &&
        !s.exception_runtime_dispatch_escapes && !s.exception_runtime_unknown_transfers && !s.exception_runtime_landings &&
        !s.exception_runtime_original_landings && !s.exception_runtime_landing_returns && !s.exception_runtime_landing_transfers &&
        !s.exception_runtime_landing_terminations, "Exception reset independent scalar fields");
    for (unsigned n = 0; n < 21; ++n) require(!s.exception_runtime_entry_original[n] && !s.exception_runtime_calls[n] &&
        !s.exception_runtime_original_calls[n] && !s.exception_runtime_returns[n] && !s.exception_runtime_transfers[n] &&
        !s.exception_runtime_terminations[n], "Exception reset every entry");
    require(s.code_fragment_original && s.code_fragment_calls[0] == 91 && s.cpp_object_storage_calls[0] == 92,
        "Exception reset preserves adjacent module diagnostics");
}
void terminal_and_child_negative_checks(const char* program) {
    auto input = initial(program, 16); auto a = std::move(input.state), b = clone(*a); a->exception_runtime_original = true;
    Imports x, y; x.returning_exit = y.returning_exit = false; Boundaries boundary;
    Effects ea, eb; ea.state = a.get(); eb.state = b.get();
    a->runtime_lifecycle_observer = &x; b->runtime_lifecycle_observer = &y; b->exception_runtime_observer = &boundary;
    a->memory.date_observer = &ea; b->memory.date_observer = &eb;
    const auto terminated = [](State& state) {
        try { run(state); } catch (const ApplicationExit&) { return true; } return false;
    };
    require(terminated(*a) && terminated(*b), "Production abort propagates actual native termination");
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->runtime_lifecycle_observer = b->runtime_lifecycle_observer = nullptr; b->exception_runtime_observer = nullptr;
    Image(*a).compare(*b, "Production native-exit prefix");
    require(ea.bytes == eb.bytes && x.imports == y.imports && b->exception_runtime_terminations[16] == 1 &&
        !b->exception_runtime_returns[16] && !b->exception_runtime_transfers[16], "Abort termination is distinct from return/transfer");
    require(std::any_of(boundary.events.begin(), boundary.events.end(), [](const auto& e) {
        return e.entry == 0x10772e70 && e.returned && e.outcome == ExceptionRuntimeOutcome::terminated && !e.original;
    }), "Observed production termination boundary"); ++comparisons;

    struct ChildBoundary final : ExceptionRuntimeBoundaryObserver {
        std::unique_ptr<Image> original; bool corrupted = false;
        void enter(const State&, std::uint32_t, bool) override {}
        void leave(State& state, std::uint32_t entry, bool baseline, ExceptionRuntimeOutcome result) override {
            if (entry != 0x10772f10) return;
            require(result == ExceptionRuntimeOutcome::returned, "Matching child returns before specification parent");
            if (baseline) original = std::make_unique<Image>(state);
            else {
                require(bool(original), "Original matching child boundary"); original->compare(state, "Unmutated matching child");
                state.wr<4>(12, state.r<4>(12) ^ 1); corrupted = true; original->compare(state, "Corrupted matching child");
            }
        }
    } child;
    input = initial(program, 3); a = std::move(input.state); b = clone(*a); a->exception_runtime_original = true;
    a->exception_runtime_observer = b->exception_runtime_observer = &child;
    run(*a); const auto error = outcome([&] { run(*b); });
    a->exception_runtime_observer = b->exception_runtime_observer = nullptr;
    require(child.corrupted && error.starts_with("Corrupted matching child") && b->pc == 0x10772474 && b->r<4>(4) == stack - 80 &&
        !b->exception_runtime_returns[3] && !b->exception_runtime_transfers[3], "Matching child is rejected before specification parent consumes return");
    ++negatives;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-exception-integration-checks <program>");
        dispatch_checks(argv[1]); unexpected_checks(argv[1]); reset_checks(argv[1]); terminal_and_child_negative_checks(argv[1]);
        for (unsigned n = 0; n < 21; ++n) require(direct_witnesses[n] && cfm_witnesses[n], "Every production root needs direct and CFM witnesses");
        require(negatives == 23, "Every production root, unexpected continuation and matching child need negatives");
        std::cout << comparisons << " production exception comparisons, " << negatives
                  << " root/landing/child negatives; all 21 direct/CFM/original roots, mixed children, nested unexpected landing, unknown transfers, native exit and independent reset passed\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
