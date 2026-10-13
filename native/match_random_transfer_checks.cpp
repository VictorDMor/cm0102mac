#include "match_random_adapter.hpp"
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
namespace actor = cm::aot::match_random;
constexpr std::uint32_t caller = 0x7fff1000, parent_import = 280984548, parent_function = parent_import - 0x100;
constexpr std::uint32_t saved_context = fixture + 0x800, marker = fixture + 0xc00;
constexpr std::array<std::uint32_t,1> framed_reads{0x101d3ca4};
constexpr std::array<unsigned,1> frames{112};
enum class Scenario { jump, restore_fault, handler_fault };
struct Model final : CodeFragmentBoundaryObserver {
    unsigned index, stage = 0, parent_after = 0, callback_after = 0, callbacks = 0, handlers = 0;
    Scenario scenario;
    unsigned depth;
    std::uint32_t parent_frame = 0;
    std::uint64_t context_steps = 0;
    std::array<std::uint32_t, 8> arguments;
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
    Model(unsigned root, Scenario kind, const State& s, unsigned nesting) : index(root), scenario(kind), depth(nesting),
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
        { Accounting measured(s, context_steps); parent.call(0x10773eb0, parent_import); }
        require(s.r<4>(12) == 0, "Original scalar capture initial zero");
        for(unsigned n=0;n<arguments.size();++n)s.wr<4>((n+3)*4,arguments[n]);
        stage = 1; parent.call(actor::entries[index], parent_function + 0x40); ++parent_after;
        parent.store(marker, 1, parent_function + 0x44); parent.pop(112, parent_function + 0x48);
        parent.restore_link(); parent.finish(); return true;
    }
    void interrupt(State& s) {
        if (stage != 1 || s.pc != framed_reads[index] ||
            s.r<4>(4) != parent_frame - depth * frames[index]) return;
        ++callbacks; stage = 2;
        s.wr<4>(12, scenario == Scenario::restore_fault ? fixture + 0xffe : saved_context); s.wr<4>(16, 7);
        { Accounting measured(s, context_steps); invoke_register_context(s, 0x10773f30); }
        ++callback_after; throw std::runtime_error("Scalar restore returned to abandoned match frame");
    }
};
struct Monitor final : MemoryObserver {
    State& state; Model& model; Effects effects;
    Monitor(State& s, Model& m) : state(s), model(m) { effects.state = &s; s.memory.date_observer = this; }
    ~Monitor() { state.memory.date_observer = nullptr; }
    void mapped(std::uint32_t a, std::uint64_t n) override { effects.mapped(a, n); }
    void cleared(std::uint32_t a, std::uint64_t n) override { effects.cleared(a, n); }
    void read(std::uint32_t a, const std::uint8_t* b, unsigned n) override { effects.read(a, b, n); model.interrupt(state); }
    void written(std::uint32_t a, const std::uint8_t* b, std::size_t n) override { effects.written(a, b, n); model.interrupt(state); }
};
struct OwnedBacking {
    std::filesystem::path directory;
    ~OwnedBacking(){std::error_code e;if(!std::filesystem::exists(directory,e)||e||std::filesystem::is_symlink(directory,e)||e)return;
        for(const auto& p:std::filesystem::recursive_directory_iterator(directory,e))if(e||p.is_symlink(e)||e)return;
        std::filesystem::remove_all(directory,e);}
};
unsigned cases = 0, fault_cases = 0;
void compare(const State& seed, unsigned index, Scenario scenario, bool original_context, bool turbo, unsigned depth) {
    auto a = clone(seed), b = clone(seed);
    for(auto* s:{a.get(),b.get()}){
        s->pc = parent_import; s->wr<4>(4128, caller); s->budget = s->executed + 1000000;
    }
    match_random_diagnostics(*a).original = true; a->register_context_original = true;
    b->register_context_original = original_context;
    if (depth == 2) { a->wr<4>(12,1000000); b->wr<4>(12,1000000); }
    Model old(index, scenario, *a, depth), modern(index, scenario, *b, depth); Monitor x(*a, old), y(*b, modern);
    a->code_fragment_observer = &old; b->code_fragment_observer = &modern;
    const auto execute = [](State& s) {
        try { run_until(s, caller); }
        catch (const GuestContextTransfer&) { return std::string("Escaped match transfer owner"); }
        catch (const std::exception& e) { return std::string(e.what()); }
        return std::string{};
    };
    set_match_turbo_enabled(false); const auto ae = execute(*a);
    set_match_turbo_enabled(turbo); const auto be = execute(*b); set_match_turbo_enabled(false);
    a->memory.date_observer = b->memory.date_observer = nullptr; a->code_fragment_observer = b->code_fragment_observer = nullptr;
    const auto label = hex_address(actor::entries[index]) + " nonlocal scenario=" + std::to_string(unsigned(scenario));
    require(ae == be && !ae.empty() == (scenario != Scenario::jump), label + " outcomes original=" + ae + " modern=" + be);
    if (scenario == Scenario::restore_fault) require(ae.starts_with("Unmapped data access at "), label + " restore fault prefix");
    if (scenario == Scenario::handler_fault) require(ae == "Fixture match resumed-handler fault", label + " handler fault");
    Image(*a).compare(*b, label); require(x.effects.bytes == y.effects.bytes && old.imports == modern.imports, label + " ordered effects and actual import boundaries");
    if (original_context) require(a->executed == b->executed, label + " accounting with original context dependency");
    require(a->executed - old.context_steps == b->executed - modern.context_steps, label + " accounting outside measured context dependency");
    require(!old.parent_after && !modern.parent_after && !old.callback_after && !modern.callback_after && b->memory.read<4>(marker) == 0xa7a7a7a7,
        label + " abandoned host continuations");
    const auto& d = match_random_diagnostics(*b);
    require(d.calls[index] == depth && !d.returns[index] && !d.original_calls[index] && !d.original_body && !d.dispatch_escapes &&
        d.transfers[index] == depth * unsigned(scenario != Scenario::restore_fault) && d.faults[index] == depth * unsigned(scenario == Scenario::restore_fault) &&
        !d.terminations[index] && !a->register_context_unknown_transfers && !b->register_context_unknown_transfers, label + " dedicated productive transfer evidence");
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
        for (unsigned index = 0; index < actor::entries.size(); ++index) {
            State seed; load_program(seed,argv[1]);
            load_match_snapshot(seed,std::filesystem::path(argv[4])/(hex_address(actor::entries[index])+".snapshot"));
            require(seed.pc==actor::entries[index],"Transfer input must be a captured original caller scope");
            seed.memory.map(fixture, 0x1000); seed.memory.copy_in(fixture, std::vector<std::uint8_t>(0x1000, 0xa7));
            for (const auto depth : {1u,2u}) {
                for (auto scenario : {Scenario::jump, Scenario::restore_fault, Scenario::handler_fault})
                    for (bool original_context : {false,true}) compare(seed, index, scenario, original_context, false, depth);
                compare(seed, index, Scenario::jump, false, true, depth);
            }
        }
        std::cout << cases << " productive nonlocal match-random comparisons; " << fault_cases
            << " restore/handler faults; outer/recursive sampler scopes and integer saves and callback continuations abandoned\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
