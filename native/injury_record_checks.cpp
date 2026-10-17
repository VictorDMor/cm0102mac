#include "injury_record.hpp"
#include "injury_record_tables.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot {bool injury_record_reference(State&);}
namespace {
using namespace cm::semantic_io_checks;
namespace injury=cm::aot::injury_record;
constexpr std::uint32_t manager=fixture+0x1000,staff=fixture+0x2000,rows=fixture+0x4000,injuries=fixture+0x8000;
constexpr std::uint32_t allocation=fixture+0x10000,profile=fixture+0x14000,details=fixture+0x18000,table=fixture+0x19000;
constexpr std::uint32_t diagnostic=fixture+0x1a000,objects=fixture+0x1c000,globals=fixture+0x1d000,pool=fixture+0x1d100;
constexpr std::uint32_t constants=fixture+0x1d800,stats=fixture+0x1f000,club=fixture+0x23000,side=fixture+0x24000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,3> ranges{{{injury::registration_entry,0x1018aec4},{injury::notice_entry,0x1018b2b0},{injury::propagation_entry,0x1018b9ec}}};
void modern_root(State& s,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* observer) {
    using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
    constexpr Function methods[]{injury::register_injury,injury::prepare_injury_notice,injury::propagate_injury_risk};
    auto i=std::find(injury::entries.begin(),injury::entries.end(),s.pc);require(i!=injury::entries.end(),"Unknown injury component root");methods[i-injury::entries.begin()](s,dependencies,observer);
}
struct Options {
    unsigned interrupt=0,recipients=0;std::uint32_t random=0x1234567,correction=0,medical=0,eligible=1,notice_eligible=1,current_club=objects+0x300,related_type=11;
    std::uint32_t force_pc=0,force_value=65536,wrap_pc=0;unsigned force_register=13;int force_cr=-1;
    std::vector<std::uint32_t> club_results;
    bool own_negative=false,child_negative=false,notice_chain=false,propagation_chain=false,registration_chain=false,allocation_failure=false,refresh_mutate=false,reseed_mutate=false,notice_mutate=false;
};
struct Trace final:SemanticGuestObserver {
    Options options;std::vector<std::uint32_t> sites;
    explicit Trace(Options value):options(value){}
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);
        if(options.wrap_pc==pc) {
            s.pc=pc;const auto end=std::uint32_t(s.memory.read<4>(globals+4));s.memory.write<4>(globals+8,end);
            if(pc==0x10189620)s.wr<4>(12,end);
        }
        if(options.force_pc==pc) {
            s.wr<4>(options.force_register*4,options.force_value);
            if(options.force_cr>=0)s.wr<1>(2304,(s.r<1>(2304)&0xf0)|unsigned(options.force_cr)|(s.r<1>(1024)&1));
        }
    }
};
thread_local Trace* original_trace=nullptr;
struct Event {std::uint32_t target,continuation;decltype(State::registers) before,after;bool operator==(const Event&)const=default;};
struct Model final:SemanticDependencyDispatch {
    bool original;Options options;Trace& trace;std::vector<Event> calls;unsigned club_queries=0;
    Model(bool old,Options value,Trace& observer):original(old),options(value),trace(observer){}
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto r){return r.first==entry;});require(range!=ranges.end(),"Unknown injury reference root");
        while(s.pc!=stop) {
            if(s.pc>=entry && s.pc<range->second)require(injury_record_reference(s),"Missing independent injury reference");
            else call(s,s.pc,std::uint32_t(s.r<4>(4128)));
        }
    }
    void resume(State& s,std::uint32_t,std::uint32_t stop)override {
        auto* previous=original_trace;original_trace=&trace;
        try {
            while(s.pc!=stop) {
                if(s.pc>=injury::propagation_entry && s.pc<0x1018b9ec)require(injury_record_reference(s),"Missing injury continuation reference");
                else call(s,s.pc,std::uint32_t(s.r<4>(4128)));
            }
        } catch(...) {original_trace=previous;throw;}
        original_trace=previous;
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        Event event{target,continuation,s.registers,{}};
        if((target==injury::registration_entry && options.registration_chain)||(target==injury::notice_entry && options.notice_chain)||(target==injury::propagation_entry && options.propagation_chain)) {
            if(original)reference(s,continuation);else modern_root(s,this,&trace);
        } else {
            constexpr std::array<std::uint32_t,33> allowed{0x100d2f50,0x100d3040,0x1014c2e0,0x101864a0,0x10187a40,0x1018aed0,0x1018b6e0,0x1018bb50,0x102b8460,0x102de470,0x1030d4a0,0x10363b80,0x103d0990,0x10717b20,0x10739890,0x10748ce0,0x10768810,0x10769b60,0x10775130,0x107751b0,0x1077b720,0x1077b770,0x10082f20,0x102bdf90,0x102cb4b0,0x102cc140,0x102cc8e0,0x102cdbf0,0x1030dcd0,0x103fc710,0x1018c2f0,0x10188cc0,0x12345678};
            require(std::find(allowed.begin(),allowed.end(),target)!=allowed.end(),"Unknown injury dependency "+hex_address(target));
            if(target==0x12345678)throw std::runtime_error("Fixture injury external destination");
            const auto arg=[&](unsigned n){return std::uint32_t(s.r<4>(n*4));};
            s.memory.write<4>(fixture+0x30000+unsigned(calls.size())*4,target^arg(3));
            if(target==0x10739890) {s.memory.write<4>(arg(6),0x12345678);s.memory.write<4>(arg(7),0x87654321);}
            if(target==0x10748ce0) {s.memory.write<4>(arg(4),0x12345678);s.memory.write<4>(arg(5),0x87654321);}
            if(target==0x10775130 || target==0x107751b0) {
                if(!options.allocation_failure) {
                    const auto size=target==0x10775130?arg(3):arg(4);require(size<=0x10000,"Fixture injury allocation bound");
                    auto bytes=std::vector<std::uint8_t>(size,0x5a);
                    if(target==0x107751b0) {const auto old=s.memory.copy_out(arg(3),s.memory.read<4>(manager+8)*0x38);require(old.size()<=bytes.size(),"Fixture injury growth allocation");std::copy(old.begin(),old.end(),bytes.begin());}
                    s.memory.copy_in(allocation,bytes);
                }
            }
            if(target==0x102bdf90)s.memory.copy_in(arg(3),std::vector<std::uint8_t>(80,0));
            if(target==0x102cb4b0)s.memory.write<4>(arg(3)+arg(4)*4,arg(5));
            if(target==0x1030d4a0) {s.memory.write<4>(arg(5)+0x14,7);s.memory.write<4>(arg(5)+0x18,9);s.memory.write<1>(arg(5)+0x1c,0);}
            if(target==0x10717b20) {s.memory.write<2>(arg(3),20+arg(5));s.memory.write<2>(arg(3)+2,2001);s.memory.write<1>(arg(3)+4,2);}
            if(target==0x103fc710)for(unsigned n=0;n<options.recipients && n<30;++n)s.memory.write<4>(arg(6)+4*n,club+0x400+n*4);
            if(target==0x10187a40 && options.refresh_mutate) {s.memory.write<4>(staff,1);s.memory.write<4>(manager,rows+0x100);s.memory.write<4>(rows+0x100+0x24+0x10,0xffffffff);}
            if(target==0x1077b770 && options.reseed_mutate) {s.memory.write<1>(profile+0x2d,31);s.memory.write<4>(objects+0x414,stats+0x100);}
            if(target==0x1030dcd0 && options.notice_mutate) {s.memory.write<2>(injuries+0x30,101);s.memory.write<4>(staff+0x20,club+0x500);}
            for(unsigned n=0;n<=12;++n)if(n!=1 && n!=2)s.wr<4>(n*4,0xac000000+n*17);
            for(unsigned n=0;n<=13;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-11.0-n));
            s.wr<1>(2304,5);s.wr<1>(1027,1);std::uint32_t result=0;
            if(target==0x1077b770)result=options.random;
            if(target==0x101864a0)result=options.correction;
            if(target==0x100d2f50)result=options.medical;
            if(target==0x1014c2e0)result=options.eligible;
            if(target==0x102cdbf0)result=options.notice_eligible;
            if(target==0x10082f20) {result=club_queries<options.club_results.size()?options.club_results[club_queries]:options.current_club;++club_queries;}
            if(target==0x103fc710)result=options.recipients;
            if(target==0x1018c2f0)result=options.related_type;
            if(target==0x10775130 || target==0x107751b0)result=options.allocation_failure?0:allocation;
            s.wr<4>(12,result);s.pc=continuation;
        }
        if(options.child_negative && !original)s.wr<4>(12,s.r<4>(12)^1);
        event.after=s.registers;calls.push_back(event);
        if(options.interrupt && calls.size()>=options.interrupt)throw std::runtime_error("Fixture injury dependency interruption");
    }
};
std::unique_ptr<State> seed(std::uint32_t entry=injury::registration_entry) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);for(unsigned n=1280;n<1312;++n)s->wr<1>(n,0);
    s->memory.map(fixture,0x40000);s->memory.map(stack-0x4000,0x5000);s->memory.map(toc-0x8000,0x9000);
    s->pc=entry;s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);
    for(auto [slot,pointer]:{std::pair{-0x7be4,objects+0x100},std::pair{-0x7be0,objects+0x410},std::pair{-0x7b9c,objects+0x414},
        std::pair{-0x64d4,details},std::pair{-0x545c,table},std::pair{-0x5438,diagnostic},std::pair{-0x543c,constants},
        std::pair{-0x7ba8,objects},std::pair{-0x7bac,objects+0x104},std::pair{-0x7bb8,objects+0x108},std::pair{-0x7b80,objects+0x800},
        std::pair{-0x7b90,globals},std::pair{-0x7b8c,pool},std::pair{-0x7b88,globals+4},std::pair{-0x7b84,globals+8},
        std::pair{-0x7a7c,objects+0x404},std::pair{-0x7a64,objects+0x408},std::pair{-0x7b28,objects+0x40c},std::pair{-0x7b40,objects+0x400},
        std::pair{-0x7b7c,objects+0x418},std::pair{-0x7b78,objects+0x41c},std::pair{-0x7bf0,objects+0x420},std::pair{-0x7be8,objects+0x424}})s->memory.write<4>(toc+slot,pointer);
    s->memory.write<4>(objects+0x100,2);s->memory.write<4>(objects+0x410,staff);s->memory.write<4>(objects+0x414,stats);s->memory.write<4>(objects+0x418,objects);s->memory.write<4>(objects+0x420,objects);s->memory.write<4>(objects+0x424,club);
    s->memory.write<4>(manager,rows);s->memory.write<4>(manager+4,injuries);s->memory.write<4>(manager+8,2);s->memory.write<4>(staff,0);s->memory.write<4>(staff+0x70,1);
    for(unsigned n=0;n<2;++n) {s->memory.write<4>(rows+n*0x24+0x10,0xffffffff);s->memory.write<1>(rows+n*0x24+0x14,255);s->memory.write<1>(injuries+n*0x38,255);s->memory.write<4>(staff+n*0x70+0x14,profile+n*0x100);}
    s->memory.write<2>(staff+0x5c,25);s->memory.write<1>(profile+0x2d,10);s->memory.write<1>(profile+0x34,10);s->memory.write<2>(profile+12,9000);
    for(unsigned n=0;n<82;++n) {s->memory.write<2>(details+n*12+4,5);s->memory.write<2>(details+n*12+6,0);s->memory.write<1>(details+n*12+8,50);}
    for(unsigned n=0;n<66;++n)s->memory.write<4>(table+4*n,original_injury_propagation_table[n]);
    s->memory.write<4>(globals,0);s->memory.write<4>(globals+4,pool+0xfe);s->memory.write<4>(globals+8,pool);
    s->memory.write<8>(constants,float_bits<8>(65535.0));s->memory.write<8>(constants+8,float_bits<8>(4503601774854144.0));s->memory.write<8>(constants+0x70,float_bits<8>(100.0));
    s->memory.write<2>(objects+0x800,20);s->memory.write<2>(objects+0x802,2001);s->memory.write<1>(objects+0x804,2);s->memory.write<4>(objects+0x304,1);s->memory.write<4>(side+0x98,staff);
    s->wr<4>(12,manager);s->wr<4>(16,entry==injury::registration_entry?0:entry==injury::notice_entry?staff:rows);s->wr<4>(20,entry==injury::registration_entry?0:injuries);
    s->wr<4>(24,staff);s->wr<4>(28,0);s->wr<4>(32,0);s->wr<4>(36,0xffffffff);s->wr<4>(40,0);s->memory.write<4>(stack+0x38,0);return s;
}

unsigned cases=0,completed=0,interruptions=0,negatives=0,structural=0;
std::set<std::uint32_t> sites;
struct Result { std::unique_ptr<State> state; std::vector<Effects::Access> accesses; std::vector<Event> calls;unsigned club_queries=0; };
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
    const auto label="Injury record entry="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(!expected.starts_with("Unknown injury"),expected);
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
        require(old.calls==modern.calls,label+" dependency ABI"); if(old_trace.sites!=new_trace.sites) {
            for(unsigned n=0;n<std::min(old_trace.sites.size(),new_trace.sites.size());++n)if(old_trace.sites[n]!=new_trace.sites[n]) {
                std::cerr<<"Site "<<n<<" expected="<<hex_address(old_trace.sites[n])<<" actual="<<hex_address(new_trace.sites[n])<<std::endl;break;
            }
        }
        require(old_trace.sites==new_trace.sites,label+" original-site order");
    }
    if((options.force_pc || options.wrap_pc) && expected.empty())require(std::find(old_trace.sites.begin(),old_trace.sites.end(),options.force_pc?options.force_pc:options.wrap_pc)!=old_trace.sites.end(),label+" structural control reached");
    ++cases; completed+=expected.empty()&&!rejected; interruptions+=!expected.empty(); negatives+=rejected;
    structural+=options.force_pc!=0 || options.wrap_pc!=0; sites.insert(old_trace.sites.begin(),old_trace.sites.end());
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

void sampled_boundaries(const State& input,Options options={}) {
    auto baseline=compare(input,options);const auto steps=baseline.state->executed-input.executed;
    for(auto cut:{std::uint64_t(0),std::uint64_t(1),steps/2,steps?steps-1:0,steps}) {auto x=clone(input);x->budget=x->executed+cut;compare(*x,options);}
    std::set<std::uint32_t> bytes;for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(auto hole:bytes) {auto x=std::make_unique<State>();x->registers=input.registers;x->unique=input.unique;x->pc=input.pc;
        for(auto byte:bytes)if(byte!=hole){x->memory.map(byte,1);x->memory.write<1>(byte,input.memory.read<1>(byte));}compare(*x,options);}
    for(unsigned n=1;n<=baseline.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}
std::unique_ptr<State> retirement_seed(unsigned age=40,unsigned duration=300) {
    auto x=seed();x->memory.write<2>(staff+0x5c,age);x->memory.write<2>(details+4,duration);x->memory.write<1>(details+8,100);x->memory.write<1>(profile+0x2d,20);x->memory.write<1>(stats+10,100);return x;
}
void structural_checks() {
    for(auto [pc,reg]:std::array<std::pair<std::uint32_t,unsigned>,15>{{{0x10188f28,13},{0x10189490,22},{0x10189644,19},
        {0x10189804,13},{0x1018997c,13},{0x10189b08,25},{0x10189e84,20},{0x1018a0b4,14},{0x1018a230,20},
        {0x1018a4c8,13},{0x1018a620,20},{0x1018a7dc,13},{0x1018a9a8,13},{0x1018ab64,13},{0x1018ad18,13}}}) {
        auto x=seed();
        if(pc==0x10188f28) {x->wr<4>(16,11);x->wr<4>(40,236);x->memory.write<4>(stack+0x38,1);x->memory.write<1>(details+11*12+9,1);}
        if(pc==0x10189644) {x->memory.write<2>(details+6,20);x->memory.write<2>(pool+2,4);}
        if(pc==0x10189804 || pc==0x1018997c) {x->memory.write<2>(staff+0x5c,40);x->memory.write<2>(details+4,12);x->memory.write<1>(profile+0x2d,20);}
        if(pc==0x10189e84)x->wr<4>(20,1);
        if(pc==0x1018a0b4 || pc==0x1018a230) {x->wr<4>(16,11);x->wr<4>(40,236);x->memory.write<1>(details+11*12+9,1);}
        if(pc==0x1018a4c8 || pc==0x1018a620) {x->memory.write<1>(rows+0x14,0);x->memory.write<1>(rows+0x15,1);}
        if(pc==0x1018a7dc)x=retirement_seed(40);
        if(pc==0x1018a9a8)x=retirement_seed(34);
        if(pc==0x1018ab64)x=retirement_seed(32,600);
        if(pc==0x1018ad18)x=retirement_seed(25,600);
        for(auto wide:{65536u,0xffff0000u,0x7fffffffu,0x80000000u}) {Options o;o.force_pc=pc;o.force_register=reg;o.force_value=wide;
            if(wide==65536)sampled_boundaries(*x,o);else compare(*x,o);}
        {Options o;o.force_pc=pc;o.force_register=reg;o.force_value=7;sampled_boundaries(*x,o);}
        {Options o;o.wrap_pc=pc==0x10189644?0x10189620:pc-0x28;o.random=0;sampled_boundaries(*x,o);}
        if(pc==0x10189490) {Options o;o.force_pc=pc;o.force_register=reg;o.force_value=65536;o.correction=10000000;sampled_boundaries(*x,o);}
    }
    for(auto [pc,reg]:std::array<std::pair<std::uint32_t,unsigned>,6>{{{0x10189450,19},{0x101897d0,13},{0x10189948,13},{0x10189ad4,25},{0x1018a1fc,20},{0x1018a494,13}}}) {
        auto x=seed();Options o;o.force_pc=pc;o.force_register=reg;o.force_value=0;o.force_cr=2;
        if(pc==0x10189450) {o.force_value=0xfffffffbu;o.force_cr=-1;}
        if(pc==0x101897d0 || pc==0x10189948) {x->memory.write<2>(staff+0x5c,40);x->memory.write<2>(details+4,12);x->memory.write<1>(profile+0x2d,20);}
        if(pc==0x1018a1fc) {x->wr<4>(16,11);x->wr<4>(40,236);x->memory.write<1>(details+11*12+9,1);}
        if(pc==0x1018a494) {x->memory.write<1>(rows+0x14,0);x->memory.write<1>(rows+0x15,1);}
        sampled_boundaries(*x,o);
    }
    for(auto age:{34u,127u}) {auto x=retirement_seed(age,0);x->memory.write<1>(profile+0x2d,127);sampled_boundaries(*x);}
    {auto x=retirement_seed(32,600);Options o;o.force_pc=0x1018ab30;o.force_register=13;o.force_value=0;o.force_cr=2;sampled_boundaries(*x,o);}
    {auto x=retirement_seed(25,600);x->memory.write<1>(profile+0x34,251);sampled_boundaries(*x);}
    {auto x=seed();x->wr<4>(20,1);x->memory.write<2>(details+4,0);sampled_boundaries(*x);}
    {auto x=seed();x->memory.write<2>(pool+2,4);sampled_boundaries(*x);}
    {auto x=retirement_seed(25,199);x->wr<4>(40,127);x->memory.write<1>(rows+0x14,0);x->memory.write<1>(rows+0x15,1);sampled_boundaries(*x);}
    for(auto slot:{-0x7a7c,-0x7a64,-0x7b28,-0x7b40}) {auto x=retirement_seed(40,600);x->memory.write<4>(staff+0x24,club);x->memory.write<4>(staff+0x20,club+0x500);
        x->memory.write<4>(x->memory.read<4>(toc+slot),club+0x1000);x->memory.write<4>(club+0x1000+0x988,club+0x2000);x->memory.write<4>(club+0x2000+0xdc,stats+0x1000);x->wr<4>(32,1);Options o;o.eligible=0;sampled_boundaries(*x,o);}
    {auto x=seed();Options o;o.wrap_pc=0x10189d00;o.random=0;sampled_boundaries(*x,o);}
    {auto x=retirement_seed(25,300);x->memory.write<1>(details+8,0);sampled_boundaries(*x);}
    {auto x=seed();x->memory.write<1>(rows+0x14,0);x->memory.write<1>(rows+0x15,1);x->memory.write<4>(objects+0x40c,objects);
        for(unsigned n=0;n<0x100;n+=2)x->memory.write<2>(pool+n,n==20?0:1);auto r=compare(*x);
        require(std::any_of(r.calls.begin(),r.calls.end(),[](auto e){return e.target==0x102de470;}),"Counter draw triggers the original retirement-risk callback");sampled_boundaries(*x);}
    for(auto age:{32u,34u,40u})for(bool service:{false,true}) {auto x=retirement_seed(age,600);x->memory.write<4>(objects+0x40c,service?objects:0);
        for(unsigned n=0;n<0x100;n+=2)x->memory.write<2>(pool+n,age==32?200:100);sampled_boundaries(*x);}
    for(auto entry:injury::entries) {auto x=seed(entry);constexpr auto relocated=toc+0x10000;x->memory.map(relocated-0x8000,0x9000);x->memory.copy_in(relocated-0x8000,x->memory.copy_out(toc-0x8000,0x9000));x->wr<4>(8,relocated);sampled_boundaries(*x);}
}

void percentage_checks() {
    for(auto [percent,expected]:std::array<std::pair<unsigned,int>,8>{{{0,0},{1,0},{50,2},{99,4},{100,5},{127,6},{128,-6},{255,0}}}) {
        auto x=seed();x->memory.write<1>(details+8,percent);auto r=compare(*x);require(signed_value<2>(r.state->memory.read<2>(injuries+0x32))==expected,"Independent signed injury percentage and truncation");
    }
    {auto x=seed();x->memory.write<2>(details+4,500);x->memory.write<1>(details+8,128);x->memory.write<1>(stats+10,100);auto r=compare(*x);require(r.state->memory.read<2>(injuries+0x32)==440 && r.state->memory.read<2>(injuries+0x36)==440,"Independent sixty-day duration gap clamp and final copy");}
    for(auto bits:std::array<std::uint64_t,9>{0,0x8000000000000000ULL,0x7ff0000000000000ULL,0xfff0000000000000ULL,0x7ff8123456789000ULL,0x7ff0123456789000ULL,1,0x7fefffffffffffffULL,std::uint64_t(float_bits<8>(3.0))})
        for(bool enabled:{false,true}) {auto x=seed();x->memory.write<8>(constants+0x70,bits);if(enabled){x->wr<1>(1304,1);x->wr<1>(1308,1);}sampled_boundaries(*x);}
}

void registration_checks() {
    auto basic=compare(*seed());require(basic.state->r<4>(12)==0 && basic.state->memory.read<4>(rows+0x10)==0,"Independent slot binding");
    require(basic.state->memory.read<2>(injuries+0x30)==5 && basic.state->memory.read<2>(injuries+0x32)==2,"Independent duration and percentage conversion");
    require(basic.state->memory.read<1>(stats+10)==5 && basic.state->memory.read<2>(injuries+0x28)==20,"Independent accumulated days and live date");
    boundaries(*seed());
    for(auto kind:{0u,1u,2u,4u,11u,76u,77u,78u,79u,80u,81u,82u,127u,128u,255u,256u,0xffffff0bu}) {
        auto x=seed();x->wr<4>(16,kind);boundaries(*x);
    }
    for(auto id:{0u,1u,2u,0x7fffffffu,0x80000000u,0xffffffffu}) {auto x=seed();x->memory.write<4>(staff,id);boundaries(*x);}
    for(auto pointer:{0u,staff,stack-336}) {auto x=seed();x->wr<4>(24,pointer);boundaries(*x);}
    {auto x=seed();x->memory.write<4>(staff+0x14,0);boundaries(*x);}
    for(auto flags:{0u,1u,128u,255u,0x100u})for(auto risk:{0u,1u,9u,10u,127u,128u,236u,255u}) {
        auto x=seed();x->memory.write<4>(stack+0x38,flags);x->wr<4>(40,risk);x->wr<4>(16,11);x->memory.write<1>(details+11*12+9,1);boundaries(*x);
    }
    for(auto previous:{0u,1u,0xffffffffu})for(auto kind:{0u,77u,78u,79u,80u,81u}) {
        auto x=seed();x->memory.write<4>(rows+0x10,previous);x->memory.write<2>(injuries+0x34,7);x->wr<4>(16,kind);boundaries(*x);
    }
    for(auto capacity:{0u,1u,2u,7u,8u,9u})for(bool absent:{false,true}) {
        auto x=seed();x->memory.write<4>(manager+8,capacity);if(absent)x->memory.write<4>(manager+4,0);
        for(unsigned n=0;n<capacity;++n)x->memory.write<1>(injuries+n*0x38,0);
        compare(*x);if(capacity<=1) {Options o;o.allocation_failure=true;boundaries(*x,o);}
    }
    {auto x=seed();x->memory.write<4>(manager+4,0);x->memory.write<4>(manager+8,0);boundaries(*x);}
    for(auto context:{0u,side+0x1000})for(unsigned side_index=0;side_index<3;++side_index) {
        auto x=seed();x->wr<4>(28,context);x->memory.write<4>(side+0x1000+0x1c,side);x->memory.write<4>(side+0x1000+0x20,side+0x400);
        x->memory.write<4>(side+0x98,side_index==0?staff:0);x->memory.write<4>(side+0x400+0x98,side_index==1?staff:0);boundaries(*x);
    }
    for(auto medical:{0u,1u,20u,127u,128u,255u})for(auto age:{0u,25u,31u,34u,37u,40u,128u,255u}) {
        auto x=seed();x->memory.write<4>(staff+0x24,club);x->memory.write<4>(club+0x190,club+0x400);x->memory.write<4>(club+0x400+0x1c,profile);
        x->memory.write<2>(staff+0x5c,age);x->memory.write<2>(details+4,12);x->memory.write<2>(details+6,20);Options o;o.medical=medical;compare(*x,o);
        if(medical==0 && age==25)boundaries(*x,o);
    }
    for(auto backdate:{0u,1u,128u,255u})for(auto history:{0u,1u,128u,255u}) {
        auto x=seed();x->wr<4>(20,backdate);x->wr<4>(32,history);x->memory.write<4>(staff+0x24,club);boundaries(*x);
    }
    for(auto risk:{0u,1u,5u,10u,127u,128u,236u,255u})for(auto previous:{0u,11u,255u}) {
        auto x=seed();x->wr<4>(16,11);x->wr<4>(40,risk);x->memory.write<1>(details+11*12+9,1);x->memory.write<1>(rows+0x14,previous);x->memory.write<1>(rows+0x15,1);boundaries(*x);
    }
    for(auto duration:{0u,1u,10u,11u,99u,100u,101u,300u,501u,32767u,32768u,65535u})for(auto age:{25u,32u,34u,37u,40u}) {
        auto x=seed();x->memory.write<2>(details+4,duration);x->memory.write<2>(staff+0x5c,age);x->memory.write<1>(details+8,100);x->memory.write<1>(stats+10,100);
        compare(*x);
    }
    for(auto cursor:{pool+0xfc,pool+0xfe,0xfffffffeu}) {
        auto x=seed();x->memory.write<4>(globals+8,cursor);boundaries(*x);Options o;o.reseed_mutate=true;boundaries(*x,o);
    }
    for(auto slot:{-0x64d4,-0x7b9c,-0x5438,-0x7ba8,-0x7bac,-0x7be4,-0x7a7c,-0x7b84,-0x7b88}) {auto x=seed();x->memory.write<4>(toc+slot,0);boundaries(*x);}
    {Options o;o.refresh_mutate=true;boundaries(*seed(),o);}
    for(auto flags:{0u,1u,2u,3u}) {Options o;o.notice_chain=flags&1;o.propagation_chain=flags&2;boundaries(*seed(),o);}
    {Options o;o.own_negative=true;compare(*seed(),o);o.own_negative=false;o.child_negative=true;auto x=seed();x->memory.write<4>(manager+4,0);x->memory.write<4>(manager+8,0);compare(*x,o);}
}
void notice_checks() {
    boundaries(*seed(injury::notice_entry));
    for(auto duration:{0u,100u,101u,32767u,32768u,65535u})for(auto fitness:{0u,8000u,8001u,32768u})for(auto recipients:{0u,1u,29u,30u,127u,128u,255u}) {
        auto x=seed(injury::notice_entry);x->memory.write<2>(injuries+0x30,duration);x->memory.write<2>(profile+12,fitness);x->memory.write<4>(staff+0x20,club+0x500);x->memory.write<4>(objects+0x400,objects);
        Options o;o.recipients=recipients;compare(*x,o);if(fitness==8000 && recipients==1)boundaries(*x,o);
    }
    for(auto context:{0u,255u})for(auto pointer:{0u,club})for(auto owner:{0u,objects+0x300})for(auto eligible:{0u,1u,128u}) {
        auto x=seed(injury::notice_entry);x->memory.write<1>(injuries+0x20,context);x->memory.write<4>(staff+0x24,pointer);x->memory.write<4>(staff+0x20,club+0x500);
        for(auto field:{0x28u,0x2cu,0x30u})x->memory.write<4>(staff+field,pointer);
        Options o;o.current_club=owner;o.notice_eligible=eligible;boundaries(*x,o);
    }
    for(auto queries:{std::vector<std::uint32_t>{objects+0x300,0},std::vector<std::uint32_t>{objects+0x300,objects+0x300,0}}) {
        auto x=seed(injury::notice_entry);x->memory.write<4>(staff+0x24,club);x->memory.write<4>(staff+0x20,club+0x500);Options o;o.club_results=queries;boundaries(*x,o);
    }
    {auto x=seed(injury::notice_entry);x->memory.write<4>(staff+0x24,club);x->memory.write<4>(objects+0x304,0);boundaries(*x);Options o;o.notice_mutate=true;boundaries(*x,o);}
    {Options o;o.own_negative=true;compare(*seed(injury::notice_entry),o);o.own_negative=false;o.child_negative=true;auto x=seed(injury::notice_entry);x->memory.write<4>(staff+0x24,club);compare(*x,o);}
}
std::unique_ptr<State> propagation_seed() {
    auto x=seed(injury::propagation_entry);x->memory.write<1>(injuries,11);x->memory.write<1>(details+11*12+9,1);x->memory.write<4>(staff+0x24,club);x->memory.write<2>(club+0x1ec,5000);x->memory.write<2>(pool+4,100);return x;
}
void propagation_checks() {
    boundaries(*seed(injury::propagation_entry));Options o;o.eligible=0;boundaries(*propagation_seed(),o);
    for(unsigned type=0;type<256;++type) {auto x=propagation_seed();o.related_type=type;auto r=compare(*x,o);
        const auto it=std::find_if(r.calls.begin(),r.calls.end(),[](auto e){return e.target==injury::registration_entry;});require(it!=r.calls.end(),"Related injury calls registration");
        const auto signed_type=type<128?int(type):int(type)-256;const auto destination=signed_type>=11 && signed_type<=76?original_injury_propagation_table[signed_type-11]:0x1018b998u;
        const auto expected=destination==0x1018b988?78u:destination==0x1018b990?79u:77u;require(read_be<4>(it->before.data()+16)==expected,"Independent related-injury type table");
    }
    o.related_type=11;
    for(auto amount:{0u,1u,19u,20u,95u,100u,127u,128u,255u})for(auto trait:{0u,10u,127u,128u,255u}) {
        auto x=propagation_seed();x->memory.write<1>(rows+0x15,amount);x->memory.write<1>(profile+0x2d,trait);boundaries(*x,o);
    }
    for(auto pointer:{0u,profile})for(auto team:{0u,club})for(auto quality:{0u,4250u,4251u,32768u}) {
        auto x=propagation_seed();x->memory.write<4>(staff+0x14,pointer);x->memory.write<4>(staff+0x24,team);x->memory.write<2>(club+0x1ec,quality);boundaries(*x,o);
    }
    for(auto salt:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu})for(auto word:{0u,1u,19u,20u,65535u}) {
        auto x=propagation_seed();x->memory.write<4>(globals,salt);x->memory.write<2>(pool+2,word);compare(*x,o);
    }
    for(auto cursor:{pool+0xfa,pool+0xfc,pool+0xfe,0xfffffffeu}) {auto x=propagation_seed();x->memory.write<4>(globals+8,cursor);x->memory.write<2>(pool+0xfe,100);Options m=o;m.random=0;boundaries(*x,m);}
    for(auto index:{0u,1u,65u}) {auto x=propagation_seed();o.related_type=11+index;x->memory.write<4>(table+4*index,0x12345678);boundaries(*x,o);}
    o.related_type=11;
    for(auto target:{0x1018b9c8u,0x1018b9d8u}) {auto x=propagation_seed();x->memory.write<4>(table,target);boundaries(*x,o);}
    {auto x=propagation_seed();o.registration_chain=true;o.notice_chain=true;boundaries(*x,o);o.registration_chain=false;o.notice_chain=false;}
    {Options n=o;n.own_negative=true;compare(*propagation_seed(),n);n.own_negative=false;n.child_negative=true;compare(*propagation_seed(),n);}
}

}
namespace cm::aot {void injury_record_step_seen(State& s,std::uint32_t pc) {if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try {
    const auto focused=argc==2?std::string_view(argv[1]):std::string_view{};
    if(focused=="--complete-coverage")structural_checks();
    if(focused.empty() || focused=="--registration")registration_checks();
    if(focused.empty() || focused=="--percentage")percentage_checks();
    if(focused.empty() || focused=="--structural")structural_checks();
    if(focused.empty() || focused=="--notice")notice_checks();
    if(focused.empty() || focused=="--propagation")propagation_checks();
    for(auto [start,end]:ranges)for(auto pc=start;pc<end;pc+=4)if(pc!=0x10189304 && pc!=0x10189324 && pc!=0x1018b7c8 && !sites.contains(pc))std::cerr<<"Uncovered injury "<<hex_address(pc)<<std::endl;
    if(focused.empty())require(sites.size()==2617,"Complete injury-record source coverage");
    std::cout<<"Injury comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interruptions<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
