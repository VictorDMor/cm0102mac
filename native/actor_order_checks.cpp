#include "actor_order.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot { bool actor_order_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace order = cm::aot::actor_order;
constexpr auto match=fixture+0x1000, first=match+0x311c, second=first+0x1c4;
constexpr auto slots=fixture+0x20000, constants=fixture+0x30000, alternate=fixture+0x40000;
using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
constexpr Function methods[]{order::secondary_rating,order::primary_rating,order::build_queue,order::compare_actors,order::current_fitness};
struct Trace final:SemanticGuestObserver {
    std::vector<std::uint32_t> order;
    void before_step(State&,std::uint32_t pc)override{order.push_back(pc);}
};
thread_local Trace* original_trace=nullptr;
struct Event {
    std::uint32_t target,continuation;decltype(State::registers) before,after;
    bool operator==(const Event&)const=default;
};
struct Options {
    bool own_negative=false,child_negative=false,reverse_sort=false,change_count=false,change_fitness=false,retarget_priority=false;
    unsigned interrupt=0;
    std::vector<std::uint32_t> sides{0,1};
};
struct Model final:SemanticDependencyDispatch {
    unsigned index;bool original;Options options;std::vector<Event> calls;unsigned draws=0,positions=0;
    Model(unsigned entry,bool old,const Options& value):index(entry),original(old),options(value){}
    void reference(State& s){
        constexpr std::array<std::uint32_t,5> ends{0x101d5678,0x101d56b8,0x101d91a4,0x101d939c,0x101d93a8};
        while(s.pc!=return_sentinel){
            if(s.pc>=order::entries[index]&&s.pc<ends[index])require(actor_order_reference(s),"Missing isolated actor-order body");
            else call(s,s.pc,s.r<4>(4128));
        }
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override{
        Event event{target,continuation,s.registers,{}};
        const auto actor=std::uint32_t(s.r<4>(12));std::uint32_t result=0;std::uint64_t floating=0;
        switch(target){
        case 0x101d3c90:
            require(actor==2,"Side sampler upper bound");result=options.sides[draws++%options.sides.size()];
            if(options.child_negative&&!original)result^=1;
            break;
        case 0x101d3b50:
            result=s.memory.read<1>(actor+0x19);
            if(options.change_count&&positions++==0)s.memory.write<1>(match+0x792e,127);
            break;
        case 0x10768300:{
            const auto count=std::uint32_t(s.r<4>(16));require(s.r<4>(20)==4,"Queue pointer element width");
            if(options.reverse_sort)for(unsigned i=0;i<count/2;++i){
                const auto a=actor+4*i,b=actor+4*(count-i-1);
                const auto x=s.memory.read<4>(a),y=s.memory.read<4>(b);
                s.memory.write<4>(a,y);s.memory.write<4>(b,x);
            }
            break;
        }
        case 0x101d5670:case 0x101d56b0:
            result=s.memory.read<1>(actor+(target==0x101d5670?0x108:0x107));
            if(options.child_negative&&!original)result+=20;
            if(options.change_fitness)s.memory.write<4>(actor+0x50,float_bits<4>(2.25));
            break;
        case 0x101d93a0:floating=float_bits<8>(float_value<4>(s.memory.read<4>(actor+0x50)));break;
        default:throw std::runtime_error("Unknown actor-order dependency "+hex_address(target));
        }
        for(unsigned reg=0;reg<=12;++reg)if(reg!=1&&reg!=2)s.wr<4>(reg*4,0xab000000+reg*73);
        for(unsigned reg=0;reg<=13;++reg)s.wr<8>(0x4000+reg*16,float_bits<8>(-19.0-reg));
        s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);
        if(target==0x101d93a0)s.wr<8>(0x4010,floating);
        s.pc=continuation;event.after=s.registers;calls.push_back(std::move(event));
        if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture actor-order dependency interrupted");
    }
};
std::unique_ptr<State> seed(unsigned index){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    for(unsigned i=1280;i<1312;++i)s->wr<1>(i,0);
    s->memory.map(fixture,0x60000);s->memory.map(stack-0x800,0x1000);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,index==2?match:index==3?slots:first);s->wr<4>(16,slots+4);
    s->wr<4>(4128,return_sentinel);s->pc=order::entries[index];
    s->memory.write<4>(toc-0x52c4,constants);s->memory.write<4>(toc-0x3910,match);s->memory.write<4>(toc-0x6420,fixture+0x50000);
    s->memory.write<4>(slots,first);s->memory.write<4>(slots+4,second);
    s->memory.write<8>(constants+0x60,UINT64_C(0x4330000080000000));
    s->memory.write<8>(constants+0x90,float_bits<8>(1.25));s->memory.write<8>(constants+0xf0,float_bits<8>(50.0));
    for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){
        const auto actor=first+side*0x2350+slot*0x1c4;
        s->memory.write<1>(actor+0x19,slot<11?slot:255);s->memory.write<1>(actor+0x107,7+slot*2);s->memory.write<1>(actor+0x108,11+slot*2);
        s->memory.write<4>(actor+0x50,float_bits<4>(0.5+slot*0.25));s->memory.write<2>(actor+0x3c,10+slot*10);
    }
    return s;
}
unsigned cases=0,returns=0,interruptions=0,negatives=0;std::set<std::uint32_t> sites;
std::array<unsigned,5> returned_entries{};
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;};
Result compare(const State& input,unsigned index,const Options& options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();
    if(options.retarget_priority)x.on_read=y.on_read=[](State& s,Effects& effects,std::uint32_t address,unsigned){
        if(!effects.mutated&&address==match+0x7928){effects.mutated=true;s.memory.write<4>(toc-0x3910,alternate);}
    };
    a->memory.date_observer=&x;b->memory.date_observer=&y;Trace before,after;Model old(index,true,options),modern(index,false,options);
    const auto run=[&](State& s,Model& model,bool original){try{if(original){original_trace=&before;model.reference(s);}else methods[index](s,&model,&after);}
        catch(const std::exception&e){original_trace=nullptr;return std::string(e.what());}original_trace=nullptr;return std::string{};};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Actor order "+hex_address(input.pc)+" case="+std::to_string(cases);
    require(!expected.starts_with("Unknown actor-order dependency"),expected);require(expected==actual,label+" outcomes original="+expected+" modern="+actual);
    if(options.own_negative){if(index==4)b->wr<8>(0x4010,b->r<8>(0x4010)^1);else b->wr<4>(12,b->r<4>(12)^1);}
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){
        if(!options.own_negative&&!options.child_negative){
            std::cerr<<label<<" original calls="<<old.calls.size()<<" modern calls="<<modern.calls.size()<<" original steps="<<before.order.size()<<" modern steps="<<after.order.size()<<'\n';
            for(unsigned n=0;n<std::min(before.order.size(),after.order.size());++n)if(before.order[n]!=after.order[n]){std::cerr<<"First site difference "<<n<<" "<<hex_address(before.order[n])<<" / "<<hex_address(after.order[n])<<'\n';break;}
            throw;
        }
        rejected=true;
    }
    require(rejected==(options.own_negative||options.child_negative),label+" negative failed to discriminate");
    if(!rejected){
        require(x.bytes==y.bytes,label+" ordered memory effects");require(a->executed==b->executed,label+" instruction accounting");
        require(old.calls==modern.calls,label+" complete dependency ABI");require(before.order==after.order,label+" source-site order");
    }
    ++cases;negatives+=rejected;interruptions+=!expected.empty();if(expected.empty()&&!rejected){++returns;++returned_entries[index];}
    sites.insert(before.order.begin(),before.order.end());return {std::move(a),x.log,unsigned(old.calls.size())};
}
void boundaries(const State& input,unsigned index){
    const auto baseline=compare(input,index);const auto steps=baseline.state->executed-input.executed;
    for(std::uint64_t n=0;n<=steps;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s,index);}
    std::set<std::uint32_t> bytes;
    for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(const auto hole:bytes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;
        for(const auto byte:bytes)if(byte!=hole){s->memory.map(byte,1);s->memory.write<1>(byte,input.memory.read<1>(byte));}compare(*s,index);}
    for(unsigned n=1;n<=baseline.calls;++n){Options o;o.interrupt=n;compare(input,index,o);}
}
void positions(State& s,unsigned value){for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot)s.memory.write<1>(first+side*0x2350+slot*0x1c4+0x19,value);}
}
namespace cm::aot {void actor_order_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{
    for(unsigned index=0;index<5;++index){
        auto input=seed(index);auto initial=compare(*input,index);
        if(index==0||index==1)require(initial.state->r<4>(12)==(index==0?11:7),"Unsigned ordering attribute");
        if(index==3)require(initial.state->r<4>(12)==14,"Independent weighted actor priority");
        if(index==4)require(float_value<8>(initial.state->r<8>(0x4010))==0.5,"Current fitness promotion");
        if(index==2){
            require(initial.state->memory.read<1>(match+0x792e)==22&&initial.state->memory.read<4>(toc-0x3910)==match,"Independent active queue count and callback owner");
            for(unsigned slot=0;slot<11;++slot)for(unsigned n=0;n<2;++n)
                require(initial.state->memory.read<4>(match+0x79f8+(slot*2+n)*4)==first+slot*0x1c4+((slot%2)^n)*0x2350,"Independent interleaved queue before sort dependency");
        }
        boundaries(*input,index);Options own;own.own_negative=true;compare(*input,index,own);
        if(index==2||index==3){Options child;child.child_negative=true;compare(*input,index,child);}
        auto relocated=clone(*input);relocated->memory.map(fixture+0x70000,0x9000);relocated->memory.copy_in(fixture+0x70000,relocated->memory.copy_out(toc-0x8000,0x9000));relocated->wr<4>(8,fixture+0x78000);compare(*relocated,index);
        for(auto pointer:{0u,first+1,stack-0x90,stack-8,stack+8}){auto s=clone(*input);s->wr<4>(12,pointer);compare(*s,index);}
    }
    for(auto value:{0u,10u,11u,127u,128u,255u}){auto s=seed(2);positions(*s,value);compare(*s,2);}
    auto empty=seed(2);positions(*empty,255);boundaries(*empty,2);require(compare(*empty,2).state->memory.read<1>(match+0x792e)==0,"Empty queue still reaches sort");
    for(auto sequence:{std::vector<std::uint32_t>{1},{0},{0xabcdef01u},{128},{255}}){Options o;o.sides=sequence;compare(*seed(2),2,o);}
    Options reverse;reverse.reverse_sort=true;compare(*seed(2),2,reverse);Options count;count.change_count=true;compare(*seed(2),2,count);
    for(auto gate:{0u,1u,128u,255u})for(auto priority:{0u,first,second})for(auto carrier:{0u,first,second}){
        auto s=seed(3);s->memory.write<1>(match+0x7928,gate);s->memory.write<4>(match+0xe000,priority);s->memory.write<4>(match+0x7c68,carrier);compare(*s,3);
    }
    for(auto cache:{0xe000u,0x7c68u}){
        auto s=seed(3);s->memory.write<1>(match+0x7928,1);s->memory.write<4>(match+cache,first);s->memory.write<4>(slots+4,first);
        require(compare(*s,3).state->r<4>(12)==0xffffffffu,"Equal priority actors retain first-operand precedence");boundaries(*s,3);
    }
    auto retargeted=seed(3);retargeted->memory.write<1>(match+0x7928,1);retargeted->memory.write<4>(alternate+0xe000,second);
    Options retarget;retarget.retarget_priority=true;compare(*retargeted,3,retarget);
    Options changed;changed.change_fitness=true;compare(*seed(3),3,changed);
    for(auto pointer:{0u,constants+1,stack-0x90,stack-8}){auto s=seed(3);s->memory.write<4>(toc-0x52c4,pointer);compare(*s,3);}
    for(auto pointer:{slots,stack+0x18,stack+0x1c,slots+1}){auto s=seed(3);s->wr<4>(12,pointer);s->wr<4>(16,pointer);compare(*s,3);}
    for(unsigned byte=0;byte<256;++byte)for(auto offset:{0x107u,0x108u}){
        auto s=seed(3);s->memory.write<1>(first+offset,byte);s->memory.write<1>(second+offset,255-byte);compare(*s,3);
        auto leaf=seed(offset==0x108?0:1);leaf->memory.write<1>(first+offset,byte);require(compare(*leaf,offset==0x108?0:1).state->r<4>(12)==byte,"Unnarrowed attribute byte result");
    }
    for(auto value:{0u,1u,32767u,32768u,65535u}){auto s=seed(3);s->memory.write<2>(first+0x3c,value);s->memory.write<2>(second+0x3c,value^32768u);compare(*s,3);}
    for(auto bits:{0u,0x80000000u,0x7f800000u,0xff800000u,0x7fc00123u,0x7f800123u,1u,0x7f7fffffu}){
        for(auto index:{3u,4u}){auto s=seed(index);s->memory.write<4>(first+0x50,bits);compare(*s,index);}
    }
    for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000000000123),UINT64_C(1)})
        for(auto offset:{0x60u,0x90u,0xf0u}){auto s=seed(3);s->memory.write<8>(constants+offset,bits);compare(*s,3);}
    require(sites.size()==202,"Every complete actor-order original site must be covered");
    for(auto n:returned_entries)require(n>0,"Entry lacks normal return evidence");
    std::cout<<"PASS: "<<cases<<" actor-order comparisons; "<<returns<<" returns, "<<interruptions<<" interruptions, "<<negatives
        <<" own/child negatives; all "<<sites.size()<<" original sites; full state, effects, dependency ABI and exact accounting\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
