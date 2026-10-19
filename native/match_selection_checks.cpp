#include "match_selection.hpp"
#include "semantic_io_checks.hpp"
#include <fstream>
#include <set>
namespace cm::aot {bool match_selection_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace selection=cm::aot::match_selection;
constexpr auto match=fixture+0x10000,first=match+0x311c,profile=fixture+0x50000,constants=fixture+0x60000,doubles=constants+0x1000;
constexpr auto row_output=fixture+0x8000,column_output=row_output+4,table0=fixture+0x70000,table1=fixture+0x74000,cursor=fixture+0x20;
constexpr std::array<std::uint32_t,4> ends{0x101e20f4,0x101e22d8,0x101e2598,0x101e28a8};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{selection::select_attribute_weighted_actor,selection::select_role_weighted_actor,selection::select_profile_weighted_actor,selection::preset_target_cell};
struct Trace:SemanticGuestObserver{std::vector<std::uint32_t> order;void before_step(State& s,std::uint32_t pc)override{if(s.executed<s.budget)order.push_back(pc);}};
thread_local Trace* original_trace=nullptr;
struct Event{std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Options{std::vector<std::uint32_t> draws{1};bool own_negative=false,child_negative=false,changing_positions=false;unsigned interrupt=0;};
struct Model:SemanticDependencyDispatch{
    bool original;Options options;Trace* trace;std::vector<Event> calls;unsigned draws=0,child_mutations=0;
    Model(bool old,const Options& o,Trace* t):original(old),options(o),trace(t){}
    void reference(State& s,std::uint32_t until){
        auto it=std::find(selection::entries.begin(),selection::entries.end(),s.pc);require(it!=selection::entries.end(),"Independent selection starts at complete root");
        auto index=unsigned(it-selection::entries.begin());auto caller_stack=std::uint32_t(s.r<4>(4));
        while(s.pc!=until){if(!match_selection_reference(s))call(s,s.pc,s.r<4>(4128));
            if(!trace->order.empty()&&trace->order.back()==ends[index]-4&&s.r<4>(4)==caller_stack)return;
        }
    }
    void call(State& s,std::uint32_t entry,std::uint32_t continuation)override{
        const auto n=calls.size();calls.push_back({entry,continuation,s.registers,{}});auto input=std::uint32_t(s.r<4>(12));std::uint32_t value=0;
        switch(entry){
        case 0x101d3b50:value=s.memory.read<1>(input+0x19);if(options.changing_positions)s.memory.write<1>(input+0x19,value+1);break;
        case 0x10289560:value=std::uint32_t(signed_value<2>(s.memory.read<2>(input+0x1c2)));break;
        case 0x101d3c90:
            value=options.draws[draws++%options.draws.size()];s.memory.write<4>(cursor,draws);
            if(options.child_negative&&!original){value^=1;++child_mutations;}break;
        default:throw std::runtime_error("Unknown selection dependency "+hex_address(entry));
        }
        for(unsigned reg=0;reg<=12;++reg)if(reg!=1&&reg!=2)s.wr<4>(reg*4,0xac000000+reg*71);
        for(unsigned reg=0;reg<=13;++reg)s.wr<8>(0x4000+reg*16,float_bits<8>(-17.0-reg));
        s.wr<1>(2304,5);s.wr<4>(12,value);s.pc=continuation;calls[n].after=s.registers;
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
    for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){
        const auto actor=first+side*0x2350+slot*0x1c4,record=profile+(side*20+slot)*0x100;
        s->memory.write<1>(actor+0x19,slot%11);s->memory.write<1>(actor+0x27,side);s->memory.write<2>(actor+0x1c2,2);s->memory.write<4>(actor+0x70,record);
        for(auto offset:{0x90u,0x9cu,0xa4u,0xacu,0xb0u,0xc8u,0xf4u})s->memory.write<4>(actor+offset,float_bits<4>(5));
        s->memory.write<2>(record+4,80);for(unsigned off=0x10;off<0x50;++off)s->memory.write<1>(record+off,10);
    }
    for(int side=-128;side<128;++side)s->memory.write<4>(match+0xdfec+side*4,side==0?table0:table1);
    s->memory.copy_in(table0,std::vector<std::uint8_t>(0x4000,0x53));s->memory.copy_in(table1,std::vector<std::uint8_t>(0x4000,0x62));
    s->memory.write<1>(match+0x7924,0);
    if(index==3){s->wr<4>(16,first);s->wr<4>(20,row_output);s->wr<4>(24,column_output);}
    return s;
}
unsigned cases=0,returns=0,interruptions=0,negatives=0;std::array<unsigned,4> root_returns{};std::array<std::set<std::uint32_t>,4> complete_sites;
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;};
Result compare(const State& input,unsigned index,const Options& options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace before,after;Model old(true,options,&before),modern(false,options,&after);
    const auto run=[&](State& s,Model& m){try{if(m.original){original_trace=m.trace;m.reference(s,return_sentinel);}else methods[index](s,&m,m.trace);}
        catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}original_trace=nullptr;return std::string{};};
    auto expected=run(*a,old),actual=run(*b,modern);a->memory.date_observer=b->memory.date_observer=nullptr;
    auto label="Selection root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected==actual,label+" original="+expected+" modern="+actual);require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture selection dependency interrupted"||expected.starts_with("Unmapped data access at "),label+" unexpected boundary="+expected);
    if(options.own_negative){if(index==3)b->memory.write<1>(std::uint32_t(input.r<4>(20)),b->memory.read<1>(input.r<4>(20))^1);else b->wr<4>(12,b->r<4>(12)^1);}
    if(options.child_negative)require(modern.child_mutations>0,label+" RNG child negative not exercised");
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" negative must discriminate");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered memory effects");require(a->executed==b->executed,label+" instruction count");require(old.calls==modern.calls,label+" dependency ABI");require(before.order==after.order,label+" source order");}
    ++cases;interruptions+=!expected.empty();negatives+=rejected;
    if(expected.empty()&&!rejected){++returns;++root_returns[index];for(auto pc:before.order)if(selection::entries[index]<=pc&&pc<ends[index])complete_sites[index].insert(pc);}
    return{std::move(a),x.log,unsigned(old.calls.size())};
}
void actor_words(State& s,unsigned offset,unsigned bytes,std::uint64_t value){for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){auto address=first+side*0x2350+slot*0x1c4+offset;if(bytes==1)s.memory.write<1>(address,value);else if(bytes==2)s.memory.write<2>(address,value);else s.memory.write<4>(address,value);}}
void profile_words(State& s,unsigned offset,unsigned bytes,std::uint64_t value){for(unsigned slot=0;slot<40;++slot){auto address=profile+slot*0x100+offset;if(bytes==1)s.memory.write<1>(address,value);else s.memory.write<2>(address,value);}}
void boundaries(unsigned index){
    auto input=seed(index);if(index<3){actor_words(*input,0x19,1,255);input->memory.write<1>(first+0x19,0);}
    const auto result=compare(*input,index);const auto steps=result.state->executed-input->executed;
    for(std::uint64_t n=0;n<=steps;++n){auto s=clone(*input);s->budget=s->executed+n;compare(*s,index);}
    std::set<std::uint32_t> bytes;for(const auto& access:result.accesses)for(unsigned n=0;n<access.size;++n)bytes.insert(access.address+n);
    for(auto hole:bytes){auto s=std::make_unique<State>();s->registers=input->registers;s->unique=input->unique;s->pc=input->pc;for(auto byte:bytes)if(byte!=hole){s->memory.map(byte,1);s->memory.write<1>(byte,input->memory.read<1>(byte));}compare(*s,index);}
    for(unsigned n=1;n<=result.calls;++n){Options o;o.interrupt=n;compare(*input,index,o);}
}
}
namespace cm::aot{void match_selection_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{
    require(argc==2,"Expected selection coverage output");
    for(unsigned index=0;index<4;++index){
        auto s=seed(index);auto result=compare(*s,index);if(index<3)require(result.state->r<4>(12)==0,"Independent first actor wins equal scores");else require(result.state->memory.read<1>(row_output)==3&&result.state->memory.read<1>(column_output)==5,"Independent packed target");
        boundaries(index);Options own;own.own_negative=true;compare(*s,index,own);if(index<3){Options child;child.child_negative=true;compare(*s,index,child);}
        for(auto side:{0u,1u,0x100u,0xabcdef01u,127u,128u,255u}){auto input=seed(index);if(index<3)input->wr<4>(16,side);else input->memory.write<1>(first+0x27,side);compare(*input,index);}
        auto moved=seed(index);moved->memory.map(fixture+0x110000,0x9000);moved->memory.copy_in(fixture+0x110000,moved->memory.copy_out(toc-0x8000,0x9000));moved->wr<4>(8,fixture+0x118000);compare(*moved,index);
    }
    for(unsigned index=0;index<3;++index){
        for(auto role:{0u,1u,2u,4u,8u,0x88u,0x808u,16u,0x90u,0x810u,32u,64u,0x140u,0x240u,0x440u,0x7fffu,0x8000u,0xffffu}){auto s=seed(index);actor_words(*s,0x1c2,2,role);compare(*s,index);}
        for(auto position:{0u,127u,128u,255u}){auto s=seed(index);actor_words(*s,0x19,1,position);auto v=compare(*s,index);if(position>=128)require(v.state->r<4>(12)==UINT32_MAX,"No eligible actors returns minus one");}
        for(auto value:{0u,1u,24u,0x7fffffffu,0x80000000u,0xffffffffu}){Options o;o.draws={value};compare(*seed(index),index,o);}
        for(auto pattern:std::vector<std::vector<std::uint32_t>>{{1,2,3,4},{24,0},{0,24}}){Options o;o.draws=pattern;compare(*seed(index),index,o);}
        auto absent=seed(index);actor_words(*absent,0x70,4,0);compare(*absent,index);
        for(auto bits:{0u,0x80000000u,0x7f800000u,0xff800000u,0x7fc00123u}){auto s=seed(index);for(unsigned off=0;off<0x200;off+=4)s->memory.write<4>(constants+off,bits);compare(*s,index);}
        for(unsigned slot=0;slot<20;++slot){auto s=seed(index);actor_words(*s,0x19,1,255);s->memory.write<1>(first+slot*0x1c4+0x19,0);compare(*s,index);}
    }
    for(auto mode:{0u,1u,0x100u,0xffffffffu})for(auto profile_halfword:{0u,59u,60u,80u,140u,141u,0x7fffu,0x8000u,0xffffu})for(auto attribute:{0u,5u,6u,9u,10u,15u,16u,127u,128u,255u}){
        auto s=seed(0);s->wr<4>(20,mode);profile_words(*s,4,2,profile_halfword);profile_words(*s,0x37,1,attribute);profile_words(*s,0x43,1,attribute);compare(*s,0);
    }
    for(auto sum:{0u,24u,25u,39u,40u,60u,61u,75u,76u,127u,128u,255u}){auto s=seed(0);for(auto off:{0x40u,0x26u,0x36u})profile_words(*s,off,1,0);profile_words(*s,0x1b,1,sum);compare(*s,0);}
    for(auto a:{0u,4u,5u,10u,11u,15u,16u,127u,128u})for(auto b:{0u,5u,15u,16u})for(auto c:{0u,5u,15u,16u}){auto s=seed(2);profile_words(*s,0x44,1,a);profile_words(*s,0x39,1,b);profile_words(*s,0x3e,1,c);compare(*s,2);}
    for(auto owner:{0u,1u,2u,128u,255u})for(auto side:{0u,1u})for(auto packed:{0u,0x53u,0x88u,0xffu}){auto s=seed(3);s->memory.write<1>(match+0x7924,owner);s->memory.write<1>(first+0x27,side);s->memory.copy_in(side?table1:table0,std::vector<std::uint8_t>(0x4000,std::uint8_t(packed)));compare(*s,3);}
    for(auto alias:{row_output,first+0x27,first+0x19,match+0x7924,match+0xdfec,stack-0x40,stack+8}){auto s=seed(3);s->wr<4>(20,alias);s->wr<4>(24,alias);compare(*s,3);}
    Options change;change.changing_positions=true;compare(*seed(3),3,change);
    std::ofstream coverage(argv[1]);unsigned total=0;constexpr std::array<unsigned,4> expected{329,118,174,194};
    for(unsigned index=0;index<4;++index){for(auto pc:complete_sites[index])coverage<<index<<" "<<hex_address(pc)<<'\n';require(root_returns[index]>0&&complete_sites[index].size()==expected[index],"Complete selection source sites root="+hex_address(selection::entries[index])+" sites="+std::to_string(complete_sites[index].size()));total+=complete_sites[index].size();std::cout<<"Root "<<hex_address(selection::entries[index])<<" complete_sites="<<complete_sites[index].size()<<" returns="<<root_returns[index]<<'\n';}
    std::cout<<"Match selection comparisons="<<cases<<" returns="<<returns<<" interruptions="<<interruptions<<" negatives="<<negatives<<" completed_sites="<<total<<"/815\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
