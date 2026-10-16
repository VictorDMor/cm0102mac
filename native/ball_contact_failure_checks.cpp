#include "ball_contact.hpp"
#include "semantic_io_checks.hpp"
#include <set>
namespace cm::aot {bool ball_contact_failure_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace contact=cm::aot::ball_contact;
constexpr std::uint32_t actor=fixture+0x1000,profile=fixture+0x3000,condition=fixture+0x4000,match=fixture+0x5000;
constexpr std::uint32_t doubles=fixture+0x500,globals=fixture+0x600,stream=fixture+0x800;
struct Options {unsigned force_wide=0;bool zero_second=false,own_negative=false,child_negative=false,mutate_trait=false,mutate_owner=false;unsigned interrupt=0;std::uint32_t random=0x1234567,reaction=1;std::int32_t sampled=0;};
struct Trace final:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;
    explicit Trace(Options o):options(o){}
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);
        if(options.force_wide==1&&pc==0x102703fc)s.wr<4>(96,65536);
        if(options.force_wide==2&&pc==0x1027040c)s.wr<4>(96,std::uint32_t(-65536));
        if(options.force_wide==3&&pc==0x102709a0)s.wr<4>(96,65536);
        if(options.force_wide==4&&pc==0x102709b0)s.wr<4>(96,std::uint32_t(-65536));
        if(options.zero_second&&pc==0x10270960)s.wr<4>(12,std::uint32_t(-20));
    }
};
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;std::vector<Event> calls;
    Model(bool old,Options o):original(old),options(o){}
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        Event e{target,continuation,s.registers,{}};std::uint32_t result=0;
        switch(target) {
        case 0x1077b770:result=options.random;if(options.mutate_trait)s.memory.write<1>(actor+0x109,0x80);break;
        case 0x10226520:result=std::uint32_t(options.sampled);break;
        case 0x10201180:if(options.mutate_owner){s.memory.write<4>(actor+0x1a4,match+4);s.memory.write<4>(match+4+0x79f4,0x76543210);}break;
        case 0x10285ed0:case 0x101f1100:case 0x1022ba10:break;
        case 0x10288f30:result=options.reaction;break;
        default:throw std::runtime_error("Unknown contact-failure dependency "+hex_address(target));
        }
        s.memory.write<4>(globals+0x100+std::uint32_t(calls.size())*4,target^std::uint32_t(s.r<4>(16))^unsigned(options.child_negative&&!original));
        for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xac000000+n*17);
        for(unsigned n=0;n<=13;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-11.0-n));
        s.wr<1>(2304,5);s.wr<1>(1027,1);s.wr<4>(12,result);s.pc=continuation;e.after=s.registers;calls.push_back(e);
        if(options.interrupt&&calls.size()>=options.interrupt)throw std::runtime_error("Fixture contact-failure interruption");
    }
    void reference(State& s) {
        while(s.pc!=return_sentinel) {
            if(s.pc>=contact::failure_entry&&s.pc<0x10270b94)require(ball_contact_failure_reference(s),"Missing independent contact-failure reference");
            else call(s,s.pc,std::uint32_t(s.r<4>(4128)));
        }
    }
};
std::unique_ptr<State> seed() {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);s->memory.map(fixture,0x15000);s->memory.map(stack-0x300,0x500);s->memory.map(toc-0x8000,0x9000);
    s->pc=contact::failure_entry;s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,actor);s->wr<4>(16,100);s->wr<4>(4128,return_sentinel);
    for(auto [offset,value]:std::array<std::pair<int,unsigned>,5>{{{-0x51e8,doubles},{-0x7b90,globals},{-0x7b8c,stream},{-0x7b88,globals+4},{-0x7b84,globals+8}}})s->memory.write<4>(toc+offset,value);
    s->memory.write<4>(globals,0);s->memory.write<4>(globals+4,stream+126);s->memory.write<4>(globals+8,stream);
    for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,0);s->memory.write<2>(stream+4,1);
    s->memory.write<8>(doubles,float_bits<8>(65535));s->memory.write<8>(doubles+0x80,float_bits<8>(4503601774854144.0));
    s->memory.write<4>(actor+0x70,profile);s->memory.write<4>(actor+0x6c,condition);s->memory.write<4>(actor+0x1a4,match);s->memory.write<4>(actor+0xa8,float_bits<4>(1));
    s->memory.write<1>(actor+0x109,0);s->memory.write<1>(actor+0x27,1);s->memory.write<1>(actor+0x2f,23);s->memory.write<1>(actor+0x105,4);s->memory.write<1>(actor+0x106,6);
    s->memory.write<2>(actor+0x36,7000);s->memory.write<4>(match+0x79f4,0x12345678);return s;
}
unsigned cases=0,completed=0,interruptions=0,negatives=0,structural=0;std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace old_trace(options),new_trace(options);Model old(true,options),modern(false,options);
    const auto run=[&](State& s,Model& m,bool reference){try{if(reference){original_trace=&old_trace;m.reference(s);}else contact::react_to_failure(s,&m,&new_trace);original_trace=nullptr;return std::string{};}catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}};
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Contact failure case="+std::to_string(cases);require(!expected.starts_with("Unknown contact"),expected);require(expected==actual,label+" original="+expected+" modern="+actual);
    if(options.own_negative)b->wr<4>(52,b->r<4>(52)^1);bool rejected=false;
    try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative&&!options.child_negative){std::cerr<<"Final "<<hex_address(a->pc)<<" / "<<hex_address(b->pc)<<"; steps "<<old_trace.sites.size()<<" / "<<new_trace.sites.size()<<'\n';throw;}rejected=true;}
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating own/child negative");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");require(a->executed==b->executed,label+" accounting");require(old.calls==modern.calls,label+" dependency ABI");
        if(old_trace.sites!=new_trace.sites)for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)if(old_trace.sites[n]!=new_trace.sites[n]){std::cerr<<"First site "<<hex_address(old_trace.sites[n])<<" / "<<hex_address(new_trace.sites[n])<<'\n';break;}
        require(old_trace.sites==new_trace.sites,label+" original-site order");}
    ++cases;completed+=expected.empty()&&!rejected;interruptions+=!expected.empty();negatives+=rejected;structural+=options.force_wide!=0||options.zero_second;sites.insert(old_trace.sites.begin(),old_trace.sites.end());return {std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}) {
    const auto baseline=compare(input,options);const auto steps=baseline.state->executed-input.executed;
    for(std::uint64_t n=0;n<=steps;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(auto hole:bytes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;for(auto b:bytes)if(b!=hole){s->memory.map(b,1);s->memory.write<1>(b,input.memory.read<1>(b));}compare(*s,options);}
    for(unsigned n=1;n<=baseline.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}
}
namespace cm::aot {void ball_contact_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try {
    boundaries(*seed());
    for(auto score:{0u,999u,1000u,1001u,32767u,32768u,65535u,0xffff03e9u}){auto s=seed();s->wr<4>(16,score);auto r=compare(*s);if(signed_value<2>(score)>1000)require(r.state->r<4>(12)==1&&r.calls.size()==1&&read_be<4>(r.calls[0].before.data()+16)==0x1fe2,"Signed narrowed score above one thousand returns the original failure report immediately");boundaries(*s);}
    for(auto value:{-100000.0,-65536.0,-65535.0,-128.0,-1.0,0.0,1.0,127.0,65535.0,65536.0,100000.0})for(auto trait:{0u,1u,127u,128u,255u}){auto s=seed();s->memory.write<4>(actor+0xa8,float_bits<4>(value));s->memory.write<1>(profile+0x21,trait);compare(*s);}
    for(auto bits:{0x7fc12345u,0x7f812345u,0x7f800000u,0xff800000u,0x80000000u}){auto s=seed();s->memory.write<4>(actor+0xa8,bits);boundaries(*s);}
    for(unsigned trait=0;trait<256;++trait)for(auto draw:{0u,1u,7u,8u,32767u,65535u}){auto s=seed();s->memory.write<1>(actor+0x109,trait);s->memory.write<2>(stream+6,draw);compare(*s);}
    for(auto hard:{false,true})for(auto first:{0u,1u})for(auto second:{0u,1u}){auto s=seed();s->memory.write<1>(actor+0x109,hard?127:0);s->memory.write<2>(stream+10,first);s->memory.write<2>(stream+12,second);auto r=compare(*s);require(r.state->r<4>(12)==0&&r.state->memory.read<2>(actor+0x36)==(hard?7000u:6750u),"Hard and soft failed contacts preserve distinct receiver balance effects");boundaries(*s);}
    for(unsigned trait=0;trait<256;++trait){auto s=seed();s->memory.write<1>(condition+0x66,trait);compare(*s);}
    for(auto wide:{1u,2u,3u,4u})for(auto sampled:{-100000,0,10000000}){Options o;o.force_wide=wide;o.sampled=sampled;boundaries(*seed(),o);}
    {Options o;o.zero_second=true;boundaries(*seed(),o);}
    for(auto random:{0u,17u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto cursor:{stream+110,stream+112,stream+114,stream+116,stream+118,stream+120,stream+122,stream+124,stream+126}){auto s=seed();s->memory.write<4>(globals,random);s->memory.write<4>(globals+8,cursor);Options o;o.mutate_trait=true;compare(*s,o);}
    for(auto cursor:{stream+114,stream+120,stream+124,stream+126}){auto s=seed();s->memory.write<4>(globals+8,cursor);Options o;o.random=0;o.mutate_trait=true;boundaries(*s,o);}
    for(auto response:{0u,1u,0x100u,0x1ffu}){Options o;o.reaction=response;boundaries(*seed(),o);}
    {auto s=seed();s->memory.write<2>(stream+4,0);auto r=compare(*s);require(r.state->r<4>(12)==1&&r.calls.size()==1,"Counter sample at the random twentieth threshold returns one");boundaries(*s);}
    {Options o;o.mutate_owner=true;boundaries(*seed(),o);}
    for(auto pointer:{0u,actor+1,stack-0x70,globals}){auto s=seed();s->memory.write<4>(actor+0x70,pointer);compare(*s);s=seed();s->memory.write<4>(actor+0x6c,pointer);compare(*s);}
    for(bool child:{false,true}){Options o;o.own_negative=!child;o.child_negative=child;compare(*seed(),o);}
    for(auto [hard,last]:std::array<std::pair<bool,unsigned>,5>{{{true,6},{true,8},{true,10},{false,10},{false,14}}}) {
        auto s=seed();s->memory.write<1>(actor+0x109,hard?127:0);s->memory.write<4>(globals+4,stream+last);Options o;o.random=0;boundaries(*s,o);
    }
    for(auto pc=contact::failure_entry;pc<0x10270b94;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered "<<hex_address(pc)<<'\n';require(sites.size()==541,"All 541 contact-failure original sites covered");
    std::cout<<"PASS: "<<cases<<" contact-failure comparisons; "<<completed<<" completions, "<<interruptions<<" interruptions, "<<negatives<<" own/child negatives, "<<structural<<" paired structural cases; all "<<sites.size()<<" original sites; full state/effects/ABI/accounting; independent original references\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
