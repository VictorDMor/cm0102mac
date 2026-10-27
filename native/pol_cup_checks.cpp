// Complete returned comparisons against private, hook-free original pages.
// Dependency fixtures record call order, return sites, arguments and preserved
// registers in compared guest memory. Calendar fixtures run real date helpers;
// optional career-snapshot cases run every dependency from real guest records.
#include "pol_cup_adapter.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include "code_fragment_adapter.hpp"
#include "actor_attributes_io_checks.hpp"
#include "pol_cup_detail.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <array>
#include <chrono>
#include <fstream>
#include <map>
#include <iostream>
#include <limits>
#include <set>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

#if defined(CM_POL_CUP_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
void flush_coverage() {
#if defined(CM_POL_CUP_COVERAGE)
    __llvm_profile_write_file();
#endif
}

using namespace cm::aot;
using guest::Cpu;
namespace cm::aot { bool pol_cup_reference(State&); }
namespace {
constexpr std::uint32_t data = 0x60000000, object = data + 0x1000,
    clubs = data + 0x10000, definitions = data + 0x2000,
    log_at = data + 0x40000, sentinel = 0xfffffff0;
struct Scenario {
    std::uint32_t entry;
    std::string name;
    unsigned first = 16, second = 20, lower = 16, excluded = 0;
    int stage = -1, year = 2001, fail_allocation = 0, fail_virtual = -1;
    unsigned storage = 7;
    int virtual_failure = -1;
    int round_count = 3, deleting = 0;
    bool null_object = false, output_flag = true, mixed = false, live_calendar = false;
};
Scenario active{};
unsigned allocations = 0, calls = 0;
std::uint32_t next_allocation = 0;
bool reference_mode = false;
const std::set<std::uint32_t> dependencies{
    0x104533a0,0x104536c0,0x104564a0,0x104635b0,0x10478f20,0x10478f70,
    0x104854f0,0x10485570,0x10739890,0x10768810,0x10769b60,0x10773520,
    0x107735a0,0x10775130,0x10775170,0x107755c0,0x1077b720};
void trace(Cpu& c, std::uint32_t target) {
    if (++calls > 2000) throw std::runtime_error("fixture call limit");
    auto index = c.u32(log_at);
    const auto add = [&](std::uint32_t word) { c.put32(log_at + 4 + index++ * 4, word); };
    add(target); add(c.lr());
    for (unsigned r = 3; r < 11; ++r) add(c.r(r));
    for (unsigned r = 13; r < 32; ++r) add(c.r(r));
    for (unsigned field = 0; field < 8; ++field) add(c.cr(field));
    if (target == 0x10485570) add(c.u32(c.sp() + 0x38));
    c.put32(log_at, index);
}
bool probe(State& s) {
    Cpu c(s); const auto target = s.pc;
    // Cpu::call executes lowered leaves before the dispatcher. Use that same
    // real leaf path on the isolated original side before fixture interception.
    if (run_leaf(s)) return true;
    if (target == 0x10781320) {
        const auto descriptor = c.r(12), entry = c.u32(descriptor);
        c.put32(c.sp() + 0x14, c.r(2));
        const unsigned method = entry == 0x106cdbe0 ? 0x44 : entry == 0x106ce7c0 ? 0x94 :
                                entry == 0x10479000 ? 0x20 : entry == 0x10479010 ? 0x9c : 0x64;
        if (active.fail_virtual != int(method) && pol_cup_lists(entry)) {
            c.set(2, c.u32(descriptor + 4)); s.pc = entry; return true;
        }
        trace(c, entry);
        c.set(3, active.fail_virtual == int(method) ? (method == 0x20 ? 1 : std::uint32_t(active.virtual_failure))
                                                  : method == 0x20 ? 0 : 1);
        s.pc = c.lr(); return true;
    }
    if (pol_cup_lists(target)) {
        if (reference_mode) return pol_cup_reference(s);
        pol_cup_run_rewrite(s, target); return true;
    }
    // The generated reference handles interior PCs without production hooks.
    if (reference_mode && target >= 0x106cd810 && target < 0x106ce820) return pol_cup_reference(s);
    if (active.live_calendar && (target == 0x104854f0 || target == 0x10485570)) return false;
    if (!dependencies.contains(target)) return false;
    trace(c, target);
    if (target == 0x10775130 || target == 0x10773520) {
        ++allocations;
        if (active.fail_allocation == int(allocations)) c.set(3, 0);
        else {
            const auto size = c.r(3), allocated = next_allocation;
            next_allocation += (size + 31) & ~15u; c.set(3, allocated);
        }
    } else if (target == 0x10739890) {
        c.put32(c.r(6), 0x504f4c00); c.put32(c.r(7), 0x43505000);
    } else if (target == 0x1077b720) c.put32(c.r(3), c.r(8));
    else if (target == 0x10478f20) {
        c.put32(c.r(3) + 8, 0); c.put32(c.r(3) + 0xa0, 0);
        c.put32(c.r(3) + 0xa4, 0); c.put32(c.r(3) + 0xac, 0);
    } else if (target != 0x104635b0 && target != 0x104854f0 && target != 0x10485570) c.set(3, 1);
    s.pc = c.lr(); return true;
}
void initialize(Cpu& c, const Scenario& test) {
    c.s.memory.map(data, 0x200000); c.s.memory.map(0x71000000, 0x20000);
    c.s.memory.copy_in(data, std::vector<std::uint8_t>(0x200000, 0xa5));
    c.s.memory.copy_in(0x71000000, std::vector<std::uint8_t>(0x20000, 0xa5));
    c.put32(log_at, 0);
    for (unsigned r = 0; r < 32; ++r) c.set(r, 0x51000000 + r * 0x101);
    c.set(1, 0x71010000); c.set(2, 0x108bba90); c.set_lr(sentinel);
    for (unsigned field = 0; field < 8; ++field) c.set_cr(field, field + 1);
    const auto variable = [&](int slot, unsigned offset, std::uint32_t value) {
        c.put32(c.r(2) + std::uint32_t(slot), data + offset); c.put32(data + offset, value);
    };
    const auto total = test.first + test.second + test.lower;
    variable(-0x7b50, 0x100, total + 2 * test.excluded);
    variable(-0x7b04, 0x104, test.excluded); variable(-0x7be8, 0x108, clubs);
    variable(-0x7b58, 0x10c, 0); variable(-0x69fc, 0x110, 1); variable(-0x6a08, 0x114, 2);
    c.put32(c.r(2) - 0x7ba8, data + 0x8000); c.put32(c.r(2) - 0x7bac, data + 0x9000);
    // A real vtable shape and real module TVs; unused base methods are fixture
    // descriptors, preserving virtual dispatch and the saved TOC on both sides.
    const auto vtable = data + 0x3000, descriptors = data + 0x4000;
    c.put32(c.r(2) - 0x5d0c, vtable); c.put32(object, vtable);
    unsigned descriptor_index = 0;
    for (auto [method, entry] : std::array<std::pair<unsigned, unsigned>,5>{{
        {0x20,0x10479000},{0x44,0x106cdbe0},{0x64,0x10479020},{0x94,0x106ce7c0},{0x9c,0x10479010}}}) {
        const auto descriptor = descriptors + descriptor_index++ * 8;
        c.put32(vtable + method, descriptor); c.put32(descriptor, entry); c.put32(descriptor + 4, c.r(2));
    }
    for (unsigned i = 0; i < 4; ++i) c.put32(definitions + i * 16, i);
    c.put32(object + 4, definitions); c.put16(object + 0x40, test.year);
    c.put32(object + 8, test.storage & 1 ? data + 0xa000 : 0);
    c.put32(object + 0xa0, test.storage & 2 ? data + 0xb000 : 0);
    c.put32(object + 0xa4, test.storage & 4 ? data + 0xc000 : 0);
    c.put32(object + 0xac, test.storage & 8 ? data + 0xd000 : 0);
    c.put16(object + 0x3c, test.round_count);
    for (int i = 0; i < test.round_count; ++i) c.put32(data + 0xd000 + i * 4, i % 2 ? 0 : data + 0xe000 + i * 32);
    for (unsigned i = 0; i < total + 2 * test.excluded; ++i) {
        const auto club = clubs + i * 0x248;
        const auto division = i < test.first ? 0u : i < test.first + test.second ? 1u : 2u;
        c.put32(club, i); c.put32(club + 0x34, definitions + division * 16);
        c.put32(club + 0x30, definitions + (division == 2 && i % 2 ? 2 : 3) * 16);
        c.put8(club + 0x23c, division == 1 ? i - test.first + 1 : 0);
        if (test.mixed) {
            if (i % 17 == 0) c.put32(club + 0x34, 0);
            if (i % 17 == 1) c.put32(club + 0x30, 0);
            if (i % 17 == 2) c.put32(club + 0x34, definitions + 48);
            if (i % 17 == 3) c.put8(club + 0x23c, 0xff);
            if (i % 17 == 4) c.put8(club + 0x23c, 21);
        }
    }
    c.set({{3, test.null_object ? 0 : object}, {4, std::uint32_t(test.stage)},
           {5, data + 0xf000}, {6, data + 0xf004}, {7, test.output_flag ? data + 0xf008 : 0}});
    if (test.entry == 0x106cd810) c.set({{4, std::uint32_t(test.year)}, {5, definitions}});
    if (test.entry == 0x106cdaf0) c.set(4, test.deleting);
    c.s.budget = 20000000;
}
void postconditions(State& s, std::uint32_t entry) {
    Cpu c(s);
    if (c.sp() != 0x71010000 || c.lr() != sentinel) throw std::runtime_error("fixture did not return");
    if (entry == 0x106cdbe0 && std::int8_t(active.stage) == -1) {
        if (c.u16(data + 0xf000) != 7 || c.u16(data + 0xf004) != 0 ||
            (active.output_flag && c.u8(data + 0xf008) != 0)) throw std::runtime_error("calendar output fields");
        if (active.fail_allocation != 1) {
            const auto schedule = c.r(3);
            for (unsigned i = 0; i < 7; ++i)
                if (c.u16(schedule + i * 0x78 + 10) != i || c.u8(schedule + i * 0x78 + 34) != 0)
                    throw std::runtime_error("round postcondition");
            if (c.u32(schedule + 0x340) != 500000 || c.u8(schedule + 0x347) != 0)
                throw std::runtime_error("final prize/padding postcondition");
        }
    }
    if (entry == 0x106ce7c0 && c.r(3) != 1) throw std::runtime_error("configure result");
    if (entry == 0x106ce180 && active.fail_allocation != 1 && active.first == 16 && active.second == 20 &&
        active.lower == 16 && !active.mixed) {
        const auto entrants = c.u32(object + 0xa4);
        if (c.u32(entrants) != clubs + (active.first + 12) * 0x248 || c.u8(entrants + 4) != 2 ||
            c.u8(entrants + 36 * 8 + 4) != 1 || c.u8(entrants + 6) != 0xa5)
            throw std::runtime_error("selection order/class/padding postcondition");
    }
}
void original(State& s, std::uint32_t entry) {
    reference_mode = true; s.pc = entry; run_until(s, sentinel); postconditions(s, entry);
}
void rewrite(State& s, std::uint32_t entry) {
    reference_mode = false; pol_cup_run_rewrite(s, entry); postconditions(s, entry); flush_coverage();
}
}
std::filesystem::path live_game, live_backing;
// Carbon's opaque FSRef contains the issuing process ID. Mint the common
// native inputs in a separate process: initializing Carbon in the parent would
// make subsequent forked native calls unsafe. Both runners still execute the
// ordinary bridge, then validate the issued identity against this shared input.
std::map<std::filesystem::path, NativeFileReference> live_references;
void prepare_live_inputs() {
    using cm::semantic_io_checks::require;
    const auto working = live_backing / "working";
    std::filesystem::create_directories(working / "Files");
    std::ofstream(working / "Files/comp records history.tmp", std::ios::binary).close();
    live_references.clear();
    for (const auto& path : {live_game, live_game / "Files", working, working / "Files",
                             working / "Files/comp records history.tmp"})
        if (std::filesystem::exists(path)) live_references.emplace(std::filesystem::canonical(path), NativeFileReference{});
    const auto capture = live_backing / "native-reference-inputs.bin";
    const auto child = ::fork();
    require(child >= 0, "Cannot fork native reference preparation");
    if (!child) {
        std::ofstream output(capture, std::ios::binary);
        for (auto& [path, reference] : live_references) {
            if (native_file_reference(path, reference)) ::_exit(1);
            output.write(reinterpret_cast<const char*>(reference.data()), reference.size());
        }
        output.close(); ::_exit(output ? 0 : 1);
    }
    int status = 0;
    require(::waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
            "Native reference preparation did not return");
    std::ifstream input(capture, std::ios::binary);
    for (auto& [path, reference] : live_references) {
        (void)path;
        input.read(reinterpret_cast<char*>(reference.data()), reference.size());
        require(bool(input), "Cannot read shared native reference input");
    }
}
void snapshot_outputs(const char* side);
void live_platform(State& s, const char* side) {
    (void)side;
    if (!live_game.empty()) {
        // The same native directory/file identities must be presented to both
        // children: FSRefs expose their host identities in guest stack bytes.
        const auto history = live_backing / "working/Files/comp records history.tmp";
        std::ofstream reset(history, std::ios::binary | std::ios::trunc);
        if (!reset) throw std::runtime_error("cannot reset private cup history");
        reset.close();
        s.platform = std::make_shared<NativePlatform>(live_game, live_backing / "working",
                                                      AudioMode::offline, EventMode::isolated);
    }
}
struct ImportTrace : CodeFragmentBoundaryObserver {
    std::ofstream output;
    cm::semantic_io_checks::ActorAttributesForkInputs forks;
    ImportTrace(State& s, const char* side) : output(live_backing / (std::string(side) + "-imports.log")) { s.code_fragment_observer = this; }
    bool import_fixture(State& s, const char* name) override {
        if (live_game.empty()) return false;
        if (forks.dispatch(s, name, live_backing / "working")) return true;
        const std::string_view import(name);
        unsigned output_register = 0;
        if (import == "CarbonLib::FSpMakeFSRef") output_register = 4;
        else if (import == "CarbonLib::FSMakeFSRefUnicode") output_register = 7;
        else if (import == "CarbonLib::FSGetCatalogInfo") output_register = 8;
        else return false;
        const auto destination = std::uint32_t(s.r<4>(output_register * 4));
        if (!s.platform->dispatch(s, name)) throw std::runtime_error("Native reference bridge missing");
        if (s.r<4>(12) != 0 || !destination) return true;
        NativeFileReference issued{};
        const auto bytes = s.memory.copy_out(destination, issued.size());
        std::copy(bytes.begin(), bytes.end(), issued.begin());
        std::filesystem::path path;
        cm::semantic_io_checks::require(native_reference_path(issued, path) == 0, "Cannot resolve issued native reference");
        path = std::filesystem::canonical(path);
        const auto common = live_references.find(path);
        cm::semantic_io_checks::require(common != live_references.end(), "Unprepared native reference input: " + path.string());
        std::filesystem::path shared_path;
        cm::semantic_io_checks::require(native_reference_path(common->second, shared_path) == 0 &&
            std::filesystem::canonical(shared_path) == path && native_compare_references(issued, common->second) == 0,
            "Shared native reference has a different native identity");
        output << "native-reference " << std::hex << destination << ' ' << path << " native-equivalent\n";
        s.memory.copy_in(destination, {common->second.begin(), common->second.end()});
        return true;
    }
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void event(const State& s, const char* name, const char* phase) {
        output << phase << ' ' << name << ' ' << std::hex << s.pc;
        for (unsigned r = 3; r < 11; ++r) output << ' ' << s.r<4>(r * 4);
        output << '\n';
    }
    void import_enter(const State& s, const char* name) override { event(s, name, "in"); }
    void import_leave(const State& s, const char* name) override { event(s, name, "out"); }
};
void live_original(State& s, std::uint32_t entry) { live_platform(s, "original"); ImportTrace trace(s, "original"); pol_cup_run_original(s, entry); snapshot_outputs("original"); }
void live_rewrite(State& s, std::uint32_t entry) { live_platform(s, "rewrite"); ImportTrace trace(s, "rewrite"); pol_cup_run_rewrite(s, entry); snapshot_outputs("rewrite"); flush_coverage(); }
std::map<std::filesystem::path, std::vector<char>> files(const std::filesystem::path& root) {
    std::map<std::filesystem::path, std::vector<char>> result;
    if (std::filesystem::exists(root)) for (const auto& file : std::filesystem::recursive_directory_iterator(root)) {
        if (!file.is_regular_file()) continue;
        std::ifstream input(file.path(), std::ios::binary);
        result.emplace(std::filesystem::relative(file.path(), root), std::vector<char>(std::istreambuf_iterator<char>(input), {}));
    }
    return result;
}
void snapshot_outputs(const char* side) {
    if (live_game.empty()) return;
    for (const auto& [relative, bytes] : files(live_backing / "working")) {
        const auto destination = live_backing / side / relative;
        std::filesystem::create_directories(destination.parent_path());
        std::ofstream output(destination, std::ios::binary);
        output.write(bytes.data(), std::streamsize(bytes.size()));
        if (!output) throw std::runtime_error("cannot preserve private cup output");
    }
}

int main(int argc, char** argv) {
    if (argc != 2 && argc != 5) { std::cerr << "Usage: cm-pol-cup-checks <original-segments> [career.snapshot game-directory private-backing]\n"; return 2; }
    try {
        auto base = std::make_unique<State>(); load_program(*base, argv[1]); dispatch_probe = probe;
        std::vector<Scenario> cases;
        for (auto entry : {0x106cd810u,0x106cdaf0u,0x106cdbe0u,0x106ce180u,0x106ce570u,0x106ce7c0u}) {
            cases.push_back({entry, "default"});
            for (int failure = 1; failure <= 3; ++failure) {
                Scenario test{entry, "allocation " + std::to_string(failure)};
                test.fail_allocation = failure; cases.push_back(test);
            }
        }
        for (auto entry : {0x106cd810u,0x106ce570u}) for (int method : {0x20,0x94,0x9c}) {
            Scenario test{entry, "virtual failure " + std::to_string(method)};
            test.fail_virtual = method; cases.push_back(test);
            test.virtual_failure = 0; test.name += " zero"; cases.push_back(test);
        }
        for (int stage : {-128,-2,0,1,127,255,511}) {
            Scenario test{0x106cdbe0, "stage " + std::to_string(stage)}; test.stage = stage; cases.push_back(test);
        }
        for (int year : {1998,1999,2000,2001,2004,2099,2100,32767,-32768}) {
            Scenario test{0x106cdbe0, "live calendar " + std::to_string(year)};
            test.year = year; test.live_calendar = true; cases.push_back(test);
        }
        for (int year : {32767,-32768,65535}) {
            Scenario test{0x106ce570, "year wrap " + std::to_string(year)};
            test.year = year; cases.push_back(test);
        }
        Scenario no_flag{0x106cdbe0, "no optional output"}; no_flag.output_flag = false; cases.push_back(no_flag);
        for (auto entry : {0x106cdaf0u,0x106ce570u}) for (unsigned storage = 0; storage < 16; ++storage)
            for (int rounds : {-1,0,1,7}) {
                Scenario test{entry, "storage " + std::to_string(storage) + " rounds " + std::to_string(rounds)};
                test.storage = storage; test.round_count = rounds; cases.push_back(test);
            }
        for (int deleting : {-1,1,65535,65537}) {
            Scenario test{0x106cdaf0, "delete " + std::to_string(deleting)}; test.deleting = deleting; cases.push_back(test);
        }
        Scenario null{0x106cdaf0, "null destructor"}; null.null_object = true; cases.push_back(null);
        for (unsigned lower : {0u,1u,35u,70u,71u,100u}) for (unsigned second : {0u,1u,12u,13u,20u,21u}) {
            Scenario test{0x106ce180, "selection lower " + std::to_string(lower) + " second " + std::to_string(second)};
            test.lower = lower; test.second = second; cases.push_back(test);
            test.mixed = true; test.name += " filters"; cases.push_back(test);
        }
        Scenario empty{0x106ce180, "empty database"}; empty.first = empty.second = empty.lower = 0; cases.push_back(empty);
        Scenario excluded{0x106ce180, "excluded database tail"}; excluded.excluded = 4; cases.push_back(excluded);
        unsigned failures = 0;
        if (argc >= 3) {
            if (argc == 5) live_game = std::filesystem::canonical(argv[3]);
            auto career = std::make_unique<State>(); load_match_snapshot(*career, argv[2]);
            career->budget = std::numeric_limits<std::uint64_t>::max();
            Cpu saved(*career); const auto vtable = saved.toc(pol_cup::detail::slot::vtable);
            std::uint32_t cup = 0;
            const auto table = saved.u32(saved.toc(-0x7b5c));
            const auto competition_count = saved.u32(saved.toc(-0x7b60));
            for (std::uint32_t id = 0; id < competition_count; ++id) {
                const auto competition = saved.u32(table + id * 4);
                if (competition && saved.u32(competition) == vtable &&
                    saved.u32(saved.u32(competition + 4)) == id) {
                    if (cup) throw std::runtime_error("multiple registered Polish Cups");
                    cup = competition;
                }
            }
            if (!cup) throw std::runtime_error("snapshot contains no registered Polish Cup object");
            dispatch_probe = nullptr;
            for (auto entry : {0x106cd810u,0x106cdaf0u,0x106cdbe0u,0x106ce180u,0x106ce570u,0x106ce7c0u}) {
                if (argc == 5) live_backing = std::filesystem::weakly_canonical(std::filesystem::absolute(argv[4])) /
                    (std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()) + "-" + hex_address(entry));
                if (argc == 5) prepare_live_inputs();
                auto state = cm::semantic_io_checks::clone(*career); Cpu c(*state);
                c.set_lr(sentinel); c.set({{3, cup}, {4, 0}, {5, c.sp() - 0x2000},
                                          {6, c.sp() - 0x2004}, {7, c.sp() - 0x2008}});
                if (entry == 0x106cdbe0) c.set(4, 0xffffffff);
                if (entry == 0x106cd810) {
                    const auto year = c.u16(cup + 0x40);
                    const auto definition = c.u32(cup + 4);
                    c.set(3, 0xb4); c.call(0x10773520, sentinel);
                    c.set({{4, year}, {5, definition}});
                }
                state->budget = std::numeric_limits<std::uint64_t>::max();
                const auto differences = compare_routine_call(*state, entry, live_original, live_rewrite,
                    entry == 0x106ce570 ? 0 : RoutineChecker::result_r3);
                std::cout << (differences.empty() ? "ok " : "DIFFERS ") << hex_address(entry) << " real career dependencies\n";
                for (const auto& difference : differences) std::cout << "  " << difference << '\n';
                failures += !differences.empty();
                if (argc == 5 && files(live_backing / "original") != files(live_backing / "rewrite")) {
                    ++failures; std::cout << "DIFFERS native backing files\n";
                }
            }
            dispatch_probe = probe;
        }
        for (const auto& test : cases) {
            auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state);
            active = test; allocations = calls = 0; next_allocation = data + 0x80000;
            initialize(c, test); state->pc = test.entry;
            const auto differences = compare_routine_call(*state, test.entry, original, rewrite,
                test.entry == 0x106ce570 ? 0 : RoutineChecker::result_r3);
            std::cout << (differences.empty() ? "ok " : "DIFFERS ") << hex_address(test.entry) << " " << test.name << '\n';
            for (const auto& difference : differences) std::cout << "  " << difference << '\n';
            failures += !differences.empty();
        }
        active = cases.front(); allocations = calls = 0; next_allocation = data + 0x80000;
        auto state = cm::semantic_io_checks::clone(*base); Cpu c(*state); initialize(c, active);
        const auto negative = compare_routine_call(*state, active.entry, original, rewrite, RoutineChecker::result_r3, true);
        if (negative.empty()) { ++failures; std::cout << "FAIL negative control\n"; }
        else std::cout << "ok corrupted result detected\n";
        std::cout << cases.size() << " fixture comparisons, " << (argc == 5 ? 6 : 0) << " career-state comparisons, " << failures << " failed\n";
        return failures ? 1 : 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
