#include "match_debugger_goal.hpp"
#include <stdexcept>

namespace cm::aot::match_debugger {
namespace {
struct GoalSetupReady {};
struct GoalStatisticsReady {};

class Boundaries final : public MemoryObserver {
    State& state_;
    MemoryObserver* previous_;
    std::uint32_t match_;
public:
    enum class Stage { setup, statistics, confirmation } stage = Stage::setup;
    Boundaries(State& state, std::uint32_t match)
        : state_(state), previous_(state.memory.observer), match_(match) { state.memory.observer = this; }
    ~Boundaries() { state_.memory.observer = previous_; }
    void mapped(std::uint32_t address, std::uint64_t length) override {
        if (previous_) previous_->mapped(address, length);
    }
    void cleared(std::uint32_t address, std::uint64_t length) override {
        if (previous_) previous_->cleared(address, length);
    }
    // Each watched instruction runs alone in generated code, so the PC is
    // exactly theirs here (tools/compile_recomp.py scans for these).
    void written(std::uint32_t address, const std::uint8_t* bytes, std::size_t length) override {
        if (previous_) previous_->written(address, bytes, length);
        if (stage == Stage::setup && state_.pc == 0x102578d8 && address == match_ + 0x77c2 && length == 2)
            throw GoalSetupReady{};
        if (stage == Stage::statistics && state_.pc == 0x10263614 && address == match_ + 0x3108 && length == 4)
            throw GoalStatisticsReady{};
    }
};
}

void apply_original_awarded_goal(State& state, std::uint32_t match, std::uint32_t actor, unsigned team) {
    const auto parent = std::uint32_t(state.r<4>(4));
    if (parent < 0x300) throw std::runtime_error("Invalid original goal caller stack");
    const auto caller = parent - 0x80, context = caller + 0x50;
    for (unsigned offset = 0; offset < 16; offset += 4) state.memory.write<4>(context + offset, 0);
    state.wr<4>(4, caller);
    unsigned reg = 3;
    for (auto value : {actor, 0x16u, 1u, 0x1f86u, 1u, 0u, context}) state.wr<4>(reg++ * 4, value);
    state.wr<4>(4128, return_sentinel);
    state.pc = 0x102576e0;
    Boundaries boundaries(state, match);
    bool ready = false;
    try { run_until(state, return_sentinel); } catch (const GoalSetupReady&) { ready = true; }
    if (!ready || state.r<4>(4) != caller - 0x1b0 || state.r<4>(60) != actor)
        throw std::runtime_error("Original goal setup did not reach its reserved live frame");

    // Resolve the actual opposing goalkeeper through the original selector.
    // The normal action's movement, chance and rejection decisions are outside
    // an intentional awarded-goal intervention. No score/counter is host-edited.
    state.wr<4>(12, match); state.wr<4>(16, 1 - team);
    state.wr<4>(4128, return_sentinel); state.pc = 0x101f1e10;
    run_until(state, return_sentinel);
    const auto keeper = state.r<4>(12);
    if (!keeper || state.r<4>(4) != caller - 0x1b0)
        throw std::runtime_error("Original goal operation could not resolve its live goalkeeper");
    state.wr<4>(96, keeper);
    boundaries.stage = Boundaries::Stage::statistics;
    state.pc = 0x102626bc;
    ready = false;
    try { run_until(state, return_sentinel); } catch (const GoalStatisticsReady&) { ready = true; }
    if (!ready || state.r<4>(4) != caller - 0x1b0)
        throw std::runtime_error("Original awarded-goal statistics did not reach their boundary");

    boundaries.stage = Boundaries::Stage::confirmation;
    state.pc = 0x10267154;
    run_until(state, return_sentinel);
    if (state.pc != return_sentinel || state.r<4>(4) != caller)
        throw std::runtime_error("Original awarded goal did not return through its own epilogue");
    state.wr<4>(4, parent);
}
}
