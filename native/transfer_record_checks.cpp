#include "transfer_records.hpp"
#include "semantic_io_checks.hpp"
#include <set>
namespace cm::aot {std::vector<std::uint32_t> transfer_record_expected_sites();bool transfer_record_reference(State&);void transfer_record_fixture_tables(State&,std::uint32_t,std::uint32_t);}
namespace {
using namespace cm::semantic_io_checks;
namespace records=cm::aot::transfer_records;
constexpr std::uint32_t offer=fixture+0x1000,staff=fixture+0x4000,clubs=fixture+0x8000,profile=fixture+0xa000,contract=fixture+0xb000,prior=fixture+0xc000,prior_offer=fixture+0xc100,group=fixture+0xd000,exchange=fixture+0xd100,nodes=fixture+0x10000,allocated_lists=fixture+0x18000,appended_nodes=fixture+0x20000,globals=fixture+0x30000,tables=fixture+0x34000,output=fixture+0x38000,pool=fixture+0x39000;
constexpr std::array<std::uint32_t,8> ends{0x10411ea0,0x10411f08,0x10411f9c,0x104123d4,0x104126cc,0x104164e4,0x10416710,0x10416834};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{records::construct_empty_offer,records::construct_generated_offer,records::destroy_offer,records::read_offer,records::write_offer,records::initialize_zero_fee_offer,records::initialize_priced_offer,records::offer_value};
struct Options {unsigned interrupt=0,short_io=0;std::uint32_t io_result=0,seed=0x1234,valuation=1234;int primary_count=2,exchange_count=2;bool null_primary_after_header=false,replace_exchange_on_next=false,partial=false,allocation_failure=false,own_negative=false,child_negative=false,structural=false;std::uint32_t header_primary=0x12345678,header_exchange=0xabcdef00;std::vector<std::uint8_t> input_bytes;std::vector<std::uint32_t> primary_items{0,3},primary_results;std::vector<std::array<std::uint32_t,2>> exchange_items{{1,75},{2,125}};};
struct Trace:SemanticGuestObserver {std::vector<std::uint32_t> sites;void before_step(State&,std::uint32_t pc)override{sites.push_back(pc);}};
thread_local Trace* original_trace=nullptr;
unsigned index(std::uint32_t root){const auto i=std::find(records::entries.begin(),records::entries.end(),root);require(i!=records::entries.end(),"Known transfer-record root "+hex_address(root));return unsigned(i-records::entries.begin());}
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o){methods[index(s.pc)](s,d,o);}
struct Event{std::uint32_t target,stop;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
void list(State& s,std::uint32_t object,std::uint32_t arena,std::initializer_list<std::array<std::uint32_t,2>> values){s.memory.clear(object,16);unsigned n=0;for(auto v:values){const auto node=arena+n*32;s.memory.write<4>(node,v[0]);s.memory.write<4>(node+4,v[1]);s.memory.write<4>(node+16,n+1<values.size()?node+32:0);++n;}s.memory.write<2>(object,n);s.memory.write<4>(object+4,n?arena:0);s.memory.write<4>(object+8,n?arena+(n-1)*32:0);s.memory.write<4>(object+12,n?arena:0);}
struct Model:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;unsigned primary_calls=0,io_calls=0;std::size_t position=0;std::vector<std::uint8_t> input,written;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){
        input.resize(0x54,0x5a);const auto put=[&](unsigned off,unsigned size,std::uint32_t value){for(unsigned n=0;n<size;++n)input[off+n]=std::uint8_t(value>>((size-1-n)*8));};put(0,4,options.header_primary);put(4,4,options.header_exchange);put(8,4,1);put(12,4,0);put(0x28,4,3);
        const auto append=[&](unsigned size,std::uint32_t value){for(unsigned n=size;n;--n)input.push_back(std::uint8_t(value>>((n-1)*8)));};
        append(2,std::uint16_t(options.primary_count));for(int n=0;n<options.primary_count;++n)append(4,options.primary_items[unsigned(n)%options.primary_items.size()]);append(2,std::uint16_t(options.exchange_count));for(int n=0;n<options.exchange_count;++n){auto item=options.exchange_items[unsigned(n)%options.exchange_items.size()];append(4,item[0]);append(4,item[1]);}if(!options.input_bytes.empty())input=options.input_bytes;
    }
    void reference(State& s,std::uint32_t stop=return_sentinel){const auto root=s.pc,n=index(root);bool first=true;while(s.pc!=stop){if(s.pc>=root&&s.pc<ends[n]&&(first||s.pc!=root))require(transfer_record_reference(s),"Complete transfer-record reference");else call(s,s.pc,std::uint32_t(s.r<4>(4128)));first=false;}}
    static std::vector<std::uint8_t> text(State& s,std::uint32_t at){std::vector<std::uint8_t> b;for(unsigned n=0;n<8192;++n){auto c=std::uint8_t(s.memory.read<1>(at+n));if(!c)return b;b.push_back(c);}throw std::runtime_error("Unterminated record fixture text");}
    void call(State& s,std::uint32_t target,std::uint32_t stop)override {
        constexpr std::array allowed{0x10715880u,0x104126d0u,0x10738c20u,0x10738e60u,0x107735a0u,0x10747a80u,0x10747cc0u,0x10739890u,0x10768810u,0x1077b720u,0x10769b60u,0x10773520u,0x10738c00u,0x10738d10u,0x10738ca0u,0x10738e00u,0x10082f20u,0x1077b770u,0x10084be0u};
        require(std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Known transfer-record dependency "+hex_address(target));const auto serial=calls.size();calls.push_back({target,stop,s.registers,{}});
        try {
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};const auto first=arg(3),second=arg(4);std::uint32_t result=0;
            s.memory.write<4>(globals+0x600,target);s.memory.write<4>(globals+0x604,s.memory.read<4>(globals+0x604)+1);
            if(target==0x10715880){s.memory.copy_in(first,{0,1,0,2,3});result=first;}
            if(target==0x104126d0){s.memory.write<4>(first,0);s.memory.write<4>(first+4,0);s.memory.write<4>(first+0x20,5000);result=1;}
            if(target==0x10773520){require(first==16,"Known record fixture allocation width");const auto n=std::uint32_t(s.memory.read<4>(globals+0x80));s.memory.write<4>(globals+0x80,n+1);result=options.allocation_failure?0:allocated_lists+n*0x100;}
            if(target==0x107735a0)s.memory.write<4>(globals+0x84,first);
            if(target==0x10738c00||target==0x10738c20||target==0x10738ca0){if(first)s.memory.clear(first,16);result=first;}
            if(target==0x10738e00){const auto node=std::uint32_t(s.memory.read<4>(first+12));s.memory.write<4>(first+12,node?s.memory.read<4>(node+16):0);result=node;if(options.replace_exchange_on_next&&first==exchange&&node)s.memory.write<4>(offer+4,group);}
            if(target==0x10738d10){const auto width=arg(5);require(width==4||width==8,"Known record fixture list width");const auto bytes=s.memory.copy_out(second,width);const auto n=std::uint32_t(s.memory.read<4>(globals+0x88));require(n<512,"Bounded record append arena");s.memory.write<4>(globals+0x88,n+1);const auto node=appended_nodes+n*32;s.memory.copy_in(node,bytes);s.memory.write<4>(node+16,0);const auto tail=std::uint32_t(s.memory.read<4>(first+8));if(tail)s.memory.write<4>(tail+16,node);else s.memory.write<4>(first+4,node);s.memory.write<4>(first+8,node);s.memory.write<2>(first,s.memory.read<2>(first)+1);result=node;}
            if(target==0x10738e60){const auto value=s.memory.read<4>(second);auto node=std::uint32_t(s.memory.read<4>(first+4)),previous=0u;while(node){const auto next=std::uint32_t(s.memory.read<4>(node+16));if(s.memory.read<4>(node)==value){if(previous)s.memory.write<4>(previous+16,next);else s.memory.write<4>(first+4,next);if(!next)s.memory.write<4>(first+8,previous);s.memory.write<2>(first,s.memory.read<2>(first)-1);break;}previous=node;node=next;}result=first;}
            if(target==0x10747a80||target==0x10747cc0){require(arg(5)==1&&(second==0x54||second==2||second==4||second==8),"Known record IO shape");++io_calls;const bool shorted=options.short_io==io_calls;const auto width=shorted?(options.partial?second/2:0):second;
                if(target==0x10747a80){require(position+width<=input.size(),"Record fixture input available");s.memory.copy_in(first,std::vector<std::uint8_t>(input.begin()+std::ptrdiff_t(position),input.begin()+std::ptrdiff_t(position+width)));position+=width;}
                else{const auto bytes=s.memory.copy_out(first,width);written.insert(written.end(),bytes.begin(),bytes.end());}if(target==0x10747cc0&&io_calls==1&&options.null_primary_after_header)s.memory.write<4>(offer,0);result=shorted?options.io_result:1;
            }
            if(target==0x10082f20){(void)s.memory.read<4>(second);result=primary_calls<options.primary_results.size()?options.primary_results[primary_calls]:contract;++primary_calls;}
            if(target==0x1077b770)result=options.seed;
            if(target==0x10084be0){(void)s.memory.read<4>(second);result=options.valuation;}
            if(target==0x10739890){(void)text(s,first);s.memory.write<4>(arg(6),123);s.memory.write<4>(arg(7),45);}
            if(target==0x10768810)result=output+0x800;
            if(target==0x1077b720){const auto b=text(s,second);s.memory.copy_in(first,b);s.memory.write<1>(first+std::uint32_t(b.size()),0);result=first;}
            if(target==0x10769b60){(void)text(s,first);(void)text(s,second);}
            for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xdd000000+n*0x101);
            for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-70.0-n));s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=stop;
            if(options.child_negative&&!original)s.memory.write<1>(globals+0x700,1);calls[serial].after=s.registers;if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture transfer-record dependency interruption");
        }catch(...){calls[serial].after=s.registers;throw;}
    }
};
std::unique_ptr<State> seed(unsigned n){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned b=1280;b<1312;++b)s->wr<1>(b,0);s->memory.map(fixture,0x40000);s->memory.map(stack-0x6000,0x7000);s->memory.map(toc-0x8000,0x9000);s->pc=records::entries[n];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=100000;s->wr<4>(12,offer);s->wr<4>(16,staff);s->wr<4>(20,clubs+0x248);s->wr<4>(24,2500);
    for(auto [slot,ptr]:{std::pair{-0x7be0,globals},std::pair{-0x7be8,globals+4},std::pair{-0x7b7c,globals+8},std::pair{-0x7b78,globals+12},std::pair{-0x7b90,globals+16},std::pair{-0x7b84,globals+20},std::pair{-0x7b88,globals+24},std::pair{-0x7b8c,pool},std::pair{-0x7ba8,output},std::pair{-0x7bac,globals+28}})s->memory.write<4>(toc+slot,ptr);
    s->memory.write<4>(globals,staff);s->memory.write<4>(globals+4,clubs);s->memory.write<4>(globals+8,globals+0x100);s->memory.write<4>(globals+16,0);s->memory.write<4>(globals+20,pool);s->memory.write<4>(globals+24,pool+8);s->memory.write<1>(globals+28,1);
    for(unsigned k=0;k<4;++k){s->memory.write<4>(staff+k*0x70,k);s->memory.write<4>(staff+k*0x70+8,10000+k*1000);s->memory.write<4>(staff+k*0x70+0x14,profile);s->memory.write<4>(clubs+k*0x248,k);s->memory.write<4>(clubs+k*0x248+0x30,17);s->memory.write<4>(prior+k*4,prior_offer);s->memory.write<2>(pool+k*2,20);}
    s->memory.write<2>(profile+10,7001);s->memory.write<4>(contract+4,0);s->memory.write<4>(prior_offer,group);s->memory.copy_in(offer,std::vector<std::uint8_t>(0x54,0xab));s->memory.write<4>(offer,group);s->memory.write<4>(offer+4,exchange);s->memory.write<4>(offer+12,0);s->memory.write<4>(offer+0x20,1000);s->memory.write<4>(offer+0x28,3);s->memory.write<4>(offer+0x40,200);s->memory.write<4>(offer+0x48,300);s->memory.write<1>(offer+0x50,10);
    list(*s,group,nodes,{{0,0},{3,0}});list(*s,exchange,nodes+0x100,{{1,75},{2,125}});transfer_record_fixture_tables(*s,toc,tables);s->memory.copy_in(output+0x800,{102,105,120,116,117,114,101,0});
    if(n==2)s->wr<4>(16,1);
    if(n==3){s->wr<4>(16,0x1234);s->wr<4>(20,prior);}
    if(n==4)s->wr<4>(16,0x1234);
    if(n==7){s->wr<4>(16,0);s->wr<4>(20,0);s->wr<4>(24,0);s->wr<4>(28,1);}
    return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;std::vector<std::uint8_t> written;};
Result compare(const State& input,Options options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;Trace ot,nt;Model old(true,options,ot),modern(false,options,nt);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&ot;m.reference(s);}else modern_root(s,&m,&nt);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;const auto label="Transfer record root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture transfer-record dependency interruption"||expected.starts_with("Unmapped data access at "),label+" fixture assertion: "+expected);
    if(ot.sites!=nt.sites){for(unsigned k=0;k<std::min(ot.sites.size(),nt.sites.size());++k)if(ot.sites[k]!=nt.sites[k]){std::cerr<<"First transfer-record site "<<k<<" "<<hex_address(ot.sites[k])<<"/"<<hex_address(nt.sites[k])<<std::endl;break;}}
    require(expected==actual,label+" original="+expected+" modern="+actual);if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" effects");require(old.position==modern.position&&old.written==modern.written,label+" fixture IO stream");if(old.calls!=modern.calls){for(unsigned n=0;n<std::min(old.calls.size(),modern.calls.size());++n)if(!(old.calls[n]==modern.calls[n])){const auto& xcall=old.calls[n];const auto& ycall=modern.calls[n];std::cerr<<"Call mismatch "<<n<<" target="<<hex_address(xcall.target)<<"/"<<hex_address(ycall.target)<<" stop="<<hex_address(xcall.stop)<<"/"<<hex_address(ycall.stop)<<std::endl;for(unsigned b=0;b<xcall.before.size();++b)if(xcall.before[b]!=ycall.before[b]){std::cerr<<"before byte="<<b<<" "<<unsigned(xcall.before[b])<<"/"<<unsigned(ycall.before[b])<<std::endl;break;}for(unsigned b=0;b<xcall.after.size();++b)if(xcall.after[b]!=ycall.after[b]){std::cerr<<"after byte="<<b<<" "<<unsigned(xcall.after[b])<<"/"<<unsigned(ycall.after[b])<<std::endl;break;}break;}}require(old.calls==modern.calls,label+" dependency ABI");require(ot.sites==nt.sites,label+" source order");require(a->executed==b->executed,label+" accounting");}
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;structural+=options.structural;if(expected.empty()&&!rejected)sites.insert(ot.sites.begin(),ot.sites.end());return{std::move(a),x.log,old.calls,old.written};
}
void boundaries(const State& input,Options options={}){
    auto base=compare(input,options);require(base.state->pc==return_sentinel,"Completed transfer-record boundary fixture root="+hex_address(input.pc)+" stop="+hex_address(base.state->pc));const auto steps=base.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(auto a:base.accesses)if(a.size){bytes.insert(a.address);bytes.insert(a.address+std::uint32_t(a.size)-1);}std::set<std::uint32_t> holes;if(!bytes.empty())for(unsigned n=0;n<6;++n){auto i=bytes.begin();std::advance(i,std::ptrdiff_t(n*(bytes.size()-1)/5));holes.insert(*i);}
    for(auto hole:holes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->budget=input.budget;s->executed=input.executed;for(auto [start,size]:input.memory.mapped_ranges()){if(hole<start||std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}for(auto [a,n]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})if(n){s->memory.map(a,n);s->memory.copy_in(a,input.memory.copy_out(a,n));}}compare(*s,options);}
    for(unsigned n=1;n<=base.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}

void lifecycle_cases(){
    for(auto flag:{0u,1u,2u,0x7fffu,0x8000u,0xffffu,0x10000u,0xffff0001u,0xffffffffu})for(auto count:{0u,1u,2u,0x7fffu,0x8000u,0xffffu}){
        auto s=seed(2);s->wr<4>(16,flag);s->memory.write<2>(group,count);boundaries(*s);
    }
    for(auto primary:{0u,group})for(auto payment:{0u,exchange}){auto s=seed(2);s->memory.write<4>(offer,primary);s->memory.write<4>(offer+4,payment);boundaries(*s);}
    {auto s=seed(2);s->wr<4>(12,0);boundaries(*s);}
    for(auto n:{0u,1u}){auto s=seed(n);s->wr<4>(12,0xf0000000);compare(*s);}
    {auto s=seed(0);const auto before=s->memory.copy_out(offer,0x54);const auto r=compare(*s);require(r.state->r<4>(12)==offer&&r.state->memory.read<4>(offer)==0&&r.state->memory.read<4>(offer+4)==0,"Independent empty constructor");require(r.state->memory.copy_out(offer+0x20,0x34)==std::vector<std::uint8_t>(before.begin()+0x20,before.end()),"Constructor leaves uninitialized fields untouched");}
    {auto s=seed(1);const auto r=compare(*s);require(r.state->r<4>(12)==offer&&r.state->memory.read<4>(offer+0x20)==5000,"Independent delegating constructor");}
}
void initialization_cases(){
    for(auto n:{5u,6u})for(auto person:{0u,staff})for(auto buyer:{0u,clubs+0x248}){auto s=seed(n);s->wr<4>(16,person);s->wr<4>(20,buyer);boundaries(*s);}
    for(auto n:{5u,6u})for(auto alias:{staff,clubs+0x248}){auto s=seed(n);s->wr<4>(12,alias);boundaries(*s);}
    for(auto byte:{0u,1u,15u,16u,127u,128u,255u})for(auto n:{5u,6u}){auto s=seed(n);s->memory.write<1>(offer+0x38,byte);const auto r=compare(*s);require(r.state->memory.read<1>(offer+0x38)==(byte&15u),"Independent packed nibble initialization");}
    for(auto amount:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=seed(6);s->wr<4>(24,amount);const auto r=compare(*s);require(r.state->memory.read<4>(offer+0x20)==amount,"Independent explicit amount bits");}
    for(auto skill:{0u,6999u,7000u,7001u,32767u,32768u,65535u})for(auto value:{0u,1u,19u,20u,65535u})for(auto accumulated:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}){
        auto s=seed(6);s->memory.write<2>(profile+10,skill);s->memory.write<2>(pool+2,value);s->memory.write<4>(globals+16,accumulated);const auto r=compare(*s);const auto sum=std::bit_cast<std::int32_t>(accumulated+value);require(r.state->memory.read<1>(offer+0x35)==unsigned(sum%20==0&&std::int16_t(skill)>7000),"Independent source-width RNG and skill predicate");
    }
    for(auto value:{0u,1u,0xffffu,0x10000u,0x80000014u,0xffffffffu}){auto s=seed(6);s->memory.write<4>(globals+20,pool+8);Options o;o.seed=value;boundaries(*s,o);}
    {auto s=seed(6);s->memory.write<4>(clubs+0x248+0x30,18);boundaries(*s);}
    {auto s=seed(6);Options o;o.primary_results={0};boundaries(*s,o);}
    {auto s=seed(6);Options o;o.primary_results={contract,0};compare(*s,o);}
    {auto s=seed(6);s->memory.map(0x30,4);s->memory.write<4>(0x30,18);Options o;o.primary_results={contract,0};o.structural=true;boundaries(*s,o);}
}
void value_cases(){
    for(unsigned flag=0;flag<256;++flag)for(auto which:{4u,5u,6u,7u}){auto s=seed(7);s->wr<4>(which*4,flag);compare(*s);}
    for(auto high:{0x100u,0x101u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto which:{4u,5u,6u,7u}){auto s=seed(7);s->wr<4>(which*4,high);boundaries(*s);}
    for(unsigned percentage=0;percentage<256;++percentage){auto s=seed(7);s->memory.write<1>(offer+0x50,percentage);const auto r=compare(*s);const auto percent=std::int8_t(percentage);const std::uint32_t expected=1000+200+300+11000+12000+(percent>0?10000u*unsigned(percent)/100:0);require(r.state->r<4>(12)==expected,"Independent signed percentage contribution");}
    for(auto base:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto first:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto second:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}){
        auto s=seed(7);s->memory.write<4>(offer+0x20,base);s->memory.write<4>(offer+0x40,first);s->memory.write<4>(offer+0x48,second);s->memory.write<1>(offer+0x50,0);s->wr<4>(20,1);const auto r=compare(*s);const auto expected=base+(std::int32_t(first)>0?first:0)+(std::int32_t(second)>0?second:0);require(r.state->r<4>(12)==expected,"Independent positive installments and wrapping value");
    }
    for(auto value:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto percent:{1u,50u,100u,127u}){auto s=seed(7);s->memory.write<4>(staff+8,value);s->memory.write<1>(offer+0x50,percent);const auto r=compare(*s);const auto product=std::uint32_t(value*percent);const auto contribution=std::bit_cast<std::int32_t>(product)/100;require(r.state->r<4>(12)==24500u+std::uint32_t(contribution),"Independent wrapped percentage product and signed truncation");}
    {auto s=seed(7);s->wr<4>(24,1);Options o;o.valuation=4321;const auto r=compare(*s,o);require(r.state->r<4>(12)==1500+1000+4321*2,"Independent alternate exchange valuation");unsigned calls=0;for(const auto& call:r.calls)if(call.target==0x10084be0){++calls;std::uint32_t person=0;for(unsigned n=0;n<4;++n)person=(person<<8)|call.before[16+n];require(person==staff,"Alternate mode retains the main target argument for each exchange item");}require(calls==2,"Two alternate value service calls");}
    {auto s=seed(7);Options o;o.replace_exchange_on_next=true;boundaries(*s,o);}
    {auto s=seed(7);s->memory.write<4>(offer+4,0);boundaries(*s);}
    {auto s=seed(7);list(*s,exchange,nodes+0x100,{});boundaries(*s);}
}
void persistence_cases(){
    for(auto primary_count:{-32768,-1,0,1,2,3})for(auto exchange_count:{-32768,-1,0,1,2,3}){
        auto s=seed(3);Options o;o.primary_count=primary_count;o.exchange_count=exchange_count;boundaries(*s,o);
    }
    {auto s=seed(3);Options o;const auto r=compare(*s,o);require(r.state->memory.read<4>(offer)==group,"Read restores an earlier shared family");require(r.state->memory.read<4>(offer+4)==allocated_lists+0x100,"Read creates a separate exchange list");}
    {auto s=seed(3);Options o;o.primary_count=0;o.exchange_count=0;const auto r=compare(*s,o);require(r.state->memory.read<4>(offer)==o.header_primary&&r.state->memory.read<4>(offer+4)==0,"Zero group count preserves raw header pointer; zero exchange count clears its pointer");}
    for(auto item:{3u,4u,0x7fffffffu}){auto s=seed(3);Options o;o.primary_items={item};boundaries(*s,o);}
    {auto s=seed(3);Options o;o.primary_items={0xffffffffu};compare(*s,o);}
    {auto s=seed(3);s->memory.write<4>(prior,0);boundaries(*s);}
    {auto s=seed(3);s->memory.write<4>(prior_offer,0);boundaries(*s);}
    {auto s=seed(3);s->wr<4>(20,0);boundaries(*s);}
    for(auto n:{3u,4u})for(auto shorted:{1u,2u,3u,4u,5u,6u,7u})for(auto result:{0u,2u,0xffffffffu})for(auto partial:{false,true}){auto s=seed(n);Options o;o.short_io=shorted;o.io_result=result;o.partial=partial;boundaries(*s,o);}
    for(auto n:{3u,4u})for(auto bad:{0xf0000000u}){auto s=seed(n);s->wr<4>(12,bad);compare(*s);}
    for(auto first:{0u,group})for(auto second:{0u,exchange}){auto s=seed(4);s->memory.write<4>(offer,first);s->memory.write<4>(offer+4,second);boundaries(*s);}
    {auto s=seed(4);const auto header=s->memory.copy_out(offer,0x54);const auto r=compare(*s);require(r.written.size()==112&&std::equal(header.begin(),header.end(),r.written.begin()),"Independent raw header and list payload byte count");}
    {auto s=seed(4);Options o;o.null_primary_after_header=true;boundaries(*s,o);}
    for(auto primary:{0,1}){auto s=seed(3);Options o;o.allocation_failure=true;o.primary_count=primary;compare(*s,o);}
    {auto s=seed(3);Options o;o.primary_count=32767;o.exchange_count=0;s->budget=1000;compare(*s,o);}
}
void stream_roundtrip_case(){
    auto source=seed(4);const auto first=compare(*source);auto expected=source->memory.copy_out(offer,0x54);
    const auto append=[&](unsigned width,std::uint32_t value){for(unsigned n=width;n;--n)expected.push_back(std::uint8_t(value>>((n-1)*8)));};
    append(2,2);append(4,0);append(4,3);append(2,2);append(4,1);append(4,75);append(4,2);append(4,125);require(first.written==expected,"Independent complete serialized record and list byte sequence");
    auto input=seed(3);Options o;o.input_bytes=first.written;const auto read=compare(*input,o);require(read.state->memory.read<4>(offer)==group,"Roundtrip restores shared ownership");
    const auto payment=std::uint32_t(read.state->memory.read<4>(offer+4));require(payment==allocated_lists+0x100&&read.state->memory.read<2>(payment)==2,"Roundtrip rebuilds exchange allocation");const auto first_node=std::uint32_t(read.state->memory.read<4>(payment+4));const auto second_node=std::uint32_t(read.state->memory.read<4>(first_node+16));require(read.state->memory.read<4>(first_node)==1&&read.state->memory.read<4>(first_node+4)==75&&read.state->memory.read<4>(second_node)==2&&read.state->memory.read<4>(second_node+4)==125,"Roundtrip preserves both exchange payload words");
    auto output=clone(*read.state);output->pc=records::write_offer_entry;output->wr<4>(12,offer);output->wr<4>(16,0x1234);const auto again=compare(*output);require(again.written.size()==first.written.size()&&std::equal(again.written.begin()+8,again.written.end(),first.written.begin()+8),"Roundtrip preserves nonpointer fields and serialized list payloads");
}
void relocation_cases(){
    for(auto n:{3u,4u,5u,6u,7u}){auto s=seed(n);constexpr std::uint32_t other_toc=0x18008000;s->memory.map(other_toc-0x8000,0x9000);s->memory.copy_in(other_toc-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,other_toc);transfer_record_fixture_tables(*s,other_toc,tables+0x1000);Options o;if(n==3||n==4)o.short_io=1;if(n==5||n==6)s->wr<4>(16,0);boundaries(*s,o);}
}

void cases_all(){
    for(unsigned n=0;n<records::entries.size();++n){auto s=seed(n);boundaries(*s);Options o;o.own_negative=true;compare(*s,o);o.own_negative=false;o.child_negative=true;if(n==5)s->wr<4>(16,0);compare(*s,o);}
}
}
namespace cm::aot {void transfer_record_step_seen(State&,std::uint32_t pc){if(original_trace)original_trace->sites.push_back(pc);}}
int main(){try{cases_all();lifecycle_cases();initialization_cases();value_cases();persistence_cases();stream_roundtrip_case();relocation_cases();
    const auto expected=transfer_record_expected_sites();for(auto pc:expected)if(!sites.contains(pc))std::cerr<<"Uncovered source site "<<hex_address(pc)<<std::endl;require(sites.size()==expected.size()&&std::all_of(expected.begin(),expected.end(),[](auto pc){return sites.contains(pc);}),"Every complete original transfer-record site required");
    for(unsigned n=0;n<records::entries.size();++n)std::cout<<"PASS root="<<hex_address(records::entries[n])<<std::endl;
    std::cout<<"Transfer record comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<"/806"<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
