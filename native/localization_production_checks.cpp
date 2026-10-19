#include "localization_lookup_adapter.hpp"
#include "localization_format_adapter.hpp"
#include "formatted_output_adapter.hpp"
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
constexpr std::array<std::uint32_t,4> roots{0x10727860,0x10727880,0x10728400,0x10727890};
constexpr std::array<std::uint32_t,4> ends{0x1072787c,0x10727888,0x10728ac0,0x107283f8};
constexpr unsigned full_limit=100000000;
constexpr std::uint32_t record=fixture+0x8000,index_output=fixture+0x9000,query=fixture+0x2000,output=fixture+0x4000,argument=fixture+0x6000;
enum class Path{direct,generated,cfm};enum class Mode{modern,original,entry_original,profile};
void originals(State& s,bool selected=true){localization_lookup_diagnostics(s).original=selected;localization_format_diagnostics(s).original=selected;}
void text(State& s,std::uint32_t at,const std::string& value){s.memory.copy_in(at,std::vector<std::uint8_t>(value.begin(),value.end()));s.memory.write<1>(at+std::uint32_t(value.size()),0);}
std::string text(const State& s,std::uint32_t at){std::string out;for(unsigned n=0;n<4096;++n){const auto c=s.memory.read<1>(at+n);if(!c)return out;out+=char(c);}throw std::runtime_error("Unterminated actual fixture text");}
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
        if(std::string_view(name)=="CarbonLib::WaitNextEvent" || std::string_view(name)=="CarbonLib::GetNextEvent")throw std::runtime_error("Localization awaits native event input");
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
    pair->root=input.platform->writable_directory().parent_path()/("localization-io-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(++serial));
    require(!std::filesystem::exists(pair->root),"Fresh private localization IO pair");pair->working=pair->root/"working";ActorAttributesIoPair::copy(input.platform->writable_directory(),pair->working);
    pair->baseline=ActorAttributesIoPair::files(pair->working);for(const auto& [name,bytes]:pair->baseline){(void)bytes;pair->times.emplace(name,std::filesystem::last_write_time(pair->working/name));}
    pair->original=std::make_shared<NativePlatform>(input.platform->game_directory(),pair->working,AudioMode::offline);pair->modern=std::make_shared<NativePlatform>(input.platform->game_directory(),pair->working,AudioMode::offline);return pair;
}


std::array<std::set<std::uint32_t>,4> runtime_sites,complete_sites;
struct Witness:LocalizationLookupBoundaryObserver,LocalizationFormatBoundaryObserver,FormattedOutputBoundaryObserver{
 std::uint32_t negative_root=0,child_record=0;bool negative_child=false;std::set<std::uint32_t> seen;unsigned substitution_returns=0,formatting_returns=0;
 struct Pending{std::uint32_t frame,continuation;};std::vector<Pending> substitutions;
 void enter(const State&,std::uint32_t,bool)override{}
 void returned(State& s,std::uint32_t root,bool original,bool returned){if(root==negative_root&&!original&&returned)s.wr<4>(12,s.r<4>(12)^1);}
 void leave(State& s,std::uint32_t root,bool original,LocalizationLookupOutcome result)override{if(negative_child&&root==0x10728400&&!original&&result==LocalizationLookupOutcome::returned&&child_record){const auto value=std::uint32_t(s.memory.read<4>(child_record+8));require(value!=0,"Actual child negative needs a returned translated string");s.memory.write<1>(value,s.memory.read<1>(value)^1);negative_child=false;}returned(s,root,original,result==LocalizationLookupOutcome::returned);}
 void leave(State& s,std::uint32_t root,bool original,LocalizationFormatOutcome result)override{returned(s,root,original,result==LocalizationFormatOutcome::returned);}
 void leave(State&,std::uint32_t root,bool original)override{if(!original&&root==0x1077b6f0)++formatting_returns;}
 void before_dependency(State& s,std::uint32_t pc,std::uint32_t target)override{if(target==0x10728400)child_record=std::uint32_t(s.r<4>(12));if(target==0x10728ac0)substitutions.push_back({std::uint32_t(s.r<4>(4)),pc+4});}
 void before_step(State& s,std::uint32_t pc)override{
  if(s.executed>=s.budget)return;
  for(auto it=substitutions.begin();it!=substitutions.end();)if(it->frame==s.r<4>(4)&&it->continuation==pc){++substitution_returns;it=substitutions.erase(it);}else ++it;
  seen.insert(pc);for(unsigned n=0;n<roots.size();++n)if(roots[n]<=pc&&pc<ends[n])runtime_sites[n].insert(pc);
 }
};
void observe(State& s,Witness* w){localization_lookup_diagnostics(s).observer=w;localization_format_diagnostics(s).observer=w;s.formatted_output_observer=w;}
void reset_diagnostics(State& s){localization_lookup_diagnostics(s)={};localization_format_diagnostics(s)={};reset_formatted_output_diagnostics(s);}
void select(State& s,unsigned i,Mode mode){if(mode==Mode::original)originals(s);if(mode==Mode::entry_original){if(i<3)localization_lookup_diagnostics(s).entry_original[i]=true;else localization_format_diagnostics(s).entry_original[0]=true;}}
void direct(State& s,unsigned i){if(i<3)try_modern_localization_lookup(s,roots[i]);else try_modern_localization_format(s,roots[i]);}
std::string execute(State& s,unsigned i,Path path,unsigned limit,std::uint64_t& steps){try{
 s.pc=roots[i];s.wr<4>(4128,return_sentinel);s.budget=s.executed+limit;const auto before=s.executed;
 try{if(path==Path::direct)direct(s,i);else{if(path==Path::cfm){s.memory.write<4>(fixture+0x400,roots[i]);s.memory.write<4>(fixture+0x404,s.r<4>(8));s.wr<4>(48,fixture+0x400);s.pc=0x10781320;}run_until(s,return_sentinel);}}catch(...){steps=s.executed-before;throw;}
 steps=s.executed-before;
}catch(const std::exception& e){return e.what();}return {};}
struct Inputs{std::string simple_key,simple_translation,disk_key,disk_descriptor,disk_translation;std::array<std::uint32_t,10> disk_words{};};
struct Input{unsigned root,variant=0;};
std::uint32_t setup(State& s,std::uint32_t entry,std::initializer_list<std::uint32_t> args){originals(s);unsigned n=3;for(auto v:args)s.wr<4>(n++*4,v);s.pc=entry;s.wr<4>(4128,return_sentinel);s.budget=s.executed+1000000000;run_until(s,return_sentinel);originals(s,false);return std::uint32_t(s.r<4>(12));}
void prepare(State& s,const Input& request,const Inputs& inputs){
 const auto toc=std::uint32_t(s.r<4>(8));s.memory.write<1>(toc-0x35ac,2);s.memory.write<1>(toc-0x359c,1);s.memory.write<4>(toc-0x35a8,0);s.memory.write<4>(index_output,0x87654321);
 setup(s,roots[0],{record});for(unsigned n=0;n<10;++n)s.memory.write<4>(record+12+n*4,0xfeed0000+n);
 const auto table=std::uint32_t(s.memory.read<4>(toc-0x3590)),first=std::uint32_t(s.memory.read<4>(table));
 text(s,query,request.variant==2?inputs.disk_key:request.variant==3?"__missing_CM_localization_fixture__":text(s,first));
 if(request.root==0){for(unsigned n=0;n<3;++n)s.memory.write<4>(record+n*4,0x12345600+n);s.wr<4>(12,record);}
 else if(request.root==1){if(request.variant)s.memory.write<1>(toc-0x35ac,request.variant);}
 else if(request.root==2){
  if(request.variant==1)require(setup(s,roots[2],{record,first,index_output})==1,"Original owned lookup-record preparation");
  if(request.variant==2)s.memory.write<1>(toc-0x359c,0);
  s.wr<4>(12,record);s.wr<4>(16,query);s.wr<4>(20,request.variant==4?0:index_output);
 }else{
  text(s,query,request.variant==1?"Hello %s":request.variant==2?"{p}word{q}":inputs.simple_key);text(s,argument,"Ada");text(s,output,"initial-output");
  if(request.variant)s.memory.write<1>(toc-0x35ac,0);
  s.wr<4>(12,output);s.wr<4>(16,query);s.wr<4>(20,argument);s.wr<4>(24,0);s.wr<4>(28,0);
 }
 reset_diagnostics(s);
}
unsigned cases=0,returns=0,budgets=0,negatives=0,faults=0;
std::array<unsigned,4> root_returns{},root_modern_returns{};
std::uint64_t real_lookup_children=0,real_formatting_children=0,real_substitution_children=0;
void between(ActorAttributesIoPair& io){for(const auto& [guest,native]:io.old_forks.references){(void)guest;require(io.old_forks.forks.close(native)==0,"Release actual localization native fork");}io.old_forks.references.clear();io.between();}
std::uint64_t compare(const State& source,const Input& request,const Inputs& inputs,Path path,Mode mode,unsigned limit=full_limit,bool negative=false,bool fault=false,bool child_negative=false){
 const auto i=request.root;auto backing=paired_backing(source);auto& io=*backing;auto a=clone(source),b=clone(source);a->platform=io.original;b->platform=io.modern;prepare(*a,request,inputs);prepare(*b,request,inputs);Image(*a).compare(*b,"Original localization input preparations");require(a->executed==b->executed,"Localization preparation accounting");
 if(fault){if(i==1){a->wr<4>(8,0);b->wr<4>(8,0);}else{a->wr<4>(12,0);b->wr<4>(12,0);}}
 originals(*a);select(*b,i,mode);Witness witness;if(negative&&!child_negative)witness.negative_root=roots[i];witness.negative_child=child_negative;if(mode==Mode::modern)observe(*b,&witness);
 Clock old_clock,new_clock;ResourceInputs old_resources,new_resources;old_clock.io=new_clock.io=&io;old_clock.resources=&old_resources;new_clock.resources=&new_resources;new_clock.modern=true;
 a->code_fragment_observer=&old_clock;b->code_fragment_observer=&new_clock;DigestEffects x{*a},y{*b};a->memory.date_observer=&x;b->memory.date_observer=&y;std::uint64_t old_steps=0,new_steps=0;
 const auto expected=execute(*a,i,path,limit,old_steps);a->memory.date_observer=nullptr;a->platform.reset();between(io);
 const auto actual=execute(*b,i,path,limit,new_steps);a->memory.date_observer=b->memory.date_observer=nullptr;a->code_fragment_observer=b->code_fragment_observer=nullptr;observe(*b,nullptr);
 const auto label="Localization root="+hex_address(roots[i])+" variant="+std::to_string(request.variant)+" case="+std::to_string(cases)+" path="+std::to_string(unsigned(path))+" mode="+std::to_string(unsigned(mode));
 if(!expected.empty()&&limit==full_limit&&!fault)std::cerr<<label<<" pc="<<hex_address(a->pc)<<" error="<<expected<<std::endl;
 require(expected==actual,label+" original="+expected+" modern="+actual);require(fault?expected.starts_with("Unmapped data access at "):limit==full_limit?expected.empty():expected=="Native instruction budget exhausted",label+" unexpected outcome="+expected);
 require(old_clock.count==new_clock.count&&old_clock.events==new_clock.events,label+" actual import ABI/input order");
 if(negative){bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){rejected=true;}require(rejected,label+" negative must discriminate");++negatives;}
 else{Image(*a).compare(*b,label);require(x.events==y.events&&x.bytes==y.bytes&&x.digest()==y.digest()&&a->executed==b->executed&&old_steps==new_steps,label+" ordered timed effects/accounting");}io.compare();
 if(expected.empty()&&!negative){
  if(i==0)for(unsigned n=0;n<3;++n)require(!b->memory.read<4>(record+n*4),label+" independently zeroed record pointers");
  if(i==1)require(b->r<4>(12)==(request.variant?request.variant&255:2),label+" unsigned current-language byte");
  if(i==2){const bool found=request.variant!=3;require(b->r<4>(12)==unsigned(found),label+" lookup result");if(found){if(request.variant==2){require(text(*b,b->memory.read<4>(record))==inputs.disk_key&&text(*b,b->memory.read<4>(record+4))==inputs.disk_descriptor&&text(*b,b->memory.read<4>(record+8))==inputs.disk_translation,label+" independently parsed disk fields");for(unsigned n=0;n<10;++n)require(b->memory.read<4>(record+12+n*4)==inputs.disk_words[n],label+" independently decoded little-endian word");}else{const auto table=std::uint32_t(b->memory.read<4>(b->r<4>(8)-0x3590));for(unsigned n=0;n<3;++n)require(text(*b,b->memory.read<4>(record+n*4))==text(*b,b->memory.read<4>(table+n*4)),label+" actual first cached row strings");for(unsigned n=0;n<10;++n)require(b->memory.read<4>(record+12+n*4)==b->memory.read<4>(table+12+n*4),label+" actual cached metadata");}require(b->memory.read<4>(index_output)==(request.variant==2||request.variant==4?0x87654321u:0u),label+" optional index policy");}}
  if(i==3){require(b->r<4>(12)==output,label+" formatter destination");if(request.variant<2)require(text(*b,output)==(request.variant==0?inputs.simple_translation:"Hello Ada"),label+" actual independent plain/variadic output");}
 }
 const auto audit=[&](const auto& d,unsigned n){require(!d.original_body&&!d.dispatch_escapes,label+" original scope/dispatch residue");if(mode==Mode::modern){require(d.calls[n]>=1&&!d.original_calls[n]&&!d.profile_original_calls[n],label+" productive root");require(d.calls[n]==d.returns[n]+d.faults[n]+d.budget_stops[n]+d.transfers[n]+d.terminations[n],label+" balanced outcomes");require(expected.empty()?d.returns[n]==d.calls[n]:fault?d.faults[n]>=1&&!d.budget_stops[n]:d.budget_stops[n]>=1&&!d.faults[n],label+" productive outcome");}else{require(!d.calls[n]&&!d.returns[n],label+" original control entered native root");require(mode==Mode::profile?d.profile_original_calls[n]>=1:d.original_calls[n]>=1,label+" missing original control");}};
 if(i<3)audit(localization_lookup_diagnostics(*b),i);else audit(localization_format_diagnostics(*b),0);
 for(auto pc:witness.seen)if(expected.empty()&&!negative)for(unsigned n=0;n<roots.size();++n)if(roots[n]<=pc&&pc<ends[n])complete_sites[n].insert(pc);
 if(mode==Mode::modern&&expected.empty()&&!negative){if(i==3)real_lookup_children+=localization_lookup_diagnostics(*b).returns[2];real_formatting_children+=witness.formatting_returns;real_substitution_children+=witness.substitution_returns;}
 std::cout<<"case="<<cases<<" root="<<hex_address(roots[i])<<" variant="<<request.variant<<" path="<<unsigned(path)<<" mode="<<unsigned(mode)<<" steps="<<old_steps<<" events="<<x.events<<std::endl;
 root_modern_returns[i]+=expected.empty()&&!negative&&mode==Mode::modern;++cases;returns+=expected.empty();if(expected.empty()&&!negative)++root_returns[i];budgets+=!fault&&!expected.empty();faults+=fault;return old_steps;
}
void matrix(const State& source,const Input& request,const Inputs& inputs,bool profile){
 if(profile){compare(source,request,inputs,Path::generated,Mode::profile);compare(source,request,inputs,Path::cfm,Mode::profile);return;}
 std::uint64_t steps=0;for(auto path:{Path::direct,Path::generated,Path::cfm}){const auto n=compare(source,request,inputs,path,Mode::modern);if(path==Path::generated)steps=n;}
 compare(source,request,inputs,Path::generated,Mode::original);compare(source,request,inputs,Path::generated,Mode::entry_original);compare(source,request,inputs,Path::generated,Mode::modern,full_limit,true);
 for(auto cut:std::set<std::uint64_t>{0,steps/2,steps-1})compare(source,request,inputs,Path::generated,Mode::modern,unsigned(cut));compare(source,request,inputs,Path::generated,Mode::modern,full_limit,false,true);
}
struct OwnedBacking{std::filesystem::path path;~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(path,e)||e||std::filesystem::is_symlink(path,e)||e)return;for(const auto& p:std::filesystem::recursive_directory_iterator(path,e))if(e||p.is_symlink(e)||e)return;std::filesystem::remove_all(path,e);}};
Inputs inputs_from_original(const State& source,const std::filesystem::path& game){
 Inputs inputs;const auto toc=std::uint32_t(source.r<4>(8)),table=std::uint32_t(source.memory.read<4>(toc-0x3590));
 const auto plain=[](const std::string& value){return !value.empty()&&value.find_first_of("%{}[]<>\b\n\r")==std::string::npos;};
 for(unsigned n=0;n<source.memory.read<4>(toc-0x3594);++n){const auto key=text(source,source.memory.read<4>(table+n*52)),translated=text(source,source.memory.read<4>(table+n*52+8));if(plain(key)&&plain(translated)){inputs.simple_key=key;inputs.simple_translation=translated;break;}}
 require(!inputs.simple_key.empty(),"Actual catalog contains a plain translated key");
 std::ifstream file(game/"Data/language.ldb",std::ios::binary);require(bool(file),"Actual original language file");file.seekg(52);
 const auto field=[&](unsigned size){std::string value(size,'\0');file.read(value.data(),size);require(bool(file),"Complete original language field");value.resize(value.find('\0'));return value;};
 inputs.disk_key=field(550);inputs.disk_descriptor=field(30);inputs.disk_translation=field(550);for(auto& word:inputs.disk_words){std::array<unsigned char,4> bytes{};file.read(reinterpret_cast<char*>(bytes.data()),4);require(bool(file),"Complete original metadata word");word=std::uint32_t(bytes[0])|(std::uint32_t(bytes[1])<<8)|(std::uint32_t(bytes[2])<<16)|(std::uint32_t(bytes[3])<<24);}
 return inputs;
}
}
int main(int argc,char** argv){try{
 require(argc==5||(argc==6&&(std::string_view(argv[5])=="--profile"||std::string_view(argv[5])=="--probe")),"Expected export,game,private backing,original prepared snapshot and optional profile/probe");const bool profile=argc==6&&std::string_view(argv[5])=="--profile",probe=argc==6&&std::string_view(argv[5])=="--probe";require(profile==match_pc_profile_enabled(),"Localization profile flag mismatch");set_match_turbo_enabled(false);
 const auto directory=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());require(!std::filesystem::exists(directory),"Fresh private localization backing");OwnedBacking cleanup{directory};const auto snapshots=std::filesystem::path(argv[4]).parent_path();std::ifstream pointer(snapshots/"prepared-backing.txt");std::string name;std::getline(pointer,name);require(std::filesystem::path(name).parent_path().empty()&&name.starts_with("prepared-backing-"),"Retained original backing name");ActorAttributesIoPair::copy(snapshots/name,directory);
 State source;load_program(source,argv[1]);source.platform=std::make_shared<NativePlatform>(argv[2],directory,AudioMode::offline);load_match_snapshot(source,argv[4]);source.memory.map(fixture,0x10000);const auto toc=std::uint32_t(source.r<4>(8));require(toc==cm::semantic_io_checks::toc,"Original loaded TOC");
 require(setup(source,0x10726e90,{2,0})==1,"Whole original catalog initialization for supported language2");require(source.memory.read<4>(toc-0x3590)&&source.memory.read<4>(toc-0x3594)==9807&&source.memory.read<4>(toc-0x3598)==10&&source.memory.read<4>(toc-0x35a0)==1,"Real original catalog/table/header inputs");const auto inputs=inputs_from_original(source,argv[2]);
 for(unsigned i=0;i<4;++i)if(probe)compare(source,{i},inputs,Path::generated,Mode::modern);else matrix(source,{i},inputs,profile);
 if(!probe){for(auto byte:{256u,128u,255u})compare(source,{1,byte},inputs,Path::generated,profile?Mode::profile:Mode::modern);for(unsigned variant:{1u,2u,3u,4u})compare(source,{2,variant},inputs,Path::generated,profile?Mode::profile:Mode::modern);for(unsigned variant:{1u,2u})compare(source,{3,variant},inputs,Path::generated,profile?Mode::profile:Mode::modern);}
 if(!profile&&!probe)compare(source,{3},inputs,Path::generated,Mode::modern,full_limit,true,false,true);
 if(!profile){std::ofstream out(std::filesystem::path(argv[3]).parent_path()/"localization-runtime-sites.txt");for(unsigned n=0;n<roots.size();++n){require(root_returns[n]&&root_modern_returns[n]&&!complete_sites[n].empty(),"Every localization root needs complete actual-service returns");for(auto pc:runtime_sites[n])out<<n<<" "<<hex_address(pc)<<" "<<complete_sites[n].contains(pc)<<'\n';std::cout<<"Root "<<hex_address(roots[n])<<" runtime_sites="<<runtime_sites[n].size()<<" complete_sites="<<complete_sites[n].size()<<" returns="<<root_returns[n]<<" productive_returns="<<root_modern_returns[n]<<std::endl;}if(!probe)require(real_lookup_children&&real_formatting_children&&real_substitution_children,"Actual returning lookup/format/substitution composition");}
 std::cout<<"PASS: localization cases="<<cases<<" returns="<<returns<<" budgets="<<budgets<<" negatives="<<negatives<<" faults="<<faults<<" lookup_children="<<real_lookup_children<<" formatting_children="<<real_formatting_children<<" substitution_children="<<real_substitution_children<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
