#include "contract_query_adapter.hpp"
#include "contract_storage_adapter.hpp"
#include "actor_attributes_io_checks.hpp"
#include "code_fragment_adapter.hpp"
#include "match_turbo.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include <chrono>
#include <fstream>
#include <set>
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
constexpr std::array roots{0x10082f20u,0x10082f90u,0x10083110u,0x10078e70u,0x10078f00u,0x103727c0u};
constexpr std::array ends{0x10082f84u,0x10083108u,0x10083178u,0x10078ef8u,0x10079048u,0x10374570u};
enum class Path {direct,generated,cfm};
enum class Mode {modern,original,entry_original,profile};
void original_contracts(State& s){contract_query_diagnostics(s).original=true;contract_storage_diagnostics(s).original=true;}
struct Clock:CodeFragmentBoundaryObserver {
    unsigned count=0;ActorAttributesIoPair* io=nullptr;bool modern=false;
    struct Event {bool leave;std::string name;decltype(State::registers) registers;std::uint32_t pc;bool operator==(const Event&)const=default;};
    std::vector<Event> events;
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool)override{}
    void import_enter(const State& s,const char* name)override{events.push_back({false,name,s.registers,s.pc});}
    void import_leave(const State& s,const char* name)override{events.push_back({true,name,s.registers,s.pc});}
    bool import_fixture(State& s,const char* name)override {
        if(io && io->dispatch(s,name,modern))return true;
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Contract preparation awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),1000000000ULL+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
std::array<std::set<std::uint32_t>,6> runtime_sites;
struct Witness:ContractQueryBoundaryObserver,ContractStorageBoundaryObserver {
    std::uint32_t negative_root=0;
    void enter(const State&,std::uint32_t,bool)override{}
    void finish(State& s,std::uint32_t root,bool original,bool returned){if(root==negative_root && !original && returned)s.wr<4>(12,s.r<4>(12)^1);}
    void leave(State& s,std::uint32_t root,bool original,ContractQueryOutcome result)override{finish(s,root,original,result==ContractQueryOutcome::returned);}
    void leave(State& s,std::uint32_t root,bool original,ContractStorageOutcome result)override{finish(s,root,original,result==ContractStorageOutcome::returned);}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed>=s.budget)return;for(unsigned n=0;n<roots.size();++n)if(pc>=roots[n] && pc<ends[n])runtime_sites[n].insert(pc);}
};
struct Counters {std::uint64_t calls,original,returns,faults,budgets,profile,escapes;std::uint32_t body;};
Counters counters(const State& s,unsigned index) {
    const auto read=[&](const auto& d,unsigned n){return Counters{d.calls[n],d.original_calls[n],d.returns[n],d.faults[n],d.budget_stops[n],d.profile_original_calls[n],d.dispatch_escapes,d.original_body};};
    return index<3?read(contract_query_diagnostics(s),index):read(contract_storage_diagnostics(s),index-3);
}
void select(State& s,unsigned index,Mode mode) {
    if(mode==Mode::original)original_contracts(s);
    if(mode==Mode::entry_original){if(index<3)contract_query_diagnostics(s).entry_original[index]=true;else contract_storage_diagnostics(s).entry_original[index-3]=true;}
}
void observer(State& s,Witness* w){contract_query_diagnostics(s).observer=w;contract_storage_diagnostics(s).observer=w;}
std::string execute(State& s,unsigned index,Path path) {
    try {
        if(path==Path::direct){if(index<3)try_modern_contract_query(s,roots[index]);else try_modern_contract_storage(s,roots[index]);}
        else {if(path==Path::cfm){s.memory.write<4>(fixture+0x400,roots[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}run_until(s,return_sentinel);}
    }catch(const std::exception& e){return e.what();}return {};
}
unsigned cases=0,returns=0,budgets=0,negatives=0,faults=0;
void between(ActorAttributesIoPair& io){for(const auto& [guest,native]:io.old_forks.references){(void)guest;require(io.old_forks.forks.close(native)==0,"Release original contract native fork");}io.old_forks.references.clear();io.between();}
std::uint64_t compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,bool fault=false) {
    Clock probe;
    ActorAttributesIoPair io(input,index,roots[index],[&](State& s){original_contracts(s);s.code_fragment_observer=&probe;});
    auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->memory.map(fixture,0x1000);s->pc=roots[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;if(fault)s->wr<4>(index<3?16:12,0xf0000000);}
    original_contracts(*a);select(*b,index,mode);Witness witness;if(negative)witness.negative_root=roots[index];if(mode==Mode::modern)observer(*b,&witness);
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,index,path);a->platform.reset();between(io);const auto actual=execute(*b,index,path);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;observer(*b,nullptr);
    const auto label="Contract runtime root="+hex_address(roots[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(fault?expected.starts_with("Unmapped data access at "):limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected outcome: "+expected);
    require(old_clock.count==new_clock.count && old_clock.events==new_clock.events,label+" host-input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative not discriminating");++negatives;}
    else {Image(*a).compare(*b,label);require(x.bytes==y.bytes && a->executed==b->executed,label+" ordered effects/accounting");}
    io.compare();const auto d=counters(*b,index);require(!d.body && !d.escapes,label+" leaked original scope or dispatch escape");
    require(!contract_query_diagnostics(*b).original_body && !contract_storage_diagnostics(*b).original_body,label+" nested original scope residue");
    if(mode==Mode::modern){require(d.calls>=1 && !d.original && !d.profile,label+" dedicated modern call");require(d.returns==(expected.empty()?d.calls:0) && d.faults==unsigned(fault) && d.budgets==unsigned(!fault&&!expected.empty()),label+" productive outcome counters");}
    else {require(!d.calls && !d.returns,label+" original control entered C++");require(mode==Mode::profile?d.profile>=1:d.original>=1,label+" original control not reached");}
    ++cases;returns+=expected.empty();budgets+=!fault&&!expected.empty();faults+=fault;return a->executed-input.executed;
}
void matrix(const State& input,unsigned index,bool profile) {
    if(profile){compare(input,index,Path::generated,Mode::profile);compare(input,index,Path::cfm,Mode::profile);return;}
    std::uint64_t steps=0;for(auto path:{Path::direct,Path::generated,Path::cfm}){const auto measured=compare(input,index,path,Mode::modern);if(path==Path::generated)steps=measured;}
    compare(input,index,Path::generated,Mode::original);compare(input,index,Path::generated,Mode::entry_original);compare(input,index,Path::generated,Mode::modern,50000000,true);
    require(steps>=2,"Measured contract path for focused budget cuts");for(auto cut:std::set<std::uint64_t>{0,steps/2,steps-1})compare(input,index,Path::generated,Mode::modern,unsigned(cut));
    compare(input,index,Path::generated,Mode::modern,50000000,false,true);
    std::cout<<"PASS: contract runtime root="<<hex_address(roots[index])<<" sites="<<runtime_sites[index].size()<<std::endl;
}
std::uint32_t setup(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args) {
    auto& q=contract_query_diagnostics(s).original;auto& c=contract_storage_diagnostics(s).original;
    struct Restore {bool& q;bool& c;bool oldq,oldc;~Restore(){q=oldq;c=oldc;}} restore{q,c,q,c};q=c=true;
    unsigned n=3;for(auto v:args)s.wr<4>(n++*4,v);s.pc=entry;s.wr<4>(4128,return_sentinel);s.budget=s.executed+50000000;
    run_until(s,return_sentinel);return std::uint32_t(s.r<4>(12));
}
struct OwnedBacking {std::filesystem::path path;~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(path,e)||e||std::filesystem::is_symlink(path,e)||e)return;for(const auto& p:std::filesystem::recursive_directory_iterator(path,e))if(e||p.is_symlink(e)||e)return;std::filesystem::remove_all(path,e);}};
}
int main(int argc,char** argv){try {
    require(argc==5 || (argc==6 && std::string_view(argv[5])=="--profile"),"Expected program, game, private backing root and original prepared snapshot, optionally --profile");
    const bool profile=argc==6;require(profile==match_pc_profile_enabled(),"Contract profile flag mismatch");set_match_turbo_enabled(false);
    const auto directory=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(directory),"New private contract backing");OwnedBacking cleanup{directory};
    const auto snapshots=std::filesystem::path(argv[4]).parent_path();std::ifstream pointer(snapshots/"prepared-backing.txt");std::string name;std::getline(pointer,name);require(std::filesystem::path(name).parent_path().empty() && name.starts_with("prepared-backing-"),"Retained original backing name");ActorAttributesIoPair::copy(snapshots/name,directory);
    State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],directory,AudioMode::offline);load_match_snapshot(source,argv[4]);Clock clock;source.code_fragment_observer=&clock;
    const auto toc=std::uint32_t(source.r<4>(8)),count=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be4))),staff_base=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be0)));
    std::uint32_t player=0,other_club=0;for(unsigned n=0;n<count;++n){const auto staff=staff_base+n*0x70,club=std::uint32_t(source.memory.read<4>(staff+0x24));if(club && source.memory.read<4>(staff+0x14)){if(!player)player=staff;else if(club!=source.memory.read<4>(player+0x24)){other_club=club;break;}}}
    require(count==153796 && player && other_club,"Actual full staff population and club players");
    const auto club=std::uint32_t(source.memory.read<4>(player+0x24)),manager=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7b7c)));require(manager!=0,"Original contract manager");
    const auto value=setup(source,0x10084be0,{manager,player,club,1,0xffffffff,0});
    if(!setup(source,roots[0],{manager,player}))setup(source,roots[3],{manager,player,value});
    const auto record=setup(source,roots[0],{manager,player});require(record && source.memory.read<4>(record+4)==source.memory.read<4>(club),"Original actual-player primary contract binding");
    source.memory.map(fixture,0x1000);
    const auto check=[&](const State& base,unsigned index,std::initializer_list<std::uint32_t> args){auto s=clone(base);s->platform=base.platform;unsigned n=3;for(auto v:args)s->wr<4>(n++*4,v);matrix(*s,index,profile);};
    std::cout<<"Prepared original contract world staff="<<count<<" player="<<source.memory.read<4>(player)<<" used="<<source.memory.read<4>(manager+8)<<" capacity="<<source.memory.read<4>(manager+4)<<std::endl;
    check(source,0,{manager,player});check(source,1,{manager,player,club});check(source,2,{manager,player});
    check(source,3,{manager,player,value});check(source,4,{manager,1});check(source,5,{record,player,value});
    auto secondary=clone(source);secondary->platform=source.platform;const auto statistics=std::uint32_t(secondary->memory.read<4>(secondary->memory.read<4>(toc-0x7b9c))),offset=std::uint32_t(secondary->memory.read<4>(player))*100;
    secondary->memory.write<4>(statistics+offset+4,secondary->memory.read<4>(statistics+offset));check(*secondary,2,{manager,player});check(*secondary,1,{manager,player,other_club});
    auto grow=clone(source);grow->platform=source.platform;grow->memory.write<4>(manager+4,grow->memory.read<4>(manager+8));check(*grow,4,{manager,1});check(*grow,3,{manager,player,value});
    auto reuse=clone(source);reuse->platform=source.platform;reuse->memory.write<4>(record,0xffffffff);check(*reuse,4,{manager,0});
    check(source,5,{record,player,0xffffffff});
    if(!profile){std::ofstream out(std::filesystem::path(argv[3]).parent_path()/"contract-runtime-sites.txt");for(unsigned n=0;n<roots.size();++n){require(!runtime_sites[n].empty(),"Every contract root has actual-data runtime sites");for(auto pc:runtime_sites[n])out<<n<<" "<<hex_address(pc)<<'\n';}}
    std::cout<<"PASS: contract production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
