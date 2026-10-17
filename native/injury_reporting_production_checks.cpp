#include "injury_reporting_adapter.hpp"
#include "injury_readouts_adapter.hpp"
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
namespace injury=cm::aot::injury_reporting;
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
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Injury reporting awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
constexpr std::array<std::uint32_t,4> ends{0x1018f468,0x1018fbd8,0x1018fdf0,0x1018ff04};
std::array<std::set<std::uint32_t>,4> runtime_sites;
void record_site(State& s,std::uint32_t pc) {if(s.executed>=s.budget)return;for(unsigned n=0;n<4;++n)if(pc>=injury::entries[n] && pc<ends[n])runtime_sites[n].insert(pc);}
struct Witness:InjuryReportingBoundaryObserver {
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool,InjuryReportingOutcome)override{}
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
};
struct Capture:InjuryReportingBoundaryObserver {
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
    std::array<std::unique_ptr<State>,4> inputs;
    std::array<std::uint32_t,4> callsites{};
    void enter(const State& state,std::uint32_t entry,bool original)override {
        if(original)return;auto it=std::find(injury::entries.begin(),injury::entries.end(),entry);require(it!=injury::entries.end(),"Unknown injury capture");const auto index=unsigned(it-injury::entries.begin());
        if(inputs[index])return;callsites[index]=std::uint32_t(state.r<4>(4128))-4;inputs[index]=clone(state);inputs[index]->platform=state.platform;inputs[index]->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,InjuryReportingOutcome)override{}
};
struct Negative:InjuryReportingBoundaryObserver {
    unsigned index;
    explicit Negative(unsigned value):index(value){}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,InjuryReportingOutcome result)override {
        if(entry==injury::entries[index] && !original && result==InjuryReportingOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);
    }
};
struct OwnedBacking {
    std::filesystem::path directory;
    ~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(directory,e)||e||std::filesystem::is_symlink(directory,e)||e)return;
        for(const auto& p:std::filesystem::recursive_directory_iterator(directory,e))if(e||p.is_symlink(e)||e)return;
        std::filesystem::remove_all(directory,e);}
};
void copy_retained_backing(const std::filesystem::path& snapshots,const std::filesystem::path& root) {
    std::ifstream pointer(snapshots/"prepared-backing.txt");std::string name;std::getline(pointer,name);const auto relative=std::filesystem::path(name);
    require(relative.parent_path().empty() && name.starts_with("prepared-backing-") && std::filesystem::is_directory(snapshots/relative) && !std::filesystem::is_symlink(snapshots/relative),"Retained original reporting history backing");
    ActorAttributesIoPair::copy(snapshots/relative,root);
}
unsigned cases=0,returns=0,budgets=0,negatives=0;
std::string execute(State& s,Path path,unsigned index) {
    try {
        if(path==Path::direct)try_modern_injury_reporting(s,injury::entries[index]);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,injury::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
std::uint64_t compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,bool budget_expected=true) {
    Clock probe_clock;probe_clock.base=1000000000;
    ActorAttributesIoPair io=[&]() {
        try {return ActorAttributesIoPair(input,index,injury::entries[index],[&](State& x){injury_reporting_diagnostics(x).original=true;x.code_fragment_observer=&probe_clock;});}
        catch(...) {for(auto it=probe_clock.events.rbegin();it!=probe_clock.events.rend() && it<probe_clock.events.rbegin()+std::min<std::size_t>(8,probe_clock.events.size());++it)std::cerr<<"Probe import "<<it->name<<" leave="<<it->leave<<" pc="<<hex_address(it->pc)<<std::endl;throw;}
    }();auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    injury_reporting_diagnostics(*a).original=true;actor_attributes_diagnostics(*a).original=true;injury_readouts_diagnostics(*a).original=true;auto& d=injury_reporting_diagnostics(*b);
    if(mode==Mode::original){d.original=true;actor_attributes_diagnostics(*b).original=true;}if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(index);Witness witness;if(negative)d.observer=&mutation;else if(mode==Mode::modern)d.observer=&witness;
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;old_clock.base=new_clock.base=1000000000;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index);a->platform.reset();io.between();const auto actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production injury-reporting entry="+hex_address(injury::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(limit==50000000 || !budget_expected?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected measured boundary");
    require(expected.empty()?a->executed<=a->budget:a->executed==a->budget+1,label+" exact original budget position");
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,label+" host input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative failed");++negatives;}
    else {Image(*a).compare(*b,label);require(x.bytes==y.bytes && a->executed==b->executed,label+" ordered effects/accounting");}
    io.compare();
    require(!d.original_body && !d.dispatch_escapes,label+" own-body scope residue/dispatch escape");
    if(mode==Mode::modern){require(d.calls[index]>=1 && !d.original_calls[index] && !d.profile_original_calls[index],label+" no dedicated modern call");
        require(d.returns[index]==(expected.empty()?d.calls[index]:0) && d.budget_stops[index]==unsigned(!expected.empty()),label+" productive outcome counts");}
    else {require(!d.calls[index] && !d.returns[index],label+" original control entered C++ root");require(mode==Mode::profile?d.profile_original_calls[index]>=1:d.original_calls[index]>=1,label+" selected original/profile not reached");}
    ++cases;returns+=expected.empty();budgets+=!expected.empty();
    return a->executed-input.executed;
}

unsigned fault_cases=0;
void actual_fault(const State& input,unsigned index) {
    Clock probe;probe.base=1000000000;ActorAttributesIoPair io(input,index,injury::entries[index],[&](State& x){injury_reporting_diagnostics(x).original=true;x.code_fragment_observer=&probe;});
    auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;
    for(auto* s:{old.get(),modern.get()}) {
        const auto toc=s->r<4>(8);constexpr std::uint32_t bad=0xf0000000;
        if(index==0)s->memory.write<4>(toc-0x7ba0,bad);
        else if(index==3)s->wr<4>(12,bad);
        else s->wr<4>(16,bad);
        s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;
    }
    injury_reporting_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;injury_readouts_diagnostics(*old).original=true;Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    const auto expected=execute(*old,Path::generated,index);old->platform.reset();io.between();const auto actual=execute(*modern,Path::generated,index);
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;
    require(expected==actual && expected.starts_with("Unmapped data access at "),"Actual reporting pointer fault original="+expected+" modern="+actual);
    Image(*old).compare(*modern,"Actual injury reporting pointer fault");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Actual reporting fault effects/accounting/import ABI");io.compare();
    const auto& d=injury_reporting_diagnostics(*modern);require(d.calls[index]==1 && d.faults[index]==1 && !d.returns[index] && !d.dispatch_escapes && !d.original_body,"Actual reporting fault counters");++fault_cases;
}

void matrix(const State& input,unsigned index,bool profile) {
    std::uint64_t full_steps=0;
    for(auto path:{Path::direct,Path::generated,Path::cfm}) {
        if(profile)compare(input,index,path,Mode::profile);
        else for(auto mode:{Mode::modern,Mode::original,Mode::entry_original}){const auto elapsed=compare(input,index,path,mode);if(path==Path::generated && mode==Mode::modern)full_steps=elapsed;}
    }
    if(!profile) {
        compare(input,index,Path::generated,Mode::modern,50000000,true);
        require(full_steps>0,"Original reporting length measured before budget cuts");
        for(auto cut:{0u,1u,2u,3u,8u,12u,16u})compare(input,index,Path::generated,Mode::modern,cut,false,cut<full_steps);
    }
}
std::uint32_t invoke_original(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args) {
    auto& own=injury_reporting_diagnostics(s);auto& actor=actor_attributes_diagnostics(s);auto& record=injury_record_diagnostics(s);auto& readouts=injury_readouts_diagnostics(s);
    const auto old_own=own.original,old_actor=actor.original,old_record=record.original,old_readouts=readouts.original;own.original=actor.original=record.original=readouts.original=true;
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);s.wr<4>(4128,return_sentinel);s.pc=entry;s.budget=s.executed+50000000;
    try{run_until(s,return_sentinel);}catch(const std::exception& e){std::cerr<<"Original injury setup entry="<<hex_address(entry)<<" pc="<<hex_address(s.pc)<<" reason="<<e.what()<<std::endl;own.original=old_own;actor.original=old_actor;record.original=old_record;readouts.original=old_readouts;throw;}
    own.original=old_own;actor.original=old_actor;record.original=old_record;readouts.original=old_readouts;return std::uint32_t(s.r<4>(12));
}
std::unique_ptr<State> complete_match(const State& input,std::uint32_t match) {
    auto old=clone(input),modern=clone(input);old->platform=modern->platform=input.platform;injury_reporting_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;injury_readouts_diagnostics(*old).original=true;
    Clock x,y;x.base=y.base=1000000000;old->code_fragment_observer=&x;modern->code_fragment_observer=&y;unsigned advances=0;
    for(;advances<1400;++advances) {
        for(auto* state:{old.get(),modern.get()}) {state->wr<4>(12,match);state->wr<4>(16,1);state->wr<4>(4128,return_sentinel);state->pc=0x101d3f60;state->budget=state->executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(*state,return_sentinel);}
        require(old->executed==modern->executed && old->r<4>(12)==modern->r<4>(12),"Injury paired match reporting/accounting");if(advances%50==0)Image(*old).compare(*modern,"Injury paired intermediate match");
        if(modern->r<4>(12)==2){++advances;break;}
    }
    require(advances<1400 && modern->r<4>(12)==2,"Injury paired match reaches full time");Image(*old).compare(*modern,"Injury paired complete match");require(x.events==y.events && x.count==y.count,"Injury paired imports");
    const auto& a=injury_reporting_diagnostics(*old);const auto& b=injury_reporting_diagnostics(*modern);
    for(unsigned n=0;n<4;++n)require(a.original_calls[n]==b.calls[n] && b.calls[n]==b.returns[n] && !b.faults[n] && !b.dispatch_escapes,"Injury paired natural counts");
    std::cout<<"PASS: injury reporting paired full match advances="<<advances<<" calls=";for(unsigned n=0;n<4;++n)std::cout<<(n?",":"")<<b.calls[n];std::cout<<"; complete state/import ABI/per-tick accounting"<<std::endl;
    old->code_fragment_observer=modern->code_fragment_observer=nullptr;return modern;
}

std::unique_ptr<State> prepare_reporting(const State& full,std::uint32_t match) {
    auto s=clone(full);s->platform=full.platform;const auto toc=std::uint32_t(s->r<4>(8));
    const auto manager=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7ba0))),staffs=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7be0)));
    // The isolated match factory omits the career's statistics archive. Build
    // its original four-slot instance only in this prepared reporting fixture.
    const auto archive_slot=std::uint32_t(s->memory.read<4>(toc-0x7b4c));
    if(!s->memory.read<4>(archive_slot)) {
        const auto archive=invoke_original(*s,0x10773520,{0x20});require(archive!=0,"Original reporting archive allocation");
        invoke_original(*s,0x1029e610,{archive,0,4});s->memory.write<4>(archive_slot,archive);
        require(s->memory.read<4>(archive+0x1c)==4,"Original reporting archive constructor");
    }
    const auto stats=invoke_original(*s,0x10773520,{0x1000}),service=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7b4c)));
    require(stats!=0,"Original reporting statistics allocation");invoke_original(*s,0x1029f920,{service,match,stats,0});
    const auto id=std::uint32_t(s->memory.read<4>(stats+0x194)),staff=staffs+id*0x70,count=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7be4)));
    require(id<count && s->memory.read<4>(staff)==id && s->memory.read<4>(staff+0x14)!=0,"Actual reporting player/profile bounds");
    invoke_original(*s,0x107156b0,{std::uint32_t(s->memory.read<4>(toc-0x7b80)),194,2001});
    invoke_original(*s,0x107156b0,{std::uint32_t(s->memory.read<4>(toc-0x7a9c)),194,2001});
    invoke_original(*s,actor_attributes::reset_entry,{manager,id});
    const auto fixture_record=std::uint32_t(s->memory.read<4>(match+0x3118));
    const auto sp=std::uint32_t(s->r<4>(4));s->wr<4>(4,sp-0x80);s->memory.write<4>(sp-0x80+0x38,0);
    invoke_original(*s,injury_record::registration_entry,{manager,39,0,staff,fixture_record,0,0xffffffff,0});
    const auto row=std::uint32_t(s->memory.read<4>(manager))+id*0x24;
    const auto index=signed_value<4>(s->memory.read<4>(row+0x10));require(index>=0,"Original reporting setup has an active injury");
    const auto active=std::uint32_t(s->memory.read<4>(manager+4))+std::uint32_t(index)*0x38;
    const auto context=invoke_original(*s,0x10773520,{0x38});require(context!=0,"Original injury match context allocation");s->memory.copy_in(context,s->memory.copy_out(active,0x38));
    require(s->memory.read<4>(context+0x18)!=s->memory.read<4>(context+0x1c) && s->memory.read<1>(context+0x20)!=255,"Original registration captures the actual match context");
    invoke_original(*s,actor_attributes::retirement_entry,{manager,staff,0});
    s->memory.write<4>(sp-0x80+0x38,0);invoke_original(*s,injury_record::registration_entry,{manager,39,0,staff,0,0,0xffffffff,0});s->wr<4>(4,sp);
    require(signed_value<4>(s->memory.read<4>(row+0x10))>=0 && s->memory.read<4>(row+0x1c)!=0xffffffff,"Original reporting setup has active and completed history");
    const auto message=invoke_original(*s,0x10773520,{0x400}),output=invoke_original(*s,0x10773520,{16});require(message && output,"Original reporting text/message allocations");
    invoke_original(*s,0x102bdf90,{message,2001,0});
    const std::array<std::uint32_t,11> arguments{0,39,id,
        std::uint32_t(s->memory.read<4>(s->memory.read<4>(staff+0x28)+0x20)),std::uint32_t(s->memory.read<4>(s->memory.read<4>(staff+0x2c)+0x20)),std::uint32_t(s->memory.read<4>(s->memory.read<4>(staff+0x30)+0x20)),194,2001,12,0,0};
    for(unsigned n=0;n<arguments.size();++n)invoke_original(*s,0x102cb4b0,{message,n,arguments[n]});
    invoke_original(*s,0x1074b6e0,{output,0});
    s->wr<4>(12,manager);s->wr<4>(16,staff);s->wr<4>(20,message);s->wr<4>(24,output);s->wr<4>(28,context);s->wr<4>(4128,return_sentinel);s->pc=injury::special_type_entry;
    injury_reporting_diagnostics(*s).original=false;actor_attributes_diagnostics(*s).original=false;injury_record_diagnostics(*s).original=false;injury_readouts_diagnostics(*s).original=false;return s;
}
constexpr std::array<std::uint32_t,4> callsites{0x10060e14,0x102c55ac,0x10397f54,0x10186f3c};
constexpr std::array<std::pair<unsigned,const char*>,12> variants{{
    {0,"special-81.snapshot"},{0,"special-none.snapshot"},{0,"special-guard.snapshot"},
    {1,"notice-headline.snapshot"},{1,"notice-progress-headline.snapshot"},{1,"notice-category.snapshot"},
    {1,"notice-special-start.snapshot"},{1,"notice-special-progress.snapshot"},
    {2,"match-less.snapshot"},{2,"match-equal.snapshot"},{2,"match-greater.snapshot"},
    {3,"history-already-invalid.snapshot"}}};
void capture_caller(const State& input,unsigned index,Capture& capture) {
    Clock probe;probe.base=1000000000;
    ActorAttributesIoPair io(input,index,injury::entries[index],[&](State& x){injury_reporting_diagnostics(x).original=true;x.code_fragment_observer=&probe;});
    auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;injury_reporting_diagnostics(*old).original=true;injury_readouts_diagnostics(*old).original=true;
    capture.inputs[index].reset();capture.callsites[index]=0;injury_reporting_diagnostics(*modern).observer=&capture;
    Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    for(auto* s:{old.get(),modern.get()}){s->pc=callsites[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;}
    run_until(*old,callsites[index]+4);old->platform.reset();io.between();run_until(*modern,callsites[index]+4);
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;injury_reporting_diagnostics(*modern).observer=nullptr;
    Image(*old).compare(*modern,"Complete reporting through original BL");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Reporting caller effects/accounting/import ABI");io.compare();
    const auto& d=injury_reporting_diagnostics(*modern);require(d.calls[index]==1 && d.returns[index]==1 && !d.original_calls[index] && !d.faults[index] && !d.dispatch_escapes,"Dedicated productive reporting caller counter");
    if(index==1){const auto& child=injury_readouts_diagnostics(*modern);require(child.calls[4]>=1 && child.calls[5]>=1 && child.calls[4]==child.returns[4] && child.calls[5]==child.returns[5],"Actual notice reaches reconstructed type text/form");}
    if(index==3){const auto manager=input.r<4>(12),id=input.memory.read<4>(input.r<4>(16)),row=input.memory.read<4>(manager)+id*0x24;require(modern->memory.read<4>(row+0x18)==input.memory.read<4>(row+0x18) && modern->memory.read<4>(row+0x1c)==0xffffffff,"Actual history invalidation preserves the start offset");require(io.expected==io.baseline,"Actual history invalidation preserves archived file bytes");}
    std::cout<<"PASS: original injury reporting BL="<<hex_address(callsites[index])<<" entry="<<hex_address(injury::entries[index])<<" calls="<<d.calls[index]<<" returns="<<d.returns[index]<<std::endl;
}
void extra_cases(const std::array<std::unique_ptr<State>,4>& inputs,const std::filesystem::path& snapshots) {
    for(auto [index,name]:variants) {
        auto input=clone(*inputs[index]);input->platform=inputs[index]->platform;const auto label=std::string_view(name);
        if(index==0 || index==3) {
            const auto manager=index==0?input->memory.read<4>(input->memory.read<4>(input->r<4>(8)-0x7ba0)):input->r<4>(12),staff=input->r<4>(16),id=input->memory.read<4>(staff),row=input->memory.read<4>(manager)+id*0x24;
            if(label=="special-81.snapshot"){const auto active=input->memory.read<4>(manager+4)+input->memory.read<4>(row+0x10)*0x38;input->memory.write<1>(active,81);}
            if(label=="special-none.snapshot")input->memory.write<4>(row+0x10,0xffffffff);
            if(label=="special-guard.snapshot")input->memory.write<4>(staff,input->memory.read<4>(input->memory.read<4>(input->r<4>(8)-0x7be4)));
            if(index==3)input->memory.write<4>(row+0x1c,0xffffffff);
        }
        if(index==1) {
            const auto message=std::uint32_t(input->r<4>(16));
            const auto set=[&](unsigned n,std::uint32_t value){const auto registers=input->registers;invoke_original(*input,0x102cb4b0,{message,n,value});input->registers=registers;};
            if(label=="notice-headline.snapshot")input->wr<4>(20,0);
            if(label=="notice-progress-headline.snapshot"){input->wr<4>(20,0);set(0,1);set(1,81);set(10,3);}
            if(label=="notice-category.snapshot")set(1,11);
            if(label=="notice-special-start.snapshot")set(1,81);
            if(label=="notice-special-progress.snapshot"){set(0,1);set(1,79);set(9,2);set(10,1);}
        }
        if(index==2){const auto context=input->r<4>(16);input->memory.write<1>(context+0x20,label=="match-less.snapshot"?0:label=="match-equal.snapshot"?1:2);input->memory.write<1>(context+0x21,1);}
        injury_reporting_diagnostics(*input).original=false;actor_attributes_diagnostics(*input).original=false;injury_record_diagnostics(*input).original=false;injury_readouts_diagnostics(*input).original=false;
        input->pc=injury::entries[index];input->wr<4>(4128,return_sentinel);save_runtime_snapshot(*input,snapshots/name);matrix(*input,index,false);
        std::cout<<"PASS: controlled actual-data injury reporting input="<<name<<std::endl;
    }
}
void capture_reporting(const State& input,const std::filesystem::path& snapshots) {
    Capture capture;const auto manager=std::uint32_t(input.r<4>(12)),staff=std::uint32_t(input.r<4>(16)),message=std::uint32_t(input.r<4>(20)),output=std::uint32_t(input.r<4>(24)),context=std::uint32_t(input.r<4>(28));
    for(unsigned index=0;index<4;++index) {
        auto prepared=clone(input);prepared->platform=input.platform;
        prepared->wr<4>(12,manager);prepared->wr<4>(16,index==1?message:index==2?context:staff);
        if(index==1){prepared->wr<4>(20,1);prepared->wr<4>(24,output);}if(index==2)prepared->wr<4>(20,output);
        capture_caller(*prepared,index,capture);
    }
    for(unsigned n=0;n<4;++n){require(bool(capture.inputs[n]) && capture.callsites[n]==callsites[n],"Audited original callers capture every reporting root");capture.inputs[n]->platform=input.platform;save_runtime_snapshot(*capture.inputs[n],snapshots/(hex_address(injury::entries[n])+".snapshot"));matrix(*capture.inputs[n],n,false);}
    extra_cases(capture.inputs,snapshots);for(unsigned n=0;n<4;++n)actual_fault(*capture.inputs[n],n);
}
}
int main(int argc,char** argv){try {
    const bool prepared=argc==6 && std::string_view(argv[5])=="--prepared";
    require(argc==4 || argc==5 || prepared,"Expected original program/game/private backing and optional reporting profile directory");const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Reporting profile flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(root),"Fresh private reporting backing");OwnedBacking cleanup{root};
    if(prepared){const auto snapshots=std::filesystem::path(argv[4]).parent_path();copy_retained_backing(snapshots,root);State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,argv[4]);capture_reporting(input,snapshots);}
    else if(profile) {
        copy_retained_backing(argv[4],root);
        for(unsigned n=0;n<4;++n){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(injury::entries[n])+".snapshot"));matrix(input,n,true);}
        for(auto [index,name]:variants){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,std::filesystem::path(argv[4])/name);matrix(input,index,true);}
    } else {
        auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto full=complete_match(*game.state,game.match);auto input=prepare_reporting(*full,game.match);
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"injury-reporting-live-profile";std::filesystem::create_directories(snapshots);
        const auto backing="prepared-backing-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());ActorAttributesIoPair::copy(input->platform->writable_directory(),snapshots/backing);
        {std::ofstream pointer(snapshots/"prepared-backing.txt");require(bool(pointer),"Retain original reporting history backing path");pointer<<backing<<std::endl;}
        save_runtime_snapshot(*input,snapshots/"prepared-reporting.snapshot");capture_reporting(*input,snapshots);
    }
    if(!profile){require(runtime_sites[0].contains(0x1018f43c),"Actual type-81 input reaches the positive predicate result");std::ofstream log(std::filesystem::path(argv[3]).parent_path()/"injury-reporting-runtime-sites.txt");for(unsigned n=0;n<4;++n)for(auto pc:runtime_sites[n])log<<n<<" "<<hex_address(pc)<<std::endl;}
    std::cout<<"Runtime injury reporting source sites=";for(unsigned n=0;n<4;++n)std::cout<<(n?",":"")<<runtime_sites[n].size();std::cout<<std::endl;
    std::cout<<"Actual injury reporting pointer faults="<<fault_cases<<std::endl;
    std::cout<<"PASS: injury reporting production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
