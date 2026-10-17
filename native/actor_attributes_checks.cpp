#include "actor_attributes.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot { bool actor_attributes_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace attributes = cm::aot::actor_attributes;
constexpr std::uint32_t actor=fixture+0x1000, singles=fixture+0x400, doubles=fixture+0x500;
constexpr std::uint32_t globals=fixture+0x600, stream=fixture+0x800;
constexpr std::array<std::uint32_t,14> draws{0x10235f04,0x10236098,0x10236218,0x10236388,
    0x10236508,0x1023667c,0x102367f0,0x10236964,0x10236ad4,0x10236c54,0x10236dc4,
    0x10236f44,0x102370b8,0x1023722c};
constexpr std::array<unsigned,14> bounds{23,25,24,25,24,24,24,24,25,24,25,24,24,23};
struct Options {
    unsigned forced_draw=0, interrupt=0;
    std::int32_t wide=65536, correction=0;
    std::uint32_t random=0x1234567;
    bool own_negative=false, child_negative=false, mutate=false;
};
struct Trace final : SemanticGuestObserver {
    Options options;
    std::vector<std::uint32_t> sites;
    explicit Trace(Options o) : options(o) {}
    void before_step(State& s, std::uint32_t pc) override {
        sites.push_back(pc);
        if (options.forced_draw && pc==draws[options.forced_draw-1]+0x28)
            s.wr<4>(bounds[options.forced_draw-1]*4,std::uint32_t(options.wide));
    }
};
thread_local Trace* original_trace=nullptr;
struct Event {
    std::uint32_t target, continuation;
    decltype(State::registers) before, after;
    bool operator==(const Event&) const = default;
};
struct Model final : SemanticDependencyDispatch {
    bool original;
    Options options;
    Trace& trace;
    std::vector<Event> calls;
    Model(bool old, Options o, Trace& t) : original(old), options(o), trace(t) {}
    void reference(State& s, std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;
        const auto end=entry==attributes::refresh_entry ? attributes::maximum_entry :
            entry==attributes::maximum_entry ? 0x102373b4u : 0x10235e9cu;
        while(s.pc!=stop) {
            if(s.pc>=entry && s.pc<end)
                require(actor_attributes_reference(s),"Missing independent actor-attributes reference");
            else call(s,s.pc,std::uint32_t(s.r<4>(4128)));
        }
    }
    void call(State& s, std::uint32_t target, std::uint32_t continuation) override {
        Event e{target,continuation,s.registers,{}};
        if(target==attributes::maximum_entry) {
            if(original) reference(s,continuation);
            else attributes::signed_maximum(s,this,&trace);
        } else {
            require(target==0x1077b770 || target==0x10226520,"Unknown actor-attributes dependency");
            s.memory.write<4>(globals+0x100+std::uint32_t(calls.size())*4,target^std::uint32_t(s.r<4>(12)));
            if(options.mutate) {
                s.memory.write<1>(actor+0x111,31); s.memory.write<1>(actor+0x113,13);
                s.memory.write<1>(actor+0x117,17); s.memory.write<1>(actor+0x112,19);
                s.memory.write<4>(actor+0x50,float_bits<4>(32.0));
            }
            for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xac000000+n*17);
            for(unsigned n=0;n<=13;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-11.0-n));
            s.wr<1>(2304,5); s.wr<1>(1027,1);
            s.wr<4>(12,target==0x1077b770 ? options.random : std::uint32_t(options.correction));
            s.pc=continuation;
        }
        if(options.child_negative&&!original)s.wr<4>(12,s.r<4>(12)^1);
        e.after=s.registers; calls.push_back(e);
        if(options.interrupt && calls.size()>=options.interrupt)
            throw std::runtime_error("Fixture actor-attributes dependency interruption");
    }
};
std::unique_ptr<State> seed(std::uint32_t entry=attributes::refresh_entry) {
    auto s=std::make_unique<State>(); s->registers.fill(0xa5); s->unique.fill(0xb7);
    for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x3000); s->memory.map(stack-0x300,0x500); s->memory.map(toc-0x8000,0x9000);
    s->pc=entry; s->wr<4>(4,stack); s->wr<4>(8,toc); s->wr<4>(12,actor); s->wr<4>(4128,return_sentinel);
    for(auto [offset,value]:std::array<std::pair<int,unsigned>,6>{{{-0x51e0,singles},{-0x51e8,doubles},
        {-0x7b90,globals},{-0x7b8c,stream},{-0x7b88,globals+4},{-0x7b84,globals+8}}})s->memory.write<4>(toc+offset,value);
    s->memory.write<4>(globals,0); s->memory.write<4>(globals+4,stream+126); s->memory.write<4>(globals+8,stream);
    for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,n*17+7);
    s->memory.write<4>(singles+0xa8,float_bits<4>(4.0));
    s->memory.write<8>(doubles,float_bits<8>(65535.0));
    s->memory.write<8>(doubles+0x80,float_bits<8>(4503601774854144.0));
    s->memory.write<4>(actor+0x50,float_bits<4>(100.0));
    for(unsigned n=0x111;n<=0x11a;++n)s->memory.write<1>(actor+n,20);
    return s;
}
unsigned cases=0,completed=0,interruptions=0,negatives=0,structural=0;
std::set<std::uint32_t> sites;
struct Result { std::unique_ptr<State> state; std::vector<Effects::Access> accesses; std::vector<Event> calls; };
Result compare(const State& input, Options options={}) {
    auto a=clone(input),b=clone(input); Effects x,y; x.state=a.get(); y.state=b.get();
    a->memory.date_observer=&x; b->memory.date_observer=&y;
    Trace old_trace(options),new_trace(options); Model old(true,options,old_trace),modern(false,options,new_trace);
    const auto run=[&](State& s,Model& m,bool reference) {
        try {
            if(reference) {original_trace=&old_trace;m.reference(s);}
            else if(input.pc==attributes::refresh_entry) attributes::refresh(s,&m,&new_trace);
            else if(input.pc==attributes::maximum_entry) attributes::signed_maximum(s,&m,&new_trace);
            else attributes::recover_condition(s,&m,&new_trace);
            original_trace=nullptr; return std::string{};
        } catch(const std::exception& e) {original_trace=nullptr;return std::string(e.what());}
    };
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);
    a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Actor attributes case="+std::to_string(cases);
    require(!expected.starts_with("Unknown actor"),expected);
    require(expected==actual,label+" original="+expected+" modern="+actual);
    if(options.own_negative)b->wr<4>(52,b->r<4>(52)^1);
    bool rejected=false;
    try {Image(*a).compare(*b,label);} catch(const std::runtime_error&) {
        if(!options.own_negative&&!options.child_negative) {
            std::cerr<<"Final "<<hex_address(a->pc)<<" / "<<hex_address(b->pc)<<"; steps "<<old_trace.sites.size()<<" / "<<new_trace.sites.size()<<'\n';
            for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)
                if(old_trace.sites[n]!=new_trace.sites[n]) {std::cerr<<"First site "<<hex_address(old_trace.sites[n])<<" / "<<hex_address(new_trace.sites[n])<<'\n';break;}
            throw;
        }
        rejected=true;
    }
    require(rejected==(options.own_negative||options.child_negative),label+" discriminating own/child negative");
    if(!rejected) {
        require(x.bytes==y.bytes,label+" ordered effects"); require(a->executed==b->executed,label+" accounting");
        require(old.calls==modern.calls,label+" dependency ABI"); require(old_trace.sites==new_trace.sites,label+" original-site order");
    }
    ++cases; completed+=expected.empty()&&!rejected; interruptions+=!expected.empty(); negatives+=rejected;
    structural+=options.forced_draw!=0; sites.insert(old_trace.sites.begin(),old_trace.sites.end());
    return {std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}) {
    const auto baseline=compare(input,options); const auto steps=baseline.state->executed-input.executed;
    for(std::uint64_t n=0;n<=steps;++n) {auto s=clone(input);s->budget=s->executed+n;compare(*s,options);}
    std::set<std::uint32_t> bytes;
    for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(auto hole:bytes) {
        auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;
        for(auto b:bytes)if(b!=hole) {s->memory.map(b,1);s->memory.write<1>(b,input.memory.read<1>(b));}
        compare(*s,options);
    }
    for(unsigned n=1;n<=baseline.calls.size();++n) {auto o=options;o.interrupt=n;compare(input,o);}
}

void recovery_checks() {
    auto basic=compare(*seed(attributes::recovery_entry));
    require(basic.state->memory.read<4>(actor+0x50)==float_bits<4>(106.0),"Independent recovery increment");
    require(basic.state->memory.read<2>(actor+0x19c)==103 && basic.state->memory.read<1>(actor+0x2c)==1,
        "Recovery marker and initially clear flag");
    require(basic.state->memory.read<4>(globals+8)==stream+2,"Recovery consumes one pool word");
    boundaries(*seed(attributes::recovery_entry));
    for(auto flag:{0u,1u,127u,128u,255u})for(auto sample:{0u,1u,4u,5u,32767u,65535u})
        for(auto salt:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}) {
            auto s=seed(attributes::recovery_entry);s->memory.write<1>(actor+0x2c,flag);
            s->memory.write<2>(stream+2,sample);s->memory.write<4>(globals,salt);
            auto result=compare(*s);const auto value=std::bit_cast<std::int32_t>(sample+salt);
            require(result.state->memory.read<4>(actor+0x50)==float_bits<4>(100.0+(value%5)+5),"Independent signed recovery remainder");
            require(result.state->memory.read<1>(actor+0x2c)==(flag?flag:1),"Nonzero recovery flag retained");
        }
    for(auto bits:std::array<std::uint32_t,10>{0,0x80000000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u,
        std::uint32_t(float_bits<4>(-100.0)),std::uint32_t(float_bits<4>(99.5)),0x7f7fffffu,1}) {
        auto s=seed(attributes::recovery_entry);s->memory.write<4>(actor+0x50,bits);boundaries(*s);
        s->wr<1>(1304,1);s->wr<1>(1308,1);compare(*s);
    }
    for(auto cursor:{stream+124,stream+126,stream+128,0xfffffffeu}) {
        auto s=seed(attributes::recovery_entry);s->memory.write<4>(globals+8,cursor);
        Options o;o.mutate=true;boundaries(*s,o);
    }
    for(auto offset:{-0x51e8,-0x7b90,-0x7b8c,-0x7b88,-0x7b84})
        for(auto pointer:{0u,actor,stack-0x50,globals}) {
            auto s=seed(attributes::recovery_entry);s->memory.write<4>(toc+offset,pointer);compare(*s);
        }
    for(auto pointer:{stack-0x160,globals-0x100,stream-0x50}) {
        auto s=seed(attributes::recovery_entry);s->wr<4>(12,pointer);boundaries(*s);
    }
    {
        auto s=seed(attributes::recovery_entry);constexpr auto relocated=toc+0x10000;
        s->memory.map(relocated-0x8000,0x9000);
        s->memory.copy_in(relocated-0x8000,s->memory.copy_out(toc-0x8000,0x9000));
        s->wr<4>(8,relocated);boundaries(*s);
    }
    for(bool child:{false,true}) {
        auto s=seed(attributes::recovery_entry);s->memory.write<4>(globals+8,stream+126);
        Options o;o.own_negative=!child;o.child_negative=child;compare(*s,o);
    }
}
}
namespace cm::aot {void actor_attributes_step_seen(State& s,std::uint32_t pc) {if(original_trace)original_trace->before_step(s,pc);}}
int main() {try {
    auto baseline=compare(*seed());
    require(baseline.state->memory.read<4>(globals+8)==stream+28,"Four paired and six single traits consume fourteen draws");
    constexpr std::array<unsigned,10> destinations{0x107,0x10d,0x10a,0x10b,0x10c,0x10e,0x109,0x108,0x110,0x10f};
    constexpr std::array<unsigned,10> expected{15,9,17,11,5,19,7,15,9,3};
    for(unsigned n=0;n<destinations.size();++n)
        require(baseline.state->memory.read<1>(actor+destinations[n])==expected[n],"Independent ordinary trait expectation "+std::to_string(n));
    boundaries(*seed());
    for(unsigned field=0x111;field<=0x11a;++field)for(unsigned trait=0;trait<256;++trait) {
        auto s=seed();s->memory.write<1>(actor+field,trait);compare(*s);
    }
    for(auto bits:std::array<std::uint64_t,10>{0u,0x80000000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u,
        float_bits<4>(-131072.0),float_bits<4>(131068.0),float_bits<4>(100.0),float_bits<4>(20.0)}) {
        auto s=seed();s->memory.write<4>(actor+0x50,bits);boundaries(*s);
        s=seed();s->memory.write<4>(singles+0xa8,bits);compare(*s);
    }
    {auto s=seed();for(unsigned n=0x111;n<=0x11a;++n)s->memory.write<1>(actor+n,0);
        auto r=compare(*s);require(r.state->memory.read<4>(globals+8)==stream,"Zero ranges consume no random values");boundaries(*s);}
    for(auto salt:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto sample:{0u,1u,32767u,65535u}) {
        auto s=seed();s->memory.write<4>(globals,salt);for(unsigned n=0;n<128;n+=2)s->memory.write<2>(stream+n,sample);compare(*s);
    }
    for(unsigned n=0;n<draws.size();++n) {
        auto s=seed();s->memory.write<4>(globals+8,stream+126-n*2);Options o;o.mutate=true;boundaries(*s,o);
        for(auto wide:{65536,-65536,2147483647,(-2147483647-1)})for(auto correction:{-100000,0,10000000}) {
            Options w;w.forced_draw=n+1;w.wide=wide;w.correction=correction;compare(*seed(),w);
            if(wide==65536&&correction==0)boundaries(*seed(),w);
        }
    }
    for(auto offset:{-0x51e0,-0x51e8,-0x7b90,-0x7b8c,-0x7b88,-0x7b84})for(auto pointer:{0u,actor,stack-0x80,globals}) {
        auto s=seed();s->memory.write<4>(toc+offset,pointer);compare(*s);
    }
    for(auto bits:std::array<std::uint64_t,6>{0,0x8000000000000000ULL,0x7ff0000000000000ULL,
        0xfff0000000000000ULL,0x7ff8123456789000ULL,float_bits<8>(1.0)}) {
        for(auto offset:{0u,0x80u}) {
            auto s=seed();s->memory.write<8>(doubles+offset,bits);Options o;o.forced_draw=1;
            compare(*s,o);s->wr<1>(1304,1);compare(*s,o);
        }
    }
    for(auto enabled:{1304u,1308u}) {
        auto s=seed();s->wr<1>(enabled,1);s->memory.write<4>(actor+0x50,enabled==1304?0x7fc12345:float_bits<4>(99.0));
        boundaries(*s);
        s=seed();s->wr<1>(enabled,1);Options o;o.forced_draw=1;boundaries(*s,o);
    }
    for(auto pointer:{stack-0x160,globals-0x100,stream-0x111}) {
        auto s=seed();s->wr<4>(12,pointer);s->memory.write<4>(pointer+0x50,float_bits<4>(100.0));
        for(unsigned n=0x111;n<=0x11a;++n)s->memory.write<1>(pointer+n,20);
        boundaries(*s);
    }
    {
        auto s=seed();constexpr auto relocated=toc+0x10000;
        s->memory.map(relocated-0x8000,0x9000);
        s->memory.copy_in(relocated-0x8000,s->memory.copy_out(toc-0x8000,0x9000));
        s->wr<4>(8,relocated);boundaries(*s);
    }
    for(auto left_value:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})
        for(auto right_value:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}) {
            auto s=seed(attributes::maximum_entry);s->wr<4>(12,left_value);s->wr<4>(16,right_value);
            auto r=compare(*s);require(signed_value<4>(r.state->r<4>(12))==std::max(signed_value<4>(left_value),signed_value<4>(right_value)),"Independent signed maximum");boundaries(*s);
        }
    for(bool child:{false,true}) {Options o;o.own_negative=!child;o.child_negative=child;compare(*seed(),o);}
    {Options o;o.own_negative=true;compare(*seed(attributes::maximum_entry),o);}
    recovery_checks();
    for(auto [start,end]:std::array<std::pair<unsigned,unsigned>,2>{{{attributes::refresh_entry,0x102373b4},{attributes::recovery_entry,0x10235e9c}}})
        for(auto pc=start;pc<end;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered "<<hex_address(pc)<<'\n';
    require(sites.size()==1408,"All 1408 actor-attribute/recovery original sites covered");
    std::cout<<"PASS: "<<cases<<" actor-attribute comparisons; "<<completed<<" completions, "<<interruptions
        <<" interruptions, "<<negatives<<" own/child negatives, "<<structural<<" paired structural cases; all "<<sites.size()
        <<" original sites; full state/effects/ABI/accounting; independent original references\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}}
