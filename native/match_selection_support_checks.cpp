#include "match_selection_support.hpp"
#include "semantic_io_checks.hpp"
#include <fstream>
#include <set>
namespace cm::aot {bool match_selection_support_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace selection=cm::aot::match_selection_support;
constexpr auto match=fixture+0x10000,first=match+0x311c,profile=fixture+0x50000,constants=fixture+0x60000,doubles=constants+0x1000;
constexpr auto cursor=fixture+0x20,score_constants=constants+0x2000;
constexpr std::array<std::uint32_t,3> ends{0x101e1a44,0x101e1bc4,0x102876b8};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{selection::select_context_weighted_actor,selection::select_score_weighted_actor,selection::actor_selection_score};
struct Trace:SemanticGuestObserver{std::vector<std::uint32_t> order;void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget)order.push_back(pc);}};
thread_local Trace* original_trace=nullptr;
struct Event{std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Options{std::vector<std::uint32_t> draws{1};bool own_negative=false,child_negative=false,rng_negative=false,bound_dependent=false,changing_positions=false;unsigned interrupt=0;};
struct Model:SemanticDependencyDispatch{
    bool original;Options options;Trace* trace;std::vector<Event> calls;unsigned draws=0,child_mutations=0;
    Model(bool old,const Options& o,Trace* t):original(old),options(o),trace(t){}
    void reference(State& s,std::uint32_t until){
        auto it=std::find(selection::entries.begin(),selection::entries.end(),s.pc);require(it!=selection::entries.end(),"Independent selection starts at complete root");
        auto index=unsigned(it-selection::entries.begin());auto caller_stack=std::uint32_t(s.r<4>(4));
        bool first_call=true;
        while(s.pc!=until){if((!first_call&&std::find(selection::entries.begin(),selection::entries.end(),s.pc)!=selection::entries.end())||!match_selection_support_reference(s))call(s,s.pc,s.r<4>(4128));
            if(!trace->order.empty()&&trace->order.back()==ends[index]-4&&s.r<4>(4)==caller_stack)return;
            first_call=false;
        }
    }
    void call(State& s,std::uint32_t entry,std::uint32_t continuation)override{
        const auto n=calls.size();calls.push_back({entry,continuation,s.registers,{}});auto input=std::uint32_t(s.r<4>(12));std::uint32_t value=0;
        const auto own=std::find(selection::entries.begin(),selection::entries.end(),entry);
        if(own!=selection::entries.end()){
            if(original)reference(s,continuation);else methods[unsigned(own-selection::entries.begin())](s,this,trace);
            if(options.child_negative&&!original&&entry==selection::entries[2]){s.wr<4>(12,s.r<4>(12)^1);++child_mutations;}
        }else{
        switch(entry){
        case 0x101d3b50:value=s.memory.read<1>(input+0x19);if(options.changing_positions)s.memory.write<1>(input+0x19,value+1);break;
        case 0x10289560:value=std::uint32_t(signed_value<2>(s.memory.read<2>(input+0x1c2)));break;
        case 0x101d3c90:
            value=options.draws[draws++%options.draws.size()];s.memory.write<4>(cursor,draws);
            if(options.bound_dependent)value=input?input-1:0;
            if(options.rng_negative&&!original){value^=1;++child_mutations;}break;
        default:throw std::runtime_error("Unknown selection dependency "+hex_address(entry));
        }
        for(unsigned reg=0;reg<=12;++reg)if(reg!=1&&reg!=2)s.wr<4>(reg*4,0xac000000+reg*71);
        for(unsigned reg=0;reg<=13;++reg)s.wr<8>(0x4000+reg*16,float_bits<8>(-17.0-reg));
        s.wr<1>(2304,5);s.wr<4>(12,value);s.pc=continuation;}calls[n].after=s.registers;
        if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture selection dependency interrupted");
    }
};
std::unique_ptr<State> seed(unsigned index){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x100000);s->memory.map(stack-0x800,0x1000);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->pc=selection::entries[index];s->wr<4>(12,match);s->wr<4>(16,0);s->wr<4>(20,0);
    s->memory.write<4>(toc-0x52c8,constants);s->memory.write<4>(toc-0x52c4,doubles);
    for(unsigned offset=0;offset<0x200;offset+=4)s->memory.write<4>(constants+offset,float_bits<4>(1.0+double(offset)/256));
    s->memory.write<4>(constants+0xc,float_bits<4>(0));s->memory.write<4>(constants+8,float_bits<4>(20));s->memory.write<4>(constants+0x2c,float_bits<4>(20));
    s->memory.write<8>(doubles+0x60,UINT64_C(0x4330000080000000));s->memory.write<8>(doubles+0x1f8,float_bits<8>(0.01));s->memory.write<8>(doubles+0x118,float_bits<8>(0.25));
    s->wr<8>(0x4000+31*16,float_bits<8>(1.0));
    s->memory.write<4>(toc-0x51e8,score_constants);
    for(auto [offset,value]:std::vector<std::pair<unsigned,double>>{{0x18,2},{0x28,1},{0x30,1},{0xc0,0.5},{0x3a8,100}})s->memory.write<8>(score_constants+offset,float_bits<8>(value));
    s->memory.write<8>(score_constants+0x80,UINT64_C(0x4330000080000000));
    for(auto offset:{0xe0u,0x1c0u,0x1e0u,0x258u,0x260u})s->memory.write<8>(doubles+offset,float_bits<8>(0.5));
    for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){
        const auto actor=first+side*0x2350+slot*0x1c4,record=profile+(side*20+slot)*0x100;
        s->memory.write<1>(actor+0x19,slot%11);s->memory.write<1>(actor+0x27,side);s->memory.write<2>(actor+0x1c2,2);s->memory.write<4>(actor+0x70,record);s->memory.write<4>(actor+0x1a8,float_bits<4>(60));s->memory.write<1>(actor+0x102,10);s->memory.write<4>(actor+0x1ac,0);
        for(auto offset:{0x80u,0x90u,0x98u,0x9cu,0xa0u,0xa4u,0xacu,0xb0u,0xb4u,0xc8u,0xccu,0xdcu,0xf4u})s->memory.write<4>(actor+offset,float_bits<4>(5));
        s->memory.write<2>(record+4,80);for(unsigned off=0x10;off<0x50;++off)s->memory.write<1>(record+off,10);
    }
    if(index==2){s->wr<4>(12,first);s->wr<4>(16,0);}
    return s;
}
unsigned cases=0,returns=0,interruptions=0,negatives=0;std::array<unsigned,3> root_returns{};std::array<std::set<std::uint32_t>,3> complete_sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;};
Result compare(const State& input,unsigned index,const Options& options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace before,after;Model old(true,options,&before),modern(false,options,&after);
    const auto run=[&](State& s,Model& m){try{if(m.original){original_trace=m.trace;m.reference(s,return_sentinel);}else methods[index](s,&m,m.trace);}
        catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}original_trace=nullptr;return std::string{};};
    auto expected=run(*a,old),actual=run(*b,modern);a->memory.date_observer=b->memory.date_observer=nullptr;
    auto label="Selection root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected==actual,label+" original="+expected+" modern="+actual);require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture selection dependency interrupted"||expected.starts_with("Unmapped data access at "),label+" unexpected boundary="+expected);
    if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);
    if(options.child_negative||options.rng_negative)require(modern.child_mutations>0,label+" RNG child negative not exercised");
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative&&!options.rng_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative||options.rng_negative),label+" negative must discriminate");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered memory effects");require(a->executed==b->executed,label+" instruction count");require(old.calls==modern.calls,label+" dependency ABI");require(before.order==after.order,label+" source order");}
    ++cases;interruptions+=!expected.empty();negatives+=rejected;
    if(expected.empty()&&!rejected){++returns;++root_returns[index];for(auto pc:before.order)if(selection::entries[index]<=pc&&pc<ends[index])complete_sites[index].insert(pc);}
    return{std::move(a),x.log,unsigned(old.calls.size())};
}
void actor_words(State& s,unsigned offset,unsigned bytes,std::uint64_t value){for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){auto address=first+side*0x2350+slot*0x1c4+offset;if(bytes==1)s.memory.write<1>(address,value);else if(bytes==2)s.memory.write<2>(address,value);else s.memory.write<4>(address,value);}}
void profile_words(State& s,unsigned offset,unsigned bytes,std::uint64_t value){for(unsigned slot=0;slot<40;++slot){auto address=profile+slot*0x100+offset;if(bytes==1)s.memory.write<1>(address,value);else s.memory.write<2>(address,value);}}
void boundaries(unsigned index){
    auto input=seed(index);if(index<2){actor_words(*input,0x19,1,255);input->memory.write<1>(first+0x19,0);}
    const auto result=compare(*input,index);const auto steps=result.state->executed-input->executed;
    for(std::uint64_t n=0;n<=steps;++n){auto s=clone(*input);s->budget=s->executed+n;compare(*s,index);}
    std::set<std::uint32_t> bytes;for(const auto& access:result.accesses)for(unsigned n=0;n<access.size;++n)bytes.insert(access.address+n);
    for(auto hole:bytes){auto s=std::make_unique<State>();s->registers=input->registers;s->unique=input->unique;s->pc=input->pc;for(auto byte:bytes)if(byte!=hole){s->memory.map(byte,1);s->memory.write<1>(byte,input->memory.read<1>(byte));}compare(*s,index);}
    for(unsigned n=1;n<=result.calls;++n){Options o;o.interrupt=n;compare(*input,index,o);}
}
}
namespace cm::aot{void match_selection_support_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{
    require(argc==2,"Expected selection-support coverage output");
    for(unsigned index=0;index<3;++index){
        auto s=seed(index);auto result=compare(*s,index);require(result.state->r<4>(12)==(index<2?0:18),"Independent tied first winner or ordinary score");
        boundaries(index);Options own;own.own_negative=true;compare(*s,index,own);
        for(auto side:{0u,1u,0x100u,0xabcdef01u,127u,128u,255u}){auto input=seed(index);input->wr<4>(16,side);compare(*input,index);}
        auto moved=seed(index);moved->memory.map(fixture+0x110000,0x9000);moved->memory.copy_in(fixture+0x110000,moved->memory.copy_out(toc-0x8000,0x9000));moved->wr<4>(8,fixture+0x118000);compare(*moved,index);
    }
    Options rng;rng.rng_negative=true;compare(*seed(0),0,rng);Options child;child.child_negative=true;child.bound_dependent=true;compare(*seed(1),1,child);
    for(unsigned index=0;index<2;++index){
        for(auto role:{0u,1u,2u,4u,8u,16u,32u,64u,0x140u,0x240u,0x440u,0x7fffu,0x8000u,0xffffu}){auto s=seed(index);actor_words(*s,0x1c2,2,role);compare(*s,index);}
        for(auto position:{0u,127u,128u,255u}){auto s=seed(index);actor_words(*s,0x19,1,position);auto v=compare(*s,index);if(position>=128)require(v.state->r<4>(12)==UINT32_MAX,"No eligible actors returns minus one");}
        for(auto mode:{0u,1u,2u,0x100u,0x102u,128u,255u,0xabcdef02u}){auto s=seed(index);s->wr<4>(20,mode);compare(*s,index);}
        for(auto value:{0u,1u,2u,6u,24u,0x7fffffffu,0x80000000u,0xffffffffu}){Options o;o.draws={value};compare(*seed(index),index,o);}
        for(auto pattern:std::vector<std::vector<std::uint32_t>>{{1,2,3,4},{24,0},{0,24}}){Options o;o.draws=pattern;compare(*seed(index),index,o);}
        for(auto bits:{0u,0x80000000u,0x7f800000u,0xff800000u,0x7fc00123u}){auto s=seed(index);for(unsigned off=0;off<0x200;off+=4)s->memory.write<4>(constants+off,bits);compare(*s,index);}
        for(unsigned slot=0;slot<20;++slot){auto s=seed(index);actor_words(*s,0x19,1,255);s->memory.write<1>(first+slot*0x1c4+0x19,0);compare(*s,index);}
    }
    auto absent=seed(0);actor_words(*absent,0x70,4,0);compare(*absent,0);
    for(auto mode:{0u,1u,0x100u,0xffffffffu})for(auto profile_halfword:{0u,59u,60u,80u,140u,141u,0x7fffu,0x8000u,0xffffu})for(auto draw:{0u,1u,24u}){
        auto s=seed(0);s->wr<4>(20,mode);profile_words(*s,4,2,profile_halfword);Options o;o.draws={draw};compare(*s,0,o);
    }
    for(auto mode:{0u,1u,2u})for(auto level:{0u,1u,10u,127u,128u,255u})for(auto bits:{0u,0x42700000u,0xc2700000u,0x7f800000u,0xff800000u,0x7fc00123u}){
        auto s=seed(2);s->wr<4>(16,mode==0?0:0x1234);s->memory.write<4>(first+0x1ac,mode==1?0x1234:0x4321);s->memory.write<1>(first+0x102,level);s->memory.write<4>(first+0x1a8,bits);compare(*s,2);
    }
    auto matching=seed(2);matching->wr<4>(16,0x1234);matching->memory.write<4>(first+0x1ac,0x1234);require(compare(*matching,2).state->r<4>(12)==12,"Independent matching-marker score");
    for(auto offset:{0x18u,0x28u,0x30u,0x80u,0xc0u,0x3a8u})for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000000000123)}){
        auto s=seed(2);s->memory.write<8>(score_constants+offset,bits);compare(*s,2);s->wr<4>(16,1);s->memory.write<4>(first+0x1ac,1);compare(*s,2);
    }
    for(auto score_base:{stack-0x90,stack-0x88,first+0x80}){
        auto s=seed(2);s->memory.copy_in(score_base,s->memory.copy_out(score_constants,0x400));s->memory.write<4>(toc-0x51e8,score_base);compare(*s,2);s->wr<4>(16,1);s->memory.write<4>(first+0x1ac,1);compare(*s,2);
    }
    std::ofstream coverage(argv[1]);unsigned total=0;constexpr std::array<unsigned,3> expected{257,93,50};
    for(unsigned index=0;index<3;++index){for(auto pc:complete_sites[index])coverage<<index<<" "<<hex_address(pc)<<'\n';require(root_returns[index]>0&&complete_sites[index].size()==expected[index],"Complete selection-support sites root="+hex_address(selection::entries[index])+" sites="+std::to_string(complete_sites[index].size()));total+=complete_sites[index].size();std::cout<<"Root "<<hex_address(selection::entries[index])<<" complete_sites="<<complete_sites[index].size()<<" returns="<<root_returns[index]<<'\n';}
    std::cout<<"Match selection support comparisons="<<cases<<" returns="<<returns<<" interruptions="<<interruptions<<" negatives="<<negatives<<" completed_sites="<<total<<"/400\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
