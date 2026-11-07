// DATABASE.CPP's C++ routines against an independent generated reference of
// their original bodies (tools/make_database_reference.py), from controlled
// states: every value a rating's inputs can take, preference lists with
// every hit position, misses, duplicates, null records and extreme ids, and
// entry registers, CR, XER[SO] and FPSCR bits set to values the original
// must leave or carry through. No game window or career is opened.
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
                        preferences = data + 0x400, stack = 0x71010000, sentinel = 0xfffffff0;
constexpr std::array ratings{0x100d2d70u, 0x100d2e60u, 0x100d2f50u};
constexpr std::array club_lists{0x100d9480u, 0x100d94d0u};
constexpr std::array person_lists{0x100d9520u, 0x100d9580u};
constexpr std::array person_club_lists{0x100da050u, 0x100da0b0u};

bool original_mode = false, production_mode = false;

bool probe(State& s) {
    if (!database_lists(s.pc)) return false;
    if (original_mode) return database_reference(s);
    if (production_mode) return false;  // the generated page and its hook
    database_run_rewrite(s, s.pc);
    return true;
}

struct Case {
    std::uint32_t entry = 0, r3 = 0, r4 = 0;
    std::int16_t field = 0;
    std::array<std::uint32_t, 3> slots{};
    bool null_preferences = false, summary_overflow = false, sticky_fpscr = false;
};

void initialize(Cpu& c, const Case& t) {
    c.s.memory.map(data, 0x10000);
    c.s.memory.map(0x71000000, 0x20000);
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
void corrupt(State& s, std::uint32_t entry) {
    rewrite(s, entry);
    Cpu c(s);
    c.set(3, c.r(3) ^ 1);
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
                              << " r3=" << hex_address(test.r3) << " r4=" << hex_address(test.r4) << " field=" << test.field
                              << " slots=" << hex_address(test.slots[0]) << ',' << hex_address(test.slots[1]) << ','
                              << hex_address(test.slots[2]) << " null_preferences=" << test.null_preferences
                              << " so=" << test.summary_overflow << '\n';
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
        // The comparison must fail when the rewrite's result is wrong.
        for (const auto entry : {ratings[2], club_lists[0]}) {
            Case t;
            t.entry = entry;
            t.r3 = entry == ratings[2] ? record : 0x60001000;
            t.r4 = entry == ratings[2] ? 7 : club;
            t.slots = {0x60001000, 0, 0};
            t.field = 120;
            auto s = cm::semantic_io_checks::clone(*base);
            Cpu c(*s);
            initialize(c, t);
            if (compare_routine_call(*s, entry, original, corrupt, RoutineChecker::result_r3).empty()) {
                ++failures;
                std::cout << "FAIL corruption control " << hex_address(entry) << '\n';
            } else {
                std::cout << "ok deliberate result corruption detected at " << hex_address(entry) << '\n';
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
        for (const auto& [entry, n] : counts)
            std::cout << hex_address(entry) << " returned " << n[0] << " controlled + " << n[1] << " production\n";
        const auto entries_owned = std::ranges::count_if(owned_entries, [](const auto& item) { return item.second; });
        std::cout << entries_owned << "/9 routines owned in production over " << owned << " budget-free calls; " << completed << " returned comparisons; " << failures
                  << " failures\n";
        return failures ? 1 : 0;
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
