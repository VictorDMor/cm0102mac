#include "ball_action.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot { bool ball_action_preparation_reference(State&);bool ball_action_orientation_reference(State&);bool ball_action_facing_reference(State&);bool ball_action_area_reference(State&);bool ball_action_elevated_reference(State&);bool ball_action_low_reference(State&);bool ball_action_selected_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace action=cm::aot::ball_action;
constexpr auto match=fixture+0x1000,actor=fixture+0x11000,doubles=fixture+0x12000;
constexpr auto singles=fixture+0x12400,stream=fixture+0x12800,globals=fixture+0x12a00;
struct Options {
    unsigned interrupt=0,force_wide=0;
    unsigned adjustment=0;int adjustment_wide=0;
    bool own_negative=false,child_negative=false,mutate_fitness=false,mutate_owner=false,mutate_facing=false,mutate_area=false;
    bool mutate_elevated=false,zero_elevated_threshold=false,mutate_low=false;
    std::int32_t sampled=9;
    std::int32_t facing_interval=200;
    std::uint32_t random=0xabcd5678,facing_gate=0,area_status=0,lane_clear=0,elevated_preference=0,low_alignment=1;
    double distance=25.25;
};
struct Trace final:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> order;
    explicit Trace(Options o):options(o){}
    void before_step(State& s,std::uint32_t pc)override {
        order.push_back(pc);
        // Signed-halfword fitness cannot naturally select the wide interval.
        // These paired boundary injections cover its dormant original body;
        // they are structural evidence, not gameplay reachability evidence.
        if(options.force_wide==1&&pc==0x1023ce80)s.wr<4>(0,65536);
        if(options.force_wide==2&&pc==0x1023ced4)s.wr<4>(0,std::uint32_t(-65536));
        if(options.force_wide==3&&pc==0x1023d3b0)s.wr<4>(0,65536);
        if(options.force_wide==4&&pc==0x1023d3c4)s.wr<4>(0,std::uint32_t(-65536));
        if(options.force_wide==5&&pc==0x1023d7ac)s.wr<4>(56,65536);
        if(options.force_wide==6&&pc==0x1023d7bc)s.wr<4>(56,std::uint32_t(-65536));
        if(options.force_wide==7&&pc==0x1023db30)s.wr<4>(68,65536);
        if(options.force_wide==8&&pc==0x1023db40)s.wr<4>(68,std::uint32_t(-65536));
        if(options.force_wide==9&&pc==0x1023dcb4)s.wr<4>(68,65536);
        if(options.force_wide==10&&pc==0x1023dcc4)s.wr<4>(68,std::uint32_t(-65536));
        if(options.force_wide==11&&pc==0x1023df40)s.wr<4>(56,65536);
        if(options.force_wide==12&&pc==0x1023df50)s.wr<4>(56,std::uint32_t(-65536));
        if(options.force_wide==13&&pc==0x1023e0b0)s.wr<4>(68,65536);
        if(options.force_wide==14&&pc==0x1023e0c0)s.wr<4>(68,std::uint32_t(-65536));
        if(options.zero_elevated_threshold&&pc==0x1023dc74)s.wr<4>(68,std::uint32_t(-70));
        if(options.adjustment_wide) {
            const auto base=action::selected_action_phase+options.adjustment*0x188;
            if(options.adjustment_wide>0&&pc==base+0x44)s.wr<4>(0,65536);
            if(options.adjustment_wide<0&&pc==base+0x58)s.wr<4>(0,std::uint32_t(-65536));
        }
        if(options.facing_interval!=200&&pc==0x1023d364)s.wr<4>(76,std::uint32_t(options.facing_interval));
    }
};
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;unsigned phase;std::vector<Event> calls;
    Model(bool old,Options value,unsigned part):original(old),options(value),phase(part){}
    void reference(State& s) {
        const auto done=phase>=11?action::contact_selection_phase:phase>=9?action::selected_action_phase:phase>=7?action::low_ball_selection_phase:phase>=5?action::ball_state_selection_phase:phase>=3?action::area_selection_phase:phase?action::action_bias_phase:action::orientation_phase;
        while(s.pc!=done&&!(phase>=5&&phase<=10&&s.pc==action::selected_action_phase)) {
            if(s.pc>=action::entry&&s.pc<action::orientation_phase)
                require(ball_action_preparation_reference(s),"Missing original ball-action preparation");
            else if(s.pc>=action::orientation_phase&&s.pc<action::action_bias_phase)
                require(ball_action_orientation_reference(s),"Missing original ball-action orientation");
            else if(s.pc>=action::action_bias_phase&&s.pc<action::area_selection_phase)
                require(ball_action_facing_reference(s),"Missing original ball-action facing");
            else if(s.pc>=action::area_selection_phase&&s.pc<action::ball_state_selection_phase)
                require(ball_action_area_reference(s),"Missing original ball-action area selection");
            else if(s.pc>=action::ball_state_selection_phase&&s.pc<action::low_ball_selection_phase)
                require(ball_action_elevated_reference(s),"Missing original ball-action elevated selection");
            else if(s.pc>=action::low_ball_selection_phase&&s.pc<action::selected_action_phase)
                require(ball_action_low_reference(s),"Missing original ball-action low-ball selection");
            else if(s.pc>=action::selected_action_phase&&s.pc<action::contact_selection_phase)
                require(ball_action_selected_reference(s),"Missing original ball-action selected-action sampling");
            else call(s,s.pc,std::uint32_t(s.r<4>(4128)));
        }
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        Event event{target,continuation,s.registers,{}};std::uint32_t result=0;
        switch(target) {
        case 0x10245160:
            if(options.mutate_owner)s.memory.write<4>(actor+0x1a4,match+1);
            break;
        case 0x1077b770:result=options.random;break;
        case 0x10226520:result=std::uint32_t(options.sampled);break;
        case 0x10775540: {
            const auto input=std::uint32_t(s.r<4>(12));result=std::bit_cast<std::int32_t>(input)<0?0u-input:input;
            if(options.child_negative&&!original)result^=1;
            break;
        }
        case 0x101d8d70:
            result=options.facing_gate;if(options.child_negative&&!original)result^=1;
            if(options.mutate_facing){s.memory.write<1>(actor+0x106,10);s.memory.write<1>(match+0x791f,3);}
            break;
        case 0x10237830:
            s.memory.write<2>(std::uint32_t(s.r<4>(12))+0x19e,((s.r<4>(24)&255)<<8)|(s.r<4>(28)&255));
            break;
        case 0x101e3d50:
            result=options.area_status;if(options.child_negative&&!original)result^=1;
            if(options.mutate_area){s.memory.write<1>(actor+0x105,0);s.memory.write<1>(actor+0x27,1);}
            break;
        case 0x101e4ed0:
            result=options.lane_clear;
            if(options.mutate_elevated){s.memory.write<1>(actor+0x10f,128);s.memory.write<4>(actor+0x1a4,match+1);}
            break;
        case 0x102875f0:
            result=options.elevated_preference;if(options.child_negative&&!original)result^=1;
            if(options.mutate_elevated){s.memory.write<1>(actor+0x24,128);s.memory.write<1>(actor+0x106,10);}
            break;
        case 0x10244c60:
            result=options.low_alignment;if(options.child_negative&&!original)result^=1;
            if(options.mutate_low){s.memory.write<2>(actor+0x194,32768);s.memory.write<2>(actor+0x136,0);}
            break;
        case 0x10239590: {
            auto bits=std::uint32_t(float_bits<4>(float_value<8>(s.r<8>(0x4010))));
            if(options.child_negative&&!original)bits^=1;
            s.memory.write<4>(std::uint32_t(s.r<4>(12))+0x50,bits);break;
        }
        default:throw std::runtime_error("Unknown ball-action dependency "+hex_address(target));
        }
        for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xac000000+n*17);
        for(unsigned n=0;n<=13;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-11.0-n));
        s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);
        if(target==0x10245160)s.wr<8>(0x4010,float_bits<8>(options.distance+unsigned(options.child_negative&&!original)));
        s.pc=continuation;event.after=s.registers;calls.push_back(std::move(event));
        if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture ball-action dependency interrupted");
    }
};
std::unique_ptr<State> seed() {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x13000);s->memory.map(stack-0x800,0xa00);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,actor);s->wr<4>(16,700);s->wr<4>(20,0x123456ff);s->wr<4>(24,8);
    s->wr<4>(4128,return_sentinel);s->pc=action::entry;
    for(auto [offset,value]:std::array<std::pair<int,unsigned>,6>{{{-0x51e8,doubles},{-0x51e0,singles},
        {-0x7b90,globals},{-0x7b8c,stream},{-0x7b88,globals+4},{-0x7b84,globals+8}}})s->memory.write<4>(toc+offset,value);
    s->memory.write<4>(actor+0x1a4,match);s->memory.write<4>(actor+0x50,float_bits<4>(7500));
    s->memory.write<4>(actor+0x6c,actor+0x300);s->memory.write<1>(actor+0x368,2);
    s->memory.write<1>(match+0x791d,4);s->memory.write<1>(match+0x791e,7);s->memory.write<1>(match+0x791f,2);
    s->memory.write<4>(globals,17);s->memory.write<4>(globals+4,stream+126);s->memory.write<4>(globals+8,stream);
    for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,100+n);
    s->memory.write<8>(doubles,float_bits<8>(65535));s->memory.write<8>(doubles+0x80,float_bits<8>(4503601774854144.0));
    return s;
}
unsigned cases=0,completed=0,interruptions=0,negatives=0,structural=0;
std::array<unsigned,13> phase_cases{},phase_completed{};
std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={},unsigned phase=0) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();
    if(options.mutate_fitness) {
        auto edit=[](State& s,Effects& e,std::uint32_t at,unsigned){if(at==actor+0x50&&!e.mutated){e.mutated=true;s.memory.write<4>(at,float_bits<4>(-32768));}};
        x.on_read=edit;y.on_read=edit;
    }
    a->memory.date_observer=&x;b->memory.date_observer=&y;Trace old_trace(options),new_trace(options);
    Model old(true,options,phase),modern(false,options,phase);
    const auto run=[&](State& s,Model& m,bool original){try{if(original){original_trace=&old_trace;m.reference(s);}else {
            if(phase==0||phase==2||phase==4||phase==6||phase==8||phase==10||phase==12)action::prepare(s,&m,&new_trace);
            if(phase==1||phase==2||phase==4||phase==6||phase==8||phase==10||phase==12)action::orient(s,&m,&new_trace);
            if(phase==3||phase==4||phase==6||phase==8||phase==10||phase==12)action::face_action(s,&m,&new_trace);
            if(phase==5||phase==6||phase==8||phase==10||phase==12)action::select_area_action(s,&m,&new_trace);
            if((phase==7||phase==8||phase==10||phase==12)&&s.pc!=action::selected_action_phase)action::select_elevated_action(s,&m,&new_trace);
            if(phase==9||((phase==10||phase==12)&&s.pc==action::low_ball_selection_phase))action::select_low_ball_action(s,&m,&new_trace);
            if(phase>=11)action::sample_selected_action(s,&m,&new_trace);
        }}
        catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}original_trace=nullptr;return std::string{};};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Ball-action phase="+std::to_string(phase)+" case="+std::to_string(cases);
    require(!expected.starts_with("Unknown ball-action dependency"),expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    if(options.own_negative)b->wr<4>(52,b->r<4>(52)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating negative");
    if(!rejected) {
        require(x.bytes==y.bytes,label+" ordered effects");require(a->executed==b->executed,label+" accounting");
        require(old.calls==modern.calls,label+" dependency ABI");
        if(old_trace.order!=new_trace.order)for(unsigned n=0;n<std::min(old_trace.order.size(),new_trace.order.size());++n)if(old_trace.order[n]!=new_trace.order[n]){
            std::cerr<<"First site "<<hex_address(old_trace.order[n])<<" / "<<hex_address(new_trace.order[n])<<'\n';break;}
        require(old_trace.order==new_trace.order,label+" original-site order");
    }
    ++cases;++phase_cases[phase];phase_completed[phase]+=expected.empty()&&!rejected;
    negatives+=rejected;interruptions+=!expected.empty();structural+=options.force_wide!=0||options.facing_interval!=200||options.zero_elevated_threshold||options.adjustment_wide!=0;
    completed+=expected.empty()&&!rejected;sites.insert(old_trace.order.begin(),old_trace.order.end());
    return {std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={},unsigned phase=0) {
    const auto baseline=compare(input,options,phase);const auto steps=baseline.state->executed-input.executed;
    for(std::uint64_t n=0;n<=steps;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s,options,phase);}
    std::set<std::uint32_t> bytes;for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(auto hole:bytes) {
        auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;
        for(auto b:bytes)if(b!=hole){s->memory.map(b,1);s->memory.write<1>(b,input.memory.read<1>(b));}
        compare(*s,options,phase);
    }
    for(unsigned n=1;n<=baseline.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o,phase);}
}
std::unique_ptr<State> orientation_seed() {
    auto s=seed();s->pc=action::orientation_phase;s->wr<4>(4,stack-0x150);s->wr<4>(60,actor);s->wr<4>(52,117);
    s->wr<4>(64,700);s->wr<4>(88,2);s->wr<4>(104,doubles);s->wr<4>(108,globals);s->wr<4>(112,stream);
    s->wr<4>(116,globals+4);s->wr<4>(120,globals+8);s->wr<4>(124,singles);
    s->memory.write<2>(actor+0x19e,30);s->memory.write<2>(match+0x7950,10);s->memory.write<1>(match+0x7920,2);
    s->memory.write<1>(match+1,2);return s;
}
std::unique_ptr<State> facing_seed() {
    auto s=orientation_seed();s->pc=action::action_bias_phase;s->wr<4>(52,20);s->wr<4>(68,5);s->wr<4>(72,8);
    s->wr<8>(0x4000+29*16,float_bits<8>(25.25));s->memory.write<8>(doubles+0xc8,float_bits<8>(10));
    s->memory.write<1>(actor+0x10f,2);s->memory.write<1>(actor+0x108,3);s->memory.write<1>(actor+0x27,0);
    s->memory.write<1>(actor+0x105,4);s->memory.write<1>(actor+0x106,6);s->memory.write<4>(match+0xdff8,actor+0x200);
    return s;
}
std::unique_ptr<State> area_seed() {
    auto s=facing_seed();s->pc=action::area_selection_phase;s->memory.write<1>(actor+0x106,1);
    s->memory.write<4>(actor+0x6c,actor+0x300);s->memory.write<1>(actor+0x368,2);
    s->memory.write<2>(actor + 0x11e + 37*2,0x8765);return s;
}
std::unique_ptr<State> elevated_seed() {
    auto s=area_seed();s->pc=action::ball_state_selection_phase;s->wr<4>(88,4);s->memory.write<1>(actor+0x24,2);
    for(auto kind:{12u,21u,46u})s->memory.write<2>(actor + 0x11e + kind*2,0x8000+kind);
    return s;
}
std::unique_ptr<State> low_seed() {
    auto s=elevated_seed();s->pc=action::low_ball_selection_phase;s->wr<4>(88,2);
    s->memory.write<1>(actor+0x106,6);s->memory.write<2>(actor+0x194,200);s->memory.write<2>(actor+0x136,2);
    s->memory.write<2>(actor+0x138,0xcfc7);s->memory.write<4>(actor+0x9c,float_bits<4>(20));
    s->memory.write<8>(doubles+0x328,float_bits<8>(50));s->memory.write<4>(singles,float_bits<4>(1));return s;
}
std::unique_ptr<State> selected_seed() {
    auto s=low_seed();s->pc=action::selected_action_phase;s->wr<4>(52,12);s->wr<4>(68,200);s->wr<4>(64,700);
    s->wr<4>(84,117);s->wr<4>(80,88);s->memory.write<4>(singles+0xe0,float_bits<4>(0.5));return s;
}
}
namespace cm::aot {void ball_action_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try {
    auto initial=seed();boundaries(*initial);
    for(auto fitness:{0.0,1.0,7499.99,7500.0,9999.0,10000.0,10001.0,32767.0,32768.0,65535.0,65536.0,-1.0,-32768.0,-32769.0,1.0e30,-1.0e30})
        for(auto random:{0u,17u,0x7fffffffu,0x80000000u,0xffffffffu}) {
            auto s=seed();s->memory.write<4>(actor+0x50,float_bits<4>(fitness));s->memory.write<4>(globals,random);
            compare(*s);
        }
    for(auto bits:{0x7fc12345u,0x7f812345u,0xffc12345u,0x7f800000u,0xff800000u,0x80000000u}){
        auto s=seed();s->memory.write<4>(actor+0x50,bits);compare(*s);
    }
    for(unsigned coord=0;coord<256;++coord){auto s=seed();s->wr<4>(20,0xabcd0000|coord);s->wr<4>(24,0xfedc0000|(255-coord));
        auto result=compare(*s);require(result.state->r<4>(68)==(coord==255?4:(0xabcd0000|coord)),"Low-byte coordinate sentinel");}
    for(auto cursor:{stream,stream+124,stream+126,stream+128,0xfffffffeu})for(auto random:{0u,0xabcd5678u,0xffffffffu}){
        auto s=seed();s->memory.write<4>(globals+8,cursor);Options o;o.random=random;compare(*s,o);
    }
    {auto s=seed();s->memory.write<4>(actor+0x50,float_bits<4>(10000));boundaries(*s);
        require(compare(*s).calls.size()==1,"Zero fitness interval consumes no random sample");}
    {auto s=seed();s->memory.write<4>(globals+8,stream+126);boundaries(*s);}
    for(unsigned wide:{1u,2u})for(auto sampled:{-100000,0,9,1000000}) {
        auto s=seed();Options o;o.force_wide=wide;o.sampled=sampled;compare(*s,o);
        if(sampled==1000000)boundaries(*s,o);
    }
    for(auto value:{0.0,-1.0,1.0e100}){auto s=seed();s->memory.write<8>(doubles,float_bits<8>(value));Options o;o.force_wide=1;compare(*s,o);}
    for(auto fitness:{-32768.0,7500.0,32767.0}){auto s=seed();s->memory.write<4>(actor+0x50,float_bits<4>(fitness));Options o;o.mutate_fitness=true;boundaries(*s,o);}
    {Options o;o.mutate_owner=true;boundaries(*initial,o);}
    for(auto ptr:{0u,actor+1,stack-0x108,stack-0x150,stack+8}){auto s=seed();s->wr<4>(12,ptr);compare(*s);}
    for(auto sp:{stack+1,stack+8,globals+0x150,actor+0x6c}){auto s=seed();s->wr<4>(4,sp);compare(*s);}
    for(auto offset:{-0x51e8,-0x51e0,-0x7b90,-0x7b8c,-0x7b88,-0x7b84})for(auto ptr:{0u,stack-0x150,globals+8}){
        auto s=seed();s->memory.write<4>(toc+offset,ptr);compare(*s);
    }
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*initial,o);}
    for(auto surface:{0u,1u,2u,31u,32u,33u,127u,128u,255u})for(auto height:{0u,1u,2u,3u,127u,128u,129u,255u})
        for(auto heading:{0u,20u,89u,90u,91u,180u,181u,359u,360u,32767u,32768u,65535u}) {
            auto s=orientation_seed();s->memory.write<1>(match+1,surface);s->memory.write<1>(match+0x7920,height);
            s->memory.write<2>(actor+0x19e,heading);s->memory.write<2>(match+0x7950,0);
            const auto result=compare(*s,{},1);
            auto angle=std::int32_t(signed_value<2>(heading));if(angle<0)angle=-angle;
            angle=std::int32_t(signed_value<2>(std::uint32_t(angle)));
            if(angle>180)angle=std::int32_t(signed_value<2>(std::uint32_t(360-angle)));
            auto bias=std::int32_t(signed_value<2>(std::uint32_t(360-2*angle)))/3;
            bias=std::int32_t(signed_value<2>(std::uint32_t(bias)));
            bias=std::int32_t(signed_value<2>(std::uint32_t(bias*(angle<90?11-angle/10:2))));
            require(result.state->r<4>(80)==std::uint32_t(bias),"Independent orientation bias with signed narrowing");
            const auto h=std::int32_t(signed_value<1>(height-1));
            const auto cube=std::int32_t(signed_value<2>(std::uint32_t(h*h*h*20)));
            const auto surface_penalty=std::int32_t(signed_value<2>(std::uint32_t(h*(surface==2?119%3+1:surface==32?119%8+3:0))));
            require(result.state->r<4>(64)==std::uint32_t(700-cube-surface_penalty),"Independent height and surface adjustment");
        }
    for(unsigned kind=0;kind<256;++kind){auto s=orientation_seed();s->wr<4>(88,kind|0xabcdef00);compare(*s,{},1);}
    for(auto surface:{0u,2u,32u,33u})for(auto heading:{20u,90u,270u})for(bool wrap:{false,true}){
        auto s=orientation_seed();s->memory.write<1>(match+1,surface);s->memory.write<2>(actor+0x19e,heading);
        if(wrap)s->memory.write<4>(globals+8,stream+126);
        boundaries(*s,{},1);
    }
    for(auto surface:{2u,32u})for(auto random:{0u,0x7fffffffu,0x80000000u,0xfffffff0u,0xffffffffu}) {
        auto s=orientation_seed();s->memory.write<1>(match+1,surface);s->memory.write<4>(globals,random);compare(*s,{},1);
    }
    for(auto ptr:{0u,actor+1,stack-0x150,globals}){auto s=orientation_seed();s->wr<4>(60,ptr);compare(*s,{},1);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*orientation_seed(),o,1);}
    for(auto surface:{0u,2u,32u})for(auto fitness:{7500,10000,-32768})for(auto heading:{20u,90u,270u}){
        auto s=seed();s->memory.write<1>(match+1,surface);s->memory.write<1>(match+0x7920,2);s->memory.write<2>(actor+0x19e,heading);
        s->memory.write<4>(actor+0x50,float_bits<4>(fitness));compare(*s,{},2);
    }
    {auto s=seed();s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);s->memory.write<4>(globals+8,stream+124);boundaries(*s,{},2);}
    for(auto side:{0u,1u,2u,128u,255u})for(auto column:{0u,2u,3u,4u,8u,9u,10u,11u,127u,128u,255u})
        for(auto trait:{0u,2u,20u,127u,128u,255u})for(auto gate:{0u,1u,0x100u,0x1ffu}) {
            auto s=facing_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x106,column);
            s->memory.write<1>(actor+0x10f,trait);Options o;o.facing_gate=gate;
            auto result=compare(*s,o,3);
            const bool attack=std::int32_t(signed_value<1>(trait))*3>119||(gate&255)!=0;
            const auto destination=attack?(side?std::max(std::int32_t(signed_value<1>(column))-3,0):
                std::int32_t(signed_value<1>(column))>=9?11:std::int32_t(signed_value<1>(column))+3):8;
            require(result.state->memory.read<2>(actor+0x19e)==(((attack?4u:5u)<<8)|(std::uint32_t(destination)&255)),"Independent facing destination and low-byte gate");
        }
    for(auto angle:{0u,90u,91u,180u,32767u,32768u,0xffffu}){auto s=facing_seed();s->wr<4>(52,angle);compare(*s,{},3);}
    for(auto height:{0u,2u,3u,4u,5u,127u,128u,255u})for(auto distance:{-1.0,9.99,10.0,10.01,1.0e100}){
        auto s=facing_seed();s->memory.write<1>(match+0x791f,height);s->wr<8>(0x4000+29*16,float_bits<8>(distance));compare(*s,{},3);
    }
    for(auto bits:{0x7ff8000000001234ull,0x7ff0000000001234ull,0x7ff0000000000000ull,0xfff0000000000000ull}){
        auto s=facing_seed();s->wr<8>(0x4000+29*16,bits);compare(*s,{},3);
    }
    for(auto side:{0u,1u})for(auto column:{2u,6u,10u})for(bool wrap:{false,true}) {
        auto s=facing_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x106,column);
        if(wrap)s->memory.write<4>(globals+8,stream+126);
        Options o;o.facing_gate=1;boundaries(*s,o,3);
    }
    for(unsigned wide:{3u,4u})for(auto sampled:{-10000,0,100000}){
        Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*facing_seed(),o,3);
    }
    for(auto interval:{0,-1,-32768,32767}){Options o;o.facing_interval=interval;boundaries(*facing_seed(),o,3);}
    {auto s=facing_seed();s->memory.write<1>(match+0x791f,4);boundaries(*s,{},3);}
    for(auto gate:{0u,1u}){Options o;o.facing_gate=gate;o.mutate_facing=true;boundaries(*facing_seed(),o,3);}
    for(auto ptr:{0u,actor+1,stack-0x150,globals}){auto s=facing_seed();s->wr<4>(60,ptr);compare(*s,{},3);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*facing_seed(),o,3);}
    for(auto surface:{0u,2u,32u})for(auto fitness:{7500,10000,-32768})for(auto heading:{20u,90u,270u}){
        auto s=seed();s->memory.write<1>(match+1,surface);s->memory.write<1>(match+0x7920,2);s->memory.write<2>(actor+0x19e,heading);
        s->memory.write<4>(actor+0x50,float_bits<4>(fitness));compare(*s,{},4);
    }
    {auto s=seed();s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);s->memory.write<4>(globals+8,stream+124);boundaries(*s,{},4);}
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{1u,2u,4u,6u,7u,127u,128u,255u})
        for(auto column:{0u,1u,2u,9u,10u,11u,12u,127u,128u,255u})for(auto trait:{0u,1u,127u,128u,253u,255u}) {
            auto s=area_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);
            s->memory.write<1>(actor+0x106,column);s->memory.write<1>(actor+0x368,trait);
            auto result=compare(*s,{},5);
            const bool inside=row>=2&&row<=6&&(side==1?(column>=10&&column<=11):column<=1);
            const auto interval=std::int32_t(signed_value<1>(trait))*200+int(inside)*500+100;
            const auto sample=interval==0?0:interval>0?119%interval:-(119%interval);
            require(result.state->pc==(sample==0?action::selected_action_phase:action::ball_state_selection_phase),"Independent area reaction selection");
            if(sample==0)require(result.state->r<4>(52)==37&&result.state->r<4>(68)==0xffff8765,"Selected action reads its signed attribute");
            else require(result.state->r<4>(52)==std::uint32_t(sample),"Independent signed area sample");
        }
    for(auto side:{0u,1u})for(auto status:{0u,1u,2u,0xffffu,0x10001u}){
        auto s=area_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x106,side?10:1);
        Options o;o.area_status=status;boundaries(*s,o,5);
    }
    for(auto row:{1u,4u,7u})for(auto col:{0u,2u,10u,12u}){
        auto s=area_seed();s->memory.write<1>(actor+0x27,1);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,col);boundaries(*s,{},5);
    }
    for(auto trait:{0u,128u,253u})for(auto draw:{0u,100u,65535u}) {
        auto s=area_seed();s->memory.write<1>(actor+0x368,trait);s->memory.write<4>(globals,0);s->memory.write<2>(stream+2,draw);boundaries(*s,{},5);
    }
    for(unsigned wide:{5u,6u})for(auto sampled:{-100000,0,10000000}){Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*area_seed(),o,5);}
    {auto s=area_seed();s->memory.write<4>(globals+8,stream+126);boundaries(*s,{},5);}
    for(auto random:{0u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=area_seed();s->memory.write<4>(globals,random);compare(*s,{},5);}
    {Options o;o.mutate_area=true;boundaries(*area_seed(),o,5);}
    for(auto ptr:{0u,actor+1,stack-0x150,globals}){auto s=area_seed();s->wr<4>(60,ptr);compare(*s,{},5);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*area_seed(),o,5);}
    for(auto surface:{0u,2u,32u})for(auto fitness:{7500,10000,-32768})for(auto heading:{20u,90u,270u}){
        auto s=seed();s->memory.write<1>(match+1,surface);s->memory.write<1>(match+0x7920,2);s->memory.write<2>(actor+0x19e,heading);
        s->memory.write<4>(actor+0x50,float_bits<4>(fitness));compare(*s,{},6);
    }
    {auto s=seed();s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);s->memory.write<4>(globals+8,stream+124);boundaries(*s,{},6);}
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{1u,2u,6u,7u})for(auto column:{0u,1u,2u,9u,10u,11u,12u})
        for(auto trait:{0u,2u,128u,255u})for(auto preference:{0u,1u,20u,127u,128u,255u}) {
            auto s=elevated_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);
            s->memory.write<1>(actor+0x106,column);s->memory.write<1>(actor+0x24,trait);Options o;o.elevated_preference=preference;
            auto result=compare(*s,o,7);
            const bool inside=row>=2&&row<=6&&(side==1?(column>=10&&column<=11):column<=1);
            const auto first=std::int32_t(signed_value<1>(preference));
            const auto first_sample=first==0?0:first>0?119%first:-(119%first);
            const auto interval=int(inside)*1500+std::int32_t(signed_value<1>(trait))*80+70;
            const auto draw=first==0?119:121;
            const auto second_sample=interval>0?draw%interval:-(draw%interval);
            const auto kind=first_sample>second_sample?46u:21u;
            require(result.state->r<4>(52)==kind&&result.state->r<4>(68)==0xffff8000u+kind,"Independent elevated preference and threshold selection");
        }
    for(unsigned state=0;state<256;++state){auto s=elevated_seed();s->wr<4>(88,state|0x12340000);
        auto result=compare(*s,{},7);require(result.state->pc==(std::int32_t(signed_value<1>(state))<4?action::low_ball_selection_phase:action::selected_action_phase),"Signed ball-state selection");}
    for(auto side:{0u,1u,2u,255u})for(auto col:{0u,1u,6u,10u,11u})for(auto control:{0u,125u,126u,127u,0xffffu})for(auto gate:{0u,1u,0x100u,0x1ffu}) {
        auto s=elevated_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x106,col);s->wr<4>(64,control);
        Options o;o.lane_clear=gate;auto result=compare(*s,o,7);
        const bool opposite=side==0?col>=10&&col<=11:col<=1;
        const bool clear=!opposite&&std::int32_t(signed_value<2>(control))>126&&(gate&255)!=0;
        require((result.state->r<4>(52)==12)==clear,"Independent clear-lane action gate");
        if(clear)require(result.state->r<4>(92)==1&&result.state->r<4>(88)==2&&result.state->memory.read<1>(match+0x791f)==2,"Clear-lane action updates live ball state");
    }
    for(auto side:{0u,1u})for(auto col:{0u,6u,10u})for(auto preference:{0u,20u,255u}){
        auto s=elevated_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x106,col);
        Options o;o.elevated_preference=preference;boundaries(*s,o,7);
    }
    for(auto gate:{0u,1u})for(bool mutation:{false,true}){Options o;o.lane_clear=gate;o.mutate_elevated=mutation;boundaries(*elevated_seed(),o,7);}
    for(unsigned wide:{7u,8u,9u,10u})for(auto sampled:{-100000,0,10000000}){
        Options o;o.force_wide=wide;o.sampled=sampled;o.elevated_preference=20;boundaries(*elevated_seed(),o,7);
    }
    {Options o;o.zero_elevated_threshold=true;boundaries(*elevated_seed(),o,7);}
    for(auto cursor:{stream+124,stream+126}){auto s=elevated_seed();s->memory.write<4>(globals+8,cursor);Options o;o.elevated_preference=20;boundaries(*s,o,7);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*elevated_seed(),o,7);}
    for(auto state:{2u,4u})for(auto fitness:{7500,10000,-32768})for(auto heading:{20u,90u,270u}){
        auto s=seed();s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);s->memory.write<2>(actor+0x19e,heading);
        s->memory.write<4>(actor+0x50,float_bits<4>(fitness));s->memory.write<1>(match+0x791f,state);compare(*s,{},8);
    }
    {auto s=seed();s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);s->memory.write<4>(globals+8,stream+124);
        s->memory.write<1>(match+0x791f,4);boundaries(*s,{},8);}
    for(unsigned state=0;state<256;++state){auto s=low_seed();s->wr<4>(88,state|0xabcd0000);auto result=compare(*s,{},9);
        if(state!=2&&state!=3)require(result.state->r<4>(52)==13&&result.state->r<4>(68)==0xffffcfc7,"Other low-ball states select action 13");}
    for(auto side:{0u,1u,2u,128u,255u})for(auto row:{2u,3u,4u,5u,6u,128u})for(auto col:{0u,1u,6u,11u,12u,128u})
        for(auto gate:{0u,1u,0x100u,0x1ffu}) {
            auto s=low_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,col);
            Options o;o.low_alignment=gate;auto result=compare(*s,o,9);
            const bool goal=row>=3&&row<=5&&col==(side?0u:11u);
            require(result.state->r<4>(52)==(goal||(gate&255)?59u:12u),"Goal cell or low-byte alignment permits the attempt");
        }
    for(auto first:{0,1,2,119,120,200,32767,-1,-32768})for(auto second:{0,1,2,119,120,200,32767,-1,-32768})for(auto rating:{0,1,2,4,5,20}){
        auto s=low_seed();s->memory.write<2>(actor+0x194,std::uint32_t(first));s->memory.write<2>(actor+0x136,std::uint32_t(second));
        s->memory.write<4>(actor+0x9c,float_bits<4>(rating));auto result=compare(*s,{},9);
        int draws=0;
        const auto draw=[&](int interval){if(!interval)return 0;const int number=117+2*++draws;return interval>0?number%interval:-(number%interval);};
        const auto a=draw(first),b=draw(second);unsigned kind=12;
        if(a>b){const auto sampled=draw(rating);const auto threshold=(117+2*++draws)%5;if(sampled>=threshold)kind=59;}
        require(result.state->r<4>(52)==kind,"Independent low-ball competing samples and attempt threshold");
        require(result.state->r<4>(68)==std::uint32_t(kind==59?first:second),"Low-ball selected attribute aliases its earlier interval");
    }
    for(auto distance:{-1.0,0.0,49.999,50.0,50.001})for(auto limit:{-1.0,0.0,50.0}){
        auto s=low_seed();s->wr<8>(0x4000+29*16,float_bits<8>(distance));s->memory.write<8>(doubles+0x328,float_bits<8>(limit));compare(*s,{},9);
    }
    for(auto bits:{0x7ff8000000001234ull,0x7ff0000000001234ull,0x7ff0000000000000ull,0xfff0000000000000ull}){
        auto s=low_seed();s->wr<8>(0x4000+29*16,bits);compare(*s,{},9);s->wr<8>(0x4000+29*16,float_bits<8>(25));s->memory.write<8>(doubles+0x328,bits);compare(*s,{},9);
    }
    for(auto side:{0u,1u})for(auto row:{2u,4u,6u})for(auto col:{0u,6u,11u}){
        auto s=low_seed();s->memory.write<1>(actor+0x27,side);s->memory.write<1>(actor+0x105,row);s->memory.write<1>(actor+0x106,col);boundaries(*s,{},9);
    }
    for(auto first:{0,200,-1})for(auto second:{0,2,-1}){auto s=low_seed();s->memory.write<2>(actor+0x194,std::uint32_t(first));
        s->memory.write<2>(actor+0x136,std::uint32_t(second));boundaries(*s,{},9);}
    for(auto rating:{0.0,0.5,-1.0,2.0,65535.0,65536.0,-65535.0,-65536.0,2147483520.0,-2147483648.0,1.0e30}){
        auto s=low_seed();s->memory.write<4>(actor+0x9c,float_bits<4>(rating));boundaries(*s,{},9);
    }
    for(auto bits:{0x7fc12345u,0x7f812345u,0xffc12345u,0x7f800000u,0xff800000u}){auto s=low_seed();s->memory.write<4>(actor+0x9c,bits);compare(*s,{},9);}
    for(unsigned wide:{11u,12u,13u,14u})for(auto sampled:{-100000,0,10000000}){Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*low_seed(),o,9);}
    for(auto sampled:{-100000,10000000}){auto s=low_seed();s->memory.write<4>(actor+0x9c,float_bits<4>(65536));Options o;o.sampled=sampled;boundaries(*s,o,9);}
    for(auto cursor:{stream+120,stream+122,stream+124,stream+126}){auto s=low_seed();s->memory.write<4>(globals+8,cursor);boundaries(*s,{},9);}
    for(auto random:{0u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=low_seed();s->memory.write<4>(globals,random);compare(*s,{},9);}
    {Options o;o.mutate_low=true;boundaries(*low_seed(),o,9);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*low_seed(),o,9);}
    for(auto state:{0u,2u,3u,4u})for(auto fitness:{7500,10000,-32768})for(auto heading:{20u,90u,270u}){
        auto s=low_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);
        s->memory.write<2>(actor+0x19e,heading);s->memory.write<4>(actor+0x50,float_bits<4>(fitness));s->memory.write<1>(match+0x791f,state);compare(*s,{},10);
    }
    {auto s=low_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<4>(globals+8,stream+120);boundaries(*s,{},10);}
    for(auto first:{0u,1u,200u,32768u,65535u})for(auto control:{0u,1u,700u,32768u,65535u})
        for(auto fatigue:{0u,1u,117u,65535u})for(auto orientation:{0u,1u,88u,65535u}) {
            auto s=selected_seed();s->wr<4>(68,first|0xabcd0000);s->wr<4>(64,control|0xef120000);
            s->wr<4>(84,fatigue|0x34560000);s->wr<4>(80,orientation|0x789a0000);auto result=compare(*s,{},11);
            int draws=0;const auto draw=[&](unsigned value){const auto interval=std::int32_t(signed_value<2>(value));
                if(!interval)return 0;const auto number=117+2*++draws;return interval>0?number%interval:-(number%interval);};
            const auto a=draw(first),b=draw(control),c=draw(fatigue),d=draw(orientation);
            const auto expected=std::uint32_t(signed_value<2>(std::uint32_t(a+b-c-d)));
            require(result.state->r<4>(68)==expected,"Independent selected-action four-term adjustment");
            require(result.state->memory.read<4>(actor+0x50)==float_bits<4>(0.5),"Selected-action fitness dependency consumes the live constant");
        }
    for(unsigned kind=0;kind<256;++kind){auto s=selected_seed();s->wr<4>(52,kind|0xabcd0000);compare(*s,{},11);}
    for(unsigned condition=0;condition<256;++condition)for(unsigned overflow:{0u,1u})for(auto interval:{0u,1u,0x8000u}){
        auto s=selected_seed();s->wr<1>(2304,condition);s->wr<1>(1024,overflow);s->wr<4>(68,interval);
        s->budget=1;compare(*s,{},11);
    }
    for(auto reg:{17u,16u,21u,20u})for(auto value:{0u,1u,0xffffu,0x8000u,0x10000u,0x80000000u,0xffffffffu}){
        auto s=selected_seed();s->wr<4>(reg*4,value);boundaries(*s,{},11);
    }
    {auto s=selected_seed();for(auto reg:{17u,16u,21u,20u})s->wr<4>(reg*4,0);boundaries(*s,{},11);}
    for(unsigned adjustment=0;adjustment<4;++adjustment)for(auto wide:{-1,1})for(auto sampled:{-100000,0,10000000}){
        Options o;o.adjustment=adjustment;o.adjustment_wide=wide;o.sampled=sampled;boundaries(*selected_seed(),o,11);
    }
    for(auto cursor:{stream+120,stream+122,stream+124,stream+126}){auto s=selected_seed();s->memory.write<4>(globals+8,cursor);boundaries(*s,{},11);}
    for(auto random:{0u,0x7fffffffu,0x80000000u,0xffffffffu}){auto s=selected_seed();s->memory.write<4>(globals,random);compare(*s,{},11);}
    for(auto ptr:{0u,actor+1,stack-0x150,globals}){auto s=selected_seed();s->wr<4>(60,ptr);compare(*s,{},11);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*selected_seed(),o,11);}
    for(auto state:{0u,2u,3u,4u})for(auto fitness:{7500,10000,-32768})for(auto heading:{20u,90u,270u}){
        auto s=low_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<1>(match+1,32);s->memory.write<1>(match+0x7920,2);
        s->memory.write<2>(actor+0x19e,heading);s->memory.write<4>(actor+0x50,float_bits<4>(fitness));s->memory.write<1>(match+0x791f,state);compare(*s,{},12);
    }
    {auto s=low_seed();s->pc=action::entry;s->wr<4>(4,stack);s->memory.write<4>(globals+8,stream+120);boundaries(*s,{},12);}
    for(auto pc=action::entry;pc<action::contact_selection_phase;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered "<<hex_address(pc)<<'\n';
    require(sites.size()==1865,"All original ball-action preparation, initial selection and adjustment sites covered");
    for(unsigned p=0;p<phase_cases.size();++p){require(phase_completed[p]>0,"Each ball-action phase completed");
        std::cout<<"Phase "<<p<<": "<<phase_cases[p]<<" comparisons, "<<phase_completed[p]<<" completions\n";}
    std::cout<<"PASS: "<<cases<<" ball-action component comparisons; "<<completed<<" phase completions, "<<interruptions
        <<" interruptions, "<<negatives<<" own/child negatives; "<<structural<<" dormant-branch structural cases; all "<<sites.size()
        <<" original sites; complete state/effects/ABI/accounting; production unchanged\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
