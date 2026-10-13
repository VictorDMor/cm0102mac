#include "movement_choice.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot {bool movement_choice_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace choice=cm::aot::movement_choice;
constexpr auto match=fixture+0x1000, actor=fixture+0x20000, occupant=actor+0x1000;
constexpr auto constants=fixture+0x30000, singles=constants+0x1000, traits=constants+0x2000, outputs=constants+0x3000;
using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
constexpr Function methods[]{choice::choose_cell,choice::adjustment_byte,choice::outside_central_rows,choice::inside_end_area};
struct Trace final:SemanticGuestObserver {std::vector<std::uint32_t> order;void before_step(State&,std::uint32_t pc)override{order.push_back(pc);}};
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Options {
    bool own_negative=false,child_negative=false,change_count=false,change_outputs=false;
    unsigned interrupt=0;
    std::uint32_t column_pressure=0,team_flag=1,can_face=1,role=0;
    double distance=0;
    std::vector<std::uint32_t> draws{5,0,2,1};
};
std::int32_t byte(std::uint32_t v){return std::int32_t(signed_value<1>(v));}
struct Model final:SemanticDependencyDispatch {
    unsigned index;bool original;Options options;std::vector<Event> calls;unsigned draws=0,positions=0;
    Model(unsigned entry,bool old,const Options& value):index(entry),original(old),options(value){}
    void reference(State& s){
        constexpr std::array<std::uint32_t,4> ends{0x101d8d60,0x101d8d68,0x101ee9e8,0x101eeab4};
        while(s.pc!=return_sentinel){
            if(s.pc>=choice::entries[index]&&s.pc<ends[index])require(movement_choice_reference(s),"Missing isolated movement-choice body");
            else call(s,s.pc,s.r<4>(4128));
        }
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override{
        Event event{target,continuation,s.registers,{}};const auto argument=std::uint32_t(s.r<4>(12));
        std::uint32_t result=0;std::uint64_t floating=0;
        const bool suffix=target==0x10773808||target==0x10773858;
        switch(target){
        case 0x10773808:case 0x10773858:
            for(unsigned n=28;n<32;++n){const auto at=std::uint32_t(s.r<4>(4))-(32-n)*8;
                if(target==0x10773808)s.memory.write<8>(at,s.r<8>(0x4000+n*16));else s.wr<8>(0x4000+n*16,s.memory.read<8>(at));}
            break;
        case 0x101d5680:result=s.memory.read<1>(argument+0x106);break;
        case 0x101d5690:result=s.memory.read<1>(argument+0x105);break;
        case 0x101d5660:result=s.memory.read<1>(argument+0x2c);break;
        case 0x101d5670:result=s.memory.read<1>(argument+0x108);break;
        case 0x101d8d60:result=s.memory.read<1>(argument+0x109);break;
        case 0x10283690:result=s.memory.read<1>(argument+0x27);break;
        case 0x101d3c90:result=options.draws[draws++%options.draws.size()];break;
        case 0x101e4ea0:result=options.column_pressure;break;
        case 0x101d7d30:result=options.team_flag;break;
        case 0x10245050:result=options.can_face;break;
        case 0x101e3d50:result=options.role;break;
        case 0x101ee9c0:result=byte(s.r<4>(16))<2||byte(s.r<4>(16))>6;break;
        case 0x101eea30:{
            const auto side=byte(s.r<4>(16)),row=byte(s.r<4>(20)),col=byte(s.r<4>(24));
            result=row>=2&&row<=6&&(side==1?(col>=10&&col<=11):(col>=0&&col<=1));break;
        }
        case 0x101d3b50:
            result=s.memory.read<1>(argument+0x19);
            if(options.change_count&&!positions++)for(unsigned row=0;row<9;++row)for(unsigned col=0;col<12;++col)
                for(unsigned side=0;side<2;++side)s.memory.write<1>(match+0xa08+row*0x450+col*0x5c+0x58+side,0);
            break;
        case 0x101e3f10:floating=float_bits<8>(options.distance);break;
        default:throw std::runtime_error("Unknown movement-choice dependency "+hex_address(target));
        }
        if(!suffix){
            if(options.child_negative&&!original&&target==0x101e3f10&&s.r<4>(20)==3&&s.r<4>(24)==5)floating=float_bits<8>(options.distance+13.0);
            for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xab000000+n*73);
            for(unsigned n=0;n<=13;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-19.0-n));
            s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);
            if(target==0x101e3f10)s.wr<8>(0x4010,floating);
            if(options.change_outputs){s.memory.write<1>(outputs,5);s.memory.write<1>(outputs+4,7);}
        }
        s.pc=continuation;event.after=s.registers;calls.push_back(std::move(event));
        if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture movement-choice dependency interrupted");
    }
};
void occupancy(State& s,unsigned count){
    for(unsigned row=0;row<9;++row)for(unsigned col=0;col<12;++col){const auto cell=match+0xa08+row*0x450+col*0x5c;
        for(unsigned side=0;side<2;++side){s.memory.write<1>(cell+0x58+side,count);
            for(unsigned n=0;n<11;++n)s.memory.write<4>(cell+side*0x2c+n*4,occupant+n*0x1c4);}}
}
std::unique_ptr<State> seed(unsigned index){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x40000);s->memory.map(stack-0x1000,0x2000);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,index==1?actor:match);s->wr<4>(16,index==0?actor:4);
    s->wr<4>(20,index==0?outputs:4);s->wr<4>(24,index==0?outputs+4:0);s->wr<4>(28,2);
    s->wr<4>(4128,return_sentinel);s->pc=choice::entries[index];
    s->memory.write<4>(toc-0x52c4,constants);s->memory.write<4>(toc-0x52c8,singles);
    s->memory.write<8>(constants+0x60,UINT64_C(0x4330000080000000));s->memory.write<8>(constants+0x90,float_bits<8>(1));
    s->memory.write<8>(constants+0xd0,float_bits<8>(1));s->memory.write<8>(constants+0x98,float_bits<8>(2));
    s->memory.write<4>(singles+0x30,float_bits<4>(-1000));s->memory.write<4>(actor+0x70,traits);s->memory.write<1>(traits+0x26,10);
    s->memory.write<1>(actor+0x105,4);s->memory.write<1>(actor+0x106,6);s->memory.write<1>(actor+0x108,1);s->memory.write<1>(actor+0x109,1);
    s->memory.write<1>(actor+0x2c,2);s->memory.write<1>(actor+0x2e,3);s->memory.write<4>(actor+0xe4,float_bits<4>(0.75));
    s->memory.write<1>(outputs,4);s->memory.write<1>(outputs+4,6);occupancy(*s,0);
    for(unsigned n=0;n<11;++n){s->memory.write<1>(occupant+n*0x1c4+0x19,n%2?255:1);s->memory.write<1>(occupant+n*0x1c4+0x2c,n%3);}
    for(unsigned n=28;n<32;++n)s->wr<8>(0x4000+n*16,float_bits<8>(100+n));
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
    const auto label="Movement choice "+hex_address(input.pc)+" case="+std::to_string(cases);
    require(!expected.starts_with("Unknown movement-choice dependency"),expected);require(expected==actual,label+" outcomes original="+expected+" modern="+actual);
    if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){
        if(!options.own_negative&&!options.child_negative){
            std::cerr<<label<<" original calls="<<old.calls.size()<<" modern calls="<<modern.calls.size()<<" original steps="<<before.order.size()<<" modern steps="<<after.order.size()<<'\n';
            for(unsigned n=0;n<std::min(before.order.size(),after.order.size());++n)if(before.order[n]!=after.order[n]){std::cerr<<"First site difference "<<n<<" "<<hex_address(before.order[n])<<" / "<<hex_address(after.order[n])<<'\n';break;}
            throw;
        }
        rejected=true;
    }
    require(rejected==(options.own_negative||options.child_negative),label+" negative failed to discriminate");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered memory effects");require(a->executed==b->executed,label+" instruction accounting");
        require(old.calls==modern.calls,label+" complete dependency ABI");require(before.order==after.order,label+" source-site order");}
    ++cases;negatives+=rejected;interruptions+=!expected.empty();if(expected.empty()&&!rejected){++returns;++returned_entries[index];}
    sites.insert(before.order.begin(),before.order.end());return {std::move(a),x.log,unsigned(old.calls.size())};
}
void boundaries(const State& input,unsigned index){
    const auto baseline=compare(input,index);const auto steps=baseline.state->executed-input.executed;
    for(std::uint64_t n=0;n<=steps;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s,index);}
    std::set<std::uint32_t> bytes;for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(const auto hole:bytes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;
        for(const auto b:bytes)if(b!=hole){s->memory.map(b,1);s->memory.write<1>(b,input.memory.read<1>(b));}compare(*s,index);}
    for(unsigned n=1;n<=baseline.calls;++n){Options o;o.interrupt=n;compare(input,index,o);}
}
}
namespace cm::aot {void movement_choice_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{
    for(unsigned index=0;index<4;++index){auto s=seed(index);auto result=compare(*s,index);
        if(index==0)require(result.state->r<4>(12)==1&&result.state->memory.read<1>(outputs)==5&&result.state->memory.read<1>(outputs+4)==7,"Independent final equal-score cell choice");
        boundaries(*s,index);Options negative;negative.own_negative=true;compare(*s,index,negative);
    }
    Options child;child.child_negative=true;compare(*seed(0),0,child);
    for(unsigned value=0;value<256;++value){
        auto query=seed(1);query->memory.write<1>(actor+0x109,value);require(compare(*query,1).state->r<4>(12)==value,"Unnarrowed adjustment byte");
        auto region=seed(2);region->wr<4>(16,value);require(compare(*region,2).state->r<4>(12)==unsigned(byte(value)<2||byte(value)>6),"Independent central-row predicate");
        auto s=seed(0);s->wr<4>(28,value);compare(*s,0);
    }
    for(auto side:{0u,1u,2u,127u,128u,255u})for(auto row:{0u,1u,2u,4u,6u,7u,127u,128u,255u})for(auto col:{0u,1u,2u,9u,10u,11u,12u,127u,128u,255u}){
        auto s=seed(3);s->wr<4>(16,side);s->wr<4>(20,row);s->wr<4>(24,col);const bool valid=byte(row)>=2&&byte(row)<=6&&(byte(side)==1?(byte(col)>=10&&byte(col)<=11):(byte(col)>=0&&byte(col)<=1));
        require(compare(*s,3).state->r<4>(12)==unsigned(valid),"Independent end-area predicate");
    }
    for(auto row:{0u,1u,4u,7u,8u,127u,128u,255u})for(auto col:{0u,1u,5u,10u,11u,127u,128u,255u})for(auto flags:{0u,2u,7u,15u}){
        auto s=seed(0);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,col);s->wr<4>(28,flags);compare(*s,0);
    }
    for(auto side:{0u,1u,255u})for(auto mode:{0u,1u,2u,255u})for(auto row:{1u,3u,4u,5u,7u})for(auto count:{0u,1u,3u}){
        auto s=seed(0);s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x2d,mode);s->memory.write<1>(actor+0x105,row);occupancy(*s,count);
        for(auto offset:{0x40u,0xc0u,0xd8u,0xe0u,0xe8u})s->memory.write<8>(constants+offset,float_bits<8>(0.75));
        compare(*s,0);
    }
    for(auto flags:{0u,2u,4u,6u,7u,15u})for(auto action:{0u,1u,2u,255u})for(auto pressure:{0u,1u,2u,3u,255u}){
        auto s=seed(0);s->wr<4>(28,flags);s->memory.write<1>(actor+0x2c,action);Options o;o.column_pressure=pressure;compare(*s,0,o);
    }
    for(auto row:{1u,2u,6u,7u})for(auto flags:{0u,1u,2u,3u})for(auto side:{0u,1u}){
        auto s=seed(0);s->wr<4>(28,flags);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x27,side);
        Options o;o.role=0x8b0;compare(*s,0,o);
    }
    for(auto flags:{0u,2u,6u}){auto s=seed(0);s->wr<4>(28,flags);occupancy(*s,3);Options o;o.draws={0,5};compare(*s,0,o);o.change_count=true;compare(*s,0,o);o.change_outputs=true;compare(*s,0,o);}
    {auto s=seed(0);occupancy(*s,1);Options o;o.draws={0,5};compare(*s,0,o);}
    {auto s=seed(0);s->wr<4>(28,0);occupancy(*s,3);boundaries(*s,0);}
    {auto s=seed(0);s->wr<4>(28,0);s->memory.write<1>(actor+0x2c,0);s->memory.write<1>(actor+0x108,0);boundaries(*s,0);}
    for(auto field:{0x105u,0x106u}){auto s=seed(0);s->memory.write<1>(actor+field,126);s->budget=1000;compare(*s,0);}
    for(auto index:{0u,1u,2u,3u}){
        auto s=seed(index);s->memory.map(fixture+0x70000,0x9000);s->memory.copy_in(fixture+0x70000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,fixture+0x78000);compare(*s,index);
    }
    for(auto pointer:{0u,actor+1,stack-0xc0,stack-8}){auto s=seed(0);s->wr<4>(16,pointer);s->budget=10000;compare(*s,0);}
    for(auto count:{127u,128u,255u}){auto s=seed(0);s->wr<4>(28,0);occupancy(*s,count);s->budget=10000;compare(*s,0);}
    for(auto flags:{2u,6u}){auto s=seed(0);s->wr<4>(28,flags);Options o;o.can_face=0;compare(*s,0,o);o.can_face=1;o.team_flag=0;compare(*s,0,o);}
    for(auto value:{0u,128u,255u}){auto s=seed(0);s->wr<4>(28,0);s->memory.write<1>(actor+0x2c,0);s->memory.write<1>(actor+0x108,value);compare(*s,0);}
    for(auto field:{20u,24u})for(auto pointer:{0u,outputs,outputs+1,stack+0x20,stack+0x24,stack-0x88,constants+0x60,actor+0x106}){auto s=seed(0);s->wr<4>(field,pointer);compare(*s,0);}
    for(auto flags:{0u,2u,6u})for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000000000123),UINT64_C(1)}){
        auto s=seed(0);s->wr<4>(28,flags);Options o;o.distance=float_value<8>(bits);compare(*s,0,o);
        for(auto offset:{0x60u,0x90u,0xc0u,0xd0u,0xe8u}){auto t=clone(*s);t->memory.write<8>(constants+offset,bits);compare(*t,0);}
    }
    if(sites.size()!=749){
        constexpr std::array<std::uint32_t,4> ends{0x101d8d60,0x101d8d68,0x101ee9e8,0x101eeab4};
        for(unsigned n=0;n<4;++n)for(auto pc=choice::entries[n];pc<ends[n];pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered "<<hex_address(pc)<<'\n';
    }
    require(sites.size()==749,"Every complete movement-choice original site must be covered");for(auto n:returned_entries)require(n>0,"Entry lacks normal return evidence");
    std::cout<<"PASS: "<<cases<<" movement-choice comparisons; "<<returns<<" returns, "<<interruptions<<" interruptions, "<<negatives<<" own/child negatives; all "<<sites.size()<<" original sites; full state, effects, dependency ABI and exact accounting\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
