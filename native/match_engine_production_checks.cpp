#include "match_engine_adapter.hpp"
#include "match_replay.hpp"
#include "code_fragment_adapter.hpp"
#include "semantic_io_checks.hpp"
#include "match_turbo.hpp"
#include <string_view>
#include <utility>

namespace {
using namespace cm::semantic_io_checks;
namespace engine = cm::aot::match_engine;
enum class Path { direct, generated, cfm };
enum class Mode { modern, global, entry, profile };
std::array<unsigned,3> comparisons{}, direct{}, generated{}, cfm{}, negatives{}, budget_stops{};
struct Imports final : CodeFragmentBoundaryObserver {
    struct Event {
        bool leave; std::string name; decltype(State::registers) registers; std::uint32_t pc;
        bool operator==(const Event&) const = default;
    };
    std::vector<Event> events;
    unsigned clocks = 0;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s,const char* name) override {events.push_back({false,name,s.registers,s.pc});}
    void import_leave(const State& s,const char* name) override {events.push_back({true,name,s.registers,s.pc});}
    bool import_fixture(State& s,const char* name) override {
        // Only external clock input is fixed. Football, RNG, commentary,
        // resources and CFM marshalling retain production implementations.
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        ++clocks;s.memory.write<8>(s.r<4>(12),1000000);s.pc=std::uint32_t(s.r<4>(4128));return true;
    }
};
struct Boundary final : MatchEngineBoundaryObserver {
    std::uint32_t root, match; bool corrupt, corrupt_child;
    std::vector<std::uint32_t> pending,entered;
    std::vector<std::pair<std::uint32_t,MatchEngineOutcome>> left;
    Boundary(std::uint32_t entry,std::uint32_t owner,bool negative,bool child) : root(entry),match(owner),corrupt(negative),corrupt_child(child) {}
    void enter(const State&,std::uint32_t entry,bool) override {entered.push_back(entry);pending.push_back(entry);}
    void leave(State& s,std::uint32_t entry,bool original,MatchEngineOutcome outcome) override {
        require(!pending.empty()&&pending.back()==entry,"Nested productive match boundary order");pending.pop_back();left.emplace_back(entry,outcome);
        if(corrupt&&!corrupt_child&&pending.empty()&&entry==root&&!original&&outcome==MatchEngineOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);
        if(corrupt_child&&entry==engine::entries[2]&&!original&&outcome==MatchEngineOutcome::returned)
            s.memory.write<1>(match+0xe03d,s.memory.read<1>(match+0xe03d)^1);
    }
};
std::uint32_t prepare(State& s,unsigned index) {
    const auto match=std::uint32_t(s.r<4>(12)),manager=std::uint32_t(s.r<4>(27*4)),watched=std::uint32_t(s.r<4>(28*4));
    require(s.memory.read<4>(watched+0x58)==match,"Captured original watched caller owns the actual match");
    if(index==1) {
        const auto list=std::uint32_t(s.memory.read<4>(manager+0x1c));
        require(watched>=list&&(watched-list)%0x6c==0,"Original fixture belongs to its manager array");
        s.memory.write<4>(manager+0x30,(watched-list)/0x6c);
        const auto settings=s.memory.read<4>(s.r<4>(8)-0x7ac8),hardware=s.memory.read<4>(settings);
        s.memory.write<4>(hardware+4,s.memory.read<4>(hardware+4)&~UINT32_C(0xc0000));
    }
    s.wr<4>(12,index==2?match:manager);s.wr<4>(16,index==0?watched:index==1?0:1);
    s.memory.map(fixture,0x1000);s.memory.copy_in(fixture,std::vector<std::uint8_t>(0x1000,0xa7));
    reset_match_engine_diagnostics(s);s.budget=s.executed+1000000;return match;
}
std::string execute(State& s,unsigned index,Path path) {
    try {
        s.wr<4>(4128,return_sentinel);s.pc=engine::entries[index];
        if(path==Path::direct)require(try_modern_match_engine(s,s.pc),"Direct productive match adapter handles this entry");
        else {
            if(path==Path::cfm) {
                s.memory.write<4>(fixture,s.pc);s.memory.write<4>(fixture+4,s.r<4>(8));s.wr<4>(48,fixture);s.pc=0x10781320;
            }
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& e){return std::string(e.what())+" at PC "+hex_address(s.pc);}return {};
}
void compare(const State& seed,unsigned index,Path path,Mode mode,bool negative=false,bool child_negative=false,std::uint64_t budget=1000000,bool turbo=false) {
    auto a=clone(seed),b=clone(seed);const auto owner_a=prepare(*a,index),owner_b=prepare(*b,index);
    require(owner_a==owner_b,"Identical actual match owner");Image(*a).compare(*b,"Identical original match prerequisite");
    a->budget=a->executed+budget;b->budget=b->executed+budget;
    auto& old=match_engine_diagnostics(*a);auto& modern=match_engine_diagnostics(*b);old.original=true;
    if(mode==Mode::global)modern.original=true;if(mode==Mode::entry)modern.entry_original[index]=true;
    Imports oi,mi;Boundary ob(engine::entries[index],owner_a,false,false),mb(engine::entries[index],owner_b,negative,child_negative);
    old.observer=&ob;modern.observer=&mb;a->code_fragment_observer=&oi;b->code_fragment_observer=&mi;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    set_match_turbo_enabled(false);const auto ae=execute(*a,index,path);
    set_match_turbo_enabled(turbo);const auto be=execute(*b,index,path);set_match_turbo_enabled(false);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;
    old.observer=modern.observer=nullptr;
    const auto label=hex_address(engine::entries[index])+" productive path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    const bool interrupted=budget!=1000000;
    // Where a budget stop lands is a diagnostic (2026-10-20): generated code
    // finishes the run it started, the C++ owner stops at the exact
    // instruction; a cut inside the original's last run lets it complete.
    // Complete runs are compared whole.
    require(interrupted?be.starts_with("Native instruction budget exhausted")&&(ae.empty()||ae.starts_with("Native instruction budget exhausted")):ae==be,
        label+" original="+ae+" modern="+be);
    require(interrupted||ae.empty(),label+" explicit normal/budget result="+ae);
    if(negative) {
        bool rejected=false;try {Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}
        require(rejected,"Productive match negative must change observable guest state");++negatives[index];
    }else if(!interrupted) {
        Image(*a).compare(*b,label);require(x.bytes==y.bytes&&oi.events==mi.events,label+" ordered memory and actual import ABI");
        require(a->executed==b->executed,label+" complete instruction accounting");
    }
    require(ob.pending.empty()&&mb.pending.empty(),label+" balanced productive boundaries");
    require(!modern.original_body&&!modern.dispatch_escapes,label+" no original-scope residue or dispatch escape");
    if(mode==Mode::modern) {
        require(modern.calls[index]==1&&!modern.original_calls[index]&&modern.returns[index]==unsigned(!interrupted)&&modern.budget_stops[index]==unsigned(interrupted)&&
            !modern.faults[index]&&!modern.transfers[index]&&!modern.terminations[index],label+" dedicated productive entry coverage");
        if(!interrupted&&index!=2)require(modern.calls[2]>=1&&modern.returns[2]>=1&&!modern.original_calls[2],label+" actual shared child composition");
    }else {
        require(!modern.calls[index]&&!modern.returns[index],label+" requested original root excludes modern execution");
        require(mode==Mode::profile?modern.profile_original_calls[index]==1:modern.original_calls[index]==1,label+" explicit original control evidence");
    }
    ++comparisons[index];budget_stops[index]+=interrupted;
    if(path==Path::direct)++direct[index];else if(path==Path::generated)++generated[index];else ++cfm[index];
}
void diagnostic_ownership() {
    State a,b;auto& d=match_engine_diagnostics(a);d.original=true;d.calls[2]=17;
    require(!match_engine_diagnostics(std::as_const(b)).calls[2]&&!b.runtime_diagnostics,"Allocation-free const match diagnostics");
    reset_match_engine_diagnostics(a);require(!d.original&&!d.calls[2],"Reset clears only owned match diagnostics");
}
}
int main(int argc,char** argv) {
    try {
        const bool profile=argc==3&&std::string_view(argv[2])=="--profile";
        require((argc==2||profile)&&match_pc_profile_enabled()==profile,"Explicit private match fixture and separate profile mode");
        auto seed=std::make_unique<State>();load_match_snapshot(*seed,argv[1]);
        require(seed->pc==engine::entries[2]&&seed->r<4>(4128)==0x101ccf1c,"Actual initialized original engine snapshot");
        diagnostic_ownership();
        for(unsigned root=0;root<engine::entries.size();++root) {
            for(auto path:{Path::direct,Path::generated,Path::cfm}) {
                if(profile)compare(*seed,root,path,Mode::profile);
                else for(auto mode:{Mode::modern,Mode::global,Mode::entry})compare(*seed,root,path,mode);
            }
            if(!profile) {
                compare(*seed,root,Path::generated,Mode::modern,true);
                compare(*seed,root,Path::generated,Mode::modern,false,false,1000000,true);
                for(auto budget:{0u,1u,4u,10u,30u,100u})compare(*seed,root,Path::generated,Mode::modern,false,false,budget);
                if(root!=2)compare(*seed,root,Path::generated,Mode::modern,true,true);
            }
            std::cout<<hex_address(engine::entries[root])<<" comparisons="<<comparisons[root]<<" direct="<<direct[root]<<" generated="<<generated[root]
                <<" cfm="<<cfm[root]<<" negatives="<<negatives[root]<<" budget_stops="<<budget_stops[root]<<std::endl;
        }
        std::cout<<"PASS: actual original/modern entry composition, productive direct/generated/CFM controls and ordered effects; clock input fixed\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
