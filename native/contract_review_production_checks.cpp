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
namespace review=cm::aot::contract_review;
constexpr auto roots=review::entries;
constexpr std::array<std::uint32_t,13> ends{0x1037f110,0x1037f588,0x1037f7c4,0x1037f830,0x1037f898,0x1037fa48,0x1037fa60,0x1037fa6c,0x1037fa78,0x10380264,0x10380d4c,0x10380f1c,0x103810d0};
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
struct Witness:ContractReviewBoundaryObserver {
    std::uint32_t negative_root=0;
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t root,bool original,ContractReviewOutcome result)override{if(root==negative_root&&!original&&result==ContractReviewOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed>=s.budget)return;for(unsigned n=0;n<roots.size();++n)if(pc>=roots[n]&&pc<ends[n])runtime_sites[n].insert(pc);}
};
void select(State& s,unsigned index,Mode mode){auto& d=contract_review_diagnostics(s);if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;}
std::string execute(State& s,unsigned index,Path path){try{if(path==Path::direct)try_modern_contract_review(s,roots[index]);else{if(path==Path::cfm){s.memory.write<4>(fixture+0x400,roots[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}run_until(s,return_sentinel);}}catch(const std::exception& e){return e.what();}return {};}
unsigned cases=0,returns=0,budgets=0,negatives=0,faults=0;
void between(ActorAttributesIoPair& io){for(const auto& [guest,native]:io.old_forks.references){(void)guest;require(io.old_forks.forks.close(native)==0,"Release original contract native fork");}io.old_forks.references.clear();io.between();}

std::uint64_t compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,bool fault=false){
    Clock probe;ActorAttributesIoPair io(input,index,roots[index],[&](State& s){original_contracts(s);s.code_fragment_observer=&probe;});
    auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->pc=roots[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;if(fault)s->wr<4>(index==12?20:12,0xf0000000);}
    original_contracts(*a);select(*b,index,mode);Witness witness;if(negative)witness.negative_root=roots[index];if(mode==Mode::modern)contract_review_diagnostics(*b).observer=&witness;
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,index,path);a->platform.reset();between(io);const auto actual=execute(*b,index,path);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;contract_review_diagnostics(*b).observer=nullptr;
    const auto label="Contract terms runtime root="+hex_address(roots[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(fault?expected.starts_with("Unmapped data access at "):limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected outcome: "+expected);
    require(old_clock.count==new_clock.count&&old_clock.events==new_clock.events,label+" host-input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative not discriminating");++negatives;}else{Image(*a).compare(*b,label);require(x.bytes==y.bytes&&a->executed==b->executed,label+" ordered effects/accounting");}
    io.compare();const auto& d=contract_review_diagnostics(*b);require(!d.original_body&&!d.dispatch_escapes,label+" leaked original scope or dispatch escape");
    require(!contract_query_diagnostics(*b).original_body&&!contract_storage_diagnostics(*b).original_body,label+" nested original scope residue");
    if(mode==Mode::modern){require(d.calls[index]>=1&&!d.original_calls[index]&&!d.profile_original_calls[index],label+" dedicated modern call");require(d.returns[index]+d.faults[index]+d.budget_stops[index]==d.calls[index],label+" balanced productive outcomes");require(expected.empty()?d.returns[index]==d.calls[index]:fault?d.faults[index]>=1&&!d.budget_stops[index]:d.budget_stops[index]>=1&&!d.faults[index],label+" productive outcome counters");}
    else{require(!d.calls[index]&&!d.returns[index],label+" original control entered C++");require(mode==Mode::profile?d.profile_original_calls[index]>=1:d.original_calls[index]>=1,label+" original control not reached");}
    ++cases;returns+=expected.empty();budgets+=!fault&&!expected.empty();faults+=fault;return a->executed-input.executed;
}
void matrix(const State& input,unsigned index,bool profile){
    if(profile){compare(input,index,Path::generated,Mode::profile);compare(input,index,Path::cfm,Mode::profile);return;}
    std::uint64_t steps=0;for(auto path:{Path::direct,Path::generated,Path::cfm}){const auto measured=compare(input,index,path,Mode::modern);if(path==Path::generated)steps=measured;}
    compare(input,index,Path::generated,Mode::original);compare(input,index,Path::generated,Mode::entry_original);compare(input,index,Path::generated,Mode::modern,50000000,true);
    require(steps>=2,"Measured contract review path for focused budget cuts");for(auto cut:std::set<std::uint64_t>{0,steps/2,steps-1})compare(input,index,Path::generated,Mode::modern,unsigned(cut));
    compare(input,index,Path::generated,Mode::modern,50000000,false,true);
    std::cout<<"PASS: contract review runtime root="<<hex_address(roots[index])<<" sites="<<runtime_sites[index].size()<<std::endl;
}
std::uint32_t setup(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args){
    auto& q=contract_query_diagnostics(s).original;auto& c=contract_storage_diagnostics(s).original;
    auto& terms=contract_terms_diagnostics(s).original;auto& v=contract_review_diagnostics(s).original;auto& offers=transfer_offer_diagnostics(s).original;
    struct Restore{bool& q;bool& c;bool& t;bool& v;bool& offers;bool a,b,d,e,f;~Restore(){q=a;c=b;t=d;v=e;offers=f;}}restore{q,c,terms,v,offers,q,c,terms,v,offers};q=c=terms=v=offers=true;
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);s.pc=entry;s.wr<4>(4128,return_sentinel);s.budget=s.executed+500000000;try{run_until(s,return_sentinel);}catch(const std::exception& e){throw std::runtime_error("Review setup entry="+hex_address(entry)+" pc="+hex_address(s.pc)+" reason="+e.what());}return std::uint32_t(s.r<4>(12));
}
struct OwnedBacking {std::filesystem::path path;~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(path,e)||e||std::filesystem::is_symlink(path,e)||e)return;for(const auto& p:std::filesystem::recursive_directory_iterator(path,e))if(e||p.is_symlink(e)||e)return;std::filesystem::remove_all(path,e);}};
}
int main(int argc,char** argv){try{
    require(argc==5||(argc==6&&std::string_view(argv[5])=="--profile"),"Expected export directory, game, private backing root and original prepared snapshot, optionally --profile");const bool profile=argc==6;require(profile==match_pc_profile_enabled(),"Contract terms profile flag mismatch");set_match_turbo_enabled(false);
    const auto directory=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(directory),"New private contract review backing");OwnedBacking cleanup{directory};
    const auto snapshots=std::filesystem::path(argv[4]).parent_path();std::ifstream pointer(snapshots/"prepared-backing.txt");std::string name;std::getline(pointer,name);require(std::filesystem::path(name).parent_path().empty()&&name.starts_with("prepared-backing-"),"Retained original backing name");ActorAttributesIoPair::copy(snapshots/name,directory);
    State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],directory,AudioMode::offline);load_match_snapshot(source,argv[4]);Clock clock;source.code_fragment_observer=&clock;
    const auto toc=std::uint32_t(source.r<4>(8)),count=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be4))),staff_base=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be0)));
    std::uint32_t player=0;for(unsigned n=0;n<count;++n){const auto staff=staff_base+n*0x70;if(source.memory.read<4>(staff+0x24)&&source.memory.read<4>(staff+0x14)){player=staff;break;}}
    require(count==153796&&player,"Actual full staff population and club player");const auto club=std::uint32_t(source.memory.read<4>(player+0x24)),manager=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7b7c)));require(manager!=0,"Original contract manager");
    // Original career allocation, constructor and publication sequences:
    // 0x10344d54..0x10344d74 and 0x1034614c..0x1034616c.
    for(auto spec:std::array<std::array<unsigned,3>,2>{{{0x7b3c,0x48,0x10301540},{0x7b40,0xa7c,0x103df0d0}}}){
        const auto slot=std::uint32_t(source.memory.read<4>(toc-spec[0]));
        if(!source.memory.read<4>(slot)){
            const auto object=setup(source,0x10773520,{spec[1]});require(object!=0,"Original review management-service allocation");
            require(setup(source,spec[2],{object,0})==object,"Original review management-service construction");source.memory.write<4>(slot,object);
        }
    }
    const auto value=setup(source,0x10084be0,{manager,player,club,1,0xffffffff,0});if(!setup(source,0x10082f20,{manager,player}))setup(source,0x10078e70,{manager,player,value});const auto record=setup(source,0x10082f20,{manager,player});require(record&&source.memory.read<4>(record+4)==source.memory.read<4>(club),"Original actual-player primary contract binding");source.memory.map(fixture,0x10000);
    const auto check=[&](const State& base,unsigned index,std::initializer_list<std::uint32_t> args){auto s=clone(base);s->platform=base.platform;unsigned n=3;for(auto v:args)s->wr<4>(n++*4,v);matrix(*s,index,profile);};
    std::cout<<"Prepared original terms world staff="<<count<<" player="<<source.memory.read<4>(player)<<std::endl;
    for(unsigned n=0;n<roots.size();++n){
        if(n==0)check(source,n,{record,player,club});
        else if(n==10||n==11)check(source,n,{record,fixture+0x2000,200});
        else if(n==12)check(source,n,{record,0,fixture+0x3000,fixture+0x3010});
        else check(source,n,{record,1,0});
    }
    check(source,0,{record,player,0});check(source,2,{record,0});check(source,4,{record,0});
    for(auto mask:{0u,0x100u,0x10000u,0xffffffffu})check(source,3,{record,mask});
    for(auto mask:{0u,4u,0x1000u})check(source,5,{record,mask});
    for(auto mask:{0u,0xffffffffu})check(source,6,{record,mask});
    for(auto mask:{0u,0x10000u,0x20001u,0xe0000000u})check(source,9,{record,mask,player});
    for(unsigned status=0;status<9;++status){auto s=clone(source);s->platform=source.platform;s->memory.write<1>(record+0x60,status);check(*s,10,{record,fixture+0x2000,200});}
    for(auto fields:std::array<std::array<unsigned,4>,7>{{{0,0,0,0},{0,2,0,0},{0,1,1,0},{0,0,2,0},{0,0,0,1},{2,0,0,0},{255,0,0,0}}}){
        auto s=clone(source);s->platform=source.platform;
        s->memory.write<1>(record+0x1d,fields[0]);s->memory.write<1>(record+0x1c,fields[1]);s->memory.write<1>(record+0x1f,fields[2]);s->memory.write<1>(record+0x20,fields[3]);
        check(*s,11,{record,fixture+0x2000,200});
    }
    // Calendar and age paths retain real services and use copied real date values.
    for(auto flag:{0u,1u})for(auto slot:{-0x7b80,-0x79e8}){
        auto s=clone(source);s->platform=source.platform;
        s->memory.write<1>(s->memory.read<4>(toc-0x7a68),flag);
        const auto date=std::uint32_t(s->memory.read<4>(toc+slot));s->memory.copy_in(record+0x28,s->memory.copy_out(date,8));
        check(*s,12,{record,0,fixture+0x3000,fixture+0x3010});
    }
    check(source,12,{record,0,0,0});
    std::array<std::uint32_t,2> age_cohorts{};
    for(unsigned n=0;n<count&&(!age_cohorts[0]||!age_cohorts[1]);++n){
        const auto staff=staff_base+n*0x70;if(!source.memory.read<4>(staff+0x24)||!source.memory.read<4>(staff+0x14))continue;
        const auto age=static_cast<std::int8_t>(setup(source,0x100c6a10,{staff,record+0x28}));
        if(age>=16&&age<65&&!age_cohorts[age>=28])age_cohorts[age>=28]=staff;
    }
    require(age_cohorts[0]&&age_cohorts[1],"Actual staff below and above the age threshold");
    for(const auto staff:age_cohorts){auto s=clone(source);s->platform=source.platform;
        s->memory.write<4>(record,s->memory.read<4>(staff));s->memory.write<1>(s->memory.read<4>(toc-0x7a68),1);
        const auto reference=std::uint32_t(s->memory.read<4>(toc-0x79e8));s->memory.copy_in(record+0x28,s->memory.copy_out(reference,8));
        check(*s,12,{record,0,fixture+0x3000,fixture+0x3010});
    }
    if(!profile){std::ofstream out(std::filesystem::path(argv[3]).parent_path()/"contract-review-runtime-sites.txt");for(unsigned n=0;n<roots.size();++n){require(!runtime_sites[n].empty(),"Every review root has actual-data runtime sites");for(auto pc:runtime_sites[n])out<<n<<" "<<hex_address(pc)<<'\n';}}
    std::cout<<"PASS: contract review production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
