#include "native_service_adapter.hpp"
#include "import_descriptor.hpp"
#include "code_fragment_adapter.hpp"
#include "semantic_io_checks.hpp"
#include "match_turbo.hpp"
#include <numeric>
#include <set>

namespace cm::aot { bool native_service_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace service = cm::aot::native_service;
constexpr std::uint32_t caller = 0x7fff1000, relocated_toc = 0x31008000,
    descriptor = fixture + 0x100, facade_descriptor = fixture + 0x180,
    alternate_stack = stack + 64, heap = 0x20000000;
constexpr std::array<std::uint32_t, 3> imports{280984676, 280984408, 280984700};
constexpr std::array<const char*, 3> names{"CarbonLib::AEProcessAppleEvent", "CarbonLib::NewPtr", "CarbonLib::DisposePtr"};
enum class Path { component, adapter, generated, cfm };
enum class Mode { modern, entry, global, unrelated, profile };
std::array<unsigned, 3> comparisons{}, normal{}, faults{}, negatives{}, direct{}, generated{}, cfm{}, selected{};
unsigned exact = 0, budget_stops = 0;
std::uint64_t total(const auto& values) { return std::accumulate(values.begin(), values.end(), UINT64_C(0)); }
struct Monitor final : MemoryObserver {
    Effects effects;
    std::vector<std::uint32_t> pcs;
    std::vector<decltype(State::registers)> registers;
    std::function<void(State&, std::uint32_t)> on_write;
    void mapped(std::uint32_t address, std::uint64_t size) override { effects.mapped(address, size); }
    void cleared(std::uint32_t address, std::uint64_t size) override { effects.cleared(address, size); }
    void read(std::uint32_t address, const std::uint8_t* data, unsigned size) override {
        pcs.push_back(effects.state->pc); registers.push_back(effects.state->registers); effects.read(address, data, size);
    }
    void written(std::uint32_t address, const std::uint8_t* data, std::size_t size) override {
        pcs.push_back(effects.state->pc); registers.push_back(effects.state->registers); effects.written(address, data, size);
        if (on_write) on_write(*effects.state, address);
    }
};
struct Imports final : CodeFragmentBoundaryObserver {
    struct Boundary {
        bool leave; std::string name; decltype(State::registers) registers;
        std::uint32_t pc; std::uint64_t native_calls;
        bool operator==(const Boundary&) const = default;
    };
    std::vector<Boundary> boundaries;
    bool real = false;
    std::function<void(State&)> body;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override { boundaries.push_back({false, name, s.registers, s.pc, s.native_calls}); }
    void import_leave(const State& s, const char* name) override { boundaries.push_back({true, name, s.registers, s.pc, s.native_calls}); }
    bool import_fixture(State& s, const char* name) override {
        const std::string_view called(name);
        require(std::find(names.begin(), names.end(), called) != names.end(), "Explicit service-facade fixture");
        if (body) body(s);
        else if (real) {
            require(called != names[0] && bridge_import(s, std::string(called)), "Real existing pointer bridge");
            return true;
        } else {
            s.memory.write<4>(fixture + 0x300, std::uint32_t(s.r<4>(12)) ^ 0x13579bdf);
            s.wr<4>(12, 0x14253647);
        }
        s.pc = std::uint32_t(s.r<4>(4128)); return true;
    }
};
struct Boundary final : NativeServiceBoundaryObserver {
    bool corrupt = false;
    std::vector<std::pair<std::uint32_t, bool>> entered;
    std::vector<NativeServiceOutcome> outcomes;
    void enter(const State&, std::uint32_t root, bool original) override { entered.emplace_back(root, original); }
    void leave(State& s, std::uint32_t, bool, NativeServiceOutcome outcome) override {
        outcomes.push_back(outcome); if (corrupt) s.wr<4>(12, s.r<4>(12) ^ 1);
    }
};
std::uint32_t slot(unsigned index, std::uint32_t toc_value = relocated_toc) {
    return toc_value + std::int32_t(import_descriptor::toc_displacements[import_descriptor::entry_index(service::import_glues[index])]);
}
std::unique_ptr<State> initial(unsigned index) {
    auto s = std::make_unique<State>(); s->registers.fill(0x6d); s->unique.fill(0xb3);
    for (unsigned n = 0; n < 32; ++n) {
        s->wr<4>(n * 4, 0x2468a000 + n * 0x101);
        s->wr<8>(0x4000 + n * 16, UINT64_C(0xfff0000100000000) + n);
    }
    for (unsigned n = 0; n < 8; ++n) s->wr<1>(2304 + n, 0xa0 | n);
    s->wr<1>(1024, 1); s->wr<4>(4, stack); s->wr<4>(8, relocated_toc);
    s->wr<4>(4128, caller); s->wr<4>(4132, 0x31415926);
    for (auto [address, length] : std::array<std::pair<std::uint32_t, unsigned>, 4>{
            {{fixture, 0x1000}, {stack - 512, 1024}, {relocated_toc - 0x8000, 0x10000}, {heap, 0x100}}}) {
        s->memory.map(address, length); s->memory.copy_in(address, std::vector<std::uint8_t>(length, 0xa7));
    }
    s->memory.write<4>(slot(index), descriptor);
    s->memory.write<4>(descriptor, imports[index]); s->memory.write<4>(descriptor + 4, 0x32008000);
    s->memory.write<4>(facade_descriptor, service::entries[index]); s->memory.write<4>(facade_descriptor + 4, relocated_toc);
    s->memory.write<4>(alternate_stack + 8, caller);
    s->pc = service::entries[index]; s->executed = 37; s->budget = 50000; s->native_calls = 23;
    s->heap_end = heap + 0x100; s->memory_error = -108; s->allocated = {{heap, 64}}; s->available = {{heap + 0x80, 16}};
    s->wr<4>(12, index == 0 ? fixture + 0x400 : index == 1 ? 31 : heap); return s;
}
void reference_until(State& s) {
    while (s.pc != caller) {
        if (!native_service_reference(s)) run_until(s, std::uint32_t(s.r<4>(4128)));
    }
}
std::string execute(State& s, unsigned index, Path path, bool original) {
    try {
        if (path == Path::component) {
            if (original) reference_until(s); else service::invoke(s, service::Service(index));
        } else if (path == Path::adapter) require(try_modern_native_service(s, service::entries[index]), "Direct service facade");
        else if (path == Path::generated) require(dispatch_page(s), "Generated service facade");
        else run_until(s, caller);
    } catch (const std::runtime_error& e) { return e.what(); }
    return {};
}
using Mutation = std::function<void(Monitor&, Imports&)>;
using Expectation = std::function<void(const State&)>;
std::vector<Effects::Access> compare(unsigned index, const State& seed, const std::string& name, Path path,
        const Mutation& mutate = {}, const Expectation& expect = {}, const std::string& expected_error = {},
        Mode mode = Mode::modern, bool negative = false, bool real = false) {
    auto a = clone(seed), b = clone(seed); a->native_service_original = true; a->import_descriptor_original = true;
    if (mode == Mode::entry) b->native_service_entry_original[index] = true;
    if (mode == Mode::global) b->native_service_original = true;
    if (mode == Mode::unrelated) b->native_service_entry_original[(index + 1) % 3] = true;
    Monitor x, y; Imports old, modern; Boundary boundary;
    x.effects.state = a.get(); y.effects.state = b.get(); old.real = modern.real = real; boundary.corrupt = negative;
    if (mutate) { mutate(x, old); mutate(y, modern); }
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    a->code_fragment_observer = &old; b->code_fragment_observer = &modern;
    if (path != Path::component) b->native_service_observer = &boundary;
    const auto ax = execute(*a, index, path, true), bx = execute(*b, index, path, false);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr; b->native_service_observer = nullptr;
    const auto label = hex_address(service::entries[index]) + "/" + name;
    require(ax == bx && (expected_error.empty() ? ax.empty() : ax.starts_with(expected_error)), label + " original=" + ax + " modern=" + bx);
    if (negative) {
        bool detected = false;
        try {
            if (path == Path::component) b->wr<4>(12, b->r<4>(12) ^ 1);
            Image(*a).compare(*b, label);
        } catch (const std::runtime_error&) { detected = true; }
        require(detected, label + " own negative accepted"); ++negatives[index];
    } else {
        Image(*a).compare(*b, label);
        require(x.effects.bytes == y.effects.bytes && x.pcs == y.pcs && x.registers == y.registers, label + " ordered effects/PC/register prefixes");
        require(old.boundaries == modern.boundaries, label + " exact import boundaries");
        if (expect) expect(*b);
    }
    require(a->executed == b->executed, label + " exact instruction accounting"); ++exact;
    require(!b->native_service_original_body && !b->native_service_dispatch_escapes, label + " balanced original guard/no escape");
    const bool stopped = ax == "Native instruction budget exhausted";
    if (!ax.empty()) { ++faults[index]; budget_stops += stopped; }
    else if (!negative) ++normal[index];
    if (path != Path::component) {
        const bool chosen = mode == Mode::entry || mode == Mode::global || mode == Mode::profile;
        require(boundary.entered == std::vector<std::pair<std::uint32_t, bool>>{{service::entries[index], chosen}} &&
            boundary.outcomes.size() == 1, label + " exact facade boundary");
        require(boundary.outcomes[0] == (ax.empty() ? NativeServiceOutcome::returned : stopped ? NativeServiceOutcome::budget_stop : NativeServiceOutcome::fault), label + " outcome classification");
        if (chosen) {
            require(!total(b->native_service_calls), label + " original counted modern");
            require((mode == Mode::profile ? b->native_service_profile_original_calls[index] : b->native_service_original_calls[index]) == 1,
                label + " separate original/profile selection"); ++selected[index];
        } else {
            require(b->native_service_calls[index] == 1 && !total(b->native_service_original_calls) && !total(b->native_service_profile_original_calls), label + " dedicated semantic facade");
            require(b->native_service_returns[index] == unsigned(ax.empty()) && b->native_service_budget_stops[index] == unsigned(stopped) &&
                b->native_service_faults[index] == unsigned(!ax.empty() && !stopped), label + " dedicated outcome counters");
            if (ax.empty() && !negative && mode == Mode::modern) {
                if (path == Path::adapter) ++direct[index];
                if (path == Path::generated) ++generated[index];
                if (path == Path::cfm) ++cfm[index];
            }
        }
    }
    ++comparisons[index]; return x.effects.log;
}
std::unique_ptr<State> minimal(const State& source, const std::vector<Effects::Access>& accesses, std::uint32_t excluded = UINT32_MAX) {
    auto s = std::make_unique<State>(); s->registers = source.registers; s->unique = source.unique; s->pc = source.pc;
    s->executed = source.executed; s->budget = source.budget; s->native_calls = source.native_calls;
    s->heap_end = source.heap_end; s->allocated = source.allocated; s->available = source.available; s->memory_error = source.memory_error;
    std::set<std::uint32_t> bytes;
    for (const auto& access : accesses) for (std::uint64_t n = 0; n < access.size; ++n) bytes.insert(access.address + std::uint32_t(n));
    for (auto address : bytes) if (address != excluded) { s->memory.map(address, 1); s->memory.copy_in(address, source.memory.copy_out(address, 1)); }
    return s;
}
void matrix(unsigned index, bool production) {
    const auto path = production ? Path::adapter : Path::component; auto seed = initial(index);
    const auto baseline = compare(index, *seed, "normal", path, {}, [](const State& s) {
        require(s.pc == caller && s.r<4>(4) == stack && s.r<4>(8) == relocated_toc && s.r<4>(12) == 0x14253647 &&
            s.r<4>(2440) == relocated_toc && s.native_calls == 24, "Independent caller/result contract");
    });
    compare(index, *seed, "own-negative", path, {}, {}, {}, Mode::modern, true);
    for (unsigned offset : {1u, 3u}) {
        auto unaligned = clone(*seed); unaligned->wr<4>(4, stack + offset);
        unaligned->memory.write<4>(descriptor + offset, imports[index]); unaligned->memory.write<4>(descriptor + offset + 4, 0x32008000);
        unaligned->memory.write<4>(slot(index), descriptor + offset);
        compare(index, *unaligned, "unaligned=" + std::to_string(offset), path);
    }
    compare(index, *seed, "live-stack-after-link-save", path, [](Monitor& m, Imports&) {
        m.on_write = [](State& s, std::uint32_t address) { if (address == stack + 8) s.wr<4>(4, alternate_stack); };
    }, [](const State& s) { require(s.r<4>(4) == alternate_stack && s.pc == caller, "Live stack reload after saved-link observer"); });
    compare(index, *seed, "cached-frame-despite-observer", path, [](Monitor& m, Imports&) {
        m.on_write = [](State& s, std::uint32_t address) { if (address == stack - 64) s.wr<4>(4, alternate_stack); };
    });
    auto relocated = clone(*seed); relocated->wr<4>(8, relocated_toc + 0x100); relocated->memory.write<4>(slot(index, relocated_toc + 0x100), descriptor);
    compare(index, *relocated, "relocated-live-toc", path);
    auto aliased_slot = clone(*seed);
    const auto overlapping_toc = std::uint32_t(stack - 44 - std::int32_t(import_descriptor::toc_displacements[import_descriptor::entry_index(service::import_glues[index])]));
    aliased_slot->wr<4>(8, overlapping_toc); aliased_slot->memory.write<4>(stack - 44, descriptor);
    compare(index, *aliased_slot, "slot-aliases-saved-toc", path);
    auto aliased_descriptor = clone(*seed); aliased_descriptor->memory.write<4>(slot(index), stack - 48);
    aliased_descriptor->memory.write<4>(stack - 48, imports[index]); aliased_descriptor->memory.write<4>(stack - 44, 0x12345678);
    compare(index, *aliased_descriptor, "descriptor-toc-aliases-saved-toc", path);
    compare(index, *seed, "native-mutated-toc-restored", path, [](Monitor&, Imports& i) {
        i.body = [](State& s) { s.wr<4>(8, 0x34567890); s.wr<4>(12, 7); };
    }, [](const State& s) { require(s.r<4>(8) == relocated_toc && s.r<4>(12) == 7, "Facade restores caller TOC only"); });
    auto live_epilogue = clone(*seed); live_epilogue->memory.write<4>(stack + 72, caller);
    compare(index, *live_epilogue, "live-stack-during-toc-restore", path, [index](Monitor& m, Imports&) {
        m.effects.on_read = [index](State& s, Effects&, std::uint32_t address, unsigned) {
            if (s.pc == service::entries[index] + 16 && address == stack - 44) s.wr<4>(4, stack);
        };
    }, [](const State& s) { require(s.r<4>(4) == stack + 64 && s.pc == caller, "Epilogue reloads live SP after TOC read"); });
    auto wrapped_stack = clone(*seed); wrapped_stack->wr<4>(4, 32);
    wrapped_stack->memory.map(0, 64); wrapped_stack->memory.map(0xffffffe0, 32);
    compare(index, *wrapped_stack, "word32-stack-frame-wrap", path);
    compare(index, *seed, "native-fault", path, [](Monitor&, Imports& i) {
        i.body = [](State& s) { s.wr<4>(12, 9); throw std::runtime_error("Fixture native-service fault"); };
    }, {}, "Fixture native-service fault");
    for (unsigned budget = 0; budget <= 16; ++budget) {
        auto bounded = clone(*seed); bounded->budget = bounded->executed + budget;
        compare(index, *bounded, "budget=" + std::to_string(budget), path, {}, {}, budget < 15 ? "Native instruction budget exhausted" : "");
    }
    for (unsigned remaining : {0u, 1u, 2u, 4u}) compare(index, *seed, "live-budget=" + std::to_string(remaining), path,
        [remaining](Monitor&, Imports& i) { i.body = [remaining](State& s) { s.budget = s.executed + remaining; }; }, {}, "Native instruction budget exhausted");
    auto tiny = minimal(*seed, baseline); compare(index, *tiny, "minimal-mappings", path);
    std::set<std::uint32_t> bytes;
    for (const auto& access : baseline) for (std::uint64_t n = 0; n < access.size; ++n) bytes.insert(access.address + std::uint32_t(n));
    for (auto byte : bytes) {
        auto hole = minimal(*seed, baseline, byte);
        compare(index, *hole, "removed-byte=" + hex_address(byte), path, {}, {}, "Unmapped data access at ");
    }
    if (index) {
        for (unsigned value : index == 1 ? std::vector<unsigned>{0, 1, 16, 17, 0x10000001, UINT32_MAX} : std::vector<unsigned>{0, heap, heap + 4}) {
            auto real = clone(*seed); real->wr<4>(12, value);
            const bool invalid = index == 2 && value == heap + 4;
            compare(index, *real, "real-pointer=" + hex_address(value), path, {}, [index, value, invalid](const State& s) {
                if (invalid) {
                    require(s.allocated.at(heap) == 64 && s.available.at(heap + 0x80) == 16 && s.r<4>(4) == stack - 64,
                        "Invalid release preserves heap and abandons facade epilogue");
                } else if (index == 1 && value > 0x10000000) {
                    require(!s.r<4>(12) && s.memory_error == -108 && s.allocated.size() == 1 && s.heap_end == heap + 0x100,
                        "Pointer allocation limit returns null without growing heap");
                } else if (index == 1) {
                    const auto pointer = std::uint32_t(s.r<4>(12));
                    require(pointer && !(pointer & 15) && s.allocated.at(pointer) == value && !s.memory_error,
                        "Pointer allocation preserves aligned guest address and requested size");
                    if (value <= 16) require(pointer == heap + 0x80 && s.memory.read<1>(pointer) == 0xa7,
                        "NewPtr reuses the existing interval without clearing its payload");
                } else if (!value) require(s.allocated.at(heap) == 64 && !s.memory_error, "Null release is a heap no-op");
                else require(!s.allocated.contains(heap) && s.available.at(heap) == 64 && s.memory.read<1>(heap) == 0xa7,
                    "Released pointer remains mapped with surviving content");
            }, invalid ? "Invalid or double DisposePtr" : "", Mode::modern, false, true);
        }
    }
    if (!production) return;
    for (auto kind : {Path::generated, Path::cfm}) {
        auto incoming = clone(*seed);
        if (kind == Path::cfm) { incoming->pc = 0x10781320; incoming->wr<4>(48, facade_descriptor); }
        compare(index, *incoming, "generated/incoming-cfm", kind);
    }
    for (auto mode : {Mode::entry, Mode::global, Mode::unrelated}) compare(index, *seed, "selection", path, {}, {}, {}, mode);
    auto after = clone(*seed); after->pc = service::entries[index] + 16; after->wr<4>(4, stack - 64);
    after->memory.write<4>(stack - 44, relocated_toc); after->memory.write<4>(stack + 8, caller);
    require(dispatch_page(*after) && after->pc == caller && !total(after->native_service_calls), "Interior continuation executes original epilogue without facade hook");
}
void reset_check() {
    auto s = initial(0); s->native_service_original = true; s->native_service_entry_original.fill(true);
    s->native_service_calls.fill(9); s->native_service_faults.fill(9); s->native_service_original_calls.fill(9);
    s->native_service_returns.fill(9); s->native_service_budget_stops.fill(9); s->native_service_transfers.fill(9);
    s->native_service_profile_original_calls.fill(9); s->native_service_original_body = 7; s->native_service_dispatch_escapes = 9;
    s->import_descriptor_calls[1] = 7; Boundary observer; s->native_service_observer = &observer;
    const auto image = Image(*s); reset_native_service_diagnostics(*s); image.compare(*s, "Reset preserves architecture");
    require(!s->native_service_original && std::none_of(s->native_service_entry_original.begin(), s->native_service_entry_original.end(), [](bool v) { return v; }) &&
        !total(s->native_service_calls) && !total(s->native_service_original_calls) && !total(s->native_service_returns) &&
        !total(s->native_service_faults) && !total(s->native_service_budget_stops) && !total(s->native_service_transfers) &&
        !total(s->native_service_profile_original_calls) && !s->native_service_original_body && !s->native_service_dispatch_escapes &&
        !s->native_service_observer && s->import_descriptor_calls[1] == 7, "Independent complete diagnostic reset");
}
}
int main(int argc, char** argv) {
    try {
        const bool production = argc == 2 && std::string_view(argv[1]) == "--production";
        const bool profile = argc == 2 && std::string_view(argv[1]) == "--profile";
        require(argc == 1 || production || profile, "Explicit service check mode");
        require(match_pc_profile_enabled() == profile, "Separate profile acceptance process");
        reset_check();
        for (unsigned index = 0; index < 3; ++index) {
            if (profile) { auto seed = initial(index); compare(index, *seed, "profile-original", Path::generated, {}, {}, {}, Mode::profile); }
            else matrix(index, production);
            require(profile || (normal[index] && negatives[index] == 1), "Per-entry positive and own negative");
            if (production) require(direct[index] && generated[index] && cfm[index], "Per-entry production dispatch coverage");
            std::cout << hex_address(service::entries[index]) << " comparisons=" << comparisons[index] << " normal=" << normal[index]
                << " faults=" << faults[index] << " negatives=" << negatives[index] << " direct=" << direct[index]
                << " generated=" << generated[index] << " cfm=" << cfm[index] << " selected=" << selected[index] << '\n';
        }
        std::cout << exact << " exact original/semantic facade comparisons; " << budget_stops << " budget stops\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
