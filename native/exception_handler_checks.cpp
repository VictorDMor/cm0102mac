#include "exception_search_fixtures.hpp"
#include "runtime_lifecycle_adapter.hpp"
#include "cpp_object_storage_adapter.hpp"
#include "application_exit.hpp"
#include <set>

using namespace cm::semantic_io_checks;
using namespace cm::exception_checks;
namespace cm::aot { bool exception_full_reference(State&); }
namespace {
constexpr std::array<std::uint32_t, 10> entries{0x107724c0, 0x107725a0, 0x10772600, 0x10772d80,
    0x10772dd0, 0x10772de0, 0x10772e70, 0x10772e90, 0x10772eb0, 0x10772ee0};
constexpr std::array<Operation, 10> operations{exception_runtime::handle_unexpected,
    exception_runtime::destroy_derived_exception, exception_runtime::destroy_base_exception,
    exception_runtime::cleanup_exception_object, exception_runtime::derived_exception_type,
    exception_runtime::base_exception_type, exception_runtime::abort_facade,
    exception_runtime::termination_facade, exception_runtime::invoke_termination, exception_runtime::invoke_unexpected};
constexpr auto record = fixture + 0x9000, specification = fixture + 0x9100,
    allowed = fixture + 0xa100, bad_type = fixture + 0xa200, callbacks = fixture + 0xe000,
    callback_toc = fixture + 0xe100, derived_table = 0x11223344u, base_table = 0x55667788u;
unsigned comparisons = 0, faults = 0, negatives = 0, terminals = 0;
struct Model final : RuntimeLifecycleBoundaryObserver {
    struct Import { std::string name; decltype(State::registers) registers; std::uint32_t pc;
        std::uint64_t calls; bool operator==(const Import&) const = default; };
    bool returning = true;
    std::vector<Import> imports;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State&, std::uint32_t, bool, bool) override {}
    void import_enter(const State& s, const char* name) override { imports.push_back({name, s.registers, s.pc, s.native_calls}); }
    void import_leave(const State& s, const char* name) override { import_enter(s, name); }
    bool import_fixture(State& s, const char* name) override {
        if (std::string_view(name) != "CarbonLib::Gestalt" &&
            !(returning && std::string_view(name) == "CarbonLib::ExitToShell")) return false;
        s.wr<4>(12, 0x12345678); s.pc = std::uint32_t(s.r<4>(4128)); return true;
    }
};
struct Outcome { bool terminal = false; std::string error; bool operator==(const Outcome&) const = default; };
void reference(State& s, std::uint32_t stop) {
    while (s.pc != stop) if (!cm::aot::exception_full_reference(s)) run_until(s, std::uint32_t(s.r<4>(4128)));
}
template<class F> Outcome outcome(F&& f) {
    try { f(); } catch (const ApplicationExit&) { return {true, {}}; }
    catch (const std::exception& e) { return {false, e.what()}; } return {};
}
Effects check(std::unique_ptr<State> a, Operation operation, const std::string& label,
        bool fault = false, bool negative = false, bool returning = true, bool terminal = false,
        std::uint32_t stop = return_sentinel, const Expect& expect = {}, const Mutation& mutation = {}) {
    auto b = clone(*a); Effects ea, eb; ea.state = a.get(); eb.state = b.get(); ea.on_read = eb.on_read = mutation;
    Model x, y; x.returning = y.returning = returning;
    a->runtime_lifecycle_observer = &x; b->runtime_lifecycle_observer = &y;
    a->memory.date_observer = &ea; b->memory.date_observer = &eb;
    const auto ax = outcome([&] { reference(*a, stop); }), bx = outcome([&] { operation(*b); });
    a->runtime_lifecycle_observer = b->runtime_lifecycle_observer = nullptr;
    a->memory.date_observer = b->memory.date_observer = nullptr;
    require(ax == bx, label + " original=" + ax.error + " semantic=" + bx.error);
    require(fault == !ax.error.empty() && terminal == ax.terminal, label + " expected outcome=" + ax.error);
    if (fault) require(ax.error.starts_with("Unmapped data access at "), label + " mapping fault");
    Image image(*a); image.compare(*b, label); require(ea.bytes == eb.bytes && ea.callbacks == eb.callbacks && x.imports == y.imports,
        label + " ordered access/callback/import prefix");
    if (!fault && expect) expect(*b, eb);
    if (negative) {
        b->wr<4>(12, b->r<4>(12) ^ 1); bool rejected = false;
        try { image.compare(*b, label + " corrupted result"); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " own negative accepted"); ++negatives;
    }
    ++comparisons; faults += fault; terminals += terminal; eb.state = nullptr; eb.on_read = {}; return eb;
}
std::unique_ptr<State> initial(const char* program, unsigned index, std::uint32_t deleting = 0) {
    auto f = setup(program); auto s = std::move(f.state); std::uint32_t address = object;
    if ((index == 1 || index == 2) && std::int16_t(deleting) > 0) {
        const auto registers = s->registers; const auto unique = s->unique;
        s->wr<4>(12, 128); s->wr<4>(4128, return_sentinel); s->pc = 0x10775130; s->budget = 100000; run(*s);
        address = std::uint32_t(s->r<4>(12)); require(address, "Real guest allocation for deleting exception destructor");
        s->registers = registers; s->unique = unique; s->memory.copy_in(stack - 8192, std::vector<std::uint8_t>(8256, 0xa7));
    }
    for (unsigned n = 0; n < 4; ++n) {
        s->memory.write<4>(callbacks + n * 8, 280984548); s->memory.write<4>(callbacks + n * 8 + 4, callback_toc + n * 0x100);
    }
    for (auto [offset, value] : {std::pair{0x5abcu, callbacks + 24}, {0x3b94u, bad_type}, {0x5ab8u, derived_table},
            {0x5ab4u, base_table}, {0x3b90u, allowed + 0x20}, {0x3b98u, allowed + 0x40},
            {0x3ba0u, bad_type}, {0x3ba4u, actual}, {0x3520u, callbacks}, {0x351cu, callbacks + 8}})
        s->memory.write<4>(toc - offset, value);
    text(*s, bad_type, "!Bad!0!"); text(*s, allowed + 0x20, "l"); text(*s, allowed + 0x40, "q");
    s->memory.write<4>(record, address); s->memory.write<4>(record + 4, actual); s->memory.write<4>(record + 8, callbacks + 16);
    s->memory.write<4>(record + 0x14, specification); s->memory.write<2>(specification + 2, 0);
    constexpr auto lifecycle = fixture + 0xb000;
    for (auto [offset, value] : {std::pair{0x5a7cu, lifecycle}, {0x5a80u, lifecycle + 4}, {0x5a84u, lifecycle + 8},
            {0x3b78u, lifecycle + 0x100}, {0x3b3cu, lifecycle + 0x200}, {0x5ab0u, lifecycle + 12}})
        s->memory.write<4>(toc - offset, value);
    s->memory.write<4>(toc - 0x3514, 0);
    for (unsigned n = 0; n < 4; ++n) s->memory.write<4>(lifecycle + n * 4, 0);
    for (unsigned n = 0; n < 6; ++n) s->memory.write<4>(lifecycle + 0x200 + n * 4, 1);
    s->wr<4>(12, index == 0 || index == 3 ? record : address); s->wr<4>(16, deleting);
    s->wr<4>(4128, return_sentinel); s->pc = entries[index]; s->budget = 100000; return s;
}
void direct_checks(const char* program) {
    for (unsigned i = 0; i < entries.size(); ++i) {
        auto a = initial(program, i); auto inventory = clone(*a);
        const auto effects = check(std::move(a), operations[i], "Direct exception handler " + hex_address(entries[i]), false, true);
        std::set<std::uint32_t> omitted;
        for (const auto& access : effects.log) if (access.size) {
            omitted.insert(access.address); omitted.insert(access.address + std::uint32_t(access.size) - 1);
        }
        for (auto address : omitted) check(without_byte(*inventory, address), operations[i],
            "Handler fault entry=" + hex_address(entries[i]) + " byte=" + hex_address(address), true);
    }
    check(initial(program, 6), operations[6], "Abort facade propagates native application exit", false, false, false, true);
    for (unsigned i : {1u, 2u}) for (auto flag : {0u, 1u, 0x7fffu, 0x8000u, 0xffffu, 0x10000u, 0x10001u}) {
        auto a = initial(program, i, flag); const auto address = std::uint32_t(a->r<4>(12));
        check(std::move(a), operations[i], "Exception destructor signed flag entry=" + hex_address(entries[i]) + " flag=" + hex_address(flag),
            false, false, true, false, return_sentinel, [=](const State& state, const Effects& effects) {
                require(state.r<4>(12) == address, "Exception destruction returns original object address");
                std::vector<std::uint64_t> stores;
                for (const auto& access : effects.log) if (access.kind == 2 && access.address == address && access.size == 4) stores.push_back(access.value);
                if (i == 1) require(stores.size() >= 2 && stores[0] == derived_table && stores[1] == base_table,
                    "Derived then base vtable publication precedes any allocator reuse");
                else require(!stores.empty() && stores[0] == base_table, "Base vtable publication precedes any allocator reuse");
                if (std::int16_t(flag) <= 0) require(state.memory.read<4>(address) == base_table, "Nondeleting destruction retains base vtable");
                require(state.cpp_object_storage_calls[6] == unsigned(std::int16_t(flag) > 0), "Signed-halfword deleting flag selects actual object release");
            });
    }
    for (unsigned i : {1u, 2u}) {
        auto a = initial(program, i, 1); a->wr<4>(12, 0);
        check(std::move(a), operations[i], "Null exception destructor " + hex_address(entries[i]), false, false, true, false, return_sentinel,
            [](const State& state, const Effects&) { require(!state.r<4>(12) && !state.cpp_object_storage_calls[6], "Null object skips deleting destructor"); });
    }
    auto live = initial(program, 1);
    check(std::move(live), operations[1], "Derived destructor retains live condition across first vtable load", false, false, true, false, return_sentinel,
        [](const State& state, const Effects& effects) { require(effects.mutated && state.memory.read<4>(object) == derived_table, "Live EQ skips second vtable publication"); },
        [](State& state, Effects& effects, std::uint32_t address, unsigned) {
            if (address == toc - 0x5ab8 && !effects.mutated) { effects.mutated = true; state.wr<1>(2304, state.r<1>(2304) | 2); }
        });
    for (bool absent_object : {false, true}) {
        auto a = initial(program, 3); a->memory.write<4>(record + (absent_object ? 0 : 8), 0);
        check(std::move(a), operations[3], "Absent exception cleanup object/callback", false, false, true, false, return_sentinel,
            [](const State&, const Effects& effects) { require(effects.callbacks.empty(), "Absent object or callback has no CFM dispatch"); });
    }
    auto null_record = initial(program, 3); null_record->wr<4>(12, 0);
    check(std::move(null_record), operations[3], "Null cleanup record retains original mapping fault", true);
    auto alias = initial(program, 1); alias->wr<4>(12, toc - 0x5ab4);
    check(std::move(alias), operations[1], "Derived object aliases live base-vtable slot", false, false, true, false, return_sentinel,
        [](const State& state, const Effects&) { require(state.memory.read<4>(toc - 0x5ab4) == derived_table,
            "First object store changes the following live base-vtable load"); });
    for (unsigned i : {4u, 5u}) {
        auto a = initial(program, i); constexpr auto relocated = fixture + 0xf000;
        a->wr<4>(8, relocated); a->memory.write<4>(relocated - (i == 4 ? 0x3ba0 : 0x3ba4), 0xdead0000 + i);
        check(std::move(a), operations[i], "Relocated live exception type slot", false, false, true, false, return_sentinel,
            [i](const State& state, const Effects&) { require(state.r<4>(12) == 0xdead0000 + i, "Type metadata is read from the live TOC"); });
    }
    for (unsigned i : {8u, 9u}) {
        auto a = initial(program, i); const auto slot = toc - (i == 8 ? 0x3520 : 0x351c);
        check(std::move(a), operations[i], "Live handler descriptor replacement", false, false, true, false, return_sentinel,
            [](const State& state, const Effects& effects) { require(effects.mutated && effects.callbacks.size() == 1 &&
                effects.callbacks[0][3] == callbacks + 16 && state.r<4>(8) == toc, "Replacement CFM descriptor executes and caller TOC is restored"); },
            [slot](State& state, Effects& effects, std::uint32_t address, unsigned) {
                if (!effects.mutated && address == slot) { effects.mutated = true; state.memory.write<4>(slot, callbacks + 16); }
            });
    }
}
std::unique_ptr<State> landing_initial(const char* program, unsigned branch) {
    auto a = initial(program, 0); constexpr auto current = stack - 112;
    a->memory.write<4>(node + 12, base + 0x900000); a->memory.write<4>(node + 4, ranges + 36);
    a->memory.write<4>(ranges + 12, 0x7724c0); a->memory.write<4>(ranges + 16, 0xdc); a->memory.write<4>(ranges + 20, 0x300);
    a->memory.write<4>(ranges + 24, 0x772ad0); a->memory.write<4>(ranges + 28, 0x34); a->memory.write<4>(ranges + 32, 0x400);
    const auto own = descriptors + 0x300;
    a->memory.write<2>(own, 6u << 11); a->memory.write<2>(own + 2, 4); a->memory.write<2>(own + 4, 0xd8);
    a->memory.write<2>(own + 6, 0x80); a->memory.write<1>(own + 0x80, 0x8d); a->memory.write<2>(own + 0x82, 0x40);
    a->memory.write<4>(current, stack); a->memory.write<4>(current + 0x54, current);
    a->memory.write<4>(stack, stack + 128); a->memory.write<4>(stack + 8, base + 0x120);
    for (unsigned n = 26; n < 32; ++n) a->memory.write<4>(stack - (32 - n) * 4, 0xcafe0000 + n);
    a->wr<4>(4, current); a->wr<4>(104, specification); a->wr<4>(108, callbacks + 24);
    a->wr<4>(112, bad_type); a->wr<4>(116, derived_table); a->wr<4>(120, base_table); a->wr<4>(124, current);
    a->memory.write<4>(current + 0x40, object); a->memory.write<4>(current + 0x44, actual);
    a->memory.write<4>(current + 0x48, callbacks + 16);
    a->memory.write<2>(specification + 2, 1); a->memory.write<4>(specification + 12, allowed);
    text(*a, allowed, branch == 1 ? "i" : branch == 2 ? "l" : branch == 3 ? "q" : "x");
    if (branch >= 2) text(*a, expected, "!Bad!");
    a->pc = 0x107724f4; return a;
}
void landing_checks(const char* program) {
    for (unsigned branch = 0; branch < 4; ++branch) {
        const auto stop = branch ? base + 0x300 : base + 0x120;
        auto a = landing_initial(program, branch); auto inventory = clone(*a);
        const auto effects = check(std::move(a), exception_runtime::resume_unexpected, "Unexpected landing branch=" + std::to_string(branch),
            false, branch == 0, true, false, stop, [=](const State& state, const Effects& effects) {
                require(state.pc == stop, "Unexpected landing destination");
                if (!branch) require(effects.callbacks.size() == 2 && state.r<4>(4) == stack,
                    "Rejected landing cleans original object, invokes termination and returns through original frame");
                else {
                    const auto thrown = branch == 1 ? object : stack - 112 + (branch == 2 ? 0x3c : 0x38);
                    require(state.memory.read<4>(stack + 0x40) == thrown, "Unexpected landing rethrows or publishes bad-exception object");
                    if (branch >= 2) require(state.memory.read<4>(thrown) == derived_table, "Bad-exception publication retains final derived vtable");
                }
            });
        std::set<std::uint32_t> omitted;
        for (const auto& access : effects.log) if (access.size) {
            omitted.insert(access.address); omitted.insert(access.address + std::uint32_t(access.size) - 1);
        }
        for (auto address : omitted) check(without_byte(*inventory, address), exception_runtime::resume_unexpected,
            "Unexpected landing fault branch=" + std::to_string(branch) + " byte=" + hex_address(address), true,
            false, true, false, stop);
    }
}
void child_negatives(const char* program) {
    auto a = initial(program, 7), b = clone(*a); std::unique_ptr<Image> child;
    Effects ea, eb; ea.state = a.get(); eb.state = b.get(); Model x, y;
    a->runtime_lifecycle_observer = &x; b->runtime_lifecycle_observer = &y;
    ea.on_read = [&](State& state, Effects&, std::uint32_t, unsigned) {
        if (state.pc == 0x10772ea0) child = std::make_unique<Image>(state);
    };
    a->memory.date_observer = &ea; reference(*a, return_sentinel); a->memory.date_observer = nullptr;
    require(bool(child), "Original termination child reaches facade epilogue"); bool reached = false;
    eb.on_read = [&](State& state, Effects&, std::uint32_t, unsigned) {
        if (state.pc == 0x10772ea0) { reached = true; child->compare(state, "Unmutated termination child");
            state.wr<4>(12, state.r<4>(12) ^ 1); child->compare(state, "Corrupted termination child"); }
    };
    b->memory.date_observer = &eb; const auto result = outcome([&] { operations[7](*b); });
    b->memory.date_observer = nullptr; a->runtime_lifecycle_observer = b->runtime_lifecycle_observer = nullptr;
    require(reached && result.error.starts_with("Corrupted termination child") && b->pc == 0x10772ea0 && b->r<4>(4) == stack - 64,
        "Termination child is rejected before facade consumes the return word"); ++negatives;
    struct ReleaseBoundary final : CppObjectStorageBoundaryObserver {
        std::unique_ptr<Image> original; bool mutated = false;
        void enter(const State&, std::uint32_t, bool) override {}
        void leave(State& state, std::uint32_t entry, bool baseline, bool terminal) override {
            if (entry != 0x107735a0) return; require(!terminal, "Deleting destructor release child returns");
            if (baseline) original = std::make_unique<Image>(state);
            else { require(bool(original), "Original deleting-destructor release boundary"); original->compare(state, "Unmutated release child");
                state.wr<4>(12, state.r<4>(12) ^ 1); mutated = true; original->compare(state, "Corrupted release child"); }
        }
    } observer;
    a = initial(program, 1, 1); b = clone(*a); a->cpp_object_storage_original = true;
    a->cpp_object_storage_observer = &observer; b->cpp_object_storage_observer = &observer;
    a->runtime_lifecycle_observer = &x; b->runtime_lifecycle_observer = &y;
    reference(*a, return_sentinel); const auto release = outcome([&] { operations[1](*b); });
    a->cpp_object_storage_observer = b->cpp_object_storage_observer = nullptr;
    a->runtime_lifecycle_observer = b->runtime_lifecycle_observer = nullptr;
    require(observer.mutated && release.error.starts_with("Corrupted release child") && b->pc == 0x107725dc && b->r<4>(4) == stack - 64,
        "Release child is rejected before destructor restores the caller"); ++negatives;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-exception-handler-checks <program>"); direct_checks(argv[1]); landing_checks(argv[1]); child_negatives(argv[1]);
        require(negatives == 13, "Every remaining root, unexpected landing and release/termination children need discriminating negatives");
        std::cout << comparisons << " original/C++ exception-handler comparisons (" << faults << " mapping faults, " << terminals
                  << " native exits), " << negatives << " corruption negatives; object/callback/unexpected paths passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
