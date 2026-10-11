#include "code_fragment_internal.hpp"
#include "code_fragment_adapter.hpp"
#include "semantic_io_checks.hpp"
using namespace cm::semantic_io_checks;
namespace {
constexpr auto entries = code_fragment::entries;
constexpr std::uint32_t node = fixture + 0x1000, tables = fixture + 0x2000,
    descriptors = fixture + 0x3000, output = fixture + 0x4000, frame = fixture + 0x6000,
    base = 0x10000000;
unsigned cases = 0, faults = 0, negatives = 0;
std::array<std::uint64_t, 5> witnesses{};
void args(State& s, unsigned i, std::initializer_list<std::uint32_t> values) {
    unsigned n = 3; for (auto value : values) s.wr<4>(n++ * 4, value);
    s.pc = entries[i]; s.wr<4>(4128, return_sentinel); s.budget = 100000;
}
std::unique_ptr<State> setup(const char* program, unsigned i) {
    auto s = std::make_unique<State>(); load_program(*s, program);
    s->memory.map(fixture, 0x10000); s->memory.clear(fixture, 0x10000);
    s->memory.copy_in(stack - 8192, std::vector<std::uint8_t>(8256, 0xa7));
    s->registers.fill(0x5b); s->unique.fill(0x6d);
    for (unsigned n = 0; n < 32; ++n) s->wr<4>(n * 4, 0xcafe0000 + n);
    s->wr<4>(4, stack); s->wr<4>(8, toc); s->wr<4>(2440, 0xbadc0ffe); s->wr<4>(4132, 0x31415926);
    s->wr<1>(2304, i % 2 ? 0xf9 : 0xa6); s->wr<1>(1024, i % 4); s->wr<1>(1027, (i / 2) % 2);
    s->memory.write<1>(toc - 0x3529, 1); s->memory.write<1>(toc - 0x352a, 0);
    s->memory.write<4>(toc - 0x3524, node); s->memory.write<4>(toc - 0x3528, 0);
    for (auto [offset, value] : {std::pair{0u, tables}, {4u, tables + 24}, {8u, base},
            {12u, base + 0x1000}, {16u, descriptors}, {20u, 0u}, {24u, toc}}) s->memory.write<4>(node + offset, value);
    for (unsigned k = 0; k < 3; ++k) {
        s->memory.write<4>(tables + k * 12, 0x100 + k * 0x100);
        s->memory.write<4>(tables + k * 12 + 4, 0x40); s->memory.write<4>(tables + k * 12 + 8, k * 0x100);
        s->memory.write<2>(descriptors + k * 0x100 + 2, 0x10);
        s->memory.write<2>(descriptors + k * 0x100 + 4, 0x30);
        s->memory.write<2>(descriptors + k * 0x100 + 6, 0x80);
        s->memory.write<1>(descriptors + k * 0x100 + 0x80, 0x93);
    }
    s->memory.write<4>(output, descriptors); s->memory.write<4>(output + 8, descriptors + 0x80);
    s->memory.write<4>(frame - 4, 0xfade1234);
    if (i == 0) args(*s, i, {base + 0x5000, base + 0x6000, descriptors, 0, tables, tables + 24, toc});
    else if (i == 1) args(*s, i, {base + 0x130});
    else if (i == 2) args(*s, i, {base + 0x130, output});
    else if (i == 3) args(*s, i, {frame, output});
    else args(*s, i, {output});
    return s;
}
struct Imports : CodeFragmentBoundaryObserver {
    struct Record { std::string name; decltype(State::registers) registers; std::uint32_t pc;
        std::uint64_t calls; bool operator==(const Record&) const = default; };
    std::vector<Record> records;
    std::optional<std::uint32_t> gestalt;
    std::uint32_t cpu = 0x10;
    bool fail_allocation = false;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override { records.push_back({name, s.registers, s.pc, s.native_calls}); }
    void import_leave(const State& s, const char* name) override { import_enter(s, name); }
    bool import_fixture(State& s, const char* name) override {
        if (gestalt && std::string_view(name) == "CarbonLib::Gestalt") {
            require(s.r<4>(12) == 0x70706366, "Original ppcf selector");
            if (!*gestalt) s.memory.write<4>(std::uint32_t(s.r<4>(16)), cpu);
            s.wr<4>(12, *gestalt); s.pc = std::uint32_t(s.r<4>(4128)); return true;
        }
        if (fail_allocation && std::string_view(name) == "CarbonLib::NewPtrClear") {
            require(s.r<4>(12) == 0x384, "Original code-fragment block size");
            s.wr<4>(12, 0); s.memory_error = -108; s.pc = std::uint32_t(s.r<4>(4128)); return true;
        }
        return false;
    }
};
using Expect = std::function<void(const State&, const Effects&, const Imports&)>;
void check(std::unique_ptr<State> original, unsigned i, const std::string& label,
        const Imports& model = {}, const Expect& expected = {}, bool fault = false,
        const std::array<bool, 5>& selected_original = {}) {
    auto modern = clone(*original); original->code_fragment_original = true;
    modern->code_fragment_entry_original = selected_original; Imports ia, ib; ia.gestalt = ib.gestalt = model.gestalt;
    ia.cpu = ib.cpu = model.cpu; ia.fail_allocation = ib.fail_allocation = model.fail_allocation;
    Effects ea, eb; ea.state = original.get(); eb.state = modern.get();
    original->code_fragment_observer = &ia; modern->code_fragment_observer = &ib;
    original->memory.date_observer = &ea; modern->memory.date_observer = &eb;
    std::string ax, bx;
    try { run(*original); } catch (const std::exception& e) { ax = e.what(); }
    try { run(*modern); } catch (const std::exception& e) { bx = e.what(); }
    original->code_fragment_observer = modern->code_fragment_observer = nullptr;
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    require(ax == bx, label + " outcome original=" + ax + " semantic=" + bx);
    require(fault == !ax.empty(), label + " fault expectation " + ax);
    if (fault) require(ax.starts_with("Unmapped data access at "), label + " mapping fault");
    Image(*original).compare(*modern, label); require(ea.bytes == eb.bytes, label + " ordered effects");
    require(ia.records == ib.records, label + " import ABI/history");
    if (expected) expected(*modern, eb, ib);
    require(!modern->code_fragment_dispatch_escapes, label + " dispatch escape");
    for (unsigned n = 0; n < 5; ++n) {
        if (selected_original[n]) require(modern->code_fragment_original_calls[n] && !modern->code_fragment_calls[n], label + " mixed original witness");
        else require(!modern->code_fragment_original_calls[n], label + " unexpected original call");
        if (!fault) witnesses[n] += modern->code_fragment_calls[n];
    }
    if (!fault && !selected_original[i]) require(modern->code_fragment_calls[i], label + " root production witness");
    ++cases; faults += fault;
}
struct Negative : Imports {
    struct Boundary { Image image; std::vector<std::uint8_t> effects; std::vector<Record> imports; };
    std::uint32_t target;
    std::vector<Boundary> reference;
    Effects* effects = nullptr;
    bool mutated = false;
    unsigned parent_returns = 0;
    explicit Negative(std::uint32_t entry) : target(entry) {}
    void leave(State& s, std::uint32_t entry, bool original) override {
        if (entry != target) { if (mutated) ++parent_returns; return; }
        if (original) { reference.push_back({Image(s), effects->bytes, records}); return; }
        require(!reference.empty(), "Missing fragment negative boundary"); const auto& expected = reference.front();
        expected.image.compare(s, "Unmutated fragment boundary " + hex_address(entry));
        require(expected.effects == effects->bytes && expected.imports == records, "Fragment boundary effect/import prefix");
        s.wr<4>(12, s.r<4>(12) ^ 1); mutated = true;
        expected.image.compare(s, "Own fragment boundary " + hex_address(entry));
    }
};
void negative(std::unique_ptr<State> original, std::uint32_t target) {
    auto modern = clone(*original); original->code_fragment_original = true;
    Effects ea, eb; ea.state = original.get(); eb.state = modern.get(); Negative observer(target); observer.effects = &ea;
    original->code_fragment_observer = &observer; original->memory.date_observer = &ea; run(*original);
    require(!observer.reference.empty(), "Unreached fragment negative boundary"); observer.records.clear(); observer.effects = &eb;
    modern->code_fragment_observer = &observer; modern->memory.date_observer = &eb; bool rejected = false; std::string error;
    try { run(*modern); } catch (const std::runtime_error& e) { error = e.what(); rejected = error.starts_with("Own fragment boundary " + hex_address(target)); }
    original->code_fragment_observer = modern->code_fragment_observer = nullptr;
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    require(rejected && observer.mutated && !observer.parent_returns, "Own/child fragment mutation escaped: " + error); ++negatives;
}
void composition(const char* program) {
    auto original = setup(program, 0); original->memory.write<4>(toc - 0x3524, 0);
    original->memory.write<1>(toc - 0x3529, 0); auto modern = clone(*original); original->code_fragment_original = true;
    Imports ia, ib; Effects ea, eb; ea.state = original.get(); eb.state = modern.get();
    original->code_fragment_observer = &ia; modern->code_fragment_observer = &ib;
    original->memory.date_observer = &ea; modern->memory.date_observer = &eb;
    for (unsigned step : {0u, 1u, 2u, 4u, 3u}) {
        if (step == 1) { args(*original, step, {base + 0x5130}); args(*modern, step, {base + 0x5130}); }
        else if (step == 2) { args(*original, step, {base + 0x5130, output}); args(*modern, step, {base + 0x5130, output}); }
        else if (step == 4) { args(*original, step, {output}); args(*modern, step, {output}); }
        else if (step == 3) { args(*original, step, {frame, output}); args(*modern, step, {frame, output}); }
        run(*original); run(*modern); Image(*original).compare(*modern, "Production fragment composition " + std::to_string(step));
        require(ea.bytes == eb.bytes && ia.records == ib.records, "Fragment composition cumulative effects/import history");
        original->memory.date_observer = modern->memory.date_observer = nullptr;
        if (step == 0) require(modern->r<4>(12) == 0 && modern->allocated.size() == 1, "Composition lazily allocated fragment block");
        if (step == 1) require(modern->r<4>(12) == modern->memory.read<4>(toc - 0x3524), "Composition finds newly registered range");
        if (step == 2) require(modern->memory.read<4>(output + 8) == descriptors + 0x80, "Composition resolves action metadata");
        if (step == 4) require(modern->r<4>(12) == 19, "Composition reads action kind");
        if (step == 3) require(modern->r<4>(12) == 0xfade1234, "Composition reads saved frame word");
        original->memory.date_observer = &ea; modern->memory.date_observer = &eb;
        ++cases;
    }
    original->code_fragment_observer = modern->code_fragment_observer = nullptr;
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    require(ib.records.size() == 4 && !modern->code_fragment_dispatch_escapes, "Composition uses real Gestalt and NewPtrClear");
    for (unsigned n = 0; n < 5; ++n) { require(modern->code_fragment_calls[n] && !modern->code_fragment_original_calls[n], "All-entry composition default witness"); witnesses[n] += modern->code_fragment_calls[n]; }
}
std::unique_ptr<State> sparse(const State& initial, std::uint64_t omitted = UINT64_MAX) {
    auto probe = clone(initial); probe->code_fragment_original = true; Effects effects; effects.state = probe.get(); Imports imports;
    probe->memory.date_observer = &effects; probe->code_fragment_observer = &imports; run(*probe);
    probe->memory.date_observer = nullptr; probe->code_fragment_observer = nullptr;
    auto s = std::make_unique<State>(); s->registers = initial.registers; s->unique = initial.unique;
    s->pc = initial.pc; s->budget = initial.budget; s->heap_end = initial.heap_end;
    s->allocated = initial.allocated; s->available = initial.available; s->memory_error = initial.memory_error;
    for (auto [begin, length] : effects.accesses) for (auto [address, size] : initial.memory.mapped_ranges()) {
        const auto low = std::max(std::uint64_t(begin), std::uint64_t(address));
        const auto high = std::min(std::uint64_t(begin) + length, std::uint64_t(address) + size);
        if (low >= high) continue;
        if (low <= omitted && omitted < high) {
            if (low < omitted) s->memory.map(std::uint32_t(low), omitted - low);
            if (omitted + 1 < high) s->memory.map(std::uint32_t(omitted + 1), high - omitted - 1);
        } else s->memory.map(std::uint32_t(low), high - low);
    }
    for (auto [address, size] : s->memory.mapped_ranges()) s->memory.copy_in(address, initial.memory.copy_out(address, size));
    return s;
}
}
int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        for (unsigned i = 0; i < 5; ++i) check(setup(argv[1], i), i, "Direct fragment " + std::to_string(i));
        composition(argv[1]);
        for (unsigned i = 0; i < 5; ++i) {
            auto s = setup(argv[1], i); s->memory.write<4>(fixture + 0x8000, entries[i]); s->memory.write<4>(fixture + 0x8004, toc);
            s->wr<4>(48, fixture + 0x8000); s->pc = 0x10781320;
            check(std::move(s), i, "Generated CFM fragment " + std::to_string(i)); negative(setup(argv[1], i), entries[i]);
            auto original = setup(argv[1], i); original->code_fragment_original = true; run(*original);
            require(original->code_fragment_original_calls[i] && !original->code_fragment_calls[i], "Original no-observer selection");
        }
        negative(setup(argv[1], 2), entries[1]);
        std::array<bool, 5> selected{}; selected[1] = true;
        check(setup(argv[1], 2), 2, "Mixed original lookup child", {}, {}, false, selected);
        selected.fill(false); selected[2] = true;
        check(setup(argv[1], 2), 2, "Mixed original metadata parent", {}, {}, false, selected);
        for (unsigned slot = 0; slot < 32; ++slot) {
            auto s = setup(argv[1], 0);
            for (unsigned n = 0; n < 32; ++n) s->memory.write<4>(node + n * 28 + 8, n == slot ? 0 : 0x200 + n);
            check(std::move(s), 0, "Fragment reused slot " + std::to_string(slot), {}, [slot](const State& s, const Effects&, const Imports&) {
                require(s.r<4>(12) == slot && s.memory.read<4>(node + slot * 28 + 8) == base + 0x5000, "First free slot and returned index");
            });
            auto t = setup(argv[1], 1); t->memory.clear(node, 0x384);
            t->memory.write<4>(node + slot * 28 + 8, base); t->memory.write<4>(node + slot * 28 + 12, base + 0x1000);
            check(std::move(t), 1, "Fragment lookup lane " + std::to_string(slot), {}, [slot](const State& s, const Effects&, const Imports&) {
                require(s.r<4>(12) == node + slot * 28 && s.memory.read<4>(toc - 0x3528) == node + slot * 28, "Matched slot publishes cache");
            });
        }
        for (bool empty : {false, true}) {
            auto s = setup(argv[1], 0);
            if (empty) s->memory.write<4>(toc - 0x3524, 0);
            else for (unsigned n = 0; n < 32; ++n) s->memory.write<4>(node + n * 28 + 8, n + 1);
            check(std::move(s), 0, "Real NewPtrClear registry growth", {}, [empty](const State& s, const Effects&, const Imports& imports) {
                const auto fresh = std::uint32_t(s.memory.read<4>(empty ? toc - 0x3524 : node + 0x380));
                require(fresh && s.allocated.at(fresh) == 0x384 && !s.memory.read<4>(fresh + 0x380), "Zeroed new registry block and owner map");
                require(s.r<4>(12) == (empty ? 0u : 32u) && imports.records.size() == 2, "Growth index and native allocation history");
            });
        }
        for (bool available : {false, true}) {
            auto s = setup(argv[1], 0); s->memory.write<4>(toc - 0x3524, 0);
            if (!available) s->memory.write<4>(toc - 0x7d74, 0);
            Imports model; model.fail_allocation = true;
            check(std::move(s), 0, "Unavailable/failing registry allocation", model, [available](const State& s, const Effects&, const Imports& imports) {
                require(s.r<4>(12) == UINT32_MAX && !s.memory.read<4>(toc - 0x3524), "Failed registration retains empty head");
                require(imports.records.size() == (available ? 2u : 0u), "Allocation availability gate");
            });
        }
        for (std::uint32_t status : {0u, 0x10000u, 1u, UINT32_MAX}) for (std::uint32_t mask : {0u, 0x10u}) {
            auto s = setup(argv[1], 0); s->memory.write<1>(toc - 0x3529, 0); Imports model; model.gestalt = status; model.cpu = mask;
            check(std::move(s), 0, "Gestalt original low-word status", model, [status, mask](const State& s, const Effects&, const Imports&) {
                require(s.memory.read<1>(toc - 0x3529) == 1, "Probe completion published");
                require(s.memory.read<1>(toc - 0x352a) == (!status && mask ? 1u : 0u), "Probe optional capability contract");
            });
        }
        for (bool cached : {false, true}) for (std::uint32_t address : {base - 1, base, base + 0xfff, base + 0x1000, UINT32_MAX}) {
            auto s = setup(argv[1], 1); s->wr<4>(12, address); if (cached) s->memory.write<4>(toc - 0x3528, node);
            check(std::move(s), 1, "Region boundary cache " + std::to_string(cached), {}, [address](const State& s, const Effects&, const Imports&) {
                require(s.r<4>(12) == (address >= base && address < base + 0x1000 ? node : 0u), "Unsigned fragment half-open bounds");
            });
        }
        for (unsigned format : {0u, 4u, 8u}) for (unsigned relative : {0u, 0x10u, 0x30u, 0x40u}) {
            auto s = setup(argv[1], 2); s->wr<4>(12, base + 0x100 + relative); s->memory.clear(descriptors, 0x100);
            s->memory.write<1>(descriptors + 1, format);
            if (format == 8) { s->memory.write<4>(descriptors + 4, 0x10); s->memory.write<2>(descriptors + 8, 8); s->memory.write<2>(descriptors + 10, 0x80); }
            else { const auto list = descriptors + (format ? 4 : 2); s->memory.write<2>(list, 0x10); s->memory.write<2>(list + 2, 0x30); s->memory.write<2>(list + 4, 0x80); }
            check(std::move(s), 2, "Metadata action format and inclusive boundary", {}, [relative](const State& s, const Effects&, const Imports&) {
                require(s.memory.read<4>(output) == descriptors && s.memory.read<4>(output + 4) == base + 0x100, "Metadata descriptor/function base");
                require(s.memory.read<4>(output + 8) == (relative >= 0x10 && relative <= 0x30 ? descriptors + 0x80 : 0u), "Inclusive action range and sentinel");
            });
        }
        auto inline_info = setup(argv[1], 2); inline_info->memory.write<4>(tables + 4, 0x80000040); inline_info->memory.write<4>(tables + 8, 0);
        check(std::move(inline_info), 2, "Inline exception descriptor", {}, [](const State& s, const Effects&, const Imports&) {
            require(s.memory.read<4>(output) == tables + 8 && !s.memory.read<4>(output + 8), "Inline descriptor bypasses external base");
        });
        for (auto [header, delta] : {std::pair{0u, 0u}, {0x40u, 16u}, {0x80u, 16u}, {0x200u, 64u}}) {
            auto s = setup(argv[1], 3); s->memory.write<2>(descriptors, header); s->memory.write<4>(frame - delta - 4, 0xabcddcba);
            check(std::move(s), 3, "Saved frame word header", {}, [](const State& s, const Effects&, const Imports&) { require(s.r<4>(12) == 0xabcddcba, "Saved frame word selection"); });
        }
        for (unsigned kind : {0u, 1u, 0x7fu, 0x80u, 0xffu}) {
            auto s = setup(argv[1], 4); s->memory.write<1>(descriptors + 0x80, kind);
            check(std::move(s), 4, "Action low seven bits", {}, [kind](const State& s, const Effects&, const Imports&) { require(s.r<4>(12) == (kind & 0x7f), "Action kind strips high flag"); });
        }
        auto absent = setup(argv[1], 4); absent->memory.write<4>(output + 8, 0); check(std::move(absent), 4, "Null action guarded");
        for (bool cached : {false, true}) {
            auto s = setup(argv[1], 1); s->memory.clear(node, 0x384); s->memory.write<4>(node + 12, 100); s->wr<4>(12, 17);
            if (cached) s->memory.write<4>(toc - 0x3528, node);
            check(std::move(s), 1, "Cached zero-start legacy asymmetry", {}, [cached](const State& s, const Effects&, const Imports&) {
                require(s.r<4>(12) == (cached ? node : 0u), "Cached zero-start versus cold free-slot policy");
            });
        }
        for (std::uint32_t end : {tables - 12, tables + 24}) {
            auto s = setup(argv[1], 2); s->memory.copy_in(output, std::vector<std::uint8_t>(24, 0xa5));
            s->memory.write<4>(node + 4, end); if (end == tables + 24) s->wr<4>(12, base + 0x1000);
            check(std::move(s), 2, "Missing metadata retains unrelated output fields", {}, [end](const State& s, const Effects&, const Imports&) {
                require(!s.memory.read<4>(output) && !s.memory.read<4>(output + 8) && s.memory.read<4>(output + 4) == 0xa5a5a5a5, "Only metadata/action pointers cleared on miss");
                require(s.memory.read<4>(output + 12) == (end == tables - 12 ? base : 0xa5a5a5a5), "Fragment fields published only when its region was found");
            });
        }
        for (auto [alias, fault] : {std::pair{tables, false}, {descriptors, false}, {node + 16, true}}) {
            auto s = setup(argv[1], 2); s->wr<4>(16, alias);
            check(std::move(s), 2, "Live metadata/output alias " + hex_address(alias), {}, {}, fault);
        }
        for (unsigned i = 0; i < 5; ++i) {
            auto s = setup(argv[1], i); const auto relocated = fixture + 0x40000; s->memory.map(relocated - 0x9000, 0xb000);
            const auto ranges = s->memory.mapped_ranges();
            for (auto [address, size] : ranges) {
                const auto low = std::max(std::uint64_t(address), std::uint64_t(toc - 0x9000));
                const auto high = std::min(std::uint64_t(address) + size, std::uint64_t(toc + 0x2000));
                if (low < high) s->memory.copy_in(std::uint32_t(relocated + low - toc), s->memory.copy_out(std::uint32_t(low), high - low));
            }
            s->wr<4>(8, relocated); check(std::move(s), i, "Relocated fragment TOC " + std::to_string(i));
        }
        for (unsigned i = 0; i < 5; ++i) {
            auto s = setup(argv[1], i); check(sparse(*s), i, "Minimal fragment mapping " + std::to_string(i));
            const auto omitted = i == 0 ? stack - 36 : i == 1 ? toc - 0x3528 : i == 2 ? output : i == 3 ? frame - 4 : output + 8;
            check(sparse(*s, omitted), i, "Fragment mapping fault " + std::to_string(i), {}, {}, true);
        }
        for (auto [i, omitted] : {std::pair{0u, node + 8}, {0u, node + 28 + 8}, {0u, node + 28 + 12},
                {1u, node + 8}, {1u, node + 12}, {2u, output + 8}, {2u, output + 12}, {2u, tables},
                {2u, tables + 4}, {2u, tables + 8}, {2u, descriptors + 1}, {2u, descriptors + 2},
                {2u, descriptors + 4}, {2u, descriptors + 6}, {3u, output}, {3u, descriptors}, {4u, descriptors + 0x80}})
            { auto s = setup(argv[1], i); check(sparse(*s, omitted), i, "Metadata/record ordered fault prefix " + hex_address(omitted), {}, {}, true); }
        for (unsigned i = 0; i < 5; ++i) {
            auto s = setup(argv[1], i); s->code_fragment_original = true; s->code_fragment_entry_original.fill(true);
            s->code_fragment_calls.fill(9); s->code_fragment_original_calls.fill(9); s->code_fragment_dispatch_escapes = 9;
            s->code_fragment_original_resume = entries[i]; Imports observer; s->code_fragment_observer = &observer;
            Image before(*s); reset_code_fragment_diagnostics(*s); before.compare(*s, "Fragment reset architectural isolation");
            require(!s->code_fragment_original && !s->code_fragment_observer && !s->code_fragment_original_resume && !s->code_fragment_dispatch_escapes, "Reset fragment scalar diagnostics");
            for (unsigned n = 0; n < 5; ++n) require(!s->code_fragment_entry_original[n] && !s->code_fragment_calls[n] && !s->code_fragment_original_calls[n], "Reset fragment entry diagnostics");
        }
        for (unsigned i = 0; i < 5; ++i) { require(witnesses[i], "Missing fragment production witness"); std::cout << "code_fragment.entry=" << hex_address(entries[i]) << " modern=" << witnesses[i] << '\n'; }
        std::cout << cases << " code-fragment production comparisons; " << faults << " faults; " << negatives << " own/child negatives\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
