#include "team_spatial.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot { bool team_spatial_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace spatial = cm::aot::team_spatial;
constexpr auto match=fixture+0x1000, first=match+0x311c, constants=fixture+0x30000;
using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
constexpr Function methods[]{spatial::nearest_actor,spatial::nearest_distance,spatial::nearby_weight,spatial::refresh_column_limits};
struct Trace final:SemanticGuestObserver {
    std::vector<std::uint32_t> order;
    void before_step(State&,std::uint32_t pc)override{order.push_back(pc);}
};
thread_local Trace* original_trace=nullptr;
struct Event {
    std::uint32_t target,continuation;decltype(State::registers) before,after;
    bool operator==(const Event&)const=default;
};
struct Options {bool own_negative=false,child_negative=false,changing_columns=false;unsigned interrupt=0;};
struct Model final:SemanticDependencyDispatch {
    unsigned index;bool original;Options options;std::vector<Event> calls;
    Model(unsigned i,bool old,const Options& o):index(i),original(old),options(o){}
    void reference(State& s){while(s.pc!=return_sentinel)if(!team_spatial_reference(s))call(s,s.pc,s.r<4>(4128));}
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override{
        Event event{target,continuation,s.registers,{}};
        const auto actor=std::uint32_t(s.r<4>(12));std::uint32_t result=0;std::uint64_t floating=0;
        switch(target){
        case 0x101d8110:result=s.memory.read<1>(actor+0x19);break;
        case 0x10289560:result=std::uint32_t(signed_value<2>(s.memory.read<2>(actor+0x1c2)));break;
        case 0x101d5680:
            result=s.memory.read<1>(actor+0x106);
            if(options.changing_columns)s.memory.write<1>(actor+0x106,result+1);
            if(options.child_negative&&!original)result=100;
            break;
        case 0x1022e760:
            floating=s.memory.read<8>(actor+0x140);
            if(options.child_negative&&!original)floating=float_bits<8>(index==2?0.0:-100.0);
            break;
        default:throw std::runtime_error("Unknown spatial dependency "+hex_address(target));
        }
        for(unsigned reg=0;reg<=12;++reg)if(reg!=1&&reg!=2)s.wr<4>(reg*4,0xab000000+reg*73);
        for(unsigned reg=0;reg<=13;++reg)s.wr<8>(0x4000+reg*16,float_bits<8>(-19.0-reg));
        s.wr<1>(2304,5);s.wr<4>(12,result);
        if(target==0x1022e760)s.wr<8>(0x4010,floating);
        s.pc=continuation;event.after=s.registers;calls.push_back(std::move(event));
        if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture spatial dependency interrupted");
    }
};
std::unique_ptr<State> seed(unsigned index){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    for(unsigned i=1280;i<1312;++i)s->wr<1>(i,0);
    s->memory.map(fixture,0x40000);s->memory.map(stack-0x800,0x1000);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,match);s->wr<4>(16,0);s->wr<4>(20,8);s->wr<4>(24,6);s->wr<4>(28,0);
    s->wr<4>(4128,return_sentinel);s->pc=spatial::entries[index];
    s->memory.write<4>(toc-0x52c4,constants);s->memory.write<4>(toc-0x52c8,constants+0x1000);
    s->memory.write<4>(constants+0x1000+0xc8,float_bits<4>(1000.0));
    s->memory.write<4>(constants+0x1000+0xc,float_bits<4>(0.0));
    s->memory.write<8>(constants+0x50,float_bits<8>(0.0));s->memory.write<8>(constants+0x1b8,float_bits<8>(2.0));
    for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){
        const auto actor=first+side*0x2350+slot*0x1c4;
        s->memory.write<1>(actor+0x19,slot%11);s->memory.write<2>(actor+0x1c2,2);
        s->memory.write<1>(actor+0x2c,0);s->memory.write<1>(actor+0x106,side?7+slot:4-slot);
        s->memory.write<8>(actor+0x140,float_bits<8>(20.0-slot));
    }
    return s;
}
unsigned cases=0,returns=0,interruptions=0,negatives=0;std::set<std::uint32_t> sites;
std::array<unsigned,4> returned_entries{};
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;};
Result compare(const State& input,unsigned index,const Options& options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();
    a->memory.date_observer=&x;b->memory.date_observer=&y;Trace before,after;Model old(index,true,options),modern(index,false,options);
    const auto run=[&](State& s,Model& model,bool original){try{if(original){original_trace=&before;model.reference(s);}else methods[index](s,&model,&after);}
        catch(const std::exception&e){original_trace=nullptr;return std::string(e.what());}original_trace=nullptr;return std::string{};};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Team spatial "+hex_address(input.pc)+" case="+std::to_string(cases);
    require(!expected.starts_with("Unknown spatial dependency"),expected);require(expected==actual,label+" outcomes original="+expected+" modern="+actual);
    if(options.own_negative){if(index==1)b->wr<8>(0x4010,b->r<8>(0x4010)^1);else if(index==3)b->memory.write<1>(match+0x7925,b->memory.read<1>(match+0x7925)^1);else b->wr<4>(12,b->r<4>(12)^1);}
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" negative failed to discriminate");
    if(!rejected){
        require(x.bytes==y.bytes,label+" ordered memory effects");require(a->executed==b->executed,label+" instruction accounting");
        if(old.calls!=modern.calls){
            for(unsigned n=0;n<std::min(old.calls.size(),modern.calls.size());++n)if(old.calls[n]!=modern.calls[n]){
                const auto& p=old.calls[n];const auto& q=modern.calls[n];const auto d=std::mismatch(p.before.begin(),p.before.end(),q.before.begin());
                throw std::runtime_error(label+" dependency="+std::to_string(n)+" target="+hex_address(p.target)+" continuation="+hex_address(p.continuation)+"/"+hex_address(q.continuation)+" register-byte="+std::to_string(d.first-p.before.begin()));
            }
            throw std::runtime_error(label+" dependency count differs");
        }
        require(before.order==after.order,label+" source-site order");
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
void fill(State& s,std::uint32_t offset,unsigned size,std::uint64_t value){
    for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){const auto p=first+side*0x2350+slot*0x1c4+offset;
        if(size==1)s.memory.write<1>(p,value);else if(size==2)s.memory.write<2>(p,value);else s.memory.write<8>(p,value);}
}
}
namespace cm::aot {void team_spatial_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{
    for(unsigned index=0;index<4;++index){
        auto input=seed(index);auto result=compare(*input,index);
        if(index==0)require(result.state->r<4>(12)==first+19*0x1c4,"Independent nearest actor");
        if(index==1)require(float_value<8>(result.state->r<8>(0x4010))==1.0,"Independent nearest distance");
        if(index==2)require(result.state->r<4>(12)==2,"Independent nearby weight");
        if(index==3)require(result.state->memory.read<1>(match+0x7925)==27&&result.state->memory.read<1>(match+0x7926)==240,"Independent column limits");
        boundaries(*input,index);Options own;own.own_negative=true;compare(*input,index,own);Options child;child.child_negative=true;compare(*input,index,child);
        for(auto position:{0u,10u,11u,127u,128u,255u}){auto s=seed(index);fill(*s,0x19,1,position);compare(*s,index);}
        for(auto role:{0u,1u,2u,3u,4u,0x8001u,0xffffu}){auto s=seed(index);fill(*s,0x1c2,2,role);compare(*s,index);}
        for(auto state:{0u,1u,2u,127u,128u,255u})for(auto flag:{0u,1u,0x100u,0xffffff80u}){
            auto s=seed(index);fill(*s,0x2c,1,state);s->wr<4>(28,flag);compare(*s,index);}
        for(auto side:{0u,1u,2u,0x100u,0xabcdef01u,0x7fu,0x80u,0xffu}){auto s=seed(index);s->wr<4>(16,side);compare(*s,index);}
        auto unavailable=seed(index);fill(*unavailable,0x19,1,255);boundaries(*unavailable,index);
        auto relocated=seed(index);relocated->memory.map(fixture+0x50000,0x9000);relocated->memory.copy_in(fixture+0x50000,relocated->memory.copy_out(toc-0x8000,0x9000));relocated->wr<4>(8,fixture+0x58000);compare(*relocated,index);
        for(auto alias:{stack-0x90,stack-0x60,stack-0x10,stack+8}){auto s=seed(index);s->memory.write<4>(toc-0x52c4,alias);compare(*s,index);}
    }
    for(unsigned index=0;index<3;++index){
        for(unsigned slot=0;slot<20;++slot){auto s=seed(index);s->memory.write<8>(first+slot*0x1c4+0x140,float_bits<8>(0.0));compare(*s,index);}
        for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000000000123),UINT64_C(0x4000000000000000),UINT64_C(0x4000000000000001)}){
            auto s=seed(index);fill(*s,0x140,8,bits);compare(*s,index);
            for(auto offset:{0x50u,0x1b8u}){s=seed(index);s->memory.write<8>(constants+offset,bits);compare(*s,index);}
        }
        auto exact=seed(index);fill(*exact,0x140,8,float_bits<8>(0.0));boundaries(*exact,index);
        auto ties=seed(index);fill(*ties,0x140,8,float_bits<8>(1.0));auto result=compare(*ties,index);
        if(index==0)require(result.state->r<4>(12)==first,"Nearest ties retain first actor");
        for(auto value:{0u,255u,0xabcdef80u}){auto s=seed(index);s->wr<4>(20,value);s->wr<4>(24,value^0x80000000);compare(*s,index);}
    }
    for(auto coordinate:{0u,5u,6u,7u,127u,128u,255u}){auto s=seed(3);fill(*s,0x106,1,coordinate);compare(*s,3);}
    Options changing;changing.changing_columns=true;compare(*seed(3),3,changing);
    require(sites.size()==1624,"Every complete team-spatial original site must be covered");
    for(auto n:returned_entries)require(n>0,"Entry lacks normal return evidence");
    std::cout<<"PASS: "<<cases<<" team-spatial comparisons; "<<returns<<" returns, "<<interruptions<<" interruptions, "<<negatives
        <<" own/child negatives; all "<<sites.size()<<" original sites; full state, effects, dependency ABI and exact accounting\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
