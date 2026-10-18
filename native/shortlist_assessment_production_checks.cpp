#include "contract_terms_adapter.hpp"
#include "contract_review_adapter.hpp"
#include "transfer_offer_adapter.hpp"
#include "transfer_record_adapter.hpp"
#include "transfer_valuation_adapter.hpp"
#include "transfer_synthesis_adapter.hpp"
#include "club_shortlist_storage_adapter.hpp"
#include "shortlist_category_adapter.hpp"
#include "shortlist_links_adapter.hpp"
#include "shortlist_assessment_adapter.hpp"
#include "shortlist_quotation_adapter.hpp"
#include "shortlist_policy_adapter.hpp"
#include "profile_qualification_adapter.hpp"
#include "shortlist_recruitment_adapter.hpp"
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
constexpr auto roots=shortlist_assessment::entries;
constexpr std::array<std::uint32_t,2> ends{0x10362d6c,0x1036d238};
constexpr unsigned full_limit=4000000000u;
enum class Path {direct,generated,cfm};
enum class Mode {modern,original,entry_original,profile};
void original_contracts(State& s){profile_qualification_diagnostics(s).original=true;shortlist_policy_diagnostics(s).original=true;shortlist_quotation_diagnostics(s).original=true;shortlist_assessment_diagnostics(s).original=true;shortlist_recruitment_diagnostics(s).original=true;shortlist_links_diagnostics(s).original=true;shortlist_category_diagnostics(s).original=true;club_shortlist_storage_diagnostics(s).original=true;contract_query_diagnostics(s).original=true;contract_storage_diagnostics(s).original=true;contract_terms_diagnostics(s).original=true;contract_review_diagnostics(s).original=true;transfer_offer_diagnostics(s).original=true;transfer_record_diagnostics(s).original=true;transfer_valuation_diagnostics(s).original=true;transfer_synthesis_diagnostics(s).original=true;}

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

std::uint32_t setup(State&,std::uint32_t,std::initializer_list<std::uint32_t>,std::uint32_t stop=return_sentinel);
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
    // The original preparation owns the baseline. Every observed file
    // must exist in this baseline; no full-operation probe is needed per case.
    auto pair=std::make_unique<ActorAttributesIoPair>(input,0);static unsigned serial=0;
    pair->root=input.platform->writable_directory().parent_path()/("shortlist-io-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(++serial));
    require(!std::filesystem::exists(pair->root),"Fresh private shortlist IO pair");pair->working=pair->root/"working";ActorAttributesIoPair::copy(input.platform->writable_directory(),pair->working);
    pair->baseline=ActorAttributesIoPair::files(pair->working);for(const auto& [name,bytes]:pair->baseline){(void)bytes;pair->times.emplace(name,std::filesystem::last_write_time(pair->working/name));}
    pair->original=std::make_shared<NativePlatform>(input.platform->game_directory(),pair->working,AudioMode::offline);pair->modern=std::make_shared<NativePlatform>(input.platform->game_directory(),pair->working,AudioMode::offline);return pair;
}
std::array<std::set<std::uint32_t>,roots.size()> runtime_sites;
struct Witness:ShortlistAssessmentBoundaryObserver {
    std::uint32_t negative_root=0;
    void enter(const State&,std::uint32_t,bool)override{}
    void damage(State& s,std::uint32_t root,bool original,bool returned){if(root==negative_root&&!original&&returned){if(root==roots[0])s.wr<4>(12,s.r<4>(12)^1);else s.wr<8>(0x4010,s.r<8>(0x4010)^1);}}
    void leave(State& s,std::uint32_t root,bool original,ShortlistAssessmentOutcome result)override{damage(s,root,original,result==ShortlistAssessmentOutcome::returned);}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed>=s.budget)return;for(unsigned n=0;n<roots.size();++n)if(pc>=roots[n]&&pc<ends[n])runtime_sites[n].insert(pc);}
};
void select(State& s,unsigned index,Mode mode){auto& d=shortlist_assessment_diagnostics(s);if(mode==Mode::original)d.original=true;if(mode==Mode::entry_original)d.entry_original[index]=true;}

std::string execute(State& s,unsigned index,Path path,unsigned limit,std::uint64_t& steps){try{
    s.pc=roots[index];s.wr<4>(4128,return_sentinel);s.budget=s.executed+limit;const auto before=s.executed;
    try{if(path==Path::direct){try_modern_shortlist_assessment(s,roots[index]);}else{if(path==Path::cfm){s.memory.write<4>(fixture+0x400,roots[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}run_until(s,return_sentinel);}}catch(...){steps=s.executed-before;throw;}
    steps=s.executed-before;

}catch(const std::exception& e){return e.what();}return {};}
unsigned cases=0,returns=0,budgets=0,negatives=0,faults=0,window_faults=0;
std::uint64_t quotation_child_returns=0;
void between(ActorAttributesIoPair& io){for(const auto& [guest,native]:io.old_forks.references){(void)guest;require(io.old_forks.forks.close(native)==0,"Release original transfer native fork");}io.old_forks.references.clear();io.between();}
std::uint64_t compare(const State& input,unsigned index,Path path,Mode mode,unsigned limit=full_limit,bool negative=false,bool fault=false,bool window_absent=false){
    auto backing=paired_backing(input);auto& io=*backing;
    auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){s->pc=roots[index];s->wr<4>(4128,return_sentinel);s->budget=s->executed+full_limit;if(fault)s->wr<4>(8,0xf0000000);}
    original_contracts(*a);select(*b,index,mode);Witness witness;if(negative)witness.negative_root=roots[index];if(mode==Mode::modern){shortlist_assessment_diagnostics(*b).observer=&witness;}
    Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
    DigestEffects x,y;a->memory.date_observer=&x;b->memory.date_observer=&y;std::uint64_t old_steps=0,new_steps=0;
    const auto expected=execute(*a,index,path,limit,old_steps);a->platform.reset();between(io);const auto actual=execute(*b,index,path,limit,new_steps);
    a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;shortlist_assessment_diagnostics(*b).observer=nullptr;
    const auto label="Shortlist assessment runtime root="+hex_address(roots[index])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
    require(expected==actual,label+" original="+expected+" modern="+actual);
    // The management snapshot carries a guest window token without a live host
    // registry. This one diagnostic case must stop identically at that exact
    // import, rather than silently treating an arbitrary service fault as a return.
    const bool absent_window_stop=window_absent&&expected.starts_with("map::at")&&!old_clock.events.empty()&&old_clock.events.back().name=="CarbonLib::GetWindowPort"&&!old_clock.events.back().leave;
    require(window_absent?absent_window_stop:fault?expected.starts_with("Unmapped data access at "):limit==full_limit?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected outcome: "+expected+" pc="+hex_address(a->pc)+" lastImport="+(old_clock.events.empty()?"none":old_clock.events.back().name));
    require(old_clock.count==new_clock.count&&old_clock.events==new_clock.events,label+" host-input/import ABI");
    if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative not discriminating");++negatives;}else{Image(*a).compare(*b,label);require(x.events==y.events&&x.bytes==y.bytes&&x.digest()==y.digest()&&a->executed==b->executed&&old_steps==new_steps,label+" ordered effects/accounting");}
    if(mode==Mode::modern&&expected.empty()&&!negative)quotation_child_returns+=shortlist_quotation_diagnostics(*b).returns[1];
    io.compare();const auto audit=[&](const auto& d,unsigned n){require(!d.original_body&&!d.dispatch_escapes,label+" leaked original scope or dispatch escape");if(mode==Mode::modern){require(d.calls[n]>=1&&!d.original_calls[n]&&!d.profile_original_calls[n],label+" dedicated modern call");require(d.returns[n]+d.faults[n]+d.budget_stops[n]==d.calls[n],label+" balanced productive outcomes");require(expected.empty()?d.returns[n]==d.calls[n]:(fault||window_absent)?d.faults[n]>=1&&!d.budget_stops[n]:d.budget_stops[n]>=1&&!d.faults[n],label+" productive outcome counters");}else{require(!d.calls[n]&&!d.returns[n],label+" original control entered C++");require(mode==Mode::profile?d.profile_original_calls[n]>=1:d.original_calls[n]>=1,label+" original control not reached");}};
    audit(shortlist_assessment_diagnostics(*b),index);

    require(!shortlist_assessment_diagnostics(*b).original_body&&!transfer_record_diagnostics(*b).original_body&&!transfer_valuation_diagnostics(*b).original_body,label+" nested original scope residue");
    std::cout<<"case="<<cases<<" root="<<hex_address(roots[index])<<" mode="<<unsigned(mode)<<" steps="<<old_steps<<" effect_events="<<x.events<<" effect_bytes="<<x.bytes<<" result="<<hex_address(std::uint32_t(a->r<4>(12)))<<" score_bits="<<hex_address(std::uint32_t(a->r<8>(0x4010)>>32))<<" assessment_calls="<<shortlist_assessment_diagnostics(*b).calls[index]<<std::endl;
    if(window_absent)std::cout<<"PREREQUISITE STOP: original and modern diagnostic agree at CarbonLib::GetWindowPort; no live window registry in the snapshot"<<std::endl;
    ++cases;returns+=expected.empty();budgets+=!fault&&!window_absent&&!expected.empty();faults+=fault||window_absent;window_faults+=window_absent;return old_steps;
}
void matrix(const State& input,unsigned index,bool profile){
    if(profile){compare(input,index,Path::generated,Mode::profile);compare(input,index,Path::cfm,Mode::profile);return;}
    std::uint64_t steps=0;for(auto path:{Path::direct,Path::generated,Path::cfm}){const auto measured=compare(input,index,path,Mode::modern);if(path==Path::generated)steps=measured;}
    compare(input,index,Path::generated,Mode::original);compare(input,index,Path::generated,Mode::entry_original);compare(input,index,Path::generated,Mode::modern,full_limit,true);
    require(steps>=2,"Measured shortlist assessment path");for(auto cut:std::set<std::uint64_t>{0,steps/2,steps-1})compare(input,index,Path::generated,Mode::modern,unsigned(cut));
    compare(input,index,Path::generated,Mode::modern,full_limit,false,true);std::cout<<"PASS: shortlist assessment runtime root="<<hex_address(roots[index])<<" sites="<<runtime_sites[index].size()<<std::endl;
}
std::uint32_t setup(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args,std::uint32_t stop){
    std::array<bool*,16> selected{&profile_qualification_diagnostics(s).original,&shortlist_policy_diagnostics(s).original,&shortlist_quotation_diagnostics(s).original,&shortlist_assessment_diagnostics(s).original,&shortlist_recruitment_diagnostics(s).original,&shortlist_links_diagnostics(s).original,&shortlist_category_diagnostics(s).original,&club_shortlist_storage_diagnostics(s).original,&contract_query_diagnostics(s).original,&contract_storage_diagnostics(s).original,&contract_terms_diagnostics(s).original,&contract_review_diagnostics(s).original,&transfer_offer_diagnostics(s).original,&transfer_record_diagnostics(s).original,&transfer_valuation_diagnostics(s).original,&transfer_synthesis_diagnostics(s).original};
    struct Restore{std::array<bool*,16> flags;std::array<bool,16> before{};explicit Restore(std::array<bool*,16> f):flags(f){for(unsigned n=0;n<flags.size();++n){before[n]=*flags[n];*flags[n]=true;}}~Restore(){for(unsigned n=0;n<flags.size();++n)*flags[n]=before[n];}}restore{selected};
    unsigned n=3;for(auto value:args)s.wr<4>(n++*4,value);s.pc=entry;s.wr<4>(4128,return_sentinel);s.budget=s.executed+full_limit;
    // Generated pages may run through an interior dispatch target. Arm the
    // normal instruction budget at the final loop condition to stop exactly
    // before the next career step; every initializer instruction still runs.
    struct SpanEnd final:MemoryObserver {
        State& state;unsigned count;
        SpanEnd(State& s,unsigned n):state(s),count(n){}
        void mapped(std::uint32_t,std::uint64_t)override{}
        void written(std::uint32_t,const std::uint8_t*,std::size_t)override{}
        void cleared(std::uint32_t,std::uint64_t)override{}
        void read(std::uint32_t,const std::uint8_t*,unsigned)override {
            if(state.pc==0x1034411c&&state.r<4>(28)==count)state.budget=state.executed+3;
        }
    } span_end{s,stop==return_sentinel?0u:unsigned(s.memory.read<4>(s.memory.read<4>(s.r<4>(8)-0x7b50)))};
    struct RestoreObserver{State& state;MemoryObserver* previous;~RestoreObserver(){state.memory.date_observer=previous;}} observer{s,s.memory.date_observer};
    if(stop!=return_sentinel){require(entry==0x10344068&&stop==0x1034412c&&!observer.previous,"Only the characterized original club-table span may use the bounded setup stop");s.memory.date_observer=&span_end;}
    try{try{run_until(s,stop);}catch(const std::runtime_error& e){if(!(stop!=return_sentinel&&s.pc==stop&&std::string_view(e.what())=="Native instruction budget exhausted"))throw;}}catch(const std::exception& e){if(auto* clock=dynamic_cast<Clock*>(s.code_fragment_observer)){const auto first=clock->events.size()>16?clock->events.size()-16:0;for(auto n=first;n<clock->events.size();++n){const auto& event=clock->events[n];std::uint32_t value=0;for(unsigned b=12;b<16;++b)value=(value<<8)|event.registers[b];std::cerr<<"Setup import "<<event.name<<" leave="<<event.leave<<" r3="<<hex_address(value)<<std::endl;}}throw std::runtime_error("Transfer setup entry="+hex_address(entry)+" pc="+hex_address(s.pc)+" reason="+e.what());}return std::uint32_t(s.r<4>(12));
}
struct OwnedBacking {std::filesystem::path path;~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(path,e)||e||std::filesystem::is_symlink(path,e)||e)return;for(const auto& p:std::filesystem::recursive_directory_iterator(path,e))if(e||p.is_symlink(e)||e)return;std::filesystem::remove_all(path,e);}};
}
int main(int argc,char** argv){try{
    require(argc==5||(argc==6&&(std::string_view(argv[5])=="--profile"||std::string_view(argv[5])=="--probe")),"Expected export, game, private backing and original prepared snapshot, optionally --profile or --probe");
    const bool profile=argc==6&&std::string_view(argv[5])=="--profile",probe=argc==6&&std::string_view(argv[5])=="--probe";require(profile==match_pc_profile_enabled(),"Shortlist assessment profile flag mismatch");set_match_turbo_enabled(false);
    const auto directory=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(directory),"New private recruitment backing");OwnedBacking cleanup{directory};
    const auto snapshots=std::filesystem::path(argv[4]).parent_path();std::ifstream pointer(snapshots/"prepared-backing.txt");std::string name;std::getline(pointer,name);require(std::filesystem::path(name).parent_path().empty()&&name.starts_with("prepared-backing-"),"Retained original backing name");ActorAttributesIoPair::copy(snapshots/name,directory);
    State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],directory,AudioMode::offline);load_match_snapshot(source,argv[4]);Clock clock;source.code_fragment_observer=&clock;
    const auto toc=std::uint32_t(source.r<4>(8)),count=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be4))),staff_base=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be0)));
    require(count==153796,"Complete original staff population");std::uint32_t player=0,nonplayer=0;
    for(unsigned n=0;n<count;++n){const auto staff=staff_base+n*0x70;if(!source.memory.read<4>(staff+0x14)&&!nonplayer)nonplayer=staff;if(source.memory.read<4>(staff+0x24)&&source.memory.read<4>(staff+0x14)&&source.memory.read<1>(staff+0x62)==11&&!player)player=staff;}
    require(player&&nonplayer,"Actual player and absent-profile staff");const auto club=std::uint32_t(source.memory.read<4>(player+0x24)),manager=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7b7c)));require(manager,"Original contract manager");
    // Career constructors and publication sequences retained from the original.
    for(auto spec:std::array<std::array<unsigned,3>,5>{{{0x7b3c,0x48,0x10301540},{0x7bf0,0x20,0x102cb930},{0x7b40,0xa7c,0x103df0d0},{0x7ab8,4,0x100f84e0},{0x7b24,0x10,0x100031b0}}}){
        const auto slot=std::uint32_t(source.memory.read<4>(toc-spec[0]));if(!source.memory.read<4>(slot)){const auto allocation=setup(source,0x10773520,{spec[1]});require(allocation,"Original link-service allocation");require(setup(source,spec[2],{allocation,0})==allocation,"Original link-service constructor");source.memory.write<4>(slot,allocation);}
    }
    const auto value=setup(source,0x10084be0,{manager,player,club,1,0xffffffff,0});if(!setup(source,0x10082f20,{manager,player}))setup(source,0x10078e70,{manager,player,value});const auto record=setup(source,0x10082f20,{manager,player});require(record&&source.memory.read<4>(record+4)==source.memory.read<4>(club),"Original primary contract binding");source.memory.map(fixture,0x10000);
    const auto transfer=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7b40))),object=std::uint32_t(source.memory.read<4>(transfer+0x988)),clubs=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7be8))),club_count=std::uint32_t(source.memory.read<4>(source.memory.read<4>(toc-0x7b50)));
    require(object&&club_count==11019,"Original shortlist object and complete club population");std::array<std::uint32_t,3> buyers{};unsigned found=0;
    // This table belongs to the career initialization sequence rather than a
    // standalone constructor. Execute its original allocation, publication and
    // complete loop, stopping before the following career initialization step.
    const auto table_slot=std::uint32_t(source.memory.read<4>(toc-0x7984));if(!source.memory.read<4>(table_slot))setup(source,0x10344068,{},0x1034412c);
    const auto club_metadata=std::uint32_t(source.memory.read<4>(table_slot));require(club_metadata&&source.memory.read<4>(source.memory.read<4>(toc-0x62dc))==club_count,"Original per-club table allocation and count publication");
    for(unsigned n=0;n<club_count;++n)require(source.memory.read<4>(club_metadata+n*8)==0xffffffff&&!source.memory.read<1>(club_metadata+n*8+4)&&!source.memory.read<1>(club_metadata+n*8+5),"Original per-club initialization loop defaults");
    // Allocation/publication at 0x1034390c..0x10343924, followed by the
    // original ordering service at 0x10343988. Preserve its actual ordering.
    const auto order_slot=std::uint32_t(source.memory.read<4>(toc-0x765c));if(!source.memory.read<4>(order_slot)){const auto allocation=setup(source,0x10775130,{club_count*4});require(allocation,"Original club-order allocation");source.memory.write<4>(order_slot,allocation);setup(source,0x100cae30,{1});}
    const auto club_order=std::uint32_t(source.memory.read<4>(order_slot));std::set<std::uint32_t> ordered_ids;for(unsigned n=0;n<club_count;++n)ordered_ids.insert(std::uint32_t(source.memory.read<4>(club_order+n*4)));require(ordered_ids.size()==club_count&&*ordered_ids.begin()==0&&*ordered_ids.rbegin()==club_count-1,"Original club-order service covers the full population exactly once");
    for(unsigned n=0;n<club_count&&found<buyers.size();++n){const auto candidate=clubs+std::uint32_t(source.memory.read<4>(club_order+n*4))*0x248;if(candidate!=club)buyers[found++]=candidate;}require(found==buyers.size(),"Actual distinct club records");
    const auto list=setup(source,0x10773520,{16});require(list&&setup(source,0x10738c00,{list})==list,"Original temporary output list");
    // Declared inputs in original-allocated records: enable three club entries
    // and one candidate row in each. No helper return values are substituted.
    const auto profile_pointer=std::uint32_t(source.memory.read<4>(player+0x14));
    const auto raw_roles=setup(source,0x10117490,{profile_pointer});
    std::uint32_t roles=raw_roles;if(roles&2)roles|=4;if(roles&8)roles|=16;if(roles&32)roles|=16;if(roles&128)roles|=256;if(roles&2048)roles|=1024;
    require(roles&&roles<=0xffff,"Actual service returns bounded candidate qualification bits");
    const auto today=std::uint32_t(source.memory.read<4>(toc-0x7b80));
    setup(source,0x10717b20,{fixture+0x1000,today,30});
    for(auto buyer:buyers){
        const auto id=std::uint32_t(source.memory.read<4>(buyer)),entry=std::uint32_t(source.memory.read<4>(object+0xdc))+id*12,row=std::uint32_t(source.memory.read<4>(entry));
        require(row,"Original forty-row array");source.memory.write<1>(entry+4,1);source.memory.write<1>(entry+9,0);
        source.memory.write<1>(row,11);source.memory.write<2>(row+2,roles);source.memory.write<4>(row+4,100);source.memory.write<4>(row+8,source.memory.read<4>(player+8)+1);
        for(unsigned offset:{0x10u,0x14u,0x18u})source.memory.write<4>(row+offset,0xffffffff);
        source.memory.copy_in(row+0x1c,source.memory.copy_out(fixture+0x1000,8));source.memory.write<1>(row+0x28,0);
    }

    const auto player_id=std::uint32_t(source.memory.read<4>(player));
    const auto buyer_row=[&](std::uint32_t buyer){const auto id=std::uint32_t(source.memory.read<4>(buyer)),entry=std::uint32_t(source.memory.read<4>(object+0xdc))+id*12;return std::uint32_t(source.memory.read<4>(entry));};
    const auto check=[&](const State& base,unsigned index,std::initializer_list<std::uint32_t> args,bool complete=true){auto state=clone(base);state->platform=base.platform;unsigned n=3;for(auto value:args)state->wr<4>(n++*4,value);if(probe)compare(*state,index,Path::generated,Mode::modern);else if(complete)matrix(*state,index,profile);else compare(*state,index,Path::generated,profile?Mode::profile:Mode::modern);};
    std::cout<<"Prepared original assessment world staff="<<count<<" clubs="<<club_count<<" playerID="<<player_id<<" rawRoles="<<raw_roles<<" controlledClubRows=3"<<std::endl;
    // A declared signed field avoids the original early nationality rejection.
    // A date prepared by the original subtraction service also avoids the
    // recent-date rejection. Preserve the untouched staff record separately; do not infer
    // an age or other UI meaning for this raw field.
    setup(source,0x10717b20,{fixture+0x1020,today,400});
    auto eligible=clone(source);eligible->platform=source.platform;eligible->memory.write<2>(player+0x5c,25);
    eligible->memory.copy_in(player+0x4c,source.memory.copy_out(fixture+0x1020,4));
    check(*eligible,0,{object,buyers[0],player,buyer_row(buyers[0]),fixture+0x3000,std::uint32_t(source.memory.read<4>(player+8)),1});
    check(*eligible,1,{object,player,buyers[0],1,0,0,fixture+0x3100});
    if(!probe){
        check(source,0,{object,buyers[0],player,buyer_row(buyers[0]),fixture+0x3000,std::uint32_t(source.memory.read<4>(player+8)),1},false);
        check(source,1,{object,player,buyers[0],1,0,0,fixture+0x3100},false);
        check(source,0,{object,buyers[1],player,buyer_row(buyers[1]),fixture+0x3000,0,0},false);
        check(source,0,{object,club,player,buyer_row(buyers[0]),fixture+0x3000,0xffffffff,1},false);
        check(source,1,{object,player,club,1,0,0,fixture+0x3100},false);
        check(source,1,{object,player,buyers[1],0,1,1,0},false);
        check(source,1,{object,nonplayer,buyers[0],1,0,0,fixture+0x3100},false);
        for(unsigned category:{2u,3u,4u,5u,6u,7u})check(source,1,{object,player,buyers[0],1,0,category,fixture+0x3100},false);
        auto altered=clone(source);altered->platform=source.platform;altered->memory.write<1>(buyer_row(buyers[0])+0x28,24);
        check(*altered,0,{object,buyers[0],player,buyer_row(buyers[0]),fixture+0x3000,0,1},false);
    }
    if(!profile){std::ofstream out(std::filesystem::path(argv[3]).parent_path()/"shortlist-assessment-runtime-sites.txt");for(unsigned n=0;n<roots.size();++n){require(!runtime_sites[n].empty(),"Every assessment root has actual-service sites");for(auto pc:runtime_sites[n])out<<n<<" "<<hex_address(pc)<<'\n';}}
    if(!profile)require(runtime_sites[1].contains(0x1036b92c),"Actual-service interest must reach financial-context calculation beyond the initial guards");
    if(!profile){require(quotation_child_returns,"Actual-service parent must complete modern reference quotation");std::cout<<"Completed modern quotation children="<<quotation_child_returns<<std::endl;}
    std::cout<<"PASS: shortlist assessment production cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
