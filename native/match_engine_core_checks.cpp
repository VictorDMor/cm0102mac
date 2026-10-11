#include "match_engine.hpp"
#include "semantic_io_checks.hpp"
#include <map>
#include <set>
namespace cm::aot { bool match_engine_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace engine = cm::aot::match_engine;
constexpr auto owner = fixture + 0x10000, control = fixture + 0x1000;
constexpr auto clubs = fixture + 0x4000, record = fixture + 0x3000, player = fixture + 0x30000;
constexpr std::uint32_t relocated_toc = 0x32010000, caller = 0x7fff1000;
unsigned comparisons = 0, faults = 0, budgets = 0, negatives = 0;
struct Dependencies final : engine::DependencyDispatch {
    std::map<std::uint32_t, std::vector<std::uint32_t>> outcomes;
    std::map<std::uint32_t, unsigned> counts;
    std::function<void(State&, std::uint32_t, unsigned)> effect;
    std::vector<std::pair<std::uint32_t, decltype(State::registers)>> calls;
    bool clear_phase = true;
    void reference(State& s) {
        while (s.pc != caller) {
            if (!cm::aot::match_engine_reference(s)) call(s, s.pc, std::uint32_t(s.r<4>(4128)));
        }
    }
    void call(State& s, std::uint32_t target, std::uint32_t continuation) override {
        require(s.pc == target && s.r<4>(4128) == continuation && s.r<4>(2440) == s.r<4>(8), "Shared engine dependency ABI");
        calls.emplace_back(target, s.registers);
        s.step(target); const auto count = counts[target]++;
        if (effect) effect(s, target, count);
        const auto found = outcomes.find(target);
        const auto result = found != outcomes.end() ? found->second.at(std::min<std::size_t>(count, found->second.size()-1)) :
            target == 0x101d6980 ? UINT32_MAX : target == 0x101f3a30 ? player : 0;
        if (target == 0x101da820 && clear_phase) s.memory.write<1>(owner + 0x7929, 0);
        if (target == 0x10739890) { s.memory.write<4>(s.r<4>(24), 0); s.memory.write<4>(s.r<4>(28), 0); }
        s.wr<4>(12, result); s.wr<4>(0, 0x24681357); ++s.native_calls;
        s.step(target + 4); s.pc = std::uint32_t(s.r<4>(4128));
    }
};
std::unique_ptr<State> initial() {
    auto s = std::make_unique<State>(); s->registers.fill(0x6d); s->unique.fill(0xb3); s->native_calls = 17;
    for (unsigned flag=1280;flag<=1308;++flag) s->wr<1>(flag,0);
    for (auto [a,n] : std::array<std::pair<std::uint32_t, unsigned>,3>{{
        {relocated_toc-0x8000,0x8000}, {stack-4096,8192}, {fixture,0x50000}}}) {
        s->memory.map(a,n); s->memory.copy_in(a,std::vector<std::uint8_t>(n,0));
    }
    s->wr<4>(4,stack); s->wr<4>(8,relocated_toc); s->wr<4>(12,owner); s->wr<4>(16,1); s->wr<4>(4128,caller);
    s->memory.write<4>(stack+8,caller); s->pc=engine::entries[2]; s->budget=10000;
    for (auto [offset,pointer] : std::array<std::pair<std::int32_t,std::uint32_t>,7>{{
        {-0x7a54,control}, {-0x7a6c,control+0x100}, {-0x7bac,control+0x200}, {-0x7ba8,control+0x300},
        {-0x52cc,control+0x400}, {-0x52c8,control+0x500}, {-0x52c4,control+0x600}}})
        s->memory.write<4>(relocated_toc+std::uint32_t(offset),pointer);
    s->memory.write<4>(owner+0x3118,record); s->memory.write<4>(record+0x1c,clubs); s->memory.write<4>(record+0x20,clubs+0x100);
    s->memory.write<4>(owner+0x79f4,control+0x700); s->memory.write<1>(owner+0xe0b8,0xff);
    s->memory.write<1>(owner+0x792e,1); s->memory.write<4>(owner+0x79f8,player); s->memory.write<4>(owner+0x79fc,player+0x200);
    s->memory.write<8>(control+0x660,UINT64_C(0x4330000080000000)); s->memory.write<8>(control+0x678,float_bits<8>(10.0));
    s->memory.write<4>(control+0x51c,float_bits<4>(10.0)); s->memory.write<4>(control+0x520,float_bits<4>(2.0));
    s->memory.write<4>(player+0x74,float_bits<4>(0.5)); s->memory.write<4>(player+0xc4,float_bits<4>(0.75));
    return s;
}
using Configure = std::function<void(Dependencies&)>;
std::vector<Effects::Access> compare(const State& seed,const std::string& name,Configure configure={},bool normal=true,bool negative=false,
    std::function<void(const State&,const Dependencies&)> expectation={}) {
    auto a=clone(seed),b=clone(seed); Effects x,y; x.state=a.get(); y.state=b.get(); a->memory.date_observer=&x; b->memory.date_observer=&y;
    Dependencies old,modern; if(configure) {configure(old);configure(modern);} std::string ae,be;
    try {old.reference(*a);} catch(const std::exception& e) {ae=e.what();}
    try {engine::advance_shared(*b,&modern);} catch(const std::exception& e) {be=e.what();}
    a->memory.date_observer=b->memory.date_observer=nullptr;
    require(ae==be,name+" original="+ae+" modern="+be); if(normal) require(ae.empty(),name+" unexpected matching fault="+ae);
    if(negative) {
        b->memory.write<1>(owner+0x792d,b->memory.read<1>(owner+0x792d)^1);
        bool rejected=false;try {Image(*a).compare(*b,name);}catch(const std::runtime_error&) {rejected=true;}
        require(rejected,"Shared-engine own effect negative rejected");++negatives;
    } else {
        Image(*a).compare(*b,name); require(x.bytes==y.bytes,name+" ordered memory effects");
        if(old.calls!=modern.calls) {
            const auto mismatch=std::mismatch(old.calls.begin(),old.calls.end(),modern.calls.begin(),modern.calls.end());
            const auto index=std::size_t(mismatch.first-old.calls.begin());
            throw std::runtime_error(name+" dependency register/order mismatch at call="+std::to_string(index));
        }
    }
    require(a->executed==b->executed,name+" accounting old="+std::to_string(a->executed)+" modern="+std::to_string(b->executed));
    if(normal&&expectation) expectation(*b,modern);
    ++comparisons;faults+=!ae.empty();budgets+=ae=="Native instruction budget exhausted"; return x.log;
}
std::unique_ptr<State> minimal(const State& seed,const std::vector<Effects::Access>& accesses,std::uint32_t hole=UINT32_MAX) {
    auto s=std::make_unique<State>();s->registers=seed.registers;s->unique=seed.unique;s->pc=seed.pc;s->budget=seed.budget;
    s->native_calls=seed.native_calls;s->executed=seed.executed;s->heap_end=seed.heap_end;s->allocated=seed.allocated;s->available=seed.available;s->memory_error=seed.memory_error;
    std::set<std::uint32_t> bytes;for(const auto& a:accesses)for(std::uint64_t i=0;i<a.size;++i)bytes.insert(a.address+std::uint32_t(i));
    for(auto address:bytes)if(address!=hole){s->memory.map(address,1);s->memory.copy_in(address,seed.memory.copy_out(address,1));}return s;
}
void matrix() {
    auto seed=initial();
    const auto variant=[&](const std::string& name,auto change,Configure config={}) {auto s=clone(*seed);change(*s);compare(*s,name,config);};
    compare(*seed,"normal",{},true,false,[](const State& s,const Dependencies&) {
        require(s.pc==caller&&s.r<4>(4)==stack&&s.r<4>(8)==relocated_toc&&s.r<4>(12)==1,"Independent shared-engine return ABI");
        require(s.memory.read<2>(owner+0x7948)==1&&s.memory.read<2>(owner+0x794a)==0&&s.memory.read<1>(owner+0x792e)==1,"Independent one-tick clock and live queue effects");
    });
    compare(*seed,"own-negative",{},true,true);
    for(auto mode:{0u,0x100u,2u,0xffu})variant("mode-byte",[mode](State& s){s.wr<4>(16,mode);},[](Dependencies& d){d.effect=[](State& s,std::uint32_t target,unsigned){if(target==0x101d95a0)s.memory.write<1>(owner+0xe0b8,0);};});
    for(auto skew:{1u,3u})variant("unaligned-stack",[skew](State& s){s.wr<4>(4,stack+skew);});
    for(auto phase:{1u,2u,3u,4u,5u,6u,7u,0xffu})
        for(auto clear:{false,true})variant("stopped-phase",[phase](State& s){s.memory.write<1>(owner+0x7929,phase);},[clear](Dependencies& d){d.clear_phase=clear;});
    variant("pending-transition",[](State& s){s.memory.write<1>(owner+0x792a,1);});
    variant("clear-transition",[](State& s){s.memory.write<1>(owner+0x792a,2);});
    for(auto done:{1u,0x80u})for(auto allowed:{0u,1u})for(auto managed:{0u,1u})
        variant("completed-club-notifications",[done](State& s){s.memory.write<1>(owner,done);},[allowed,managed](Dependencies& d){d.outcomes[0x1014c2e0]={allowed};d.outcomes[0x101481d0]={managed};});
    for(auto flags:{0u,1u,0xffu})variant("completed-suppression",[flags](State& s){s.memory.write<1>(owner,1);s.memory.write<1>(record+0x40,flags);s.memory.write<1>(record+0x41,flags);});
    for(auto phase:{1u,4u})for(auto allowed:{0u,1u})for(auto managed:{0u,1u})
        variant("stopped-phase-club-notifications",[phase](State& s){s.memory.write<1>(owner+0x7929,phase);},[allowed,managed](Dependencies& d){d.outcomes[0x1014c2e0]={allowed};d.outcomes[0x101481d0]={managed};});
    for(auto allowed:{0u,1u})for(auto managed:{0u,1u})
        variant("regular-completion-club-notifications",[](State&){},[allowed,managed](Dependencies& d){d.outcomes[0x101d95a0]={4};d.outcomes[0x1014c2e0]={allowed};d.outcomes[0x101481d0]={managed};});
    for(auto time:{0u,2u})variant("restart-announcement",[time](State& s){s.memory.write<1>(owner+0x7928,8);s.memory.write<2>(owner+0x7948,time);},[](Dependencies& d){d.effect=[](State& s,std::uint32_t t,unsigned){if(t==0x101d5810)s.memory.write<1>(owner+0x7928,0);};});
    for(auto valid:{0u,UINT32_MAX})variant("restart-player-selection",[](State& s){s.memory.write<1>(owner+0x7928,1);},[valid](Dependencies& d){d.outcomes[0x101d3b50]={valid};d.effect=[](State& s,std::uint32_t t,unsigned){if(t==0x101d95a0)s.memory.write<1>(owner+0xe0b8,0);};});
    for(auto stopped:{0u,1u})variant("incident-notification",[stopped](State& s){s.memory.write<1>(owner+0x7928,stopped);},[](Dependencies& d){d.outcomes[0x101d6980]={0};d.outcomes[0x101d5880]={1};});
    for(auto stop_time:{0u,1u,0xffu})for(auto time:{0u,1u,32767u,0x8000u})variant("configured-abort",[stop_time,time](State& s){s.memory.write<1>(owner+0xe0b8,stop_time);s.memory.write<2>(owner+0x7948,time);});
    for(auto period:{0u,1u,2u,3u,4u,5u,UINT32_MAX})variant("period-result",[](State&){},[period](Dependencies& d){d.outcomes[0x101d95a0]={period};});
    for(auto phase:{0u,1u,2u})for(auto prior:{0xffu,0u})variant("full-period-score",[prior](State& s){s.memory.write<1>(record+0x4b,2);s.memory.write<1>(record+0x4c,3);s.memory.write<1>(record+0x47,prior);s.memory.write<1>(record+0x48,1);s.memory.write<1>(record+0x43,1);s.memory.write<1>(record+0x44,2);},[phase](Dependencies& d){d.outcomes[0x101d95a0]={4};d.effect=[phase](State& s,std::uint32_t t,unsigned){if(t==0x101d95a0)s.memory.write<1>(owner+0x7929,phase);};});
    variant("post-clock-restart",[](State&){},[](Dependencies& d){d.effect=[](State& s,std::uint32_t t,unsigned){if(t==0x101d6da0)s.memory.write<1>(owner+0x7928,1);};d.outcomes[0x101d3c90]={9,0};});
    for(auto count:{0u,2u,0xffu})variant("queue-count",[count](State& s){s.memory.write<1>(owner+0x792e,count);});
    for(auto flags:{1u,2u,4u,8u,16u,0x4000u})variant("actor-flags",[](State&){},[flags](Dependencies& d){d.outcomes[0x10289560]={flags};d.outcomes[0x101d56f0]={flags};d.outcomes[0x101ee9f0]={1};d.outcomes[0x101eea30]={1};d.outcomes[0x10283620]={1};d.outcomes[0x101d70d0]={player+0x200};});
    variant("existing-partner",[](State&){},[](Dependencies& d){d.outcomes[0x10285ef0]={player+0x200};});
    variant("excluded-actor",[](State&){},[](Dependencies& d){d.outcomes[0x101149f0]={1};});
    variant("other-side-action",[](State& s){s.memory.write<1>(player+0x27,1);},[](Dependencies& d){d.outcomes[0x101d56c0]={1};});
    for(auto action:{17u,18u,0x104u,0x76u})variant("player-action",[](State&){},[action](Dependencies& d){d.outcomes[0x101d56a0]={action};d.outcomes[0x101d3c90]={0};});
    for(auto action:{17u,18u})for(auto bits:{0u,0x80000000u,0x00000001u,0x3effffffu,0x3f000001u,0x7f7fffffu,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u})
        variant("probability-IEEE-input",[action,bits](State& s){s.memory.write<4>(player+(action==17?0x74:0xc4),bits);},[action](Dependencies& d){d.outcomes[0x101d56a0]={action};d.outcomes[0x101d3c90]={0};});
    for(auto divisor:{0.0,-0.0,1.0,10.0,1e300})variant("restart-time-division",[divisor](State& s){s.memory.write<8>(control+0x678,float_bits<8>(divisor));},[](Dependencies& d){d.effect=[](State& s,std::uint32_t t,unsigned){if(t==0x101d6da0)s.memory.write<1>(owner+0x7928,1);};d.outcomes[0x101d3c90]={9,0};});
    for(auto enabled:{1304u,1308u}) {
        auto s=clone(*seed);s->wr<1>(enabled,1);s->memory.write<4>(player+0x74,enabled==1304?0x7fc12345:float_bits<4>(0.15));
        compare(*s,"enabled-floating-exception",[](Dependencies& d){d.outcomes[0x101d56a0]={17};d.outcomes[0x101d3c90]={0};},false);
    }
    for(auto flag:{0x7928u,0x1c1u,0x7c68u})variant("action-causes-stop",[](State&){},[flag](Dependencies& d){d.effect=[flag](State& s,std::uint32_t t,unsigned){if(t==0x102775e0){if(flag==0x7c68)s.memory.write<4>(owner+flag,1);else s.memory.write<1>(owner+flag,1);}};d.outcomes[0x101d3c90]={9,0};});
    for(auto timer:{0u,1u,2u})variant("action-timer",[timer](State& s){s.memory.write<1>(owner+0x1de,timer);s.memory.write<1>(owner+0x791f,4);});
    for(auto position:{0u,1u})variant("ball-owner-position",[](State& s){s.memory.write<4>(owner+0x3104,player);s.memory.write<4>(owner+0xdff8,player);},[position](Dependencies& d){d.outcomes[0x101d5690]={position};d.outcomes[0x101d5680]={position};d.outcomes[0x101d5670]={1};d.outcomes[0x101d5660]={1};d.effect=[](State& s,std::uint32_t t,unsigned){if(t==0x10281af0){s.memory.write<1>(owner+0x791d,1);s.memory.write<1>(owner+0x791e,1);}};});
    variant("ball-owner-causes-stop",[](State& s){s.memory.write<4>(owner+0x3104,player);},[](Dependencies& d){d.effect=[](State& s,std::uint32_t t,unsigned){if(t==0x1023cd20)s.memory.write<1>(owner+0x7928,1);};d.outcomes[0x101d3c90]={9,0};});
    variant("refresh-team-position",[](State&){},[](Dependencies& d){d.effect=[](State& s,std::uint32_t t,unsigned n){if(t==0x101e4130&&n)s.memory.write<1>(owner+0x791d,1);};});
    auto counted=clone(*seed);Dependencies original;original.reference(*counted);
    for(std::uint64_t budget=0;budget<counted->executed;++budget){auto s=clone(*seed);s->budget=budget;compare(*s,"finite-budget",{},false);}
    const auto accesses=compare(*seed,"normal-accesses");std::set<std::uint32_t> bytes;
    for(const auto& a:accesses)for(std::uint64_t i=0;i<a.size;++i)bytes.insert(a.address+std::uint32_t(i));
    auto small=minimal(*seed,accesses);compare(*small,"minimal-normal");
    for(auto hole:bytes){auto s=minimal(*seed,accesses,hole);compare(*s,"distinct-accessed-byte-hole",{},false);}
    compare(*seed,"interrupted-child",[](Dependencies& d){d.effect=[](State& s,std::uint32_t t,unsigned){if(t==0x102775e0){s.memory.write<1>(owner+0x792d,7);throw std::runtime_error("Interrupted football child");}};},false);
    std::cout<<"0x101d3f60 comparisons="<<comparisons<<" faults="<<faults<<" budget_stops="<<budgets<<" own_negatives="<<negatives<<'\n';
}
}
int main(){try{matrix();}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
