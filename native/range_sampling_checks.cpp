#include "range_sampling.hpp"
#include "semantic_io_checks.hpp"
#include <fstream>
#include <set>

namespace cm::aot { bool range_sampling_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace sampling = cm::aot::range_sampling;
constexpr auto salt=fixture, cursor=fixture+4, limit=fixture+8, words=fixture+0x100, constants=fixture+0x200;
constexpr std::array<std::uint32_t,2> ends{0x102270e4,0x10712a58};
constexpr std::array branch_sites{0x10226548u,0x10226568u,0x10226588u,0x10226598u,0x10226648u,0x10226668u,0x102266a8u,0x102266d8u,0x102267c8u,0x102267e8u,0x10226848u,0x10226898u,0x102269c8u,0x102269e8u,0x10226a68u,0x10226ad8u,0x10226cd4u,0x10226e20u,0x10226e94u,0x10226f80u,0x10226fd4u,0x10227060u,0x10227094u,0x102270c8u,0x10712508u,0x10712528u,0x10712548u,0x10712558u,0x107125f8u,0x10712614u,0x10712634u,0x10712644u,0x107126e4u,0x10712700u,0x10712720u,0x10712730u,0x107127d0u,0x107127ecu,0x1071280cu,0x1071281cu,0x107128e8u,0x10712918u,0x10712948u,0x10712978u,0x107129a8u,0x107129d8u,0x10712a08u,0x10712a3cu};
struct Mutation {std::uint32_t pc;unsigned reg;std::uint32_t value;};
struct Trace final:SemanticGuestObserver {
    std::vector<std::uint32_t> order;
    std::vector<Mutation> mutations;
    void before_step(State& s,std::uint32_t pc)override {
        if(s.executed>=s.budget)return;
        order.push_back(pc);
        for(const auto& m:mutations)if(pc==m.pc){
            if(m.reg==32)s.wr<1>(2304,(s.r<1>(2304)&0xf1)|m.value);
            else s.wr<4>(m.reg*4,m.value);
        }
    }
};
thread_local Trace* original_trace=nullptr;
void invoke(State& s,SemanticDependencyDispatch* dependencies,SemanticGuestObserver* trace){
    if(s.pc==sampling::match_entry)sampling::sample_match(s,dependencies,trace);
    else {require(s.pc==sampling::general_entry,"Unknown range sampler");sampling::sample_general(s,dependencies,trace);}
}
struct CallEvent {
    std::uint32_t target,continuation;decltype(State::registers) before,after;
    bool operator==(const CallEvent&)const=default;
};
struct Model final:SemanticDependencyDispatch {
    bool original=false,negative_child=false;unsigned interrupt=0;
    Trace* trace=nullptr;std::vector<CallEvent> calls;
    void reference(State& s,std::uint32_t until){
        bool first=true;
        while(s.pc!=until){
            if((!first&&std::find(sampling::entries.begin(),sampling::entries.end(),s.pc)!=sampling::entries.end())||!range_sampling_reference(s))call(s,s.pc,std::uint32_t(s.r<4>(4128)));
            first=false;
        }
    }
    void call(State& s,std::uint32_t target,std::uint32_t continuation)override {
        const auto index=calls.size();calls.push_back({target,continuation,s.registers,{}});
        if(std::find(sampling::entries.begin(),sampling::entries.end(),target)!=sampling::entries.end()){
            if(original)reference(s,continuation);else invoke(s,this,trace);
            if(negative_child&&!original){s.wr<4>(12,s.r<4>(12)+100);s.memory.write<4>(salt,s.memory.read<4>(salt)^1);}
        }else{
            require(target==0x1077b770,"Unknown range-sampling dependency");
            for(unsigned reg=0;reg<=12;++reg)if(reg!=1&&reg!=2)s.wr<4>(reg*4,0xabc00000+reg*73);
            s.wr<4>(12,0x1234abcd);s.pc=continuation;
        }
        calls[index].after=s.registers;
        if(interrupt&&calls.size()>=interrupt)throw std::runtime_error("Fixture range-sampling dependency interrupted");
    }
};
std::unique_ptr<State> seed(unsigned index,std::uint32_t bound,std::uint32_t sample=9123,std::uint32_t added=513,bool wrap=false){
    auto s=std::make_unique<State>();s->registers.fill(0xa5);s->unique.fill(0xb7);
    for(unsigned i=1280;i<1312;++i)s->wr<1>(i,0);
    s->memory.map(fixture,0x400);s->memory.map(stack-0x800,0x900);s->memory.map(toc-0x8000,0x9000);
    s->wr<4>(4,stack);s->wr<4>(8,toc);s->wr<4>(12,bound);s->wr<4>(4128,return_sentinel);s->pc=sampling::entries[index];
    s->memory.write<4>(toc-0x7b90,salt);s->memory.write<4>(toc-0x7b84,cursor);
    s->memory.write<4>(toc-0x7b88,limit);s->memory.write<4>(toc-0x7b8c,words);
    s->memory.write<4>(toc-0x51e8,constants);s->memory.write<4>(toc-0x3ff8,constants);s->memory.write<4>(salt,added);
    s->memory.write<4>(cursor,words);s->memory.write<4>(limit,words+(wrap?0:126));
    for(unsigned i=0;i<128;i+=2)s->memory.write<2>(words+i,sample+i/2);
    s->memory.write<8>(constants,float_bits<8>(65535.0));
    for(unsigned offset:{8u,0x80u})s->memory.write<8>(constants+offset,float_bits<8>(4503601774854144.0));
    return s;
}
unsigned cases=0,normal=0,interruptions=0,negatives=0,forced=0;
std::array<std::set<std::uint32_t>,2> sites,unforced_sites;
struct Result {std::unique_ptr<State> state;std::vector<Effects::Access> accesses;std::vector<std::uint32_t> order;};
Result compare(const State& input,const std::vector<Mutation>& mutations={},bool negative=false,bool child=false,unsigned interrupt=0){
    auto a=clone(input),b=clone(input);Effects x,y;x.state=a.get();y.state=b.get();
    a->memory.date_observer=&x;b->memory.date_observer=&y;Trace old_path,new_path;old_path.mutations=new_path.mutations=mutations;
    Model old,next;old.original=true;old.trace=&old_path;next.trace=&new_path;old.interrupt=next.interrupt=interrupt;next.negative_child=child;
    const auto run=[](State& s,Model& m){
        try{if(m.original){original_trace=m.trace;m.reference(s,return_sentinel);}else invoke(s,&m,m.trace);}
        catch(const std::exception& e){original_trace=nullptr;return std::string(e.what());}
        original_trace=nullptr;return std::string{};
    };
    const auto expected=run(*a,old),actual=run(*b,next);a->memory.date_observer=b->memory.date_observer=nullptr;
    const auto label="Range sampling case="+std::to_string(cases)+" root="+hex_address(input.pc);
    require(expected==actual,label+" original="+expected+" modern="+actual);
    require(expected.empty()||expected=="Native instruction budget exhausted"||expected=="Fixture range-sampling dependency interrupted"||
            expected.starts_with("Unmapped data access at ")||expected.starts_with("Signed division "),label+" unexpected outcome="+expected);
    if(negative)b->wr<4>(12,b->r<4>(12)^1);
    if(child)require(std::any_of(next.calls.begin(),next.calls.end(),[&](const auto& event){return event.target==input.pc;}),label+" recursive negative child not reached");
    bool rejected=false;try{Image(*a).compare(*b,label);}catch(const std::runtime_error&){if(!negative&&!child)throw;rejected=true;}
    require(rejected==(negative||child),label+" negative must discriminate");
    if(!rejected){
        require(x.bytes==y.bytes,label+" ordered effects");require(a->executed==b->executed,label+" instruction accounting");
        require(old.calls==next.calls,label+" dependency ABI");require(old_path.order==new_path.order,label+" source order");
    }
    ++cases;normal+=expected.empty()&&!rejected;interruptions+=!expected.empty();negatives+=rejected;forced+=!mutations.empty();
    if(expected.empty()&&!rejected)for(const auto pc:old_path.order)for(unsigned i=0;i<2;++i)if(sampling::entries[i]<=pc&&pc<ends[i]){
        sites[i].insert(pc);if(mutations.empty())unforced_sites[i].insert(pc);
    }
    return {std::move(a),x.log,old_path.order};
}
void boundaries(const State& input,const std::vector<Mutation>& mutations={}){
    const auto baseline=compare(input,mutations);const auto count=baseline.state->executed-input.executed;
    for(std::uint64_t n=0;n<=count;++n){auto s=clone(input);s->budget=s->executed+n;compare(*s,mutations);}
    std::set<std::uint32_t> bytes;
    for(const auto& access:baseline.accesses)for(unsigned n=0;n<access.size;++n)bytes.insert(access.address+n);
    for(const auto hole:bytes){
        auto s=std::make_unique<State>();s->registers=input.registers;s->unique=input.unique;s->pc=input.pc;
        for(const auto byte:bytes)if(byte!=hole){s->memory.map(byte,1);s->memory.write<1>(byte,input.memory.read<1>(byte));}
        compare(*s,mutations);
    }
}
std::vector<Mutation> deep(unsigned index){
    std::vector<Mutation> out;
    if(index==0)for(auto pc:{0x102266a8u,0x102267c8u,0x10226848u,0x102269c8u,0x10226a68u})out.push_back({pc,32,4});
    else for(auto pc:{0x10712634u,0x107126e4u,0x10712720u,0x107127d0u,0x1071280cu})out.push_back({pc,32,4});
    return out;
}
void original_branch_variants(unsigned index){
    // Compiler-inlined correction levels contain paths unreachable with the
    // ordinary signed32-bit derived bound. Apply identical explicit source
    // boundary mutations to the isolated original and candidate; these are
    // component evidence and never presented as natural gameplay coverage.
    std::vector<std::vector<Mutation>> queue{{},deep(index)};
    std::set<std::pair<std::uint32_t,unsigned>> explored;
    for(std::size_t n=0;n<queue.size();++n){
        require(queue.size()<1024,"Bounded original correction-branch exploration");
        const auto changes=queue[n];const auto result=compare(*seed(index,1000000),changes);
        for(auto pc:result.order){
            if(std::find(branch_sites.begin(),branch_sites.end(),pc)==branch_sites.end())continue;
            for(unsigned condition:{2u,4u,8u})if(explored.emplace(pc,condition).second){auto next=changes;next.push_back({pc,32,condition});queue.push_back(std::move(next));}
        }
    }
}
void correction_leaf_variants(unsigned index){
    // With signed32-bit wrap, a third derived bound is zero. A controlled
    // nonzero divisor is necessary to complete its otherwise faulting leaf;
    // branch forcing alone must not be counted as complete source coverage.
    for(unsigned level:{2u,3u})for(unsigned condition:{2u,4u,8u}){
        auto changes=deep(index);
        if(index==0){
            changes.push_back({level==2?0x10226848u:0x10226a68u,32,2});
            changes.push_back({level==2?0x10226f60u:0x10226de8u,level==2?7u:8u,7});
            changes.push_back({level==2?0x10226f80u:0x10226e20u,32,condition});
        }else{
            changes.push_back({level==2?0x10712720u:0x1071280cu,32,2});
            changes.push_back({level==2?0x10712954u:0x107128f4u,level==2?21u:20u,7});
            changes.push_back({level==2?0x10712978u:0x10712918u,32,condition});
        }
        compare(*seed(index,1000000),changes);
        if(level==3&&condition==4)boundaries(*seed(index,1000000),changes);
    }
}
}
namespace cm::aot {void range_sampling_step_seen(State& s,std::uint32_t pc){if(original_trace)original_trace->before_step(s,pc);}}
int main(int argc,char** argv){try{
    require(argc==2,"Expected complete source-coverage output path");
    for(unsigned index=0;index<2;++index){
        for(auto bound:{0u,1u,2u,17u,65535u,65536u,1000000u,2147483647u,2147483648u,0xffff0000u,0xffff0001u,0xfffffffeu,0xffffffffu})
            for(auto word:{0u,65535u})for(auto added:{0u,65535u,0x7fffffffu,0x80000000u,0xffffffffu})for(bool wrap:{false,true})compare(*seed(index,bound,word,added,wrap));
        for(auto bound:{0u,17u,1000000u})for(bool wrap:{false,true})boundaries(*seed(index,bound,1234,73,wrap));
        compare(*seed(index,17),{},true);compare(*seed(index,1000000),deep(index),false,true);
        compare(*seed(index,17,1234,73,true),{},false,false,1);
        compare(*seed(index,1000000),deep(index),false,false,1);
        for(auto bound:{0u,17u,1000000u}){
            auto s=seed(index,bound);s->memory.map(fixture+0x10000,0x9000);
            s->memory.copy_in(fixture+0x10000,s->memory.copy_out(toc-0x8000,0x9000));s->wr<4>(8,fixture+0x18000);compare(*s);
            for(auto alias:{cursor,limit,words,stack-0x80,stack-0x30,stack+8}){s=seed(index,bound);s->memory.write<4>(toc-0x7b90,alias);compare(*s);}
            s=seed(index,bound);s->memory.write<4>(toc-0x7b88,cursor);compare(*s);
            s=seed(index,bound);s->memory.write<4>(toc-0x7b8c,cursor);compare(*s);
            s=seed(index,bound);s->memory.write<4>(toc-(index?0x3ff8:0x51e8),stack-0x80);compare(*s);
        }
        for(auto bits:{UINT64_C(0),UINT64_C(0x8000000000000000),UINT64_C(0x7ff0000000000000),UINT64_C(0xfff0000000000000),UINT64_C(0x7ff8000000000123),UINT64_C(0x3ff0000000000000)})
            for(auto offset:{0u,index?8u:0x80u}){auto s=seed(index,1000000);s->memory.write<8>(constants+offset,bits);compare(*s);}
        original_branch_variants(index);correction_leaf_variants(index);boundaries(*seed(index,1000000),deep(index));
        for(int bound:{0,1,17,-17,65535,-65535}){
            auto s=seed(index,std::uint32_t(bound),100,20);const auto result=compare(*s);
            require(std::int32_t(result.state->r<4>(12))==(bound?(bound>0?121%bound:-(121%bound)):0),"Independent signed remainder expectation");
            require(result.state->memory.read<4>(cursor)==words+(bound?2:0),"Independent cursor expectation");
        }
    }
    std::ofstream coverage(argv[1]);for(unsigned i=0;i<2;++i){
        for(auto pc:sites[i])coverage<<i<<" "<<hex_address(pc)<<" "<<unforced_sites[i].contains(pc)<<'\n';
        require(sites[i].size()==(i?350:753),"Complete sampler source coverage root="+hex_address(sampling::entries[i])+" sites="+std::to_string(sites[i].size()));
        std::cout<<"Root "<<hex_address(sampling::entries[i])<<" complete_sites="<<sites[i].size()<<" unforced_complete_sites="<<unforced_sites[i].size()<<'\n';
    }
    std::cout<<"Range sampling comparisons="<<cases<<" returns="<<normal<<" interruptions="<<interruptions<<" negatives="<<negatives<<" source_mutation_cases="<<forced<<" completed_sites=1103/1103\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
