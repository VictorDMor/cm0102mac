#include "native_service_adapter.hpp"
#include "import_descriptor.hpp"
#include "code_fragment_adapter.hpp"
#include "register_context_adapter.hpp"
#include "guest_context_transfer.hpp"
#include "semantic_io_checks.hpp"
#include "mac_abi.hpp"
#include "match_turbo.hpp"

namespace {
using namespace cm::semantic_io_checks;
namespace service = cm::aot::native_service;
constexpr std::uint32_t caller = 0x7fff1000, relocated_toc = 0x31008000,
    parent_import = 280984548, parent_function = parent_import - 0x100,
    saved_context = fixture + 0xa00, marker = fixture + 0xb00, descriptor = fixture + 0xc00;
enum class Scenario { jump, recursive, restore_fault, handler_fault };
unsigned cases = 0, fault_cases = 0;
struct Model final : CodeFragmentBoundaryObserver {
    struct Boundary {
        bool leave; std::string name; decltype(State::registers) registers;
        std::uint32_t pc; std::uint64_t calls;
        bool operator==(const Boundary&) const = default;
    };
    Scenario scenario; unsigned index, stage = 0, callbacks = 0, parent_after = 0, callback_after = 0, handlers = 0;
    std::uint32_t parent_frame = 0;
    std::uint64_t context_steps = 0;
    // The already delivered scalar-context core charges one semantic operation
    // rather than each original instruction. Measure that dependency locally;
    // facade/import accounting must still match exactly in every mixed mode.
    struct ContextAccounting {
        State& state; std::uint64_t& steps; std::uint64_t before;
        ContextAccounting(State& s, std::uint64_t& total) : state(s), steps(total), before(s.executed) {}
        ~ContextAccounting() { steps += state.executed - before; }
    };
    std::vector<Boundary> boundaries;
    Model(Scenario kind, unsigned root) : scenario(kind), index(root) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool) override {}
    void import_enter(const State& s, const char* name) override { boundaries.push_back({false, name, s.registers, s.pc, s.native_calls}); }
    void import_leave(const State& s, const char* name) override { boundaries.push_back({true, name, s.registers, s.pc, s.native_calls}); }
    bool import_fixture(State& s, const char* name) override {
        require(std::string_view(name) == "CarbonLib::Gestalt" || std::string_view(name) == "CarbonLib::AEProcessAppleEvent" ||
            std::string_view(name) == "CarbonLib::NewPtr" || std::string_view(name) == "CarbonLib::DisposePtr", "Explicit facade transfer import");
        if (stage == 2) {
            ++handlers; require(s.r<4>(12) == 7 && s.r<4>(4) == parent_frame, "Actual surviving scalar destination");
            if (scenario == Scenario::handler_fault) throw std::runtime_error("Fixture facade resumed-handler fault");
            s.wr<4>(4128, caller); s.pc = caller; return true;
        }
        std::uint64_t escapes = 0;
        if (!stage) {
            s.pc = parent_function; MacAbi parent(s, escapes, parent_function);
            parent.capture_link(); parent.save_link(parent_function + 4); parent.frame(112, parent_function + 8);
            parent_frame = parent.r(1); s.wr<4>(12, saved_context);
            { ContextAccounting measured(s, context_steps); parent.call(0x10773eb0, parent_import); }
            require(s.r<4>(12) == 0, "Actual scalar capture initial zero");
            stage = 1; s.wr<4>(12, 31); parent.call(service::entries[index], parent_function + 0x40); ++parent_after;
            parent.store(marker, 1, parent_function + 0x44); parent.pop(112, parent_function + 0x48);
            parent.restore_link(); parent.finish(); return true;
        }
        ++callbacks;
        if (scenario == Scenario::recursive && callbacks == 1) {
            MacAbi nested(s, escapes, service::entries[index]);
            s.memory.write<4>(descriptor, service::entries[index]); s.memory.write<4>(descriptor + 4, relocated_toc);
            s.wr<4>(48, descriptor); s.wr<4>(12, 31); nested.call(0x10781320, parent_function + 0x80);
            ++callback_after; throw std::runtime_error("Recursive facade returned to abandoned import");
        }
        stage = 2; s.wr<4>(12, scenario == Scenario::restore_fault ? fixture + 0xffe : saved_context);
        s.wr<4>(16, 7);
        { ContextAccounting measured(s, context_steps); invoke_register_context(s, 0x10773f30); }
        ++callback_after;
        throw std::runtime_error("Scalar restore returned to abandoned native-service import");
    }
};
std::unique_ptr<State> initial(unsigned index) {
    auto s = std::make_unique<State>(); s->registers.fill(0x6d); s->unique.fill(0xb3);
    for (auto [address, length] : std::array<std::pair<std::uint32_t, unsigned>, 3>{
            {{fixture, 0x1000}, {stack - 512, 1024}, {relocated_toc - 0x8000, 0x10000}}}) {
        s->memory.map(address, length); s->memory.copy_in(address, std::vector<std::uint8_t>(length, 0xa7));
    }
    s->wr<4>(4, stack); s->wr<4>(8, relocated_toc); s->wr<4>(4128, caller);
    s->pc = parent_import; s->budget = 50000;
    const auto slot = relocated_toc + std::int32_t(import_descriptor::toc_displacements[import_descriptor::entry_index(service::import_glues[index])]);
    constexpr std::array<std::uint32_t, 3> imports{280984676, 280984408, 280984700};
    s->memory.write<4>(slot, fixture + 0x800); s->memory.write<4>(fixture + 0x800, imports[index]);
    s->memory.write<4>(fixture + 0x804, 0x32008000); return s;
}
void compare(unsigned index, Scenario kind, bool turbo, unsigned mode) {
    auto a = initial(index), b = clone(*a); a->native_service_original = true; a->register_context_original = true;
    if (mode == 0) b->register_context_original = true;
    if (mode == 2) b->register_context_entry_original[28] = true;
    if (mode == 3) b->register_context_entry_original[29] = true;
    Effects x, y; x.state = a.get(); y.state = b.get(); Model old(kind, index), modern(kind, index);
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    a->code_fragment_observer = &old; b->code_fragment_observer = &modern;
    const auto execute = [](State& s) {
        try { run_until(s, caller); }
        catch (const GuestContextTransfer&) { return std::string("Escaped facade transfer owner"); }
        catch (const std::exception& e) { return std::string(e.what()); }
        return std::string{};
    };
    set_match_turbo_enabled(false); const auto ax = execute(*a);
    set_match_turbo_enabled(turbo); const auto bx = execute(*b);
    a->memory.date_observer = b->memory.date_observer = nullptr; a->code_fragment_observer = b->code_fragment_observer = nullptr;
    const auto label = hex_address(service::entries[index]) + "/transfer=" + std::to_string(unsigned(kind)) + "/mode=" + std::to_string(mode);
    const bool fault = kind == Scenario::restore_fault || kind == Scenario::handler_fault;
    require(ax == bx && !ax.empty() == fault, label + " outcomes old=" + ax + " modern=" + bx);
    if (kind == Scenario::restore_fault) require(ax.starts_with("Unmapped data access at "), label + " restore fault prefix");
    if (kind == Scenario::handler_fault) require(ax == "Fixture facade resumed-handler fault", label + " handler fault");
    Image(*a).compare(*b, label);
    require(x.bytes == y.bytes, label + " ordered memory effects");
    require(old.boundaries == modern.boundaries, label + " import boundaries");
    if (!mode) require(a->executed == b->executed, label + " complete accounting with identical original scalar dependency");
    require(a->executed - old.context_steps == b->executed - modern.context_steps,
        label + " exact facade/import accounting outside measured scalar-context dependency");
    require(!old.parent_after && !modern.parent_after && !old.callback_after && !modern.callback_after &&
        b->memory.read<4>(marker) == 0xa7a7a7a7, label + " abandoned epilogues never execute");
    const unsigned roots = kind == Scenario::recursive ? 2 : 1;
    require(b->native_service_calls[index] == roots && !b->native_service_returns[index] &&
        b->native_service_transfers[index] == (kind == Scenario::restore_fault ? 0u : roots) &&
        b->native_service_faults[index] == (kind == Scenario::restore_fault ? roots : 0u) &&
        !b->native_service_original_calls[index] && !b->native_service_original_body && !b->native_service_dispatch_escapes &&
        !a->register_context_unknown_transfers && !b->register_context_unknown_transfers, label + " known transfer and dedicated semantic roots");
    require(old.callbacks == modern.callbacks && modern.callbacks == roots && old.handlers == modern.handlers &&
        modern.handlers == unsigned(kind != Scenario::restore_fault), label + " exact callback/surviving-handler counts");
    ++cases; fault_cases += fault;
}
}
int main() {
    try {
        require(!match_pc_profile_enabled(), "Facade transfer acceptance requires profiling disabled");
        // Index 0, the Apple-event facade, is MAIN.CPP's routine now (dispatch
        // reaches main_program's hook there).
        for (unsigned index = 1; index < 3; ++index)
            for (auto kind : {Scenario::jump, Scenario::recursive, Scenario::restore_fault, Scenario::handler_fault})
                for (bool turbo : {false, true}) for (unsigned mode = 0; mode < 4; ++mode) compare(index, kind, turbo, mode);
        std::cout << cases << " exact nonlocal facade comparisons; " << fault_cases << " restore/handler faults; interrupted callbacks and service frames abandoned\n";
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
