#include "preferences_actions_adapter.hpp"
#include "preferences_screen_adapter.hpp"
#include "preferences_adapter.hpp"
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
constexpr std::array<std::uint32_t,4> roots{0x1033ce70,0x1033cf20,0x1033f840,0x1033d030};
constexpr std::array<std::uint32_t,4> ends{0x1033cf14,0x1033d028,0x1033fc60,0x1033f840};
constexpr unsigned full_limit=50000000;
enum class Path {direct,generated,cfm};
enum class Mode {modern,original,entry_original,profile};
void originals(State& s,bool selected=true){preferences_actions_diagnostics(s).original=selected;preferences_screen_diagnostics(s).original=selected;}
// Resource-file numbers, like native data-fork numbers, belong to the host
// process. Translate only these opaque input/output identifiers; dispatch every
// resource operation to the ordinary native platform.
struct ResourceInputs{
 std::map<std::int16_t,std::int16_t> references;std::int16_t next=20001;
 bool dispatch(State& s,std::string_view name){
  const bool result=name=="CarbonLib::CurResFile"||name=="CarbonLib::FSpOpenResFile",argument=name=="CarbonLib::UseResFile"||name=="CarbonLib::CloseResFile";
  if(!result&&!argument)return false;
  const auto original=std::uint32_t(s.r<4>(12));
  if(argument){const auto guest=std::int16_t(original);const auto found=references.find(guest);if(found!=references.end())s.wr<4>(12,std::uint32_t(std::int32_t(found->second)));}
  require(s.platform->dispatch(s,std::string(name)),"Real resource-number input service");
  if(argument)s.wr<4>(12,original);
  else {const auto native=std::int16_t(s.r<4>(12));if(native>=0){auto found=std::find_if(references.begin(),references.end(),[&](const auto& p){return p.second==native;});if(found==references.end())found=references.emplace(next++,native).first;s.wr<4>(12,std::uint32_t(std::int32_t(found->first)));}}
  return true;
 }
};
struct Clock:CodeFragmentBoundaryObserver {
    unsigned count=0;ActorAttributesIoPair* io=nullptr;ResourceInputs* resources=nullptr;bool modern=false;
    struct Event {bool leave;std::string name;decltype(State::registers) registers;std::uint32_t pc,result;bool operator==(const Event&)const=default;};
    std::vector<Event> events;
    void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool)override{}
    void import_enter(const State& s,const char* name)override{events.push_back({false,name,s.registers,s.pc,std::uint32_t(s.r<4>(12))});}
    void import_leave(const State& s,const char* name)override{events.push_back({true,name,s.registers,s.pc,std::uint32_t(s.r<4>(12))});}
    bool import_fixture(State& s,const char* name)override {
        if(resources&&resources->dispatch(s,name))return true;
        if(io&&std::string_view(name)=="CarbonLib::FSSetForkSize"){
            auto& inputs=modern?io->new_forks:io->old_forks;const auto guest=std::int16_t(s.r<4>(12));require(inputs.references.contains(guest),"Resize requires a known private/native fixture fork");
            const auto offset=std::bit_cast<std::int64_t>((s.r<4>(20)<<32)|s.r<4>(24));const auto result=inputs.forks.resize(inputs.references.at(guest),std::uint16_t(s.r<4>(16)),offset);
            s.wr<4>(12,std::uint32_t(std::int32_t(result)));s.pc=s.r<4>(4128);return true;
        }
        if(io&&std::string_view(name)=="CarbonLib::FSOpenFork"){
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>((n+3)*4));};
            NativeFileReference reference{};const auto bytes=s.memory.copy_out(arg(0),reference.size());std::copy(bytes.begin(),bytes.end(),reference.begin());std::filesystem::path path;
            const auto status=native_reference_path(reference,path);
            if(!status){
                const auto game=std::filesystem::canonical(s.platform->game_directory()),relative=std::filesystem::canonical(path).lexically_relative(game);
                if(!relative.empty()&&*relative.begin()!=".."&&!relative.is_absolute()){
                    require(arg(1)==0&&std::int8_t(arg(3))==1,"External game fork must be unnamed and strictly read-only");
                    auto& inputs=modern?io->new_forks:io->old_forks;std::int16_t native=0;const auto result=inputs.forks.open(path,1,native);
                    if(!result){const auto guest=inputs.next++;inputs.references.emplace(guest,native);s.memory.write<2>(arg(4),std::uint16_t(guest));}
                    s.wr<4>(12,std::uint32_t(std::int32_t(result)));s.pc=s.r<4>(4128);return true;
                }
            }
        }
        if(io && io->dispatch(s,name,modern))return true;
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Preferences awaits native event input");
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
    pair->root=input.platform->writable_directory().parent_path()/("preferences-io-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(++serial));
    require(!std::filesystem::exists(pair->root),"Fresh private shortlist IO pair");pair->working=pair->root/"working";ActorAttributesIoPair::copy(input.platform->writable_directory(),pair->working);
    pair->baseline=ActorAttributesIoPair::files(pair->working);for(const auto& [name,bytes]:pair->baseline){(void)bytes;pair->times.emplace(name,std::filesystem::last_write_time(pair->working/name));}
    pair->original=std::make_shared<NativePlatform>(input.platform->game_directory(),pair->working,AudioMode::offline);pair->modern=std::make_shared<NativePlatform>(input.platform->game_directory(),pair->working,AudioMode::offline);return pair;
}

std::array<std::set<std::uint32_t>,roots.size()> runtime_sites,complete_sites;
struct Witness:PreferencesActionsBoundaryObserver,PreferencesScreenBoundaryObserver {
    std::uint32_t negative_root=0;
    std::set<std::uint32_t> seen;
    void enter(const State&,std::uint32_t,bool)override{}
    void returned(State& s,std::uint32_t root,bool original,bool returned){if(root==negative_root&&!original&&returned)s.wr<4>(12,s.r<4>(12)^1);}
    void leave(State& s,std::uint32_t root,bool original,PreferencesActionsOutcome result)override{returned(s,root,original,result==PreferencesActionsOutcome::returned);}
    void leave(State& s,std::uint32_t root,bool original,PreferencesScreenOutcome result)override{returned(s,root,original,result==PreferencesScreenOutcome::returned);}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed>=s.budget)return;seen.insert(pc);for(unsigned n=0;n<roots.size();++n)if(roots[n]<=pc&&pc<ends[n])runtime_sites[n].insert(pc);}
};
void observe(State& s,Witness* w){preferences_actions_diagnostics(s).observer=w;preferences_screen_diagnostics(s).observer=w;}
void reset_diagnostics(State& s){preferences_actions_diagnostics(s)={};preferences_screen_diagnostics(s)={};preferences_diagnostics(s)={};money_text_diagnostics(s)={};}
void select(State& s,unsigned i,Mode mode){if(mode==Mode::original)originals(s);if(mode==Mode::entry_original){if(i<3)preferences_actions_diagnostics(s).entry_original[i]=true;else preferences_screen_diagnostics(s).entry_original[0]=true;}}
void direct(State& s,unsigned i){if(i<3)try_modern_preferences_actions(s,roots[i]);else try_modern_preferences_screen(s,roots[i]);}
std::string execute(State& s,unsigned index,Path path,unsigned limit,std::uint64_t& steps){try{
 s.pc=roots[index];s.wr<4>(4128,return_sentinel);s.budget=s.executed+limit;const auto before=s.executed;
 try{if(path==Path::direct)direct(s,index);else{if(path==Path::cfm){s.memory.write<4>(fixture+0x400,roots[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}run_until(s,return_sentinel);}}catch(...){steps=s.executed-before;throw;}
 steps=s.executed-before;
}catch(const std::exception& e){return e.what();}return {};}
unsigned cases=0,returns=0,budgets=0,negatives=0,faults=0;
std::array<unsigned,4> root_returns{},root_modern_returns{};
std::uint64_t real_currency_children=0,real_video_children=0,real_copy_children=0,real_apply_children=0,real_destroy_children=0;
void between(ActorAttributesIoPair& io){for(const auto& [guest,native]:io.old_forks.references){(void)guest;require(io.old_forks.forks.close(native)==0,"Release original preference native fork");}io.old_forks.references.clear();io.between();}
void prepare_display(State& s,ActorAttributesIoPair& io,ResourceInputs& resources,bool modern){
 // Guest snapshots do not serialize native window/port objects. Recreate the
 // display through the original initializer before observing operations that
 // require an existing display. Preserve their incoming architecture.
 const auto registers=s.registers;const auto unique=s.unique;const auto pc=s.pc;const auto toc=std::uint32_t(s.r<4>(8));
 s.wr<4>(12,1);require(s.platform->dispatch(s,"CarbonLib::SetResLoad"),"Fresh native resource-load input");s.registers=registers;
 // The saved guest desktop still names an absent host window. Its old
 // number would alias the first newly created foreground window.
 s.memory.write<4>(std::uint32_t(s.memory.read<4>(toc-0x5aec)),0);s.memory.write<4>(std::uint32_t(s.memory.read<4>(toc-0x5af8)),0);
 s.memory.write<1>(std::uint32_t(s.memory.read<4>(toc-0x5ae8)),0);s.memory.write<4>(toc-0x356c,0);
 s.memory.write<1>(toc-0x354f,0);const auto bank=std::uint32_t(s.memory.read<4>(toc-0x54e8));originals(s);
 s.wr<4>(12,800);s.wr<4>(16,600);s.wr<4>(20,2);s.wr<4>(24,0xfffffffe);s.wr<4>(28,1);s.wr<4>(32,bank+0x77);s.pc=0x10768820;s.wr<4>(4128,return_sentinel);s.budget=s.executed+full_limit;
 Clock clock;clock.io=&io;clock.resources=&resources;clock.modern=modern;s.code_fragment_observer=&clock;
 try{run_until(s,return_sentinel);}catch(...){std::cerr<<"Display preparation pc="<<hex_address(s.pc)<<" arg="<<hex_address(std::uint32_t(s.r<4>(12)))<<std::endl;for(auto it=clock.events.end()-std::min(std::ptrdiff_t(8),std::ptrdiff_t(clock.events.size()));it!=clock.events.end();++it)std::cerr<<"Preparation import "<<it->name<<" leave="<<it->leave<<std::endl;s.code_fragment_observer=nullptr;throw;}
 s.code_fragment_observer=nullptr;require(s.r<4>(12)==1,"Original native display preparation");originals(s,false);s.registers=registers;s.unique=unique;s.pc=pc;
}
// All UI allocation, text, layout, device and resource services are actual
// production dependencies. The fixture only supplies explicit user commands.
struct Input {unsigned root;std::uint32_t mode=0,command=39,value=0;bool fallback=false;};
std::uint32_t setup(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args){originals(s);unsigned n=3;for(auto v:args)s.wr<4>(n++*4,v);s.pc=entry;s.wr<4>(4128,return_sentinel);s.budget=s.executed+full_limit;run_until(s,return_sentinel);originals(s,false);return std::uint32_t(s.r<4>(12));}
void prepare(State& s,ActorAttributesIoPair& io,ResourceInputs& resources,bool modern,const Input& input){
 prepare_display(s,io,resources,modern);
 Clock clock;clock.io=&io;clock.resources=&resources;clock.modern=modern;s.code_fragment_observer=&clock;
 try{
  if(input.root>=2){
   setup(s,roots[0],{input.mode});
   if(input.root==2){
    setup(s,roots[3],{});
    const auto context=std::uint32_t(s.memory.read<4>(s.r<4>(8)-0x7bb0));
    const auto view=std::int16_t(s.memory.read<2>(context+0x2c58));
    const auto page=std::uint32_t(s.memory.read<4>(context+std::uint32_t(std::int32_t(view)*0x2c0)+0x24));
    require(page!=0,"Original options builder must create an actual event page");
    // Row zero is a controlled event input on the actual constructed page.
    // No UI dependency is substituted, and both versions receive equal bytes.
    s.memory.write<4>(page+0xba86c,input.fallback?0:input.command);
    s.memory.write<4>(page+0xba8ac,input.value);
    s.memory.write<2>(context+0x25819e,input.command);
   }
  }
 }catch(...){s.code_fragment_observer=nullptr;throw;}
 s.code_fragment_observer=nullptr;s.wr<4>(12,input.root<2?input.mode:0);reset_diagnostics(s);
}
std::uint64_t compare(const State& input,const Input& request,Path path,Mode mode,unsigned limit=full_limit,bool negative=false,bool fault=false){
 const auto i=request.root;auto backing=paired_backing(input);auto& io=*backing;auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
 ResourceInputs old_resources,new_resources;prepare(*a,io,old_resources,false,request);Image prepared(*a);const auto prepared_steps=a->executed;
 const auto inject_fault=[&](State& s){if(fault)s.memory.write<4>(s.r<4>(8)-0x7bb0,0xf0000000);};inject_fault(*a);
 originals(*a);Witness witness;if(negative)witness.negative_root=roots[i];
 Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;old_clock.resources=&old_resources;new_clock.resources=&new_resources;new_clock.modern=true;a->code_fragment_observer=&old_clock;
 DigestEffects x{*a},y{*b};a->memory.date_observer=&x;std::uint64_t old_steps=0,new_steps=0;
 const auto expected=execute(*a,i,path,limit,old_steps);a->memory.date_observer=nullptr;a->platform.reset();between(io);
 prepare(*b,io,new_resources,true,request);prepared.compare(*b,"Whole original native UI preparations");require(b->executed==prepared_steps,"Native UI preparation accounting");
 inject_fault(*b);select(*b,i,mode);if(mode==Mode::modern)observe(*b,&witness);b->code_fragment_observer=&new_clock;b->memory.date_observer=&y;
 const auto actual=execute(*b,i,path,limit,new_steps);
 a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;observe(*b,nullptr);
 const auto label="Preferences UI root="+hex_address(roots[i])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
 if(!expected.empty()&&limit==full_limit&&!fault){std::cerr<<label<<" original pc="<<hex_address(a->pc)<<" argument="<<hex_address(std::uint32_t(a->r<4>(12)))<<std::endl;for(auto it=old_clock.events.end()-std::min(std::ptrdiff_t(8),std::ptrdiff_t(old_clock.events.size()));it!=old_clock.events.end();++it)std::cerr<<"Recent actual import "<<it->name<<" leave="<<it->leave<<std::endl;}
 require(expected==actual,label+" original="+expected+" modern="+actual);require(fault?expected.starts_with("Unmapped data access at "):limit==full_limit?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected outcome="+expected+" pc="+hex_address(a->pc));
 require(old_clock.count==new_clock.count&&old_clock.events==new_clock.events,label+" actual import ABI/input order");
 if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative must discriminate");++negatives;}
 else{Image(*a).compare(*b,label);require(x.events==y.events&&x.bytes==y.bytes&&x.digest()==y.digest()&&a->executed==b->executed&&old_steps==new_steps,label+" ordered timed effects/accounting");}
 io.compare();
 if(expected.empty()&&!negative){
  const auto context=std::uint32_t(b->memory.read<4>(b->r<4>(8)-0x7bb0));
  const auto view=std::int16_t(b->memory.read<2>(context+0x2c58));
  const auto session=std::uint32_t(b->memory.read<4>(context+std::uint32_t(std::int32_t(view)*0x2c0)+0x10));
  require(session!=0,label+" actual active session");
  if(i<2){
   const auto edit=std::uint32_t(b->memory.read<4>(session+0x14));
   require(edit!=0&&b->memory.read<4>(edit)!=0,label+" actual linked editable preference copy");
   require(b->memory.read<4>(session+0x1c)==(i==0?request.mode&255:(request.mode&255)?3:2),label+" independent opening mode");
  }
  if(i==2){
   const auto result=std::uint32_t(b->r<4>(12));
   const bool recognized=(request.command>=39&&request.command<=45)||request.command==61;
   require(result==(request.command==40||request.command==41?0xfffffff7u:recognized?0xfffffff5u:0u),label+" independent action result");
  }
 }
 const auto audit=[&](const auto& d,unsigned n){require(!d.original_body&&!d.dispatch_escapes,label+" original scope/dispatch residue");if(mode==Mode::modern){require(d.calls[n]>=1&&!d.original_calls[n]&&!d.profile_original_calls[n],label+" productive root");require(d.calls[n]==d.returns[n]+d.faults[n]+d.budget_stops[n]+d.transfers[n]+d.terminations[n],label+" balanced outcomes");require(expected.empty()?d.returns[n]==d.calls[n]:fault?d.faults[n]>=1&&!d.budget_stops[n]:d.budget_stops[n]>=1&&!d.faults[n],label+" productive outcome");}else{require(!d.calls[n]&&!d.returns[n],label+" original control entered native root");require(mode==Mode::profile?d.profile_original_calls[n]>=1:d.original_calls[n]>=1,label+" missing original control");}};
 if(i<3)audit(preferences_actions_diagnostics(*b),i);else audit(preferences_screen_diagnostics(*b),0);
 for(auto pc:witness.seen)if(expected.empty()&&!negative)for(unsigned n=0;n<roots.size();++n)if(roots[n]<=pc&&pc<ends[n])complete_sites[n].insert(pc);
 if(mode==Mode::modern&&expected.empty()&&!negative){real_currency_children+=money_text_diagnostics(*b).returns[2];real_video_children+=preferences_diagnostics(*b).returns[10];real_copy_children+=preferences_diagnostics(*b).returns[1];real_apply_children+=preferences_diagnostics(*b).returns[9];real_destroy_children+=preferences_diagnostics(*b).returns[2];}
 std::cout<<"case="<<cases<<" root="<<hex_address(roots[i])<<" input_mode="<<request.mode<<" command="<<request.command<<" path="<<unsigned(path)<<" mode="<<unsigned(mode)<<" steps="<<old_steps<<" events="<<x.events<<" result="<<hex_address(std::uint32_t(a->r<4>(12)))<<std::endl;
 root_modern_returns[i]+=expected.empty()&&!negative&&mode==Mode::modern;++cases;returns+=expected.empty();if(expected.empty()&&!negative)++root_returns[i];budgets+=!fault&&!expected.empty();faults+=fault;return old_steps;
}
void matrix(const State& input,const Input& request,bool profile){
 if(profile){compare(input,request,Path::generated,Mode::profile);compare(input,request,Path::cfm,Mode::profile);return;}
 std::uint64_t steps=0;for(auto path:{Path::direct,Path::generated,Path::cfm}){const auto n=compare(input,request,path,Mode::modern);if(path==Path::generated)steps=n;}
 compare(input,request,Path::generated,Mode::original);compare(input,request,Path::generated,Mode::entry_original);compare(input,request,Path::generated,Mode::modern,full_limit,true);
 for(auto cut:std::set<std::uint64_t>{0,steps/2,steps-1})compare(input,request,Path::generated,Mode::modern,unsigned(cut));compare(input,request,Path::generated,Mode::modern,full_limit,false,true);
}
struct OwnedBacking{std::filesystem::path path;~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(path,e)||e||std::filesystem::is_symlink(path,e)||e)return;for(const auto& p:std::filesystem::recursive_directory_iterator(path,e))if(e||p.is_symlink(e)||e)return;std::filesystem::remove_all(path,e);}};
}
int main(int argc,char** argv){try{
 require(argc==5||(argc==6&&(std::string_view(argv[5])=="--profile"||std::string_view(argv[5])=="--probe")),"Expected export,game,private backing,original prepared snapshot and optional profile/probe");const bool profile=argc==6&&std::string_view(argv[5])=="--profile",probe=argc==6&&std::string_view(argv[5])=="--probe";require(profile==match_pc_profile_enabled(),"Preferences UI profile flag mismatch");set_match_turbo_enabled(false);
 const auto directory=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(directory),"Fresh private preferences UI backing");OwnedBacking cleanup{directory};const auto snapshots=std::filesystem::path(argv[4]).parent_path();std::ifstream pointer(snapshots/"prepared-backing.txt");std::string name;std::getline(pointer,name);require(std::filesystem::path(name).parent_path().empty()&&name.starts_with("prepared-backing-"),"Retained original backing name");ActorAttributesIoPair::copy(snapshots/name,directory);
 State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],directory,AudioMode::offline);load_match_snapshot(source,argv[4]);source.memory.map(fixture,0x10000);const auto toc=std::uint32_t(source.r<4>(8));require(toc==cm::semantic_io_checks::toc,"Original loaded TOC");
 require(setup(source,0x1000a440,{0})==1,"Baseline currency configuration");const auto preference_pointer=std::uint32_t(source.memory.read<4>(toc-0x7ac8)),preferences=std::uint32_t(source.memory.read<4>(preference_pointer));require(preferences,"Original preference object");
 // Explicit sound-off/current-currency inputs avoid a live audio owner from the
 // captured process. No career file or installed preference is changed.
 source.memory.write<4>(preferences+4,0x21255329);source.memory.write<1>(preferences+9,4);source.memory.write<4>(std::uint32_t(source.memory.read<4>(toc-0x7468)),0);
 // Build the baseline through the original resource and stdio services,
 // before observation. This discovers the program's own private file path.
 const auto bank=std::uint32_t(source.memory.read<4>(toc-0x54e8));
 const auto path=setup(source,0x10746280,{bank+0x3f,bank+0x49});
 const auto file=setup(source,0x10739de0,{path,bank+0x103});require(file,"Original private preference open");
 source.memory.write<4>(fixture+0x3000,0x1553117c);
 for(auto [at,size]:{std::pair{fixture+0x3000,4u},{preferences+4,4u},{preferences+8,1u},{preferences+9,1u},{preferences+10,2u},{preferences+12,256u},{preferences+0x12c,4u},{preferences+0x130,31u}})require(setup(source,0x10776360,{at,size,1,file})==1,"Original private preference field write");
 setup(source,0x107766f0,{file});

 const auto check=[&](Input request,bool full=true){if(probe)compare(source,request,Path::generated,Mode::modern);else if(full)matrix(source,request,profile);else compare(source,request,Path::generated,profile?Mode::profile:Mode::modern);};
 for(unsigned i=0;i<4;++i)check({i});
 if(!probe){
  for(auto mode:{1u,2u,3u,0x123400ffu})check({0,mode},false);
  for(auto state:{1u,0x100u})check({1,state},false);
  for(auto mode:{1u,2u,3u})check({3,mode},false);
  for(auto command:{40u,41u,42u,43u,44u,45u,61u,0u,99u})check({2,0,command,command==43?4u:command==44?1u:command==45?30u:command==61?0xfffffffeu:0u},false);
  check({2,0,39,0,true},false);check({2,0,45,7,true},false);
 }
 if(!profile){std::ofstream out(std::filesystem::path(argv[3]).parent_path()/"preferences-ui-runtime-sites.txt");for(unsigned n=0;n<roots.size();++n){require(root_returns[n]&&root_modern_returns[n]&&!complete_sites[n].empty(),"Every UI root needs a complete actual-service return");for(auto pc:runtime_sites[n])out<<n<<" "<<hex_address(pc)<<" "<<complete_sites[n].contains(pc)<<'\n';std::cout<<"Root "<<hex_address(roots[n])<<" runtime_sites="<<runtime_sites[n].size()<<" complete_sites="<<complete_sites[n].size()<<" returns="<<root_returns[n]<<" productive_returns="<<root_modern_returns[n]<<std::endl;}require(real_copy_children&&real_currency_children&&real_video_children,"Actual complete copy/currency/video child returns");if(!probe)require(real_apply_children&&real_destroy_children,"Actual apply and destroy children");}
 std::cout<<"PASS: preferences UI cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<" currency_children="<<real_currency_children<<" video_children="<<real_video_children<<" copy_children="<<real_copy_children<<" apply_children="<<real_apply_children<<" destroy_children="<<real_destroy_children<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
