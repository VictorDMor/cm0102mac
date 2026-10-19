#include "match_selection_adapter.hpp"
#include "match_random_adapter.hpp"
#include "spatial_role_adapter.hpp"
#include "match_debugger_factory.hpp"
#include "semantic_io_checks.hpp"
#include "code_fragment_adapter.hpp"
#include "match_turbo.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include "process_bridge.hpp"
#include <chrono>
#include <fstream>
#include <set>
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
namespace selection=cm::aot::match_selection;
enum class Path { direct, generated, cfm, original_bl };
constexpr std::array<std::uint32_t,3> callsites{0x101dc8f8,0x101dbe90,0x101dbb50};
enum class Mode { modern, original, entry_original, profile };
struct Clock:CodeFragmentBoundaryObserver {
    std::uint64_t base=native_microseconds();unsigned count=0;
    struct Event {bool leave;std::string name;decltype(State::registers) registers;std::uint32_t pc;bool operator==(const Event&)const=default;};
    std::vector<Event> events;
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool)override{}
    void import_enter(const State& s,const char* name)override{events.push_back({false,name,s.registers,s.pc});}
    void import_leave(const State& s,const char* name)override{events.push_back({true,name,s.registers,s.pc});}
    bool import_fixture(State& s,const char* name)override {
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Selection awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
struct Capture:MatchSelectionBoundaryObserver {
    std::array<std::unique_ptr<State>,selection::entries.size()> inputs;
    void enter(const State& s,std::uint32_t root,bool original)override {
        if(original)return;
        const auto entry=std::find(selection::entries.begin(),selection::entries.end(),root);
        require(entry!=selection::entries.end(),"Unknown target capture");
        auto& input=inputs[unsigned(entry-selection::entries.begin())];
        if(input)return;
        input=clone(s);input->platform=s.platform;input->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,MatchSelectionOutcome)override{}
};
std::array<std::set<std::uint32_t>,selection::entries.size()> complete_sites;
std::array<unsigned,4> random_returns{},position_returns{},role_returns{};
constexpr std::array<std::uint32_t,4> ends{0x101e20f4,0x101e22d8,0x101e2598,0x101e28a8};
struct Negative:MatchSelectionBoundaryObserver {
    unsigned index;bool negative=false;std::uint32_t output=0;std::set<std::uint32_t> seen;std::array<unsigned,4> returned{};
    explicit Negative(unsigned value):index(value){}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget)seen.insert(pc);}
    void enter(const State& s,std::uint32_t root,bool)override{if(root==selection::entries[3]&&index==3)output=std::uint32_t(s.r<4>(20));}
    void leave(State& s,std::uint32_t entry,bool original,MatchSelectionOutcome result)override{
        if(original||result!=MatchSelectionOutcome::returned)return;
        const auto found=std::find(selection::entries.begin(),selection::entries.end(),entry);require(found!=selection::entries.end(),"Known target outcome");++returned[unsigned(found-selection::entries.begin())];
        if(negative&&entry==selection::entries[index]){if(index==3)s.memory.write<1>(output,s.memory.read<1>(output)^1);else s.wr<4>(12,s.r<4>(12)^1);}
    }
};
struct OwnedBacking {
    std::filesystem::path directory;
    ~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(directory,e)||e||std::filesystem::is_symlink(directory,e)||e)return;
        for(const auto& p:std::filesystem::recursive_directory_iterator(directory,e))if(e||p.is_symlink(e)||e)return;
        std::filesystem::remove_all(directory,e);}
};
unsigned cases=0,returns=0,budgets=0,negatives=0,faults=0;
std::string execute(State& s,Path path,unsigned index) {
    try {
        if(path==Path::direct)try_modern_match_selection(s,selection::entries[index]);
        else if(path==Path::original_bl){s.pc=callsites.at(index);run_until(s,callsites.at(index)+4);}
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,selection::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
void compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,unsigned fault=0) {
    auto a=clone(input),b=clone(input);a->platform=b->platform=input.platform;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=selection::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    if(fault)for(auto* s:{a.get(),b.get()}){
        s->wr<4>(12,0);
    }
    match_selection_diagnostics(*a).original=true;auto& d=match_selection_diagnostics(*b);
    match_random_diagnostics(*b)={};spatial_role_diagnostics(*b)={};
    if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(index);mutation.negative=negative;if(mode==Mode::modern)d.observer=&mutation;
    Clock old_clock,new_clock;new_clock.base=old_clock.base;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index),actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production selection entry="+hex_address(selection::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(fault?expected.starts_with("Unmapped data access at "):limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected boundary="+expected);
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,label+" host input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative failed");++negatives;}
    else {Image(*a).compare(*b,label);require(x.bytes==y.bytes && a->executed==b->executed,label+" ordered effects/accounting");}
    require(!d.original_body && !d.dispatch_escapes,label+" own-body scope residue/dispatch escape");
    if(mode==Mode::modern){require(d.calls[index]>=1 && !d.original_calls[index] && !d.profile_original_calls[index],label+" no dedicated modern call");
        require(d.returns[index]==(expected.empty()?d.calls[index]:0) && d.budget_stops[index]==unsigned(!fault&&!expected.empty()) && d.faults[index]==unsigned(bool(fault)),label+" productive outcome counts");}
    else {require(!d.calls[index] && !d.returns[index],label+" original control entered C++ root");require(mode==Mode::profile?d.profile_original_calls[index]>=1:d.original_calls[index]>=1,label+" selected original/profile not reached");}
    if(mode==Mode::modern&&expected.empty()&&!negative){
        for(auto pc:mutation.seen)if(selection::entries[index]<=pc&&pc<ends[index])complete_sites[index].insert(pc);
        const auto& rng=match_random_diagnostics(*b);const auto& spatial=spatial_role_diagnostics(*b);
        const auto pos=unsigned(std::find(spatial_role::entries.begin(),spatial_role::entries.end(),0x101d3b50)-spatial_role::entries.begin());
        const auto role=unsigned(std::find(spatial_role::entries.begin(),spatial_role::entries.end(),0x10289560)-spatial_role::entries.begin());
        require(pos<spatial_role::entries.size()&&role<spatial_role::entries.size(),"Known position and role helper");
        require(spatial.calls[pos]>0&&spatial.calls[pos]==spatial.returns[pos]&&!spatial.original_calls[pos],label+" complete actual position helper");
        if(index<3)require(rng.calls[0]>0&&rng.calls[0]==rng.returns[0]&&!rng.original_calls[0]&&spatial.calls[role]>0&&spatial.calls[role]==spatial.returns[role]&&!spatial.original_calls[role],label+" complete actual RNG and role helpers");
        random_returns[index]+=rng.returns[0];position_returns[index]+=spatial.returns[pos];role_returns[index]+=spatial.returns[role];
    }
    ++cases;returns+=expected.empty();budgets+=!fault&&!expected.empty();faults+=bool(fault);
}
void matrix(const State& input,unsigned index,bool profile) {
    for(auto path:{Path::direct,Path::generated,Path::cfm}){
        if(profile)compare(input,index,path,Mode::profile);
        else for(auto mode:{Mode::modern,Mode::original,Mode::entry_original})compare(input,index,path,mode);
    }
    if(!profile){compare(input,index,Path::generated,Mode::modern,50000000,true);compare(input,index,Path::generated,Mode::modern,0);compare(input,index,Path::generated,Mode::modern,1);compare(input,index,Path::generated,Mode::modern,2);}
}
}
int main(int argc,char** argv){try {
    require(argc==4 || argc==5,"Expected original program, game, fresh backing and optional profile directory");
    const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Profile request/flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    OwnedBacking cleanup{root};
    if(profile){
        for(unsigned index=0;index<selection::entries.size();++index){
            State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
            load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(selection::entries[index])+".snapshot"));
            require(input.pc==selection::entries[index],"Snapshot is not the captured real-caller selection entry");matrix(input,index,true);
        }
    } else {
        auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto& world=*game.state;
        match_selection_diagnostics(world)={};Capture capture;match_selection_diagnostics(world).observer=&capture;
        for(unsigned step=0;step<600;++step){world.wr<4>(12,game.match);world.wr<4>(16,1);world.wr<4>(4128,return_sentinel);world.pc=0x101d3f60;world.budget=world.executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(world,return_sentinel);}
        match_selection_diagnostics(world).observer=nullptr;
        const auto natural=match_selection_diagnostics(world).calls;
        const auto& natural_diagnostics=match_selection_diagnostics(world);
        for(unsigned n=0;n<4;++n)require(natural[n]==natural_diagnostics.returns[n]&&!natural_diagnostics.original_calls[n]&&!natural_diagnostics.faults[n]&&!natural_diagnostics.budget_stops[n]&&!natural_diagnostics.dispatch_escapes,"Natural productive selection outcomes");
        require(bool(capture.inputs[3])&&natural[3]>0,"Natural preset target caller capture");
        std::array<std::uint64_t,4> prepared{};
        // These selectors are absent in the measured natural window. Execute
        // each indexed original BL with real match/team input and real services;
        // stop at its own continuation. This does not claim the whole parent.
        for(unsigned index=0;index<3;++index){
            auto input=clone(world);input->platform=world.platform;input->wr<4>(12,game.match);input->wr<4>(16,0);input->wr<4>(20,0);
            auto& d=match_selection_diagnostics(*input);d={};d.observer=&capture;input->pc=callsites[index];input->budget=input->executed+50000000;run_until(*input,callsites[index]+4);d.observer=nullptr;
            require(d.calls[index]==1&&d.returns[index]==1&&!d.original_calls[index]&&!d.faults[index]&&!d.budget_stops[index]&&!d.dispatch_escapes,"Prepared complete original BL selection return");prepared[index]=d.calls[index];
            require(bool(capture.inputs[index]),"Prepared original BL capture");compare(*capture.inputs[index],index,Path::original_bl,Mode::modern);
        }
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"match-selection-live-profile";std::filesystem::create_directories(snapshots);
        for(unsigned index=0;index<selection::entries.size();++index){
            require(bool(capture.inputs[index]),"Missing real-caller selection capture "+hex_address(selection::entries[index]));
            require(natural[index]+prepared[index]>0,"Dedicated selection caller coverage");
            save_runtime_snapshot(*capture.inputs[index],snapshots/(hex_address(selection::entries[index])+".snapshot"));
            matrix(*capture.inputs[index],index,false);compare(*capture.inputs[index],index,Path::generated,Mode::modern,50000000,false,1);
            std::cout<<"Integrated selection entry="<<hex_address(selection::entries[index])<<" natural_calls="<<natural[index]<<" prepared_bl_calls="<<prepared[index]<<std::endl;
        }
        require(cases==59&&negatives==4&&budgets==12&&faults==4,"Four-root actual selection matrix");
        std::ofstream sites(std::filesystem::path(argv[3]).parent_path()/"match-selection-runtime-sites.txt");
        for(unsigned n=0;n<4;++n){require(!complete_sites[n].empty(),"Complete productive source sites per selection root");for(auto pc:complete_sites[n])sites<<n<<" "<<hex_address(pc)<<'\n';std::cout<<"Root "<<hex_address(selection::entries[n])<<" complete_actual_sites="<<complete_sites[n].size()<<" random_returns="<<random_returns[n]<<" position_returns="<<position_returns[n]<<" role_returns="<<role_returns[n]<<std::endl;}
    }

    std::cout<<"PASS: production match selection cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives
        <<" faults="<<faults<<"; real-context direct/generated/CFM, original/profile controls, complete state/effects/accounting/import boundaries\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
