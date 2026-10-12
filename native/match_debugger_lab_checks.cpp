#include "match_debugger_factory.hpp"
#include "match_debugger_checkpoint.hpp"
#include "platform.hpp"
#include "semantic_io_checks.hpp"
#include <chrono>

namespace {
using namespace cm::semantic_io_checks;
using namespace cm::aot::match_debugger;
std::uint32_t invoke(State& state, std::uint32_t entry, std::initializer_list<std::uint32_t> arguments) {
    unsigned reg = 3;
    for (auto argument : arguments) state.wr<4>(reg++ * 4, argument);
    state.wr<4>(4128, return_sentinel); state.pc = entry;
    run_until(state, return_sentinel); return std::uint32_t(state.r<4>(12));
}
std::uint32_t actor(const Session& session, unsigned side, unsigned slot) {
    return session.match_address() + 0x311c + (side * 20 + slot) * 0x1c4;
}
std::uint32_t staff_id(const Session& session, unsigned side, unsigned slot) {
    const auto staff = session.state().memory.read<4>(actor(session, side, slot) + 0x6c);
    return staff ? std::uint32_t(session.state().memory.read<4>(staff)) : 0;
}
struct OwnedBacking {
    std::filesystem::path root;
    ~OwnedBacking() {
        std::error_code error;
        if (!std::filesystem::exists(root, error) || error || std::filesystem::is_symlink(root, error) || error) return;
        for (const auto& item : std::filesystem::recursive_directory_iterator(root, error))
            if (error || item.is_symlink(error) || error) return;
        std::filesystem::remove_all(root, error);
    }
};
template<class Action> void reject_without_effects(Session& session, Action action) {
    const Image before(session.state());
    const auto size = session.timeline_size(), cursor = session.timeline_index(), event_count = session.events().size();
    bool rejected = false; try { action(); } catch (const std::logic_error&) { rejected = true; }
    require(rejected, "Invalid lab control was not rejected");
    before.compare(session.state(), "Rejected lab control");
    require(size == session.timeline_size() && cursor == session.timeline_index() && event_count == session.events().size(),
        "Rejected lab control changed its timeline");
}
}

int main(int argc, char** argv) {
    try {
        require(argc == 4, "Expected original program, game and private check root");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        OwnedBacking cleanup{root};
        unsigned generation = 0;
        Session session([&] { return create_match({argv[1], argv[2], root / ("world-" + std::to_string(generation++))},
            {677, 5677, 12345, 2001, 194}); }, fresh_match_step_limit);
        {
            auto isolated = Checkpoint(session.state()).restore();
            constexpr std::uint32_t event_record = 0x30000000;
            isolated->memory.map(event_record, 16);
            for (const auto* operation : {"CarbonLib::WaitNextEvent", "CarbonLib::GetNextEvent"}) {
                isolated->memory.copy_in(event_record, std::vector<std::uint8_t>(16, 0xff));
                isolated->wr<4>(12, 0xffff); isolated->wr<4>(16, event_record);
                isolated->wr<4>(20, 60000); isolated->wr<4>(24, 0);
                const auto started = std::chrono::steady_clock::now();
                require(isolated->platform->dispatch(*isolated, operation) && isolated->r<4>(12) == 0 &&
                    isolated->memory.read<2>(event_record) == 0 && isolated->memory.read<4>(event_record + 2) == 0 &&
                    isolated->memory.read<4>(event_record + 10) == 0 && isolated->memory.read<2>(event_record + 14) == 0 &&
                    std::chrono::steady_clock::now() - started < std::chrono::seconds(1),
                    std::string(operation) + " waited for host input or returned a malformed isolated null event");
            }
        }
        require(session.timeline_size() == 1 && session.timeline_index() == 0 && clock_seconds(session.inspect()) == 0,
            "Initial lab timeline or clock is incorrect");
        for (unsigned side = 0; side < 2; ++side) for (unsigned slot = 11; slot < 20; ++slot)
            require(staff_id(session, side, slot) && signed_value<1>(session.state().memory.read<1>(actor(session, side, slot) + 0x19)) < 0,
                "Actual substitutes are missing from the bench");
        reject_without_effects(session, [&] { session.back(); });
        reject_without_effects(session, [&] { session.substitute(0, 8, staff_id(session, 0, 11)); });
        (void)session.step();
        require(clock_seconds(session.inspect()) > 0 && !session.events().empty(), "Kickoff/time is not visible");
        const Image kickoff(session.state());
        const auto kickoff_instructions = session.state().executed, caller_budget = session.state().budget;
        const auto kickoff_events = session.events().size();
        (void)session.add_goal(0, 8);
        const Image goal(session.state());
        const auto goal_instructions = session.state().executed;
        require(session.inspect().home_score == 1 && session.goals().size() == 1 && session.events().size() > kickoff_events,
            "Original injected goal/event is missing");
        session.back(); kickoff.compare(session.state(), "Undo awarded goal");
        require(session.state().executed == kickoff_instructions && session.state().budget == caller_budget,
            "Undo lost the caller instruction accounting/budget");
        require(session.goals().empty() && session.events().size() == kickoff_events, "Undo retained future goal/history");
        session.forward(); goal.compare(session.state(), "Redo awarded goal");
        require(session.state().executed == goal_instructions && session.state().budget == caller_budget,
            "Redo lost the original instruction accounting/budget");
        session.back(); (void)session.add_goal(1, 8);
        require(session.inspect().home_score == 0 && session.inspect().away_score == 1 && session.timeline_size() == 3,
            "Editing the past did not create an independent branch");
        const auto incoming = staff_id(session, 0, 11), outgoing = staff_id(session, 0, 8);
        const Checkpoint before_substitution(session.state());
        auto original = before_substitution.restore();
        const auto previous_budget = original->budget;
        original->budget = original->executed + fresh_match_step_limit;
        const auto context = std::uint32_t(original->r<4>(4)) - 0x1000;
        original->memory.write<4>(context + 4, session.match_address()); original->memory.write<1>(context + 0xe68, 0);
        const auto allowance = session.match_address() + 0x7aac + 0x19;
        if (signed_value<1>(original->memory.read<1>(allowance)) <= 0) original->memory.write<1>(allowance, 1);
        require(invoke(*original, 0x103cb760, {context, 11, 8}) == 1, "Direct original substitution did not complete");
        (void)invoke(*original, 0x101f1de0, {session.match_address(), 0});
        (void)invoke(*original, 0x101f1e10, {session.match_address(), 0}); original->budget = previous_budget;
        const auto replacement = session.substitute(0, 8, incoming);
        Image(*original).compare(session.state(), "Complete original substitution ABI/effects");
        require(original->executed == session.state().executed && original->budget == session.state().budget,
            "Original substitution instruction accounting/budget differs");
        require(replacement.outgoing_staff_id == outgoing && replacement.incoming_staff_id == incoming && session.on_pitch(0) == 11 &&
            signed_value<1>(session.state().memory.read<1>(actor(session, 0, 8) + 0x19)) == -4 &&
            signed_value<1>(session.state().memory.read<1>(actor(session, 0, 11) + 0x19)) == replacement.position,
            "Replacement did not change the actual engine actors");
        const Image substituted(session.state());
        session.back(); Image(*before_substitution.restore()).compare(session.state(), "Undo actual substitution");
        session.forward(); substituted.compare(session.state(), "Redo actual substitution");
        reject_without_effects(session, [&] { session.substitute(0, 11, incoming); });
        // Re-entry and more than three substitutions deliberately exercise
        // unrestricted debugger use of the original in-match operation.
        unsigned on_pitch = 11;
        for (unsigned count = 0; count < 5; ++count) {
            const auto next = on_pitch == 11 ? 8u : 11u;
            (void)session.substitute(0, on_pitch, staff_id(session, 0, next)); on_pitch = next;
        }
        require(session.substitutions().size() == 6 && session.on_pitch(0) == 11, "Substitution limit or re-entry was imposed");
        require(signed_value<1>(session.state().memory.read<1>(allowance)) >= 0, "Unlimited substitutions underflowed the original allowance");
        try {
            (void)session.withdraw_injured_player(0, 0);
            require(session.on_pitch(0) == 10, "Goalkeeper injury did not remove its player");
            reject_without_effects(session, [&] { session.substitute(0, 0, staff_id(session, 0, 0)); });
            std::uint32_t new_player = 0;
            for (const auto& member : session.roster(0)) {
                bool represented = false;
                for (unsigned slot = 0; slot < 20; ++slot) represented |= staff_id(session, 0, slot) == member.staff_id;
                if (!represented) { new_player = member.staff_id; break; }
            }
            require(new_player != 0, "No unrepresented real roster member is available");
            const auto keeper = session.substitute(0, 0, new_player);
            require(keeper.incoming_actor != 0 && session.on_pitch(0) == 11 &&
                session.state().memory.read<1>(actor(session, 0, keeper.incoming_actor) + 0x19) == 0,
                "An injured goalkeeper could not be replaced freely");
            (void)session.step();
        } catch (...) {
            std::cerr << session.failure() << " PC=" << hex_address(session.state().pc)
                << " SP=" << hex_address(std::uint32_t(session.state().r<4>(4)))
                << " LR=" << hex_address(std::uint32_t(session.state().r<4>(4128)))
                << " r3=" << hex_address(std::uint32_t(session.state().r<4>(12)))
                << " r4=" << hex_address(std::uint32_t(session.state().r<4>(16)))
                << " r5=" << hex_address(std::uint32_t(session.state().r<4>(20))) << '\n';
            throw;
        }
        const auto last_seconds = clock_seconds(session.inspect());
        const Image last(session.state());
        session.seek_seconds(0);
        require(session.steps().empty() && session.goals().empty() && session.substitutions().empty() && session.inspect().home_score == 0 &&
            session.inspect().away_score == 0 && session.on_pitch(0) == 11, "Seeking kickoff retained later effects");
        session.seek_seconds(last_seconds); last.compare(session.state(), "Seeking the recorded future");
        session.seek_seconds(last_seconds + 60);
        require(clock_seconds(session.inspect()) >= last_seconds + 60 || session.steps().back().engine_return != 1,
            "Future seek neither advanced the real engine nor stopped at its boundary");
        bool commentary = false, intervention = false;
        for (const auto& event : session.events()) {
            commentary |= event.kind == MatchEvent::Kind::commentary; intervention |= event.injected;
        }
        require(commentary && intervention, "Committed original match events or injections are absent");
        Checkpoint checkpoint(session.state());
        Checkpoint same(session.state(), &checkpoint);
        require(same.newly_owned_bytes(&checkpoint) < checkpoint.newly_owned_bytes() / 2,
            "Unchanged memory pages are not shared between timeline checkpoints");
        std::cout << "PASS: actual reserves, unrestricted original substitutions/re-entry, complete guest-state undo/redo/branch/seek, real clock and committed match events; history remains in memory\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
