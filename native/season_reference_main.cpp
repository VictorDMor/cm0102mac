#include "season_reference.hpp"
#include "match_replay.hpp"
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    if (argc != 2 && !(argc == 3 && std::string_view(argv[2]) == "--perturb-rng")) {
        std::cerr << "Usage: cm-season-reference <private-fixture-directory> [--perturb-rng]\n";
        return 2;
    }
    try {
        const std::filesystem::path directory(argv[1]);
        auto state = std::make_unique<cm::aot::State>();
        auto expected = std::make_unique<cm::aot::State>();
        cm::aot::load_match_snapshot(*state, directory / "initial.snapshot");
        cm::aot::load_match_snapshot(*expected, directory / "expected.snapshot");
        if (expected->executed <= state->executed)
            throw std::runtime_error("Private fixture has no bounded execution interval");
        state->budget = expected->executed;
        if (argc == 3) {
            const auto seed = state->memory.read<4>(0x108b3f00);
            state->memory.write<4>(seed, state->memory.read<4>(seed) ^ 1);
        }
        cm::aot::open_season_replay(directory);
        bool complete = false;
        try { cm::aot::run(*state); }
        catch (const cm::aot::SeasonReplayComplete&) { complete = true; }
        cm::aot::close_season_replay();
        if (!complete) throw std::runtime_error("Replay returned before the final full-day boundary");
        const auto difference = cm::aot::compare_season_states(*expected, *state);
        if (!difference.empty()) throw std::runtime_error(difference);
        std::cout << "PASS: exact bounded simulation agreement\n" << cm::aot::season_state_summary(*state) << '\n';
        return 0;
    } catch (const std::exception& error) {
        cm::aot::close_season_replay();
        std::cerr << error.what() << '\n';
        return 1;
    }
}
