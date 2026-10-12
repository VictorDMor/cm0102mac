#include "match_debugger_factory.hpp"
#include "match_debugger_checkpoint.hpp"
#include "semantic_io_checks.hpp"
#include <chrono>

namespace {
using namespace cm::semantic_io_checks;
using namespace cm::aot::match_debugger;
std::uint32_t actor(const Session& s, unsigned side, unsigned index) {
    return s.match_address() + 0x311c + (side * 20 + index) * 0x1c4;
}
template<class Action> void rejects(Session& s, Action action) {
    const Image before(s.state()); const auto timeline = s.timeline_size(), edits = s.card_edits().size(), events = s.events().size();
    bool rejected = false; try { action(); } catch (const std::logic_error&) { rejected = true; }
    require(rejected, "Invalid card control was accepted"); before.compare(s.state(), "Rejected card control");
    require(s.timeline_size() == timeline && s.card_edits().size() == edits && s.events().size() == events,
        "Rejected card control changed history");
}
void pitch(const Session& s, unsigned team, unsigned count) {
    require(s.on_pitch(team) == count, "Card did not change the actual on-pitch count");
    std::array<bool, 11> positions{};
    for (unsigned index = 0; index < 20; ++index) {
        const auto a = actor(s, team, index);
        const auto role = signed_value<1>(s.state().memory.read<1>(a + 0x19));
        if (role < 0) continue;
        require(role < 11 && !positions[unsigned(role)], "Card return left duplicate positions"); positions[unsigned(role)] = true;
        const auto x = s.state().memory.read<1>(a + 0x105), y = s.state().memory.read<1>(a + 0x106);
        const auto cell = s.match_address() + 0xa08 + x * 0x450 + y * 0x5c;
        const auto slot = s.state().memory.read<1>(a + 0x28);
        require(slot < s.state().memory.read<1>(cell + 0x58 + team) &&
            s.state().memory.read<4>(cell + team * 0x2c + slot * 4) == a, "Player is missing from its actual pitch cell");
    }
}
void original_yellow(State& state, std::uint32_t match, std::uint32_t selected, std::uint32_t opponent) {
    const auto parent = std::uint32_t(state.r<4>(4)), caller = parent - 0x80;
    state.wr<4>(4, caller); state.memory.write<1>(caller + 0x50, 1); state.memory.write<1>(caller + 0x3b, 0);
    const auto call = [&](std::uint32_t entry, std::initializer_list<std::uint32_t> args) {
        unsigned reg = 3; for (auto arg : args) state.wr<4>(reg++ * 4, arg);
        state.wr<4>(4128, return_sentinel); state.pc = entry; run_until(state, return_sentinel);
    };
    call(0x10221e00, {match + 0xe030, selected, opponent, caller + 0x50, 1, 6, 0, caller + 0x54});
    require(state.memory.read<1>(caller + 0x50) == 2, "Independent original yellow did not return its outcome");
    call(0x10201180, {std::uint32_t(state.memory.read<4>(match + 0x79f4)), 8185, match,
        std::uint32_t(state.memory.read<1>(selected + 0x2f)), std::uint32_t(state.memory.read<1>(selected + 0x27)),
        0xffffffff, 0xffffffff, 0});
    state.wr<4>(4, parent);
}
struct Cleanup {
    std::filesystem::path root;
    ~Cleanup() {
        if (!std::filesystem::exists(root) || std::filesystem::is_symlink(root)) return;
        for (const auto& entry : std::filesystem::recursive_directory_iterator(root)) if (entry.is_symlink()) return;
        std::error_code ignored; std::filesystem::remove_all(root, ignored);
    }
};
}
int main(int argc, char** argv) {
    try {
        require(argc == 4, "Expected original program, game and private check root");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        Cleanup cleanup{root};
        Session s([&] { return create_match({argv[1], argv[2], root / "world"}, {677, 5677, 12345, 2001, 194}); }, fresh_match_step_limit);
        rejects(s, [&] { s.change_card(0, 8, CardKind::yellow, true); });
        (void)s.step();
        rejects(s, [&] { s.change_card(2, 8, CardKind::yellow, true); });
        rejects(s, [&] { s.change_card(0, 20, CardKind::red, true); });
        rejects(s, [&] { s.change_card(0, 8, CardKind::red, false); });
        rejects(s, [&] { s.change_card(0, 11, CardKind::yellow, true); });
        s.run(); rejects(s, [&] { s.change_card(0, 8, CardKind::yellow, true); }); s.pause();
        const Image kickoff(s.state()); const auto events = s.events().size(); const auto budget = s.state().budget;
        auto original = Checkpoint(s.state()).restore();
        const auto old_budget = original->budget; original->budget = original->executed + fresh_match_step_limit;
        original_yellow(*original, s.match_address(), actor(s, 0, 8), actor(s, 1, 8)); original->budget = old_budget;
        const auto first = s.change_card(0, 8, CardKind::yellow, true);
        Image(*original).compare(s.state(), "Whole original yellow ABI/effects");
        require(original->executed == s.state().executed && original->native_calls == s.state().native_calls,
            "Original yellow accounting/imports differ");
        require(first.before == Cards{} && first.after == Cards{1, 0} && first.instructions > 0,
            "Original first-yellow bookkeeping is incorrect"); pitch(s, 0, 11);
        const Image booked(s.state()); s.back(); kickoff.compare(s.state(), "Undo yellow");
        require(s.card_edits().empty() && s.events().size() == events, "Undo retained a future card event");
        s.forward(); booked.compare(s.state(), "Redo yellow");
        const auto second = s.change_card(0, 8, CardKind::yellow, true);
        require(second.after == Cards{2, 1} && signed_value<1>(s.state().memory.read<1>(actor(s, 0, 8) + 0x19)) == -3,
            "Second yellow did not cause original dismissal"); pitch(s, 0, 10);
        rejects(s, [&] { s.change_card(0, 8, CardKind::red, true); });
        (void)s.step();
        const Image dismissed(s.state()); const auto clock = s.inspect().elapsed_engine_units;
        const auto removed = s.change_card(0, 8, CardKind::yellow, false);
        require(removed.after == Cards{1, 0} && s.inspect().elapsed_engine_units == clock && s.state().budget == budget,
            "Canceling second yellow did not cancel dismissal while preserving time/budget"); pitch(s, 0, 11);
        const Image returned(s.state()); s.back(); dismissed.compare(s.state(), "Undo card removal");
        s.forward(); returned.compare(s.state(), "Redo card removal");
        (void)s.change_card(0, 8, CardKind::yellow, false); require(s.cards(0, 8) == Cards{}, "Yellow removal failed");
        (void)s.add_goal(0, 8);
        for (const auto selection : {std::pair{0u, 8u}, std::pair{1u, 0u}}) {
            const auto [team, index] = selection;
            const auto before = s.inspect();
            (void)s.change_card(team, index, CardKind::red, true); pitch(s, team, 10);
            require(s.cards(team, index) == Cards{0, 1}, "Direct red created an unwanted yellow");
            (void)s.step();
            const auto later = s.inspect();
            (void)s.change_card(team, index, CardKind::red, false); pitch(s, team, 11);
            require(s.cards(team, index) == Cards{} && s.inspect() == later && later.home_score >= before.home_score,
                "Red removal rewound the score, time or current match state");
            (void)s.step();
        }
        // A card received through original gameplay is equally editable; card
        // inspection reads guest fields rather than an injected-card overlay.
        auto& state = const_cast<State&>(s.state());
        state.memory.write<1>(actor(s, 1, 8) + 0x24, 1);
        (void)s.step();
        require(s.cards(1, 8).yellow == 1, "Original yellow is hidden by the debugger");
        (void)s.change_card(1, 8, CardKind::yellow, false);
        s.back(); (void)s.change_card(1, 8, CardKind::red, true);
        require(s.cards(1, 8) == Cards{1, 1}, "Branch direct-red after yellow is incorrect");
        (void)s.change_card(1, 8, CardKind::red, false); pitch(s, 1, 11);
        require(s.cards(1, 8) == Cards{1, 0}, "Removing direct red lost the first yellow");
        (void)s.change_card(1, 8, CardKind::yellow, true);
        (void)s.change_card(1, 8, CardKind::red, false);
        require(s.cards(1, 8) == Cards{1, 0}, "Removing second-yellow red left an invalid two-yellow state");
        s.seek_seconds(0); require(s.card_edits().empty() && s.cards(0, 8) == Cards{} && s.on_pitch(0) == 11,
            "Seeking kickoff retained card edits");
        std::cout << "PASS: real yellow/direct-red/second-yellow additions, guest yellow removal, dismissal cancellation, outfield/keeper pitch membership, following engine calls, score/time preservation, complete undo/redo/branch/seek and rejected edits\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
