#include "actor_effects_adapter.hpp"
#include "range_sampling_adapter.hpp"
#include "actor_encounter_adapter.hpp"
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
namespace effects=cm::aot::actor_effects;
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
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Selection awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
const std::array<std::vector<std::uint32_t>,3> original_callsites{{{0x101d93e8,0x101d9428},{0x101db110,0x101db11c,0x101ee4c8,0x101f1374,0x1025d4c0,0x10268018,0x10268844,0x1026a874,0x1026d088,0x1026d3fc,0x1026dc4c,0x1026fd08,0x102700b0,0x1027065c,0x10270814},{0x101daaf8,0x101dab04,0x101dae80,0x101daf04,0x101db018,0x101db09c,0x101e52b8,0x101e5854,0x101e5bcc,0x101e6354,0x101e65b8,0x101e67d8,0x101e688c,0x101f6b84,0x101f6ba4}}};
struct Capture:ActorEffectsBoundaryObserver {
    std::array<std::uint32_t,3> callers{};
    std::array<std::unique_ptr<State>,effects::entries.size()> inputs;
    void enter(const State& s,std::uint32_t root,bool original)override {
        if(original)return;
        const auto entry=std::find(effects::entries.begin(),effects::entries.end(),root);
        require(entry!=effects::entries.end(),"Unknown target capture");
        auto& input=inputs[unsigned(entry-effects::entries.begin())];
        if(input)return;
        if(root==effects::entries[0]){const auto slot=std::int8_t(s.memory.read<1>(s.r<4>(12)+0x19));if(slot<0||slot>10)return;}
        const auto index=unsigned(entry-effects::entries.begin());callers[index]=std::uint32_t(s.r<4>(4128))-4;
        require(std::find(original_callsites[index].begin(),original_callsites[index].end(),callers[index])!=original_callsites[index].end(),"Captured root lacks indexed original BL caller");
        input=clone(s);input->platform=s.platform;input->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,ActorEffectsOutcome)override{}
};
std::array<std::set<std::uint32_t>,effects::entries.size()> complete_sites;
std::array<unsigned,3> sample_returns{},score_returns{},role_returns{},rng_returns{};
constexpr std::array<std::uint32_t,3> ends{0x102393bc,0x10285ee0,0x10285ee8};
struct Negative:ActorEffectsBoundaryObserver {
    unsigned index;bool negative=false;std::set<std::uint32_t> seen;std::array<unsigned,3> returned{};
    explicit Negative(unsigned value):index(value){}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget)seen.insert(pc);}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,ActorEffectsOutcome result)override{
        if(original||result!=ActorEffectsOutcome::returned)return;
        const auto found=std::find(effects::entries.begin(),effects::entries.end(),entry);require(found!=effects::entries.end(),"Known target outcome");++returned[unsigned(found-effects::entries.begin())];
        if(negative&&entry==effects::entries[index]){s.wr<4>(12,s.r<4>(12)^1);}
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
        if(path==Path::direct)try_modern_actor_effects(s,effects::entries[index]);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,effects::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
std::unique_ptr<State> compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,unsigned fault=0,unsigned corrupt_root=UINT32_MAX) {
    auto a=clone(input),b=clone(input);a->platform=b->platform=input.platform;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=effects::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    if(fault)for(auto* s:{a.get(),b.get()}){
        s->wr<4>(12,0);
    }
    actor_effects_diagnostics(*a).original=true;auto& d=actor_effects_diagnostics(*b);
    range_sampling_diagnostics(*b)={};actor_encounter_diagnostics(*b)={};spatial_role_diagnostics(*b)={};b->rng_calls.fill(0);b->rng_original_calls.fill(0);
    if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(corrupt_root==UINT32_MAX?index:corrupt_root);mutation.negative=negative;if(mode==Mode::modern)d.observer=&mutation;
    Clock old_clock,new_clock;new_clock.base=old_clock.base;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index),actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production effects entry="+hex_address(effects::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
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
        for(auto pc:mutation.seen)if(effects::entries[index]<=pc&&pc<ends[index])complete_sites[index].insert(pc);
        if(index==0){
            const auto& sample=range_sampling_diagnostics(*b);const auto& score=actor_encounter_diagnostics(*b);const auto& role=spatial_role_diagnostics(*b);
            constexpr unsigned scorer=1,current_role=unsigned(spatial_role::Entry::actor_current_role);
            require(actor_encounter::entries[scorer]==0x102865a0&&spatial_role::entries[current_role]==0x10289560,"Known shared helper ownership");
            require(score.calls[scorer]==1&&score.returns[scorer]==1&&!score.original_calls[scorer],label+" actual score-scaling return");
            require(role.calls[current_role]>0&&role.returns[current_role]==role.calls[current_role]&&!role.original_calls[current_role],label+" actual role returns");
            require(sample.returns[0]==sample.calls[0]&&!sample.original_calls[0],label+" actual sampler outcome");
            sample_returns[index]+=sample.returns[0];score_returns[index]+=score.returns[scorer];role_returns[index]+=role.returns[current_role];rng_returns[index]+=b->rng_calls[0];
        }
    }
    ++cases;returns+=expected.empty();budgets+=!fault&&!expected.empty();faults+=bool(fault);return a;
}
void matrix(const State& input,unsigned index,bool profile) {
    for(auto path:{Path::direct,Path::generated,Path::cfm}){
        if(profile)compare(input,index,path,Mode::profile);
        else for(auto mode:{Mode::modern,Mode::original,Mode::entry_original})compare(input,index,path,mode);
    }
    if(!profile){compare(input,index,Path::generated,Mode::modern,50000000,true);compare(input,index,Path::generated,Mode::modern,0);compare(input,index,Path::generated,Mode::modern,1);if(index!=2)compare(input,index,Path::generated,Mode::modern,2);}
}
}
int main(int argc,char** argv){try {
    require(argc==4||argc==5,"Expected original program, game, private backing and optional profile snapshots");
    const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Profile request/flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());OwnedBacking cleanup{root};
    if(profile){
        for(unsigned index=0;index<effects::entries.size();++index){
            State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
            load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(effects::entries[index])+".snapshot"));
            require(input.pc==effects::entries[index],"Snapshot is not the captured real-caller effects entry");matrix(input,index,true);
        }
        require(cases==9&&returns==9&&!budgets&&!faults&&!negatives,"Three-root profile-original matrix");
    }else{
        auto game=match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto& world=*game.state;
        actor_effects_diagnostics(world)={};Capture capture;actor_effects_diagnostics(world).observer=&capture;
        for(unsigned step=0;step<600;++step){world.wr<4>(12,game.match);world.wr<4>(16,1);world.wr<4>(4128,return_sentinel);world.pc=0x101d3f60;world.budget=world.executed+match_debugger::fresh_match_step_limit;run_until(world,return_sentinel);}
        auto& d=actor_effects_diagnostics(world);d.observer=nullptr;const auto natural=d.calls;
        for(unsigned n=0;n<3;++n)require(natural[n]>0&&natural[n]==d.returns[n]&&!d.original_calls[n]&&!d.faults[n]&&!d.budget_stops[n]&&!d.dispatch_escapes,"Natural productive effects calls per root");
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"actor-effects-live-profile";std::filesystem::create_directories(snapshots);
        for(unsigned index=0;index<3;++index){
            require(bool(capture.inputs[index]),"Missing actual caller capture");
            save_runtime_snapshot(*capture.inputs[index],snapshots/(hex_address(effects::entries[index])+".snapshot"));
            matrix(*capture.inputs[index],index,false);compare(*capture.inputs[index],index,Path::generated,Mode::modern,50000000,false,1);
            std::cout<<"Integrated effects entry="<<hex_address(effects::entries[index])<<" natural_calls="<<natural[index]<<" caller="<<hex_address(capture.callers[index])<<std::endl;
        }
        // Prepared real-context alternatives remain separate from natural callers.
        for(auto counter:{0u,1u,127u,128u,255u}){
            auto input=clone(*capture.inputs[0]);input->platform=capture.inputs[0]->platform;const auto actor=std::uint32_t(input->r<4>(12));input->memory.write<1>(actor+0x2c,counter);
            auto result=compare(*input,0,Path::generated,Mode::modern);require(result->memory.read<1>(actor+0x2c)==(counter<128?(counter?counter-1:0):0),"Actual signed counter normalization oracle");
        }
        for(auto role:{1u,2u,4u,8u,16u,32u,64u,0x84u,0x884u}){
            auto input=clone(*capture.inputs[0]);input->platform=capture.inputs[0]->platform;const auto actor=std::uint32_t(input->r<4>(12));
            const auto side=std::uint32_t(std::int8_t(input->memory.read<1>(actor+0x27)));const auto slot=std::int8_t(input->memory.read<1>(actor+0x19));const auto match=input->memory.read<4>(actor+0x1a4);
            require(slot>=0&&slot<=10,"Captured active actor slot");const auto table=input->memory.read<4>(match+0x7934+side*4);require(table!=0,"Actual tactical role table");input->memory.write<2>(table+unsigned(slot)*2,role);
            compare(*input,0,Path::generated,Mode::modern);
        }
        {auto input=clone(*capture.inputs[0]);input->platform=capture.inputs[0]->platform;const auto toc=std::uint32_t(input->r<4>(8));const auto cursor=input->memory.read<4>(toc-0x7b84),end=input->memory.read<4>(toc-0x7b88);input->memory.write<4>(end,input->memory.read<4>(cursor));compare(*input,0,Path::generated,Mode::modern);}
        for(auto increment:{1u,0xffffffffu,0x80000000u}){
            auto input=clone(*capture.inputs[1]);input->platform=capture.inputs[1]->platform;const auto actor=std::uint32_t(input->r<4>(12));input->memory.write<1>(actor+0x2c,255);input->wr<4>(16,increment);
            auto result=compare(*input,1,Path::generated,Mode::modern);require(result->memory.read<1>(actor+0x2c)==((255+increment)&255),"Actual byte-add oracle");
        }
        for(auto value:{0u,1u,127u,128u,255u,256u,0xabcdef01u}){
            auto input=clone(*capture.inputs[2]);input->platform=capture.inputs[2]->platform;const auto actor=std::uint32_t(input->r<4>(12));input->wr<4>(16,value);
            auto result=compare(*input,2,Path::generated,Mode::modern);require(result->memory.read<1>(actor+0x2c)==(value&255),"Actual byte-assignment oracle");
        }
        require(cases==66&&returns==55&&negatives==3&&budgets==8&&faults==3,"Three-root actual effects matrix");
        require(score_returns[0]==18&&role_returns[0]>0&&rng_returns[0]>0,"Complete real score/role/reseed composition");
        require(sample_returns[0]==0,"Byte/time bounded actual fixtures must not claim generic sampler calls");
        std::ofstream sites(std::filesystem::path(argv[3]).parent_path()/"actor-effects-runtime-sites.txt");
        for(unsigned n=0;n<3;++n){require(!complete_sites[n].empty(),"Complete actual own source sites per root");for(auto pc:complete_sites[n])sites<<n<<" "<<hex_address(pc)<<'\n';std::cout<<"Root "<<hex_address(effects::entries[n])<<" complete_actual_sites="<<complete_sites[n].size()<<" sample_returns="<<sample_returns[n]<<" score_returns="<<score_returns[n]<<" role_returns="<<role_returns[n]<<" rng_returns="<<rng_returns[n]<<std::endl;}
    }
    std::cout<<"PASS: production actor effects cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<"; real-context direct/generated/CFM, original/profile controls, complete state/effects/accounting/import boundaries\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
