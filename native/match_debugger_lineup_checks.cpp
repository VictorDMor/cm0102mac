#include "match_debugger_factory.hpp"
#include "match_engine_adapter.hpp"
#include "match_actor_adapter.hpp"
#include <chrono>
#include <iostream>
#include <stdexcept>

namespace {
using namespace cm::aot;
using namespace cm::aot::match_debugger;
void require(bool value, const char* message) { if (!value) throw std::runtime_error(message); }
struct Effect {
    unsigned kind = 0;
    std::uint32_t address = 0;
    std::uint64_t length = 0;
    std::vector<std::uint8_t> bytes;
    bool operator==(const Effect&) const = default;
};
struct Trace final : MemoryObserver {
    std::vector<Effect> effects;
    void mapped(std::uint32_t address, std::uint64_t length) override { effects.push_back({0, address, length, {}}); }
    void cleared(std::uint32_t address, std::uint64_t length) override { effects.push_back({1, address, length, {}}); }
    void written(std::uint32_t address, const std::uint8_t* bytes, std::size_t length) override {
        effects.push_back({2, address, length, {bytes, bytes + length}});
    }
};
struct Observe {
    Memory& memory; MemoryObserver* previous;
    Observe(State& state, Trace& trace) : memory(state.memory), previous(memory.observer) { memory.observer = &trace; }
    ~Observe() { memory.observer = previous; }
};
std::uint64_t invoke(State& state, std::uint32_t entry, std::initializer_list<std::uint32_t> values) {
    const auto before = state.executed, budget = state.budget;
    unsigned reg = 3;
    for (auto value : values) state.wr<4>(reg++ * 4, value);
    state.wr<4>(4128, return_sentinel); state.pc = entry; state.budget = before + fresh_match_step_limit;
    run_until(state, return_sentinel); state.budget = budget;
    return state.executed - before;
}
struct Baseline {
    std::uint64_t first_instructions, second_instructions, first_calls, second_calls, first_budget, second_budget;
};
void same_match(const Session& session, const EngineInstance& control, const Baseline& baseline) {
    const auto& first = session.state(); const auto& second = *control.state;
    require(session.match_address() == control.match, "Independent seeded match allocation differs");
    require(first.memory.copy_out(control.match, 0xe0bc) == second.memory.copy_out(control.match, 0xe0bc),
        "Complete actual match differs from direct original execution");
    require(first.registers == second.registers, "Original intervention registers differ");
    require(first.unique == second.unique, "Original intervention unique bytes differ");
    require(first.pc == second.pc, "Original intervention return PC differs");
    // Independent world startup can have a different path/time/import cost.
    // Compare all execution from the common initialized-match boundary onward,
    // while checking each world's own restored caller budget separately.
    require(first.executed - baseline.first_instructions == second.executed - baseline.second_instructions,
        "Original intervention instruction accounting differs");
    require(first.native_calls - baseline.first_calls == second.native_calls - baseline.second_calls,
        "Original intervention native call accounting differs");
    require(first.budget == baseline.first_budget && second.budget == baseline.second_budget, "Original intervention caller budget differs");
    require(first.allocated == second.allocated && first.available == second.available && first.heap_end == second.heap_end,
        "Original intervention allocations differ");
}
template<class Action> void reject_without_effects(Session& session, Action action) {
    const auto inspection = session.inspect();
    const auto memory = session.state().memory.copy_out(session.match_address(), 0xe0bc);
    const auto registers = session.state().registers;
    const auto unique = session.state().unique;
    const auto instructions = session.state().executed;
    const auto edits = session.lineup_edits().size();
    bool rejected = false;
    try { action(); } catch (const std::logic_error&) { rejected = true; }
    require(rejected && session.inspect() == inspection && session.state().memory.copy_out(session.match_address(), 0xe0bc) == memory &&
        session.state().registers == registers && session.state().unique == unique && session.state().executed == instructions &&
        session.lineup_edits().size() == edits, "Rejected lineup edit changed live state or audit history");
}
std::uint32_t actor_address(std::uint32_t match, unsigned team, unsigned actor) {
    return match + 0x311c + (team * 20 + actor) * 0x1c4;
}
RosterMember unused(const Session& session, unsigned team) {
    for (const auto& member : session.roster(team)) {
        bool found = false;
        for (unsigned side = 0; side < 2; ++side) for (unsigned actor = 0; actor < 20; ++actor)
            found |= session.state().memory.read<4>(actor_address(session.match_address(), side, actor) + 0x6c) == member.staff;
        if (!found) return member;
    }
    throw std::runtime_error("Actual club has no unused player for this check");
}
std::uint64_t initialize_original(State& state, std::uint32_t match, unsigned team, unsigned actor, const RosterMember& member) {
    const auto address = actor_address(match, team, actor);
    const auto previous = state.memory.read<4>(address + 0x6c);
    const auto old = signed_value<1>(state.memory.read<1>(address + 0x19));
    const auto start = state.executed;
    (void)invoke(state, 0x1020e8f0, {match + 0x7aac + team * 0xd8, actor, member.staff, actor});
    const auto position = signed_value<1>(state.memory.read<1>(address + 0x19));
    if (previous && position >= -1) {
        const auto wanted = old >= -1 ? old : -1;
        if (position != wanted) (void)invoke(state, 0x101f1870, {address, std::uint32_t(wanted)});
    }
    return state.executed - start;
}
struct CutAfterStaff final : MemoryObserver {
    State& state; std::uint32_t address; bool reached = false;
    CutAfterStaff(State& value, std::uint32_t target) : state(value), address(target) {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t target, const std::uint8_t*, std::size_t size) override {
        if (target == address && size == 4 && state.pc == 0x10223b20) { reached = true; state.budget = state.executed + 1; }
    }
};
}

int main(int argc, char** argv) {
    try {
        if (argc != 4) throw std::runtime_error("Expected original program, read-only game and private root");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        unsigned generation = 0;
        auto factory = [&] { return create_match({argv[1], argv[2], root / ("session-" + std::to_string(generation++))}, {677,5677,12345,2001,194}); };
        Session session(factory, fresh_match_step_limit);
        auto control = create_match({argv[1], argv[2], root / "control"}, {677,5677,12345,2001,194});
        match_engine_diagnostics(*control.state).entry_original[2] = true;
        match_actor_diagnostics(*control.state).original = true;
        const Baseline baseline{session.state().executed, control.state->executed, session.state().native_calls,
            control.state->native_calls, session.state().budget, control.state->budget};
        const auto initial_reserve = session.state().memory.read<4>(actor_address(session.match_address(),0,11)+0x6c);
        require(session.roster(0).size() > 20 && session.roster(1).size() > 20 && initial_reserve,
            "Real rosters or prepared substitutes are absent");
        auto member = unused(session,0);
        reject_without_effects(session,[&]{session.set_player(2,11,member.staff_id);});
        reject_without_effects(session,[&]{session.set_player(0,20,member.staff_id);});
        reject_without_effects(session,[&]{session.set_player(0,11,session.roster(1).front().staff_id);});
        reject_without_effects(session,[&]{session.set_player(0,11,UINT32_MAX);});
        reject_without_effects(session,[&]{session.set_position(0,20,8);});
        reject_without_effects(session,[&]{session.set_position(0,8,-2);});
        reject_without_effects(session,[&]{session.set_position(0,8,11);});
        reject_without_effects(session,[&]{session.set_position(0,8,8);});
        unsigned compared = 0;
        for (auto selection : {std::pair{0u,11u},std::pair{1u,8u}}) {
            const auto [team,actor] = selection; member = unused(session,team);
            Trace actual, original; LineupRecord record;
            const auto before = session.state().memory.copy_out(session.match_address(),0xe0bc);
            {Observe observe(const_cast<State&>(session.state()),actual);record=session.set_player(team,actor,member.staff_id);}
            std::uint64_t instructions = 0;
            {Observe observe(*control.state,original);instructions=initialize_original(*control.state,control.match,team,actor,member);}
            require(actual.effects==original.effects && !actual.effects.empty() && record.instructions==instructions,
                "Original complete player initialization effects or accounting differ");
            same_match(session,control,baseline);++compared;
            require(record.staff_after==member.staff_id && record.position_after==(actor==11?-1:8) && session.on_pitch(team)==11,
                "Actual player was not initialized as the requested starter/reserve");
            auto pointer_only = before;const auto offset=0x311c+(team*20+actor)*0x1c4+0x6c;
            for (unsigned byte=0;byte<4;++byte)pointer_only[offset+byte]=std::uint8_t(member.staff>>(24-8*byte));
            require(pointer_only!=session.state().memory.copy_out(session.match_address(),0xe0bc),
                "Negative control cannot distinguish real attributes/condition/identity from a staff-pointer edit");
            reject_without_effects(session,[&]{session.set_player(team,actor,member.staff_id);});
            std::cout<<"initialized team="<<team<<" actor="<<actor<<" staff="<<member.staff_id<<" instructions="<<record.instructions<<" effects="<<actual.effects.size()<<'\n';
        }
        auto position = [&](unsigned team,unsigned actor,int slot){
            Trace actual,original;LineupRecord record;
            {Observe observe(const_cast<State&>(session.state()),actual);record=session.set_position(team,actor,slot);}
            std::uint64_t instructions=0;
            {Observe observe(*control.state,original);
                if(record.displaced_actor!=20)instructions+=invoke(*control.state,0x101f1870,{actor_address(control.match,team,record.displaced_actor),std::uint32_t(record.position_before)});
                instructions+=invoke(*control.state,0x101f1870,{actor_address(control.match,team,actor),std::uint32_t(slot)});
            }
            require(actual.effects==original.effects && record.instructions==instructions && record.position_after==slot,
                "Original slot assignment/swap effects differ");same_match(session,control,baseline);++compared;return record;
        };
        const auto swap=position(0,11,8);
        require(swap.displaced_actor==8 && swap.displaced_after==-1 && session.on_pitch(0)==11,"Reserve promotion failed to swap the displaced starter");
        (void)position(0,8,8);
        require(position(0,9,-1).on_pitch_after==10,"Pre-kickoff benching did not remove a player from the active side");
        (void)position(0,0,-1);
        reject_without_effects(session,[&]{session.step();});reject_without_effects(session,[&]{session.run();});
        require(session.execution_state()==ExecutionState::paused && session.steps().empty(),"Missing goalkeeper crossed the kickoff boundary");
        (void)position(0,1,0);(void)position(0,0,0);(void)position(0,1,1);
        require(session.on_pitch(0)==10 && session.on_pitch(1)==11 && session.lineup_edits().size()==9,"Accepted lineup history or counts differ");
        for (unsigned tick=0;tick<30;++tick) {
            const auto step=session.step();require(invoke(*control.state,0x101d3f60,{control.match,1})==step.instructions,"Post-lineup original advancement accounting differs");
            same_match(session,control,baseline);
            require(session.on_pitch(0)==10 && session.state().memory.read<1>(actor_address(session.match_address(),0,9)+0x19)==255,
                "Actual engine ignored the starting lineup or brought a benched player back");
        }
        require(match_actor_diagnostics(session.state()).calls[2]>0 && match_actor_diagnostics(*control.state).original_calls[2]>0,
            "Lineup edits bypassed the C++23/original position setter");
        reject_without_effects(session,[&]{session.set_player(0,11,unused(session,0).staff_id);});
        reject_without_effects(session,[&]{session.set_position(0,9,9);});
        const auto retained=session.state().memory.copy_out(session.match_address(),0xe0bc);
        Session limited(factory,fresh_match_step_limit);member=unused(limited,0);
        auto& interrupted=const_cast<State&>(limited.state());const auto budget=interrupted.budget;
        CutAfterStaff cut(interrupted,actor_address(limited.match_address(),0,11)+0x6c);interrupted.memory.observer=&cut;
        bool faulted=false;try{limited.set_player(0,11,member.staff_id);}catch(const std::runtime_error&){faulted=true;}interrupted.memory.observer=nullptr;
        require(cut.reached && faulted && limited.execution_state()==ExecutionState::faulted && limited.lineup_edits().empty() && limited.steps().empty() &&
            interrupted.budget==budget && interrupted.memory.read<4>(actor_address(limited.match_address(),0,11)+0x6c)==member.staff,
            "Partial original player creation was accepted or discarded its faulted state");
        reject_without_effects(limited,[&]{limited.set_position(0,8,-1);});
        require(session.state().memory.copy_out(session.match_address(),0xe0bc)==retained,"Another lineup experiment changed this match");
        session.reset();require(session.lineup_edits().empty() && session.steps().empty() && session.on_pitch(0)==11 && session.on_pitch(1)==11 &&
            session.state().memory.read<4>(actor_address(session.match_address(),0,11)+0x6c)==initial_reserve,"Seeded reset retained player/lineup edits");
        std::cout<<"PASS: two complete original actor constructions, full ABI/match/accounting/effects agreement for "<<compared
            <<" lineup edits, real club roster validation, reserve/starter replacement, position swaps/benching, missing goalkeeper guard, 30 original-agreeing shared steps, pointer-only negatives, rejected edits, partial budget fault, independent state and fresh reset\n";
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
