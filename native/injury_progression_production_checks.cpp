#include "injury_progression_adapter.hpp"
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
namespace injury=cm::aot::injury_progression;
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
std::array<std::set<std::uint32_t>,4> runtime_sites;
void record_site(State& s,std::uint32_t pc) {if(s.executed>=s.budget)return;for(unsigned n=0;n<4;++n)if(pc>=injury::entries[n] && pc<(n==0?0x1018b6e0u:n==1?0x1018f33cu:n==2?0x1018ffe0u:0x101900b0u))runtime_sites[n].insert(pc);}
struct Witness:InjuryProgressionBoundaryObserver {
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool,InjuryProgressionOutcome)override{}
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
};
struct Capture:InjuryProgressionBoundaryObserver {
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
    std::array<std::unique_ptr<State>,4> inputs;
    std::array<std::uint32_t,4> callsites{};
    void enter(const State& state,std::uint32_t entry,bool original)override {
        if(original)return;auto it=std::find(injury::entries.begin(),injury::entries.end(),entry);require(it!=injury::entries.end(),"Unknown injury capture");const auto index=unsigned(it-injury::entries.begin());
        if(inputs[index])return;callsites[index]=std::uint32_t(state.r<4>(4128))-4;inputs[index]=clone(state);inputs[index]->platform=state.platform;inputs[index]->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,InjuryProgressionOutcome)override{}
};
struct Negative:InjuryProgressionBoundaryObserver {
    unsigned index;
    explicit Negative(unsigned value):index(value){}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,InjuryProgressionOutcome result)override {
        if(entry==injury::entries[index] && !original && result==InjuryProgressionOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);
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
        if(path==Path::direct)try_modern_injury_progression(s,injury::entries[index]);
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
        try {return ActorAttributesIoPair(input,index,injury::entries[index],[&](State& x){injury_progression_diagnostics(x).original=true;x.code_fragment_observer=&probe_clock;});}
        catch(...) {for(auto it=probe_clock.events.rbegin();it!=probe_clock.events.rend() && it<probe_clock.events.rbegin()+std::min<std::size_t>(8,probe_clock.events.size());++it)std::cerr<<"Probe import "<<it->name<<" leave="<<it->leave<<" pc="<<hex_address(it->pc)<<std::endl;throw;}
    }();auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    injury_progression_diagnostics(*a).original=true;actor_attributes_diagnostics(*a).original=true;auto& d=injury_progression_diagnostics(*b);
    if(mode==Mode::original){d.original=true;actor_attributes_diagnostics(*b).original=true;}if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(index);Witness witness;if(negative)d.observer=&mutation;else if(mode==Mode::modern)d.observer=&witness;
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;old_clock.base=new_clock.base=1000000000;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index);a->platform.reset();io.between();const auto actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production injury-progression entry="+hex_address(injury::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
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
    Clock probe;probe.base=1000000000;ActorAttributesIoPair io(input,index,injury::entries[index],[&](State& x){injury_progression_diagnostics(x).original=true;x.code_fragment_observer=&probe;});
    auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;
    for(auto* s:{old.get(),modern.get()}) {if(identity_field)s->memory.write<4>(s->r<4>(16)+0x28,0);else s->wr<4>(argument*4,0);s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;}
    injury_progression_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    const auto expected=execute(*old,Path::generated,index);old->platform.reset();io.between();const auto actual=execute(*modern,Path::generated,index);
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;
    require(expected==actual && expected.starts_with("Unmapped data access at "),"Actual injury pointer fault original="+expected+" modern="+actual);
    Image(*old).compare(*modern,"Actual injury pointer fault");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Actual injury fault effects/accounting/import ABI");io.compare();
    const auto& d=injury_progression_diagnostics(*modern);require(d.calls[index]==1 && d.faults[index]==1 && !d.returns[index] && !d.dispatch_escapes && !d.original_body,"Actual injury fault counters");++fault_cases;
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
    auto& own=injury_progression_diagnostics(s);auto& actor=actor_attributes_diagnostics(s);const auto old_own=own.original,old_actor=actor.original;own.original=actor.original=true;injury_record_diagnostics(s).original=true;
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);s.wr<4>(4128,return_sentinel);s.pc=entry;s.budget=s.executed+50000000;
    try{run_until(s,return_sentinel);}catch(const std::exception& e){std::cerr<<"Original injury setup entry="<<hex_address(entry)<<" pc="<<hex_address(s.pc)<<" reason="<<e.what()<<std::endl;own.original=old_own;actor.original=old_actor;throw;}
    own.original=old_own;actor.original=old_actor;return std::uint32_t(s.r<4>(12));
}
std::unique_ptr<State> complete_match(const State& input,std::uint32_t match) {
    auto old=clone(input),modern=clone(input);old->platform=modern->platform=input.platform;injury_progression_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;
    Clock x,y;x.base=y.base=1000000000;old->code_fragment_observer=&x;modern->code_fragment_observer=&y;unsigned advances=0;
    for(;advances<1400;++advances) {
        for(auto* state:{old.get(),modern.get()}) {state->wr<4>(12,match);state->wr<4>(16,1);state->wr<4>(4128,return_sentinel);state->pc=0x101d3f60;state->budget=state->executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(*state,return_sentinel);}
        require(old->executed==modern->executed && old->r<4>(12)==modern->r<4>(12),"Injury paired match progression/accounting");if(advances%50==0)Image(*old).compare(*modern,"Injury paired intermediate match");
        if(modern->r<4>(12)==2){++advances;break;}
    }
    require(advances<1400 && modern->r<4>(12)==2,"Injury paired match reaches full time");Image(*old).compare(*modern,"Injury paired complete match");require(x.events==y.events && x.count==y.count,"Injury paired imports");
    const auto& a=injury_progression_diagnostics(*old);const auto& b=injury_progression_diagnostics(*modern);
    for(unsigned n=0;n<4;++n)require(a.original_calls[n]==b.calls[n] && b.calls[n]==b.returns[n] && !b.faults[n] && !b.dispatch_escapes,"Injury paired natural counts");
    std::cout<<"PASS: injury paired full match advances="<<advances<<" dailyCalls="<<b.calls[0]<<" noticeCalls="<<b.calls[1]<<" endCompareCalls="<<b.calls[2]<<" startCompareCalls="<<b.calls[3]<<"; complete state/import ABI/per-tick accounting"<<std::endl;
    old->code_fragment_observer=modern->code_fragment_observer=nullptr;return modern;
}

std::unique_ptr<State> prepare_daily(const State& full,std::uint32_t match) {
    auto s=clone(full);s->platform=full.platform;const auto toc=std::uint32_t(s->r<4>(8));
    const auto manager=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7ba0))),staffs=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7be0)));
    const auto stats=invoke_original(*s,0x10773520,{0x1000}),service=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7b4c)));
    require(stats!=0,"Original match-statistics allocation");invoke_original(*s,0x1029f920,{service,match,stats,0});
    const auto id=std::uint32_t(s->memory.read<4>(stats+0x194)),staff=staffs+id*0x70,count=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7be4)));
    require(count>=16 && id<count && s->memory.read<4>(staff)==id,"Actual daily injury player and staff bounds");
    const auto club=std::uint32_t(s->memory.read<4>(staff+0x24));require(club!=0,"Actual daily injury club");
    for(unsigned n=0;n<16;++n)s->memory.write<4>(staffs+(count-1-n)*0x70+0x24,n==0?club:0);
    const auto records=invoke_original(*s,0x10773520,{72});require(records!=0,"Original queued injury allocation");
    s->memory.copy_in(records,std::vector<std::uint8_t>(72,0));const auto date=std::uint32_t(s->memory.read<4>(toc-0x7b80));
    // The shared factory currently supplies year/day to a day/year constructor.
    // Canonicalize only this prepared fixture through the original guest API;
    // the factory, debugger sources and installed applications stay untouched.
    invoke_original(*s,0x107156b0,{date,194,2001});
    invoke_original(*s,0x107156b0,{std::uint32_t(s->memory.read<4>(toc-0x7a9c)),194,2001});
    for(unsigned n=0;n<3;++n) {
        invoke_original(*s,0x10717890,{records+n*24,date,std::uint32_t(int(n)-1)});
        invoke_original(*s,0x10717890,{records+n*24+8,date,std::uint32_t(int(n)-1)});
        s->memory.write<4>(records+n*24+0x10,id);s->memory.write<1>(records+n*24+0x14,4);s->memory.write<1>(records+n*24+0x15,0);
    }
    s->memory.write<4>(manager+0x10,records);s->memory.write<4>(manager+0x18,3);
    s->wr<4>(12,manager);s->wr<4>(4128,return_sentinel);s->pc=injury::daily_entry;
    injury_progression_diagnostics(*s).original=false;actor_attributes_diagnostics(*s).original=false;injury_record_diagnostics(*s).original=false;
    return s;
}

constexpr std::array<std::pair<unsigned,const char*>,10> variants{{
    {0,"daily-no-queue.snapshot"},{0,"daily-current-queue.snapshot"},{0,"daily-future-queue.snapshot"},
    {1,"notice-guard.snapshot"},{1,"notice-risk-start.snapshot"},{1,"notice-risk-end.snapshot"},
    {2,"end-compare-earlier.snapshot"},{2,"end-compare-later.snapshot"},
    {3,"start-compare-earlier.snapshot"},{3,"start-compare-later.snapshot"}}};

void extra_cases(const std::array<std::unique_ptr<State>,4>& inputs,const std::filesystem::path& snapshots) {
    for(auto [index,name]:variants) {
        auto input=clone(*inputs[index]);input->platform=inputs[index]->platform;const auto label=std::string_view(name);
        if(label=="daily-no-queue.snapshot")input->memory.write<4>(input->r<4>(12)+0x10,0);
        if(label=="daily-current-queue.snapshot" || label=="daily-future-queue.snapshot") {
            const auto manager=input->r<4>(12),records=input->memory.read<4>(manager+0x10),toc=input->r<4>(8),date=input->memory.read<4>(toc-0x7b80);
            for(unsigned n=0;n<3;++n)for(unsigned field:{0u,8u})invoke_original(*input,0x10717890,{std::uint32_t(records+n*24+field),std::uint32_t(date),label=="daily-current-queue.snapshot"?0u:1u});
            input->wr<4>(12,manager);
        }
        if(label=="notice-guard.snapshot")input->wr<4>(16,0);
        if(label=="notice-risk-start.snapshot" || label=="notice-risk-end.snapshot"){input->wr<4>(24,77);input->wr<4>(20,label=="notice-risk-start.snapshot"?0:1);}
        if(index>=2) {
            const std::uint32_t field=index==2?8u:0u,key=std::uint32_t(input->r<4>(12)),item=std::uint32_t(input->r<4>(16));
            const auto original_day=std::uint16_t(input->memory.read<2>(key+field));input->memory.write<2>(item+field,std::uint16_t(original_day+(label.find("earlier")!=std::string_view::npos?-1:1)));
        }
        injury_progression_diagnostics(*input).original=false;actor_attributes_diagnostics(*input).original=false;injury_record_diagnostics(*input).original=false;
        input->pc=injury::entries[index];input->wr<4>(4128,return_sentinel);save_runtime_snapshot(*input,snapshots/name);matrix(*input,index,false);
        std::cout<<"PASS: controlled actual-data progression input="<<name<<std::endl;
    }
}

void capture_daily(const State& input,const std::filesystem::path& snapshots) {
    Clock probe;probe.base=1000000000;
    ActorAttributesIoPair io=[&]() {
        try{return ActorAttributesIoPair(input,0,injury::daily_entry,[&](State& s){injury_progression_diagnostics(s).original=true;s.code_fragment_observer=&probe;});}
        catch(...){for(auto it=probe.events.rbegin();it!=probe.events.rend() && it<probe.events.rbegin()+std::min<std::size_t>(12,probe.events.size());++it)std::cerr<<"Daily probe import="<<it->name<<" leave="<<it->leave<<" LR="<<hex_address(read_be<4>(it->registers.data()+4128))<<std::endl;throw;}
    }();
    auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;injury_progression_diagnostics(*old).original=true;
    Capture capture;injury_progression_diagnostics(*modern).observer=&capture;
    Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    for(auto* state:{old.get(),modern.get()}){state->pc=0x10186480;state->wr<4>(4128,return_sentinel);state->budget=state->executed+50000000;}
    run_until(*old,0x10186484);old->platform.reset();io.between();run_until(*modern,0x10186484);
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;injury_progression_diagnostics(*modern).observer=nullptr;
    Image(*old).compare(*modern,"Complete prepared daily injury body through original BL");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Daily original BL effects/accounting/import ABI");io.compare();
    const auto& d=injury_progression_diagnostics(*modern);
    std::cout<<"Daily productive calls="<<d.calls[0]<<","<<d.calls[1]<<","<<d.calls[2]<<","<<d.calls[3]<<std::endl;
    const std::uint32_t controller=std::uint32_t(input.r<4>(12)),date=std::uint32_t(input.memory.read<4>(input.r<4>(8)-0x7b80)),records=std::uint32_t(input.memory.read<4>(controller+0x10));
    std::cout<<"Prepared daily date="<<input.memory.read<2>(date)<<"/"<<input.memory.read<2>(date+2)<<" queueCount="<<input.memory.read<4>(controller+0x18)<<std::endl;
    for(unsigned n=0;n<3;++n)std::cout<<"Prepared queued date "<<n<<"="<<input.memory.read<2>(records+24*n)<<"/"<<input.memory.read<2>(records+24*n+2)<<".."<<input.memory.read<2>(records+24*n+8)<<"/"<<input.memory.read<2>(records+24*n+10)<<std::endl;
    for(unsigned n=0;n<4;++n) {
        require(bool(capture.inputs[n]),"Daily body captures each progression root");require(d.calls[n]>0 && d.calls[n]==d.returns[n] && !d.original_calls[n] && !d.faults[n] && !d.dispatch_escapes,"Actual daily body dedicated productive counters");
        capture.inputs[n]->platform=input.platform;save_runtime_snapshot(*capture.inputs[n],snapshots/(hex_address(injury::entries[n])+".snapshot"));matrix(*capture.inputs[n],n,false);
        std::cout<<"PASS: prepared daily entry="<<hex_address(injury::entries[n])<<" original caller="<<hex_address(capture.callsites[n])<<" calls="<<d.calls[n]<<" returns="<<d.returns[n]<<std::endl;
    }
    require(capture.callsites[0]==0x10186480,"Audited original daily BL reached");
    extra_cases(capture.inputs,snapshots);
    actual_fault(*capture.inputs[0],0,3);actual_fault(*capture.inputs[1],1,4,true);actual_fault(*capture.inputs[2],2,3);actual_fault(*capture.inputs[3],3,3);
}
}

int main(int argc,char** argv){try {
    const bool prepared=argc==6 && std::string_view(argv[5])=="--prepared";
    require(argc==4 || argc==5 || prepared,"Expected original program/game/private backing and optional captured profile directory");const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Progression profile flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());OwnedBacking cleanup{root};
    if(prepared){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,argv[4]);const auto manager=input.r<4>(12),records=input.memory.read<4>(manager+0x10),count=input.memory.read<4>(manager+0x18);require(count==3,"Prepared daily diagnostic input has three queued records");for(unsigned n=0;n<3;++n)input.memory.write<1>(records+24*n+0x14,4);capture_daily(input,std::filesystem::path(argv[4]).parent_path());}
    else if(profile) {
        for(unsigned n=0;n<4;++n){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(injury::entries[n])+".snapshot"));matrix(input,n,true);}
        for(auto [index,name]:variants){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,std::filesystem::path(argv[4])/name);matrix(input,index,true);}
    } else {
        auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto full=complete_match(*game.state,game.match);auto input=prepare_daily(*full,game.match);
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"injury-progression-live-profile";std::filesystem::create_directories(snapshots);save_runtime_snapshot(*input,snapshots/"prepared-daily.snapshot");capture_daily(*input,snapshots);
    }
    if(!profile){std::ofstream log(std::filesystem::path(argv[3]).parent_path()/"injury-progression-runtime-sites.txt");for(unsigned n=0;n<4;++n)for(auto pc:runtime_sites[n])log<<n<<" "<<hex_address(pc)<<std::endl;}
    std::cout<<"Runtime progression source sites="<<runtime_sites[0].size()<<","<<runtime_sites[1].size()<<","<<runtime_sites[2].size()<<","<<runtime_sites[3].size()<<std::endl;
    std::cout<<"Actual progression pointer faults="<<fault_cases<<std::endl;
    std::cout<<"PASS: progression production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
