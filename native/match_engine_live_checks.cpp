#include "match_engine_adapter.hpp"
#include "match_eng_adapter.hpp"
#include "match_debugger_diagnostics.hpp"
#include "player_movement_adapter.hpp"
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
        auto modern=clone(*original);std::uint64_t total_original=0,total_modern=0;
        // MATCH_ENG.CPP's hook now sits on the page of 0x101d3f60: the original route is the
        // whole State running the generated bodies, MATCH_PL's movement included.
        match_eng_original_state=original.get();player_movement_diagnostics(*original).original=true;
        require(!match_eng_declining,"Live check runs without CM_MATCH_ENG_ORIGINAL");
        for(unsigned tick=0;tick<24;++tick) {
            for(auto* s:{original.get(),modern.get()}) {
                s->budget=s->executed+1000000;s->wr<4>(12,owner);s->wr<4>(16,1);s->wr<4>(4128,return_sentinel);s->pc=engine::entries[2];
            }
            const auto before_original=original->executed,before_modern=modern->executed;
            run_until(*original,return_sentinel);run_until(*modern,return_sentinel);
            const auto name="actual original/C++23 MATCH_ENG engine step "+std::to_string(tick);
            require(modern->pc==return_sentinel,name+" returned normally");
            // Instruction counts, r0/r11/r12, f0/f2-f13, CTR, XER and Ghidra's temporaries are diagnostics;
            // everything else (observable registers, CR, FPSCR, the whole memory image) must agree.
            {
                const auto saved=std::pair{original->registers,modern->registers};const auto unique=std::pair{original->unique,modern->unique};
                clear_diagnostic_registers(*original);clear_diagnostic_registers(*modern);original->unique.fill(0);modern->unique.fill(0);
                Image(*original).compare(*modern,name);
                original->registers=saved.first;modern->registers=saved.second;original->unique=unique.first;modern->unique=unique.second;
            }
            std::cout<<"tick="<<tick<<" instructions original="<<original->executed-before_original<<" modern="<<modern->executed-before_modern<<" status="<<modern->r<4>(12)
                <<" clock_units="<<signed_value<2>(modern->memory.read<2>(owner+0x7948))
                <<" score="<<modern->memory.read<1>(owner+0xe03d)<<':'<<modern->memory.read<1>(owner+0xe073)
                <<" actor_queue="<<modern->memory.read<1>(owner+0x792e)<<std::endl;
            require(modern->r<4>(12)==original->r<4>(12),name+" result");
            total_original+=original->executed-before_original;total_modern+=modern->executed-before_modern;
        }
        match_eng_original_state=nullptr;
        {   // The comparison fails when it should: one score byte and one observable register each.
            for(const bool in_memory:{true,false}) {
                auto broken=clone(*modern);
                if(in_memory)broken->memory.write<1>(owner+0xe03d,std::uint8_t(broken->memory.read<1>(owner+0xe03d)^1));else broken->wr<4>(12,broken->r<4>(12)^1);
                clear_diagnostic_registers(*broken);auto reference=clone(*modern);clear_diagnostic_registers(*reference);broken->unique=reference->unique;
                bool rejected=false;try{Image(*reference).compare(*broken,"negative");}catch(const std::runtime_error&){rejected=true;}
                require(rejected,"The live comparison must reject a changed score byte or result register");
            }
        }
        require(total_modern<total_original,"The C++ spends fewer instructions than the original (it ran, rather than the original body)");
        std::cout<<"PASS: 24 actual live-fixture steps of 0x101d3f60 agree in observable registers, CR, FPSCR and the whole memory image\n";
    }catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
