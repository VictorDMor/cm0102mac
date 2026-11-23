// The direct comparator glue (comparator_glue.hpp) against the dispatcher path
// it replaces, from fixtures on the loaded program: bsearch (0x10775600) and
// the sort entry (0x10768300, whose three heapsorts call the glue) with the
// game's own comparators through their transition vectors, rows with ties,
// wildcards and absent keys. Each case runs twice in forked children from the
// same state, once with the direct path off (callers make the guest call:
// glue page, import-descriptor owner, comparator page and owner) and once on;
// every written page, every register (volatile ones too) and the instruction
// count must match. Covered: comparators whose owners run them, one the
// dispatcher runs (compare_history_by_club), and, under
// CM_PLAYER_STATS_ORIGINAL=1 and CM_SHORTLIST_MANAGER_ORIGINAL=1 (the second
// test), owners that decline, so the generated comparators run. A spoiled
// direct run must differ. Then, in this process: each condition under which
// the direct path must decline (dispatch probe, routine checker,
// import-descriptor observer, its three original switches, budget 4 against
// exactly 5, the switch off; the PC profile in a run of its own, --profile),
// a routine checker appearing in the middle of a sort (owners are asked at
// every call), a comparator that longjmps out of a sort and out of a bsearch
// (the dispatcher scopes resume it as on the reference path), and a scan
// showing the comparators run without a scope make no call.
#include "comparator_glue.hpp"
#include "guest_abi.hpp"
#include "guest_context_transfer.hpp"
#include "import_descriptor.hpp"
#include "import_descriptor_adapter.hpp"
#include "match_turbo.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <cstdlib>
#include <iostream>
#include <string>
#include <string_view>

using namespace cm::aot;
using namespace cm::aot::guest;
namespace {
constexpr std::uint32_t data = 0x60000000, rows_at = data + 0x1000, key_at = data + 0x100, stack = 0x71010000,
    sentinel = 0xfffffff0, game_toc = 0x108bba90;
constexpr std::uint32_t entry_bsearch = 0x10775600, entry_sort = 0x10768300;
// Transition vectors (TOC slots): player_stats_detail.hpp, shortlist_manager_detail.hpp.
constexpr std::int32_t relations_tv = -0x5058, history_by_club_tv = -0x505c, pairs_by_first_tv = -0x62a8,
    pairs_by_second_tv = -0x6298, value_then_key_tv = -0x6290, value_then_key_twin_tv = -0x6294,
    linked_first_tv = -0x62a0, rank_tv = -0x62a4;

enum class Rows { relations, pairs, values, mixed };
struct Case {
    const char* name;
    std::uint32_t entry;
    std::int32_t tv;
    Rows rows;
    std::uint32_t width, count;
    std::uint32_t seed;
    std::int32_t key = 0;   // bsearch: the key's first word (its second is `key2`)
    std::int32_t key2 = 0;
    bool finite_budget = true;
};
Case active;

std::uint32_t next(std::uint32_t& state) { return state = state * 1664525u + 1013904223u; }

void initialize(Cpu& c) {
    c.s.memory.map(data, 0x80000);
    c.s.memory.map(0x71000000, 0x20000);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x80000, 0xa5));
    c.s.memory.copy_in(0x71000000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    for (unsigned f = 0; f < 8; ++f) c.set_cr(f, f + 1);
    c.set(1, stack); c.set(2, game_toc); c.set_lr(sentinel);
    auto state = active.seed * 2654435761u + 12345;
    for (std::uint32_t row = 0; row < active.count; ++row) {
        const auto at = rows_at + row * active.width;
        for (std::uint32_t i = 0; i < active.width; ++i) c.put8(at + i, next(state) >> 24);
        switch (active.rows) {
        case Rows::relations:  // sorted by person then group, with repeats
            c.put32(at, row / 3 * 2); c.put8(at + 4, std::uint32_t(row % 3 == 2 ? -1 : int(row % 3) * 5));
            break;
        case Rows::pairs:  // sorted (first, second) pairs, a few -1 seconds
            c.put32(at, row / 2); c.put32(at + 4, row % 4 == 3 ? 0xffffffffu : row % 2);
            break;
        case Rows::values:  // few distinct values and keys: many ties
            c.put32(at + 4, (next(state) >> 20) % 3); c.put32(at + 8, (next(state) >> 20) % 4);
            break;
        case Rows::mixed:
            break;
        }
    }
    c.put32(key_at, std::uint32_t(active.key));
    if (active.rows == Rows::relations) c.put8(key_at + 4, std::uint32_t(active.key2));
    else c.put32(key_at + 4, std::uint32_t(active.key2));
    const auto tv = c.toc(active.tv);
    if (active.entry == entry_bsearch) {
        c.set(3, key_at); c.set(4, rows_at); c.set(5, active.count); c.set(6, active.width); c.set(7, tv);
    } else {
        c.set(3, rows_at); c.set(4, active.count); c.set(5, active.width); c.set(6, tv);
    }
    // A finite budget keeps the fused 8-byte sort (native_sort8) out, so the
    // pair heapsort and its glue calls run.
    c.s.budget = active.finite_budget ? c.s.executed + 50000000 : UINT64_MAX;
}
void run(State& s, std::uint32_t entry, bool direct) {
    set_direct_glue_calls(direct);
    s.pc = entry;
    run_until(s, sentinel);
}
void reference(State& s, std::uint32_t entry) { run(s, entry, false); }
void direct(State& s, std::uint32_t entry) { run(s, entry, true); }
// Controls: the comparison must catch a wrong register or a wrong stack byte.
void spoil_register(State& s, std::uint32_t entry) { direct(s, entry); Cpu c(s); c.set(5, c.r(5) ^ 1); }
void spoil_saved_toc(State& s, std::uint32_t entry) {
    direct(s, entry);
    Cpu c(s);
    // The glue's store of r2 into the search's or sort's frame (sp+20 of the callee frame).
    for (std::uint32_t at = stack - 0x200; at < stack; at += 4)
        if (c.u32(at) == game_toc) { c.put32(at, game_toc ^ 4); return; }
    c.set(31, c.r(31) ^ 1);
}

std::vector<Case> cases() {
    std::vector<Case> all;
    // bsearch: keys present, absent, below and above every row, the -1 wildcard, and an empty array.
    for (auto [key, key2] : {std::pair{4, 5}, {7, 0}, {-3, 0}, {1000, 0}, {6, -1}, {0, 0}})
        for (std::uint32_t count : {0u, 1u, 2u, 17u, 300u})
            all.push_back({"bsearch relations", entry_bsearch, relations_tv, Rows::relations, 8, count, count, key, key2});
    for (auto [key, key2] : {std::pair{3, 1}, {3, -1}, {9, 0}, {-1, 0}, {500, 1}})
        for (std::uint32_t count : {1u, 3u, 64u, 1000u})
            all.push_back({"bsearch pairs by first", entry_bsearch, pairs_by_first_tv, Rows::pairs, 8, count, count, key, key2});
    for (std::uint32_t count : {5u, 40u})
        all.push_back({"bsearch pairs by second", entry_bsearch, pairs_by_second_tv, Rows::pairs, 8, count, count, 2, 1});
    // Sorts: words (12-byte rows), pairs (8, finite budget), bytes (6), with ties.
    for (std::uint32_t seed = 1; seed <= 4; ++seed)
        for (std::uint32_t count : {2u, 3u, 9u, 64u, 257u}) {
            all.push_back({"sort values (words)", entry_sort, seed % 2 ? value_then_key_tv : value_then_key_twin_tv,
                           Rows::values, 12, count, seed * 100 + count});
            all.push_back({"sort pairs (pairs)", entry_sort, pairs_by_first_tv, Rows::mixed, 8, count, seed * 100 + count});
            all.push_back({"sort pairs (bytes)", entry_sort, pairs_by_second_tv, Rows::mixed, 6, count, seed * 100 + count});
        }
    for (std::uint32_t count : {7u, 50u}) {
        all.push_back({"sort linked first (words)", entry_sort, linked_first_tv, Rows::mixed, 0x1c, count, count});
        all.push_back({"sort rank (pairs)", entry_sort, rank_tv, Rows::mixed, 0x28, count, count});
        all.push_back({"sort relations (pairs)", entry_sort, relations_tv, Rows::relations, 8, count, count});
        // Not a destination comparator_glue.cpp lists: the dispatcher runs it.
        all.push_back({"sort history by club (words)", entry_sort, history_by_club_tv, Rows::mixed, 12, count, count});
        all.push_back({"bsearch history by club", entry_bsearch, history_by_club_tv, Rows::mixed, 12, count, count, 1, 2});
    }
    // No instruction budget (the game's setting): the pair sort of an unlisted comparator and a bsearch.
    all.push_back({"sort history by club, no budget", entry_sort, history_by_club_tv, Rows::mixed, 8, 33, 5, 0, 0, false});
    all.push_back({"bsearch relations, no budget", entry_bsearch, relations_tv, Rows::relations, 8, 300, 9, 50, 5, false});
    return all;
}

// The state a run leaves, for in-process comparisons: registers, pc,
// instruction count, the glue's and the transfer counters, the fixture's
// memory and how the run ended.
struct Outcome {
    decltype(State::registers) registers;
    std::uint32_t pc;
    std::uint64_t executed, unknown_transfers;
    std::vector<std::uint64_t> counters;
    std::vector<std::uint8_t> data, stack_bytes;
    std::string ended;
    bool operator==(const Outcome&) const = default;
};
Outcome outcome(const State& s, std::string ended) {
    Outcome o{s.registers, s.pc, s.executed, s.register_context_unknown_transfers, {}, s.memory.copy_out(data, 0x80000),
              s.memory.copy_out(0x71000000, 0x20000), std::move(ended)};
    for (const auto* counters : {&s.import_descriptor_calls, &s.import_descriptor_tails, &s.import_descriptor_faults,
                                 &s.import_descriptor_transfers, &s.import_descriptor_budget_stops,
                                 &s.import_descriptor_original_calls, &s.import_descriptor_budget_original_calls,
                                 &s.import_descriptor_profile_original_calls})
        o.counters.insert(o.counters.end(), counters->begin(), counters->end());
    return o;
}
template <class Run> Outcome run_to_end(State& s, Run&& run) {
    try { run(s); } catch (const std::exception& e) { return outcome(s, e.what()); }
    catch (const GuestContextTransfer&) { return outcome(s, "context transfer"); }
    return outcome(s, "returned");
}

// One glue call as the heapsort makes it (r3, r4, r12 set; back = sentinel).
constexpr std::uint32_t glue = 0x10781320;
void guest_glue_call(State& s) {
    Cpu c(s);
    c.call(glue, sentinel);
}
void direct_glue_call(State& s) {
    GlueTarget target;
    if (!call_glue_directly(s, sentinel, target)) guest_glue_call(s);
}
struct NoImportObserver final : ImportDescriptorBoundaryObserver {
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool, ImportDescriptorOutcome) override {}
};
bool no_dispatch(State&) { return false; }
RoutineChecker& checker_wanting_nothing() {
    static RoutineChecker none([](std::uint32_t) { return false; }, 0, 1);
    return none;
}
// Each condition under which the direct path must decline (and two where it
// must not), on one glue call into compare_relations: the direct call count
// and the state left, against the guest call.
unsigned decline_checks(const State& base, bool profile) {
    unsigned failures = 0;
    struct Condition { const char* name; bool direct; void (*set)(State&); void (*reset)(State&); };
    static NoImportObserver observer;
    const Condition conditions[]{
        {"none", true, [](State&) {}, [](State&) {}},
        {"dispatch probe", false, [](State&) { dispatch_probe = no_dispatch; }, [](State&) { dispatch_probe = nullptr; }},
        {"routine checker", false, [](State&) { routine_checker = &checker_wanting_nothing(); },
         [](State&) { routine_checker = nullptr; }},
        {"import-descriptor observer", false, [](State& s) { s.import_descriptor_observer = &observer; },
         [](State& s) { s.import_descriptor_observer = nullptr; }},
        {"import-descriptor original", false, [](State& s) { s.import_descriptor_original = true; }, [](State&) {}},
        {"glue entry original", false,
         [](State& s) { s.import_descriptor_entry_original[import_descriptor::import_count] = true; }, [](State&) {}},
        {"glue original body", false, [](State& s) { s.import_descriptor_original_body = glue; }, [](State&) {}},
        {"budget 4", false, [](State& s) { s.budget = s.executed + 4; }, [](State&) {}},
        {"budget exactly 5", true, [](State& s) { s.budget = s.executed + 5; }, [](State&) {}},
        {"direct path off", false, [](State&) { set_direct_glue_calls(false); }, [](State&) { set_direct_glue_calls(true); }},
    };
    for (const auto& condition : conditions) {
        active = Case{"glue call", entry_sort, relations_tv, Rows::relations, 8, 8, 3};
        auto reference = cm::semantic_io_checks::clone(base), candidate = cm::semantic_io_checks::clone(base);
        for (auto* s : {reference.get(), candidate.get()}) {
            Cpu c(*s); initialize(c);
            c.set(3, rows_at + 8); c.set(4, rows_at + 16); c.set(12, c.toc(relations_tv));
        }
        condition.set(*reference);
        const auto expected = run_to_end(*reference, guest_glue_call);
        condition.reset(*reference);
        condition.set(*candidate);
        const auto before = direct_glue_calls();
        const auto actual = run_to_end(*candidate, direct_glue_call);
        const auto made = direct_glue_calls() - before;
        condition.reset(*candidate);
        const bool wanted = condition.direct && !profile;
        if (!(expected == actual) || made != (wanted ? 1u : 0u)) {
            ++failures;
            std::cout << "FAIL decline condition '" << condition.name << "': direct calls " << made << ", state "
                      << (expected == actual ? "equal" : "differs") << " (" << expected.ended << " / " << actual.ended << ")\n";
        }
    }
    return failures;
}

// Finding the owner's choice at every call: a routine checker that wants
// nothing appears after the sort's 20th row store, so from then on the
// comparators' owners decline (their generated bodies run) and the direct
// path declines; both runs must leave the same state.
struct FlipAfterStores final : MemoryObserver {
    unsigned stores = 0;
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t start, const std::uint8_t*, std::size_t) override {
        if (start >= rows_at && start < rows_at + 0x30000 && ++stores == 20) routine_checker = &checker_wanting_nothing();
    }
};
unsigned flip_checks(const State& base) {
    unsigned failures = 0;
    for (const auto& test : {Case{"flip sort values", entry_sort, value_then_key_tv, Rows::values, 12, 64, 7},
                             Case{"flip sort relations", entry_sort, relations_tv, Rows::relations, 8, 64, 8}}) {
        active = test;
        Outcome results[2];
        std::uint64_t made = 0;
        for (bool direct_path : {false, true}) {
            auto s = cm::semantic_io_checks::clone(base);
            Cpu c(*s); initialize(c);
            FlipAfterStores flip;
            s->memory.observer = &flip;
            const auto before = direct_glue_calls();
            results[direct_path] = run_to_end(*s, [&](State& state) { run(state, test.entry, direct_path); });
            if (direct_path) made = direct_glue_calls() - before;
            s->memory.observer = nullptr;
            routine_checker = nullptr;
            if (flip.stores < 20) { ++failures; std::cout << "FAIL " << test.name << ": too few stores\n"; }
        }
        set_direct_glue_calls(true);
        if (!(results[0] == results[1]) || !made) {
            ++failures;
            std::cout << "FAIL " << test.name << ": " << made << " direct calls, state "
                      << (results[0] == results[1] ? "equal" : "differs") << '\n';
        }
    }
    return failures;
}

// A comparator that longjmps: the vector names longjmp (0x10773f30), and every
// row the comparator can get as r3 holds a hand-made context returning to the
// test's caller (sentinel, its stack pointer). The transfer leaves the sort or
// search through the dispatcher scopes; the outer one resumes it.
constexpr std::uint32_t longjmp_tv = data + 0x40;
void jump_buffer(Cpu& c, std::uint32_t at) {
    for (std::uint32_t i = 16; i < 96; i += 4) c.put32(at + i, 0x52000000 + i);
    for (std::uint32_t i = 96; i < 256; i += 4) c.put32(at + i, 0);
    c.put32(at, sentinel); c.put32(at + 4, 0x24000000); c.put32(at + 8, stack); c.put32(at + 12, game_toc);
}
unsigned longjmp_checks(const State& base) {
    unsigned failures = 0;
    for (const auto& test : {Case{"longjmp out of a sort", entry_sort, 0, Rows::mixed, 256, 5, 4},
                             Case{"longjmp out of a bsearch", entry_bsearch, 0, Rows::mixed, 8, 10, 5}}) {
        active = test;
        Outcome results[2];
        std::uint64_t made = 0;
        for (bool direct_path : {false, true}) {
            auto s = cm::semantic_io_checks::clone(base);
            Cpu c(*s); initialize(c);
            c.put32(longjmp_tv, 0x10773f30); c.put32(longjmp_tv + 4, game_toc);
            c.set(test.entry == entry_bsearch ? 7 : 6, longjmp_tv);
            if (test.entry == entry_bsearch) jump_buffer(c, key_at);
            else for (std::uint32_t row = 0; row < test.count; ++row) jump_buffer(c, rows_at + row * test.width);
            const auto before = direct_glue_calls();
            results[direct_path] = run_to_end(*s, [&](State& state) { run(state, test.entry, direct_path); });
            if (direct_path) made = direct_glue_calls() - before;
        }
        set_direct_glue_calls(true);
        const bool resumed = results[1].ended == "returned" && results[1].pc == sentinel &&
                             results[1].unknown_transfers > base.register_context_unknown_transfers;
        if (!(results[0] == results[1]) || made != 1 || !resumed) {
            ++failures;
            std::cout << "FAIL " << test.name << ": " << made << " direct calls, ended " << results[0].ended << " / "
                      << results[1].ended << ", state " << (results[0] == results[1] ? "equal" : "differs") << '\n';
        }
    }
    return failures;
}

// The comparators comparator_glue.cpp runs without a scope make no call: no
// branch-and-link (bl, bcl, bclrl, bcctrl) in their original bodies. bsearch
// itself must be flagged (the scan can fail).
bool links(const State& s, std::uint32_t entry, std::uint32_t end) {
    for (auto at = entry; at < end; at += 4) {
        const auto word = std::uint32_t(s.memory.read<4>(at)), op = word >> 26, xo = (word >> 1) & 0x3ff;
        if ((word & 1) && (op == 18 || op == 16 || (op == 19 && (xo == 16 || xo == 528)))) return true;
    }
    return false;
}
unsigned scan_checks(const State& base) {
    unsigned failures = 0;
    for (auto [entry, end] : {std::pair{0x10309a00u, 0x10309a70u}, {0x10358f20u, 0x10358f50u}, {0x10358f50u, 0x10358f80u},
                              {0x10358f80u, 0x10358ff0u}, {0x10358ff0u, 0x10359020u}, {0x103663b0u, 0x10366410u},
                              {0x10366410u, 0x10366470u}})
        if (links(base, entry, end)) { ++failures; std::cout << "FAIL " << hex_address(entry) << " makes a call\n"; }
    if (!links(base, entry_bsearch, 0x107756f0)) { ++failures; std::cout << "FAIL the call scan found no call in bsearch\n"; }
    return failures;
}
}

int main(int argc, char** argv) {
    const bool profile = argc == 3 && std::string_view(argv[2]) == "--profile";
    if (argc != 2 && !profile) return 2;
    try {
        auto base = std::make_unique<State>();
        load_program(*base, argv[1]);
        count_direct_glue_calls(true);
        if (profile) {
            // With the PC profile on, the direct path declines everywhere.
            if (!match_pc_profile_enabled()) throw std::runtime_error("--profile needs CM_AOT_MATCH_PC_PROFILE=1");
            const auto failures = decline_checks(*base, true);
            std::cout << "PC profile: " << (failures ? "FAILED" : "every glue call made by the dispatcher") << '\n';
            return failures ? 1 : 0;
        }
        const bool declining = std::getenv("CM_PLAYER_STATS_ORIGINAL") && std::getenv("CM_SHORTLIST_MANAGER_ORIGINAL");
        unsigned passed = 0, failures = 0;
        std::uint64_t direct_calls = 0;
        for (const auto& test : cases()) {
            active = test;
            auto s = cm::semantic_io_checks::clone(*base);
            Cpu c(*s); initialize(c);
            const auto differences =
                compare_routine_call(*s, test.entry, reference, direct, RoutineChecker::result_r3, false, true);
            // In this process: the direct path must take every glue call the reference makes.
            auto probe = cm::semantic_io_checks::clone(*s);
            const auto before = direct_glue_calls(), executed = probe->executed;
            direct(*probe, test.entry);
            const auto made = direct_glue_calls() - before;
            auto again = cm::semantic_io_checks::clone(*s);
            reference(*again, test.entry);
            set_direct_glue_calls(true);
            const bool calls_comparator = test.count > 1 || (test.entry == entry_bsearch && test.count == 1);
            const bool same_count = probe->executed == again->executed && probe->executed > executed;
            if (!differences.empty() || (calls_comparator && !made) || !same_count) {
                ++failures;
                std::cout << "FAIL " << test.name << " count=" << test.count << " seed=" << test.seed
                          << " direct glue calls=" << made << " executed " << probe->executed << '/' << again->executed << '\n';
                for (const auto& d : differences) std::cout << "  " << d << '\n';
                if (failures >= 6) break;
            } else {
                ++passed;
                direct_calls += made;
            }
        }
        set_direct_glue_calls(true);
        // Controls: a spoiled direct run must differ.
        unsigned controls = 0;
        for (const auto& [name, runner] : {std::pair{"register", &spoil_register}, {"saved TOC", &spoil_saved_toc}})
            for (const auto& test : {Case{"control bsearch", entry_bsearch, relations_tv, Rows::relations, 8, 300, 9, 50, 5},
                                     Case{"control sort", entry_sort, value_then_key_tv, Rows::values, 12, 64, 3}}) {
                active = test;
                auto s = cm::semantic_io_checks::clone(*base);
                Cpu c(*s); initialize(c);
                if (compare_routine_call(*s, test.entry, reference, runner, RoutineChecker::result_r3, false, true).empty()) {
                    ++failures;
                    std::cout << "FAIL " << name << " control not detected: " << test.name << '\n';
                } else ++controls;
            }
        set_direct_glue_calls(true);
        const auto declines = decline_checks(*base, false), flips = flip_checks(*base), jumps = longjmp_checks(*base),
                   scans = scan_checks(*base);
        failures += declines + flips + jumps + scans;
        std::cout << (declining ? "owners declining: " : "") << passed << " cases equal to the dispatcher path ("
                  << direct_calls << " direct glue calls); " << controls << "/4 spoiled runs detected; decline conditions "
                  << (declines ? "FAILED" : "ok") << "; owner flip mid-sort " << (flips ? "FAILED" : "ok") << "; longjmp "
                  << (jumps ? "FAILED" : "ok") << "; call scan " << (scans ? "FAILED" : "ok") << "; " << failures
                  << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
