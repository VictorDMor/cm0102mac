#pragma once
#include <array>
#include <cstdint>
#include <span>
#include <vector>

namespace cm::aot {
struct State;
enum class UnderdogMode { normal, maximum };
struct UnderdogFixture {
    std::uint32_t fixture, home, away, date, competition, round;
    UnderdogMode mode;
};
// Owned by State, never serialized or shared with another game/replay.
struct UnderdogDebugging {
    std::vector<UnderdogFixture> fixtures;
    unsigned depth = 0;
};
UnderdogDebugging& underdog_debugging(State&);
const UnderdogDebugging& underdog_debugging(const State&);
bool arm_debug_underdogs(State&, std::span<const std::uint32_t> fixtures, UnderdogMode);
void finish_debug_underdogs(State&, std::uint32_t fixture);
std::uint32_t debug_underdog_fixture(const State&, std::uint32_t match);

// Temporary database values are visible only to match setup, player creation
// and simulation. They cannot leak into saves, training or transfer valuation.
// Every exit, including exceptions and halftime, restores the original bytes;
// the fixture remains armed until actual completion, including extra time.
class DebugUnderdogScope {
    State& state;
    struct Byte { std::uint32_t address; std::uint8_t before, during; };
    std::vector<Byte> bytes;
    bool entered = false;
public:
    DebugUnderdogScope(State&, std::uint32_t fixture);
    ~DebugUnderdogScope();
    DebugUnderdogScope(const DebugUnderdogScope&) = delete;
    DebugUnderdogScope& operator=(const DebugUnderdogScope&) = delete;
};
}
