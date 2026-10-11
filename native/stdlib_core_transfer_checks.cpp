#include "stdlib_core_adapter.hpp"
#include "stdlib_core_fixtures.hpp"
#include "register_context_adapter.hpp"
#include "guest_context_transfer.hpp"
#include "mac_abi.hpp"
#include "match_turbo.hpp"

namespace {
using namespace cm::stdlib_core_checks;
constexpr auto parent_function = comparator_import - 0x100, saved_context = fixture + 0xa00, marker = fixture + 0xb00;
enum class Scenario { jump, recursive, restore_fault, handler_fault };
unsigned cases = 0, fault_cases = 0;
struct Model final : CodeFragmentBoundaryObserver {
    Scenario scenario; unsigned stage = 0, callbacks = 0, parent_after = 0, callback_after = 0, handlers = 0;
    std::uint32_t parent_frame = 0;
    std::vector<Comparator::Boundary> boundaries;
    explicit Model(Scenario kind) : scenario(kind) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override { boundaries.push_back({false, name, s.registers, s.pc, s.native_calls}); }
    void import_leave(const State& s, const char* name) override { boundaries.push_back({true, name, s.registers, s.pc, s.native_calls}); }
    void arguments(State& s) {
        s.wr<4>(12, key); s.wr<4>(16, table); s.wr<4>(20, 9); s.wr<4>(24, 4); s.wr<4>(28, descriptor);
    }
    bool import_fixture(State& s, const char* name) override {
        require(std::string_view(name) == "CarbonLib::Gestalt", "Explicit search transfer fixture import");
        if (stage == 2) {
            ++handlers; require(s.r<4>(12) == 7 && s.r<4>(4) == parent_frame, "Actual nonlocal search destination");
            if (scenario == Scenario::handler_fault) throw std::runtime_error("Fixture search resumed-handler fault");
            s.wr<4>(4128, caller); s.pc = caller; return true;
        }
        std::uint64_t escapes = 0;
        if (!stage) {
            s.pc = parent_function; MacAbi parent(s, escapes, parent_function);
            parent.capture_link(); parent.save_link(parent_function + 4); parent.frame(112, parent_function + 8);
            parent_frame = parent.r(1); s.wr<4>(12, saved_context); parent.call(0x10773eb0, comparator_import);
            require(s.r<4>(12) == 0, "Actual scalar capture initially returns zero");
            stage = 1; arguments(s); parent.call(entries[3], parent_function + 0x40); ++parent_after;
            parent.store(marker, 1, parent_function + 0x44); parent.pop(112, parent_function + 0x48);
            parent.restore_link(); parent.finish(); return true;
        }
        ++callbacks;
        if (scenario == Scenario::recursive && callbacks == 1) {
            MacAbi nested(s, escapes, entries[3]); arguments(s); nested.call(entries[3], parent_function + 0x80);
            ++callback_after; throw std::runtime_error("Recursive search returned to abandoned comparator");
        }
        stage = 2; s.wr<4>(12, scenario == Scenario::restore_fault ? fixture + 0xffe : saved_context);
        s.wr<4>(16, 7); invoke_register_context(s, 0x10773f30); ++callback_after;
        throw std::runtime_error("Scalar restore returned to abandoned comparator");
    }
};
void compare(Scenario kind, bool turbo, unsigned mode) {
    auto a = fixture_state(3), b = clone(*a); a->pc = b->pc = comparator_import;
    // Parent and search frames, including nested calls, fit this bounded fixture.
    for (auto* s : {a.get(), b.get()}) { s->memory.map(stack - 512, 256); s->memory.clear(stack - 512, 256); }
    a->stdlib_core_original = true; a->register_context_original = true;
    if (mode == 0) b->register_context_original = true;
    if (mode == 2) b->register_context_entry_original[28] = true;
    if (mode == 3) b->register_context_entry_original[29] = true;
    Monitor x, y; x.effects.state = a.get(); y.effects.state = b.get(); Model old(kind), modern(kind);
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    a->code_fragment_observer = &old; b->code_fragment_observer = &modern;
    const auto execute = [](State& s) {
        try { run_until(s, caller); }
        catch (const GuestContextTransfer&) { return std::string("Escaped search transfer owner"); }
        catch (const std::exception& error) { return std::string(error.what()); }
        return std::string{};
    };
    set_match_turbo_enabled(false); const auto ax = execute(*a);
    set_match_turbo_enabled(turbo); const auto bx = execute(*b);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr;
    const auto label = "Search nonlocal scenario=" + std::to_string(unsigned(kind)) + "/mode=" + std::to_string(mode);
    require(ax == bx, label + " outcomes original=" + ax + " modern=" + bx);
    const bool fault = kind == Scenario::restore_fault || kind == Scenario::handler_fault;
    require(!ax.empty() == fault, label + " expected completion " + ax);
    if (kind == Scenario::restore_fault) require(ax.starts_with("Unmapped data access at "), label + " restore fault prefix");
    if (kind == Scenario::handler_fault) require(ax == "Fixture search resumed-handler fault", label + " handler fault");
    Image(*a).compare(*b, label);
    require(x.effects.bytes == y.effects.bytes && old.boundaries == modern.boundaries, label + " ordered memory/import boundaries");
    require(!old.parent_after && !modern.parent_after && !old.callback_after && !modern.callback_after &&
        b->memory.read<4>(marker) == 0xa7a7a7a7, label + " abandoned search/callback epilogues never run");
    const unsigned roots = kind == Scenario::recursive ? 2 : 1;
    require(b->stdlib_core_calls[3] == roots && !b->stdlib_core_returns[3] &&
        b->stdlib_core_transfers[3] == (kind == Scenario::restore_fault ? 0u : roots) &&
        b->stdlib_core_faults[3] == (kind == Scenario::restore_fault ? roots : 0u) &&
        !b->stdlib_core_original_calls[3] && !b->stdlib_core_budget_resumes[3] && !b->stdlib_core_original_body &&
        !b->stdlib_core_dispatch_escapes && !a->register_context_unknown_transfers && !b->register_context_unknown_transfers,
        label + " dedicated modern search and known scalar ownership");
    require(old.callbacks == modern.callbacks && modern.callbacks == roots && old.handlers == modern.handlers &&
        modern.handlers == unsigned(kind != Scenario::restore_fault), label + " exact surviving handler and callback counts");
    ++cases; fault_cases += fault;
}
}
int main() {
    try {
        require(!match_pc_profile_enabled(), "Search transfer acceptance requires profiling disabled");
        for (auto kind : {Scenario::jump, Scenario::recursive, Scenario::restore_fault, Scenario::handler_fault})
            for (bool turbo : {false, true}) for (unsigned mode = 0; mode < 4; ++mode) compare(kind, turbo, mode);
        std::cout << cases << " original/modern/mixed nonlocal search comparisons; " << fault_cases
            << " restore/handler faults; nested searches abandon interrupted callbacks and frames\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
