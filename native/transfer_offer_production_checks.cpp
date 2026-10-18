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
namespace offers=cm::aot::transfer_offer_queries;
constexpr auto roots=offers::entries;
constexpr std::array<std::uint32_t,19> ends{0x103e6358,0x103e6464,0x103e64dc,0x103ea26c,0x103ea28c,0x103ea348,0x103ea43c,0x103ea5f8,0x103ea75c,0x103ea83c,0x103eb014,0x103eb1e4,0x103eb358,0x103eb434,0x103eb464,0x103eb494,0x103f43a4,0x103f4594,0x103fb3f8};
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
struct Witness:TransferOfferBoundaryObserver {
    std::uint32_t negative_root=0;
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State& s,std::uint32_t root,bool original,TransferOfferOutcome result)override{if(root==negative_root&&!original&&result==TransferOfferOutcome::returned)s.wr<4>(12,s.r<4>(12)^1);}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed>=s.budget)return;for(unsigned n=0;n<roots.size();++n)if(pc>=roots[n]&&pc<ends[n])runtime_sites[n].insert(pc);}
};
void select(State& s,unsigned index,Mode mode){auto& d=transfer_offer_diagnostics(s);if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;}
std::string execute(State& s,unsigned index,Path path){try{if(path==Path::direct)try_modern_transfer_offer(s,roots[index]);else{if(path==Path::cfm){s.memory.write<4>(fixture+0x400,roots[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}run_until(s,return_sentinel);}}catch(const std::exception& e){return e.what();}return {};}
unsigned cases=0,returns=0,budgets=0,negatives=0,faults=0;
void between(ActorAttributesIoPair& io){for(const auto& [guest,native]:io.old_forks.references){(void)guest;require(io.old_forks.forks.close(native)==0,"Release original contract native fork");}io.old_forks.references.clear();io.between();}

std::uint64_t compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,bool fault=false){
    Clock probe;ActorAttributesIoPair io(input,index,roots[index],[&](State& s){original_contracts(s);s.code_fragment_observer=&probe;});
    auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->pc=roots[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+limit;if(fault)s->wr<4>(index==6||index==9||index==18?16:index==7||index==8?20:12,0xf0000000);}
    original_contracts(*a);select(*b,index,mode);Witness witness;if(negative)witness.negative_root=roots[index];if(mode==Mode::modern)transfer_offer_diagnostics(*b).observer=&witness;
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto expected=execute(*a,index,path);a->platform.reset();between(io);const auto actual=execute(*b,index,path);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;transfer_offer_diagnostics(*b).observer=nullptr;
    const auto label="Transfer offer runtime root="+hex_address(roots[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(fault?expected.starts_with("Unmapped data access at "):limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected outcome: "+expected);
    require(old_clock.count==new_clock.count&&old_clock.events==new_clock.events,label+" host-input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative not discriminating");++negatives;}else{Image(*a).compare(*b,label);require(x.bytes==y.bytes&&a->executed==b->executed,label+" ordered effects/accounting");}
    io.compare();const auto& d=transfer_offer_diagnostics(*b);require(!d.original_body&&!d.dispatch_escapes,label+" leaked original scope or dispatch escape");
    require(!contract_query_diagnostics(*b).original_body&&!contract_storage_diagnostics(*b).original_body,label+" nested original scope residue");
    if(mode==Mode::modern){require(d.calls[index]>=1&&!d.original_calls[index]&&!d.profile_original_calls[index],label+" dedicated modern call");require(d.returns[index]+d.faults[index]+d.budget_stops[index]==d.calls[index],label+" balanced productive outcomes");require(expected.empty()?d.returns[index]==d.calls[index]:fault?d.faults[index]>=1&&!d.budget_stops[index]:d.budget_stops[index]>=1&&!d.faults[index],label+" productive outcome counters");}
    else{require(!d.calls[index]&&!d.returns[index],label+" original control entered C++");require(mode==Mode::profile?d.profile_original_calls[index]>=1:d.original_calls[index]>=1,label+" original control not reached");}
    ++cases;returns+=expected.empty();budgets+=!fault&&!expected.empty();faults+=fault;return a->executed-input.executed;
}
void matrix(const State& input,unsigned index,bool profile){
    if(profile){compare(input,index,Path::generated,Mode::profile);compare(input,index,Path::cfm,Mode::profile);return;}
    std::uint64_t steps=0;for(auto path:{Path::direct,Path::generated,Path::cfm}){const auto measured=compare(input,index,path,Mode::modern);if(path==Path::generated)steps=measured;}
    compare(input,index,Path::generated,Mode::original);compare(input,index,Path::generated,Mode::entry_original);compare(input,index,Path::generated,Mode::modern,50000000,true);
    require(steps>=2,"Measured transfer offer path for focused budget cuts");for(auto cut:std::set<std::uint64_t>{0,steps/2,steps-1})compare(input,index,Path::generated,Mode::modern,unsigned(cut));
    compare(input,index,Path::generated,Mode::modern,50000000,false,true);
    std::cout<<"PASS: transfer offer runtime root="<<hex_address(roots[index])<<" sites="<<runtime_sites[index].size()<<std::endl;
}
std::uint32_t setup(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args){
    std::array<bool*,5> selected{&contract_query_diagnostics(s).original,&contract_storage_diagnostics(s).original,&contract_terms_diagnostics(s).original,&contract_review_diagnostics(s).original,&transfer_offer_diagnostics(s).original};
    struct Restore{std::array<bool*,5> flags;std::array<bool,5> before{};explicit Restore(std::array<bool*,5> f):flags(f){for(unsigned n=0;n<flags.size();++n){before[n]=*flags[n];*flags[n]=true;}}~Restore(){for(unsigned n=0;n<flags.size();++n)*flags[n]=before[n];}}restore{selected};
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);s.pc=entry;s.wr<4>(4128,return_sentinel);s.budget=s.executed+500000000;
    try{run_until(s,return_sentinel);}catch(const std::exception& e){throw std::runtime_error("Transfer setup entry="+hex_address(entry)+" pc="+hex_address(s.pc)+" reason="+e.what());}return std::uint32_t(s.r<4>(12));
}
struct OwnedBacking {std::filesystem::path path;~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(path,e)||e||std::filesystem::is_symlink(path,e)||e)return;for(const auto& p:std::filesystem::recursive_directory_iterator(path,e))if(e||p.is_symlink(e)||e)return;std::filesystem::remove_all(path,e);}};
}
int main(int argc,char** argv){try{
    require(argc==5||(argc==6&&std::string_view(argv[5])=="--profile"),"Expected export directory, game, private backing root and original prepared snapshot, optionally --profile");const bool profile=argc==6;require(profile==match_pc_profile_enabled(),"Transfer offer profile flag mismatch");set_match_turbo_enabled(false);
    const auto directory=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(directory),"New private transfer offer backing");OwnedBacking cleanup{directory};
    const auto snapshots=std::filesystem::path(argv[4]).parent_path();std::ifstream pointer(snapshots/"prepared-backing.txt");std::string name;std::getline(pointer,name);require(std::filesystem::path(name).parent_path().empty()&&name.starts_with("prepared-backing-"),"Retained original backing name");ActorAttributesIoPair::copy(snapshots/name,directory);
    State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],directory,AudioMode::offline);load_match_snapshot(source,argv[4]);Clock clock;source.code_fragment_observer=&clock;
    const auto toc=std::uint32_t(source.r<4>(8)),count=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be4))),staff_base=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be0)));
    std::uint32_t player=0;for(unsigned n=0;n<count;++n){const auto staff=staff_base+n*0x70;if(source.memory.read<4>(staff+0x24)&&source.memory.read<4>(staff+0x14)){player=staff;break;}}
    require(count==153796&&player,"Actual full staff population and club player");const auto club=std::uint32_t(source.memory.read<4>(player+0x24)),manager=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7b7c)));require(manager!=0,"Original contract manager");
    // Original career allocation, constructor and publication sequences:
    // 0x10344d54..0x10344d74 and 0x1034614c..0x1034616c.
    for(auto spec:std::array<std::array<unsigned,3>,3>{{{0x7b3c,0x48,0x10301540},{0x7bf0,0x20,0x102cb930},{0x7b40,0xa7c,0x103df0d0}}}){
        const auto slot=std::uint32_t(source.memory.read<4>(toc-spec[0]));
        if(!source.memory.read<4>(slot)){
            const auto object=setup(source,0x10773520,{spec[1]});require(object!=0,"Original review management-service allocation");
            require(setup(source,spec[2],{object,0})==object,"Original review management-service construction");source.memory.write<4>(slot,object);
        }
    }
    const auto value=setup(source,0x10084be0,{manager,player,club,1,0xffffffff,0});if(!setup(source,0x10082f20,{manager,player}))setup(source,0x10078e70,{manager,player,value});const auto record=setup(source,0x10082f20,{manager,player});require(record&&source.memory.read<4>(record+4)==source.memory.read<4>(club),"Original actual-player primary contract binding");source.memory.map(fixture,0x10000);
    const auto transfer=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7b40)));
    std::array<std::uint32_t,3> buyers{};unsigned found=0;
    for(unsigned n=0;n<count&&found<buyers.size();++n){const auto candidate=std::uint32_t(source.memory.read<4>(staff_base+n*0x70+0x24));if(candidate&&candidate!=club&&std::find(buyers.begin(),buyers.end(),candidate)==buyers.end())buyers[found++]=candidate;}
    require(found==buyers.size(),"Three distinct actual buyer clubs");
    const auto new_offer=[&](std::uint32_t person,std::uint32_t buyer,std::uint32_t amount){const auto object=setup(source,0x10773520,{0x54});require(object!=0,"Original offer allocation");require(setup(source,0x10411e60,{object})==object,"Original offer constructor");setup(source,0x104164f0,{object,person,buyer,amount});require(source.memory.read<4>(object+8)==source.memory.read<4>(buyer)&&source.memory.read<4>(object+12)==source.memory.read<4>(person)&&source.memory.read<4>(object+0x20)==amount,"Original offer initialization");return object;};
    std::array<std::uint32_t,3> published{};
    for(unsigned n=0;n<buyers.size();++n){const auto input=new_offer(player,buyers[n],1000*(n+1));setup(source,0x103e25d0,{transfer,input,0});const auto index=std::uint32_t(source.memory.read<4>(input+0x28)),array=std::uint32_t(source.memory.read<4>(transfer));published[n]=std::uint32_t(source.memory.read<4>(array+index*4));require(published[n]!=0&&source.memory.read<4>(published[n]+12)==source.memory.read<4>(player),"Original offer registration and index publication");}
    const auto alternate=new_offer(player,buyers[0],7500);setup(source,0x103e2db0,{transfer,published[0],alternate,0});
    std::uint32_t secondary=0;const auto second_array=std::uint32_t(source.memory.read<4>(transfer+12)),second_capacity=std::uint32_t(source.memory.read<4>(transfer+16));
    for(unsigned n=0;n<second_capacity&&!secondary;++n)secondary=std::uint32_t(source.memory.read<4>(second_array+n*4));require(secondary!=0,"Original secondary offer registration");
    // Register a second actual player with the current player included in exchange.
    std::uint32_t other=0;for(unsigned n=0;n<count;++n){const auto candidate=staff_base+n*0x70;if(source.memory.read<4>(candidate+0x24)==buyers[0]&&source.memory.read<4>(candidate+0x14)){other=candidate;break;}}
    require(other!=0,"Actual exchange-offer target");const auto other_contract=setup(source,0x10084be0,{manager,other,buyers[0],1,0xffffffff,0});if(!setup(source,0x10082f20,{manager,other}))setup(source,0x10078e70,{manager,other,other_contract});
    const auto exchange_list=setup(source,0x10773520,{16});require(exchange_list&&setup(source,0x10738c00,{exchange_list})==exchange_list,"Original exchange list constructor");source.memory.write<4>(fixture+0x100,source.memory.read<4>(player));setup(source,0x10738d10,{exchange_list,fixture+0x100,4});
    const auto exchange_input=new_offer(other,club,5000);source.memory.write<4>(exchange_input+4,exchange_list);setup(source,0x103e25d0,{transfer,exchange_input,0});const auto exchange_index=std::uint32_t(source.memory.read<4>(exchange_input+0x28)),exchange_offer=std::uint32_t(source.memory.read<4>(source.memory.read<4>(transfer)+exchange_index*4));require(exchange_offer!=0,"Original exchange offer publication");
    const auto exchange_alternate=new_offer(other,club,9000);source.memory.write<4>(exchange_alternate+4,exchange_list);setup(source,0x103e2db0,{transfer,exchange_offer,exchange_alternate,0});
    setup(source,roots[0],{transfer,player,1,0xffffffff});
    const auto output_list=setup(source,0x10773520,{16});require(output_list!=0&&setup(source,0x10738c00,{output_list})==output_list,"Original output list construction");
    const auto check=[&](const State& base,unsigned index,std::initializer_list<std::uint32_t> args,bool complete=true){auto s=clone(base);s->platform=base.platform;unsigned n=3;for(auto v:args)s->wr<4>(n++*4,v);if(complete)matrix(*s,index,profile);else compare(*s,index,Path::generated,profile?Mode::profile:Mode::modern);};
    std::cout<<"Prepared original transfer-offer world staff="<<count<<" player="<<source.memory.read<4>(player)<<" primaryFamilies="<<source.memory.read<4>(transfer+8)<<" secondaryRecords="<<source.memory.read<4>(transfer+20)<<std::endl;
    for(unsigned n=0;n<roots.size();++n){
        if(n==0)check(source,n,{transfer,player,1,0xffffffff});
        else if(n==1||n==13)check(source,n,{transfer,club,output_list});
        else if(n==6)check(source,n,{transfer,published[0]});
        else if(n==7)check(source,n,{transfer,0x800,published[0]});
        else if(n==8)check(source,n,{transfer,0x200,fixture+0x2000,200});
        else if(n==9)check(source,n,{transfer,player,0x200});
        else if(n==14||n==15)check(source,n,{transfer,0});
        else if(n==16)check(source,n,{transfer,player,buyers[0]});
        else if(n==18)check(source,n,{transfer,published[0]});
        else check(source,n,{transfer,player,n==5||n==17?fixture+0x3000:output_list,0});
    }
    const auto cache=std::uint32_t(source.memory.read<4>(transfer+0x9a0))+std::uint32_t(source.memory.read<4>(player))*10;
    // These are controlled status changes to original-created objects; dependencies stay real.
    for(auto reason:{0u,10u,0x200u,0x400u,0x800u,0x1000u,0x2000u,3u}){
        check(source,8,{transfer,reason,fixture+0x2000,200},false);
        check(source,9,{transfer,player,reason},false);
        check(source,7,{transfer,reason,published[0]},false);
    }
    for(auto status:{1u,2u,3u,4u,5u}){auto s=clone(source);s->platform=source.platform;s->memory.write<1>(record+0x60,status);check(*s,6,{transfer,published[0]},false);}
    for(auto age:{23u,24u}){auto s=clone(source);s->platform=source.platform;s->memory.write<2>(player+0x5c,age);check(*s,7,{transfer,0x2000,published[0]},false);}
    for(auto status:{0u,2u,3u,4u,15u,16u,19u,20u}){auto s=clone(source);s->platform=source.platform;s->memory.write<1>(published[0]+0x34,status);s->memory.write<1>(published[0]+0x36,2);check(*s,18,{transfer,published[0]},false);if(status==15){s->memory.write<1>(published[0]+0x36,3);check(*s,18,{transfer,published[0]},false);}}
    check(source,10,{transfer,player,0,1},false);check(source,11,{transfer,player,0},false);check(source,12,{transfer,player,0},false);check(source,13,{transfer,buyers[0],output_list},false);
    for(auto index:{0xffffffffu,49u,50u}){check(source,14,{transfer,index},false);check(source,15,{transfer,index},false);}
    {
        auto s=clone(source);s->platform=source.platform;for(auto offer:published){s->memory.write<1>(offer+0x34,2);s->memory.write<1>(offer+0x35,1);}
        auto expected=clone(*s);expected->platform=source.platform;require(setup(*expected,roots[16],{transfer,player,buyers[0]})==published[2],"Original actual valuation selects maximum within first family");check(*s,16,{transfer,player,buyers[0]},false);
        s->memory.write<4>(published[2]+0x20,2000);expected=clone(*s);expected->platform=source.platform;require(setup(*expected,roots[16],{transfer,player,buyers[0]})==published[1],"Original actual valuation keeps first equal maximum");check(*s,16,{transfer,player,buyers[0]},false);
    }
    {
        auto s=clone(source);s->platform=source.platform;s->memory.write<1>(cache,std::uint32_t(s->memory.read<1>(cache))|0x10);s->memory.write<1>(published[1]+0x34,19);check(*s,17,{transfer,player,0},false);
        s->memory.write<1>(published[1]+0x34,0);s->memory.write<1>(exchange_offer+0x34,19);check(*s,17,{transfer,player,fixture+0x3000},false);
        s->memory.write<1>(exchange_offer+0x34,0);check(*s,17,{transfer,player,fixture+0x3000},false);
    }
    {
        auto s=clone(source);s->platform=source.platform;
        // The original pending-contract service creates then updates its own record to tag 2.
        for(unsigned n=0;n<2;++n)setup(*s,0x1007f200,{manager,record,0xffffffff,0,0,0,0,1});
        require(setup(*s,0x1007f7b0,{manager,std::uint32_t(s->memory.read<4>(player))})!=0,"Original pending-contract association");setup(*s,roots[3],{transfer,player});check(*s,5,{transfer,player,fixture+0x3000},false);check(*s,5,{transfer,player,0},false);check(*s,4,{transfer,player},false);
    }
    if(!profile){std::ofstream out(std::filesystem::path(argv[3]).parent_path()/"transfer-offer-runtime-sites.txt");for(unsigned n=0;n<roots.size();++n){require(!runtime_sites[n].empty(),"Every transfer-offer root has actual-data runtime sites");for(auto pc:runtime_sites[n])out<<n<<" "<<hex_address(pc)<<'\n';}}
    std::cout<<"PASS: transfer offer production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
