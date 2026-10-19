#include "actor_policy_adapter.hpp"
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
namespace action=cm::aot::actor_policy;
enum class Path { direct, generated, cfm };
enum class Mode { modern, original, entry_original, profile };
constexpr std::array<std::uint32_t,5> ends{0x101f5348,0x10285790,0x102857b4,0x10285ec4,0x102860c0};
const std::array<std::vector<std::uint32_t>,5> original_callsites{{
    {0x10281c8c,0x102837c8,0x10283ef8,0x1028501c,0x1028522c},
    {0x101d78a8,0x101d78c0},
    {0x1027baa4,0x1027bf98,0x1027fcd0,0x1027fde4,0x102808e8,0x10280c24,0x10280e48,0x10280eec,0x102812d4},
    {0x10278d54,0x102824d4,0x10284d10,0x10284d44,0x10284d78},
    {0x101d4e44,0x101d7144,0x101fec04,0x101fecb0}}};
struct Clock:CodeFragmentBoundaryObserver {
    std::uint64_t base=native_microseconds();unsigned count=0;
    struct Event {bool leave;std::string name;decltype(State::registers) registers;std::uint32_t pc;bool operator==(const Event&)const=default;};
    std::vector<Event> events;
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool)override{}
    void import_enter(const State& s,const char* name)override{events.push_back({false,name,s.registers,s.pc});}
    void import_leave(const State& s,const char* name)override{events.push_back({true,name,s.registers,s.pc});}
    bool import_fixture(State& s,const char* name)override {
        if(std::string_view(name)=="CarbonLib::WaitNextEvent"||std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Actor-action fixture awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),base+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
struct Capture:ActorPolicyBoundaryObserver {
    std::array<std::uint32_t,5> callers{};
    std::array<std::unique_ptr<State>,5> inputs;
    void enter(const State& s,std::uint32_t root,bool original)override {
        require(original,"Natural capture must execute original actor-policy bodies");
        const auto found=std::find(action::entries.begin(),action::entries.end(),root);
        require(found!=action::entries.end(),"Unknown actor-policy capture");
        const auto index=unsigned(found-action::entries.begin());if(inputs[index])return;
        callers[index]=std::uint32_t(s.r<4>(4128))-4;
        require(std::find(original_callsites[index].begin(),original_callsites[index].end(),callers[index])!=original_callsites[index].end(),"Captured actor-policy root lacks indexed original BL caller");
        inputs[index]=clone(s);inputs[index]->platform=s.platform;inputs[index]->wr<4>(4128,return_sentinel);
    }
    void leave(State&,std::uint32_t,bool,ActorPolicyOutcome)override{}
};
struct Trace:ActorPolicyBoundaryObserver {
    unsigned index;bool negative=false;std::set<std::uint32_t> seen;
    std::array<unsigned,5> returned{};
    explicit Trace(unsigned value):index(value){}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget)seen.insert(pc);}
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t entry,bool original,ActorPolicyOutcome result)override{
        if(original||result!=ActorPolicyOutcome::returned)return;
        const auto found=std::find(action::entries.begin(),action::entries.end(),entry);
        require(found!=action::entries.end(),"Unknown actor-policy return");++returned[unsigned(found-action::entries.begin())];
        if(negative&&entry==action::entries[index])s.wr<4>(12,s.r<4>(12)^1);
    }
};
std::array<std::set<std::uint32_t>,5> complete_sites;
std::array<std::array<unsigned,5>,5> owned_returns{};
unsigned cases=0,returns=0,budgets=0,negatives=0,faults=0;
std::string execute(State& s,Path path,unsigned index) {
    try {
        if(path==Path::direct)try_modern_actor_policy(s,action::entries[index]);
        else {
            if(path==Path::cfm){s.memory.write<4>(fixture+0x400,action::entries[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}
            run_until(s,return_sentinel);
        }
    }catch(const std::exception& e){return e.what();}
    return {};
}
void compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,bool fault=false) {
    auto a=clone(input),b=clone(input);a->platform=b->platform=input.platform;
    for(auto* s:{a.get(),b.get()}){
        s->memory.map(fixture,0x1000);s->pc=action::entries[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;
        if(fault)s->wr<4>(12,0);
    }
    actor_policy_diagnostics(*a).original=true;auto& d=actor_policy_diagnostics(*b);
    if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;
    Trace trace(index);trace.negative=negative;if(mode==Mode::modern)d.observer=&trace;
    Clock old_clock,new_clock;new_clock.base=old_clock.base;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,path,index),actual=execute(*b,path,index);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;d.observer=nullptr;
    const auto label="Production actor-policy entry="+hex_address(action::entries[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(fault?expected.starts_with("Unmapped data access at "):limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected boundary="+expected);
    require(old_clock.count==new_clock.count&&old_clock.events==new_clock.events,label+" host input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(trace.returned[index]>0&&rejected,label+" negative failed");++negatives;}
    else {Image(*a).compare(*b,label);require(x.bytes==y.bytes&&a->executed==b->executed,label+" ordered effects/accounting");}
    require(!d.original_body&&!d.dispatch_escapes,label+" own-body scope residue/dispatch escape");
    if(mode==Mode::modern){
        require(d.calls[index]>0&&!d.original_calls[index]&&!d.profile_original_calls[index],label+" missing modern entry");
        for(unsigned n=0;n<5;++n)require(d.calls[n]==d.returns[n]+d.budget_stops[n]+d.faults[n],label+" incomplete outcomes");
        if(expected.empty())require(d.calls[index]==d.returns[index],label+" nonproductive return");
        else require(fault?d.faults[index]>0:d.budget_stops[index]>0,label+" missing interruption");
    }else {
        require(!d.calls[index]&&!d.returns[index],label+" original control entered native root");
        require(mode==Mode::profile?d.profile_original_calls[index]>0:d.original_calls[index]>0,label+" original/profile not reached");
    }
    if(mode==Mode::modern&&expected.empty()&&!negative){
        for(auto pc:trace.seen)for(unsigned n=0;n<5;++n)if(action::entries[n]<=pc&&pc<ends[n])complete_sites[n].insert(pc);
        for(unsigned n=0;n<5;++n){require(d.calls[n]==d.returns[n]&&!d.original_calls[n]&&!d.profile_original_calls[n],label+" own child return");owned_returns[index][n]+=d.returns[n];}
    }
    ++cases;returns+=expected.empty();budgets+=!fault&&!expected.empty();faults+=fault;
    std::cout<<label<<" complete instructions="<<a->executed-input.executed<<" boundary="<<expected<<std::endl;
}
void matrix(const State& input,unsigned index,bool profile) {
    for(auto path:{Path::direct,Path::generated,Path::cfm}){
        if(profile)compare(input,index,path,Mode::profile);
        else for(auto mode:{Mode::modern,Mode::original,Mode::entry_original})compare(input,index,path,mode);
    }
    if(!profile){compare(input,index,Path::generated,Mode::modern,50000000,true);for(unsigned n=0;n<3;++n)compare(input,index,Path::generated,Mode::modern,n);compare(input,index,Path::generated,Mode::modern,50000000,false,true);}
}
}
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
#define COMMON_DIGEST_FOR_OPENSSL
#include <CommonCrypto/CommonDigest.h>
#include <cstring>

namespace {
// Same canonical ordered memory event stream as Effects, bounded in memory.
struct DigestEffects final:MemoryObserver {
    CC_SHA256_CTX context{};std::array<std::uint8_t,65536> buffer{};std::size_t used=0;std::uint64_t events=0,bytes=0;
    DigestEffects(){CC_SHA256_Init(&context);}
    void append(const std::uint8_t* data,std::size_t size){bytes+=size;while(size){const auto n=std::min(size,buffer.size()-used);std::memcpy(buffer.data()+used,data,n);used+=n;data+=n;size-=n;if(used==buffer.size()){CC_SHA256_Update(&context,buffer.data(),CC_LONG(used));used=0;}}}
    void event(unsigned kind,std::uint32_t address,std::uint64_t size,const std::uint8_t* data=nullptr){++events;std::array<std::uint8_t,13> head{};head[0]=std::uint8_t(kind);for(unsigned n=0;n<4;++n)head[1+n]=std::uint8_t(address>>((3-n)*8));for(unsigned n=0;n<8;++n)head[5+n]=std::uint8_t(size>>((7-n)*8));append(head.data(),head.size());if(data)append(data,std::size_t(size));}
    void mapped(std::uint32_t a,std::uint64_t n)override{event(1,a,n);}
    void written(std::uint32_t a,const std::uint8_t* d,std::size_t n)override{event(2,a,n,d);}
    void cleared(std::uint32_t a,std::uint64_t n)override{event(3,a,n);}
    void read(std::uint32_t a,const std::uint8_t* d,unsigned n)override{event(4,a,n,d);}
    auto digest()const{auto copy=context;CC_SHA256_Update(&copy,buffer.data(),CC_LONG(used));std::array<std::uint8_t,CC_SHA256_DIGEST_LENGTH> out{};CC_SHA256_Final(out.data(),&copy);return out;}
};
}
int run_actor_policy_population(int argc,char**argv){try {
    require(argc==4&&!match_pc_profile_enabled(),"Original program, game, fresh private backing; profile disabled");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    auto game=match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});
    auto a=std::move(game.state),b=clone(*a);b->platform=a->platform;actor_policy_diagnostics(*a).original=true;
    Trace trace(0);actor_policy_diagnostics(*b).observer=&trace;
    const auto before=a->executed;std::uint64_t events=0,bytes=0;
    Clock original_clock,native_clock;native_clock.base=original_clock.base;
    a->code_fragment_observer=&original_clock;b->code_fragment_observer=&native_clock;
    for(unsigned step=0;step<600;++step){
        DigestEffects x,y;a->memory.date_observer=&x;b->memory.date_observer=&y;
        for(auto* s:{a.get(),b.get()}){s->wr<4>(12,game.match);s->wr<4>(16,1);s->wr<4>(4128,return_sentinel);s->pc=0x101d3f60;s->budget=s->executed+match_debugger::fresh_match_step_limit;run_until(*s,return_sentinel);}
        a->memory.date_observer=b->memory.date_observer=nullptr;
        const auto label="Continuous actual tick="+std::to_string(step);
        require(a->registers==b->registers&&a->unique==b->unique&&a->pc==b->pc&&a->executed==b->executed,label+" architectural/accounting mismatch");
        require(a->heap_end==b->heap_end&&a->native_calls==b->native_calls&&a->memory_error==b->memory_error&&a->allocated==b->allocated&&a->available==b->available,label+" allocation/import mismatch");
        require(x.events==y.events&&x.bytes==y.bytes&&x.digest()==y.digest(),label+" ordered memory mismatch");
        require(original_clock.count==native_clock.count&&original_clock.events==native_clock.events,label+" import boundary/input mismatch");
        original_clock.events.clear();native_clock.events.clear();events+=x.events;bytes+=x.bytes;
        if((step+1)%100==0)std::cout<<"Compared "<<step+1<<" continuous actual ticks instructions="<<a->executed-before<<" memory_events="<<events<<std::endl;
    }
    a->code_fragment_observer=b->code_fragment_observer=nullptr;actor_policy_diagnostics(*b).observer=nullptr;
    Image(*a).compare(*b,"Complete continuous population final state");
    const auto& d=actor_policy_diagnostics(*b);const auto& old=actor_policy_diagnostics(*a);
    std::ofstream sites(std::filesystem::path(argv[3]).parent_path()/"population-actual-sites.txt");
    for(unsigned n=0;n<5;++n){
        require(d.calls[n]>0&&d.calls[n]==d.returns[n]&&!d.original_calls[n]&&!d.profile_original_calls[n]&&!d.faults[n]&&!d.budget_stops[n]&&!d.dispatch_escapes,"Productive continuous own native calls");
        require(old.original_calls[n]>0&&!old.calls[n]&&!old.dispatch_escapes,"Original actor-policy continuous control");
        unsigned count=0;for(auto pc:trace.seen)if(action::entries[n]<=pc&&pc<ends[n]){sites<<n<<" "<<hex_address(pc)<<'\n';++count;}
        std::cout<<"Natural root="<<hex_address(action::entries[n])<<" native_returns="<<d.returns[n]<<" original_boundary_calls="<<old.original_calls[n]<<" complete_actual_sites="<<count<<std::endl;
    }
    std::cout<<"PASS: 600 consecutive actual ticks; complete final state, each-tick architectural/import/accounting equality and SHA256 ordered memory equality; events="<<events<<" bytes="<<bytes<<" instructions="<<a->executed-before<<"; integrated actor-policy population\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}

#pragma clang diagnostic pop

int main(int argc,char**argv){try {
    if(argc==5&&std::string_view(argv[4])=="--population")return run_actor_policy_population(4,argv);
    require(argc==4||argc==5,"Expected original program, game, private backing and optional snapshots");
    const bool profile=argc==5;require(profile==match_pc_profile_enabled(),"Profile request/flag mismatch");set_match_turbo_enabled(false);
    // Fresh, owned backing is retained as private evidence; never reuse it.
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    if(profile){
        for(unsigned index=0;index<5;++index){
            State input;load_program(input,argv[1]);input.platform=std::make_shared<NativePlatform>(argv[2],root,AudioMode::offline);
            load_match_snapshot(input,std::filesystem::path(argv[4])/(hex_address(action::entries[index])+".snapshot"));
            require(input.pc==action::entries[index],"Wrong captured actor-policy root");matrix(input,index,true);
        }
        require(cases==15&&returns==15&&!budgets&&!faults&&!negatives,"Five-root profile matrix");
    }else {
        auto game=match_debugger::create_match({argv[1],argv[2],root},{677,5677,12345,2001,194});auto& world=*game.state;
        Capture capture;auto& natural=actor_policy_diagnostics(world);natural.original=true;natural.observer=&capture;
        for(unsigned step=0;step<600;++step){world.wr<4>(12,game.match);world.wr<4>(16,1);world.wr<4>(4128,return_sentinel);world.pc=0x101d3f60;world.budget=world.executed+match_debugger::fresh_match_step_limit;run_until(world,return_sentinel);}
        natural.observer=nullptr;
        const auto snapshots=std::filesystem::path(argv[3]).parent_path()/"original958-snapshots";std::filesystem::create_directories(snapshots);
        for(unsigned index=0;index<5;++index){
            require(capture.inputs[index]&&natural.original_calls[index]>0&&!natural.calls[index]&&!natural.dispatch_escapes,"Missing original actor-policy caller capture");
            save_runtime_snapshot(*capture.inputs[index],snapshots/(hex_address(action::entries[index])+".snapshot"));
            std::cout<<"Captured original entry="<<hex_address(action::entries[index])<<" observed_boundary_calls="<<natural.original_calls[index]<<" caller="<<hex_address(capture.callers[index])<<std::endl;
            matrix(*capture.inputs[index],index,false);
        }
        require(cases==70&&returns==50&&negatives==5&&budgets==15&&faults==5,"Five-root actual-context experimental matrix");
        std::ofstream sites(std::filesystem::path(argv[3]).parent_path()/"complete-actual-sites.txt");
        for(unsigned n=0;n<5;++n){require(!complete_sites[n].empty(),"Missing actual own source sites");for(auto pc:complete_sites[n])sites<<n<<" "<<hex_address(pc)<<'\n';std::cout<<"Root "<<hex_address(action::entries[n])<<" complete_actual_sites="<<complete_sites[n].size()<<" own_returns=";for(auto count:owned_returns[n])std::cout<<count<<",";std::cout<<std::endl;}
    }
    std::cout<<"PASS: production actor-policy cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<"; actual services and complete state/effects/accounting/import boundaries; per-entry actual-service controls\n";
    return 0;
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
