// Differential fixtures for every ENG_PRM.CPP entry. External dependencies
// are deterministic stand-ins; the module's own nested routines run normally.
// A trace in compared guest memory records call order, continuations, argument
// and preserved registers. No fixture uses or changes an installed career.
#include "eng_prm_adapter.hpp"
#include "eng_prm_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <iostream>
#include <map>
#include <set>
#include <vector>

using namespace cm::aot;
using guest::Cpu;
namespace cm::aot { bool eng_prm_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x60000000, object = data + 0x1000, clubs = data + 0x10000,
    divisions = data + 0x2000, definitions = data + 0x3000, table = data + 0x4000,
    rows = data + 0x30000, log_at = data + 0x40000, sentinel = 0xfffffff0;
struct Scenario {
    std::uint32_t entry;
    std::string name;
    unsigned club_total = 24, premier_total = 20, nonleague_total = 4, eligible = 1;
    int day = 13, stage = -1, year = 2001, fail_allocation = 0, fail_call = 0, fail_virtual = -1;
    bool conference = true, deleting = false, have_schedule = true, have_fixtures = true;
    bool live_dependencies = false, null_object = false, output_flag = true;
    unsigned selected_leagues = 3, relegations = 3;
    unsigned remembered = 0;
    bool varied_club_inputs = false;
};
Scenario active{};
unsigned allocations = 0, calls = 0;
bool reference_mode = false;
std::uint32_t next_allocation = 0;
std::set<std::uint32_t> dependencies{
    0x1049e4f0, 0x104a00a0, 0x1049eb30, 0x10773520, 0x104533a0, 0x104a6660,
    0x10775170, 0x104536c0, 0x1049e600, 0x107735a0, 0x10775130, 0x104a60e0,
    0x1049f540, 0x1049f6d0, 0x104a9810, 0x104a9890, 0x107156f0, 0x10739890,
    0x10768810, 0x1077b720, 0x10769b60, 0x104564a0, 0x104aa240, 0x104aa3a0,
    0x102d46c0, 0x104635b0, 0x10103510, 0x100f7cf0, 0x1049ea00, 0x10452a60,
    0x1049f780, 0x1049f8e0, 0x104a9210};

void trace(Cpu& c, std::uint32_t target) {
    if (++calls > 2000) throw std::runtime_error("fixture call limit");
    auto index = c.u32(log_at);
    const auto add = [&](std::uint32_t word) { c.put32(log_at + 4 + index++ * 4, word); };
    add(target); add(c.lr());
    for (unsigned r = 3; r < 11; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned field = 0; field < 8; ++field) add(c.cr(field));
    if (target == 0x104a9810) add(c.u32(c.sp() + 0x38));
    c.put32(log_at, index);
}

bool probe(State& s) {
    Cpu c(s);
    const auto target = s.pc;
    // Cpu::call takes the runtime's leaf shortcut before the dispatcher. The
    // isolated reference takes the same shortcut here, so leaf dependencies
    // have identical real effects on both sides.
    if (run_leaf(s)) return true;
    if (active.live_dependencies) {
        if (target >= 0x10529840 && target < 0x1052c9d0) {
            if (reference_mode) return eng_prm_reference(s);
            if (eng_prm_lists(target)) { eng_prm_run_rewrite(s, target); return true; }
        }
        return false;
    }
    if (target == 0x10781320) {
        const auto descriptor = c.r(12), entry = c.u32(descriptor);
        c.put32(c.sp() + 0x14, c.r(2));
        // Preserve the original module's virtual methods, except deliberately
        // injected failures. The base methods have deterministic answers.
        if (c.r(3) == object && entry >= 0x10529840 && entry < 0x1052c9d0 &&
            !((entry == 0x1052bd60 && active.fail_virtual == 0x94))) {
            c.set(2, c.u32(descriptor + 4)); s.pc = entry; return true;
        }
        trace(c, entry);
        const auto early = c.u32(c.u32(object) + 0x20);
        c.set(3, entry == c.u32(early) ? (active.fail_virtual == 0x20 ? 1 : 0)
                                      : entry == 0x1052bd60 ? 0 : 1);
        s.pc = c.lr(); return true;
    }
    if (!dependencies.contains(target)) {
        if (eng_prm_lists(target) && !reference_mode) { eng_prm_run_rewrite(s, target); return true; }
        return reference_mode && target >= 0x10529840 && target < 0x1052c9d0 && eng_prm_reference(s);
    }
    trace(c, target);
    if (target == 0x10775130 || target == 0x10773520) {
        ++allocations;
        if (active.fail_allocation == int(allocations)) c.set(3, 0);
        else {
            const auto size = c.r(3), result = next_allocation;
            next_allocation += (size + 31) & ~15u;
            c.set(3, result);
        }
    } else if (target == 0x1049ea00) {
        c.set(3, rows + (c.r(4) & 63) * 0x44);
    } else if (target == 0x100f7cf0) c.set(3, active.eligible);
    else if (target == 0x1049f540) c.set(3, 0x12345);
    else if (target == 0x107156f0) { c.put16(c.r(3), 100); }
    else if (target == 0x104a9810) {
        // The December switch reads the date written for round 17.
        if (c.r(4) == 17) c.put16(c.r(3) + 0x50c, 100 + active.day - 1);
    } else if (target == 0x104a60e0) {
        c.put32(c.u32(c.r(3) + 0xb0) + c.r(4) * 0x44, c.r(5));
    } else if (target == 0x1049e4f0) {
        c.put32(c.r(3) + 0x20, 1);
    } else if (target == 0x10739890) {
        c.put32(c.r(6), 0x454e4700); c.put32(c.r(7), 0x43505000);
    } else if (target == 0x1077b720) {
        c.put32(c.r(3), c.r(8));
    } else c.set(3, active.fail_call == int(target) ? 0 : 1);
    s.pc = c.lr(); return true;
}

void initialize(Cpu& c, const Scenario& test) {
    c.s.memory.map(data, 0x200000);
    c.s.memory.map(0x71000000, 0x20000);
    c.s.memory.copy_in(0x71000000, std::vector<std::uint8_t>(0x20000, 0xa5));
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    c.set(1, 0x71010000); c.set(2, 0x108bba90); c.set_lr(sentinel);
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, field + 1);
    const auto global = [&](int slot, std::uint32_t value) { c.put32(c.r(2) + std::uint32_t(slot), value); };
    const auto variable = [&](int slot, unsigned offset, std::uint32_t value) {
        global(slot, data + offset); c.put32(data + offset, value);
    };
    variable(-0x7b50, 0x100, test.club_total);
    variable(-0x7be8, 0x104, clubs);
    variable(-0x7b5c, 0x108, table);
    variable(-0x67a0, 0x10c, 0); variable(-0x67a4, 0x110, 1);
    variable(-0x67a8, 0x114, 2); variable(-0x67ac, 0x118, 3);
    variable(-0x6874, 0x11c, 4); variable(-0x7a0c, 0x120, 5);
    variable(-0x7a10, 0x124, 99); variable(-0x7bbc, 0x128, data + 0x5000);
    variable(-0x7ab8, 0x12c, data + 0x7000); variable(-0x7428, 0x130, data + 0x7100);
    // A missing finance manager is an original-supported state. It lets the
    // live collect-clubs checks exercise its real allocation, standing-row
    // initialization and prize calculation without inventing a finance model.
    if (test.live_dependencies && test.entry == 0x1052bbc0) c.put32(data + 0x12c, 0);
    global(-0x7ba8, data + 0x8000); global(-0x7bac, data + 0x9000);
    global(-0x7b80, data + 0xa000); c.put16(data + 0xa002, test.year);
    global(-0x4524, data + 0xb000); // Three remembered lower-league pointers.
    for (unsigned i = 0; i < 3; ++i)
        if (i < test.remembered)
            c.put32(data + 0xb000 + i * 4, definitions + (test.remembered == 4 ? 12 + i : 6 + i) * 16);
    for (unsigned i = 0; i < 16; ++i) {
        const auto division = divisions + i * 0x100;
        c.put32(table + i * 4, i == 4 && !test.conference ? 0 : division);
        c.put32(division, c.toc(-0x6020)); c.put32(division + 4, definitions + i * 16);
        c.put32(division + 0xb0, rows);
        c.put32(definitions + i * 16, i);
        c.put16(division + 0x3e, 4); c.put16(division + 0xe2, 10000); c.put16(division + 0xe4, 9999);
    }
    c.put32(object, c.toc(-0x6020)); c.put32(object + 4, definitions);
    c.put32(object + 0xbc, test.have_schedule ? data + 0xc000 : 0);
    c.put32(object + 8, test.have_fixtures ? data + 0xd000 : 0);
    c.put16(object + 0x40, test.year);
    for (unsigned i = 0; i < test.club_total; ++i) {
        const auto club = clubs + i * 0x248;
        c.put32(club, i);
        // Premier members followed by clubs from three distinct lower leagues.
        const auto id = i < test.premier_total ? 0u : i < test.premier_total + test.nonleague_total ? 4u : 6u + i % test.selected_leagues;
        c.put32(club + 0x30, definitions + id * 16); c.put32(club + 0x2c, data + 0xe000);
        c.put32(data + 0xe068, 99);
        if (test.varied_club_inputs && i >= 20 && i <= 28) {
            if (i == 20) c.put32(club + 0x30, 0);
            else if (i == 21) c.put32(club + 0x2c, 0);
            else if (i == 22) {
                c.put32(club + 0x2c, data + 0xe100); c.put32(data + 0xe168, 98);
            } else {
                constexpr std::array<unsigned, 6> excluded_ids{5, 1, 2, 3, 4, 0};
                c.put32(club + 0x30, definitions + excluded_ids[i - 23] * 16);
            }
        }
    }
    for (unsigned i = 0; i < 64; ++i) {
        c.put32(rows + i * 0x44, clubs + i % std::max(test.club_total, 1u) * 0x248);
        c.put8(rows + i * 0x44 + 0x40, i < test.relegations ? 3 : 0);
    }
    c.set({{3, object}, {4, std::uint32_t(test.deleting ? 1 : test.stage)},
           {5, data + 0xf000}, {6, data + 0xf004}, {7, data + 0xf008}});
    if (test.entry == 0x10529840) c.set({{4, std::uint32_t(test.year)}, {5, definitions}});
    if (test.null_object) c.set(3, 0);
    if (!test.output_flag) c.set(7, 0);
    c.s.budget = 20000000;
}

void postconditions(State& s, std::uint32_t entry) {
    Cpu c(s);
    if (c.sp() != 0x71010000 || c.lr() != sentinel) throw std::runtime_error("fixture did not return");
    if (entry == 0x10529c20 && active.stage == -1 && active.fail_allocation != 1) {
        if (c.u16(data + 0xf000) != 38 || c.u16(data + 0xf004) != 0 ||
            (active.output_flag && c.u8(data + 0xf008) != 1))
            throw std::runtime_error("calendar output fields");
        if (c.r(3) == 0) throw std::runtime_error("calendar did not return schedule");
    }
}
void original(State& s, std::uint32_t entry) {
    reference_mode = true; s.pc = entry; run_until(s, sentinel); postconditions(s, entry);
}
void rewrite(State& s, std::uint32_t entry) {
    reference_mode = false; eng_prm_run_rewrite(s, entry); postconditions(s, entry);
}
}

int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-eng-prm-checks <original-segments>\n"; return 2; }
    try {
        auto base = std::make_unique<State>(); load_program(*base, argv[1]);
        dispatch_probe = probe;
        std::vector<Scenario> cases;
        for (const auto entry : {0x10529840u, 0x10529b80u, 0x10529c20u, 0x1052bbc0u, 0x1052bd60u,
                                  0x1052be10u, 0x1052c190u, 0x1052c420u, 0x1052c770u}) {
            Scenario test{entry, "default"}; cases.push_back(test);
            for (int allocation = 1; allocation <= 3; ++allocation) {
                test.fail_allocation = allocation; test.name = "allocation " + std::to_string(allocation); cases.push_back(test);
            }
        }
        for (int day = 10; day <= 21; ++day) {
            Scenario test{0x10529c20, "calendar day " + std::to_string(day)}; test.day = day; cases.push_back(test);
        }
        for (int stage : {-128, -2, 0, 1, 127, 255}) {
            Scenario test{0x10529c20, "stage " + std::to_string(stage)}; test.stage = stage; cases.push_back(test);
        }
        for (const auto entry : {0x10529840u, 0x1052bbc0u, 0x1052be10u}) {
            for (unsigned count : {0u, 19u, 21u}) {
                Scenario test{entry, "premier count " + std::to_string(count)}; test.premier_total = count; cases.push_back(test);
            }
            for (int year : {1999, 2000, 32767}) { Scenario test{entry, "year " + std::to_string(year)}; test.year = year; cases.push_back(test); }
        }
        for (const auto entry : {0x1052c190u, 0x1052c420u, 0x1052c770u, 0x1052be10u}) {
            for (unsigned eligible : {0u, 0x100u, 0xffu}) {
                Scenario test{entry, "eligible " + std::to_string(eligible)}; test.eligible = eligible; cases.push_back(test);
            }
            for (unsigned count : {0u, 1u, 3u, 8u}) {
                Scenario test{entry, "lower clubs " + std::to_string(count)};
                test.club_total = 20 + count; test.nonleague_total = entry == 0x1052c190 ? count : 0; cases.push_back(test);
            }
            Scenario test{entry, "without conference"}; test.conference = false;
            if (entry != 0x1052c420) cases.push_back(test);
        }
        for (int offset : {0x20, 0x94}) {
            Scenario test{0x10529840, "virtual failure " + std::to_string(offset)}; test.fail_virtual = offset; cases.push_back(test);
        }
        Scenario reset_failure{0x1052be10, "reset configuration failure"};
        reset_failure.fail_virtual = 0x94; cases.push_back(reset_failure);
        for (int target : {0x104a00a0, 0x1049eb30}) {
            for (auto entry : {0x10529840u, 0x1052be10u}) {
                Scenario test{entry, "dependency failure " + hex_address(target)}; test.fail_call = target; cases.push_back(test);
            }
        }
        Scenario deleting{0x10529b80, "deleting destructor"}; deleting.deleting = true; cases.push_back(deleting);
        for (bool schedule : {false, true}) for (bool fixtures : {false, true}) {
            Scenario test{0x10529b80, "destructor optional storage"};
            test.have_schedule = schedule; test.have_fixtures = fixtures; cases.push_back(test);
        }
        Scenario null{0x10529b80, "null destructor"}; null.null_object = true; cases.push_back(null);
        Scenario no_flag{0x10529c20, "no optional output"}; no_flag.output_flag = false; cases.push_back(no_flag);
        for (auto entry : {0x10529c20u, 0x1052bd60u}) for (int year = 1998; year <= 2012; ++year) {
            Scenario test{entry, "real calendar dependencies " + std::to_string(year)};
            test.live_dependencies = true; test.year = year; cases.push_back(test);
        }
        for (unsigned remembered = 0; remembered <= 4; ++remembered)
            for (unsigned leagues : {1u, 2u, 3u}) {
                Scenario test{0x1052c420, "remembered " + std::to_string(remembered) + " leagues " + std::to_string(leagues)};
                test.club_total = 32; test.nonleague_total = 0; test.remembered = remembered;
                test.selected_leagues = leagues; cases.push_back(test);
            }
        for (auto entry : {0x1052c190u, 0x1052c420u}) for (unsigned marked : {0u, 1u, 4u}) {
            Scenario test{entry, "relegation markers " + std::to_string(marked)};
            test.club_total = 32; test.relegations = marked; cases.push_back(test);
        }
        Scenario varied{0x1052c420, "all club eligibility filters"};
        varied.club_total = 40; varied.nonleague_total = 0; varied.varied_club_inputs = true; cases.push_back(varied);
        Scenario empty{0x1052bbc0, "empty club database"}; empty.club_total = 0; cases.push_back(empty);
        for (int year : {1999, 2000, 2001}) for (unsigned count : {19u, 20u, 21u}) {
            Scenario test{0x1052bbc0, "real club dependencies " + std::to_string(year) + " count " + std::to_string(count)};
            test.live_dependencies = true; test.year = year; test.premier_total = count; cases.push_back(test);
        }
        unsigned failures = 0;
        for (const auto& test : cases) {
            auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state);
            active = test; allocations = calls = 0; next_allocation = data + 0x80000;
            initialize(c, test); state->pc = test.entry;
            const auto differences = compare_routine_call(*state, test.entry, original, rewrite, RoutineChecker::result_r3);
            std::cout << (differences.empty() ? "ok " : "DIFFERS ") << hex_address(test.entry) << " " << test.name << '\n';
            for (const auto& difference : differences) std::cout << "  " << difference << '\n';
            failures += !differences.empty();
        }
        active = cases.front(); allocations = calls = 0; next_allocation = data + 0x80000;
        auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize(c, active);
        const auto negative = compare_routine_call(*state, active.entry, original, rewrite, RoutineChecker::result_r3, true);
        if (negative.empty()) { ++failures; std::cout << "FAIL negative control\n"; }
        else std::cout << "ok corrupted result detected\n";
        std::cout << cases.size() << " comparisons, " << failures << " failed\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
