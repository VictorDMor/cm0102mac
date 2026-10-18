#include "transfer_valuation.hpp"
#include "semantic_io_checks.hpp"
#include <set>
namespace cm::aot {std::vector<std::uint32_t> transfer_valuation_expected_sites();bool transfer_valuation_reference(State&);void transfer_valuation_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace valuation=cm::aot::transfer_valuation;
constexpr std::uint32_t offer=fixture+0x1000,other=fixture+0x1100,staff=fixture+0x4000,profile=fixture+0x5000,career=fixture+0x7000,clubs=fixture+0x8000,contract=fixture+0xb000,manager=fixture+0xc000,exchange=fixture+0xd000,other_exchange=fixture+0xe000,flags=fixture+0x10000,finance=fixture+0x12000,nodes=fixture+0x18000,globals=fixture+0x30000,tables=fixture+0x34000,output=fixture+0x39000,pool=fixture+0x40000;
constexpr std::array<std::uint32_t,8> ends{0x104163c8,0x10416d24,0x10416d5c,0x10416e04,0x104170f0,0x10417260,0x104172e4,0x10417590};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{valuation::sample_offer_range,valuation::adjusted_offer_value,valuation::positive_installment_total,valuation::staff_offer_status,valuation::equivalent_offer_terms,valuation::remove_exchange_staff,valuation::find_exchange_staff,valuation::rebalance_offer_terms};
struct Control {std::uint32_t pc;unsigned offset;std::uint64_t bits;unsigned width=4;};
struct Options {std::vector<Control> controls;unsigned interrupt=0;std::uint32_t seed_return=12345,mood=0,stats0=100,stats1=200,valuation_base=4000,normal_divisor=1;std::uint64_t budget=0;bool own_negative=false,child_negative=false,replace_exchange=false,mutate_normalized_fields=false;std::vector<std::uint32_t> primary_results;std::vector<std::array<std::uint32_t,2>> normalized_results;};
struct Trace:SemanticGuestObserver {Options options;std::vector<std::uint32_t> sites;explicit Trace(Options o):options(std::move(o)){}void before_step(State& s,std::uint32_t pc)override{sites.push_back(pc);for(auto c:options.controls)if(c.pc==pc){if(c.width==1)s.wr<1>(c.offset,c.bits);else if(c.width==8)s.wr<8>(c.offset,c.bits);else s.wr<4>(c.offset,c.bits);}}};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(valuation::entries.begin(),valuation::entries.end(),root);require(i!=valuation::entries.end(),"Known transfer-valuation root "+hex_address(root));return unsigned(i-valuation::entries.begin());}
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){methods[index(s.pc)](s,d,o);}
struct Event {std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
void list(State& s,std::uint32_t object,std::uint32_t arena,std::initializer_list<std::array<std::uint32_t,2>> values){s.memory.clear(object,16);unsigned n=0;for(auto v:values){const auto node=arena+n*32;s.memory.write<4>(node,v[0]);s.memory.write<4>(node+4,v[1]);s.memory.write<4>(node+16,n+1<values.size()?node+32:0);++n;}s.memory.write<2>(object,n);s.memory.write<4>(object+4,n?arena:0);s.memory.write<4>(object+8,n?arena+(n-1)*32:0);s.memory.write<4>(object+12,n?arena:0);}
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;unsigned primary_calls=0,normal_calls=0;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto root=s.pc,n=index(root);bool first=true;while(s.pc!=stop){if(s.pc>=root&&s.pc<ends[n]&&(first||s.pc!=root))require(transfer_valuation_reference(s),"Complete valuation reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}}
    static std::vector<std::uint8_t> text(State& s,std::uint32_t at){std::vector<std::uint8_t> b;for(unsigned n=0;n<8192;++n){auto c=std::uint8_t(s.memory.read<1>(at+n));if(!c)return b;b.push_back(c);}throw std::runtime_error("Unterminated valuation fixture text");}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        const bool own=std::find(valuation::entries.begin(),valuation::entries.end(),target)!=valuation::entries.end();
        constexpr std::array allowed{0x1077b770u,0x10082f20u,0x10219e40u,0x10103510u,0x100f55b0u,0x100f5770u,0x10084be0u,0x10738e00u,0x10009f20u,0x10738e60u,0x10738c20u,0x10739890u,0x10768810u,0x1077b720u,0x10769b60u};
        require(own||std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known valuation dependency "+hex_address(target));const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            if(own){if(original)reference(s,stop);else modern_root(s,this,&trace);}
            else {
                const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};const auto first=arg(3),second=arg(4);std::uint32_t result=0,second_result=0;
                s.memory.write<4>(globals+0x600,target);s.memory.write<4>(globals+0x604,s.memory.read<4>(globals+0x604)+1);
                if(target==0x1077b770)result=options.seed_return;
                if(target==0x10082f20){(void)s.memory.read<4>(second);result=primary_calls<options.primary_results.size()?options.primary_results[primary_calls]:contract;++primary_calls;}
                if(target==0x10219e40){(void)s.memory.read<4>(second);require(arg(6)==1||arg(6)==2,"Known statistics modes");result=arg(6)==2?options.stats0:options.stats1;}
                if(target==0x10103510){(void)s.memory.read<4>(second);result=finance;}
                if(target==0x100f55b0)result=options.mood;
                if(target==0x100f5770){result=std::uint32_t(options.budget>>32);second_result=std::uint32_t(options.budget);}
                if(target==0x10084be0){const auto id=std::uint32_t(s.memory.read<4>(second));result=options.valuation_base+id*100;}
                if(target==0x10009f20){require(second==4,"Known amount normalization width");const auto raw=std::uint32_t(s.memory.read<4>(first));require(options.normal_divisor>0,"Nonzero normalization divisor");const auto normalized=std::int64_t(std::bit_cast<std::int32_t>(raw))/options.normal_divisor;const auto bits=std::bit_cast<std::uint64_t>(normalized);result=std::uint32_t(bits>>32);second_result=std::uint32_t(bits);if(normal_calls<options.normalized_results.size()){result=options.normalized_results[normal_calls][0];second_result=options.normalized_results[normal_calls][1];}++normal_calls;if(options.mutate_normalized_fields&&normal_calls==1)s.memory.write<4>(other+0x40,999);}
                if(target==0x10738e00){const auto node=std::uint32_t(s.memory.read<4>(first+12));s.memory.write<4>(first+12,node?s.memory.read<4>(node+16):0);result=node;if(options.replace_exchange&&first==exchange&&node)s.memory.write<4>(offer+4,other_exchange);}
                if(target==0x10738e60){const auto value=s.memory.read<4>(second);auto node=std::uint32_t(s.memory.read<4>(first+4)),previous=0u;unsigned visited=0;while(node){require(++visited<512,"Bounded remove fixture");const auto next=std::uint32_t(s.memory.read<4>(node+16));if(s.memory.read<4>(node)==value){if(previous)s.memory.write<4>(previous+16,next);else s.memory.write<4>(first+4,next);if(!next)s.memory.write<4>(first+8,previous);if(s.memory.read<4>(first+12)==node)s.memory.write<4>(first+12,next);s.memory.write<2>(first,s.memory.read<2>(first)-1);break;}previous=node;node=next;}result=first;}
                if(target==0x10738c20){if(first)s.memory.clear(first,16);result=first;}
                if(target==0x10739890){(void)text(s,first);s.memory.write<4>(arg(6),123);s.memory.write<4>(arg(7),45);}
                if(target==0x10768810)result=output+0x800;
                if(target==0x1077b720){const auto b=text(s,second);s.memory.copy_in(first,b);s.memory.write<1>(first+std::uint32_t(b.size()),0);result=first;}
                if(target==0x10769b60){(void)text(s,first);(void)text(s,second);}
                for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-70.0-n));s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);if(target==0x10009f20||target==0x100f5770)s.wr<4>(16,second_result);s.pc=stop;
            }
            if(options.child_negative&&!original)s.memory.write<1>(globals+0x700,1);calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture transfer-valuation dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned b=1280;b<1312;++b)s->wr<1>(b,0);s->memory.map(fixture,0x80000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);s->pc=valuation::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;s->wr<4>(12,offer);s->wr<4>(16,0);s->wr<4>(20,0);s->wr<4>(24,0);s->wr<4>(28,1);s->wr<4>(32,1);
    for(auto [slot,ptr]:{std::pair{-0x7be0,globals},std::pair{-0x7be8,globals+4},std::pair{-0x7b7c,globals+8},std::pair{-0x7b78,globals+12},std::pair{-0x7b90,globals+16},std::pair{-0x7b84,globals+20},std::pair{-0x7b88,globals+24},std::pair{-0x7b8c,pool},std::pair{-0x7bac,globals+28},std::pair{-0x7b40,globals+32},std::pair{-0x7ab8,globals+36},std::pair{-0x7ba8,output}})s->memory.write<4>(toc+slot,ptr);
    s->memory.write<4>(globals,staff);s->memory.write<4>(globals+4,clubs);s->memory.write<4>(globals+8,globals+0x100);s->memory.write<4>(globals+16,12345);s->memory.write<4>(globals+20,pool);s->memory.write<4>(globals+24,pool+0x100);s->memory.write<1>(globals+28,1);s->memory.write<4>(globals+32,manager);s->memory.write<4>(globals+36,finance);s->memory.write<4>(manager+0x9a0,flags);s->memory.write<4>(contract+4,0);s->memory.write<1>(contract+0x38,4);
    for(unsigned k=0;k<4;++k){s->memory.write<4>(staff+k*0x70,k);s->memory.write<4>(staff+k*0x70+8,10000+k*1000);s->memory.write<4>(staff+k*0x70+12,career+k*0x100);s->memory.write<4>(staff+k*0x70+0x14,profile+k*0x100);s->memory.write<4>(staff+k*0x70+0x24,clubs);s->memory.write<2>(staff+k*0x70+0x5c,20);s->memory.write<2>(profile+k*0x100+4,70);s->memory.write<2>(profile+k*0x100+6,100);s->memory.write<2>(profile+k*0x100+10,7001);s->memory.write<2>(career+k*0x100+0xac,6000);s->memory.write<4>(clubs+k*0x248,k);s->memory.write<2>(clubs+k*0x248+0x1ec,100);s->memory.write<1>(flags+k*10+2,2);}
    s->memory.write<4>(offer+4,exchange);s->memory.write<4>(offer+12,0);s->memory.write<1>(offer+0x10,2);s->memory.write<1>(offer+0x11,11);s->memory.write<1>(offer+0x34,3);s->memory.write<4>(offer+0x20,1000);s->memory.write<4>(offer+0x40,200);s->memory.write<1>(offer+0x44,10);s->memory.write<4>(offer+0x48,300);s->memory.write<1>(offer+0x4c,10);s->memory.write<1>(offer+0x50,10);s->memory.write<1>(offer+0x51,6);s->memory.copy_in(other,s->memory.copy_out(offer,0x54));s->memory.write<4>(other+4,other_exchange);
    list(*s,exchange,nodes,{{1,0x07000000},{2,0x09000000}});list(*s,other_exchange,nodes+0x100,{{1,0x07000000},{2,0x09000000}});for(unsigned k=0;k<256;++k)s->memory.write<2>(pool+k*2,(12345+k*1009)&65535);transfer_valuation_fixture_tables(*s,toc,tables);s->memory.copy_in(output+0x800,{102,105,120,116,117,114,101,0});
    if(n==0)s->wr<4>(12,10);
    if(n==4)s->wr<4>(16,other);
    if(n==5||n==6)s->wr<4>(16,staff+0x70);
    if(n==7){s->wr<4>(16,staff);s->wr<4>(24,clubs);s->memory.write<4>(offer+0x40,0);s->memory.write<4>(offer+0x48,0);s->memory.write<1>(offer+0x50,0);}
    return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;Trace ot(options),nt(options);Model old(true,options,ot),modern(false,options,nt);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&ot;m.reference(s);}else modern_root(s,&m,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Transfer valuation root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture transfer-valuation dependency interruption"||expected.starts_with("Unmapped data access at "),label+" fixture assertion: "+expected);
    if(ot.sites!=nt.sites){for(unsigned k=0;k<std::min(ot.sites.size(),nt.sites.size());++k)if(ot.sites[k]!=nt.sites[k]){std::cerr<<"First transfer-valuation site "<<k<<" "<<hex_address(ot.sites[k])<<"/"<<hex_address(nt.sites[k])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" effects");if(old.calls!=modern.calls){for(unsigned n=0;n<std::min(old.calls.size(),modern.calls.size());++n)if(!(old.calls[n]==modern.calls[n])){const auto& xcall=old.calls[n];const auto& ycall=modern.calls[n];std::cerr<<"Call mismatch "<<n<<" target="<<hex_address(xcall.target)<<"/"<<hex_address(ycall.target)<<" stop="<<hex_address(xcall.stop)<<"/"<<hex_address(ycall.stop)<<std::endl;for(unsigned b=0;b<xcall.before.size();++b)if(xcall.before[b]!=ycall.before[b]){std::cerr<<"before byte="<<b<<" "<<unsigned(xcall.before[b])<<"/"<<unsigned(ycall.before[b])<<std::endl;break;}for(unsigned b=0;b<xcall.after.size();++b)if(xcall.after[b]!=ycall.after[b]){std::cerr<<"after byte="<<b<<" "<<unsigned(xcall.after[b])<<"/"<<unsigned(ycall.after[b])<<std::endl;break;}break;}}require(old.calls==modern.calls,label+" dependency ABI");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" accounting");}
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;structural+=!options.controls.empty();if(expected.empty()&&!rejected)sites.insert(ot.sites.begin(),ot.sites.end());return{std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}){
    auto base=compare(input,options);require(base.state->pc==return_sentinel,"Completed transfer-valuation boundary fixture root="+hex_address(input.pc)+" stop="+hex_address(base.state->pc));const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(auto a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<6;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [a,n]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(n){s->memory.map(a,n);s->memory.copy_in(a,input.memory.copy_out(a,n));}}compare(*s,options);}
    for(unsigned n=1;n<=base.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}

Control gpr(std::uint32_t pc,unsigned reg,std::uint32_t value){return {pc,reg*4,value,4};}
void sampler_cases(){
    for(auto bound:{0u,1u,7u,65535u,65536u,0x7fffffffu,0x80000000u,0xffff0000u,0xffff0001u,0xffffffffu}){auto a=seed(0);a->wr<4>(12,bound);boundaries(*a);}
    constexpr std::array points{0x10415f68u,0x10416054u,0x10416140u};
    for(unsigned depth=1;depth<=3;++depth)for(auto value:{65536u,0xffff0000u}){auto a=seed(0);a->wr<4>(12,65536);Options o;for(unsigned n=0;n<depth;++n)o.controls.push_back(gpr(points[n],22-n,value));compare(*a,o);}
    for(unsigned depth=1;depth<=2;++depth){auto a=seed(0);a->wr<4>(12,65536);Options o;o.controls.push_back(gpr(points[0],22,depth==1?0x7fffffffu:65536u));if(depth==2)o.controls.push_back(gpr(points[1],21,0x7fffffffu));compare(*a,o);}
    {auto a=seed(0);a->wr<4>(12,65536);a->memory.write<4>(globals+24,pool);Options o;for(unsigned n=0;n<3;++n)o.controls.push_back(gpr(points[n],22-n,65536));boundaries(*a,o);}
    for(auto bound:{10u,65536u,0xffff0000u})for(auto reset:{0u,65535u,65536u,0xffffffffu}){auto a=seed(0);a->wr<4>(12,bound);a->memory.write<4>(globals+20,pool+0x100);Options o;o.seed_return=reset;boundaries(*a,o);}
    for(auto cursor:{pool-2,pool,pool+0xfe,pool+0x100,pool+0x102}){auto a=seed(0);a->memory.write<4>(globals+20,cursor);boundaries(*a);}
    for(auto bound:{1u,7u,65535u,0xffff0001u,0xfffffff9u})for(auto sample:{0u,1u,65535u})for(auto added:{0u,1u,65535u,0x7fffffffu,0x80000001u,0xffffffffu}){auto a=seed(0);a->wr<4>(12,bound);a->memory.write<2>(pool+2,sample);a->memory.write<4>(globals+16,added);const auto result=compare(*a);const auto sum=std::bit_cast<std::int32_t>(sample+added);const auto limit=std::bit_cast<std::int32_t>(bound);const auto remainder=std::int64_t(sum)%limit;const auto expected=std::uint32_t(limit>0?remainder:-remainder);require(result.state->r<4>(12)==expected,"Independent narrow signed range remainder");}
    for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x3ff0000000000000),UINT64_C(0x7fefffffffffffff),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000000001234)}){auto a=seed(0);a->wr<4>(12,65536);a->memory.write<8>(tables+0xa0,bits);boundaries(*a);}
}
void installment_cases(){
    for(auto x:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto y:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}){auto a=seed(2);a->memory.write<4>(offer+0x40,x);a->memory.write<4>(offer+0x48,y);const auto result=compare(*a);const auto expected=(std::int32_t(x)>0?x:0)+(std::int32_t(y)>0?y:0);require(result.state->r<4>(12)==expected,"Independent wrapping positive installment sum");}
}
void adjusted_cases(){
    {auto a=seed(1);a->memory.write<4>(offer+0x20,0);a->memory.write<4>(offer+0x48,0);a->memory.write<1>(offer+0x50,0);a->wr<4>(20,1);a->wr<4>(28,0);const auto result=compare(*a);require(result.state->r<4>(12)==190,"Independent 200-unit installment with code 10 discounts to 190");}
    for(unsigned kind=0;kind<256;++kind){auto a=seed(1);a->memory.write<1>(contract+0x38,kind);compare(*a);}
    for(auto mood:{0u,1u,2u,3u,0xfdu,0xfeu,0xffu,0x100u,0x1feu})for(auto period:{0u,1u,6u,7u,12u,13u,127u,128u,255u}){auto a=seed(1);a->memory.write<1>(offer+0x51,period);Options o;o.mood=mood;boundaries(*a,o);}
    for(auto which:{4u,5u,6u,7u,8u})for(auto value:{0u,1u,255u,256u,257u,0x80000000u,0xffffffffu}){auto a=seed(1);a->wr<4>(which*4,value);boundaries(*a);}
    for(auto age:{0u,20u,24u,27u,28u,127u,128u,255u,256u,65535u})for(auto stat:{0u,1u,99u,100u,125u,200u,32767u,32768u,65535u,65536u}){auto a=seed(1);a->memory.write<2>(staff+0x5c,age);Options o;o.stats0=stat;o.stats1=stat;compare(*a,o);}
    for(auto amount:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto code:{0u,10u,127u,128u,255u})for(auto off:{0x40u,0x48u}){auto a=seed(1);a->memory.write<4>(offer+off,amount);a->memory.write<1>(offer+off+4,code);compare(*a);}
    for(auto percentage:{0u,1u,10u,127u,128u,255u})for(auto availability:{0u,1u})for(auto reputation:{0u,6000u,12999u,13000u,32767u,32768u,65535u}){auto a=seed(1);a->memory.write<1>(offer+0x50,percentage);a->memory.write<1>(staff+0x5e,availability);a->memory.write<2>(career+0xac,reputation);compare(*a);}
    for(auto variant:{0u,1u,2u,3u,4u,5u}){auto a=seed(1);Options o;if(variant==0)o.primary_results={0};if(variant==1)a->memory.write<4>(staff+0x24,0);if(variant==2){a->memory.write<1>(offer+0x11,10);a->memory.write<4>(staff+0x14,0);}if(variant==3)a->memory.write<4>(offer+4,0);if(variant==4)list(*a,exchange,nodes,{});if(variant==5)o.replace_exchange=true;boundaries(*a,o);}
    {auto a=seed(1);a->memory.write<4>(offer+0x40,0);a->memory.write<4>(offer+0x48,0);a->memory.write<1>(offer+0x50,0);a->memory.write<1>(offer+0x51,0);const auto result=compare(*a);require(result.state->r<4>(12)==24000,"Independent base plus actual stored exchange values");}
    {auto a=seed(1);a->memory.write<4>(offer+0x40,0);a->memory.write<4>(offer+0x48,0);a->memory.write<1>(offer+0x50,0);a->memory.write<1>(offer+0x51,0);a->wr<4>(24,1);const auto result=compare(*a);require(result.state->r<4>(12)==9300,"Independent adjusted mode values each exchanged player");std::vector<std::uint32_t> people;for(const auto& call:result.calls)if(call.target==0x10084be0){std::uint32_t address=0;for(unsigned n=0;n<4;++n)address=(address<<8)|call.before[16+n];people.push_back(address);}require(people==std::vector<std::uint32_t>{staff+0x70,staff+0xe0},"Adjusted valuation passes each exchange identity");}
    for(auto mood:{0xfdu,0xfeu,0xffu,0u,1u,2u}){auto a=seed(1);a->memory.write<4>(offer+0x40,0);a->memory.write<4>(offer+0x48,0);a->memory.write<1>(offer+0x50,0);a->wr<4>(20,1);a->memory.write<1>(offer+0x51,13);Options o;o.mood=mood;const auto result=compare(*a,o);const auto expected=mood==0xfe?100u:mood==0xff?300u:mood==0?400u:mood==1?900u:1000u;require(result.state->r<4>(12)==expected,"Independent final single precision scale by mood and period");}
    for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x3fe0000000000000),UINT64_C(0x7ff0000000000000),UINT64_C(0x7ff8000000001234)}){auto a=seed(1);a->memory.write<8>(tables+0xd0,bits);boundaries(*a);}
}
void status_cases(){
    for(unsigned kind=0;kind<256;++kind)for(auto stored:{0u,7u,255u}){auto a=seed(3);a->memory.write<1>(offer+0x34,kind);a->memory.write<1>(offer+0x10,stored);const auto result=compare(*a);const auto expected=kind==3?4:stored;require(result.state->r<4>(12)==expected&&result.state->memory.read<1>(offer+0x10)==expected,"Independent stateful main-target status");}
    for(auto id:{1u,2u,3u,0xffffffffu}){auto a=seed(3);a->wr<4>(16,id);boundaries(*a);}
    for(auto n:{3u,6u})for(auto variant:{0u,1u,2u}){auto a=seed(n);if(n==3)a->wr<4>(16,1);if(variant==0)a->memory.write<4>(offer+4,0);if(variant==1)list(*a,exchange,nodes,{});if(variant==2)a->memory.write<4>(staff+0x70,3);boundaries(*a);}
    {auto a=seed(6);const auto result=compare(*a);require(result.state->r<4>(12)==nodes,"Independent matching exchange payload pointer");}
    for(auto id:{0u,1u,2u,3u,0xffffffffu}){auto a=seed(6);a->memory.write<4>(staff+0x70,id);compare(*a);}
}
void comparison_cases(){
    for(auto off:{0x20u,0x40u,0x48u})for(auto left:{0u,1u,1000u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto right:{0u,1u,1000u,0x7fffffffu,0x80000000u,0xffffffffu}){auto a=seed(4);a->memory.write<4>(offer+off,left);a->memory.write<4>(other+off,right);const auto result=compare(*a);const bool expected=left==right||(off!=0x20&&std::int32_t(left)<=0&&std::int32_t(right)<=0);require(result.state->r<4>(12)==expected,"Independent normalized amount presence comparison");}
    for(auto off:{0x44u,0x4cu,0x50u,0x51u,0x38u})for(unsigned value=0;value<256;++value){auto a=seed(4);a->memory.write<1>(other+off,value);compare(*a);}
    for(auto off:{0x44u,0x4cu})for(auto left:{0u,0x80000000u,0xffffffffu})for(auto right:{0u,0x80000000u,0xffffffffu}){auto a=seed(4);a->memory.write<4>(offer+off-4,left);a->memory.write<4>(other+off-4,right);a->memory.write<1>(other+off,255);const auto result=compare(*a);require(result.state->r<4>(12)==1,"Independent absent installments ignore differing period codes");}
    for(auto off:{0x50u,0x51u})for(auto left:{0u,127u,128u,255u})for(auto right:{0u,127u,128u,255u}){auto a=seed(4);a->memory.write<1>(offer+off,left);a->memory.write<1>(other+off,right);const auto result=compare(*a);const bool expected=left==right||(std::int8_t(left)<=0&&std::int8_t(right)<=0);require(result.state->r<4>(12)==expected,"Independent signed condition presence");}
    for(auto variant:{0u,1u,2u,3u,4u,5u,6u,7u}){auto a=seed(4);if(variant==0){a->memory.write<4>(offer+4,0);a->memory.write<4>(other+4,0);}if(variant==1)a->memory.write<4>(offer+4,0);if(variant==2)a->memory.write<4>(other+4,0);if(variant==3)list(*a,other_exchange,nodes+0x100,{{1,0}});if(variant==4)list(*a,other_exchange,nodes+0x100,{{2,0},{1,0}});if(variant==5)list(*a,other_exchange,nodes+0x100,{{1,0},{3,0}});if(variant==6){list(*a,exchange,nodes,{});list(*a,other_exchange,nodes+0x100,{});}if(variant==7)a->wr<4>(16,offer);boundaries(*a);}
    {auto a=seed(4);a->memory.write<4>(offer+0x20,1001);Options o;o.normal_divisor=100;const auto result=compare(*a,o);require(result.state->r<4>(12)==1,"Independent normalization precedes economic comparison");}
    {auto a=seed(4);list(*a,other_exchange,nodes+0x100,{{1,0},{1,0}});const auto result=compare(*a);require(result.state->r<4>(12)==1,"Original count plus directional membership does not consume duplicate matches");a->wr<4>(12,other);a->wr<4>(16,offer);require(compare(*a).state->r<4>(12)==0,"Independent duplicate-list directional asymmetry");}
    {auto a=seed(4);a->wr<4>(16,offer);list(*a,exchange,nodes,{{1,0},{1,0}});a->budget=1000;compare(*a);}
    for(auto hi:{0u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto lo:{0u,1u,0xffffffffu}){auto a=seed(4);Options o;o.normalized_results={{{hi,lo}},{{hi,lo}}};boundaries(*a,o);o.normalized_results[1][1]^=1;boundaries(*a,o);}
    {auto a=seed(4);Options o;o.mutate_normalized_fields=true;boundaries(*a,o);}
}
void removal_cases(){
    for(auto id:{1u,2u,3u})for(auto count:{0u,1u,255u}){auto a=seed(5);a->memory.write<4>(staff+0x70,id);a->memory.write<1>(flags+id*10+2,count);const auto result=compare(*a);require(result.state->memory.read<4>(offer+0x20)==5000+id*100,"Removal adds service value even when no matching item exists");require(result.state->memory.read<1>(flags+id*10+2)==std::uint8_t(count-1),"Independent wrapping cache decrement");}
    for(auto variant:{0u,1u,2u,3u,4u}){auto a=seed(5);if(variant==0)a->memory.write<4>(offer+4,0);if(variant==1)list(*a,exchange,nodes,{});if(variant==2)list(*a,exchange,nodes,{{1,0}});if(variant==3)list(*a,exchange,nodes,{{1,0},{1,0}});if(variant==4)list(*a,exchange,nodes,{{2,0}});boundaries(*a);}
    {auto a=seed(5);Options o;o.primary_results={0};boundaries(*a,o);}
    for(auto amount:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}){auto a=seed(5);a->memory.write<4>(offer+0x20,amount);Options o;o.valuation_base=0xffffffff;boundaries(*a,o);}
}
void rebalance_cases(){
    for(auto variant:{0u,1u,2u,3u}){auto a=seed(7);if(variant==0){a->memory.write<2>(clubs+0x1ec,10);a->memory.write<2>(profile+10,7500);}if(variant==2){a->memory.write<2>(profile+10,4000);a->memory.write<2>(career+0xac,0);}if(variant==3){a->memory.write<2>(clubs+0x1ec,8);a->memory.write<2>(profile+10,4000);a->memory.write<2>(career+0xac,0);}const auto result=compare(*a);const auto base=result.state->memory.read<4>(offer+0x20);require(base==(variant==3?1000:800),"Independent payment rebalance base factor");if(variant==0)require(result.state->memory.read<1>(offer+0x50)==20,"Independent percentage conversion");if(variant==1)require(result.state->memory.read<4>(offer+0x48)==300&&result.state->memory.read<1>(offer+0x4c)==10,"Independent contingent installment conversion");if(variant==2)require(result.state->memory.read<4>(offer+0x40)==300&&result.state->memory.read<1>(offer+0x44)==20,"Independent ordinary installment conversion");boundaries(*a);}
    for(auto gap:{24u,25u,26u,32767u,65535u}){auto a=seed(7);a->memory.write<2>(profile+4,0);a->memory.write<2>(profile+6,gap);boundaries(*a);}
    for(auto kind:{0u,14u,15u,16u,32767u,32768u,65535u})for(auto skill:{0u,100u,1499u,1500u,4250u,4251u,7000u,32767u,32768u,65535u}){auto a=seed(7);a->memory.write<2>(clubs+0x1ec,kind);a->memory.write<2>(profile+10,skill);compare(*a);}
    for(auto value:{0u,1u,127u,128u,255u,0x80000000u,0xffffffffu})for(auto off:{0x40u,0x48u,0x50u}){auto a=seed(7);if(off==0x50)a->memory.write<1>(offer+off,value);else a->memory.write<4>(offer+off,value);compare(*a);}
    for(auto money:{0u,1u,1000u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto finance_value:{UINT64_C(0),UINT64_C(1),UINT64_C(1999),UINT64_C(2000),UINT64_C(2001),UINT64_C(0x7fffffffffffffff),UINT64_C(0x8000000000000000),UINT64_C(0xffffffffffffffff)}){auto a=seed(7);a->memory.write<4>(offer+0x20,money);Options o;o.budget=finance_value;boundaries(*a,o);}
    for(auto availability:{0u,1u})for(auto reputation:{0u,4999u,5000u,5001u,32767u,32768u,65535u}){auto a=seed(7);a->memory.write<1>(staff+0x5e,availability);a->memory.write<2>(career+0xac,reputation);compare(*a);}
    {auto a=seed(7);a->memory.write<4>(staff+0x14,0);boundaries(*a);}
    {auto a=seed(7);a->memory.write<4>(staff+12,0);boundaries(*a);}
}
void relocation_cases(){
    for(auto n:{0u,1u,5u,7u}){auto a=seed(n);constexpr std::uint32_t other_toc=0x18008000;a->memory.map(other_toc-0x8000,0x9000);a->memory.copy_in(other_toc-0x8000,a->memory.copy_out(toc-0x8000,0x9000));a->wr<4>(8,other_toc);transfer_valuation_fixture_tables(*a,other_toc,tables+0x10000);if(n==5)a->memory.write<4>(offer+4,0);boundaries(*a);}
}

void baseline_cases(){for(unsigned n=0;n<valuation::entries.size();++n){auto s=seed(n);boundaries(*s);Options o;o.own_negative=true;compare(*s,o);if(n==2)continue;o.own_negative=false;o.child_negative=true;if(n==0)s->memory.write<4>(globals+20,pool+0x100);if(n==3)s->wr<4>(16,1);compare(*s,o);}}
}
namespace cm::aot {void transfer_valuation_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{baseline_cases();sampler_cases();installment_cases();adjusted_cases();status_cases();comparison_cases();removal_cases();rebalance_cases();relocation_cases();
    const auto expected=transfer_valuation_expected_sites();for(auto pc:expected)if(!sites.contains(pc))std::cerr<<"Uncovered valuation site "<<hex_address(pc)<<std::endl;require(sites.size()==expected.size()&&std::all_of(expected.begin(),expected.end(),[](auto pc){return sites.contains(pc);}),"Every complete original valuation site required");
    for(unsigned n=0;n<valuation::entries.size();++n)std::cout<<"PASS root="<<hex_address(valuation::entries[n])<<std::endl;
    std::cout<<"Transfer valuation comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<"/1188"<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
