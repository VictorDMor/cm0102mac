#include "pitch_distance.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot { bool pitch_distance_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace distance = cm::aot::pitch_distance;
constexpr auto actor = fixture+0x1000, constants = fixture+0x8000;
constexpr auto table = fixture+0x10000, second_table = fixture+0x1c000;
using Function = void(*)(State&, SemanticDependencyDispatch*, SemanticGuestObserver*);
constexpr Function methods[]{distance::point_to_point,distance::actor_to_cell,distance::actor_to_attacking_goal};
struct Trace final:SemanticGuestObserver {
    std::vector<std::uint32_t> order;
    void before_step(State&,std::uint32_t pc)override{order.push_back(pc);}
};
thread_local Trace* original_trace=nullptr;
struct Event {
    std::uint32_t target,continuation;decltype(State::registers) before,after;
    bool operator==(const Event&)const=default;
};
enum class Change { none, column, table_slot, spilled_column };
struct Options {
    bool own_negative=false,child_negative=false;
    unsigned interrupt=0;
    Change change=Change::none;
    std::uint32_t forced_result=0;
    bool force_result=false;
};
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;std::vector<Event> calls;
    Model(bool old,const Options& o):original(old),options(o){}
    void reference(State& s){while(s.pc!=return_sentinel)if(!pitch_distance_reference(s))call(s,s.pc,s.r<4>(4128));}
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override{
        require(target==0x10775540,"Unknown pitch-distance dependency");
        Event event{target,continuation,s.registers,{}};
        const auto value=std::uint32_t(s.r<4>(12));
        auto result=std::bit_cast<std::int32_t>(value)<0?0u-value:value;
        if(options.force_result)result=options.forced_result;
        if(options.child_negative&&!original)++result;
        if(calls.empty())switch(options.change){
        case Change::none:break;
        case Change::column:s.memory.write<1>(actor+0x106,1);break;
        case Change::table_slot:s.memory.write<4>(toc-0x6424,second_table);break;
        case Change::spilled_column:s.memory.write<1>(stack+0x23,1);break;
        }
        for(unsigned reg=0;reg<=12;++reg)if(reg!=1&&reg!=2)s.wr<4>(reg*4,0xab000000+reg*73);
        for(unsigned reg=0;reg<=13;++reg)s.wr<8>(0x4000+reg*16,float_bits<8>(-19.0-reg));
        s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;
        event.after=s.registers;calls.push_back(std::move(event));
        if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture distance dependency interrupted");
    }
};
std::unique_ptr<State> seed(unsigned index){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    for(unsigned i=1280;i<1312;++i)s->wr<1>(i,0);
    s->memory.map(fixture,0x20000);s->memory.map(stack-0x800,0x1000);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,actor);
    s->wr<4>(16,2);s->wr<4>(20,3);s->wr<4>(24,4);s->wr<4>(28,5);
    s->wr<4>(4128,return_sentinel);s->pc=distance::entries[index];
    s->memory.write<4>(toc-0x6424,table);s->memory.write<4>(toc-0x52c8,constants);
    s->memory.write<4>(constants+0xdc,float_bits<4>(999.0));
    s->memory.write<1>(actor+0x105,4);s->memory.write<1>(actor+0x106,5);s->memory.write<1>(actor+0x27,0);
    for(std::int32_t offset=-0x2000;offset<0x2000;offset+=4){
        s->memory.write<4>(table+std::uint32_t(offset),float_bits<4>(17.0+offset/4));
        s->memory.write<4>(second_table+std::uint32_t(offset),float_bits<4>(5017.0+offset/4));
    }
    return s;
}
unsigned cases=0,returns=0,interruptions=0,negatives=0;std::set<std::uint32_t> sites;
std::array<unsigned,3> returned_entries{};
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;};
Result compare(const State& input,unsigned index,const Options& options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();
    a->memory.date_observer=&x;b->memory.date_observer=&y;Trace before,after;Model old(true,options),modern(false,options);
    const auto run=[&](State& s,Model& model,bool original){try{if(original){original_trace=&before;model.reference(s);}else methods[index](s,&model,&after);}
        catch(const std::exception&e){original_trace=nullptr;return std::string(e.what());}original_trace=nullptr;return std::string{};};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Pitch distance "+hex_address(input.pc)+" case="+std::to_string(cases);
    require(!expected.starts_with("Unknown pitch-distance dependency"),expected);require(expected==actual,label+" outcomes original="+expected+" modern="+actual);
    if(options.own_negative)b->wr<8>(0x4010,b->r<8>(0x4010)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}
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
double result(const Result& r){return float_value<8>(r.state->r<8>(0x4010));}
}
namespace cm::aot {void pitch_distance_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{
    for(unsigned index=0;index<3;++index){
        auto input=seed(index);auto initial=compare(*input,index);
        require(result(initial)==(index==2?71.0:37.0),"Independent table lookup");
        boundaries(*input,index);Options own;own.own_negative=true;compare(*input,index,own);Options child;child.child_negative=true;compare(*input,index,child);
        for(auto change:{Change::column,Change::table_slot,Change::spilled_column}){Options o;o.change=change;compare(*input,index,o);}
        for(auto bits:{0u,1u,8u,9u,11u,12u,127u,128u,255u,0xffff0001u,0xffffff80u}){Options o;o.force_result=true;o.forced_result=bits;compare(*input,index,o);}
        for(auto alias:{stack-0x145,stack-0x109,stack-0x105,stack-0x101}){auto s=clone(*input);s->wr<4>(12,alias);compare(*s,index);}
        for(auto pointer:{0u,table-1,table+1,stack-0x40,stack-4}){auto s=clone(*input);s->memory.write<4>(toc-0x6424,pointer);compare(*s,index);}
        auto relocated=clone(*input);relocated->memory.map(fixture+0x30000,0x9000);relocated->memory.copy_in(fixture+0x30000,relocated->memory.copy_out(toc-0x8000,0x9000));relocated->wr<4>(8,fixture+0x38000);compare(*relocated,index);
        auto aliased=clone(*input);aliased->wr<4>(8,stack-4+0x6424);compare(*aliased,index);
        auto null_actor=clone(*input);null_actor->wr<4>(12,0);compare(*null_actor,index);
        for(auto bits:{0u,0x80000000u,0x7f800000u,0xff800000u,0x7fc00123u,0x7f800123u,0x00000001u,0x7f7fffffu}){
            auto s=clone(*input);s->memory.write<4>(table+(index==2?54:20)*4,bits);compare(*s,index);
        }
    }
    auto point=seed(0);
    for(unsigned row=0;row<9;++row)for(unsigned column=0;column<12;++column)
        for(unsigned other_row=0;other_row<9;++other_row)for(unsigned other_column=0;other_column<12;++other_column){
            point->wr<4>(16,row);point->wr<4>(20,column);point->wr<4>(24,other_row);point->wr<4>(28,other_column);
            const auto dx=int(row)-int(other_row),dy=int(column)-int(other_column);
            require(result(compare(*point,0))==17.0+std::abs(dx)+9*std::abs(dy),"Independent legal pitch grid");
        }
    for(unsigned index=0;index<3;++index){
        auto s=seed(index);
        for(auto fixed:{0u,127u,128u,255u})for(unsigned byte=0;byte<256;++byte){
            if(index==0){s->wr<4>(16,0x12340000|fixed);s->wr<4>(24,0xdead0000|byte);compare(*s,index);s->wr<4>(20,byte);s->wr<4>(28,fixed);compare(*s,index);}
            else {s->memory.write<1>(actor+0x105,fixed);s->wr<4>(16,0xabcd0000|byte);s->memory.write<1>(actor+0x106,byte);s->wr<4>(20,0xffff0000|fixed);s->memory.write<1>(actor+0x27,index==2?fixed:0);compare(*s,index);}
        }
    }
    for(auto delta:{9u,12u,128u}){auto s=seed(0);s->wr<4>(16,0);s->wr<4>(24,delta);boundaries(*s,0);require(result(compare(*s,0))==999.0,"Independent out-of-range fallback");
        for(auto bits:{0x80000000u,0x7fc00456u,0x7f800000u}){s->memory.write<4>(constants+0xdc,bits);compare(*s,0);}}
    auto away=seed(2);away->memory.write<1>(actor+0x27,1);boundaries(*away,2);require(result(compare(*away,2))==62.0,"Independent away goal column");
    require(sites.size()==129,"Every complete pitch-distance original site must be covered");
    for(auto count:returned_entries)require(count>0,"Entry lacks normal return evidence");
    std::cout<<"PASS: "<<cases<<" pitch-distance comparisons; "<<returns<<" returns, "<<interruptions<<" interruptions, "<<negatives
        <<" own/child negatives; all "<<sites.size()<<" original sites; full state, effects, dependency ABI and exact accounting\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
