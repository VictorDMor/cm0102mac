#include "season_reference.hpp"
#include "match_replay.hpp"
#include "calendar_adapter.hpp"
#include <iostream>
#include <string_view>

int main(int argc, char** argv) {
    try {
        if (argc < 2) throw std::runtime_error("Usage: cm-season-reference <fixture> [--modern|--calendar-baseline] [--perturb-rng|--perturb-calendar|--perturb-difference] [--calendar-record|--calendar-check <private-edges.gz>]");
        bool modern = false, baseline = false, perturb_rng = false, perturb_calendar = false, record = false;
        bool perturb_difference = false;
        std::filesystem::path edges;
        for (int i = 2; i < argc; ++i) {
            const std::string_view option(argv[i]);
            if (option == "--modern") modern = true;
            else if (option == "--calendar-baseline") { modern = true; baseline = true; }
            else if (option == "--perturb-rng") perturb_rng = true;
            else if (option == "--perturb-calendar") perturb_calendar = true;
            else if (option == "--perturb-difference") perturb_difference = true;
            else if ((option == "--calendar-record" || option == "--calendar-check") && i + 1 < argc) {
                record = option == "--calendar-record"; edges = argv[++i];
            } else throw std::runtime_error("Invalid season replay option");
        }
        if ((record && modern) || ((perturb_calendar || perturb_difference) && (!modern || edges.empty())) ||
            (perturb_difference && baseline))
            throw std::runtime_error("Calendar references require original execution; calendar perturbation requires a modern edge check");
        const std::filesystem::path directory(argv[1]);
        auto state = std::make_unique<cm::aot::State>();
        auto expected = std::make_unique<cm::aot::State>();
        cm::aot::load_match_snapshot(*state, directory / "initial.snapshot");
        cm::aot::load_match_snapshot(*expected, directory / "expected.snapshot");
        state->calendar_original = !modern;
        state->date_difference_original = baseline;
        if (expected->executed <= state->executed)
            throw std::runtime_error("Private fixture has no bounded execution interval");
        // A real finite upper bound remains independent of the replacement's
        // reduced accounting. End-of-transcript plus final morning terminates.
        state->budget = expected->executed;
        if (perturb_rng) {
            const auto seed = state->memory.read<4>(0x108b3f00);
            state->memory.write<4>(seed, state->memory.read<4>(seed) ^ 1);
        }
        if (!edges.empty()) cm::aot::open_calendar_audit(edges, record, perturb_calendar, perturb_difference);
        cm::aot::open_season_replay(directory, modern);
        bool complete = false;
        try { cm::aot::run(*state); }
        catch (const cm::aot::SeasonReplayComplete&) { complete = true; }
        cm::aot::close_season_replay();
        cm::aot::close_calendar_audit(true);
        if (!complete) throw std::runtime_error("Replay returned before the final full-day boundary");
        const auto difference = cm::aot::compare_season_states(*expected, *state, modern);
        if (!difference.empty()) throw std::runtime_error(difference);
        std::cout << (modern ? "PASS: semantic calendar bounded simulation agreement\n" : "PASS: exact bounded simulation agreement\n")
                  << cm::aot::season_state_summary(*state) << '\n'
                  << cm::aot::calendar_call_summary(*state) << '\n';
        if (modern && (!state->calendar_calls[0] || !state->calendar_calls[1] || state->calendar_exceptional_calls))
            throw std::runtime_error("Real fixture did not exercise both modern calendar routines exclusively");
        if (modern && !baseline && !state->date_difference_calls)
            throw std::runtime_error("Real fixture did not exercise semantic date difference");
        if (baseline && state->date_difference_calls)
            throw std::runtime_error("Delivered calendar baseline unexpectedly used modern date difference");
        return 0;
    } catch (const std::exception& error) {
        cm::aot::close_season_replay();
        try { cm::aot::close_calendar_audit(); } catch (...) {}
        std::cerr << error.what() << '\n';
        return 1;
    }
}
