// Full constructor comparison on captured career records and actual native
// services. No day transcript or scripted game-service results are used.
#include "inter_amer_cup_adapter.hpp"
#include "inter_amer_cup_detail.hpp"
#include "code_fragment_adapter.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include "file_reference_bridge.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>
#include <sys/wait.h>
#include <unistd.h>

#if defined(CM_INTER_AMER_CUP_COVERAGE)
extern "C" int __llvm_profile_write_file(void);
#endif
using namespace cm::aot;
using guest::Cpu;
namespace {
using cm::semantic_io_checks::require;
constexpr std::uint32_t constructor = 0x106ebdd0, sentinel = 0xfffffff0;
std::filesystem::path game, evidence;
std::uint32_t object = 0, definition = 0, year = 0, caller_stack = 0;
bool corrupt_result = false;
std::map<std::filesystem::path, NativeFileReference> common_references;
// FSRefs expose their issuing PID in guest memory. Both runs execute the
// real bridge, then validate and use the same native-equivalent input, as in
// pol_cup_checks.cpp. No guest comparison bytes are excluded.
void prepare_native_references() {
    const auto working = evidence / "working";
    common_references.clear();
    for (const auto& path : {game, game / "Files", working, working / "Files"})
        if (std::filesystem::exists(path))
            common_references.emplace(std::filesystem::canonical(path), NativeFileReference{});
    const auto capture = evidence / "reference-inputs.bin";
    // Do not initialize Carbon in the parent before the comparison forks.
    const auto child = ::fork(); require(child >= 0, "Cannot fork native input preparation");
    if (!child) {
        std::ofstream output(capture, std::ios::binary);
        for (auto& [path, reference] : common_references) {
            if (native_file_reference(path, reference)) ::_exit(1);
            output.write(reinterpret_cast<const char*>(reference.data()), reference.size());
        }
        output.close(); ::_exit(output ? 0 : 1);
    }
    int status = 0;
    require(::waitpid(child, &status, 0) == child && WIFEXITED(status) && WEXITSTATUS(status) == 0,
            "Native input preparation did not return");
    std::ifstream input(capture, std::ios::binary);
    for (auto& [path, reference] : common_references) {
        (void)path; input.read(reinterpret_cast<char*>(reference.data()), reference.size());
        require(bool(input), "Cannot read shared native reference input");
    }
}
using Files = std::map<std::filesystem::path, std::vector<char>>;
Files files(const std::filesystem::path& root) {
    Files result;
    for (const auto& item : std::filesystem::recursive_directory_iterator(root)) {
        require(!item.is_symlink(), "Native constructor IO must stay in its private directory");
        if (!item.is_regular_file()) continue;
        std::ifstream input(item.path(), std::ios::binary);
        require(bool(input), "Cannot read native constructor output");
        result.emplace(std::filesystem::relative(item.path(), root),
                       std::vector<char>(std::istreambuf_iterator<char>(input), {}));
        require(!input.bad(), "Cannot read complete native constructor output");
    }
    return result;
}
struct Imports final : CodeFragmentBoundaryObserver {
    std::ofstream log;
    std::map<std::string, unsigned> counts;
    Imports(State& s, const char* side) : log(evidence / (std::string(side) + "-imports.log")) {
        require(bool(log), "Cannot record native imports"); s.code_fragment_observer = this;
    }
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    bool import_fixture(State& s, const char* name) override {
        const std::string_view import(name);
        unsigned output_register = 0;
        if (import == "CarbonLib::FSpMakeFSRef") output_register = 4;
        else if (import == "CarbonLib::FSMakeFSRefUnicode") output_register = 7;
        else if (import == "CarbonLib::FSGetCatalogInfo") output_register = 8;
        else return false;
        const auto destination = std::uint32_t(s.r<4>(output_register * 4));
        require(s.platform->dispatch(s, name), "Actual native reference service unavailable");
        if (s.r<4>(12) != 0 || !destination) return true;
        NativeFileReference issued{};
        const auto bytes = s.memory.copy_out(destination, issued.size());
        std::copy(bytes.begin(), bytes.end(), issued.begin());
        std::filesystem::path path, shared_path;
        require(native_reference_path(issued, path) == 0, "Cannot resolve issued native reference");
        path = std::filesystem::canonical(path);
        const auto found = common_references.find(path);
        require(found != common_references.end(), "Unprepared native reference input: " + path.string());
        require(native_reference_path(found->second, shared_path) == 0 &&
                std::filesystem::canonical(shared_path) == path &&
                native_compare_references(issued, found->second) == 0,
                "Shared reference does not name the same real native object");
        log << "native-equivalent " << path << '\n';
        s.memory.copy_in(destination, {found->second.begin(), found->second.end()});
        return true;
    }
    void event(const State& s, const char* name, const char* phase) {
        log << phase << ' ' << name << ' ' << std::hex << s.pc;
        for (unsigned r = 3; r <= 10; ++r) log << ' ' << s.r<4>(r * 4);
        log << '\n'; log.flush();
    }
    void import_enter(const State& s, const char* name) override { ++counts[name]; event(s, name, "in"); }
    void import_leave(const State& s, const char* name) override { event(s, name, "out"); }
};
void postconditions(State& s, const Imports& imports, const char* side) {
    Cpu c(s);
    require(s.pc == sentinel && c.lr() == sentinel && c.sp() == caller_stack,
            "Native constructor did not return to its caller");
    require(c.r(3) == object, "Native constructor returned a different object");
    require(c.u32(object) == c.toc(inter_amer_cup::detail::slot::vtable) &&
            c.u32(object + 4) == definition && c.u16(object + 0x40) == year,
            "Native constructor object identity/year");
    require(c.i16(object + 0x3e) == 2 && c.i16(object + 0x3c) == 1,
            "Native constructor entrant/round counts");
    require(c.u32(object + 8) && c.u32(object + 0xa0) && c.u32(object + 0xa4),
            "Native constructor did not complete fixture/calendar/entrant initialization");
    require(c.u8(c.toc(-0x7bac)) != 0, "Native constructor reported an original error");
    require(imports.counts.contains("CarbonLib::HGetVol"), "Native volume query was not exercised");
    std::ofstream output(evidence / (std::string(side) + "-result.txt"));
    output << "returned=1 object=" << std::hex << object << " year=" << std::dec << year
           << " entrants=" << c.i16(object + 0x3e) << " rounds=" << c.i16(object + 0x3c) << '\n';
    const auto entrants = c.u32(object + 0xa4), calendar = c.u32(object + 0xa0);
    for (unsigned i = 0; i < 2; ++i) {
        const auto club = c.u32(entrants + i * 8);
        require(club != 0, "Native constructor selected a null entrant");
        output << "entrant=" << i << " club-id=" << std::dec << c.u32(club)
               << " flags=" << unsigned(c.u8(entrants + i * 8 + 4)) << ','
               << unsigned(c.u8(entrants + i * 8 + 5)) << '\n';
    }
    output << "calendar=" << std::hex << calendar << " fixture=" << c.u32(object + 8) << '\n';
    for (const auto& [name, count] : imports.counts) output << "import " << name << ' ' << std::dec << count << '\n';
    require(bool(output), "Cannot record native constructor postconditions");
}
void run(State& s, std::uint32_t entry, bool modern) {
    const auto working = evidence / "working";
    // Forks isolate guest memory, not the filesystem. This constructor makes
    // no file writes; reject any contamination before the other side starts.
    require(files(working).empty(), "Native constructor backing must start empty for each runner");
    s.platform = std::make_shared<NativePlatform>(game, working, AudioMode::offline, EventMode::isolated);
    const char* side = modern ? "rewrite" : "original";
    Imports imports(s, side);
    if (modern) require(try_modern_inter_amer_cup(s, entry), "Default native constructor hook declined");
    else inter_amer_cup_run_original(s, entry);
    postconditions(s, imports, side);
    std::filesystem::create_directories(evidence / side);
    for (const auto& [relative, bytes] : files(working)) {
        const auto destination = evidence / side / relative;
        std::filesystem::create_directories(destination.parent_path());
        std::ofstream output(destination, std::ios::binary);
        output.write(bytes.data(), std::streamsize(bytes.size()));
        require(bool(output), "Cannot preserve native constructor file effects");
    }
    s.code_fragment_observer = nullptr;
#if defined(CM_INTER_AMER_CUP_COVERAGE)
    if (modern) __llvm_profile_write_file();
#endif
    if (modern && corrupt_result) { Cpu c(s); c.set(3, c.r(3) ^ 1); }
}
void original(State& s, std::uint32_t entry) { run(s, entry, false); }
void rewrite(State& s, std::uint32_t entry) { run(s, entry, true); }
}
int inter_amer_cup_native_checks(const char* snapshot, const char* game_directory, const char* backing) {
    try {
        corrupt_result = false;
        game = std::filesystem::canonical(game_directory);
        const auto root = std::filesystem::absolute(backing) /
            ("constructor-" + std::to_string(std::chrono::steady_clock::now().time_since_epoch().count()));
        evidence = root / "complete";
        std::filesystem::create_directories(evidence / "working/Files");
        prepare_native_references();
        auto state = std::make_unique<State>(); load_match_snapshot(*state, snapshot); Cpu c(*state);
        require(dispatch_probe == nullptr && routine_checker == nullptr, "Native constructor must not use boundary fixtures");
        const auto vtable = c.toc(inter_amer_cup::detail::slot::vtable);
        const auto table = c.u32(c.toc(-0x7b5c)), count = c.u32(c.toc(-0x7b60));
        std::uint32_t registered = 0;
        for (std::uint32_t id = 0; id < count; ++id) {
            const auto competition = c.u32(table + id * 4);
            if (competition && c.u32(competition) == vtable && c.u32(c.u32(competition + 4)) == id) {
                require(!registered, "Multiple registered Inter-American Cups"); registered = competition;
            }
        }
        require(registered != 0, "Snapshot has no registered Inter-American Cup");
        definition = c.u32(registered + 4); year = c.u16(registered + 0x40); caller_stack = c.sp();
        state->budget = state->executed + 1000000000;
        c.set_lr(sentinel); c.set(3, 0xb4); c.call(0x10773520, sentinel);
        object = c.r(3); require(object != 0, "Cannot allocate native constructor object");
        // The day snapshot's shared assertion flag is already zero. Start a
        // fresh call with a set canary so any constructor assertion must fail.
        c.put8(c.toc(-0x7bac), 1);
        c.set({{3, object}, {4, year}, {5, definition}}); c.set_lr(sentinel);
        const auto differences = compare_routine_call(*state, constructor, original, rewrite,
            RoutineChecker::result_r3 | RoutineChecker::result_f1);
        for (const auto& difference : differences) std::cout << "  " << difference << '\n';
        const bool file_match = std::filesystem::is_directory(evidence / "original") &&
            std::filesystem::is_directory(evidence / "rewrite") && files(evidence / "original") == files(evidence / "rewrite");
        const auto trace = [&](const char* side) {
            std::ifstream input(evidence / (std::string(side) + "-imports.log"));
            require(bool(input), "Cannot read actual native import trace");
            return std::string(std::istreambuf_iterator<char>(input), {});
        };
        require(differences.empty() && file_match && trace("original") == trace("rewrite"),
                "Native constructor result, file effects or import sequence differs");
        std::cout << "ok native constructor returned comparison; evidence=" << evidence << '\n';

        // Re-run both complete constructors, then spoil only the C++ result.
        // Faults, early exits or unrelated differences cannot pass this check.
        evidence = root / "wrong-result";
        std::filesystem::create_directories(evidence / "working/Files");
        prepare_native_references(); corrupt_result = true;
        const auto negative = compare_routine_call(*state, constructor, original, rewrite,
            RoutineChecker::result_r3 | RoutineChecker::result_f1);
        require(negative.size() == 1 && negative.front().starts_with("r3 "),
                "Native constructor wrong-result control was not detected exactly");
        require(files(evidence / "original") == files(evidence / "rewrite") &&
                trace("original") == trace("rewrite"), "Negative control changed native effects");
        std::cout << "ok native constructor wrong-result control detected after complete returns\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 2; }
}
