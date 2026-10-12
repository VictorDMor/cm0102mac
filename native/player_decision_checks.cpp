#include "player_decision_test_trace.hpp"
#include <fstream>
using namespace cm::player_decision_checks;

namespace {
constexpr std::uint32_t owner=fixture+0x1000, subject=owner+0x311c+8*0x1c4, opponent=owner+0x546c+8*0x1c4;
constexpr std::uint32_t constants=fixture+0x22000, singles=fixture+0x24000, stream=fixture+0x26000, output=fixture+0x28000;
struct Options {
    std::uint32_t role=4,flags=0,high_flags=0,low_flags=0,jitter=0;
    std::int32_t can_act=0,available=-1,offside=0,mark_gate=0,protect=0,special=1;
    double distance=1;
    bool missing_keeper=false,child_negative=false;
    unsigned fault_call=0;
};
struct DependencyEvent {
    std::uint32_t target,continuation;
    decltype(State::registers) before,after;
    bool operator==(const DependencyEvent&)const=default;
};
struct Model final:SemanticDependencyDispatch {
    Options options;bool original;
    std::vector<DependencyEvent> events;
    Model(Options o,bool old):options(o),original(old){}
    void reference(State& s,std::uint32_t end_pc) {
        while(s.pc!=end_pc)if(!player_decision_reference(s))call(s,s.pc,std::uint32_t(s.r<4>(4128)));
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        const auto args=std::array<std::uint32_t,8>{std::uint32_t(s.r<4>(12)),std::uint32_t(s.r<4>(16)),std::uint32_t(s.r<4>(20)),std::uint32_t(s.r<4>(24)),std::uint32_t(s.r<4>(28)),std::uint32_t(s.r<4>(32)),std::uint32_t(s.r<4>(36)),std::uint32_t(s.r<4>(40))};
        events.push_back({target,continuation,s.registers,{}});
        const auto index=events.size()-1;
        std::uint32_t result=0;
        switch(target){
        case 0x101e3d50:case 0x10289560:result=options.role;break;
        case 0x101149f0:result=options.flags;break;
        case 0x10113e10:result=options.high_flags;break;
        case 0x101f5280:result=std::uint32_t(options.can_act);break;
        case 0x101e4ea0:result=std::uint32_t(options.offside);break;
        case 0x101e4e10:result=std::uint32_t(options.available);break;
        case 0x101ee9f0:result=std::uint32_t(options.mark_gate);break;
        case 0x101f0dd0:result=std::uint32_t(options.special);break;
        case 0x101eea30:result=std::uint32_t(options.protect);break;
        case 0x101f1e10:result=options.missing_keeper?0:owner+0x311c;break;
        case 0x10775540:result=std::bit_cast<std::int32_t>(args[0])<0?0u-args[0]:args[0];break;
        case 0x10226520:result=options.jitter;break;
        case 0x1077b770:result=0x1234;break;
        case 0x101e25a0:case 0x101e28b0:s.memory.write<1>(args[2],7);s.memory.write<1>(args[3],6);break;
        case 0x102857c0:result=args[1];break;
        case 0x101e3f10:case 0x101ef8d0:break;
        case 0x1022e7e0:case 0x102374f0:case 0x10237830:case 0x10201180:case 0x1022e3a0:case 0x10239e50:
        case 0x10739890:case 0x10768810:case 0x10769b60:case 0x1077b720:
            s.memory.write<4>(output+(index%32)*4,target);break;
        case 0x1076f7d0:throw std::runtime_error("Fixture decision termination");
        default:throw std::runtime_error("Unknown decision fixture dependency "+hex_address(target));
        }
        for(unsigned r=0;r<=12;++r)if(r!=1 && r!=2)s.wr<4>(r*4,0xbad00000+r*73);
        s.wr<4>(12,result);
        if(target==0x10113e10)s.wr<4>(16,options.low_flags);
        if(target==0x101e3f10 || target==0x101ef8d0)s.wr<8>(0x4010,float_bits<8>(options.distance));
        if(!original && options.child_negative && target==0x101e3d50)s.wr<4>(12,result^1);
        s.pc=continuation;events[index].after=s.registers;
        if(options.fault_call && events.size()==options.fault_call)throw std::runtime_error("Fixture decision dependency interruption");
    }
};
std::unique_ptr<State> seed() {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x50000);s->memory.map(stack-0x800,0x900);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,subject);s->wr<4>(4128,return_sentinel);s->pc=entry;
    for(auto [offset,value]:std::array<std::pair<std::int32_t,std::uint32_t>,9>{{{-0x7ba8,fixture+0x20000},{-0x6424,fixture+0x21000},{-0x51e8,constants},{-0x51e4,fixture+0x23000},{-0x51e0,singles},{-0x7b90,fixture+0x25000},{-0x7b8c,stream},{-0x7b88,fixture+0x25004},{-0x7b84,fixture+0x25008}}})s->memory.write<4>(toc+std::uint32_t(offset),value);
    s->memory.write<4>(fixture+0x25000,17);s->memory.write<4>(fixture+0x25004,stream+0x1fe);s->memory.write<4>(fixture+0x25008,stream);
    for(unsigned n=0;n<512;n+=2)s->memory.write<2>(stream+n,100);
    for(unsigned n=0;n<4096;n+=4)s->memory.write<4>(fixture+0x21000+n,float_bits<4>(2.0));
    for(auto [offset,value]:std::array<std::pair<unsigned,double>,7>{{{0,65535},{0x28,20},{0x80,4503601774854144.0},{0x88,2},{0x90,4},{0xc0,0.5},{0x3a0,1.5}}})s->memory.write<8>(constants+offset,float_bits<8>(value));
    s->memory.write<4>(singles,float_bits<4>(0));s->memory.write<4>(singles+0xc8,float_bits<4>(12));s->memory.write<4>(singles+0x2c,float_bits<4>(3));
    for(unsigned side=0;side<2;++side)for(unsigned slot=0;slot<20;++slot){const auto p=owner+0x311c+side*0x2350+slot*0x1c4;
        s->memory.write<4>(p+0x1a4,owner);s->memory.write<1>(p+0x27,side);s->memory.write<1>(p+0x19,slot);s->memory.write<1>(p+0x105,4);s->memory.write<1>(p+0x106,6);
        for(auto offset:{0x108u,0x109u,0x10au,0x10du,0x10eu,0x110u})s->memory.write<1>(p+offset,10);
        s->memory.write<4>(p+0xe0,float_bits<4>(10));s->memory.write<4>(p+0xe4,float_bits<4>(10));s->memory.write<4>(p+0x1a8,float_bits<4>(100));s->memory.write<4>(p+0xb0,float_bits<4>(20));
    }
    s->memory.write<1>(owner+0x791d,4);s->memory.write<1>(owner+0x791e,6);s->memory.write<1>(owner+0x7925,4);s->memory.write<1>(owner+0x7926,7);
    s->memory.write<1>(owner+0x77bc,11);s->memory.write<1>(owner+0x77bd,11);s->memory.write<4>(owner+0xe000,opponent);
    return s;
}
unsigned component_cases=0,component_returns=0,component_interruptions=0,component_negatives=0;
std::set<std::uint32_t> component_sites;
std::vector<Effects::Access> last_accesses;
void compare_component(const State& input,const Options& options={},bool negative=false) {
    auto a=clone(input),b=clone(input);Model old(options,true),modern(options,false);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace original_trace,semantic_trace;
    auto execute=[](State&s,Model&m,bool previous,Trace&t){TraceScope scope(t);try{if(previous)m.reference(s,return_sentinel);else cm::aot::player_decision::decide(s,&m,&probe);}catch(const std::exception&e){return std::string(e.what());}return std::string{};};
    const auto ae=execute(*a,old,true,original_trace),be=execute(*b,modern,false,semantic_trace);a->memory.date_observer=b->memory.date_observer=nullptr;
    if(negative)b->wr<4>(12,b->r<4>(12)^1);
    const auto label="decision component "+std::to_string(component_cases);
    bool rejected=false;
    try{
        require(ae==be,label+" original="+ae+" cpp="+be);Image(*a).compare(*b,label);
        require(x.bytes==y.bytes && a->executed==b->executed,label+" effects/accounting");
        if(original_trace.ordered!=semantic_trace.ordered){auto m=std::mismatch(original_trace.ordered.begin(),original_trace.ordered.end(),semantic_trace.ordered.begin(),semantic_trace.ordered.end());std::cerr<<label<<" checkpoint "<<m.first-original_trace.ordered.begin()<<" original="<<(m.first==original_trace.ordered.end()?"end":hex_address(*m.first))<<" cpp="<<(m.second==semantic_trace.ordered.end()?"end":hex_address(*m.second))<<'\n';throw std::runtime_error(label+" exact checkpoint sequence");}
        require(old.events==modern.events,label+" full dependency entry/return ABI");
    }catch(const std::runtime_error&){if(!negative&&!options.child_negative)throw;rejected=true;}
    require(rejected==(negative||options.child_negative),label+" own/child negative");
    require(!ae.starts_with("Unknown decision fixture"),label+" incomplete model");
    if(options.child_negative){
        bool before_parent=false;
        for(unsigned n=0;n<std::min(old.events.size(),modern.events.size());++n)if(old.events[n].target==0x101e3d50 && old.events[n].before==modern.events[n].before && old.events[n].after!=modern.events[n].after)before_parent=true;
        require(before_parent,label+" child negative rejected at dependency return before parent continuation");
    }
    last_accesses=x.log;
    component_sites.insert(original_trace.instructions.begin(),original_trace.instructions.end());
    ++component_cases;component_returns+=ae.empty()&&!rejected;component_interruptions+=!ae.empty();component_negatives+=rejected;
}
}
int main(int argc,char** argv){try{
    require(argc==1 || argc==2,"Optional private instruction-site output path expected");
    auto baseline=seed();compare_component(*baseline);
    std::set<std::uint32_t> accessed;
    for(const auto& access:last_accesses)for(unsigned n=0;n<access.size;++n)accessed.insert(access.address+n);
    for(auto hole:accessed){auto sparse=std::make_unique<State>();sparse->registers=baseline->registers;sparse->unique=baseline->unique;sparse->pc=baseline->pc;
        for(auto byte:accessed)if(byte!=hole){sparse->memory.map(byte,1);sparse->memory.write<1>(byte,baseline->memory.read<1>(byte));}compare_component(*sparse);}

    for(auto role:{0u,1u,2u,4u,8u,0x308u,0x10001u,0xffffu}){Options o;o.role=role;compare_component(*seed(),o);}
    for(unsigned side=0;side<2;++side)for(auto slot:{0u,8u}){
        auto s=seed();s->wr<4>(12,owner+0x311c+side*0x2350+slot*0x1c4);Options o;o.role=slot?4:1;
        compare_component(*s,o);o.can_act=1;compare_component(*s,o);o.available=0;compare_component(*s,o);
    }
    for(auto kind:{0u,255u})for(auto phase:{0u,255u,5u}){auto s=seed();s->memory.write<1>(owner+0x7924,kind);s->memory.write<1>(owner+0x791f,phase);compare_component(*s);}
    for(unsigned side=0;side<2;++side){auto s=seed();const auto player=owner+0x311c+side*0x2350+8*0x1c4;s->wr<4>(12,player);s->memory.write<1>(owner+0x7924,255);s->memory.write<4>(owner+0xdff8,player);compare_component(*s);}
    for(auto position:{0u,4u,251u,252u,253u,254u,255u}){auto s=seed();s->memory.write<4>(subject+0x1b0,opponent);s->memory.write<1>(opponent+0x19,position);compare_component(*s);}
    for(auto value:{0.0,10.0,100000.0,-100000.0}){auto s=seed();s->memory.write<4>(subject+0xe0,float_bits<4>(value));s->memory.write<4>(subject+0xe4,float_bits<4>(value));Options o;o.role=0x308;compare_component(*s,o);}
    for(auto flags:{0u,4u}){Options o;o.flags=flags;o.low_flags=0x8000;compare_component(*seed(),o);o.low_flags=0x2000;compare_component(*seed(),o);}
    Options can;can.can_act=1;can.protect=1;compare_component(*seed(),can);
    auto area=seed();area->memory.write<1>(subject+0x106,0);area->memory.write<1>(owner+0x791e,0);area->memory.write<1>(owner+0x1e6,1);compare_component(*area,can);
    can.missing_keeper=true;compare_component(*area,can);
    // Capture paths require a designated teammate so dead-ball/pressure
    // priorities do not consume the same action before the terminal branch.
    for(auto role:{1u,4u})for(unsigned side=0;side<2;++side){
        auto t=seed();const auto player=owner+0x311c+side*0x2350+8*0x1c4;const auto teammate=player+0x1c4;
        t->wr<4>(12,player);t->memory.write<1>(owner+0x7924,255);t->memory.write<4>(owner+0xdff8,teammate);
        t->memory.write<1>(player+0x106,side?8:4);t->memory.write<1>(owner+0x791e,side?8:4);
        Options o;o.role=role;o.can_act=1;compare_component(*t,o);
        t->memory.write<1>(owner+0x1e6,1);t->memory.write<1>(player+0x106,side?1:10);t->memory.write<1>(owner+0x791e,side?1:10);compare_component(*t,o);
    }
    {auto t=seed();t->memory.write<1>(subject+0x106,0);t->memory.write<1>(owner+0x791e,0);t->memory.write<1>(owner+0x311c+0x2c,2);Options o;o.protect=1;compare_component(*t,o);}
    // Both defensive-line directions and their live flag/position alternatives.
    for(unsigned side=0;side<2;++side){
        auto t=seed();const auto player=owner+0x311c+side*0x2350+8*0x1c4;t->wr<4>(12,player);
        t->memory.write<1>(player+0x106,side?8:3);t->memory.write<1>(owner+0x791e,side?4:8);
        t->memory.write<1>(owner+0x7925,7);t->memory.write<1>(owner+0x7926,4);
        t->memory.write<4>(owner+0x81f4+side*0x18e0,0x400);compare_component(*t);
        t->memory.write<4>(owner+0x81f4+side*0x18e0,0);compare_component(*t);
        t->memory.write<1>(player+0x106,side?6:5);t->memory.write<1>(player+0x10e,127);t->memory.write<1>(player+0x109,127);compare_component(*t);
    }
    // Follow a live marker, then exercise its probabilistic coordinate offset.
    for(unsigned side=0;side<2;++side){auto t=seed();const auto player=owner+0x311c+side*0x2350+8*0x1c4;
        const auto marked=owner+0x311c+(1-side)*0x2350+8*0x1c4;t->wr<4>(12,player);t->memory.write<4>(player+0x1b0,marked);
        t->memory.write<4>(marked+0xb0,float_bits<4>(-100));t->memory.write<1>(marked+0x105,2);t->memory.write<1>(marked+0x106,side?4:8);compare_component(*t);
        t->memory.write<1>(marked+0x105,7);compare_component(*t);
    }
    {auto t=seed();t->memory.write<4>(subject+0xe0,float_bits<4>(100000));t->memory.write<4>(singles+0xc8,float_bits<4>(100000));Options o;o.role=0x308;o.jitter=10000000;compare_component(*t,o);}
    {auto t=seed();t->memory.write<4>(subject+0xe4,float_bits<4>(100000));Options o;o.jitter=10000000;compare_component(*t,o);}
    for(unsigned side=0;side<2;++side){
        auto t=seed();const auto player=owner+0x311c+side*0x2350+8*0x1c4;const auto other=owner+0x311c+(1-side)*0x2350+8*0x1c4;t->wr<4>(12,player);
        t->memory.write<1>(player+0x106,side?8:3);t->memory.write<1>(owner+0x791e,side?4:8);t->memory.write<1>(other+0x106,side?7:4);
        t->memory.write<1>(owner+0x7925,7);t->memory.write<1>(owner+0x7926,4);t->memory.write<4>(owner+(side?0xe014:0xe018),other);
        t->memory.write<4>(owner+0x81f4+side*0x18e0,0x400);compare_component(*t);
        t->memory.write<4>(owner+0x81f4+side*0x18e0,0);compare_component(*t);
        t->memory.write<1>(player+0x106,side?6:5);t->memory.write<1>(other+0x106,side?8:3);compare_component(*t);
    }
    for(unsigned side=0;side<2;++side){auto t=seed();const auto player=owner+0x311c+side*0x2350+8*0x1c4;t->wr<4>(12,player);Options o;o.role=1;o.can_act=1;
        t->memory.write<1>(owner+0x791d,5);compare_component(*t,o);
        t->memory.write<1>(owner+0x7924,255);t->memory.write<4>(owner+0xdff8,player+0x1c4);compare_component(*t,o);
        o.protect=1;compare_component(*t,o);o.protect=0;o.distance=0.25;compare_component(*t,o);
        t->memory.write<1>(owner+0x791d,127);o.distance=1;compare_component(*t,o);
    }
    {auto t=seed();t->memory.write<1>(subject+0x106,0);t->memory.write<1>(owner+0x791e,0);Options o;o.protect=1;o.missing_keeper=true;compare_component(*t,o);}
    {auto t=seed();t->memory.write<1>(subject+0x106,0);t->memory.write<1>(owner+0x791e,0);const auto cell=owner+4*0x450;
        t->memory.write<1>(cell+0xa60,1);t->memory.write<1>(cell+0xa61,2);compare_component(*t);}
    {auto t=seed();t->memory.write<4>(subject+0x1b0,opponent);t->memory.write<4>(owner+0xdff8,opponent);compare_component(*t);t->memory.write<4>(owner+0x81f4,0x400);compare_component(*t);}
    {auto t=seed();Options o;o.role=0x308;t->memory.write<4>(fixture+0x25008,stream+0x1fe);compare_component(*t,o);}
    {auto t=seed();t->memory.write<1>(owner+0x7924,255);Options o;o.can_act=1;compare_component(*t,o);}
    {auto t=seed();for(unsigned n=0;n<4096;n+=4)t->memory.write<4>(fixture+0x21000+n,float_bits<4>(1000));compare_component(*t);}
    {auto t=seed();Options o;o.role=0x28;compare_component(*t,o);o.role=0x210;compare_component(*t,o);}
    for(auto value:{std::numeric_limits<float>::quiet_NaN(),std::numeric_limits<float>::infinity(),-std::numeric_limits<float>::infinity()}){
        auto t=seed();Options o;o.role=0x308;t->memory.write<4>(subject+0xe0,float_bits<4>(value));t->memory.write<4>(subject+0xe4,float_bits<4>(value));compare_component(*t,o);
        t=seed();t->memory.write<4>(singles+0xc8,float_bits<4>(value));o.role=0x308;compare_component(*t,o);
    }
    for(auto value:{0u,0x80000000u,0xffffffffu}){auto t=seed();t->memory.write<4>(fixture+0x25000,value);Options o;o.role=0x308;compare_component(*t,o);}
    {auto t=seed();t->memory.write<4>(subject+0x1b0,opponent);t->memory.write<1>(subject+0x105,255);t->memory.write<1>(opponent+0x19,250);compare_component(*t);}
    for(auto limit:{0u,1u,8u,19u,24u,64u,128u,256u,512u}){auto s=seed();s->budget=limit;compare_component(*s);}
    compare_component(*seed(),{},true);Options child;child.child_negative=true;compare_component(*seed(),child);
    for(unsigned n=1;n<=8;++n){Options o;o.fault_call=n;compare_component(*seed(),o);}
    if(argc==2){std::ofstream sites_file(argv[1]);require(bool(sites_file),"Cannot create private site receipt");for(auto pc:component_sites)sites_file<<hex_address(pc)<<"\n";}
    std::cout<<"PASS: whole decision component cases="<<component_cases<<" normal="<<component_returns<<" interrupted="<<component_interruptions<<" negatives="<<component_negatives<<" sites="<<component_sites.size()<<"/2090; full architecture/memory/ordered effects/accounting/instruction sites/dependency ABI\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
