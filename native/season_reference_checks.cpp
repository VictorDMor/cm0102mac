#include "season_reference.hpp"
#include "match_replay.hpp"
#include "callbacks.hpp"
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <unistd.h>

using namespace cm::aot;
namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
constexpr std::uint32_t fixture = 0x30000000, boundary = 0x101195e8, service = 0x10bf7b48;
struct Directory {
    std::filesystem::path path;
    Directory() {
        auto pattern = (std::filesystem::temp_directory_path() / "cm-season-checks-XXXXXX").string();
        if (!mkdtemp(pattern.data())) throw std::runtime_error("Cannot create private test directory");
        path = pattern;
    }
    ~Directory() { std::error_code ignored; std::filesystem::remove_all(path, ignored); }
};
void synthetic_input(State& s) { s.pc = service; s.wr<4>(12, 16); ++s.executed; }
void synthetic_engine_step(State& s) {
    const auto cursor = s.memory.read<4>(0x108b3f0c);
    s.memory.write<4>(cursor, s.memory.read<4>(cursor) + 2);
    s.memory.write<2>(fixture, 40);
    s.pc = boundary; ++s.executed;
}
std::unique_ptr<State> initial(const std::filesystem::path& path) {
    auto s = std::make_unique<State>(); load_match_snapshot(*s, path / "initial.snapshot"); return s;
}
void replay_fixture(State& s, const std::filesystem::path& path) {
    open_season_replay(path); season_day_boundary(s, boundary); synthetic_input(s);
    require(season_replay_import(s, "synthetic-native-service"), "Recorded native service was not replayed");
    ++s.native_calls; synthetic_engine_step(s);
    bool complete = false;
    try { season_day_boundary(s, boundary); } catch (const SeasonReplayComplete&) { complete = true; }
    require(complete, "Final private fixture boundary was not reached"); close_season_replay();
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-season-reference-checks <original-segments>\n"; return 2; }
    try {
        Directory directory;
        setenv("CM_AOT_SEASON_REFERENCE", directory.path.c_str(), 1);
        auto s = std::make_unique<State>(); load_program(*s, argv[1]);
        s->memory.map(fixture, 128);
        s->memory.write<4>(0x108b3f10, fixture); s->memory.write<2>(fixture, 39);
        s->memory.write<2>(fixture + 2, 2002); s->memory.write<1>(fixture + 4, 0);
        s->memory.write<4>(0x108b40a4, fixture + 8); s->memory.write<1>(fixture + 8, 0);
        for (const auto& [global, offset] : std::vector<std::pair<std::uint32_t, unsigned>>{
            {0x108b3f00, 16}, {0x108b3f0c, 20}, {0x108b3f08, 24}}) {
            s->memory.write<4>(global, fixture + offset); s->memory.write<4>(fixture + offset, 123 + offset);
        }
        s->memory.write<1>(0x108b8564, 0); s->executed = 100;
        season_day_boundary(*s, boundary); synthetic_input(*s);
        season_record_import_begin(*s);
        const auto allocation = allocate(*s, 32, true);
        s->memory.write<4>(allocation, 0x12345678);
        s->memory.copy_in(allocation + 4, {1,2,3,4}); s->memory.clear(allocation + 8, 8);
        // Execute the same four original launch instructions during replay,
        // including stack marshalling/restoration, rather than copying their
        // game-memory effects into the native transcript.
        require(call_original(*s, 0x108bdaf4, {}) == 0, "Original callback fixture did not return");
        require(s->memory.read<1>(0x108b8564) == 1, "Original callback effect was not exercised");
        s->wr<4>(12, allocation); s->pc = boundary;
        season_record_import_end(*s); ++s->native_calls;
        synthetic_engine_step(*s); season_day_boundary(*s, boundary);
        require(s->memory.observer == nullptr, "Native memory observer leaked into normal engine execution");
        auto a = initial(directory.path), b = initial(directory.path);
        replay_fixture(*a, directory.path); replay_fixture(*b, directory.path);
        require(compare_season_states(*s, *a).empty() && compare_season_states(*a, *b).empty(),
                "Equivalent synthetic transcript executions disagreed");
        const auto seed = a->memory.read<4>(0x108b3f00);
        a->memory.write<4>(seed, a->memory.read<4>(seed) ^ 1);
        require(compare_season_states(*s, *a).find("rng.seed") != std::string::npos,
                "Named RNG divergence was not detected");
        b->memory.write<1>(allocation + 9, 42);
        require(compare_season_states(*s, *b).find("allocation") != std::string::npos,
                "Allocated-memory divergence was not detected");
        auto changed = initial(directory.path);
        open_season_replay(directory.path); season_day_boundary(*changed, boundary); synthetic_input(*changed);
        changed->wr<4>(12, 17);
        bool rejected = false;
        try { (void)season_replay_import(*changed, "synthetic-native-service"); }
        catch (const std::runtime_error& error) { rejected = std::string(error.what()).find("native input register") != std::string::npos; }
        close_season_replay(); require(rejected, "Changed native service input was accepted");
        const auto transcript = directory.path / "imports.gz";
        std::filesystem::resize_file(transcript, std::filesystem::file_size(transcript) / 2);
        rejected = false;
        try { auto truncated = initial(directory.path); replay_fixture(*truncated, directory.path); }
        catch (const std::runtime_error&) { rejected = true; }
        close_season_replay(); require(rejected, "Truncated native transcript was accepted");
        std::cout << "Synthetic season transcript: native writes/clear/map/allocation, original callback, two equivalent executions, RNG/input/memory divergence and truncation checks passed\n";
        return 0;
    } catch (const std::exception& error) { close_season_replay(); std::cerr << error.what() << '\n'; return 1; }
}
