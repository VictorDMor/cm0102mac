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
#include "shortlist_assessment_adapter.hpp"
#include "shortlist_quotation_adapter.hpp"
#include "shortlist_policy_adapter.hpp"
#include "contract_query_adapter.hpp"
#include "contract_storage_adapter.hpp"
#include "actor_attributes_io_checks.hpp"
#include "code_fragment_adapter.hpp"
#include "match_turbo.hpp"
#include "match_replay.hpp"
#include "platform.hpp"
#include <CommonCrypto/CommonDigest.h>
#include <cstring>
#include <chrono>
#include <fstream>
#include <set>
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
constexpr auto roots=club_shortlist_storage::entries;
constexpr std::array<std::uint32_t,7> ends{0x103567a8,0x103568d4,0x10356e3c,0x103573b8,0x10358af0,0x10358b24,0x10358f14};
constexpr unsigned full_limit=4000000000u;
enum class Path {direct,generated,cfm};
enum class Mode {modern,original,entry_original,profile};
void original_contracts(State& s){shortlist_policy_diagnostics(s).original=true;shortlist_quotation_diagnostics(s).original=true;shortlist_assessment_diagnostics(s).original=true;shortlist_recruitment_diagnostics(s).original=true;shortlist_links_diagnostics(s).original=true;shortlist_category_diagnostics(s).original=true;club_shortlist_storage_diagnostics(s).original=true;contract_query_diagnostics(s).original=true;contract_storage_diagnostics(s).original=true;contract_terms_diagnostics(s).original=true;contract_review_diagnostics(s).original=true;transfer_offer_diagnostics(s).original=true;transfer_record_diagnostics(s).original=true;transfer_valuation_diagnostics(s).original=true;transfer_synthesis_diagnostics(s).original=true;}

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
std::uint32_t prepare_stream(State& s,unsigned index){
    const bool read=index==4||(index==0&&s.r<4>(16)!=0),write=index==6;if(!read&&!write)return 0;
    const auto registers=s.registers;const auto stream=setup(s,0x107475b0,{fixture+0x600,read?fixture+0x720:fixture+0x760,read?0u:1u,0x16,4});
    require(stream!=0,"Original private shortlist stream open");s.registers=registers;s.wr<4>(16,stream);return stream;
}
// Bound memory while observing the full population: hash the same canonical
// ordered event bytes as Effects, including payload bytes for reads and writes.
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
std::unique_ptr<ActorAttributesIoPair> paired_backing(const State& input){
    // The main preparation has already created the archive. Every created file
    // must exist in this baseline; no full-operation probe is needed per case.
    auto pair=std::make_unique<ActorAttributesIoPair>(input,0);static unsigned serial=0;
    pair->root=input.platform->writable_directory().parent_path()/("shortlist-io-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(++serial));
    require(!std::filesystem::exists(pair->root),"Fresh private shortlist IO pair");pair->working=pair->root/"working";ActorAttributesIoPair::copy(input.platform->writable_directory(),pair->working);
    pair->baseline=ActorAttributesIoPair::files(pair->working);for(const auto& [name,bytes]:pair->baseline){(void)bytes;pair->times.emplace(name,std::filesystem::last_write_time(pair->working/name));}
    pair->original=std::make_shared<NativePlatform>(input.platform->game_directory(),pair->working,AudioMode::offline);pair->modern=std::make_shared<NativePlatform>(input.platform->game_directory(),pair->working,AudioMode::offline);return pair;
}
std::array<std::set<std::uint32_t>,roots.size()> runtime_sites;
struct Witness:ClubShortlistStorageBoundaryObserver {
    std::uint32_t negative_root=0;
    void enter(const State&,std::uint32_t,bool)override{}
    void damage(State& s,std::uint32_t root,bool original,bool returned){if(root==negative_root&&!original&&returned)s.wr<4>(12,s.r<4>(12)^1);}
    void leave(State& s,std::uint32_t root,bool original,ClubShortlistStorageOutcome result)override{damage(s,root,original,result==ClubShortlistStorageOutcome::returned);}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed>=s.budget)return;for(unsigned n=0;n<roots.size();++n)if(pc>=roots[n]&&pc<ends[n])runtime_sites[n].insert(pc);}
};
void select(State& s,unsigned index,Mode mode){auto& d=club_shortlist_storage_diagnostics(s);if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;}

std::string execute(State& s,unsigned index,Path path,unsigned limit,std::uint64_t& steps){try{
    const auto stream=prepare_stream(s,index);s.pc=roots[index];s.wr<4>(4128,return_sentinel);s.budget=s.executed+limit;const auto before=s.executed;
    try{if(path==Path::direct){try_modern_club_shortlist_storage(s,roots[index]);}else{if(path==Path::cfm){s.memory.write<4>(fixture+0x400,roots[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}run_until(s,return_sentinel);}}catch(...){steps=s.executed-before;throw;}
    steps=s.executed-before;
    if(stream){const auto result=s.registers;s.memory.write<4>(fixture+0x740,stream);setup(s,0x10747900,{fixture+0x740});require(!s.memory.read<4>(fixture+0x740),"Original private shortlist archive close");s.registers=result;}
}catch(const std::exception& e){return e.what();}return {};}
unsigned cases=0,returns=0,budgets=0,negatives=0,faults=0;
void between(ActorAttributesIoPair& io){for(const auto& [guest,native]:io.old_forks.references){(void)guest;require(io.old_forks.forks.close(native)==0,"Release original transfer native fork");}io.old_forks.references.clear();io.between();}
std::uint64_t compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=full_limit,bool negative=false,bool fault=false){
    auto backing=paired_backing(input);auto& io=*backing;
    auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->pc=roots[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+full_limit;if(fault)s->wr<4>(index==3?8:12,0xf0000000);}
    original_contracts(*a);select(*b,index,mode);Witness witness;if(negative)witness.negative_root=roots[index];if(mode==Mode::modern){club_shortlist_storage_diagnostics(*b).observer=&witness;}
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    DigestEffects x,y;a->memory.date_observer=&x;b->memory.date_observer=&y;std::uint64_t old_steps=0,new_steps=0;
    const auto expected=execute(*a,index,path,limit,old_steps);a->platform.reset();between(io);const auto actual=execute(*b,index,path,limit,new_steps);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;club_shortlist_storage_diagnostics(*b).observer=nullptr;
    const auto label="Club shortlist storage runtime root="+hex_address(roots[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(fault?expected.starts_with("Unmapped data access at "):limit==full_limit?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected outcome: "+expected);
    require(old_clock.count==new_clock.count&&old_clock.events==new_clock.events,label+" host-input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative not discriminating");++negatives;}else{Image(*a).compare(*b,label);require(x.events==y.events&&x.bytes==y.bytes&&x.digest()==y.digest()&&a->executed==b->executed&&old_steps==new_steps,label+" ordered effects/accounting");}
    io.compare();const auto audit=[&](const auto& d,unsigned n){require(!d.original_body&&!d.dispatch_escapes,label+" leaked original scope or dispatch escape");if(mode==Mode::modern){require(d.calls[n]>=1&&!d.original_calls[n]&&!d.profile_original_calls[n],label+" dedicated modern call");require(d.returns[n]+d.faults[n]+d.budget_stops[n]==d.calls[n],label+" balanced productive outcomes");require(expected.empty()?d.returns[n]==d.calls[n]:fault?d.faults[n]>=1&&!d.budget_stops[n]:d.budget_stops[n]>=1&&!d.faults[n],label+" productive outcome counters");}else{require(!d.calls[n]&&!d.returns[n],label+" original control entered C++");require(mode==Mode::profile?d.profile_original_calls[n]>=1:d.original_calls[n]>=1,label+" original control not reached");}};
    audit(club_shortlist_storage_diagnostics(*b),index);
    if(index==2&&mode==Mode::modern&&!fault&&limit==full_limit){const auto& child=shortlist_category_diagnostics(*b);require(child.calls[0]>0&&child.calls[0]==child.returns[0]&&!child.faults[0]&&!child.original_calls[0]&&!child.original_body&&!child.dispatch_escapes,label+" actual category child coverage");std::cout<<"Full-population category child calls="<<child.calls[0]<<std::endl;}
    require(!club_shortlist_storage_diagnostics(*b).original_body&&!transfer_record_diagnostics(*b).original_body&&!transfer_valuation_diagnostics(*b).original_body,label+" nested original scope residue");
    std::cout<<"case="<<cases<<" root="<<hex_address(roots[index])<<" mode="<<unsigned(mode)<<" steps="<<old_steps<<" effect_events="<<x.events<<" effect_bytes="<<x.bytes<<std::endl;
    ++cases;returns+=expected.empty();budgets+=!fault&&!expected.empty();faults+=fault;return old_steps;
}
void matrix(const State& input,unsigned index,bool profile){
    if(profile){compare(input,index,Path::generated,Mode::profile);compare(input,index,Path::cfm,Mode::profile);return;}
    std::uint64_t steps=0;for(auto path:{Path::direct,Path::generated,Path::cfm}){const auto measured=compare(input,index,path,Mode::modern);if(path==Path::generated)steps=measured;}
    compare(input,index,Path::generated,Mode::original);compare(input,index,Path::generated,Mode::entry_original);compare(input,index,Path::generated,Mode::modern,full_limit,true);
    require(steps>=2,"Measured club shortlist storage path");for(auto cut:std::set<std::uint64_t>{0,steps/2,steps-1})compare(input,index,Path::generated,Mode::modern,unsigned(cut));
    compare(input,index,Path::generated,Mode::modern,full_limit,false,true);std::cout<<"PASS: club shortlist storage runtime root="<<hex_address(roots[index])<<" sites="<<runtime_sites[index].size()<<std::endl;
}
std::uint32_t setup(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args){
    std::array<bool*,15> selected{&shortlist_policy_diagnostics(s).original,&shortlist_quotation_diagnostics(s).original,&shortlist_assessment_diagnostics(s).original,&shortlist_recruitment_diagnostics(s).original,&shortlist_links_diagnostics(s).original,&shortlist_category_diagnostics(s).original,&club_shortlist_storage_diagnostics(s).original,&contract_query_diagnostics(s).original,&contract_storage_diagnostics(s).original,&contract_terms_diagnostics(s).original,&contract_review_diagnostics(s).original,&transfer_offer_diagnostics(s).original,&transfer_record_diagnostics(s).original,&transfer_valuation_diagnostics(s).original,&transfer_synthesis_diagnostics(s).original};
    struct Restore{std::array<bool*,15> flags;std::array<bool,15> before{};explicit Restore(std::array<bool*,15> f):flags(f){for(unsigned n=0;n<flags.size();++n){before[n]=*flags[n];*flags[n]=true;}}~Restore(){for(unsigned n=0;n<flags.size();++n)*flags[n]=before[n];}}restore{selected};
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);s.pc=entry;s.wr<4>(4128,return_sentinel);s.budget=s.executed+full_limit;
    try{run_until(s,return_sentinel);}catch(const std::exception& e){if(auto* clock=dynamic_cast<Clock*>(s.code_fragment_observer)){const auto first=clock->events.size()>16?clock->events.size()-16:0;for(auto n=first;n<clock->events.size();++n){const auto& event=clock->events[n];std::uint32_t value=0;for(unsigned b=12;b<16;++b)value=(value<<8)|event.registers[b];std::cerr<<"Setup import "<<event.name<<" leave="<<event.leave<<" r3="<<hex_address(value)<<std::endl;}}throw std::runtime_error("Transfer setup entry="+hex_address(entry)+" pc="+hex_address(s.pc)+" reason="+e.what());}return std::uint32_t(s.r<4>(12));
}
struct OwnedBacking {std::filesystem::path path;~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(path,e)||e||std::filesystem::is_symlink(path,e)||e)return;for(const auto& p:std::filesystem::recursive_directory_iterator(path,e))if(e||p.is_symlink(e)||e)return;std::filesystem::remove_all(path,e);}};
}
int main(int argc,char** argv){try{
    require(argc==5||(argc==6&&(std::string_view(argv[5])=="--profile"||std::string_view(argv[5])=="--probe"||std::string_view(argv[5])=="--category-seam")),"Expected export, game, backing and original prepared snapshot, optionally --profile or --probe");
    {DigestEffects a,b;const std::array<std::uint8_t,2> first{1,2},second{1,3};a.written(0x1000,first.data(),first.size());b.written(0x1000,second.data(),second.size());require(a.events==b.events&&a.bytes==b.bytes&&a.digest()!=b.digest(),"Ordered-effect digest discriminates differing payload bytes");}
    const bool profile=argc==6&&std::string_view(argv[5])=="--profile",probe=argc==6&&std::string_view(argv[5])=="--probe";require(profile==match_pc_profile_enabled(),"Shortlist profile flag mismatch");set_match_turbo_enabled(false);
    const auto directory=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(directory),"New private shortlist backing");OwnedBacking cleanup{directory};
    const auto snapshots=std::filesystem::path(argv[4]).parent_path();std::ifstream pointer(snapshots/"prepared-backing.txt");std::string name;std::getline(pointer,name);require(std::filesystem::path(name).parent_path().empty()&&name.starts_with("prepared-backing-"),"Retained original backing name");ActorAttributesIoPair::copy(snapshots/name,directory);
    State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],directory,AudioMode::offline);load_match_snapshot(source,argv[4]);Clock clock;source.code_fragment_observer=&clock;
    const auto toc=std::uint32_t(source.r<4>(8)),count=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be4))),staff_base=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be0)));
    std::uint32_t player=0;for(unsigned n=0;n<count;++n){const auto staff=staff_base+n*0x70;if(source.memory.read<4>(staff+0x24)&&source.memory.read<4>(staff+0x14)){player=staff;break;}}
    require(count==153796&&player,"Actual full staff population and club player");const auto manager=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7b7c)));require(manager!=0,"Original contract manager");
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

    source.memory.map(fixture,0x10000);const auto transfer=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7b40))),object=std::uint32_t(source.memory.read<4>(transfer+0x988));require(object!=0,"Original transfer manager's shortlist storage");
    const auto string=[&](std::uint32_t at,std::string_view text){source.memory.copy_in(at,{text.begin(),text.end()});source.memory.write<1>(at+std::uint32_t(text.size()),0);};
    string(fixture+0x600,"CM User Data:club-shortlist-storage.dat");string(fixture+0x720,"shortlist-reference");string(fixture+0x760,"shortlist-candidate");
    std::uint64_t category_entries=0;for(unsigned n=0;n<16;++n)category_entries+=source.memory.read<4>(object+0x98+n*4);
    const auto club_count=source.memory.read<4>(source.memory.read<4>(toc-0x7b50));
    std::cout<<"Original shortlist dimensions staff="<<count<<" clubs="<<club_count<<" categoryEntries="<<category_entries<<" capacity="<<source.memory.read<4>(object+0xd8)<<" pairs="<<source.memory.read<4>(object+0x10)<<" logicalBytes="<<(4+source.memory.read<4>(object+0x10)*8+64+4+category_entries*24+club_count*(12+40*44))<<std::endl;
    if(argc==6&&std::string_view(argv[5])=="--category-seam"){
        auto input=clone(source);input->platform=source.platform;input->wr<4>(12,object);
        compare(*input,2,Path::generated,Mode::modern);
        std::cout<<"PASS: full-population shortlist reset/category seam cases="<<cases<<std::endl;return 0;
    }
    const auto stream=setup(source,0x107475b0,{fixture+0x600,fixture+0x720,1,0x16,4});require(stream!=0,"Original shortlist archive open");setup(source,roots[6],{object,stream});source.memory.write<4>(fixture+0x740,stream);setup(source,0x10747900,{fixture+0x740});require(!source.memory.read<4>(fixture+0x740),"Original shortlist archive flushed");
    require(std::filesystem::file_size(directory/"club-shortlist-storage.dat")>1000,"Original complete shortlist archive bytes");
    const auto fresh=setup(source,0x10773520,{0xe0}),auxiliary=setup(source,0x10773520,{0x2c});require(fresh&&auxiliary,"Original shortlist object allocations");
    const auto check=[&](const State& base,unsigned index,std::initializer_list<std::uint32_t> args,bool complete=true){auto s=clone(base);s->platform=base.platform;unsigned n=3;for(auto value:args)s->wr<4>(n++*4,value);if(probe)compare(*s,index,Path::generated,Mode::modern);else if(complete)matrix(*s,index,profile);else compare(*s,index,Path::generated,profile?Mode::profile:Mode::modern);};
    std::cout<<"Prepared original shortlist world staff="<<count<<" clubs="<<source.memory.read<4>(source.memory.read<4>(toc-0x7b50))<<" capacity="<<source.memory.read<4>(object+0xd8)<<" archiveBytes="<<std::filesystem::file_size(directory/"club-shortlist-storage.dat")<<std::endl;
    check(source,0,{fresh,0});check(source,1,{object,0});check(source,2,{object,0});check(source,3,{10});
    // Reproduce the original constructor prelude and stop at its reader call.
    // No table layout or reader destination is invented by the harness.
    auto load_source=clone(source);load_source->platform=source.platform;load_source->wr<4>(12,fresh);load_source->wr<4>(16,1);load_source->pc=roots[0];load_source->wr<4>(4128,return_sentinel);load_source->budget=load_source->executed+full_limit;club_shortlist_storage_diagnostics(*load_source).original_body=roots[0];run_until(*load_source,roots[4]);club_shortlist_storage_diagnostics(*load_source).original_body=0;require(load_source->pc==roots[4],"Original shortlist reader call state");
    check(*load_source,4,{fresh,0});check(source,5,{auxiliary});check(source,6,{object,0});
    if(!probe){check(source,0,{fresh,1},false);check(source,1,{object,1},false);check(source,3,{0},false);check(source,3,{65536},false);}
    if(!profile){std::ofstream out(std::filesystem::path(argv[3]).parent_path()/"club-shortlist-storage-runtime-sites.txt");for(unsigned n=0;n<roots.size();++n){require(!runtime_sites[n].empty(),"Every shortlist root has actual-service sites");for(auto pc:runtime_sites[n])out<<n<<" "<<hex_address(pc)<<'\n';}}
    std::cout<<"PASS: club shortlist storage production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
