// Replays a captured world operation (a recruitment pass, a daily transition)
// from its initial snapshot to its captured end, checks the end state against
// the capture, and prints one JSON line per run. Used for world-processing
// benchmarks and for checking rewrites against the generated reference.
//
// cm-world-replay <capture> [runs] [--write-expected <path>] [--flip <address>]
//                 [--profile <samples>] [--timing]
//   <capture>         directory with initial.snapshot, expected.snapshot and,
//                     when the operation calls the OS, imports.gz; a season
//                     capture (capture.txt "days=N", recorded by the app with
//                     CM_AOT_SEASON_REFERENCE) runs to its final day boundary
//   --write-expected  save the first run's end state instead of checking it,
//                     to re-reference a capture after a deliberate change to
//                     the reference
//   --flip            invert one byte of guest memory before the run and put
//                     it back afterwards, so only its effect on the run can
//                     fail the end-state check (shows the check fails when it
//                     should)
//   --timing          skip the recorded RNG state writes instead of checking
//                     each (checking needs a memory observer, which switches
//                     C++ owners to their observed paths); the end state is
//                     still checked
//   --profile         sample the guest PC and its return addresses every
//                     millisecond of CPU time, over all runs, into one file;
//                     tools/guest_profile.py charges them to original routines
//   --check-club-control
//                     compare club_control::human_controlled with the original
//                     0x1014c2e0 for every club of the initial snapshot and
//                     several arguments, then exit; --check-club-control-shifted
//                     asks the C++ version about the next club instead, which
//                     must fail (shows the comparison can fail)
//   --scramble-stack <seed>
//                     with --stack-audit: overwrite every freed frame with
//                     pseudo-random bytes, and list every end-state byte that
//                     differs instead of stopping at the first
//   --check-routines <first> [<every>]
//                     run the C++ tactics, MAIN.CPP, PLAYER_STATS.CPP and
//                     SHORTLIST_MANAGER.CPP routines (tactics_routines.inc,
//                     main_program_routines.inc, player_stats_routines.inc,
//                     shortlist_manager_routines.inc),
//                     also those the generator has not hooked yet, and compare
//                     each with its generated original at its first <first>
//                     calls and then one in <every> (routine_check.hpp); stops
//                     at the first difference; 0 runs them unchecked
//   --only <entry>[,<entry>...]
//                     with --check-routines: run and check only these C++
//                     routines (others the generator has not hooked keep
//                     their generated originals)
//   --check-routines-corrupted <first> [<every>]
//                     as above, but spoils one byte of the first checked
//                     call's rewrite result, which must fail
//   --strict-volatile with --check-routines: also compare r0, r3, r11, r12,
//                     XER and CTR at return (r4-r10 and CR always are), to
//                     locate a leftover that later reaches memory
//   --compare-f1      with --check-routines: also compare f1 for routines
//                     that do not return it (an operation capture ending at
//                     their return compares it)
//   --synthetic <file>
//                     with --check-routines: synthetic calls of tactics
//                     routines no capture reaches, compared like checked calls
//                     (routine_check.hpp SyntheticCall gives the file's format)
//   --census <file>   count the calls of each entry listed in <file> (one
//                     hex address per line) that pass through the dispatcher
//                     (tools/tactics_audit.py --entries lists the tactics system)
//   --stack-audit     list the original's reads of stack it left uninitialised
//                     (values earlier calls left behind; see stack_audit.hpp);
//                     owners take their observed paths, so the run is slower
#include "match_replay.hpp"
#include "match_speed.hpp"
#include "match_turbo.hpp"
#include "guest_profiler.hpp"
#include "season_reference.hpp"
#include "semantic_io_checks.hpp"
#include "stack_audit.hpp"
#include "routine_check.hpp"
#include "tactics_adapter.hpp"
#include "main_program_adapter.hpp"
#include "player_stats_adapter.hpp"
#include "club_history_adapter.hpp"
#include "eng_prm_adapter.hpp"
#include "pol_cup_adapter.hpp"
#include "oceania_nations_adapter.hpp"
#include "news_adapter.hpp"
#include "screen_manager_adapter.hpp"
#include "contract_manager_adapter.hpp"
#include "shortlist_manager_adapter.hpp"
#include "club_control.hpp"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <optional>
#include <sys/time.h>
#include <csignal>

using namespace cm::aot;

namespace {
// Every club of the snapshot against the original, with and without the
// option checks, with r6 null, the club's own manager and the managers of
// the clubs humans control (which differ from most clubs' own).
int check_club_control(const State& initial, bool shifted) {
    const auto r2 = std::uint32_t(initial.r<4>(8));
    const auto word = [&](std::uint32_t address) { return std::uint32_t(initial.memory.read<4>(address)); };
    const auto clubs = word(word(r2 - 0x7be8)), count = word(word(r2 - 0x7b50));
    unsigned compared = 0, controlled = 0, errors = 0, mismatches = 0;
    // The original only reads (its writes are in the error reports, which are
    // left to it), so one copy of the state serves every call.
    auto s = cm::semantic_io_checks::clone(initial);
    s->budget = UINT64_MAX;
    const auto sp = std::uint32_t(s->r<4>(4)) - 0x400;
    std::vector<std::uint32_t> human_managers;
    for (std::uint32_t index = 0; index < count && human_managers.size() < 3; ++index)
        if (const auto club = clubs + index * 0x248, manager = word(club + 0x90);
            manager && club_control::human_controlled(initial, club, 1, 1, 0, 0).value_or(false))
            human_managers.push_back(manager);
    for (std::uint32_t index = 0; index + (shifted ? 1 : 0) < count; ++index) {
        const auto club = clubs + index * 0x248;
        for (std::uint32_t r4 : {0u, 1u}) for (std::uint32_t r5 : {0u, 1u}) for (std::uint32_t r7 : {0u, 1u})
            for (std::uint32_t r6 : [&] { auto values = human_managers; values.push_back(0); values.push_back(word(club + 0x90)); return values; }()) {
                const auto native = club_control::human_controlled(initial, club + (shifted ? 0x248 : 0), r4, r5, r6, r7);
                if (!native) { ++errors; continue; }
                s->wr<4>(12, club); s->wr<4>(16, r4); s->wr<4>(20, r5); s->wr<4>(24, r6); s->wr<4>(28, r7);
                s->wr<4>(4, sp);
                s->wr<4>(4128, return_sentinel); s->pc = 0x1014c2e0;
                run_until(*s, return_sentinel);
                const bool original = std::int8_t(s->r<4>(12)) != 0;
                ++compared; controlled += original;
                if (original != *native && ++mismatches <= 10)
                    std::cerr << "club " << hex_address(club) << " r4=" << r4 << " r5=" << r5 << " r6=" << hex_address(r6)
                              << " r7=" << r7 << " original=" << original << " native=" << *native << '\n';
            }
    }
    std::cout << "club control: " << compared << " compared, " << controlled << " controlled, " << errors
              << " left to the original, " << mismatches << " mismatches\n";
    return mismatches || !compared || !controlled ? 1 : 0;
}
std::optional<std::string> capture_field(const std::filesystem::path& capture, std::string_view name) {
    std::ifstream report(capture / "capture.txt");
    for (std::string line; std::getline(report, line);)
        if (line.starts_with(name) && line.size() > name.size() && line[name.size()] == '=') return line.substr(name.size() + 1);
    return std::nullopt;
}
}

int main(int argc, char** argv) {
    try {
        if (argc < 2) throw std::runtime_error("Usage: cm-world-replay <capture> [runs] [--write-expected <path>] [--flip <address>]");
        const std::filesystem::path capture = argv[1];
        unsigned runs = 1;
        std::optional<std::filesystem::path> write_expected;
        std::optional<std::uint32_t> flip;
        std::optional<std::filesystem::path> profile;
        bool timing = false, stack_audit = false, club_control_check = false, club_control_shifted = false;
        std::optional<std::uint64_t> scramble;
        std::optional<RoutineChecker> checker;
        bool checker_corrupted = false;
        std::optional<std::set<std::uint32_t>> only;
        bool strict_volatile = false, compare_f1 = false;
        static std::vector<SyntheticCall> synthetic;
        static unsigned synthetic_differences = 0;
        static std::map<std::uint32_t, std::uint64_t> census;
        for (int i = 2; i < argc; ++i) {
            const std::string_view option(argv[i]);
            if (option == "--write-expected" && i + 1 < argc) write_expected = argv[++i];
            else if (option == "--flip" && i + 1 < argc) flip = std::uint32_t(std::stoul(argv[++i], nullptr, 0));
            else if (option == "--profile" && i + 1 < argc) profile = argv[++i];
            else if (option == "--timing") timing = true;
            else if (option == "--stack-audit") stack_audit = true;
            else if (option == "--scramble-stack" && i + 1 < argc) { scramble = std::stoull(argv[++i], nullptr, 0); stack_audit = true; }
            else if (option == "--check-club-control") club_control_check = true;
            else if (option == "--check-club-control-shifted") club_control_check = club_control_shifted = true;
            else if ((option == "--check-routines" || option == "--check-routines-corrupted") && i + 1 < argc) {
                const auto first = std::stoull(argv[++i]);
                std::uint64_t every = 0;
                if (i + 1 < argc && std::string_view(argv[i + 1]).find_first_not_of("0123456789") == std::string_view::npos)
                    every = std::stoull(argv[++i]);
                checker.emplace([&only](std::uint32_t entry) { return !only || only->contains(entry); }, first, every);
                checker_corrupted = option == "--check-routines-corrupted";
            }
            else if (option == "--only" && i + 1 < argc) {
                only.emplace();
                std::istringstream list(argv[++i]);
                for (std::string item; std::getline(list, item, ',');) only->insert(std::uint32_t(std::stoul(item, nullptr, 16)));
            }
            else if (option == "--strict-volatile") strict_volatile = true;
            else if (option == "--compare-f1") compare_f1 = true;
            else if (option == "--synthetic" && i + 1 < argc) synthetic = read_synthetic_calls(argv[++i]);
            else if (option == "--census" && i + 1 < argc) {
                std::ifstream list(argv[++i]);
                for (std::string line; std::getline(list, line);)
                    if (!line.empty()) census[std::uint32_t(std::stoul(line, nullptr, 16))] = 0;
                if (census.empty()) throw std::runtime_error("The census list is empty");
            }
            else if (!option.starts_with("--")) runs = unsigned(std::stoul(argv[i]));
            else throw std::runtime_error("Unknown option " + std::string(option));
        }
        if (!runs) throw std::runtime_error("At least one run is needed");
        if (checker) {
            checker->corrupt_first = checker_corrupted;
            checker->strict_volatile = strict_volatile;
            checker->compare_f1 = compare_f1;
            routine_checker = &*checker;
            dispatch_probe = [](State& s) {
                return tactics_dispatch_probe(s) || main_program_dispatch_probe(s) || player_stats_dispatch_probe(s) || club_history_dispatch_probe(s) || eng_prm_dispatch_probe(s) || news_dispatch_probe(s) || pol_cup_dispatch_probe(s) || oceania_nations_dispatch_probe(s) || screen_manager_dispatch_probe(s) || contract_manager_dispatch_probe(s) ||
                       shortlist_manager_dispatch_probe(s);
            };
            if (!synthetic.empty())
                dispatch_probe = [](State& s) {
                    // Each synthetic call runs in the family that lists its target.
                    synthetic_differences += run_synthetic_calls(s, synthetic,
                        [](State& t, std::uint32_t entry) {
                            (main_program_lists(entry)        ? main_program_run_original
                             : screen_manager_lists(entry)    ? screen_manager_run_original
                             : news_lists(entry)              ? news_run_original
                             : oceania_nations_lists(entry)   ? oceania_nations_run_original
                             : pol_cup_lists(entry)           ? pol_cup_run_original
                             : eng_prm_lists(entry)           ? eng_prm_run_original
                             : club_history_lists(entry)      ? club_history_run_original
                             : contract_manager_lists(entry)  ? contract_manager_run_original
                             : player_stats_lists(entry)      ? player_stats_run_original
                             : shortlist_manager_lists(entry) ? shortlist_manager_run_original
                                                              : tactics_run_original)(t, entry);
                        },
                        [](State& t, std::uint32_t entry) {
                            (main_program_lists(entry)        ? main_program_run_rewrite
                             : screen_manager_lists(entry)    ? screen_manager_run_rewrite
                             : news_lists(entry)              ? news_run_rewrite
                             : oceania_nations_lists(entry)   ? oceania_nations_run_rewrite
                             : pol_cup_lists(entry)           ? pol_cup_run_rewrite
                             : eng_prm_lists(entry)           ? eng_prm_run_rewrite
                             : club_history_lists(entry)      ? club_history_run_rewrite
                             : contract_manager_lists(entry)  ? contract_manager_run_rewrite
                             : player_stats_lists(entry)      ? player_stats_run_rewrite
                             : shortlist_manager_lists(entry) ? shortlist_manager_run_rewrite
                                                              : tactics_run_rewrite)(t, entry);
                        },
                        routine_checker && routine_checker->strict_volatile);
                    return tactics_dispatch_probe(s) || main_program_dispatch_probe(s) || player_stats_dispatch_probe(s) || club_history_dispatch_probe(s) || eng_prm_dispatch_probe(s) || news_dispatch_probe(s) || pol_cup_dispatch_probe(s) || oceania_nations_dispatch_probe(s) || screen_manager_dispatch_probe(s) || contract_manager_dispatch_probe(s) ||
                       shortlist_manager_dispatch_probe(s);
                };
        }
        if (!census.empty()) {
            if (checker) throw std::runtime_error("--census and --check-routines are separate runs");
            dispatch_probe = [](State& s) {
                if (const auto found = census.find(s.pc); found != census.end()) ++found->second;
                return false;
            };
        }
        initialize_native_match_speed();
        set_match_turbo_enabled(true);
        auto initial = std::make_unique<State>(), expected = std::make_unique<State>();
        load_match_snapshot(*initial, capture / "initial.snapshot");
        load_match_snapshot(*expected, capture / "expected.snapshot");
        if (club_control_check) return check_club_control(*initial, club_control_shifted);
        // A sort capture starts at its routine's entry rather than its saved PC.
        if (const auto entry = capture_field(capture, "entry")) initial->pc = std::uint32_t(std::stoul(*entry, nullptr, 16));
        const auto days = capture_field(capture, "days");
        // A session capture (automation "capture start/stop") ends where its
        // transcript does; a season capture at its final day boundary.
        const bool session = capture_field(capture, "session") == "1";
        const bool season = (days && std::stoul(*days) > 0) || session;
        if (season && !std::filesystem::exists(capture / "imports.gz"))
            throw std::runtime_error("A season capture needs its imports.gz transcript");
        if (flip) initial->memory.write<1>(*flip, initial->memory.read<1>(*flip) ^ 0xff);
        const bool transcript = std::filesystem::exists(capture / "imports.gz");
        if (profile) guest_profiler::reset(4000000);
                for (unsigned run = 0; run < runs; ++run) {
            auto state = cm::semantic_io_checks::clone(*initial);
            state->budget = UINT64_MAX;
            if (transcript) {
                if (session) open_session_replay(capture, timing);
                else if (timing) open_timing_replay(capture); else open_operation_replay(capture);
                seed_season_replay(*state);
                if (session) begin_session_replay(*state);
            }
            std::optional<StackAudit> audit;
            if (stack_audit) {
                if (scramble) audit.emplace(*state, *scramble); else audit.emplace(*state);
                state->memory.date_observer = audit->observer();
            }
            const auto executed = state->executed, calls = state->native_calls;
            if (profile) guest_profiler::start(*state);
            const auto begin = std::chrono::steady_clock::now();
            const auto cpu = std::clock();
            if (season) {
                bool complete = false;
                try { cm::aot::run(*state); } catch (const SeasonReplayComplete&) { complete = true; }
                if (!complete) throw std::runtime_error("The replay ended before the end of its capture");
            } else {
                run_until(*state, expected->pc);
            }
            if (profile) guest_profiler::stop();
            const auto wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
            const auto cpu_seconds = double(std::clock() - cpu) / CLOCKS_PER_SEC;
            if (transcript) { require_season_replay_eof(); close_season_replay(); }
            state->memory.observer = nullptr;
            if (audit) { state->memory.date_observer = nullptr; audit->report(std::cerr); }
            // The flipped byte itself is excluded; only its effects on the run count.
            if (flip) state->memory.write<1>(*flip, expected->memory.read<1>(*flip));
            if (scramble) {
                // Every live byte that differs (the stack below the final SP is dead).
                const auto sp = std::uint32_t(expected->r<4>(4));
                std::uint64_t differing = 0;
                std::map<std::uint32_t, std::uint64_t> by_allocation;
                for (const auto& [start, size] : expected->memory.mapped_ranges()) {
                    const auto a = expected->memory.copy_out(start, size), b = state->memory.copy_out(start, size);
                    for (std::size_t i = 0; i < a.size(); ++i) {
                        const auto at = start + std::uint32_t(i);
                        if (a[i] == b[i] || (at >= 0x70000000 && at < sp)) continue;
                        ++differing;
                        auto allocation = expected->allocated.upper_bound(at);
                        std::uint32_t owner = at;
                        if (allocation != expected->allocated.begin() && at - std::prev(allocation)->first < std::prev(allocation)->second)
                            owner = std::prev(allocation)->first;
                        if (++by_allocation[owner] == 1 && by_allocation.size() <= 40)
                            std::cerr << "differs at " << hex_address(at) << " (allocation " << hex_address(owner) << ") expected="
                                      << unsigned(a[i]) << " actual=" << unsigned(b[i]) << '\n';
                    }
                }
                std::cerr << differing << " live bytes differ, in " << by_allocation.size() << " allocations or places\n";
            } else if (write_expected) {
                save_runtime_snapshot(*state, *write_expected);
                std::cerr << "Wrote " << write_expected->string() << '\n';
            } else if (const auto difference = compare_operation_states(*expected, *state); !difference.empty()) {
                throw std::runtime_error(difference);
            }
            std::cout << std::setprecision(9) << "{\"run\":" << run << ",\"wall\":" << wall << ",\"cpu\":" << cpu_seconds
                      << ",\"instructions\":" << state->executed - executed << ",\"nativeCalls\":" << state->native_calls - calls
                      << "}" << std::endl;
            if (write_expected) break;
        }
        if (profile) guest_profiler::write(*profile);
        for (const auto& [entry, calls] : census) std::cout << "census " << hex_address(entry) << ' ' << calls << '\n';
        if (checker) {
            std::uint64_t checked = 0, calls = 0;
            for (const auto& [entry, totals] : checker->totals()) {
                checked += totals.checked; calls += totals.calls;
                std::cerr << "routine " << hex_address(entry) << ": " << totals.calls << " calls, " << totals.checked
                          << " checked, " << totals.differences << " differ\n";
            }
            std::cout << "routine check: " << checker->totals().size() << " routines, " << calls << " calls, "
                      << checked << " checked, " << checker->differences() << " differ" << std::endl;
            routine_checker = nullptr;
            dispatch_probe = nullptr;
            if (!synthetic.empty()) std::cout << "synthetic calls: " << synthetic.size() << " listed, " << synthetic_calls_made() << " made, "
                                               << synthetic_differences << " differ" << std::endl;
            // A line whose trigger never dispatched checked nothing: a hooked
            // trigger, a wrong from= or an nth past the recording.
            unsigned unmade = 0;
            for (const auto& call : synthetic)
                if (!call.made && ++unmade) std::cerr << "synthetic call never made: " << call.line << '\n';
            if (checker->differences() || synthetic_differences || unmade) return 1;
        }
        return 0;
    } catch (const std::exception& error) {
        close_season_replay();
        std::cerr << error.what() << '\n';
        return 1;
    }
}
