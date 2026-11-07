// DATABASE.CPP's C++ routines against an independent generated reference of
// their original bodies (tools/make_database_reference.py), from controlled
// states: every value a rating's inputs can take, preference lists with
// every hit position, misses, duplicates, null records and extreme ids, and
// entry registers, CR, XER[SO] and FPSCR bits set to values the original
// must leave or carry through; contract queries with scripted contracts
// (CONTRACT_MANAGER.CPP's two lookups are stubbed and every call's
// registers recorded in memory); position masks over every rating that
// changes a test. No game window or career is opened.
#include "database_adapter.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include "guest_abi.hpp"
#include <array>
#include <iostream>
#include <algorithm>
#include <map>

using namespace cm::aot;
using namespace cm::aot::guest;
namespace cm::aot { bool database_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x60000000, record = data + 0x100, club = data + 0x200, person = data + 0x300,
                        preferences = data + 0x400, primary = data + 0x500, club_contract = data + 0x600,
                        player = data + 0x900, manager_word = data + 0xa00, last_primary_word = data + 0xa04,
                        table_word = data + 0xa08, trace_at = data + 0x2000, stack = 0x71010000, sentinel = 0xfffffff0,
                        high = 0x90000000;  // records above 0x7fffffff, for signed compares
constexpr std::size_t listed_routines = 13;
constexpr std::array ratings{0x100d2d70u, 0x100d2e60u, 0x100d2f50u};
constexpr std::array club_lists{0x100d9480u, 0x100d94d0u};
constexpr std::array person_lists{0x100d9520u, 0x100d9580u};
constexpr std::array person_club_lists{0x100da050u, 0x100da0b0u};
constexpr std::array contract_flags{0x100d3150u, 0x100d3230u, 0x100d3300u};
constexpr std::uint32_t position_mask = 0x100db1b0, primary_lookup = 0x10082f20, club_lookup = 0x10082f90;

bool original_mode = false, production_mode = false;

struct Case {
    std::uint32_t entry = 0, r3 = 0, r4 = 0;
    std::int16_t field = 0;
    std::array<std::uint32_t, 3> slots{};
    bool null_preferences = false, summary_overflow = false, sticky_fpscr = false;
    // Contract queries: what the two lookups answer, and the club table.
    std::uint32_t primary_answer = 0, index = 0, table = 0, club_answer = 0;
    std::uint8_t flags = 0;
    bool clobber = false;
    // Position masks: the ratings from the goalkeeper's (+0xf) to +0x19.
    std::array<std::uint8_t, 11> ratings{};
    bool null_player = false;
};
Case active;

// CONTRACT_MANAGER.CPP's lookups, scripted: each call appends its target,
// LR, r0-r10, r13-r31 and CR to the trace in guest memory (so the compared
// memory holds the call's arguments and preserved registers), answers from
// the case and, with `clobber`, leaves volatile registers and CR the caller
// must carry through.
void lookup(State& s) {
    Cpu c(s);
    const auto size = c.u32(trace_at);
    auto at = trace_at + 4 + size * 4;
    const auto add = [&](std::uint32_t value) { c.put32(at, value); at += 4; };
    add(s.pc);
    add(c.lr());
    for (unsigned r = 0; r <= 10; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned f = 0; f < 8; ++f) add(c.cr(f));
    c.put32(trace_at, (at - trace_at - 4) / 4);
    if (active.clobber) {
        for (unsigned r = 4; r <= 10; ++r) c.set(r, 0x41000000 + size * 0x100 + r);
        c.compare(0, 17, 29);
        c.set_cr(6, 0x5);
        c.set_f(1, 3.5 + size);
    }
    c.set(3, s.pc == primary_lookup ? active.primary_answer : active.club_answer);
    s.pc = c.lr();
}

bool probe(State& s) {
    if (s.pc == primary_lookup || s.pc == club_lookup) {
        lookup(s);
        return true;
    }
    if (!database_lists(s.pc)) return false;
    if (original_mode) return database_reference(s);
    if (production_mode) return false;  // the generated page and its hook
    database_run_rewrite(s, s.pc);
    return true;
}

void initialize(Cpu& c, const Case& t) {
    c.s.memory.map(data, 0x10000);
    c.s.memory.map(0x71000000, 0x20000);
    c.s.memory.map(high, 0x1000);
    c.s.memory.copy_in(high, std::vector<std::uint8_t>(0x1000, 0xa5));
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x10000, 0xa5));
    c.s.memory.copy_in(0x71000000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    for (unsigned f = 0; f < 32; ++f) c.set_f(f, 1.25 + f);
    for (unsigned f = 0; f < 8; ++f) c.set_cr(f, f + 1);
    c.set(1, stack);
    c.set(2, 0x108bba90);
    c.set_lr(sentinel);
    c.s.wr<1>(reg::xer_so, t.summary_overflow);
    if (t.sticky_fpscr) for (const std::size_t bit : {1280u, 1286u, 1294u, 1295u}) c.s.wr<1>(bit, 1);
    // Ratings: the 16-bit value at both offsets the routines read.
    c.put16(record + 0x4, std::uint16_t(t.field));
    c.put16(record + 0x24, std::uint16_t(t.field));
    // Lists: the same three ids in every list of the club and the
    // preferences, so each routine sees them at its own offsets.
    for (unsigned i = 0; i < 3; ++i) {
        for (const std::uint32_t list : {0x5cu, 0x68u}) c.put32(club + list + 4 * i, t.slots[i]);
        for (const std::uint32_t list : {0x4u, 0x10u, 0x1cu, 0x28u}) c.put32(preferences + list + 4 * i, t.slots[i]);
    }
    c.put32(person + 0x18, t.null_preferences ? 0 : preferences);
    // Contract queries: the TOC words point at the manager's word, the
    // "last primary contract" word and the club table's base word.
    c.put32(c.r(2) - 0x7b7c, manager_word);
    c.put32(manager_word, data + 0xb00);
    c.put32(c.r(2) - 0x7b78, last_primary_word);
    c.put32(c.r(2) - 0x7be8, table_word);
    c.put32(table_word, t.table);
    if (t.primary_answer) c.put32(t.primary_answer + 0x4, t.index);
    if (t.club_answer) c.put8(t.club_answer + 0x5c, t.flags);
    c.put32(trace_at, 0);
    // Position masks.
    c.put32(person + 0x14, t.null_player ? 0 : player);
    for (unsigned i = 0; i < t.ratings.size(); ++i) c.put8(player + 0xf + i, t.ratings[i]);
    c.set(3, t.r3);
    c.set(4, t.r4);
    c.s.budget = c.s.executed + 100000;
}

void original(State& s, std::uint32_t entry) {
    original_mode = true;
    s.pc = entry;
    run_until(s, sentinel);
}
void rewrite(State& s, std::uint32_t entry) {
    original_mode = false;
    if (production_mode) {
        s.pc = entry;
        run_until(s, sentinel);
    } else {
        database_run_rewrite(s, entry);
    }
}
// Spoilers for the controls that show the comparison can fail: the result,
// the "last primary contract" word, and the recorded arguments of the first
// CONTRACT_MANAGER call (its r4, the person).
void corrupt(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu c(s);
    c.set(3, c.r(3) ^ 1);
}
void corrupt_last_primary(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu c(s);
    c.put8(last_primary_word, c.u8(last_primary_word) ^ 1);
}
void corrupt_call_argument(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu c(s);
    const auto at = trace_at + 4 + 4 * (2 + 4);  // target, LR, r0-r3, then r4
    c.put8(at, c.u8(at) ^ 1);
}
std::string describe(const Case& t) {
    std::string text = " r3=" + hex_address(t.r3) + " r4=" + hex_address(t.r4) + " field=" + std::to_string(t.field) +
                       " slots=" + hex_address(t.slots[0]) + ',' + hex_address(t.slots[1]) + ',' + hex_address(t.slots[2]) +
                       " null_preferences=" + std::to_string(t.null_preferences) + " so=" + std::to_string(t.summary_overflow) +
                       " primary=" + hex_address(t.primary_answer) + " index=" + hex_address(t.index) +
                       " table=" + hex_address(t.table) + " club_contract=" + hex_address(t.club_answer) +
                       " flags=" + std::to_string(t.flags) + " clobber=" + std::to_string(t.clobber) +
                       " null_player=" + std::to_string(t.null_player) + " ratings=";
    for (const auto rating : t.ratings) text += std::to_string(rating) + ',';
    return text;
}

std::vector<Case> cases() {
    std::vector<Case> found;
    // Ratings: the byte (with junk above it, which the original drops) and
    // the 16-bit value across its range and the formula's clamps.
    const std::array<std::int16_t, 17> fields{-32768, -1000, -200, -100, -1, 0, 1, 50, 99, 100,
                                              150, 199, 200, 201, 255, 1000, 32767};
    for (const auto entry : ratings)
        for (const auto field : fields)
            for (int value = -128; value < 128; ++value) {
                Case t;
                t.entry = entry;
                t.r3 = record;
                t.r4 = (value & 0x0f ? 0x12345600u : 0u) | std::uint8_t(value);
                t.field = field;
                t.sticky_fpscr = (value & 0x1f) == 3;
                found.push_back(t);
            }
    // Lists: ids at each slot, absent or repeated, logical extremes.
    const std::uint32_t a = 0x60001000, b = 0x80000000, d = 0xffffffff;
    const std::array<std::array<std::uint32_t, 3>, 6> patterns{{
        {a, b, d}, {b, a, d}, {d, b, a}, {a, a, a}, {0, 0, 0}, {0x7fffffff, 1, 0x60000fff}}};
    const std::array<std::uint32_t, 5> ids{0, a, b, d, 0x60002000};
    for (const auto& slots : patterns)
        for (const auto id : ids)
            for (const bool null_record : {false, true})
                for (const bool null_preferences : {false, true})
                    for (const bool overflow : {false, true}) {
                        Case t;
                        t.slots = slots;
                        t.null_preferences = null_preferences;
                        t.summary_overflow = overflow;
                        for (const auto entry : club_lists) {
                            t.entry = entry;
                            t.r3 = id;
                            t.r4 = null_record ? 0 : club;
                            found.push_back(t);
                        }
                        for (const auto entry : person_lists) {
                            t.entry = entry;
                            t.r3 = id;
                            t.r4 = null_record ? 0 : person;
                            found.push_back(t);
                        }
                        for (const auto entry : person_club_lists) {
                            t.entry = entry;
                            t.r3 = null_record ? 0 : person;
                            t.r4 = id;
                            found.push_back(t);
                        }
                    }
    // Contract queries: every way to a lookup, the flag bytes that change a
    // test, and records on both sides of 0x80000000 (signed compares) with
    // and without XER[SO].
    const std::array<std::uint8_t, 9> flag_values{0, 1, 2, 4, 8, 9, 0xa, 0xf6, 0xff};
    for (const auto entry : contract_flags) {
        Case t;
        t.entry = entry;
        found.push_back(t);  // null person
        for (const bool overflow : {false, true})
            for (const std::uint32_t someone : {data + 0x700, high + 0x700})
                for (const bool clobber : {false, true}) {
                    t.summary_overflow = overflow;
                    t.r3 = someone;
                    t.clobber = clobber;
                    const auto with_contract = [&](Case u) {
                        u.club_answer = 0;
                        found.push_back(u);
                        for (const std::uint32_t answer : {club_contract, high + 0x600}) {
                            u.club_answer = answer;
                            for (const auto flags : flag_values) {
                                u.flags = flags;
                                found.push_back(u);
                            }
                        }
                    };
                    for (const std::uint32_t the_club : {0u, data + 0x800, high + 0x800}) {
                        t.r4 = the_club;
                        if (the_club) {
                            with_contract(t);
                            continue;
                        }
                        t.primary_answer = 0;
                        found.push_back(t);
                        for (const std::uint32_t answer : {primary, high + 0x500}) {
                            t.primary_answer = answer;
                            // The club address comes out 0, positive, negative or wrapped.
                            for (const auto& [index, table] : std::array<std::pair<std::uint32_t, std::uint32_t>, 4>{{
                                     {0, 0}, {3, data + 0x1000}, {3, 0x7ffffff0}, {0xffffffff, 0x100}}}) {
                                t.index = index;
                                t.table = table;
                                if (index == 0 && table == 0) found.push_back(t);
                                else with_contract(t);
                            }
                        }
                    }
                }
    }
    // Position masks: each rating alone at the values that change a test,
    // then pseudo-random mixes of them.
    const std::array<std::uint8_t, 9> values{0, 10, 11, 14, 15, 19, 20, 0x94, 0xff};
    {
        Case t;
        t.entry = position_mask;
        found.push_back(t);  // null person
        t.r3 = person;
        t.null_player = true;
        found.push_back(t);
        t.null_player = false;
        for (unsigned i = 0; i < t.ratings.size(); ++i)
            for (const auto value : values) {
                t.ratings = {};
                t.ratings[i] = value;
                found.push_back(t);
            }
        std::uint32_t seed = 0x2545f491;
        for (unsigned n = 0; n < 4000; ++n) {
            for (auto& rating : t.ratings) {
                seed = seed * 1664525u + 1013904223u;
                // Mostly low ratings, so the "no natural position" pass runs.
                rating = (seed >> 24) < 96 ? values[(seed >> 8) % values.size()] : std::uint8_t((seed >> 12) % 15);
            }
            t.summary_overflow = n % 7 == 0;
            found.push_back(t);
        }
    }
    return found;
}

// The whole register file the routines can touch, byte for byte, after both
// sides ran in this process: r0-r31, f0-f31, XER, the FPSCR, CR, LR and CTR.
// compare_routine_call leaves the volatile FPRs out, since no caller reads
// them; these leaves set them all, so all of them are compared here.
std::string register_differences(State& a, State& b) {
    const std::array<std::pair<std::size_t, std::size_t>, 7> ranges{{
        {reg::gpr(0), reg::gpr(32)}, {reg::fpr(0), reg::fpr(32)}, {1024, 1028}, {1280, 1312},
        {reg::cr(0), reg::cr(8)}, {reg::lr, reg::lr + 4}, {reg::ctr, reg::ctr + 4}}};
    for (const auto& [begin, end] : ranges)
        for (auto at = begin; at < end; ++at)
            if (a.r<1>(at) != b.r<1>(at))
                return "register byte " + std::to_string(at) + " original " + std::to_string(a.r<1>(at)) +
                       " rewrite " + std::to_string(b.r<1>(at));
    return {};
}

struct Observed final : MemoryObserver {
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
};
}

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    try {
        auto base = std::make_unique<State>();
        load_program(*base, argv[1]);
        dispatch_probe = probe;
        const auto all = cases();
        unsigned completed = 0, failures = 0;
        std::map<std::uint32_t, std::array<unsigned, 2>> counts;
        for (const bool production : {false, true}) {
            production_mode = production;
            for (const auto& test : all) {
                active = test;
                auto s = cm::semantic_io_checks::clone(*base);
                Cpu c(*s);
                initialize(c, test);
                Observed observer;
                if (production) s->memory.observer = &observer;
                auto differences = compare_routine_call(*s, test.entry, original, rewrite,
                    RoutineChecker::result_r3 | RoutineChecker::result_f1, false, true);
                if (differences.empty()) {
                    auto a = cm::semantic_io_checks::clone(*s), b = cm::semantic_io_checks::clone(*s);
                    original(*a, test.entry);
                    if (production) b->memory.observer = &observer;
                    rewrite(*b, test.entry);
                    if (const auto found = register_differences(*a, *b); !found.empty()) differences.push_back(found);
                }
                if (!differences.empty()) {
                    std::cout << "DIFFERS " << hex_address(test.entry) << " production=" << production
                              << describe(test) << '\n';
                    for (const auto& d : differences) std::cout << "  " << d << '\n';
                    if (++failures >= 6) break;
                } else {
                    ++completed;
                    ++counts[test.entry][production ? 1 : 0];
                }
            }
            if (failures) break;
        }
        production_mode = false;
        // The comparison must fail when the rewrite is wrong: its result, a
        // global it writes, the arguments it passes.
        {
            Case rating;
            rating.entry = ratings[2];
            rating.r3 = record;
            rating.r4 = 7;
            rating.field = 120;
            Case list;
            list.entry = club_lists[0];
            list.r3 = 0x60001000;
            list.r4 = club;
            list.slots = {0x60001000, 0, 0};
            Case lookup_case;
            lookup_case.entry = contract_flags[0];
            lookup_case.r3 = data + 0x700;
            lookup_case.primary_answer = primary;
            lookup_case.index = 3;
            lookup_case.table = data + 0x1000;
            lookup_case.club_answer = club_contract;
            lookup_case.flags = 1;
            Case positions;
            positions.entry = position_mask;
            positions.r3 = person;
            positions.ratings[6] = 15;
            const std::array<std::pair<Case, RoutineRunner>, 6> controls{{
                {rating, corrupt}, {list, corrupt}, {lookup_case, corrupt}, {lookup_case, corrupt_last_primary},
                {lookup_case, corrupt_call_argument}, {positions, corrupt}}};
            for (const auto& [t, spoiler] : controls) {
                active = t;
                auto s = cm::semantic_io_checks::clone(*base);
                Cpu c(*s);
                initialize(c, t);
                if (compare_routine_call(*s, t.entry, original, spoiler, RoutineChecker::result_r3).empty()) {
                    ++failures;
                    std::cout << "FAIL corruption control " << hex_address(t.entry) << '\n';
                } else {
                    std::cout << "ok deliberate corruption detected at " << hex_address(t.entry) << '\n';
                }
            }
        }
        // Production ownership: with no instruction budget, the normal app
        // path (the generated page and its hook) must still return, so no
        // generated instruction of a hooked routine runs.
        // Every controlled case, so that no path of any routine runs one.
        unsigned owned = 0;
        std::map<std::uint32_t, bool> owned_entries;
        production_mode = true;
        for (const auto& test : all) {
            active = test;
            auto s = cm::semantic_io_checks::clone(*base);
            Cpu c(*s);
            initialize(c, test);
            s->budget = s->executed;
            try {
                rewrite(*s, test.entry);
                ++owned;
                owned_entries.try_emplace(test.entry, true);
            } catch (const std::exception& e) {
                owned_entries[test.entry] = false;
                if (++failures <= 6)
                    std::cout << "FAIL production ownership " << hex_address(test.entry) << ": " << e.what() << '\n';
            }
        }
        production_mode = false;
        if (owned_entries.size() != listed_routines) {
            ++failures;
            std::cout << "FAIL cases cover " << owned_entries.size() << " of the " << listed_routines << " listed routines\n";
        }
        for (const auto& [entry, n] : counts)
            std::cout << hex_address(entry) << " returned " << n[0] << " controlled + " << n[1] << " production\n";
        const auto entries_owned = std::ranges::count_if(owned_entries, [](const auto& item) { return item.second; });
        std::cout << entries_owned << "/" << owned_entries.size() << " routines owned in production over " << owned << " budget-free calls; " << completed << " returned comparisons; " << failures
                  << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
