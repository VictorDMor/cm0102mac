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
namespace injury=cm::aot::injury_readouts;
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
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Injury readout awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
constexpr std::array<std::uint32_t,8> ends{0x1018c26c,0x1018c2e4,0x1018c33c,0x1018c560,0x1018d140,0x1018d94c,0x1018daa8,0x1018dcb0};
std::array<std::set<std::uint32_t>,8> runtime_sites;
void record_site(State& s,std::uint32_t pc) {if(s.executed>=s.budget)return;for(unsigned n=0;n<8;++n)if(pc>=injury::entries[n] && pc<ends[n])runtime_sites[n].insert(pc);}
struct Witness:InjuryReadoutsBoundaryObserver {
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool,InjuryReadoutsOutcome)override{}
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
};
struct Capture:InjuryReadoutsBoundaryObserver {
    void before_step(State& s,std::uint32_t pc)override{record_site(s,pc);}
    std::array<std::unique_ptr<State>,8> inputs;
    std::array<std::uint32_t,8> callsites{};
    void enter(const State& state,std::uint32_t entry,bool original)override {
        if(original)return;auto it=std::find(injury::entries.begin(),injury::entries.end(),entry);require(it!=injury::entries.end(),"Unknown injury capture");const auto index=unsigned(it-injury::entries.begin());
        if(inputs[index])return;callsites[index]=std::uint32_t(state.r<4>(4128))-4;inputs[index]=clone(state);inputs[index]->platform=state.platform;inputs[index]->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,InjuryReadoutsOutcome)override{}
};
struct Negative:InjuryReadoutsBoundaryObserver {
    unsigned index;
    explicit Negative(unsigned value):index(value){}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,InjuryReadoutsOutcome result)override {
        if(entry==injury::entries[index] && !original && result==InjuryReadoutsOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);
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
    require(relative.parent_path().empty() && name.starts_with("prepared-backing-") && std::filesystem::is_directory(snapshots/relative) && !std::filesystem::is_symlink(snapshots/relative),"Retained original readout history backing");
    ActorAttributesIoPair::copy(snapshots/relative,root);
}
unsigned cases=0,returns=0,budgets=0,negatives=0;
std::string execute(State& s,Path path,unsigned index) {
    try {
        if(path==Path::direct)try_modern_injury_readouts(s,injury::entries[index]);
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
        try {return ActorAttributesIoPair(input,index,injury::entries[index],[&](State& x){injury_readouts_diagnostics(x).original=true;x.code_fragment_observer=&probe_clock;});}
        catch(...) {for(auto it=probe_clock.events.rbegin();it!=probe_clock.events.rend() && it<probe_clock.events.rbegin()+std::min<std::size_t>(8,probe_clock.events.size());++it)std::cerr<<"Probe import "<<it->name<<" leave="<<it->leave<<" pc="<<hex_address(it->pc)<<std::endl;throw;}
    }();auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    injury_readouts_diagnostics(*a).original=true;actor_attributes_diagnostics(*a).original=true;auto& d=injury_readouts_diagnostics(*b);
    if(mode==Mode::original){d.original=true;actor_attributes_diagnostics(*b).original=true;}if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(index);Witness witness;if(negative)d.observer=&mutation;else if(mode==Mode::modern)d.observer=&witness;
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;old_clock.base=new_clock.base=1000000000;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index);a->platform.reset();io.between();const auto actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production injury-readouts entry="+hex_address(injury::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
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
    Clock probe;probe.base=1000000000;ActorAttributesIoPair io(input,index,injury::entries[index],[&](State& x){injury_readouts_diagnostics(x).original=true;x.code_fragment_observer=&probe;});
    auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;
    for(auto* s:{old.get(),modern.get()}) {
        const auto toc=s->r<4>(8);constexpr std::uint32_t bad=0xf0000000;
        if(index<3)s->wr<4>(12,bad);
        else if(index==3 || index==6)s->memory.write<4>(toc-0x7be4,bad);
        else if(index==4)s->memory.write<4>(toc-0x5464,bad);
        else if(index==5){s->memory.write<1>(toc-0x35ac,9);s->memory.write<4>(toc-0x546c,bad);}
        else s->memory.write<4>(toc-0x5478,bad);
        s->pc=injury::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;
    }
    injury_readouts_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    const auto expected=execute(*old,Path::generated,index);old->platform.reset();io.between();const auto actual=execute(*modern,Path::generated,index);
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;
    require(expected==actual && expected.starts_with("Unmapped data access at "),"Actual readout pointer fault original="+expected+" modern="+actual);
    Image(*old).compare(*modern,"Actual injury readout pointer fault");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Actual readout fault effects/accounting/import ABI");io.compare();
    const auto& d=injury_readouts_diagnostics(*modern);require(d.calls[index]==1 && d.faults[index]==1 && !d.returns[index] && !d.dispatch_escapes && !d.original_body,"Actual readout fault counters");++fault_cases;
}

void matrix(const State& input,unsigned index,bool profile) {
    std::uint64_t full_steps=0;
    for(auto path:{Path::direct,Path::generated,Path::cfm}) {
        if(profile)compare(input,index,path,Mode::profile);
        else for(auto mode:{Mode::modern,Mode::original,Mode::entry_original}){const auto elapsed=compare(input,index,path,mode);if(path==Path::generated && mode==Mode::modern)full_steps=elapsed;}
    }
    if(!profile) {
        compare(input,index,Path::generated,Mode::modern,50000000,true);
        require(full_steps>0,"Original readout length measured before budget cuts");
        for(auto cut:{0u,1u,2u,3u,8u,12u,16u})compare(input,index,Path::generated,Mode::modern,cut,false,cut<full_steps);
    }
}
std::uint32_t invoke_original(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args) {
    auto& own=injury_readouts_diagnostics(s);auto& actor=actor_attributes_diagnostics(s);auto& record=injury_record_diagnostics(s);const auto old_own=own.original,old_actor=actor.original,old_record=record.original;own.original=actor.original=record.original=true;
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);s.wr<4>(4128,return_sentinel);s.pc=entry;s.budget=s.executed+50000000;
    try{run_until(s,return_sentinel);}catch(const std::exception& e){std::cerr<<"Original injury setup entry="<<hex_address(entry)<<" pc="<<hex_address(s.pc)<<" reason="<<e.what()<<std::endl;own.original=old_own;actor.original=old_actor;record.original=old_record;throw;}
    own.original=old_own;actor.original=old_actor;record.original=old_record;return std::uint32_t(s.r<4>(12));
}
std::unique_ptr<State> complete_match(const State& input,std::uint32_t match) {
    auto old=clone(input),modern=clone(input);old->platform=modern->platform=input.platform;injury_readouts_diagnostics(*old).original=true;actor_attributes_diagnostics(*old).original=true;
    Clock x,y;x.base=y.base=1000000000;old->code_fragment_observer=&x;modern->code_fragment_observer=&y;unsigned advances=0;
    for(;advances<1400;++advances) {
        for(auto* state:{old.get(),modern.get()}) {state->wr<4>(12,match);state->wr<4>(16,1);state->wr<4>(4128,return_sentinel);state->pc=0x101d3f60;state->budget=state->executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(*state,return_sentinel);}
        require(old->executed==modern->executed && old->r<4>(12)==modern->r<4>(12),"Injury paired match readout/accounting");if(advances%50==0)Image(*old).compare(*modern,"Injury paired intermediate match");
        if(modern->r<4>(12)==2){++advances;break;}
    }
    require(advances<1400 && modern->r<4>(12)==2,"Injury paired match reaches full time");Image(*old).compare(*modern,"Injury paired complete match");require(x.events==y.events && x.count==y.count,"Injury paired imports");
    const auto& a=injury_readouts_diagnostics(*old);const auto& b=injury_readouts_diagnostics(*modern);
    for(unsigned n=0;n<8;++n)require(a.original_calls[n]==b.calls[n] && b.calls[n]==b.returns[n] && !b.faults[n] && !b.dispatch_escapes,"Injury paired natural counts");
    std::cout<<"PASS: injury readout paired full match advances="<<advances<<" calls=";for(unsigned n=0;n<8;++n)std::cout<<(n?",":"")<<b.calls[n];std::cout<<"; complete state/import ABI/per-tick accounting"<<std::endl;
    old->code_fragment_observer=modern->code_fragment_observer=nullptr;return modern;
}

std::unique_ptr<State> prepare_readouts(const State& full,std::uint32_t match) {
    auto s=clone(full);s->platform=full.platform;const auto toc=std::uint32_t(s->r<4>(8));
    const auto manager=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7ba0))),staffs=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7be0)));
    const auto stats=invoke_original(*s,0x10773520,{0x1000}),service=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7b4c)));
    require(stats!=0,"Original readout statistics allocation");invoke_original(*s,0x1029f920,{service,match,stats,0});
    const auto id=std::uint32_t(s->memory.read<4>(stats+0x194)),staff=staffs+id*0x70,count=std::uint32_t(s->memory.read<4>(s->memory.read<4>(toc-0x7be4)));
    require(id<count && s->memory.read<4>(staff)==id && s->memory.read<4>(staff+0x14)!=0,"Actual readout player/profile bounds");
    invoke_original(*s,0x107156b0,{std::uint32_t(s->memory.read<4>(toc-0x7b80)),194,2001});
    invoke_original(*s,0x107156b0,{std::uint32_t(s->memory.read<4>(toc-0x7a9c)),194,2001});
    invoke_original(*s,actor_attributes::reset_entry,{manager,id});
    const auto sp=std::uint32_t(s->r<4>(4));s->wr<4>(4,sp-0x80);s->memory.write<4>(sp-0x80+0x38,0);
    invoke_original(*s,injury_record::registration_entry,{manager,0,0,staff,0,0,0xffffffff,0});
    invoke_original(*s,actor_attributes::retirement_entry,{manager,staff,0});
    s->memory.write<4>(sp-0x80+0x38,0);invoke_original(*s,injury_record::registration_entry,{manager,0,0,staff,0,0,0xffffffff,0});
    s->memory.write<4>(sp-0x80+0x38,1);invoke_original(*s,injury_record::registration_entry,{manager,11,0,staff,0,0,0xffffffff,3});s->wr<4>(4,sp);
    const auto row=std::uint32_t(s->memory.read<4>(manager))+id*0x24;
    require(signed_value<4>(s->memory.read<4>(row+0x10))>=0 && s->memory.read<4>(row+0x1c)!=0xffffffff && s->memory.read<1>(row+0x14)==11 && s->memory.read<1>(row+0x15)==3,"Original setup provides active/history and deferred-risk readouts");
    const auto output=invoke_original(*s,0x10773520,{0x800});require(output!=0,"Original readout output allocation");s->memory.copy_in(output,std::vector<std::uint8_t>(0x800,0x5a));
    s->wr<4>(12,manager);s->wr<4>(16,staff);s->wr<4>(20,output);s->wr<4>(24,80);s->wr<4>(28,0);s->wr<4>(32,0);s->wr<4>(4128,return_sentinel);s->pc=injury::history_entry;
    injury_readouts_diagnostics(*s).original=false;actor_attributes_diagnostics(*s).original=false;injury_record_diagnostics(*s).original=false;return s;
}
constexpr std::array<std::uint32_t,8> callsites{0x103a623c,0x101f55cc,0x1018b960,0x10397c14,0x1018c548,0x1018f784,0x10397c30,0x10397d74};
constexpr std::array<std::pair<unsigned,const char*>,11> variants{{
    {0,"history-empty.snapshot"},{1,"risk-inactive.snapshot"},{1,"risk-minimum.snapshot"},{2,"risk-profile-guard.snapshot"},
    {3,"current-none.snapshot"},{4,"type-alternate.snapshot"},{5,"form-nine.snapshot"},{5,"form-three.snapshot"},{5,"form-five.snapshot"},
    {6,"remaining-total.snapshot"},{7,"category-last.snapshot"}}};
void extra_cases(const std::array<std::unique_ptr<State>,8>& inputs,const std::filesystem::path& snapshots) {
    for(auto [index,name]:variants) {
        auto input=clone(*inputs[index]);input->platform=inputs[index]->platform;const auto label=std::string_view(name);const auto toc=input->r<4>(8);
        if(index<4 || index==6) {
            const auto manager=std::uint32_t(input->r<4>(12)),staff=std::uint32_t(input->r<4>(16)),id=std::uint32_t(input->memory.read<4>(staff)),row=std::uint32_t(input->memory.read<4>(manager))+id*0x24;
            if(label=="history-empty.snapshot")input->memory.write<4>(manager+12,0);
            if(label=="risk-inactive.snapshot")input->memory.write<1>(row+0x14,255);
            if(label=="risk-minimum.snapshot")input->memory.write<1>(row+0x15,0);
            if(label=="risk-profile-guard.snapshot")input->memory.write<4>(staff+0x14,0);
            if(label=="current-none.snapshot"){const auto registers=input->registers;invoke_original(*input,actor_attributes::retirement_entry,{manager,staff,0});input->registers=registers;require(input->memory.read<4>(row+0x10)==0xffffffff,"Original retirement gives the empty readout");}
            if(label=="remaining-total.snapshot")input->wr<4>(28,1);
        }
        if(label=="type-alternate.snapshot"){input->wr<4>(28,1);input->memory.write<1>(toc-0x35ac,0);}
        if(index==5)input->memory.write<1>(toc-0x35ac,label=="form-nine.snapshot"?9:label=="form-three.snapshot"?3:5);
        if(label=="category-last.snapshot"){input->wr<4>(16,12);input->memory.write<1>(toc-0x35ac,3);}
        injury_readouts_diagnostics(*input).original=false;actor_attributes_diagnostics(*input).original=false;injury_record_diagnostics(*input).original=false;
        input->pc=injury::entries[index];input->wr<4>(4128,return_sentinel);save_runtime_snapshot(*input,snapshots/name);matrix(*input,index,false);
        std::cout<<"PASS: controlled actual-data injury readout input="<<name<<std::endl;
    }
}
void capture_caller(const State& input,unsigned index,Capture& capture) {
    Clock probe;probe.base=1000000000;
    ActorAttributesIoPair io(input,index,injury::entries[index],[&](State& x){injury_readouts_diagnostics(x).original=true;x.code_fragment_observer=&probe;});
    auto old=clone(input),modern=clone(input);old->platform=io.original;modern->platform=io.modern;injury_readouts_diagnostics(*old).original=true;
    capture.inputs[index].reset();capture.callsites[index]=0;injury_readouts_diagnostics(*modern).observer=&capture;
    Clock a,b;a.io=b.io=&io;b.modern=true;a.base=b.base=1000000000;old->code_fragment_observer=&a;modern->code_fragment_observer=&b;
    Effects x,y;x.state=old.get();y.state=modern.get();old->memory.date_observer=&x;modern->memory.date_observer=&y;
    for(auto* s:{old.get(),modern.get()}){s->pc=callsites[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;}
    run_until(*old,callsites[index]+4);old->platform.reset();io.between();run_until(*modern,callsites[index]+4);
    old->memory.date_observer=modern->memory.date_observer=nullptr;old->code_fragment_observer=modern->code_fragment_observer=nullptr;injury_readouts_diagnostics(*modern).observer=nullptr;
    Image(*old).compare(*modern,"Complete readout through original BL");require(x.bytes==y.bytes && old->executed==modern->executed && a.events==b.events,"Readout caller effects/accounting/import ABI");io.compare();
    const auto& d=injury_readouts_diagnostics(*modern);require(d.calls[index]==1 && d.returns[index]==1 && !d.original_calls[index] && !d.faults[index] && !d.dispatch_escapes,"Dedicated productive readout caller counter");
    if(index==3)require(d.calls[4]>=1 && d.calls[4]==d.returns[4],"Current readout reaches the reconstructed type description");
    if(index==0)require(modern->r<4>(12)!=0,"Actual injury history query finds the registered record");
    if(index==1)require(modern->r<4>(12)==3,"Actual deferred-risk multiplier");if(index==2)require(modern->r<4>(12)==11,"Actual deferred-risk type");
    std::cout<<"PASS: original injury readout BL="<<hex_address(callsites[index])<<" entry="<<hex_address(injury::entries[index])<<" calls="<<d.calls[index]<<" returns="<<d.returns[index]<<std::endl;
}
void capture_readouts(const State& input,const std::filesystem::path& snapshots) {
    Capture capture;
    for(unsigned index=0;index<8;++index) {
        if(index==4)continue;auto prepared=clone(input);prepared->platform=input.platform;
        if(index==5 || index==7)prepared->wr<4>(16,0);
        prepared->pc=injury::entries[index];capture_caller(*prepared,index,capture);
    }
    for(unsigned n=0;n<8;++n){require(bool(capture.inputs[n]),"Original callers capture every readout root");require(capture.callsites[n]==callsites[n],"Audited original readout caller reached");capture.inputs[n]->platform=input.platform;save_runtime_snapshot(*capture.inputs[n],snapshots/(hex_address(injury::entries[n])+".snapshot"));matrix(*capture.inputs[n],n,false);}
    extra_cases(capture.inputs,snapshots);for(unsigned n=0;n<8;++n)actual_fault(*capture.inputs[n],n);
}
}
int main(int argc,char** argv){try {
    const bool prepared=argc==6 && std::string_view(argv[5])=="--prepared";
    require(argc==4 || argc==5 || prepared,"Expected original program/game/private backing and optional readout profile directory");const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Readout profile flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(root),"Fresh private readout backing");OwnedBacking cleanup{root};
    if(prepared){
        const auto snapshots=std::filesystem::path(argv[4]).parent_path();copy_retained_backing(snapshots,root);State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,argv[4]);capture_readouts(input,snapshots);
    }
    else if(profile) {
        copy_retained_backing(argv[4],root);
        for(unsigned n=0;n<8;++n){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(injury::entries[n])+".snapshot"));matrix(input,n,true);}
        for(auto [index,name]:variants){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);load_match_snapshot(input,std::filesystem::path(argv[4])/name);matrix(input,index,true);}
    } else {
        auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto full=complete_match(*game.state,game.match);auto input=prepare_readouts(*full,game.match);
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"injury-readouts-live-profile";std::filesystem::create_directories(snapshots);
        const auto backing="prepared-backing-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());ActorAttributesIoPair::copy(input->platform->writable_directory(),snapshots/backing);
        {std::ofstream pointer(snapshots/"prepared-backing.txt");require(bool(pointer),"Retain original readout history backing path");pointer<<backing<<std::endl;}
        save_runtime_snapshot(*input,snapshots/"prepared-readouts.snapshot");capture_readouts(*input,snapshots);
    }
    if(!profile){std::ofstream log(std::filesystem::path(argv[3]).parent_path()/"injury-readouts-runtime-sites.txt");for(unsigned n=0;n<8;++n)for(auto pc:runtime_sites[n])log<<n<<" "<<hex_address(pc)<<std::endl;}
    std::cout<<"Runtime injury readout source sites=";for(unsigned n=0;n<8;++n)std::cout<<(n?",":"")<<runtime_sites[n].size();std::cout<<std::endl;
    std::cout<<"Actual injury readout pointer faults="<<fault_cases<<std::endl;
    std::cout<<"PASS: injury readout production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
