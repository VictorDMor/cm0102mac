#include "ball_action_adapter.hpp"
#include "code_fragment_adapter.hpp"
#include "match_debugger_factory.hpp"
#include "semantic_io_checks.hpp"
#include "match_turbo.hpp"
#include <chrono>
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
namespace debugger=cm::aot::match_debugger;
struct OwnedBacking {
    std::filesystem::path root;
    ~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(root,e)||e||std::filesystem::is_symlink(root,e)||e)return;
        for(const auto& p:std::filesystem::recursive_directory_iterator(root,e))if(e||p.is_symlink(e)||e)return;
        std::filesystem::remove_all(root,e);}
};
struct Clock final:CodeFragmentBoundaryObserver {
    unsigned count=0;
    void enter(const State&,std::uint32_t,bool)override{}
    void leave(State&,std::uint32_t,bool)override{}
    bool import_fixture(State& s,const char* name)override {
        if(std::string_view(name)!="CarbonLib::Microseconds")return false;
        s.memory.write<8>(s.r<4>(12),1000000000ULL+1000000ULL*++count);s.pc=s.r<4>(4128);return true;
    }
};
std::uint32_t advance(State& s,std::uint32_t match) {
    s.wr<4>(12,match);s.wr<4>(16,1);s.wr<4>(4128,return_sentinel);s.pc=0x101d3f60;s.budget=s.executed+debugger::fresh_match_step_limit;
    run_until(s,return_sentinel);return std::uint32_t(s.r<4>(12));
}
}
int main(int argc,char** argv){try {
    require(argc==4&&!match_pc_profile_enabled(),"Original program/game/private root with profiling disabled");set_match_turbo_enabled(false);
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());OwnedBacking cleanup{root};
    auto game=debugger::create_match({argv[1],argv[2],root},{2262,5677,12345,2001,194});auto& modern=*game.state;auto original=clone(modern);original->platform=modern.platform;
    ball_action_diagnostics(*original).original=true;Clock old_clock,new_clock;original->code_fragment_observer=&old_clock;modern.code_fragment_observer=&new_clock;
    unsigned ticks=0;
    for(;;) {
        require(++ticks<=1400,"Paired original ball-action match exceeded the full-time bound");
        const auto a=advance(*original,game.match),b=advance(modern,game.match);
        require(a==b&&(b==1||b==2||b==4),"Paired original/C++23 match advancement result differs");
        require(original->executed==modern.executed&&old_clock.count==new_clock.count,"Paired full-match instruction/clock consumption differs");
        if(ticks%50==0||b==2)Image(*original).compare(modern,"Paired full-match ball-action state tick="+std::to_string(ticks));
        if(b==2)break;
    }
    original->code_fragment_observer=modern.code_fragment_observer=nullptr;
    const auto& d=ball_action_diagnostics(modern);const auto& old=ball_action_diagnostics(*original);
    require(d.calls[0]>0&&d.calls[0]==d.returns[0]&&!d.original_calls[0]&&!d.profile_original_calls[0]&&!d.faults[0]&&!d.budget_stops[0]&&!d.transfers[0]&&!d.terminations[0]&&!d.dispatch_escapes&&!d.original_body,"Full match did not retain productive C++ ball-action ownership");
    require(old.original_calls[0]==d.calls[0]&&!old.calls[0]&&!old.dispatch_escapes&&!old.original_body,"Full match did not retain the independent original ball-action control");
    std::cout<<"PASS: paired complete actual-club match; ticks="<<ticks<<" ball-action calls="<<d.calls[0]<<" returns="<<d.returns[0]<<" original controls="<<old.original_calls[0]<<" clocks="<<new_clock.count<<"; full final state, intermediate states and every-tick instruction agreement; zero root faults/escapes\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
