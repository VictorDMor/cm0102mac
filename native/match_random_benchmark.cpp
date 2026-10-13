#include "match_random_adapter.hpp"
#include "match_debugger_factory.hpp"
#include "semantic_io_checks.hpp"
#include "code_fragment_adapter.hpp"
#include "match_turbo.hpp"
#include <chrono>
#include <iomanip>

namespace {
using namespace cm::semantic_io_checks;
using Clock=std::chrono::steady_clock;
struct Inputs final:CodeFragmentBoundaryObserver {
    unsigned clock_calls=0;
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State&,std::uint32_t,bool)override{}
    bool import_fixture(State& s,const char* name)override {
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),1000000000ULL+1000000ULL*++clock_calls);
        s.pc=std::uint32_t(s.r<4>(4128));return true;
    }
};
struct OwnedBacking {
    std::filesystem::path root;
    ~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(root,e)||e||std::filesystem::is_symlink(root,e)||e)return;
        for(const auto& p:std::filesystem::recursive_directory_iterator(root,e))if(e||p.is_symlink(e)||e)return;
        std::filesystem::remove_all(root,e);}
};
struct Run {std::unique_ptr<State> state;double seconds;unsigned clock_calls;};
Run execute(const State& initial,std::uint32_t match,bool original) {
    auto s=clone(initial);s->platform=initial.platform;
    // Bypass the diagnostic wrapper on the baseline, retaining the ordinary
    // generated body and its recursive calls. Otherwise an extra run_until
    // per original selection would inflate the apparent semantic speedup.
    if(original)match_random_diagnostics(*s).original_body=match_random::entry;
    Inputs inputs;s->code_fragment_observer=&inputs;
    const auto begin=Clock::now();
    for(unsigned tick=0;tick<600;++tick){
        s->wr<4>(12,match);s->wr<4>(16,1);s->wr<4>(4128,return_sentinel);s->pc=0x101d3f60;
        s->budget=s->executed+cm::aot::match_debugger::fresh_match_step_limit;run_until(*s,return_sentinel);
        require(s->r<4>(12)==1||s->r<4>(12)==4,"Match interval ended unexpectedly");
    }
    const auto seconds=std::chrono::duration<double>(Clock::now()-begin).count();s->code_fragment_observer=nullptr;
    const auto& d=match_random_diagnostics(*s);
    require(original?d.original_body==match_random::entry&&!d.calls[0]&&!d.original_calls[0]:d.calls[0]>0&&d.calls[0]==d.returns[0]&&!d.original_calls[0],"Measured sampler not reached");
    require(!d.faults[0]&&!d.budget_stops[0]&&!d.dispatch_escapes,"Measured sampler fault/escape");
    return {std::move(s),seconds,inputs.clock_calls};
}
}
int main(int argc,char**argv){try{
    require(argc==4&&!match_pc_profile_enabled(),"Expected original program, read-only game and private benchmark root; profiling disabled");
    set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(Clock::now().time_since_epoch().count());OwnedBacking cleanup{root};
    std::cout<<std::setprecision(9);
    for(auto home:{677u,2262u}){
        auto world=cm::aot::match_debugger::create_match({argv[1],argv[2],root/std::to_string(home)},{home,5677,12345,2001,194});
        for(unsigned pair=0;pair<10;++pair){
            auto first=execute(*world.state,world.match,pair%2==0),second=execute(*world.state,world.match,pair%2!=0);
            auto& original=pair%2==0?first:second;auto& modern=pair%2==0?second:first;
            Image(*original.state).compare(*modern.state,"Paired actual 600-tick match interval");
            require(original.state->executed==modern.state->executed&&original.clock_calls==modern.clock_calls,"Measured interval accounting or host inputs differ");
            const auto& d=match_random_diagnostics(*modern.state);
            std::cout<<"{\"home\":"<<home<<",\"away\":5677,\"seed\":12345,\"ticks\":600,\"pair\":"<<pair
                <<",\"warmup\":"<<(pair==0?"true":"false")<<",\"originalSeconds\":"<<original.seconds<<",\"modernSeconds\":"<<modern.seconds
                <<",\"modernCalls\":"<<d.calls[0]<<",\"instructions\":"<<modern.state->executed-world.state->executed<<",\"equivalent\":true}\n";
        }
    }
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}}
