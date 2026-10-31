#include "rng_core_adapter.hpp"
#include "rng_core.hpp"
#include "callbacks.hpp"
#include "exception_runtime_adapter.hpp"
#include "season_reference.hpp"
#include "match_replay.hpp"
#include <algorithm>
#include <functional>
#include <iostream>
#include <fstream>
#include <zlib.h>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000, stack = 0x700fff00, toc = 0x108bba90;
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
struct Effects final : MemoryObserver {
    std::vector<std::uint8_t> bytes;
    std::vector<std::pair<std::uint32_t, std::uint64_t>> accesses;
    void number(std::uint64_t value, unsigned width) {
        for (unsigned n = width; n; --n) bytes.push_back(std::uint8_t(value >> ((n - 1) * 8)));
    }
    void event(unsigned kind, std::uint32_t address, std::uint64_t size, const std::uint8_t* data = nullptr) {
        number(kind, 1); number(address, 4); number(size, 8);
        accesses.emplace_back(address, size);
        if (data) bytes.insert(bytes.end(), data, data + size);
    }
    void mapped(std::uint32_t address, std::uint64_t size) override { event(1, address, size); }
    void written(std::uint32_t address, const std::uint8_t* data, std::size_t size) override { event(2, address, size, data); }
    void cleared(std::uint32_t address, std::uint64_t size) override { event(3, address, size); }
    void read(std::uint32_t address, const std::uint8_t* data, unsigned size) override { event(4, address, size, data); }
};
struct Image {
    decltype(State::registers) registers;
    decltype(State::unique) unique;
    std::uint32_t pc, heap_end;
    std::uint64_t native_calls;
    std::int16_t memory_error;
    decltype(State::allocated) allocated, available;
    std::vector<std::pair<std::uint32_t, std::vector<std::uint8_t>>> memory;
    explicit Image(const State& s) : registers(s.registers), unique(s.unique), pc(s.pc), heap_end(s.heap_end),
        native_calls(s.native_calls), memory_error(s.memory_error), allocated(s.allocated), available(s.available) {
        for (const auto& [address, size] : s.memory.mapped_ranges()) memory.emplace_back(address, s.memory.copy_out(address, size));
    }
    void compare(const State& s, const std::string& label) const {
        if (registers != s.registers) {
            const auto p = std::mismatch(registers.begin(), registers.end(), s.registers.begin());
            throw std::runtime_error(label + " register byte=" + std::to_string(p.first - registers.begin()) +
                " expected=" + std::to_string(*p.first) + " actual=" + std::to_string(*p.second));
        }
        require(unique == s.unique && pc == s.pc, label + " unique/PC mismatch");
        require(heap_end == s.heap_end && native_calls == s.native_calls && memory_error == s.memory_error &&
                allocated == s.allocated && available == s.available, label + " allocator/import mismatch");
        const auto ranges = s.memory.mapped_ranges();
        require(ranges.size() == memory.size(), label + " mapping count mismatch");
        for (unsigned i = 0; i < memory.size(); ++i) {
            const auto& [address, bytes] = memory[i];
            require(ranges[i].first == address && ranges[i].second == bytes.size(), label + " mapping mismatch");
            auto actual = s.memory.copy_out(address, bytes.size());
            if (bytes != actual) {
                const auto p = std::mismatch(bytes.begin(), bytes.end(), actual.begin());
                throw std::runtime_error(label + " memory mismatch at " + hex_address(address + std::uint32_t(p.first - bytes.begin())));
            }
        }
    }
};
std::unique_ptr<State> clone(const State& s) {
    auto t = std::make_unique<State>();
    t->registers = s.registers; t->unique = s.unique; t->pc = s.pc;
    t->executed = s.executed; t->budget = s.budget; t->native_calls = s.native_calls;
    t->allocated = s.allocated; t->available = s.available; t->heap_end = s.heap_end; t->memory_error = s.memory_error;
    for (const auto& [address, size] : s.memory.mapped_ranges()) {
        t->memory.map(address, size); t->memory.copy_in(address, s.memory.copy_out(address, size));
    }
    return t;
}
void registers(State& s, std::uint32_t entry, std::uint32_t context, unsigned variation) {
    s.registers.fill(0x5b); s.unique.fill(0x6d);
    for (unsigned n = 0; n < 32; ++n) s.wr<4>(n * 4, 0xcafe0000 + n);
    s.wr<4>(4, stack); s.wr<4>(8, context); s.wr<4>(4128, return_sentinel);
    s.wr<4>(4132, 0x31415926); s.wr<4>(2440, 0xbadc0ffe);
    s.wr<1>(2304, variation % 2 ? 0xf9 : 0xa6);
    s.wr<1>(1024, variation % 4); s.wr<1>(1027, (variation / 2) % 2);
    s.pc = entry; s.executed = s.native_calls = 0; s.budget = 1000000;
}
std::unique_ptr<State> initialization(const char* program, std::uint32_t seed, unsigned variation = 0) {
    auto s = std::make_unique<State>(); load_program(*s, program);
    s->memory.map(fixture, 0x20000);
    s->memory.copy_in(fixture, std::vector<std::uint8_t>(0x20000, 0xa5));
    s->memory.copy_in(stack - 8192, std::vector<std::uint8_t>(8256, 0xa7));
    registers(*s, 0x107474a0, toc, variation); s->wr<4>(12, seed);
    for (auto [slot, value] : std::array<std::pair<std::uint32_t, std::uint32_t>, 5>{{
        {0x7b90, fixture}, {0x7b8c, fixture + 4096}, {0x7b88, fixture + 4},
        {0x7b84, fixture + 8}, {0x7bb8, fixture + 256}}}) s->memory.write<4>(toc - slot, value);
    s->memory.write<4>(toc - 0x34c4, 0x12345678);
    return s;
}
unsigned cases = 0;
std::array<unsigned, 3> own_cases{};
void compare_pair(State& original, State& modern, const std::string& label,
                  const std::function<void(State&)>& execute = [](State& s) { run(s); }) {
    Effects expected, actual;
    original.rng_original = true;
    original.memory.date_observer = &expected; modern.memory.date_observer = &actual;
    try { execute(original); } catch (const std::exception& e) { throw std::runtime_error(label + " original: " + e.what() + " pc=" + hex_address(original.pc)); }
    try { execute(modern); } catch (const std::exception& e) { throw std::runtime_error(label + " semantic: " + e.what() + " pc=" + hex_address(modern.pc)); }
    original.memory.date_observer = modern.memory.date_observer = nullptr;
    Image(original).compare(modern, label);
    if (expected.bytes != actual.bytes) {
        const auto p = std::mismatch(expected.bytes.begin(), expected.bytes.end(), actual.bytes.begin(), actual.bytes.end());
        throw std::runtime_error(label + " ordered access mismatch byte=" + std::to_string(p.first - expected.bytes.begin()) +
            " sizes=" + std::to_string(expected.bytes.size()) + "/" + std::to_string(actual.bytes.size()));
    }
    ++cases;
}
struct Boundaries final : RngBoundaryObserver {
    struct Boundary { std::uint32_t entry; Image image; Boundary(std::uint32_t e, const State& s) : entry(e), image(s) {} };
    std::vector<Boundary> expected;
    std::size_t cursor = 0;
    std::uint32_t perturb = 0;
    bool mutated = false;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& s, std::uint32_t entry, bool original) override {
        if (original) { expected.emplace_back(entry, s); return; }
        if (entry == perturb && !mutated) {
            mutated = true;
            if (entry == 0x1077b790) {
                const auto address = std::uint32_t(s.r<4>(8)) - 0x34c4;
                auto bytes = s.memory.copy_out(address, 4); bytes[3] ^= 1; s.memory.copy_in(address, bytes);
            } else s.wr<4>(12, s.r<4>(12) ^ 1);
        }
        require(cursor < expected.size() && expected[cursor].entry == entry, "RNG boundary order mismatch");
        expected[cursor++].image.compare(s, "Own RNG boundary " + hex_address(entry));
    }
};
// Use the existing CMDAY001 import encoding for a controlled GetDateTime.
// The clock service body and its transition stub execute; no wall clock is read.
void controlled_clock_transcript(State& s, const std::filesystem::path& directory, std::uint32_t clock) {
    require(s.rng_original, "Controlled clock oracle requires original RNG selection");
    std::filesystem::create_directories(directory);
    gzFile out = gzopen((directory / "imports.gz").c_str(), "wb1");
    require(out != nullptr, "Cannot write controlled clock transcript");
    auto put = [&](const auto& value) { require(gzwrite(out, &value, sizeof(value)) == int(sizeof(value)), "Clock transcript write"); };
    const std::array<char, 8> magic{'C','M','D','A','Y','0','0','1'}; put(magic);
    unsigned clocks = 0;
    struct ImportEffects final : MemoryObserver {
        gzFile stream;
        explicit ImportEffects(gzFile file) : stream(file) {}
        void mapped(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Unexpected clock fixture mapping"); }
        void cleared(std::uint32_t, std::uint64_t) override { throw std::runtime_error("Unexpected clock fixture clear"); }
        void written(std::uint32_t address, const std::uint8_t* bytes, std::size_t size) override {
            const std::uint8_t event = 1; const auto length = std::uint64_t(size);
            require(gzwrite(stream, &event, 1) == 1 && gzwrite(stream, &address, 4) == 4 &&
                gzwrite(stream, &length, 8) == 8 && gzwrite(stream, bytes, unsigned(size)) == int(size), "Clock native write recording");
        }
    } effects(out);
    // Imports now happen inside rewritten hooks' nested dispatches (the clock,
    // formatted output), so they are recorded where the dispatcher serves them.
    struct Recorder final : ExceptionRuntimeBoundaryObserver {
        std::function<void(State&, const char*)> record;
        void enter(const State&, std::uint32_t, bool) override {}
        void leave(State&, std::uint32_t, bool, ExceptionRuntimeOutcome) override {}
        bool import_fixture(State& state, const char* name) override { record(state, name); return true; }
    } recorder;
    recorder.record = [&](State& state, const char* name) {
        put(std::uint8_t(0)); put(state.pc); put(state.executed); put(state.native_calls); put(state.registers);
        state.memory.observer = &effects;
        if (std::string_view(name) == "CarbonLib::GetDateTime") {
            require(++clocks == 1, "Repeated clock query in fixture");
            state.memory.write<4>(state.r<4>(12), clock); state.pc = state.r<4>(4128);
        } else {
            // Zero formatting uses an original BlockMoveData dependency.
            require(std::string_view(name) == "CarbonLib::BlockMoveData", "Unexpected nondeterministic clock fixture dependency");
            require(bridge_import(state, name), "Unimplemented deterministic clock fixture dependency");
        }
        state.memory.observer = nullptr;
        put(std::uint8_t(4)); put(state.pc); put(state.registers); put(state.unique); put(state.heap_end); put(state.memory_error);
        for (const auto* table : {&state.allocated, &state.available}) {
            put(std::uint32_t(table->size()));
            for (const auto& [a, n] : *table) { put(a); put(n); }
        }
        put(std::uint8_t(7));
    };
    s.exception_runtime_observer = &recorder;
    run(s);
    s.exception_runtime_observer = nullptr;
    require(clocks == 1 && gzclose(out) == Z_OK, "Clock transcript finalization");
}
}
int main(int argc, char** argv) {
    if (argc != 3) { std::cerr << "Usage: cm-rng-core-checks <original-segments> <private-test-directory>\n"; return 2; }
    try {
        const std::array<std::uint32_t, 7> seeds{0, 1, 65535, 65536, 0x7fffffff, 0x80000000, 0xffffffff};
        for (unsigned i = 0; i < 2; ++i) for (auto seed : seeds)
            for (auto context : {toc, 0x31008000u, 0x34c4u, 0x34c0u}) {
                auto a = std::make_unique<State>();
                registers(*a, rng_core_entries[i], context, cases);
                const auto address = context - 0x34c4;
                // Exactly the required four bytes, including address 0 and the
                // last four-byte interval in the 32-bit address space.
                if (address > UINT32_MAX - 3) continue;
                a->memory.map(address, 4); a->memory.write<4>(address, seed);
                a->wr<4>(12, seed); auto b = clone(*a);
                compare_pair(*a, *b, "leaf " + hex_address(rng_core_entries[i])); ++own_cases[i];
                const auto state = i == 0 ? cm::rng::next_state(seed) : seed;
                require(b->memory.read<4>(address) == state && b->r<4>(12) == (i == 0 ? cm::rng::sample(state) : seed), "Independent LCG oracle mismatch");
            }
        // Consecutive draws compare every complete boundary in minimal memory.
        for (auto seed : seeds) {
            auto a = std::make_unique<State>(); registers(*a, 0x1077b770, toc, cases);
            a->memory.map(toc - 0x34c4, 4); a->memory.write<4>(toc - 0x34c4, seed); auto b = clone(*a);
            auto state = seed;
            for (unsigned n = 0; n < 256; ++n) {
                a->pc = b->pc = 0x1077b770;
                compare_pair(*a, *b, "consecutive draw " + std::to_string(n)); ++own_cases[0];
                state = cm::rng::next_state(state);
                require(b->r<4>(12) == cm::rng::sample(state) && b->memory.read<4>(toc - 0x34c4) == state, "Consecutive mathematical oracle mismatch");
            }
        }
        for (auto seed : seeds) {
            if (!seed) continue;
            for (unsigned alias = 0; alias < 19; ++alias) {
                auto a = initialization(argv[1], seed, cases);
                // Destination aliases test load/store order, including saved
                // registers, linkage, backchain, salt/end/cursor and CRT state.
                const std::array<std::uint32_t, 18> destinations{
                    fixture, fixture + 1, fixture + 2, fixture + 3, fixture + 4, fixture + 8,
                    stack - 16, stack - 12, stack - 8, stack - 4, stack - 80,
                    stack - 40, stack - 36, stack - 32, toc - 0x34c4,
                    toc - 0x7b8c, toc - 0x7b90, toc - 0x7b88};
                if (alias) a->memory.write<4>(toc - 0x7b84, destinations[alias - 1]);
                auto b = clone(*a);
                compare_pair(*a, *b, "initialize seed=" + hex_address(seed) + " alias=" + std::to_string(alias)); ++own_cases[2];
                require(b->rng_calls == std::array<std::uint64_t,3>{2,1,1}, "Initializer composition counts mismatch");
                if (!alias) {
                    auto first = cm::rng::next_state(seed), second = cm::rng::next_state(first);
                    require(b->memory.read<4>(fixture + 8) == fixture + 4096 + 2 * cm::rng::sample(first), "Cursor oracle mismatch");
                    require(b->memory.read<4>(fixture + 4) == fixture + 4096 + 2 * 50999, "Inclusive end oracle mismatch");
                    require(b->memory.read<4>(fixture) == cm::rng::sample(second), "Salt oracle mismatch");
                }
            }
        }
        // Relocated complete TOC retains its mutable pointers and dependency ABI.
        for (auto base : {0u, 0x10000000u, 0xffff0000u, 0xfffffff0u}) {
            auto a = initialization(argv[1], 1, cases);
            const auto relocated = 0x31008000u;
            a->memory.map(relocated - 0x8000, 0x10000);
            a->memory.copy_in(relocated - 0x8000, a->memory.copy_out(toc - 0x8000, 0x10000));
            a->wr<4>(8, relocated); a->memory.write<4>(relocated - 0x7b8c, base);
            auto b = clone(*a); compare_pair(*a, *b, "relocated/wrapping pool"); ++own_cases[2];
        }
        for (int offset : {-3,-2,-1,1,2,3}) {
            auto a = initialization(argv[1], 0xffffffff);
            a->memory.write<4>(toc - 0x7b84, toc - 0x34c4 + offset);
            auto b = clone(*a); compare_pair(*a, *b, "partial cursor/CRT alias"); ++own_cases[2];
        }
        // Ordinary 15-bit draws cannot initially select index 50999. A
        // partial CRT alias can reach equality, testing the inclusive branch.
        {
            auto a = initialization(argv[1], 0x922aa683);
            a->memory.write<4>(toc - 0x7b8c, 0x27f60000);
            a->memory.write<4>(toc - 0x7b84, toc - 0x34c4 + 2);
            auto b = clone(*a); compare_pair(*a, *b, "inclusive cursor end equality"); ++own_cases[2];
            require(b->r<4>(32) == 0x27f60000u + 101998u && (b->r<1>(2304) & 14) == 2,
                    "Inclusive cursor end branch was not reached");
        }
        {
            auto a = initialization(argv[1], 0xfc77a683); auto b = clone(*a);
            compare_pair(*a, *b, "cursor at pool start"); ++own_cases[2];
            require(b->memory.read<4>(fixture + 8) == fixture + 4096, "Initial cursor start not reached");
        }
        {
            auto a = initialization(argv[1], 1);
            const auto context = stack + 0x7b80;
            const auto table = a->memory.copy_out(toc - 0x8000, 0x10000);
            a->memory.map(context - 0x8000, 0x10000); a->memory.copy_in(context - 0x8000, table);
            a->wr<4>(8, context); a->wr<4>(31 * 4, fixture + 8);
            a->wr<4>(30 * 4, fixture + 4); a->wr<4>(28 * 4, fixture);
            auto b = clone(*a); compare_pair(*a, *b, "TOC slots alias ordered entry spills"); ++own_cases[2];
        }
        // Re-run the initializer with only the exact bytes accessed by the
        // original body/dependencies mapped, not the full program data ranges.
        {
            auto initial = initialization(argv[1], 1); auto probe = clone(*initial);
            Effects accesses; probe->rng_original = true; probe->memory.date_observer = &accesses; run(*probe);
            probe->memory.date_observer = nullptr;
            auto a = std::make_unique<State>();
            a->registers = initial->registers; a->unique = initial->unique; a->pc = initial->pc; a->budget = initial->budget;
            for (const auto& [address, size] : accesses.accesses) a->memory.map(address, size);
            for (const auto& [address, size] : a->memory.mapped_ranges()) a->memory.copy_in(address, initial->memory.copy_out(address, size));
            auto b = clone(*a); compare_pair(*a, *b, "initializer exact mappings"); ++own_cases[2];
        }
        // Original parent/semantic children and semantic parent/original children.
        for (unsigned mode = 0; mode < 2; ++mode) {
            auto a = initialization(argv[1], 65536); auto b = clone(*a);
            b->rng_entry_original = mode == 0 ? std::array<bool,3>{true,true,false} : std::array<bool,3>{false,false,true};
            compare_pair(*a, *b, "mixed RNG composition"); ++own_cases[2];
            require(b->rng_calls[mode == 0 ? 2 : 0] != 0, "Mixed mode lost own coverage");
        }
        // The generated sampler remains unchanged and renews salt through the hook.
        for (auto bound : {0,1,-1,17,-17,65535,-65535,65536,-65536,1000000}) for (unsigned position : {0u,50998u,50999u}) {
            auto a = initialization(argv[1], 1); a->pc = 0x107124e0; a->wr<4>(12, std::uint32_t(bound));
            for (unsigned enable = 1304; enable <= 1308; ++enable) a->wr<1>(enable, 0);
            a->memory.write<4>(fixture, 123); a->memory.write<4>(fixture + 4, fixture + 4096 + 2 * 50999);
            a->memory.write<4>(fixture + 8, fixture + 4096 + 2 * position);
            auto b = clone(*a); compare_pair(*a, *b, "generated sampler wrap");
            if (bound && position == 50999) require(b->rng_calls[0] != 0, "Sampler wrap bypassed rand hook");
        }
        // Each negative is injected at that entry's first own return boundary.
        // A child rejection is identified before its parent returns.
        for (auto target : rng_core_entries) {
            auto a = initialization(argv[1], 1); auto b = clone(*a);
            Boundaries boundaries; a->rng_observer = &boundaries; a->rng_original = true; run(*a);
            b->rng_observer = &boundaries; run(*b); require(boundaries.cursor == boundaries.expected.size(), "Missing nested boundary");
            auto c = initialization(argv[1], 1); c->rng_observer = &boundaries;
            boundaries.cursor = 0; boundaries.perturb = target;
            bool rejected = false;
            try { run(*c); } catch (const std::runtime_error& e) {
                rejected = std::string(e.what()).starts_with("Own RNG boundary " + hex_address(target));
            }
            require(rejected && boundaries.mutated, "Own RNG perturbation was not rejected at its first boundary");
            reset_rng_diagnostics(*c);
            require(!c->rng_observer && !c->rng_original_resume && c->rng_calls == std::array<std::uint64_t,3>{}, "Diagnostic reset leaked state");
        }
        // Callback marshalling is checked separately from every direct ABI case.
        for (auto entry : rng_core_entries) {
            auto a = initialization(argv[1], 1); a->memory.write<4>(fixture + 512, entry); a->memory.write<4>(fixture + 516, toc);
            auto b = clone(*a);
            compare_pair(*a, *b, "callback " + hex_address(entry), [&](State& s) {
                const std::array<std::uint32_t,1> arguments{1}; (void)call_original(s, fixture + 512, arguments);
            });
            auto indirect = initialization(argv[1], 1);
            indirect->memory.write<4>(fixture + 512, entry); indirect->memory.write<4>(fixture + 516, toc);
            indirect->memory.write<4>(toc - 0x7ee4, fixture + 512); indirect->pc = 0x10780900;
            auto candidate = clone(*indirect);
            compare_pair(*indirect, *candidate, "generated indirect transition " + hex_address(entry));
        }
        {
            auto a = initialization(argv[1], 1); auto b = clone(*a);
            a->rng_trace = b->rng_trace = true;
            compare_pair(*a, *b, "compact ordered evidence");
            require(a->rng_ordered_digest == b->rng_ordered_digest, "Compact ordered boundary evidence differs");
            const auto directory = std::filesystem::path(argv[2]); std::filesystem::create_directories(directory);
            const auto first = directory / "snapshot-before.bin", second = directory / "snapshot-diagnostics.bin";
            save_runtime_snapshot(*b, first);
            b->rng_original = true; b->rng_calls.fill(0xdeadbeef); b->rng_original_calls.fill(123);
            b->rng_entry_original.fill(true); b->rng_ordered_digest = 123; b->rng_original_resume = 0x1077b770;
            save_runtime_snapshot(*b, second);
            std::ifstream x(first, std::ios::binary), y(second, std::ios::binary);
            require(std::equal(std::istreambuf_iterator<char>(x), {}, std::istreambuf_iterator<char>(y), {}), "RNG diagnostics entered machine snapshot");
            x.close(); y.close(); std::filesystem::remove(first); std::filesystem::remove(second);
        }
        for (auto seed : seeds) {
            const auto directory = std::filesystem::path(argv[2]) / ("clock-" + std::to_string(seed));
            if (!seed) {
                auto candidate = initialization(argv[1], 0);
                bool rejected = false;
                try { controlled_clock_transcript(*candidate, directory, 1); }
                catch (const std::runtime_error& e) { rejected = std::string(e.what()).find("requires original RNG selection") != std::string::npos; }
                require(rejected, "Candidate was allowed to record a clock oracle");
            }
            auto probe = initialization(argv[1], 0); probe->rng_original = true; controlled_clock_transcript(*probe, directory, seed + 2082844800u);
            auto a = initialization(argv[1], 0); auto b = clone(*a);
            compare_pair(*a, *b, "controlled zero seed", [&](State& s) {
                open_season_replay(directory, true); run(s); require_season_replay_eof(); close_season_replay();
            }); ++own_cases[2];
            require(b->native_calls >= 1 && b->memory.read<4>(toc - 0x34c4) == cm::rng::next_state(cm::rng::next_state(seed)), "Controlled clock/seed oracle mismatch");
        }
        const auto good_clock = std::filesystem::path(argv[2]) / "clock-0/imports.gz";
        for (unsigned defect = 0; defect < 3; ++defect) {
            const auto directory = std::filesystem::path(argv[2]) / ("damaged-" + std::to_string(defect));
            std::filesystem::create_directories(directory);
            std::ifstream source(good_clock, std::ios::binary);
            std::vector<char> bytes{std::istreambuf_iterator<char>(source), {}};
            if (defect == 0) bytes.resize(bytes.size() / 2);
            if (defect == 1) bytes[bytes.size() - 8] ^= 1;
            const auto path = directory / "imports.gz";
            { std::ofstream out(path, std::ios::binary); out.write(bytes.data(), bytes.size()); }
            if (defect == 2) {
                gzFile trailing = gzopen(path.c_str(), "ab1");
                require(trailing && gzwrite(trailing, "extra", 5) == 5 && gzclose(trailing) == Z_OK, "Cannot create trailing data control");
            }
            auto candidate = initialization(argv[1], 0);
            bool rejected = false;
            try { open_season_replay(directory, true); run(*candidate); require_season_replay_eof(); }
            catch (const std::runtime_error&) { rejected = true; }
            close_season_replay(); require(rejected, "Damaged controlled clock transcript was accepted");
        }
        std::cout << "PASS: RNG direct ABI/memory/ordered effects cases=" << cases
                  << " rand=" << own_cases[0] << " seed=" << own_cases[1] << " initializer=" << own_cases[2]
                  << "; own negatives=3; callbacks=3; controlled clocks=7; generated consumers=30\n";
        return 0;
    } catch (const std::exception& e) { close_season_replay(); std::cerr << e.what() << '\n'; return 1; }
}
