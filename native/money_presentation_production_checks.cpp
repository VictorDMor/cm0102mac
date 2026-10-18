#include "contract_display_adapter.hpp"
#include "money_value_adapter.hpp"
#include "money_text_adapter.hpp"
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
constexpr std::array<std::uint32_t,13> roots{0x10372590,0x10372730,0x10009f20,0x1000a010,0x1000a060,0x1000a170,0x1000a270,0x1000a330,0x1000c770,0x10009ef0,0x1000b150,0x1000b160,0x1000b410};
constexpr std::array<std::uint32_t,13> ends{0x10372724,0x103727bc,0x1000a008,0x1000a05c,0x1000a164,0x1000a26c,0x1000a32c,0x1000a440,0x1000cbb4,0x10009f18,0x1000b158,0x1000b410,0x1000c764};
constexpr unsigned full_limit=50000000;
constexpr std::uint32_t amount=fixture+0x1000,output=fixture+0x2000,record=fixture+0x3000;
enum class Path {direct,generated,cfm};
enum class Mode {modern,original,entry_original,profile};
void originals(State& s,bool selected=true){contract_display_diagnostics(s).original=selected;money_value_diagnostics(s).original=selected;money_text_diagnostics(s).original=selected;}
struct Clock:CodeFragmentBoundaryObserver {
    unsigned count=0;ActorAttributesIoPair* io=nullptr;bool modern=false;
    struct Event {bool leave;std::string name;decltype(State::registers) registers;std::uint32_t pc;bool operator==(const Event&)const=default;};
    std::vector<Event> events;
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool)override{}
    void import_enter(const State& s,const char* name)override{events.push_back({false,name,s.registers,s.pc});}
    void import_leave(const State& s,const char* name)override{events.push_back({true,name,s.registers,s.pc});}
    bool import_fixture(State& s,const char* name)override {
        if(io && io->dispatch(s,name,modern))return true;
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Money presentation awaits native event input");
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),1000000000ULL+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};

// Bound memory while observing the full population: hash the same canonical
// ordered event bytes as Effects, including payload bytes for reads and writes.
struct DigestEffects final:MemoryObserver {
    State& state;CC_SHA256_CTX context{};std::array<std::uint8_t,65536> buffer{};std::size_t used=0;std::uint64_t events=0,bytes=0;
    explicit DigestEffects(State& s):state(s){CC_SHA256_Init(&context);}
    void append(const std::uint8_t* data,std::size_t size){bytes+=size;while(size){const auto n=std::min(size,buffer.size()-used);std::memcpy(buffer.data()+used,data,n);used+=n;data+=n;size-=n;if(used==buffer.size()){CC_SHA256_Update(&context,buffer.data(),CC_LONG(used));used=0;}}}
    void event(unsigned kind,std::uint32_t address,std::uint64_t size,const std::uint8_t* data=nullptr){++events;std::array<std::uint8_t,17> head{};head[0]=std::uint8_t(kind);for(unsigned n=0;n<4;++n)head[13+n]=std::uint8_t(state.pc>>((3-n)*8));for(unsigned n=0;n<4;++n)head[1+n]=std::uint8_t(address>>((3-n)*8));for(unsigned n=0;n<8;++n)head[5+n]=std::uint8_t(size>>((7-n)*8));append(head.data(),head.size());if(data)append(data,std::size_t(size));}
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
    pair->root=input.platform->writable_directory().parent_path()/("money-presentation-io-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(++serial));
    require(!std::filesystem::exists(pair->root),"Fresh private shortlist IO pair");pair->working=pair->root/"working";ActorAttributesIoPair::copy(input.platform->writable_directory(),pair->working);
    pair->baseline=ActorAttributesIoPair::files(pair->working);for(const auto& [name,bytes]:pair->baseline){(void)bytes;pair->times.emplace(name,std::filesystem::last_write_time(pair->working/name));}
    pair->original=std::make_shared<NativePlatform>(input.platform->game_directory(),pair->working,AudioMode::offline);pair->modern=std::make_shared<NativePlatform>(input.platform->game_directory(),pair->working,AudioMode::offline);return pair;
}

std::array<std::set<std::uint32_t>,roots.size()> runtime_sites,complete_sites;
struct Witness:ContractDisplayBoundaryObserver,MoneyValueBoundaryObserver,MoneyTextBoundaryObserver {
    std::uint32_t negative_root=0;
    std::set<std::uint32_t> seen;
    void enter(const State&,std::uint32_t,bool)override{}
    void returned(State& s,std::uint32_t root,bool original,bool returned){if(root==negative_root&&!original&&returned)s.wr<4>(12,s.r<4>(12)^1);}
    void leave(State& s,std::uint32_t root,bool original,ContractDisplayOutcome result)override{returned(s,root,original,result==ContractDisplayOutcome::returned);}
    void leave(State& s,std::uint32_t root,bool original,MoneyValueOutcome result)override{returned(s,root,original,result==MoneyValueOutcome::returned);}
    void leave(State& s,std::uint32_t root,bool original,MoneyTextOutcome result)override{returned(s,root,original,result==MoneyTextOutcome::returned);}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed>=s.budget)return;seen.insert(pc);for(unsigned n=0;n<roots.size();++n)if(roots[n]<=pc&&pc<ends[n])runtime_sites[n].insert(pc);}
};
void observe(State& s,Witness* w){contract_display_diagnostics(s).observer=w;money_value_diagnostics(s).observer=w;money_text_diagnostics(s).observer=w;}
void select(State& s,unsigned i,Mode mode){if(mode==Mode::original)originals(s);if(mode==Mode::entry_original){if(i<2)contract_display_diagnostics(s).entry_original[i]=true;else if(i<9)money_value_diagnostics(s).entry_original[i-2]=true;else money_text_diagnostics(s).entry_original[i-9]=true;}}
void direct(State& s,unsigned i){if(i<2)try_modern_contract_display(s,roots[i]);else if(i<9)try_modern_money_value(s,roots[i]);else try_modern_money_text(s,roots[i]);}
std::string execute(State& s,unsigned index,Path path,unsigned limit,std::uint64_t& steps){try{
 s.pc=roots[index];s.wr<4>(4128,return_sentinel);s.budget=s.executed+limit;const auto before=s.executed;
 try{if(path==Path::direct)direct(s,index);else{if(path==Path::cfm){s.memory.write<4>(fixture+0x400,roots[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}run_until(s,return_sentinel);}}catch(...){steps=s.executed-before;throw;}
 steps=s.executed-before;
}catch(const std::exception& e){return e.what();}return {};}
unsigned cases=0,returns=0,budgets=0,negatives=0,faults=0;
std::array<unsigned,13> root_returns{};
std::uint64_t wage_value_returns=0,wage_text_returns=0,wage_quantum_returns=0;
void between(ActorAttributesIoPair& io){for(const auto& [guest,native]:io.old_forks.references){(void)guest;require(io.old_forks.forks.close(native)==0,"Release original money native fork");}io.old_forks.references.clear();io.between();}
std::uint64_t compare(const State& input,unsigned i,Path path,Mode mode,unsigned limit=full_limit,bool negative=false,bool fault=false){
 auto backing=paired_backing(input);auto& io=*backing;auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
 if(fault){for(auto* s:{a.get(),b.get()}){if(i==9)s->memory.write<4>(s->r<4>(8)-0x59d0,0xf0000000);else if(i==10)s->wr<4>(8,0xf0000000);else if(i==8)s->memory.write<4>(s->r<4>(8)-0x59d4,0xf0000000);else s->wr<4>(12,0xf0000000);}}
 originals(*a);select(*b,i,mode);Witness witness;if(negative)witness.negative_root=roots[i];if(mode==Mode::modern)observe(*b,&witness);
 Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;new_clock.modern=true;a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;
 DigestEffects x{*a},y{*b};a->memory.date_observer=&x;b->memory.date_observer=&y;std::uint64_t old_steps=0,new_steps=0;
 const auto expected=execute(*a,i,path,limit,old_steps);a->platform.reset();between(io);const auto actual=execute(*b,i,path,limit,new_steps);
 a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;observe(*b,nullptr);
 const auto label="Money presentation root="+hex_address(roots[i])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
 require(expected==actual,label+" original="+expected+" modern="+actual);require(fault?expected.starts_with("Unmapped data access at "):limit==full_limit?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected outcome="+expected+" pc="+hex_address(a->pc));
 require(old_clock.count==new_clock.count&&old_clock.events==new_clock.events,label+" import ABI/input order");
 if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative must discriminate");++negatives;}
 else{Image(*a).compare(*b,label);require(x.events==y.events&&x.bytes==y.bytes&&x.digest()==y.digest()&&a->executed==b->executed&&old_steps==new_steps,label+" ordered timed effects/accounting");}
 io.compare();
 const auto audit=[&](const auto& d,unsigned n){require(!d.original_body&&!d.dispatch_escapes,label+" original scope/dispatch residue");if(mode==Mode::modern){require(d.calls[n]>=1&&!d.original_calls[n]&&!d.profile_original_calls[n],label+" productive root");require(d.calls[n]==d.returns[n]+d.faults[n]+d.budget_stops[n]+d.transfers[n]+d.terminations[n],label+" balanced outcomes");require(expected.empty()?d.returns[n]==d.calls[n]:fault?d.faults[n]>=1&&!d.budget_stops[n]:d.budget_stops[n]>=1&&!d.faults[n],label+" expected productive outcome");}else{require(!d.calls[n]&&!d.returns[n],label+" original control entered native root");require(mode==Mode::profile?d.profile_original_calls[n]>=1:d.original_calls[n]>=1,label+" missing original control");}};
 if(i<2)audit(contract_display_diagnostics(*b),i);else if(i<9)audit(money_value_diagnostics(*b),i-2);else audit(money_text_diagnostics(*b),i-9);
 for(auto pc:witness.seen)if(expected.empty()&&!negative)for(unsigned n=0;n<roots.size();++n)if(roots[n]<=pc&&pc<ends[n])complete_sites[n].insert(pc);
 if(i==0&&mode==Mode::modern&&expected.empty()&&!negative){const auto& v=money_value_diagnostics(*b);const auto& t=money_text_diagnostics(*b);require(v.returns[0]&&t.returns[3],label+" complete wage conversion and text children");wage_value_returns+=v.returns[0];wage_text_returns+=t.returns[3];wage_quantum_returns+=v.returns[6];}
 std::cout<<"case="<<cases<<" root="<<hex_address(roots[i])<<" mode="<<unsigned(mode)<<" steps="<<old_steps<<" events="<<x.events<<" result="<<hex_address(std::uint32_t(a->r<4>(12)))<<std::endl;
 ++cases;returns+=expected.empty();if(expected.empty()&&!negative)++root_returns[i];budgets+=!fault&&!expected.empty();faults+=fault;return old_steps;
}
void matrix(const State& input,unsigned i,bool profile){
 if(profile){compare(input,i,Path::generated,Mode::profile);compare(input,i,Path::cfm,Mode::profile);return;}
 std::uint64_t steps=0;for(auto path:{Path::direct,Path::generated,Path::cfm}){const auto n=compare(input,i,path,Mode::modern);if(path==Path::generated)steps=n;}
 compare(input,i,Path::generated,Mode::original);compare(input,i,Path::generated,Mode::entry_original);compare(input,i,Path::generated,Mode::modern,full_limit,true);
 for(auto cut:std::set<std::uint64_t>{0,steps/2,steps-1})compare(input,i,Path::generated,Mode::modern,unsigned(cut));compare(input,i,Path::generated,Mode::modern,full_limit,false,true);
}
std::uint32_t setup(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args){originals(s);unsigned n=3;for(auto v:args)s.wr<4>(n++*4,v);s.pc=entry;s.wr<4>(4128,return_sentinel);s.budget=s.executed+full_limit;run_until(s,return_sentinel);originals(s,false);return std::uint32_t(s.r<4>(12));}
struct OwnedBacking{std::filesystem::path path;~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(path,e)||e||std::filesystem::is_symlink(path,e)||e)return;for(const auto& p:std::filesystem::recursive_directory_iterator(path,e))if(e||p.is_symlink(e)||e)return;std::filesystem::remove_all(path,e);}};
}
int main(int argc,char** argv){try{
 require(argc==5||(argc==6&&(std::string_view(argv[5])=="--profile"||std::string_view(argv[5])=="--probe")),"Expected export,game,private backing,original prepared snapshot and optional profile/probe");const bool profile=argc==6&&std::string_view(argv[5])=="--profile",probe=argc==6&&std::string_view(argv[5])=="--probe";require(profile==match_pc_profile_enabled(),"Money presentation profile flag mismatch");set_match_turbo_enabled(false);
 const auto directory=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(directory),"Fresh private money backing");OwnedBacking cleanup{directory};const auto snapshots=std::filesystem::path(argv[4]).parent_path();std::ifstream pointer(snapshots/"prepared-backing.txt");std::string name;std::getline(pointer,name);require(std::filesystem::path(name).parent_path().empty()&&name.starts_with("prepared-backing-"),"Retained original backing name");ActorAttributesIoPair::copy(snapshots/name,directory);
 State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],directory,AudioMode::offline);load_match_snapshot(source,argv[4]);source.memory.map(fixture,0x10000);const auto toc=std::uint32_t(source.r<4>(8));require(toc==cm::semantic_io_checks::toc,"Original loaded TOC");
 require(setup(source,0x1000a440,{0})==1,"Original currency configuration completed");const auto preference_pointer=std::uint32_t(source.memory.read<4>(toc-0x7ac8)),preferences=std::uint32_t(source.memory.read<4>(preference_pointer));require(preferences,"Original preference object");source.memory.write<4>(preferences+4,0x200000);source.memory.copy_in(record,std::vector<std::uint8_t>(100,0));source.memory.write<4>(record+12,50000);source.memory.copy_in(record+0x30,source.memory.copy_out(std::uint32_t(source.memory.read<4>(toc-0x7b80)),8));source.memory.write<4>(amount,50000);
 const auto check=[&](const State& base,unsigned i,std::initializer_list<std::uint32_t> args,bool full=true){auto state=clone(base);state->platform=base.platform;unsigned n=3;for(auto v:args)state->wr<4>(n++*4,v);if(probe)compare(*state,i,Path::generated,Mode::modern);else if(full)matrix(*state,i,profile);else compare(*state,i,Path::generated,profile?Mode::profile:Mode::modern);};
 check(source,0,{record,output,0,0});check(source,1,{record,output});check(source,2,{amount,8});check(source,3,{amount,0,50000});check(source,4,{amount,8});check(source,5,{amount,8});check(source,6,{amount,3});check(source,7,{amount,3,std::uint32_t(source.memory.read<4>(toc-0x59dc)),7});check(source,8,{0,50000,8});check(source,9,{});check(source,10,{});check(source,11,{output,200,3});check(source,12,{output,200,0,1234567,2});
 if(!probe){
  for(auto flags:{0x200000u,0x400000u,0x800000u,0x600000u})for(auto mode:{0u,1u,2u,255u}){auto s=clone(source);s->platform=source.platform;s->memory.write<4>(preferences+4,flags);check(*s,0,{record,output,mode,2},false);}
  for(auto kind:{4u,12u,16u}){auto s=clone(source);s->platform=source.platform;s->memory.write<1>(record+0x38,kind);check(*s,1,{record,output},false);}
  for(unsigned id=0;id<25;++id){auto s=clone(source);s->platform=source.platform;require(setup(*s,0x1000a440,{id})==1,"Original alternate currency configuration");check(*s,0,{record,output,2,0},false);check(*s,11,{output,200,id},false);check(*s,12,{output,200,0,1234567,0},false);}
  for(auto value:{0u,1u,10000u,100000u,1000001u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=clone(source);s->platform=source.platform;s->memory.write<4>(amount,value);check(*s,2,{amount,8},false);check(*s,4,{amount,8},false);check(*s,5,{amount,8},false);}
  for(auto flags:{0u,4u,8u,12u,32u})check(source,8,{0,50001,flags},false);
  for(auto value:{0u,1u,1234u,1234567u})for(auto flags:{0u,2u})check(source,12,{output,200,0,value,flags},false);
  // Exercise the real printf service beyond the GPR-only layouts, including
  // the deepest spill area and the original fifteen-character cap.
  for(auto value:{UINT64_C(12345678),UINT64_C(123456789),UINT64_C(1234567890),UINT64_C(12345678901),UINT64_C(123456789012),UINT64_C(1234567890123),UINT64_C(12345678901234),UINT64_C(123456789012345),UINT64_C(1000000000000000),UINT64_MAX,UINT64_C(0x8000000000000000)})for(auto flags:{0u,2u})check(source,12,{output,200,std::uint32_t(value>>32),std::uint32_t(value),flags},false);
 }
 if(!profile){std::ofstream out(std::filesystem::path(argv[3]).parent_path()/"money-presentation-runtime-sites.txt");for(unsigned n=0;n<roots.size();++n){require(root_returns[n]&&!complete_sites[n].empty(),"Every root needs a complete actual-service return");for(auto pc:runtime_sites[n])out<<n<<" "<<hex_address(pc)<<" "<<complete_sites[n].contains(pc)<<'\n';std::cout<<"Root "<<hex_address(roots[n])<<" runtime_sites="<<runtime_sites[n].size()<<" complete_sites="<<complete_sites[n].size()<<" returns="<<root_returns[n]<<std::endl;}require(wage_value_returns&&wage_text_returns&&wage_quantum_returns,"Complete modern wage children required");}
 std::cout<<"PASS: money presentation cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<" wage_value_children="<<wage_value_returns<<" wage_text_children="<<wage_text_returns<<" wage_quantum_children="<<wage_quantum_returns<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
