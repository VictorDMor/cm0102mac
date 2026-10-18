#include "money_configuration_adapter.hpp"
#include "preferences_adapter.hpp"
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
constexpr std::array<std::uint32_t,13> roots{0x1000a440,0x1013f030,0x1013f410,0x1013f4d0,0x1013f570,0x1013f680,0x1013fb60,0x1013fc50,0x1013fd00,0x1013fe00,0x1013feb0,0x101406e0,0x101406f0};
constexpr std::array<std::uint32_t,13> ends{0x1000b148,0x1013f404,0x1013f4c4,0x1013f564,0x1013f67c,0x1013fb58,0x1013fc44,0x1013fcfc,0x1013fdf4,0x1013feac,0x101406d4,0x101406e8,0x10140724};
constexpr unsigned full_limit=50000000;
constexpr std::uint32_t record=fixture+0x1000,source_record=fixture+0x2000;
enum class Path {direct,generated,cfm};
enum class Mode {modern,original,entry_original,profile};
void originals(State& s,bool selected=true){money_configuration_diagnostics(s).original=selected;preferences_diagnostics(s).original=selected;}
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
struct Witness:MoneyConfigurationBoundaryObserver,PreferencesBoundaryObserver {
    std::uint32_t negative_root=0;
    std::set<std::uint32_t> seen;
    void enter(const State&,std::uint32_t,bool)override{}
    void returned(State& s,std::uint32_t root,bool original,bool returned){if(root==negative_root&&!original&&returned)s.wr<4>(12,s.r<4>(12)^1);}
    void leave(State& s,std::uint32_t root,bool original,MoneyConfigurationOutcome result)override{returned(s,root,original,result==MoneyConfigurationOutcome::returned);}
    void leave(State& s,std::uint32_t root,bool original,PreferencesOutcome result)override{returned(s,root,original,result==PreferencesOutcome::returned);}
    void before_step(State& s,std::uint32_t pc)override{if(s.executed>=s.budget)return;seen.insert(pc);for(unsigned n=0;n<roots.size();++n)if(roots[n]<=pc&&pc<ends[n])runtime_sites[n].insert(pc);}
};
void observe(State& s,Witness* w){money_configuration_diagnostics(s).observer=w;preferences_diagnostics(s).observer=w;}
void select(State& s,unsigned i,Mode mode){if(mode==Mode::original)originals(s);if(mode==Mode::entry_original){if(!i)money_configuration_diagnostics(s).entry_original[0]=true;else preferences_diagnostics(s).entry_original[i-1]=true;}}
void direct(State& s,unsigned i){if(!i)try_modern_money_configuration(s,roots[i]);else try_modern_preferences(s,roots[i]);}
std::string execute(State& s,unsigned index,Path path,unsigned limit,std::uint64_t& steps){try{
 s.pc=roots[index];s.wr<4>(4128,return_sentinel);s.budget=s.executed+limit;const auto before=s.executed;
 try{if(path==Path::direct)direct(s,index);else{if(path==Path::cfm){s.memory.write<4>(fixture+0x400,roots[index]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}run_until(s,return_sentinel);}}catch(...){steps=s.executed-before;throw;}
 steps=s.executed-before;
}catch(const std::exception& e){return e.what();}return {};}
unsigned cases=0,returns=0,budgets=0,negatives=0,faults=0;
std::array<unsigned,13> root_returns{},root_modern_returns{};
std::string preference_file;
std::uint64_t apply_currency_returns=0,initialize_currency_returns=0,linked_apply_returns=0;
void between(ActorAttributesIoPair& io){for(const auto& [guest,native]:io.old_forks.references){(void)guest;require(io.old_forks.forks.close(native)==0,"Release original money native fork");}io.old_forks.references.clear();io.between();}
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
std::uint64_t compare(const State& input,unsigned i,Path path,Mode mode,unsigned limit=full_limit,bool negative=false,bool fault=false){
 auto backing=paired_backing(input);auto& io=*backing;auto a=clone(input),b=clone(input);a->platform=io.original;b->platform=io.modern;
 const bool display=i==1||i==3||i==6||i==10;std::optional<Image> prepared;std::uint64_t prepared_steps=0;ResourceInputs old_resources,new_resources;
 if(display){prepare_display(*a,io,old_resources,false);prepared.emplace(*a);prepared_steps=a->executed;}
 const auto inject_fault=[&](State& s){if(fault){if(!i)s.memory.write<4>(s.r<4>(8)-0x59d4,0xf0000000);else s.wr<4>(12,0xf0000000);}};inject_fault(*a);

 originals(*a);Witness witness;if(negative)witness.negative_root=roots[i];
 Clock old_clock,new_clock;old_clock.io=new_clock.io=&io;old_clock.resources=&old_resources;new_clock.resources=&new_resources;new_clock.modern=true;a->code_fragment_observer=&old_clock;
 DigestEffects x{*a},y{*b};a->memory.date_observer=&x;std::uint64_t old_steps=0,new_steps=0;
 const auto expected=execute(*a,i,path,limit,old_steps);a->memory.date_observer=nullptr;a->platform.reset();between(io);
 // Resource Manager selection/load flags are process-global. Prepare and run
 // each platform sequentially, restoring private files before the second one.
 if(display){prepare_display(*b,io,new_resources,true);prepared->compare(*b,"Original display preparations match");require(b->executed==prepared_steps,"Original display preparation accounting");}
 inject_fault(*b);select(*b,i,mode);if(mode==Mode::modern)observe(*b,&witness);b->code_fragment_observer=&new_clock;b->memory.date_observer=&y;
 const auto actual=execute(*b,i,path,limit,new_steps);
 if(!expected.empty()&&limit==full_limit&&!fault){const auto scratch=std::uint32_t(a->memory.read<4>(cm::semantic_io_checks::toc-0x7ba8));std::string diagnostic;for(unsigned n=0;n<300;++n){const auto byte=a->memory.read<1>(scratch+n);if(!byte)break;diagnostic+=char(byte);}std::cerr<<"Guest diagnostic="<<diagnostic<<std::endl;for(const auto& e:old_clock.events)if(e.name.find("Fork")!=std::string::npos||e.name.find("Spec")!=std::string::npos||e.name.find("Create")!=std::string::npos)std::cerr<<"File import "<<e.name<<" leave="<<e.leave<<" value="<<hex_address(e.result)<<std::endl;std::cerr<<"Fault argument="<<hex_address(std::uint32_t(a->r<4>(12)))<<std::endl;for(auto it=old_clock.events.end()-std::min(std::ptrdiff_t(8),std::ptrdiff_t(old_clock.events.size()));it!=old_clock.events.end();++it)std::cerr<<"Recent import "<<it->name<<" leave="<<it->leave<<" pc="<<hex_address(it->pc)<<std::endl;}
 a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;observe(*b,nullptr);
 const auto label="Preferences root="+hex_address(roots[i])+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
 require(expected==actual,label+" original="+expected+" modern="+actual);require(fault?expected.starts_with("Unmapped data access at "):limit==full_limit?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected outcome="+expected+" pc="+hex_address(a->pc));
 if(old_clock.events!=new_clock.events){const auto pair=std::mismatch(old_clock.events.begin(),old_clock.events.end(),new_clock.events.begin(),new_clock.events.end());if(pair.first!=old_clock.events.end()&&pair.second!=new_clock.events.end()){const auto bytes=std::mismatch(pair.first->registers.begin(),pair.first->registers.end(),pair.second->registers.begin());std::cerr<<"Import difference event="<<(pair.first-old_clock.events.begin())<<" old="<<pair.first->name<<" new="<<pair.second->name<<" leave="<<pair.first->leave<<" register_byte="<<(bytes.first-pair.first->registers.begin())<<std::endl;}}
 require(old_clock.count==new_clock.count&&old_clock.events==new_clock.events,label+" import ABI/input order");
 if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative must discriminate");++negatives;}
 else{Image(*a).compare(*b,label);require(x.events==y.events&&x.bytes==y.bytes&&x.digest()==y.digest()&&a->executed==b->executed&&old_steps==new_steps,label+" ordered timed effects/accounting");}
 io.compare();
 if(i==10&&expected.empty()){
  require(a->r<4>(12)==1,"Original complete preference write succeeds");const auto object=std::uint32_t(input.r<4>(12)),link=std::uint32_t(input.memory.read<4>(object)),owner=link?link:object;
  std::vector<char> bytes{0x15,0x53,0x11,0x7c};for(auto [at,size]:{std::pair{owner+4,4u},{owner+8,1u},{owner+9,1u},{owner+10,2u},{owner+12,256u},{owner+0x12c,4u},{owner+0x130,31u}}){const auto part=a->memory.copy_out(at,size);for(auto byte:part)bytes.push_back(char(byte));}
  require(bytes.size()==303&&io.expected.at(preference_file)==bytes,"Independent eight-field303-byte actual preference file layout");
 }

 const auto audit=[&](const auto& d,unsigned n){require(!d.original_body&&!d.dispatch_escapes,label+" original scope/dispatch residue");if(mode==Mode::modern){require(d.calls[n]>=1&&!d.original_calls[n]&&!d.profile_original_calls[n],label+" productive root");require(d.calls[n]==d.returns[n]+d.faults[n]+d.budget_stops[n]+d.transfers[n]+d.terminations[n],label+" balanced outcomes");require(expected.empty()?d.returns[n]==d.calls[n]:fault?d.faults[n]>=1&&!d.budget_stops[n]:d.budget_stops[n]>=1&&!d.faults[n],label+" expected productive outcome");}else{require(!d.calls[n]&&!d.returns[n],label+" original control entered native root");require(mode==Mode::profile?d.profile_original_calls[n]>=1:d.original_calls[n]>=1,label+" missing original control");}};
 if(!i)audit(money_configuration_diagnostics(*b),0);else audit(preferences_diagnostics(*b),i-1);
 for(auto pc:witness.seen)if(expected.empty()&&!negative)for(unsigned n=0;n<roots.size();++n)if(roots[n]<=pc&&pc<ends[n])complete_sites[n].insert(pc);
 if(mode==Mode::modern&&expected.empty()&&!negative){const auto& c=money_configuration_diagnostics(*b);const auto& q=preferences_diagnostics(*b);if(i==10)apply_currency_returns+=c.returns[0];if(i==1)initialize_currency_returns+=c.returns[0];if(i==10)linked_apply_returns+=q.returns[9]>1;}
 std::cout<<"case="<<cases<<" root="<<hex_address(roots[i])<<" mode="<<unsigned(mode)<<" steps="<<old_steps<<" events="<<x.events<<" result="<<hex_address(std::uint32_t(a->r<4>(12)))<<std::endl;
 root_modern_returns[i]+=expected.empty()&&!negative&&mode==Mode::modern;++cases;returns+=expected.empty();if(expected.empty()&&!negative)++root_returns[i];budgets+=!fault&&!expected.empty();faults+=fault;return old_steps;
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
 require(argc==5||(argc==6&&(std::string_view(argv[5])=="--profile"||std::string_view(argv[5])=="--probe")),"Expected export,game,private backing,original prepared snapshot and optional profile/probe");const bool profile=argc==6&&std::string_view(argv[5])=="--profile",probe=argc==6&&std::string_view(argv[5])=="--probe";require(profile==match_pc_profile_enabled(),"Preferences profile flag mismatch");set_match_turbo_enabled(false);
 const auto directory=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(directory),"Fresh private preferences backing");OwnedBacking cleanup{directory};const auto snapshots=std::filesystem::path(argv[4]).parent_path();std::ifstream pointer(snapshots/"prepared-backing.txt");std::string name;std::getline(pointer,name);require(std::filesystem::path(name).parent_path().empty()&&name.starts_with("prepared-backing-"),"Retained original backing name");ActorAttributesIoPair::copy(snapshots/name,directory);
 State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],directory,AudioMode::offline);load_match_snapshot(source,argv[4]);source.memory.map(fixture,0x10000);const auto toc=std::uint32_t(source.r<4>(8));require(toc==cm::semantic_io_checks::toc,"Original loaded TOC");
 require(setup(source,0x1000a440,{0})==1,"Original baseline currency configuration");const auto preference_pointer=std::uint32_t(source.memory.read<4>(toc-0x7ac8)),preferences=std::uint32_t(source.memory.read<4>(preference_pointer));require(preferences,"Original preference object");
 source.memory.copy_in(record,source.memory.copy_out(preferences,0x150));source.memory.copy_in(source_record,source.memory.copy_out(preferences,0x150));
 source.memory.write<4>(record,0);source.memory.write<4>(source_record,0);source.memory.write<4>(record+4,0x21255329);source.memory.write<4>(source_record+4,0x21255329);source.memory.write<4>(std::uint32_t(source.memory.read<4>(toc-0x7468)),0);source.memory.write<1>(record+9,4);source.memory.write<1>(source_record+9,4);
 // Build the baseline through the original resource and stdio services,
 // before observation. This discovers the program's own private file path.
 const auto bank=std::uint32_t(source.memory.read<4>(toc-0x54e8));
 const auto path=setup(source,0x10746280,{bank+0x3f,bank+0x49});
 const auto file=setup(source,0x10739de0,{path,bank+0x103});require(file,"Original private preference open");
 source.memory.write<4>(fixture+0x3000,0x1553117c);
 for(auto [at,size]:{std::pair{fixture+0x3000,4u},{record+4,4u},{record+8,1u},{record+9,1u},{record+10,2u},{record+12,256u},{record+0x12c,4u},{record+0x130,31u}})require(setup(source,0x10776360,{at,size,1,file})==1,"Original private preference field write");
 setup(source,0x107766f0,{file});
 for(const auto& [name,bytes]:ActorAttributesIoPair::files(directory))if(bytes.size()==303&&std::uint8_t(bytes[0])==0x15&&std::uint8_t(bytes[1])==0x53&&std::uint8_t(bytes[2])==0x11&&std::uint8_t(bytes[3])==0x7c){require(preference_file.empty(),"Unique original preference output");preference_file=name;}
 require(!preference_file.empty(),"Original stdio preparation produced its preference file");

 std::cout<<"Original preferences="<<hex_address(preferences)<<" language="<<source.memory.read<1>(record+8)<<" video="<<hex_address(std::uint32_t(source.memory.read<4>(record+0x12c)))<<std::endl;
 const auto check=[&](const State& base,unsigned i,std::initializer_list<std::uint32_t> args,bool full=true){auto state=clone(base);state->platform=base.platform;if(i==1)state->memory.write<1>(toc-0x354f,0);unsigned n=3;for(auto v:args)state->wr<4>(n++*4,v);if(probe)compare(*state,i,Path::generated,Mode::modern);else if(full)matrix(*state,i,profile);else compare(*state,i,Path::generated,profile?Mode::profile:Mode::modern);};
 const auto lang=std::uint32_t(source.memory.read<1>(record+8));
 check(source,0,{4});check(source,1,{record});check(source,2,{record,source_record});check(source,3,{record,0});check(source,4,{record});check(source,5,{record,1});check(source,6,{record,lang});check(source,7,{record,4});check(source,8,{record});check(source,9,{record,30});check(source,10,{record});check(source,11,{record});check(source,12,{record,std::uint32_t(source.memory.read<4>(record+0x12c))});
 if(!probe){
  for(unsigned id=0;id<25;++id){check(source,0,{id},false);check(source,7,{record,id},false);auto s=clone(source);s->platform=source.platform;s->memory.write<1>(record+9,id);check(*s,10,{record},false);}
  for(auto choice:{1u,2u,4u,8u,16u,32u,64u,128u,256u,512u,1024u,2048u,4096u,8192u,16384u,32768u,65536u,131072u,262144u,524288u,1048576u,2097152u,4194304u,8388608u,16777216u,33554432u,67108864u,536870912u,1073741824u})check(source,5,{record,choice},false);
  for(auto interval:{7u,30u,365u,0x12340007u,0xffff016du})check(source,9,{record,interval},false);
  {auto s=clone(source);s->platform=source.platform;s->memory.write<4>(record,source_record);check(*s,10,{record},false);check(*s,3,{record,0},false);}
  {auto s=clone(source);s->platform=source.platform;s->memory.write<4>(record,0);check(*s,3,{record,0},false);}
 }
 if(!profile){std::ofstream out(std::filesystem::path(argv[3]).parent_path()/"preferences-runtime-sites.txt");for(unsigned n=0;n<roots.size();++n){require(root_returns[n]&&root_modern_returns[n]&&!complete_sites[n].empty(),"Every root needs a complete actual-service return");for(auto pc:runtime_sites[n])out<<n<<" "<<hex_address(pc)<<" "<<complete_sites[n].contains(pc)<<'\n';std::cout<<"Root "<<hex_address(roots[n])<<" runtime_sites="<<runtime_sites[n].size()<<" complete_sites="<<complete_sites[n].size()<<" returns="<<root_returns[n]<<" productive_returns="<<root_modern_returns[n]<<std::endl;}require(apply_currency_returns&&initialize_currency_returns,"Whole load/apply callers require complete configuration children");if(!probe)require(linked_apply_returns,"Linked copy must complete recursive primary application");}
 std::cout<<"PASS: preferences cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<" initialize_currency_children="<<initialize_currency_returns<<" apply_currency_children="<<apply_currency_returns<<" linked_apply_children="<<linked_apply_returns<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
