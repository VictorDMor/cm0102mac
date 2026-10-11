#include "vector_context.hpp"
#include "exception_runtime_context.hpp"
#include "semantic_io_checks.hpp"
#include <limits>

using namespace cm::semantic_io_checks;
namespace cm::aot { bool exception_context_reference(State&); }
namespace {
unsigned cases = 0;
unsigned comparisons = 0;
unsigned context_faults = 0;
void reference_until(State& s, std::uint32_t stop) {
    while (s.pc != stop) {
        if (!cm::aot::exception_context_reference(s)) run_until(s, std::uint32_t(s.r<4>(4128)));
    }
}
Vector128 pattern(unsigned n) {
    Vector128 value;
    for (unsigned i = 0; i < value.size(); ++i) value[i] = std::uint8_t(n * 29 + i * 7);
    return value;
}
template<class F> void rejects(F operation, const char* label) {
    bool rejected = false;
    try { operation(); } catch (const std::exception&) { rejected = true; }
    require(rejected, label); ++cases;
}
void storage_checks() {
    State s;
    s.registers.fill(0x5b); s.unique.fill(0x6d);
    const auto saved = s.registers;
    for (unsigned n = 0; n < 32; ++n) {
        s.write_vector_register(0x4200 + n * 16, pattern(n));
        require(s.read_vector_register(0x4200 + n * 16) == pattern(n), "Vector register bytes");
        require(s.r<8>(0x4200 + n * 16) == read_be<8>(pattern(n).data()), "Vector/scalar register alias");
        ++cases;
    }
    require(std::equal(saved.begin(), saved.begin() + 0x4200, s.registers.begin()), "Scalar registers preserved");
    require(s.registers[0x4400] == 0x5b, "VSCR boundary guard");
    s.write_vector_temporary(0x44af0, pattern(37));
    require(s.read_vector_temporary(0x44af0) == pattern(37), "High vector temporary bytes");
    require(s.unique[0x44aef] == 0x6d && s.unique[0x44b00] == 0x6d, "Vector temporary boundary guards");
    for (auto offset : {s.registers.size() - 15, std::numeric_limits<std::size_t>::max()}) {
        rejects([&] { s.write_vector_register(offset, pattern(0)); }, "Vector register overflow");
        rejects([&] { (void)s.read_vector_register(offset); }, "Vector register read overflow");
    }
    rejects([&] { s.write_vector_temporary(s.unique.size() - 15, pattern(0)); }, "Vector temporary overflow");
    s.memory.map(fixture, 32); s.memory.clear(fixture, 32);
    Effects effects; effects.state = &s; s.memory.date_observer = &effects;
    s.memory.write_vector(fixture + 16, pattern(3));
    require(s.memory.read_vector(fixture + 16) == pattern(3), "Vector memory bytes");
    s.memory.date_observer = nullptr;
    require(effects.bytes.size() == 2 * (1 + 4 + 8 + 16), "One observer event per complete vector access");
    require(effects.bytes[0] == 2 && effects.bytes[29] == 4, "Vector access order"); ++cases;
    State partial; partial.memory.map(fixture, 15); partial.memory.clear(fixture, 15);
    Effects failed; failed.state = &partial; partial.memory.date_observer = &failed;
    const auto prefix = partial.memory.copy_out(fixture, 15);
    rejects([&] { partial.memory.write_vector(fixture, pattern(1)); }, "Partial vector store must fault");
    rejects([&] { (void)partial.memory.read_vector(fixture); }, "Partial vector load must fault");
    require(failed.bytes.empty() && partial.memory.copy_out(fixture, 15) == prefix, "Vector fault has no partial effects");
    State high; high.memory.map(0xfffffff0, 16); high.memory.write_vector(0xfffffff0, pattern(4));
    require(high.memory.read_vector(0xfffffff0) == pattern(4), "Last guest vector accessible"); ++cases;
    for (auto address : {UINT64_C(0xfffffff1), UINT64_C(0x100000000), UINT64_MAX}) {
        rejects([&] { (void)high.memory.read_vector(address); }, "Wrapping vector load must fault");
        rejects([&] { high.memory.write_vector(address, pattern(0)); }, "Wrapping vector store must fault");
    }
    for (unsigned lane = 0; lane < 4; ++lane) {
        s.memory.write<4>(fixture + lane * 4, 0xa0b0c000 + lane);
        const auto value = load_vector_word(s, fixture, lane * 4 + 3);
        require(read_be<4>(value.data() + lane * 4) == 0xa0b0c000 + lane, "Word address and lane alignment");
        require(vector_word_for_address(pattern(8), fixture + lane * 4 + 3) ==
            read_be<4>(pattern(8).data() + lane * 4), "Word store lane selection");
        for (unsigned other = 0; other < 4; ++other)
            if (other != lane) require(read_be<4>(value.data() + other * 4) == 0, "Defined choice for undefined lanes");
        ++cases;
    }
    for (auto status : {0u, 1u, 0x10000u, 0x10001u, 0xffffffffu}) {
        Vector128 value{}; write_be<4>(value.data() + 12, status); move_to_vector_status(s, value);
        const auto returned = vector_status_register(s);
        require(s.r<4>(vector_status_offset) == (status & 0x10001), "Defined VSCR bits");
        require(read_be<8>(returned.data()) == 0 && read_be<4>(returned.data() + 8) == 0 &&
            read_be<4>(returned.data() + 12) == (status & 0x10001), "VSCR vector placement"); ++cases;
    }
}
std::unique_ptr<State> setup(const char* program) {
    auto s = std::make_unique<State>(); load_program(*s, program);
    s->memory.map(fixture, 0x10000); s->memory.clear(fixture, 0x10000);
    s->memory.copy_in(stack - 8192, std::vector<std::uint8_t>(8256, 0xa7));
    s->registers.fill(0x5b); s->unique.fill(0x6d);
    for (unsigned n = 0; n < 32; ++n) {
        s->wr<4>(n * 4, 0xcafe0000 + n);
        s->wr<8>(0x4000 + n * 16, UINT64_C(0x1234000000000000) + n);
        s->write_vector_register(0x4200 + n * 16, pattern(n));
    }
    s->wr<4>(4, stack); s->wr<4>(8, toc); s->wr<4>(4128, return_sentinel);
    s->wr<4>(vector_status_offset, 0x10001); s->budget = 10000;
    return s;
}
std::unique_ptr<State> without_byte(const State& initial, std::uint32_t omitted) {
    auto s = std::make_unique<State>();
    s->registers = initial.registers; s->unique = initial.unique; s->pc = initial.pc;
    s->budget = initial.budget; s->heap_end = initial.heap_end; s->memory_error = initial.memory_error;
    s->allocated = initial.allocated; s->available = initial.available; s->platform = initial.platform;
    const auto copy = [&](std::uint32_t address, std::uint64_t size) {
        if (!size) return;
        s->memory.map(address, size); s->memory.copy_in(address, initial.memory.copy_out(address, size));
    };
    for (const auto& [address, size] : initial.memory.mapped_ranges()) {
        if (omitted >= address && std::uint64_t(omitted) < std::uint64_t(address) + size) {
            copy(address, omitted - address);
            const auto rest = std::uint64_t(address) + size - omitted - 1;
            if (rest) copy(omitted + 1, rest);
        } else copy(address, size);
    }
    return s;
}
void compare_context_faults(const State& initial, bool capture, std::initializer_list<std::uint32_t> omitted) {
    for (auto address : omitted) {
        auto original = without_byte(initial, address); auto semantic = clone(*original);
        Effects a, b; a.state = original.get(); b.state = semantic.get();
        original->memory.date_observer = &a; semantic->memory.date_observer = &b;
        std::string original_error, semantic_error;
        try { reference_until(*original, capture ? 0x10772810 : return_sentinel); }
        catch (const std::runtime_error& error) { original_error = error.what(); }
        try {
            if (capture) cm::aot::exception_runtime::capture_vector_throw(*semantic);
            else cm::aot::exception_runtime::restore_context(*semantic);
        } catch (const std::runtime_error& error) { semantic_error = error.what(); }
        original->memory.date_observer = semantic->memory.date_observer = nullptr;
        require(original_error.starts_with("Unmapped data access at ") && original_error == semantic_error,
            "Context mapping fault outcome original=" + original_error + " semantic=" + semantic_error);
        Image(*original).compare(*semantic, "Context fault architecture, PC and memory prefix");
        require(a.bytes == b.bytes, "Context fault ordered access prefix"); ++context_faults; ++comparisons;
    }
}
void original_context_checks(const char* program) {
    struct Scenario { std::uint32_t mask; bool vectors; };
    std::vector<Scenario> scenarios{{0, true}, {0xfff, true}, {0x101, true}, {0x80000100, true}, {0xfff, false}};
    for (unsigned bit = 0; bit < 12; ++bit) scenarios.push_back({1u << bit, true});
    for (auto [mask, vectors] : scenarios) {
        auto s = setup(program); const auto old_vectors = s->registers;
        const auto context = fixture + 0x2000 + (mask == 0x101 ? 3u : 0u);
        constexpr auto backchain = fixture + 0x4000, restored_stack = fixture + 0x5000;
        const auto live_vrsave = s->r<4>(5120);
        for (unsigned n = 13; n < 32; ++n) s->memory.write<4>(context + 0x34c + (n - 13) * 4, 0xa1100000 + n);
        for (unsigned n = 14; n < 32; ++n) s->memory.write<8>(context + 0x288 + (n - 14) * 8, UINT64_C(0xabcd000000000000) + n);
        for (unsigned n = 20; n < 32; ++n) s->memory.write_vector((context + n * 16) & ~std::uint32_t(15), pattern(n + 40));
        s->memory.write<4>(context + 0x398, 0x12345678); s->memory.write<4>(context + 0x200, 0x13572468);
        s->memory.write<4>(context + 0x210, mask); s->memory.write<4>(context + 0x39c, backchain);
        s->memory.write<4>(context + 0x3a4, restored_stack); s->memory.write<4>(backchain, 0x98765430);
        s->wr<4>(12, context); s->wr<4>(16, toc); s->wr<4>(20, return_sentinel); s->wr<4>(24, vectors);
        s->pc = 0x10772650; Effects effects; effects.state = s.get(); s->memory.date_observer = &effects;
        auto semantic = clone(*s); Effects semantic_effects; semantic_effects.state = semantic.get();
        semantic->memory.date_observer = &semantic_effects;
        if (mask == 0xfff && vectors) compare_context_faults(*s, false, {context + 0x398, context + 0x210,
            context + 0x14f, context + 0x34f, context + 0x28f, context + 0x200,
            context + 0x39c, backchain + 3, restored_stack + 3});
        reference_until(*s, return_sentinel); s->memory.date_observer = nullptr;
        cm::aot::exception_runtime::restore_context(*semantic); semantic->memory.date_observer = nullptr;
        Image(*s).compare(*semantic, "Restored-context full architecture and memory");
        require(effects.bytes == semantic_effects.bytes, "Restored-context ordered accesses"); ++comparisons;
        require(s->pc == return_sentinel && s->r<4>(4) == restored_stack && s->r<4>(12) == 0x98765430,
            "Original restored-context transfer and live backchain");
        require(s->memory.read<4>(restored_stack) == 0x98765430 && s->r<4>(5120) == (vectors ? mask : live_vrsave),
            "Original restored-context memory and VRSAVE");
        require(s->r<4>(vector_status_offset) == (vectors ? 0u : 0x10001u), "Original VSCR restore policy");
        unsigned first = 32; for (unsigned n = 20; n < 32; ++n) if ((mask & 0xfff) & (1u << (31 - n))) { first = n; break; }
        for (unsigned n = 20; n < 32; ++n) {
            Vector128 expected;
            std::copy_n(old_vectors.data() + 0x4200 + n * 16, 16, expected.begin());
            if (vectors && n >= first) expected = pattern(n + 40);
            require(s->read_vector_register(0x4200 + n * 16) == expected, "Original suffix vector restore");
        }
        for (unsigned n = 13; n < 32; ++n) require(s->r<4>(n * 4) == 0xa1100000 + n, "Original GPR restore");
        for (unsigned n = 14; n < 32; ++n) require(s->r<8>(0x4000 + n * 16) == UINT64_C(0xabcd000000000000) + n, "Original FPR restore");
        ++cases;
        auto snapshot = setup(program); snapshot->wr<4>(5120, mask);
        if (mask == 0x101) snapshot->wr<4>(4, stack + 4);
        const auto before = snapshot->registers; snapshot->pc = 0x10772b10;
        snapshot->wr<4>(12, fixture + 0x6000); snapshot->wr<4>(16, fixture + 0x7000); snapshot->wr<4>(20, fixture + 0x8000);
        auto semantic_snapshot = clone(*snapshot); Effects captured, semantic_captured;
        captured.state = snapshot.get(); semantic_captured.state = semantic_snapshot.get();
        snapshot->memory.date_observer = &captured; semantic_snapshot->memory.date_observer = &semantic_captured;
        if (mask == 0xfff && vectors) compare_context_faults(*snapshot, true, {stack - 1, stack + 0x18, stack - 0x400,
            stack - 0x3c0 + 0x34f, stack - 0x3c0 + 0x210, stack - 0x3c0 + 0x14f,
            stack - 0x3c0 + 0x28f, stack - 0x3c0 + 0x398, stack - 0x3c0 + 0x39c});
        reference_until(*snapshot, 0x10772810);
        cm::aot::exception_runtime::capture_vector_throw(*semantic_snapshot);
        snapshot->memory.date_observer = semantic_snapshot->memory.date_observer = nullptr;
        Image(*snapshot).compare(*semantic_snapshot, "Vector capture prefix full architecture and memory");
        require(captured.bytes == semantic_captured.bytes, "Vector capture prefix ordered accesses"); ++comparisons;
        const auto saved_context = std::uint32_t(snapshot->r<4>(12));
        require(saved_context == stack - 0x3c0 && snapshot->r<4>(5120) == 0, "Original aligned snapshot context");
        require(snapshot->memory.read<4>(saved_context + 0x210) == mask, "Original snapshot VRSAVE");
        require(snapshot->memory.read<4>(saved_context + 0x200) == 0, "Original word-zero VSCR store quirk");
        require(snapshot->memory.read<4>(saved_context + 0x3ac) == fixture + 0x6000 &&
            snapshot->memory.read<4>(saved_context + 0x3b0) == fixture + 0x7000 &&
            snapshot->memory.read<4>(saved_context + 0x3b4) == fixture + 0x8000, "Original throw arguments captured");
        for (unsigned n = first; n < 32; ++n) {
            Vector128 expected; std::copy_n(before.data() + 0x4200 + n * 16, 16, expected.begin());
            require(snapshot->memory.read_vector(saved_context + n * 16) == expected, "Original suffix vector snapshot");
        }
        ++cases;
    }
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-vector-context-checks <program>");
        storage_checks(); original_context_checks(argv[1]);
        std::cout << cases << " vector storage and original context checks, " << comparisons
                  << " C++ context comparisons (" << context_faults << " mapping faults) passed\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
