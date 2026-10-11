#include "match_engine_adapter.hpp"
#include "match_replay.hpp"
#include "semantic_io_checks.hpp"
using namespace cm::semantic_io_checks;
namespace engine = cm::aot::match_engine;
int main(int argc,char** argv) {
    try {
        require(argc==2,"Expected one captured original engine fixture");
        auto original=std::make_unique<State>();load_match_snapshot(*original,argv[1]);
        const auto owner=std::uint32_t(original->r<4>(12));
        require(original->pc==engine::entries[2]&&original->r<4>(16)==1&&original->r<4>(4128)==0x101ccf1c,"Actual initialized original shared-engine entry");
        auto modern=clone(*original);
        match_engine_diagnostics(*original).original=true;
        for(unsigned tick=0;tick<24;++tick) {
            for(auto* s:{original.get(),modern.get()}) {
                s->budget=s->executed+1000000;s->wr<4>(12,owner);s->wr<4>(16,1);s->wr<4>(4128,return_sentinel);s->pc=engine::entries[2];
            }
            const auto before=original->executed;Effects x,y;x.state=original.get();y.state=modern.get();
            original->memory.date_observer=&x;modern->memory.date_observer=&y;
            run_until(*original,return_sentinel);run_until(*modern,return_sentinel);
            original->memory.date_observer=modern->memory.date_observer=nullptr;
            const auto name="actual original/shared C++23 engine step "+std::to_string(tick);
            Image(*original).compare(*modern,name);require(x.bytes==y.bytes,name+" ordered memory effects");
            require(original->executed==modern->executed,name+" instruction accounting old="+std::to_string(original->executed)+" modern="+std::to_string(modern->executed));
            require(modern->pc==return_sentinel,name+" returned normally");
            std::cout<<"tick="<<tick<<" instructions="<<modern->executed-before<<" status="<<modern->r<4>(12)
                <<" clock_units="<<signed_value<2>(modern->memory.read<2>(owner+0x7948))
                <<" score="<<modern->memory.read<1>(owner+0xe03d)<<':'<<modern->memory.read<1>(owner+0xe073)
                <<" actor_queue="<<modern->memory.read<1>(owner+0x792e)<<std::endl;
        }
        const auto& d=match_engine_diagnostics(*modern);
        require(d.calls[2]==24&&d.returns[2]==24&&!d.original_calls[2]&&!d.faults[2]&&!d.budget_stops[2]&&!d.dispatch_escapes,
            "Dedicated productive shared-engine hook coverage");
        std::cout<<"PASS: 24 actual live-fixture steps agree in ABI, complete guest state, ordered effects and accounting\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
