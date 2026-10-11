#include "record_query_fixtures.hpp"
#include "code_fragment_adapter.hpp"
#include "register_context_adapter.hpp"
#include "guest_context_transfer.hpp"
#include "mac_abi.hpp"
#include "match_turbo.hpp"

namespace {
using namespace cm::record_query_checks;
namespace record = cm::aot::record_query;
constexpr std::uint32_t caller = 0x7fff1000, parent_import = 280984548, parent_function = parent_import - 0x100;
constexpr std::uint32_t saved_context = fixture + 0x800, marker = fixture + 0xc00;
constexpr std::array<unsigned, 6> link_reads{0, 0x40, 0xb8, 0xdc, 0xac, 0xbc};
enum class Scenario { jump, restore_fault, handler_fault };
struct Model final : CodeFragmentBoundaryObserver {
    unsigned index, stage = 0, parent_after = 0, callback_after = 0, callbacks = 0, handlers = 0;
    Scenario scenario;
    std::uint32_t parent_frame = 0;
    std::uint64_t context_steps = 0;
    std::array<std::uint32_t, 8> arguments;
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
    Model(unsigned root, Scenario kind, const State& s) : index(root), scenario(kind),
        arguments{std::uint32_t(s.r<4>(12)), std::uint32_t(s.r<4>(16)), std::uint32_t(s.r<4>(20)), std::uint32_t(s.r<4>(24)), std::uint32_t(s.r<4>(28)), std::uint32_t(s.r<4>(32)), std::uint32_t(s.r<4>(36)), std::uint32_t(s.r<4>(40))} {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override { imports.push_back({false, name, s.registers, s.pc}); }
    void import_leave(const State& s, const char* name) override { imports.push_back({true, name, s.registers, s.pc}); }
    bool import_fixture(State& s, const char* name) override {
        // The surviving parent import is the only controlled dependency.
        // Object allocation, query evaluation and CFM dispatch
        // all retain their actual production implementations.
        if (s.pc != parent_import) return false;
        require(std::string_view(name) == "CarbonLib::Gestalt", "Explicit record transfer parent import");
        if (stage == 2) {
            ++handlers; require(s.r<4>(12) == 7 && s.r<4>(4) == parent_frame, "Original surviving scalar destination");
            if (scenario == Scenario::handler_fault) throw std::runtime_error("Fixture record resumed-handler fault");
            s.wr<4>(4128, caller); s.pc = caller; return true;
        }
        require(!stage, "Parent import has one initial capture");
        std::uint64_t escapes = 0; s.pc = parent_function; MacAbi parent(s, escapes, parent_function);
        parent.capture_link(); parent.save_link(parent_function + 4); parent.frame(112, parent_function + 8);
        parent_frame = parent.r(1); s.wr<4>(12, saved_context);
        { Accounting measured(s, context_steps); parent.call(0x10773eb0, parent_import); }
        require(s.r<4>(12) == 0, "Original scalar capture initial zero");
        for (unsigned i = 0; i < arguments.size(); ++i) s.wr<4>(12 + i * 4, arguments[i]);
        stage = 1; parent.call(record::entries[index], parent_function + 0x40); ++parent_after;
        parent.store(marker, 1, parent_function + 0x44); parent.pop(112, parent_function + 0x48);
        parent.restore_link(); parent.finish(); return true;
    }
    void interrupt(State& s, bool writing) {
        const bool write_site = index == 0;
        const auto pc = record::entries[index] + link_reads[index];
        if (stage != 1 || writing != write_site || s.pc != pc ||
            s.r<4>(4) != parent_frame - std::array<unsigned, 6>{0, 0, 112, 112, 112, 112}[index]) return;
        ++callbacks; stage = 2;
        s.wr<4>(12, scenario == Scenario::restore_fault ? fixture + 0xffe : saved_context); s.wr<4>(16, 7);
        { Accounting measured(s, context_steps); invoke_register_context(s, 0x10773f30); }
        ++callback_after; throw std::runtime_error("Scalar restore returned to abandoned record object");
    }
};
struct Monitor final : MemoryObserver {
    State& state; Model& model; Effects effects;
    Monitor(State& s, Model& m) : state(s), model(m) { effects.state = &s; s.memory.date_observer = this; }
    ~Monitor() { state.memory.date_observer = nullptr; }
    void mapped(std::uint32_t a, std::uint64_t n) override { effects.mapped(a, n); }
    void cleared(std::uint32_t a, std::uint64_t n) override { effects.cleared(a, n); }
    void read(std::uint32_t a, const std::uint8_t* b, unsigned n) override { effects.read(a, b, n); model.interrupt(state, false); }
    void written(std::uint32_t a, const std::uint8_t* b, std::size_t n) override { effects.written(a, b, n); model.interrupt(state, true); }
};
unsigned cases = 0, fault_cases = 0;
void compare(const State& seed, unsigned index, Scenario scenario, unsigned mode, bool turbo) {
    auto a = clone(seed), b = clone(seed);
    cm::engine_initialization_checks::NativeEnvironment environment(*a, *b);
    prepare(*a, index); prepare(*b, index);
    a->pc = b->pc = parent_import; a->wr<4>(4128, caller); b->wr<4>(4128, caller);
    record_query_diagnostics(*a).original = true; a->register_context_original = true;
    if (mode == 0) b->register_context_original = true;
    if (mode == 2) b->register_context_entry_original[28] = true;
    if (mode == 3) b->register_context_entry_original[29] = true;
    Model old(index, scenario, *a), modern(index, scenario, *b); Monitor x(*a, old), y(*b, modern);
    a->code_fragment_observer = &old; b->code_fragment_observer = &modern;
    const auto execute = [](State& s) {
        try { run_until(s, caller); }
        catch (const GuestContextTransfer&) { return std::string("Escaped record transfer owner"); }
        catch (const std::exception& e) { return std::string(e.what()); }
        return std::string{};
    };
    set_match_turbo_enabled(false); const auto ae = execute(*a);
    set_match_turbo_enabled(turbo); const auto be = execute(*b); set_match_turbo_enabled(false);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr;
    const auto label = hex_address(record::entries[index]) + " nonlocal scenario=" + std::to_string(unsigned(scenario));
    require(ae == be && !ae.empty() == (scenario != Scenario::jump), label + " outcomes old=" + ae + " modern=" + be);
    if (scenario == Scenario::restore_fault) require(ae.starts_with("Unmapped data access at "), label + " restore fault prefix");
    if (scenario == Scenario::handler_fault) require(ae == "Fixture record resumed-handler fault", label + " handler fault");
    Image(*a).compare(*b, label);
    require(x.effects.bytes == y.effects.bytes && old.imports == modern.imports, label + " ordered effects and actual import boundaries");
    if (!mode) require(a->executed == b->executed, label + " record accounting with original scalar dependency");
    require(a->executed - old.context_steps == b->executed - modern.context_steps,
        label + " exact lifetime accounting outside measured scalar-context dependency");
    require(!old.parent_after && !modern.parent_after && !old.callback_after && !modern.callback_after &&
        b->memory.read<4>(marker) == 0xa7a7a7a7, label + " abandoned host continuations");
    const auto& d = record_query_diagnostics(*b);
    require(d.calls[index] == d.returns[index] + 1 && !d.original_calls[index] && !d.original_body && !d.dispatch_escapes &&
        d.transfers[index] == unsigned(scenario != Scenario::restore_fault) && d.faults[index] == unsigned(scenario == Scenario::restore_fault) &&
        !d.terminations[index] && !a->register_context_unknown_transfers && !b->register_context_unknown_transfers,
        label + " dedicated known-transfer evidence");
    require(old.callbacks == 1 && modern.callbacks == 1 && old.handlers == modern.handlers &&
        modern.handlers == unsigned(scenario != Scenario::restore_fault), label + " callback and resumed-handler counts");
    ++cases; fault_cases += scenario != Scenario::jump;
}
}
void record_query_transfer_checks(const std::filesystem::path& input) {
    require(!match_pc_profile_enabled(), "Record transfer acceptance requires profiling disabled");
    auto seed = std::make_unique<State>(); load_program(*seed, input); seed->budget = 100000000;
    seed->memory.map(fixture, 0x1000); seed->memory.copy_in(fixture, std::vector<std::uint8_t>(0x1000, 0xa7));
    for (unsigned root = 0; root < record::entries.size(); ++root) {
        for (auto kind : {Scenario::jump, Scenario::restore_fault, Scenario::handler_fault})
            for (unsigned mode = 0; mode < 2; ++mode) compare(*seed, root, kind, mode, false);
        compare(*seed, root, Scenario::jump, 1, true);
    }
    for (unsigned mode : {2u, 3u}) for (unsigned root : {0u, 1u}) compare(*seed, root, Scenario::jump, mode, false);
    std::cout << cases << " nonlocal record comparisons; " << fault_cases
        << " restore/handler faults; two leaves and four query frames and callback continuations abandoned\n";
}
