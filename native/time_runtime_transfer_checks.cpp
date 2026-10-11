#include "time_runtime_adapter.hpp"
#include "time_runtime_fixtures.hpp"
#include "register_context_adapter.hpp"
#include "guest_context_transfer.hpp"
#include "mac_abi.hpp"
#include "match_turbo.hpp"

namespace {
using namespace cm::time_runtime_checks;
constexpr std::uint32_t parent_import = 280984548;
constexpr auto parent_function = parent_import - 0x100, saved_context = fixture + 0xa00, marker = fixture + 0xb00;
enum class Scenario { jump, recursive, restore_fault, handler_fault };
unsigned cases = 0, fault_cases = 0;
struct Model final : CodeFragmentBoundaryObserver {
    Scenario scenario; unsigned index; unsigned stage = 0, callbacks = 0, parent_after = 0, callback_after = 0, handlers = 0;
    std::uint32_t parent_frame = 0;
    std::vector<Imports::Boundary> boundaries;
    Model(Scenario kind, unsigned root) : scenario(kind), index(root) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override { boundaries.push_back({false, name, s.registers, s.pc, s.native_calls}); }
    void import_leave(const State& s, const char* name) override { boundaries.push_back({true, name, s.registers, s.pc, s.native_calls}); }
    void arguments(State& s) {
        s.wr<4>(12, index == 4 || index == 5 ? timestamp : 0xdeadbeef); s.wr<4>(16, fields);
    }
    bool import_fixture(State& s, const char* name) override {
        require(std::string_view(name) == "CarbonLib::Gestalt" || std::string_view(name) == "CarbonLib::GetDateTime" || std::string_view(name) == "CarbonLib::ReadLocation", "Explicit time-runtime transfer fixture import");
        if (stage == 2) {
            ++handlers; require(s.r<4>(12) == 7 && s.r<4>(4) == parent_frame, "Actual nonlocal time-runtime destination");
            if (scenario == Scenario::handler_fault) throw std::runtime_error("Fixture time-runtime resumed-handler fault");
            s.wr<4>(4128, caller); s.pc = caller; return true;
        }
        std::uint64_t escapes = 0;
        if (!stage) {
            s.pc = parent_function; MacAbi parent(s, escapes, parent_function);
            parent.capture_link(); parent.save_link(parent_function + 4); parent.frame(112, parent_function + 8);
            parent_frame = parent.r(1); s.wr<4>(12, saved_context); parent.call(0x10773eb0, parent_import);
            require(s.r<4>(12) == 0, "Actual scalar capture initially returns zero");
            stage = 1; arguments(s); parent.call(time_runtime_entries[index], parent_function + 0x40); ++parent_after;
            parent.store(marker, 1, parent_function + 0x44); parent.pop(112, parent_function + 0x48);
            parent.restore_link(); parent.finish(); return true;
        }
        ++callbacks;
        if (scenario == Scenario::recursive && callbacks == 1) {
            MacAbi nested(s, escapes, time_runtime_entries[index]); arguments(s);
            constexpr auto descriptor = fixture + 0xc00;
            s.memory.write<4>(descriptor, time_runtime_entries[index]); s.memory.write<4>(descriptor + 4, relocated_toc);
            s.wr<4>(48, descriptor); nested.call(0x10781320, parent_function + 0x80);
            ++callback_after; throw std::runtime_error("Recursive time-runtime returned to abandoned clock import");
        }
        stage = 2; s.wr<4>(12, scenario == Scenario::restore_fault ? fixture + 0xffe : saved_context);
        s.wr<4>(16, 7); invoke_register_context(s, 0x10773f30); ++callback_after;
        throw std::runtime_error("Scalar restore returned to abandoned clock import");
    }
};
void compare(unsigned index, Scenario kind, bool turbo, unsigned mode) {
    auto a = fixture_state(index), b = clone(*a); a->pc = b->pc = parent_import;
    // Parent and time-runtime frames, including nested calls, fit this bounded fixture.
    a->time_runtime_original = true; a->register_context_original = true;
    if (mode == 0) b->register_context_original = true;
    if (mode == 2) b->register_context_entry_original[28] = true;
    if (mode == 3) b->register_context_entry_original[29] = true;
    Monitor x, y; x.effects.state = a.get(); y.effects.state = b.get(); Model old(kind, index), modern(kind, index);
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    a->code_fragment_observer = &old; b->code_fragment_observer = &modern;
    const auto execute = [](State& s) {
        try { run_until(s, caller); }
        catch (const GuestContextTransfer&) { return std::string("Escaped time-runtime transfer owner"); }
        catch (const std::exception& error) { return std::string(error.what()); }
        return std::string{};
    };
    set_match_turbo_enabled(false); const auto ax = execute(*a);
    set_match_turbo_enabled(turbo); const auto bx = execute(*b);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    a->code_fragment_observer = b->code_fragment_observer = nullptr;
    const auto label = "Time-runtime nonlocal scenario=" + std::to_string(unsigned(kind)) + "/mode=" + std::to_string(mode);
    require(ax == bx, label + " outcomes original=" + ax + " modern=" + bx);
    const bool fault = kind == Scenario::restore_fault || kind == Scenario::handler_fault;
    require(!ax.empty() == fault, label + " expected completion " + ax);
    if (kind == Scenario::restore_fault) require(ax.starts_with("Unmapped data access at "), label + " restore fault prefix");
    if (kind == Scenario::handler_fault) require(ax == "Fixture time-runtime resumed-handler fault", label + " handler fault");
    Image(*a).compare(*b, label);
    require(x.effects.bytes == y.effects.bytes && old.boundaries == modern.boundaries, label + " ordered memory/import boundaries");
    require(!old.parent_after && !modern.parent_after && !old.callback_after && !modern.callback_after &&
        b->memory.read<4>(marker) == 0xa7a7a7a7, label + " abandoned time-runtime/callback epilogues never run");
    const unsigned roots = kind == Scenario::recursive ? 2 : 1;
    require(b->time_runtime_calls[index] == roots && !b->time_runtime_returns[index] &&
        b->time_runtime_transfers[index] == (kind == Scenario::restore_fault ? 0u : roots) &&
        b->time_runtime_faults[index] == (kind == Scenario::restore_fault ? roots : 0u) &&
        !b->time_runtime_original_calls[index] && !b->time_runtime_budget_resumes[index] && !b->time_runtime_original_body &&
        !b->time_runtime_dispatch_escapes && !a->register_context_unknown_transfers && !b->register_context_unknown_transfers,
        label + " dedicated modern time-runtime and known scalar ownership");
    require(old.callbacks == modern.callbacks && modern.callbacks == roots && old.handlers == modern.handlers &&
        modern.handlers == unsigned(kind != Scenario::restore_fault), label + " exact surviving handler and callback counts");
    ++cases; fault_cases += fault;
}
}
int main() {
    try {
        require(!match_pc_profile_enabled(), "Time-runtime transfer acceptance requires profiling disabled");
        for (unsigned index : {4u, 5u, 6u, 7u})
        for (auto kind : {Scenario::jump, Scenario::recursive, Scenario::restore_fault, Scenario::handler_fault})
            for (bool turbo : {false, true}) for (unsigned mode = 0; mode < 4; ++mode) compare(index, kind, turbo, mode);
        std::cout << cases << " original/modern/mixed nonlocal time-runtime comparisons; " << fault_cases
            << " restore/handler faults; nested clock calls abandon interrupted callbacks and frames\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
