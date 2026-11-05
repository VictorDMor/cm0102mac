#include "match_eng_tendency_kernel.hpp"
#include <iostream>
#include <memory>
#include <string>
#include <vector>
using namespace cm::aot;
struct Op { unsigned kind; double a,b,c; };
struct StateObserver final : MemoryObserver {
    State* state = nullptr;
    void mapped(std::uint32_t,std::uint64_t) override {}
    void written(std::uint32_t,const std::uint8_t*,std::size_t) override {
        if (state) guest::Cpu(*state).set_f(29, 789.);
    }
    void cleared(std::uint32_t,std::uint64_t) override {}
};
void observation_checks(State& state) {
    using namespace match_eng::tendency_detail;
    const auto require=[](bool ok) { if (!ok) throw std::runtime_error("tendency observer qualification"); };
    state.budget=UINT64_MAX;
    require(native_allowed(state));
    NativeDrawingObserver drawing;
    state.memory.observer=&drawing; require(native_allowed(state));
    StateObserver diagnostic;
    state.memory.observer=&diagnostic; require(!native_allowed(state));
    state.memory.observer=nullptr; state.memory.date_observer=&diagnostic; require(!native_allowed(state));
    state.memory.date_observer=nullptr;
    std::vector<std::pair<std::uint32_t,unsigned>> reads;
    state.memory.read_trace=&reads; require(!native_allowed(state));state.memory.read_trace=nullptr;
    state.budget=1; require(!native_allowed(state));state.budget=UINT64_MAX;
    dispatch_probe=+[](State&) {return false;};require(!native_allowed(state));dispatch_probe=nullptr;
    guest::Cpu c(state);
    ObservedKernel observed(c);
    observed.ratings.first=123.; require(c.f(29)==123.);
    c.set_f(29,456.); require(double(observed.ratings.first)==456.);
    require(!passive_season_replay_observer(&diagnostic));
    state.memory.map(0x50000000, 8);
    diagnostic.state=&state;state.memory.observer=&diagnostic;
    c.put32(0x50000000, 0);
    require(double(observed.ratings.first)==789.);
    state.memory.observer=nullptr;
}
int main() {
    auto left=std::make_unique<State>(),right=std::make_unique<State>();
    observation_checks(*left);
    guest::Cpu a(*left),b(*right);
    std::uint64_t rng=0xf5417e6ab;
    auto next=[&]() {rng^=rng<<13;rng^=rng>>7;rng^=rng<<17;return rng;};
    const double edges[]={0.,-0.,1.,-1.,0.5,-0.5,2147483647.,2147483648.,-2147483648.,-2147483649.,
        std::numeric_limits<double>::infinity(),-std::numeric_limits<double>::infinity(),
        std::bit_cast<double>(0x7ff8000000000001ULL),std::bit_cast<double>(0x7ff0000000000001ULL),1e-300,1e300};
    unsigned operations=0,exceptions=0;
    for(unsigned test=0;test<10000;++test) {
        left->registers.fill(0);
        for(unsigned i=1280;i<1312;++i)left->registers[i]=next()&1;
        right->registers=left->registers;
        std::vector<Op> ops;
        auto number=[&] {return (next()%3) ? double(std::int32_t(next()))/128 : edges[next()%std::size(edges)];};
        for(unsigned i=0,n=1+next()%32;i<n;++i)ops.push_back({unsigned(next()%12),number(),number(),number()});
        guest::Fpu old(a); match_eng::tendency_detail::Arithmetic candidate(b);
        auto run=[&](auto& fp) {
            std::vector<std::uint64_t> results;
            std::string error;
            for(const auto& op:ops)try {
                double value=0;
                switch(op.kind) {
                    case 0:value=fp.fadd(op.a,op.b);break;
                    case 1:value=fp.fsub(op.a,op.b);break;
                    case 2:value=fp.fmul(op.a,op.b);break;
                    case 3:value=fp.fdiv(op.a,op.b);break;
                    case 4:value=fp.fadds(op.a,op.b);break;
                    case 5:value=fp.fsubs(op.a,op.b);break;
                    case 6:value=fp.fmuls(op.a,op.b);break;
                    case 7:value=fp.frsp(op.a);break;
                    case 8:results.push_back(fp.fctiwz(1,op.a));continue;
                    case 10:value=fp.fdivs(op.a,op.b);break;
                    case 11:fp.fcmp(0,op.a,op.b);value=0;break;
                    case 9:value=fp.multiply_add(0x101df2dc,op.a,op.b,op.c);break;
                }
                results.push_back(std::bit_cast<std::uint64_t>(value));
            } catch(const std::exception& e) {error=e.what();break;}
            return std::pair{results,error};
        };
        auto expected=run(old),actual=run(candidate);
        candidate.publish();
        if(expected!=actual || left->registers!=right->registers) {
            std::cerr<<"Mismatch sequence "<<test<<" result "<<(expected!=actual)<<'\n';
            for(unsigned i=0;i<left->registers.size();++i)if(left->registers[i]!=right->registers[i])
                std::cerr<<i<<" expected="<<unsigned(left->registers[i])<<" got="<<unsigned(right->registers[i])<<'\n';
            return 1;
        }
        operations+=unsigned(expected.first.size());exceptions+=!expected.second.empty();
    }
    std::cout<<"10000 native arithmetic sequences match, "<<operations<<" completed operations, "<<exceptions<<" matched exceptions\n";
}
