// Exercise internal Program calls against the real adapters, checker children
// and generated original bodies. Synthetic records need no game files or UI.
#include "contract_manager_adapter.hpp"
#include "contract_manager_detail.hpp"
#include "shortlist_manager_adapter.hpp"
#include "shortlist_manager_detail.hpp"
#include "routine_check.hpp"
#include <cstdlib>
#include <iostream>
#include <memory>
#include <string_view>

namespace {
using namespace cm::aot;
using guest::Cpu;
constexpr std::uint32_t data = 0x18000000, stack = 0x70008000;
constexpr std::uint32_t back = return_sentinel, expected = 7;
void require(bool ok, const char* message) {
    if (!ok) throw std::runtime_error(message);
}
struct Family {
    const char* name;
    const char* original_variable;
    std::uint32_t entry;
    bool (*direct)(Cpu&, std::uint32_t, std::uint32_t);
    void (*program)(Cpu&, std::uint32_t, std::uint32_t);
    RoutineRunner original, rewrite;
};
const Family families[] = {
    {"contract", "CM_CONTRACT_MANAGER_ORIGINAL", 0x1007f020,
     contract_manager::detail::call_rewritten,
     [](Cpu& c, std::uint32_t entry, std::uint32_t ret) { contract_manager::detail::Program(c).call(entry, ret); },
     contract_manager_run_original, contract_manager_run_rewrite},
    {"shortlist", "CM_SHORTLIST_MANAGER_ORIGINAL", 0x10358ff0,
     shortlist_manager::detail::call_rewritten,
     [](Cpu& c, std::uint32_t entry, std::uint32_t ret) { shortlist_manager::detail::Program(c).call(entry, ret); },
     shortlist_manager_run_original, shortlist_manager_run_rewrite}
};
std::unique_ptr<State> seed(const Family& f) {
    auto s = std::make_unique<State>();
    s->memory.map(data, 0x1000);
    s->memory.map(stack - 0x8000, 0x10000);
    Cpu c(*s);
    for (unsigned n = 0; n < 32; ++n) c.set(n, 0x5a000000 + n);
    c.set({{1, stack}, {2, data + 0x800}, {3, data}, {4, data + 0x100}});
    c.put32(data + 0x114, data + 0x200); // contract: person's attributes
    c.put8(data + 0x245, expected);
    c.put8(data + 0x24, 12);            // shortlist: rank difference
    c.put8(data + 0x124, 5);
    if (f.entry == 0x10082bf0) {
        for (unsigned offset = 0; offset < 12; offset += 4) c.put32(data + 0x100 + offset, 0x12340000 + offset);
    }
    c.set_lr(back);
    s->wr<4>(guest::reg::r2_save, c.r(2));
    s->pc = f.entry;
    return s;
}
struct CheckerScope {
    explicit CheckerScope(RoutineChecker& checker) { require(!routine_checker, "No leaked checker"); routine_checker = &checker; }
    ~CheckerScope() { routine_checker = nullptr; }
};
void returned(const State& s, const Family& f) {
    require(s.pc == back && s.r<4>(guest::reg::lr) == back, "Return PC and LR");
    require(s.r<4>(4) == stack && s.r<4>(8) == data + 0x800, "Stack and TOC preserved");
    require(s.r<4>(guest::reg::r2_save) == data + 0x800, "Call-site TOC saved");
    const auto result = f.entry == 0x10082bf0 ? data : f.entry == 0x10362d70 ? 0u : expected;
    require(s.r<4>(12) == result, "Expected routine result");
    if (f.entry == 0x10082bf0)
        for (unsigned offset = 0; offset < 12; offset += 4)
            require(s.memory.read<4>(data + offset) == 0x12340000 + offset, "Copied guest record bytes");
}
void selected_parent(const Family& f) {
    RoutineChecker checker([&](auto entry) { return entry == f.entry; }, 2, 0);
    CheckerScope scope(checker);
    for (unsigned n = 0; n < 2; ++n) {
        auto s = seed(f); Cpu c(*s);
        f.program(c, f.entry, back);
        returned(*s, f);
        require(s->executed == 0, "Selected parent continues with C++ after checking");
    }
    const auto found = checker.totals().find(f.entry);
    require(found != checker.totals().end(), "Internal Program call must reach the checker");
    require(found->second.calls == 2 && found->second.checked == 2 && !found->second.differences,
            "Each internal call has its own successful original/C++ comparison");
}
void excluded_parent(const Family& f) {
    RoutineChecker checker([](auto) { return false; }, 2, 0);
    CheckerScope scope(checker);
    auto s = seed(f); Cpu c(*s);
    const auto registers = s->registers;
    require(!f.direct(c, f.entry, back), "Excluded direct call must decline");
    require(s->registers == registers && s->pc == f.entry && !s->executed, "Decline must not change caller state");
    f.program(c, f.entry, back);
    returned(*s, f);
    require(s->executed > 0, "Excluded Program call must execute generated original instructions");
    require(checker.totals().empty(), "Excluded routine is not counted as checked");
}
const Family* active_family = nullptr;
bool child_selected = false;
void child_dispatch(State& s, std::uint32_t entry) {
    require(routine_check_child(), "Real checker child required");
    Cpu c(s);
    c.call(entry, back);
}
void child_internal(State& s, std::uint32_t entry) {
    require(routine_check_child(), "Real checker child required");
    Cpu c(s);
    const auto before = s.executed;
    const bool direct = active_family->direct(c, entry, back);
    require(direct == child_selected, "Child must honor selected/excluded internal routing");
    if (!direct) c.call(entry, back);
    require(child_selected ? s.executed == before : s.executed > before, "Child took the expected C++/original body");
}
void child_routes(const Family& f, bool selected) {
    // The artificial caller is selected so check() forks even when the real
    // callee is excluded. Both runners call the same real module entry.
    constexpr std::uint32_t caller = 0x18000f00;
    active_family = &f; child_selected = selected;
    RoutineChecker checker([&](auto entry) { return entry == caller || (selected && entry == f.entry); }, 1, 0);
    CheckerScope scope(checker);
    auto s = seed(f);
    const auto dispatched = [](State& state, std::uint32_t) { child_dispatch(state, active_family->entry); };
    const auto internal = [](State& state, std::uint32_t) { child_internal(state, active_family->entry); };
    require(checker.check(*s, caller, dispatched, internal, RoutineChecker::result_r3), "Child routing comparison ran");
    require(checker.totals().size() == 1 && checker.totals().at(caller).checked == 1,
            "Child does not recursively create parent check totals");
}
void production_and_original(const Family& f, bool original) {
    auto s = seed(f); Cpu c(*s);
    const auto registers = s->registers;
    require(!f.direct(c, 0xdead0000, back), "Unknown entry declines");
    require(s->registers == registers && s->pc == f.entry, "Unknown entry preserves state");
    require(f.direct(c, f.entry, back) != original, "Normal direct path / original override");
    if (original) c.call(f.entry, back);
    returned(*s, f);
    require(original ? s->executed > 0 : s->executed == 0, "Production uses the requested body");
    // Independent real original-versus-rewrite comparison, also under the
    // process-wide original override (run_rewrite is intentionally explicit).
    auto paired = seed(f);
    RoutineChecker checker([&](auto entry) { return entry == f.entry; }, 1, 0);
    require(checker.check(*paired, f.entry, f.original, f.rewrite, RoutineChecker::result_r3), "Real body equivalence");
}
struct Hook : ContractManagerCallHook, ShortlistManagerCallHook {
    unsigned calls = 0;
    bool accept = true, transfer = false;
    bool call(State& s) override {
        ++calls;
        if (transfer) { s.pc = 0x18000e00; throw GuestContextTransfer{&s, nullptr}; }
        if (accept) { s.wr<4>(12, 99); s.pc = std::uint32_t(s.r<4>(guest::reg::lr)); }
        return accept;
    }
    void direct(std::uint32_t) override {}
};
void fixture_hooks(const Family& f) {
    Hook hook;
    struct Restore {
        ~Restore() { contract_manager_call_hook = nullptr; shortlist_manager_call_hook = nullptr; }
    } restore;
    if (f.direct == families[0].direct) contract_manager_call_hook = &hook;
    else shortlist_manager_call_hook = &hook;
    RoutineChecker checker([](auto) { return false; }, 1, 0);
    CheckerScope scope(checker);
    auto s = seed(f); Cpu c(*s);
    require(f.direct(c, f.entry, back) && hook.calls == 1 && c.r(3) == 99 && s->pc == back,
            "Fixture hook retains precedence over checker exclusion");
    hook.accept = false;
    require(!f.direct(c, f.entry, back) && hook.calls == 2, "Fixture refusal goes back to caller");
    hook.transfer = true;
    bool transferred = false;
    try { f.direct(c, f.entry, back); }
    catch (const GuestContextTransfer& t) { transferred = t.state == s.get(); }
    require(transferred && s->pc == 0x18000e00, "Guest transfer propagates without fabricating a return");
    require(checker.totals().empty(), "Fixture hook is not reinterpreted as a checked game routine");
}
void corruption_control(const Family& f) {
    RoutineChecker checker([&](auto entry) { return entry == f.entry; }, 1, 0);
    checker.corrupt_first = true;
    CheckerScope scope(checker);
    auto s = seed(f); Cpu c(*s);
    bool rejected = false;
    try { f.program(c, f.entry, back); }
    catch (const std::runtime_error&) { rejected = checker.differences() == 1; }
    require(rejected, "Internal call must detect deliberate child-state corruption");
}
}
int main(int argc, char** argv) {
    try {
        require(argc <= 2, "Usage: cm-manager-routing-checks [contract|shortlist]");
        unsigned tested = 0;
        for (const auto& family : families) {
            if (argc == 2 && std::string_view(argv[1]) != family.name) continue;
            // Cover both a register-only leaf and a routine that writes a
            // record or stack frame, using the same production Program path.
            for (const auto entry : {family.entry, family.direct == families[0].direct ? 0x10082bf0u : 0x10362d70u}) {
                auto f = family;
                f.entry = entry;
                const bool original = std::getenv(f.original_variable) != nullptr;
                production_and_original(f, original);
                fixture_hooks(f);
                if (!original) {
                    selected_parent(f);
                    excluded_parent(f);
                    child_routes(f, true);
                    child_routes(f, false);
                    corruption_control(f);
                }
                ++tested;
                std::cout << f.name << ' ' << hex_address(entry)
                          << (original ? ": original override passed\n" : ": internal checker routing passed\n");
            }
        }
        require(tested != 0, "Unknown family");
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
