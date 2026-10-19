#include "actor_encounter_adapter.hpp"
#include "range_sampling_adapter.hpp"
#include "import_descriptor_adapter.hpp"
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
namespace selection=cm::aot::actor_encounter;
enum class Path { direct, generated, cfm, original_bl };
constexpr std::array<std::uint32_t,4> callsites{0x102877a4,0,0,0};
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
struct Capture:ActorEncounterBoundaryObserver {
    std::array<std::unique_ptr<State>,selection::entries.size()> inputs;
    void enter(const State& s,std::uint32_t root,bool original)override {
        if(original)return;
        const auto entry=std::find(selection::entries.begin(),selection::entries.end(),root);
        require(entry!=selection::entries.end(),"Unknown target capture");
        auto& input=inputs[unsigned(entry-selection::entries.begin())];
        if(input)return;
        input=clone(s);input->platform=s.platform;input->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,ActorEncounterOutcome)override{}
};
std::array<std::set<std::uint32_t>,selection::entries.size()> complete_sites;
std::array<unsigned,4> random_returns{},power_returns{},team_children{},score_children{},response_children{},peer_children{};
constexpr std::array<std::uint32_t,4> ends{0x101d68ec,0x10286650,0x102875f0,0x10288228};
struct Negative:ActorEncounterBoundaryObserver {
    unsigned index;bool negative=false;std::set<std::uint32_t> seen;std::array<unsigned,4> returned{};
    explicit Negative(unsigned value):index(value){}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget)seen.insert(pc);}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,ActorEncounterOutcome result)override{
        if(original||result!=ActorEncounterOutcome::returned)return;
        const auto found=std::find(selection::entries.begin(),selection::entries.end(),entry);require(found!=selection::entries.end(),"Known target outcome");++returned[unsigned(found-selection::entries.begin())];
        if(negative&&entry==selection::entries[index]){s.wr<4>(12,s.r<4>(12)^1);}
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
        if(path==Path::direct)try_modern_actor_encounter(s,selection::entries[index]);
        else if(path==Path::original_bl){s.pc=callsites.at(index);run_until(s,callsites.at(index)+4);}
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,selection::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
void compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,unsigned fault=0,unsigned corrupt_root=UINT32_MAX) {
    auto a=clone(input),b=clone(input);a->platform=b->platform=input.platform;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=selection::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    if(fault)for(auto* s:{a.get(),b.get()}){
        s->wr<4>(12,0);
    }
    actor_encounter_diagnostics(*a).original=true;auto& d=actor_encounter_diagnostics(*b);
    range_sampling_diagnostics(*b)={};reset_import_descriptor_diagnostics(*b);b->rng_calls.fill(0);
    if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(corrupt_root==UINT32_MAX?index:corrupt_root);mutation.negative=negative;if(mode==Mode::modern)d.observer=&mutation;
    Clock old_clock,new_clock;new_clock.base=old_clock.base;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index),actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production encounter entry="+hex_address(selection::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
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
        const auto& sampler=range_sampling_diagnostics(*b);
        const auto power=import_descriptor::entry_index(0x10780fc0);
        power_returns[index]+=b->import_descriptor_tails[power];
        if(index==2)require(b->import_descriptor_calls[power]>0&&b->import_descriptor_tails[power]==b->import_descriptor_calls[power]&&!b->import_descriptor_original_calls[power],label+" real pow descriptor/tail");
        random_returns[index]+=sampler.returns[0]+sampler.returns[1]+b->rng_calls[0];
        team_children[index]+=mutation.returned[0];score_children[index]+=mutation.returned[1];response_children[index]+=mutation.returned[2];peer_children[index]+=mutation.returned[3];
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
        actor_encounter_diagnostics(world)={};Capture capture;actor_encounter_diagnostics(world).observer=&capture;
        for(unsigned step=0;step<600;++step){world.wr<4>(12,game.match);world.wr<4>(16,1);world.wr<4>(4128,return_sentinel);world.pc=0x101d3f60;world.budget=world.executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(world,return_sentinel);}
        actor_encounter_diagnostics(world).observer=nullptr;
        const auto natural=actor_encounter_diagnostics(world).calls;
        const auto& natural_diagnostics=actor_encounter_diagnostics(world);
        for(unsigned n=0;n<4;++n)require(natural[n]==natural_diagnostics.returns[n]&&!natural_diagnostics.original_calls[n]&&!natural_diagnostics.faults[n]&&!natural_diagnostics.budget_stops[n]&&!natural_diagnostics.dispatch_escapes,"Natural productive encounter outcomes");
        for(unsigned n=1;n<4;++n)require(bool(capture.inputs[n])&&natural[n]>0,"Natural encounter caller capture");
        std::array<std::uint64_t,4> prepared{};
        // Compose the complete peer root with real actors and real services.
        // Prepare one opponent in the actual cell and a zero kind-four
        // probability numerator. The unmodified pow import still executes.
        auto peer=clone(world);peer->platform=world.platform;
        const auto actor=game.match+0x311c;
        const auto opponent=actor+0x2350;
        require(peer->memory.read<4>(actor+0x6c)&&peer->memory.read<4>(opponent+0x6c)&&peer->memory.read<4>(opponent+0x70),"Actual staff and player records for composed encounter");
        const auto row=std::int8_t(peer->memory.read<1>(actor+0x105)),column=std::int8_t(peer->memory.read<1>(actor+0x106));
        require(row>=0&&column>=0,"Prepared encounter cell coordinates");
        const auto cell=game.match+0xa08+row*0x450+column*0x5c;
        const auto side=peer->memory.read<1>(actor+0x27);require(side<=1,"Actual actor side");
        peer->memory.write<1>(cell+0x58+!side,1);peer->memory.write<4>(cell+(!side)*0x2c,opponent);
        const auto constants=std::uint32_t(peer->memory.read<4>(peer->r<4>(8)-0x51e8));peer->memory.write<8>(constants+0x3c0,float_bits<8>(0));
        peer->wr<4>(12,actor);peer->wr<4>(16,10);peer->wr<4>(4128,return_sentinel);peer->pc=selection::entries[3];peer->budget=peer->executed+50000000;
        auto& prepared_diagnostics=actor_encounter_diagnostics(*peer);prepared_diagnostics={};prepared_diagnostics.observer=&capture;
        try_modern_actor_encounter(*peer,selection::entries[3]);prepared_diagnostics.observer=nullptr;
        require(prepared_diagnostics.calls[0]>0&&prepared_diagnostics.calls[0]==prepared_diagnostics.returns[0]&&prepared_diagnostics.calls[1]>=20&&prepared_diagnostics.calls[1]==prepared_diagnostics.returns[1],"Prepared peer composes complete team and actor scaling");
        prepared[0]=prepared_diagnostics.calls[0];require(bool(capture.inputs[0]),"Composed original BL team-scaling capture");
        compare(*capture.inputs[0],0,Path::original_bl,Mode::modern);
        // Replay the entire prepared peer against its original body as well.
        auto replay=clone(world);replay->platform=world.platform;replay->memory.write<1>(cell+0x58+!side,1);replay->memory.write<4>(cell+(!side)*0x2c,opponent);replay->memory.write<8>(constants+0x3c0,float_bits<8>(0));replay->wr<4>(12,actor);replay->wr<4>(16,10);
        compare(*replay,3,Path::generated,Mode::modern);
        require(team_children[3]>0&&score_children[3]>=20&&response_children[3]>0,"Actual completed peer/team/score/response composition");
        compare(*capture.inputs[0],0,Path::generated,Mode::modern,50000000,true,0,1);
        for(unsigned matching:{0u,1u}){
            auto input=clone(*capture.inputs[2]);input->platform=capture.inputs[2]->platform;const auto subject=std::uint32_t(input->r<4>(12));
            input->wr<4>(20,matching?subject:0);if(matching)input->memory.write<4>(subject+0x1ac,subject);compare(*input,2,Path::generated,Mode::modern);
        }
        // Prepared finite response bounds exercise the actual pool reseeder
        // and the existing native sampler. Only input data changes; pow,
        // descriptors, RNG and sampling execute their production services.
        for(unsigned kind=1;kind<=5;++kind)for(auto bound:{100.0,100000.0}){
            auto input=clone(*capture.inputs[2]);input->platform=capture.inputs[2]->platform;const auto subject=std::uint32_t(input->r<4>(12));
            const auto table=std::uint32_t(input->memory.read<4>(input->r<4>(8)-0x51e8));
            const auto exponent=float_value<8>(input->memory.read<8>(table+0xc8));
            input->memory.write<4>(subject+0x1a8,float_bits<4>(60));input->wr<4>(16,kind);input->wr<4>(20,0);
            if(kind>=2)input->memory.write<8>(table+0x3b0+(kind-2)*8,float_bits<8>(std::pow(60.0,exponent)*bound));
            const auto cursor_pointer=std::uint32_t(input->memory.read<4>(input->r<4>(8)-0x7b84));
            const auto end_pointer=std::uint32_t(input->memory.read<4>(input->r<4>(8)-0x7b88));
            input->memory.write<4>(cursor_pointer,input->memory.read<4>(end_pointer));compare(*input,2,Path::generated,Mode::modern);
        }
        require(random_returns[2]>0,"Completed actual pool reseeder/sampling service composition");
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"actor-encounter-live-profile";std::filesystem::create_directories(snapshots);
        for(unsigned index=0;index<selection::entries.size();++index){
            require(bool(capture.inputs[index]),"Missing real-caller selection capture "+hex_address(selection::entries[index]));
            require(natural[index]+prepared[index]>0,"Dedicated selection caller coverage");
            save_runtime_snapshot(*capture.inputs[index],snapshots/(hex_address(selection::entries[index])+".snapshot"));
            matrix(*capture.inputs[index],index,false);compare(*capture.inputs[index],index,Path::generated,Mode::modern,50000000,false,1);
            std::cout<<"Integrated encounter entry="<<hex_address(selection::entries[index])<<" natural_calls="<<natural[index]<<" prepared_bl_calls="<<prepared[index]<<std::endl;
        }
        require(cases==71&&negatives==5&&budgets==12&&faults==4,"Four-root actual encounter matrix");
        require(score_children[0]>0,"Completed actual actor-score child composition");
        std::ofstream sites(std::filesystem::path(argv[3]).parent_path()/"actor-encounter-runtime-sites.txt");
        for(unsigned n=0;n<4;++n){require(!complete_sites[n].empty(),"Complete productive source sites per selection root");for(auto pc:complete_sites[n])sites<<n<<" "<<hex_address(pc)<<'\n';std::cout<<"Root "<<hex_address(selection::entries[n])<<" complete_actual_sites="<<complete_sites[n].size()<<" random_returns="<<random_returns[n]<<" power_returns="<<power_returns[n]<<" team_children="<<team_children[n]<<" score_children="<<score_children[n]<<" response_children="<<response_children[n]<<" peer_children="<<peer_children[n]<<std::endl;}
    }

    std::cout<<"PASS: production actor encounter cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives
        <<" faults="<<faults<<"; real-context direct/generated/CFM, original/profile controls, complete state/effects/accounting/import boundaries\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
