#include "contract_terms_adapter.hpp"
#include "contract_review_adapter.hpp"
#include "transfer_offer_adapter.hpp"
#include "transfer_record_adapter.hpp"
#include "transfer_valuation_adapter.hpp"
#include "transfer_synthesis_adapter.hpp"
#include "club_shortlist_storage_adapter.hpp"
#include "shortlist_category_adapter.hpp"
#include "shortlist_links_adapter.hpp"
#include "shortlist_recruitment_adapter.hpp"
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
constexpr std::array<std::uint32_t,1> roots{0x104126d0};
constexpr std::array<std::uint32_t,1> ends{0x10415e4c};
enum class Path {direct,generated,cfm};
enum class Mode {modern,original,entry_original,profile};
void original_contracts(State& s){shortlist_recruitment_diagnostics(s).original=true;shortlist_links_diagnostics(s).original=true;shortlist_category_diagnostics(s).original=true;club_shortlist_storage_diagnostics(s).original=true;contract_query_diagnostics(s).original=true;contract_storage_diagnostics(s).original=true;contract_terms_diagnostics(s).original=true;contract_review_diagnostics(s).original=true;transfer_offer_diagnostics(s).original=true;transfer_record_diagnostics(s).original=true;transfer_valuation_diagnostics(s).original=true;transfer_synthesis_diagnostics(s).original=true;}

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

std::uint32_t setup(State&,std::uint32_t,std::initializer_list<std::uint32_t>);
std::array<std::set<std::uint32_t>,roots.size()> runtime_sites;
struct Witness:TransferSynthesisBoundaryObserver {
    std::uint32_t negative_root=0;
    void enter(const State&,std::uint32_t,bool)override{}
    void damage(State& s,std::uint32_t root,bool original,bool returned){if(root==negative_root&&!original&&returned)s.wr<4>(12,s.r<4>(12)^1);}
    void leave(State& s,std::uint32_t root,bool original,TransferSynthesisOutcome result)override{damage(s,root,original,result==TransferSynthesisOutcome::returned);}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed>=s.budget)return;for(unsigned n=0;n<roots.size();++n)if(pc>=roots[n]&&pc<ends[n])runtime_sites[n].insert(pc);}
};
void select(State& s,unsigned,Mode mode){auto& d=transfer_synthesis_diagnostics(s);if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[0]=true;}

std::string execute(State& s,unsigned index,Path path,unsigned limit,std::uint64_t& steps){try{
    s.pc=roots[index];s.wr<4>(4128,return_sentinel);s.budget=s.executed+limit;const auto before=s.executed;
    try{if(path==Path::direct){try_modern_transfer_synthesis(s,roots[index]);}else{if(path==Path::cfm){s.memory.write<4>(fixture+0x400,roots[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}run_until(s,return_sentinel);}}catch(...){steps=s.executed-before;throw;}
    steps=s.executed-before;
}catch(const std::exception& e){return e.what();}return {};}
unsigned cases=0,returns=0,budgets=0,negatives=0,faults=0;
void between(ActorAttributesIoPair& io){for(const auto& [guest,native]:io.old_forks.references){(void)guest;require(io.old_forks.forks.close(native)==0,"Release original transfer native fork");}io.old_forks.references.clear();io.between();}
std::uint64_t compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=50000000,bool negative=false,bool fault=false){
    Clock probe;ActorAttributesIoPair io(input,index,roots[index],[&](State& s){original_contracts(s);s.code_fragment_observer=&probe;});
    auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->pc=roots[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+50000000;if(fault)s->wr<4>(12,0xf0000000);}
    original_contracts(*a);select(*b,index,mode);Witness witness;if(negative)witness.negative_root=roots[index];if(mode==Mode::modern){transfer_synthesis_diagnostics(*b).observer=&witness;}
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;std::uint64_t old_steps=0,new_steps=0;
    const auto expected=execute(*a,index,path,limit,old_steps);a->platform.reset();between(io);const auto actual=execute(*b,index,path,limit,new_steps);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;transfer_synthesis_diagnostics(*b).observer=nullptr;
    const auto label="Transfer synthesis runtime root="+hex_address(roots[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(fault?expected.starts_with("Unmapped data access at "):limit==50000000?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected outcome: "+expected);
    require(old_clock.count==new_clock.count&&old_clock.events==new_clock.events,label+" host-input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative not discriminating");++negatives;}else{Image(*a).compare(*b,label);require(x.bytes==y.bytes&&a->executed==b->executed&&old_steps==new_steps,label+" ordered effects/accounting");}
    io.compare();const auto audit=[&](const auto& d,unsigned n){require(!d.original_body&&!d.dispatch_escapes,label+" leaked original scope or dispatch escape");if(mode==Mode::modern){require(d.calls[n]>=1&&!d.original_calls[n]&&!d.profile_original_calls[n],label+" dedicated modern call");require(d.returns[n]+d.faults[n]+d.budget_stops[n]==d.calls[n],label+" balanced productive outcomes");require(expected.empty()?d.returns[n]==d.calls[n]:fault?d.faults[n]>=1&&!d.budget_stops[n]:d.budget_stops[n]>=1&&!d.faults[n],label+" productive outcome counters");}else{require(!d.calls[n]&&!d.returns[n],label+" original control entered C++");require(mode==Mode::profile?d.profile_original_calls[n]>=1:d.original_calls[n]>=1,label+" original control not reached");}};
    audit(transfer_synthesis_diagnostics(*b),0);
    require(!transfer_synthesis_diagnostics(*b).original_body&&!transfer_record_diagnostics(*b).original_body&&!transfer_valuation_diagnostics(*b).original_body,label+" nested original scope residue");
    ++cases;returns+=expected.empty();budgets+=!fault&&!expected.empty();faults+=fault;return old_steps;
}
void matrix(const State& input,unsigned index,bool profile){
    if(profile){compare(input,index,Path::generated,Mode::profile);compare(input,index,Path::cfm,Mode::profile);return;}
    std::uint64_t steps=0;for(auto path:{Path::direct,Path::generated,Path::cfm}){const auto measured=compare(input,index,path,Mode::modern);if(path==Path::generated)steps=measured;}
    compare(input,index,Path::generated,Mode::original);compare(input,index,Path::generated,Mode::entry_original);compare(input,index,Path::generated,Mode::modern,50000000,true);
    require(steps>=2,"Measured transfer synthesis path");for(auto cut:std::set<std::uint64_t>{0,steps/2,steps-1})compare(input,index,Path::generated,Mode::modern,unsigned(cut));
    compare(input,index,Path::generated,Mode::modern,50000000,false,true);std::cout<<"PASS: transfer synthesis runtime root="<<hex_address(roots[index])<<" sites="<<runtime_sites[index].size()<<std::endl;
}
std::uint32_t setup(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args){
    std::array<bool*,12> selected{&shortlist_recruitment_diagnostics(s).original,&shortlist_links_diagnostics(s).original,&shortlist_category_diagnostics(s).original,&club_shortlist_storage_diagnostics(s).original,&contract_query_diagnostics(s).original,&contract_storage_diagnostics(s).original,&contract_terms_diagnostics(s).original,&contract_review_diagnostics(s).original,&transfer_offer_diagnostics(s).original,&transfer_record_diagnostics(s).original,&transfer_valuation_diagnostics(s).original,&transfer_synthesis_diagnostics(s).original};
    struct Restore{std::array<bool*,12> flags;std::array<bool,12> before{};explicit Restore(std::array<bool*,12> f):flags(f){for(unsigned n=0;n<flags.size();++n){before[n]=*flags[n];*flags[n]=true;}}~Restore(){for(unsigned n=0;n<flags.size();++n)*flags[n]=before[n];}}restore{selected};
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);s.pc=entry;s.wr<4>(4128,return_sentinel);s.budget=s.executed+500000000;
    try{run_until(s,return_sentinel);}catch(const std::exception& e){if(auto* clock=dynamic_cast<Clock*>(s.code_fragment_observer)){const auto first=clock->events.size()>16?clock->events.size()-16:0;for(auto n=first;n<clock->events.size();++n){const auto& event=clock->events[n];std::uint32_t value=0;for(unsigned b=12;b<16;++b)value=(value<<8)|event.registers[b];std::cerr<<"Setup import "<<event.name<<" leave="<<event.leave<<" r3="<<hex_address(value)<<std::endl;}}throw std::runtime_error("Transfer setup entry="+hex_address(entry)+" pc="+hex_address(s.pc)+" reason="+e.what());}return std::uint32_t(s.r<4>(12));
}
struct OwnedBacking {std::filesystem::path path;~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(path,e)||e||std::filesystem::is_symlink(path,e)||e)return;for(const auto& p:std::filesystem::recursive_directory_iterator(path,e))if(e||p.is_symlink(e)||e)return;std::filesystem::remove_all(path,e);}};
}
int main(int argc,char** argv){try{
    require(argc==5||(argc==6&&std::string_view(argv[5])=="--profile"),"Expected export directory, game, private backing root and original prepared snapshot, optionally --profile");const bool profile=argc==6;require(profile==match_pc_profile_enabled(),"Transfer synthesis profile flag mismatch");set_match_turbo_enabled(false);
    const auto directory=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(directory),"New private transfer synthesis backing");OwnedBacking cleanup{directory};
    const auto snapshots=std::filesystem::path(argv[4]).parent_path();std::ifstream pointer(snapshots/"prepared-backing.txt");std::string name;std::getline(pointer,name);require(std::filesystem::path(name).parent_path().empty()&&name.starts_with("prepared-backing-"),"Retained original backing name");ActorAttributesIoPair::copy(snapshots/name,directory);
    State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],directory,AudioMode::offline);load_match_snapshot(source,argv[4]);Clock clock;source.code_fragment_observer=&clock;
    const auto toc=std::uint32_t(source.r<4>(8)),count=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be4))),staff_base=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be0)));
    std::uint32_t player=0;for(unsigned n=0;n<count;++n){const auto staff=staff_base+n*0x70;if(source.memory.read<4>(staff+0x24)&&source.memory.read<4>(staff+0x14)){player=staff;break;}}
    require(count==153796&&player,"Actual full staff population and club player");const auto club=std::uint32_t(source.memory.read<4>(player+0x24)),manager=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7b7c)));require(manager!=0,"Original contract manager");
    // Original career allocation, constructor and publication sequences:
    // 0x10344d54..0x10344d74, 0x1034614c..0x1034616c and
    // 0x1034530c..0x1034532c (staff-status lists and byte flags).
    for(auto spec:std::array<std::array<unsigned,3>,5>{{{0x7b3c,0x48,0x10301540},{0x7bf0,0x20,0x102cb930},{0x7b40,0xa7c,0x103df0d0},{0x7ab8,4,0x100f84e0},{0x7b24,0x10,0x100031b0}}}){
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
    const auto exchange_list=setup(source,0x10773520,{16});require(exchange_list&&setup(source,0x10738c00,{exchange_list})==exchange_list,"Original exchange list constructor");source.memory.write<4>(fixture+0x100,source.memory.read<4>(player));source.memory.write<4>(fixture+0x104,0);setup(source,0x10738d10,{exchange_list,fixture+0x100,8});
    const auto exchange_input=new_offer(other,club,5000);source.memory.write<4>(exchange_input+4,exchange_list);setup(source,0x103e25d0,{transfer,exchange_input,0});const auto exchange_index=std::uint32_t(source.memory.read<4>(exchange_input+0x28)),exchange_offer=std::uint32_t(source.memory.read<4>(source.memory.read<4>(transfer)+exchange_index*4));require(exchange_offer!=0,"Original exchange offer publication");
    const auto exchange_alternate=new_offer(other,club,9000);source.memory.write<4>(exchange_alternate+4,exchange_list);setup(source,0x103e2db0,{transfer,exchange_offer,exchange_alternate,0});
    const auto object=setup(source,0x10773520,{0x54});require(object!=0,"Original scratch offer allocation");
    const auto check=[&](std::uint32_t person,std::uint32_t buyer,std::uint32_t terms,std::uint32_t limit,std::uint32_t mode,std::uint32_t output,bool complete){auto s=clone(source);s->platform=source.platform;unsigned n=3;for(auto value:std::array{object,person,buyer,terms,limit,mode,output})s->wr<4>(n++*4,value);if(complete)matrix(*s,0,profile);else if(profile){compare(*s,0,Path::generated,Mode::profile);compare(*s,0,Path::cfm,Mode::profile);}else compare(*s,0,Path::generated,Mode::modern);};
    std::cout<<"Prepared original synthesis world staff="<<count<<" player="<<source.memory.read<4>(player)<<" primaryFamilies="<<source.memory.read<4>(transfer+8)<<" secondaryRecords="<<source.memory.read<4>(transfer+20)<<std::endl;
    // The record constructor and direct caller 0x103e358c pass null terms.
    // Scheduling caller 0x1036e198 instead passes a 0x2c-byte club-policy row.
    check(player,buyers[0],0,0xffffffff,0,fixture+0x200,true);
    check(player,buyers[1],0,0xffffffff,1,0,true);
    for(auto buyer:buyers)for(auto limit:{0u,5000u,100000u,0xffffffffu})for(auto mode:{0u,1u})check(player,buyer,0,limit,mode,fixture+0x200,false);
    check(other,club,0,0xffffffff,0,fixture+0x200,false);
    // The original policy constructor (0x10356410) initializes forty rows per
    // club; reuse its allocated, original-written record without fabricating it.
    const auto policy=std::uint32_t(source.memory.read<4>(transfer+0x988));
    const auto policy_clubs=std::uint32_t(source.memory.read<4>(policy+0xdc));
    const auto terms=std::uint32_t(source.memory.read<4>(policy_clubs+source.memory.read<4>(buyers[0])*12));
    require(terms&&source.memory.read<1>(terms)==11,"Original-constructed policy row");
    check(player,buyers[0],terms,0xffffffff,0,fixture+0x200,true);
    check(player,buyers[0],terms,std::uint32_t(source.memory.read<4>(terms+8)),0,fixture+0x200,false);
    if(!profile){std::ofstream out(std::filesystem::path(argv[3]).parent_path()/"transfer-synthesis-runtime-sites.txt");require(!runtime_sites[0].empty(),"Actual-service synthesis sites");for(auto pc:runtime_sites[0])out<<0<<" "<<hex_address(pc)<<'\n';}
    std::cout<<"PASS: transfer synthesis production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
