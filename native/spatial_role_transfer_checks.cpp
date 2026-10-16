#include "spatial_role_adapter.hpp"
#include "match_debugger_factory.hpp"
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
namespace actor = cm::aot::spatial_role;
constexpr std::uint32_t caller = 0x7fff1000, parent_import = 280984548, parent_function = parent_import - 0x100;
constexpr std::uint32_t saved_context = fixture + 0x800, marker = fixture + 0xc00;
constexpr std::array<std::uint32_t,23> framed_reads{
    0x101d3b50,0x101d5680,0x101d5690,0x101d56c0,0x101d5730,0x101d5744,0x101d7db0,
    0x101e3dc4,0x101e3e54,0x101e3ee4,0x101e3f00,0x101f189c,0x101f1930,0x101f1940,
    0x101f196c,0x101f1dcc,0x101f1df4,0x101f1e2c,0x101f21c0,0x10289550,0x10289570,0x102895b0,0x102895f0};
constexpr std::array<unsigned,23> frames{0,0,0,0,0,0,0,64,64,64,0,64,0,0,160,0,0,96,0,0,64,64,64};
enum class Scenario { jump, restore_fault, handler_fault };
struct Model final : CodeFragmentBoundaryObserver {
    unsigned index, stage = 0, parent_after = 0, callback_after = 0, callbacks = 0, handlers = 0;
    Scenario scenario;
    std::uint32_t parent_frame = 0;
    std::uint64_t context_steps = 0;
    std::array<std::uint32_t, 6> arguments;
    struct Event {
        bool leave; std::string name; decltype(State::registers) registers; std::uint32_t pc;
        bool operator==(const Event&) const = default;
    };
    std::vector<Event> imports;
    struct Accounting {
        State& state; std::uint64_t& total; std::uint64_t before;
        Accounting(State& s, std::uint64_t& t) : state(s), total(t), before(s.executed) {}
        ~Accounting() { total += state.executed - before; }
    };
    Model(unsigned root, Scenario kind, const State& s) : index(root), scenario(kind),
        arguments{std::uint32_t(s.r<4>(12)), std::uint32_t(s.r<4>(16)), std::uint32_t(s.r<4>(20)),std::uint32_t(s.r<4>(24)),std::uint32_t(s.r<4>(28)),std::uint32_t(s.r<4>(32))} {}
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
        { Accounting measured(s, context_steps); parent.call(0x10773eb0, parent_import); }
        require(s.r<4>(12) == 0, "Original scalar capture initial zero");
        for(unsigned n=0;n<arguments.size();++n)s.wr<4>((n+3)*4,arguments[n]);
        stage = 1; parent.call(actor::entries[index], parent_function + 0x40); ++parent_after;
        parent.store(marker, 1, parent_function + 0x44); parent.pop(112, parent_function + 0x48);
        parent.restore_link(); parent.finish(); return true;
    }
    void interrupt(State& s) {
        if (stage != 1 || s.pc != framed_reads[index] ||
            s.r<4>(4) != parent_frame - frames[index]) return;
        ++callbacks; stage = 2;
        s.wr<4>(12, scenario == Scenario::restore_fault ? fixture + 0xffe : saved_context); s.wr<4>(16, 7);
        { Accounting measured(s, context_steps); invoke_register_context(s, 0x10773f30); }
        ++callback_after; throw std::runtime_error("Scalar restore returned to abandoned match frame");
    }
};
struct Monitor final : MemoryObserver {
    State& state; Model& model; Effects effects;
    Monitor(State& s, Model& m, bool primary) : state(s), model(m) {
        effects.state = &s;
        if(primary)s.memory.observer=this;else s.memory.date_observer=this;
    }
    ~Monitor() { state.memory.date_observer = nullptr; state.memory.observer = nullptr; }
    void mapped(std::uint32_t a, std::uint64_t n) override { effects.mapped(a, n); }
    void cleared(std::uint32_t a, std::uint64_t n) override { effects.cleared(a, n); }
    void read(std::uint32_t a, const std::uint8_t* b, unsigned n) override { effects.read(a, b, n); model.interrupt(state); }
    void written(std::uint32_t a, const std::uint8_t* b, std::size_t n) override { effects.written(a, b, n); model.interrupt(state); }
};
unsigned cases = 0, fault_cases = 0;
void compare(const State& seed, unsigned index, Scenario scenario, bool original_context, bool turbo, bool primary=false) {
    auto a = clone(seed), b = clone(seed);
    const auto match=std::uint32_t(seed.r<4>(12)),player=match+0x311c+8*0x1c4;
    for(auto* s:{a.get(),b.get()}){
        using E=actor::Entry;const auto entry=E(index);s->wr<4>(12,player);s->wr<4>(16,0);s->wr<4>(20,0xffffffff);
        if(entry==E::current_role || entry==E::secondary_role || entry==E::previous_role || entry==E::register_center_cell || entry==E::remove_from_cell){s->wr<4>(12,match);s->wr<4>(16,player);}
        if(entry==E::clear_goalkeeper || entry==E::resolve_goalkeeper)s->wr<4>(12,match);
        s->pc = parent_import; s->wr<4>(4128, caller); s->budget = s->executed + 1000000;
    }
    spatial_role_diagnostics(*a).original = true; a->register_context_original = true;
    b->register_context_original = original_context;
    Model old(index, scenario, *a), modern(index, scenario, *b); Monitor x(*a, old, primary), y(*b, modern, primary);
    a->code_fragment_observer = &old; b->code_fragment_observer = &modern;
    const auto execute = [](State& s) {
        try { run_until(s, caller); }
        catch (const GuestContextTransfer&) { return std::string("Escaped match transfer owner"); }
        catch (const std::exception& e) { return std::string(e.what()); }
        return std::string{};
    };
    set_match_turbo_enabled(false); const auto ae = execute(*a);
    set_match_turbo_enabled(turbo); const auto be = execute(*b); set_match_turbo_enabled(false);
    a->memory.date_observer = b->memory.date_observer = nullptr; a->memory.observer = b->memory.observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr;
    const auto label = hex_address(actor::entries[index]) + " nonlocal scenario=" + std::to_string(unsigned(scenario))+" primary-observer="+std::to_string(primary);
    require(ae == be && !ae.empty() == (scenario != Scenario::jump), label + " outcomes original=" + ae + " modern=" + be);
    if (scenario == Scenario::restore_fault) require(ae.starts_with("Unmapped data access at "), label + " restore fault prefix");
    if (scenario == Scenario::handler_fault) require(ae == "Fixture match resumed-handler fault", label + " handler fault");
    Image(*a).compare(*b, label); require(x.effects.bytes == y.effects.bytes && old.imports == modern.imports, label + " ordered effects and actual import boundaries");
    if (original_context) require(a->executed == b->executed, label + " accounting with original context dependency");
    require(a->executed - old.context_steps == b->executed - modern.context_steps, label + " accounting outside measured context dependency");
    require(!old.parent_after && !modern.parent_after && !old.callback_after && !modern.callback_after && b->memory.read<4>(marker) == 0xa7a7a7a7,
        label + " abandoned host continuations");
    const auto& d = spatial_role_diagnostics(*b);
    require(d.calls[index] == 1 && !d.returns[index] && !d.original_calls[index] && !d.original_body && !d.dispatch_escapes &&
        d.transfers[index] == unsigned(scenario != Scenario::restore_fault) && d.faults[index] == unsigned(scenario == Scenario::restore_fault) &&
        !d.terminations[index] && !a->register_context_unknown_transfers && !b->register_context_unknown_transfers, label + " dedicated productive transfer evidence");
    require(old.callbacks == 1 && modern.callbacks == 1 && old.handlers == modern.handlers && modern.handlers == unsigned(scenario != Scenario::restore_fault),
        label + " exact interrupted callback and resumed handler counts");
    ++cases; fault_cases += scenario != Scenario::jump;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 4 && !match_pc_profile_enabled(), "Original program/game/private root with profiling disabled");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        auto created = cm::aot::match_debugger::create_match({argv[1], argv[2], root}, {677, 5677, 12345, 2001, 194});
        auto seed = std::move(created.state); seed->wr<4>(12, created.match);
        seed->memory.map(fixture, 0x1000); seed->memory.copy_in(fixture, std::vector<std::uint8_t>(0x1000, 0xa7));
        for (unsigned root = 0; root < actor::entries.size(); ++root) {
            for (auto scenario : {Scenario::jump, Scenario::restore_fault, Scenario::handler_fault})
                for (bool original_context : {false,true}) compare(*seed, root, scenario, original_context, false);
            compare(*seed, root, Scenario::jump, false, true);
        }
        // Writes can also call out through the independent transcript observer.
        // Its presence must retain the unframed native continuation scope.
        for(unsigned root:{4u,5u,12u,13u,18u}){
            for(auto scenario:{Scenario::jump,Scenario::restore_fault,Scenario::handler_fault})
                for(bool original_context:{false,true})compare(*seed,root,scenario,original_context,false,true);
            compare(*seed,root,Scenario::jump,false,true,true);
        }
        require(cases==196 && fault_cases==112,"Complete spatial-role transfer matrix with both observer channels");
        std::cout << cases << " productive nonlocal spatial-role comparisons; " << fault_cases
            << " restore/handler faults; unframed leaves and 64/96/160-byte role/cell/keeper scopes and callback continuations abandoned\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
