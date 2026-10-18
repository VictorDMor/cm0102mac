#include "contract_display.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <set>
#include <fstream>
namespace cm::aot {bool contract_display_reference(State&);std::vector<std::uint32_t> contract_display_expected_sites();void contract_display_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace display=cm::aot::contract_display;
constexpr std::uint32_t record=fixture+0x1000,output=fixture+0x4000,tables=fixture+0x10000,globals=fixture+0x20000;
constexpr std::array<std::uint32_t,2> ends{0x10372724,0x103727bc};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{display::format_wage_text,display::format_contract_end};
struct Options{unsigned interrupt=0;bool own_negative=false,effect_negative=false,rewrite_preferences=false;std::uint32_t changed_preferences=0,high=0x12345678,low=0x89abcdef,result=0x76543210;std::string number="NUM";};
struct Trace:SemanticGuestObserver {
    std::vector<std::uint32_t> sites;bool effect_negative=false,damaged=false;
    void before_step(State& s,std::uint32_t pc)override {sites.push_back(pc);if(effect_negative&&!damaged&&s.executed<s.budget){damaged=true;const auto v=s.memory.read<1>(globals+0x700);s.memory.write<1>(globals+0x700,v^1);s.memory.write<1>(globals+0x700,v);}}
};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(display::entries.begin(),display::entries.end(),root);require(i!=display::entries.end(),"Known display root");return unsigned(i-display::entries.begin());}
struct TimedEffects:MemoryObserver {
    State& state;Effects effects;std::vector<std::uint32_t> pcs;
    explicit TimedEffects(State& s):state(s){effects.state=&s;}
    void mapped(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.mapped(a,n);}
    void written(std::uint32_t a,const std::uint8_t* d,std::size_t n)override{pcs.push_back(state.pc);effects.written(a,d,n);}
    void cleared(std::uint32_t a,std::uint64_t n)override{pcs.push_back(state.pc);effects.cleared(a,n);}
    void read(std::uint32_t a,const std::uint8_t* d,unsigned n)override{pcs.push_back(state.pc);effects.read(a,d,n);}
};
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
std::string text(State& s,std::uint32_t at){std::string out;for(unsigned n=0;n<1024;++n){const auto byte=s.memory.read<1>(at+n);if(!byte)return out;out.push_back(char(byte));}throw std::runtime_error("Unterminated display fixture string");}
void put(State& s,std::uint32_t at,const std::string& value){std::vector<std::uint8_t> bytes(value.begin(),value.end());bytes.push_back(0);s.memory.copy_in(at,bytes);}
struct Model:SemanticDependencyDispatch {
    Options options;std::vector<Event> calls;
    Model(bool,Options o):options(std::move(o)){}
    void reference(State& s){const auto n=index(s.pc),root=s.pc;while(s.pc!=return_sentinel){if(root<=s.pc&&s.pc<ends[n])require(contract_display_reference(s),"Complete original display reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
            // Explicit external models expose each argument, output effect and
            // volatile residue; they make no claim about the helper algorithms.
            s.memory.write<4>(globals+0x500,target);s.memory.write<4>(globals+0x504,s.memory.read<4>(globals+0x504)+1);
            if(target==0x10009f20){s.memory.write<4>(globals+0x510,s.memory.read<4>(arg(3)));s.memory.write<4>(globals+0x514,arg(4));}
            else if(target==0x1000b410){require(arg(4)==200,"Numeric output capacity ABI");s.memory.write<4>(globals+0x518,arg(5));s.memory.write<4>(globals+0x51c,arg(6));s.memory.write<4>(globals+0x520,arg(7));put(s,arg(3),options.number);if(options.rewrite_preferences)s.memory.write<4>(globals+0x104,options.changed_preferences);}
            else if(target==0x1077b720||target==0x10727890){auto label=text(s,arg(4));s.memory.write<4>(globals+0x524,arg(4));const auto mark=label.find("%s");if(mark!=std::string::npos)label.replace(mark,2,text(s,arg(5)));put(s,arg(3),label);}
            else if(target==0x10715e00){require(arg(5)==200&&arg(6)==255,"End-date capacity and format ABI");const auto day=std::uint16_t(s.memory.read<2>(arg(3))),year=std::uint16_t(s.memory.read<2>(arg(3)+2));const auto flag=std::uint8_t(s.memory.read<1>(arg(3)+4));s.memory.write<2>(globals+0x540,day);s.memory.write<2>(globals+0x542,year);s.memory.write<1>(globals+0x544,flag);put(s,arg(4),"DATE:"+std::to_string(day)+":"+std::to_string(year)+":"+std::to_string(flag));}
            else throw std::runtime_error("Unknown display dependency "+hex_address(target));
            for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
            for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(1000.25+n));
            s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,target==0x10009f20?options.high:options.result);
            if(target==0x10009f20)s.wr<4>(16,options.low);
            s.pc=stop;calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Display fixture interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x30000);s->memory.map(stack-0x1000,0x2000);s->memory.map(toc-0x8000,0x9000);s->pc=display::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;
    s->memory.copy_in(record,std::vector<std::uint8_t>(100,0xa5));s->memory.copy_in(output,std::vector<std::uint8_t>(1024,0xa5));s->memory.write<4>(record+12,1000);s->memory.write<1>(record+0x38,0);s->memory.write<2>(record+0x30,364);s->memory.write<2>(record+0x32,2000);s->memory.write<1>(record+0x34,1);
    s->memory.write<4>(toc-0x7ac8,globals);s->memory.write<4>(globals,globals+0x100);s->memory.write<4>(globals+0x104,0x200000);s->wr<4>(12,record);s->wr<4>(16,output);s->wr<4>(20,1);s->wr<4>(24,0);contract_display_fixture_tables(*s,toc,tables);return s;
}
unsigned cases=0,returns=0,stops=0,negatives=0;std::array<unsigned,2> root_cases{},root_returns{},root_stops{},root_negatives{};std::set<std::uint32_t> complete_sites,all_sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;std::string outcome;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);TimedEffects x(*a),y(*b);Trace ot,nt;nt.effect_negative=options.effect_negative;Model old(true,options),modern(false,options);a->memory.date_observer=&x;b->memory.date_observer=&y;
    const auto run=[&](State& s,Model& d,bool reference){try{if(reference){original_trace=&ot;d.reference(s);}else methods[index(s.pc)](s,&d,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Display root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Display fixture interruption"||expected.starts_with("Unmapped data access at "),label+" unexpected reference: "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    const auto negative=options.own_negative||options.effect_negative;if(negative)require(expected.empty(),label+" negative needs a complete return");if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);if(options.effect_negative){require(nt.damaged,label+" effect negative must execute");Image(*a).compare(*b,label+" restored effect image");}
    bool rejected=false;try{require(old.calls==modern.calls,label+" dependency ABI/order");require(x.effects.bytes==y.effects.bytes&&x.pcs==y.pcs,label+" ordered timed effects");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" instruction count");Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!negative)throw;rejected=true;}require(rejected==negative,label+" discriminating negative");
    const auto root=index(input.pc);++cases;++root_cases[root];returns+=expected.empty()&&!negative;root_returns[root]+=expected.empty()&&!negative;stops+=!expected.empty();root_stops[root]+=!expected.empty();negatives+=negative;root_negatives[root]+=negative;if(!negative){all_sites.insert(ot.sites.begin(),ot.sites.end());if(expected.empty())complete_sites.insert(ot.sites.begin(),ot.sites.end());}
    return{std::move(a),x.effects.log,unsigned(old.calls.size()),expected};
}
void boundaries(const State& input,Options options={}){
    const auto base=compare(input,options);require(base.outcome.empty()&&base.state->pc==return_sentinel,"Display boundary baseline return");const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<5;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/4));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [at,size]:input.memory.mapped_ranges())for(auto [address,n]:hole<at||std::uint64_t(hole)>=std::uint64_t(at)+size?std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,size}}:std::vector<std::pair<std::uint32_t,std::uint64_t>>{{at,hole-at},{hole+1,std::uint64_t(at)+size-hole-1}})if(n){s->memory.map(address,n);s->memory.copy_in(address,input.memory.copy_out(address,n));}require(compare(*s,options).outcome.starts_with("Unmapped data access at "),"Removed display byte must fault");}
    for(unsigned cut:std::set<unsigned>{1,base.calls/2,base.calls})if(cut&&cut<=base.calls){auto o=options;o.interrupt=cut;compare(input,o);}
}
void corpus(){
    for(unsigned n=0;n<2;++n){auto s=seed(n);boundaries(*s);Options o;o.own_negative=true;compare(*s,o);o.own_negative=false;o.effect_negative=true;compare(*s,o);}
    for(unsigned bits=0;bits<8;++bits)for(unsigned mode=0;mode<256;++mode){auto s=seed(0);s->memory.write<4>(globals+0x104,bits<<21);s->wr<4>(20,mode);auto out=compare(*s);const auto factor=(bits&1)?1u:(bits&2)?4u:52u;require(out.state->memory.read<4>(globals+0x510)==1000*factor,"Independent period scaling and precedence");require(out.state->memory.read<4>(record+12)==1000,"Presentation retains the stored wage");require(out.state->memory.read<4>(globals+0x518)==0x12345678&&out.state->memory.read<4>(globals+0x51c)==0x89abcdef,"Numeric pair forwarding");if(mode==0)require(text(*out.state,output)=="NUM","Plain mode forwards numeric buffer");else if(!bits)require(out.state->memory.copy_out(output,1024)==s->memory.copy_out(output,1024),"No selected suffix leaves caller output unchanged");else{const auto offset=(bits&1)?(mode==2?0x2au:0x69u):(bits&2)?(mode==2?0x3fu:0x83u):(mode==2?0x54u:0x9eu);require(out.state->memory.read<4>(globals+0x524)==tables+offset,"Independent short or long period label and precedence");}}
    for(unsigned bits=0;bits<8;++bits)for(unsigned flags=0;flags<256;++flags){auto s=seed(0);s->memory.write<4>(globals+0x104,bits<<21);s->wr<4>(24,flags|0x12340000);auto out=compare(*s);const auto base=(bits&3)?8u:4u;const auto expected=std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(flags)))|int(base));require(out.state->memory.read<4>(globals+0x514)==expected&&out.state->memory.read<4>(globals+0x520)==expected,"Signed flag byte combined and forwarded to both numeric services");}
    for(auto amount:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto flags:{0x200000u,0x400000u,0x800000u}){auto s=seed(0);s->memory.write<4>(record+12,amount);s->memory.write<4>(globals+0x104,flags);auto out=compare(*s);const auto factor=flags==0x200000?1u:flags==0x400000?4u:52u;require(out.state->memory.read<4>(globals+0x510)==amount*factor,"32-bit period multiplication wraps before numeric service");}
    for(unsigned bits=0;bits<8;++bits)for(unsigned mode:{0u,1u,2u}){auto s=seed(0);s->memory.write<4>(globals+0x104,bits<<21);s->wr<4>(20,mode);boundaries(*s);}
    for(auto mode:{256u,257u,258u,0x12345680u,0xffffffffu}){auto s=seed(0);s->wr<4>(20,mode);compare(*s);}
    for(auto flags:{0u,0x200000u,0x400000u,0x800000u}){auto s=seed(0);Options o;o.rewrite_preferences=true;o.changed_preferences=flags;auto out=compare(*s,o);require(out.state->memory.read<4>(globals+0x510)==1000,"Period scaling precedes helper mutation");if(flags)require(out.state->memory.read<4>(globals+0x524)==tables+(flags==0x200000?0x69:flags==0x400000?0x83:0x9e),"Suffix re-reads preferences after numeric service");else require(out.state->memory.copy_out(output,1024)==s->memory.copy_out(output,1024),"Late missing suffix preserves output");}
    {auto s=seed(0);s->memory.write<4>(globals+0x104,0x1234);auto out=compare(*s);require(out.state->r<4>(12)==0x1234,"No suffix retains loaded preference return residue");}
    for(unsigned bits=0;bits<256;++bits){auto s=seed(1);s->memory.write<1>(record+0x38,bits);auto out=compare(*s);const auto kind=bits>>2;if(kind==1||kind==3||kind==4)require(out.state->memory.read<4>(globals+0x524)==tables+(kind==1?0xf3:kind==3?0xb9:0xca),"Independent packed type label");else require(out.state->memory.read<4>(globals+0x500)==0x10715e00,"Other packed types route to Date service");require(out.state->r<4>(12)==0x76543210,"Selected formatter result remains observable");}
    for(unsigned kind:{0u,1u,3u,4u}){auto s=seed(1);s->memory.write<1>(record+0x38,kind<<2);boundaries(*s);}
    for(auto day:{0u,0x7fffu,0x8000u,0xffffu})for(auto year:{0u,2000u,0xffffu})for(auto flag:{0u,1u,255u}){auto s=seed(1);s->memory.write<2>(record+0x30,day);s->memory.write<2>(record+0x32,year);s->memory.write<1>(record+0x34,flag);auto out=compare(*s);require(out.state->memory.read<2>(globals+0x540)==day&&out.state->memory.read<2>(globals+0x542)==year&&out.state->memory.read<1>(globals+0x544)==flag,"Date fields remain delegated without normalization");}
    for(unsigned n=0;n<2;++n)for(auto buffer:{record,record+0x30,record+0x38,tables+0xb9,tables+0xf3,stack-0x120+0x3c}){auto s=seed(n);s->wr<4>(16,buffer);compare(*s);}
    for(unsigned n=0;n<2;++n)for(auto arg:{3u,4u}){auto s=seed(n);s->wr<4>(arg*4,0);require(compare(*s).outcome.starts_with("Unmapped data access at "),"Mandatory record/output null fault retained");}
    {auto s=seed(0);s->memory.write<4>(globals,0);require(compare(*s).outcome.starts_with("Unmapped data access at "),"Mandatory preferences null fault retained");}
    for(unsigned n=0;n<2;++n){auto s=seed(n);Options o;o.number="";o.high=0xffffffff;o.low=0x80000000;o.result=0xabcde123;auto out=compare(*s,o);require(out.state->r<4>(12)==o.result,"Last dependency return is not normalized to output");}
}
}
namespace cm::aot {void contract_display_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{require(argc==1||argc==2,"Optional coverage output");corpus();const auto expected=contract_display_expected_sites();if(argc==2){std::ofstream out(argv[1]);for(auto pc:expected)out<<hex_address(pc)<<" "<<all_sites.contains(pc)<<" "<<complete_sites.contains(pc)<<'\n';}for(auto pc:expected)if(!complete_sites.contains(pc))std::cerr<<"Uncovered display site "<<hex_address(pc)<<std::endl;std::cout<<"Display comparisons="<<cases<<" returns="<<returns<<" stops="<<stops<<" negatives="<<negatives<<" completed_sites="<<complete_sites.size()<<"/"<<expected.size()<<std::endl;for(unsigned n=0;n<2;++n)std::cout<<"Root "<<hex_address(display::entries[n])<<" comparisons="<<root_cases[n]<<" returns="<<root_returns[n]<<" stops="<<root_stops[n]<<" negatives="<<root_negatives[n]<<std::endl;require(complete_sites.size()==expected.size(),"Every display instruction needs complete-return evidence");require(std::all_of(root_negatives.begin(),root_negatives.end(),[](auto n){return n==2;}),"Every display root needs result and restored-effect negatives");}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
