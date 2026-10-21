#include "spatial_role_adapter.hpp"
#include "match_debugger_factory.hpp"
#include "semantic_io_checks.hpp"
#include "match_turbo.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include "code_fragment_adapter.hpp"
#include "native_drawing.hpp"
#include <chrono>
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
namespace actor = cm::aot::spatial_role;
enum class Path { direct, generated, cfm };
enum class Mode { modern, global_original, entry_original, profile };
struct Negative final : SpatialRoleBoundaryObserver {
    unsigned index; bool enabled;
    Negative(unsigned n, bool value) : index(n), enabled(value) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& state, std::uint32_t entry, bool original, SpatialRoleOutcome outcome) override {
        if (enabled && !original && entry == actor::entries[index] && outcome == SpatialRoleOutcome::returned)
            state.wr<4>(12, state.r<4>(12) ^ 1);
    }
};
unsigned cases = 0, negatives = 0, budgets = 0;
struct Clock final:CodeFragmentBoundaryObserver {
    unsigned count=0;
    struct Event {bool leave;std::string name;decltype(State::registers) registers;std::uint32_t pc;bool operator==(const Event&)const=default;};
    std::vector<Event> events;
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State&,std::uint32_t,bool)override{}
    void import_enter(const State& s,const char* name)override{events.push_back({false,name,s.registers,s.pc});}
    void import_leave(const State& s,const char* name)override{events.push_back({true,name,s.registers,s.pc});}
    bool import_fixture(State& s,const char* name)override{
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")
            throw std::runtime_error("Spatial-role comparison awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),1000000000ULL+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
void complete_match(const State& seed,std::uint32_t match,bool drawing){
    auto original=clone(seed),modern=clone(seed);original->platform=modern->platform=seed.platform;
    NativeDrawingObserver original_drawing,modern_drawing;
    if(drawing){
        original_drawing.register_surface(*original,0x70000000,8,8,16,16);
        modern_drawing.register_surface(*modern,0x70000000,8,8,16,16);
    }
    spatial_role_diagnostics(*original).original=true;
    Clock old_clock,new_clock;original->code_fragment_observer=&old_clock;modern->code_fragment_observer=&new_clock;
    unsigned advances=0;
    for(;advances<1400;++advances){
        for(auto* s:{original.get(),modern.get()}){
            s->wr<4>(12,match);s->wr<4>(16,1);s->wr<4>(4128,return_sentinel);s->pc=0x101d3f60;
            s->budget=s->executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(*s,return_sentinel);
        }
        require(original->executed==modern->executed && original->r<4>(12)==modern->r<4>(12),"Paired spatial-role match progression/accounting");
        if(advances%50==0)Image(*original).compare(*modern,"Paired spatial-role intermediate state");
        if(modern->r<4>(12)==2){++advances;break;}
    }
    original->code_fragment_observer=modern->code_fragment_observer=nullptr;
    original->memory.observer=modern->memory.observer=nullptr;
    require(advances<1400 && modern->r<4>(12)==2,"Paired spatial-role match reached full time");
    Image(*original).compare(*modern,"Paired spatial-role complete match state");
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,"Paired spatial-role match host inputs/import ABI");
    const auto& old=spatial_role_diagnostics(*original);const auto& next=spatial_role_diagnostics(*modern);
    // Generated code inlines these field getters at its call sites (leaf
    // inlining in tools/compile_recomp.py), so only C++ callers reach their
    // hooks and their counts differ between original and C++ role tables.
    // Every other entry is entered as often by both.
    constexpr std::array<unsigned,13> inlined_leaves{0,1,2,3,4,5,6,10,12,13,15,18,19};
    for(unsigned i=0;i<actor::entries.size();++i){
        const bool inlined=std::find(inlined_leaves.begin(),inlined_leaves.end(),i)!=inlined_leaves.end();
        require(next.calls[i]==next.returns[i] && (inlined || next.calls[i]==old.original_calls[i]) && !old.calls[i] &&
            !next.original_calls[i] && !next.profile_original_calls[i] && !next.faults[i] && !next.budget_stops[i],"Paired spatial-role match entry accounting");
    }
    require(next.calls[0]>0 && next.calls[7]>0 && next.calls[20]>0 && !next.dispatch_escapes,"Paired spatial-role match exercised position and role chain");
    std::cout<<"PASS: paired spatial-role full match drawing="<<drawing<<" advances="<<advances<<" positionCalls="<<next.calls[0]<<" roleCalls="<<next.calls[7]
        <<" ownerRoleCalls="<<next.calls[20]<<"; complete state, import ABI and per-tick accounting\n";
}
void arguments(State& state, std::uint32_t owner, unsigned index) {
    const auto player=owner+0x311c+8*0x1c4;
    using E=actor::Entry; const auto entry=E(index);
    state.wr<4>(12,player);state.wr<4>(16,0);state.wr<4>(20,0xffffffff);
    if(entry==E::set_marking_target)state.wr<4>(16,owner+0x311c+7*0x1c4);
    if(entry==E::current_role || entry==E::secondary_role || entry==E::previous_role || entry==E::register_center_cell || entry==E::remove_from_cell){state.wr<4>(12,owner);state.wr<4>(16,player);}
    if(entry==E::clear_goalkeeper || entry==E::resolve_goalkeeper)state.wr<4>(12,owner);
    if(entry==E::set_column)state.wr<4>(16,6);
    if(entry==E::set_row)state.wr<4>(16,8);
    if(entry==E::set_previous_position)state.wr<4>(16,8);
}
std::string execute(State& state, unsigned index, Path path) {
    try {
        if (path == Path::direct) try_modern_spatial_role(state, actor::entries[index]);
        else {
            if (path == Path::cfm) {
                state.memory.write<4>(fixture + 0x400, state.pc); state.memory.write<4>(fixture + 0x404, state.r<4>(8));
                state.wr<4>(48, fixture + 0x400); state.pc = 0x10781320;
            }
            run_until(state, return_sentinel);
        }
    } catch (const std::exception& error) { return std::string(error.what()); }
    return {};
}
void position_fault(unsigned index,bool primary=false){
    State seed;seed.registers.fill(0xa5);seed.unique.fill(0xb7);
    seed.memory.map(stack-0x300,0x400);seed.wr<4>(4,stack);seed.wr<4>(8,toc);
    seed.wr<4>(12,fixture);seed.wr<4>(16,fixture+0x1000);seed.wr<4>(4128,return_sentinel);
    seed.pc=actor::entries[index];
    auto original=clone(seed),modern=clone(seed);spatial_role_diagnostics(*original).original=true;
    Effects x,y;x.state=original.get();y.state=modern.get();
    if(primary){original->memory.observer=&x;modern->memory.observer=&y;}
    const auto expected=execute(*original,index,Path::generated),actual=execute(*modern,index,Path::generated);
    require(expected==actual && expected.starts_with("Unmapped data access at "),"Unobserved position-child fault outcome");
    // Counts after a fault are diagnostics (2026-10-20).
    Image(*original).compare(*modern,"Unobserved position-child fault state",true);
    require(x.bytes==y.bytes,"Primary-observer position-child fault effects");
    original->memory.observer=modern->memory.observer=nullptr;
    const auto& next=spatial_role_diagnostics(*modern);const unsigned child=index==9?10:0;
    require(next.calls[index]==1 && next.faults[index]==1 && next.calls[child]==1 && next.faults[child]==1 &&
        !next.returns[index] && !next.returns[child] && !next.dispatch_escapes,"Unobserved position-child fault counters");
    ++cases;
}
void compare(const State& seed, std::uint32_t owner, unsigned index, Path path, Mode mode,
    bool negative = false, std::uint64_t limit = 50000000, bool observed = true,
    bool original_position = false, bool boundary_only = false, bool primary = false, bool tracing = false) {
    auto original = clone(seed), modern = clone(seed);
    // Platform input is immutable in this interval; each clone owns all guest
    // state. Fresh-world/worker checks separately prove process independence.
    original->platform = modern->platform = seed.platform;
    for (auto* state : {original.get(), modern.get()}) {
        state->memory.map(fixture, 0x1000); arguments(*state,owner,index);
        state->wr<4>(4128, return_sentinel); state->pc = actor::entries[index];
        state->budget = state->executed + limit;
    }
    auto& old = spatial_role_diagnostics(*original); auto& next = spatial_role_diagnostics(*modern); old.original = true;
    if (mode == Mode::global_original) next.original = true;
    if (mode == Mode::entry_original) next.entry_original[index] = true;
    const unsigned position=index==unsigned(actor::Entry::previous_role)?10u:0u;
    if(original_position)next.entry_original[position]=true;
    Negative mutation(index, negative); next.observer = negative || boundary_only ? &mutation : nullptr;
    Effects x, y; x.state = original.get(); y.state = modern.get();
    if(observed){
        if(primary){original->memory.observer=&x;modern->memory.observer=&y;}
        else{original->memory.date_observer = &x; modern->memory.date_observer = &y;}
    }
    std::vector<std::pair<std::uint32_t,unsigned>> old_reads,new_reads;
    if(tracing){original->memory.read_trace=&old_reads;modern->memory.read_trace=&new_reads;}
    const auto expected = execute(*original, index, path), actual = execute(*modern, index, path);
    original->memory.date_observer = modern->memory.date_observer = nullptr;
    original->memory.observer = modern->memory.observer = nullptr; next.observer = nullptr;
    original->memory.read_trace=modern->memory.read_trace=nullptr;
    const auto label = hex_address(actor::entries[index]) + " actual path=" + std::to_string(unsigned(path)) + " mode=" + std::to_string(unsigned(mode))+" observed="+std::to_string(observed)+" primary="+std::to_string(primary);
    // A budget cut can fall inside the original's last run, which completes.
    const bool cut = limit != 50000000;
    require(cut ? actual == "Native instruction budget exhausted" && (expected.empty() || expected == actual) : expected == actual && expected.empty(),
            label + " original=" + expected + " modern=" + actual);
    if (negative) {
        bool rejected = false; try { Image(*original).compare(*modern, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Actual entry negative did not change observable state"); ++negatives;
    } else if (limit == 50000000) {
        Image(*original).compare(*modern, label);
        require(x.bytes == y.bytes && old_reads == new_reads && original->executed == modern->executed, label + " complete ordered effects, read trace and accounting");
    }
    // Where a budget stop lands is a diagnostic (2026-10-20): generated code
    // finishes the run it started, the C++ owner stops at the exact
    // instruction. Complete runs above are compared whole.
    require(!next.original_body && !next.dispatch_escapes, label + " original scope residue or escape");
    if(original_position)require(next.original_calls[position]>0 && !next.calls[position],label+" selected original position child");
    if (mode == Mode::modern) {
        require(next.calls[index] == 1 && next.returns[index] == unsigned(limit == 50000000) &&
            next.budget_stops[index] == unsigned(limit != 50000000) && !next.original_calls[index], label + " dedicated C++23 root coverage");

    } else {
        require(!next.calls[index] && !next.returns[index], label + " original control executed modern root");
        require(mode == Mode::profile ? next.profile_original_calls[index] == 1 : next.original_calls[index] == 1,
            label + " selected original/profile root coverage");
    }
    ++cases; budgets += limit != 50000000;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 4 || argc == 5, "Expected original program, game, private root and optional live profile fixture");
        const bool profile = argc == 5;
        require(profile == match_pc_profile_enabled(), "Actual profile flag differs from requested mode");
        set_match_turbo_enabled(false);
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        cm::aot::match_debugger::EngineInstance match;
        if (profile) {
            match.state = std::make_unique<State>(); load_program(*match.state, argv[1]);
            match.state->platform = std::make_shared<NativePlatform>(argv[2], root, AudioMode::offline);
            load_match_snapshot(*match.state, argv[4]);
            match.match = match.state->memory.read<4>(match.state->r<4>(12) + 0x1a4);
            require(match.state->pc == actor::entries[0], "Actual freshly created profile prerequisite");
        } else {
            match = cm::aot::match_debugger::create_match({argv[1], argv[2], root}, {677, 5677, 12345, 2001, 194});
            for(bool drawing:{false,true})complete_match(*match.state,match.match,drawing);
            auto& state=*match.state;state.wr<4>(12,match.match);state.wr<4>(16,1);state.wr<4>(4128,return_sentinel);
            state.pc=0x101d3f60;state.budget=state.executed+50000000;run_until(state,return_sentinel);
            state.wr<4>(12,match.match+0x311c+8*0x1c4);state.pc=actor::entries[0];
            save_runtime_snapshot(state,std::filesystem::path(argv[3]).parent_path()/"spatial-role-live-profile.snapshot");
        }
        auto& state = *match.state;
        for (unsigned index = 0; index < actor::entries.size(); ++index) {
            for (auto path : {Path::direct, Path::generated, Path::cfm}) {
                if (profile) compare(state, match.match, index, path, Mode::profile);
                else {
                    for (auto mode : {Mode::modern, Mode::global_original, Mode::entry_original}) compare(state, match.match, index, path, mode);
                    compare(state,match.match,index,path,Mode::modern,false,50000000,false);
                    compare(state,match.match,index,path,Mode::modern,false,50000000,true,false,false,true);
                }
            }
            if (!profile) {
                compare(state, match.match, index, Path::generated, Mode::modern, true);
                for (auto limit : {0u, 1u}) compare(state, match.match, index, Path::generated, Mode::modern, false, limit);
            }
        }
        if(!profile){
            for(unsigned index:{7u,8u,9u}){
                position_fault(index);
                position_fault(index,true);
                compare(state,match.match,index,Path::generated,Mode::modern,false,50000000,false,true);
                compare(state,match.match,index,Path::generated,Mode::modern,false,50000000,false,false,true);
                for(auto limit:{9u,10u,11u}){
                    compare(state,match.match,index,Path::generated,Mode::modern,false,limit,false);
                    compare(state,match.match,index,Path::generated,Mode::modern,false,limit,true,false,false,true);
                }
                for(auto path:{Path::direct,Path::generated,Path::cfm})
                    compare(state,match.match,index,path,Mode::modern,false,50000000,true,false,false,true,true);
            }
            auto emergency=clone(state);emergency->platform=state.platform;
            emergency->memory.write<1>(match.match+0x311c+0x19,254);
            emergency->memory.write<1>(match.match+0x311c+0x20,255);
            compare(*emergency,match.match,unsigned(actor::Entry::resolve_goalkeeper),Path::generated,Mode::modern);
        }
        require(profile?cases==69:(cases==454 && negatives==23 && budgets==64),"Complete spatial-role production and position-routing matrix");
        std::cout << "PASS: actual spatial-role production cases=" << cases << " negatives=" << negatives << " budgets=" << budgets
                  << "; direct/generated/CFM, global/entry/profile originals, cell/role/mark/keeper chain, complete guest state/ordered effects/accounting\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
