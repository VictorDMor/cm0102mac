#include "contract_terms_adapter.hpp"
#include "contract_review_adapter.hpp"
#include "transfer_offer_adapter.hpp"
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
namespace terms=cm::aot::contract_terms;
constexpr auto roots=terms::entries;
constexpr std::array<std::uint32_t,21> ends{0x10374ae8,0x10374b34,0x10374b54,0x10374b74,0x10374b98,0x10374bd4,0x10374ca4,0x10374cd8,0x10374d04,0x10374d44,0x10374d74,0x10374f44,0x103802f4,0x103803ac,0x10380400,0x10380520,0x10380554,0x10380630,0x10380700,0x10380708,0x10380c04};
enum class Path {direct,generated,cfm};
enum class Mode {modern,original,entry_original,profile};
void original_contracts(State& s){contract_query_diagnostics(s).original=true;contract_storage_diagnostics(s).original=true;contract_terms_diagnostics(s).original=true;contract_review_diagnostics(s).original=true;transfer_offer_diagnostics(s).original=true;}
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

std::array<std::set<std::uint32_t>,roots.size()> runtime_sites;
struct Witness:ContractTermsBoundaryObserver {
    std::uint32_t negative_root=0;
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t root,bool original,ContractTermsOutcome result)override{if(root==negative_root&&!original&&result==ContractTermsOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed>=s.budget)return;for(unsigned n=0;n<roots.size();++n)if(pc>=roots[n]&&pc<ends[n])runtime_sites[n].insert(pc);}
};
void select(State& s,unsigned index,Mode mode){auto& d=contract_terms_diagnostics(s);if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;}
std::string execute(State& s,unsigned index,Path path){try{if(path==Path::direct)try_modern_contract_terms(s,roots[index]);else{if(path==Path::cfm){s.memory.write<4>(fixture+0x400,roots[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}run_until(s,return_sentinel);}}catch(const std::exception& e){return e.what();}return {};}
unsigned cases=0,returns=0,budgets=0,negatives=0,faults=0;
void between(ActorAttributesIoPair& io){for(const auto& [guest,native]:io.old_forks.references){(void)guest;require(io.old_forks.forks.close(native)==0,"Release original contract native fork");}io.old_forks.references.clear();io.between();}

std::uint64_t compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,bool fault=false){
    Clock probe;ActorAttributesIoPair io(input,index,roots[index],[&](State& s){original_contracts(s);s.code_fragment_observer=&probe;});
    auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->pc=roots[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;if(fault)s->wr<4>(index==0?8:index==6||index==7?20:index==12||index==13||index==15?16:12,0xf0000000);}
    original_contracts(*a);select(*b,index,mode);Witness witness;if(negative)witness.negative_root=roots[index];if(mode==Mode::modern)contract_terms_diagnostics(*b).observer=&witness;
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,index,path);a->platform.reset();between(io);const auto actual=execute(*b,index,path);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;contract_terms_diagnostics(*b).observer=nullptr;
    const auto label="Contract terms runtime root="+hex_address(roots[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(fault?expected.starts_with("Unmapped data access at "):limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected outcome: "+expected);
    require(old_clock.count==new_clock.count&&old_clock.events==new_clock.events,label+" host-input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative not discriminating");++negatives;}else{Image(*a).compare(*b,label);require(x.bytes==y.bytes&&a->executed==b->executed,label+" ordered effects/accounting");}
    io.compare();const auto& d=contract_terms_diagnostics(*b);require(!d.original_body&&!d.dispatch_escapes,label+" leaked original scope or dispatch escape");
    require(!contract_query_diagnostics(*b).original_body&&!contract_storage_diagnostics(*b).original_body,label+" nested original scope residue");
    if(mode==Mode::modern){require(d.calls[index]>=1&&!d.original_calls[index]&&!d.profile_original_calls[index],label+" dedicated modern call");require(d.returns[index]+d.faults[index]+d.budget_stops[index]==d.calls[index],label+" balanced productive outcomes");require(expected.empty()?d.returns[index]==d.calls[index]:fault?d.faults[index]>=1&&!d.budget_stops[index]:d.budget_stops[index]>=1&&!d.faults[index],label+" productive outcome counters");}
    else{require(!d.calls[index]&&!d.returns[index],label+" original control entered C++");require(mode==Mode::profile?d.profile_original_calls[index]>=1:d.original_calls[index]>=1,label+" original control not reached");}
    ++cases;returns+=expected.empty();budgets+=!fault&&!expected.empty();faults+=fault;return a->executed-input.executed;
}
void matrix(const State& input,unsigned index,bool profile){
    if(profile){compare(input,index,Path::generated,Mode::profile);compare(input,index,Path::cfm,Mode::profile);return;}
    std::uint64_t steps=0;for(auto path:{Path::direct,Path::generated,Path::cfm}){const auto measured=compare(input,index,path,Mode::modern);if(path==Path::generated)steps=measured;}
    compare(input,index,Path::generated,Mode::original);compare(input,index,Path::generated,Mode::entry_original);compare(input,index,Path::generated,Mode::modern,50000000,true);
    require(steps>=2,"Measured contract terms path for focused budget cuts");for(auto cut:std::set<std::uint64_t>{0,steps/2,steps-1})compare(input,index,Path::generated,Mode::modern,unsigned(cut));
    compare(input,index,Path::generated,Mode::modern,50000000,false,true);
    std::cout<<"PASS: contract terms runtime root="<<hex_address(roots[index])<<" sites="<<runtime_sites[index].size()<<std::endl;
}
std::uint32_t setup(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args){auto& q=contract_query_diagnostics(s).original;auto& c=contract_storage_diagnostics(s).original;auto& t=contract_terms_diagnostics(s).original;auto& v=contract_review_diagnostics(s).original;auto& offers=transfer_offer_diagnostics(s).original;struct Restore{bool& q;bool& c;bool& t;bool& v;bool& offers;bool a,b,d,e,f;~Restore(){q=a;c=b;t=d;v=e;offers=f;}}restore{q,c,t,v,offers,q,c,t,v,offers};q=c=t=v=offers=true;unsigned n=3;for(auto v:args)s.wr<4>(n++*4,v);s.pc=entry;s.wr<4>(4128,return_sentinel);s.budget=s.executed+50000000;run_until(s,return_sentinel);return std::uint32_t(s.r<4>(12));}
struct OwnedBacking {std::filesystem::path path;~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(path,e)||e||std::filesystem::is_symlink(path,e)||e)return;for(const auto& p:std::filesystem::recursive_directory_iterator(path,e))if(e||p.is_symlink(e)||e)return;std::filesystem::remove_all(path,e);}};
}
int main(int argc,char** argv){try{
    require(argc==5||(argc==6&&std::string_view(argv[5])=="--profile"),"Expected export directory, game, private backing root and original prepared snapshot, optionally --profile");const bool profile=argc==6;require(profile==match_pc_profile_enabled(),"Contract terms profile flag mismatch");set_match_turbo_enabled(false);
    const auto directory=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(directory),"New private contract terms backing");OwnedBacking cleanup{directory};
    const auto snapshots=std::filesystem::path(argv[4]).parent_path();std::ifstream pointer(snapshots/"prepared-backing.txt");std::string name;std::getline(pointer,name);require(std::filesystem::path(name).parent_path().empty()&&name.starts_with("prepared-backing-"),"Retained original backing name");ActorAttributesIoPair::copy(snapshots/name,directory);
    State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],directory,AudioMode::offline);load_match_snapshot(source,argv[4]);Clock clock;source.code_fragment_observer=&clock;
    const auto toc=std::uint32_t(source.r<4>(8)),count=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be4))),staff_base=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be0)));
    std::uint32_t player=0;for(unsigned n=0;n<count;++n){const auto staff=staff_base+n*0x70;if(source.memory.read<4>(staff+0x24)&&source.memory.read<4>(staff+0x14)){player=staff;break;}}
    require(count==153796&&player,"Actual full staff population and club player");const auto club=std::uint32_t(source.memory.read<4>(player+0x24)),manager=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7b7c)));require(manager!=0,"Original contract manager");
    const auto value=setup(source,0x10084be0,{manager,player,club,1,0xffffffff,0});if(!setup(source,0x10082f20,{manager,player}))setup(source,0x10078e70,{manager,player,value});const auto record=setup(source,0x10082f20,{manager,player});require(record&&source.memory.read<4>(record+4)==source.memory.read<4>(club),"Original actual-player primary contract binding");source.memory.map(fixture,0x10000);
    const auto check=[&](const State& base,unsigned index,std::initializer_list<std::uint32_t> args){auto s=clone(base);s->platform=base.platform;unsigned n=3;for(auto v:args)s->wr<4>(n++*4,v);matrix(*s,index,profile);};
    std::cout<<"Prepared original terms world staff="<<count<<" player="<<source.memory.read<4>(player)<<std::endl;
    for(unsigned n=0;n<roots.size();++n){
        if(n==0)check(source,n,{10});else if(n==6)check(source,n,{record,0,fixture+0x2000,0,0});else if(n==7)check(source,n,{record,1,fixture+0x2000});else if(n==11)check(source,n,{record,fixture+0x2000,1});else if(n==12||n==13)check(source,n,{0,fixture+0x2000});else if(n==14)check(source,n,{player,0});else if(n==15)check(source,n,{player,club,0});else check(source,n,{record,1,1234});
    }
    for(auto bound:{0u,65536u})check(source,0,{bound});
    for(auto index:{1u,2u,3u,4u,6u,7u})check(source,index,{record,index==2||index==4||index==6?2u:4u,index==2||index==4?1234u:fixture+0x2000,0,1});
    for(auto index:{12u,13u})for(auto type:{2u,4u,255u})check(source,index,{type,fixture+0x2000});
    check(source,11,{record,fixture+0x2000,0});check(source,14,{player,2});for(auto type:{1u,2u,3u,4u})check(source,15,{player,club,type});
    for(auto prefs:{0x400000u,0x800000u}){auto s=clone(source);s->platform=source.platform;const auto first=std::uint32_t(s->memory.read<4>(toc-0x7ac8)),settings=std::uint32_t(s->memory.read<4>(first));s->memory.write<4>(settings+4,prefs);check(*s,17,{record});check(*s,18,{record});}
    for(auto kind:{4u,12u}){auto s=clone(source);s->platform=source.platform;s->memory.write<1>(record+0x38,kind);check(*s,11,{record,fixture+0x2000,1});check(*s,20,{record});}
    for(auto slot:{-0x79b4,-0x79a8,-0x79ac,-0x79b0,-0x79a0,-0x79a4,-0x799c}){auto s=clone(source);s->platform=source.platform;const auto date=std::uint32_t(s->memory.read<4>(toc+slot));s->memory.copy_in(record+0x30,s->memory.copy_out(date,8));s->memory.write<1>(record+0x38,8);check(*s,20,{record});}
    if(!profile){std::ofstream out(std::filesystem::path(argv[3]).parent_path()/"contract-terms-runtime-sites.txt");for(unsigned n=0;n<roots.size();++n){require(!runtime_sites[n].empty(),"Every terms root has actual-data runtime sites");for(auto pc:runtime_sites[n])out<<n<<" "<<hex_address(pc)<<'\n';}}
    std::cout<<"PASS: contract terms production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
