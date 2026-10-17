#include "injury_club_events_adapter.hpp"
#include "injury_record_adapter.hpp"
#include "actor_attributes_io_checks.hpp"
#include "match_debugger_factory.hpp"
#include "semantic_io_checks.hpp"
#include "code_fragment_adapter.hpp"
#include "match_turbo.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include "process_bridge.hpp"
#include <chrono>
#include <fstream>
#include <iterator>
#include <map>
#include <set>
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
namespace injury=cm::aot::injury_club_events;
enum class Path { direct, generated, cfm };
enum class Mode { modern, original, entry_original, profile };
struct Clock:CodeFragmentBoundaryObserver {
    std::uint64_t base=native_microseconds();unsigned count=0;
    ActorAttributesIoPair* io=nullptr;bool modern=false;
    struct Event {bool leave;std::string name;decltype(State::registers) registers;std::uint32_t pc;bool operator==(const Event&)const=default;};
    std::vector<Event> events;
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool)override{}
    void import_enter(const State& s,const char* name)override{events.push_back({false,name,s.registers,s.pc});}
    void import_leave(const State& s,const char* name)override{events.push_back({true,name,s.registers,s.pc});}
    bool import_fixture(State& s,const char* name)override {
        if(io && io->dispatch(s,name,modern))return true;
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Injury record await native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
std::array<std::set<std::uint32_t>,3> runtime_sites;
void record_site(State& s,std::uint32_t pc) {if(s.executed>=s.budget)return;for(unsigned n=0;n<3;++n)if(pc>=injury::entries[n] && pc<(n==0?0x1018bdd0u:n==1?0x1018c180u:0x10190180u))runtime_sites[n].insert(pc);}
struct Witness:InjuryClubEventsBoundaryObserver {
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool,InjuryClubEventsOutcome)override{}
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
};
struct Capture:InjuryClubEventsBoundaryObserver {
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
    std::array<std::unique_ptr<State>,3> inputs;
    std::array<std::uint32_t,3> callsites{};
    void enter(const State& state,std::uint32_t entry,bool original)override {
        if(original)return;auto it=std::find(injury::entries.begin(),injury::entries.end(),entry);require(it!=injury::entries.end(),"Unknown injury capture");const auto index=unsigned(it-injury::entries.begin());
        if(inputs[index])return;callsites[index]=std::uint32_t(state.r<4>(4128))-4;inputs[index]=clone(state);inputs[index]->platform=state.platform;inputs[index]->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,InjuryClubEventsOutcome)override{}
};
struct Negative:InjuryClubEventsBoundaryObserver {
    unsigned index;
    explicit Negative(unsigned value):index(value){}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,InjuryClubEventsOutcome result)override {
        if(entry==injury::entries[index] && !original && result==InjuryClubEventsOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);
    }
};
struct OwnedBacking {
    std::filesystem::path directory;
    ~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(directory,e)||e||std::filesystem::is_symlink(directory,e)||e)return;
        for(const auto& p:std::filesystem::recursive_directory_iterator(directory,e))if(e||p.is_symlink(e)||e)return;
        std::filesystem::remove_all(directory,e);}
};
unsigned cases=0,returns=0,budgets=0,negatives=0;
std::string execute(State& s,Path path,unsigned index) {
    try {
        if(path==Path::direct)try_modern_injury_club_events(s,injury::entries[index]);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,injury::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
void compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false) {
    Clock probe_clock;probe_clock.base=1000000000;
    ActorAttributesIoPair io=[&]() {
        try {return ActorAttributesIoPair(input,index,injury::entries[index],[&](State& x){injury_club_events_diagnostics(x).original=true;x.code_fragment_observer=&probe_clock;});}
        catch(...) {for(auto it=probe_clock.events.rbegin();it!=probe_clock.events.rend() && it<probe_clock.events.rbegin()+std::min<std::size_t>(8,probe_clock.events.size());++it)std::cerr<<"Probe import "<<it->name<<" leave="<<it->leave<<" pc="<<hex_address(it->pc)<<std::endl;throw;}
    }();auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    injury_club_events_diagnostics(*a).original=true;actor_attributes_diagnostics(*a).original=true;auto& d=injury_club_events_diagnostics(*b);
    if(mode==Mode::original){d.original=true;actor_attributes_diagnostics(*b).original=true;}if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(index);Witness witness;if(negative)d.observer=&mutation;else if(mode==Mode::modern)d.observer=&witness;
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;old_clock.base=new_clock.base=1000000000;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index);a->platform.reset();io.between();const auto actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production injury-club-events entry="+hex_address(injury::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected boundary");
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,label+" host input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative failed");++negatives;}
    else {Image(*a).compare(*b,label);require(x.bytes==y.bytes && a->executed==b->executed,label+" ordered effects/accounting");}
    io.compare();
    require(!d.original_body && !d.dispatch_escapes,label+" own-body scope residue/dispatch escape");
    if(mode==Mode::modern){require(d.calls[index]>=1 && !d.original_calls[index] && !d.profile_original_calls[index],label+" no dedicated modern call");
        require(d.returns[index]==(expected.empty()?d.calls[index]:0) && d.budget_stops[index]==unsigned(!expected.empty()),label+" productive outcome counts");}
    else {require(!d.calls[index] && !d.returns[index],label+" original control entered C++ root");require(mode==Mode::profile?d.profile_original_calls[index]>=1:d.original_calls[index]>=1,label+" selected original/profile not reached");}
    ++cases;returns+=expected.empty();budgets+=!expected.empty();
}

unsigned fault_cases=0;
void actual_fault(const State& input,unsigned index,unsigned argument,bool identity_field=false) {
    Clock probe;probe.base=1000000000;ActorAttributesIoPair io(input,index,injury::entries[index],[&](State& x){injury_club_events_diagnostics(x).original=true;x.code_fragment_observer=&probe;});
    auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;
    for(auto* s:{old.get(),modern.get()}) {if(identity_field)s->memory.write<4>(s->r<4>(16)+0x28,0);else s->wr<4>(argument*4,0);s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;}
    injury_club_events_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    const auto expected=execute(*old,Path::generated,index);old->platform.reset();io.between();const auto actual=execute(*modern,Path::generated,index);
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;
    require(expected==actual && expected.starts_with("Unmapped data access at "),"Actual injury pointer fault original="+expected+" modern="+actual);
    Image(*old).compare(*modern,"Actual injury pointer fault");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Actual injury fault effects/accounting/import ABI");io.compare();
    const auto& d=injury_club_events_diagnostics(*modern);require(d.calls[index]==1 && d.faults[index]==1 && !d.returns[index] && !d.dispatch_escapes && !d.original_body,"Actual injury fault counters");++fault_cases;
}

void matrix(const State& input,unsigned index,bool profile) {
    for(auto path:{Path::direct,Path::generated,Path::cfm}) {
        if(profile)compare(input,index,path,Mode::profile);
        else for(auto mode:{Mode::modern,Mode::original,Mode::entry_original})compare(input,index,path,mode);
    }
    if(!profile) {
        compare(input,index,Path::generated,Mode::modern,50000000,true);
        for(auto cut:{0u,1u,2u,3u,8u,12u,16u})compare(input,index,Path::generated,Mode::modern,cut);
    }
}
std::uint32_t invoke_original(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args) {
    auto& own=injury_club_events_diagnostics(s);auto& actor=actor_attributes_diagnostics(s);auto& record=injury_record_diagnostics(s);const auto old_own=own.original,old_actor=actor.original,old_record=record.original;own.original=actor.original=record.original=true;
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);s.wr<4>(4128,return_sentinel);s.pc=entry;s.budget=s.executed+50000000;
    try{run_until(s,return_sentinel);}catch(const std::exception& e){std::cerr<<"Original injury setup entry="<<hex_address(entry)<<" pc="<<hex_address(s.pc)<<" reason="<<e.what()<<std::endl;own.original=old_own;actor.original=old_actor;record.original=old_record;throw;}
    own.original=old_own;actor.original=old_actor;record.original=old_record;return std::uint32_t(s.r<4>(12));
}
std::unique_ptr<State> complete_match(const State& input,std::uint32_t match) {
    auto old=clone(input),modern=clone(input);old->platform=modern->platform=input.platform;injury_club_events_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;
    Clock x,y;x.base=y.base=1000000000;old->code_fragment_observer=&x;modern->code_fragment_observer=&y;unsigned advances=0;
    for(;advances<1400;++advances) {
        for(auto* state:{old.get(),modern.get()}) {state->wr<4>(12,match);state->wr<4>(16,1);state->wr<4>(4128,return_sentinel);state->pc=0x101d3f60;state->budget=state->executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(*state,return_sentinel);}
        require(old->executed==modern->executed && old->r<4>(12)==modern->r<4>(12),"Injury paired match club event/accounting");if(advances%50==0)Image(*old).compare(*modern,"Injury paired intermediate match");
        if(modern->r<4>(12)==2){++advances;break;}
    }
    require(advances<1400 && modern->r<4>(12)==2,"Injury paired match reaches full time");Image(*old).compare(*modern,"Injury paired complete match");require(x.events==y.events && x.count==y.count,"Injury paired imports");
    const auto& a=injury_club_events_diagnostics(*old);const auto& b=injury_club_events_diagnostics(*modern);
    for(unsigned n=0;n<3;++n)require(a.original_calls[n]==b.calls[n] && b.calls[n]==b.returns[n] && !b.faults[n] && !b.dispatch_escapes,"Injury paired natural counts");
    std::cout<<"PASS: injury paired full match advances="<<advances<<" enqueueCalls="<<b.calls[0]<<" processCalls="<<b.calls[1]<<" compareCalls="<<b.calls[2]<<"; complete state/import ABI/per-tick accounting"<<std::endl;
    old->code_fragment_observer=modern->code_fragment_observer=nullptr;return modern;
}

void prepare_pool(State& s,unsigned word) {
    const auto toc=s.r<4>(8),pool=s.memory.read<4>(toc-0x7b8c),cursor=s.memory.read<4>(toc-0x7b84),salt=s.memory.read<4>(toc-0x7b90);
    s.memory.write<4>(cursor,pool);s.memory.write<4>(salt,0);
    for(unsigned n=0;n<64;++n)s.memory.write<2>(pool+n*2,word);
}
std::unique_ptr<State> prepare_events(const State& full,std::uint32_t match) {
    auto s=clone(full);s->platform=full.platform;const auto toc=std::uint32_t(s->r<4>(8));
    const auto manager=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7ba0))),staffs=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7be0)));
    const auto stats=invoke_original(*s,0x10773520,{0x1000}),service=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7b4c)));
    require(stats!=0,"Original match-statistics allocation");invoke_original(*s,0x1029f920,{service,match,stats,0});
    const auto id=std::uint32_t(s->memory.read<4>(stats+0x194)),staff=staffs+id*0x70,count=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7be4)));
    require(id<count && s->memory.read<4>(staff)==id,"Actual club event player bounds");
    const auto club=std::uint32_t(s->memory.read<4>(staff+0x24));require(club!=0,"Actual club event roster");
    invoke_original(*s,actor_attributes::reset_entry,{manager,id});
    for(unsigned n=0;n<50;++n)s->memory.write<4>(club+0x98+n*4,staff);
    s->memory.write<1>(s->memory.read<4>(manager)+id*0x24+0x21,255);
    const auto records=invoke_original(*s,0x10773520,{80});require(records!=0,"Original club event allocation");s->memory.copy_in(records,std::vector<std::uint8_t>(80,0));
    const auto date=std::uint32_t(s->memory.read<4>(toc-0x7b80));
    invoke_original(*s,0x107156b0,{date,194,2001});invoke_original(*s,0x107156b0,{std::uint32_t(s->memory.read<4>(toc-0x7a9c)),194,2001});
    for(unsigned n=0;n<3;++n) {
        invoke_original(*s,0x10717890,{records+n*20,date,n==0?std::uint32_t(-6):n==1?0u:1u});
        s->memory.write<1>(records+n*20+8,0);s->memory.write<4>(records+n*20+12,s->memory.read<4>(club));s->memory.write<4>(records+n*20+16,0xffffffff);
    }
    s->memory.write<4>(manager+0x1c,records);s->memory.write<4>(manager+0x20,4);s->memory.write<4>(manager+0x24,3);prepare_pool(*s,1);
    injury_club_events_diagnostics(*s).original=false;actor_attributes_diagnostics(*s).original=false;injury_record_diagnostics(*s).original=false;
    s->wr<4>(12,manager);s->wr<4>(16,date);s->wr<4>(20,0);s->wr<4>(24,s->memory.read<4>(club));s->wr<4>(28,0);s->wr<4>(4128,return_sentinel);s->pc=injury::enqueue_entry;return s;
}
constexpr std::array<std::pair<unsigned,const char*>,11> variants{{
    {0,"enqueue-skip.snapshot"},{0,"enqueue-allocation.snapshot"},{0,"enqueue-reallocation.snapshot"},{0,"enqueue-context.snapshot"},
    {1,"process-empty.snapshot"},{1,"process-expired.snapshot"},{1,"process-future.snapshot"},{1,"process-type-three.snapshot"},{1,"process-empty-roster.snapshot"},
    {2,"compare-earlier.snapshot"},{2,"compare-later.snapshot"}}};
void extra_cases(const std::array<std::unique_ptr<State>,3>& inputs,const std::filesystem::path& snapshots) {
    for(auto [index,name]:variants) {
        auto input=clone(*inputs[index]);input->platform=inputs[index]->platform;const auto label=std::string_view(name);
        if(index<2) {
            const auto manager=std::uint32_t(input->r<4>(12)),records=std::uint32_t(input->memory.read<4>(manager+0x1c)),count=std::uint32_t(input->memory.read<4>(manager+0x24));
            const auto date=std::uint32_t(input->memory.read<4>(input->r<4>(8)-0x7b80));
            if(label=="enqueue-skip.snapshot")input->wr<4>(20,4);
            if(label=="enqueue-allocation.snapshot"){input->memory.write<4>(manager+0x1c,0);input->memory.write<4>(manager+0x20,0);input->memory.write<4>(manager+0x24,0);}
            if(label=="enqueue-reallocation.snapshot")input->memory.write<4>(manager+0x20,count);
            if(label=="enqueue-context.snapshot"){const auto context=invoke_original(*input,0x10773520,{0x70});input->memory.write<4>(context+0x68,123);input->registers=inputs[index]->registers;input->wr<4>(28,context);}
            if(label=="process-empty.snapshot")input->memory.write<4>(manager+0x24,0);
            if(label=="process-expired.snapshot" || label=="process-future.snapshot")for(unsigned n=0;n<count;++n)invoke_original(*input,0x10717890,{records+n*20,date,label=="process-expired.snapshot"?std::uint32_t(-6):1u});
            if(label=="process-type-three.snapshot")for(unsigned n=0;n<count;++n)input->memory.write<1>(records+n*20+8,3);
            if(label=="process-empty-roster.snapshot"){
                const auto club_id=input->memory.read<4>(records+12),clubs=input->memory.read<4>(input->memory.read<4>(input->r<4>(8)-0x7be8)),club=clubs+club_id*0x248;
                for(unsigned n=0;n<50;++n)input->memory.write<4>(club+0x98+n*4,0);
            }
            if(label=="process-expired.snapshot" || label=="process-future.snapshot")input->registers=inputs[index]->registers;
        } else {
            const auto key=input->r<4>(12),item=input->r<4>(16);const auto day=std::uint16_t(input->memory.read<2>(key));input->memory.write<2>(item,std::uint16_t(day+(label=="compare-earlier.snapshot"?-1:1)));
        }
        injury_club_events_diagnostics(*input).original=false;actor_attributes_diagnostics(*input).original=false;injury_record_diagnostics(*input).original=false;
        input->pc=injury::entries[index];input->wr<4>(4128,return_sentinel);save_runtime_snapshot(*input,snapshots/name);matrix(*input,index,false);
        std::cout<<"PASS: controlled actual-data club event input="<<name<<std::endl;
    }
}
void capture_parent(const State& input,unsigned index,Capture& capture) {
    Clock probe;probe.base=1000000000;
    ActorAttributesIoPair io(input,index,injury::entries[index],[&](State& x){injury_club_events_diagnostics(x).original=true;x.code_fragment_observer=&probe;});
    auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;injury_club_events_diagnostics(*old).original=true;injury_club_events_diagnostics(*modern).observer=&capture;
    Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    const std::uint32_t call=index==0?0x1018aea8:0x10186488;
    for(auto* s:{old.get(),modern.get()}){s->pc=call;s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;}
    run_until(*old,call+4);old->platform.reset();io.between();run_until(*modern,call+4);
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;injury_club_events_diagnostics(*modern).observer=nullptr;
    Image(*old).compare(*modern,"Complete club event body through original BL");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Club event parent effects/accounting/import ABI");io.compare();
    const auto& d=injury_club_events_diagnostics(*modern);require(d.calls[index]==1 && d.returns[index]==1 && !d.original_calls[index] && !d.faults[index] && !d.dispatch_escapes,"Dedicated productive club event parent counter");
    if(index==0)require(d.calls[2]>0 && d.calls[2]==d.returns[2],"Actual qsort reaches the CFM date comparator");
    if(index==1)require(injury_record_diagnostics(*modern).calls[0]>0,"Actual club processing reaches reconstructed injury registration");
    std::cout<<"PASS: original club event BL="<<hex_address(call)<<" calls="<<d.calls[0]<<","<<d.calls[1]<<","<<d.calls[2]<<std::endl;
}
void capture_events(const State& input,const std::filesystem::path& snapshots) {
    Capture capture;capture_parent(input,0,capture);
    auto processing=clone(input);processing->platform=input.platform;prepare_pool(*processing,1);processing->pc=injury::process_entry;capture_parent(*processing,1,capture);
    for(unsigned n=0;n<3;++n){require(bool(capture.inputs[n]),"Original parents capture all club event roots");capture.inputs[n]->platform=input.platform;save_runtime_snapshot(*capture.inputs[n],snapshots/(hex_address(injury::entries[n])+".snapshot"));matrix(*capture.inputs[n],n,false);std::cout<<"PASS: prepared club event entry="<<hex_address(injury::entries[n])<<" caller="<<hex_address(capture.callsites[n])<<std::endl;}
    require(capture.callsites[0]==0x1018aea8 && capture.callsites[1]==0x10186488,"Audited club event original BLs reached");
    extra_cases(capture.inputs,snapshots);for(unsigned n=0;n<3;++n)actual_fault(*capture.inputs[n],n,3);
}
}
int main(int argc,char** argv){try {
    const bool prepared=argc==6 && std::string_view(argv[5])=="--prepared";
    require(argc==4 || argc==5 || prepared,"Expected original program/game/private backing and optional profile directory");const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Club event profile flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());OwnedBacking cleanup{root};
    if(prepared){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,argv[4]);capture_events(input,std::filesystem::path(argv[4]).parent_path());}
    else if(profile) {
        for(unsigned n=0;n<3;++n){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(injury::entries[n])+".snapshot"));matrix(input,n,true);}
        for(auto [index,name]:variants){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,std::filesystem::path(argv[4])/name);matrix(input,index,true);}
    } else {
        auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto full=complete_match(*game.state,game.match);auto input=prepare_events(*full,game.match);
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"injury-club-events-live-profile";std::filesystem::create_directories(snapshots);save_runtime_snapshot(*input,snapshots/"prepared-events.snapshot");capture_events(*input,snapshots);
    }
    if(!profile){std::ofstream log(std::filesystem::path(argv[3]).parent_path()/"injury-club-events-runtime-sites.txt");for(unsigned n=0;n<3;++n)for(auto pc:runtime_sites[n])log<<n<<" "<<hex_address(pc)<<std::endl;}
    std::cout<<"Runtime club event source sites="<<runtime_sites[0].size()<<","<<runtime_sites[1].size()<<","<<runtime_sites[2].size()<<std::endl;
    std::cout<<"Actual club event pointer faults="<<fault_cases<<std::endl;
    std::cout<<"PASS: club event production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
