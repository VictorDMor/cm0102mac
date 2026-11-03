#include "actor_construction_adapter.hpp"
#include "match_pl_adapter.hpp"
#include "match_replay.hpp"
#include <fstream>
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
namespace actor = cm::aot::actor_construction;
constexpr std::uint32_t caller = 0x7fff1000, parent_import = 280984548, parent_function = parent_import - 0x100;
constexpr std::uint32_t saved_context = fixture + 0x800, marker = fixture + 0xc00;
enum class Scenario { jump, restore_fault, handler_fault };
// The same live-register contract as RoutineChecker, including all CR fields
// and FPSCR. r0/r11/r12 and translator temporaries are not resumed-handler inputs.
auto observable_registers(const State& state) {
    decltype(State::registers) result{};
    const auto retain = [&](unsigned offset, unsigned size) {
        std::copy_n(state.registers.begin() + offset, size, result.begin() + offset);
    };
    for (unsigned n = 1; n < 32; ++n) if (n != 11 && n != 12) retain(n * 4, 4);
    retain(0x4010, 8);
    for (unsigned n = 14; n < 32; ++n) retain(0x4000 + n * 16, 8);
    retain(2304, 8); retain(1280, 32); retain(4128, 4);
    return result;
}
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
    void import_enter(const State& s, const char* name) override { imports.push_back({false, name, observable_registers(s), s.pc}); }
    void import_leave(const State& s, const char* name) override { imports.push_back({true, name, observable_registers(s), s.pc}); }
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
        stage = 1;
        s.wr<4>(2440, s.r<4>(8)); s.wr<4>(4128, parent_function + 0x40);
        try_modern_actor_construction(s, actor::entries[index]);
        ++parent_after;
        parent.store(marker, 1, parent_function + 0x44); parent.pop(112, parent_function + 0x48);
        parent.restore_link(); parent.finish(); return true;
    }
    void interrupt(State& s, std::uint32_t address, bool read) {
        // The constructor's first staff-record read is an observable boundary
        // on either route; typed C++ does not publish translated interior PCs.
        const bool boundary = read && address == (index == 0 ? arguments[0] + 0x28 : arguments[2] + 0x14);
        if (stage != 1 || !boundary ||
            s.r<4>(4) != parent_frame - std::array<unsigned, 2>{128,208}[index]) return;
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
    void read(std::uint32_t a, const std::uint8_t* b, unsigned n) override { effects.read(a, b, n); model.interrupt(state, a, true); }
    void written(std::uint32_t a, const std::uint8_t* b, std::size_t n) override { effects.written(a, b, n); model.interrupt(state, a, false); }
};
unsigned cases = 0, fault_cases = 0;
void compare(const State& seed, unsigned index, Scenario scenario, bool original_context, bool turbo) {
    auto a = clone(seed), b = clone(seed);
    const auto match = std::uint32_t(seed.r<4>(12)), player = match + 0x311c + 8 * 0x1c4;
    for (auto* s : {a.get(), b.get()}) {
        const auto staff=std::uint32_t(s->memory.read<4>(player+0x6c)); unsigned reg=3;
        if(index==0)for(auto value:{match+0x7aac,8u,staff,8u})s->wr<4>(reg++*4,value);
        else for(auto value:{player,match,staff,0u,8u,8u})s->wr<4>(reg++*4,value);
        s->pc = parent_import; s->wr<4>(4128, caller); s->budget = s->executed + 1000000;
    }
    actor_construction_diagnostics(*a).original = true; a->register_context_original = true;
    b->register_context_original = original_context;
    Model old(index, scenario, *a), modern(index, scenario, *b); Monitor x(*a, old), y(*b, modern);
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
    require(observable_registers(*a) == observable_registers(*b), label + " live registers");
    Image image(*a);
    image.registers = b->registers; image.unique = b->unique;
    image.compare(*b, label); // Complete memory, PC, allocator and import state.
    // Stack save/read ordering is diagnostic; its complete bytes were compared
    // above. Keep ordered writes to game records and the saved context itself.
    const auto writes = [&](const Effects& effects) {
        std::vector<std::tuple<unsigned, std::uint32_t, std::uint64_t, std::uint64_t>> result;
        for (const auto& access : effects.log)
            if (access.kind != 4 && !(access.address >= old.parent_frame - 0x1000 && access.address < old.parent_frame + 0x100))
                result.emplace_back(access.kind, access.address, access.size, access.value);
        return result;
    };
    require(writes(x.effects) == writes(y.effects) && old.imports == modern.imports,
        label + " ordered game/context writes and actual import boundaries");
    require(!old.parent_after && !modern.parent_after && !old.callback_after && !modern.callback_after && b->memory.read<4>(marker) == 0xa7a7a7a7,
        label + " abandoned host continuations");
    const auto& d = actor_construction_diagnostics(*b);
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
        const bool captured = argc == 6 && std::string_view(argv[4]) == "--capture";
        require((argc == 4 || captured) && !match_pc_profile_enabled(), "Original program/game/private root with profiling disabled");
        const auto root = std::filesystem::path(argv[3]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
        cm::aot::match_debugger::EngineInstance created;
        if (captured) {
            created.state = std::make_unique<State>();
            const auto input = std::filesystem::path(argv[5]);
            load_match_snapshot(*created.state, input / "initial.snapshot");
            std::ifstream(input / "match.txt") >> created.match;
            require(created.match != 0, "Existing capture lacks match identity");
        } else created = cm::aot::match_debugger::create_match({argv[1], argv[2], root}, {677, 5677, 12345, 2001, 194});
        auto seed = std::move(created.state); seed->wr<4>(12, created.match);
        seed->memory.map(fixture, 0x1000); seed->memory.copy_in(fixture, std::vector<std::uint8_t>(0x1000, 0xa7));
        for (unsigned root = 0; root < actor::entries.size(); ++root) {
            for (auto scenario : {Scenario::jump, Scenario::restore_fault, Scenario::handler_fault})
                for (bool original_context : {false,true}) compare(*seed, root, scenario, original_context, false);
            compare(*seed, root, Scenario::jump, false, true);
        }
        std::cout << cases << " productive nonlocal actor-construction comparisons; " << fault_cases
            << " restore/handler faults; 128-byte preparation and 208-byte initialization scopes and callback continuations abandoned\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
