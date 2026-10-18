#include "transfer_synthesis.hpp"
#include "semantic_io_checks.hpp"
#include <set>
namespace cm::aot {std::vector<std::uint32_t> transfer_synthesis_expected_sites();bool transfer_synthesis_reference(State&);void transfer_synthesis_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace synthesis=cm::aot::transfer_synthesis;
constexpr std::uint32_t offer=fixture+0x1000,other=fixture+0x1100,previous=fixture+0x1200,staff=fixture+0x4000,profile=fixture+0x5000,career=fixture+0x7000,clubs=fixture+0x8000,buyer=clubs+0x248,contract=fixture+0xb000,exchange_contract=fixture+0xb100,manager=fixture+0xc000,exchange=fixture+0xd000,finance=fixture+0x12000,nodes=fixture+0x18000,globals=fixture+0x30000,tables=fixture+0x34000,output=fixture+0x39000,pool=fixture+0x40000,arena=fixture+0x50000;
struct Control {std::uint32_t pc;unsigned offset;std::uint64_t bits;unsigned width=4;};
struct Options {
    std::vector<Control> controls;unsigned interrupt=0;bool own_negative=false,child_negative=false;
    std::uint32_t seed_return=12345,prior=0,best=0,base=2000000,delta=100000,target=1800000,valuation=1900000,existing_price=1000000,child_sample=0,shortcut=0,excluded=1,allocate_null=0,contract_success=1,clause=0,secondary=0,membership=0,pending=0,affinity=0,policy=1,active=1,accept=1,reason=0,buyer_special=0,primary_special=0;
    std::uint64_t budget=8000000;std::vector<std::uint32_t> primary_results,moods;
};
struct Trace:SemanticGuestObserver {Options options;std::vector<std::uint32_t> sites;explicit Trace(Options o):options(std::move(o)){}void before_step(State& s,std::uint32_t pc)override{sites.push_back(pc);for(auto c:options.controls)if(c.pc==pc){if(c.width==1)s.wr<1>(c.offset,c.bits);else if(c.width==8)s.wr<8>(c.offset,c.bits);else s.wr<4>(c.offset,c.bits);}}};
thread_local Trace* original_trace=nullptr;
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){synthesis::synthesize_offer(s,d,o);}
struct Event {std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
void list(State& s,std::uint32_t object,std::uint32_t storage,std::initializer_list<std::array<std::uint32_t,2>> values){s.memory.clear(object,16);unsigned n=0;for(auto v:values){const auto node=storage+n*32;s.memory.write<4>(node,v[0]);s.memory.write<4>(node+4,v[1]);s.memory.write<4>(node+16,n+1<values.size()?node+32:0);++n;}s.memory.write<2>(object,n);s.memory.write<4>(object+4,n?storage:0);s.memory.write<4>(object+8,n?storage+(n-1)*32:0);s.memory.write<4>(object+12,n?storage:0);}
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;unsigned primary_calls=0,mood_calls=0,allocations=0,appends=0;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){while(s.pc!=stop){if(s.pc>=synthesis::synthesize_offer_entry&&s.pc<0x10415e4c)require(transfer_synthesis_reference(s),"Complete synthesis reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));}}
    static std::vector<std::uint8_t> text(State& s,std::uint32_t at){std::vector<std::uint8_t> b;for(unsigned n=0;n<8192;++n){auto c=std::uint8_t(s.memory.read<1>(at+n));if(!c)return b;b.push_back(c);}throw std::runtime_error("Unterminated synthesis fixture text");}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        constexpr std::array allowed{0x10739890u,0x10768810u,0x1077b720u,0x10769b60u,0x1077b770u,0x10082f20u,0x1014c2e0u,0x103f5d20u,0x10738c00u,0x10773520u,0x10083110u,0x103eaea0u,0x103eb020u,0x10738e00u,0x103eb440u,0x10738d10u,0x10738c20u,0x10103510u,0x100f5770u,0x1036fbf0u,0x103faff0u,0x103721f0u,0x10080d30u,0x107735a0u,0x10089a40u,0x100d3040u,0x10085860u,0x10380520u,0x10361490u,0x10084be0u,0x10415e50u,0x100f55b0u,0x100ce250u,0x103f4240u,0x10416710u,0x104172f0u,0x100c7a20u,0x10380f20u,0x103fac70u,0x103f8820u};
        require(std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known synthesis dependency "+hex_address(target));const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try{
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};const auto first=arg(3),second=arg(4);std::uint32_t result=0,second_result=0;
            s.memory.write<4>(globals+0x600,target);s.memory.write<4>(globals+0x604,s.memory.read<4>(globals+0x604)+1);
            if(target==0x1077b770)result=options.seed_return;
            if(target==0x10082f20){(void)s.memory.read<4>(second);result=primary_calls<options.primary_results.size()?options.primary_results[primary_calls]:(second==staff?contract:exchange_contract);++primary_calls;}
            if(target==0x1014c2e0)result=first==buyer?options.buyer_special:options.primary_special;
            if(target==0x103f5d20)result=options.prior;
            if(target==0x10773520){++allocations;result=options.allocate_null==allocations?0:arena+allocations*0x200;}
            if(target==0x10738c00){s.memory.clear(first,16);result=first;}
            if(target==0x103721f0){s.memory.clear(first,100);result=first;}
            if(target==0x10083110)result=options.secondary;
            if(target==0x103eaea0)result=options.membership;
            if(target==0x103eb020){if(options.pending)list(s,arg(5),nodes+0x1000,{{0,0}});result=options.pending;}
            if(target==0x103eb440){s.memory.write<4>(output+0x400,previous);result=output+0x400;}
            if(target==0x10738e00){const auto node=std::uint32_t(s.memory.read<4>(first+12));s.memory.write<4>(first+12,node?s.memory.read<4>(node+16):0);result=node;}
            if(target==0x10738d10){require(arg(5)==8,"Eight-byte exchange payload");const auto n=std::uint32_t(s.memory.read<2>(first)),node=arena+0x2000+appends++*32;s.memory.copy_in(node,s.memory.copy_out(second,8));s.memory.write<4>(node+16,0);const auto last=std::uint32_t(s.memory.read<4>(first+8));if(last)s.memory.write<4>(last+16,node);else s.memory.write<4>(first+4,node);s.memory.write<4>(first+8,node);s.memory.write<2>(first,n+1);result=first;}
            if(target==0x10738c20){if(first)s.memory.clear(first,16);result=first;}
            if(target==0x107735a0){if(first)s.memory.write<4>(globals+0x608,first);}
            if(target==0x10103510){(void)s.memory.read<4>(second);result=finance;}
            if(target==0x100f5770){result=std::uint32_t(options.budget>>32);second_result=std::uint32_t(options.budget);}
            if(target==0x1036fbf0){s.memory.write<4>(arg(7),0x12345678);result=options.shortcut;}
            if(target==0x103faff0)result=options.excluded;
            if(target==0x10080d30)result=options.contract_success;
            if(target==0x10089a40)result=options.base;
            if(target==0x100d3040)result=options.policy;
            if(target==0x10085860)result=arg(9)?options.base:options.base-options.delta;
            if(target==0x10380520)result=options.clause;
            if(target==0x10361490){s.memory.write<4>(arg(7),0x7f00a5a5);result=options.target;}
            if(target==0x10084be0)result=options.valuation;
            if(target==0x10415e50)result=options.child_sample;
            if(target==0x100f55b0){result=mood_calls<options.moods.size()?options.moods[mood_calls]:0;++mood_calls;}
            if(target==0x100ce250)result=options.affinity;
            if(target==0x103f4240)result=options.best;
            if(target==0x10416710)result=options.existing_price;
            if(target==0x104172f0){s.memory.write<4>(globals+0x60c,first);result=0x100;}
            if(target==0x100c7a20)result=options.policy;
            if(target==0x10380f20)result=options.active;
            if(target==0x103fac70)result=options.valuation;
            if(target==0x103f8820){s.memory.write<2>(arg(9),options.reason);result=options.accept;}
            if(target==0x10739890){(void)text(s,first);s.memory.write<4>(arg(6),123);s.memory.write<4>(arg(7),45);}
            if(target==0x10768810)result=output+0x800;
            if(target==0x1077b720){const auto b=text(s,second);s.memory.copy_in(first,b);s.memory.write<1>(first+std::uint32_t(b.size()),0);result=first;}
            if(target==0x10769b60){(void)text(s,first);(void)text(s,second);}
            for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
            for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-70.0-n));s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);if(target==0x100f5770)s.wr<4>(16,second_result);s.pc=stop;
            if(options.child_negative&&!original)s.memory.write<1>(globals+0x700,1);calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture transfer-synthesis dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned b=1280;b<1312;++b)s->wr<1>(b,0);s->memory.map(fixture,0x80000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);s->pc=synthesis::synthesize_offer_entry;s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;s->wr<4>(12,offer);s->wr<4>(16,staff);s->wr<4>(20,buyer);s->wr<4>(24,other);s->wr<4>(28,0xffffffff);s->wr<4>(32,0);s->wr<4>(36,output+0x100);
    for(auto [slot,ptr]:{std::pair{-0x7be0,globals},std::pair{-0x7be8,globals+4},std::pair{-0x7b7c,globals+8},std::pair{-0x7b78,globals+12},std::pair{-0x7b90,globals+16},std::pair{-0x7b84,globals+20},std::pair{-0x7b88,globals+24},std::pair{-0x7b8c,pool},std::pair{-0x7bac,globals+28},std::pair{-0x7b40,globals+32},std::pair{-0x7ab8,globals+36},std::pair{-0x7ba8,output},std::pair{-0x7b80,globals+0x200},std::pair{-0x7a10,globals+0x210},std::pair{-0x7a1c,globals+0x214}})s->memory.write<4>(toc+slot,ptr);
    s->memory.write<4>(globals,staff);s->memory.write<4>(globals+4,clubs);s->memory.write<4>(globals+8,globals+0x100);s->memory.write<4>(globals+16,12345);s->memory.write<4>(globals+20,pool);s->memory.write<4>(globals+24,pool+0x100);s->memory.write<1>(globals+28,1);s->memory.write<4>(globals+32,manager);s->memory.write<4>(globals+36,finance);s->memory.write<4>(manager+0x988,manager+0xa00);s->memory.write<4>(contract+4,0);s->memory.write<4>(exchange_contract+4,1);s->memory.write<4>(contract+0x24,2500000);s->memory.write<1>(contract+0x38,0);s->memory.write<2>(globals+0x200,2001);s->memory.write<2>(globals+0x202,100);s->memory.write<1>(globals+0x204,0);
    for(unsigned k=0;k<4;++k){s->memory.write<4>(staff+k*0x70,k);s->memory.write<4>(staff+k*0x70+8,2000000);s->memory.write<4>(staff+k*0x70+12,career+k*0x100);s->memory.write<4>(staff+k*0x70+0x14,profile+k*0x100);s->memory.write<4>(staff+k*0x70+0x24,clubs);s->memory.write<2>(staff+k*0x70+0x5c,20);s->memory.write<1>(staff+k*0x70+0x62,11);s->memory.write<2>(profile+k*0x100+8,10000);s->memory.write<2>(profile+k*0x100+10,7001);s->memory.write<2>(career+k*0x100+0xac,6000);s->memory.write<4>(clubs+k*0x248,k);s->memory.write<4>(clubs+k*0x248+0x2c,globals+0x300);s->memory.write<2>(clubs+k*0x248+0x1ec,100);}
    s->memory.write<4>(other+4,2000000);s->memory.write<4>(other+12,1000000);s->memory.write<1>(other,11);s->memory.write<1>(other+0x28,5<<3);s->memory.write<4>(other+0x20,2000000);s->memory.write<1>(other+0x44,10);s->memory.write<1>(other+0x4c,10);s->memory.write<1>(other+0x34,2);s->memory.write<4>(other+0x2c,0xffffffff);s->memory.write<4>(other+0x30,0xffffffff);s->memory.copy_in(offer,std::vector<std::uint8_t>(0x54,0x9f));
    s->memory.copy_in(previous,s->memory.copy_out(other,0x54));s->memory.write<4>(previous+4,exchange);s->memory.write<4>(previous+12,0);list(*s,exchange,nodes,{{1,0x07010203},{2,0x09040506}});for(unsigned k=0;k<256;++k)s->memory.write<2>(pool+k*2,(12345+k*1009)&65535);transfer_synthesis_fixture_tables(*s,toc,tables);s->memory.copy_in(output+0x800,{102,105,120,116,117,114,101,0});return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;Trace ot(options),nt(options);Model old(true,options,ot),modern(false,options,nt);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&ot;m.reference(s);}else modern_root(s,&m,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Transfer synthesis root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture transfer-synthesis dependency interruption"||expected.starts_with("Unmapped data access at "),label+" fixture assertion: "+expected);
    if(ot.sites!=nt.sites){for(unsigned k=0;k<std::min(ot.sites.size(),nt.sites.size());++k)if(ot.sites[k]!=nt.sites[k]){std::cerr<<"First transfer-synthesis site "<<k<<" "<<hex_address(ot.sites[k])<<"/"<<hex_address(nt.sites[k])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){if(x.bytes!=y.bytes){for(unsigned n=0;n<std::min(x.log.size(),y.log.size());++n){const auto& ex=x.log[n];const auto& ac=y.log[n];if(ex.kind!=ac.kind||ex.address!=ac.address||ex.size!=ac.size||ex.value!=ac.value){std::cerr<<"First synthesis effect "<<n<<" expected="<<ex.kind<<":"<<hex_address(ex.address)<<":"<<ex.size<<":"<<ex.value<<" actual="<<ac.kind<<":"<<hex_address(ac.address)<<":"<<ac.size<<":"<<ac.value<<" sites="<<ot.sites.size()<<"/"<<nt.sites.size()<<" special="<<options.buyer_special<<" prior="<<hex_address(options.prior)<<std::endl;break;}}}require(x.bytes==y.bytes,label+" effects");if(old.calls!=modern.calls){for(unsigned n=0;n<std::min(old.calls.size(),modern.calls.size());++n)if(!(old.calls[n]==modern.calls[n])){const auto& xcall=old.calls[n];const auto& ycall=modern.calls[n];std::cerr<<"Call mismatch "<<n<<" target="<<hex_address(xcall.target)<<"/"<<hex_address(ycall.target)<<" stop="<<hex_address(xcall.stop)<<"/"<<hex_address(ycall.stop)<<std::endl;for(unsigned b=0;b<xcall.before.size();++b)if(xcall.before[b]!=ycall.before[b]){std::cerr<<"before byte="<<b<<" "<<unsigned(xcall.before[b])<<"/"<<unsigned(ycall.before[b])<<std::endl;break;}for(unsigned b=0;b<xcall.after.size();++b)if(xcall.after[b]!=ycall.after[b]){std::cerr<<"after byte="<<b<<" "<<unsigned(xcall.after[b])<<"/"<<unsigned(ycall.after[b])<<std::endl;break;}break;}}require(old.calls==modern.calls,label+" dependency ABI");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" accounting");}
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;structural+=!options.controls.empty();if(expected.empty()&&!rejected)sites.insert(ot.sites.begin(),ot.sites.end());return{std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}){
    auto base=compare(input,options);require(base.state->pc==return_sentinel,"Completed transfer-synthesis boundary fixture root="+hex_address(input.pc)+" stop="+hex_address(base.state->pc));const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(auto a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<6;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [a,n]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(n){s->memory.map(a,n);s->memory.copy_in(a,input.memory.copy_out(a,n));}}compare(*s,options);}
    for(unsigned n=1;n<=base.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}

Control gpr(std::uint32_t pc,unsigned reg,std::uint32_t value){return {pc,reg*4,value,4};}
void synthesis_cases(){
    {auto s=seed();Options o;o.controls.push_back(gpr(0x104131c4,18,0));compare(*s,o);}
    {auto s=seed();boundaries(*s);Options o;o.own_negative=true;compare(*s,o);o.own_negative=false;o.child_negative=true;compare(*s,o);}
    for(auto nullarg:{4u,5u}){auto s=seed();s->wr<4>(nullarg*4,0);boundaries(*s);}
    for(auto special:{0u,1u,256u,257u}){auto s=seed();Options o;o.buyer_special=special;boundaries(*s,o);o.prior=previous;boundaries(*s,o);}
    for(auto limit:{0xffffffffu,0u,1u,4999u,5000u,99999u,100000u,500000u,1000000u,1500000u,1800000u,2000000u,3000000u}){auto s=seed();s->wr<4>(28,limit);boundaries(*s);}
    for(auto first:{0u,1u,2u,3u,0xfeu,0xffu})for(auto second:{0u,1u,2u,3u,0xfeu,0xffu}){auto s=seed();Options o;o.moods={first,second};compare(*s,o);}
}
void contract_and_finalization_cases(){
    for(auto source:{0u,other})for(auto out:{0u,output+0x100})for(auto mode:{0u,1u,255u,256u,257u}){auto a=seed();a->wr<4>(24,source);a->wr<4>(36,out);a->wr<4>(32,mode);compare(*a);}
    for(auto klass:{0u,4u,12u,20u,128u,255u})for(auto shortcut:{0u,1u,256u,257u})for(auto excluded:{0u,1u}){auto a=seed();a->memory.write<1>(contract+0x38,klass);Options o;o.shortcut=shortcut;o.excluded=excluded;compare(*a,o);}
    for(auto result:{0u,1u,256u,257u})for(auto allocation:{0u,1u}){auto a=seed();Options o;o.shortcut=1;o.contract_success=result;o.allocate_null=allocation;boundaries(*a,o);}
    for(auto first:{0u,contract}){auto a=seed();Options o;o.primary_results=std::vector<std::uint32_t>(12,first);boundaries(*a,o);a->wr<4>(36,0);boundaries(*a,o);}
    for(auto accept:{0u,1u,256u,257u})for(auto reason:{0u,8u,9u,10u,65535u})for(auto policy:{0u,1u}){auto a=seed();Options o;o.accept=accept;o.reason=reason;o.policy=policy;compare(*a,o);}
    for(auto active:{0u,1u,256u,257u})for(auto policy:{0u,1u})for(auto mode:{0u,1u}){auto a=seed();a->wr<4>(32,mode);Options o;o.shortcut=1;o.active=active;o.policy=policy;boundaries(*a,o);}
    for(auto clause:{0u,1u,256u,257u})for(auto value:{0u,1000000u,2000000u,2500000u}){auto a=seed();a->memory.write<4>(contract+0x24,value);Options o;o.clause=clause;compare(*a,o);}
    for(auto source_kind:{0u,10u,11u,12u,255u}){auto a=seed();a->memory.write<1>(other,source_kind);compare(*a);a->memory.write<4>(staff+0x14,0);compare(*a);}
    {auto a=seed();a->wr<4>(24,0);a->memory.write<4>(staff+0x14,0);boundaries(*a);}
    for(auto best:{0u,previous})for(auto price:{0u,1u,500000u,1000000u,1800000u,3000000u,0xffffffffu}){auto a=seed();Options o;o.best=best;o.existing_price=price;compare(*a,o);}
    for(auto money:{UINT64_C(0),UINT64_C(1),UINT64_C(1799999),UINT64_C(1800000),UINT64_C(1800001),UINT64_C(0x7fffffffffffffff),UINT64_C(0x8000000000000000),UINT64_C(0xffffffffffffffff)}){auto a=seed();Options o;o.budget=money;boundaries(*a,o);}
}
void priority_and_exchange_cases(){
    for(auto copied:{false,true})for(auto skill:{0u,3999u,4000u,7000u,7001u,32767u,32768u,65535u})for(auto tag:{0u,3u,4u,5u}){auto a=seed();a->memory.write<2>(profile+10,skill);a->memory.write<1>(other+0x28,tag<<3);Options o;if(copied){o.buyer_special=1;o.prior=previous;}compare(*a,o);}
    for(auto copied:{false,true})for(auto added:{0u,1u,65535u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto value:{0u,1u,3u,20u,65535u}){auto a=seed();a->memory.write<1>(other+0x28,0);a->memory.write<2>(profile+10,32767);a->memory.write<4>(globals+16,added);for(unsigned n=0;n<256;++n)a->memory.write<2>(pool+n*2,value);Options o;if(copied){o.buyer_special=1;o.prior=previous;}compare(*a,o);}
    for(auto copied:{false,true})for(auto cursor:{pool-2,pool,pool+0xfe,pool+0x100,pool+0x102}){auto a=seed();a->memory.write<1>(other+0x28,0);a->memory.write<4>(globals+20,cursor);Options o;if(copied){o.buyer_special=1;o.prior=previous;}boundaries(*a,o);}
    for(auto mode:{0u,1u,256u,257u}){auto a=seed();a->wr<4>(32,mode);Options o;o.buyer_special=1;o.prior=previous;boundaries(*a,o);}
    for(auto secondary:{0u,contract})for(auto member:{0u,1u})for(auto pending:{0u,1u}){auto a=seed();Options o;o.buyer_special=1;o.prior=previous;o.secondary=secondary;o.membership=member;o.pending=pending;boundaries(*a,o);}
    for(auto kind:{0u,10u,11u,12u}){auto a=seed();a->memory.write<1>(staff+0x70+0x62,kind);a->memory.write<1>(staff+0xe0+0x62,kind);Options o;o.buyer_special=1;o.prior=previous;compare(*a,o);}
    for(auto klass:{0u,20u,128u,255u}){auto a=seed();a->memory.write<1>(exchange_contract+0x38,klass);Options o;o.buyer_special=1;o.prior=previous;compare(*a,o);}
    for(auto club_index:{0u,1u,2u}){auto a=seed();a->memory.write<4>(exchange_contract+4,club_index);Options o;o.buyer_special=1;o.prior=previous;boundaries(*a,o);}
    for(auto empty:{false,true}){auto a=seed();if(empty)list(*a,exchange,nodes,{});else a->memory.write<4>(previous+4,0);Options o;o.buyer_special=1;o.prior=previous;boundaries(*a,o);}
}
void pricing_cases(){
    for(auto limit:{0xffffffffu,0u,50000u,99999u,100000u,500000u,1000000u,1500000u,2000000u,3000000u})for(auto target:{0u,600000u,800000u,1200000u,1500000u,1600000u,1800000u,2000000u,2400000u})for(auto delta:{0u,100000u}){auto a=seed();a->wr<4>(28,limit);Options o;o.target=target;o.delta=delta;compare(*a,o);}
    for(auto availability:{0u,1u,4u,5u})for(auto rep:{4999u,5000u,5001u})for(auto ability:{1000u,10000u})for(auto affinity:{0u,1u}){auto a=seed();a->wr<4>(28,500000);a->memory.write<1>(staff+0x5e,availability);a->memory.write<2>(career+0xac,rep);a->memory.write<2>(profile+8,ability);Options o;o.affinity=affinity;compare(*a,o);o.target=100000;compare(*a,o);}
    for(auto tag:{0u,3u,4u,5u})for(auto age:{20u,27u,28u,40u,128u,255u})for(auto delta:{0u,100000u}){auto a=seed();a->wr<4>(28,500000);a->memory.write<1>(other+0x28,tag<<3);a->memory.write<2>(staff+0x5c,age);Options o;o.delta=delta;compare(*a,o);}
    for(auto limit:{0xffffffffu,500000u,2000000u})for(auto available:{0u,1u}){auto a=seed();a->wr<4>(28,limit);Options o;o.primary_special=1;o.policy=available;boundaries(*a,o);}
    for(auto special:{0u,1u})for(auto target:{1000000u,1800000u,2200000u})for(auto value:{500000u,1800000u,2400000u}){auto a=seed();a->memory.write<1>(finance+0x165,1);Options o;o.primary_special=special;o.target=target;o.valuation=value;boundaries(*a,o);}
    for(auto added:{0u,1u,65535u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto sample:{0u,1u,2u,3u,4u,5u,9u,19u,20u,65535u})for(auto limit:{500000u,1800000u}){auto a=seed();a->wr<4>(28,limit);a->memory.write<4>(globals+16,added);for(unsigned n=0;n<256;++n)a->memory.write<2>(pool+n*2,sample);Options o;o.child_sample=sample;compare(*a,o);}
}

void optional_distribution_cases(){
    for(auto availability:{0u,1u,5u})for(auto tag:{0u,4u,5u})for(auto age:{20u,40u})for(auto sample:{0u,1u,2u,5u,10000u,35000u,50000u,60000u,65535u}){auto a=seed();a->wr<4>(28,1500000);a->memory.write<1>(staff+0x5e,availability);a->memory.write<1>(other+0x28,tag<<3);a->memory.write<2>(staff+0x5c,age);a->memory.write<4>(globals+16,0);for(unsigned n=0;n<256;++n)a->memory.write<2>(pool+n*2,sample);Options o;o.delta=0;o.moods={2,0};compare(*a,o);}
    for(auto availability:{0u,1u,5u})for(auto ability:{1000u,10000u})for(auto affinity:{0u,1u}){auto a=seed();a->memory.write<1>(staff+0x5e,availability);a->memory.write<2>(profile+8,ability);Options o;o.target=1000000;o.affinity=affinity;o.moods={2,0};boundaries(*a,o);}
    for(auto target:{0u,1000000u,1500000u,1550000u,1599999u,1600000u,1800000u,2100000u,2500000u})for(auto value:{0u,1000000u,2000000u,3000000u})for(auto sample:{0u,1u,2u,3u,65535u}){auto a=seed();a->wr<4>(28,3000000);a->memory.write<4>(globals+16,0);for(unsigned n=0;n<256;++n)a->memory.write<2>(pool+n*2,sample);Options o;o.target=target;o.best=previous;o.existing_price=value;o.moods={2,0};compare(*a,o);}
    for(auto value:{0u,1u,2u,3u,4u,5u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto availability:{0u,5u}){auto a=seed();a->wr<4>(28,1500000);a->memory.write<1>(staff+0x5e,availability);a->memory.write<4>(globals+16,value);Options o;o.delta=0;o.moods={2,0};compare(*a,o);}
    for(auto value:{0u,10000u,100000u,500000u,1500000u,2000000u,3000000u}){auto a=seed();a->memory.write<1>(finance+0x165,1);Options o;o.target=2500000;o.valuation=value;o.moods={2,0};compare(*a,o);}
    {auto a=seed();Options o;o.primary_results={contract,0};boundaries(*a,o);}
    for(auto value:{0u,1000000u,1999999u}){auto a=seed();Options o;o.buyer_special=1;o.budget=value;boundaries(*a,o);}
    for(auto mode:{0u,1u}){auto a=seed();a->wr<4>(28,1);a->wr<4>(32,mode);Options o;o.shortcut=1;o.valuation=10000;boundaries(*a,o);}
    for(auto mood:{0xfdu,0xfeu,0xffu,0x1feu}){auto a=seed();Options o;o.moods={mood,mood};compare(*a,o);}
    for(auto named:{0u,1u,2u}){auto a=seed();a->memory.write<4>(globals+0x368,named);a->memory.write<4>(globals+0x210,1);a->memory.write<4>(globals+0x214,2);compare(*a);}
}

void rollover_and_numeric_cases(){
    for(auto availability:{0u,5u})for(auto ability:{1000u,10000u})for(auto limit:{500000u,1500000u,3000000u})for(auto target:{600000u,1000000u,1550000u,1800000u})for(auto sample:{0u,1u,2u,5u,60000u}){auto a=seed();a->wr<4>(28,limit);a->memory.write<4>(globals+24,pool);a->memory.write<4>(globals+16,0);a->memory.write<1>(staff+0x5e,availability);a->memory.write<2>(profile+8,ability);for(unsigned n=0;n<256;++n)a->memory.write<2>(pool+n*2,sample);Options o;o.seed_return=0;o.delta=limit==1500000?0:1800000;o.target=target;o.moods={2,0};compare(*a,o);}
    for(auto age:{20u,40u})for(auto tag:{4u,5u})for(auto sample:{0u,1u,2u,60000u}){auto a=seed();a->wr<4>(28,1500000);a->memory.write<4>(globals+24,pool);a->memory.write<4>(globals+16,0);a->memory.write<1>(staff+0x5e,5);a->memory.write<1>(other+0x28,tag<<3);a->memory.write<2>(staff+0x5c,age);for(unsigned n=0;n<256;++n)a->memory.write<2>(pool+n*2,sample);Options o;o.seed_return=0;o.delta=0;o.moods={2,0};boundaries(*a,o);}
    for(auto copied:{false,true}){auto a=seed();a->memory.write<1>(other+0x28,0);a->memory.write<2>(profile+10,32767);a->memory.write<4>(globals+24,pool);Options o;o.seed_return=65535;if(copied){o.buyer_special=1;o.prior=previous;}boundaries(*a,o);}
    for(auto amount:{0u,1u,50000u,2000000u})for(auto availability:{0u,5u}){auto a=seed();a->wr<4>(28,1500000);a->memory.write<1>(staff+0x5e,availability);a->memory.write<1>(other+0x28,4<<3);a->memory.write<2>(staff+0x5c,40);a->memory.write<4>(other+12,amount);a->memory.write<4>(other+4,amount);Options o;o.delta=0;o.moods={2,0};compare(*a,o);}
    for(auto up:{0u,1u,50000u,1000000u,2000000u,0xffffffffu})for(auto down:{1u,1000000u,2000000u})for(auto sample:{0u,1u}){auto a=seed();a->wr<4>(28,10000000);a->memory.write<4>(other+12,up);a->memory.write<4>(other+4,down);a->memory.write<4>(globals+16,0);for(unsigned n=0;n<256;++n)a->memory.write<2>(pool+n*2,sample);Options o;o.target=1550000;o.moods={2,0};compare(*a,o);}
    for(auto low_profile:{false,true}){auto a=seed();a->wr<4>(28,1500000);a->memory.write<4>(staff+0x20,clubs);if(low_profile)a->memory.write<2>(profile+8,1000);Options o;o.delta=0;o.affinity=1;o.moods={2,0};boundaries(*a,o);a->wr<4>(28,500000);boundaries(*a,o);}
    {auto a=seed();a->memory.write<1>(finance+0x165,1);Options o;o.base=500000;o.target=1000000;o.moods={2,0};boundaries(*a,o);}
    for(auto out:{0u,output+0x100}){auto a=seed();a->wr<4>(28,500000);a->wr<4>(36,out);Options o;o.buyer_special=1;boundaries(*a,o);}
    for(auto sequence:{std::vector<std::uint32_t>{contract,0},std::vector<std::uint32_t>{contract,exchange_contract,0}}){auto a=seed();Options o;o.buyer_special=1;o.prior=previous;o.primary_results=sequence;boundaries(*a,o);}
}
void structural_and_alias_cases(){
    // Explicit register-state inputs exercise paths that stable game objects
    // cannot normally reach, including redundant guards and signed samplers.
    for(auto pc:{0x104144b4u,0x104147c0u,0x10414ac0u,0x10414c5cu})for(auto bound:{1u,7u,65535u,65536u,0xffff0000u,0xffffffffu}){auto a=seed();a->wr<4>(28,1500000);a->memory.write<1>(staff+0x5e,5);a->memory.write<1>(other+0x28,4<<3);a->memory.write<2>(staff+0x5c,40);Options o;o.delta=0;o.moods={2,0};o.controls.push_back(gpr(pc,14,bound));boundaries(*a,o);}
    for(auto pc:{0x10414480u,0x1041478cu}){auto a=seed();a->wr<4>(28,1500000);a->memory.write<1>(staff+0x5e,5);Options o;o.delta=0;o.moods={2,0};o.controls.push_back({pc,2304,2,1});o.controls.push_back(gpr(pc,14,0));compare(*a,o);}
    for(auto pc:{0x10413c08u,0x10413eacu})for(auto base:{1u,10000u,0xffffffffu}){auto a=seed();a->wr<4>(28,500000);Options o;o.delta=1800000;o.controls.push_back(gpr(pc,22,base));if(pc==0x10413eac)a->memory.write<2>(profile+8,1000);compare(*a,o);}
    {auto a=seed();a->wr<4>(28,10000000);a->memory.write<4>(globals+16,0);for(unsigned n=0;n<256;++n)a->memory.write<2>(pool+n*2,1);Options o;o.target=1550000;o.moods={2,0};o.controls.push_back(gpr(0x104158c4,3,0));compare(*a,o);}
    for(auto copied:{false,true})for(auto primary:{std::vector<std::uint32_t>{0},std::vector<std::uint32_t>{contract,0}}){auto a=seed();a->memory.map(0,0x100);a->memory.write<1>(other+0x28,0);a->memory.write<2>(profile+10,32767);Options o;o.controls.push_back(gpr(0x104127a4,18,other));if(copied){o.buyer_special=1;o.prior=previous;o.primary_results={contract,contract,contract};o.primary_results.insert(o.primary_results.end(),primary.begin(),primary.end());}else o.primary_results=primary;compare(*a,o);}
    for(auto destination:{offer,other,previous})for(auto source:{0u,other,offer}){auto a=seed();a->wr<4>(12,destination);a->wr<4>(24,source);Options o;o.buyer_special=1;o.prior=previous;compare(*a,o);}
    for(auto slot:{offer+0x20,previous+4,staff+0x14}){auto a=seed();a->wr<4>(36,slot);compare(*a);}
}

void remaining_control_flow_cases(){
    for(auto availability:{0u,5u})for(auto ability:{1000u,10000u})for(unsigned pattern=1;pattern<=12;++pattern){auto a=seed();a->wr<4>(28,500000);a->memory.write<1>(staff+0x5e,availability);a->memory.write<2>(profile+8,ability);a->memory.write<4>(globals+16,0);std::uint32_t random=pattern;for(unsigned n=0;n<256;++n){random=random*1664525+1013904223;a->memory.write<2>(pool+n*2,random>>16);}Options o;o.delta=1800000;o.moods={2,0};compare(*a,o);o.delta=0;compare(*a,o);a->wr<4>(28,0xffffffff);o.target=1000000;compare(*a,o);}
    for(auto availability:{0u,5u})for(auto tag:{0u,4u,5u})for(auto age:{20u,28u,40u})for(auto affinity:{0u,1u}){auto a=seed();a->memory.write<1>(staff+0x5e,availability);a->memory.write<2>(profile+8,1000);a->memory.write<4>(staff+0x20,clubs);a->memory.write<1>(other+0x28,tag<<3);a->memory.write<2>(staff+0x5c,age);Options o;o.target=1000000;o.moods={2,0};o.affinity=affinity;compare(*a,o);}
    {auto a=seed();a->wr<4>(28,1500000);a->memory.write<4>(globals+24,pool);a->memory.write<4>(globals+16,0);a->memory.write<1>(staff+0x5e,5);a->memory.write<1>(other+0x28,4<<3);a->memory.write<2>(staff+0x5c,40);a->memory.write<4>(other+12,2000000);a->memory.write<4>(other+4,1000000);for(unsigned n=0;n<256;++n)a->memory.write<2>(pool+n*2,60000);Options o;o.delta=0;o.seed_return=0;o.moods={2,0};boundaries(*a,o);}
    // Structural state inputs isolate the cap-selection branches; ordinary
    // period limits can make their comparison redundant for normal proposals.
    for(auto pc:{0x10413c34u,0x10413ed8u}){auto a=seed();a->wr<4>(28,500000);if(pc==0x10413ed8)a->memory.write<2>(profile+8,1000);Options o;o.delta=1800000;o.moods={2,0};o.controls.push_back(gpr(pc,3,0));o.controls.push_back(gpr(pc,0,1));compare(*a,o);}
}

void independent_expectation_and_relocation_cases(){
    for(auto budget:{UINT64_C(1234567),UINT64_C(8000000)}){auto a=seed();Options o;o.buyer_special=1;o.budget=budget;auto result=compare(*a,o);require(result.state->r<4>(12)==1,"Independent special-club proposal accepted");require(result.state->memory.read<4>(offer+0x20)==std::min(std::uint64_t(2000000),budget),"Independent special-club base uses signed low finance word");}
    {auto a=seed();Options o;o.shortcut=1;o.contract_success=0;auto result=compare(*a,o);require(result.state->r<4>(12)==0,"Independent contract constructor failure");require(result.state->memory.read<4>(output+0x100)==arena+0x200,"Original failed constructor retains freed output pointer");require(result.state->memory.read<4>(globals+0x608)==arena+0x200,"Independent output-contract disposal");}
    {auto a=seed();Options o;o.buyer_special=1;o.prior=previous;auto result=compare(*a,o);const auto list=std::uint32_t(result.state->memory.read<4>(offer+4));require(list&&result.state->memory.read<2>(list)==2,"Both eligible exchange players retained");const auto node=std::uint32_t(result.state->memory.read<4>(list+4));require(result.state->memory.read<4>(node)==1&&result.state->memory.read<4>(node+4)==0x00010200,"Exchange ID and original partial payload initialization");}
    for(auto special:{0u,1u}){auto a=seed();constexpr std::uint32_t alternate_toc=0x18008000;a->memory.map(alternate_toc-0x8000,0x9000);a->memory.copy_in(alternate_toc-0x8000,a->memory.copy_out(toc-0x8000,0x9000));a->wr<4>(8,alternate_toc);transfer_synthesis_fixture_tables(*a,alternate_toc,tables+0x10000);Options o;o.buyer_special=special;if(special)o.prior=previous;boundaries(*a,o);}
    for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x3ff0000000000000),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000000001234)})for(auto off:{0x18u,0x78u,0xa0u}){auto a=seed();a->memory.write<8>(tables+off,bits);a->wr<4>(28,1500000);Options o;o.delta=0;compare(*a,o);}
}

}
namespace cm::aot {void transfer_synthesis_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{synthesis_cases();contract_and_finalization_cases();priority_and_exchange_cases();pricing_cases();optional_distribution_cases();rollover_and_numeric_cases();structural_and_alias_cases();remaining_control_flow_cases();independent_expectation_and_relocation_cases();const auto expected=transfer_synthesis_expected_sites();for(auto pc:expected)if(!sites.contains(pc))std::cerr<<"Uncovered synthesis site "<<hex_address(pc)<<std::endl;require(sites.size()==expected.size()&&std::all_of(expected.begin(),expected.end(),[](auto pc){return sites.contains(pc);}),"Every complete original synthesis site required");std::cout<<"Transfer synthesis comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<"/3551"<<std::endl;}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
