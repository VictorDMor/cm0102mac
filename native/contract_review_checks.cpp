#include "contract_review.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot {bool contract_review_reference(State&);void contract_review_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace review=cm::aot::contract_review;
constexpr std::uint32_t record=fixture+0x2000,staff=fixture+0x4000,club=fixture+0x6000,profile=fixture+0x8000,primary=fixture+0x9000,secondary=fixture+0xa000,role_profile=fixture+0xb000,context=fixture+0xc000;
constexpr std::uint32_t globals=fixture+0x10000,pool=fixture+0x14000,dates=fixture+0x18000,output=fixture+0x30000,tables=fixture+0x40000,external=fixture+0x60000;
constexpr std::array<std::uint32_t,13> ends{0x1037f110,0x1037f588,0x1037f7c4,0x1037f830,0x1037f898,0x1037fa48,0x1037fa60,0x1037fa6c,0x1037fa78,0x10380264,0x10380d4c,0x10380f1c,0x103810d0};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{review::reset_review_state,review::has_review_trigger,review::refresh_review_conditions,review::clear_review_condition,review::clear_review_conditions,review::add_status_bits,review::clear_status_bits,review::reset_status_bits,review::status_bits,review::apply_review_conditions,review::describe_squad_status,review::describe_release_summary,review::query_start_date_window};
struct Control{std::uint32_t pc;unsigned offset;std::uint64_t bits;unsigned width=4;};
struct Options{std::vector<Control> controls;unsigned interrupt=0,seed_return=12345,range_return=1,preference=5,gate0=0,gate1=0,age=28,elapsed_years=3,elapsed_days=10,veto_management=0,veto_first=0,veto_second=0,resolved_flags=0x101;bool own_negative=false,child_negative=false,missing_contract=false,mutate_record=false;};
struct Trace:SemanticGuestObserver{Options options;std::vector<std::uint32_t> sites;explicit Trace(Options o):options(std::move(o)){}void before_step(State& s,std::uint32_t pc)override{sites.push_back(pc);for(auto c:options.controls)if(c.pc==pc){if(c.width==1)s.wr<1>(c.offset,c.bits);else if(c.width==8)s.wr<8>(c.offset,c.bits);else s.wr<4>(c.offset,c.bits);}}};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(review::entries.begin(),review::entries.end(),root);require(i!=review::entries.end(),"Known review root "+hex_address(root));return unsigned(i-review::entries.begin());}
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){methods[index(s.pc)](s,d,o);}
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model:SemanticDependencyDispatch{
    bool original;Options options;Trace& trace;std::vector<Event> calls;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto root=s.pc,n=index(root);bool first=true;while(s.pc!=stop){if(s.pc>=root&&s.pc<ends[n]&&(first||s.pc!=root))require(contract_review_reference(s),"Complete original review reference");else if(s.pc==external)resume(s,s.pc,stop);else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}}
    void resume(State& s,std::uint32_t target,std::uint32_t stop)override{
        if(target==external){calls.push_back({target,stop,s.registers,{}});s.memory.write<4>(globals+0x610,s.r<4>(4128));calls.back().after=s.registers;throw std::runtime_error("Fixture contract review dependency interruption");}
        const auto previous=original_trace;original_trace=&trace;
        try{while(s.pc!=stop){bool internal=false;for(unsigned n=0;n<review::entries.size();++n)if(s.pc>=review::entries[n]&&s.pc<ends[n]){internal=true;break;}if(internal)require(contract_review_reference(s),"Original changed review continuation");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}original_trace=previous;}catch(...){original_trace=previous;throw;}
    }
    static std::vector<std::uint8_t> text(State& s,std::uint32_t pointer){std::vector<std::uint8_t> b;for(unsigned n=0;n<4096;++n){auto c=std::uint8_t(s.memory.read<1>(pointer+n));if(!c)return b;b.push_back(c);}throw std::runtime_error("Unterminated review fixture text");}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override{
        const bool own=std::find(review::entries.begin(),review::entries.end(),target)!=review::entries.end();
        constexpr std::array allowed{0x10082f20u,0x103092f0u,0x103ea290u,0x1008f620u,0x100de090u,0x100ddf30u,0x103f43b0u,0x103e6470u,0x1008e350u,0x1014c2e0u,0x1077b770u,0x10374570u,0x100d9b80u,0x100ddfc0u,0x10739890u,0x10768810u,0x1077b720u,0x10769b60u,0x10727890u,0x10715880u,0x10717dd0u,0x100c6a10u};
        require(own||std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known review dependency "+hex_address(target));const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try{
            if(own){if(original)reference(s,stop);else modern_root(s,this,&trace);}
            else{
                const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};const auto first=arg(3),second=arg(4);std::uint32_t result=0;
                s.memory.write<4>(globals+0x600,target);s.memory.write<4>(globals+0x604,s.memory.read<4>(globals+0x604)+1);
                if(target==0x10082f20){(void)s.memory.read<4>(second);result=options.missing_contract?0:record;}
                if(target==0x103092f0)result=options.preference;
                if(target==0x103ea290)result=options.veto_management;
                if(target==0x103f43b0)result=options.veto_first;
                if(target==0x103e6470)result=options.veto_second;
                if(target==0x1008e350){(void)s.memory.read<4>(second);if(options.mutate_record)s.memory.write<1>(second+0x54,1);}
                if(target==0x1008f620){(void)s.memory.read<4>(second);if(options.mutate_record)s.memory.write<4>(second+0x50,options.resolved_flags);}
                if(target==0x100de090)s.memory.write<4>(first+0x30,s.memory.read<4>(first+0x30)&~second);
                if(target==0x100ddf30)s.memory.write<4>(first+0x30,0);
                if(target==0x100ddfc0||target==0x100d9b80){(void)s.memory.read<4>(first);(void)s.memory.read<4>(second);s.memory.write<4>(globals+0x608,first^second^arg(5));}
                if(target==0x1014c2e0){(void)s.memory.read<4>(first);result=second?options.gate1:options.gate0;}
                if(target==0x1077b770)result=options.seed_return;
                if(target==0x10374570){const auto slot=std::uint32_t(s.memory.read<4>(s.r<4>(8)-0x7b84)),cursor=std::uint32_t(s.memory.read<4>(slot));(void)s.memory.read<2>(cursor);s.memory.write<4>(slot,cursor+2);result=options.range_return;}
                if(target==0x10739890){(void)text(s,first);s.memory.write<4>(arg(6),123);s.memory.write<4>(arg(7),45);}
                if(target==0x10768810)result=output+0x800;
                if(target==0x1077b720||target==0x10727890){const auto bytes=text(s,second);s.memory.copy_in(first,bytes);s.memory.write<1>(first+std::uint32_t(bytes.size()),0);if(options.mutate_record)s.memory.write<1>(record+0x1d,7);}
                if(target==0x10769b60){(void)text(s,first);(void)text(s,second);}
                if(target==0x10715880)s.memory.clear(first,8);
                if(target==0x10717dd0){(void)s.memory.read<4>(second);(void)s.memory.read<4>(second+4);s.memory.write<2>(first,options.elapsed_days);s.memory.write<2>(first+2,options.elapsed_years);s.memory.write<4>(first+4,0x22334455);}
                if(target==0x100c6a10){(void)s.memory.read<4>(first);(void)s.memory.read<4>(second);result=options.age;}
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-70.0-n));s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            }
            if(options.child_negative&&!original)s.memory.write<1>(globals+0x700,1);calls[serial].after=s.registers;
            if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture contract review dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned b=1280;b<1312;++b)s->wr<1>(b,0);s->memory.map(fixture,0x80000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);s->pc=review::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;s->wr<4>(12,record);s->wr<4>(16,1);s->wr<4>(20,0);s->wr<4>(24,0);
    s->memory.write<4>(record,0);s->memory.write<4>(record+4,0);s->memory.write<1>(record+0x38,8);s->memory.write<1>(record+0x4c,10);s->memory.write<1>(record+0x44,10);s->memory.write<1>(record+0x5c,4);s->memory.write<1>(record+0x60,2);s->memory.write<4>(record+0x48,8);s->memory.write<4>(record+0x50,1);s->memory.write<1>(record+0x1d,2);s->memory.write<2>(record+0x28,60);s->memory.write<2>(record+0x2a,2001);s->memory.write<4>(record+0x2c,0x12345678);
    s->memory.write<4>(staff,0);s->memory.write<4>(staff+0x14,profile);s->memory.write<1>(staff+0x62,11);s->memory.write<1>(staff+0x63,10);for(unsigned off=0x65;off<=0x69;++off)s->memory.write<1>(staff+off,10);s->memory.write<2>(staff+0x5c,33);s->memory.write<2>(profile+10,4000);
    s->memory.write<4>(club,0);s->memory.write<4>(club+0x90,primary);s->memory.write<4>(club+0x94,secondary);s->memory.write<4>(club+0x80,secondary);s->memory.write<4>(primary,1);s->memory.write<4>(secondary,2);s->memory.write<4>(primary+0x1c,role_profile);s->memory.write<4>(secondary+0x1c,role_profile+0x100);s->memory.write<1>(primary+0x65,10);s->memory.write<1>(primary+0x69,10);s->memory.write<1>(role_profile+0x34,10);s->memory.write<1>(role_profile+0x136,20);s->memory.write<4>(context+0x2c,context+0x200);s->memory.write<1>(context+0x319,4);
    for(auto [slot,ptr]:{std::pair{-0x7be0,globals},std::pair{-0x7be8,globals+4},std::pair{-0x7b7c,globals+8},std::pair{-0x7b78,globals+12},std::pair{-0x7b40,globals+16},std::pair{-0x7b3c,globals+20},std::pair{-0x7ba8,globals+0x400},std::pair{-0x7bac,globals+0x800},std::pair{-0x7b90,globals+0x20},std::pair{-0x7b8c,pool},std::pair{-0x7b88,globals+0x24},std::pair{-0x7b84,globals+0x28},std::pair{-0x7b80,dates},std::pair{-0x79e8,dates+16},std::pair{-0x7a68,globals+0x900}})s->memory.write<4>(toc+slot,ptr);
    s->memory.write<4>(globals,staff);s->memory.write<4>(globals+4,club);s->memory.write<4>(globals+8,globals+0x2000);s->memory.write<4>(globals+16,globals+0x2100);s->memory.write<4>(globals+20,globals+0x2200);s->memory.write<4>(globals+0x24,pool+256);s->memory.write<4>(globals+0x28,pool);s->memory.write<1>(globals+0x800,1);s->memory.write<1>(globals+0x900,1);s->memory.write<2>(dates,100);s->memory.write<2>(dates+2,2005);s->memory.write<2>(dates+16,1);s->memory.write<2>(dates+18,2001);
    for(unsigned k=0;k<128;++k)s->memory.write<2>(pool+k*2,(12345+k*1009)&65535);contract_review_fixture_tables(*s,toc,tables);s->memory.copy_in(output+0x800,{102,105,120,116,117,114,101,0});
    if(n==0){s->wr<4>(16,staff);s->wr<4>(20,context);}if(n==10||n==11){s->wr<4>(16,output);s->wr<4>(20,200);}if(n==12){s->wr<4>(20,output+0x100);s->wr<4>(24,output+0x200);}return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;Trace ot(options),nt(options);Model old(true,options,ot),modern(false,options,nt);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&ot;m.reference(s);}else modern_root(s,&m,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Review root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture contract review dependency interruption"||expected.starts_with("Unmapped data access at "),label+" fixture assertion: "+expected);
    if(ot.sites!=nt.sites){for(unsigned k=0;k<std::min(ot.sites.size(),nt.sites.size());++k)if(ot.sites[k]!=nt.sites[k]){std::cerr<<"First review site "<<k<<" "<<hex_address(ot.sites[k])<<"/"<<hex_address(nt.sites[k])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" effects");require(old.calls==modern.calls,label+" dependency ABI");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" accounting");}
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;structural+=!options.controls.empty();if(expected.empty()&&!rejected)sites.insert(ot.sites.begin(),ot.sites.end());return{std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}){
    auto base=compare(input,options);require(base.state->pc==return_sentinel,"Completed review boundary fixture root="+hex_address(input.pc)+" stop="+hex_address(base.state->pc));const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(auto a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<6;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [a,n]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(n){s->memory.map(a,n);s->memory.copy_in(a,input.memory.copy_out(a,n));}}compare(*s,options);}
    for(unsigned n=1;n<=base.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}

Control gpr(std::uint32_t pc,unsigned reg,std::uint32_t value){return {pc,reg*4,value,4};}
constexpr std::array flag_sets{0u,1u,2u,4u,8u,0x10u,0x14u,0x20u,0x40u,0x80u,0x100u,0x200u,0x400u,0x600u,0x800u,0x1000u,0x2000u,0x2808u,0x2840u,0x10000u,0x16288u,0x20000u,0x40000u,0x80000u,0x200001u,0x200600u,0x1000000u,0x1000002u,0x4582208u,0x80000000u,0xffffffffu};
void scoring_cases(){
    for(auto flags:flag_sets)for(auto off:{0x63u,0x65u,0x66u,0x67u,0x69u})for(auto value:{0u,1u,2u,5u,6u,9u,10u,14u,15u,16u,20u,31u,75u,127u,128u,255u}){auto s=seed(1);s->memory.write<4>(record+0x50,flags);s->memory.write<1>(staff+off,value);compare(*s);}
    for(auto flags:flag_sets)for(auto value:{0u,1u,2u,50u,51u,75u,76u,127u,128u,200u,255u}){auto s=seed(1);s->memory.write<4>(record+0x50,flags);s->memory.write<1>(record+0x4c,value);compare(*s);}
    for(auto flags:flag_sets)for(auto value:{0u,5750u,5751u,7250u,7251u,32767u,32768u,65535u}){auto s=seed(1);s->memory.write<4>(record+0x50,flags);s->memory.write<2>(profile+10,value);compare(*s);}
    for(auto flags:flag_sets)for(auto first:{0u,staff,primary})for(auto second:{0u,staff,secondary}){auto s=seed(1);s->memory.write<4>(record+0x50,flags);s->memory.write<4>(club+0x90,first);s->memory.write<4>(club+0x94,second);compare(*s);}
    for(unsigned value=0;value<256;++value){auto s=seed(1);s->memory.write<1>(record+0x5c,value);s->memory.write<4>(record+0x50,0x200);s->memory.write<1>(staff+0x65,14);compare(*s);}
    for(auto id:{0u,1u,6u,7u,8u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=seed(1);s->memory.write<4>(staff,id);compare(*s);}
    {auto s=seed(1);s->memory.write<4>(staff+0x14,0);boundaries(*s);}
    for(auto score:{0u,17u,18u,19u,20u,24u,25u,29u,30u,0xffffff80u,0xffffffffu})for(auto flags:flag_sets){auto s=seed(1);s->memory.write<4>(record+0x50,flags);Options o;o.controls={gpr(0x1037f3c0,30,score)};compare(*s,o);}
}
void correlated_cases(){
    for(auto flags:flag_sets)for(auto status:{0u,1u,4u,8u,15u,0xffffffffu}){auto s=seed(1);s->memory.write<4>(record+0x50,flags);s->memory.write<4>(record+0x48,status);compare(*s);}
    {auto s=seed(1);s->memory.write<4>(record+0x50,4);s->memory.write<1>(staff+0x63,16);s->memory.write<1>(staff+0x67,9);boundaries(*s);}
    {auto s=seed(1);s->memory.write<4>(record+0x50,8);s->memory.write<1>(staff+0x65,9);s->memory.write<1>(staff+0x66,5);boundaries(*s);}
    {auto s=seed(9);s->wr<4>(16,0x16288);s->memory.write<1>(primary+0x69,10);s->memory.write<1>(role_profile+0x34,0);s->memory.write<2>(pool+2,1);boundaries(*s);}
}
void reset_and_refresh_cases(){
    for(unsigned value=0;value<256;++value){auto s=seed(0);Options o;o.preference=value;const auto result=compare(*s,o);const auto signed_value=std::int8_t(std::uint8_t(value));const auto expected=(2u|(unsigned(signed_value>10?10:signed_value)&15u)<<4);require(result.state->memory.read<1>(record+0x60)==expected,"Independent packed preference clamp");}
    for(auto pointer:{0u,context}){auto s=seed(0);s->wr<4>(20,pointer);boundaries(*s);}
    {auto s=seed(0);s->memory.write<4>(context+0x2c,0);boundaries(*s);}
    for(auto flags:flag_sets)for(auto n:{3u,4u,5u,6u,7u,8u}){auto s=seed(n);s->wr<4>(16,flags);compare(*s);}
    for(auto mode:{0u,1u,255u,256u,257u,0xffffffffu}){auto s=seed(4);s->wr<4>(16,mode);boundaries(*s);}
    for(auto kind:{0u,4u,8u,12u,20u,255u})for(auto marker:{0u,1u,8u,9u,255u}){auto s=seed(2);s->memory.write<1>(record+0x38,kind);s->memory.write<1>(record+0x5c,marker);s->memory.write<4>(record+0x50,0);compare(*s);}
    for(auto failure:{0u,1u,2u,3u,4u,5u}){auto s=seed(2);s->memory.write<1>(record+0x5c,8);s->memory.write<4>(record+0x50,0);Options o;
        if(failure==0)o.missing_contract=true;if(failure==1)s->memory.write<4>(staff+0x14,0);if(failure==2)s->memory.write<4>(record+4,1);if(failure==3)s->memory.write<1>(record+0x54,1);if(failure==4)s->memory.write<1>(record+0x60,7);if(failure==5)o.veto_management=1;compare(*s,o);}
    {auto s=seed(2);s->memory.write<1>(record+0x5c,8);s->memory.write<4>(record+0x50,0);s->memory.write<1>(record+0x45,1);Options o;o.mutate_record=true;o.resolved_flags=0x1019d;boundaries(*s,o);const auto r=compare(*s,o);require(r.state->memory.read<4>(record+0x50)==0&&r.state->memory.read<1>(record+0x5c)==0&&r.state->memory.read<1>(record+0x44)==15,"Independent composed review clearing");}
}
void consequence_cases(){
    for(unsigned value=0;value<256;++value)for(auto n:{5u,9u}){auto s=seed(n);s->memory.write<1>(staff+0x62,value);compare(*s);}
    for(auto n:{5u,9u})for(auto flags:flag_sets)for(auto marker:{0u,1u,8u,9u,255u}){auto s=seed(n);s->wr<4>(16,flags);s->memory.write<1>(record+0x5c,marker);Options o;o.mutate_record=true;compare(*s,o);}
    for(auto n:{5u,9u})for(auto gate:{0u,1u,2u,256u})for(auto field:{0u,1u,2u,3u,4u,5u,6u}){auto s=seed(n);s->wr<4>(16,8);Options o;if(field==0)o.veto_first=gate;if(field==1)o.veto_second=gate;if(field==2)s->memory.write<1>(record+0x54,gate);if(field==3)s->memory.write<1>(record+0x4c,gate);if(field==4)s->memory.write<4>(club+0x90,gate?staff:0);if(field==5)s->memory.write<4>(club+0x94,gate?staff:0);if(field==6)s->memory.write<2>(staff+0x5c,gate?20:21);compare(*s,o);}
    for(auto first:{0u,1u,255u,256u})for(auto second:{0u,1u,255u,256u})for(auto flags:{0u,1u,8u,0x16288u,0x20001u,0xe0000000u,0xffffffffu}){auto s=seed(9);s->wr<4>(16,flags);s->wr<4>(20,context);Options o;o.gate0=first;o.gate1=second;compare(*s,o);}
    for(auto gate:{0u,1u})for(auto value:{0u,1u,2u,5u,10u,127u,128u,255u})for(auto draw:{0u,1u,4u,5u,65535u}){auto s=seed(9);s->wr<4>(16,0x16289);s->memory.write<1>(primary+0x65,value);s->memory.write<1>(primary+0x69,value);s->memory.write<1>(role_profile+0x34,value);s->memory.write<2>(pool+2,draw);Options o;o.gate0=gate;compare(*s,o);}
    for(auto which:{0u,1u,2u,3u}){auto s=seed(9);s->wr<4>(16,0x16289);if(which==0)s->memory.write<4>(club+0x90,0);if(which==1)s->memory.write<4>(primary+0x1c,0);if(which==2)s->memory.write<4>(club+0x80,0);if(which==3)s->memory.write<4>(secondary+0x1c,0);boundaries(*s);}
    for(auto gate:{0u,1u})for(auto result:{0u,65535u,65536u,0xffffffffu}){auto s=seed(9);s->wr<4>(16,0x16289);s->memory.write<4>(globals+0x24,pool);Options o;o.gate0=gate;o.seed_return=result;boundaries(*s,o);}
    constexpr std::array points{0x1037fd44u,0x1037feb0u,0x1038002cu};
    for(auto point:points)for(auto bound:{65536u,0xffff0000u,0x7fffffffu,0x80000000u}){auto s=seed(9);s->wr<4>(16,0x16288);Options o;o.controls={gpr(point,21,bound),gpr(0x1037fe64,20,1)};compare(*s,o);}
    {auto s=seed(9);s->wr<4>(16,0x16288);Options o;o.controls={gpr(0x1037fff8,21,0),Control{0x1037fff8,2304,2,1},gpr(0x1037fe64,20,1)};compare(*s,o);}
}
void presentation_cases(){
    for(unsigned value=0;value<256;++value){auto s=seed(10);s->memory.write<1>(record+0x60,value);compare(*s);}
    for(auto n:{10u,11u})for(auto size:{0u,1u,19u,20u,21u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=seed(n);s->wr<4>(20,size);boundaries(*s);}
    for(auto off:{0x1cu,0x1du,0x1fu,0x20u})for(unsigned value=0;value<256;++value){auto s=seed(11);for(auto at:{0x1cu,0x1du,0x1fu,0x20u})s->memory.write<1>(record+at,0);s->memory.write<1>(record+off,value);compare(*s);}
    for(auto first:{0u,1u,2u,127u,128u,255u})for(auto second:{0u,1u,2u,127u,128u,255u})for(auto last:{0u,1u,2u,255u}){auto s=seed(11);s->memory.write<1>(record+0x1d,0);s->memory.write<1>(record+0x1c,first);s->memory.write<1>(record+0x1f,second);s->memory.write<1>(record+0x20,last);compare(*s);}
    for(auto n:{10u,11u}){auto s=seed(n);s->wr<4>(16,record);boundaries(*s);}
    {Options o;o.mutate_record=true;const auto r=compare(*seed(11),o);require(r.state->r<4>(12)==7,"Independent release-label callback reread");}
    for(auto target:{0x10380cb4u,0x10380cb8u,0x10380cbcu,0x10380cc0u,0x10380d24u,0x10380d28u,0x10380d2cu,0x10380d30u,0x10380d34u,external}){auto s=seed(10);s->memory.write<1>(record+0x60,0);s->memory.write<4>(tables+0x2000,target);compare(*s);}
    {auto s=seed(10);s->memory.copy_in(globals+0xa00,s->memory.copy_out(tables+0x2000,32));s->memory.write<4>(toc-0x4dc0,globals+0xa00);boundaries(*s);}
}
void date_window_cases(){
    for(unsigned age=0;age<256;++age)for(auto periods:{0u,1u,2u,3u})for(auto days:{0u,1u}){auto s=seed(12);Options o;o.age=age;o.elapsed_years=periods;o.elapsed_days=days;const auto r=compare(*s,o);const auto threshold=std::int8_t(std::uint8_t(age))<28?3u:2u;require(r.state->memory.read<1>(output+0x200)==threshold,"Independent signed-age window threshold");require(r.state->r<4>(12)==unsigned(periods<threshold),"Independent elapsed window result");require(r.state->memory.read<1>(output+0x100)==unsigned(periods==threshold&&days==0),"Independent exact window boundary");}
    for(auto mode:{0u,1u,2u,255u})for(auto year:{0u,2000u,2001u,2002u,65535u})for(auto day:{0u,1u,59u,60u,61u,32767u,32768u,65535u}){auto s=seed(12);s->memory.write<1>(globals+0x900,mode);s->memory.write<2>(dates+16,day);s->memory.write<2>(dates+18,year);compare(*s);}
    for(auto a:{0u,output+0x100})for(auto b:{0u,output+0x100,output+0x200}){auto s=seed(12);s->wr<4>(20,a);s->wr<4>(24,b);boundaries(*s);}
    for(auto years:{32767u,32768u,65535u})for(auto days:{0u,1u,32767u,32768u,65535u}){Options o;o.elapsed_years=years;o.elapsed_days=days;compare(*seed(12),o);}
    for(auto age:{0x7fffffffu,0x80000000u,0xffffffffu}){Options o;o.age=age;compare(*seed(12),o);}
}
void faults_relocation_and_negatives(){
    for(unsigned n=0;n<review::entries.size();++n){const auto s=seed(n);const auto base=compare(*s);if(!base.calls.empty()){Options o;o.child_negative=true;compare(*s,o);}auto bad=seed(n);bad->wr<4>(n==10||n==11?16:12,0xf0000000);compare(*bad);}
    for(auto n:{0u,1u,2u,5u,9u,10u,11u,12u}){auto s=seed(n);constexpr auto moved=toc+0x10000;s->memory.map(moved-0x8000,0x9000);s->memory.copy_in(moved-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,moved);boundaries(*s);}
    for(auto n:{6u,7u,8u}){auto s=seed(n);const auto result=compare(*s);const auto expected=n==6?8u:n==7?0u:8u;require(n==8?result.state->r<4>(12)==expected:result.state->memory.read<4>(record+0x48)==expected,"Independent status flags");}
}

void initial_cases(){for(unsigned n=0;n<review::entries.size();++n){boundaries(*seed(n));Options o;o.own_negative=true;compare(*seed(n),o);}}
}
namespace cm::aot {void contract_review_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{initial_cases();scoring_cases();correlated_cases();reset_and_refresh_cases();consequence_cases();presentation_cases();date_window_cases();faults_relocation_and_negatives();for(unsigned n=0;n<review::entries.size();++n)for(auto pc=review::entries[n];pc<ends[n];pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered review "<<hex_address(pc)<<std::endl;require(sites.size()==1447,"Complete 13-operation review source coverage");std::cout<<"Review comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<"/1447"<<std::endl;}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
