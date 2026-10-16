#include "ball_contact_check_support.hpp"
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
namespace line=cm::aot::ball_contact;
using namespace cm::ball_contact_checks;
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
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Movement choice awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
struct Capture:Observer {
    std::array<std::unique_ptr<State>,6> inputs;
    void enter(const State& s,std::uint32_t root,bool original)override {
        if(original)return;
        const auto entry=std::find(line::entries.begin(),line::entries.end(),root);
        if(entry==line::entries.end())return;
        auto& input=inputs[unsigned(entry-line::entries.begin())];
        if(input)return;
        input=clone(s);input->platform=s.platform;input->wr<4>(4128,return_sentinel);
    }
};
struct Negative:Observer {
    unsigned index;
    explicit Negative(unsigned value):index(value){}
    void enter(const State&,std::uint32_t,bool)override{}
    void mutate(State& s,std::uint32_t entry,bool original,bool returned) {
        if(entry!=line::entries[index] || original || !returned)return;
        s.wr<4>(12,s.r<4>(12)^1);
    }
    void leave(State& s,std::uint32_t entry,bool original,BallContactOutcome result)override{mutate(s,entry,original,result==BallContactOutcome::returned);}
    void leave(State& s,std::uint32_t entry,bool original,MatchDisciplineOutcome result)override{mutate(s,entry,original,result==MatchDisciplineOutcome::returned);}
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
        if(path==Path::direct)invoke(s,index);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,line::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& error){return error.what();}
    return {};
}
void compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false) {
    auto a=clone(input),b=clone(input);a->platform=b->platform=input.platform;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=line::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;}
    original_block(*a);
    if(mode==Mode::original)original_block(*b);
    if(mode==Mode::entry_original)diagnostics(*b,index,[](auto& d,unsigned slot){d.entry_original[slot]=true;});
    Negative mutation(index);if(negative)observe(*b,&mutation);
    Clock old_clock,new_clock;new_clock.base=old_clock.base;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index),actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;observe(*b,nullptr);
    const auto label="Production ball-contact entry="+hex_address(line::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected boundary");
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,label+" host input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative failed");++negatives;}
    else {Image(*a).compare(*b,label);require(x.bytes==y.bytes && a->executed==b->executed,label+" ordered effects/accounting");}
    diagnostics(*b,index,[&](auto& d,unsigned slot){
    require(!d.original_body && !d.dispatch_escapes,label+" own-body scope residue/dispatch escape");
    if(mode==Mode::modern){require(d.calls[slot]>=1 && !d.original_calls[slot] && !d.profile_original_calls[slot],label+" no dedicated modern call");
        require(d.returns[slot]==(expected.empty()?d.calls[slot]:0) && d.budget_stops[slot]==unsigned(!expected.empty()),label+" productive outcome counts");}
    else {require(!d.calls[slot] && !d.returns[slot],label+" original control entered C++ root");require(mode==Mode::profile?d.profile_original_calls[slot]>=1:d.original_calls[slot]>=1,label+" selected original/profile not reached");}
    });
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
        for(unsigned index=0;index<line::entries.size();++index){
            State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
            load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(line::entries[index])+".snapshot"));
            require(input.pc==line::entries[index],"Snapshot is not an actual original-caller ball-contact entry");matrix(input,index,true);
        }
    } else {
        auto game=cm::aot::match_debugger::create_match({argv[1],argv[2],root},{2262,5677,12345,2001,194});auto& world=*game.state;
        auto original=clone(world);original->platform=world.platform;original_block(*original);
        Clock old_flow,new_flow;new_flow.base=old_flow.base;
        original->code_fragment_observer=&old_flow;world.code_fragment_observer=&new_flow;
        Capture capture;observe(world,&capture);
        unsigned advances=0;for(;advances<1400;++advances){
            for(auto* state:{original.get(),&world}) {
                state->wr<4>(12,game.match);state->wr<4>(16,1);state->wr<4>(4128,return_sentinel);state->pc=0x101d3f60;
                state->budget=state->executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(*state,return_sentinel);
            }
            require(original->r<4>(12)==world.r<4>(12) && original->executed==world.executed,
                "Original/C++23 ball-contact match progression and accounting");
            if(advances%50==0)Image(*original).compare(world,"Paired contact intermediate state tick="+std::to_string(advances));
            if(world.r<4>(12)==2){++advances;break;}
        }
        observe(world,nullptr);world.code_fragment_observer=nullptr;original->code_fragment_observer=nullptr;
        Image(*original).compare(world,"Original/C++23 ball-contact complete full-match state");
        require(old_flow.count==new_flow.count && old_flow.events==new_flow.events,"Original/C++23 ball-contact interval import ABI");
        require(advances<1400&&world.r<4>(12)==2,"Actual contact capture match reached full time");
        std::cout<<"PASS: actual Cruzeiro/Manchester United full match advances="<<advances<<" with all six contact roots original versus C++23; complete state, accounting and import boundaries\n";
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"ball-contact-live-profile";
        std::filesystem::create_directories(snapshots);

        for(unsigned index=0;index<line::entries.size();++index){
            require(bool(capture.inputs[index]),"Missing original-caller ball-contact capture "+hex_address(line::entries[index]));
            diagnostics(world,index,[&](auto& d,unsigned slot){
                require(d.calls[slot]>0&&d.calls[slot]==d.returns[slot]&&!d.original_calls[slot]&&!d.faults[slot]&&!d.budget_stops[slot]&&!d.dispatch_escapes,"Actual contact coverage/outcome mismatch");
                std::cout<<"Integrated contact entry="<<hex_address(line::entries[index])<<" calls="<<d.calls[slot]<<" returns="<<d.returns[slot]<<std::endl;
            });
            save_runtime_snapshot(*capture.inputs[index],snapshots/(hex_address(line::entries[index])+".snapshot"));
            matrix(*capture.inputs[index],index,false);
        }
        require(cases==72 && negatives==6 && budgets==12,"Real ball-contact matrix incomplete");
    }
    std::cout<<"PASS: production ball-contact cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives
        <<"; real match caller direct/generated/CFM, original/profile controls, dedicated productive counts, complete state/effects/accounting/import boundaries\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
