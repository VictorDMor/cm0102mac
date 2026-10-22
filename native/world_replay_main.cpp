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
#include "match_replay.hpp"
#include "match_speed.hpp"
#include "match_turbo.hpp"
#include "guest_profiler.hpp"
#include "season_reference.hpp"
#include "semantic_io_checks.hpp"
#include <chrono>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <optional>
#include <sys/time.h>
#include <csignal>

using namespace cm::aot;

namespace {
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
        bool timing = false;
        for (int i = 2; i < argc; ++i) {
            const std::string_view option(argv[i]);
            if (option == "--write-expected" && i + 1 < argc) write_expected = argv[++i];
            else if (option == "--flip" && i + 1 < argc) flip = std::uint32_t(std::stoul(argv[++i], nullptr, 0));
            else if (option == "--profile" && i + 1 < argc) profile = argv[++i];
            else if (option == "--timing") timing = true;
            else if (!option.starts_with("--")) runs = unsigned(std::stoul(argv[i]));
            else throw std::runtime_error("Unknown option " + std::string(option));
        }
        if (!runs) throw std::runtime_error("At least one run is needed");
        initialize_native_match_speed();
        set_match_turbo_enabled(true);
        auto initial = std::make_unique<State>(), expected = std::make_unique<State>();
        load_match_snapshot(*initial, capture / "initial.snapshot");
        load_match_snapshot(*expected, capture / "expected.snapshot");
        // A sort capture starts at its routine's entry rather than its saved PC.
        if (const auto entry = capture_field(capture, "entry")) initial->pc = std::uint32_t(std::stoul(*entry, nullptr, 16));
        const auto days = capture_field(capture, "days");
        const bool season = days && std::stoul(*days) > 0;
        if (season && !std::filesystem::exists(capture / "imports.gz"))
            throw std::runtime_error("A season capture needs its imports.gz transcript");
        if (flip) initial->memory.write<1>(*flip, initial->memory.read<1>(*flip) ^ 0xff);
        const bool transcript = std::filesystem::exists(capture / "imports.gz");
        if (profile) guest_profiler::reset(4000000);
                for (unsigned run = 0; run < runs; ++run) {
            auto state = cm::semantic_io_checks::clone(*initial);
            state->budget = UINT64_MAX;
            if (transcript) {
                if (timing) open_timing_replay(capture); else open_operation_replay(capture);
                seed_season_replay(*state);
            }
            const auto executed = state->executed, calls = state->native_calls;
            if (profile) guest_profiler::start(*state);
            const auto begin = std::chrono::steady_clock::now();
            const auto cpu = std::clock();
            if (season) {
                bool complete = false;
                try { cm::aot::run(*state); } catch (const SeasonReplayComplete&) { complete = true; }
                if (!complete) throw std::runtime_error("The season replay ended before its final day boundary");
            } else {
                run_until(*state, expected->pc);
            }
            if (profile) guest_profiler::stop();
            const auto wall = std::chrono::duration<double>(std::chrono::steady_clock::now() - begin).count();
            const auto cpu_seconds = double(std::clock() - cpu) / CLOCKS_PER_SEC;
            if (transcript) { require_season_replay_eof(); close_season_replay(); }
            state->memory.observer = nullptr;
            // The flipped byte itself is excluded; only its effects on the run count.
            if (flip) state->memory.write<1>(*flip, expected->memory.read<1>(*flip));
            if (write_expected) {
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
        return 0;
    } catch (const std::exception& error) {
        close_season_replay();
        std::cerr << error.what() << '\n';
        return 1;
    }
}
