#include "match_debugger_factory.hpp"
#include "match_engine_adapter.hpp"
#include <chrono>
#include <iostream>
#include <set>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::uint64_t digest(const cm::aot::State& state, std::uint32_t match) {
    std::uint64_t result = 14695981039346656037ULL;
    for (auto byte : state.memory.copy_out(match, 0xe0bc)) { result ^= byte; result *= 1099511628211ULL; }
    return result;
}
void validate_players(const cm::aot::match_debugger::Session& session, std::uint32_t home, std::uint32_t away) {
    const auto& memory = session.state().memory;
    const auto match = session.match_address(), fixture = std::uint32_t(memory.read<4>(match + 0x3118));
    for (unsigned team = 0; team < 2; ++team) {
        const auto club = std::uint32_t(memory.read<4>(fixture + 0x1c + team * 4));
        require(memory.read<4>(club) == (team ? away : home), "Selected actual club identity");
        std::set<std::uint32_t> pitch, positions;
        for (unsigned actor = 0; actor < 20; ++actor) {
            const auto record = match + 0x311c + (team * 20 + actor) * 0x1c4;
            const auto position = std::int8_t(memory.read<1>(record + 0x19));
            if (position < 0 || position >= 11) continue;
            const auto staff = std::uint32_t(memory.read<4>(record + 0x6c));
            require(staff && memory.read<4>(staff + 0x14), "Selected actor has actual staff/player records");
            require(pitch.insert(std::uint32_t(memory.read<4>(staff))).second, "Duplicate starting player");
            require(positions.insert(std::uint32_t(position)).second, "Duplicate starting position");
            bool member = false;
            for (unsigned slot = 0; slot < 50; ++slot) member |= memory.read<4>(club + 0x98 + slot * 4) == staff;
            require(member, "Selected actor is not in the actual selected club roster");
        }
        require(pitch.size() == 11, "Original detailed selector did not produce eleven starting players");
    }
}
}

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("Expected original program, read-only game and private check root");
        using namespace cm::aot::match_debugger;
        const auto root = std::filesystem::path(argv[3]) /
            std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        unsigned generation = 0;
        bool original_shared = false;
        std::vector<std::string> progress;
        MatchSetup setup{677, 5677, 12345, 2001, 194};
        Session first([&] {
            auto created = create_match({argv[1], argv[2], root / ("first-" + std::to_string(generation++))}, setup,
                [&](std::string_view stage) { progress.emplace_back(stage); });
            cm::aot::match_engine_diagnostics(*created.state).entry_original[2] = original_shared;
            return created;
        }, fresh_match_step_limit);
        validate_players(first, setup.home_club, setup.away_club);
        require(first.execution_state() == ExecutionState::paused && first.inspect().elapsed_engine_units == 0,
            "Fresh detailed match does not open paused before kickoff");
        require(progress.size() == 6, "Match creation progress boundaries include actual substitutes");
        const auto initial = digest(first.state(), first.match_address());
        const auto inspection = first.inspect();
        require(!first.advance_if_running() && first.inspect() == inspection, "Paused engine advanced");
        std::vector<StepRecord> interval;
        for (unsigned step = 0; step < 4; ++step) {
            interval.push_back(first.step());
            const auto& record = interval.back();
            std::cout << "seeded tick " << step << " result=" << record.engine_return << " instructions=" << record.instructions
                      << " time=" << record.after.elapsed_engine_units << " phase=" << int(record.after.phase)
                      << " transition=" << int(record.after.transition) << '\n';
        }
        require(first.inspect().elapsed_engine_units == 4 && interval.front().instructions > 0,
            "Fresh kickoff and individual actual ticks were not executed");
        require(cm::aot::match_engine_diagnostics(first.state()).calls[2] == 4, "Shared C++23 advancement was bypassed");
        const auto modern_interval = digest(first.state(), first.match_address());
        original_shared = true;
        first.reset();
        validate_players(first, setup.home_club, setup.away_club);
        require(first.inspect() == inspection && digest(first.state(), first.match_address()) == initial,
            "Explicit-seed reset did not recreate the initial actual match");
        for (unsigned step = 0; step < interval.size(); ++step) {
            const auto repeated = first.step();
            require(repeated.before == interval[step].before && repeated.after == interval[step].after &&
                repeated.instructions == interval[step].instructions && repeated.engine_return == interval[step].engine_return,
                "Reset changed the actual original seeded interval");
        }
        require(digest(first.state(), first.match_address()) == modern_interval &&
            cm::aot::match_engine_diagnostics(first.state()).original_calls[2] == 4,
            "Actual original shared advancement differs from the C++23 interval");
        const auto retained = digest(first.state(), first.match_address());
        const auto stable = first.inspect();
        setup.random_seed = 0;
        bool rejected = false;
        try { first.reset(); } catch (const std::invalid_argument&) { rejected = true; }
        setup.random_seed = 12345;
        require(rejected && first.inspect() == stable && digest(first.state(), first.match_address()) == retained,
            "Invalid replacement creation discarded the previous actual experiment");
        MatchSetup different{5346, 1953, 67890, 2001, 194};
        Session second([&] { return create_match({argv[1], argv[2], root / "second"}, different); }, fresh_match_step_limit);
        validate_players(second, different.home_club, different.away_club);
        require(&first.state() != &second.state(), "Different pairings share guest state");
        second.run();
        for (unsigned step = 0; step < 4; ++step) require(bool(second.advance_if_running()), "Scheduled real tick missing");
        second.pause();
        const auto paused = second.inspect();
        require(!second.advance_if_running() && second.inspect() == paused, "Scheduled pause did not stop advancement");
        require(digest(first.state(), first.match_address()) == retained, "Another experiment changed the first match");
        require(cm::aot::match_engine_diagnostics(second.state()).calls[2] == 4, "Second pairing bypassed shared C++23 engine");
        std::cout << "PASS: 2 actual club pairings, 3 fresh worlds, 11 real starters per side, 12 engine ticks, original/C++23 agreement, paused/scheduled execution, explicit-seed reset, failed-reset preservation and independent state\n";
        std::cout << "No career or captured match input; remaining original creation dependencies retained\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
