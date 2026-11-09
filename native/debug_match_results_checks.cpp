#include "debug_match_results.hpp"
#include "match_debugger_checkpoint.hpp"
#include "match_debugger_factory.hpp"
#include "match_debugging.hpp"
#include "match_eng_fields.hpp"
#include "platform.hpp"
#include "routine_check.hpp"
#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace {
using namespace cm::aot;
using namespace cm::aot::match_debugger;
using match_eng::clock_at;
using match_eng::fixture_at;
using match_eng::player_size;
using match_eng::players_at;
using match_eng::side_size;

constexpr std::uint32_t fixture_size = 0x54;
constexpr std::uint32_t first_human_pending = 0x109806f0 + 0xec;
constexpr unsigned result_feature = 1u << unsigned(DebugFeature::match_result);
constexpr std::uint32_t quick_advance_entry = 0x101d3f60;
constexpr std::uint32_t quick_events_entry = 0x101db340;

void require(bool condition, std::string_view message) {
    if (!condition) throw std::runtime_error(std::string(message));
}

std::uint32_t invoke(State& state, std::uint32_t entry, std::initializer_list<std::uint32_t> args) {
    unsigned reg = 3;
    for (auto arg : args) state.wr<4>(reg++ * 4, arg);
    state.wr<4>(4128, return_sentinel);
    state.pc = entry;
    state.budget = state.executed + fresh_match_step_limit;
    run_until(state, return_sentinel);
    return std::uint32_t(state.r<4>(12));
}

std::uint32_t toc(const State& state, std::int32_t displacement) {
    return std::uint32_t(state.memory.read<4>(std::uint32_t(state.r<4>(8)) + std::uint32_t(displacement)));
}

std::uint32_t club_at(const State& state, std::uint32_t id) {
    const auto clubs = std::uint32_t(state.memory.read<4>(toc(state, -0x7be8)));
    const auto address = clubs + id * 0x248;
    require(state.memory.read<4>(address) == id, "Second fixture club identity mismatch");
    return address;
}

std::vector<RosterMember> roster(const State& state, std::uint32_t club) {
    std::vector<RosterMember> members;
    for (unsigned slot = 0; slot < 50; ++slot) {
        const auto staff = std::uint32_t(state.memory.read<4>(club + 0x98 + slot * 4));
        if (!staff) continue;
        const auto player = std::uint32_t(state.memory.read<4>(staff + 0x14));
        if (player) members.push_back({std::uint32_t(state.memory.read<4>(staff)), staff, player});
    }
    require(members.size() >= 11, "Second fixture has fewer than eleven players");
    return members;
}

std::uint32_t copy_fixture(State& state, std::uint32_t fixture) {
    const auto copy = allocate(state, fixture_size, true);
    require(copy != 0, "Could not allocate fixture copy");
    state.memory.copy_in(copy, state.memory.copy_out(fixture, fixture_size));
    return copy;
}

struct Fixture {
    std::uint32_t engine, original, queued, queue_entry;
};

Fixture queue_fixture(State& state, std::uint32_t engine, std::uint32_t original) {
    // Keep the original fixture separate from the core embedded in a queue
    // entry, as the fixtures screen and quick engine do in a career.
    for (unsigned offset = 0x43; offset <= 0x4c; ++offset)
        state.memory.write<1>(original + offset, 0xff);
    const auto entry = allocate(state, 0x6c, true);
    require(entry != 0, "Could not allocate queue entry");
    state.memory.write<4>(entry, original);
    state.memory.copy_in(entry + 4, state.memory.copy_out(original, fixture_size));
    state.memory.write<4>(engine + fixture_at, entry + 4);
    return {engine, original, entry + 4, entry};
}

struct Cleanup {
    std::filesystem::path root;
    ~Cleanup() { std::error_code ignored; std::filesystem::remove_all(root, ignored); }
};

struct World {
    std::unique_ptr<State> state;
    Fixture first, second;
};

World prepare_round(const MatchInputs& inputs) {
    auto prepared = prepare_match(inputs, {677, 5677, 12345, 2001, 194});
    auto& state = *prepared.state;
    const auto second_engine = invoke(state, 0x10773520, {0xe0bc});
    require(second_engine != 0, "Could not allocate the second match engine");
    invoke(state, 0x101d16a0, {second_engine});
    const auto second_fixture = copy_fixture(state, prepared.fixture);
    // The same alternate pairing already passes the original factory checks.
    const auto second_home = club_at(state, 5346), second_away = club_at(state, 1953);
    state.memory.write<4>(second_fixture + 0x1c, second_home);
    state.memory.write<4>(second_fixture + 0x20, second_away);
    state.memory.write<4>(second_fixture + 0x24, state.memory.read<4>(second_home + 0x40));
    auto second_rosters = std::array{roster(state, second_home), roster(state, second_away)};
    auto first = start_match(std::move(prepared.state), prepared.match, prepared.fixture, std::move(prepared.rosters));
    auto second = start_match(std::move(first.state), second_engine, second_fixture, std::move(second_rosters));
    const auto a = queue_fixture(state, first.match, prepared.fixture);
    const auto b = queue_fixture(state, second.match, second_fixture);
    return {std::move(second.state), a, b};
}

void check_choices(const Checkpoint& initial, const Fixture& a, const Fixture& b) {
    auto state = initial.restore();
    auto& s = *state;
    initialize_native_debugging(false);
    require(!set_debug_match_result(s, a.original, ForcedMatchResult::home_win), "Disabled feature accepted a selection");
    select_native_debugging_features(result_feature);
    require(debug_match_result_eligible(s, a.original) && debug_match_result_eligible(s, b.original), "Pending fixtures were ineligible");
    require(set_debug_match_result(s, a.original, ForcedMatchResult::home_win), "Could not select a home win");
    require(set_debug_match_result(s, b.original, ForcedMatchResult::draw), "Could not select a draw");
    require(debug_match_result(s, a.queued) == ForcedMatchResult::home_win, "Queued copy lost its original selection");
    require(debug_match_result(s, b.queued) == ForcedMatchResult::draw, "Second fixture lost its independent selection");
    require(debug_match_result(*initial.restore(), a.original) == ForcedMatchResult::normal, "A new state inherited a selection");
    const auto other_date = copy_fixture(s, a.original);
    s.memory.write<2>(other_date + 0x28, s.memory.read<2>(other_date + 0x28) + 1);
    require(debug_match_result(s, other_date) == ForcedMatchResult::normal, "Selection leaked to the same clubs on a different date");
    require(set_debug_match_result(s, a.original, ForcedMatchResult::away_win), "Could not replace a choice");
    require(debug_match_result(s, a.queued) == ForcedMatchResult::away_win, "Replacement did not reach the queued copy");
    require(set_debug_match_result(s, a.original, ForcedMatchResult::normal), "Could not clear a choice");
    require(debug_match_result(s, a.queued) == ForcedMatchResult::normal &&
        debug_match_result(s, b.queued) == ForcedMatchResult::draw, "Clearing one match affected the other");
    s.memory.write<1>(a.original + 0x43, 1);
    require(!debug_match_result_eligible(s, a.original) &&
        !set_debug_match_result(s, a.original, ForcedMatchResult::draw), "Completed fixture accepted a choice");
    s.memory.write<1>(a.original + 0x43, 0xff);
    s.memory.write<4>(first_human_pending, a.queue_entry);
    require(!debug_match_result_eligible(s, a.original) && !debug_match_result_eligible(s, a.queued),
        "Watched fixture was eligible");
    require(!set_debug_match_result(s, a.original, ForcedMatchResult::draw) &&
        debug_match_result_eligible(s, b.original), "Watched fixture exclusion did not stay local");
}

void play_quick(State& state, const Fixture& fixture) {
    // The factory supplies actual lineups and match services. Select the
    // original quick branch, which plays the fixture in one advance call.
    state.memory.write<1>(fixture.engine, 1);
    require(invoke(state, quick_advance_entry, {fixture.engine, 1}) == 2, "Quick match did not return completion");
}

std::pair<unsigned, unsigned> score(const State& state, const Fixture& fixture) {
    const unsigned offset = state.memory.read<1>(fixture.queued + 0x47) == 0xff ? 0x43 : 0x47;
    return {unsigned(state.memory.read<1>(fixture.queued + offset)), unsigned(state.memory.read<1>(fixture.queued + offset + 1))};
}

unsigned player_total(const State& state, const Fixture& fixture, unsigned side, unsigned field) {
    unsigned total = 0;
    for (unsigned slot = 0; slot < 20; ++slot)
        total += unsigned(state.memory.read<1>(fixture.engine + players_at + side * side_size + slot * player_size + field));
    return total;
}

void check_played(const State& state, const Fixture& fixture, ForcedMatchResult expected) {
    const auto [home, away] = score(state, fixture);
    const auto actual = home == away ? ForcedMatchResult::draw : home > away ? ForcedMatchResult::home_win : ForcedMatchResult::away_win;
    std::cout << "Quick match " << home << '-' << away << " (choice " << unsigned(expected) << ")\n";
    require(actual == expected, "Quick match result disagrees with the selected outcome");
    require(home <= 3 && away <= 3, "Quick match produced an implausible score");
    require(player_total(state, fixture, 0, 0xc) == home && player_total(state, fixture, 1, 0xc) == away,
        "Scorer goals do not add up to the published score");
    require(player_total(state, fixture, 0, 0xf) == away && player_total(state, fixture, 1, 0xf) == home,
        "Goalkeeper concessions do not add up to the published score");
    require(state.memory.read<1>(fixture.engine + 0xe03d) == home &&
        state.memory.read<1>(fixture.engine + 0xe073) == away, "Team goal counts do not add up to the published score");
    require(debug_match_result(state, fixture.original) == ForcedMatchResult::normal,
        "Completed queued fixture left its original choice armed");
}

void check_round(const Checkpoint& initial, const Fixture& a, const Fixture& b) {
    auto state = initial.restore();
    require(set_debug_match_result(*state, a.original, ForcedMatchResult::home_win), "Could not arm first match");
    require(set_debug_match_result(*state, b.original, ForcedMatchResult::draw), "Could not arm second match");
    play_quick(*state, a);
    check_played(*state, a, ForcedMatchResult::home_win);
    require(debug_match_result(*state, b.original) == ForcedMatchResult::draw, "First result consumed the second fixture's choice");
    play_quick(*state, b);
    check_played(*state, b, ForcedMatchResult::draw);
}

void check_seeds(const Checkpoint& initial, const Fixture& a) {
    bool goalless_draw = false, five_goals = false;
    for (const auto [seed, day] : {std::pair{193u, 194u}, std::pair{9029u, 201u}, std::pair{65521u, 203u}}) {
        for (auto choice : {ForcedMatchResult::home_win, ForcedMatchResult::draw, ForcedMatchResult::away_win}) {
            auto state = initial.restore();
            state->memory.write<4>(toc(*state, -0x7b90), seed);
            state->memory.write<2>(a.original + 0x28, day);
            state->memory.write<2>(a.queued + 0x28, day);
            require(set_debug_match_result(*state, a.original, choice), "Could not arm the fixture for the selected seed");
            play_quick(*state, a);
            check_played(*state, a, choice);
            const auto [home, away] = score(*state, a);
            goalless_draw |= choice == ForcedMatchResult::draw && home == 0 && away == 0;
            five_goals |= home + away == 5;
        }
    }
    require(goalless_draw && five_goals, "Selected fixture dates did not exercise both goal-schedule boundaries");
}

void check_cup(const Checkpoint& initial, const Fixture& a) {
    auto state = initial.restore();
    // A score-level match needs extra time and a shootout under these flags.
    state->memory.write<2>(a.queued + 0x38, 3);
    require(set_debug_match_result(*state, a.original, ForcedMatchResult::draw), "Could not arm cup draw");
    play_quick(*state, a);
    check_played(*state, a, ForcedMatchResult::draw);
    require(state->memory.read<2>(a.engine + clock_at) == 120 && state->memory.read<1>(a.queued + 0x47) != 0xff,
        "Original cup extra time did not play");
    const auto home_penalties = state->memory.read<1>(a.queued + 0x49);
    const auto away_penalties = state->memory.read<1>(a.queued + 0x4a);
    require(home_penalties < 6 && away_penalties < 6 && home_penalties != away_penalties,
        "Forced draw prevented the original penalty shootout from deciding qualification");
}

struct MatchRngWrites final : MemoryObserver {
    std::uint32_t seed, cursor;
    std::vector<std::pair<std::uint32_t, std::uint64_t>> writes;

    explicit MatchRngWrites(const State& state) : seed(toc(state, -0x7b90)), cursor(toc(state, -0x7b84)) {
        restrict_write_notifications(std::min(seed, cursor), std::uint64_t(std::max(seed, cursor)) + 4);
    }
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t address, const std::uint8_t* bytes, std::size_t size) override {
        if (size == 4 && (address == seed || address == cursor)) writes.emplace_back(address, read_be<4>(bytes));
    }
};

void same_guest_result(const State& expected, const State& actual, std::string_view context) {
    require(expected.registers == actual.registers && expected.unique == actual.unique && expected.pc == actual.pc,
        std::string(context) + " changed the guest return state");
    require(expected.allocated == actual.allocated && expected.available == actual.available &&
        expected.heap_end == actual.heap_end && expected.memory_error == actual.memory_error,
        std::string(context) + " changed guest allocations");
    const auto ranges = expected.memory.mapped_ranges();
    require(ranges == actual.memory.mapped_ranges(), std::string(context) + " changed guest mappings");
    for (const auto& [start, size] : ranges)
        require(expected.memory.copy_out(start, size) == actual.memory.copy_out(start, size),
            std::string(context) + " changed guest memory at " + hex_address(start));
}

void check_load_boundary(const Checkpoint& initial, const Fixture& a, const Fixture& b) {
    auto state = initial.restore();
    require(set_debug_match_result(*state, a.original, ForcedMatchResult::home_win) &&
        set_debug_match_result(*state, b.original, ForcedMatchResult::draw), "Could not arm fixtures before loading");
    require(bool(state->platform), "Isolated fixture lacks the original load bridge");
    MatchRngWrites rng(*state);
    state->memory.observer = &rng;
    state->platform->quick_save_load_begin(*state);
    state->memory.observer = nullptr;
    require(debug_match_result(*state, a.original) == ForcedMatchResult::normal &&
        debug_match_result(*state, b.original) == ForcedMatchResult::normal, "Load boundary retained old career choices");
    require(rng.writes.empty(), "Clearing choices at load consumed match RNG");
    same_guest_result(*initial.restore(), *state, "Clearing choices at load");
}

// Route nested match calls through their normal dispatcher, with no
// differential sampling, to isolate two rare inputs at their own call sites.
struct MatchScript {
    enum class Kind { missing_scorer, fifth_dismissal };
    inline static MatchScript* current = nullptr;
    State& state;
    const Kind kind;
    RoutineChecker router{[](std::uint32_t) { return true; }, 0, 0};
    RoutineChecker* previous_router = routine_checker;
    bool (*previous_probe)(State&) = dispatch_probe;
    unsigned answered = 0;
    bool valid_goal = false;
    std::uint32_t invalid_actor = 0;
    std::uint64_t invalid_goals = 0, invalid_rating = 0;

    MatchScript(State& s, Kind script) : state(s), kind(script) {
        require(current == nullptr, "Nested match script");
        current = this;
        routine_checker = &router;
        dispatch_probe = dispatch;
    }
    ~MatchScript() {
        routine_checker = previous_router;
        dispatch_probe = previous_probe;
        current = nullptr;
    }
    static bool dispatch(State& s) {
        if (&s != &current->state) return false;
        return current->answer();
    }
    bool reply(std::uint32_t result) {
        ++answered;
        state.wr<4>(12, result);
        state.pc = std::uint32_t(state.r<4>(4128));
        return true;
    }
    bool answer() {
        const auto back = std::uint32_t(state.r<4>(4128));
        if (kind == Kind::missing_scorer) {
            // Missed chances use another return PC and keep the real picker.
            if (state.pc == 0x101e1640 && back == 0x101dca18 && !answered) {
                require(state.r<4>(16) == 0, "Fallback fixture should schedule a home goal first");
                invalid_actor = std::uint32_t(state.r<4>(12)) + players_at - player_size;
                invalid_goals = state.memory.read<1>(invalid_actor + 0xc);
                invalid_rating = state.memory.read<2>(invalid_actor + 0x36);
                return reply(0xffffffff);
            }
            if (state.pc == 0x101d6710 && back == 0x101dcbe0 && answered && !valid_goal) {
                require(state.r<4>(24) < 20, "The original goal message received an invalid scorer");
                require(state.memory.read<1>(invalid_actor + 0xc) == invalid_goals &&
                    state.memory.read<2>(invalid_actor + 0x36) == invalid_rating,
                    "Failed picker wrote before the home roster");
                valid_goal = true;
            }
            return false;
        }
        if (state.pc == 0x101d3c90) {
            switch (back) {
                case 0x101db858: // Home starts the minute.
                case 0x101dbf60: // A foul.
                case 0x101dc0f4: // A second yellow card.
                    return reply(1);
                case 0x101dbf40:
                case 0x101dbfb4:
                case 0x101dc0d4:
                    return reply(0);
            }
        }
        if (state.pc == 0x101e1a50 && back == 0x101dc118) return reply(1);
        return false;
    }
};

void check_missing_scorer(const Checkpoint& initial, const Fixture& fixture) {
    auto state = initial.restore();
    require(set_debug_match_result(*state, fixture.original, ForcedMatchResult::home_win), "Could not arm fallback fixture");
    MatchScript script(*state, MatchScript::Kind::missing_scorer);
    play_quick(*state, fixture);
    require(script.answered == 1 && script.valid_goal, "The scheduled scorer fallback was not exercised");
    check_played(*state, fixture, ForcedMatchResult::home_win);
}

void check_official_forfeit(const Checkpoint& initial, const Fixture& fixture) {
    auto state = initial.restore();
    require(set_debug_match_result(*state, fixture.original, ForcedMatchResult::draw), "Could not arm forfeit fixture");
    // The away side has four dismissals and slot 1 is about to get a
    // second yellow. End the match before the first scheduled goal.
    state->memory.write<1>(fixture.engine + 0xe081, 4);
    state->memory.write<1>(fixture.engine + players_at + side_size + player_size + 0x24, 1);
    MatchScript script(*state, MatchScript::Kind::fifth_dismissal);
    play_quick(*state, fixture);
    require(script.answered == 7 && state->memory.read<2>(fixture.engine + clock_at) == 1,
        "The scripted fifth dismissal did not terminate play in the first minute");
    require(score(*state, fixture) == std::pair{3u, 0u}, "A selected draw replaced the official forfeit");
    require(player_total(*state, fixture, 0, 0xc) == 0 && player_total(*state, fixture, 1, 0xc) == 0,
        "The official forfeit created fictional scorers");
    require(debug_match_result(*state, fixture.original) == ForcedMatchResult::normal,
        "Official forfeit left the completed choice armed");
}

enum class InactivePlan { other_fixture, cleared, disabled, watched, detailed };

void check_inactive(const Checkpoint& initial, const Fixture& a, const Fixture& b, InactivePlan kind) {
    auto expected = initial.restore();
    auto actual = initial.restore();
    select_native_debugging_features(result_feature);
    const auto planned = kind == InactivePlan::other_fixture ? b.original : a.original;
    require(set_debug_match_result(*actual, planned, ForcedMatchResult::away_win), "Could not prepare inactive choice");
    if (kind == InactivePlan::cleared)
        require(set_debug_match_result(*actual, planned, ForcedMatchResult::normal), "Could not clear inactive choice");
    if (kind == InactivePlan::disabled) select_native_debugging_features(0);
    if (kind == InactivePlan::watched) {
        expected->memory.write<4>(first_human_pending, a.queue_entry);
        actual->memory.write<4>(first_human_pending, a.queue_entry);
    }
    MatchRngWrites expected_rng(*expected), actual_rng(*actual);
    expected->memory.observer = &expected_rng;
    actual->memory.observer = &actual_rng;
    for (auto* state : {expected.get(), actual.get()}) {
        if (kind == InactivePlan::detailed) {
            // Exercise the quick events routine with a detailed match byte as
            // an explicit negative control for the simulation's mode gate.
            require(state->memory.read<1>(a.engine) == 0, "Detailed mode precondition");
            invoke(*state, quick_events_entry, {a.engine});
        } else play_quick(*state, a);
        state->memory.observer = nullptr;
    }
    require(!expected_rng.writes.empty() && expected_rng.writes == actual_rng.writes,
        "An inactive match result choice changed ordered match RNG writes");
    same_guest_result(*expected, *actual, "Inactive match result choice");
    select_native_debugging_features(result_feature);
}
}

int main(int argc, char** argv) {
    try {
        require(argc == 4, "Expected original program, read-only game and private check root");
        Cleanup cleanup{std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())};
        const auto world = prepare_round({argv[1], argv[2], cleanup.root});
        const Checkpoint initial(*world.state);
        check_choices(initial, world.first, world.second);
        check_round(initial, world.first, world.second);
        check_seeds(initial, world.first);
        check_cup(initial, world.first);
        check_load_boundary(initial, world.first, world.second);
        check_missing_scorer(initial, world.first);
        check_official_forfeit(initial, world.first);
        for (auto kind : {InactivePlan::other_fixture, InactivePlan::cleared, InactivePlan::disabled,
                          InactivePlan::watched, InactivePlan::detailed})
            check_inactive(initial, world.first, world.second, kind);
        std::cout << "PASS: independent queued fixture results, three choices across seeds, cup extra time and penalties, "
                     "scorer and keeper accounting, load reset, scorer fallback, official forfeit, "
                     "and inactive guest state and ordered RNG equivalence\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
