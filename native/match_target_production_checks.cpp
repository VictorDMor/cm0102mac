#include "match_target_adapter.hpp"
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
namespace target=cm::aot::match_target;
enum class Path { direct, generated, cfm };
enum class Mode { modern, original, entry_original, profile };
struct Clock:CodeFragmentBoundaryObserver {
    std::uint64_t base=native_microseconds();unsigned count=0;
    struct Event {bool leave;std::string name;decltype(State::registers) registers;std::uint32_t pc;bool operator==(const Event&)const=default;};
    std::vector<Event> events;
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool)override{}
    void import_enter(const State& s,const char* name)override{events.push_back({false,name,s.registers,s.pc});}
    void import_leave(const State& s,const char* name)override{events.push_back({true,name,s.registers,s.pc});}
    bool import_fixture(State& s,const char* name)override {
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Targeting awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
struct Capture:MatchTargetBoundaryObserver {
    std::array<std::unique_ptr<State>,target::entries.size()> inputs;
    void enter(const State& s,std::uint32_t root,bool original)override {
        if(original)return;
        const auto entry=std::find(target::entries.begin(),target::entries.end(),root);
        require(entry!=target::entries.end(),"Unknown target capture");
        auto& input=inputs[unsigned(entry-target::entries.begin())];
        if(input)return;
        input=clone(s);input->platform=s.platform;input->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,MatchTargetOutcome)override{}
};
std::array<std::set<std::uint32_t>,target::entries.size()> complete_sites;
std::array<unsigned,8> child_returns{};
constexpr std::array<std::uint32_t,8> ends{0x101d57b8,0x101e305c,0x101e3150,0x101e3174,0x101e32d4,0x102861a8,0x102862a8,0x10113e28};
struct Negative:MatchTargetBoundaryObserver {
    unsigned index;bool negative=false;std::uint32_t output=0;std::set<std::uint32_t> seen;std::array<unsigned,8> returned{};
    explicit Negative(unsigned value):index(value){}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget)seen.insert(pc);}
    void enter(const State& s,std::uint32_t root,bool)override{if(root==target::entries[1]&&index==1)output=std::uint32_t(s.r<4>(20));}
    void leave(State& s,std::uint32_t entry,bool original,MatchTargetOutcome result)override{
        if(original||result!=MatchTargetOutcome::returned)return;
        const auto found=std::find(target::entries.begin(),target::entries.end(),entry);require(found!=target::entries.end(),"Known target outcome");++returned[unsigned(found-target::entries.begin())];
        if(negative&&entry==target::entries[index]){if(index==1)s.memory.write<1>(output,s.memory.read<1>(output)^1);else s.wr<4>(12,s.r<4>(12)^1);}
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
        if(path==Path::direct)try_modern_match_target(s,target::entries[index]);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,target::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
void compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,unsigned fault=0) {
    auto a=clone(input),b=clone(input);a->platform=b->platform=input.platform;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=target::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    if(fault)for(auto* s:{a.get(),b.get()}){
        if(index==4)s->wr<4>(16,0);else s->wr<4>(12,0);
        if(index==7)s->wr<4>(16,0);
    }
    match_target_diagnostics(*a).original=true;auto& d=match_target_diagnostics(*b);
    if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(index);mutation.negative=negative;if(mode==Mode::modern)d.observer=&mutation;
    Clock old_clock,new_clock;new_clock.base=old_clock.base;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index),actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production targeting entry="+hex_address(target::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(fault?expected.starts_with("Unmapped data access at "):limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected boundary="+expected);
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,label+" host input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative failed");++negatives;}
    else {Image(*a).compare(*b,label);require(x.bytes==y.bytes && a->executed==b->executed,label+" ordered effects/accounting");}
    require(!d.original_body && !d.dispatch_escapes,label+" own-body scope residue/dispatch escape");
    if(mode==Mode::modern){require(d.calls[index]>=1 && !d.original_calls[index] && !d.profile_original_calls[index],label+" no dedicated modern call");
        require(d.returns[index]==(expected.empty()?d.calls[index]:0) && d.budget_stops[index]==unsigned(!fault&&!expected.empty()) && d.faults[index]==unsigned(bool(fault)),label+" productive outcome counts");}
    else {require(!d.calls[index] && !d.returns[index],label+" original control entered C++ root");require(mode==Mode::profile?d.profile_original_calls[index]>=1:d.original_calls[index]>=1,label+" selected original/profile not reached");}
    if(mode==Mode::modern&&expected.empty()&&!negative){for(unsigned n=0;n<8;++n)if(n!=index)child_returns[n]+=mutation.returned[n];for(auto pc:mutation.seen)for(unsigned n=0;n<8;++n)if(target::entries[n]<=pc&&pc<ends[n])complete_sites[n].insert(pc);}
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
        for(unsigned index=0;index<target::entries.size();++index){
            State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
            load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(target::entries[index])+".snapshot"));
            require(input.pc==target::entries[index],"Snapshot is not the captured real-caller targeting entry");matrix(input,index,true);
        }
    } else {
        auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto& world=*game.state;
        match_target_diagnostics(world)={};Capture capture;match_target_diagnostics(world).observer=&capture;
        for(unsigned step=0;step<20;++step){world.wr<4>(12,game.match);world.wr<4>(16,1);world.wr<4>(4128,return_sentinel);world.pc=0x101d3f60;world.budget=world.executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(world,return_sentinel);}
        match_target_diagnostics(world).observer=nullptr;
        const auto natural=match_target_diagnostics(world).calls;
        const auto& natural_diagnostics=match_target_diagnostics(world);
        for(unsigned n=0;n<8;++n)require(natural[n]==natural_diagnostics.returns[n]&&!natural_diagnostics.original_calls[n]&&!natural_diagnostics.faults[n]&&!natural_diagnostics.budget_stops[n]&&!natural_diagnostics.dispatch_escapes,"Natural productive target outcomes");
        require(bool(capture.inputs[1]),"Natural target-cell caller capture");
        std::array<std::uint64_t,8> prepared{};
        for(unsigned controlled:{0u,1u}){
            auto input=clone(*capture.inputs[1]);input->platform=capture.inputs[1]->platform;
            const auto match=std::uint32_t(input->r<4>(12)),actor=std::uint32_t(input->r<4>(16)),side=std::uint32_t(input->memory.read<1>(actor+0x27));require(side<2,"Actual target actor side");
            input->memory.write<1>(match+0x7927,1);input->memory.write<1>(match+0x7924,controlled?side:1-side);input->memory.write<4>(match+0xdff8,0);
            auto paired_parent=clone(*input);paired_parent->platform=input->platform;
            auto& d=match_target_diagnostics(*input);d={};d.observer=&capture;input->pc=target::entries[1];input->wr<4>(4128,return_sentinel);input->budget=input->executed+50000000;run_until(*input,return_sentinel);d.observer=nullptr;
            for(unsigned n=0;n<8;++n){require(d.calls[n]==d.returns[n]&&!d.original_calls[n]&&!d.faults[n]&&!d.budget_stops[n]&&!d.dispatch_escapes,"Prepared parent/child productive outcomes");prepared[n]+=d.calls[n];}
            compare(*paired_parent,1,Path::generated,Mode::modern);
        }
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"match-target-live-profile";std::filesystem::create_directories(snapshots);
        for(unsigned index=0;index<target::entries.size();++index){
            require(bool(capture.inputs[index]),"Missing real-caller targeting capture "+hex_address(target::entries[index]));
            require(natural[index]+prepared[index]>0,"Dedicated target caller coverage");
            save_runtime_snapshot(*capture.inputs[index],snapshots/(hex_address(target::entries[index])+".snapshot"));
            matrix(*capture.inputs[index],index,false);compare(*capture.inputs[index],index,Path::generated,Mode::modern,50000000,false,1);
            std::cout<<"Integrated target entry="<<hex_address(target::entries[index])<<" natural_calls="<<natural[index]<<" prepared_parent_calls="<<prepared[index]<<std::endl;
        }
        require(cases==114&&negatives==8&&budgets==24&&faults==8,"Eight-root actual targeting matrix");
        require(child_returns[2]&&child_returns[3]&&child_returns[5]&&child_returns[6]&&child_returns[7],"Completed side/controlled/uncontrolled/strategy child composition");
        std::ofstream sites(std::filesystem::path(argv[3]).parent_path()/"match-target-runtime-sites.txt");
        for(unsigned n=0;n<8;++n){require(!complete_sites[n].empty(),"Complete productive source sites per target root");for(auto pc:complete_sites[n])sites<<n<<" "<<hex_address(pc)<<'\n';std::cout<<"Root "<<hex_address(target::entries[n])<<" complete_actual_sites="<<complete_sites[n].size()<<" child_returns="<<child_returns[n]<<std::endl;}
    }

    std::cout<<"PASS: production match targeting cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives
        <<" faults="<<faults<<"; real-context direct/generated/CFM, original/profile controls, complete state/effects/accounting/import boundaries\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
