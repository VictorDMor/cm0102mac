#include "actor_flags_adapter.hpp"
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
namespace flags=cm::aot::actor_flags;
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
const std::array<std::vector<std::uint32_t>,7> original_callsites{{{0x1022bec0,0x10281dc4},{0x1027dc60},{0x1027e1f4},{0x1027e0d4},{0x101eb1c0,0x101eb250,0x101eb28c},{0x101e8edc,0x101e8f3c,0x101e8f84,0x101e9230,0x101e92f8,0x101e933c,0x101e9380,0x101e9e74,0x101e9f18,0x101e9fa8,0x101ea034,0x101ea3dc,0x10247c6c,0x10247edc,0x1027e094,0x1027f83c,0x1027fd48,0x102824e4},{0x101d7bf0,0x10282940}}};
struct Capture:ActorFlagsBoundaryObserver {
    std::array<std::uint32_t,7> callers{};
    std::array<std::unique_ptr<State>,flags::entries.size()> inputs;
    void enter(const State& s,std::uint32_t root,bool original)override {
        if(original)return;
        const auto entry=std::find(flags::entries.begin(),flags::entries.end(),root);
        require(entry!=flags::entries.end(),"Unknown target capture");
        auto& input=inputs[unsigned(entry-flags::entries.begin())];
        if(input)return;
        const auto index=unsigned(entry-flags::entries.begin());callers[index]=std::uint32_t(s.r<4>(4128))-4;
        require(std::find(original_callsites[index].begin(),original_callsites[index].end(),callers[index])!=original_callsites[index].end(),"Captured root lacks indexed original BL caller");
        input=clone(s);input->platform=s.platform;input->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,ActorFlagsOutcome)override{}
};
std::array<std::set<std::uint32_t>,flags::entries.size()> complete_sites;
std::array<unsigned,7> shared_returns{};
constexpr std::array<std::uint32_t,7> ends{0x10286320,0x10286390,0x10286400,0x10286470,0x102864ac,0x102864ec,0x1028659c};
struct Negative:ActorFlagsBoundaryObserver {
    unsigned index;bool negative=false;std::set<std::uint32_t> seen;std::array<unsigned,7> returned{};
    explicit Negative(unsigned value):index(value){}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget)seen.insert(pc);}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,ActorFlagsOutcome result)override{
        if(original||result!=ActorFlagsOutcome::returned)return;
        const auto found=std::find(flags::entries.begin(),flags::entries.end(),entry);require(found!=flags::entries.end(),"Known target outcome");++returned[unsigned(found-flags::entries.begin())];
        if(negative&&entry==flags::entries[index]){s.wr<4>(12,s.r<4>(12)^1);}
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
        if(path==Path::direct)try_modern_actor_flags(s,flags::entries[index]);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,flags::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
void compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,unsigned fault=0,unsigned corrupt_root=UINT32_MAX) {
    auto a=clone(input),b=clone(input);a->platform=b->platform=input.platform;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=flags::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    if(fault)for(auto* s:{a.get(),b.get()}){
        s->wr<4>(12,0);
    }
    actor_flags_diagnostics(*a).original=true;auto& d=actor_flags_diagnostics(*b);
    match_target_diagnostics(*a).original=true;match_target_diagnostics(*b)={};
    if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(corrupt_root==UINT32_MAX?index:corrupt_root);mutation.negative=negative;if(mode==Mode::modern)d.observer=&mutation;
    Clock old_clock,new_clock;new_clock.base=old_clock.base;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index),actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production flags entry="+hex_address(flags::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
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
        for(auto pc:mutation.seen)if(flags::entries[index]<=pc&&pc<ends[index])complete_sites[index].insert(pc);
        const auto& shared=match_target_diagnostics(*b);
        constexpr unsigned reader=7;
        require(match_target::entries[reader]==0x10113e10,"Known shared strategy reader");
        require(shared.calls[reader]==shared.returns[reader]&&!shared.original_calls[reader],label+" shared reader complete native outcome");
        if(index<6)require(shared.returns[reader]==1,label+" required shared-reader composition");
        shared_returns[index]+=shared.returns[reader];
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
    require(argc==4||argc==5,"Expected original program, game, private backing and optional profile snapshots");
    const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Profile request/flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());OwnedBacking cleanup{root};
    if(profile){
        for(unsigned index=0;index<flags::entries.size();++index){
            State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
            load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(flags::entries[index])+".snapshot"));
            require(input.pc==flags::entries[index],"Snapshot is not the captured real-caller flags entry");matrix(input,index,true);
        }
        require(cases==21&&returns==21&&!budgets&&!faults&&!negatives,"Seven-root profile-original matrix");
    }else{
        auto game=match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto& world=*game.state;
        actor_flags_diagnostics(world)={};Capture capture;actor_flags_diagnostics(world).observer=&capture;
        for(unsigned step=0;step<600;++step){world.wr<4>(12,game.match);world.wr<4>(16,1);world.wr<4>(4128,return_sentinel);world.pc=0x101d3f60;world.budget=world.executed+match_debugger::fresh_match_step_limit;run_until(world,return_sentinel);}
        auto& d=actor_flags_diagnostics(world);d.observer=nullptr;const auto natural=d.calls;
        for(unsigned n=0;n<7;++n)require(natural[n]>0&&natural[n]==d.returns[n]&&!d.original_calls[n]&&!d.faults[n]&&!d.budget_stops[n]&&!d.dispatch_escapes,"Natural productive flags calls per root");
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"actor-flags-live-profile";std::filesystem::create_directories(snapshots);
        for(unsigned index=0;index<7;++index){
            require(bool(capture.inputs[index]),"Missing actual caller capture");
            save_runtime_snapshot(*capture.inputs[index],snapshots/(hex_address(flags::entries[index])+".snapshot"));
            matrix(*capture.inputs[index],index,false);compare(*capture.inputs[index],index,Path::generated,Mode::modern,50000000,false,1);
            std::cout<<"Integrated flags entry="<<hex_address(flags::entries[index])<<" natural_calls="<<natural[index]<<" caller="<<hex_address(capture.callers[index])<<std::endl;
        }
        // Prepared real-context alternatives are separate from natural callers.
        constexpr unsigned mask_bits[]{21,25,29,27};
        for(unsigned index=0;index<4;++index)for(bool present:{false,true}){
            auto input=clone(*capture.inputs[index]);input->platform=capture.inputs[index]->platform;
            const auto actor=std::uint32_t(input->r<4>(12));const auto side=std::uint32_t(std::int8_t(input->memory.read<1>(actor+0x27)));
            const auto slot=std::uint32_t(std::int8_t(input->memory.read<1>(actor+0x19)));const auto match=input->memory.read<4>(actor+0x1a4);
            const auto word=match+0x7c6c+side*0x18e0+0x188c+slot*8;input->memory.write<4>(word,present?1u<<mask_bits[index]:0);
            compare(*input,index,Path::generated,Mode::modern);
        }
        for(bool override:{false,true}){
            auto input=clone(*capture.inputs[6]);input->platform=capture.inputs[6]->platform;
            const auto actor=std::uint32_t(input->r<4>(12));const auto side=std::uint32_t(std::int8_t(input->memory.read<1>(actor+0x27)));const auto match=input->memory.read<4>(actor+0x1a4);
            const auto status=match+0x81f4+side*0x18e0;const auto old=input->memory.read<4>(status);
            input->memory.write<4>(status,override?(old|0x1000):(old&~0x1000u));if(override)input->memory.write<1>(match+side+6,255);
            compare(*input,6,Path::generated,Mode::modern);
        }
        require(cases==108&&returns==80&&negatives==7&&budgets==21&&faults==7,"Seven-root actual flags matrix");
        std::ofstream sites(std::filesystem::path(argv[3]).parent_path()/"actor-flags-runtime-sites.txt");unsigned total=0;
        for(unsigned n=0;n<7;++n){require(complete_sites[n].size()==(ends[n]-flags::entries[n])/4,"Complete actual own source-site coverage");require(shared_returns[n]>0,"Complete actual shared-reader composition per root");total+=complete_sites[n].size();for(auto pc:complete_sites[n])sites<<n<<" "<<hex_address(pc)<<'\n';std::cout<<"Root "<<hex_address(flags::entries[n])<<" complete_actual_sites="<<complete_sites[n].size()<<" shared_reader_returns="<<shared_returns[n]<<std::endl;}
        require(total==185,"Complete seven-root actual source-site total");
    }
    std::cout<<"PASS: production actor strategy flags cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<"; real-context direct/generated/CFM, original/profile controls, complete state/effects/accounting/import boundaries\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
