#include "debug_underdogs.hpp"
#include "aot_runtime.hpp"
#include "match_debugging.hpp"
#include <algorithm>
#include <cmath>
#include <unordered_set>

namespace cm::aot {
namespace {
bool matches(const State& s, const UnderdogFixture& plan, std::uint32_t fixture) {
    // The queue keeps the original pointer at +0 and a copy of the core at
    // +4. The screen lists the former; both engine paths simulate the copy.
    return s.memory.read<4>(fixture + 0x1c) == plan.home &&
        s.memory.read<4>(fixture + 0x20) == plan.away && s.memory.read<4>(fixture + 0x28) == plan.date &&
        s.memory.read<4>(fixture) == plan.competition && s.memory.read<4>(fixture + 0x30) == plan.round;
}
// The 30 player-record attributes in the original profile's 31-field getter
// (100d2330); Determination is in the staff record at +0x64.
enum class Scale { raw, ability, goalkeeper_tenth, outfield_tenth };
struct Attribute { unsigned offset; Scale scale; };
constexpr Attribute attributes[] = {
    {0x1b,Scale::raw}, {0x1c,Scale::raw}, {0x1d,Scale::raw}, {0x1e,Scale::ability},
    {0x1f,Scale::raw}, {0x20,Scale::raw}, {0x43,Scale::goalkeeper_tenth}, {0x23,Scale::goalkeeper_tenth},
    {0x24,Scale::ability}, {0x26,Scale::goalkeeper_tenth}, {0x27,Scale::goalkeeper_tenth},
    {0x28,Scale::raw}, {0x2a,Scale::outfield_tenth}, {0x2b,Scale::ability}, {0x2f,Scale::raw},
    {0x2e,Scale::raw}, {0x31,Scale::ability}, {0x32,Scale::goalkeeper_tenth}, {0x33,Scale::goalkeeper_tenth},
    {0x36,Scale::raw}, {0x37,Scale::ability}, {0x39,Scale::ability}, {0x3a,Scale::outfield_tenth},
    {0x29,Scale::raw}, {0x3c,Scale::raw}, {0x3d,Scale::raw}, {0x3e,Scale::ability},
    {0x3f,Scale::raw}, {0x40,Scale::raw}, {0x44,Scale::raw}
};
std::uint8_t encoded(const State& s, std::uint32_t player, Attribute attribute, unsigned target) {
    if (attribute.scale == Scale::raw) return std::uint8_t(target);
    const auto goalkeeper = std::int8_t(s.memory.read<1>(player + 0xf));
    const bool tenth = (attribute.scale == Scale::goalkeeper_tenth && goalkeeper == 20) ||
        (attribute.scale == Scale::outfield_tenth && goalkeeper < 15);
    const double ability = std::int16_t(s.memory.read<2>(player + 4)) * (tenth ? 0.1 : 1.0);
    // Original display: r=10+(ability+2*byte)/20, floor(.5+(r+r*r/10)/3).
    // x=200 is inside the 20 interval; x=-35 is inside the 5 interval.
    // Keep current/potential ability and position ratings unchanged.
    const auto value = int(std::lround(((target == 20 ? 200.0 : -35.0) - ability) / 2.0));
    return std::uint8_t(std::clamp(value, -128, 127));
}
}

bool arm_debug_underdogs(State& s, std::span<const std::uint32_t> fixtures, UnderdogMode mode) {
    if (!native_debugging_enabled()) return false;
    auto& plans = underdog_debugging(s).fixtures;
    bool accepted = false;
    for (auto fixture : fixtures) {
        if (!fixture || std::int8_t(s.memory.read<1>(fixture + 0x43)) != -1) continue;
        const auto home = std::uint32_t(s.memory.read<4>(fixture + 0x1c));
        const auto away = std::uint32_t(s.memory.read<4>(fixture + 0x20));
        if (!home || !away || home == away || s.memory.read<2>(home + 0x1ec) == s.memory.read<2>(away + 0x1ec)) continue;
        std::erase_if(plans, [&](const auto& p) { return p.fixture == fixture; });
        plans.push_back({fixture, home, away, std::uint32_t(s.memory.read<4>(fixture + 0x28)),
            std::uint32_t(s.memory.read<4>(fixture)), std::uint32_t(s.memory.read<4>(fixture + 0x30)), mode});
        accepted = true;
    }
    return accepted;
}
void finish_debug_underdogs(State& s, std::uint32_t fixture) {
    if (!underdog_debugging(static_cast<const State&>(s)).fixtures.empty())
        std::erase_if(underdog_debugging(s).fixtures, [&](const auto& p) { return matches(s, p, fixture); });
}
std::uint32_t debug_underdog_fixture(const State& s, std::uint32_t match) {
    if (!match || underdog_debugging(s).fixtures.empty()) return 0;
    return std::uint32_t(s.memory.read<4>(match + 0x3118));
}

DebugUnderdogScope::DebugUnderdogScope(State& s, std::uint32_t fixture) : state(s) {
    const auto& data = underdog_debugging(static_cast<const State&>(s));
    if (!fixture || data.fixtures.empty() || data.depth) return;
    const auto plan = std::find_if(data.fixtures.begin(), data.fixtures.end(),
        [&](const auto& p) { return matches(s, p, fixture); });
    if (plan == data.fixtures.end()) return;
    const auto home_rep = s.memory.read<2>(plan->home + 0x1ec), away_rep = s.memory.read<2>(plan->away + 0x1ec);
    if (home_rep == away_rep) return;
    const auto weaker = home_rep < away_rep ? plan->home : plan->away;
    const auto stronger = home_rep < away_rep ? plan->away : plan->home;
    std::unordered_set<std::uint32_t> addresses;
    const auto add = [&](std::uint32_t address, std::uint8_t value) {
        if (addresses.insert(address).second)
            bytes.push_back({address, std::uint8_t(s.memory.read<1>(address)), value});
    };
    for (auto club : {weaker, stronger}) {
        if (club == stronger && plan->mode != UnderdogMode::maximum) continue;
        const unsigned target = club == weaker ? 20 : 5;
        for (unsigned slot = 0; slot < 50; ++slot) {
            const auto staff = s.memory.read<4>(club + 0x98 + slot * 4);
            if (!staff) continue;
            const auto player = std::uint32_t(s.memory.read<4>(staff + 0x14));
            if (!player) continue;
            for (const auto attribute : attributes) add(player + attribute.offset, encoded(s, player, attribute, target));
            add(std::uint32_t(staff) + 0x64, std::uint8_t(target));
        }
    }
    // Read and allocate before the first write. A failed setup cannot leave a
    // half-modified roster, and nested player constructors share this scope.
    try {
        for (const auto& b : bytes) s.memory.write<1>(b.address, b.during);
    } catch (...) {
        for (const auto& b : bytes) s.memory.write<1>(b.address, b.before);
        throw;
    }
    ++underdog_debugging(s).depth;
    entered = true;
}
DebugUnderdogScope::~DebugUnderdogScope() {
    if (!entered) return;
    for (const auto& b : bytes) state.memory.write<1>(b.address, b.before);
    --underdog_debugging(state).depth;
}
}
