#include "contract_queries.hpp"
#include "semantic_io_checks.hpp"
#include <bit>
#include <set>

namespace cm::aot { bool contract_query_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace queries = cm::aot::contract_queries;
constexpr std::uint32_t manager=fixture+0x1000,staff=fixture+0x2000,statistics=fixture+0x4000;
constexpr std::uint32_t records=fixture+0x8000,club=fixture+0x10000,resolved=fixture+0x10100,globals=fixture+0x11000;
constexpr std::array<std::pair<std::uint32_t,std::uint32_t>,3> ranges{{{0x10082f20,0x10082f84},{0x10082f90,0x10083108},{0x10083110,0x10083178}}};
struct Control { std::uint32_t pc; unsigned reg=32; std::uint32_t value=0; int cr=-1; };
struct Options {
    std::vector<Control> controls;
    unsigned interrupt=0,read_mutation=0;
    std::uint32_t returned_club=0,secondary=records+100;
    unsigned output_flag=0;
    bool nested=true,resolver_mutation=false,own_negative=false,child_negative=false;
};
struct Trace final:SemanticGuestObserver {
    Options options; std::vector<std::uint32_t> sites;
    explicit Trace(Options o):options(std::move(o)){}
    void before_step(State& s,std::uint32_t pc)override {
        sites.push_back(pc);
        for(const auto& c:options.controls)if(c.pc==pc) {
            if(c.reg<32)s.wr<4>(c.reg*4,c.value);
            if(c.cr>=0)s.wr<1>(2304,(s.r<1>(2304)&0xf0)|unsigned(c.cr)|(s.r<1>(1024)&1));
        }
    }
};
thread_local Trace* original_trace=nullptr;
void modern_root(State& s,SemanticDependencyDispatch* d,SemanticGuestObserver* o) {
    using Function=void(*)(State&,SemanticDependencyDispatch*,SemanticGuestObserver*);
    constexpr Function methods[]{queries::find_primary,queries::find_for_club,queries::find_secondary};
    const auto selected=std::find(queries::entries.begin(),queries::entries.end(),s.pc);
    require(selected!=queries::entries.end(),"Known contract query root"); methods[selected-queries::entries.begin()](s,d,o);
}
struct Event { std::uint32_t target,continuation; decltype(State::registers) before,after; bool operator==(const Event&)const=default; };
struct Model final:SemanticDependencyDispatch {
    bool original; Options options; Trace& trace; std::vector<Event> calls;
    Model(bool old,Options o,Trace& t):original(old),options(std::move(o)),trace(t){}
    void reference(State& s,std::uint32_t stop=return_sentinel) {
        const auto entry=s.pc;
        const auto range=std::find_if(ranges.begin(),ranges.end(),[&](auto r){return r.first==entry;});
        require(range!=ranges.end(),"Known original contract query root");
        while(s.pc!=stop) {
            if(s.pc>=entry && s.pc<range->second)require(contract_query_reference(s),"Complete contract query reference");
            else call(s,s.pc,std::uint32_t(s.r<4>(4128)));
        }
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        require(target==queries::secondary_entry || target==0x100ce320,"Known contract query dependency");
        const auto serial=calls.size(); calls.push_back({target,continuation,s.registers,{}});
        try {
            if(target==queries::secondary_entry && options.nested) {
                if(original)reference(s,continuation); else modern_root(s,this,&trace);
            } else {
                const auto third=std::uint32_t(s.r<4>(20)),output=std::uint32_t(s.r<4>(16));
                if(target==0x100ce320) {
                    require(third==1,"Original club resolver flag"); s.memory.write<1>(output,options.output_flag);
                    if(options.resolver_mutation)s.memory.write<4>(records+4,s.memory.read<4>(club));
                }
                for(unsigned n=0;n<=12;++n)if(n!=1 && n!=2)s.wr<4>(n*4,0xd3000000+n*0x101);
                for(unsigned n=0;n<14;++n)s.wr<8>(0x4000+n*16,float_bits<8>(-30.0-n));
                s.wr<1>(2304,5);s.wr<1>(1027,1);
                s.wr<4>(12,target==queries::secondary_entry?options.secondary:options.returned_club);s.pc=continuation;
            }
            if(options.child_negative && !original)s.memory.write<1>(globals+0x80,1);
            calls[serial].after=s.registers;
            if(options.interrupt && calls.size()>=options.interrupt)throw std::runtime_error("Fixture contract query dependency interruption");
        } catch(...) { calls[serial].after=s.registers; throw; }
    }
};
std::unique_ptr<State> seed(unsigned index) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    s->memory.map(fixture,0x20000);s->memory.map(stack-0x1000,0x2000);s->memory.map(toc-0x8000,0x9000);
    s->pc=queries::entries[index];s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(4128,return_sentinel);s->budget=10000;
    s->wr<4>(12,manager);s->wr<4>(16,staff);s->wr<4>(20,club);
    s->memory.write<4>(toc-0x7be4,globals);s->memory.write<4>(globals,3);
    s->memory.write<4>(toc-0x7b9c,globals+0x10);s->memory.write<4>(globals+0x10,statistics);
    s->memory.write<4>(manager,records);s->memory.write<4>(staff,0);
    s->memory.write<4>(statistics,0);s->memory.write<4>(statistics+4,1);
    s->memory.write<4>(statistics+100,2);s->memory.write<4>(statistics+104,-1);
    for(unsigned n=0;n<3;++n)s->memory.write<4>(records+n*100+4,(n+1)*11);
    s->memory.write<4>(club,11);s->memory.write<4>(resolved,22);return s;
}
unsigned cases=0,completed=0,interrupted=0,negatives=0,structural=0;
std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<Event> calls;};
Result compare(const State& input,Options options={}) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();
    for(auto* effect:{&x,&y})effect->on_read=[&](State& s,Effects& effects,std::uint32_t address,unsigned) {
        if(effects.mutated || !options.read_mutation)return;
        if((options.read_mutation==1 && address==staff) || (options.read_mutation==2 && address==globals)) {
            effects.mutated=true;s.memory.write<4>(staff,1);
        }
    };
    a->memory.date_observer=&x;b->memory.date_observer=&y;
    Trace old_trace(options),new_trace(options);Model old(true,options,old_trace),modern(false,options,new_trace);
    const auto run=[&](State& s,Model& model,bool original) {
        try {if(original){original_trace=&old_trace;model.reference(s);}else modern_root(s,&model,&new_trace);original_trace=nullptr;return std::string{};}
        catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}
    };
    const auto expected=run(*a,old,true),actual=run(*b,modern,false);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Contract query entry="+hex_address(input.pc)+" case="+std::to_string(cases);
    require(expected.empty() || expected=="Native instruction budget exhausted" || expected=="Fixture contract query dependency interruption" || expected.starts_with("Unmapped data access at "),label+" unexpected fixture failure: "+expected);
    require(expected==actual,label+" original="+expected+" modern="+actual);
    if(options.own_negative)b->wr<4>(12,b->r<4>(12)^1);
    bool rejected=false;
    try {Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!options.own_negative && !options.child_negative)throw;rejected=true;}
    require(rejected==(options.own_negative || options.child_negative),label+" discriminating negative");
    if(!rejected) {
        require(x.bytes==y.bytes,label+" ordered effects");require(old.calls==modern.calls,label+" dependency ABI");
        require(a->executed==b->executed,label+" accounting");require(old_trace.sites==new_trace.sites,label+" source order");
    }
    if(expected.empty())for(const auto& c:options.controls)require(std::find(old_trace.sites.begin(),old_trace.sites.end(),c.pc)!=old_trace.sites.end(),label+" structural control reached");
    ++cases;completed+=expected.empty()&&!rejected;interrupted+=!expected.empty();negatives+=rejected;structural+=!options.controls.empty();
    if(expected.empty())sites.insert(old_trace.sites.begin(),old_trace.sites.end());return{std::move(a),x.log,old.calls};
}
void boundaries(const State& input,Options options={}) {
    const auto baseline=compare(input,options);require(baseline.state->pc==return_sentinel,"Completed contract query boundary fixture");
    const auto steps=baseline.state->executed-input.executed;
    for(auto cut:std::set<std::uint64_t>{0,1,steps/2,steps-1}){auto s=clone(input);s->budget=s->executed+cut;compare(*s,options);}
    std::set<std::uint32_t> holes;
    for(const auto& access:baseline.accesses)if(access.size){holes.insert(access.address);holes.insert(access.address+std::uint32_t(access.size)-1);}
    for(auto hole:holes) {
        auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;s->executed=input.executed;s->budget=input.budget;
        for(auto [start,size]:input.memory.mapped_ranges()) {
            if(hole<start || std::uint64_t(hole)>=std::uint64_t(start)+size){s->memory.map(start,size);s->memory.copy_in(start,input.memory.copy_out(start,size));continue;}
            for(auto [address,count]:{std::pair{start,std::uint64_t(hole-start)},std::pair{hole+1,std::uint64_t(start)+size-hole-1}})
                if(count){s->memory.map(address,count);s->memory.copy_in(address,input.memory.copy_out(address,count));}
        }
        compare(*s,options);
    }
    for(unsigned n=1;n<=baseline.calls.size();++n){auto o=options;o.interrupt=n;compare(input,o);}
}
void leaf_cases() {
    for(unsigned index:{0u,2u}) {
        boundaries(*seed(index));
        for(auto pointer:{0u,staff})for(auto id:{0u,1u,2u,3u,0x40000000u,0x7fffffffu,0x80000000u,0xffffffffu})
            for(auto count:{0u,1u,3u,0x7fffffffu,0x80000000u,0xffffffffu})
                for(auto selected:{0u,1u,2u,0x40000000u,0x7fffffffu,0x80000000u,0xffffffffu}) {
                    auto s=seed(index);s->wr<4>(16,pointer);s->memory.write<4>(staff,id);s->memory.write<4>(globals,count);
                    s->memory.write<4>(statistics+id*100u+(index==2?4:0),selected);
                    const auto result=compare(*s);const auto expected=pointer && std::bit_cast<std::int32_t>(id)<std::bit_cast<std::int32_t>(count) && std::bit_cast<std::int32_t>(selected)>=0?records+selected*100u:0;
                    require(result.state->r<4>(12)==expected,"Independent signed contract index selection");
                }
        for(auto selected:{0xffffffffu,0x80000000u,0x7fffffffu}){auto s=seed(index);s->memory.write<4>(statistics+(index==2?4:0),selected);boundaries(*s);}
        for(auto alias:{staff,statistics,records}){auto s=seed(index);s->wr<4>(12,alias);boundaries(*s);}
        for(unsigned mutation:{1u,2u}){Options o;o.read_mutation=mutation;boundaries(*seed(index),o);}
        Options o;o.controls={{index==0?0x10082f30u:0x10083120u,32,0,2}};boundaries(*seed(index),o);
    }
}
void club_cases() {
    for(auto id:{0xffffffffu,0x80000000u,0x40000000u,0x7fffffffu})
        for(auto count:{0u,3u,0x7fffffffu,0x80000000u})
            for(auto selected:{0u,1u,0x7fffffffu,0x80000000u,0xffffffffu}) {
                auto s=seed(1);s->memory.write<4>(staff,id);s->memory.write<4>(globals,count);
                s->memory.write<4>(statistics+id*100u,selected);s->memory.write<4>(statistics+id*100u+4,-1);
                const auto expected=std::bit_cast<std::int32_t>(id)<std::bit_cast<std::int32_t>(count) && std::bit_cast<std::int32_t>(selected)>=0?records+selected*100u:0;
                s->memory.write<4>(club,expected?s->memory.read<4>(expected+4):11);
                const auto result=compare(*s);require(result.state->r<4>(12)==expected,"Independent signed inline contract lookup");
            }
    for(auto primary:{0u,0xffffffffu})for(auto secondary:{1u,0xffffffffu})for(auto requested:{11u,22u,33u})
        for(auto returned:{0u,resolved})for(auto flag:{0u,1u,127u,128u,255u,256u}) {
            auto s=seed(1);s->memory.write<4>(statistics,primary);s->memory.write<4>(statistics+4,secondary);s->memory.write<4>(club,requested);
            Options o;o.returned_club=returned;o.output_flag=flag;const auto result=compare(*s,o);
            const auto chosen=returned && !(flag&255)?22u:requested;
            const auto expected=primary==0xffffffffu?0u:requested==11?records:secondary!=0xffffffffu && requested==22?records+100:chosen==11?records:secondary!=0xffffffffu && chosen==22?records+100:0u;
            require(result.state->r<4>(12)==expected,"Independent primary/secondary/resolved club priority");
        }
    for(auto requested:{11u,22u,33u}){auto s=seed(1);s->memory.write<4>(club,requested);boundaries(*s);}
    for(auto flag:{0u,1u,128u,255u}){auto s=seed(1);s->memory.write<4>(club,33);Options o;o.returned_club=resolved;o.output_flag=flag;boundaries(*s,o);}
    for(auto pointer:{0u,staff})for(auto requested:{0u,club}){auto s=seed(1);s->wr<4>(16,pointer);s->wr<4>(20,requested);boundaries(*s);}
    {auto s=seed(1);s->memory.write<4>(staff,3);boundaries(*s);}
    {auto s=seed(1);s->memory.write<4>(club,33);s->memory.write<4>(statistics+4,-1);boundaries(*s);}
    {auto s=seed(1);s->memory.write<4>(club,33);Options o;o.resolver_mutation=true;boundaries(*s,o);}
    for(unsigned mutation:{1u,2u}){auto s=seed(1);s->memory.write<4>(club,33);Options o;o.read_mutation=mutation;boundaries(*s,o);}
    for(auto alias:{records+4,stack-24}){auto s=seed(1);s->wr<4>(20,alias);s->memory.write<4>(alias,33);Options o;o.output_flag=128;boundaries(*s,o);}
    for(auto secondary:{0u,records+100}){auto s=seed(1);s->memory.write<4>(club,33);Options o;o.nested=false;o.secondary=secondary;o.returned_club=resolved;boundaries(*s,o);}
    for(auto c:{Control{0x10082fc8,4,0,-1},Control{0x10082ff8,32,0,2},Control{0x10083008,32,0,4}}){Options o;o.controls={c};boundaries(*seed(1),o);}
}
// Without an observer or a budget the builder counts in batches; it must end
// in the same state, with the same count, as when it steps one by one.
void batched_cases() {
    for(unsigned index=0;index<3;++index)for(auto requested:{11u,22u,33u})for(auto flag:{0u,1u}) {
        auto input=seed(index);input->memory.write<4>(club,requested);
        Options o;o.returned_club=resolved;o.output_flag=flag;
        auto stepped=clone(*input),batched=clone(*input);batched->budget=UINT64_MAX;
        for(auto* s:{stepped.get(),batched.get()}){Trace trace(o);Model model(false,o,trace);modern_root(*s,&model,nullptr);}
        require(stepped->pc==return_sentinel,"Batched builder fixture completes");
        Image(*stepped).compare(*batched,"Batched builder counting");
        require(stepped->executed==batched->executed && batched->executed>input->executed,"Batched builder count");
    }
}
}
namespace cm::aot {void contract_query_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try {
    leaf_cases();club_cases();batched_cases();
    for(unsigned index=0;index<3;++index) {
        auto s=seed(index);Options o;o.own_negative=true;compare(*s,o);
        constexpr auto moved=toc+0x10000;s->memory.map(moved-0x8000,0x9000);s->memory.copy_in(moved-0x8000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,moved);boundaries(*s);
    }
    {auto s=seed(1);s->memory.write<4>(club,33);Options o;o.child_negative=true;compare(*s,o);}
    for(auto [start,end]:ranges)for(auto pc=start;pc<end;pc+=4)if(!sites.contains(pc))std::cerr<<"Uncovered contract query "<<hex_address(pc)<<std::endl;
    require(sites.size()==145,"Complete original contract query source coverage");
    std::cout<<"Contract query comparisons="<<cases<<" completions="<<completed<<" interruptions="<<interrupted<<" negatives="<<negatives<<" structural="<<structural<<" sites="<<sites.size()<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
