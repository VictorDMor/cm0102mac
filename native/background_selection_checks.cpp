#include "background_selection_adapter.hpp"
#include "callbacks.hpp"
#include "rng_core.hpp"
#include <algorithm>
#include <functional>
#include <iostream>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000, stack = 0x700fff00, toc = 0x108bba90;
constexpr std::uint32_t category_table = fixture + 0x20000, catalog = fixture + 0x600;
constexpr std::uint32_t tree = fixture + 0x800, pool = fixture + 0x5000;
constexpr std::uint32_t salt = fixture + 0x5100, limit = fixture + 0x5104, cursor = fixture + 0x5108;
constexpr std::uint32_t descriptor = fixture + 0x5200, missing_descriptor = fixture + 0x5240;
constexpr std::uint32_t callback_vector = fixture + 0x5300;
constexpr std::uint32_t category_id = 1, unmapped = 0xffff0000;
constexpr unsigned list_size = 9;
constexpr std::uint32_t lists(unsigned item) { return fixture + 0x1000 + item * 0x40; }
constexpr std::uint32_t node(unsigned item, bool alternate, unsigned index) {
    return fixture + 0x2000 + (item * 2 + alternate) * 0x100 + index * 0x10;
}
constexpr std::uint32_t picture(unsigned item, bool alternate, unsigned index) {
    return fixture + 0x3000 + (item * 2 + alternate) * 0x200 + index * 0x20;
}
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
        number(kind, 1); number(address, 4); number(size, 8); accesses.emplace_back(address, size);
        if (data) bytes.insert(bytes.end(), data, data + size);
    }
    void mapped(std::uint32_t address, std::uint64_t size) override { event(1, address, size); }
    void written(std::uint32_t address, const std::uint8_t* data, std::size_t size) override { event(2, address, size, data); }
    void cleared(std::uint32_t address, std::uint64_t size) override { event(3, address, size); }
    void read(std::uint32_t address, const std::uint8_t* data, unsigned size) override { event(4, address, size, data); }
};
// Instruction accounting and process-local hook diagnostics are intentionally
// excluded. Every architectural byte, including generated temporary residues,
// and every mapped memory byte is compared at returns and faults.
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
        if (unique != s.unique) {
            const auto p = std::mismatch(unique.begin(), unique.end(), s.unique.begin());
            throw std::runtime_error(label + " temporary byte=" + std::to_string(p.first - unique.begin()) +
                " expected=" + std::to_string(*p.first) + " actual=" + std::to_string(*p.second));
        }
        require(pc == s.pc, label + " PC expected=" + hex_address(pc) + " actual=" + hex_address(s.pc));
        require(heap_end == s.heap_end && native_calls == s.native_calls && memory_error == s.memory_error &&
                allocated == s.allocated && available == s.available, label + " allocator/import mismatch");
        const auto ranges = s.memory.mapped_ranges();
        require(ranges.size() == memory.size(), label + " mapping count mismatch");
        for (unsigned i = 0; i < memory.size(); ++i) {
            const auto& [address, bytes] = memory[i];
            require(ranges[i].first == address && ranges[i].second == bytes.size(), label + " mapping mismatch");
            const auto actual = s.memory.copy_out(address, bytes.size());
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
void arguments(State& s, unsigned entry, std::uint32_t selector = 0) {
    s.pc = background_selection_entries[entry];
    s.wr<4>(12, entry == 0 ? catalog : entry == 1 ? lists(0) : category_id);
    s.wr<4>(16, entry == 3 ? 1 : selector);
    s.wr<4>(20, descriptor); s.wr<4>(24, selector);
}
std::unique_ptr<State> setup(const char* program, unsigned entry, unsigned variation = 0) {
    auto s = std::make_unique<State>(); load_program(*s, program);
    s->memory.map(fixture, 0x60000);
    s->memory.copy_in(stack - 8192, std::vector<std::uint8_t>(8256, 0xa7));
    s->registers.fill(0x5b); s->unique.fill(0x6d);
    for (unsigned r = 0; r < 32; ++r) s->wr<4>(r * 4, 0xcafe0000u + r);
    s->wr<4>(4, stack); s->wr<4>(8, toc); s->wr<4>(4128, return_sentinel);
    s->wr<4>(4132, 0x31415926); s->wr<4>(2440, 0xbadc0ffe);
    s->wr<1>(2304, variation % 2 ? 0xf9 : 0xa6);
    s->wr<1>(1024, variation % 4); s->wr<1>(1027, (variation / 2) % 2);
    s->memory.write<1>(toc - 0x35d4, 1);
    s->memory.write<4>(toc - 0x5c3c, category_table);
    for (auto id : {0u, category_id, 3u}) s->memory.write<4>(category_table + id * 4, catalog);
    s->memory.write<4>(catalog, tree);
    // Retain the original category comparator transition vector. Descriptor
    // matches execute the generated tree, transition stub and string comparison.
    s->memory.write<4>(catalog + 4, s->memory.read<4>(toc - 0x5c38));
    s->memory.write<4>(catalog + 12, 3);
    for (unsigned item = 0; item < 3; ++item) {
        const auto branch = tree + item * 0x20;
        s->memory.write<4>(branch, descriptor + item * 8);
        s->memory.write<4>(branch + 4, lists(item));
        s->memory.write<4>(branch + 8, item == 0 ? tree + 0x20 : 0);
        s->memory.write<4>(branch + 12, item == 0 ? tree + 0x40 : 0);
        for (bool alternate : {false, true}) {
            const auto list = lists(item) + (alternate ? 12 : 0);
            s->memory.write<4>(list, node(item, alternate, 0));
            s->memory.write<4>(list + 4, node(item, alternate, list_size - 1));
            s->memory.write<4>(list + 8, list_size);
            for (unsigned index = 0; index < list_size; ++index) {
                s->memory.write<4>(node(item, alternate, index), picture(item, alternate, index));
                s->memory.write<4>(node(item, alternate, index) + 4,
                    index + 1 == list_size ? 0 : node(item, alternate, index + 1));
                s->memory.copy_in(picture(item, alternate, index), {'p','i','c','t','u','r','e',0});
            }
        }
    }
    s->memory.copy_in(descriptor, {'m',0,0,0,0,0,0,0,'z',0,0,0,0,0,0,0,'a',0});
    s->memory.copy_in(missing_descriptor, {'x',0});
    s->memory.write<4>(toc - 0x7b90, salt); s->memory.write<4>(toc - 0x7b8c, pool);
    s->memory.write<4>(toc - 0x7b88, limit); s->memory.write<4>(toc - 0x7b84, cursor);
    s->memory.write<4>(salt, 0); s->memory.write<4>(limit, pool + 14); s->memory.write<4>(cursor, pool);
    s->memory.write<4>(toc - 0x34c4, 1);
    for (unsigned i = 0; i < 8; ++i) s->memory.write<2>(pool + i * 2, i);
    s->executed = s->native_calls = 0; s->budget = 100000;
    arguments(*s, entry);
    return s;
}
unsigned cases = 0, faults = 0;
std::array<unsigned, 4> own_cases{};
std::string compare_pair(State& original, State& modern, const std::string& label, bool expect_fault = false,
                  const std::function<void(State&)>& execute = [](State& s) { run(s); }) {
    require(!original.rng_original && !modern.rng_original && !original.date_subsystem_original &&
        !modern.date_subsystem_original && !original.calendar_original && !modern.calendar_original,
        "Background comparison must retain semantic RNG/Date dependencies");
    Effects expected, actual;
    original.background_original = true;
    original.memory.date_observer = &expected; modern.memory.date_observer = &actual;
    std::string expected_fault, actual_fault;
    try { execute(original); } catch (const std::exception& e) { expected_fault = e.what(); }
    try { execute(modern); } catch (const std::exception& e) { actual_fault = e.what(); }
    original.memory.date_observer = modern.memory.date_observer = nullptr;
    if (!expect_fault && !expected_fault.empty()) throw std::runtime_error(label + " original: " + expected_fault + " pc=" + hex_address(original.pc));
    require(expected_fault == actual_fault, label + " fault mismatch original=" + expected_fault + " semantic=" + actual_fault);
    require(expect_fault == !expected_fault.empty(), label + " expected fault was not reached");
    if (expect_fault) require(expected_fault.starts_with("Unmapped data access at "),
        label + " expected an unmapped-access fault, received: " + expected_fault);
    Image(original).compare(modern, label);
    if (expected.bytes != actual.bytes) {
        const auto p = std::mismatch(expected.bytes.begin(), expected.bytes.end(), actual.bytes.begin(), actual.bytes.end());
        throw std::runtime_error(label + " ordered access mismatch byte=" + std::to_string(p.first - expected.bytes.begin()) +
            " sizes=" + std::to_string(expected.bytes.size()) + "/" + std::to_string(actual.bytes.size()));
    }
    require(original.rng_calls == modern.rng_calls && original.rng_original_calls == modern.rng_original_calls,
        label + " RNG dependency dispatch mismatch");
    ++cases; if (expect_fault) ++faults;
    return expected_fault;
}
std::string check(std::unique_ptr<State> initial, unsigned entry, const std::string& label, bool expect_fault = false) {
    auto modern = clone(*initial);
    const auto fault = compare_pair(*initial, *modern, label, expect_fault); ++own_cases[entry];
    return fault;
}
struct Boundaries final : BackgroundBoundaryObserver {
    struct Boundary { std::uint32_t entry; Image image; Boundary(std::uint32_t e, const State& s) : entry(e), image(s) {} };
    std::vector<Boundary> expected;
    std::size_t cursor = 0;
    std::uint32_t perturb = 0;
    bool mutated = false;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& s, std::uint32_t entry, bool original) override {
        if (original) { expected.emplace_back(entry, s); return; }
        if (entry == perturb && !mutated) { mutated = true; s.wr<4>(12, s.r<4>(12) ^ 1); }
        require(cursor < expected.size() && expected[cursor].entry == entry, "Background boundary order mismatch");
        expected[cursor++].image.compare(s, "Own background boundary " + hex_address(entry));
    }
};
void relocate_toc(State& s, std::uint32_t context) {
    const auto table = s.memory.copy_out(toc - 0x8000, 0x10000);
    s.memory.map(context - 0x8000, 0x10000); s.memory.copy_in(context - 0x8000, table); s.wr<4>(8, context);
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-background-selection-checks <original-segments>\n"; return 2; }
    try {
        // Independent fixture choices establish useful nonempty production
        // dispatch, in addition to comparison against original generated bodies.
        const std::array<std::uint32_t, 4> expected{picture(1, false, 2), picture(0, false, 1),
            picture(1, false, 2), picture(0, false, 1)};
        for (unsigned entry = 0; entry < 4; ++entry) {
            auto a = setup(argv[1], entry, cases); auto b = clone(*a);
            compare_pair(*a, *b, "nonempty production " + hex_address(background_selection_entries[entry])); ++own_cases[entry];
            require(b->r<4>(12) == expected[entry] && b->background_calls[entry] != 0 &&
                b->background_original_calls == std::array<std::uint64_t,4>{}, "Nonempty semantic production dispatch was not reached");
            require(b->memory.read<4>(cursor) == pool + (entry == 0 || entry == 2 ? 4 : 2),
                "Independent advance-before-sample fixture oracle mismatch");
        }
        for (unsigned entry : {0u, 1u}) {
            for (auto count : {0u,1u,3u,9u,0x7fffu,0x8000u,0xffffu,0x10000u,0x10001u,0xffff0003u,0x80000001u}) {
                auto a = setup(argv[1], entry, cases);
                a->memory.write<4>(entry == 0 ? catalog + 12 : lists(0) + 8, count);
                check(std::move(a), entry, "signed16 count " + hex_address(count) + " entry=" + std::to_string(entry));
            }
            auto a = setup(argv[1], entry); a->wr<4>(12, 0); auto b = clone(*a);
            compare_pair(*a, *b, "null collection/list"); ++own_cases[entry];
            require(!b->r<4>(12) && b->memory.read<4>(cursor) == pool, "Null collection consumed RNG");
            for (auto position : {0u, 12u, 14u}) {
                a = setup(argv[1], entry, cases); a->memory.write<4>(cursor, pool + position); b = clone(*a);
                compare_pair(*a, *b, "inclusive cursor/wrap entry=" + std::to_string(entry) + " offset=" + std::to_string(position)); ++own_cases[entry];
                const bool wraps = position == 14 || (entry == 0 && position == 12);
                require(b->rng_calls[0] == (wraps ? 1u : 0u), "Inclusive end or wrap changed RNG consumption");
                if (wraps) require(b->memory.read<4>(toc - 0x34c4) == cm::rng::next_state(1) &&
                    b->memory.read<4>(salt) == cm::rng::sample(cm::rng::next_state(1)), "Wrap renewal oracle mismatch");
            }
            for (auto value : {0x7fffffffu,0x80000000u,0xffff0000u,0xffffffffu}) {
                a = setup(argv[1], entry, cases); a->memory.write<4>(salt, value);
                check(std::move(a), entry, "signed32 sample/salt " + hex_address(value));
            }
            a = setup(argv[1], entry, cases);
            a->memory.write<4>(entry == 0 ? catalog + 12 : lists(0) + 8, 0xffff);
            a->memory.write<4>(salt, 0x7fffffff); b = clone(*a);
            compare_pair(*a, *b, "signed32 INT_MIN remainder by signed16 -1"); ++own_cases[entry];
            require(b->r<4>(12) == picture(0, false, 0), "INT_MIN/-1 selection oracle mismatch");
            a = setup(argv[1], entry, cases); relocate_toc(*a, 0x31008000);
            check(std::move(a), entry, "relocated sampler TOC");
            for (unsigned alias = 0; alias < 8; ++alias) {
                a = setup(argv[1], entry, cases);
                a->memory.write<4>(catalog + 12, 1); a->memory.write<4>(lists(0) + 8, 1);
                const std::array<std::uint32_t, 8> destinations{salt,salt + 1,salt + 2,salt + 3,limit,stack - 4,stack - 8,stack - 16};
                const auto destination = destinations[alias];
                a->memory.write<4>(toc - 0x7b84, destination); a->memory.write<4>(destination, pool);
                if (destination >= stack - 16) a->wr<4>((32 - (stack - destination) / 4) * 4, pool);
                // Partial cursor writes also overlap the inclusive limit.
                // Offsets +1/+3 reduce that limit and trigger wrap; renewing
                // salt corrupts their reloaded cursor before the pool sample.
                // Offset +2 instead raises the unsigned limit and succeeds.
                const bool corrupts_cursor_on_wrap = alias == 1 || alias == 3;
                const auto fault = check(std::move(a), entry,
                    "cursor ordered alias " + std::to_string(alias) + " entry=" + std::to_string(entry), corrupts_cursor_on_wrap);
                if (corrupts_cursor_on_wrap) require(fault.starts_with("Unmapped data access at "),
                    "Partial salt/cursor alias did not retain its independent unmapped-access fault");
            }
            for (auto destination : {stack - 8, stack - 16}) {
                a = setup(argv[1], entry, cases); a->memory.write<4>(toc - 0x7b90, destination);
                a->wr<4>((32 - (stack - destination) / 4) * 4, 7);
                check(std::move(a), entry, "salt aliases entry spill");
            }
            a = setup(argv[1], entry, cases); relocate_toc(*a, stack + 0x7b80);
            a->wr<4>(31 * 4, cursor); a->wr<4>(30 * 4, limit); a->wr<4>(29 * 4, pool); a->wr<4>(28 * 4, salt);
            check(std::move(a), entry, "TOC pointer slots alias ordered entry spills");
            // Observe the original first, then map only the accessed bytes.
            auto initial = setup(argv[1], entry); auto probe = clone(*initial); Effects accesses;
            probe->background_original = true; probe->memory.date_observer = &accesses; run(*probe); probe->memory.date_observer = nullptr;
            auto exact = std::make_unique<State>(); exact->registers = initial->registers; exact->unique = initial->unique;
            exact->pc = initial->pc; exact->budget = initial->budget;
            for (const auto& [address, size] : accesses.accesses) exact->memory.map(address, size);
            for (const auto& [address, size] : exact->memory.mapped_ranges()) exact->memory.copy_in(address, initial->memory.copy_out(address, size));
            check(std::move(exact), entry, "exact original-access mappings");
        }
        for (auto selector : {0u,1u,0x80u,0xffu,0x100u,0xffffff00u,0xffffff01u}) {
            for (unsigned entry : {0u,3u}) {
                auto a = setup(argv[1], entry, cases); arguments(*a, entry, selector); auto b = clone(*a);
                compare_pair(*a, *b, "effective signed byte selector " + hex_address(selector)); ++own_cases[entry];
                require(b->r<4>(12) == picture(entry == 0 ? 1 : 0, (selector & 255) != 0, entry == 0 ? 2 : 1),
                    "Effective-byte primary/alternate fixture oracle mismatch");
            }
        }
        for (unsigned entry : {2u,3u}) {
            for (auto flag : {0u,1u,0x80u,0xffu}) {
                auto a = setup(argv[1], entry, cases); a->memory.write<1>(toc - 0x35d4, flag);
                if (!flag) a->memory.write<4>(toc - 0x5c3c, unmapped);
                auto b = clone(*a); compare_pair(*a, *b, "background enable byte " + hex_address(flag)); ++own_cases[entry];
                require(bool(b->r<4>(12)) == bool(flag) && (flag || b->memory.read<4>(cursor) == pool), "Disabled backgrounds consumed RNG or category memory");
            }
            auto a = setup(argv[1], entry, cases); a->memory.write<4>(category_table + category_id * 4, 0);
            a->wr<4>(20, unmapped); auto b = clone(*a); compare_pair(*a, *b, "missing category"); ++own_cases[entry];
            require(!b->r<4>(12) && b->memory.read<4>(cursor) == pool, "Absent category consumed RNG");
            for (auto id : {0u,3u,255u,256u,257u,0x7fffu,0x8000u,0xffffu,0x10001u,0xffff0001u}) {
                a = setup(argv[1], entry, cases); a->wr<4>(12, id);
                const auto offset = entry == 2 ? (id << 2) & 0x3fcu : std::uint32_t(signed_value<2>(id) * 4);
                a->memory.write<4>(category_table + offset, catalog);
                check(std::move(a), entry, "category index " + hex_address(id) + " entry=" + std::to_string(entry));
            }
            a = setup(argv[1], entry, cases); relocate_toc(*a, 0x31008000);
            check(std::move(a), entry, "relocated category TOC");
        }
        for (bool alternate : {false,true}) {
            auto a = setup(argv[1], 3, cases); arguments(*a, 3, alternate); a->wr<4>(16, 0); a->wr<4>(20, unmapped);
            auto b = clone(*a); compare_pair(*a, *b, "null object ignores unmapped descriptor"); ++own_cases[3];
            require(b->r<4>(12) == picture(1, alternate, 2), "Null object default composition oracle mismatch");
            for (auto key : {descriptor, descriptor + 8, descriptor + 16, missing_descriptor}) {
                a = setup(argv[1], 3, cases); arguments(*a, 3, alternate); a->wr<4>(20, key); b = clone(*a);
                compare_pair(*a, *b, "generated descriptor match/miss " + hex_address(key)); ++own_cases[3];
                require(b->r<4>(12) == (key == missing_descriptor ? 0 : picture((key - descriptor) / 8, alternate, 1)),
                    "Descriptor tree match/miss fixture oracle mismatch");
                require(b->memory.read<4>(cursor) == pool + (key == missing_descriptor ? 0 : 2), "Descriptor miss consumed RNG");
            }
            for (auto wrapper : {0x10713ad0u,0x10713b20u}) {
                a = setup(argv[1], 3, cases); a->pc = wrapper; a->wr<4>(12, 0); a->wr<4>(16, alternate); b = clone(*a);
                compare_pair(*a, *b, "generated club/competition null wrapper");
                require(b->r<4>(12) == picture(1, alternate, 2) && b->background_calls[3] != 0,
                    "Generated null-object wrapper did not dispatch semantic selection");
            }
        }
        for (unsigned entry : {0u,1u,3u}) {
            auto a = setup(argv[1], entry, cases);
            // The generated list accessor returns a null head for index zero;
            // a positive index would instead dereference its null next link.
            if (entry == 1) a->memory.write<4>(lists(0) + 8, 1);
            a->memory.write<4>(entry == 1 ? lists(0) : catalog, 0); auto b = clone(*a);
            compare_pair(*a, *b, "accessor miss retains consumed sample"); ++own_cases[entry];
            require(!b->r<4>(12) && b->memory.read<4>(cursor) == pool + (entry == 3 ? 0 : 2), "Post-sample miss changed consumption");
            if (entry == 1) {
                a = setup(argv[1], entry, cases); a->memory.write<4>(lists(0), 0);
                const auto fault = check(std::move(a), entry, "generated list positive-index null-head fault", true);
                require(fault == "Unmapped data access at 0x00000004", "Generated list null traversal fault changed");
            }
            if (entry != 1) {
                a = setup(argv[1], entry, cases); a->memory.write<4>(tree + (entry == 0 ? 0x20 : 0) + 4, 0);
                check(std::move(a), entry, "null selected node payload");
            }
        }
        for (unsigned entry = 0; entry < 4; ++entry) {
            auto a = setup(argv[1], entry, cases);
            if (entry < 2) a->wr<4>(12, unmapped); else a->memory.write<4>(toc - 0x5c3c, unmapped);
            check(std::move(a), entry, "own unmapped collection/category fault", true);
        }
        for (unsigned entry : {0u,1u}) {
            for (auto slot : {0x7b84u,0x7b90u}) {
                auto a = setup(argv[1], entry, cases); a->memory.write<4>(toc - slot, unmapped);
                check(std::move(a), entry, "sampler pointer fault " + hex_address(slot), true);
            }
            auto a = setup(argv[1], entry, cases); a->memory.write<4>(cursor, unmapped); a->memory.write<4>(limit, 0xffffffff);
            check(std::move(a), entry, "unmapped sample after cursor advance", true);
            a = setup(argv[1], entry, cases);
            a->memory.write<4>(entry == 0 ? catalog + 12 : lists(0) + 8, 1);
            a->memory.write<4>(entry == 0 ? catalog : lists(0), unmapped);
            check(std::move(a), entry, "unmapped selected node payload", true);
        }
        {
            auto a = setup(argv[1], 3, cases); a->wr<4>(20, unmapped);
            check(std::move(a), 3, "independent generated descriptor fault", true);
        }
        for (unsigned entry : {0u,2u,3u}) for (bool own_original : {false,true}) {
            auto a = setup(argv[1], entry, cases); auto b = clone(*a);
            b->background_entry_original.fill(!own_original); b->background_entry_original[entry] = own_original;
            compare_pair(*a, *b, "mixed generated/semantic selection composition");
            require((own_original ? b->background_original_calls[entry] : b->background_calls[entry]) != 0,
                "Mixed mode lost own entry dispatch");
        }
        // Each control rejects at its own first return, independently of a
        // parent that may subsequently overwrite r3. Child boundaries precede
        // their caller and are compared before an own-entry mutation occurs.
        for (unsigned entry = 0; entry < 4; ++entry) {
            auto a = setup(argv[1], entry); auto b = clone(*a); Boundaries boundaries;
            a->background_original = true; a->background_observer = &boundaries; run(*a);
            b->background_observer = &boundaries; run(*b);
            require(boundaries.cursor == boundaries.expected.size(), "Missing nested background boundary");
            auto c = setup(argv[1], entry); c->background_observer = &boundaries;
            boundaries.cursor = 0; boundaries.perturb = background_selection_entries[entry];
            bool rejected = false;
            try { run(*c); } catch (const std::runtime_error& error) {
                rejected = std::string(error.what()).starts_with("Own background boundary " + hex_address(boundaries.perturb));
            }
            require(rejected && boundaries.mutated, "Own background perturbation was not rejected at its first boundary");
            reset_background_diagnostics(*c);
            require(!c->background_observer && !c->background_original_resume &&
                c->background_calls == std::array<std::uint64_t,4>{}, "Background diagnostic reset leaked state");
        }
        for (unsigned entry = 0; entry < 4; ++entry) {
            auto a = setup(argv[1], entry, cases);
            a->memory.write<4>(callback_vector, background_selection_entries[entry]); a->memory.write<4>(callback_vector + 4, toc);
            auto b = clone(*a); std::uint32_t original_result = 0, modern_result = 0;
            const std::array<std::uint32_t, 4> parameters{std::uint32_t(a->r<4>(12)), std::uint32_t(a->r<4>(16)), descriptor, 0};
            compare_pair(*a, *b, "callback " + hex_address(background_selection_entries[entry]), false, [&](State& s) {
                const auto result = call_original(s, callback_vector, parameters);
                if (s.background_original) original_result = result; else modern_result = result;
            });
            require(original_result == expected[entry] && modern_result == expected[entry] && b->background_calls[entry] != 0,
                "Nonempty callback semantic dispatch was not reached");
            a = setup(argv[1], entry, cases);
            a->memory.write<4>(callback_vector, background_selection_entries[entry]); a->memory.write<4>(callback_vector + 4, toc);
            a->memory.write<4>(toc - 0x7ee4, callback_vector); a->pc = 0x10780900; b = clone(*a);
            compare_pair(*a, *b, "generated indirect transition " + hex_address(background_selection_entries[entry]));
            require(b->r<4>(12) == expected[entry] && b->background_calls[entry] != 0,
                "Nonempty generated indirect transition did not reach semantic dispatch");
        }
        std::cout << "PASS: background selection ABI/memory/ordered effects cases=" << cases
            << " catalog=" << own_cases[0] << " list=" << own_cases[1] << " default=" << own_cases[2]
            << " object=" << own_cases[3] << "; faults=" << faults
            << "; own negatives=4; nonempty production entries=4; callbacks=4; indirect transitions=4\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
