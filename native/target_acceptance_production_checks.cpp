#include "target_acceptance_adapter.hpp"
#include "range_sampling_adapter.hpp"
#include "pitch_distance_adapter.hpp"
#include "marking_pressure_adapter.hpp"
#include "movement_choice_adapter.hpp"
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
namespace acceptance=cm::aot::target_acceptance;
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
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Target fixture awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
const std::array<std::vector<std::uint32_t>,4> original_callsites{{{0x101e8d84,0x10237488,0x102374dc,0x1023de8c,0x1023dee4,0x10242a10,0x10242a68,0x102445b0,0x10244608,0x102448e0,0x10244938,0x102524a4,0x102524fc,0x10252a00,0x10252a58,0x10253494,0x102534ec,0x10253d80,0x10253dd8,0x10254230,0x10254288,0x1027c398,0x1027c3f0,0x1027c578,0x1027c5d0,0x1027e188,0x1027e1e0,0x1028097c,0x102809d4,0x102810d4,0x1028112c},{0x101e9d68,0x10244d10,0x102812a4},{0x10244e8c},{0x101eefe8,0x101ef0ec,0x101ef264,0x101ef37c,0x101f0398,0x101f03dc,0x101f056c,0x101f05b0,0x101f075c,0x101f07a0,0x101f0930,0x101f0974}}};
struct Capture:TargetAcceptanceBoundaryObserver {
    std::array<std::uint32_t,4> callers{};
    std::array<std::unique_ptr<State>,acceptance::entries.size()> inputs;
    void enter(const State& s,std::uint32_t root,bool original)override {
        if(original)return;
        const auto entry=std::find(acceptance::entries.begin(),acceptance::entries.end(),root);
        require(entry!=acceptance::entries.end(),"Unknown target capture");
        auto& input=inputs[unsigned(entry-acceptance::entries.begin())];
        if(input)return;
        const auto index=unsigned(entry-acceptance::entries.begin());callers[index]=std::uint32_t(s.r<4>(4128))-4;
        require(std::find(original_callsites[index].begin(),original_callsites[index].end(),callers[index])!=original_callsites[index].end(),"Captured root lacks indexed original BL caller");
        input=clone(s);input->platform=s.platform;input->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,TargetAcceptanceOutcome)override{}
};
std::array<std::set<std::uint32_t>,acceptance::entries.size()> complete_sites;
std::array<unsigned,4> sample_returns{},distance_returns{},role_returns{},position_returns{},side_returns{},rng_returns{},response_returns{};
std::array<std::array<unsigned,4>,4> owned_returns{};
constexpr std::array<std::uint32_t,4> ends{0x10245050,0x101eeca4,0x101ef3c0,0x101f0ac4};
struct Negative:TargetAcceptanceBoundaryObserver {
    unsigned index;bool negative=false;std::set<std::uint32_t> seen;std::array<unsigned,4> returned{};unsigned response_calls=0,response_completions=0;
    explicit Negative(unsigned value):index(value){}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget){seen.insert(pc);response_completions+=pc==0x101f0a70;}}
    void before_dependency(State&,std::uint32_t,std::uint32_t entry)override{response_calls+=entry==0x10239f10;}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,TargetAcceptanceOutcome result)override{
        if(original||result!=TargetAcceptanceOutcome::returned)return;
        const auto found=std::find(acceptance::entries.begin(),acceptance::entries.end(),entry);require(found!=acceptance::entries.end(),"Known target outcome");++returned[unsigned(found-acceptance::entries.begin())];
        if(negative&&entry==acceptance::entries[index]){s.wr<4>(12,s.r<4>(12)^1);}
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
        if(path==Path::direct)try_modern_target_acceptance(s,acceptance::entries[index]);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,acceptance::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
std::unique_ptr<State> compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,unsigned fault=0,unsigned corrupt_root=UINT32_MAX) {
    auto a=clone(input),b=clone(input);a->platform=b->platform=input.platform;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=acceptance::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    if(fault)for(auto* s:{a.get(),b.get()}){
        s->wr<4>(index==3?20:index==1?16:12,0);
        if(index==2){s->wr<4>(16,0);s->wr<4>(20,0);s->wr<4>(24,0);s->wr<4>(28,1);}
    }
    target_acceptance_diagnostics(*a).original=true;auto& d=target_acceptance_diagnostics(*b);
    range_sampling_diagnostics(*b)={};pitch_distance_diagnostics(*b)={};spatial_role_diagnostics(*b)={};marking_pressure_diagnostics(*b)={};movement_choice_diagnostics(*b)={};b->rng_calls.fill(0);b->rng_original_calls.fill(0);
    if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(corrupt_root==UINT32_MAX?index:corrupt_root);mutation.negative=negative;if(mode==Mode::modern)d.observer=&mutation;
    Clock old_clock,new_clock;new_clock.base=old_clock.base;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index),actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production target acceptance entry="+hex_address(acceptance::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(fault?expected.starts_with("Unmapped data access at "):limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected boundary="+expected);
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,label+" host input/import ABI");
    if(negative){if(corrupt_root!=UINT32_MAX)require(mutation.returned[corrupt_root]>0,label+" child negative was not invoked");bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative failed");++negatives;}
    else {Image(*a).compare(*b,label);require(x.bytes==y.bytes && a->executed==b->executed,label+" ordered effects/accounting");}
    require(!d.original_body && !d.dispatch_escapes,label+" own-body scope residue/dispatch escape");
    if(mode==Mode::modern){require(d.calls[index]>=1 && !d.original_calls[index] && !d.profile_original_calls[index],label+" no dedicated modern call");
        require(d.returns[index]==(expected.empty()?d.calls[index]:0) && d.budget_stops[index]==unsigned(!fault&&!expected.empty()) && d.faults[index]==unsigned(bool(fault)),label+" productive outcome counts");}
    else {require(!d.calls[index] && !d.returns[index],label+" original control entered C++ root");require(mode==Mode::profile?d.profile_original_calls[index]>=1:d.original_calls[index]>=1,label+" selected original/profile not reached");}
    if(mode==Mode::modern&&expected.empty()&&!negative){
        for(auto pc:mutation.seen)for(unsigned n=0;n<4;++n)if(acceptance::entries[n]<=pc&&pc<ends[n])complete_sites[n].insert(pc);
        for(unsigned n=0;n<4;++n){require(d.calls[n]==d.returns[n]&&!d.original_calls[n]&&!d.profile_original_calls[n],label+" complete own child return");owned_returns[index][n]+=d.returns[n];}
        const auto accumulate=[&](const auto& child){unsigned total=0;for(unsigned n=0;n<child.calls.size();++n){require(child.calls[n]==child.returns[n]&&!child.original_calls[n],label+" completed native child return");total+=child.returns[n];}return total;};
        sample_returns[index]+=accumulate(range_sampling_diagnostics(*b));distance_returns[index]+=accumulate(pitch_distance_diagnostics(*b));role_returns[index]+=accumulate(spatial_role_diagnostics(*b));position_returns[index]+=accumulate(movement_choice_diagnostics(*b));side_returns[index]+=accumulate(marking_pressure_diagnostics(*b));rng_returns[index]+=b->rng_calls[0];
        require(mutation.response_calls==mutation.response_completions,label+" complete generated response call/continuation");response_returns[index]+=mutation.response_completions;

    }
    ++cases;returns+=expected.empty();budgets+=!fault&&!expected.empty();faults+=bool(fault);return a;
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
    require(argc==4||argc==5,"Expected original program, game, private backing and optional profile snapshots");
    const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Profile request/flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());OwnedBacking cleanup{root};
    if(profile){
        for(unsigned index=0;index<acceptance::entries.size();++index){
            State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
            load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(acceptance::entries[index])+".snapshot"));
            require(input.pc==acceptance::entries[index],"Snapshot is not the captured real-caller target entry");matrix(input,index,true);
        }
        require(cases==12&&returns==12&&!budgets&&!faults&&!negatives,"Four-root profile-original matrix");
    }else{
        auto game=match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto& world=*game.state;
        target_acceptance_diagnostics(world)={};Capture capture;target_acceptance_diagnostics(world).observer=&capture;
        for(unsigned step=0;step<600;++step){world.wr<4>(12,game.match);world.wr<4>(16,1);world.wr<4>(4128,return_sentinel);world.pc=0x101d3f60;world.budget=world.executed+match_debugger::fresh_match_step_limit;run_until(world,return_sentinel);}
        auto& d=target_acceptance_diagnostics(world);d.observer=nullptr;const auto natural=d.calls;
        for(unsigned n=0;n<4;++n)require(natural[n]>0&&natural[n]==d.returns[n]&&!d.original_calls[n]&&!d.faults[n]&&!d.budget_stops[n]&&!d.dispatch_escapes,"Natural productive target acceptance calls per root");
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"target-acceptance-live-profile";std::filesystem::create_directories(snapshots);
        for(unsigned index=0;index<4;++index){
            require(bool(capture.inputs[index]),"Missing actual caller capture");
            save_runtime_snapshot(*capture.inputs[index],snapshots/(hex_address(acceptance::entries[index])+".snapshot"));
            matrix(*capture.inputs[index],index,false);compare(*capture.inputs[index],index,Path::generated,Mode::modern,50000000,false,1);
            std::cout<<"Integrated target acceptance entry="<<hex_address(acceptance::entries[index])<<" natural_calls="<<natural[index]<<" caller="<<hex_address(capture.callers[index])<<std::endl;
        }
        // Prepared alternatives use captured match memory and actual services;
        // their source registers/conditions are never checkpoint-modified.
        for(auto qualify:{0u,1u})for(auto heading:{0u,45u,180u,359u}){
            auto input=clone(*capture.inputs[0]);input->platform=capture.inputs[0]->platform;const auto actor=std::uint32_t(input->r<4>(12));input->memory.write<2>(actor+0x19e,heading);input->wr<4>(24,qualify);input->wr<4>(36,fixture+0x800);compare(*input,0,Path::generated,Mode::modern);
        }
        for(auto side:{0u,1u})for(auto wrap:{false,true}){
            auto input=clone(*capture.inputs[0]);input->platform=capture.inputs[0]->platform;input->memory.map(fixture,0x1000);const auto actor=std::uint32_t(input->r<4>(12));
            input->memory.write<1>(actor+0x105,1);input->memory.write<1>(actor+0x106,side?2:9);input->memory.write<1>(actor+0x27,side);input->memory.write<1>(actor+0x10e,20);
            input->wr<4>(16,5);input->wr<4>(20,side?1:10);input->wr<4>(24,1);input->wr<4>(36,fixture+0x800);
            const auto table=std::uint32_t(input->memory.read<4>(std::uint32_t(input->r<4>(8))-0x641c));const auto angle=input->memory.read<2>(table+4*0x30+std::uint32_t(side?-1:1)*2+0x1c8);input->memory.write<2>(actor+0x19e,angle+1);
            if(wrap){const auto toc=std::uint32_t(input->r<4>(8));const auto cursor=input->memory.read<4>(toc-0x7b84),end=input->memory.read<4>(toc-0x7b88);input->memory.write<4>(end,input->memory.read<4>(cursor));}
            compare(*input,0,Path::generated,Mode::modern);
            if(side==0&&!wrap)compare(*input,0,Path::generated,Mode::modern,50000000,true,0,1);
        }
        for(auto side:{0u,1u})for(auto x:{1u,4u,7u}){
            auto input=clone(*capture.inputs[1]);input->platform=capture.inputs[1]->platform;const auto actor=std::uint32_t(input->r<4>(16));input->memory.write<1>(actor+0x27,side);input->memory.write<1>(actor+0x105,x);input->memory.write<1>(actor+0x106,side?2:9);input->wr<4>(20,5);input->wr<4>(24,side?1:10);compare(*input,1,Path::generated,Mode::modern);
        }
        for(auto points:std::array<std::array<unsigned,4>,6>{{{0,0,0,11},{0,0,8,0},{1,1,7,4},{7,4,1,1},{1,1,4,10},{4,10,1,1}}}){
            auto input=clone(*capture.inputs[2]);input->platform=capture.inputs[2]->platform;for(unsigned n=0;n<4;++n)input->wr<4>(16+n*4,points[n]);compare(*input,2,Path::generated,Mode::modern);
        }
        for(auto mode:{0u,1u,2u})for(auto count:{0u,1u}){
            auto input=clone(*capture.inputs[3]);input->platform=capture.inputs[3]->platform;input->wr<4>(40,mode);const auto cell=std::uint32_t(input->r<4>(20)),side=std::uint32_t(std::int8_t(input->r<4>(16)));input->memory.write<1>(cell+0x58+side,count);
            if(count){require(side<=1,"Captured cell side");const auto actor=std::uint32_t(input->r<4>(12))+0x311c+side*0x2350;input->memory.write<4>(cell+side*0x2c,actor);input->memory.write<1>(actor+0x2c,0);input->memory.write<1>(actor+0x19,0);}
            compare(*input,3,Path::generated,Mode::modern);
        }
        require(cases==87&&returns==71&&negatives==5&&budgets==12&&faults==4,"Four-root actual target/corridor matrix");
        unsigned distance=0,role=0,position=0,side=0,rng=0,response=0;
        for(unsigned n=0;n<4;++n){distance+=distance_returns[n];role+=role_returns[n];position+=position_returns[n];side+=side_returns[n];rng+=rng_returns[n];response+=response_returns[n];}
        require(distance>0&&role>0&&position>0&&side>0&&rng>0&&response>0,"Complete returning real dependency composition");
        std::ofstream sites(std::filesystem::path(argv[3]).parent_path()/"target-acceptance-runtime-sites.txt");
        for(unsigned n=0;n<4;++n){require(!complete_sites[n].empty(),"Complete actual own source sites per root");for(auto pc:complete_sites[n])sites<<n<<" "<<hex_address(pc)<<'\n';std::cout<<"Root "<<hex_address(acceptance::entries[n])<<" complete_actual_sites="<<complete_sites[n].size()<<" sample_returns="<<sample_returns[n]<<" distance_returns="<<distance_returns[n]<<" role_returns="<<role_returns[n]<<" position_returns="<<position_returns[n]<<" side_returns="<<side_returns[n]<<" rng_returns="<<rng_returns[n]<<" response_returns="<<response_returns[n]<<" owned_returns=";for(auto returned:owned_returns[n])std::cout<<returned<<",";std::cout<<std::endl;}

    }
    std::cout<<"PASS: production target acceptance cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<"; real-context direct/generated/CFM, original/profile controls, complete state/effects/accounting/import boundaries\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
