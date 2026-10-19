#include "match_target.hpp"
#include "semantic_io_checks.hpp"
#include <fstream>
#include <set>

namespace cm::aot {bool match_target_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace target=cm::aot::match_target;
constexpr auto match=fixture+0x10000,first=match+0x311c,actor=first+0x1c4,constants=fixture+0x60000;
constexpr auto row_output=fixture+0x8000,column_output=row_output+4,table0=fixture+0x70000,table1=fixture+0x74000;
constexpr std::array<std::uint32_t,8> ends{0x101d57b8,0x101e305c,0x101e3150,0x101e3174,0x101e32d4,0x102861a8,0x102862a8,0x10113e28};
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{target::target_advance_enabled,target::resolve_target_cell,target::side_target_control,target::normalized_actor_side,target::is_nearest_candidate,target::controlled_target_override,target::uncontrolled_target_override,target::strategy_flags};
struct Mutation{std::uint32_t pc,address;std::uint8_t value;};
struct Trace:SemanticGuestObserver{
    std::vector<std::uint32_t> order;
    std::vector<Mutation> mutations;unsigned applied=0;
    void before_step(State& s,std::uint32_t pc)override{if(s.executed>=s.budget)return;order.push_back(pc);for(const auto& m:mutations)if(m.pc==pc){s.memory.write<1>(m.address,m.value);++applied;}}
};
thread_local Trace* original_trace=nullptr;
struct Event{std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Options{std::uint32_t team_flags=0;bool own_negative=false,child_negative=false,changing_roles=false;unsigned interrupt=0;std::vector<Mutation> mutations;};
struct Model:SemanticDependencyDispatch{
    bool original;Options options;Trace* trace;std::vector<Event> calls;unsigned child_mutations=0;
    Model(bool old,const Options& o,Trace* t):original(old),options(o),trace(t){}
    void reference(State& s,std::uint32_t until){
        const auto root=std::find(target::entries.begin(),target::entries.end(),s.pc);
        require(root!=target::entries.end(),"Independent original must start at a complete target root");
        const auto index=unsigned(root-target::entries.begin()),caller_stack=std::uint32_t(s.r<4>(4));
        bool first_call=true;
        while(s.pc!=until){
            if((!first_call&&std::find(target::entries.begin(),target::entries.end(),s.pc)!=target::entries.end())||!match_target_reference(s))call(s,s.pc,s.r<4>(4128));
            // Output aliases can overwrite the saved LR. Stop at the observed
            // own BLR boundary, retaining its changed PC for exact comparison;
            // chasing that corrupted caller would leave this component scope.
            if(!trace->order.empty()&&trace->order.back()==ends[index]-4&&s.r<4>(4)==caller_stack)return;
            first_call=false;
        }
    }
    void call(State& s,std::uint32_t entry,std::uint32_t continuation)override{
        const auto n=calls.size();calls.push_back({entry,continuation,s.registers,{}});
        const auto found=std::find(target::entries.begin(),target::entries.end(),entry);
        if(found!=target::entries.end()){
            if(original)reference(s,continuation);else methods[unsigned(found-target::entries.begin())](s,this,trace);
            if(options.child_negative&&!original&&entry==target::entries[3]){s.wr<4>(12,s.r<4>(12)^1);++child_mutations;}
        }else{
            const auto input=std::uint32_t(s.r<4>(12));std::uint32_t value=0;std::uint64_t floating=0;
            switch(entry){
            case 0x101d56f0:value=options.team_flags;break;
            case 0x101d3b50:case 0x101d8110:value=s.memory.read<1>(input+0x19);break;
            case 0x101d5680:value=s.memory.read<1>(input+0x106);break;
            case 0x101d5690:value=s.memory.read<1>(input+0x107);break;
            case 0x10283690:value=s.memory.read<1>(input+0x27);break;
            case 0x10289560:
                value=std::uint32_t(signed_value<2>(s.memory.read<2>(input+0x1c2)));
                if(options.changing_roles)s.memory.write<2>(input+0x1c2,value==1?2:1);
                break;
            case 0x1022e760:floating=s.memory.read<8>(input+0x140);break;
            case 0x101eea30:{
                const auto side=signed_value<1>(s.r<4>(16)),row=signed_value<1>(s.r<4>(20)),column=signed_value<1>(s.r<4>(24));
                value=2<=row&&row<=6&&(side==1?10<=column&&column<=11:0<=column&&column<=1);break;
            }
            default:throw std::runtime_error("Unknown targeting dependency "+hex_address(entry));
            }
            for(unsigned reg=0;reg<=12;++reg)if(reg!=1&&reg!=2)s.wr<4>(reg*4,0xab000000+reg*73);
            for(unsigned reg=0;reg<=13;++reg)s.wr<8>(0x4000+reg*16,float_bits<8>(-19.0-reg));
            s.wr<1>(2304,5);s.wr<4>(12,value);if(entry==0x1022e760)s.wr<8>(0x4010,floating);s.pc=continuation;
        }
        calls[n].after=s.registers;
        if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture targeting dependency interrupted");
    }
};
void strategy_words(State& s,std::uint32_t high,std::uint32_t low){
    for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){
        const auto address=match+0x7c6c+side*0x18e0+0x1888+slot*8;s.memory.write<4>(address,high);s.memory.write<4>(address+4,low);
    }
}
std::unique_ptr<State> seed(unsigned index){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x100000);s->memory.map(stack-0x800,0x1000);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->pc=target::entries[index];
    s->wr<4>(12,match);s->wr<4>(16,0);s->wr<4>(20,row_output);s->wr<4>(24,column_output);s->wr<4>(28,0);
    s->memory.write<4>(toc-0x52c4,constants);s->memory.write<8>(constants+0x50,float_bits<8>(0.0));
    s->memory.write<1>(match+0x791d,3);s->memory.write<1>(match+0x791e,5);s->memory.write<1>(match+0x791f,255);
    s->memory.write<1>(match+0x7924,0);s->memory.write<1>(match+0x7927,0);s->memory.write<4>(match+0xdff8,0);
    s->memory.write<4>(match+0xdfec,table0);s->memory.write<4>(match+0xdff0,table1);
    s->memory.copy_in(table0,std::vector<std::uint8_t>(0x4000,0x53));s->memory.copy_in(table1,std::vector<std::uint8_t>(0x4000,0x62));
    for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){
        const auto p=first+side*0x2350+slot*0x1c4;s->memory.write<1>(p+0x19,slot%11);s->memory.write<1>(p+0x27,side);s->memory.write<1>(p+0x2c,0);
        s->memory.write<4>(p+0x1a4,match);s->memory.write<2>(p+0x1c2,2);s->memory.write<1>(p+0x106,6);s->memory.write<1>(p+0x107,4);s->memory.write<8>(p+0x140,float_bits<8>(40.0+slot));
    }
    s->memory.write<8>(actor+0x140,float_bits<8>(1.0));strategy_words(*s,0x12345678,0);
    if(index==1)s->wr<4>(16,actor);
    if(index==3||index==5||index==6)s->wr<4>(12,actor);
    if(index==4){s->wr<4>(16,actor);s->wr<4>(20,3);s->wr<4>(24,5);}
    if(index==7){s->wr<4>(12,match+0x7c6c);s->wr<4>(16,1);}
    return s;
}
unsigned cases=0,returns=0,interruptions=0,negatives=0,mutation_cases=0;
std::array<std::set<std::uint32_t>,8> complete_sites;std::array<unsigned,8> root_returns{};
struct Result{std::unique_ptr<State> state;std::vector<Effects::Access> accesses;unsigned calls;};
Result compare(const State& input,unsigned index,const Options& options={}){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace before,after;before.mutations=after.mutations=options.mutations;Model old(true,options,&before),modern(false,options,&after);
    const auto run=[&](State& s,Model& m){try{if(m.original){original_trace=m.trace;m.reference(s,return_sentinel);}else methods[index](s,&m,m.trace);}
        catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}original_trace=nullptr;return std::string{};};
    const auto expected=run(*a,old),actual=run(*b,modern);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Targeting root="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(!expected.starts_with("Unknown targeting dependency"),label+" "+expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture targeting dependency interrupted"||expected.starts_with("Unmapped data access at "),label+" unexpected outcome="+expected);
    require(before.applied==after.applied,label+" paired source mutation count");
    if(expected.empty())require(before.applied==options.mutations.size(),label+" declared source mutation not exercised");
    if(options.own_negative){if(index==1)b->memory.write<1>(std::uint32_t(input.r<4>(20)),b->memory.read<1>(input.r<4>(20))^1);else b->wr<4>(12,b->r<4>(12)^1);}
    if(options.child_negative)require(modern.child_mutations>0,label+" composed normalized-side child negative not exercised");
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" negative must discriminate");
    if(!rejected){Image(*a).compare(*b,label);require(x.bytes==y.bytes,label+" ordered memory effects");require(a->executed==b->executed,label+" instruction count");require(old.calls==modern.calls,label+" dependency ABI");require(before.order==after.order,label+" source order");}
    ++cases;interruptions+=!expected.empty();negatives+=rejected;mutation_cases+=!options.mutations.empty();
    if(expected.empty()&&!rejected){++returns;++root_returns[index];for(auto pc:before.order)for(unsigned n=0;n<8;++n)if(target::entries[n]<=pc&&pc<ends[n])complete_sites[n].insert(pc);}
    return {std::move(a),x.log,unsigned(old.calls.size())};
}
void boundaries(const State& input,unsigned index,const Options& options={}){
    const auto result=compare(input,index,options);const auto steps=result.state->executed-input.executed;
    for(std::uint64_t n=0;n<=steps;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s,index,options);}
    std::set<std::uint32_t> bytes;for(const auto& access:result.accesses)for(unsigned n=0;n<access.size;++n)bytes.insert(access.address+n);
    for(auto hole:bytes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;
        for(auto byte:bytes)if(byte!=hole){s->memory.map(byte,1);s->memory.write<1>(byte,input.memory.read<1>(byte));}compare(*s,index,options);}
    for(unsigned n=1;n<=result.calls;++n){auto o=options;o.interrupt=n;compare(input,index,o);}
}
void actor_words(State& s,std::uint32_t offset,unsigned size,std::uint64_t value){
    for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){auto p=first+side*0x2350+slot*0x1c4+offset;
        if(size==1)s.memory.write<1>(p,value);else if(size==2)s.memory.write<2>(p,value);else s.memory.write<8>(p,value);}
}
}
namespace cm::aot{void match_target_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{
    require(argc==2,"Expected complete-source coverage output");
    for(unsigned index=0;index<8;++index){
        auto input=seed(index);const auto result=compare(*input,index);
        if(index==1)require(result.state->memory.read<1>(row_output)==3&&result.state->memory.read<1>(column_output)==5,"Independent packed target expectation");
        if(index==2||index==4)require(result.state->r<4>(12)==1,"Independent control/nearest expectation");
        if(index==7)require(result.state->r<4>(12)==0x12345678&&result.state->r<4>(16)==0,"Independent ordered strategy words");
        boundaries(*input,index);Options own;own.own_negative=true;compare(*input,index,own);
        for(auto side:{0u,1u,2u,0x100u,0xabcdef01u,0x7fu,0x80u,0xffu}){auto s=seed(index);
            if(index==0||index==2||index==7)s->wr<4>(16,side);else s->memory.write<1>(actor+0x27,side);compare(*s,index);}
        if(index!=7){auto s=seed(index);s->memory.map(fixture+0x110000,0x9000);s->memory.copy_in(fixture+0x110000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,fixture+0x118000);compare(*s,index);}
    }
    for(auto flags:{0u,0x1000u,0x10000u,0x8000u,0x4000u,0x1d000u,0xffffffffu})for(auto phase:{0u,1u,0x80u})for(auto control:{0u,1u})for(auto role:{1u,2u}){
        auto s=seed(1);s->memory.write<1>(match+0x7927,phase);s->memory.write<1>(match+0x7924,control);s->memory.write<2>(actor+0x1c2,role);Options o;o.team_flags=flags;compare(*s,1,o);
    }
    for(auto row:{0u,8u,9u,127u,128u,255u})for(auto column:{0u,11u,12u,127u,128u,255u})for(auto phase:{0u,1u}){
        auto s=seed(1);s->memory.write<1>(match+0x791d,row);s->memory.write<1>(match+0x791e,column);s->memory.write<1>(match+0x7927,phase);compare(*s,1);
    }
    for(auto packed:{0u,0x88u,0xb8u,0xcfu,0xffu})for(auto phase:{0u,1u})for(auto role:{1u,2u}){
        auto s=seed(1);s->memory.copy_in(table0,std::vector<std::uint8_t>(0x4000,std::uint8_t(packed)));s->memory.write<1>(match+0x7927,phase);s->memory.write<2>(actor+0x1c2,role);compare(*s,1);
    }
    for(auto alias:{row_output,actor+0x27,actor+0x106,match+0x791d,stack-0x80,stack-0x40,stack+8}){
        auto s=seed(1);s->wr<4>(20,alias);s->wr<4>(24,alias);compare(*s,1);
    }
    // Exercise both directions and the packed-byte endpoints that bound the
    // positive/negative target adjustments and ball-relative column limits.
    for(auto side:{0u,1u})for(auto control:{0u,1u})for(auto flags:{0x1000u,0x10000u,0x8000u,0x4000u})for(auto packed:{0u,0x53u,0xb8u,0xffu})for(auto ball_column:{0u,5u,11u}){
        auto s=seed(1);s->memory.write<1>(actor+0x27,side);s->memory.write<1>(match+0x7924,control?side:1-side);s->memory.write<1>(match+0x791e,ball_column);
        s->memory.copy_in(side?table1:table0,std::vector<std::uint8_t>(0x4000,std::uint8_t(packed)));Options o;o.team_flags=flags;compare(*s,1,o);
    }
    for(auto side:{0u,1u})for(auto control:{0u,1u})for(auto row:{0u,127u,255u})for(auto column:{0u,127u,255u}){
        auto s=seed(1);s->memory.write<1>(actor+0x27,side);s->memory.write<1>(match+0x7924,control?side:1-side);s->memory.write<1>(match+0x7927,1);
        s->memory.write<1>(actor+0x107,row);s->memory.write<1>(actor+0x106,column);strategy_words(*s,0,0xffffffff);compare(*s,1);
    }
    // Four packed-row lower-bound checks cannot receive a negative nibble in
    // the ordinary path. Mutate the already-produced byte at the exact source
    // boundary in both engines; retain this as component-only evidence.
    for(unsigned variant=0;variant<4;++variant){
        auto s=seed(1);Options o;
        if(variant==0){s->memory.write<1>(match+0x7927,1);o.mutations={{0x101e2a84,column_output,255},{0x101e2ab4,row_output,255}};}
        if(variant==1){o.team_flags=0x1000;o.mutations={{0x101e2c30,row_output,255}};}
        if(variant==2){o.team_flags=0x10000;o.mutations={{0x101e2ce8,row_output,255}};}
        if(variant==3){s->memory.write<1>(match+0x7924,1);o.team_flags=0x8000;o.mutations={{0x101e2da0,row_output,255}};}
        compare(*s,1,o);
    }
    Options child;child.child_negative=true;compare(*seed(1),1,child);
    for(unsigned index:{0u,1u})for(auto flags:{0u,0x1000u,0xffffffffu}){Options o;o.team_flags=flags;compare(*seed(index),index,o);}
    for(unsigned index:{5u,6u,7u})for(auto flags:{0u,0x20000u,0x40000000u,0x80000000u,0x80000u,0xc00a0000u,0xffffffffu})for(auto role:{0u,1u,2u,0x8001u,0xffffu}){
        auto s=seed(index);s->memory.write<2>(actor+0x1c2,role);strategy_words(*s,0xdeadbeef,flags);compare(*s,index);
    }
    for(unsigned index:{5u,6u}){auto s=seed(index);strategy_words(*s,0,0);Options o;o.changing_roles=true;compare(*s,index,o);}
    for(auto side:{0u,1u,2u,0x100u})for(auto owner:{0u,1u,255u})for(auto carrier:{0u,actor})for(auto row:{1u,3u,6u,7u})for(auto column:{0u,1u,10u,11u,12u})for(auto marker:{0u,255u}){
        auto s=seed(2);s->wr<4>(16,side);s->memory.write<1>(match+0x7924,owner);s->memory.write<4>(match+0xdff8,carrier);s->memory.write<1>(match+0x791d,row);s->memory.write<1>(match+0x791e,column);s->memory.write<1>(match+0x791f,marker);compare(*s,2);
    }
    for(auto role:{1u,2u,0xffffu})for(auto state:{0u,1u,128u,255u})for(auto position:{0u,127u,128u,255u}){
        auto s=seed(4);s->memory.write<2>(actor+0x1c2,role);actor_words(*s,0x2c,1,state);actor_words(*s,0x19,1,position);compare(*s,4);
    }
    for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000000000123)})for(auto role:{1u,2u}){
        auto s=seed(4);s->memory.write<2>(actor+0x1c2,role);s->memory.write<8>(actor+0x140,bits);compare(*s,4);
        s=seed(4);actor_words(*s,0x140,8,bits);s->memory.write<2>(actor+0x1c2,role);compare(*s,4);
        s=seed(4);s->memory.write<8>(constants+0x50,bits);compare(*s,4);
    }
    for(unsigned slot=0;slot<20;++slot){auto s=seed(4);s->memory.write<8>(first+slot*0x1c4+0x140,float_bits<8>(0.0));compare(*s,4);}
    auto tied=seed(4);actor_words(*tied,0x140,8,float_bits<8>(1.0));require(compare(*tied,4).state->r<4>(12)==1,"Nonkeeper ties remain nearest");tied->memory.write<2>(actor+0x1c2,1);require(compare(*tied,4).state->r<4>(12)==0,"Goalkeeper tie loses to another ready actor");
    auto absent=seed(4);actor_words(*absent,0x19,1,255);boundaries(*absent,4);
    const auto data=seed(1);for(auto mode:{0u,1u}){auto s=clone(*data);s->memory.write<1>(match+0x7927,1);s->memory.write<1>(match+0x7924,mode);strategy_words(*s,0,0xffffffff);boundaries(*s,1);}
    std::ofstream coverage(argv[1]);unsigned total=0;constexpr std::array<unsigned,8> expected{22,491,60,9,77,58,62,6};
    for(unsigned index=0;index<8;++index){for(auto pc:complete_sites[index])coverage<<index<<" "<<hex_address(pc)<<'\n';require(root_returns[index]>0&&complete_sites[index].size()==expected[index],"Complete targeting source sites root="+hex_address(target::entries[index])+" sites="+std::to_string(complete_sites[index].size()));total+=complete_sites[index].size();std::cout<<"Root "<<hex_address(target::entries[index])<<" complete_sites="<<complete_sites[index].size()<<" returns="<<root_returns[index]<<'\n';}
    std::cout<<"Match targeting comparisons="<<cases<<" returns="<<returns<<" interruptions="<<interruptions<<" negatives="<<negatives<<" source_mutation_cases="<<mutation_cases<<" completed_sites="<<total<<"/785\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
