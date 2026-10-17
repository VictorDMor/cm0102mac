#include "actor_attributes_adapter.hpp"
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
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
namespace attributes=cm::aot::actor_attributes;
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
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Actor attributes await native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
struct Capture:ActorAttributesBoundaryObserver {
    std::array<std::unique_ptr<State>,attributes::entries.size()> inputs;
    std::unique_ptr<State> periodic_candidate;
    bool periodic_updated=false,daily_updated=false,observe_daily_writes=false;
    std::unique_ptr<State> daily_candidate,daily_input;
    void before_step(State&,std::uint32_t pc)override { if(pc==0x10239574)periodic_updated=true;if(pc==0x10188454)daily_updated=true; }
    void enter(const State& s,std::uint32_t root,bool original)override {
        if(original)return;
        const auto entry=std::find(attributes::entries.begin(),attributes::entries.end(),root);
        require(entry!=attributes::entries.end(),"Unknown actor-attribute capture");
        auto& input=inputs[unsigned(entry-attributes::entries.begin())];
        if(root==attributes::record_refresh_entry && observe_daily_writes) {
            daily_candidate=clone(s);daily_candidate->platform=s.platform;daily_candidate->wr<4>(4128,return_sentinel);daily_updated=false;
        }
        if(input)return;
        if(root==attributes::restoration_entry && !s.memory.read<4>(s.r<4>(12)+0x70))return;
        if(root==attributes::periodic_entry) {
            periodic_candidate=clone(s);periodic_candidate->platform=s.platform;
            periodic_candidate->wr<4>(4128,return_sentinel);periodic_updated=false;return;
        }
        input=clone(s);input->platform=s.platform;input->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t entry,bool,ActorAttributesOutcome result)override {
        if(entry==attributes::record_refresh_entry && daily_candidate) {
            if(daily_updated && result==ActorAttributesOutcome::returned)daily_input=std::move(daily_candidate);
            else daily_candidate.reset();
        }
        if(entry==attributes::periodic_entry && periodic_candidate) {
            if(periodic_updated && result==ActorAttributesOutcome::returned) inputs[3]=std::move(periodic_candidate);
            else periodic_candidate.reset();
        }
    }
};
struct Negative:ActorAttributesBoundaryObserver {
    unsigned index;std::uint32_t caller_stack;
    Negative(unsigned value,std::uint32_t frame):index(value),caller_stack(frame){}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,ActorAttributesOutcome result)override {
        if(entry==attributes::entries[index] && !original && result==ActorAttributesOutcome::returned && (index!=14 || s.r<4>(4)==caller_stack))s.wr<4>(12,s.r<4>(12)^1);
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
        if(path==Path::direct)try_modern_actor_attributes(s,attributes::entries[index]);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,attributes::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
void compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,bool recursive=false) {
    ActorAttributesIoPair io(input,index);auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=attributes::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    actor_attributes_diagnostics(*a).original=true;auto& d=actor_attributes_diagnostics(*b);
    if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(index,std::uint32_t(input.r<4>(4)));if(negative)d.observer=&mutation;
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;new_clock.base=old_clock.base;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    std::array<unsigned,2> forced{};std::array<std::uint32_t,2> root_stack{};
    if(recursive)for(unsigned side=0;side<2;++side)(side?y:x).on_read=[&,side](State& state,Effects&,std::uint32_t,unsigned) {
        if(!root_stack[side] && state.pc==0x101864ac)root_stack[side]=std::uint32_t(state.r<4>(4));
        constexpr std::array<std::uint32_t,3> reads{0x101865b4,0x101866a0,0x1018678c};
        constexpr std::array<unsigned,3> registers{22,21,20};
        for(unsigned n=0;n<3;++n)if(state.pc==reads[n] && state.r<4>(4)==root_stack[side]-128) {
            state.wr<4>(registers[n]*4,65536);state.wr<1>(2304,(state.r<1>(2304)&0xf0)|4|(state.r<1>(1024)&1));++forced[side];
        }
    };
    const auto expected=execute(*a,path,index);a->platform.reset();io.between();const auto actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production actor-attribute entry="+hex_address(attributes::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected boundary");
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,label+" host input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative failed");++negatives;}
    else {Image(*a).compare(*b,label);require(x.bytes==y.bytes && a->executed==b->executed,label+" ordered effects/accounting");}
    io.compare();
    if(recursive && limit==50000000) {
        require(forced[0]==3 && forced[1]==3,label+" three equal structural inline-bound controls");
        if(mode==Mode::modern)require(d.calls[14]==2 && d.returns[14]==2,label+" productive outer and recursive C++ sampler calls");
    }
    require(!d.original_body && !d.dispatch_escapes,label+" own-body scope residue/dispatch escape");
    if(mode==Mode::modern){require(d.calls[index]>=1 && !d.original_calls[index] && !d.profile_original_calls[index],label+" no dedicated modern call");
        require(d.returns[index]==(expected.empty()?d.calls[index]:0) && d.budget_stops[index]==unsigned(!expected.empty()),label+" productive outcome counts");}
    else {require(!d.calls[index] && !d.returns[index],label+" original control entered C++ root");require(mode==Mode::profile?d.profile_original_calls[index]>=1:d.original_calls[index]>=1,label+" selected original/profile not reached");}
    ++cases;returns+=expected.empty();budgets+=!expected.empty();
}
auto complete_match(const State& seed,std::uint32_t match,Capture& capture){
    auto original=clone(seed),modern=clone(seed);original->platform=modern->platform=seed.platform;
    actor_attributes_diagnostics(*original).original=true;
    actor_attributes_diagnostics(*modern).observer=&capture;
    Clock old_clock,new_clock;old_clock.base=new_clock.base=1000000000ULL;original->code_fragment_observer=&old_clock;modern->code_fragment_observer=&new_clock;
    unsigned advances=0;
    for(;advances<1400;++advances){
        for(auto* s:{original.get(),modern.get()}){
            s->wr<4>(12,match);s->wr<4>(16,1);s->wr<4>(4128,return_sentinel);s->pc=0x101d3f60;
            s->budget=s->executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(*s,return_sentinel);
        }
        require(original->executed==modern->executed && original->r<4>(12)==modern->r<4>(12),"Paired actor-attributes match progression/accounting");
        if(advances%50==0)Image(*original).compare(*modern,"Paired actor-attributes intermediate state");
        if(modern->r<4>(12)==2){++advances;break;}
    }
    original->code_fragment_observer=modern->code_fragment_observer=nullptr;
    actor_attributes_diagnostics(*modern).observer=nullptr;
    require(advances<1400 && modern->r<4>(12)==2,"Paired actor-attributes match reached full time");
    Image(*original).compare(*modern,"Paired actor-attributes complete match state");
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,"Paired actor-attributes match host inputs/import ABI");
    const auto& old=actor_attributes_diagnostics(*original);const auto& next=actor_attributes_diagnostics(*modern);
    for(unsigned i=0;i<attributes::entries.size();++i){
        require(next.calls[i]==next.returns[i] && next.calls[i]==old.original_calls[i] && !old.calls[i] &&
            !next.original_calls[i] && !next.profile_original_calls[i] && !next.faults[i] && !next.budget_stops[i],"Paired actor-attributes match entry accounting");
    }
    require(next.calls[0]>0 && next.calls[1]==4*next.calls[0] && !next.dispatch_escapes,"Paired actor-attributes match exercised refresh and four maximum children");
    std::cout<<"PASS: paired actor-attributes full match advances="<<advances<<" refreshCalls="<<next.calls[0]<<" maximumCalls="<<next.calls[1]
        <<" recoveryCalls="<<next.calls[2]<<" periodicCalls="<<next.calls[3]<<" consumptionCalls="<<next.calls[4]
        <<" restorationCalls="<<next.calls[5]<<" recordScaleCalls="<<next.calls[6]<<" availabilityCalls="<<next.calls[7]<<" effectiveConditionCalls="<<next.calls[8]<<" recordRefreshCalls="<<next.calls[9]
        <<" capacityCalls="<<next.calls[10]<<" capacityAssignmentCalls="<<next.calls[11]<<" injuryRetirementCalls="<<next.calls[12]<<" recordResetCalls="<<next.calls[13]<<" conditionSamplingCalls="<<next.calls[14]
        <<"; complete state, import ABI and per-tick accounting\n";
    return next.calls;
}
void actual_fault(const State& input,unsigned index) {
    auto a=clone(input),b=clone(input);a->platform=b->platform=input.platform;
    for(auto* s:{a.get(),b.get()}){s->pc=attributes::entries[index];s->wr<4>(12,0);s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;}
    actor_attributes_diagnostics(*a).original=true;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,Path::generated,index),actual=execute(*b,Path::generated,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;
    require(expected==actual && expected.starts_with("Unmapped data access at "),"Actor null fault after original frame");
    Image(*a).compare(*b,"Actor null fault state");
    require(x.bytes==y.bytes && a->executed==b->executed,"Actor null fault effects/accounting");
    const auto& d=actor_attributes_diagnostics(*b);
    require(d.calls[index]==1 && d.faults[index]==1 && !d.returns[index] && !d.dispatch_escapes && !d.original_body,"Actor null fault diagnostics");
}
void matrix(const State& input,unsigned index,bool profile,bool recursive=false) {
    for(auto path:{Path::direct,Path::generated,Path::cfm}){
        if(profile)compare(input,index,path,Mode::profile,50000000,false,recursive);
        else for(auto mode:{Mode::modern,Mode::original,Mode::entry_original})compare(input,index,path,mode,50000000,false,recursive);
    }
    if(!profile){
        compare(input,index,Path::generated,Mode::modern,50000000,true,recursive);
        for(auto limit:{0u,1u,2u,3u})compare(input,index,Path::generated,Mode::modern,limit,false,recursive);
        if(index==4)for(auto limit:{4u,14u,18u})compare(input,index,Path::generated,Mode::modern,limit,false,recursive);
        else if(index!=1 && index!=6)for(auto limit:{index==14?4u:12u,index==14?9u:13u,index==14?12u:index==2?36u:index==7?30u:index>=8?40u:100u})compare(input,index,Path::generated,Mode::modern,limit,false,recursive);
    }
}
std::unique_ptr<State> daily_rollover(const State& parent) {
    auto old=clone(parent),modern=clone(parent);old->platform=modern->platform=parent.platform;
    const auto date=parent.memory.read<4>(parent.r<4>(8)-0x7b80);
    const auto year=parent.memory.read<2>(date+2),day=parent.memory.read<2>(date);
    for(auto* s:{old.get(),modern.get()}) {
        s->wr<4>(12,date);s->wr<4>(16,year);s->wr<4>(20,day+1);s->wr<4>(4128,return_sentinel);
        s->pc=0x107156b0;s->budget=s->executed+1000000;run_until(*s,return_sentinel);
        s->wr<4>(12,parent.r<4>(12));s->wr<4>(16,parent.r<4>(16));s->wr<4>(4128,return_sentinel);
        s->pc=attributes::availability_entry;s->budget=s->executed+50000000;
    }
    actor_attributes_diagnostics(*old).original=true;Capture capture;capture.observe_daily_writes=true;actor_attributes_diagnostics(*modern).observer=&capture;
    Clock xclock,yclock;xclock.base=yclock.base=1000000000;old->code_fragment_observer=&xclock;modern->code_fragment_observer=&yclock;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    run_until(*old,return_sentinel);run_until(*modern,return_sentinel);
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;
    actor_attributes_diagnostics(*modern).observer=nullptr;
    Image(*old).compare(*modern,"Original availability caller after controlled original date advance");
    require(x.bytes==y.bytes && old->executed==modern->executed && xclock.events==yclock.events,"Daily rollover ordered effects/accounting/import ABI");
    require(bool(capture.daily_input),"Daily rollover must complete an actual record write");
    const auto& d=actor_attributes_diagnostics(*modern);
    require(d.calls[9]>0 && d.calls[9]==d.returns[9] && !d.original_calls[9] && !d.faults[9] && !d.dispatch_escapes,"Productive daily cache-miss counters");
    std::cout<<"PASS: controlled original date advance and actual availability caller complete daily record writes; complete state/effects/import ABI/accounting; recordRefreshCalls="<<d.calls[9]<<std::endl;
    return std::move(capture.daily_input);
}

std::uint32_t invoke_original(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args) {
    actor_attributes_diagnostics(s).original=true;unsigned n=3;
    for(auto arg:args)s.wr<4>(n++*4,arg);
    s.wr<4>(4128,return_sentinel);s.pc=entry;s.budget=s.executed+50000000;
    try {run_until(s,return_sentinel);}catch(const std::exception& e) {
        std::cerr<<"Original setup entry="<<hex_address(entry)<<" stopped="<<hex_address(s.pc)<<" reason="<<e.what()<<std::endl;throw;
    }
    actor_attributes_diagnostics(s).original=false;return std::uint32_t(s.r<4>(12));
}
std::unique_ptr<State> source_caller(const State& input,unsigned index,std::uint32_t callsite) {
    ActorAttributesIoPair io(input,index);auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;
    for(auto* s:{old.get(),modern.get()}) {s->pc=callsite;s->budget=s->executed+50000000;}
    actor_attributes_diagnostics(*old).original=true;Capture capture;actor_attributes_diagnostics(*modern).observer=&capture;
    Clock aclock,bclock;aclock.io=bclock.io=&io;bclock.modern=true;aclock.base=bclock.base=1000000000;old->code_fragment_observer=&aclock;modern->code_fragment_observer=&bclock;
    Effects a,b;a.state=old.get();b.state=modern.get();old->memory.date_observer=&a;modern->memory.date_observer=&b;
    run_until(*old,callsite+4);old->platform.reset();io.between();run_until(*modern,callsite+4);
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;
    actor_attributes_diagnostics(*modern).observer=nullptr;
    const auto label="Controlled original callsite "+hex_address(callsite)+" entry="+hex_address(attributes::entries[index]);
    Image(*old).compare(*modern,label);
    require(a.bytes==b.bytes && old->executed==modern->executed && aclock.events==bclock.events,label+" ordered effects/accounting/import ABI");
    const auto& diag=actor_attributes_diagnostics(*modern);
    require(capture.inputs[index] && diag.calls[index]==1 && diag.returns[index]==1 && !diag.original_calls[index] && !diag.faults[index] && !diag.dispatch_escapes,label+" productive C++ root");
    io.compare();if(index==12 || index==13)require(io.expected!=io.baseline,label+" must produce an actual history file change");
    capture.inputs[index]->platform=input.platform;
    std::cout<<"PASS: "<<label<<" using captured live-world state and original remaining callees"<<std::endl;
    return std::move(capture.inputs[index]);
}
void lifecycle_production(const State& parent,const std::filesystem::path& snapshots) {
    const auto manager=std::uint32_t(parent.r<4>(12)),staff=std::uint32_t(parent.r<4>(16));
    const auto index=std::uint32_t(parent.memory.read<4>(staff)),row=std::uint32_t(parent.memory.read<4>(manager))+index*0x24;
    auto read=clone(parent);read->platform=parent.platform;read->wr<4>(20,0);
    auto query=source_caller(*read,10,0x101309d4);save_runtime_snapshot(*query,snapshots/(hex_address(attributes::capacity_entry)+".snapshot"));matrix(*query,10,false);actual_fault(*query,10);
    auto percentage=clone(*query);percentage->platform=parent.platform;percentage->wr<4>(20,1);
    save_runtime_snapshot(*percentage,snapshots/"capacity-percentage.snapshot");matrix(*percentage,10,false);
    auto assignment=clone(parent);assignment->platform=parent.platform;
    invoke_original(*assignment,attributes::reset_entry,{manager,index});
    invoke_original(*assignment,attributes::capacity_assignment_entry,{manager,staff,9000,1200});
    require(assignment->memory.read<2>(row+12)==1200,"Original preparation must reach the random-assignment scalar band");
    assignment->wr<4>(12,manager);assignment->wr<4>(16,staff);assignment->wr<4>(20,8000);assignment->wr<4>(24,5);
    auto assign=source_caller(*assignment,11,0x101309f8);save_runtime_snapshot(*assign,snapshots/(hex_address(attributes::capacity_assignment_entry)+".snapshot"));matrix(*assign,11,false);actual_fault(*assign,11);
    auto injured=clone(parent);injured->platform=parent.platform;
    const auto stack=std::uint32_t(injured->r<4>(4));injured->wr<4>(4,stack-0x80);injured->memory.write<4>(stack-0x80+0x38,0);
    invoke_original(*injured,0x10188cc0,{manager,0,0,staff,0,0,0xffffffff,0});
    require(injured->r<4>(4)==stack-0x80,"Original injury registration caller frame");injured->wr<4>(4,stack);
    require(signed_value<4>(injured->memory.read<4>(row+0x10))>=0,"Original registration must create an active injury record");
    injured->wr<4>(12,manager);injured->wr<4>(16,staff);injured->wr<4>(20,0);
    auto retirement=source_caller(*injured,12,0x10186f28);save_runtime_snapshot(*retirement,snapshots/(hex_address(attributes::retirement_entry)+".snapshot"));matrix(*retirement,12,false);actual_fault(*retirement,12);
    auto reset=clone(*injured);reset->platform=parent.platform;
    const auto queue=invoke_original(*reset,0x10773520,{16});require(queue!=0,"Original queue allocation");invoke_original(*reset,0x10738c00,{queue});reset->memory.write<4>(manager+0x28,queue);
    reset->wr<4>(4,stack-0x80);const auto item=stack-0x80+0x50;
    for(auto key:{index,index?0u:1u,index}) {reset->memory.write<4>(item,key);reset->memory.write<4>(item+4,0x01050000);invoke_original(*reset,0x10738d10,{queue,item,8});}
    reset->wr<4>(4,stack);reset->wr<4>(12,manager);reset->wr<4>(16,index);
    auto clear=source_caller(*reset,13,0x102e4720);save_runtime_snapshot(*clear,snapshots/(hex_address(attributes::reset_entry)+".snapshot"));matrix(*clear,13,false);actual_fault(*clear,13);
    std::cout<<"PASS: controlled actual-data lifecycle: raw/percentage readout, random assignment, original injury registration, retirement and queue-backed reset"<<std::endl;
}


void sampling_production(const State& parent,const std::filesystem::path& snapshots) {
    auto small=clone(parent);small->platform=parent.platform;small->wr<4>(12,17);
    auto input=source_caller(*small,14,0x10186c34);
    save_runtime_snapshot(*input,snapshots/(hex_address(attributes::sampling_entry)+".snapshot"));matrix(*input,14,false);
    for(auto [bound,name]:std::array<std::pair<std::uint32_t,const char*>,3>{{{1000000,"sampling-wide.snapshot"},{0,"sampling-zero.snapshot"},{1000000,"sampling-wrap.snapshot"}}}) {
        auto variant=clone(*input);variant->platform=parent.platform;variant->wr<4>(12,bound);
        if(std::string_view(name)=="sampling-wrap.snapshot") {
            const auto toc=std::uint32_t(variant->r<4>(8));const auto cursor=std::uint32_t(variant->memory.read<4>(toc-0x7b84)),limit=std::uint32_t(variant->memory.read<4>(toc-0x7b88));
            variant->memory.write<4>(cursor,variant->memory.read<4>(limit));
        }
        save_runtime_snapshot(*variant,snapshots/name);matrix(*variant,14,false);
    }
    auto recursive=clone(*input);recursive->platform=parent.platform;recursive->wr<4>(12,1000000);
    save_runtime_snapshot(*recursive,snapshots/"sampling-recursive.snapshot");matrix(*recursive,14,false,true);
    std::cout<<"PASS: structural recursive sampler control reaches two C++ calls/returns through direct, generated and CFM paths with full state/effects/import ABI/accounting"<<std::endl;
    std::cout<<"PASS: controlled original sampler BL with actual live TOC/random pool; prepared small, wide, zero and pool-wrap inputs; full state/effects/import ABI/accounting"<<std::endl;
}

}
int main(int argc,char** argv){try {
    const bool sampling=argc==6 && std::string_view(argv[5])=="--sampling";
    const bool focused=argc==6 && (std::string_view(argv[5])=="--lifecycle" || sampling);
    require(argc==4 || argc==5 || focused,"Expected original program, game, fresh backing and optional profile directory or focused lifecycle snapshot");
    const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Profile request/flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    OwnedBacking cleanup{root};
    if(focused) {
        State seed;load_program(seed,argv[1]);seed.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(seed,argv[4]);
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/(sampling?"sampling-focused-profile":"lifecycle-focused-profile");std::filesystem::create_directories(snapshots);
        if(sampling) {sampling_production(seed,snapshots);require(cases==85 && returns==50 && budgets==35 && negatives==5,"Focused sampler matrix incomplete");
            std::cout<<"PASS: focused sampler cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<std::endl;return 0;}
        lifecycle_production(seed,snapshots);require(cases==85 && returns==50 && budgets==35 && negatives==5,"Focused lifecycle matrix incomplete");
        std::cout<<"PASS: focused lifecycle cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<std::endl;return 0;
    }
    if(profile){
        for(unsigned index=0;index<attributes::entries.size();++index){
            State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
            load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(attributes::entries[index])+".snapshot"));
            require(input.pc==attributes::entries[index],"Snapshot is not an actual original-caller actor-attribute entry");matrix(input,index,true);
        }
        State updated;load_program(updated,argv[1]);updated.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
        load_match_snapshot(updated,std::filesystem::path(argv[4])/"daily-updated.snapshot");matrix(updated,9,true);
        State percentage;load_program(percentage,argv[1]);percentage.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
        load_match_snapshot(percentage,std::filesystem::path(argv[4])/"capacity-percentage.snapshot");matrix(percentage,10,true);
        for(const auto* name:{"sampling-wide.snapshot","sampling-zero.snapshot","sampling-wrap.snapshot"}) {
            State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
            load_match_snapshot(input,std::filesystem::path(argv[4])/name);matrix(input,14,true);
        }
        State recursive;load_program(recursive,argv[1]);recursive.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
        load_match_snapshot(recursive,std::filesystem::path(argv[4])/"sampling-recursive.snapshot");matrix(recursive,14,true,true);
    } else {
        auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto& world=*game.state;
        Capture capture;const auto calls=complete_match(world,game.match,capture);
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"actor-attributes-live-profile";
        std::filesystem::create_directories(snapshots);

        for(unsigned index=0;index<10;++index){
            require(bool(capture.inputs[index]),"Missing original-caller actor-attribute capture "+hex_address(attributes::entries[index]));
            require(calls[index]>0,"Original-caller actor-attributes coverage/outcome mismatch");
            save_runtime_snapshot(*capture.inputs[index],snapshots/(hex_address(attributes::entries[index])+".snapshot"));
            matrix(*capture.inputs[index],index,false);
            if(index!=1)actual_fault(*capture.inputs[index],index);

            std::cout<<"Integrated actor-attribute source="<<"natural complete match"<<" entry="<<hex_address(attributes::entries[index])<<" calls="<<calls[index]<<" returns="<<calls[index]<<std::endl;
        }
        auto updated=daily_rollover(*capture.inputs[7]);
        save_runtime_snapshot(*updated,snapshots/"daily-updated.snapshot");matrix(*updated,9,false);
        lifecycle_production(*capture.inputs[7],snapshots);
        sampling_production(*capture.inputs[7],snapshots);
        require(cases==351 && negatives==21 && budgets==141,"Real actor-attribute matrix incomplete");
    }
    std::cout<<"PASS: production actor-attributes cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives
        <<"; real match caller direct/generated/CFM, original/profile controls, dedicated productive counts, complete state/effects/accounting/import boundaries\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
