#include "list_search.hpp"
#include "semantic_io_checks.hpp"
#include <functional>
#include <optional>

// Runs the list search 0x101b1620 from synthetic lists twice: once through
// the original (a do-nothing memory observer makes the C++ version step
// aside) and once through the C++ version, and compares every register,
// generated temporary, mapped byte and the instruction count.
namespace {
using namespace cm::semantic_io_checks;
constexpr std::uint32_t entry = 0x101b1620, sentinel = 0x7fff1000;
constexpr std::uint32_t container = fixture, list = fixture + 0x100, nodes = fixture + 0x200,
                        elements = fixture + 0x800, key = fixture + 0xf00;
constexpr std::uint32_t cursor_offset = 0xc;  // list cursor field
struct Silent final : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
};
struct Case {
    std::string label;
    std::int16_t count;
    std::vector<std::int32_t> keys;   // element keys along the chain
    std::int32_t wanted;
    int null_node = -1, null_data = -1;  // break the chain at this position
    std::function<void(State&)> mutate = {};
    std::uint64_t budget = 100000;
};
std::unique_ptr<State> setup(const State& program, const Case& c) {
    auto s = clone(program);
    s->memory.map(fixture, 0x1000);
    s->memory.clear(fixture, 0x1000);
    s->memory.write<4>(container + 8, list);
    s->memory.write<2>(list, std::uint16_t(c.count));
    s->memory.write<4>(list + 4, c.keys.empty() || c.null_node == 0 ? 0 : nodes);
    s->memory.write<4>(list + 0xc, 0x5a5a5a5a);  // saved cursor, restored on exit
    for (std::size_t i = 0; i < c.keys.size(); ++i) {
        const auto node = nodes + std::uint32_t(i) * 0x10, element = elements + std::uint32_t(i) * 0x20;
        const bool last = i + 1 == c.keys.size() || int(i + 1) == c.null_node;
        s->memory.write<4>(node, int(i) == c.null_data ? 0 : element);
        s->memory.write<4>(node + 8, last ? 0 : node + 0x10);
        s->memory.write<4>(element + 8, std::uint32_t(c.keys[i]));
    }
    s->memory.write<4>(key, std::uint32_t(c.wanted));
    s->registers.fill(0xa5);
    s->wr<4>(4, stack); s->wr<4>(8, toc); s->wr<4>(12, container); s->wr<4>(16, key);
    s->wr<4>(4128, sentinel);
    s->pc = entry;
    s->budget = s->executed + c.budget;
    if (c.mutate) c.mutate(*s);
    return s;
}
std::string execute(State& s) {
    try { run_until(s, sentinel); } catch (const std::exception& e) { return e.what(); }
    return {};
}
// Returns the C++ path's outcome; throws when the two runs differ.
std::string compare(const State& program, const Case& c, bool expect_native) {
    auto original = setup(program, c), modern = setup(program, c);
    Silent silent;
    original->memory.observer = &silent;
    const auto before = native_list_search_completions();
    const auto expected = execute(*original), actual = execute(*modern);
    original->memory.observer = nullptr;
    require(expected == actual, c.label + " outcome original=" + expected + " modern=" + actual);
    require((native_list_search_completions() > before) == expect_native, c.label + " took the wrong path");
    Image(*original).compare(*modern, c.label, !expected.empty());
    require(original->executed == modern->executed, c.label + " instruction count " + std::to_string(original->executed) +
        " vs " + std::to_string(modern->executed));
    return actual;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-list-search-checks <original-segments>");
        auto program = std::make_unique<State>();
        load_program(*program, argv[1]);
        program->memory.map(stack - 0x300, 0x400);
        unsigned cases = 0;
        for (std::size_t length = 0; length <= 6; ++length) {
            std::vector<std::int32_t> keys;
            for (std::size_t i = 0; i < length; ++i) keys.push_back(std::int32_t(1000 + 7 * i));
            for (std::size_t at = 0; at < length; ++at) {
                compare(*program, {"found " + std::to_string(at) + "/" + std::to_string(length), std::int16_t(length), keys, keys[at]}, true);
                ++cases;
            }
            compare(*program, {"absent/" + std::to_string(length), std::int16_t(length), keys, -1}, true);
            ++cases;
            // A count below the chain stops early; duplicates find the first.
            if (length >= 2) {
                compare(*program, {"short count", std::int16_t(length - 1), keys, keys.back()}, true);
                auto twins = keys; twins[length - 1] = twins[0];
                compare(*program, {"duplicate", std::int16_t(length), twins, twins[0]}, true);
                cases += 2;
            }
        }
        compare(*program, {"negative count", -3, {1, 2, 3}, 2}, true);
        compare(*program, {"signed keys", 3, {-5, 0x7fffffff, std::int32_t(0x80000000)}, std::int32_t(0x80000000)}, true);
        cases += 2;
        // The original's error report runs for these; the C++ version steps aside.
        compare(*program, {"null node", 4, {1, 2}, 9}, false);
        compare(*program, {"null element", 3, {1, 2, 3}, 3, -1, 1}, false);
        // Overlaps with what the original writes first step aside too: an
        // element whose key is the list cursor, a key in the frame's LR slot
        // (the original compares against the saved LR), and a list whose
        // cursor field is in the register save area.
        compare(*program, {"element key is the cursor", 3, {1, 2, 3}, 9, -1, -1,
            [](State& s) { s.memory.write<4>(nodes + 0x10, list + 4); }}, false);
        compare(*program, {"key in the LR slot", 3, {1, std::int32_t(sentinel), 3}, 1, -1, -1,
            [](State& s) { s.wr<4>(16, stack + 8); s.memory.write<4>(stack + 8, 1); }}, false);
        compare(*program, {"cursor in the save area", 2, {1, 2}, 9, -1, -1,
            [](State& s) {
                const auto moved = stack - 0x18 - cursor_offset;
                for (unsigned i = 0; i < 0x10; i += 4) s.memory.write<4>(moved + i, s.memory.read<4>(list + i));
                s.memory.write<4>(container + 8, moved);
            }}, false);
        // A budget too small for the whole search, and a fault, are the original's.
        compare(*program, {"small budget", 3, {1, 2, 3}, 3, -1, -1, {}, 30}, false);
        compare(*program, {"unmapped list", 3, {1, 2, 3}, 3, -1, -1,
            [](State& s) { s.memory.write<4>(container + 8, 0x50000000); }}, false);
        cases += 7;
        // The comparison must catch a wrong answer: a different key changes r3
        // and the instruction count.
        bool caught = false;
        try {
            auto a = setup(*program, {"control", 3, {1, 2, 3}, 2}), b = setup(*program, {"control", 3, {1, 2, 3}, 3});
            execute(*a); execute(*b);
            Image(*a).compare(*b, "control");
            require(a->executed == b->executed, "control count");
        } catch (const std::runtime_error&) { caught = true; }
        require(caught, "The comparison did not notice a different search result");
        std::cout << cases << " list-search cases identical to the original; error reports left to the original; control caught\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
