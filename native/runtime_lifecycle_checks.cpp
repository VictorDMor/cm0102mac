#include "runtime_lifecycle_internal.hpp"
#include "runtime_lifecycle_adapter.hpp"
#include "semantic_io_checks.hpp"
#include "application_exit.hpp"
#include <limits>

using namespace cm::semantic_io_checks;
namespace {
constexpr auto entries = runtime_lifecycle::entries;
constexpr std::uint32_t head = fixture + 0x1000, skip = head + 4,
    stream = head + 8, terminal = head + 12, table = fixture + 0x1100,
    handlers = fixture + 0x1200, nodes = fixture + 0x2000,
    vectors = fixture + 0x3000;
unsigned cases = 0, faults = 0, terminations = 0, negatives = 0;
std::array<std::uint64_t, 6> witnesses{};
std::unique_ptr<State> setup(const char* program, unsigned index) {
    auto s = std::make_unique<State>(); load_program(*s, program);
    s->memory.map(fixture, 0x10000); s->memory.copy_in(fixture, std::vector<std::uint8_t>(0x10000, 0xa5));
    s->memory.copy_in(stack - 8192, std::vector<std::uint8_t>(8256, 0xa7));
    s->registers.fill(0x5b); s->unique.fill(0x6d);
    for (unsigned n = 0; n < 32; ++n) s->wr<4>(n * 4, 0xcafe0000 + n);
    s->wr<4>(4, stack); s->wr<4>(8, toc); s->wr<4>(4132, 0x31415926);
    s->wr<4>(2440, 0xbadc0ffe); s->wr<1>(2304, index % 2 ? 0xf9 : 0xa6);
    s->wr<1>(1024, index % 4); s->wr<1>(1027, (index / 2) % 2);
    for (auto [offset, address] : {std::pair{0x5ab0u, head}, {0x5a7cu, skip},
            {0x5a80u, stream}, {0x5a84u, terminal}, {0x3b78u, table}, {0x3b3cu, handlers}})
        s->memory.write<4>(toc - offset, address);
    s->memory.write<4>(toc - 0x3514, 0);
    for (unsigned n = 0; n < 4; ++n) s->memory.write<4>(head + n * 4, 0);
    for (unsigned n = 0; n < 6; ++n) {
        s->memory.write<4>(handlers + n * 4, 1);
        s->memory.write<4>(vectors + n * 8, 0x10776d10);
        s->memory.write<4>(vectors + n * 8 + 4, fixture + 0x4000 + n * 0x100);
        s->memory.write<4>(table + n * 4, vectors + n * 8);
        const auto node = nodes + n * 12;
        s->memory.write<4>(node, n ? node - 12 : 0);
        s->memory.write<4>(node + 4, vectors + n * 8);
        s->memory.write<4>(node + 8, 0x200 + n);
    }
    s->wr<4>(12, index == 5 ? 1 : 37); s->wr<4>(16, vectors); s->wr<4>(20, nodes);
    s->pc = entries[index]; s->wr<4>(4128, return_sentinel); s->budget = 100000;
    return s;
}
struct Imports : RuntimeLifecycleBoundaryObserver {
    struct Record { std::string name; decltype(State::registers) registers;
        std::uint32_t pc; std::uint64_t calls; bool operator==(const Record&) const = default; };
    bool returning = false;
    std::vector<Record> records;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool, bool) override {}
    void import_enter(const State& s, const char* name) override { records.push_back({name, s.registers, s.pc, s.native_calls}); }
    void import_leave(const State& s, const char* name) override { import_enter(s, name); }
    bool import_fixture(State& s, const char* name) override {
        if (!returning || std::string_view(name) != "CarbonLib::ExitToShell") return false;
        s.wr<4>(12, 0x12345678); s.pc = std::uint32_t(s.r<4>(4128)); return true;
    }
};
using Mutation = decltype(Effects::on_read);
using Expect = std::function<void(const State&, const Effects&, const Imports&)>;
std::uint32_t peek(const State& s, std::uint32_t address) {
    std::uint32_t value = 0; for (auto byte : s.memory.copy_out(address, 4)) value = (value << 8) | byte; return value;
}
struct Outcome { bool terminal = false; std::string error; bool operator==(const Outcome&) const = default; };
Outcome execute(State& s) {
    try { run(s); }
    catch (const ApplicationExit&) { return {true, {}}; }
    catch (const std::exception& e) { return {false, e.what()}; }
    return {};
}
void check(std::unique_ptr<State> original, unsigned index, const std::string& label,
        bool returning = false, bool fault = false, const Mutation& mutation = {}, const Expect& expected = {},
        const std::array<bool, 6>& selected_original = {}) {
    auto modern = clone(*original); original->runtime_lifecycle_original = true;
    modern->runtime_lifecycle_entry_original = selected_original; Imports ia, ib; ia.returning = ib.returning = returning;
    Effects ea, eb; ea.state = original.get(); eb.state = modern.get(); ea.on_read = eb.on_read = mutation;
    original->runtime_lifecycle_observer = &ia; modern->runtime_lifecycle_observer = &ib;
    original->memory.date_observer = &ea; modern->memory.date_observer = &eb;
    const auto ax = execute(*original), bx = execute(*modern);
    original->runtime_lifecycle_observer = modern->runtime_lifecycle_observer = nullptr;
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    require(ax == bx, label + " outcome original=" + ax.error + " semantic=" + bx.error);
    require(fault == !ax.error.empty(), label + " fault expectation " + ax.error);
    if (fault) require(ax.error.starts_with("Unmapped data access at "), label + " mapping fault");
    Image(*original).compare(*modern, label); require(ea.bytes == eb.bytes, label + " ordered effects");
    require(ia.records == ib.records && ea.callbacks == eb.callbacks, label + " callback/import history");
    if (expected) expected(*modern, eb, ib);
    require(!modern->runtime_lifecycle_dispatch_escapes, label + " dispatch escape");
    for (unsigned n = 0; n < 6; ++n) {
        if (selected_original[n])
            require(modern->runtime_lifecycle_original_calls[n] && !modern->runtime_lifecycle_calls[n], label + " mixed original witness");
        else require(!modern->runtime_lifecycle_original_calls[n], label + " unexpected original call");
        if (!fault) witnesses[n] += modern->runtime_lifecycle_calls[n];
    }
    if (!fault && !selected_original[index]) {
        require(modern->runtime_lifecycle_calls[index], label + " production root witness");
        require(modern->runtime_lifecycle_returns[index] + modern->runtime_lifecycle_terminations[index] == 1,
                label + " successful root boundary");
        require(bool(modern->runtime_lifecycle_terminations[index]) == bx.terminal, label + " termination diagnostic");
    }
    ++cases; faults += fault; terminations += bx.terminal;
}
struct Negative : Imports {
    struct Boundary { Image image; std::vector<std::uint8_t> effects; std::vector<Record> imports; bool terminal; };
    std::uint32_t target;
    std::vector<Boundary> reference;
    Effects* effects = nullptr;
    bool mutated = false;
    unsigned parent_boundaries = 0;
    explicit Negative(std::uint32_t entry) : target(entry) {}
    void leave(State& s, std::uint32_t entry, bool original, bool terminal) override {
        if (entry != target) { if (mutated) ++parent_boundaries; return; }
        if (original) { reference.push_back({Image(s), effects->bytes, records, terminal}); return; }
        require(!reference.empty(), "Missing lifecycle negative reference boundary");
        const auto& expected = reference.front();
        expected.image.compare(s, "Unmutated lifecycle boundary " + hex_address(entry));
        require(expected.effects == effects->bytes && expected.imports == records && expected.terminal == terminal,
                "Lifecycle return/termination boundary effect/import prefix");
        s.wr<4>(12, s.r<4>(12) ^ 1); mutated = true;
        expected.image.compare(s, "Own lifecycle boundary " + hex_address(entry));
    }
};
void negative(std::unique_ptr<State> original, std::uint32_t target, bool returning) {
    auto modern = clone(*original); original->runtime_lifecycle_original = true;
    Effects ea, eb; ea.state = original.get(); eb.state = modern.get();
    Negative observer(target); observer.returning = returning; observer.effects = &ea;
    original->memory.date_observer = &ea; original->runtime_lifecycle_observer = &observer;
    const auto reference = execute(*original); require(reference.error.empty() && !observer.reference.empty(), "Unreached lifecycle own/child boundary");
    observer.records.clear(); observer.effects = &eb;
    modern->memory.date_observer = &eb; modern->runtime_lifecycle_observer = &observer;
    const auto outcome = execute(*modern);
    original->runtime_lifecycle_observer = modern->runtime_lifecycle_observer = nullptr;
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    require(outcome.error.starts_with("Own lifecycle boundary " + hex_address(target)) && observer.mutated && !observer.parent_boundaries,
            "Own/child lifecycle mutation escaped its boundary: " + outcome.error);
    ++negatives;
}
std::unique_ptr<State> sparse(const State& initial, std::uint64_t omitted = UINT64_MAX) {
    auto probe = clone(initial); probe->runtime_lifecycle_original = true; Effects effects; effects.state = probe.get(); Imports imports;
    probe->memory.date_observer = &effects; probe->runtime_lifecycle_observer = &imports;
    const auto outcome = execute(*probe); require(outcome.error.empty(), "Sparse reference failed");
    probe->memory.date_observer = nullptr; probe->runtime_lifecycle_observer = nullptr;
    auto result = std::make_unique<State>(); result->registers = initial.registers; result->unique = initial.unique;
    result->pc = initial.pc; result->budget = initial.budget; result->heap_end = initial.heap_end;
    result->allocated = initial.allocated; result->available = initial.available; result->memory_error = initial.memory_error;
    for (auto [begin, length] : effects.accesses) for (auto [address, size] : initial.memory.mapped_ranges()) {
        const auto low = std::max(std::uint64_t(begin), std::uint64_t(address));
        const auto high = std::min(std::uint64_t(begin) + length, std::uint64_t(address) + size);
        if (low >= high) continue;
        if (low <= omitted && omitted < high) {
            if (low < omitted) result->memory.map(std::uint32_t(low), omitted - low);
            if (omitted + 1 < high) result->memory.map(std::uint32_t(omitted + 1), high - omitted - 1);
        } else result->memory.map(std::uint32_t(low), high - low);
    }
    for (auto [address, size] : result->memory.mapped_ranges()) result->memory.copy_in(address, initial.memory.copy_out(address, size));
    return result;
}
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        for (unsigned i = 0; i < 6; ++i) for (bool returning : {false, true})
            check(setup(argv[1], i), i, "Direct lifecycle " + std::to_string(i), returning);
        for (unsigned i = 0; i < 6; ++i) {
            auto s = setup(argv[1], i); s->memory.write<4>(fixture + 0x5000, entries[i]);
            s->memory.write<4>(fixture + 0x5004, toc); s->wr<4>(48, fixture + 0x5000); s->pc = 0x10781320;
            check(std::move(s), i, "Generated indirect lifecycle " + std::to_string(i));
            negative(setup(argv[1], i), entries[i], true);
            auto original = setup(argv[1], i); original->runtime_lifecycle_original = true;
            const auto outcome = execute(*original); require(outcome.error.empty(), "Original selection without observer");
            require(original->runtime_lifecycle_original_calls[i] && !original->runtime_lifecycle_calls[i], "Original no-observer diagnostic");
        }
        for (auto [parent, child] : {std::pair{2u, 5u}, {2u, 3u}, {3u, 1u}, {3u, 4u}, {5u, 3u}, {5u, 4u}}) {
            auto fixture_for = [&] { auto s = setup(argv[1], parent); if (parent == 5) { s->wr<4>(12, 2); s->memory.write<4>(handlers + 4, 0); } return s; };
            negative(fixture_for(), entries[child], parent != 5);
            std::array<bool, 6> selected{}; selected[child] = true;
            check(fixture_for(), parent, "Mixed original child " + std::to_string(parent) + "/" + std::to_string(child), false, false, {}, {}, selected);
            selected.fill(false); selected[parent] = true;
            check(fixture_for(), parent, "Mixed original parent " + std::to_string(parent) + "/" + std::to_string(child), false, false, {}, {}, selected);
        }
        for (unsigned i : {2u, 3u, 4u, 5u}) {
            auto s = setup(argv[1], i); if (i == 5) { s->wr<4>(12, 2); s->memory.write<4>(handlers + 4, 0); }
            negative(std::move(s), entries[i], false);
        }
        for (auto [parent, child] : {std::pair{2u, 3u}, {2u, 4u}, {3u, 4u}})
            negative(setup(argv[1], parent), entries[child], false);
        for (unsigned n = 0; n <= 6; ++n) {
            auto s = setup(argv[1], 1); s->memory.write<4>(head, n ? nodes + (n - 1) * 12 : 0);
            check(std::move(s), 1, "Destructor LIFO " + std::to_string(n), false, false, {}, [n](const State& s, const Effects& effects, const Imports&) {
                require(!s.memory.read<4>(head) && effects.callbacks.size() == n, "Drained destructor chain");
                for (unsigned k = 0; k < n; ++k) require(effects.callbacks[k][0] == 0x200 + n - k - 1 && effects.callbacks[k][1] == UINT32_MAX, "Destructor object and deleting flag");
            });
            auto t = setup(argv[1], 4); t->memory.write<4>(toc - 0x3514, n); t->memory.write<4>(terminal, vectors + 5 * 8);
            check(std::move(t), 4, "Exit reverse callbacks " + std::to_string(n), false, false, {}, [n](const State& s, const Effects& effects, const Imports& imports) {
                require(!s.memory.read<4>(toc - 0x3514) && !s.memory.read<4>(terminal), "Exit count and terminal cell cleared");
                require(effects.callbacks.size() == n + 1 && imports.records.size() == 1, "Native termination prefix");
                for (unsigned k = 0; k < n; ++k) require(effects.callbacks[k][3] == vectors + (n - k - 1) * 8, "Exit callback LIFO");
            });
        }
        for (std::uint32_t flag : {0u, 1u, UINT32_MAX, 0x80000000u}) {
            auto s = setup(argv[1], 3); s->memory.write<4>(head, nodes + 24); s->memory.write<4>(stream, vectors + 24);
            s->memory.write<4>(terminal, vectors + 32); s->memory.write<4>(toc - 0x3514, 2); s->memory.write<4>(skip, flag);
            check(std::move(s), 3, "Exit skip cleanup " + std::to_string(flag), false, false, {}, [flag](const State& s, const Effects& effects, const Imports&) {
                require(s.memory.read<4>(head) == (flag ? nodes + 24 : 0) && s.memory.read<4>(stream) == (flag ? vectors + 24 : 0), "Exit selective cleanup");
                require(effects.callbacks.size() == (flag ? 3u : 7u), "Exit cleanup callback prefix");
            });
        }
        for (std::uint32_t count : {UINT32_MAX, 0x80000000u}) {
            auto s = setup(argv[1], 4); s->memory.write<4>(toc - 0x3514, count);
            check(std::move(s), 4, "Signed nonpositive callback count", false, false, {}, [count](const State& s, const Effects& e, const Imports&) {
                require(s.memory.read<4>(toc - 0x3514) == count && e.callbacks.empty(), "Signed count guard skips callback table");
            });
        }
        for (std::uint32_t signal : {0u, 1u, 2u, 3u, 4u, 5u, 6u, 7u, UINT32_MAX, 0x80000000u})
            for (std::uint32_t handler : {0u, 1u, vectors}) {
                auto s = setup(argv[1], 5); s->wr<4>(12, signal);
                if (signal >= 1 && signal <= 6) s->memory.write<4>(handlers + (signal - 1) * 4, handler);
                check(std::move(s), 5, "Raise " + std::to_string(signal) + "/" + std::to_string(handler), false, false, {}, [signal, handler](const State& s, const Effects& effects, const Imports&) {
                    if (signal < 1 || signal > 6) require(s.r<4>(12) == UINT32_MAX && effects.callbacks.empty(), "Invalid signal without callback");
                    else {
                        require(s.memory.read<4>(handlers + (signal - 1) * 4) == (handler == 1 ? 1u : 0u), "One-shot handler reset");
                        require(effects.callbacks.size() == (handler == vectors ? 1u : 0u), "Signal handler invocation");
                    }
                });
            }
        auto abort = setup(argv[1], 2); abort->memory.write<4>(head, nodes); abort->memory.write<4>(stream, vectors + 8);
        abort->memory.write<4>(terminal, vectors + 16); abort->memory.write<4>(toc - 0x3514, 2);
        abort->memory.write<4>(handlers, vectors + 24);
        check(std::move(abort), 2, "Abort handler then skip globals", false, false, {}, [](const State& s, const Effects& e, const Imports&) {
            require(s.memory.read<4>(skip) == 1 && s.memory.read<4>(head) == nodes && s.memory.read<4>(stream) == vectors + 8, "Abort suppression");
            require(e.callbacks.size() == 4 && e.callbacks.front()[0] == 1, "Abort signal and tail callbacks");
        });
        auto destructors = setup(argv[1], 1); destructors->memory.write<4>(head, nodes);
        destructors->memory.write<4>(nodes + 12, 0);
        check(std::move(destructors), 1, "Destructor live head insertion", false, false,
            [](State& s, Effects& e, std::uint32_t address, unsigned) {
                if (!e.mutated && s.pc == 0x10781320 && address == vectors) {
                    require(!peek(s, head), "Unlinked before destructor callback");
                    e.mutated = true; s.memory.write<4>(head, nodes + 12);
                }
            }, [](const State& s, const Effects& e, const Imports&) {
                require(e.mutated && e.callbacks.size() == 2 && !s.memory.read<4>(head), "Inserted destructor reread mutated=" + std::to_string(e.mutated) + " callbacks=" + std::to_string(e.callbacks.size()) + " head=" + hex_address(s.memory.read<4>(head)));
            });
        auto callbacks = setup(argv[1], 4); callbacks->memory.write<4>(toc - 0x3514, 1);
        check(std::move(callbacks), 4, "Callback live count repopulation", false, false,
            [](State& s, Effects& e, std::uint32_t address, unsigned) {
                if (!e.mutated && s.pc == 0x10781320 && address == vectors) {
                    require(!peek(s, toc - 0x3514), "Count published before callback");
                    e.mutated = true; s.memory.write<4>(toc - 0x3514, 2);
                }
            }, [](const State& s, const Effects& e, const Imports&) {
                require(e.mutated && e.callbacks.size() == 3 && !s.memory.read<4>(toc - 0x3514), "Repopulated exit callbacks reread");
            });
        auto signal = setup(argv[1], 5); signal->memory.write<4>(handlers, vectors);
        check(std::move(signal), 5, "Handler reinstallation retained", false, false,
            [](State& s, Effects& e, std::uint32_t address, unsigned) {
                if (!e.mutated && s.pc == 0x10781320 && address == vectors) {
                    require(!peek(s, handlers), "Handler reset before invocation");
                    e.mutated = true; s.memory.write<4>(handlers, vectors + 8);
                }
            }, [](const State& s, const Effects&, const Imports&) { require(s.memory.read<4>(handlers) == vectors + 8, "Handler callback reinstallation"); });
        auto stream_exit = setup(argv[1], 3);
        stream_exit->memory.write<4>(stream, vectors); stream_exit->memory.write<4>(vectors, 0x107753b0);
        stream_exit->memory.write<4>(vectors + 4, toc); stream_exit->memory.write<4>(toc - 0x5a68, 0);
        check(std::move(stream_exit), 3, "Real semantic stream cleanup composition", false, false, {}, [](const State& s, const Effects&, const Imports&) {
            require(s.stream_buffer_calls[2] && !s.stream_buffer_original_calls[2] && !s.stream_buffer_dispatch_escapes, "Production dispose-all dependency witness");
        });
        for (unsigned offset : {0u, 4u, 8u}) {
            auto s = setup(argv[1], 0); s->memory.write<4>(toc - 0x5ab0, nodes + offset);
            check(std::move(s), 0, "Destructor registration head alias " + std::to_string(offset));
        }
        for (unsigned i = 0; i < 6; ++i) {
            auto s = setup(argv[1], i); const auto relocated = fixture + 0x40000;
            s->memory.map(relocated - 0x9000, 0xb000);
            const auto ranges = s->memory.mapped_ranges();
            for (auto [address, size] : ranges) {
                const auto low = std::max(std::uint64_t(address), std::uint64_t(toc - 0x9000));
                const auto high = std::min(std::uint64_t(address) + size, std::uint64_t(toc + 0x2000));
                if (low < high) s->memory.copy_in(std::uint32_t(relocated + low - toc), s->memory.copy_out(std::uint32_t(low), high - low));
            }
            s->wr<4>(8, relocated); check(std::move(s), i, "Relocated TOC " + std::to_string(i));
        }
        for (unsigned i : {0u, 1u, 2u, 3u, 4u, 5u}) {
            auto s = setup(argv[1], i); if (i == 1) s->memory.write<4>(head, nodes);
            if (i == 5) s->memory.write<4>(handlers, vectors);
            check(sparse(*s), i, "Minimal mapping " + std::to_string(i));
            for (std::uint64_t address : {std::uint64_t(i == 2 ? stack + 8 : stack - 4), std::uint64_t(toc - (i == 0 || i == 1 ? 0x5ab0 : i == 5 ? 0x3b3c : i == 4 ? 0x3b78 : 0x5a7c))}) {
                if (i == 0 && address == stack - 4) continue;
                check(sparse(*s, address), i, "Mapping fault " + std::to_string(i) + "/" + hex_address(address), false, true);
            }
        }
        for (auto [i, omitted] : {std::pair{0u, nodes}, {0u, nodes + 4}, {0u, nodes + 8},
                {1u, nodes}, {1u, nodes + 4}, {1u, nodes + 8}, {1u, vectors}, {1u, vectors + 4},
                {3u, stream}, {3u, vectors + 4}, {4u, toc - 0x3514}, {4u, table}, {4u, vectors + 4},
                {5u, handlers}, {5u, vectors}, {5u, vectors + 4}}) {
            auto s = setup(argv[1], i);
            if (i == 1) s->memory.write<4>(head, nodes);
            if (i == 3) s->memory.write<4>(stream, vectors);
            if (i == 4) s->memory.write<4>(toc - 0x3514, 1);
            if (i == 5) s->memory.write<4>(handlers, vectors);
            check(sparse(*s, omitted), i, "Callback/record fault prefix " + std::to_string(i) + "/" + hex_address(omitted), false, true);
        }
        auto impossible_return = setup(argv[1], 5); impossible_return->wr<4>(12, 2); impossible_return->memory.write<4>(handlers + 4, 0);
        check(std::move(impossible_return), 5, "Default signal dependency-return null-handler fault", true, true);
        for (unsigned i = 0; i < 6; ++i) {
            auto s = setup(argv[1], i); s->runtime_lifecycle_original = true; s->runtime_lifecycle_entry_original.fill(true);
            s->runtime_lifecycle_calls.fill(9); s->runtime_lifecycle_original_calls.fill(9); s->runtime_lifecycle_returns.fill(9);
            s->runtime_lifecycle_terminations.fill(9); s->runtime_lifecycle_dispatch_escapes = 9; s->runtime_lifecycle_original_resume = entries[i];
            Imports observer; s->runtime_lifecycle_observer = &observer; Image before(*s); reset_runtime_lifecycle_diagnostics(*s); before.compare(*s, "Diagnostic reset architectural isolation");
            require(!s->runtime_lifecycle_original && !s->runtime_lifecycle_observer && !s->runtime_lifecycle_original_resume && !s->runtime_lifecycle_dispatch_escapes, "Reset lifecycle scalar diagnostics");
            for (unsigned n = 0; n < 6; ++n) require(!s->runtime_lifecycle_entry_original[n] && !s->runtime_lifecycle_calls[n] && !s->runtime_lifecycle_original_calls[n] && !s->runtime_lifecycle_returns[n] && !s->runtime_lifecycle_terminations[n], "Reset lifecycle entry diagnostics");
        }
        for (unsigned i = 0; i < 6; ++i) { require(witnesses[i], "Missing lifecycle production witness"); std::cout << "runtime_lifecycle.entry=" << hex_address(entries[i]) << " modern=" << witnesses[i] << '\n'; }
        std::cout << cases << " lifecycle production comparisons; " << faults << " faults; " << terminations << " native terminations; " << negatives << " own/child return/termination negatives\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
