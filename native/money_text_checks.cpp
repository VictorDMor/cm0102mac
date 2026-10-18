#include "money_text.hpp"
#include "compiler_arithmetic.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <set>
#include <fstream>
namespace cm::aot {bool money_text_reference(State&);std::vector<std::uint32_t> money_text_expected_sites();void money_text_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace money=cm::aot::money_text;
constexpr std::uint32_t amount=fixture+0x1000,tables=fixture+0x10000,globals=fixture+0x20000;
constexpr std::array<std::uint32_t,4> ends{0x10009f18,0x1000b158,0x1000b410,0x1000c764};
constexpr std::array<unsigned,4> site_counts{10,2,172,1236};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{money::selected_symbol_length,money::display_setting,money::currency_label,money::format_amount};
struct Options{unsigned interrupt=0;bool own_negative=false,effect_negative=false;};
struct Trace:SemanticGuestObserver {
    std::vector<std::uint32_t> sites;bool effect_negative=false,damaged=false;
    void before_step(State& s,std::uint32_t pc)override {sites.push_back(pc);if(effect_negative&&!damaged&&s.executed<s.budget){damaged=true;const auto v=s.memory.read<1>(globals+0x700);s.memory.write<1>(globals+0x700,v^1);s.memory.write<1>(globals+0x700,v);}}
};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(money::entries.begin(),money::entries.end(),root);require(i!=money::entries.end(),"Known money text root");return unsigned(i-money::entries.begin());}
struct TimedEffects:MemoryObserver {
    State& state;Effects effects;std::vector<std::uint32_t> pcs;
    explicit TimedEffects(State& s):state(s){effects.state=&s;}
    void mapped(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.mapped(a,n);}
    void written(std::uint32_t a,const std::uint8_t* d,std::size_t n)override{pcs.push_back(state.pc);effects.written(a,d,n);}
    void cleared(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.cleared(a,n);}
    void read(std::uint32_t a,const std::uint8_t* d,unsigned n)override{pcs.push_back(state.pc);effects.read(a,d,n);}
};
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
std::string text(State& s,std::uint32_t at){std::string out;for(unsigned n=0;n<1024;++n){const auto byte=s.memory.read<1>(at+n);if(!byte)return out;out.push_back(char(byte));}throw std::runtime_error("Unterminated money text fixture string");}
void put(State& s,std::uint32_t at,const std::string& value){std::vector<std::uint8_t> bytes(value.begin(),value.end());bytes.push_back(0);s.memory.copy_in(at,bytes);}
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;std::array<unsigned,4> completed{};
    Model(bool old,Options o,Trace& t):original(old),options(o),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto n=index(s.pc),root=s.pc;while(s.pc!=stop){if(root<=s.pc&&s.pc<ends[n])require(money_text_reference(s),"Complete original money text reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            const auto entry=std::find(money::entries.begin(),money::entries.end(),target);
            if(entry!=money::entries.end()){
                const auto n=unsigned(entry-money::entries.begin());if(original)reference(s,stop);else methods[n](s,this,&trace);++completed[n];
            } else if(target==0x10773b70)compiler_arithmetic::signed_remainder(s);
            else if(target==0x10773960)compiler_arithmetic::signed_quotient(s);
            else {
                const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};std::uint32_t result=globals+0x900;
                // Strings, formatting and diagnostics are explicit external
                // models. The two accepted arithmetic bodies above are reused
                // on both sides; their original proofs are not redefined.
                s.memory.write<4>(globals+0x500,target);s.memory.write<4>(globals+0x504,s.memory.read<4>(globals+0x504)+1);
                if(target==0x1077dbd0)result=std::uint32_t(text(s,arg(3)).size());
                else if(target==0x1077dbf0){const auto source=text(s,arg(4));put(s,arg(3),source);result=arg(3);}
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
                else if(target==0x10769b60){(void)text(s,arg(3));(void)text(s,arg(4));}
                else throw std::runtime_error("Unknown money text dependency "+hex_address(target));
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
                s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            }
            require(s.pc==stop,"Money text dependency return ABI");calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Money text fixture interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x40000);s->memory.map(stack-0x1000,0x2000);s->memory.map(toc-0x8000,0x9000);s->pc=money::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;
    for(unsigned f=1280;f<=1311;++f)s->wr<1>(f,0);
    money_text_fixture_tables(*s,toc,tables);
    s->memory.write<4>(toc-0x7ba8,globals+0x1000);s->memory.write<4>(toc-0x7bac,globals+0x400);s->memory.write<1>(globals+0x400,255);put(*s,globals+0x920,"TEST");
    for(auto [offset,value]:std::array<std::pair<std::uint32_t,const char*>,6>{{{0x59f0,"T"},{0x59ec,"B"},{0x59e8,"M"},{0x59e4,"K"},{0x59e0,""},{0x59d0,"$ "}}})put(*s,std::uint32_t(s->memory.read<4>(toc-offset)),value);
    s->memory.write<1>(toc-0x3b00,'.');s->memory.write<1>(toc-0x3aff,',');
    put(*s,amount,"INITIAL");s->wr<4>(12,amount);s->wr<4>(16,100);s->wr<4>(20,n==2?3:0);s->wr<4>(24,1234567);s->wr<4>(28,2);
    return s;
}
unsigned cases=0,returns=0,stops=0,negatives=0;std::array<unsigned,4> root_cases{},root_returns{},root_stops{},root_negatives{};std::set<std::uint32_t> complete_sites,all_sites;std::array<std::set<std::uint32_t>,4> root_complete_sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;std::string outcome;std::vector<Event> dependencies;};
std::array<std::uint64_t,4> child_returns{};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);Trace ot,nt;nt.effect_negative=options.effect_negative;Model old(true,options,ot),modern(false,options,nt);a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto run=[&](State& s,Model& d,bool reference){try{if(reference){original_trace=&ot;d.reference(s);}else methods[index(s.pc)](s,&d,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Money text root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Money text fixture interruption"||expected.starts_with("Unmapped data access at "),label+" unexpected reference: "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    const auto negative=options.own_negative||options.effect_negative;if(negative)require(expected.empty(),label+" negative needs a complete return");if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);if(options.effect_negative){require(nt.damaged,label+" effect negative must execute");Image(*a).compare(*b,label+" restored effect image");}
    bool rejected=false;try{require(old.calls==modern.calls,label+" dependency ABI/order");require(x.effects.bytes==y.effects.bytes&&x.pcs==y.pcs,label+" ordered timed effects");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" instruction count");Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!negative)throw;rejected=true;}require(rejected==negative,label+" discriminating negative");
    const auto root=index(input.pc);++cases;++root_cases[root];returns+=expected.empty()&&!negative;root_returns[root]+=expected.empty()&&!negative;stops+=!expected.empty();root_stops[root]+=!expected.empty();negatives+=negative;root_negatives[root]+=negative;if(!negative){all_sites.insert(ot.sites.begin(),ot.sites.end());if(expected.empty()){complete_sites.insert(ot.sites.begin(),ot.sites.end());for(auto pc:ot.sites)if(money::entries[root]<=pc&&pc<ends[root])root_complete_sites[root].insert(pc);}}
    if(expected.empty()&&!negative)for(unsigned n=0;n<4;++n)child_returns[n]+=modern.completed[n];
    return{std::move(a),x.effects.log,unsigned(old.calls.size()),expected,std::move(old.calls)};
}
void boundaries(const State& input,Options options={}){
    const auto base=compare(input,options);require(base.outcome.empty()&&base.state->pc==return_sentinel,"Money text boundary baseline return");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<5;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/4));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [at,size]:input.memory.mapped_ranges())for(auto [address,n]:hole<at||std::uint64_t(hole)>=std::uint64_t(at)+size?std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,size}}:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,hole-at},{hole+1,std::uint64_t(at)+size-hole-1}})if(n){s->memory.map(address,n);s->memory.copy_in(address,input.memory.copy_out(address,n));}require(compare(*s,options).outcome.starts_with("Unmapped data access at "),"Removed money byte must fault");}
    for(unsigned cut:std::set<unsigned>{1,base.calls/2,base.calls})if(cut&&cut<=base.calls){auto o=options;o.interrupt=cut;compare(input,o);}
}
void set_amount(State& s,std::uint64_t v){s.wr<4>(20,std::uint32_t(v>>32));s.wr<4>(24,std::uint32_t(v));}
void corpus(bool probe){
    for(unsigned n=0;n<4;++n){auto s=seed(n);if(probe){compare(*s);continue;}boundaries(*s);Options o;o.own_negative=true;compare(*s,o);o.own_negative=false;o.effect_negative=true;compare(*s,o);}
    if(probe)return;
    for(unsigned length=0;length<256;++length){auto s=seed(0);put(*s,std::uint32_t(s->memory.read<4>(toc-0x59d0)),std::string(length,'X'));auto out=compare(*s);require(out.state->r<4>(12)==std::uint32_t(signed_value<1>(length)),"Selected symbol length preserves signed low byte");}
    for(auto length:{256u,257u,511u,512u,1022u}){auto s=seed(0);put(*s,std::uint32_t(s->memory.read<4>(toc-0x59d0)),std::string(length,'X'));require(compare(*s).state->r<4>(12)==std::uint32_t(signed_value<1>(length)),"Long symbol lengths retain low-byte wrapping");}
    {auto s=seed(0);s->memory.write<4>(toc-0x59d0,0);require(compare(*s).outcome.starts_with("Unmapped data access at "),"Mandatory symbol pointer retains null fault");}
    for(unsigned value=0;value<256;++value){auto s=seed(1);s->memory.write<1>(toc-0x3b02,value);require(compare(*s).state->r<4>(12)==value,"Display setting preserves unsigned byte");}
    for(unsigned id=0;id<256;++id){auto s=seed(2);s->wr<4>(20,id|0x12340000);auto out=compare(*s);require(out.state->r<4>(12)==(id<25),"Currency id uses signed low byte and accepts exactly25 labels");if(id>=25)require(!out.state->memory.read<1>(globals+0x400),"Unknown currency clears diagnostic flag");}
    for(unsigned n:{2u,3u})for(auto capacity:{0u,1u,24u,25u,49u,50u,127u,255u,32767u,32768u,65535u,0x10019u,0x10032u})for(auto pointer:{amount,0u}){auto s=seed(n);s->wr<4>(12,pointer);s->wr<4>(16,capacity);const bool valid=pointer&&signed_value<2>(capacity)>=(n==2?50:25);auto out=compare(*s);require(out.state->r<4>(12)==valid,"Capacity uses signed low halfword with explicit null rejection");}
    std::set<std::uint64_t> amounts{0,1,9,10,99,100,999,1000,9999,10000,99999,100000,999999,1000000,9999999,10000000,123456789012345,999999999999999,1000000000000000,UINT64_MAX,UINT64_C(0x8000000000000000)};
    std::uint64_t power=1;for(unsigned digits=1;digits<=15;++digits){for(auto delta:{-1,0,1})amounts.insert(power+delta);for(unsigned pos=0;pos<5&&pos<digits;++pos){std::uint64_t step=1;for(unsigned k=pos+1;k<digits;++k)step*=10;amounts.insert(power+7*step);}power*=10;}
    for(auto value:amounts)for(auto flags:{0u,2u,255u})for(bool four:{false,true})for(bool billion:{false,true}){auto s=seed(3);set_amount(*s,value);s->wr<4>(28,flags|0xabcd0000);s->memory.write<1>(toc-0x3b01,four);if(!billion)put(*s,std::uint32_t(s->memory.read<4>(toc-0x59ec)),"");compare(*s);}
    for(unsigned flag=0;flag<256;++flag){auto s=seed(3);s->wr<4>(28,flag);compare(*s);}
    for(auto group:{0u,32u,44u,127u,128u,255u})for(auto decimal:{0u,32u,46u,127u,128u,255u})for(auto flags:{0u,2u})for(auto value:{UINT64_C(1234567),UINT64_C(1234567890),UINT64_C(123456789012345)}){auto s=seed(3);s->memory.write<1>(toc-0x3aff,group);s->memory.write<1>(toc-0x3b00,decimal);set_amount(*s,value);s->wr<4>(28,flags);compare(*s);}
    for(auto value:{UINT64_C(0),UINT64_C(1234),UINT64_C(1234567),UINT64_C(123456789012345)})for(auto flags:{0u,2u}){auto s=seed(3);set_amount(*s,value);s->wr<4>(28,flags);boundaries(*s);}
    for(unsigned n:{2u,3u}){auto s=seed(n);s->wr<4>(12,0);boundaries(*s);s=seed(n);s->wr<4>(16,1);boundaries(*s);}
    {auto s=seed(2);s->wr<4>(20,255);boundaries(*s);}
    for(unsigned n:{0u,2u,3u}){auto s=seed(n);s->wr<4>(8,0);require(compare(*s).outcome.starts_with("Unmapped data access at "),"Required TOC fault retained");}
    for(unsigned n:{2u,3u})for(auto pointer:{amount+1,globals+0x400,globals+0x1000,tables+0x6000,tables+0xa000}){auto s=seed(n);s->wr<4>(12,pointer);compare(*s);}
    for(auto value:{UINT64_C(0),UINT64_C(1),UINT64_C(1234),UINT64_C(1234567)}){auto s=seed(3);set_amount(*s,value);const auto out=compare(*s);std::string expected=value==0?"$|0":value==1?"$|1":value==1234?"$|1,234":"$|1,234,567";require(text(*out.state,amount)==expected,"Independent grouped amount text and space substitution");}
    for(auto [value,expected]:std::array<std::pair<std::uint64_t,const char*>,3>{{{1000000,"$|1M"},{1200000,"$|1.2M"},{1230000,"$|1.23M"}}}){auto s=seed(3);set_amount(*s,value);s->wr<4>(28,0);require(text(*compare(*s).state,amount)==expected,"Independent trailing decimal zero suppression");}
    for(auto setting:{0u,1u,127u,128u,255u}){auto s=seed(3);set_amount(*s,1234);s->memory.write<1>(toc-0x3b01,setting);require(text(*compare(*s).state,amount)==(setting?"$|1234":"$|1,234"),"Four digit grouping setting uses zero versus nonzero");}
    for(auto symbol:{"","$ ","USD ","A B ","EUR"})for(auto suffix:{""," M"," million"})for(auto flags:{0u,2u}){auto s=seed(3);set_amount(*s,1234567);s->wr<4>(28,flags);put(*s,std::uint32_t(s->memory.read<4>(toc-0x59d0)),symbol);put(*s,std::uint32_t(s->memory.read<4>(toc-0x59e8)),suffix);compare(*s);}
    {auto s=seed(3);set_amount(*s,1234567);s->wr<4>(28,0);require(text(*compare(*s).state,amount)=="$|1.23M","Independent compact amount layout");}
}
}
namespace cm::aot {void money_text_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{require(argc==1||argc==2,"Optional coverage output or probe");const bool probe=argc==2&&std::string_view(argv[1])=="--probe";corpus(probe);const auto expected=money_text_expected_sites();if(argc==2&&!probe){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<all_sites.contains(pc)<<" "<<complete_sites.contains(pc)<<'\n';}if(!probe)for(auto pc:expected)if(!complete_sites.contains(pc))std::cerr<<"Uncovered money text site "<<hex_address(pc)<<std::endl;std::cout<<"Money text comparisons="<<cases<<" returns="<<returns<<" stops="<<stops<<" negatives="<<negatives<<" completed_sites="<<complete_sites.size()<<"/"<<expected.size()<<std::endl;for(unsigned n=0;n<4;++n)std::cout<<"Root "<<hex_address(money::entries[n])<<" comparisons="<<root_cases[n]<<" returns="<<root_returns[n]<<" stops="<<root_stops[n]<<" negatives="<<root_negatives[n]<<" completed_sites="<<root_complete_sites[n].size()<<std::endl;if(!probe){require(complete_sites.size()==expected.size(),"Every money text instruction needs complete-return evidence");for(unsigned n=0;n<4;++n)require(root_complete_sites[n].size()==site_counts[n],"Every entry needs its own complete-return source coverage");require(std::all_of(root_negatives.begin(),root_negatives.end(),[](auto n){return n==2;}),"Every money text root needs result and restored-effect negatives");}}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
