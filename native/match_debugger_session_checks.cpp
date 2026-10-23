#include "match_debugger_session.hpp"
#include "match_replay.hpp"
#include "semantic_io_checks.hpp"
#include <iostream>
#include <memory>

namespace debugger = cm::aot::match_debugger;
struct ControlledStop {};
struct StopObserver final : cm::aot::MemoryObserver {
    cm::aot::State* state;
    explicit StopObserver(cm::aot::State* s) : state(s) {}
    void mapped(std::uint32_t, std::uint64_t) override {}
    void written(std::uint32_t, const std::uint8_t*, std::size_t) override {}
    void cleared(std::uint32_t, std::uint64_t) override {}
    void read(std::uint32_t, const std::uint8_t*, unsigned) override {
        if (state->pc == 0x101d3f78) throw ControlledStop{};
    }
};
using cm::semantic_io_checks::require;
using cm::semantic_io_checks::Image;

int main(int argc, char** argv) {
    try {
        require(argc==2,"Actual private match fixture argument");
        cm::aot::State* live=nullptr;
        bool fail_factory=false;
        unsigned instances=0;
        auto factory=[&]() -> debugger::EngineInstance {
            if(fail_factory) throw std::runtime_error("Controlled independent session creation failure");
            auto state=std::make_unique<cm::aot::State>();
            cm::aot::load_match_snapshot(*state,argv[1]);
            require(state->pc==0x101d3f60 && state->r<4>(4128)==0x101ccf1c && state->r<4>(16)==1,
                "Actual initialized original shared-engine capture");
            const auto owner=std::uint32_t(state->r<4>(12));
            live=state.get(); ++instances;
            return {std::move(state),owner};
        };
        debugger::Session session(factory);
        Image initial(*live);
        const auto initial_inspection=session.inspect();
        const auto initial_instructions=live->executed, initial_budget=live->budget;
        require(session.execution_state()==debugger::ExecutionState::paused && session.steps().empty(),"Fresh experiment starts paused");
        for(unsigned poll=0;poll<5;++poll) require(!session.advance_if_running(),"Paused scheduler does not execute engine");
        initial.compare(*live,"Paused scheduler preserves complete captured state");
        require(live->executed==initial_instructions,"Paused scheduler preserves instruction accounting");
        const auto first=session.step();
        // Instruction counts are diagnostics (decided 2026-10-20): routines
        // rewritten in C++ do not count, so only require that the step ran.
        require(first.number==1 && first.engine_return==1 && first.instructions>0,
            "First actual engine step matches original captured interval");
        require(first.before==initial_inspection && first.after.elapsed_engine_units==1 && first.after.actor_queue_count==22,
            "Real engine changes the clock and live actor queue");
        require(session.execution_state()==debugger::ExecutionState::paused && live->budget==initial_budget,
            "One explicit step returns paused and restores caller budget");
        session.run();
        bool rejected=false;
        try {session.step();} catch(const std::logic_error&) {rejected=true;}
        require(rejected && session.steps().size()==1 && session.execution_state()==debugger::ExecutionState::running,
            "An explicit step cannot race scheduled running advancement");
        for(unsigned tick=1;tick<24;++tick) {
            auto step=session.advance_if_running();
            require(step && step->number==tick+1 && step->engine_return==1 && step->instructions>0,
                "Running scheduler advances one actual engine boundary");
        }
        require(session.inspect().elapsed_engine_units==33 && session.inspect().home_score==0 &&
            session.inspect().away_score==0 && session.inspect().actor_queue_count==25,
            "24 steps agree with the independently captured original probe interval");
        session.pause();
        Image paused(*live);
        const auto paused_instructions=live->executed;
        for(unsigned poll=0;poll<5;++poll) require(!session.advance_if_running(),"Pause suppresses scheduled advancement");
        paused.compare(*live,"Pause preserves all guest state after real execution");
        require(live->executed==paused_instructions,"Pause leaves real execution accounting unchanged");
        fail_factory=true; rejected=false;
        try {session.reset();} catch(const std::runtime_error&) {rejected=true;}
        require(rejected && session.steps().size()==24 && session.execution_state()==debugger::ExecutionState::paused,
            "Failed replacement creation retains the current paused experiment");
        paused.compare(*live,"Failed reset retains complete prior guest state");
        fail_factory=false; session.reset();
        require(instances==2 && session.steps().empty() && session.inspect()==initial_inspection && session.failure().empty(),
            "Reset creates a fresh independent match and clears its debugger history");
        initial.compare(*live,"Reset restores original guest memory, registers, allocator and random state");
        require(live->executed==initial_instructions,"Reset restores captured execution state");
        const auto repeated=session.step();
        require(repeated.after==first.after && repeated.instructions==first.instructions,
            "Reset replay of one real engine step is deterministic");
        debugger::Session limited(factory,1); rejected=false;
        try {limited.step();} catch(const std::runtime_error&) {rejected=true;}
        require(rejected && limited.execution_state()==debugger::ExecutionState::faulted &&
            !limited.failure().empty() && limited.steps().empty(),"Budget failure is not a completed debugger step");
        require(live->budget==initial_budget,"Fault restores previous budget");
        rejected=false;
        try {limited.run();} catch(const std::logic_error&) {rejected=true;}
        require(rejected,"Faulted experiment cannot run before reset");
        require(!limited.advance_if_running(),"Faulted scheduler cannot resume a partial engine step");
        limited.reset();
        require(limited.execution_state()==debugger::ExecutionState::paused && limited.failure().empty(),
            "Reset clears a failed experiment");
        initial.compare(*live,"Fault reset restores complete initial state");
        debugger::Session interrupted(factory);
        StopObserver stop(live); live->memory.date_observer = &stop; rejected=false;
        try {interrupted.step();} catch(const ControlledStop&) {rejected=true;}
        live->memory.date_observer=nullptr;
        require(rejected && interrupted.execution_state()==debugger::ExecutionState::faulted &&
            interrupted.steps().empty() && !interrupted.failure().empty(),
            "A nonstandard interrupted engine prefix cannot be reported as paused or completed");
        require(live->pc==0x101d3f78 && live->executed>initial_instructions,
            "The interrupted real engine prefix remains available for diagnosis");
        require(!interrupted.advance_if_running(),"Nonstandard interrupted step cannot resume through scheduler");
        interrupted.reset();
        initial.compare(*live,"Reset after a nonstandard interrupted engine prefix restores exact initial state");
        rejected=false;
        try {debugger::Session bad([] {return debugger::EngineInstance{};});} catch(const std::runtime_error&) {rejected=true;}
        require(rejected,"Missing independent match is rejected");
        rejected=false;
        try {debugger::Session bad(factory,0);} catch(const std::invalid_argument&) {rejected=true;}
        require(rejected,"Zero debugger step budget is rejected");
        std::cout << "PASS: actual engine stepping, pause, deterministic reset, failed-reset preservation, budget and nonstandard faults and blocked partial resume\n"
            << "24 original shared-engine steps; factory uses a private live fixture, not arbitrary club bootstrap; no debugger app delivery claim\n";
        return 0;
    } catch(const std::exception& error) {std::cerr << error.what() << '\n';return 1;}
}
