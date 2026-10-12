#include "match_debugger_cards.hpp"
#include <stdexcept>

namespace cm::aot::match_debugger {
namespace {
std::uint32_t invoke(State& state, std::uint32_t entry, std::initializer_list<std::uint32_t> arguments) {
    unsigned reg = 3;
    for (const auto argument : arguments) state.wr<4>(reg++ * 4, argument);
    state.wr<4>(4128, return_sentinel); state.pc = entry;
    run_until(state, return_sentinel);
    if (state.pc != return_sentinel) throw std::runtime_error("Original discipline operation did not return");
    return std::uint32_t(state.r<4>(12));
}
void refresh_team(State& state, std::uint32_t match, unsigned side) {
    const auto controller = match + 0x7aac + side * 0xd8;
    (void)invoke(state, 0x102104d0, {controller});
    (void)invoke(state, 0x1020ef70, {controller, 0});
    (void)invoke(state, 0x101f1de0, {match, side});
    (void)invoke(state, 0x101f1e10, {match, side});
    state.memory.write<2>(match + 0x7944 + side * 2, state.memory.read<2>(match + 0x7948));
}
}

void apply_original_card(State& state, std::uint32_t match, std::uint32_t actor,
                         std::uint32_t opponent, bool yellow) {
    const auto parent = std::uint32_t(state.r<4>(4));
    if (parent < 0x200) throw std::runtime_error("Invalid discipline caller stack");
    const auto caller = parent - 0x80, decision = caller + 0x50, consequence = caller + 0x54;
    state.wr<4>(4, caller);
    state.memory.write<1>(decision, 1);
    state.memory.write<1>(caller + 0x3b, 0); // Ninth original argument.
    // In the complete original method, severity 6 exceeds every yellow
    // threshold (0..5), but no direct-red threshold (7..21). Severity 127
    // exceeds every direct-red threshold. Its RNG, rating, reasons and foul
    // statistics still execute normally; an injected card is guaranteed.
    (void)invoke(state, 0x10221e00, {match + 0xe030, actor, opponent, decision, 1,
        yellow ? 6u : 127u, 0, consequence});
    const auto outcome = state.memory.read<1>(decision);
    if ((yellow && outcome != 2 && outcome != 3) || (!yellow && outcome != 4))
        throw std::runtime_error("Original discipline did not award the requested card");
    const auto event = outcome == 2 ? 8185u : outcome == 3 ? 8183u : 8184u;
    (void)invoke(state, 0x10201180, {std::uint32_t(state.memory.read<4>(match + 0x79f4)), event, match,
        std::uint32_t(state.memory.read<1>(actor + 0x2f)), std::uint32_t(state.memory.read<1>(actor + 0x27)),
        0xffffffff, 0xffffffff, 0});
    if (outcome != 2) (void)invoke(state, 0x101f1530, {match, actor, 0});
    if (state.r<4>(4) != caller) throw std::runtime_error("Original discipline lost its caller frame");
    state.wr<4>(4, parent);
}

void apply_original_injury(State& state, std::uint32_t match, std::uint32_t actor) {
    // Original pending injury flags, followed by the complete injury processor.
    // Unlike dismissal, this does not increment actor +0x23 (red cards).
    state.memory.write<1>(actor + 0x1c, 1);
    state.memory.write<1>(actor + 0x1d, 1);
    (void)invoke(state, 0x101f6c60, {match});
}

void return_dismissed_player(State& state, std::uint32_t match, std::uint32_t actor, unsigned position) {
    const auto side = unsigned(state.memory.read<1>(actor + 0x27));
    (void)invoke(state, 0x101f1870, {actor, position});
    // Rebuild cell membership through the original re-entry method; restoring
    // a position byte alone leaves an actor absent from the live pitch lists.
    (void)invoke(state, 0x101f1880, {match, actor});
    refresh_team(state, match, side);
}
}
