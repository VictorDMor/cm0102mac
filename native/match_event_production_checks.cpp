#include "match_event_adapter.hpp"
#include "rng_core_adapter.hpp"
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
namespace selection=cm::aot::match_event;
enum class Path { direct, generated, cfm, original_bl };
constexpr std::array<std::uint32_t,9> callsites{0x10201be8,0x101ffbac,0x1020135c,0x10200b70,0x1020128c,0x101ffef0,0x102002e4,0x10201310,0x10201cac};
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
struct Capture:MatchEventBoundaryObserver {
    std::array<std::unique_ptr<State>,selection::entries.size()> inputs;
    void enter(const State& s,std::uint32_t root,bool original)override {
        if(original)return;
        const auto entry=std::find(selection::entries.begin(),selection::entries.end(),root);
        require(entry!=selection::entries.end(),"Unknown target capture");
        auto& input=inputs[unsigned(entry-selection::entries.begin())];
        if(input)return;
        input=clone(s);input->platform=s.platform;input->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,MatchEventOutcome)override{}
};
std::array<std::set<std::uint32_t>,selection::entries.size()> complete_sites;
std::array<std::array<unsigned,9>,9> completed_children{};std::array<unsigned,9> random_calls{};
constexpr std::array<std::uint32_t,9> ends{0x101e31a0,0x101fea8c,0x101ff5ec,0x101ffb68,0x10201078,0x102010c0,0x10201180,0x10201e30,0x10201ee4};
struct Negative:MatchEventBoundaryObserver {
    unsigned index;bool negative=false;std::set<std::uint32_t> seen;std::array<unsigned,9> returned{};
    explicit Negative(unsigned value):index(value){}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget)seen.insert(pc);}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,MatchEventOutcome result)override{
        if(original||result!=MatchEventOutcome::returned)return;
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
        if(path==Path::direct)try_modern_match_event(s,selection::entries[index]);
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
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x2000);s->pc=selection::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    if(fault)for(auto* s:{a.get(),b.get()}){
        if(index==3)s->memory.write<4>(s->r<4>(8)-0x7b84,0);else s->wr<4>((index==1||index==7)?12:16,0);
    }
    match_event_diagnostics(*a).original=true;auto& d=match_event_diagnostics(*b);
    reset_rng_diagnostics(*b);
    if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(corrupt_root==UINT32_MAX?index:corrupt_root);mutation.negative=negative;if(mode==Mode::modern)d.observer=&mutation;
    Clock old_clock,new_clock;new_clock.base=old_clock.base;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index),actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production event entry="+hex_address(selection::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
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
        for(unsigned child=0;child<9;++child)completed_children[index][child]+=mutation.returned[child]-(child==index?1u:0u);
        random_calls[index]+=b->rng_calls[0];
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
std::unique_ptr<State> prepare_input(const State& input,unsigned index,std::uint32_t queue,std::uint32_t match){
    auto s=clone(input);s->platform=input.platform;s->memory.map(fixture,0x2000);s->memory.copy_in(fixture+0x800,s->memory.copy_out(queue+0x30,20));s->memory.write<2>(fixture+0x800,0x1f40);
    s->wr<4>(12,queue);s->wr<4>(16,fixture+0x800);s->wr<4>(20,match);s->wr<4>(24,0);s->wr<4>(28,0);s->wr<4>(32,UINT32_MAX);s->wr<4>(36,UINT32_MAX);s->wr<4>(40,0);
    if(index==0||index==1){s->wr<4>(12,match);s->wr<4>(16,0);s->wr<4>(20,0);}
    if(index==2){s->wr<4>(20,s->memory.read<4>(match+0x30d8));s->wr<4>(24,s->memory.read<1>(match+1));}
    if(index==3)s->wr<4>(12,100000);
    if(index==5||index==6)s->wr<4>(20,queue+0x30);
    if(index==7)s->wr<4>(16,0x1f40);
    if(index==8)s->wr<4>(16,0x1f46);
    return s;
}
void prepare_pool(State& s,unsigned value,bool wrap){
    const auto toc=std::uint32_t(s.r<4>(8)),base=std::uint32_t(s.memory.read<4>(toc-0x7b8c));
    const auto end_ref=std::uint32_t(s.memory.read<4>(toc-0x7b88)),cursor_ref=std::uint32_t(s.memory.read<4>(toc-0x7b84)),salt_ref=std::uint32_t(s.memory.read<4>(toc-0x7b90));
    const auto last=std::uint32_t(s.memory.read<4>(end_ref));require(last>=base&&last-base<=0x20000,"Actual bounded random pool base="+hex_address(base)+" last="+hex_address(last));
    for(auto address=base;address<=last;address+=2)s.memory.write<2>(address,value);
    s.memory.write<4>(cursor_ref,wrap?last:base);s.memory.write<4>(salt_ref,0);
}
int main(int argc,char** argv){try {
    require(argc==4||argc==5,"Expected original program, game, fresh backing and optional profile directory");
    const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Profile request/flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());OwnedBacking cleanup{root};
    if(profile){
        for(unsigned index=0;index<9;++index){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(selection::entries[index])+".snapshot"));require(input.pc==selection::entries[index],"Captured real-caller event entry");matrix(input,index,true);}
    }else{
        auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto& world=*game.state;
        match_event_diagnostics(world)={};Capture capture;match_event_diagnostics(world).observer=&capture;
        for(unsigned step=0;step<600;++step){world.wr<4>(12,game.match);world.wr<4>(16,1);world.wr<4>(4128,return_sentinel);world.pc=0x101d3f60;world.budget=world.executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(world,return_sentinel);}
        match_event_diagnostics(world).observer=nullptr;const auto natural=match_event_diagnostics(world).calls;const auto& nd=match_event_diagnostics(world);
        for(unsigned n=0;n<9;++n)require(natural[n]==nd.returns[n]&&!nd.original_calls[n]&&!nd.faults[n]&&!nd.budget_stops[n]&&!nd.dispatch_escapes,"Natural productive event outcomes");
        for(unsigned n:{2u,4u,7u})require(bool(capture.inputs[n])&&natural[n]>0,"Measured natural event/transition/qualifier capture");
        const auto& base=*capture.inputs[7];const auto queue=std::uint32_t(base.r<4>(12));require(base.r<4>(20)==game.match&&queue,"Actual event queue and match context");
        std::array<std::uint64_t,9> prepared{};unsigned prepared_comparisons=0;
        for(unsigned index=0;index<9;++index)if(!capture.inputs[index]){
            auto input=prepare_input(base,index,queue,game.match);auto& d=match_event_diagnostics(*input);d={};d.observer=&capture;input->pc=callsites[index];input->budget=input->executed+50000000;run_until(*input,callsites[index]+4);d.observer=nullptr;
            require(d.calls[index]>=1&&d.calls[index]==d.returns[index]&&!d.original_calls[index]&&!d.faults[index]&&!d.budget_stops[index]&&!d.dispatch_escapes,"Prepared complete indexed original BL event root");
            prepared[index]=d.calls[index];require(bool(capture.inputs[index]),"Prepared original BL capture");compare(*capture.inputs[index],index,Path::original_bl,Mode::modern);++prepared_comparisons;
        }
        // This prepared complete emitter uses actual match/queue context and
        // native services. Its three finite weight cells are private input;
        // rendering remains disabled for this deliberate data variant.
        auto event=prepare_input(base,7,queue,game.match);event->memory.write<1>(game.match+0x7929,0);event->memory.write<4>(game.match+0x3118,fixture+0x1800);event->memory.write<4>(fixture+0x1818,0);event->memory.write<1>(queue,0);event->memory.write<1>(queue+1,1);event->memory.write<1>(queue+3,0);event->memory.write<1>(queue+4,0);event->memory.write<2>(queue+6,0);event->memory.write<2>(queue+8,0);
        const auto catalog=std::uint32_t(event->memory.read<4>(event->r<4>(8)-0x5298));event->memory.write<1>(catalog,0);event->memory.write<1>(catalog+2,0);event->memory.write<1>(catalog+4,1);event->memory.write<1>(catalog+7,0);event->memory.write<4>(catalog+8,fixture+0x1000);event->memory.write<2>(catalog+12,3);event->memory.write<4>(catalog+16,90000);
        for(unsigned n=0;n<3;++n)event->memory.write<2>(fixture+0x1000+n*0x10c,30000);prepare_pool(*event,1,true);compare(*event,7,Path::generated,Mode::modern);
        for(unsigned child:{1u,2u,3u,4u,8u})require(completed_children[7][child]>0,"Complete actual emitter/helper composition child="+hex_address(selection::entries[child]));require(random_calls[7]>0,"Actual emitter RNG reseeder composition");
        compare(*event,7,Path::generated,Mode::modern,50000000,true,0,4);compare(*event,7,Path::generated,Mode::modern,50000000,true,0,2);
        // Two complete transition variants exercise both owned actor/context
        // comparisons, including a wildcard second participant and a swap.
        for(unsigned comparison=5;comparison<=6;++comparison){
            auto input=prepare_input(base,4,queue,game.match);input->memory.write<2>(queue+6,0);input->memory.write<2>(queue+8,1);input->memory.write<2>(fixture+0x800,comparison==5?0x1f46:0x1f4a);input->memory.write<2>(queue+0x30,comparison==5?0x213e:0x1f4d);
            input->memory.write<1>(fixture+0x805,0);input->memory.write<1>(fixture+0x806,0);input->memory.write<1>(fixture+0x807,1);input->memory.write<1>(fixture+0x808,0);input->memory.write<1>(queue+0x35,comparison==5?1:0);input->memory.write<1>(queue+0x36,0);input->memory.write<1>(queue+0x37,255);input->memory.write<1>(queue+0x38,0);prepare_pool(*input,1,false);compare(*input,4,Path::generated,Mode::modern);
            require(completed_children[4][comparison]>0,"Complete actual transition comparison child="+hex_address(selection::entries[comparison]));
        }
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"match-event-live-profile";std::filesystem::create_directories(snapshots);
        for(unsigned index=0;index<9;++index){
            require(bool(capture.inputs[index])&&natural[index]+prepared[index]>0,"Dedicated event caller coverage");save_runtime_snapshot(*capture.inputs[index],snapshots/(hex_address(selection::entries[index])+".snapshot"));matrix(*capture.inputs[index],index,false);
            // Actor address and classification are pure operations with no
            // required memory operand; only the seven memory readers fault.
            if(index!=0&&index!=8)compare(*capture.inputs[index],index,Path::generated,Mode::modern,50000000,false,1);
            std::cout<<"Integrated event entry="<<hex_address(selection::entries[index])<<" natural_calls="<<natural[index]<<" prepared_bl_calls="<<prepared[index]<<std::endl;
        }
        require(cases==129+prepared_comparisons&&negatives==11&&budgets==27&&faults==7,"Nine-root actual event matrix");
        std::ofstream sites(std::filesystem::path(argv[3]).parent_path()/"match-event-runtime-sites.txt");
        for(unsigned n=0;n<9;++n){require(!complete_sites[n].empty(),"Productive event source sites per root");for(auto pc:complete_sites[n])sites<<n<<" "<<hex_address(pc)<<'\n';std::cout<<"Root "<<hex_address(selection::entries[n])<<" complete_actual_sites="<<complete_sites[n].size()<<" rng_calls="<<random_calls[n]<<" children=";for(auto count:completed_children[n])std::cout<<count<<",";std::cout<<std::endl;}
    }
    std::cout<<"PASS: production match event cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<"; actual-context direct/generated/CFM, original/profile controls, complete state/effects/accounting/import boundaries\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
