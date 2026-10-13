#include "match_random.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot { bool match_random_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
constexpr auto salt=fixture, cursor=fixture+4, limit=fixture+8, words=fixture+0x100, constants=fixture+0x200;
struct Trace final:SemanticGuestObserver {
    std::vector<std::uint32_t> order;
    void before_step(State&,std::uint32_t pc) override {order.push_back(pc);}
};
thread_local Trace* original_trace=nullptr;
struct CallEvent {
    std::uint32_t target,continuation;decltype(State::registers) before,after;
    bool operator==(const CallEvent&)const=default;
};
struct Model final:SemanticDependencyDispatch {
    bool original=false,negative_child=false;
    unsigned interrupt=0;
    Trace* trace=nullptr;
    std::vector<CallEvent> calls;
    void reference(State& s,std::uint32_t until) {
        bool first=true;
        while(s.pc!=until) {
            if((!first && s.pc==match_random::entry) || !match_random_reference(s))call(s,s.pc,std::uint32_t(s.r<4>(4128)));
            first=false;
        }
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        const auto index=calls.size();calls.push_back({target,continuation,s.registers,{}});
        if(target==match_random::entry) {
            if(original)reference(s,continuation);else match_random::sample(s,this,trace);
            if(negative_child&&!original)s.wr<4>(12,s.r<4>(12)+100);
        }else{
            require(target==0x1077b770,"Unknown match-random dependency");
            for(unsigned reg=0;reg<=12;++reg)if(reg!=1&&reg!=2)s.wr<4>(reg*4,0xabc00000+reg*73);
            s.wr<4>(12,0x1234abcd);s.pc=continuation;
        }
        calls[index].after=s.registers;
        if(interrupt && calls.size()>=interrupt)throw std::runtime_error("Fixture sample dependency interrupted");
    }
};
std::unique_ptr<State> seed(std::uint32_t bound,std::uint32_t sample=9123,std::uint32_t added=513,bool wrap=false) {
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    for(unsigned i=1280;i<1312;++i)s->wr<1>(i,0);
    s->memory.map(fixture,0x400);s->memory.map(stack-0x800,0x900);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,bound);s->wr<4>(4128,return_sentinel);s->pc=match_random::entry;
    s->memory.write<4>(toc-0x7b90,salt);s->memory.write<4>(toc-0x7b84,cursor);
    s->memory.write<4>(toc-0x7b88,limit);s->memory.write<4>(toc-0x7b8c,words);
    s->memory.write<4>(toc-0x52c4,constants);s->memory.write<4>(salt,added);
    s->memory.write<4>(cursor,words);s->memory.write<4>(limit,words+(wrap?0:126));
    for(unsigned i=0;i<128;i+=2)s->memory.write<2>(words+i,sample+i/2);
    s->memory.write<8>(constants+0x60,float_bits<8>(4503601774854144.0));
    s->memory.write<8>(constants+0x70,float_bits<8>(65535.0));return s;
}
unsigned cases=0,normal=0,interruptions=0,negatives=0;std::set<std::uint32_t> sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;};
Result compare(const State& input,bool negative=false,bool child=false,unsigned interrupt=0) {
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();
    a->memory.date_observer=&x;b->memory.date_observer=&y;Trace old_path,new_path;
    Model old,next;old.original=true;old.trace=&old_path;next.trace=&new_path;
    old.interrupt=next.interrupt=interrupt;next.negative_child=child;
    const auto run=[](State& s,Model& m){try{if(m.original){original_trace=m.trace;m.reference(s,return_sentinel);}
        else match_random::sample(s,&m,m.trace);}catch(const std::exception&e){original_trace=nullptr;return std::string(e.what());}
        original_trace=nullptr;return std::string{};};
    const auto expected=run(*a,old),actual=run(*b,next);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Match-random case "+std::to_string(cases);
    require(expected==actual,label+" outcomes original="+expected+" modern="+actual);
    if(negative)b->wr<4>(12,b->r<4>(12)^1);
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!negative&&!child)throw;rejected=true;}
    require(rejected==(negative||child),label+" negative did not discriminate");
    if(!rejected){require(x.bytes==y.bytes,label+" ordered effects");require(a->executed==b->executed,label+" instruction count");
        if(old.calls!=next.calls){
            for(unsigned n=0;n<std::min(old.calls.size(),next.calls.size());++n){
                for(const auto after:{false,true}){
                    const auto& x=after?old.calls[n].after:old.calls[n].before;
                    const auto& y=after?next.calls[n].after:next.calls[n].before;
                    if(x!=y){const auto d=std::mismatch(x.begin(),x.end(),y.begin());
                        std::cerr<<"call "<<n<<" target="<<hex_address(old.calls[n].target)<<" after="<<after<<" register="<<(d.first-x.begin())<<" original="<<unsigned(*d.first)<<" modern="<<unsigned(*d.second)<<'\n';}
                }
            }
        }
        require(old.calls==next.calls,label+" dependency ABI");require(old_path.order==new_path.order,label+" source path");}
    ++cases;normal+=expected.empty()&&!rejected;interruptions+=!expected.empty();negatives+=rejected;
    sites.insert(old_path.order.begin(),old_path.order.end());return {std::move(a),x.log};
}
void boundaries(const State& input) {
    const auto baseline=compare(input);const auto count=baseline.state->executed-input.executed;
    for(std::uint64_t n=0;n<=count;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s);}
    std::set<std::uint32_t> bytes;
    for(const auto& a:baseline.accesses)for(unsigned n=0;n<a.size;++n)bytes.insert(a.address+n);
    for(const auto hole:bytes){auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;
        for(const auto byte:bytes)if(byte!=hole){s->memory.map(byte,1);s->memory.write<1>(byte,input.memory.read<1>(byte));}compare(*s);}
}
}
namespace cm::aot {void match_random_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(){try{
    for(const auto bound:{0u,1u,2u,10u,255u,65534u,65535u,65536u,65537u,1000000u,2147483647u,2147483648u,0xffff0000u,0xffff0001u,0xfffffffeu,0xffffffffu})
        for(const auto word:{0u,32767u,65534u,65535u})for(const auto added:{0u,65535u,0x7fffffffu,0x80000000u,0xffffffffu})
            for(bool wrap:{false,true})compare(*seed(bound,word,added,wrap));
    for(const auto bound:{0u,17u,1000000u,0xffff0000u})for(bool wrap:{false,true})boundaries(*seed(bound,1234,73,wrap));
    compare(*seed(17),true);compare(*seed(1000000),false,true);
    compare(*seed(17,1234,73,true),false,false,1);compare(*seed(1000000),false,false,1);
    for(const auto bound:{0u,17u,1000000u}){
        auto relocated=seed(bound);relocated->memory.map(fixture+0x10000,0x9000);
        relocated->memory.copy_in(fixture+0x10000,relocated->memory.copy_out(toc-0x8000,0x9000));
        relocated->wr<4>(8,fixture+0x18000);compare(*relocated);
        for(const auto alias:{cursor,limit,words,stack-0x70,stack-0x14,stack+8}){
            auto s=seed(bound);s->memory.write<4>(toc-0x7b90,alias);compare(*s);
        }
        auto s=seed(bound);s->memory.write<4>(toc-0x7b88,cursor);compare(*s);
        s=seed(bound);s->memory.write<4>(toc-0x52c4,stack-0xc0);compare(*s);
    }
    for(const auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x7ff0000000000000),
                         UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000000000123),UINT64_C(0x3ff0000000000000)}) {
        for(const auto offset:{0x60u,0x70u}){auto s=seed(1000000);s->memory.write<8>(constants+offset,bits);compare(*s);}
    }
    // Independent expected values for the ordinary signed-remainder path and
    // the zero-bound rule (no draw), beyond original/semantic state agreement.
    for(const auto bound:{0,1,17,-17,65535,-65535}){
        auto s=seed(std::uint32_t(bound),100,20);const auto result=compare(*s);
        require(std::int32_t(result.state->r<4>(12))==(bound? (bound>0?121%bound:-(121%bound)):0),"Independent signed sample expectation");
        require(result.state->memory.read<4>(cursor)==words+(bound?2:0),"Independent pool advance expectation");
    }
    require(sites.size()==110,"Complete sampler source-site coverage");
    std::cout<<"PASS: "<<cases<<" match-random comparisons; "<<normal<<" returns, "<<interruptions<<" interruptions, "<<negatives
        <<" own/recursive negatives; all "<<sites.size()<<" original sites; complete state, effects, dependency ABI and accounting\n";
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
