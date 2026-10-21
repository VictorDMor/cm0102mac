#include "match_debugger_factory.hpp"
#include "match_debugger_bootstrap.hpp"
#include "calendar.hpp"
#include <array>
#include <limits>
#include <stdexcept>
#include <tuple>

namespace cm::aot::match_debugger {
namespace {
std::uint32_t slot(const State& state, std::int32_t offset) {
    return std::uint32_t(state.memory.read<4>(std::uint32_t(state.r<4>(8) + offset)));
}
std::uint32_t invoke(State& state, std::uint32_t entry,
    std::initializer_list<std::uint32_t> arguments) {
    unsigned reg = 3;
    for (auto value : arguments) state.wr<4>(reg++ * 4, value);
    state.wr<4>(4128, return_sentinel);
    state.pc = entry;
    run_until(state, return_sentinel);
    return std::uint32_t(state.r<4>(12));
}

class ObserverScope {
    State& state_;
    MemoryObserver* previous_;
public:
    ObserverScope(State& state, MemoryObserver& observer)
        : state_(state), previous_(state.memory.observer) { state.memory.observer = &observer; }
    ~ObserverScope() { state_.memory.observer = previous_; }
};
struct StaffRecordsReady {};
class StaffRecordBoundary final : public MemoryObserver {
    const State& state_;
    std::uint32_t target_;
public:
    explicit StaffRecordBoundary(const State& state)
        : state_(state), target_(slot(state, -0x62dc)) {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t address, const std::uint8_t*, std::size_t size) override {
        if (address == target_ && size == 4 && state_.pc == 0x10344074) throw StaffRecordsReady{};
    }
};

// The exact instructions this file stops at or watches run alone in generated
// code, so the PC is theirs: tools/compile_recomp.py finds every PC comparison
// with a literal address in native/ (regenerate after adding one).
void initialize_staff_records(State& state) {
    // This original allocation/date/fitness/random region belongs to the
    // new-world routine. Its earlier career filters discard unselected clubs;
    // the independent experiment enters only its staff-record preparation.
    // Keep this remaining original dependency explicit until reconstruction.
    const auto previous_budget = state.budget, before = state.executed;
    state.wr<4>(4128, return_sentinel);
    state.pc = 0x10343630;
    state.budget = before + 22;
    try { run_until(state, return_sentinel); }
    catch (const std::runtime_error& error) {
        if (std::string_view(error.what()) != "Native instruction budget exhausted") throw;
    }
    if (state.pc != 0x10343688 || state.executed != before + 23)
        throw std::runtime_error("Original staff preparation did not reach its reserved frame");
    state.budget = previous_budget;
    StaffRecordBoundary boundary(state);
    bool ready = false;
    {
        ObserverScope observe(state, boundary);
        state.pc = 0x10343990;
        try { run_until(state, return_sentinel); }
        catch (const StaffRecordsReady&) { ready = true; }
    }
    if (!ready) throw std::runtime_error("Original live player records did not initialize");
    // The dependency stops before career club ranking/services. Restore the
    // reserved frame after all nested original calls have returned. This is a
    // host setup boundary, not a return claim for the whole career routine.
    const auto parent = std::uint32_t(state.r<4>(4)) + 0xc90;
    for (unsigned reg = 13; reg < 32; ++reg)
        state.wr<4>(reg * 4, state.memory.read<4>(parent - 0x4c + (reg - 13) * 4));
    state.wr<4>(0, state.memory.read<4>(parent + 8));
    state.wr<4>(4, parent);
    state.wr<4>(4128, state.r<4>(0));
    state.pc = return_sentinel;
}

void initialize_match_services(State& state) {
    // Preserve the characterized original construction order. There are no
    // active career competition objects in this independent world.
    constexpr std::array<std::tuple<std::uint32_t, std::uint32_t, std::int32_t>, 10> services{{
        {0x5cc, 0x103acf80, -0x7a6c}, {0x10, 0x10145af0, -0x7a54},
        {0x28a4, 0x1048ebb0, -0x7b54}, {0x30, 0x1000e6b0, -0x7afc},
        {0x48, 0x10301540, -0x7b3c}, {0x2c, 0x101840e0, -0x7ba0},
        {0x3c, 0x10077ee0, -0x7b7c},
        // Original injury/history queries call 1069e700 on this manager.
        // Its original empty constructor is required even without a career.
        {0x18, 0x10646d90, -0x7b94},
        {0xc, 0x10432540, -0x7aac}, {0x67bc, 0x103cfe50, -0x7a7c}}};
    for (auto [size, constructor, offset] : services) {
        const auto object = invoke(state, 0x10773520, {size});
        if (!object) throw std::runtime_error("Could not allocate an original match service");
        (void)invoke(state, constructor, {object, 0});
        state.memory.write<4>(slot(state, offset), object);
    }
    if (invoke(state, 0x10354cb0, {}) != 1)
        throw std::runtime_error("Could not initialize the original empty competition index");
}

class LineupBoundary final : public MemoryObserver {
    const State& state_;
public:
    explicit LineupBoundary(const State& state) : state_(state) {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t* bytes, std::size_t size) override {
        if (size != 1 || (state_.pc != 0x101d28ac && state_.pc != 0x101d2a60)) return;
        if (std::bit_cast<std::int8_t>(bytes[0]) < 11)
            throw std::runtime_error(state_.pc == 0x101d28ac ?
                "The home club cannot field eleven eligible players" : "The away club cannot field eleven eligible players");
    }
};

void bind_detailed_friendly(State& state, std::uint32_t match, std::uint32_t fixture) {
    // The original -2 statistics fixture prepares independent players without
    // career eligibility. Detailed end-of-match commentary instead queries a
    // real competition ID and player assessment reads the competition record.
    // Bind the existing original Friendly manager after player preparation;
    // do not feed the negative statistics ID into a competition array.
    const auto competition = std::uint32_t(state.memory.read<4>(slot(state, -0x7a50)));
    const auto count = state.memory.read<4>(slot(state, -0x7b60));
    const auto index = std::uint32_t(state.memory.read<4>(slot(state, -0x7b5c)));
    const auto manager = std::uint32_t(state.memory.read<4>(slot(state, -0x7b54)));
    const auto competition_record = manager ? std::uint32_t(state.memory.read<4>(manager + 4)) : 0;
    const auto official = std::uint32_t(state.memory.read<4>(slot(state, -0x7ab4)));
    if (competition >= count || !index || !manager ||
        state.memory.read<4>(index + competition * 4) != manager || !competition_record ||
        state.memory.read<4>(competition_record) != competition ||
        !state.memory.read<4>(slot(state, -0x7528)) || !official || state.memory.read<4>(official) != 0)
        throw std::runtime_error("Original friendly competition or match official is unavailable");
    // A repeatable sandbox default: the first actual original official. The
    // original statistics-fixture builder uses -1 for absent previous-leg goals
    // (10371bcc..10371bdc); zero incorrectly represents an aggregate fixture.
    state.memory.write<4>(fixture + 0x14, competition_record);
    state.memory.write<2>(fixture + 0x2e, 0);
    state.memory.write<1>(fixture + 0x4b, 0xff);
    state.memory.write<1>(fixture + 0x4c, 0xff);
    state.memory.write<4>(fixture, competition);
    // Isolated initialization deliberately passes no referee to 1021b020.
    // After binding a detailed friendly, construct its actual referee context
    // just as 101d2580..101d25a4 does for a scheduled match. Foul/card queries
    // read the resulting match + e034 pointer, not fixture + 14.
    (void)invoke(state, 0x1021b020, {match + 0xe030, match, official});
    if (state.memory.read<4>(match + 0xe034) != official)
        throw std::runtime_error("Original match referee context did not initialize");
}
}

EngineInstance create_match(const MatchInputs& inputs, const MatchSetup& setup,
    const CreationProgress& progress) {
    if (setup.home_club == setup.away_club || !setup.random_seed ||
        setup.year < 1972 || setup.year > 2100 || setup.ordinal_day >= cm::calendar::year_length(setup.year))
        throw std::invalid_argument("Invalid isolated match clubs, date or random seed");
    const auto report = [&](std::string_view stage) { if (progress) progress(stage); };
    report("Loading original club database");
    auto world = DatabaseWorld::load(inputs.original_program, inputs.original_game,
        inputs.new_private_directory, 600000000, setup.random_seed);
    for (auto id : {setup.home_club, setup.away_club}) {
        if (world.catalog().club(id).kind != ClubKind::club || world.roster(id).size() < 11)
            throw std::invalid_argument("Select a club with at least eleven original players");
    }
    auto& state = world.state();
    if (state.executed > std::numeric_limits<std::uint64_t>::max() - 4000000000ULL)
        throw std::runtime_error("Match creation instruction counter would overflow");
    state.budget = state.executed + 4000000000ULL;
    (void)invoke(state, 0x107156b0, {slot(state, -0x7b80), setup.year, setup.ordinal_day});
    (void)invoke(state, 0x107156b0, {slot(state, -0x7a9c), setup.year, setup.ordinal_day});
    report("Preparing original player condition records");
    initialize_staff_records(state);
    report("Preparing original match services");
    initialize_match_services(state);
    for (auto id : {setup.home_club, setup.away_club}) (void)world.roster(id);
    report("Selecting actual match lineups");
    state.memory.write<1>(slot(state, -0x642c), 1);
    const auto match = invoke(state, 0x10773520, {0xe0bc});
    if (!match) throw std::runtime_error("Could not allocate the original match record");
    (void)invoke(state, 0x101d16a0, {match});
    const auto fixture = allocate(state, 0x54, true);
    if (!fixture) throw std::runtime_error("Could not allocate the isolated fixture record");
    state.memory.write<4>(fixture, 0xfffffffe); // Original isolated/statistics fixture kind.
    state.memory.write<4>(fixture + 0x1c, world.club_pointer(setup.home_club));
    state.memory.write<4>(fixture + 0x20, world.club_pointer(setup.away_club));
    state.memory.write<4>(fixture + 0x24, state.memory.read<4>(world.club_pointer(setup.home_club) + 0x40));
    state.memory.write<2>(fixture + 0x28, setup.ordinal_day);
    state.memory.write<2>(fixture + 0x2a, setup.year);
    LineupBoundary lineup(state);
    {
        ObserverScope observe(state, lineup);
        if (invoke(state, 0x101d1760, {match, fixture, 0, 0, 0}) != 1)
            throw std::runtime_error("The original detailed match initializer did not complete");
    }
    if (state.memory.read<1>(match) || state.memory.read<2>(match + 0x7948))
        throw std::runtime_error("New match is not at its detailed pre-kickoff boundary");
    std::array<std::vector<RosterMember>, 2> rosters{world.roster(setup.home_club), world.roster(setup.away_club)};
    report("Preparing substitutes");
    for (unsigned team = 0; team < 2; ++team) {
        unsigned actor = 11;
        for (const auto& member : rosters[team]) {
            if (actor == 20) break;
            bool assigned = false;
            for (unsigned starter = 0; starter < 11; ++starter)
                assigned |= state.memory.read<4>(match + 0x311c + (team * 20 + starter) * 0x1c4 + 0x6c) == member.staff;
            if (assigned) continue;
            if (invoke(state, 0x1020e8f0, {match + 0x7aac + team * 0xd8, actor, member.staff, actor}) != 1)
                throw std::runtime_error("Could not prepare an actual substitute");
            const auto address = match + 0x311c + (team * 20 + actor) * 0x1c4;
            const auto position = signed_value<1>(state.memory.read<1>(address + 0x19));
            if (position >= 0) (void)invoke(state, 0x101f1870, {address, 0xffffffff});
            ++actor;
        }
    }
    bind_detailed_friendly(state, match, fixture);
    report("Match ready and paused");
    return {std::move(world).release_state(), match, std::move(rosters)};
}
}
