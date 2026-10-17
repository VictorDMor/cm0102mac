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
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,18> ranges{{
    {attributes::refresh_entry,0x102373a0},{attributes::maximum_entry,0x102373b4},{attributes::recovery_entry,0x10235e9c},
    {attributes::periodic_entry,0x1023958c},{attributes::consumption_entry,0x1023978c},{attributes::restoration_entry,0x10239d10},
    {attributes::record_scale_entry,0x10187680},{attributes::availability_entry,0x101877dc},{attributes::effective_condition_entry,0x10187a1c},{attributes::record_refresh_entry,0x10188488},
    {attributes::capacity_entry,0x101878d0},{attributes::capacity_assignment_entry,0x10186cd0},
    {attributes::retirement_entry,0x1018bb4c},{attributes::reset_entry,0x10186fb4},{attributes::sampling_entry,0x10186a18},{attributes::queue_merge_entry,0x10186e0c},{attributes::queue_lookup_entry,0x10186e94},{attributes::post_match_entry,0x10187450}}};
void modern_root(State& s,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
    constexpr Function methods[]{attributes::refresh,attributes::signed_maximum,attributes::recover_condition,
        attributes::refresh_condition,attributes::consume_condition,attributes::restore_condition,
        attributes::record_condition_scale,attributes::record_availability,attributes::effective_condition,attributes::refresh_condition_record,
        attributes::record_capacity,attributes::assign_capacity,attributes::retire_injury,attributes::reset_condition_record,attributes::sample_condition_range,attributes::merge_condition_update,attributes::find_condition_update,attributes::apply_match_condition};
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
    std::int16_t injury_days=20;std::uint32_t calendar_first=2,calendar_second=20,scheduled=1,cache_wide=0;
    std::uint32_t forced_condition_site=0;std::uint8_t forced_condition_bits=0;
    std::uint32_t assignment_wide=0,retire_eligible=1;std::vector<std::uint32_t> queue;
    bool lifecycle_chain=false,lifecycle_mutate=false,sampling_chain=false,recursive_negative=false;
    std::uint32_t injury_result=2;bool post_match_chain=false,post_match_mutate=false;
    bool rich_queue=false,allocation_failure=false,queue_mutate=false;
    unsigned sampling_mask=0;std::array<std::uint32_t,3> sampling_bounds{};
    bool own_negative=false, child_negative=false, mutate=false;
};
struct Trace final : SemanticGuestObserver {
    Options options;
    std::vector<std::uint32_t> sites;
    explicit Trace(Options o) : options(o) {}
    void before_step(State& s, std::uint32_t pc) override {
        sites.push_back(pc);
        constexpr std::array<std::uint32_t,3> sampling_sites{0x101865b8,0x101866a4,0x10186790};
        constexpr std::array<unsigned,3> sampling_registers{22,21,20};
        for(unsigned n=0;n<3;++n)if((options.sampling_mask&(1u<<n)) && pc==sampling_sites[n] && s.r<4>(4)==stack-128) {
            const auto bound=options.sampling_bounds[n];s.wr<4>(sampling_registers[n]*4,bound);
            s.wr<1>(2304,(s.r<1>(2304)&0xf0)|(std::bit_cast<std::int32_t>(bound)<0?8:bound?4:2)|(s.r<1>(1024)&1));
        }
        if(options.assignment_wide && pc==0x10186b78)s.wr<4>(28*4,options.assignment_wide);
        if(options.forced_condition_site==pc)s.wr<1>(2304,options.forced_condition_bits);
        if(options.cache_wide && pc==0x10187e98)s.wr<4>(21*4,options.cache_wide);
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
    std::vector<Event> calls;unsigned queue_position=0;
    Model(bool old, Options o, Trace& t) : original(old), options(o), trace(t) {}
    void reference(State& s, std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;
        const auto found=std::find_if(ranges.begin(),ranges.end(),[&](const auto& range){return range.first==entry;});
        require(found!=ranges.end(),"Unknown actor reference root");const auto end=found->second;
        bool first=true;while(s.pc!=stop) {
            if(!first && entry==attributes::sampling_entry && s.pc==entry)call(s,s.pc,std::uint32_t(s.r<4>(4128)));
            else if(s.pc>=entry && s.pc<end)
                require(actor_attributes_reference(s),"Missing independent actor-attributes reference");
            else call(s,s.pc,std::uint32_t(s.r<4>(4128)));
            first=false;
        }
    }
    void call(State& s, std::uint32_t target, std::uint32_t continuation) override {
        Event e{target,continuation,s.registers,{}};
        if(target==0x12345678) throw std::runtime_error("Fixture actor-condition external destination");
        if((target==attributes::capacity_assignment_entry && options.post_match_chain) || (target==attributes::sampling_entry && options.sampling_chain) || (target==attributes::retirement_entry && options.lifecycle_chain) || target==attributes::maximum_entry || target==attributes::record_scale_entry ||
            target==attributes::availability_entry || target==attributes::effective_condition_entry) {
            if(original) reference(s,continuation);
            else modern_root(s,this,&trace);
        } else {
            require(target==0x1077b770 || target==0x10226520 || target==attributes::refresh_entry || target==0x10780fc0 || target==0x10289560 || target==0x10187a40 || target==0x10717dd0 ||
                target==0x10739890 || target==0x10768810 || target==0x1077b720 || target==0x10769b60 || target==0x10715880 || target==0x1018b9f0 || target==0x102ae9c0 ||
                target==0x102af650 || target==0x101929b0 || target==0x10718130 || target==0x101864a0 || target==0x10748ce0 || target==0x1014c2e0 || target==0x1018dcb0 ||
                target==0x10738e00 || target==0x10738e60 || target==0x1018fdf0 || target==0x10773520 || target==0x10738c00 || target==0x10738d10 || target==attributes::capacity_assignment_entry || target==0x10188930 || target==0x10188cc0,
                "Unknown actor-attributes dependency");
            s.memory.write<4>(globals+0x100+std::uint32_t(calls.size())*4,target^std::uint32_t(s.r<4>(12)));
            if(options.mutate) {
                s.memory.write<1>(actor+0x111,31); s.memory.write<1>(actor+0x113,13);
                s.memory.write<1>(actor+0x117,17); s.memory.write<1>(actor+0x112,19);
                s.memory.write<4>(actor+0x50,float_bits<4>(32.0));
            }
            if(target==0x10187a40 && options.mutate) {
                const auto row=s.memory.read<4>(s.r<4>(12))+s.memory.read<4>(s.r<4>(16))*0x24;
                s.memory.write<2>(row+8,8000);s.memory.write<2>(row+10,8000);s.memory.write<4>(row+0x10,0);
            }
            if(target==0x10187a40 && options.child_negative && !original) {
                const auto row=s.memory.read<4>(s.r<4>(12))+s.memory.read<4>(s.r<4>(16))*0x24;
                s.memory.write<4>(row+0x10,0);s.memory.write<2>(row+10,s.memory.read<2>(row+10)^1);
            }
            if(target==0x10717dd0) {
                const auto days=continuation==0x10187c14?options.injury_days:options.days;
                s.memory.write<2>(s.r<4>(12),std::uint16_t(days+(options.child_negative&&!original)));s.memory.write<2>(s.r<4>(12)+2,options.years);
            }
            if(target==0x10715880)s.memory.write<8>(s.r<4>(12),0);
            if(target==0x1018b9f0) {
                const auto row=s.memory.read<4>(s.r<4>(12))+s.memory.read<4>(s.r<4>(16))*0x24;
                s.memory.write<4>(row+0x10,0xffffffff);
            }
            if(target==attributes::capacity_assignment_entry && options.post_match_mutate) {
                s.memory.write<1>(fixture+0x6000+0x1b3,0);s.memory.write<1>(fixture+0x7800+0x40,0);
            }
            if(target==0x10188cc0) {s.memory.write<4>(fixture+0x4df8,s.r<4>(16));s.memory.write<4>(fixture+0x4dfc,s.r<4>(28));}
            if(target==0x101929b0) {
                s.memory.write<4>(s.r<4>(16),0x000ac001);s.memory.write<4>(s.r<4>(16)+4,0x01000000);
                s.memory.write<4>(s.r<4>(20),0x0014c001);s.memory.write<4>(s.r<4>(20)+4,0x02000000);
            }
            if(target==0x10739890) {s.memory.write<4>(s.r<4>(24),0x12345678);s.memory.write<4>(s.r<4>(28),0x87654321);}
            std::uint32_t queue_result=0;
            if(target==0x10748ce0) {
                s.memory.write<4>(s.r<4>(16),0x12345678);s.memory.write<4>(s.r<4>(20),0x87654321);
                if(options.lifecycle_mutate)s.memory.write<4>(staff,0);
            }
            if(target==0x1018fdf0 && options.child_negative && !original)s.memory.write<1>(records+0x24+0x20,1);
            if(target==0x10738e00 && !options.rich_queue && queue_position<options.queue.size()) {
                queue_result=fixture+0x4c00+8*queue_position;
                s.memory.write<4>(queue_result,options.queue[queue_position++]);
            }
            if(options.rich_queue) {
                const auto q=std::uint32_t(s.r<4>(12));
                if(target==0x10773520)queue_result=options.allocation_failure?0:fixture+0x4a00;
                if(target==0x10738c00) {
                    s.memory.write<4>(q+4,fixture+0x4c00);s.memory.write<4>(q+8,0);s.memory.write<4>(q+12,fixture+0x4c00);
                }
                if(target==0x10738e00) {
                    const auto cursor=s.memory.read<4>(q+12),base=s.memory.read<4>(q+4),count=s.memory.read<4>(q+8);
                    if(cursor<base+count*8) {queue_result=std::uint32_t(cursor);s.memory.write<4>(q+12,cursor+8);}
                    if(options.queue_mutate) {s.memory.write<1>(fixture+0x4b00,7);s.memory.write<1>(fixture+0x4b01,99);}
                }
                if(target==0x10738d10) {
                    const auto size=std::uint32_t(s.r<4>(20)),input=std::uint32_t(s.r<4>(16));
                    const auto base=s.memory.read<4>(q+4),count=s.memory.read<4>(q+8);
                    s.memory.copy_in(std::uint32_t(base+8*count),s.memory.copy_out(input,size));s.memory.write<4>(q+8,count+1);
                }
            }
            if(target==0x10738e60)s.memory.write<1>(fixture+0x4df0,1);
            for(unsigned n=0;n<=12;++n)if(n!=1&&n!=2)s.wr<4>(n*4,0xac000000+n*17);
            for(unsigned n=0;n<=13;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-11.0-n));
            s.wr<1>(2304,5); s.wr<1>(1027,1);
            s.wr<4>(12,target==0x1077b770 ? options.random : std::uint32_t(options.correction));
            if(target==0x10188930)s.wr<4>(12,options.injury_result ^ (options.child_negative && !original?1u:0u));
            if(target==0x1014c2e0)s.wr<4>(12,options.retire_eligible);
            if(target==0x10738e00 || target==0x10773520)s.wr<4>(12,queue_result);
            if(target==0x10289560)s.wr<4>(12,options.role);
            if(target==0x102af650)s.wr<4>(12,fixture+0x4740);
            if(target==0x101929b0)s.wr<4>(12,options.scheduled);
            if(target==0x10718130)s.wr<4>(12,continuation==0x10187e08?options.calendar_first:options.calendar_second);
            if(target==0x10780fc0) {
                s.memory.write<4>(s.r<4>(4)+0x14,s.r<4>(8));
                s.wr<8>(0x4010,float_bits<8>(options.power));
            }
            if(target==attributes::refresh_entry && options.child_negative && !original)
                s.memory.write<4>(actor+0x50,float_bits<4>(-31.0));
            s.pc=continuation;
        }
        if(options.recursive_negative && !original && target==attributes::sampling_entry)s.wr<4>(12,s.r<4>(12)+10000000);
        if(options.child_negative&&!original) {
            if(options.rich_queue)s.memory.write<1>(fixture+0x4c05,s.memory.read<1>(fixture+0x4c05)^1);
            else s.wr<4>(12,s.r<4>(12)^1);
        }
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
        if(!options.own_negative&&!options.child_negative&&!options.recursive_negative) {
            std::cerr<<"Final "<<hex_address(a->pc)<<" / "<<hex_address(b->pc)<<"; steps "<<old_trace.sites.size()<<" / "<<new_trace.sites.size()<<'\n';
            for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)
                if(old_trace.sites[n]!=new_trace.sites[n]) {std::cerr<<"First site "<<hex_address(old_trace.sites[n])<<" / "<<hex_address(new_trace.sites[n])<<'\n';break;}
            throw;
        }
        rejected=true;
    }
    require(rejected==(options.own_negative||options.child_negative||options.recursive_negative),label+" discriminating own/child negative");
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
        require(old.calls==modern.calls,label+" dependency ABI"); if(old_trace.sites!=new_trace.sites) {
            for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)if(old_trace.sites[n]!=new_trace.sites[n]) {
                std::cerr<<"Site "<<n<<" expected="<<hex_address(old_trace.sites[n])<<" actual="<<hex_address(new_trace.sites[n])<<std::endl;break;
            }
        }
        require(old_trace.sites==new_trace.sites,label+" original-site order");
    }
    ++cases; completed+=expected.empty()&&!rejected; interruptions+=!expected.empty(); negatives+=rejected;
    structural+=options.sampling_mask!=0 || options.forced_draw!=0 || options.cache_wide!=0 || options.assignment_wide!=0 || options.forced_condition_site!=0; sites.insert(old_trace.sites.begin(),old_trace.sites.end());
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

std::unique_ptr<State> cache_seed() {
    auto s=restoration_seed(attributes::record_refresh_entry);
    for(auto [slot,pointer]:std::array<std::pair<int,unsigned>,5>{{{-0x5450,fixture+0x4600},{-0x544c,fixture+0x4620},
        {-0x5448,fixture+0x4640},{-0x7b9c,fixture+0x4660},{-0x7a64,fixture+0x4680}}})s->memory.write<4>(toc+slot,pointer);
    s->memory.write<4>(fixture+0x4660,fixture+0x4900);s->memory.write<4>(fixture+0x4680,fixture+0x46a0);
    s->memory.write<4>(controller+8,2);s->memory.write<4>(staff+0x14,condition_profile);s->memory.write<4>(staff+0x20,0);s->memory.write<4>(staff+0x24,0);
    s->memory.write<1>(staff+0x67,6);s->memory.write<1>(records+0x24+0x22,10);
    for(auto offset:{-0x3947,-0x3948,-0x3949,-0x394a,-0x394c})s->memory.write<1>(toc+offset,1);
    s->memory.write<4>(records+0x24,0x000a07d1);s->memory.write<4>(records+0x24+4,0x01000000);
    s->memory.write<4>(diagnostic+0x210,0x000c07d1);s->memory.write<4>(diagnostic+0x214,0x02000000);
    s->memory.write<4>(fixture+0x4740,0x000c07d1);s->memory.write<4>(fixture+0x4744,0x03000000);
    for(auto [offset,value]:std::array<std::pair<unsigned,double>,10>{{{0,65535},{0x28,0.5},{0x30,0.15},{0x38,0.35},
        {0x40,0.75},{0x48,0.75},{0x50,1},{0x58,1.5},{0x60,0.0005},{0x68,0.8}}})s->memory.write<8>(record_constants+offset,float_bits<8>(value));
    return s;
}
void cache_checks() {
    auto input=cache_seed();boundaries(*input);Options negative;negative.own_negative=true;compare(*input,negative);
    negative.own_negative=false;negative.child_negative=true;compare(*input,negative);
    for(auto flags:{0u,1u,128u,255u}) {auto s=cache_seed();for(auto offset:{-0x3947,-0x3948,-0x3949,-0x394a,-0x394c})s->memory.write<1>(toc+offset,flags);boundaries(*s);}
    for(auto pointer:{0u,staff,stack-0x100}) {auto s=cache_seed();s->wr<4>(16,pointer);boundaries(*s);}
    for(auto index:{0u,1u,2u,0xffffffffu,0x80000000u,0x7fffffffu}) {auto s=cache_seed();s->memory.write<4>(staff,index);boundaries(*s);}
    for(auto pointer:{0u,condition_profile,stack-0x100}) {auto s=cache_seed();s->memory.write<4>(staff+0x14,pointer);boundaries(*s);}
    for(auto [day,year]:std::array<std::pair<unsigned,unsigned>,5>{{{12,2001},{10,2001},{12,2000},{12,2002},{65535,65535}}}) {
        auto s=cache_seed();s->memory.write<2>(records+0x24,day);s->memory.write<2>(records+0x24+2,year);boundaries(*s);
    }
    for(auto days:{-32768,-1,0,1,2,10,20,31,32767}) {Options o;o.days=std::int16_t(days);compare(*cache_seed(),o);}
    for(auto injury:{0xffffffffu,0u,1u,2u,3u,0xfffffffeu,0x80000000u}) {
        auto s=cache_seed();s->memory.write<4>(records+0x24+0x10,injury);boundaries(*s);
    }
    for(auto injury_days:{0,9,10,11,20,29,30,31,40})for(auto days:{1,2,10,20,35,45}) {
        auto s=cache_seed();s->memory.write<4>(records+0x24+0x10,0);Options o;o.days=std::int16_t(days);o.injury_days=std::int16_t(injury_days);
        compare(*s,o);if(days==20)boundaries(*s,o);
    }
    for(auto team:{0u,fixture+0x4700})for(auto day:{11u,12u,13u})for(auto year:{2000u,2001u,2002u}) {
        auto s=cache_seed();s->memory.write<4>(staff+0x20,team);s->memory.write<4>(staff+0x24,fixture+0x4800);
        s->memory.write<2>(fixture+0x4740,day);s->memory.write<2>(fixture+0x4742,year);Options o;o.scheduled=0;boundaries(*s,o);
    }
    for(bool equal:{false,true})for(auto day:{0u,12u})for(auto scheduled:{0u,1u,255u}) {
        auto s=cache_seed();s->memory.write<4>(staff+0x24,fixture+0x4800);s->memory.write<4>(toc-0x3944,equal?fixture+0x4800:0);
        s->memory.write<2>(fixture+0x4640,day);s->memory.write<2>(fixture+0x4642,2001);s->memory.write<1>(toc-0x394b,scheduled);
        Options o;o.scheduled=scheduled;boundaries(*s,o);
    }
    for(auto first:{0u,1u,2u,15u,20u,25u})for(auto second:{0u,14u,15u,16u,20u,25u}) {
        auto s=cache_seed();s->memory.write<4>(staff+0x24,fixture+0x4800);Options o;o.scheduled=0;o.calendar_first=first;o.calendar_second=second;o.days=2;
        compare(*s,o);if(first==2 || first==20)boundaries(*s,o);
    }
    for(auto counter:{0u,49u,50u,255u})for(auto trait:{0u,6u,127u,128u,250u,255u}) {
        auto s=cache_seed();s->memory.write<4>(staff+0x24,fixture+0x4800);s->memory.write<1>(staff+0x67,trait);s->memory.write<1>(fixture+0x4900+110,counter);
        Options o;o.scheduled=0;o.days=2;boundaries(*s,o);
    }
    for(auto wide:{65536u,0xffff0000u,0x7fffffffu,0x80000000u})for(auto correction:{-100000,0,10000000}) {
        auto s=cache_seed();s->memory.write<4>(staff+0x24,fixture+0x4800);Options o;o.scheduled=0;o.days=1;o.cache_wide=wide;o.correction=correction;
        boundaries(*s,o);
    }
    for(auto cursor:{stream+126,stream+128,0xfffffffeu}) {auto s=cache_seed();s->memory.write<4>(staff+0x24,fixture+0x4800);s->memory.write<4>(globals+8,cursor);Options o;o.scheduled=0;o.days=1;o.mutate=true;boundaries(*s,o);}
    for(auto current:{0u,100u,3999u,4000u,6999u,7000u,8999u,9000u,9001u,9849u,9850u,9851u,9999u,10000u,32767u,32768u,65535u})for(auto trait:{0u,10u,127u,128u,255u}) {
        auto s=cache_seed();s->memory.write<2>(records+0x24+8,current);s->memory.write<1>(condition_profile+0x34,trait);s->memory.write<1>(records+0x24+0x22,trait);
        Options o;o.days=2;compare(*s,o);if(trait==10)boundaries(*s,o);
    }
    for(auto scale:{0u,1000u,1001u,2000u,32767u,32768u,65535u})for(auto capacity:{0u,9999u,10000u,32767u,65535u}) {
        auto s=cache_seed();s->memory.write<2>(records+0x24+12,scale);s->memory.write<2>(records+0x24+10,capacity);boundaries(*s);
    }
    for(auto offset:{0x28u,0x30u,0x38u,0x40u,0x48u,0x50u,0x58u,0x60u,0x68u})for(auto bits:std::array<std::uint64_t,7>{0,0x8000000000000000ULL,0x7ff0000000000000ULL,0xfff0000000000000ULL,0x7ff8123456789000ULL,0x7ff0123456789000ULL,1}) {
        auto s=cache_seed();s->memory.write<8>(record_constants+offset,bits);s->memory.write<2>(records+0x24+12,2000);boundaries(*s);
        s->wr<1>(1304,1);s->wr<1>(1308,1);compare(*s);
    }
    {auto s=cache_seed();s->memory.write<2>(records+0x24+8,9000);boundaries(*s);}
    for(auto first:{2u,20u})for(auto cursor:{stream,stream+126,stream+128,0xfffffffeu}) {
        auto s=cache_seed();s->memory.write<2>(records+0x24+8,7000);s->memory.write<2>(records+0x24+10,7000);
        s->memory.write<4>(staff+0x24,fixture+0x4800);s->memory.write<4>(globals+8,cursor);
        Options o;o.scheduled=0;o.days=2;o.calendar_first=first;boundaries(*s,o);
    }
    // Preserve source branches that ordinary predicate invariants cannot reach.
    for(auto site:{0x10187c88u,0x1018801cu,0x1018838cu}) {
        auto s=cache_seed();Options o;o.forced_condition_site=site;o.forced_condition_bits=site==0x1018838c?4:8;
        if(site==0x10187c88) {s->memory.write<4>(records+0x24+0x10,0);o.injury_days=31;o.days=2;}
        if(site==0x1018801c) {s->memory.write<4>(staff+0x24,fixture+0x4800);o.scheduled=0;o.calendar_first=20;}
        boundaries(*s,o);
    }
    for(auto slot:{-0x5450,-0x544c,-0x5448,-0x7b9c,-0x543c,-0x7b80,-0x7be4}) {auto s=cache_seed();s->memory.write<4>(toc+slot,0);boundaries(*s);}
    {auto s=cache_seed();constexpr auto relocated=toc+0x10000;s->memory.map(relocated-0x8000,0x9000);s->memory.copy_in(relocated-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,relocated);boundaries(*s);}
}

std::unique_ptr<State> lifecycle_seed(std::uint32_t entry) {
    auto s=cache_seed();s->pc=entry;s->memory.write<4>(staff+0x14,condition_profile);s->memory.write<4>(staff+0x24,fixture+0x4800);
    s->memory.write<1>(condition_profile+0x3c,10);s->memory.write<1>(condition_profile+0x44,10);
    s->memory.write<4>(toc-0x7be0,fixture+0x4880);s->memory.write<4>(fixture+0x4880,staff-0x70);
    s->memory.write<4>(controller+0x28,0);s->memory.write<4>(records+0x24+0x10,0);s->memory.write<1>(injuries,1);
    s->memory.write<8>(record_constants+0x10,float_bits<8>(-0.0005));s->memory.write<8>(record_constants+0x18,float_bits<8>(0.5));
    s->wr<4>(12,controller);s->wr<4>(16,entry==attributes::reset_entry?1:staff);
    s->wr<4>(20,entry==attributes::capacity_assignment_entry?9000:entry==attributes::retirement_entry?1:0);s->wr<4>(24,5);return s;
}
void lifecycle_checks() {
    for(auto entry:{attributes::capacity_entry,attributes::capacity_assignment_entry,attributes::retirement_entry,attributes::reset_entry}) {
        Options o;o.lifecycle_chain=true;boundaries(*lifecycle_seed(entry),o);o.own_negative=true;compare(*lifecycle_seed(entry),o);
        o.own_negative=false;o.child_negative=true;auto s=lifecycle_seed(entry);
        if(entry==attributes::capacity_assignment_entry) {s->memory.write<2>(records+0x24+12,1200);s->memory.write<4>(globals+8,stream+126);}
        compare(*s,o);
    }
    for(auto percent:{0u,1u,128u,255u,256u})for(auto capacity:{0u,99u,100u,9999u,10000u,12345u,32767u,32768u,65535u}) {
        auto s=lifecycle_seed(attributes::capacity_entry);s->wr<4>(20,percent);s->memory.write<2>(records+0x24+10,capacity);boundaries(*s);
        Options o;o.mutate=true;compare(*s,o);
    }
    for(auto percent:{0u,1u,128u,256u}) {auto s=lifecycle_seed(attributes::capacity_entry);s->memory.write<4>(staff,2);s->wr<4>(20,percent);boundaries(*s);}
    for(auto scalar:{0u,1000u,1173u,1174u,1200u,1249u,1250u,32767u,32768u,64535u,64536u,65535u})for(auto delta:{0u,1u,5u,32767u,32768u,65535u}) {
        auto s=lifecycle_seed(attributes::capacity_assignment_entry);s->memory.write<2>(records+0x24+12,scalar);s->wr<4>(24,delta);boundaries(*s);
    }
    for(auto trait:{0u,127u,128u,255u})for(auto capacity:{0u,10000u,32767u,65535u}) {
        auto s=lifecycle_seed(attributes::capacity_assignment_entry);s->memory.write<1>(staff+0x67,trait);s->memory.write<1>(condition_profile+0x3c,trait);
        s->memory.write<1>(condition_profile+0x44,trait);s->memory.write<2>(records+0x24+12,1200);s->wr<4>(20,capacity);compare(*s);
    }
    for(auto cursor:{stream+126,stream+128,0xfffffffeu}) {
        auto s=lifecycle_seed(attributes::capacity_assignment_entry);s->memory.write<2>(records+0x24+12,1200);s->memory.write<4>(globals+8,cursor);boundaries(*s);
    }
    for(auto wide:{65536u,0xffff0000u,0x7fffffffu,0x80000000u})for(auto correction:{-100000,0,10000000}) {
        auto s=lifecycle_seed(attributes::capacity_assignment_entry);s->memory.write<2>(records+0x24+12,1200);Options o;o.assignment_wide=wide;o.correction=correction;boundaries(*s,o);
    }
    for(auto type:{0u,1u,80u,81u,82u,127u,255u})for(auto notify:{0u,1u,128u,255u,256u})for(auto eligible:{0u,1u,128u,255u,256u}) {
        auto s=lifecycle_seed(attributes::retirement_entry);s->memory.write<1>(injuries,type);s->wr<4>(20,notify);Options o;o.retire_eligible=eligible;compare(*s,o);
        if(notify==1 && eligible==1)boundaries(*s,o);
    }
    for(auto notify:{0u,1u}) {auto s=lifecycle_seed(attributes::retirement_entry);s->wr<4>(20,notify);s->memory.write<4>(staff+0x24,0);boundaries(*s);}
    for(auto index:{0xffffffffu,0u,1u,0x80000000u}) {auto s=lifecycle_seed(attributes::retirement_entry);s->memory.write<4>(records+0x24+0x10,index);boundaries(*s);}
    {auto s=lifecycle_seed(attributes::retirement_entry);Options o;o.lifecycle_mutate=true;boundaries(*s,o);}
    for(auto pointer:{0u,fixture+0x4a00})for(auto queue:std::array<std::vector<std::uint32_t>,4>{{{},{1},{2,1,1,3},{2,3}}}) {
        auto s=lifecycle_seed(attributes::reset_entry);s->memory.write<4>(controller+0x28,pointer);Options o;o.lifecycle_chain=true;o.queue=queue;boundaries(*s,o);
    }
    for(unsigned flag=0;flag<256;++flag) {auto s=lifecycle_seed(attributes::reset_entry);s->memory.write<1>(fixture+0x4900+113,flag);Options o;o.lifecycle_chain=true;auto result=compare(*s,o);
        require(result.state->memory.read<1>(fixture+0x4900+113)==(flag&0x7f),"Independent reset preserves every other flag bit");}
    for(auto entry:{attributes::capacity_entry,attributes::capacity_assignment_entry,attributes::retirement_entry,attributes::reset_entry}) {
        for(auto index:{0u,1u,2u,0xffffffffu,0x80000000u,0x7fffffffu}) {
            auto s=lifecycle_seed(entry);if(entry==attributes::reset_entry)s->wr<4>(16,index);else s->memory.write<4>(staff,index);Options o;o.lifecycle_chain=true;boundaries(*s,o);
        }
        for(auto argument:{12u,16u}) {auto s=lifecycle_seed(entry);s->wr<4>(argument,0);Options o;o.lifecycle_chain=true;boundaries(*s,o);}
        auto s=lifecycle_seed(entry);constexpr auto relocated=toc+0x10000;s->memory.map(relocated-0x8000,0x9000);s->memory.copy_in(relocated-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,relocated);Options o;o.lifecycle_chain=true;boundaries(*s,o);
    }
    for(auto entry:{attributes::capacity_entry,attributes::capacity_assignment_entry})for(auto offset:{0u,8u,0x10u,0x18u})for(auto bits:std::array<std::uint64_t,7>{0,0x8000000000000000ULL,0x7ff0000000000000ULL,0xfff0000000000000ULL,0x7ff8123456789000ULL,0x7ff0123456789000ULL,1}) {
        auto s=lifecycle_seed(entry);s->memory.write<8>(record_constants+offset,bits);s->memory.write<2>(records+0x24+12,64535);s->wr<4>(20,1);boundaries(*s);s->wr<1>(1304,1);s->wr<1>(1308,1);compare(*s);
    }
    for(auto bits:std::array<std::uint32_t,7>{0,0x80000000u,0x7f800000u,0xff800000u,0x7fc12345u,0x7f812345u,1}) {
        auto s=lifecycle_seed(attributes::capacity_entry);s->wr<4>(20,1);s->memory.write<4>(percentage_divisor,bits);boundaries(*s);s->wr<1>(1304,1);s->wr<1>(1308,1);compare(*s);
    }
}


std::unique_ptr<State> sampling_seed(std::uint32_t bound=1000000) {
    auto s=cache_seed();s->pc=attributes::sampling_entry;s->wr<4>(12,bound);
    s->memory.write<8>(record_constants,float_bits<8>(65535.0));s->memory.write<8>(record_constants+8,float_bits<8>(4503601774854144.0));return s;
}
void sampling_checks() {
    Options chain;chain.sampling_chain=true;
    for(auto bound:{0u,1u,2u,10u,255u,65534u,65535u,65536u,65537u,1000000u,0x7fffffffu,0x80000000u,0xffff0000u,0xffff0001u,0xfffffffeu,0xffffffffu})
        for(auto word:{0u,32767u,65534u,65535u})for(auto salt:{0u,65535u,0x7fffffffu,0x80000000u,0xffffffffu}) {
            auto s=sampling_seed(bound);s->memory.write<2>(stream+2,word);s->memory.write<4>(globals,salt);compare(*s,chain);
        }
    for(auto bound:{0u,1u,17u,65535u,65536u,1000000u,0x7fffffffu,0x80000000u,0xffff0000u,0xffff0001u,0xffffffffu})for(auto cursor:{stream,stream+124,stream+126,stream+128,0xfffffffeu}) {
        auto s=sampling_seed(bound);s->memory.write<4>(globals+8,cursor);boundaries(*s,chain);
    }
    for(int bound:{0,1,17,-17,65535,-65535}) {
        auto s=sampling_seed(std::uint32_t(bound));s->memory.write<2>(stream+2,100);s->memory.write<4>(globals,20);const auto r=compare(*s,chain);
        require(std::bit_cast<std::int32_t>(std::uint32_t(r.state->r<4>(12)))==(bound?(bound>0?120%bound:-(120%bound)):0),"Independent condition sampler signed remainder");
        require(r.state->memory.read<4>(globals+8)==stream+(bound?2:0),"Independent condition sampler zero/no-draw rule");
    }
    for(unsigned depth=0;depth<3;++depth)for(auto final_bound:{0u,1u,65535u,65536u,0xffff0000u,0x7fffffffu,0x80000000u}) {
        auto o=chain;o.sampling_mask=(1u<<(depth+1))-1;o.sampling_bounds.fill(65536);o.sampling_bounds[depth]=final_bound;boundaries(*sampling_seed(),o);
    }
    for(unsigned mask:{1u,3u,7u})for(auto cursor:{stream+120,stream+122,stream+124,stream+126}) {
        auto s=sampling_seed();s->memory.write<4>(globals+8,cursor);auto o=chain;o.sampling_mask=mask;o.sampling_bounds.fill(65536);boundaries(*s,o);
    }
    auto recursive=chain;recursive.sampling_mask=7;recursive.sampling_bounds.fill(65536);
    auto own=chain;own.own_negative=true;compare(*sampling_seed(17),own);
    auto child=chain;child.child_negative=true;auto wrap=sampling_seed();wrap->memory.write<4>(globals+8,stream+126);compare(*wrap,child);
    recursive.recursive_negative=true;compare(*sampling_seed(),recursive);recursive.recursive_negative=false;
    for(auto bound:{17u,1000000u}) {
        auto s=sampling_seed(bound);constexpr auto relocated=toc+0x10000;s->memory.map(relocated-0x8000,0x9000);s->memory.copy_in(relocated-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,relocated);boundaries(*s,chain);
        for(auto alias:{globals+8,globals+4,stream,stack-0x80,stack-0x30,stack+8}) {auto a=sampling_seed(bound);a->memory.write<4>(toc-0x7b90,alias);boundaries(*a,chain);}
        s=sampling_seed(bound);s->memory.write<4>(toc-0x7b88,globals+8);boundaries(*s,chain);
        s=sampling_seed(bound);s->memory.write<4>(toc-0x543c,stack-0x80);boundaries(*s,chain);
    }
    // The wide conversion overwrites this cursor slot. The following inline
    // advance must retain its cached r3 pointer and repair the slot before use.
    for(auto alias:{stack-0x40,stack-0x48}) {
        auto s=sampling_seed();s->memory.write<4>(toc-0x7b84,alias);s->memory.write<4>(alias,stream);boundaries(*s,chain);
    }
    for(auto field:{0u,8u})for(auto bits:std::array<std::uint64_t,8>{0,0x8000000000000000ULL,0x7ff0000000000000ULL,0xfff0000000000000ULL,0x7ff8123456789000ULL,0x7ff0123456789000ULL,1,0x3ff0000000000001ULL}) {
        auto s=sampling_seed();s->memory.write<8>(record_constants+field,bits);boundaries(*s,chain);s->wr<1>(1304,1);s->wr<1>(1308,1);compare(*s,chain);
    }
}

constexpr std::uint32_t update_queue=fixture+0x4a00,update_bytes=fixture+0x4b00,update_items=fixture+0x4c00;
std::unique_ptr<State> queue_seed(std::uint32_t entry=attributes::queue_merge_entry,unsigned count=1) {
    auto s=seed(entry);s->wr<4>(12,controller);s->wr<4>(16,staff);s->wr<4>(20,update_bytes);s->wr<4>(24,update_bytes+1);
    s->memory.write<4>(controller+0x28,update_queue);s->memory.write<4>(staff,1);
    s->memory.write<4>(update_queue+4,update_items);s->memory.write<4>(update_queue+8,count);s->memory.write<4>(update_queue+12,0xfeedbeef);
    for(unsigned n=0;n<count;++n) {s->memory.write<4>(update_items+8*n,n==1?2:1);s->memory.write<4>(update_items+8*n+4,0x0132aabb);}
    s->memory.write<1>(update_bytes,1);s->memory.write<1>(update_bytes+1,20);
    s->memory.write<2>(stack-96+0x3e,0xc3d4);return s;
}
void queue_checks() {
    Options o;o.rich_queue=true;
    const auto signed_byte=[](unsigned value){return value<128?int(value):int(value)-256;};
    for(auto old_kind:{0u,1u,127u,128u,255u})for(auto new_kind:{0u,1u,127u,128u,255u})
        for(auto old_amount:{0u,1u,50u,99u,100u,127u,128u,155u,255u})for(auto amount:{0u,1u,50u,99u,100u,127u,128u,155u,255u}) {
            auto s=queue_seed();s->memory.write<1>(update_items+4,old_kind);s->memory.write<1>(update_items+5,old_amount);
            s->memory.write<1>(update_bytes,new_kind);s->memory.write<1>(update_bytes+1,amount);auto r=compare(*s,o);
            const auto expected_kind=old_kind==new_kind || signed_byte(amount)<=signed_byte(old_amount)?old_kind:new_kind;
            const auto expected_amount=old_kind==new_kind?unsigned(std::min(100,signed_byte(old_amount)+signed_byte(amount)))&255:
                signed_byte(amount)>signed_byte(old_amount)?amount:old_amount;
            require(r.state->memory.read<1>(update_items+4)==expected_kind && r.state->memory.read<1>(update_items+5)==expected_amount,"Independent signed queue merge");
            require(r.state->memory.read<1>(update_bytes)==expected_kind && r.state->memory.read<1>(update_bytes+1)==expected_amount,"Retained queue bytes copied to caller");
        }
    for(auto count:{0u,1u,3u})for(auto key:{1u,2u,3u,0xffffffffu})for(auto root:{attributes::queue_merge_entry,attributes::queue_lookup_entry}) {
        auto s=queue_seed(root,count);s->memory.write<4>(staff,key);auto r=compare(*s,o);boundaries(*s,o);
        if(root==attributes::queue_merge_entry && (count==0 || (key!=1 && (count<2 || key!=2)))) {
            const auto tail=update_items+8*count;
            require(r.state->memory.read<4>(tail)==key && r.state->memory.read<4>(tail+4)==0x0114c3d4,"Append preserves uninitialized original tail padding");
        }
        if(root==attributes::queue_lookup_entry) {
            const auto expected=count && key==1?update_items:count>=2 && key==2?update_items+8:0;
            require(r.state->r<4>(12)==expected,"Borrowed first-match pointer or null");
            if(expected) {r.state->memory.write<1>(expected+5,93);require(r.state->memory.read<1>(expected+5)==93,"Borrowed item remains live");}
        }
    }
    for(auto root:{attributes::queue_merge_entry,attributes::queue_lookup_entry}) {
        auto s=queue_seed(root,0);s->memory.write<4>(controller+0x28,0);boundaries(*s,o);
        auto negative=o;negative.own_negative=true;compare(*queue_seed(root),negative);
        negative=o;negative.child_negative=true;compare(*queue_seed(root),negative);
        for(auto argument:{12u,16u,20u,24u}) {auto z=queue_seed(root);z->wr<4>(argument,0);boundaries(*z,o);}
        for(auto key:{update_items,update_queue+4,stack-20}) {auto z=queue_seed(root,3);z->wr<4>(16,key);boundaries(*z,o);}
    }
    {auto s=queue_seed(attributes::queue_merge_entry,0);s->memory.write<4>(controller+0x28,0);auto failure=o;failure.allocation_failure=true;auto r=compare(*s,failure);
        require(r.state->pc==0x10186d18 && r.state->memory.read<4>(controller+0x28)==0,"Allocation failure stores null then faults at original queue read");boundaries(*s,failure);}
    for(auto old_amount:{50u,100u,128u})for(auto amount:{1u,100u,255u})for(auto kind:{1u,7u}) {
        auto s=queue_seed();s->memory.write<1>(update_items+5,old_amount);s->memory.write<1>(update_bytes+1,amount);s->memory.write<1>(update_bytes,kind);boundaries(*s,o);
    }
    for(auto first:{update_bytes,update_items,update_items+4,update_items+5,update_items+6,stack-96+0x3c,stack-20})
        for(auto second:{first,first+1,update_bytes+1}) {
            auto s=queue_seed();s->wr<4>(20,first);s->wr<4>(24,second);boundaries(*s,o);
        }
    auto mutating=o;mutating.queue_mutate=true;boundaries(*queue_seed(),mutating);
}

constexpr std::uint32_t match_rows=fixture+0x6000,match_context=fixture+0x7800,match_limit_date=fixture+0x7880;
std::unique_ptr<State> post_match_seed(unsigned team=0,unsigned position=0) {
    auto s=lifecycle_seed(attributes::post_match_entry);s->memory.map(fixture+0x6000,0x2000);
    s->wr<4>(12,controller);s->wr<4>(16,match_context);s->wr<4>(20,match_rows);s->wr<4>(18*4,0x12345678);
    s->memory.write<4>(toc-0x7a9c,match_limit_date);s->memory.write<4>(match_limit_date,0x000c07d1);
    s->memory.write<8>(record_constants+0x20,float_bits<8>(0.5));s->memory.write<2>(staff+0x5c,25);
    for(unsigned side=0;side<2;++side)for(unsigned n=0;n<20;++n)s->memory.write<4>(match_rows+side*0x664+n*0x4c+0x194,0xffffffff);
    const auto row=match_rows+team*0x664+position*0x4c;
    s->memory.write<4>(row+0x194,1);s->memory.write<1>(row+0x1c4,255);s->memory.write<1>(row+0x1c6,255);s->memory.write<1>(row+0x1c8,255);
    s->memory.write<2>(row+0x1ca,9000);s->memory.write<2>(match_rows+0x4e,90);s->memory.write<4>(match_context,0xfffffffe);
    s->memory.write<4>(controller+0x28,update_queue);s->memory.write<4>(update_queue+4,update_items);s->memory.write<4>(update_queue+8,1);
    s->memory.write<4>(update_queue+12,0);s->memory.write<4>(update_items,1);s->memory.write<4>(update_items+4,0x0163aabb);
    s->memory.write<1>(row+0x1b3,1);s->memory.write<1>(condition_profile+15,20);return s;
}
void post_match_checks() {
    Options o;o.rich_queue=true;
    boundaries(*post_match_seed(),o);
    for(unsigned team=0;team<2;++team)for(unsigned position=0;position<20;++position) {
        auto s=post_match_seed(team,position);auto r=compare(*s,o);
        const auto assignments=std::count_if(r.calls.begin(),r.calls.end(),[](const auto& e){return e.target==attributes::capacity_assignment_entry;});
        require(assignments==1,"Exactly the selected team/position reaches capacity assignment");
    }
    for(auto position:{0u,11u})for(auto full:{0u,1u})for(auto end:{0u,70u,127u,128u,255u})for(auto earlier:{30u,255u})for(auto start:{20u,255u}) {
        auto s=post_match_seed(0,position);const auto row=match_rows+position*0x4c;
        s->memory.write<1>(match_rows+0x4c,full);s->memory.write<1>(row+0x1c8,end);s->memory.write<1>(row+0x1c4,earlier);s->memory.write<1>(row+0x1c6,start);
        auto r=compare(*s,o);auto e=std::find_if(r.calls.begin(),r.calls.end(),[](const auto& e){return e.target==attributes::capacity_assignment_entry;});require(e!=r.calls.end(),"Minutes assignment call");
        const auto sign=[](unsigned x){return x<128?int(x):int(x)-256;};
        const auto stop=sign(end)>-1?sign(end):sign(earlier)>-1?sign(earlier):90;
        const auto minutes=position<11?(full?90:stop):start==255?0:sign((unsigned(stop)-start) & 255);
        const auto expected=(minutes+10)/2;
        require(signed_value<4>(read_be<4>(e->before.data()+24))==expected,"Independent minutes plus depletion, halved for inactive context");
        if(end==255 && earlier==255)boundaries(*s,o);
    }
    for(auto trait:{0u,25u,35u,127u,128u,255u})for(auto cursor:{stream,stream+124,stream+126}) {
        auto s=post_match_seed();s->memory.write<1>(match_context+0x40,1);s->memory.write<2>(staff+0x5c,trait);s->memory.write<4>(globals+8,cursor);boundaries(*s,o);
        for(auto salt:{0u,0x7fffffffu,0x80000000u,0xffffffffu}) {s->memory.write<4>(globals,salt);compare(*s,o);}
    }
    for(auto value:{0u,1u,32767u,32768u,65535u}) {auto s=post_match_seed();s->memory.write<2>(match_rows+0x1ca,value);boundaries(*s,o);}
    for(auto current:std::array<std::uint32_t,7>{0x000107d0,0x000107d2,0x000b07d1,0x000c07d1,0x000d07d1,0x800007d1,0x7fff07d1}) {
        auto s=post_match_seed();s->memory.write<4>(diagnostic+0x210,current);boundaries(*s,o);
    }
    for(auto amount:{0u,50u,51u,65u,66u,95u,96u,127u,128u,255u})for(auto kind:{1u,7u,255u})for(auto result:{0u,2u,127u,128u,255u}) {
        auto s=post_match_seed();s->memory.write<1>(update_items+4,kind);s->memory.write<1>(update_items+5,amount);auto m=o;m.injury_result=result;auto r=compare(*s,m);
        const auto select=std::find_if(r.calls.begin(),r.calls.end(),[](const auto& e){return e.target==0x10188930;});require(select!=r.calls.end(),"Queued injury selector");
        auto snap=clone(*s);snap->registers=select->before;
        const auto severity=amount>=128 || amount<=50?0x12345678u:amount>95?2u:amount>65?1u:0u;
        require(snap->r<4>(20)==severity,"Independent strict signed severity thresholds and incoming-register residue");
        if(kind==1 && result==2)boundaries(*s,m);
    }
    for(auto trait:{0u,14u,15u,127u,128u,255u}) {auto s=post_match_seed();s->memory.write<1>(update_items+4,7);s->memory.write<1>(condition_profile+15,trait);boundaries(*s,o);}
    for(auto context:{0u,0xfffffffeu}) {auto s=post_match_seed();s->memory.write<4>(match_context,context);boundaries(*s,o);}
    for(auto queue:{0u,update_queue})for(auto flag:{0u,1u,128u,255u}) {auto s=post_match_seed();s->memory.write<4>(controller+0x28,queue);s->memory.write<1>(match_rows+0x1b3,flag);boundaries(*s,o);}
    {auto s=post_match_seed();s->memory.write<4>(update_queue+8,3);s->memory.write<4>(update_items+8,2);s->memory.write<4>(update_items+16,1);s->memory.write<4>(update_items+20,0x0132aabb);boundaries(*s,o);}
    for(auto id:{0u,2u,0xffffffffu,0x80000000u}) {auto s=post_match_seed();s->memory.write<4>(match_rows+0x194,id);boundaries(*s,o);}
    {auto s=post_match_seed();s->memory.write<4>(fixture+0x4880,0xffffff90);s->memory.write<1>(match_rows+0x1b3,0);boundaries(*s,o);}
    for(auto arg:{12u,16u,20u}) {auto s=post_match_seed();s->wr<4>(arg,0);boundaries(*s,o);}
    {auto s=post_match_seed();constexpr auto relocated=toc+0x10000;s->memory.map(relocated-0x8000,0x9000);s->memory.copy_in(relocated-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,relocated);boundaries(*s,o);}
    {auto s=post_match_seed();auto m=o;m.post_match_mutate=true;boundaries(*s,m);}
    {auto s=post_match_seed();auto m=o;m.post_match_chain=m.lifecycle_chain=true;boundaries(*s,m);}
    for(bool own:{false,true}) {auto m=o;m.own_negative=own;m.child_negative=!own;compare(*post_match_seed(),m);}
    for(auto alias:{stack-176+0x50,globals+8,stream}) {auto s=post_match_seed();s->memory.write<4>(toc-0x7a9c,alias);boundaries(*s,o);}
    for(auto bits:std::array<std::uint64_t,8>{0,0x8000000000000000ULL,0x7ff0000000000000ULL,0xfff0000000000000ULL,0x7ff8123456789000ULL,0x7ff0123456789000ULL,1,0x3ff0000000000001ULL}) {
        auto s=post_match_seed();s->memory.write<1>(match_context+0x40,1);s->memory.write<2>(staff+0x5c,127);s->memory.write<8>(record_constants+0x20,bits);boundaries(*s,o);s->wr<1>(1304,1);s->wr<1>(1308,1);compare(*s,o);
    }
}

}
namespace cm::aot {void actor_attributes_step_seen(State& s,std::uint32_t pc) {if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv) {try {
    if(argc==2 && std::string(argv[1])=="--post-match") {
        post_match_checks();unsigned covered=0;for(auto pc=attributes::post_match_entry;pc<0x10187450;pc+=4) {covered+=sites.contains(pc);if(!sites.contains(pc))std::cerr<<"Uncovered post-match "<<hex_address(pc)<<std::endl;}
        require(covered==292,"Complete post-match source coverage");std::cout<<"Focused post-match comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interruptions<<" negatives="<<negatives<<" sourceSites="<<covered<<std::endl;return 0;
    }
    if(argc==2 && std::string(argv[1])=="--queue") {
        queue_checks();for(auto [start,end]:ranges)if(start==attributes::queue_merge_entry || start==attributes::queue_lookup_entry)
            for(auto pc=start;pc<end;pc+=4)if(pc!=0x10186d24 && pc!=0x10186e4c && !sites.contains(pc))std::cerr<<"Uncovered queue "<<hex_address(pc)<<std::endl;
        require(sites.size()==110,"Complete queue source coverage");std::cout<<"Focused queue comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interruptions<<" negatives="<<negatives<<" sourceSites="<<sites.size()<<std::endl;return 0;
    }
    if(argc==2 && std::string(argv[1])=="--sampling") {
        sampling_checks();for(auto pc=attributes::sampling_entry;pc<0x10186a18;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered sampling "<<hex_address(pc)<<std::endl;
        require(sites.size()==350,"Complete sampling source coverage");std::cout<<"Focused sampling comparisons="<<cases<<" sourceSites="<<sites.size()<<std::endl;return 0;
    }
    if(argc==2 && std::string(argv[1])=="--lifecycle") {
        lifecycle_checks();
        for(auto [start,end]:ranges)if(start==attributes::capacity_entry || start==attributes::capacity_assignment_entry || start==attributes::retirement_entry || start==attributes::reset_entry)
            for(auto pc=start;pc<end;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered lifecycle "<<hex_address(pc)<<std::endl;
        require(sites.size()==388,"Complete lifecycle source coverage");
        std::cout<<"Focused lifecycle comparisons="<<cases<<" sourceSites="<<sites.size()<<std::endl;return 0;
    }
    require(argc==1,"Unknown actor-attribute check selection");
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
    cache_checks();
    lifecycle_checks();
    sampling_checks();
    queue_checks();
    post_match_checks();
    for(auto [start,end]:ranges)
        for(auto pc=start;pc<end;pc+=4)if(pc!=0x10186d24 && pc!=0x10186e4c && !sites.contains(pc))std::cerr<<"Uncovered "<<hex_address(pc)<<'\n';
    require(sites.size()==3982,"All 3982 actor-attribute/condition original sites covered");
    std::cout<<"PASS: "<<cases<<" actor-attribute comparisons; "<<completed<<" completions, "<<interruptions
        <<" interruptions, "<<negatives<<" own/child negatives, "<<structural<<" paired structural cases; all "<<sites.size()
        <<" original sites; full state/effects/ABI/accounting; independent original references\n";
} catch(const std::exception& e) {std::cerr<<e.what()<<'\n';return 1;}}
