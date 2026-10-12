#include "player_movement_adapter.hpp"
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
namespace movement=cm::aot::player_movement;
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
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Movement awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
struct Capture:PlayerMovementBoundaryObserver {
    bool require_possession;std::unique_ptr<State> input;
    explicit Capture(bool owns):require_possession(owns){}
    void enter(const State& s,std::uint32_t root,bool original)override {
        if(input || root!=movement::entry || original)return;
        const auto actor=std::uint32_t(s.r<4>(12));const auto owner=s.memory.read<4>(actor+0x1a4);
        if(require_possession && s.memory.read<4>(owner+0xe000)!=actor)return;
        input=clone(s);input->platform=s.platform;input->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,PlayerMovementOutcome)override{}
};
struct Negative:PlayerMovementBoundaryObserver {
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t,bool original,PlayerMovementOutcome result)override {
        if(!original && result==PlayerMovementOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);
    }
};
unsigned cases=0,returns=0,budgets=0,negatives=0,captures=0,owned=0;
std::string execute(State& s,Path path) {
    try {
        if(path==Path::direct)try_modern_player_movement(s,movement::entry);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,movement::entry);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
void compare(const State& input,Path path,Mode mode,unsigned limit=50000000,bool negative=false) {
    auto a=clone(input),b=clone(input);a->platform=b->platform=input.platform;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=movement::entry;s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    player_movement_diagnostics(*a).original=true;auto& d=player_movement_diagnostics(*b);
    if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[0]=true;
    Negative mutation;if(negative)d.observer=&mutation;
    Clock old_clock,new_clock;new_clock.base=old_clock.base;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path),actual=execute(*b,path);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production movement case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected boundary");
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,label+" host input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative failed");++negatives;}
    else {Image(*a).compare(*b,label);require(x.bytes==y.bytes && a->executed==b->executed,label+" ordered effects/accounting");}
    require(!d.original_body && !d.dispatch_escapes,label+" own-body scope residue/dispatch escape");
    if(mode==Mode::modern){require(d.calls[0]>=1 && !d.original_calls[0] && !d.profile_original_calls[0],label+" no dedicated modern call");
        require(d.returns[0]==(expected.empty()?d.calls[0]:0) && d.budget_stops[0]==unsigned(!expected.empty()),label+" productive outcome counts");}
    else {require(!d.calls[0] && !d.returns[0],label+" original control entered C++ root");require(mode==Mode::profile?d.profile_original_calls[0]>=1:d.original_calls[0]>=1,label+" selected original/profile not reached");}
    ++cases;returns+=expected.empty();budgets+=!expected.empty();
}
void matrix(const State& input,bool profile) {
    for(auto path:{Path::direct,Path::generated,Path::cfm}){
        if(profile)compare(input,path,Mode::profile);
        else for(auto mode:{Mode::modern,Mode::original,Mode::entry_original})compare(input,path,mode);
    }
    if(!profile){compare(input,Path::generated,Mode::modern,50000000,true);compare(input,Path::generated,Mode::modern,0);compare(input,Path::generated,Mode::modern,1);}
}
}
int main(int argc,char** argv){try {
    require(argc==4 || argc==5,"Expected original program, game, fresh backing and optional profile snapshot");
    const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Profile request/flag mismatch");set_match_turbo_enabled(false);
    if(profile){State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],argv[3],AudioMode::offline);load_match_snapshot(input,argv[4]);require(input.pc==movement::entry,"Snapshot is not an actual movement entry");matrix(input,true);}
    else {
        auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],argv[3]},{677,5677,12345,2001,194});auto& world=*game.state;
        for(unsigned step=1;step<=120 && captures<6;++step){Capture capture(captures<3);player_movement_diagnostics(world).observer=&capture;
            world.wr<4>(12,game.match);world.wr<4>(16,1);world.wr<4>(4128,return_sentinel);world.pc=0x101d3f60;world.budget=world.executed+cm::aot::match_debugger::fresh_match_step_limit;
            run_until(world,return_sentinel);player_movement_diagnostics(world).observer=nullptr;
            if(world.r<4>(12)==2)break;if(!capture.input)continue;
            const auto actor=std::uint32_t(capture.input->r<4>(12));const auto owner=capture.input->memory.read<4>(actor+0x1a4);const bool owns=capture.input->memory.read<4>(owner+0xe000)==actor;owned+=owns;++captures;
            if(captures==1)save_runtime_snapshot(*capture.input,std::filesystem::path(argv[3]).parent_path()/"player-movement-live-profile.snapshot");
            matrix(*capture.input,false);std::cout<<"Actual integrated movement capture="<<captures<<" step="<<step<<" owns="<<owns<<" kind="<<capture.input->r<4>(24)<<std::endl;
        }
        require(captures==6 && owned>=3 && cases==72 && negatives==6 && budgets==12,"Integrated real movement matrix incomplete");
        const auto& d=player_movement_diagnostics(world);require(d.calls[0]>0 && d.returns[0]==d.calls[0] && !d.original_calls[0] && !d.faults[0] && !d.dispatch_escapes,"Fresh original tick did not use the complete C++ movement root");
        std::cout<<"Original tick productive movement calls="<<d.calls[0]<<" returns="<<d.returns[0]<<std::endl;
    }
    std::cout<<"PASS: production movement cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<"; actual direct/generated/CFM, original/profile controls, dedicated productive counts, complete state/effects/accounting/import boundaries\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
