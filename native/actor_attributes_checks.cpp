#include "actor_attributes.hpp"
#include "actor_attributes_tables.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot { bool actor_attributes_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace attributes = cm::aot::actor_attributes;
constexpr std::uint32_t actor=fixture+0x1000, singles=fixture+0x400, doubles=fixture+0x500;
constexpr std::uint32_t globals=fixture+0x600, stream=fixture+0x800;
constexpr std::uint32_t condition_profile=fixture+0x1800, cost_table=fixture+0x1c00;
constexpr std::uint32_t condition_doubles=fixture+0x2000, condition_singles=fixture+0x2400;
constexpr std::uint32_t controller=fixture+0x3000, records=fixture+0x3400, staff=fixture+0x3800;
constexpr std::uint32_t injuries=fixture+0x3c00, record_limit=fixture+0x4000, controller_slot=fixture+0x4004;
constexpr std::uint32_t diagnostic=fixture+0x4100, record_constants=fixture+0x4400, percentage_divisor=fixture+0x4480;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,9> ranges{{
    {attributes::refresh_entry,0x102373a0},{attributes::maximum_entry,0x102373b4},{attributes::recovery_entry,0x10235e9c},
    {attributes::periodic_entry,0x1023958c},{attributes::consumption_entry,0x1023978c},{attributes::restoration_entry,0x10239d10},
    {attributes::record_scale_entry,0x10187680},{attributes::availability_entry,0x101877dc},{attributes::effective_condition_entry,0x10187a1c}}};
void modern_root(State& s,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
    constexpr Function methods[]{attributes::refresh,attributes::signed_maximum,attributes::recover_condition,
        attributes::refresh_condition,attributes::consume_condition,attributes::restore_condition,
        attributes::record_condition_scale,attributes::record_availability,attributes::effective_condition};
    const auto found=std::find(attributes::entries.begin(),attributes::entries.end(),s.pc);
    require(found!=attributes::entries.end(),"Unknown actor component root");methods[found-attributes::entries.begin()](s,dependencies,observer);
}
constexpr std::array<std::uint32_t,14> draws{0x10235f04,0x10236098,0x10236218,0x10236388,
    0x10236508,0x1023667c,0x102367f0,0x10236964,0x10236ad4,0x10236c54,0x10236dc4,
    0x10236f44,0x102370b8,0x1023722c};
constexpr std::array<unsigned,14> bounds{23,25,24,25,24,24,24,24,25,24,25,24,24,23};
struct Options {
    unsigned forced_draw=0, interrupt=0;
    std::int32_t wide=65536, correction=0;
    std::uint32_t random=0x1234567;
    double power=2.0;
    std::uint32_t role=2; std::int16_t days=20; std::uint16_t years=0;
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
        const auto found=std::find_if(ranges.begin(),ranges.end(),[&](const auto& range){return range.first==entry;});
        require(found!=ranges.end(),"Unknown actor reference root");const auto end=found->second;
        while(s.pc!=stop) {
            if(s.pc>=entry && s.pc<end)
                require(actor_attributes_reference(s),"Missing independent actor-attributes reference");
            else call(s,s.pc,std::uint32_t(s.r<4>(4128)));
        }
    }
    void call(State& s, std::uint32_t target, std::uint32_t continuation) override {
        Event e{target,continuation,s.registers,{}};
        if(target==0x12345678) throw std::runtime_error("Fixture actor-condition external destination");
        if(target==attributes::maximum_entry || target==attributes::record_scale_entry ||
            target==attributes::availability_entry || target==attributes::effective_condition_entry) {
            if(original) reference(s,continuation);
            else modern_root(s,this,&trace);
        } else {
            require(target==0x1077b770 || target==0x10226520 || target==attributes::refresh_entry || target==0x10780fc0 || target==0x10289560 || target==0x10187a40 || target==0x10717dd0 ||
                target==0x10739890 || target==0x10768810 || target==0x1077b720 || target==0x10769b60,
                "Unknown actor-attributes dependency");
            s.memory.write<4>(globals+0x100+std::uint32_t(calls.size())*4,target^std::uint32_t(s.r<4>(12)));
            if(options.mutate) {
                s.memory.write<1>(actor+0x111,31); s.memory.write<1>(actor+0x113,13);
                s.memory.write<1>(actor+0x117,17); s.memory.write<1>(actor+0x112,19);
                s.memory.write<4>(actor+0x50,float_bits<4>(32.0));
            }
            if(target==0x10187a40 && options.mutate) {
                const auto row=s.memory.read<4>(s.r<4>(12))+s.memory.read<4>(s.r<4>(16))*0x24;
                s.memory.write<2>(row+8,8000);s.memory.write<4>(row+0x10,0);
            }
            if(target==0x10187a40 && options.child_negative && !original) {
                const auto row=s.memory.read<4>(s.r<4>(12))+s.memory.read<4>(s.r<4>(16))*0x24;
                s.memory.write<4>(row+0x10,0);
            }
            if(target==0x10717dd0) {s.memory.write<2>(s.r<4>(12),std::uint16_t(options.days));s.memory.write<2>(s.r<4>(12)+2,options.years);}
            if(target==0x10739890) {s.memory.write<4>(s.r<4>(24),0x12345678);s.memory.write<4>(s.r<4>(28),0x87654321);}
            for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xac000000+n*17);
            for(unsigned n=0;n<=13;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-11.0-n));
            s.wr<1>(2304,5); s.wr<1>(1027,1);
            s.wr<4>(12,target==0x1077b770 ? options.random : std::uint32_t(options.correction));
            if(target==0x10289560)s.wr<4>(12,options.role);
            if(target==0x10780fc0) {
                s.memory.write<4>(s.r<4>(4)+0x14,s.r<4>(8));
                s.wr<8>(0x4010,float_bits<8>(options.power));
            }
            if(target==attributes::refresh_entry && options.child_negative && !original)
                s.memory.write<4>(actor+0x50,float_bits<4>(-31.0));
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
    s->memory.map(fixture,0x5000); s->memory.map(stack-0x300,0x500); s->memory.map(toc-0x8000,0x9000);
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
            else modern_root(s,&m,&new_trace);
            original_trace=nullptr; return std::string{};
        } catch(const std::exception& e) {original_trace=nullptr;return std::string(e.what());}
    };
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);
    a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Actor attributes entry="+hex_address(input.pc)+" case="+std::to_string(cases);
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
        if(x.bytes!=y.bytes) {
            for(unsigned n=0;n<std::min(x.log.size(),y.log.size());++n) {
                const auto& a=x.log[n];const auto& b=y.log[n];
                if(a.kind!=b.kind || a.address!=b.address || a.size!=b.size || a.value!=b.value) {
                    std::cerr<<"Effect "<<n<<" kind "<<a.kind<<"/"<<b.kind<<" pc-address "<<hex_address(a.address)<<"/"<<hex_address(b.address)<<" value "<<a.value<<"/"<<b.value<<'\n';break;
                }
            }
        }
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

std::unique_ptr<State> condition_seed(std::uint32_t entry) {
    auto s=seed(entry);
    s->memory.write<4>(toc-0x51e8,condition_doubles);s->memory.write<4>(toc-0x51e0,condition_singles);
    s->memory.write<4>(toc-0x5200,cost_table);
    for(unsigned n=0;n<original_attribute_cost_table.size();++n)s->memory.write<4>(cost_table+n*4,original_attribute_cost_table[n]);
    for(auto [offset,value]:std::array<std::pair<unsigned,double>,12>{{{0,0},{0x34,1},{0x5c,10},{0x64,12},
        {0x70,0.25},{0xb4,25},{0xb8,9500},{0xbc,11},{0xc0,8750},{0xc4,7750},{0xc8,8},{0xcc,6}}})
        s->memory.write<4>(condition_singles+offset,float_bits<4>(value));
    for(auto [offset,value]:std::array<std::pair<unsigned,double>,12>{{{0x10,1},{0x28,0.5},{0x38,200},
        {0x80,4503601774854144.0},{0x168,1.25},{0x170,0.35},{0x1f0,12},{0x2d8,25},
        {0x2e0,10000},{0x2e8,8.5},{0x2f0,11},{0x2f8,0.15}}})
        s->memory.write<8>(condition_doubles+offset,float_bits<8>(value));
    s->memory.write<4>(actor+0x70,condition_profile);s->memory.write<1>(condition_profile+0x25,20);
    s->memory.write<4>(actor+0x50,float_bits<4>(9000.0));s->memory.write<4>(actor+0x5c,float_bits<4>(1.0));
    s->memory.write<4>(actor+0xd8,float_bits<4>(10.0));s->memory.write<1>(actor+1,50);
    s->memory.write<1>(actor+0x102,20);s->memory.write<1>(actor+0x103,20);
    s->memory.write<2>(stream+2,11);s->wr<4>(20,19);s->wr<8>(0x4010,float_bits<8>(11.0));return s;
}

void condition_checks() {
    auto periodic=condition_seed(attributes::periodic_entry);auto result=compare(*periodic);
    require(result.state->memory.read<4>(actor+0x50)==float_bits<4>(9001.0) &&
        result.state->memory.read<4>(actor+0x5c)==float_bits<4>(0.0),"Independent periodic drift and exact sentinel reset");
    require(result.state->memory.read<1>(actor+0x102)==19,"Independent periodic exertion decrement");
    require(result.calls.size()==1 && result.calls[0].target==attributes::refresh_entry,"Periodic refresh keeps its semantic child boundary");
    boundaries(*periodic);
    for(auto value:{0u,1u,10u,11u,22u,65535u})for(auto salt:{0u,0x7fffffffu,0x80000000u,0xffffffffu}) {
        auto s=condition_seed(attributes::periodic_entry);s->memory.write<2>(stream+2,value);s->memory.write<4>(globals,salt);compare(*s);
    }
    for(auto load_level:{0u,49u,50u,51u,75u,127u,255u})for(auto exertion:{0u,1u,127u,128u,255u}) {
        auto s=condition_seed(attributes::periodic_entry);s->memory.write<1>(actor+1,load_level);
        s->memory.write<1>(actor+0x102,exertion);s->memory.write<1>(actor+0x103,exertion);
        s->memory.write<1>(condition_profile+0x25,exertion);compare(*s);
    }
    for(auto condition:{-1.0,0.0,1.0,9999.0,10000.0,20000.0})for(auto drift:{-10.0,0.0,1.0,2.0}) {
        auto s=condition_seed(attributes::periodic_entry);s->memory.write<4>(actor+0x50,float_bits<4>(condition));
        s->memory.write<4>(actor+0x5c,float_bits<4>(drift));boundaries(*s);
    }
    for(auto power:std::array<double,8>{-1.0,-0.0,0.0,1.0,100.0,101.0,std::numeric_limits<double>::infinity(),std::numeric_limits<double>::quiet_NaN()}) {
        auto s=condition_seed(attributes::periodic_entry);s->memory.write<1>(actor+1,75);Options o;o.power=power;boundaries(*s,o);
    }
    for(auto cursor:{stream+126,stream+128,0xfffffffeu}) {
        auto s=condition_seed(attributes::periodic_entry);s->memory.write<4>(globals+8,cursor);Options o;o.mutate=true;boundaries(*s,o);
    }
    for(auto action:{0u,18u,19u,20u,21u,33u,36u,46u,47u,127u,32767u,32768u,65534u,65535u,0x1234ffffu}) {
        auto s=condition_seed(attributes::consumption_entry);s->wr<4>(20,action);boundaries(*s);
    }
    for(unsigned action=19;action<=46;++action)for(auto target:{0x10239690u,0x102396e4u,0x10239738u}) {
        auto s=condition_seed(attributes::consumption_entry);s->wr<4>(20,action);s->memory.write<4>(cost_table+(action-19)*4,target);compare(*s);
    }
    for(auto action:{19u,20u,21u}) {
        auto s=condition_seed(attributes::consumption_entry);s->wr<4>(20,action);
        s->memory.write<4>(actor+0x50,float_bits<4>(-1.0));boundaries(*s);
    }
    {
        auto s=condition_seed(attributes::consumption_entry);s->memory.copy_in(cost_table+0x100,s->memory.copy_out(cost_table,112));
        s->memory.write<4>(toc-0x5200,cost_table+0x100);boundaries(*s);
        s->memory.write<4>(cost_table+0x100,0x12345678);auto r=compare(*s);
        require(r.state->pc==0x12345678 && r.state->r<4>(4128)==return_sentinel,"Mutated table retains external destination and LR");
    }
    for(auto condition:{-10.0,0.0,1.0,7749.0,7750.0,7751.0,8749.0,8750.0,8751.0,9499.0,9500.0,9501.0,10000.0}) {
        auto s=condition_seed(attributes::consumption_entry);s->wr<4>(20,0xffffffff);s->memory.write<4>(actor+0x50,float_bits<4>(condition));
        boundaries(*s);
        if(condition>0) {
            const auto factor=condition>9500?12:condition>8750?10:condition>7750?8:6;
            auto r=compare(*s);require(r.state->memory.read<4>(actor+0x50)==float_bits<4>(std::max(0.0,condition-factor)),"Independent movement cost threshold and floor");
        }
    }
    for(auto bits:std::array<std::uint32_t,8>{0,0x80000000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u,0x7f7fffffu,1}) {
        for(auto entry:{attributes::periodic_entry,attributes::consumption_entry}) {
            auto s=condition_seed(entry);s->memory.write<4>(actor+0x50,bits);boundaries(*s);
            s=condition_seed(entry);s->memory.write<4>(actor+(entry==attributes::periodic_entry?0x5c:0xd8),bits);compare(*s);
            s->wr<1>(1304,1);s->wr<1>(1308,1);compare(*s);
        }
    }
    for(auto offset:{0x28u,0x38u,0x2e8u,0x1f0u,0x2f0u})for(auto bits:std::array<std::uint64_t,6>{0,0x8000000000000000ULL,
        0x7ff0000000000000ULL,0xfff0000000000000ULL,0x7ff8123456789000ULL,0x7ff0123456789000ULL}) {
        auto s=condition_seed(attributes::consumption_entry);s->memory.write<8>(condition_doubles+offset,bits);compare(*s);
    }
    for(auto entry:{attributes::periodic_entry,attributes::consumption_entry}) {
        for(auto pointer:{0u,stack-0x160,globals-0x100,condition_doubles}) {
            auto s=condition_seed(entry);s->wr<4>(12,pointer);boundaries(*s);
        }
        auto s=condition_seed(entry);constexpr auto relocated=toc+0x10000;
        s->memory.map(relocated-0x8000,0x9000);s->memory.copy_in(relocated-0x8000,s->memory.copy_out(toc-0x8000,0x9000));
        s->wr<4>(8,relocated);boundaries(*s);
        Options own;own.own_negative=true;compare(*condition_seed(entry),own);
    }
    {Options o;o.child_negative=true;compare(*periodic,o);}
}
std::unique_ptr<State> restoration_seed(std::uint32_t entry=attributes::restoration_entry) {
    auto s=condition_seed(entry);
    for(auto [offset,pointer]:std::array<std::pair<int,unsigned>,7>{{{-0x7ba0,controller_slot},{-0x7be4,record_limit},
        {-0x5438,diagnostic},{-0x7ba8,diagnostic+0x100},{-0x7bac,diagnostic+0x200},
        {-0x543c,record_constants},{-0x5444,percentage_divisor}}})s->memory.write<4>(toc+offset,pointer);
    s->memory.write<4>(toc-0x7b80,diagnostic+0x210);s->memory.write<4>(controller_slot,controller);
    s->memory.write<4>(controller,records);s->memory.write<4>(controller+4,injuries);s->memory.write<4>(record_limit,2);
    s->memory.write<4>(staff,1);s->memory.write<2>(records+0x24+8,10000);s->memory.write<2>(records+0x24+10,10000);
    s->memory.write<2>(records+0x24+12,1000);s->memory.write<4>(records+0x24+0x10,0xffffffff);
    s->memory.write<2>(injuries+0x30,30);s->memory.write<2>(injuries+0x32,10);
    s->memory.write<4>(actor+0x6c,staff);s->memory.write<1>(condition_profile+0x34,10);
    s->memory.write<1>(actor+0x20,255);s->memory.write<1>(actor+0x21,255);
    s->memory.write<4>(actor+0x54,float_bits<4>(8000));
    s->memory.write<4>(condition_singles+0xd0,float_bits<4>(10000));s->memory.write<4>(condition_singles+0xd4,float_bits<4>(6500));
    for(auto [offset,value]:std::array<std::pair<unsigned,double>,4>{{{0x300,1.5},{0x308,0.0005},{0x310,0.05},{0x318,2.25}}})
        s->memory.write<8>(condition_doubles+offset,float_bits<8>(value));
    s->memory.write<8>(record_constants+8,float_bits<8>(4503601774854144.0));s->memory.write<8>(record_constants+0x28,float_bits<8>(0.5));
    s->memory.write<4>(percentage_divisor,float_bits<4>(100));s->wr<4>(16,1);
    if(entry!=attributes::restoration_entry) {s->wr<4>(12,controller);s->wr<4>(16,entry==attributes::record_scale_entry?1:staff);s->wr<4>(20,0);}
    return s;
}

void restoration_checks() {
    for(auto entry:{attributes::restoration_entry,attributes::record_scale_entry,attributes::availability_entry,attributes::effective_condition_entry}) {
        boundaries(*restoration_seed(entry));Options o;o.own_negative=true;compare(*restoration_seed(entry),o);
        if(entry!=attributes::record_scale_entry) {o.own_negative=false;o.child_negative=true;compare(*restoration_seed(entry),o);}
    }
    auto ordinary=compare(*restoration_seed());require(ordinary.state->memory.read<4>(actor+0x54)==float_bits<4>(9206),"Independent projected restoration");
    for(auto mode:{0u,1u,128u,255u,256u})for(auto role:{1u,2u,0xffffu})for(auto returning:{0u,255u}) {
        auto s=restoration_seed();s->wr<4>(16,mode);s->memory.write<1>(actor+0x1f,returning);Options o;o.role=role;boundaries(*s,o);
    }
    for(auto mode:{0u,1u})for(auto field:{0x19u,0x1cu,0x20u,0x21u})for(auto value:{0u,44u,45u,46u,55u,64u,65u,66u,127u,128u,255u}) {
        auto s=restoration_seed();s->wr<4>(16,mode);s->memory.write<1>(actor+field,value);boundaries(*s);
    }
    for(auto scale:{0u,999u,1000u,1001u,2000u,32767u,32768u,65535u})for(auto trait:{0u,127u,128u,255u}) {
        auto s=restoration_seed();s->memory.write<2>(records+0x24+12,scale);s->memory.write<1>(condition_profile+0x34,trait);boundaries(*s);
    }
    for(auto initial:{-10000.0,0.0,100.0,6499.0,6500.0,9999.0,10000.0,12000.0})for(auto drift:{-1.0,0.0,5000.0}) {
        auto s=restoration_seed();s->memory.write<4>(actor+0x50,float_bits<4>(initial));s->memory.write<4>(actor+0x5c,float_bits<4>(drift));boundaries(*s);
        s->wr<4>(16,0);Options o;o.role=1;boundaries(*s,o);
    }
    for(auto returning:{0u,255u}) {
        auto s=restoration_seed();s->wr<4>(16,0);s->memory.write<1>(actor+0x1f,returning);
        s->memory.write<4>(actor+0x50,float_bits<4>(12000));boundaries(*s);
    }
    for(auto field:{0x20u,0x21u}) {
        auto s=restoration_seed();s->memory.write<1>(actor+field,55);s->memory.write<4>(actor+0x50,float_bits<4>(100));boundaries(*s);
        auto r=compare(*s);require(r.state->memory.read<4>(actor+0x54)==float_bits<4>(6501),"Independent original availability floor plus final drift");
    }
    for(auto bits:std::array<std::uint32_t,8>{0,0x80000000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u,0x7f7fffffu,1}) {
        for(auto field:{0x50u,0x54u,0x5cu}) {auto s=restoration_seed();s->memory.write<4>(actor+field,bits);boundaries(*s);s->wr<1>(1304,1);s->wr<1>(1308,1);compare(*s);}
    }
    for(auto field:{0x300u,0x308u,0x310u,0x318u})for(auto bits:std::array<std::uint64_t,8>{0,0x8000000000000000ULL,0x7ff0000000000000ULL,
        0xfff0000000000000ULL,0x7ff8123456789000ULL,0x7ff0123456789000ULL,1,0x3ff0000000000001ULL}) {
        auto s=restoration_seed();s->memory.write<1>(actor+0x20,55);s->memory.write<2>(records+0x24+12,2000);
        s->memory.write<8>(condition_doubles+field,bits);boundaries(*s);s->wr<1>(1304,1);s->wr<1>(1308,1);compare(*s);
    }
    for(auto field:{0x6cu,0x70u})for(auto pointer:{0u,condition_doubles,stack-0x100}) {auto s=restoration_seed();s->memory.write<4>(actor+field,pointer);boundaries(*s);}
    for(auto entry:{attributes::record_scale_entry,attributes::availability_entry,attributes::effective_condition_entry}) {
        for(auto index:{0u,1u,2u,3u,0xffffffffu,0x80000000u,0x7fffffffu}) {
            auto s=restoration_seed(entry);if(entry==attributes::record_scale_entry)s->wr<4>(16,index);else s->memory.write<4>(staff,index);boundaries(*s);
        }
        if(entry!=attributes::record_scale_entry) {auto s=restoration_seed(entry);s->wr<4>(16,0);boundaries(*s);}
        for(auto pointer:{0u,staff,stack-0x100}) {auto s=restoration_seed(entry);s->wr<4>(12,pointer);boundaries(*s);}
    }
    for(auto entry:{attributes::availability_entry,attributes::effective_condition_entry})for(auto days:{-32768,-1,0,9,10,20,29,30,31,32767}) {
        auto s=restoration_seed(entry);s->memory.write<4>(records+0x24+0x10,0);Options o;o.days=std::int16_t(days);boundaries(*s,o);
        o.years=65535;compare(*s,o);o.mutate=true;boundaries(*s,o);
    }
    for(auto entry:{attributes::availability_entry,attributes::effective_condition_entry})for(auto injury:{0xffffffffu,0u,1u,0x7fffffffu}) {
        auto s=restoration_seed(entry);s->memory.write<4>(records+0x24+0x10,injury);boundaries(*s);
    }
    for(auto first:{0u,1u,99u,100u,8999u,9000u,10000u,32767u,32768u,65535u})for(auto second:{0u,100u,10000u,32767u,32768u,65535u})for(auto percent:{0u,1u,128u,255u,256u}) {
        auto s=restoration_seed(attributes::effective_condition_entry);s->memory.write<2>(records+0x24+8,first);s->memory.write<2>(records+0x24+10,second);s->wr<4>(20,percent);compare(*s);
        if(first==10000 && second==10000)boundaries(*s);
        s->memory.write<4>(records+0x24+0x10,0);compare(*s);
    }
    for(auto percent:{0u,1u,128u,256u}) {auto s=restoration_seed(attributes::effective_condition_entry);s->memory.write<4>(staff,2);s->wr<4>(20,percent);boundaries(*s);}
    for(auto bits:std::array<std::uint32_t,7>{0,0x80000000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u,1}) {
        auto s=restoration_seed(attributes::effective_condition_entry);s->wr<4>(20,1);s->memory.write<4>(percentage_divisor,bits);boundaries(*s);s->wr<1>(1304,1);s->wr<1>(1308,1);compare(*s);
    }
    for(auto entry:{attributes::restoration_entry,attributes::record_scale_entry,attributes::availability_entry,attributes::effective_condition_entry}) {
        auto s=restoration_seed(entry);constexpr auto relocated=toc+0x10000;s->memory.map(relocated-0x8000,0x9000);
        s->memory.copy_in(relocated-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,relocated);boundaries(*s);
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
    condition_checks();
    restoration_checks();
    for(auto [start,end]:ranges)
        for(auto pc=start;pc<end;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered "<<hex_address(pc)<<'\n';
    require(sites.size()==2184,"All 2184 actor-attribute/condition original sites covered");
    std::cout<<"PASS: "<<cases<<" actor-attribute comparisons; "<<completed<<" completions, "<<interruptions
        <<" interruptions, "<<negatives<<" own/child negatives, "<<structural<<" paired structural cases; all "<<sites.size()
        <<" original sites; full state/effects/ABI/accounting; independent original references\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}}
