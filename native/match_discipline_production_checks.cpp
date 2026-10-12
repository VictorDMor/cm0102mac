#include "match_discipline_adapter.hpp"
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
namespace discipline=cm::aot::match_discipline;
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
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Discipline awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
struct Capture:MatchDisciplineBoundaryObserver {
    std::array<std::unique_ptr<State>,6> inputs;
    void enter(const State& s,std::uint32_t root,bool original)override {
        if(original)return;
        const auto entry=std::find(discipline::entries.begin(),discipline::entries.end(),root);
        require(entry!=discipline::entries.end(),"Unknown discipline capture");
        auto& input=inputs[unsigned(entry-discipline::entries.begin())];
        if(input)return;
        input=clone(s);input->platform=s.platform;input->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,MatchDisciplineOutcome)override{}
};
struct Negative:MatchDisciplineBoundaryObserver {
    unsigned index;
    explicit Negative(unsigned value):index(value){}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,MatchDisciplineOutcome result)override {
        if(entry==discipline::entries[index] && !original && result==MatchDisciplineOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);
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
        if(path==Path::direct)try_modern_match_discipline(s,discipline::entries[index]);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,discipline::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
void compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false) {
    auto a=clone(input),b=clone(input);a->platform=b->platform=input.platform;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=discipline::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    match_discipline_diagnostics(*a).original=true;auto& d=match_discipline_diagnostics(*b);
    if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;
    Negative mutation(index);if(negative)d.observer=&mutation;
    Clock old_clock,new_clock;new_clock.base=old_clock.base;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index),actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production discipline entry="+hex_address(discipline::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
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
void matrix(const State& input,unsigned index,bool profile) {
    for(auto path:{Path::direct,Path::generated,Path::cfm}){
        if(profile)compare(input,index,path,Mode::profile);
        else for(auto mode:{Mode::modern,Mode::original,Mode::entry_original})compare(input,index,path,mode);
    }
    if(!profile){compare(input,index,Path::generated,Mode::modern,50000000,true);compare(input,index,Path::generated,Mode::modern,0);compare(input,index,Path::generated,Mode::modern,1);}
}
}
int main(int argc,char** argv){try {
    require(argc==4 || argc==5,"Expected original program, game, fresh backing and optional profile directory");
    const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Profile request/flag mismatch");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    OwnedBacking cleanup{root};
    if(profile){
        for(unsigned index=0;index<6;++index){
            State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
            load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(discipline::entries[index])+".snapshot"));
            require(input.pc==discipline::entries[index],"Snapshot is not an actual original-caller discipline entry");matrix(input,index,true);
        }
    } else {
        auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto& world=*game.state;
        Capture capture; match_discipline_diagnostics(world).observer=&capture;
        unsigned step=0;
        while(step++<1200){
            world.wr<4>(12,game.match);world.wr<4>(16,1);world.wr<4>(4128,return_sentinel);world.pc=0x101d3f60;
            world.budget=world.executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(world,return_sentinel);
            if((capture.inputs[0] && capture.inputs[1] && capture.inputs[3] && capture.inputs[4]) || world.r<4>(12)==2)break;
        }
        match_discipline_diagnostics(world).observer=nullptr;
        // Referee response is a separate incident caller. Exercise that whole
        // original caller once using a real match actor, then capture its actual
        // facade/core arguments. This is directed integration evidence, distinct
        // from incidents naturally encountered by the bounded match tick run.
        std::unique_ptr<State> incident;
        if(!capture.inputs[2] || !capture.inputs[5]) {
            incident=clone(world); incident->platform=world.platform;
            const auto actor=game.match+0x311c+2*0x1c4;
            require(incident->memory.read<4>(actor+0x1a4)==game.match,"Incident actor belongs to the actual match");
            incident->wr<4>(12,actor);incident->wr<4>(4128,return_sentinel);incident->pc=0x10288230;
            incident->budget=incident->executed+cm::aot::match_debugger::fresh_match_step_limit;
            match_discipline_diagnostics(*incident).observer=&capture;
            run_until(*incident,return_sentinel);
            match_discipline_diagnostics(*incident).observer=nullptr;
        }

        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"match-discipline-live-profile";
        std::filesystem::create_directories(snapshots);

        for(unsigned index=0;index<6;++index){
            require(bool(capture.inputs[index]),"Missing original-caller discipline capture "+hex_address(discipline::entries[index]));
            const bool directed=incident && (index==2 || index==5);
            const auto& diagnostics=match_discipline_diagnostics(directed?*incident:world);
            require(diagnostics.calls[index]>0 && diagnostics.calls[index]==diagnostics.returns[index] && !diagnostics.original_calls[index] &&
                !diagnostics.faults[index] && !diagnostics.budget_stops[index] && !diagnostics.dispatch_escapes,"Original-caller discipline coverage/outcome mismatch");
            save_runtime_snapshot(*capture.inputs[index],snapshots/(hex_address(discipline::entries[index])+".snapshot"));
            matrix(*capture.inputs[index],index,false);
            std::cout<<"Integrated discipline source="<<(directed?"original incident caller":"natural match tick")<<" entry="<<hex_address(discipline::entries[index])<<" calls="<<diagnostics.calls[index]<<" returns="<<diagnostics.returns[index]<<std::endl;
        }
        require(cases==72 && negatives==6 && budgets==12,"Real discipline matrix incomplete");
    }
    std::cout<<"PASS: production discipline cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives
        <<"; real match/original incident caller direct/generated/CFM, original/profile controls, dedicated productive counts, complete state/effects/accounting/import boundaries\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
