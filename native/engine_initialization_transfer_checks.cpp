#include "engine_initialization_adapter.hpp"
#include "code_fragment_adapter.hpp"
#include "register_context_adapter.hpp"
#include "guest_context_transfer.hpp"
#include "semantic_io_checks.hpp"
#include "mac_abi.hpp"
#include "match_turbo.hpp"
#include "engine_initialization_fixtures.hpp"

namespace {
using namespace cm::semantic_io_checks;
namespace init = cm::aot::engine_initialization;
constexpr std::uint32_t caller = 0x7fff1000, parent_import = 280984548, parent_function = parent_import - 0x100;
constexpr std::uint32_t saved_context = fixture + 0x400, marker = fixture + 0x800;
enum class Scenario { jump, restore_fault, handler_fault };
struct Model final : CodeFragmentBoundaryObserver {
    unsigned index, stage = 0, parent_after = 0, callback_after = 0, callbacks = 0, handlers = 0;
    Scenario scenario;
    std::uint32_t parent_frame = 0, initializer_link = 0;
    std::uint64_t context_steps = 0;
    struct ImportBoundary {
        bool leave; std::string name; decltype(State::registers) registers; std::uint32_t pc;
        bool operator==(const ImportBoundary&) const = default;
    };
    std::vector<ImportBoundary> imports;
    struct Accounting {
        State& state; std::uint64_t& total; std::uint64_t before;
        Accounting(State& s, std::uint64_t& t) : state(s), total(t), before(s.executed) {}
        ~Accounting() { total += state.executed - before; }
    };
    Model(unsigned root, Scenario kind) : index(root), scenario(kind) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override { imports.push_back({false, name, s.registers, s.pc}); }
    void import_leave(const State& s, const char* name) override { imports.push_back({true, name, s.registers, s.pc}); }
    bool import_fixture(State& s, const char* name) override {
        // Only the explicit parent import is controlled. All initializer
        // dependencies and allocation imports execute their production bodies.
        if (s.pc != parent_import) return false;
        require(std::string_view(name) == "CarbonLib::Gestalt", "Explicit transfer parent import");
        if (stage == 2) {
            ++handlers; require(s.r<4>(12) == 7 && s.r<4>(4) == parent_frame, "Original surviving scalar destination");
            if (scenario == Scenario::handler_fault) throw std::runtime_error("Fixture initializer resumed-handler fault");
            s.wr<4>(4128, caller); s.pc = caller; return true;
        }
        require(!stage, "Parent import has one initial capture");
        std::uint64_t escapes = 0; s.pc = parent_function; MacAbi parent(s, escapes, parent_function);
        parent.capture_link(); parent.save_link(parent_function + 4); parent.frame(112, parent_function + 8);
        parent_frame = parent.r(1); s.wr<4>(12, saved_context);
        { Accounting measured(s, context_steps); parent.call(0x10773eb0, parent_import); }
        require(s.r<4>(12) == 0, "Original scalar capture initial zero");
        initializer_link = parent.r(1) + 8;
        stage = 1; parent.call(init::entries[index], parent_function + 0x40); ++parent_after;
        parent.store(marker, 1, parent_function + 0x44); parent.pop(112, parent_function + 0x48);
        parent.restore_link(); parent.finish(); return true;
    }
    void read(State& s, std::uint32_t address, unsigned width) {
        // Generated PC is exact only at a straight-line block's start.
        // Interrupt the actual saved-link load in both implementations.
        if (stage != 1 || width != 4 || address != initializer_link) return;
        ++callbacks; stage = 2;
        s.wr<4>(12, scenario == Scenario::restore_fault ? fixture + 0xffe : saved_context); s.wr<4>(16, 7);
        { Accounting measured(s, context_steps); invoke_register_context(s, 0x10773f30); }
        ++callback_after; throw std::runtime_error("Scalar restore returned to abandoned initializer");
    }
};
unsigned cases = 0, fault_cases = 0;
void compare(const State& seed, unsigned index, Scenario scenario, unsigned mode, bool turbo) {
    auto a = clone(seed), b = clone(seed);
    cm::engine_initialization_checks::NativeEnvironment environment(*a, *b);
    cm::engine_initialization_checks::prepare_initializer(*a, *b, index);
    a->pc = b->pc = parent_import; a->wr<4>(4128, caller); b->wr<4>(4128, caller);
    engine_initialization_diagnostics(*a).original = true; a->register_context_original = true;
    if (mode == 0) b->register_context_original = true;
    if (mode == 2) b->register_context_entry_original[28] = true;
    if (mode == 3) b->register_context_entry_original[29] = true;
    Model old(index, scenario), modern(index, scenario); Effects x, y;
    x.state = a.get(); y.state = b.get();
    x.on_read = [&old](State& s, Effects&, std::uint32_t address, unsigned width) { old.read(s, address, width); };
    y.on_read = [&modern](State& s, Effects&, std::uint32_t address, unsigned width) { modern.read(s, address, width); };
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    a->code_fragment_observer = &old; b->code_fragment_observer = &modern;
    const auto execute = [](State& s) {
        try { run_until(s, caller); }
        catch (const GuestContextTransfer&) { return std::string("Escaped initializer transfer owner"); }
        catch (const std::exception& e) { return std::string(e.what()); }
        return std::string{};
    };
    set_match_turbo_enabled(false); const auto ae = execute(*a);
    set_match_turbo_enabled(turbo); const auto be = execute(*b); set_match_turbo_enabled(false);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr;
    const auto label = hex_address(init::entries[index]) + " nonlocal scenario=" + std::to_string(unsigned(scenario));
    require(ae == be && !ae.empty() == (scenario != Scenario::jump), label + " outcomes old=" + ae + " modern=" + be);
    if (scenario == Scenario::restore_fault) require(ae.starts_with("Unmapped data access at "), label + " restore fault prefix");
    if (scenario == Scenario::handler_fault) require(ae == "Fixture initializer resumed-handler fault", label + " handler fault");
    Image(*a).compare(*b, label);
    require(x.bytes == y.bytes && old.imports == modern.imports, label + " ordered memory effects and actual import boundaries");
    // An interrupted generated block charges its whole straight-line run;
    // semantic checkpoints stop at the observed read. This count is a
    // diagnostic, not a difference in the architectural state checked above.
    if (a->executed - old.context_steps != b->executed - modern.context_steps)
        std::cout << label << " accounting diagnostic original=" << a->executed - old.context_steps
            << " modern=" << b->executed - modern.context_steps << '\n';
    require(!old.parent_after && !modern.parent_after && !old.callback_after && !modern.callback_after &&
        b->memory.read<4>(marker) == 0xa7a7a7a7, label + " abandoned host continuations");
    const auto& d = engine_initialization_diagnostics(*b);
    require(d.calls[index] == 1 && !d.returns[index] && !d.original_calls[index] && !d.original_body && !d.dispatch_escapes &&
        d.transfers[index] == unsigned(scenario != Scenario::restore_fault) && d.faults[index] == unsigned(scenario == Scenario::restore_fault) &&
        !a->register_context_unknown_transfers && !b->register_context_unknown_transfers, label + " dedicated known-transfer evidence");
    require(old.callbacks == 1 && modern.callbacks == 1 && old.handlers == modern.handlers &&
        modern.handlers == unsigned(scenario != Scenario::restore_fault), label + " callback and resumed-handler counts");
    ++cases; fault_cases += scenario != Scenario::jump;
}
}
void engine_initialization_transfer_checks(const std::filesystem::path& input) {
    require(!match_pc_profile_enabled(), "Initializer transfer acceptance requires profiling disabled");
    auto seed = std::make_unique<State>(); load_program(*seed, input); seed->budget = 100000000;
    seed->memory.map(fixture, 0x1000); seed->memory.copy_in(fixture, std::vector<std::uint8_t>(0x1000, 0xa7));
    for (unsigned root = 0; root < init::entries.size(); ++root) {
        // The date and screen initializers have direct C++ owners; their
        // frames no longer use this adapter's per-instruction checkpoints.
        if (init::entries[root] == 0x100939b0 || init::entries[root] == 0x10744640) continue;
        for (auto kind : {Scenario::jump, Scenario::restore_fault, Scenario::handler_fault})
            for (unsigned mode = 0; mode < 2; ++mode) compare(*seed, root, kind, mode, false);
        compare(*seed, root, Scenario::jump, 1, true);
    }
    for (unsigned mode : {2u, 3u}) compare(*seed, 0, Scenario::jump, mode, false);
    std::cout << cases << " nonlocal initializer comparisons; " << fault_cases
        << " restore/handler faults; eighteen legacy initializer frames and interrupted continuations abandoned\n";
}
