#include "actor_attributes_adapter.hpp"
#include "match_debugger_factory.hpp"
#include "semantic_io_checks.hpp"
#include "code_fragment_adapter.hpp"
#include "match_turbo.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include "process_bridge.hpp"
#include <chrono>
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
namespace attributes=cm::aot::actor_attributes;
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
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Actor attributes await native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
struct Capture:ActorAttributesBoundaryObserver {
    std::array<std::unique_ptr<State>,attributes::entries.size()> inputs;
    std::unique_ptr<State> periodic_candidate;
    bool periodic_updated=false;
    void before_step(State&,std::uint32_t pc)override { if(pc==0x10239574)periodic_updated=true; }
    void enter(const State& s,std::uint32_t root,bool original)override {
        if(original)return;
        const auto entry=std::find(attributes::entries.begin(),attributes::entries.end(),root);
        require(entry!=attributes::entries.end(),"Unknown actor-attribute capture");
        auto& input=inputs[unsigned(entry-attributes::entries.begin())];
        if(input)return;
        if(root==attributes::periodic_entry) {
            periodic_candidate=clone(s);periodic_candidate->platform=s.platform;
            periodic_candidate->wr<4>(4128,return_sentinel);periodic_updated=false;return;
        }
        input=clone(s);input->platform=s.platform;input->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t entry,bool,ActorAttributesOutcome result)override {
        if(entry==attributes::periodic_entry && periodic_candidate) {
            if(periodic_updated && result==ActorAttributesOutcome::returned) inputs[3]=std::move(periodic_candidate);
            else periodic_candidate.reset();
        }
    }
};
struct Negative:ActorAttributesBoundaryObserver {
    unsigned index;
    explicit Negative(unsigned value):index(value){}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,ActorAttributesOutcome result)override {
        if(entry==attributes::entries[index] && !original && result==ActorAttributesOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);
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
void compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false) {
    auto a=clone(input),b=clone(input);a->platform=b->platform=input.platform;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=attributes::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    actor_attributes_diagnostics(*a).original=true;auto& d=actor_attributes_diagnostics(*b);
    if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(index);if(negative)d.observer=&mutation;
    Clock old_clock,new_clock;new_clock.base=old_clock.base;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index),actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production actor-attribute entry="+hex_address(attributes::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected boundary");
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,label+" host input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative failed");++negatives;}
    else {Image(*a).compare(*b,label);require(x.bytes==y.bytes && a->executed==b->executed,label+" ordered effects/accounting");}
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
void matrix(const State& input,unsigned index,bool profile) {
    for(auto path:{Path::direct,Path::generated,Path::cfm}){
        if(profile)compare(input,index,path,Mode::profile);
        else for(auto mode:{Mode::modern,Mode::original,Mode::entry_original})compare(input,index,path,mode);
    }
    if(!profile){
        compare(input,index,Path::generated,Mode::modern,50000000,true);
        for(auto limit:{0u,1u,2u,3u})compare(input,index,Path::generated,Mode::modern,limit);
        if(index==4)for(auto limit:{4u,14u,18u})compare(input,index,Path::generated,Mode::modern,limit);
        else if(index!=1)for(auto limit:{12u,13u,index==2?36u:100u})compare(input,index,Path::generated,Mode::modern,limit);
    }
}
}
int main(int argc,char** argv){try {
    require(argc==4 || argc==5,"Expected original program, game, fresh backing and optional profile directory");
    const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Profile request/flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    OwnedBacking cleanup{root};
    if(profile){
        for(unsigned index=0;index<attributes::entries.size();++index){
            State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
            load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(attributes::entries[index])+".snapshot"));
            require(input.pc==attributes::entries[index],"Snapshot is not an actual original-caller actor-attribute entry");matrix(input,index,true);
        }
    } else {
        auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto& world=*game.state;
        Capture capture;const auto calls=complete_match(world,game.match,capture);
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"actor-attributes-live-profile";
        std::filesystem::create_directories(snapshots);

        for(unsigned index=0;index<attributes::entries.size();++index){
            require(bool(capture.inputs[index]),"Missing original-caller actor-attribute capture "+hex_address(attributes::entries[index]));
            require(calls[index]>0,"Original-caller actor-attributes coverage/outcome mismatch");
            save_runtime_snapshot(*capture.inputs[index],snapshots/(hex_address(attributes::entries[index])+".snapshot"));
            matrix(*capture.inputs[index],index,false);
            if(index!=1)actual_fault(*capture.inputs[index],index);

            std::cout<<"Integrated actor-attribute source="<<"natural complete match"<<" entry="<<hex_address(attributes::entries[index])<<" calls="<<calls[index]<<" returns="<<calls[index]<<std::endl;
        }
        require(cases==82 && negatives==5 && budgets==32,"Real actor-attribute matrix incomplete");
    }
    std::cout<<"PASS: production actor-attributes cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives
        <<"; real match caller direct/generated/CFM, original/profile controls, dedicated productive counts, complete state/effects/accounting/import boundaries\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
