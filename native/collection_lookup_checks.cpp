#include "collection_lookup_adapter.hpp"
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
constexpr std::uint32_t position = fixture + 0x5400, second_vector = fixture + 0x5500;
constexpr std::uint32_t comparator_toc = 0x31008000;
constexpr std::uint32_t tree = fixture + 0x800, pool = fixture + 0x5000;
constexpr std::uint32_t salt = fixture + 0x5100, limit = fixture + 0x5104, cursor = fixture + 0x5108;
constexpr std::uint32_t descriptor = fixture + 0x5200, missing_descriptor = fixture + 0x5600;
constexpr std::uint32_t callback_vector = fixture + 0x5300;
constexpr std::uint32_t category_id = 1, unmapped = 0xffff0000;
constexpr unsigned list_size = 18, tree_size = 31;
constexpr std::uint32_t branch(unsigned rank) { return tree + rank * 0x20; }
constexpr std::uint32_t key(unsigned rank) { return descriptor + rank * 8; }
constexpr std::uint32_t lists(unsigned item) { return fixture + 0x1000 + item * 0x40; }
constexpr std::uint32_t node(unsigned item, bool alternate, unsigned index) {
    return fixture + 0x10000 + (item * 2 + alternate) * 0x400 + index * 0x10;
}
constexpr std::uint32_t picture(unsigned item, bool alternate, unsigned index) {
    return fixture + 0x14000 + (item * 2 + alternate) * 0x400 + index * 0x20;
}
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
struct Effects final : MemoryObserver {
    std::vector<std::uint8_t> bytes;
    struct Access { unsigned kind; std::uint32_t address; std::uint64_t size, value; };
    std::vector<Access> log;
    std::vector<std::pair<std::uint32_t, std::uint64_t>> accesses;
    State* state = nullptr;
    std::function<void(State&, Effects&, std::uint32_t, unsigned)> after_read;
    unsigned callback_reads = 0;
    bool mutated = false;
    void number(std::uint64_t value, unsigned width) {
        for (unsigned n = width; n; --n) bytes.push_back(std::uint8_t(value >> ((n - 1) * 8)));
    }
    void event(unsigned kind, std::uint32_t address, std::uint64_t size, const std::uint8_t* data = nullptr) {
        number(kind, 1); number(address, 4); number(size, 8); accesses.emplace_back(address, size);
        std::uint64_t value = 0;
        if (data) {
            bytes.insert(bytes.end(), data, data + size);
            for (unsigned i = 0; i < std::min(size, std::uint64_t(8)); ++i) value = (value << 8) | data[i];
        }
        log.push_back({kind,address,size,value});
    }
    void mapped(std::uint32_t address, std::uint64_t size) override { event(1, address, size); }
    void written(std::uint32_t address, const std::uint8_t* data, std::size_t size) override { event(2, address, size, data); }
    void cleared(std::uint32_t address, std::uint64_t size) override { event(3, address, size); }
    void read(std::uint32_t address, const std::uint8_t* data, unsigned size) override {
        event(4, address, size, data);
        if (after_read) after_read(*state, *this, address, size);
    }
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
// Reversed-key fixture: a negative original string comparison selects +8,
// whose key is greater; a positive comparison selects +12, whose key is lower.
// Its independent interval layout covers every unrolled search branch.
std::vector<unsigned> preorder;
std::uint32_t build_tree(State& s, unsigned low, unsigned high) {
    if (low >= high) return 0;
    const unsigned rank = (low + high) / 2;
    preorder.push_back(rank);
    const auto left = build_tree(s, rank + 1, high);
    const auto right = build_tree(s, low, rank);
    s.memory.write<4>(branch(rank), key(rank));
    s.memory.write<4>(branch(rank) + 4, lists(rank % 3));
    s.memory.write<4>(branch(rank) + 8, left);
    s.memory.write<4>(branch(rank) + 12, right);
    s.memory.copy_in(key(rank), {std::uint8_t(32 + rank),0});
    return branch(rank);
}
void arguments(State& s, unsigned entry, std::uint32_t value = 0) {
    s.pc = collection_lookup_entries[entry];
    s.wr<4>(12, entry == 0 ? lists(0) : catalog);
    s.wr<4>(16, entry < 3 ? value : branch(tree_size / 2));
    s.wr<4>(20, entry == 4 ? value : position);
    if (entry == 3) s.memory.write<4>(position, value);
}
std::unique_ptr<State> setup(const char* program, unsigned entry, std::uint32_t value = 0, unsigned variation = 0) {
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
    s->memory.write<4>(category_table + category_id * 4, catalog);
    preorder.clear(); s->memory.write<4>(catalog, build_tree(*s, 0, tree_size));
    s->memory.write<4>(catalog + 4, s->memory.read<4>(toc - 0x5c38));
    s->memory.write<4>(catalog + 12, tree_size);
    for (unsigned item = 0; item < 3; ++item) for (bool alternate : {false,true}) {
        const auto list = lists(item) + (alternate ? 12 : 0);
        s->memory.write<4>(list, node(item, alternate, 0));
        s->memory.write<4>(list + 4, node(item, alternate, list_size - 1));
        s->memory.write<4>(list + 8, list_size);
        for (unsigned index = 0; index < list_size; ++index) {
            s->memory.write<4>(node(item, alternate, index), picture(item, alternate, index));
            s->memory.write<4>(node(item, alternate, index) + 4,
                index + 1 == list_size ? 0 : node(item, alternate, index + 1));
            s->memory.copy_in(picture(item, alternate, index), {'p','i','c',std::uint8_t(index),0});
        }
    }
    s->memory.copy_in(missing_descriptor, {'x',0});
    s->memory.write<4>(toc - 0x7b90, salt); s->memory.write<4>(toc - 0x7b8c, pool);
    s->memory.write<4>(toc - 0x7b88, limit); s->memory.write<4>(toc - 0x7b84, cursor);
    s->memory.write<4>(salt, 0); s->memory.write<4>(limit, pool + 14); s->memory.write<4>(cursor, pool);
    s->memory.write<4>(toc - 0x34c4, 1);
    for (unsigned i = 0; i < 8; ++i) s->memory.write<2>(pool + i * 2, i);
    s->executed = s->native_calls = 0; s->budget = 100000;
    arguments(*s, entry, value);
    return s;
}
unsigned cases = 0, faults = 0;
std::array<unsigned, 5> own_cases{};
struct Comparison {
    Effects effects;
    std::string fault;
};
Comparison compare_pair(State& original, State& modern, const std::string& label, bool expect_fault = false,
                  const std::function<void(State&)>& execute = [](State& s) { run(s); },
                  const std::function<void(State&,Effects&,std::uint32_t,unsigned)>& callback = {}) {
    require(!original.rng_original && !modern.rng_original && !original.background_original && !modern.background_original &&
        !original.date_subsystem_original && !modern.date_subsystem_original,
        "Collection comparisons must retain semantic background/RNG/Date dependencies");
    Effects expected, actual; expected.state = &original; actual.state = &modern;
    expected.after_read = actual.after_read = callback;
    original.collection_original = true;
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
    require(original.rng_calls == modern.rng_calls && original.rng_original_calls == modern.rng_original_calls &&
            original.background_calls == modern.background_calls && original.background_original_calls == modern.background_original_calls,
        label + " unrelated dependency dispatch mismatch");
    ++cases; if (expect_fault) ++faults;
    return {std::move(actual),std::move(actual_fault)};
}
Comparison check(std::unique_ptr<State> initial, unsigned entry, const std::string& label, bool expect_fault = false) {
    auto modern = clone(*initial);
    auto result = compare_pair(*initial, *modern, label, expect_fault); ++own_cases[entry];
    return result;
}
std::vector<std::uint32_t> filtered(const Effects& effects, unsigned kind, const std::function<bool(std::uint32_t)>& accepts) {
    std::vector<std::uint32_t> result;
    for (const auto& event : effects.log) if (event.kind == kind && event.size == 4 && accepts(event.address)) result.push_back(event.address);
    return result;
}
bool is_branch_slot(std::uint32_t address, unsigned first, unsigned second) {
    if (address < tree || address >= tree + tree_size * 0x20) return false;
    return (address - tree) % 0x20 == first || (address - tree) % 0x20 == second;
}
// Independently predict DFS link accesses from the fixture's interval tree.
// A selected node performs no link load; every skipped node decrements once.
bool preorder_links(unsigned low, unsigned high, std::uint32_t& remaining, std::vector<std::uint32_t>& links) {
    if (low >= high) return false;
    if (!remaining) return true;
    --remaining;
    const auto rank = (low + high) / 2;
    links.push_back(branch(rank) + 8);
    if (preorder_links(rank + 1, high, remaining, links)) return true;
    links.push_back(branch(rank) + 12);
    return preorder_links(low, rank, remaining, links);
}
std::vector<std::uint32_t> search_path(unsigned wanted) {
    std::vector<std::uint32_t> result;
    unsigned low = 0, high = tree_size;
    while (low < high) {
        const auto rank = (low + high) / 2; result.push_back(branch(rank));
        if (rank == wanted) break;
        if (rank < wanted) low = rank + 1; else high = rank;
    }
    return result;
}
struct Boundaries final : CollectionBoundaryObserver {
    struct Boundary { std::uint32_t entry; Image image; Boundary(std::uint32_t e, const State& s) : entry(e), image(s) {} };
    std::vector<Boundary> expected;
    std::size_t cursor = 0;
    std::uint32_t perturb = 0;
    bool mutated = false;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& s, std::uint32_t entry, bool original) override {
        if (original) { expected.emplace_back(entry, s); return; }
        if (entry == perturb && !mutated) { mutated = true; s.wr<4>(12, s.r<4>(12) ^ 1); }
        require(cursor < expected.size() && expected[cursor].entry == entry, "Collection boundary order mismatch");
        expected[cursor++].image.compare(s, "Own collection boundary " + hex_address(entry));
    }
};
void relocate_toc(State& s, std::uint32_t context) {
    const auto table = s.memory.copy_out(toc - 0x8000, 0x10000);
    s.memory.map(context - 0x8000, 0x10000); s.memory.copy_in(context - 0x8000, table); s.wr<4>(8, context);
}
std::unique_ptr<State> accessed_mapping(const State& initial) {
    auto probe = clone(initial); Effects effects; probe->collection_original = true;
    probe->memory.date_observer = &effects; run(*probe); probe->memory.date_observer = nullptr;
    auto result = std::make_unique<State>(); result->registers = initial.registers; result->unique = initial.unique;
    result->pc = initial.pc; result->budget = initial.budget;
    for (const auto& [address, size] : effects.accesses) result->memory.map(address, size);
    for (const auto& [address,size] : result->memory.mapped_ranges()) result->memory.copy_in(address, initial.memory.copy_out(address,size));
    return result;
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-collection-lookup-checks <original-segments>\n"; return 2; }
    try {
        const std::array<std::uint32_t,5> nonempty_arguments{8,4,key(30),4,key(30)};
        std::array<std::uint32_t,5> expected_nodes{};
        for (unsigned entry = 0; entry < 5; ++entry) {
            auto a = setup(argv[1], entry, nonempty_arguments[entry], cases); auto b = clone(*a);
            expected_nodes[entry] = entry == 0 ? node(0,false,8) : entry == 1 || entry == 3 ? branch(preorder[4]) : branch(30);
            compare_pair(*a,*b,"nonempty production " + hex_address(collection_lookup_entries[entry])); ++own_cases[entry];
            require(b->r<4>(12) == expected_nodes[entry] && b->collection_calls[entry] != 0 &&
                b->collection_original_calls == std::array<std::uint64_t,5>{}, "Nonempty semantic collection production dispatch was not reached");
            if (entry >= 3) require(b->collection_calls[entry] > 1, "Recursive helper did not re-enter semantic dispatch");
        }
        for (auto index : {0xffffffffu,0x80000000u,0xfffffff8u,0u,1u,7u,8u,9u,15u,16u,17u,18u}) {
            auto a = setup(argv[1],0,index,cases); a->memory.write<4>(lists(0)+8,0xdeadbeef); auto b = clone(*a);
            const auto result = compare_pair(*a,*b,"list signed index " + hex_address(index)); ++own_cases[0];
            const auto selected = signed_value<4>(index) <= 0 ? node(0,false,0) : index == list_size ? 0 : node(0,false,index);
            require(b->r<4>(12) == selected, "Independent list node oracle mismatch");
            std::vector<std::uint32_t> expected{lists(0)};
            if (signed_value<4>(index) > 0) for (unsigned i = 0; i < index; ++i) expected.push_back(node(0,false,i)+4);
            const auto actual = filtered(result.effects,4,[](auto address) { return address == lists(0) ||
                (address >= node(0,false,0) && address < node(0,false,list_size) && (address-node(0,false,0))%0x10 == 4); });
            require(actual == expected, "Independent list head/link order mismatch");
        }
        for (auto index : {0u,0xffffffffu,0x80000000u}) {
            auto a = setup(argv[1],0,index); a->memory.write<4>(lists(0),0); auto b = clone(*a);
            compare_pair(*a,*b,"null list head nonpositive index"); ++own_cases[0];
            require(!b->r<4>(12),"Null list head did not return zero");
        }
        for (auto index : {1u,8u,0x7fffffffu}) {
            auto a = setup(argv[1],0,index); a->memory.write<4>(lists(0),0);
            const auto result = check(std::move(a),0,"null head controlled large-index fault",true);
            require(result.fault == "Unmapped data access at 0x00000004", "Null list fault address changed");
        }
        for (auto index : {19u,24u}) {
            auto a = setup(argv[1],0,index);
            const auto result = check(std::move(a),0,"list beyond chain fault",true);
            require(result.fault == "Unmapped data access at 0x00000004", "List exhaustion fault changed");
        }
        for (unsigned entry : {1u,3u}) {
            std::vector<std::uint32_t> indices;
            for (unsigned index=0;index<=tree_size+1;++index) indices.push_back(index);
            indices.insert(indices.end(),{0xffffffffu,0x80000000u,0x7fffffffu});
            for (auto index : indices) {
                auto a = setup(argv[1],entry,index,cases); auto b = clone(*a);
                const auto result = compare_pair(*a,*b,"tree preorder position " + hex_address(index) + " entry="+std::to_string(entry)); ++own_cases[entry];
                auto remaining = index; std::vector<std::uint32_t> expected_links;
                const bool found = preorder_links(0,tree_size,remaining,expected_links);
                require(b->r<4>(12) == (found ? branch(preorder[index]) : 0),"Independent preorder node oracle mismatch");
                const auto position_slot = entry == 1 ? stack+28 : position;
                require(b->memory.read<4>(position_slot) == remaining,"Independent residual position oracle mismatch");
                const auto actual_links = filtered(result.effects,4,[](auto address) { return is_branch_slot(address,8,12); });
                require(actual_links == expected_links,"Independent preorder visited/link order mismatch");
                std::vector<std::uint32_t> positions;
                for (const auto& event : result.effects.log) if (event.kind == 2 && event.address == position_slot && event.size == 4) positions.push_back(event.value);
                std::vector<std::uint32_t> expected_positions;
                if (entry == 1) expected_positions.push_back(index);
                const unsigned skipped = found ? index : tree_size;
                for (unsigned n = 1; n <= skipped; ++n) expected_positions.push_back(index-n);
                require(positions == expected_positions,"Independent decrement/store order mismatch");
            }
            for (bool empty : {false,true}) for (auto index : {0u,1u,0xffffffffu,0x80000000u}) {
                auto a = setup(argv[1],entry,index); a->memory.write<4>(branch(15)+8,0); a->memory.write<4>(branch(15)+12,0);
                if (empty) { if (entry == 1) a->memory.write<4>(catalog,0); else a->wr<4>(16,0); }
                auto b = clone(*a); compare_pair(*a,*b,empty ? "empty tree" : "singleton tree"); ++own_cases[entry];
                require(b->r<4>(12) == (!empty && !index ? branch(15) : 0),"Singleton/empty node oracle mismatch");
                require(b->memory.read<4>(entry == 1 ? stack+28 : position) == index-unsigned(!empty && bool(index)),
                    "Singleton/empty residual position mismatch");
            }
            for (auto index : {2u,3u,4u,5u}) {
                auto a = setup(argv[1],entry,index); a->memory.write<4>(branch(15)+8,branch(30)); a->memory.write<4>(branch(30)+8,0);
                a->memory.write<4>(branch(30)+12,branch(29)); a->memory.write<4>(branch(15)+12,branch(0));
                for (auto rank : {0u,29u}) {a->memory.write<4>(branch(rank)+8,0);a->memory.write<4>(branch(rank)+12,0);}
                auto b = clone(*a); compare_pair(*a,*b,"asymmetric tree left-right grandchild/root right leaf"); ++own_cases[entry];
                require(b->r<4>(12)==(index==2 ? branch(29) : index==3 ? branch(0) : 0),"Asymmetric preorder node mismatch");
                require(b->memory.read<4>(entry==1 ? stack+28 : position)==(index<4 ? 0 : index-4),"Asymmetric preorder residual mismatch");
            }
        }
        for (unsigned entry : {2u,4u}) {
            for (unsigned wanted = 0; wanted < tree_size; ++wanted) {
                auto a = setup(argv[1],entry,key(wanted),cases); auto b = clone(*a);
                const auto result = compare_pair(*a,*b,"descriptor rank="+std::to_string(wanted)+" entry="+std::to_string(entry)); ++own_cases[entry];
                require(b->r<4>(12)==branch(wanted),"Independent reversed-order search node mismatch");
                const auto actual = filtered(result.effects,4,[](auto address) { return is_branch_slot(address,0,0); });
                const auto expected = search_path(wanted);
                require(actual == expected,"Independent comparison key/count/order mismatch");
                const auto vectors = filtered(result.effects,4,[](auto address) { return address == catalog+4; });
                require(vectors.size() == expected.size(),"Comparator vector was not reloaded once per node");
            }
            for (auto wanted : {0u,31u,63u,255u}) {
                auto a = setup(argv[1],entry,missing_descriptor); a->memory.write<1>(missing_descriptor,wanted); auto b = clone(*a);
                const auto result = compare_pair(*a,*b,"descriptor miss"); ++own_cases[entry];
                require(!b->r<4>(12),"Missing descriptor found a node");
                const auto actual = filtered(result.effects,4,[](auto address) { return is_branch_slot(address,0,0); });
                require(actual == search_path(wanted >= 32 && wanted < 128 ? tree_size : 0),"Independent search miss path mismatch");
            }
            auto a = setup(argv[1],entry,unmapped);
            if (entry == 2) a->memory.write<4>(catalog,0); else a->wr<4>(16,0);
            a->memory.write<4>(catalog+4,unmapped); auto b=clone(*a);
            const auto result=compare_pair(*a,*b,"null root ignores unmapped descriptor and comparator"); ++own_cases[entry];
            require(!b->r<4>(12) && filtered(result.effects,4,[](auto address){return address == catalog+4;}).empty(),
                "Null search root consulted descriptor/vector");
            a=setup(argv[1],entry,key(30)); relocate_toc(*a,comparator_toc);
            check(std::move(a),entry,"relocated caller TOC");
        }
        // A separate generated leaf accessor serves as a transition callback
        // returning the fixture key's +4 word. This proves signed branch choice
        // for arbitrary magnitudes while preserving the original callback ABI.
        for (unsigned entry : {2u,4u}) for (auto wanted : {0u,15u,30u}) {
            auto a=setup(argv[1],entry,unmapped);
            a->memory.write<4>(callback_vector,0x1077d9e0); a->memory.write<4>(callback_vector+4,toc);
            a->memory.write<4>(catalog+4,callback_vector);
            for (unsigned rank=0;rank<tree_size;++rank) a->memory.write<4>(key(rank)+4,
                rank==wanted ? 0 : rank<wanted ? (rank%2 ? 0xfffffff9u : 0x80000000u) : (rank%2 ? 19u : 0x7fffffffu));
            auto b=clone(*a); const auto result=compare_pair(*a,*b,"non-normalized signed comparator returns"); ++own_cases[entry];
            require(b->r<4>(12)==branch(wanted),"Non-normalized signed comparator chose the wrong branch");
            const auto compared=filtered(result.effects,4,[](auto address){return is_branch_slot(address,0,0);});
            require(compared==search_path(wanted),"Non-normalized comparator order/count mismatch");
        }
        // The comparator remains the original generated transition wrapper and
        // string routine. An adversarial observer changes state during its key
        // read, equally in both runs, proving reloads happen after the callback.
        for (unsigned entry : {2u,4u}) {
            auto a=setup(argv[1],entry,key(30)); relocate_toc(*a,comparator_toc); a->wr<4>(8,toc);
            a->memory.write<4>(callback_vector,0x1074de10); a->memory.write<4>(callback_vector+4,comparator_toc);
            a->memory.write<4>(second_vector,0x1074de10); a->memory.write<4>(second_vector+4,comparator_toc+0x100);
            a->memory.write<4>(catalog+4,callback_vector); auto b=clone(*a);
            const auto result=compare_pair(*a,*b,"mutable comparator vector/child/clobbers/own TOC",false,
                [](State& s){run(s);},[](State& s,Effects& e,std::uint32_t address,unsigned size){
                    if (size != 1 || s.pc != 0x10739484 || (address != key(15) && address != key(30))) return;
                    ++e.callback_reads;
                    if (!e.mutated && address == key(15)) {
                        require(s.r<4>(8)==comparator_toc,"Comparator did not receive its transition-vector TOC");
                        e.mutated=true;
                        s.memory.write<4>(catalog+4,second_vector); s.memory.write<4>(branch(15)+8,branch(30));
                        s.wr<4>(44,0x10203040); s.wr<4>(48,0x50607080); s.wr<4>(4132,0x11112222);
                    } else if (address == key(30)) {
                        require(s.r<4>(8)==comparator_toc+0x100,"Mutated vector was not reloaded before the next comparator");
                    }
                }); ++own_cases[entry];
            require(b->r<4>(12)==branch(30) && result.effects.mutated && b->r<4>(8)==toc,
                "Callback-mutated child or restored caller TOC mismatch");
            const auto comparisons=filtered(result.effects,4,[](auto address){return is_branch_slot(address,0,0);});
            require(comparisons==std::vector<std::uint32_t>{branch(15),branch(30)},"Callback-mutated child was not loaded after return");
            const auto vectors=filtered(result.effects,4,[](auto address){return address==callback_vector || address==second_vector;});
            require(vectors==std::vector<std::uint32_t>{callback_vector,second_vector},"Callback-mutated transition vector was not reloaded");
            // Different nodes deliberately repeat their key pointer; no read
            // cache may suppress either original comparison or vector reload.
            a=setup(argv[1],entry,key(30)); a->memory.write<4>(branch(23),key(15));
            a->memory.write<4>(branch(23)+8,branch(30)); b=clone(*a);
            const auto repeated=compare_pair(*a,*b,"repeated comparator/key pointers"); ++own_cases[entry];
            require(b->r<4>(12)==branch(30),"Repeated-pointer search result mismatch");
            require(filtered(repeated.effects,4,[](auto address){return is_branch_slot(address,0,0);}) ==
                std::vector<std::uint32_t>{branch(15),branch(23),branch(30)},"Repeated comparator/key pointers collapsed comparisons");
            require(filtered(repeated.effects,4,[](auto address){return address==catalog+4;}).size()==3,
                "Repeated comparator vector was cached");
        }
        for (auto alias : {branch(15)+8,branch(15)+12,catalog,stack-24,stack-80}) {
            auto a=setup(argv[1],3,1); a->wr<4>(20,alias); a->memory.write<4>(alias,1);
            if (alias==stack-24) a->wr<4>(26*4,1);
            auto b=clone(*a); compare_pair(*a,*b,"position aliases links/collection/spills " + hex_address(alias)); ++own_cases[3];
            const auto expected=alias==branch(15)+8 ? branch(7) : alias==stack-80 ? 0 : branch(23);
            require(b->r<4>(12)==expected,"Ordered position alias node expectation mismatch");
            require(b->memory.read<4>(alias)==(alias==stack-80 ? stack-tree_size : 0),"Ordered position alias residual mismatch");
        }
        for (auto collection_alias : {stack+28,stack-64}) {
            auto a=setup(argv[1],1,0); a->wr<4>(12,collection_alias); a->memory.write<4>(collection_alias,branch(15)); auto b=clone(*a);
            compare_pair(*a,*b,"wrapper root aliases position/frame " + hex_address(collection_alias)); ++own_cases[1];
            require(b->r<4>(12)==(collection_alias==stack+28 ? 0 : stack),"Wrapper frame/position alias read order mismatch");
        }
        {
            auto a=setup(argv[1],4,key(15)); const auto vector=std::uint32_t(a->memory.read<4>(catalog+4));
            a->wr<4>(12,stack-8); a->wr<4>(31*4,vector); a->memory.write<4>(stack-4,unmapped); auto b=clone(*a);
            compare_pair(*a,*b,"comparator collection/vector aliases saved register spill"); ++own_cases[4];
            require(b->r<4>(12)==branch(15),"Vector spill was not written before comparator reload");
        }
        // Fault snapshots include the same instruction PC and ordered prefix,
        // including every spill/decrement before a failing dependency access.
        for (unsigned entry=0;entry<5;++entry) {
            auto a=setup(argv[1],entry,nonempty_arguments[entry]);
            if (entry<3) a->wr<4>(12,unmapped); else a->wr<4>(16,unmapped);
            check(std::move(a),entry,"own collection/root access fault",true);
            a=setup(argv[1],entry,nonempty_arguments[entry]); auto minimal=accessed_mapping(*a); minimal->wr<4>(4,unmapped);
            if (entry) check(std::move(minimal),entry,"entry first spill/frame fault",true);
        }
        for (unsigned entry : {1u,3u}) {
            auto a=setup(argv[1],entry,2); a->memory.write<4>(branch(15)+8,unmapped);
            check(std::move(a),entry,"tree child fault after decrement",true);
        }
        {
            auto a=setup(argv[1],3,1); a->wr<4>(20,unmapped); check(std::move(a),3,"mutable position first load fault",true);
            a=setup(argv[1],3,1); a->wr<4>(16,0); a->wr<4>(20,unmapped); auto b=clone(*a);
            const auto result=compare_pair(*a,*b,"null preorder root ignores unmapped position"); ++own_cases[3];
            require(!b->r<4>(12) && filtered(result.effects,4,[](auto address){return address==unmapped;}).empty(),
                "Null preorder root read position");
        }
        for (unsigned entry : {2u,4u}) {
            auto a=setup(argv[1],entry,unmapped); check(std::move(a),entry,"generated comparator descriptor fault",true);
            a=setup(argv[1],entry,key(30)); a->memory.write<4>(branch(15),unmapped);
            check(std::move(a),entry,"generated comparator key fault",true);
            a=setup(argv[1],entry,key(30)); a->memory.write<4>(catalog+4,unmapped);
            check(std::move(a),entry,"transition vector first word fault",true);
            a=setup(argv[1],entry,key(30)); a->memory.write<4>(branch(15)+8,unmapped);
            check(std::move(a),entry,"search child fault after comparator",true);
            a=setup(argv[1],entry,key(30)); auto minimal=accessed_mapping(*a);
            minimal->memory.map(unmapped-4,4); minimal->memory.write<4>(unmapped-4,0x1074de10);
            minimal->memory.write<4>(catalog+4,unmapped-4);
            check(std::move(minimal),entry,"transition vector TOC second word fault",true);
        }
        for (unsigned entry=0;entry<5;++entry) {
            auto a=setup(argv[1],entry,nonempty_arguments[entry]);
            check(accessed_mapping(*a),entry,"exact original-access mappings");
        }
        // Mixed modes independently prove hooks at generated destination labels,
        // including generated-wrapper calls into a newly semantic helper.
        for (unsigned entry : {1u,2u}) for (bool own_original : {false,true}) {
            auto a=setup(argv[1],entry,nonempty_arguments[entry]); auto b=clone(*a);
            b->collection_entry_original.fill(!own_original); b->collection_entry_original[entry]=own_original;
            compare_pair(*a,*b,"mixed wrapper/helper original-semantic"); ++own_cases[entry];
            const unsigned helper=entry==1 ? 3 : 4;
            require((own_original ? b->collection_original_calls[entry] : b->collection_calls[entry])!=0 &&
                (own_original ? b->collection_calls[helper] : b->collection_original_calls[helper])!=0,
                "Mixed wrapper/helper mode lost selected dispatch");
        }
        // Existing semantic selectors keep their guest callsites. These nonempty
        // fixtures compose list, positional tree and real descriptor lookup and
        // compare the unchanged RNG state, cursor and dispatch on each run.
        for (unsigned background=0;background<4;++background) for (bool alternate : {false,true}) {
            auto a=setup(argv[1],0); a->pc=background_selection_entries[background];
            a->wr<4>(12,background==0 ? catalog : background==1 ? lists(0)+(alternate ? 12 : 0) : category_id);
            a->wr<4>(16,background==3 ? 1 : alternate); a->wr<4>(20,key(30)); a->wr<4>(24,alternate);
            auto b=clone(*a); compare_pair(*a,*b,"semantic backgrounds composed with collection lookups");
            const unsigned item=background==0 || background==2 ? 2 : 0;
            const unsigned image_index=background==0 || background==2 ? 2 : 1;
            require(b->r<4>(12)==picture(item,alternate && background!=2,image_index),"Independent nonempty background composition node mismatch");
            require(b->memory.read<4>(cursor)==pool+(background==0 || background==2 ? 4 : 2),"Background composition RNG cursor mismatch");
            require(b->background_calls[background]!=0 && b->collection_calls[0]!=0 &&
                b->collection_original_calls==std::array<std::uint64_t,5>{},"Background composition failed semantic collection dispatch");
            if (background==0 || background==2) require(b->collection_calls[1]!=0 && b->collection_calls[3]!=0,
                "Catalog selection missed semantic positional wrapper/helper");
            if (background==3) require(b->collection_calls[2]!=0 && b->collection_calls[4]>1,
                "Descriptor selection missed recursive semantic search wrapper/helper");
        }
        // Reject every own entry at its first return. Child boundaries are
        // checked before the parent's own perturbation, so helper divergence
        // cannot stand in for a wrapper's independent negative control.
        for (unsigned entry=0;entry<5;++entry) {
            auto a=setup(argv[1],entry,nonempty_arguments[entry]); auto b=clone(*a); Boundaries boundaries;
            a->collection_original=true; a->collection_observer=&boundaries; run(*a);
            b->collection_observer=&boundaries; run(*b);
            require(boundaries.cursor==boundaries.expected.size(),"Missing nested collection boundary");
            auto c=setup(argv[1],entry,nonempty_arguments[entry]); c->collection_observer=&boundaries;
            boundaries.cursor=0; boundaries.perturb=collection_lookup_entries[entry]; bool rejected=false;
            try {run(*c);} catch (const std::runtime_error& error) {
                rejected=std::string(error.what()).starts_with("Own collection boundary "+hex_address(boundaries.perturb));
            }
            require(rejected && boundaries.mutated,"Own collection perturbation was not rejected at its first boundary");
            c->collection_original=true; c->collection_entry_original.fill(true); c->collection_original_resume=collection_lookup_entries[entry];
            reset_collection_lookup_diagnostics(*c);
            require(!c->collection_observer && !c->collection_original && !c->collection_original_resume &&
                c->collection_entry_original==std::array<bool,5>{} && c->collection_calls==std::array<std::uint64_t,5>{} &&
                c->collection_original_calls==std::array<std::uint64_t,5>{},"Collection diagnostic reset leaked state");
        }
        for (unsigned entry=0;entry<5;++entry) {
            auto a=setup(argv[1],entry,nonempty_arguments[entry]);
            a->memory.write<4>(callback_vector,collection_lookup_entries[entry]); a->memory.write<4>(callback_vector+4,toc);
            auto b=clone(*a); std::uint32_t original_result=0,modern_result=0;
            const std::array<std::uint32_t,3> parameters{std::uint32_t(a->r<4>(12)),std::uint32_t(a->r<4>(16)),std::uint32_t(a->r<4>(20))};
            compare_pair(*a,*b,"callback "+hex_address(collection_lookup_entries[entry]),false,[&](State& s){
                const auto result=call_original(s,callback_vector,parameters);
                if (s.collection_original) original_result=result; else modern_result=result;
            });
            require(original_result==expected_nodes[entry] && modern_result==expected_nodes[entry] && b->collection_calls[entry]!=0 &&
                b->collection_original_calls==std::array<std::uint64_t,5>{},"Callback did not reach nonempty semantic collection dispatch");
            a=setup(argv[1],entry,nonempty_arguments[entry]);
            a->memory.write<4>(callback_vector,collection_lookup_entries[entry]); a->memory.write<4>(callback_vector+4,toc);
            a->memory.write<4>(toc-0x7ee4,callback_vector); a->pc=0x10780900; b=clone(*a);
            compare_pair(*a,*b,"generated indirect transition "+hex_address(collection_lookup_entries[entry]));
            require(b->r<4>(12)==expected_nodes[entry] && b->collection_calls[entry]!=0 &&
                b->collection_original_calls==std::array<std::uint64_t,5>{},"Generated indirect transition missed nonempty semantic collection dispatch");
        }
        std::cout << "PASS: collection lookup ABI/memory/ordered effects cases=" << cases
            << " list=" << own_cases[0] << " position-wrapper=" << own_cases[1] << " search-wrapper=" << own_cases[2]
            << " preorder=" << own_cases[3] << " search=" << own_cases[4] << "; faults=" << faults
            << "; own negatives=5; nonempty production entries=5; recursive helpers=2; callbacks=5; indirect transitions=5; background compositions=8\n";
        return 0;
    } catch (const std::exception& error) {std::cerr << error.what() << '\n';return 1;}
}
