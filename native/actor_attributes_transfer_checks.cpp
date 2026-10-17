#include "actor_attributes_adapter.hpp"
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
namespace line = cm::aot::actor_attributes;
constexpr std::uint32_t caller = 0x7fff1000, parent_import = 280984548, parent_function = parent_import - 0x100;
constexpr std::uint32_t saved_context = fixture + 0x800, marker = fixture + 0xc00;
constexpr std::array<std::uint32_t,3> interrupted_sites{0x10235ed0,0x10236200,0x10237388};
enum class Scenario { jump, restore_fault, handler_fault };
enum class Site { before_draws, after_maximum, final_output };
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
    unsigned index, stage = 0, parent_after = 0, callback_after = 0, callbacks = 0, handlers = 0;
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
        arguments{std::uint32_t(s.r<4>(12)), std::uint32_t(s.r<4>(16)), std::uint32_t(s.r<4>(20)),std::uint32_t(s.r<4>(24)),std::uint32_t(s.r<4>(28)),std::uint32_t(s.r<4>(32)),std::uint32_t(s.r<4>(36)),std::uint32_t(s.r<4>(40))} {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override { imports.push_back({false, name, s.registers, s.pc}); }
    void import_leave(const State& s, const char* name) override { imports.push_back({true, name, s.registers, s.pc}); }
    bool import_fixture(State& s, const char* name) override {
        if (s.pc != parent_import) return false;
        require(std::string_view(name) == "CarbonLib::Gestalt", "Explicit surviving match transfer parent import");
        if (stage == 2) {
            ++handlers; require(s.r<4>(12) == 7 && s.r<4>(4) == parent_frame, "Original surviving scalar destination");
            if (scenario == Scenario::handler_fault) throw std::runtime_error("Fixture match resumed-handler fault");
            s.wr<4>(4128, caller); s.pc = caller; return true;
        }
        require(!stage, "Parent import captures its context exactly once");
        std::uint64_t escapes = 0; s.pc = parent_function; MacAbi parent(s, escapes, parent_function);
        parent.capture_link(); parent.save_link(parent_function + 4); parent.frame(112, parent_function + 8);
        parent_frame = parent.r(1); s.wr<4>(12, saved_context);
        parent.call(0x10773eb0, parent_import);
        require(s.r<4>(12) == 0, "Original scalar capture initial zero");
        for(unsigned n=0;n<arguments.size();++n)s.wr<4>((n+3)*4,arguments[n]);
        stage = 1; parent.call(line::entries[index], parent_function + 0x40); ++parent_after;
        parent.store(marker, 1, parent_function + 0x44); parent.pop(112, parent_function + 0x48);
        parent.restore_link(); parent.finish(); return true;
    }
    void interrupt(State& s, bool primary) {
        // The primary observer reports writes only. Use the conversion scratch
        // store before the first draw; the secondary channel also covers reads.
        constexpr std::array<std::uint32_t,3> recovery_sites{0x10235de0,0x10235e3c,0x10235e7c};
        constexpr std::array<std::uint32_t,3> periodic_sites{0x10235ea4,0x10239400,0x10239574};
        constexpr std::array<std::uint32_t,11> consumption_stores{0x102395cc,0x102395fc,0x1023962c,0x1023964c,
            0x10239664,0x102396c8,0x102396dc,0x1023971c,0x10239730,0x10239770,0x10239784};
        if(index>=5) {
            bool point=false;unsigned frame=0;
            if(index==5) {
                if(site==Site::before_draws)point=s.pc==0x10239794;
                else if(site==Site::final_output) {point=s.pc==0x10239ce0;frame=112;}
                else {
                    constexpr std::array<std::uint32_t,8> scratch{0x10239844,0x102398bc,0x10239904,0x10239998,0x10239a08,0x10239af4,0x10239b64,0x10239c20};
                    point=std::find(scratch.begin(),scratch.end(),s.pc)!=scratch.end();frame=112;
                }
            } else if(index==6) {point=!primary && s.pc==(site==Site::before_draws?0x10187650u:0x10187674u);}
            else if(index==7) {
                point=primary ? s.pc==(site==Site::before_draws?0x10187684u:site==Site::after_maximum?0x101876a4u:0x101876a8u) :
                    s.pc==(site==Site::before_draws?0x10187738u:site==Site::after_maximum?0x10187748u:0x101877bcu);
                frame=primary?0:128;
            } else {
                point=primary ? s.pc==(site==Site::before_draws?0x101878d4u:site==Site::after_maximum?0x101878ecu:0x101878f0u) :
                    s.pc==(site==Site::before_draws?0x10187958u:site==Site::after_maximum?0x1018795cu:0x10187a00u);
                frame=primary?0:96;
            }
            if(stage!=1 || !point || s.r<4>(4)!=parent_frame-frame)return;
        } else {
        const auto pc=index==3 ? periodic_sites[unsigned(site)] : index==2 ? recovery_sites[unsigned(site)] :
            primary && site==Site::before_draws ? 0x10235ee4u : interrupted_sites[unsigned(site)];
        const auto frame=index==4?0u:index==3?96u:index==2?80u:128u;
        const bool point=index==4 ? std::find(consumption_stores.begin(),consumption_stores.end(),s.pc)!=consumption_stores.end() : s.pc==pc;
        if(stage!=1 || !point || s.r<4>(4)!=parent_frame-frame)return;
        }
        ++callbacks; stage = 2;
        s.wr<4>(12, scenario == Scenario::restore_fault ? fixture + 0xffe : saved_context); s.wr<4>(16, 7);
        invoke_register_context(s, 0x10773f30);
        ++callback_after; throw std::runtime_error("Scalar restore returned to abandoned match frame");
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
void compare(const State& seed, unsigned index, Scenario scenario, bool original_context, bool turbo, Site site, bool primary) {
    auto a = clone(seed), b = clone(seed);
    for(auto* s:{a.get(),b.get()}){
        s->pc = parent_import; s->wr<4>(4128, caller); s->budget = s->executed + 1000000;
    }
    actor_attributes_diagnostics(*a).original = true; a->register_context_original = true;
    b->register_context_original = original_context;
    Model old(index, scenario, *a, site), modern(index, scenario, *b, site); Monitor x(*a, old,primary), y(*b, modern,primary);
    a->code_fragment_observer = &old; b->code_fragment_observer = &modern;
    a->register_context_observer=&old.context;b->register_context_observer=&modern.context;
    const auto execute = [](State& s) {
        try { run_until(s, caller); }
        catch (const GuestContextTransfer&) { return std::string("Escaped match transfer owner"); }
        catch (const std::exception& e) { return std::string(e.what()); }
        return std::string{};
    };
    set_match_turbo_enabled(false); const auto ae = execute(*a);
    set_match_turbo_enabled(turbo); const auto be = execute(*b); set_match_turbo_enabled(false);
    a->memory.observer=b->memory.observer=nullptr; a->memory.date_observer = b->memory.date_observer = nullptr; a->code_fragment_observer = b->code_fragment_observer = nullptr;
    a->register_context_observer=b->register_context_observer=nullptr;
    require(old.context.frames.empty()&&modern.context.frames.empty(),"No abandoned context accounting scope");
    const auto label = hex_address(line::entries[index]) + " nonlocal scenario=" + std::to_string(unsigned(scenario))+" site="+std::to_string(unsigned(site))+" primary="+std::to_string(primary);
    require(ae == be && !ae.empty() == (scenario != Scenario::jump), label + " outcomes original=" + ae + " modern=" + be);
    if (scenario == Scenario::restore_fault) require(ae.starts_with("Unmapped data access at "), label + " restore fault prefix");
    if (scenario == Scenario::handler_fault) require(ae == "Fixture match resumed-handler fault", label + " handler fault");
    Image(*a).compare(*b, label); require(x.effects.bytes == y.effects.bytes && old.imports == modern.imports, label + " ordered effects and actual import boundaries");
    if (original_context) require(a->executed == b->executed, label + " accounting with original context dependency");
    require(a->executed - old.context.total == b->executed - modern.context.total, label + " accounting outside measured context dependency");
    require(!old.parent_after && !modern.parent_after && !old.callback_after && !modern.callback_after && b->memory.read<4>(marker) == 0xa7a7a7a7,
        label + " abandoned host continuations");
    const auto& d = actor_attributes_diagnostics(*b);
    require(d.calls[index] == 1 && !d.returns[index] && !d.original_calls[index] && !d.original_body && !d.dispatch_escapes &&
        d.transfers[index] == unsigned(scenario != Scenario::restore_fault) && d.faults[index] == unsigned(scenario == Scenario::restore_fault) &&
        !d.terminations[index] && !a->register_context_unknown_transfers && !b->register_context_unknown_transfers, label + " dedicated productive transfer evidence");
    const auto helpers=index>=5 || index==2 || index==4 || site==Site::before_draws?0u:index==3?4u:site==Site::after_maximum?1u:4u;
    require(d.calls[1]==helpers && d.returns[1]==helpers && !d.transfers[1] && !d.faults[1],label+" completed maximum children");
    if(index==3) {
        const bool interrupted_child=site==Site::before_draws;
        require(d.calls[0]==1 && d.returns[0]==unsigned(!interrupted_child) &&
            d.transfers[0]==unsigned(interrupted_child && scenario!=Scenario::restore_fault) &&
            d.faults[0]==unsigned(interrupted_child && scenario==Scenario::restore_fault),label+" nested refresh outcome counts");
    }
    require(old.callbacks == 1 && modern.callbacks == 1 && old.handlers == modern.handlers && modern.handlers == unsigned(scenario != Scenario::restore_fault),
        label + " exact interrupted callback and resumed handler counts");
    ++cases; fault_cases += scenario != Scenario::jump;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 5 && !match_pc_profile_enabled(), "Original program/game/private root and original-caller capture directory with profiling disabled");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        OwnedBacking cleanup{root};
        for (unsigned index : {0u,2u,3u,4u,5u,6u,7u,8u}) {
            State seed; load_program(seed,argv[1]);
            load_match_snapshot(seed,std::filesystem::path(argv[4])/(hex_address(line::entries[index])+".snapshot"));
            require(seed.pc==line::entries[index],"Transfer input must be a captured original caller scope");
            seed.memory.map(fixture, 0x1000); seed.memory.copy_in(fixture, std::vector<std::uint8_t>(0x1000, 0xa7));
            for(bool primary:{false,true})for(auto site:{Site::before_draws,Site::after_maximum,Site::final_output}) {
                if(index==4 && site!=Site::final_output)continue;
                if(index==6 && (primary || site==Site::final_output))continue;
                for(auto scenario:{Scenario::jump,Scenario::restore_fault,Scenario::handler_fault})
                    for(bool original_context:{false,true})compare(seed,index,scenario,original_context,false,site,primary);
                compare(seed,index,Scenario::jump,false,true,site,primary);
            }
        }
        require(cases==280 && fault_cases==160,"Complete actor attribute/condition callback transfer matrix");
        std::cout << cases << " productive nonlocal actor-attributes comparisons; " << fault_cases
            << " restore/handler faults; actor-attribute scopes, integer/float saves and callback continuations abandoned\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
