#include "actor_motion.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot {
bool actor_motion_reference(State&);
}
namespace {
using namespace cm::semantic_io_checks;
using Entry = cm::aot::actor_motion::Entry;
constexpr std::uint32_t owner=fixture+0x1000, subject=owner+0x311c, other=subject+0x1c4;
constexpr std::uint32_t angles=fixture+0x21000, doubles=fixture+0x23000, strings=fixture+0x24000;
constexpr std::uint32_t globals=fixture+0x28000, stream=fixture+0x29000, player=fixture+0x30000;
struct Trace {
    std::vector<std::uint32_t> order;
    std::set<std::uint32_t> sites;
};
thread_local Trace* trace=nullptr;
struct Probe final : SemanticGuestObserver {
    void before_step(State& s,std::uint32_t pc) override;
};
struct DependencyEvent {
    std::uint32_t target, continuation;
    decltype(State::registers) before, after;
    bool operator==(const DependencyEvent&) const=default;
};
struct Model final : SemanticDependencyDispatch {
    bool possession=true;
    unsigned fault_call=0;
    std::vector<DependencyEvent> calls;
    void call(State& s,std::uint32_t target,std::uint32_t continuation) override {
        const auto argument=std::uint32_t(s.r<4>(12));
        calls.push_back({target,continuation,s.registers,{}}); const auto index=calls.size()-1;
        std::uint32_t result=0;
        switch(target) {
        case 0x102775c0: result=possession; break;
        case 0x10775540: result=std::bit_cast<std::int32_t>(argument)<0 ? 0u-argument : argument; break;
        case 0x1077b770: result=0x12345678; break;
        case 0x10739890: case 0x10768810: case 0x1077b720: case 0x10769b60: result=0x42424242; break;
        case 0x1076f7d0: result=argument; break;
        default: throw std::runtime_error("Unknown actor-motion fixture dependency "+hex_address(target));
        }
        for(unsigned reg=0;reg<=12;++reg)if(reg!=1 && reg!=2)s.wr<4>(reg*4,0xbad00000+reg*73);
        s.wr<4>(12,result); s.pc=continuation; calls[index].after=s.registers;
        if(target==0x1076f7d0)throw std::runtime_error("Fixture motion termination");
        if(fault_call && calls.size()==fault_call)throw std::runtime_error("Fixture motion dependency interruption");
    }
    void reference(State& s) {
        constexpr std::array<std::uint32_t,5> ends{0x10237830,0x10237af4,0x10237b08,0x1024515c,0x102775e0};
        const auto start=s.pc;
        const auto found=std::find(cm::aot::actor_motion::entries.begin(),cm::aot::actor_motion::entries.end(),start);
        require(found!=cm::aot::actor_motion::entries.end(),"Unknown original motion fixture root");
        const auto end=ends[unsigned(found-cm::aot::actor_motion::entries.begin())];
        // Per-entry component checks model exterior dependencies identically,
        // even when another body in this block is present in the reference.
        while(s.pc!=return_sentinel)
            if(!(s.pc>=start && s.pc<end && actor_motion_reference(s)))call(s,s.pc,std::uint32_t(s.r<4>(4128)));
    }
};
std::uint32_t cell(unsigned x,unsigned y) { return owner+0xa08+x*0x450+y*0x5c; }
std::unique_ptr<State> seed(Entry entry) {
    auto s=std::make_unique<State>(); s->registers.fill(0xa5); s->unique.fill(0xb7);
    for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x50000); s->memory.map(stack-0x800,0x900); s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,subject);s->wr<4>(4128,return_sentinel);
    s->pc=cm::aot::actor_motion::entries[unsigned(entry)];
    for(auto [offset,value]:std::array<std::pair<std::int32_t,std::uint32_t>,8>{{
        {-0x641c,angles},{-0x51e8,doubles},{-0x51e4,strings},{-0x7bac,globals+16},
        {-0x7ba8,globals+20},{-0x7b90,globals},{-0x7b8c,stream},{-0x7b88,globals+4}}})
        s->memory.write<4>(toc+std::uint32_t(offset),value);
    s->memory.write<4>(toc-0x7b84,globals+8);s->memory.write<4>(globals,0);s->memory.write<4>(globals+4,stream+126);
    s->memory.write<4>(globals+8,stream);s->memory.write<1>(globals+16,1);
    for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,100+n);
    s->memory.write<8>(doubles+0x80,float_bits<8>(4503601774854144.0));
    s->memory.write<8>(doubles+0x1f8,float_bits<8>(1.5));s->memory.write<8>(doubles+0x338,float_bits<8>(10));
    for(int x=-12;x<=12;++x)for(int y=-12;y<=12;++y)
        s->memory.write<2>(std::uint32_t(angles+0x1c8+x*48+y*2),std::uint16_t(x*17+y*11+225));
    s->memory.write<4>(subject+0x1a4,owner);s->memory.write<4>(subject+0x70,player);
    s->memory.write<1>(subject+0x105,4);s->memory.write<1>(subject+0x106,6);s->memory.write<1>(subject+0x27,0);
    s->memory.write<1>(subject+0x28,0);s->memory.write<1>(subject+0x108,20);s->memory.write<1>(subject+0x10f,20);
    s->memory.write<2>(subject+0x19e,90);s->memory.write<1>(player+0x30,10);s->memory.write<1>(player+0x3b,10);
    s->memory.write<1>(cell(4,6)+0x58,2);s->memory.write<4>(cell(4,6),subject);s->memory.write<4>(cell(4,6)+4,other);
    s->memory.write<1>(other+0x28,1);s->memory.write<4>(owner+0xe000,subject);
    s->wr<4>(16,5);s->wr<4>(20,6);s->wr<4>(24,5);s->wr<4>(28,6);
    return s;
}
std::array<unsigned,5> cases{}, returns{}, faults{}, negatives{};
std::array<std::set<std::uint32_t>,5> visited;
Probe probe;
void compare(const State& input,Entry entry,Model options={},bool negative=false) {
    auto a=clone(input), b=clone(input); Model original=options, modern=options;
    Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace old_trace,new_trace;
    const auto execute=[](State& s,Entry entry,Model& model,bool old,Trace& path) {
        trace=&path; std::string outcome;
        try { if(old)model.reference(s);else cm::aot::actor_motion::invoke(s,entry,&model,&probe); }
        catch(const std::exception& e){outcome=e.what();}
        trace=nullptr; return outcome;
    };
    const auto expected=execute(*a,entry,original,true,old_trace),actual=execute(*b,entry,modern,false,new_trace);
    a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="actor-motion entry "+hex_address(input.pc)+" case "+std::to_string(cases[unsigned(entry)]);
    require(expected==actual,label+" outcome original="+expected+" modern="+actual);
    if(x.bytes!=y.bytes) {
        unsigned n=0;for(;n<std::min(x.log.size(),y.log.size());++n)
            if(x.log[n].kind!=y.log[n].kind || x.log[n].address!=y.log[n].address || x.log[n].value!=y.log[n].value || x.log[n].size!=y.log[n].size)break;
        std::cerr<<label<<" first effect="<<n<<" counts="<<x.log.size()<<'/'<<y.log.size()<<'\n';
        for(const auto* log:{&x.log,&y.log})if(n<log->size()){
            const auto& e=(*log)[n];std::cerr<<"kind="<<e.kind<<" address="<<hex_address(e.address)<<" size="<<e.size<<" value="<<e.value<<'\n';}
    }
    require(x.bytes==y.bytes,label+" ordered guest effects differ");
    require(a->executed==b->executed && original.calls==modern.calls,label+" instruction accounting or dependency ABI differs");
    require(old_trace.order==new_trace.order,label+" original execution path differs");
    if(negative)b->wr<4>(12,b->r<4>(12)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!negative)throw;rejected=true;}
    require(rejected==negative,label+" negative discriminant");
    ++cases[unsigned(entry)];returns[unsigned(entry)]+=actual.empty()&&!negative;
    faults[unsigned(entry)]+=!actual.empty();negatives[unsigned(entry)]+=negative;
    visited[unsigned(entry)].insert(old_trace.sites.begin(),old_trace.sites.end());
}
void budget_sweep(const State& input,Entry entry) {
    // Each entry must preserve every attempted-prefix boundary for this path.
    auto original=clone(input); Model model; Trace path; trace=&path;
    try{model.reference(*original);}catch(const std::exception&){}trace=nullptr;
    const auto count=original->executed-input.executed;
    for(std::uint64_t limit=0;limit<count;++limit){auto variant=clone(input);variant->budget=variant->executed+limit;compare(*variant,entry);}
}
}
namespace cm::aot {
void actor_motion_reference_step_seen(State&,std::uint32_t pc){if(trace){trace->sites.insert(pc);trace->order.push_back(pc);}}
void actor_motion_reference_call_seen(State&,std::uint32_t,std::uint32_t){}
}
namespace { void Probe::before_step(State& s,std::uint32_t pc){actor_motion_reference_step_seen(s,pc);} }

int main(){try {
    for(unsigned index=0;index<5;++index){const auto entry=Entry(index);auto input=seed(entry);compare(*input,entry);compare(*input,entry,{},true);budget_sweep(*input,entry);}
    for(unsigned side:{0u,1u})for(unsigned member:{0u,1u})for(unsigned destination_count:{0u,4u}){
        auto s=seed(Entry::set_coordinates);s->memory.write<1>(subject+0x27,side);s->memory.write<1>(subject+0x28,member);
        s->memory.write<1>(cell(4,6)+0x58+side,2);s->memory.write<4>(cell(4,6)+side*0x2c+member*4,subject);
        s->memory.write<4>(cell(4,6)+side*0x2c+(1-member)*4,other);s->memory.write<1>(cell(5,6)+0x58+side,destination_count);
        compare(*s,Entry::set_coordinates);Model no_ball;no_ball.possession=false;compare(*s,Entry::set_coordinates,no_ball);
    }
    for(int member:{-1,0})for(unsigned count:{0u,11u}){
        auto s=seed(Entry::set_coordinates);s->memory.write<1>(subject+0x28,std::uint8_t(member));
        s->memory.write<1>(member<0?cell(5,6)+0x58:cell(4,6)+0x58,count);compare(*s,Entry::set_coordinates);
    }
    {auto s=seed(Entry::set_coordinates);s->wr<4>(16,4);compare(*s,Entry::set_coordinates);}
    for(unsigned fault=1;fault<=4;++fault){auto s=seed(Entry::set_coordinates);s->memory.write<1>(cell(4,6)+0x58,0);Model model;model.fault_call=fault;compare(*s,Entry::set_coordinates,model);}
    for(auto target:std::array<std::pair<unsigned,unsigned>,4>{{{4,11},{4,0},{4,6},{5,6}}})
      for(unsigned trait:{0u,20u,255u})for(unsigned cursor:{stream,stream+126})for(int bias:{-100,0,100}){
        auto s=seed(Entry::set_heading);s->wr<4>(16,4);s->wr<4>(20,target.second);s->wr<4>(24,target.first);s->wr<4>(28,target.second);
        s->memory.write<1>(subject+0x108,trait);s->memory.write<1>(subject+0x10f,trait);s->memory.write<4>(globals+8,cursor);
        s->memory.write<1>(player+0x3b,std::uint8_t(10+bias));compare(*s,Entry::set_heading);
      }
    {auto s=seed(Entry::set_heading);s->memory.write<4>(subject+0x70,0);compare(*s,Entry::set_heading);}
    for(unsigned cursor:{stream+122,stream+124}){
        auto s=seed(Entry::set_heading);s->wr<4>(16,4);s->wr<4>(20,11);s->wr<4>(24,4);s->wr<4>(28,11);
        s->memory.write<1>(subject+0x108,0);s->memory.write<4>(globals+8,cursor);compare(*s,Entry::set_heading);
    }
    for(int heading:{-32768,-1,0,180,359,32767}){auto s=seed(Entry::heading);s->memory.write<2>(subject+0x19e,std::uint16_t(heading));compare(*s,Entry::heading);}
    for(unsigned trait:{0u,20u,255u})for(int heading:{-32768,-1,0,180,225,242,359,32767}){
        auto s=seed(Entry::can_face_destination);s->memory.write<1>(subject+0x10f,trait);s->memory.write<2>(subject+0x19e,std::uint16_t(heading));compare(*s,Entry::can_face_destination);
    }
    {auto s=seed(Entry::can_face_destination);s->wr<4>(16,4);compare(*s,Entry::can_face_destination);}
    {auto s=seed(Entry::has_possession);s->memory.write<4>(owner+0xe000,other);compare(*s,Entry::has_possession);}
    for(const auto entry:{Entry::has_possession,Entry::can_face_destination}){auto s=seed(entry);s->memory.write<4>(subject+0x1a4,0);if(entry==Entry::can_face_destination)s->memory.write<4>(toc-0x641c,0);compare(*s,entry);}
    for(unsigned index=0;index<5;++index){require(cases[index]&&returns[index]&&faults[index]&&negatives[index],"Missing per-entry evidence");
        std::cout<<hex_address(cm::aot::actor_motion::entries[index])<<" cases="<<cases[index]<<" normal="<<returns[index]<<" interrupted/fault="<<faults[index]<<" negative="<<negatives[index]<<" sites="<<visited[index].size()<<'\n';}
    std::cout<<"PASS: complete coordinate/heading/turn/possession bodies, per-entry ABI/architecture/effects/dependency order, RNG boundaries, cell migration and faults, signed inputs, full-prefix budgets and negatives\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
