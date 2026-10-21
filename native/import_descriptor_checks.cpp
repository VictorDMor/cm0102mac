#include "import_descriptor_adapter.hpp"
#include "code_fragment_adapter.hpp"
#include "semantic_io_checks.hpp"
#include "match_turbo.hpp"
#include <numeric>
#include <set>

namespace cm::aot { bool import_descriptor_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace descriptor = cm::aot::import_descriptor;
constexpr auto count = descriptor::entry_count;
constexpr std::uint32_t relocated_toc = 0x31008000, caller = 0x7fff1000,
    record = fixture + 0x100, alternate = fixture + 0x200, target = 0x7fff2000,
    callee_toc = 0x32008000, native_target = 280984380;
enum class Path { component, adapter, generated, composed };
enum class Mode { modern, entry, global, unrelated, profile };
std::array<unsigned, count> comparisons{}, normal{}, negatives{}, faults{}, direct{}, generated{}, composed{}, indirect{}, selected{};
unsigned exact = 0, budget_stops = 0, downstream_faults = 0;
std::uint64_t total(const auto& values) { return std::accumulate(values.begin(), values.end(), UINT64_C(0)); }
struct Monitor final : MemoryObserver {
    Effects effects;
    std::vector<decltype(State::registers)> registers;
    std::function<void(State&, std::uint32_t)> on_write;
    void mapped(std::uint32_t address, std::uint64_t size) override { effects.mapped(address, size); }
    void cleared(std::uint32_t address, std::uint64_t size) override { effects.cleared(address, size); }
    void read(std::uint32_t address, const std::uint8_t* data, unsigned size) override {
        registers.push_back(effects.state->registers); effects.read(address, data, size);
    }
    void written(std::uint32_t address, const std::uint8_t* data, std::size_t size) override {
        registers.push_back(effects.state->registers); effects.written(address, data, size);
        if (on_write) on_write(*effects.state, address);
    }
};
struct Boundary final : ImportDescriptorBoundaryObserver {
    bool corrupt = false;
    std::vector<std::pair<std::uint32_t, bool>> entered;
    std::vector<ImportDescriptorOutcome> outcomes;
    void enter(const State&, std::uint32_t entry, bool original) override { entered.emplace_back(entry, original); }
    void leave(State& s, std::uint32_t, bool original, ImportDescriptorOutcome outcome) override {
        outcomes.push_back(outcome);
        if (corrupt && !original && outcome == ImportDescriptorOutcome::tailed) s.pc ^= 1;
    }
};
struct NativeFixture final : CodeFragmentBoundaryObserver {
    bool fail = false;
    std::vector<std::string> names;
    std::vector<decltype(State::registers)> arguments;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    bool import_fixture(State& s, const char* name) override {
        names.emplace_back(name); arguments.push_back(s.registers);
        require(names.back() == "CarbonLib::GetDateTime", "Controlled import destination");
        if (fail) throw std::runtime_error("Controlled downstream import failure");
        s.memory.write<4>(s.r<4>(12), 0x12345678); s.pc = std::uint32_t(s.r<4>(4128)); return true;
    }
};
std::unique_ptr<State> fixture_state(unsigned index, unsigned alignment = 0) {
    auto s = std::make_unique<State>(); s->registers.fill(0x6d); s->unique.fill(0xb3);
    for (unsigned n = 0; n < 32; ++n) {
        s->wr<4>(n * 4, 0x2468a000 + n * 0x101);
        s->wr<8>(0x4000 + n * 16, UINT64_C(0xfff0000100000000) + n);
    }
    for (unsigned n = 0; n < 8; ++n) s->wr<1>(2304 + n, 0xa0 | n);
    s->wr<4>(4, stack + alignment); s->wr<4>(8, relocated_toc + alignment);
    s->wr<4>(48, record + alignment); s->wr<4>(4128, caller); s->wr<4>(4132, 0x31415926);
    for (auto [address, length] : std::array<std::pair<std::uint32_t, unsigned>, 3>{
            {{fixture, 0x1000}, {stack - 512, 1024}, {relocated_toc - 0x8000, 0x10000}}}) {
        s->memory.map(address, length); s->memory.copy_in(address, std::vector<std::uint8_t>(length, 0xa7));
    }
    if (index < descriptor::import_count)
        s->memory.write<4>(std::uint32_t(s->r<4>(8) + descriptor::toc_displacements[index]), record + alignment);
    s->memory.write<4>(record + alignment, target); s->memory.write<4>(record + alignment + 4, callee_toc);
    s->memory.write<4>(alternate, target + 4); s->memory.write<4>(alternate + 4, callee_toc + 4);
    s->wr<4>(12, fixture + 0x300); s->pc = descriptor::entries[index]; s->executed = 37; s->budget = 50000;
    s->native_calls = 23; s->heap_end = 0x50000300; s->memory_error = -108;
    s->allocated = {{0x50000000, 64}}; s->available = {{0x50000100, 128}};
    return s;
}
void original_tail(State& s, unsigned index) {
    require(import_descriptor_reference(s), "Isolated original descriptor body");
    if (descriptor::entries[index] == 0x10780ff0) {
        require(s.pc == 0x10781000, "Original cross-page continuation");
        require(import_descriptor_reference(s), "Isolated original descriptor continuation");
    }
}
std::string execute(State& s, unsigned index, Path path, bool original) {
    try {
        if (path == Path::component) {
            if (original) original_tail(s, index); else descriptor::invoke(s, index);
        } else if (path == Path::adapter) require(try_modern_import_descriptor(s, descriptor::entries[index]), "Direct descriptor adapter");
        else if (path == Path::composed) run_until(s, caller);
        else {
            require(dispatch_page(s), "Generated descriptor destination");
        }
    } catch (const std::runtime_error& e) { return e.what(); }
    return {};
}
using Mutation = std::function<void(Monitor&)>;
using Expectation = std::function<void(const State&)>;
struct Evidence { std::vector<Effects::Access> accesses; };
Evidence compare(unsigned index, const State& initial, const std::string& name, Path path = Path::component,
        const Mutation& mutate = {}, const Expectation& expect = {}, const std::string& expected_error = {},
        Mode mode = Mode::modern, bool negative = false, bool native_fail = false) {
    auto a = clone(initial), b = clone(initial);
    a->import_descriptor_original = true;
    if (mode == Mode::entry) b->import_descriptor_entry_original[index] = true;
    if (mode == Mode::global) b->import_descriptor_original = true;
    if (mode == Mode::unrelated) b->import_descriptor_entry_original[(index + 1) % count] = true;
    Monitor x, y; x.effects.state = a.get(); y.effects.state = b.get();
    if (mutate) { mutate(x); mutate(y); }
    Boundary boundary; boundary.corrupt = negative;
    NativeFixture old_native, new_native; old_native.fail = new_native.fail = native_fail;
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    a->code_fragment_observer = &old_native; b->code_fragment_observer = &new_native;
    if (path != Path::component) b->import_descriptor_observer = &boundary;
    const auto ax = execute(*a, index, path, true), bx = execute(*b, index, path, false);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr; b->import_descriptor_observer = nullptr;
    const auto label = hex_address(descriptor::entries[index]) + "/" + name;
    // "*" accepts completion or a budget stop, as long as both sides agree.
    require(ax == bx && (expected_error == "*" ? ax.empty() || ax == "Native instruction budget exhausted"
                                               : expected_error.empty() ? ax.empty() : ax.starts_with(expected_error)),
            label + " original=" + ax + " modern=" + bx);
    if (negative) {
        bool detected = false;
        try {
            if (path == Path::component) b->wr<4>(4132, b->r<4>(4132) ^ 1);
            Image(*a).compare(*b, label);
        } catch (const std::runtime_error&) { detected = true; }
        require(detected, label + " own negative accepted"); ++negatives[index];
    } else {
        Image(*a).compare(*b, label, !ax.empty());
        // Every access keeps its order, bytes and full register file; its PC is
        // the access's run start in generated code (block accounting).
        require(x.effects.bytes == y.effects.bytes && x.registers == y.registers, label + " ordered effects and access registers");
        require(old_native.names == new_native.names && old_native.arguments == new_native.arguments, label + " downstream import ABI");
        if (expect) expect(*b);
    }
    // Counts after a fault or budget stop are diagnostics (2026-10-20).
    if (ax.empty()) { require(a->executed == b->executed, label + " exact accounting"); ++exact; }
    require(!b->import_descriptor_original_body && !b->import_descriptor_dispatch_escapes, label + " body guard/escape");
    if (!ax.empty()) { ++faults[index]; budget_stops += ax == "Native instruction budget exhausted"; }
    else if (!negative) ++normal[index];
    if (path != Path::component) {
        require(boundary.entered.size() == 1 && boundary.outcomes.size() == 1, label + " one entry/leave boundary");
        const bool insufficient = initial.executed > initial.budget ||
            initial.budget - initial.executed < (index == descriptor::import_count ? 5u : 6u);
        const bool chosen = mode == Mode::entry || mode == Mode::global || mode == Mode::profile || insufficient;
        require(boundary.entered[0] == std::pair(descriptor::entries[index], chosen), label + " exact selected root");
        if (chosen) {
            require(!total(b->import_descriptor_calls), label + " diagnostic path counted modern");
            if (mode == Mode::profile) require(b->import_descriptor_profile_original_calls[index] == 1 && !total(b->import_descriptor_original_calls), label + " profile selection");
            else if (insufficient && mode != Mode::entry && mode != Mode::global)
                require(b->import_descriptor_budget_original_calls[index] == 1 && !total(b->import_descriptor_original_calls), label + " budget selection");
            else require(b->import_descriptor_original_calls[index] == 1, label + " explicit original selection");
            ++selected[index];
        } else {
            require(b->import_descriptor_calls[index] == 1 && !total(b->import_descriptor_original_calls) &&
                !total(b->import_descriptor_profile_original_calls) && !total(b->import_descriptor_budget_original_calls), label + " dedicated semantic root");
            const bool own_fault = !ax.empty() && path != Path::composed;
            require(b->import_descriptor_tails[index] == !own_fault, label + " tail boundary distinct from downstream outcome");
            if (own_fault) require(b->import_descriptor_faults[index] + b->import_descriptor_budget_stops[index] == 1, label + " own fault classification");
            if (!ax.empty() && path == Path::composed) { require(!total(b->import_descriptor_faults), label + " downstream fault classified as glue fault"); ++downstream_faults; }
            if (ax.empty() && !negative && mode == Mode::modern) {
                if (path == Path::adapter) ++direct[index];
                if (path == Path::generated) ++generated[index];
                if (path == Path::composed) ++composed[index];
            }
        }
    }
    ++comparisons[index]; return {x.effects.log};
}
std::unique_ptr<State> minimal(const State& source, const std::vector<Effects::Access>& accesses,
                              std::uint32_t excluded = UINT32_MAX) {
    auto s = std::make_unique<State>(); s->registers = source.registers; s->unique = source.unique;
    s->pc = source.pc; s->executed = source.executed; s->budget = source.budget;
    s->native_calls = source.native_calls; s->allocated = source.allocated; s->available = source.available;
    s->heap_end = source.heap_end; s->memory_error = source.memory_error;
    std::set<std::uint32_t> bytes;
    for (const auto& access : accesses) for (unsigned n = 0; n < access.size; ++n)
        if (access.address + n != excluded) bytes.insert(access.address + n);
    for (auto address : bytes) { s->memory.map(address, 1); s->memory.write<1>(address, source.memory.read<1>(address)); }
    return s;
}
void expect_regular(const State& out, unsigned index, unsigned alignment, std::uint32_t destination) {
    require(out.pc == destination && out.r<4>(4132) == destination && out.r<4>(0) == destination, "Independent function/CTR/tail");
    require(out.r<4>(8) == callee_toc && out.r<4>(48) == record + alignment, "Independent descriptor/TOC");
    require(out.memory.read<4>(stack + alignment + 20) == relocated_toc + alignment, "Caller TOC saved");
    require(out.r<4>(4) == stack + alignment && out.r<4>(4128) == caller && out.native_calls == 23,
        "Leaf tail preserves caller frame/link and does not invoke destination");
    require(out.executed == 37 + (index == descriptor::import_count ? 5 : 6), "Independent exact leaf accounting");
}
void component_roots() {
    for (unsigned index = 0; index < count; ++index) {
        auto s = fixture_state(index);
        const auto evidence = compare(index, *s, "normal", Path::component, {}, [index](const State& out) { expect_regular(out, index, 0, target); });
        require(evidence.accesses.size() == (index == descriptor::import_count ? 3u : 4u), "Only required guest descriptor/frame accesses");
        for (unsigned alignment : {1u, 3u}) {
            s = fixture_state(index, alignment);
            compare(index, *s, "unaligned descriptor/frame/TOC", Path::component, {}, [index, alignment](const State& out) { expect_regular(out, index, alignment, target); });
        }
        for (auto destination : {0u, UINT32_MAX, descriptor::entries[index] + 4}) {
            s = fixture_state(index); s->memory.write<4>(record, destination);
            compare(index, *s, "dynamic destination", Path::component, {}, [index, destination](const State& out) { expect_regular(out, index, 0, destination); });
        }
        s = fixture_state(index); s->wr<4>(4, 0xffffffec); s->memory.map(0, 4);
        compare(index, *s, "word32 caller frame address wrap", Path::component, {}, [](const State& out) {
            require(out.memory.read<4>(0) == relocated_toc && out.r<4>(4) == 0xffffffec, "Wrapped caller TOC store");
        });
        s = fixture_state(index); s->wr<4>(48, 0xfffffffc); s->memory.map(0xfffffffc, 4); s->memory.map(0, 4);
        if (index < descriptor::import_count) s->memory.write<4>(std::uint32_t(s->r<4>(8) + descriptor::toc_displacements[index]), 0xfffffffc);
        s->memory.write<4>(0xfffffffc, target); s->memory.write<4>(0, callee_toc);
        compare(index, *s, "word32 descriptor field address wrap");
        if (index < descriptor::import_count) {
            s = fixture_state(index); s->wr<4>(8, 3);
            const auto wrapped_slot = std::uint32_t(3 + descriptor::toc_displacements[index]);
            s->memory.map(wrapped_slot, 4); s->memory.write<4>(wrapped_slot, record);
            compare(index, *s, "signed TOC slot address wrap");
        }
        s = fixture_state(index); auto tiny = minimal(*s, evidence.accesses);
        compare(index, *tiny, "minimal mappings");
        for (const auto& access : evidence.accesses) for (unsigned byte = 0; byte < access.size; ++byte) {
            auto bad = minimal(*s, evidence.accesses, access.address + byte);
            auto original = clone(*bad); const auto error = execute(*original, index, Path::component, true);
            require(!error.empty(), "Removed accessed byte must fault"); compare(index, *bad, "unmapped accessed byte", Path::component, {}, {}, error);
        }
        for (unsigned available = 0; available <= 7; ++available) {
            s = fixture_state(index); s->budget = s->executed + available;
            compare(index, *s, "budget=" + std::to_string(available), Path::component, {}, {},
                available < (index == descriptor::import_count ? 5u : 6u) ? "Native instruction budget exhausted" : "");
        }
        s = fixture_state(index); compare(index, *s, "own CTR negative", Path::component, {}, {}, {}, Mode::modern, true);
    }
}
void aliasing_and_live_values(Path path) {
    for (unsigned index = 0; index < count; ++index) {
        auto s = fixture_state(index);
        const auto set_descriptor = [index](State& out, std::uint32_t address) {
            if (index < descriptor::import_count) out.memory.write<4>(std::uint32_t(out.r<4>(8) + descriptor::toc_displacements[index]), address);
            out.wr<4>(48, address);
        };
        set_descriptor(*s, stack + 20); s->memory.write<4>(stack + 20, target); s->memory.write<4>(stack + 24, callee_toc);
        compare(index, *s, "function aliases saved TOC", path, {}, [index](const State& out) {
            require(out.pc == (index == descriptor::import_count ? target : relocated_toc), "Independent generic/import function alias order");
        });
        s = fixture_state(index); set_descriptor(*s, stack + 16);
        s->memory.write<4>(stack + 16, target); s->memory.write<4>(stack + 20, callee_toc);
        compare(index, *s, "callee TOC aliases saved TOC", path, {}, [](const State& out) {
            require(out.r<4>(8) == relocated_toc, "Saved caller TOC is subsequently read as callee TOC");
        });
        if (index < descriptor::import_count) {
            s = fixture_state(index); const auto moved = std::uint32_t(stack + 20 - descriptor::toc_displacements[index]);
            s->wr<4>(8, moved); s->memory.write<4>(stack + 20, record);
            compare(index, *s, "slot aliases saved TOC", path, {}, [moved](const State& out) {
                require(out.pc == target && out.memory.read<4>(stack + 20) == moved, "Slot read precedes caller save");
            });
        }
        s = fixture_state(index);
        compare(index, *s, "live registers after caller TOC write", path, [](Monitor& m) {
            m.on_write = [](State& out, std::uint32_t address) {
                if (address == stack + 20) { out.wr<4>(48, alternate); out.wr<4>(0, 0xfedcba98); out.wr<4>(4, stack + 64); out.wr<4>(8, callee_toc + 128); }
            };
        }, [index](const State& out) {
            require(out.pc == (index == descriptor::import_count ? 0xfedcba98u : target + 4) && out.r<4>(8) == callee_toc + 4,
                    "Independent live register reads after save");
        });
        compare(index, *s, "live descriptor pointer during function read", path, [](Monitor& m) {
            m.effects.on_read = [](State& out, Effects&, std::uint32_t address, unsigned) {
                if (address == record) out.wr<4>(48, alternate);
            };
        }, [](const State& out) { require(out.pc == target && out.r<4>(8) == callee_toc + 4, "Function and TOC use live descriptor reads"); });
        compare(index, *s, "live descriptor bytes during function read", path, [](Monitor& m) {
            m.effects.on_read = [](State& out, Effects&, std::uint32_t address, unsigned) {
                if (address == record) out.memory.write<4>(record, target + 8);
            };
        }, [](const State& out) { require(out.pc == target + 8, "Descriptor bytes are read after observer boundary"); });
        for (bool mutate_ctr : {false, true}) {
            compare(index, *s, "live destination during callee TOC read", path, [mutate_ctr](Monitor& m) {
                m.effects.on_read = [mutate_ctr](State& out, Effects&, std::uint32_t address, unsigned) {
                    if (address == record + 4) out.wr<4>(mutate_ctr ? 4132 : 0, 0xfedcba98);
                };
            }, [index, mutate_ctr](const State& out) {
                const bool changed = index == descriptor::import_count ? mutate_ctr : !mutate_ctr;
                require(out.pc == (changed ? 0xfedcba98u : target), "Independent publication order of CTR");
            });
        }
        // A budget lowered inside a run takes effect at the next run start in
        // generated code, but at the next instruction in the C++ owner; exact
        // budget stops are no longer compared (2026-10-20).
    }
}
void cfm_composition(unsigned index) {
    auto initial = fixture_state(index);
    initial->memory.write<4>(record, native_target);
    initial->memory.write<4>(alternate, descriptor::entries[index]);
    initial->memory.write<4>(alternate + 4, relocated_toc);
    initial->wr<4>(48, alternate); initial->wr<4>(8, 0x33008000); initial->pc = descriptor::generic_entry;
    auto a = clone(*initial), b = clone(*initial); a->import_descriptor_original = true;
    Monitor x, y; x.effects.state = a.get(); y.effects.state = b.get();
    NativeFixture old_native, new_native; Boundary boundary;
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    a->code_fragment_observer = &old_native; b->code_fragment_observer = &new_native;
    b->import_descriptor_observer = &boundary;
    run_until(*a, caller); run_until(*b, caller);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr; b->import_descriptor_observer = nullptr;
    const auto label = hex_address(descriptor::entries[index]) + "/CFM-import-native composition";
    Image(*a).compare(*b, label);
    require(x.effects.bytes == y.effects.bytes && x.registers == y.registers && old_native.names == new_native.names &&
        old_native.arguments == new_native.arguments && a->executed == b->executed, label + " exact ABI/effects/accounting");
    require(boundary.entered == std::vector<std::pair<std::uint32_t,bool>>{{descriptor::generic_entry,false},{descriptor::entries[index],false}} &&
        boundary.outcomes == std::vector<ImportDescriptorOutcome>(2, ImportDescriptorOutcome::tailed), label + " two real semantic tail boundaries");
    require(b->import_descriptor_calls[index] == 1 && b->import_descriptor_calls[descriptor::import_count] == 1 &&
        total(b->import_descriptor_calls) == 2 && total(b->import_descriptor_tails) == 2 && !total(b->import_descriptor_original_calls) &&
        !total(b->import_descriptor_faults) && !total(b->import_descriptor_budget_original_calls) && !b->import_descriptor_dispatch_escapes,
        label + " separate successful CFM and import entries");
    require(b->pc == caller && b->memory.read<4>(stack + 20) == relocated_toc &&
        b->memory.read<4>(fixture + 0x300) == 0x12345678 && b->native_calls == 24, label + " independent composed result");
    ++indirect[index]; ++comparisons[index]; ++normal[index]; ++exact;
}
void production_roots(bool profile) {
    for (unsigned index = 0; index < count; ++index) {
        auto s = fixture_state(index);
        if (profile) { compare(index, *s, "profile", Path::generated, {}, {}, {}, Mode::profile); continue; }
        for (bool turbo : {false, true}) {
            set_match_turbo_enabled(turbo);
            for (Path path : {Path::adapter, Path::generated}) compare(index, *s, "normal production", path);
        }
        for (auto destination : {descriptor::entries[index] + 4,
                descriptor::entries[index] == 0x10780ff0 ? 0x10781000u : descriptor::entries[index] + 4}) {
            if (destination == descriptor::entries[index] + 4 && s->memory.read<4>(record) == destination) continue;
            s->memory.write<4>(record, destination);
            compare(index, *s, "generated interior destination", Path::generated, {}, [destination](const State& out) {
                require(out.pc == destination, "Semantic generated tail stops at live interior destination");
            });
            compare(index, *s, "original interior destination", Path::generated, {}, [destination](const State& out) {
                require(out.pc == destination, "Guarded original tail stops at live interior destination");
            }, {}, Mode::global);
        }
        s = fixture_state(index);
        compare(index, *s, "global original", Path::generated, {}, {}, {}, Mode::global);
        compare(index, *s, "entry original", Path::adapter, {}, {}, {}, Mode::entry);
        compare(index, *s, "unrelated selection", Path::generated, {}, {}, {}, Mode::unrelated);
        compare(index, *s, "own boundary negative", Path::adapter, {}, {}, {}, Mode::modern, true);
        // Budget stops fall on run starts: no budget stops the glue, its full
        // length finishes it, and a partial budget may stop at a later run.
        const unsigned length = index == descriptor::import_count ? 5u : 6u;
        for (unsigned available = 0; available <= 7; ++available) {
            s = fixture_state(index); s->budget = s->executed + available;
            compare(index, *s, "production budget", Path::generated, {}, {},
                available == 0 ? "Native instruction budget exhausted" : available >= length ? "" : "*");
        }
        s = fixture_state(index);
        std::vector<Effects::Access> accesses{{2, stack + 20, 4, 0}, {4, record, 4, 0}, {4, record + 4, 4, 0}};
        if (index < descriptor::import_count) accesses.push_back({4, std::uint32_t(relocated_toc + descriptor::toc_displacements[index]), 4, 0});
        auto missing = minimal(*s, accesses, record + 3); auto original = clone(*missing);
        const auto mapping_error = execute(*original, index, Path::component, true);
        require(!mapping_error.empty(), "Production descriptor mapping fault required");
        compare(index, *missing, "generated descriptor mapping fault", Path::generated, {}, {}, mapping_error);
        if (index < descriptor::import_count) cfm_composition(index);
        s = fixture_state(index); s->memory.write<4>(record, native_target);
        compare(index, *s, "composed native destination", Path::composed, {}, [](const State& out) {
            require(out.pc == caller && out.memory.read<4>(fixture + 0x300) == 0x12345678 && out.native_calls == 24,
                "Native fixture invoked by caller dispatcher after tail");
        });
        compare(index, *s, "native destination fault", Path::composed, {}, {}, "Controlled downstream import failure", Mode::modern, false, true);
        s = fixture_state(index); s->memory.write<4>(record, 0x1077f6d0); // Existing semantic guest zero-status leaf.
        compare(index, *s, "composed guest destination", Path::composed, {}, [](const State& out) {
            require(out.pc == caller && out.r<4>(12) == 0 && out.time_runtime_calls[0] == 1 && !out.time_runtime_original_calls[0],
                "Guest semantic destination remains separately counted");
        });
        s = fixture_state(index);
        compare(index, *s, "uncompiled downstream destination", Path::composed, {}, {}, "Uncompiled original address:");
    }
    if (!profile) aliasing_and_live_values(Path::adapter);
}
void reset_checks() {
    auto s = fixture_state(0); s->import_descriptor_calls.fill(4); s->import_descriptor_original_calls.fill(5);
    s->import_descriptor_tails.fill(6); s->import_descriptor_faults.fill(7); s->import_descriptor_transfers.fill(8);
    s->import_descriptor_budget_stops.fill(9); s->import_descriptor_budget_original_calls.fill(10);
    s->import_descriptor_profile_original_calls.fill(11); s->import_descriptor_entry_original.fill(true);
    s->import_descriptor_original = true; s->import_descriptor_original_body = descriptor::first_entry;
    s->import_descriptor_dispatch_escapes = 12; Boundary observer; s->import_descriptor_observer = &observer;
    s->time_runtime_calls[0] = 23; const Image image(*s);
    reset_import_descriptor_diagnostics(*s); image.compare(*s, "Reset preserves architecture/memory");
    require(!s->import_descriptor_original && !s->import_descriptor_original_body && !s->import_descriptor_observer &&
        !s->import_descriptor_dispatch_escapes && !total(s->import_descriptor_calls) && !total(s->import_descriptor_original_calls) &&
        !total(s->import_descriptor_tails) && !total(s->import_descriptor_faults) && !total(s->import_descriptor_budget_stops) &&
        !total(s->import_descriptor_transfers) && !total(s->import_descriptor_budget_original_calls) &&
        !total(s->import_descriptor_profile_original_calls) && s->time_runtime_calls[0] == 23,
        "Independent complete process-local diagnostics reset");
    require(std::none_of(s->import_descriptor_entry_original.begin(), s->import_descriptor_entry_original.end(), [](bool v) { return v; }), "Entry selections reset");
}
}
int main(int argc, char** argv) {
    try {
        const bool profile = argc == 2 && std::string_view(argv[1]) == "--profile";
        const bool production = argc == 2 && std::string_view(argv[1]) == "--production";
        require(argc == 1 || profile || production, "Expected component, --production or --profile");
        if (profile) require(match_pc_profile_enabled(), "Profile diagnostic environment");
        if (production || profile) production_roots(profile);
        else { component_roots(); aliasing_and_live_values(Path::component); }
        reset_checks();
        for (unsigned n = 0; n < count; ++n) {
            require(comparisons[n] && normal[n], "Individual descriptor evidence");
            if (!profile) require(negatives[n] == 1, "One own negative per entry");
            if (production) require(indirect[n] == unsigned(n < descriptor::import_count), "CFM incoming import coverage per entry");
            if (production) require(direct[n] >= 2 && generated[n] >= 2 && composed[n] == 2, "Direct/generated/native/guest coverage per entry");
            std::cout << "import_descriptor.entry=" << hex_address(descriptor::entries[n]) << " comparisons=" << comparisons[n]
                << " normal=" << normal[n] << " faults=" << faults[n] << " negative=" << negatives[n]
                << " direct=" << direct[n] << " generated=" << generated[n] << " composed=" << composed[n] << " indirect=" << indirect[n] << " selected=" << selected[n] << '\n';
        }
        std::cout << "import_descriptor.exact=" << exact << " budget_stops=" << budget_stops << " downstream_faults=" << downstream_faults << '\n';
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
