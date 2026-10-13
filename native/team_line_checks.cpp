#include "team_line.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot { bool team_line_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace line=cm::aot::team_line;
constexpr auto match=fixture+0x1000, actor=fixture+0x11000;
constexpr std::array<std::uint32_t,5> ends{0x101d56f8,0x101d7d88,0x101e4ea0,0x101e4ecc,0x101e5064};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{line::tactical_flags,line::line_adjustment_enabled,line::actor_column_excess,line::column_excess,line::row_lane_clear};
struct Trace final:SemanticGuestObserver { std::vector<std::uint32_t> order; void before_step(State&,std::uint32_t pc)override{order.push_back(pc);} };
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Options {unsigned interrupt=0;bool negative=false,child_negative=false,mutate=false;};
struct Model final:SemanticDependencyDispatch {
    unsigned index;bool original;Options options;std::vector<Event> calls;
    Model(unsigned root,bool old,Options value):index(root),original(old),options(value){}
    void reference(State& s) {
        while(s.pc!=return_sentinel) {
            if(s.pc>=line::entries[index]&&s.pc<ends[index])require(team_line_reference(s),"Missing isolated team-line body");
            else call(s,s.pc,s.r<4>(4128));
        }
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        Event event{target,continuation,s.registers,{}};const auto arg=std::uint32_t(s.r<4>(12));std::uint32_t result=0;
        switch(target) {
        case 0x101d56f0:result=s.memory.read<4>(arg+0x588);break;
        case 0x101d5680:result=s.memory.read<1>(arg+0x106);break;
        case 0x101d5690:result=s.memory.read<1>(arg+0x105);break;
        case 0x101d8110:result=s.memory.read<1>(arg+0x19);break;
        case 0x10283690:result=s.memory.read<1>(arg+0x27);break;
        case 0x10289560:result=std::uint32_t(signed_value<2>(s.memory.read<2>(arg+0x2a)));break;
        default:throw std::runtime_error("Unknown team-line dependency "+hex_address(target));
        }
        if(options.child_negative&&!original)result^=target==line::entries[0]?0x400:1;
        if(options.mutate) {
            if(target==0x101d5680) {
                s.memory.write<1>(arg+0x106,result+1);s.memory.write<1>(match+0x7925,7);s.memory.write<1>(match+0x7926,4);
                s.memory.write<4>(match+0xe000,arg);
            }
            if(target==0x101d5690)s.memory.write<1>(arg+0x105,result+1);
        }
        for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xac000000+n*17);
        for(unsigned n=0;n<=13;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-11.0-n));
        s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;
        event.after=s.registers;calls.push_back(std::move(event));
        if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture team-line dependency interrupted");
    }
};
std::int32_t byte(std::uint32_t x){return std::int32_t(signed_value<1>(x));}
std::uint32_t slot(unsigned side,unsigned n){return match+0x311c+side*0x2350+n*0x1c4;}
std::unique_ptr<State> seed(unsigned index) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    s->memory.map(fixture,0x14000);s->memory.map(stack-0x1000,0x2000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,index==0?actor:match);s->wr<4>(16,index==2?actor:0);
    s->wr<4>(20,index==4?4:6);s->wr<4>(24,6);s->wr<4>(4128,return_sentinel);s->pc=line::entries[index];
    s->memory.write<4>(actor+0x588,0x12345678);s->memory.write<4>(match+0x7c6c+0x588,0x400);
    s->memory.write<1>(match+0x7925,7);s->memory.write<1>(match+0x7926,4);
    s->memory.write<1>(actor+0x106,6);s->memory.write<1>(actor+0x27,1);
    for(unsigned side=0;side<2;++side)for(unsigned n=0;n<20;++n)s->memory.write<1>(slot(side,n)+0x19,255);
    return s;
}
unsigned cases=0,returns=0,interruptions=0,negatives=0;std::set<std::uint32_t> sites;
std::array<unsigned,5> returned{};
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;};
Result compare(const State& input,unsigned index,Options options={}) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();
    a->memory.date_observer=&x;b->memory.date_observer=&y;Trace old_trace,new_trace;Model old(index,true,options),modern(index,false,options);
    const auto run=[&](State& s,Model& m,bool original){try{if(original){original_trace=&old_trace;m.reference(s);}else methods[index](s,&m,&new_trace);}
        catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}original_trace=nullptr;return std::string{};};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Team line "+hex_address(input.pc)+" case="+std::to_string(cases);
    require(!expected.starts_with("Unknown team-line dependency"),expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    if(options.negative)b->wr<4>(12,b->r<4>(12)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.negative&&!options.child_negative)throw;rejected=true;}
    require(rejected==(options.negative||options.child_negative),label+" discriminating negative");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");require(a->executed==b->executed,label+" accounting");require(old.calls==modern.calls,label+" dependency ABI");
        if(old_trace.order!=new_trace.order){for(unsigned n=0;n<std::min(old_trace.order.size(),new_trace.order.size());++n)if(old_trace.order[n]!=new_trace.order[n]){std::cerr<<"First site "<<hex_address(old_trace.order[n])<<" / "<<hex_address(new_trace.order[n])<<'\n';break;}}
        require(old_trace.order==new_trace.order,label+" original-site order");}
    ++cases;negatives+=rejected;interruptions+=!expected.empty();if(expected.empty()&&!rejected){++returns;++returned[index];}
    sites.insert(old_trace.order.begin(),old_trace.order.end());return {std::move(a),x.log,unsigned(old.calls.size())};
}
void boundaries(const State& input,unsigned index) {
    const auto baseline=compare(input,index);const auto steps=baseline.state->executed-input.executed;
    for(std::uint64_t n=0;n<=steps;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s,index);}
    std::set<std::uint32_t> bytes;for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(auto hole:bytes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;
        for(auto b:bytes)if(b!=hole){s->memory.map(b,1);s->memory.write<1>(b,input.memory.read<1>(b));}compare(*s,index);}
    for(unsigned n=1;n<=baseline.calls;++n){Options o;o.interrupt=n;compare(input,index,o);}
}
}
namespace cm::aot {void team_line_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try {
    for(unsigned i=0;i<5;++i){auto s=seed(i);compare(*s,i);boundaries(*s,i);Options o;o.negative=true;compare(*s,i,o);}
    for(auto bits:{0u,1u,0x400u,0xfffffbffu,0xffffffffu,0x80000000u})for(unsigned i=0;i<2;++i){
        auto s=seed(i);s->memory.write<4>(i?match+0x7c6c+0x588:actor+0x588,bits);
        require(compare(*s,i).state->r<4>(12)==(i?unsigned((bits&0x400)!=0):bits),"Independent tactical flags");}
    for(unsigned bit=0;bit<32;++bit){auto s=seed(1);s->memory.write<4>(match+0x7c6c+0x588,1u<<bit);compare(*s,1);}
    for(unsigned side=0;side<256;++side){auto s=seed(1);s->wr<4>(16,side|0xabcdef00);
        const auto at=match+std::uint32_t(byte(side))*0x18e0+0x7c6c+0x588;s->memory.map(at,4);s->memory.write<4>(at,side&1?0x400:0);
        require(compare(*s,1).state->r<4>(12)==(side&1),"Signed side selects tactical record");}
    {Options o;o.child_negative=true;compare(*seed(1),1,o);compare(*seed(2),2,o);}
    for(unsigned side=0;side<256;++side)for(auto col:{0u,1u,4u,7u,127u,128u,255u,0x1234fffeu}){
        auto s=seed(3);s->wr<4>(16,side|0x12340000);s->wr<4>(20,col);
        const auto expected=std::uint32_t(byte(side)==1?byte(4-col):byte(col-7));
        require(compare(*s,3).state->r<4>(12)==expected,"Independent signed column excess");}
    for(unsigned limit=0;limit<256;++limit)for(auto col:{0u,1u,127u,128u,255u})for(auto side:{0u,1u}){
        auto s=seed(3);s->wr<4>(16,side);s->wr<4>(20,col);s->memory.write<1>(match+(side?0x7926:0x7925),limit);compare(*s,3);}
    for(auto side:{0u,1u,2u,128u,255u})for(auto col:{0u,7u,127u,128u,255u})for(bool exempt:{false,true}){
        auto s=seed(2);s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x106,col);s->memory.write<4>(match+0xe000,exempt?actor:0);
        const auto result=compare(*s,2).state->r<4>(12);require(result==(exempt?0:std::uint32_t(side==1?byte(4-col):byte(col-7))),"Independent actor excess and designated exemption");
        Options o;o.mutate=true;compare(*s,2,o);}
    for(auto side:{0u,1u,2u,128u,255u})for(auto target_row:{0u,4u,127u,128u,255u})for(auto target_col:{0u,2u,3u,6u,8u,9u,127u,128u,255u}){
        auto s=seed(4);s->wr<4>(16,side);s->wr<4>(20,target_row);s->wr<4>(24,target_col);
        require(compare(*s,4).state->r<4>(12)==unsigned(side==0?byte(target_col)<=8:byte(target_col)>=3),"Independent empty lane domain");}
    for(auto side:{0u,1u,2u,128u,255u})for(auto n:{0u,10u,19u})for(auto active:{0u,127u,128u,255u})for(auto role:{0u,1u,0xffffu})for(auto r:{2u,3u,4u,5u,6u})for(auto c:{3u,5u,6u,7u,9u}){
        auto s=seed(4);s->wr<4>(16,side|0xabcdef00);auto p=slot(side==0?1:0,n);s->memory.write<1>(p+0x19,active);s->memory.write<2>(p+0x2a,role);s->memory.write<1>(p+0x105,r);s->memory.write<1>(p+0x106,c);
        const bool blocks=byte(active)>=0&&((r==4&&c==6)||(role!=1&&r==4&&((side==1&&6>c)||(side==0&&6<c))));
        require(compare(*s,4).state->r<4>(12)==unsigned(!blocks),"Independent active actor lane obstruction");}
    {auto s=seed(4);auto p=slot(1,0);s->memory.write<1>(p+0x19,0);s->memory.write<1>(p+0x105,4);s->memory.write<1>(p+0x106,7);boundaries(*s,4);Options o;o.mutate=true;compare(*s,4,o);o.mutate=false;o.child_negative=true;compare(*s,4,o);}
    for(unsigned i=0;i<5;++i)for(auto ptr:{0u,fixture+1,stack-0x60,stack+0x18}){auto s=seed(i);s->wr<4>(12,ptr);compare(*s,i);}
    for(unsigned i=0;i<5;++i){auto s=seed(i);s->wr<4>(8,0x87654321);compare(*s,i);s->wr<4>(4,stack+1);compare(*s,i);}
    for(auto i:{1u,2u,4u})for(auto ptr:{stack-0x40,stack+0x18,stack+0x1c}){auto s=seed(i);s->wr<4>(12,ptr);s->memory.map(ptr+0x7925,2);s->memory.map(ptr+0xe000,4);compare(*s,i);}
    if(sites.size()!=172)for(unsigned i=0;i<5;++i)for(auto pc=line::entries[i];pc<ends[i];pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered "<<hex_address(pc)<<'\n';
    require(sites.size()==172,"All original team-line sites covered");for(auto n:returned)require(n>0,"Each team-line entry returned");
    std::cout<<"PASS: "<<cases<<" team-line comparisons; "<<returns<<" returns, "<<interruptions<<" interruptions, "<<negatives<<" own/child negatives; all "<<sites.size()<<" original sites; complete state/effects/ABI/accounting\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
