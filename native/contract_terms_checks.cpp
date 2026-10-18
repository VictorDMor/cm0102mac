#include "contract_terms.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot {bool contract_terms_reference(State&);void contract_terms_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace terms=cm::aot::contract_terms;
constexpr std::uint32_t record=fixture+0x2000,staff=fixture+0x4000,profile=fixture+0x5000,club=fixture+0x6000,competition=fixture+0x7000,service=fixture+0x8000,vtable=fixture+0x9000;
constexpr std::uint32_t globals=fixture+0x10000,pool=fixture+0x14000,dates=fixture+0x18000,output=fixture+0x30000,tables=fixture+0x40000,external=fixture+0x60000;
constexpr std::array slots{-0x7b80,-0x79b4,-0x79a8,-0x79ac,-0x79b0,-0x79a0,-0x79a4,-0x799c};
constexpr std::array<std::uint32_t,21> ends{0x10374ae8,0x10374b34,0x10374b54,0x10374b74,0x10374b98,0x10374bd4,0x10374ca4,0x10374cd8,0x10374d04,0x10374d44,0x10374d74,0x10374f44,0x103802f4,0x103803ac,0x10380400,0x10380520,0x10380554,0x10380630,0x10380700,0x10380708,0x10380c04};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{terms::sample_contract_range,terms::enable_option,terms::set_bonus,terms::clear_option,terms::clear_bonus,terms::reset_terms,terms::describe_bonus,terms::describe_option,terms::is_expired,terms::end_date_delta,terms::is_rolling,terms::describe_end,terms::bonus_name,terms::option_name,terms::supports_bonus,terms::supports_option,terms::has_release_terms,terms::adjust_value_primary,terms::adjust_value_alternate,terms::set_release_amount,terms::expiry_band};
struct Control {std::uint32_t pc;unsigned offset;std::uint64_t bits;unsigned width=4;};
struct Options {std::vector<Control> controls;unsigned interrupt=0,seed_return=12345,month=0,virtual_result=1,date_result=0xffff8001,adjusted_value=1234;bool own_negative=false,child_negative=false,mutate_record=false;};
struct Trace:SemanticGuestObserver {Options options;std::vector<std::uint32_t> sites;explicit Trace(Options o):options(std::move(o)){}void before_step(State& s,std::uint32_t pc)override{sites.push_back(pc);for(auto c:options.controls)if(c.pc==pc){if(c.width==1)s.wr<1>(c.offset,c.bits);else if(c.width==8)s.wr<8>(c.offset,c.bits);else s.wr<4>(c.offset,c.bits);}}};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(terms::entries.begin(),terms::entries.end(),root);require(i!=terms::entries.end(),"Known contract terms root "+hex_address(root));return unsigned(i-terms::entries.begin());}
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){methods[index(s.pc)](s,d,o);}
struct Event {std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto root=s.pc,n=index(root);bool first=true;while(s.pc!=stop){if(s.pc>=root&&s.pc<ends[n]&&(first||s.pc!=root))require(contract_terms_reference(s),"Complete original terms reference");else if(s.pc==external)resume(s,s.pc,stop);else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}}
    void resume(State& s,std::uint32_t target,std::uint32_t stop)override{require(target==external,"Known changed terms continuation");calls.push_back({target,stop,s.registers,{}});s.memory.write<4>(globals+0x500,s.r<4>(4128));calls.back().after=s.registers;throw std::runtime_error("Fixture contract terms dependency interruption");}
    static std::vector<std::uint8_t> text(State& s,std::uint32_t pointer){std::vector<std::uint8_t> b;for(unsigned n=0;n<4096;++n){auto c=std::uint8_t(s.memory.read<1>(pointer+n));if(!c)return b;b.push_back(c);}throw std::runtime_error("Unterminated contract terms fixture text");}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override{
        const bool own=std::find(terms::entries.begin(),terms::entries.end(),target)!=terms::entries.end();
        constexpr std::array allowed{0x1077b770u,0x10727890u,0x1077dbf0u,0x10009f20u,0x1000b410u,0x10718130u,0x10718360u,0x10715e00u,0x10781320u,0x1000a060u,0x1000a170u};
        require(own||std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known terms dependency "+hex_address(target));const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try{
            if(own){if(original)reference(s,stop);else modern_root(s,this,&trace);}
            else {
                const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};auto first=arg(3),second=arg(4);std::uint32_t result=0,second_result=0;
                if(target==0x1077b770)result=options.seed_return;
                if(target==0x10727890||target==0x1077dbf0){auto b=text(s,second);s.memory.copy_in(first,b);s.memory.write<1>(first+std::uint32_t(b.size()),0);if(target==0x10727890)s.memory.write<4>(globals+0x400,arg(5));}
                if(target==0x10009f20){result=std::uint32_t(s.memory.read<4>(first))^0x12340000;second_result=second^0x98765432;}
                if(target==0x1000b410){s.memory.write<4>(first,arg(5));s.memory.write<4>(first+4,arg(6));s.memory.write<1>(first+8,0);}
                if(target==0x10718130){(void)s.memory.read<4>(first);(void)s.memory.read<4>(first+4);result=options.date_result;}
                if(target==0x10718360){(void)s.memory.read<4>(first);result=options.month;if(options.mutate_record)s.memory.write<2>(record+0x32,2040);}
                if(target==0x10715e00){s.memory.write<4>(second,s.memory.read<4>(first));s.memory.write<1>(second+4,0);}
                if(target==0x10781320){s.memory.write<4>(s.r<4>(4)+0x14,s.r<4>(8));s.wr<4>(8,0x11223344);result=options.virtual_result;}
                if(target==0x1000a060||target==0x1000a170){s.memory.write<4>(first,options.adjusted_value);if(options.mutate_record)s.memory.write<4>(record+12,0x33445566);}
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-70.0-n));
                s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);if(target==0x10009f20)s.wr<4>(16,second_result);s.pc=stop;
            }
            if(options.child_negative&&!original)s.memory.write<1>(globals+0x700,1);calls[serial].after=s.registers;
            if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture contract terms dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned b=1280;b<1312;++b)s->wr<1>(b,0);s->memory.map(fixture,0x80000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);s->pc=terms::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;s->wr<4>(12,record);s->wr<4>(16,1);s->wr<4>(20,1234);s->wr<4>(28,0);
    s->memory.copy_in(record,std::vector<std::uint8_t>(100,0x5a));s->memory.write<1>(record+0x38,8);s->memory.write<2>(record+0x30,150);s->memory.write<2>(record+0x32,2001);s->memory.write<4>(record+12,1000);s->memory.write<4>(staff+0x14,profile);s->memory.write<1>(profile+15,20);s->memory.write<2>(staff+0x5c,33);
    s->memory.write<4>(club+0x30,competition);s->memory.write<4>(competition,0);s->memory.write<4>(globals,globals+0x200);s->memory.write<4>(globals+0x200,service);s->memory.write<4>(service,vtable);s->memory.write<4>(vtable+0xa0,external);s->memory.write<4>(vtable+0xa4,external+8);s->memory.write<4>(toc-0x7b5c,globals);
    for(unsigned k=0;k<slots.size();++k){s->memory.write<4>(toc+slots[k],dates+k*16);s->memory.write<2>(dates+k*16,std::array{1u,180u,120u,140u,160u,80u,100u,60u}[k]);s->memory.write<2>(dates+k*16+2,2001);s->memory.write<4>(dates+k*16+4,0x10203040+k);}
    for(auto [slot,ptr]:{std::pair{-0x7b90,globals+0x10},std::pair{-0x7b8c,pool},std::pair{-0x7b88,globals+0x14},std::pair{-0x7b84,globals+0x18},std::pair{-0x7ac8,globals+0x20}})s->memory.write<4>(toc+slot,ptr);
    s->memory.write<4>(globals+0x10,0);s->memory.write<4>(globals+0x14,pool+256);s->memory.write<4>(globals+0x18,pool);s->memory.write<4>(globals+0x20,globals+0x100);s->memory.write<4>(globals+0x104,0);
    for(unsigned k=0;k<128;++k)s->memory.write<2>(pool+k*2,(12345+k*1009)&65535);contract_terms_fixture_tables(*s,toc,tables);
    if(n==0)s->wr<4>(12,10);
    if(n==6||n==7){s->wr<4>(16,0);s->wr<4>(20,output);}
    if(n==11)s->wr<4>(16,output);
    if(n==12||n==13){s->wr<4>(12,0);s->wr<4>(16,output);}
    if(n==14){s->wr<4>(12,staff);s->wr<4>(16,2);}
    if(n==15){s->wr<4>(12,staff);s->wr<4>(16,club);s->wr<4>(20,0);}
    return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;Trace ot(options),nt(options);Model old(true,options,ot),modern(false,options,nt);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&ot;m.reference(s);}else modern_root(s,&m,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Terms root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture contract terms dependency interruption"||expected.starts_with("Unmapped data access at "),label+" fixture assertion: "+expected);
    if(ot.sites!=nt.sites){for(unsigned k=0;k<std::min(ot.sites.size(),nt.sites.size());++k)if(ot.sites[k]!=nt.sites[k]){std::cerr<<"First terms site "<<k<<" "<<hex_address(ot.sites[k])<<"/"<<hex_address(nt.sites[k])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" effects");require(old.calls==modern.calls,label+" dependency ABI");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" accounting");}
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;structural+=!options.controls.empty();if(expected.empty()&&!rejected)sites.insert(ot.sites.begin(),ot.sites.end());return{std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}){
    auto base=compare(input,options);require(base.state->pc==return_sentinel,"Completed terms boundary fixture root="+hex_address(input.pc)+" stop="+hex_address(base.state->pc));const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(auto a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<6;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [a,n]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(n){s->memory.map(a,n);s->memory.copy_in(a,input.memory.copy_out(a,n));}}compare(*s,options);}
    for(unsigned n=1;n<=base.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}
void basic_cases(){
    for(unsigned n=0;n<terms::entries.size();++n){boundaries(*seed(n));Options o;o.own_negative=true;compare(*seed(n),o);}
    for(auto n:{1u,2u,3u,4u,6u,7u,12u,13u,14u})for(unsigned value=0;value<256;++value){auto s=seed(n);s->wr<4>(n==12||n==13?12:16,value);compare(*s);}
    for(auto n:{8u,10u,11u,20u})for(unsigned value=0;value<256;++value){auto s=seed(n);s->memory.write<1>(record+0x38,value);compare(*s);}
    for(unsigned n:{17u,18u})for(auto prefs:{0u,0x400000u,0x800000u,0xc00000u,0xffffffffu})for(auto value:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=seed(n);s->memory.write<4>(globals+0x104,prefs);s->memory.write<4>(record+12,value);Options o;o.adjusted_value=value;o.mutate_record=true;compare(*s,o);}
    for(unsigned month=0;month<14;++month)for(auto mode:{0u,1u,255u}){auto s=seed(11);s->wr<4>(20,mode);Options o;o.month=month;compare(*s,o);}
    for(auto type:{0u,1u,2u,3u,4u,5u,127u,128u,255u,256u})for(auto answer:{0u,1u,255u,256u}){auto s=seed(15);s->wr<4>(20,type);Options o;o.virtual_result=answer;compare(*s,o);}
    for(auto bound:{0u,1u,7u,65535u,65536u,0x7fffffffu,0x80000000u,0xffff0000u,0xffff0001u,0xffffffffu}){auto s=seed(0);s->wr<4>(12,bound);boundaries(*s);}
}

Control gpr(std::uint32_t pc,unsigned reg,std::uint32_t value){return {pc,reg*4,value,4};}
void sampler_cases(){
    constexpr std::array points{0x10374688u,0x10374774u,0x10374860u};
    for(unsigned depth=1;depth<=3;++depth)for(auto value:{65536u,0xffff0000u}){auto s=seed(0);s->wr<4>(12,65536);Options o;for(unsigned n=0;n<depth;++n)o.controls.push_back(gpr(points[n],22-n,value));compare(*s,o);}
    for(unsigned depth=1;depth<=2;++depth){auto s=seed(0);s->wr<4>(12,65536);Options o;o.controls.push_back(gpr(points[0],22,depth==1?0x7fffffffu:65536u));if(depth==2)o.controls.push_back(gpr(points[1],21,0x7fffffffu));compare(*s,o);}
    for(auto bound:{10u,65536u,0xffff0000u})for(auto seed_value:{0u,65535u,65536u,0xffffffffu}){auto s=seed(0);s->wr<4>(12,bound);s->memory.write<4>(globals+0x14,pool);Options o;o.seed_return=seed_value;boundaries(*s,o);}
    {auto s=seed(0);s->wr<4>(12,65536);s->memory.write<4>(globals+0x14,pool);Options o;for(unsigned n=0;n<3;++n)o.controls.push_back(gpr(points[n],22-n,65536));boundaries(*s,o);}
}
void state_and_eligibility_cases(){
    for(auto mode:{1u,255u,256u,257u,0xffffffffu}){auto s=seed(6);s->wr<4>(28,mode);boundaries(*s);}
    for(auto value:{0u,19u,20u,255u})for(auto type:{0u,1u,2u,3u,255u,256u}){auto s=seed(14);s->memory.write<1>(profile+15,value);s->wr<4>(16,type);compare(*s);}
    {auto s=seed(14);s->memory.write<4>(staff+0x14,0);compare(*s);}
    for(auto flag0:{0u,1u,2u,255u})for(auto flag1:{0u,1u,2u,255u})for(auto flag3:{0u,1u,2u,255u}){auto s=seed(16);s->memory.write<1>(record+0x1c,flag0);s->memory.write<1>(record+0x1d,flag1);s->memory.write<1>(record+0x1f,flag3);const auto r=compare(*s);require(r.state->r<4>(12)==unsigned(flag0==2||flag1==1||flag3==2),"Independent exact release-flag query");}
    for(auto n:{1u,2u,3u,4u,6u,7u,12u,13u,14u})for(auto value:{256u,257u,0x80000000u,0xffffffffu}){auto s=seed(n);s->wr<4>(n==12||n==13?12:16,value);compare(*s);}
    for(auto pointer:{0u,competition})for(auto object:{0u,service})for(auto type:{0u,1u,2u,3u,4u,255u}){auto s=seed(15);s->memory.write<4>(club+0x30,pointer);s->memory.write<4>(globals+0x200,object);s->wr<4>(20,type);compare(*s);}
    for(auto age:{0u,30u,31u,32u,127u,128u,255u,256u,65535u}){auto s=seed(15);s->memory.write<2>(staff+0x5c,age);s->wr<4>(20,4);compare(*s);}
}
void expiry_cases(){
    for(auto year:{0u,2000u,2001u,2002u,65535u})for(auto day:{0u,1u,2u,59u,60u,61u,79u,80u,81u,99u,100u,101u,119u,120u,121u,139u,140u,141u,149u,150u,159u,160u,161u,179u,180u,181u,32767u,32768u,65535u}){auto s=seed(20);s->memory.write<2>(record+0x30,day);s->memory.write<2>(record+0x32,year);compare(*s);}
    for(unsigned n=1;n<7;++n)for(auto year:{0u,2001u,2002u})for(auto day:{0u,500u}){auto s=seed(20);s->memory.write<2>(record+0x30,n<5?150:110);s->memory.write<2>(dates+n*16,day);s->memory.write<2>(dates+n*16+2,year);compare(*s);}
    for(auto day:{60u,80u,100u,120u,140u,160u,180u}){auto s=seed(20);s->memory.write<2>(record+0x30,day);boundaries(*s);}
}
void more_boundaries_and_negatives(){
    for(unsigned n=0;n<terms::entries.size();++n){auto s=seed(n);if(n==0)s->memory.write<4>(globals+0x14,pool);const auto base=compare(*s);if(!base.calls.empty()){Options o;o.child_negative=true;compare(*s,o);}}
    for(unsigned n=0;n<terms::entries.size();++n){auto s=seed(n);if(n==0)s->memory.write<4>(globals+0x18,0xf0000000);else s->wr<4>((n==7)?20:(n==12||n==13||n==15)?16:12,0xf0000000);compare(*s);}
    for(auto n:{0u,6u,7u,11u,15u,17u,20u}){auto s=seed(n);constexpr auto moved=toc+0x10000;s->memory.map(moved-0x8000,0x9000);s->memory.copy_in(moved-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,moved);boundaries(*s);}
}


void independent_expectations(){
    for(auto selected:{0u,1u,2u,3u,4u}){auto s=seed(1);s->wr<4>(16,selected);const auto r=compare(*s);for(unsigned n=0;n<5;++n)require(r.state->memory.read<1>(record+0x1c+n)==(n==selected?1u:selected==1||n==1?0u:0x5au),"Independent minimum-fee option exclusion");}
    auto reset=compare(*seed(5));for(unsigned n=0;n<5;++n)require(reset.state->memory.read<1>(record+0x1c+n)==0,"Independent reset options");for(unsigned n=0;n<3;++n)require(reset.state->memory.read<4>(record+0x10+n*4)==0xffffffff,"Independent reset bonuses");require(reset.state->memory.read<1>(record+0x54)==0&&reset.state->memory.read<4>(record+0x58)==0xffffffff,"Independent reset markers");
    const auto set=compare(*seed(2));require(set.state->memory.read<4>(record+0x14)==1234,"Independent selected bonus write");const auto cleared=compare(*seed(4));require(cleared.state->memory.read<4>(record+0x14)==0xffffffff,"Independent selected bonus clear");const auto release=compare(*seed(19));require(release.state->memory.read<4>(record+0x24)==1,"Independent release amount");
    for(auto n:{8u,10u})for(unsigned value=0;value<256;++value){auto s=seed(n);s->memory.write<1>(record+0x38,value);const auto r=compare(*s);const auto kind=std::int8_t(std::uint8_t(value))>>2;require(r.state->r<4>(12)==unsigned(kind==(n==8?3:1)),"Independent signed contract kind");}
    for(auto bound:{0u,1u,7u,100u,65535u}){auto s=seed(0);s->wr<4>(12,bound);const auto r=compare(*s);require(r.state->r<4>(12)==(bound?(12345u+1009u)%bound:0),"Independent signed narrow pool sample");}
    for(auto day:{59u,60u,80u,100u,120u,140u,160u,180u,181u}){auto s=seed(20);s->memory.write<2>(record+0x30,day);const auto r=compare(*s);const unsigned expected=day==59?12:day==60?13:day==80?11:day==100?9:day==120?7:day==140?5:day==160?3:day==180?1:0;require(r.state->r<4>(12)==expected,"Independent expiry reference-date band");}
}
void alias_and_continuation_cases(){
    for(auto mode:{0u,1u}){auto s=seed(6);s->wr<4>(20,record);s->wr<4>(28,mode);boundaries(*s);}
    for(auto n:{11u,12u,13u}){auto s=seed(n);s->wr<4>(16,record);boundaries(*s);}
    {auto s=seed(11);s->wr<4>(20,1);Options o;o.mutate_record=true;boundaries(*s,o);}
    for(auto target:{0x10374df8u,0x10374dfcu,0x10374e00u,0x10374e04u,0x10374e08u,0x10374f28u,external}){auto s=seed(11);s->wr<4>(20,1);s->memory.write<4>(tables+0x2000,target);compare(*s);}
    {auto s=seed(11);s->wr<4>(20,1);s->memory.copy_in(globals+0x800,s->memory.copy_out(tables+0x2000,48));s->memory.write<4>(toc-0x4db0,globals+0x800);boundaries(*s);}
}

}
namespace cm::aot {void contract_terms_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{basic_cases();sampler_cases();state_and_eligibility_cases();expiry_cases();more_boundaries_and_negatives();independent_expectations();alias_and_continuation_cases();for(unsigned n=0;n<terms::entries.size();++n)for(auto pc=terms::entries[n];pc<ends[n];pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered terms "<<hex_address(pc)<<std::endl;require(sites.size()==1203,"Complete 21-operation contract terms source coverage");std::cout<<"Terms comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<"/1203"<<std::endl;}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
