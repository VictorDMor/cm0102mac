#include "money_configuration.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <set>
#include <fstream>
namespace cm::aot {bool money_configuration_reference(State&);std::vector<std::uint32_t> money_configuration_expected_sites();void money_configuration_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace money=cm::aot::money_configuration;
constexpr std::uint32_t tables=fixture+0x10000,globals=fixture+0x20000;
constexpr std::array<std::uint32_t,1> ends{0x1000b148};
constexpr std::array<unsigned,1> site_counts{834};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{money::configure_currency};
struct Options{unsigned interrupt=0;bool own_negative=false,effect_negative=false,redirect_flag=false,redirect_cached_slots=false,mutate_source=false;};
struct Trace:SemanticGuestObserver {
    std::vector<std::uint32_t> sites;bool effect_negative=false,damaged=false;
    void before_step(State& s,std::uint32_t pc)override {sites.push_back(pc);if(effect_negative&&!damaged&&s.executed<s.budget){damaged=true;const auto v=s.memory.read<1>(globals+0x700);s.memory.write<1>(globals+0x700,v^1);s.memory.write<1>(globals+0x700,v);}}
};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(money::entries.begin(),money::entries.end(),root);require(i!=money::entries.end(),"Known currency configuration root");return unsigned(i-money::entries.begin());}
struct TimedEffects:MemoryObserver {
    State& state;Effects effects;std::vector<std::uint32_t> pcs;
    explicit TimedEffects(State& s):state(s){effects.state=&s;}
    void mapped(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.mapped(a,n);}
    void written(std::uint32_t a,const std::uint8_t* d,std::size_t n)override{pcs.push_back(state.pc);effects.written(a,d,n);}
    void cleared(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.cleared(a,n);}
    void read(std::uint32_t a,const std::uint8_t* d,unsigned n)override{pcs.push_back(state.pc);effects.read(a,d,n);}
};
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
std::string text(State& s,std::uint32_t at){std::string out;for(unsigned n=0;n<1024;++n){const auto byte=s.memory.read<1>(at+n);if(!byte)return out;out.push_back(char(byte));}throw std::runtime_error("Unterminated currency configuration fixture string");}
void put(State& s,std::uint32_t at,const std::string& value){std::vector<std::uint8_t> bytes(value.begin(),value.end());bytes.push_back(0);s.memory.copy_in(at,bytes);}
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;std::array<unsigned,1> completed{};
    Model(bool old,Options o,Trace& t):original(old),options(o),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto n=index(s.pc),root=s.pc;while(s.pc!=stop){if(root<=s.pc&&s.pc<ends[n])require(money_configuration_reference(s),"Complete original currency configuration reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            const auto entry=std::find(money::entries.begin(),money::entries.end(),target);
            if(entry!=money::entries.end()){
                const auto n=unsigned(entry-money::entries.begin());if(original)reference(s,stop);else methods[n](s,this,&trace);++completed[n];
            }
            else {
                const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};std::uint32_t result=globals+0x900;
                // Byte copying and four diagnostics are explicit external models.
                // Preserve ordered reads/writes even when guest buffers alias.
                s.memory.write<4>(globals+0x500,target);s.memory.write<4>(globals+0x504,s.memory.read<4>(globals+0x504)+1);
                if(target==0x1077dbf0){
                    const auto from=arg(4),to=arg(3);result=to;
                    for(unsigned n=0;;++n){require(n<1024,"Terminated currency copy fixture");const auto byte=s.memory.read<1>(from+n);s.memory.write<1>(to+n,byte);if(!byte)break;}
                    if(serial==0&&options.redirect_cached_slots)for(auto offset:{0x59d0u,0x59e0u,0x59e4u,0x59e8u,0x59ecu,0x59f0u})s.memory.write<4>(toc-offset,globals+0x3000);
                    if(serial==0&&options.mutate_source)s.memory.write<1>(std::uint32_t(s.memory.read<4>(toc-0x59d8))+0x21,'Z');
                }
                else if(target==0x10739890){(void)text(s,arg(3));put(s,arg(6),"X");put(s,arg(7),"Y");}
                else if(target==0x10768810)result=globals+0x920;
                else if(target==0x1077b720){
                    // Explicit printf boundary: preserve byte strings, signed
                    // character low bytes, argument spill reads and call ABI.
                    const auto format=text(s,arg(4));std::string out;unsigned next=5;
                    const auto word=[&](){const auto i=next++;return i<=10?arg(i):std::uint32_t(s.memory.read<4>(arg(1)+0x38+(i-11)*4));};
                    for(std::size_t i=0;i<format.size();++i){if(format[i]!='%'){out+=format[i];continue;}require(++i<format.size(),"Complete money format");const auto v=word();if(format[i]=='s')out+=text(s,v);else if(format[i]=='c')out+=char(v);else if(format[i]=='d')out+=std::to_string(std::bit_cast<std::int32_t>(v));else throw std::runtime_error("Unknown money format conversion");}
                    put(s,arg(3),out);result=std::uint32_t(out.size());
                }
                else if(target==0x10769b60){(void)text(s,arg(3));(void)text(s,arg(4));if(options.redirect_flag)s.memory.write<4>(toc-0x7bac,globals+0x401);}
                else throw std::runtime_error("Unknown currency configuration dependency "+hex_address(target));
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
                s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            }
            require(s.pc==stop,"Currency configuration dependency return ABI");calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Currency configuration fixture interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x40000);s->memory.map(stack-0x1000,0x2000);s->memory.map(toc-0x8000,0x9000);s->pc=money::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;
    for(unsigned f=1280;f<=1311;++f)s->wr<1>(f,0);
    money_configuration_fixture_tables(*s,toc,tables);
    s->memory.write<4>(toc-0x7ba8,globals+0x1000);s->memory.write<4>(toc-0x7bac,globals+0x400);s->memory.write<1>(globals+0x400,255);s->memory.write<1>(globals+0x401,255);put(*s,globals+0x920,"TEST");
    for(auto offset:{0x59f0u,0x59ecu,0x59e8u,0x59e4u,0x59e0u,0x59d0u})put(*s,std::uint32_t(s->memory.read<4>(toc-offset)),"OLD PREFIX AND SUFFIX");
    for(auto offset:{0x3aecu,0x3af4u,0x3afcu})s->memory.write<8>(toc-offset,UINT64_C(0x7ff8000012345678));
    for(unsigned n=0;n<4;++n)s->memory.write<1>(toc-0x3b02+n,0xa5);
    s->wr<4>(12,4);return s;
}
unsigned cases=0,returns=0,stops=0,negatives=0;std::array<unsigned,1> root_cases{},root_returns{},root_stops{},root_negatives{};std::set<std::uint32_t> complete_sites,all_sites;std::array<std::set<std::uint32_t>,1> root_complete_sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;std::string outcome;std::vector<Event> dependencies;};
std::array<std::uint64_t,1> child_returns{};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);Trace ot,nt;nt.effect_negative=options.effect_negative;Model old(true,options,ot),modern(false,options,nt);a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto run=[&](State& s,Model& d,bool reference){try{if(reference){original_trace=&ot;d.reference(s);}else methods[index(s.pc)](s,&d,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Currency configuration root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Currency configuration fixture interruption"||expected.starts_with("Unmapped data access at "),label+" unexpected reference: "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    const auto negative=options.own_negative||options.effect_negative;if(negative)require(expected.empty(),label+" negative needs a complete return");if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);if(options.effect_negative){require(nt.damaged,label+" effect negative must execute");Image(*a).compare(*b,label+" restored effect image");}
    bool rejected=false;try{require(old.calls==modern.calls,label+" dependency ABI/order");require(x.effects.bytes==y.effects.bytes&&x.pcs==y.pcs,label+" ordered timed effects");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" instruction count");Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!negative)throw;rejected=true;}require(rejected==negative,label+" discriminating negative");
    const auto root=index(input.pc);++cases;++root_cases[root];returns+=expected.empty()&&!negative;root_returns[root]+=expected.empty()&&!negative;stops+=!expected.empty();root_stops[root]+=!expected.empty();negatives+=negative;root_negatives[root]+=negative;if(!negative){all_sites.insert(ot.sites.begin(),ot.sites.end());if(expected.empty()){complete_sites.insert(ot.sites.begin(),ot.sites.end());for(auto pc:ot.sites)if(money::entries[root]<=pc&&pc<ends[root])root_complete_sites[root].insert(pc);}}
    if(expected.empty()&&!negative)for(unsigned n=0;n<1;++n)child_returns[n]+=modern.completed[n];
    return{std::move(a),x.effects.log,unsigned(old.calls.size()),expected,std::move(old.calls)};
}
void boundaries(const State& input,Options options={}){
    const auto base=compare(input,options);require(base.outcome.empty()&&base.state->pc==return_sentinel,"Currency configuration boundary baseline return");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<5;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/4));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [at,size]:input.memory.mapped_ranges())for(auto [address,n]:hole<at||std::uint64_t(hole)>=std::uint64_t(at)+size?std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,size}}:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,hole-at},{hole+1,std::uint64_t(at)+size-hole-1}})if(n){s->memory.map(address,n);s->memory.copy_in(address,input.memory.copy_out(address,n));}require(compare(*s,options).outcome.starts_with("Unmapped data access at "),"Removed money byte must fault");}
    for(unsigned cut:std::set<unsigned>{1,base.calls/2,base.calls})if(cut&&cut<=base.calls){auto o=options;o.interrupt=cut;compare(input,o);}
}
constexpr std::array<std::uint32_t,6> buffer_offsets{0x59d0,0x59e0,0x59e4,0x59e8,0x59ec,0x59f0};
void corpus(bool probe){
    auto initial=seed(0);if(probe){compare(*initial);return;}
    Options o;o.own_negative=true;compare(*initial,o);o.own_negative=false;o.effect_negative=true;compare(*initial,o);
    for(unsigned byte=0;byte<256;++byte)for(auto high:{0u,0x100u,0x12340000u,0x80000000u,0xffff0000u}){auto s=seed(0);s->wr<4>(12,high|byte);const auto out=compare(*s);require(out.state->r<4>(12)==(byte<25),"Currency selection uses signed low byte and accepts25 choices");if(byte<25){require(out.calls==6&&out.state->memory.read<1>(toc-0x3b02)==byte,"Six ordered copies precede selected-index publication");}else{require(out.calls==4&&!out.state->memory.read<1>(globals+0x400),"Invalid index calls four diagnostics and clears shared flag");require(out.state->memory.read<8>(toc-0x3aec)==s->memory.read<8>(toc-0x3aec)&&out.state->memory.read<4>(toc-0x3b02)==0xa5a5a5a5,"Invalid choice leaves scales and settings untouched");}}
    for(unsigned index=0;index<25;++index){auto s=seed(0);s->wr<4>(12,index);boundaries(*s);
        for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x7ff0000000000001),UINT64_C(0x7ff8000012345678),UINT64_C(0xfff0000000000000),UINT64_C(0xffffffffffffffff)}){auto f=clone(*s);for(unsigned offset=0;offset<328;offset+=8)f->memory.write<8>(tables+0x1000+offset,bits);compare(*f);}
        for(auto destination:{globals+0x400,globals+0x1000,globals+0x2000,tables+0x1000,tables+0x2000,toc-0x3afc,toc-0x59e0,stack-0x60,stack-0x20}){auto a=clone(*s);a->memory.write<4>(toc-0x59d0,destination);compare(*a);}
        {auto a=clone(*s);for(auto offset:buffer_offsets)a->memory.write<4>(toc-offset,globals+0x2000);compare(*a);}
    }
    for(auto index:{25u,127u,128u,255u}){auto s=seed(0);s->wr<4>(12,index);boundaries(*s);}
    for(auto index:{0u,4u,20u,21u,24u})for(auto slot:buffer_offsets){auto s=seed(0);s->wr<4>(12,index);s->memory.write<4>(toc-slot,0);require(compare(*s).outcome.starts_with("Unmapped data access at "),"Required output buffer retains null fault");}
    for(auto slot:{0x59f4u,0x59d4u,0x59d8u}){auto s=seed(0);s->memory.write<4>(toc-slot,0);require(compare(*s).outcome.starts_with("Unmapped data access at "),"Required dispatch/scale/text bank retains null fault");}
    {auto s=seed(0);s->wr<4>(8,0);require(compare(*s).outcome.starts_with("Unmapped data access at "),"Required TOC retains null fault");}
    {auto s=seed(0);s->memory.write<4>(toc-0x7ba8,0);require(compare(*s).outcome.empty(),"Valid choice does not dereference diagnostic scratch");}
    {auto s=seed(0);s->wr<4>(12,255);for(auto slot:{0x59f4u,0x59d4u,0x59d0u,0x59e0u,0x59e4u,0x59e8u,0x59ecu,0x59f0u})s->memory.write<4>(toc-slot,0);require(compare(*s).state->r<4>(12)==0,"Invalid choice does not dereference unused profile banks or buffers");}
    {auto s=seed(0);s->wr<4>(12,255);Options redirected;redirected.redirect_flag=true;const auto out=compare(*s,redirected);require(out.state->memory.read<1>(globals+0x400)==255&&!out.state->memory.read<1>(globals+0x401),"Diagnostic flag pointer is read after the services");}
    {auto s=seed(0);s->wr<4>(12,0);Options cached;cached.redirect_cached_slots=true;auto out=compare(*s,cached);require(text(*out.state,tables+0x3000)=="T"&&text(*out.state,globals+0x3000).empty(),"All six destination pointers were cached before the first copy");}
    {auto s=seed(0);s->wr<4>(12,0);Options mutated;mutated.mutate_source=true;auto out=compare(*s,mutated);require(text(*out.state,tables+0x5000)=="Z","Suffix source text is read at the later copy");}
    {auto s=seed(0);const auto first=s->memory.read<4>(tables),fourth=s->memory.read<4>(tables+4*4);s->memory.write<4>(tables,fourth);s->memory.write<4>(tables+4*4,first);s->wr<4>(12,0);auto out=compare(*s);require(text(*out.state,tables+0x8000)=="R$"&&out.state->memory.read<1>(toc-0x3b02)==0,"Dispatch uses the guest table while publication retains the input byte");}
    {auto s=seed(0);const auto out=compare(*s);require(out.state->memory.read<8>(toc-0x3aec)==float_bits<8>(2.73)&&out.state->memory.read<8>(toc-0x3af4)==float_bits<8>(3.0)&&out.state->memory.read<8>(toc-0x3afc)==float_bits<8>(1.0),"Independent Real profile scale values");require(text(*out.state,tables+0x8000)=="R$"&&text(*out.state,tables+0x7000)==",00"&&out.state->memory.read<1>(toc-0x3b00)==','&&out.state->memory.read<1>(toc-0x3aff)=='.',"Independent Real symbol,units and separators");}
    {auto s=seed(0);s->wr<4>(12,20);auto out=compare(*s);require(text(*out.state,tables+0x4000).empty(),"Original Pta configuration leaves the billions suffix empty");}
    {auto s=seed(0);s->wr<4>(12,21);auto out=compare(*s);require(out.state->memory.read<1>(toc-0x3aff)==' ',"Original SKr configuration groups with ASCII space");}
}

}
namespace cm::aot {void money_configuration_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{require(argc==1||argc==2,"Optional coverage output or probe");const bool probe=argc==2&&std::string_view(argv[1])=="--probe";corpus(probe);const auto expected=money_configuration_expected_sites();if(argc==2&&!probe){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<all_sites.contains(pc)<<" "<<complete_sites.contains(pc)<<'\n';}if(!probe)for(auto pc:expected)if(!complete_sites.contains(pc))std::cerr<<"Uncovered currency configuration site "<<hex_address(pc)<<std::endl;std::cout<<"Currency configuration comparisons="<<cases<<" returns="<<returns<<" stops="<<stops<<" negatives="<<negatives<<" completed_sites="<<complete_sites.size()<<"/"<<expected.size()<<std::endl;for(unsigned n=0;n<1;++n)std::cout<<"Root "<<hex_address(money::entries[n])<<" comparisons="<<root_cases[n]<<" returns="<<root_returns[n]<<" stops="<<root_stops[n]<<" negatives="<<root_negatives[n]<<" completed_sites="<<root_complete_sites[n].size()<<std::endl;if(!probe){require(complete_sites.size()==expected.size(),"Every currency configuration instruction needs complete-return evidence");for(unsigned n=0;n<1;++n)require(root_complete_sites[n].size()==site_counts[n],"Every entry needs its own complete-return source coverage");require(std::all_of(root_negatives.begin(),root_negatives.end(),[](auto n){return n==2;}),"Every currency configuration root needs result and restored-effect negatives");}}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
