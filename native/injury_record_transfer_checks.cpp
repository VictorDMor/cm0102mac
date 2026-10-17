#include "injury_record_adapter.hpp"
#include "actor_attributes_io_checks.hpp"
#include "match_replay.hpp"
#include <chrono>
#include "code_fragment_adapter.hpp"
#include "register_context_adapter.hpp"
#include "guest_context_transfer.hpp"
#include "mac_abi.hpp"
#include "semantic_io_checks.hpp"
#include "match_turbo.hpp"
#include <string_view>

namespace {
using namespace cm::semantic_io_checks;
namespace line = cm::aot::injury_record;
constexpr std::uint32_t caller = 0x7fff1000, parent_import = 280984548, parent_function = parent_import - 0x100;
constexpr std::uint32_t saved_context = fixture + 0x800, marker = fixture + 0xc00;
enum class Scenario { jump, restore_fault, handler_fault };
enum class Site { initial_save, frame_boundary, data_effect };
// Measure the existing context dependency family without double-counting nested
// invocations. Original mode also requires exact total instruction accounting.
struct ContextCharges final:RegisterContextBoundaryObserver {
    struct Frame {std::uint32_t entry;std::uint64_t before,children;};
    std::vector<Frame> frames;std::uint64_t total=0;
    void enter(const State& s,std::uint32_t entry,bool)override{frames.push_back({entry,s.executed,0});}
    void leave(State& s,std::uint32_t entry,bool,RegisterContextOutcome)override{
        require(!frames.empty()&&frames.back().entry==entry,"Balanced context accounting boundaries");
        const auto frame=frames.back();frames.pop_back();const auto elapsed=s.executed-frame.before;
        require(elapsed>=frame.children,"Nested context accounting exceeded its parent");total+=elapsed-frame.children;
        if(!frames.empty())frames.back().children+=elapsed;
    }
};
struct Model final : CodeFragmentBoundaryObserver {
    ActorAttributesIoPair* io=nullptr;bool modern_run=false,resumed=false;unsigned time_calls=0;
    int nested=-1;std::uint32_t ninth=0;unsigned index, stage = 0, parent_after = 0, callback_after = 0, callbacks = 0, handlers = 0;
    Scenario scenario;
    Site site;
    std::uint32_t parent_frame = 0;
    ContextCharges context;
    std::array<std::uint32_t, 8> arguments;
    struct Event {
        bool leave; std::string name; decltype(State::registers) registers; std::uint32_t pc;
        bool operator==(const Event&) const = default;
    };
    std::vector<Event> imports;
    Model(unsigned root, Scenario kind, const State& s, Site point) : index(root), scenario(kind), site(point),
        arguments{std::uint32_t(s.r<4>(12)), std::uint32_t(s.r<4>(16)), std::uint32_t(s.r<4>(20)),std::uint32_t(s.r<4>(24)),std::uint32_t(s.r<4>(28)),std::uint32_t(s.r<4>(32)),std::uint32_t(s.r<4>(36)),std::uint32_t(s.r<4>(40))} {ninth=std::uint32_t(s.memory.read<4>(s.r<4>(4)+0x38));}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override { imports.push_back({false, name, s.registers, s.pc}); }
    void import_leave(const State& s, const char* name) override { imports.push_back({true, name, s.registers, s.pc}); }
    bool import_fixture(State& s, const char* name) override {
        if(io && io->dispatch(s,name,modern_run))return true;
        if(std::string_view(name)=="CarbonLib::Microseconds") {s.memory.write<8>(s.r<4>(12),1000000000ULL+1000000ULL*++time_calls);s.pc=s.r<4>(4128);return true;}
        if (s.pc != parent_import) return false;
        require(std::string_view(name) == "CarbonLib::Gestalt", "Explicit surviving match transfer parent import");
        if (stage == 2) {
            ++handlers; require(s.r<4>(12) == 7 && s.r<4>(4) == parent_frame, "Original surviving injury destination");
            if (scenario == Scenario::handler_fault) throw std::runtime_error("Fixture match resumed-handler fault");
            s.wr<4>(4128, caller); s.pc = caller; return true;
        }
        require(!stage, "Parent import captures its context exactly once");
        std::uint64_t escapes = 0; s.pc = parent_function; MacAbi parent(s, escapes, parent_function);
        parent.capture_link(); parent.save_link(parent_function + 4); parent.frame(112, parent_function + 8);
        parent_frame = parent.r(1); s.wr<4>(12, saved_context);
        parent.call(0x10773eb0, parent_import);
        require(s.r<4>(12) == 0, "Original injury capture initial zero");
        for(unsigned n=0;n<arguments.size();++n)s.wr<4>((n+3)*4,arguments[n]);
        s.memory.write<4>(parent_frame+0x38,ninth);
        stage = 1; parent.call(line::entries[index], parent_function + 0x40); ++parent_after;
        parent.store(marker, 1, parent_function + 0x44); parent.pop(112, parent_function + 0x48);
        parent.restore_link(); parent.finish(); return true;
    }
    void interrupt(State& s, bool primary) {
        bool point=false;unsigned frame=0;
        if(resumed) {point=s.pc==0x1018b9d0;frame=128;}
        else if(nested>=0) {point=s.pc==(nested==1?0x1018aed4u:nested==2?0x1018b6e4u:0x10188cc4u);frame=index==0?336:128;}
        else if(index==0) {
            if(primary)point=s.pc==(site==Site::initial_save?0x10188cc4u:site==Site::frame_boundary?0x10188d0cu:0x1018a040u);
            else point=s.pc==(site==Site::initial_save?0x10188cccu:site==Site::frame_boundary?0x10188d9cu:0x1018a034u);
            frame=primary?(site==Site::data_effect?336:0):(site==Site::initial_save?0:336);
        } else if(index==1) {
            if(primary)point=s.pc==(site==Site::initial_save?0x1018aed4u:site==Site::frame_boundary?0x1018aefcu:0x1018b110u);
            else point=s.pc==(site==Site::initial_save?0x1018aed8u:site==Site::frame_boundary?0x1018afdcu:0x1018b120u);
            frame=primary?(site==Site::data_effect?448:0):(site==Site::initial_save?0:448);
        } else {
            if(primary)point=s.pc==(site==Site::initial_save?0x1018b6e4u:site==Site::frame_boundary?0x1018b71cu:0x1018b7b8u);
            else point=s.pc==(site==Site::initial_save?0x1018b6e8u:site==Site::frame_boundary?0x1018b734u:0x1018b8a4u);
            frame=primary?(site==Site::data_effect?128:0):(site==Site::initial_save?0:128);
        }
        if(stage!=1 || !point || s.r<4>(4)!=parent_frame-frame)return;
        ++callbacks;stage=2;s.wr<4>(12,scenario==Scenario::restore_fault?fixture+0xffe:saved_context);s.wr<4>(16,7);invoke_register_context(s,0x10773f30);
        ++callback_after;throw std::runtime_error("Injury restore returned to abandoned frame");
    }
};
struct Monitor final : MemoryObserver {
    State& state; Model& model; Effects effects; bool primary;
    Monitor(State& s, Model& m, bool primary_channel) : state(s), model(m), primary(primary_channel) { effects.state=&s; if(primary)s.memory.observer=this;else s.memory.date_observer=this; }
    ~Monitor() { state.memory.observer=state.memory.date_observer=nullptr; }
    void mapped(std::uint32_t a, std::uint64_t n) override { effects.mapped(a, n); }
    void cleared(std::uint32_t a, std::uint64_t n) override { effects.cleared(a, n); }
    void read(std::uint32_t a, const std::uint8_t* b, unsigned n) override { effects.read(a, b, n); model.interrupt(state,primary); }
    void written(std::uint32_t a, const std::uint8_t* b, std::size_t n) override { effects.written(a, b, n); model.interrupt(state,primary); }
};
struct OwnedBacking {
    std::filesystem::path directory;
    ~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(directory,e)||e||std::filesystem::is_symlink(directory,e)||e)return;
        for(const auto& p:std::filesystem::recursive_directory_iterator(directory,e))if(e||p.is_symlink(e)||e)return;
        std::filesystem::remove_all(directory,e);}
};
unsigned cases = 0, fault_cases = 0;
void compare(const State& seed, unsigned index, Scenario scenario, bool original_context, bool turbo, Site site, bool primary,bool resumed=false,int nested=-1) {
    struct ProbeClock final:CodeFragmentBoundaryObserver {
        unsigned ticks=0;void enter(const State&,std::uint32_t,bool)override{}void leave(State&,std::uint32_t,bool)override{}
        bool import_fixture(State& s,const char* name)override {if(std::string_view(name)!="CarbonLib::Microseconds")return false;s.memory.write<8>(s.r<4>(12),1000000000ULL+1000000ULL*++ticks);s.pc=s.r<4>(4128);return true;}
    } probe;
    ActorAttributesIoPair io(seed,index,line::entries[index],[&](State& state){injury_record_diagnostics(state).original=true;state.code_fragment_observer=&probe;});auto a = clone(seed), b = clone(seed);a->platform=io.original;b->platform=io.modern;
    for(auto* s:{a.get(),b.get()}){
        s->pc = parent_import; s->wr<4>(4128, caller); s->budget = s->executed + 1000000;
    }
    injury_record_diagnostics(*a).original = true; actor_attributes_diagnostics(*a).original=true; a->register_context_original = true;
    b->register_context_original = original_context;
    if(resumed)for(auto* state:{a.get(),b.get()}) {const auto table=state->memory.read<4>(state->r<4>(8)-0x545c);for(unsigned n=0;n<66;++n)state->memory.write<4>(table+4*n,0x1018b9c8);}
    Model old(index, scenario, *a, site), modern(index, scenario, *b, site);old.nested=modern.nested=nested;old.resumed=modern.resumed=resumed; Monitor x(*a, old,primary), y(*b, modern,primary);
    old.io=modern.io=&io;modern.modern_run=true;
    a->code_fragment_observer = &old; b->code_fragment_observer = &modern;
    a->register_context_observer=&old.context;b->register_context_observer=&modern.context;
    const auto execute = [](State& s) {
        try { run_until(s, caller); }
        catch (const GuestContextTransfer&) { return std::string("Escaped match transfer owner"); }
        catch (const std::exception& e) { return std::string(e.what()); }
        return std::string{};
    };
    set_match_turbo_enabled(false); const auto ae = execute(*a);a->platform.reset();io.between();
    set_match_turbo_enabled(turbo); const auto be = execute(*b); set_match_turbo_enabled(false);
    a->memory.observer=b->memory.observer=nullptr; a->memory.date_observer = b->memory.date_observer = nullptr; a->code_fragment_observer = b->code_fragment_observer = nullptr;
    a->register_context_observer=b->register_context_observer=nullptr;
    require(old.context.frames.empty()&&modern.context.frames.empty(),"No abandoned context accounting scope");
    const auto label = hex_address(line::entries[index]) + " nonlocal scenario=" + std::to_string(unsigned(scenario))+" site="+std::to_string(unsigned(site))+" primary="+std::to_string(primary);
    require(ae == be && !ae.empty() == (scenario != Scenario::jump), label + " outcomes original=" + ae + " modern=" + be);
    if (scenario == Scenario::restore_fault) require(ae.starts_with("Unmapped data access at "), label + " restore fault prefix");
    if (scenario == Scenario::handler_fault) require(ae == "Fixture match resumed-handler fault", label + " handler fault");
    io.compare();
    Image(*a).compare(*b, label); require(x.effects.bytes == y.effects.bytes && old.imports == modern.imports, label + " ordered effects and actual import boundaries");
    if (original_context) require(a->executed == b->executed, label + " accounting with original context dependency");
    require(a->executed - old.context.total == b->executed - modern.context.total, label + " accounting outside measured context dependency");
    require(!old.parent_after && !modern.parent_after && !old.callback_after && !modern.callback_after && b->memory.read<4>(marker) == 0xa7a7a7a7,
        label + " abandoned host continuations");
    const auto& d = injury_record_diagnostics(*b);
    require(d.calls[index] == 1u && !d.returns[index] && !d.original_calls[index] && !d.original_body && !d.dispatch_escapes &&
        d.transfers[index] == 1u*unsigned(scenario != Scenario::restore_fault) && d.faults[index] == 1u*unsigned(scenario == Scenario::restore_fault) &&
        !d.terminations[index] && !a->register_context_unknown_transfers && !b->register_context_unknown_transfers, label + " dedicated productive transfer evidence");
    if(nested>=0)require(d.calls[nested]==1 && !d.returns[nested] && d.transfers[nested]==unsigned(scenario!=Scenario::restore_fault) && d.faults[nested]==unsigned(scenario==Scenario::restore_fault),label+" active parent and nested child both abandoned");
    require(old.callbacks == 1 && modern.callbacks == 1 && old.handlers == modern.handlers && modern.handlers == unsigned(scenario != Scenario::restore_fault),
        label + " exact interrupted callback and resumed handler counts");
    ++cases; fault_cases += scenario != Scenario::jump;
}
}
int main(int argc,char** argv){try {
    require(argc==5 && !match_pc_profile_enabled(),"Expected original program/game/backing and captured injury directory without profiling");
    const auto root=std::filesystem::path(argv[3])/std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());OwnedBacking cleanup{root};
    for(unsigned index=0;index<3;++index) {
        State seed;load_program(seed,argv[1]);seed.platform=std::make_shared<NativePlatform>(argv[2],root/"inputs",AudioMode::offline);
        load_match_snapshot(seed,std::filesystem::path(argv[4])/(index==2?"propagation-body.snapshot":hex_address(line::entries[index])+".snapshot"));require(seed.pc==line::entries[index],"Captured injury transfer input");
        seed.memory.map(fixture,0x1000);seed.memory.copy_in(fixture,std::vector<std::uint8_t>(0x1000,0xa7));
        for(bool primary:{false,true})for(auto site:{Site::initial_save,Site::frame_boundary,Site::data_effect}) {
            for(auto scenario:{Scenario::jump,Scenario::restore_fault,Scenario::handler_fault})for(bool original_context:{false,true})compare(seed,index,scenario,original_context,false,site,primary);
            compare(seed,index,Scenario::jump,false,true,site,primary);
        }
        const auto nested=index==0?std::vector<int>{1,2}:index==2?std::vector<int>{0}:std::vector<int>{};
        for(auto child:nested)for(bool primary:{false,true}) {
            for(auto scenario:{Scenario::jump,Scenario::restore_fault,Scenario::handler_fault})for(bool original_context:{false,true})compare(seed,index,scenario,original_context,false,Site::data_effect,primary,false,child);
            compare(seed,index,Scenario::jump,false,true,Site::data_effect,primary,false,child);
        }
        if(index==2)for(bool primary:{false,true}) {
            for(auto scenario:{Scenario::jump,Scenario::restore_fault,Scenario::handler_fault})for(bool original_context:{false,true})compare(seed,index,scenario,original_context,false,Site::data_effect,primary,true);
            compare(seed,index,Scenario::jump,false,true,Site::data_effect,primary,true);
        }
    }
    require(cases==182 && fault_cases==104,"Complete injury nonlocal matrix");std::cout<<"PASS: injury nonlocal cases="<<cases<<" restoreHandlerFaults="<<fault_cases<<"; original imports/effects/state/accounting, nested scopes and changed-table continuations"<<std::endl;
}catch(const std::exception& e){std::cerr<<e.what()<<std::endl;return 1;}}
