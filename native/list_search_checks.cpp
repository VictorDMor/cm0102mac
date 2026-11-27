#include "list_search.hpp"
#include "graph_carbon.hpp"
#include "match_day_driver.hpp"
#include "shortlist_manager_candidates_scan.hpp"
#include "shortlist_worker_access.hpp"
#include <chrono>
#include <mutex>
#include <set>
#include <thread>
#include "manager_manager_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <functional>
#include <optional>

// Runs the list search 0x101b1620 from synthetic lists twice: once through
// the generated original (MANAGER_MANAGER.CPP's owner declining) and once
// through the owner, whose fast path is the native search here; compares every
// register, generated temporary and mapped byte, and the instruction count
// where the fast path ran (it charges the original's; the owner's slow path,
// taken where the fast path steps aside, charges none).
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
    cm::aot::manager_manager_declining = true;
    const auto expected = execute(*original);
    cm::aot::manager_manager_declining = false;
    const auto actual = execute(*modern);
    original->memory.observer = nullptr;
    require(expected == actual, c.label + " outcome original=" + expected + " modern=" + actual);
    require((native_list_search_completions() > before) == expect_native, c.label + " took the wrong path");
    Image(*original).compare(*modern, c.label, !expected.empty());
    require(!expect_native || original->executed == modern->executed, c.label + " instruction count " +
        std::to_string(original->executed) + " vs " + std::to_string(modern->executed));
    return actual;
}
// Observe the actual cursor walk under both the slow native and generated
// dispatch paths. Tracking ownership across several writes detects overlapping
// walks, even if both candidates happen to get the right verdict.
struct CursorWalks final : MemoryObserver {
    CursorWalks() { restrict_write_notifications(list + cursor_offset, list + cursor_offset + 4); }
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t address, const std::uint8_t* value, std::size_t size) override {
        if (address != list + cursor_offset || size != 4) return;
        const auto thread = std::this_thread::get_id();
        bool opened = false;
        {
            const std::lock_guard lock(mutex);
            const bool restored = value[0] == 0x5a && value[1] == 0x5a &&
                                  value[2] == 0x5a && value[3] == 0x5a;
            if (owner != std::thread::id{} && owner != thread) ++collisions;
            if (restored) {
                owner = {};
                ++completed;
            } else if (owner == std::thread::id{}) {
                owner = thread;
                participants.insert(thread);
                opened = true;
            }
        }
        if (opened) std::this_thread::sleep_for(std::chrono::microseconds(50));
    }
    std::mutex mutex;
    std::thread::id owner;
    std::set<std::thread::id> participants;
    unsigned collisions = 0, completed = 0;
};

void parallel_candidates(const State& program) {
    namespace filter = cm::aot::shortlist_manager::candidates;
    namespace workers = cm::aot::shortlist_workers;
    auto s = setup(program, {"shared staff list", 6, {21, 22, 23, 24, 25, 1}, 1});
    constexpr std::uint32_t manager = fixture + 0x2000, entries = manager + 0x1000;
    constexpr std::uint32_t person = manager + 0x200, entry_count = 256;
    s->memory.map(manager, 0x2000);
    s->memory.clear(manager, 0x2000);
    cm::aot::guest::Cpu c(*s);
    const auto person_table = manager + 0x104, listing_cell = manager + 0x108;
    c.put32(person_table, person - 0x70);
    c.put32(c.r(2) + std::uint32_t(cm::aot::shortlist_manager::detail::slot::persons), person_table);
    const auto count = manager + 0x10;
    c.put32(count, 50);
    c.put32(c.r(2) + std::uint32_t(cm::aot::shortlist_manager::detail::slot::person_count), count);
    c.put32(listing_cell, container);
    c.put32(c.r(2) + std::uint32_t(filter::slot::listings), listing_cell);
    c.put32(person, 1);
    c.put32(person + 0x1c, person + 0x70);
    c.put32(manager + 0x18, entries);
    c.put32(manager + 0x98, entry_count);
    for (std::uint32_t i = 0; i < entry_count; ++i) c.put32(entries + 12 * i, 1);

    setenv("CM_SHORTLIST_PARALLEL", "1", 1);
    setenv("CM_MATCH_WORKERS", "2", 1);
    const auto prior_order = match_day_order();
    set_match_day_order(MatchDayOrder::parallel);
    CursorWalks trace;
    s->memory.observer = &trace;
    // The candidate fixture returns immediately for staff already in the
    // manager's list. Every entry still goes through the actual worker pool.
    {
        cm::aot::guest::Frame frame(c, filter::entry, 0x1b0, {.first_gpr = 13, .gpr_offset = -0x6c,
                                                                  .first_fpr = 29});
        c.put8(frame.at(filter::local::category), 0);
        for (const auto field : {filter::local::category_offset, filter::local::lists_offset,
                                 filter::local::last_category, filter::local::excluded})
            c.put32(frame.at(field), 0);
        filter::Scan scan(c, frame, 0, 0);
        scan.manager = manager;
        scan.salt_slot = c.toc(cm::aot::shortlist_manager::detail::slot::random_salt);
        scan.cursor_slot = c.toc(cm::aot::shortlist_manager::detail::slot::random_cursor);
        require(scan.judge_on_workers(), "The shortlist did not run on its workers");
        require(scan.verdicts.size() == entry_count, "The workers lost entries");
        for (const auto& verdict : scan.verdicts)
            require(!verdict.eligible, "A worker accepted a staff member already on the list");
    }
    set_match_day_order(prior_order);
    s->memory.observer = nullptr;
    require(trace.participants.size() == 2 && trace.completed == entry_count,
            "The two workers did not both traverse the shared list");
    require(trace.collisions == 0 && c.u32(list + cursor_offset) == 0x5a5a5a5a,
            "Concurrent workers corrupted the manager list cursor");

    workers::SharedRecords records;
    const workers::RecordScope scope(records);
    bool passed = false;
    try {
        cm::aot::graph_carbon::assertion_box(c, 0, 0, 0);
    } catch (const std::runtime_error& error) {
        passed = std::string_view(error.what()) == "Candidate evaluation reached a game assertion";
    }
    require(passed, "A shortlist worker's assertion reached the guest dialog");
}

}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-list-search-checks <original-segments>");
        auto program = std::make_unique<State>();
        load_program(*program, argv[1]);
        program->memory.map(stack - 0x300, 0x400);
        // Every case runs on a fresh State, so the C++ side answers from a fresh index (list_search_index.hpp).
        set_native_list_search_index(true);
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
        // The index across calls on one State: a key changed behind the same count and head is seen after a node
        // or list routine ran (note_list_mutation, which the utility-object owners call for each of them); without
        // that note the index answers from before the change, and the comparison must see it.
        for (const bool noted : {true, false}) {
            const Case c{noted ? "index after a list routine" : "stale index (negative control)", 4, {1, 2, 3, 4}, 3};
            auto original = setup(*program, c), modern = setup(*program, c);
            cm::aot::manager_manager_declining = true;
            require(execute(*original).empty(), c.label + ": first search");
            cm::aot::manager_manager_declining = false;
            require(execute(*modern).empty(), c.label + ": first search");
            for (auto* s : {original.get(), modern.get()}) {
                s->memory.write<4>(elements + 2 * 0x20 + 8, 7);  // key 3 becomes 7: same count, same head
                s->registers.fill(0xa5);
                s->wr<4>(4, stack); s->wr<4>(8, toc); s->wr<4>(12, container); s->wr<4>(16, key);
                s->wr<4>(4128, sentinel);
                s->pc = entry;
                s->budget = s->executed + c.budget;
            }
            if (noted) note_list_mutation(*modern);
            cm::aot::manager_manager_declining = true;
            require(execute(*original).empty(), c.label + ": second search");
            cm::aot::manager_manager_declining = false;
            require(execute(*modern).empty(), c.label + ": second search");
            const bool same = original->r<4>(12) == modern->r<4>(12);
            require(same == noted, c.label + (noted ? ": the rebuilt index differs from the walk" : ": the stale index was not seen"));
            ++cases;
        }
        // A fault is the original's. (A budget too small for the search no longer
        // stops it: the fast path steps aside to MANAGER_MANAGER.CPP's C++, which,
        // like every owner, takes no instruction budget.)
        compare(*program, {"unmapped list", 3, {1, 2, 3}, 3, -1, -1,
            [](State& s) { s.memory.write<4>(container + 8, 0x50000000); }}, false);
        cases += 6;
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
        parallel_candidates(*program);
        std::cout << cases << " list-search cases identical to the original; error reports left to the original; control caught\n";
        return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
