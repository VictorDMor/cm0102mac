#include "match_engine.hpp"
#include "semantic_io_checks.hpp"
#include <set>

namespace cm::aot { bool match_engine_reference(State&); }
namespace {
using namespace cm::semantic_io_checks;
namespace engine = cm::aot::match_engine;
constexpr std::uint32_t caller = 0x7fff1000, relocated_toc = 0x32010000;
constexpr auto manager = fixture, watched = fixture + 0x1000, background = fixture + 0x2000;
constexpr auto commentary = fixture + 0x28000, display = fixture + 0x4000, result = fixture + 0x5000;
constexpr auto match = fixture + 0x10000, cache = fixture + 0x34000, control = fixture + 0x30000;
unsigned comparisons[2]{}, faults[2]{}, negatives[2]{}, budgets = 0;
struct Dependencies final : engine::DependencyDispatch {
    bool original, allocation_failure = false;
    unsigned core_calls = 0, queue_calls = 0;
    std::uint32_t clock = 1000, queue = 0, sound = 1;
    std::vector<std::uint32_t> core_returns{1};
    std::vector<std::pair<std::uint32_t, decltype(State::registers)>> calls;
    std::function<void(State&, std::uint32_t)> before;
    explicit Dependencies(bool source) : original(source) {}
    void reference(State& s) {
        bool initial = true;
        for (;;) {
            if (!initial && (s.pc == engine::entries[1] || s.pc == engine::entries[2])) call(s, s.pc, std::uint32_t(s.r<4>(4128)));
            else if (!cm::aot::match_engine_reference(s)) call(s, s.pc, std::uint32_t(s.r<4>(4128)));
            else if (s.pc == s.r<4>(4128)) return;
            initial = false;
        }
    }
    void call(State& s, std::uint32_t target, std::uint32_t continuation) override {
        require(s.pc == target && s.r<4>(4128) == continuation && s.r<4>(2440) == s.r<4>(8), "Published match dependency ABI");
        calls.emplace_back(target, s.registers);
        if (before) before(s, target);
        if (target == engine::entries[1]) {
            if (original) reference(s); else engine::advance_background(s, this);
            require(s.pc == continuation, "Background advancement returns to its watched caller"); return;
        }
        s.step(target);
        if (target == engine::entries[2]) {
            s.wr<4>(12, core_returns[std::min<std::size_t>(core_calls++, core_returns.size() - 1)]);
        } else if (target == 0x10746a30) s.wr<4>(12, clock);
        else if (target == 0x102036f0) s.wr<4>(12, sound);
        else if (target == 0x10201ef0) { ++queue_calls; s.wr<4>(12, queue); }
        else if (target == 0x101ff180) s.wr<4>(12, 2);
        else if (target == 0x101ce350) s.wr<4>(12, s.r<4>(12) & 3);
        else if (target == 0x10773520) s.wr<4>(12, allocation_failure ? 0 : fixture + 0x41000);
        else if (target == 0x10778300) s.memory.copy_in(s.r<4>(12), std::vector<std::uint8_t>(s.r<4>(20), std::uint8_t(s.r<4>(16))));
        else if (target == 0x10773320) s.memory.write<1>(s.r<4>(12), 0x71);
        else if (target == 0x10739890) { s.memory.write<4>(s.r<4>(24), 0); s.memory.write<4>(s.r<4>(28), 0); }
        else if (target == 0x10768810) s.wr<4>(12, control + 0x800);
        else if (target == 0x1077b720 || target == 0x10769b60 || target == 0x101cd350 ||
                 target == 0x1029e9e0 || target == 0x101d3e50 || target == 0x10430550 || target == 0x1029f920)
            s.memory.write<1>(control + 0x900, std::uint8_t(target));
        else throw std::runtime_error("Uncontrolled match dependency " + hex_address(target));
        s.wr<4>(0, 0x24681357); ++s.native_calls; s.step(target + 4); s.pc = std::uint32_t(s.r<4>(4128));
    }
};
std::unique_ptr<State> initial(unsigned index) {
    auto s = std::make_unique<State>(); s->registers.fill(0x6d); s->unique.fill(0xb3); s->native_calls = 17;
    for (auto [a, n] : std::array<std::pair<std::uint32_t, unsigned>, 3>{{
        {relocated_toc - 0x8000, 0x8000}, {stack - 4096, 8192}, {fixture, 0x50000}}}) {
        s->memory.map(a, n); s->memory.copy_in(a, std::vector<std::uint8_t>(n, 0));
    }
    s->wr<4>(4, stack); s->wr<4>(8, relocated_toc); s->wr<4>(4128, caller); s->memory.write<4>(stack + 8, caller);
    for (auto [offset, pointer] : std::array<std::pair<std::int32_t, std::uint32_t>, 8>{{
        {-0x7bb0,cache}, {-0x7ac8,control}, {-0x7ba8,control+0x100}, {-0x52f0,control+0x200},
        {-0x7acc,control+0x300}, {-0x7b4c,control+0x400}, {-0x7b20,control+0x500}, {-0x7bac,control+0x600}}})
        s->memory.write<4>(relocated_toc + std::uint32_t(offset), pointer);
    s->memory.write<4>(control, control + 0x700); s->memory.write<4>(control + 0x400, control + 0xa00);
    s->memory.write<4>(control + 0x500, control + 0xb00);
    s->memory.write<4>(manager + 0x1c, background); s->memory.write<4>(manager + 0x38, watched);
    s->memory.write<4>(watched + 0x58, match); s->memory.write<4>(watched + 0x5c, display); s->memory.write<4>(watched + 0x60, result);
    s->memory.write<4>(background + 0x58, match); s->memory.write<4>(background + 0x60, result);
    s->memory.write<4>(match + 0x79f4, commentary); s->memory.write<1>(display + 0x54, 1);
    s->wr<4>(12, manager); s->wr<4>(16, index ? 0 : watched); s->pc = engine::entries[index]; s->budget = 10000;
    return s;
}
using Configure = std::function<void(Dependencies&)>;
std::vector<Effects::Access> compare(unsigned index, const State& seed, const std::string& name,
                                   Configure configure = {}, bool negative = false, bool normal = true,
                                   std::function<void(const State&, const Dependencies&)> expectation = {}) {
    auto a = clone(seed), b = clone(seed); Effects x, y; x.state = a.get(); y.state = b.get();
    a->memory.date_observer = &x; b->memory.date_observer = &y;
    Dependencies old(true), modern(false); if (configure) { configure(old); configure(modern); }
    std::string ae, be;
    try { old.reference(*a); } catch (const std::exception& e) { ae = e.what(); }
    try {
        if (index) engine::advance_background(*b, &modern); else engine::advance_watched(*b, &modern);
    } catch (const std::exception& e) { be = e.what(); }
    a->memory.date_observer = b->memory.date_observer = nullptr;
    const auto label = hex_address(engine::entries[index]) + " " + name;
    require(ae == be, label + " original=" + ae + " modern=" + be);
    if (normal) require(ae.empty(), label + " unexpected matching fault=" + ae);
    if (negative) {
        b->wr<4>(12, b->r<4>(12) ^ 1);
        bool rejected = false; try { Image(*a).compare(*b, label); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Own caller negative must be rejected"); ++negatives[index];
    } else {
        Image(*a).compare(*b, label); require(x.bytes == y.bytes && old.calls == modern.calls, label + " ordered effects and dependency registers");
    }
    require(a->executed == b->executed, label + " accounting old=" + std::to_string(a->executed) + " modern=" + std::to_string(b->executed));
    if (normal && expectation) expectation(*b, modern);
    ++comparisons[index]; faults[index] += !ae.empty(); budgets += ae == "Native instruction budget exhausted";
    return x.log;
}
std::unique_ptr<State> minimal(const State& seed, const std::vector<Effects::Access>& accesses, std::uint32_t hole = UINT32_MAX) {
    auto s = std::make_unique<State>(); s->registers = seed.registers; s->unique = seed.unique; s->pc = seed.pc;
    s->budget = seed.budget; s->native_calls = seed.native_calls; s->executed = seed.executed;
    s->heap_end = seed.heap_end; s->allocated = seed.allocated; s->available = seed.available; s->memory_error = seed.memory_error;
    std::set<std::uint32_t> bytes;
    for (const auto& access : accesses) for (std::uint64_t n = 0; n < access.size; ++n) bytes.insert(access.address + std::uint32_t(n));
    for (auto address : bytes) if (address != hole) {
        s->memory.map(address, 1); s->memory.copy_in(address, seed.memory.copy_out(address, 1));
    }
    return s;
}
void parent_child_negative() {
    auto seed = initial(0), a = clone(*seed), b = clone(*seed);
    Effects x, y; x.state = a.get(); y.state = b.get(); a->memory.date_observer = &x; b->memory.date_observer = &y;
    Dependencies old(true), modern(false);
    modern.before = [](State& s, std::uint32_t target) { if (target == engine::entries[2]) s.memory.write<1>(match + 0x7928, 1); };
    old.reference(*a); engine::advance_watched(*b, &modern);
    a->memory.date_observer = b->memory.date_observer = nullptr;
    bool rejected = false; try { Image(*a).compare(*b, "Mutated core child"); } catch (const std::runtime_error&) { rejected = true; }
    require(rejected && x.bytes != y.bytes && modern.core_calls == 1, "Watched parent rejects a changed core child effect");
}
void matrix(unsigned index) {
    auto seed = initial(index);
    const auto variant = [&](const std::string& name, auto change, Configure configure = {}) {
        auto s = clone(*seed); change(*s); compare(index, *s, name, configure);
    };
    compare(index, *seed, "normal", {}, false, true, [](const State& s, const Dependencies& d) {
        require(s.pc == caller && s.r<4>(4) == stack && s.r<4>(8) == relocated_toc && s.r<4>(12) == 0,
            "Independent normal caller ABI and zero pending result");
        require(d.core_calls == 1 && s.memory.read<4>(manager + 0x28) == 0, "One independent shared-engine tick and no completed fixture");
    });
    compare(index, *seed, "own-negative", {}, true);
    for (auto skew : {1u, 3u}) variant("unaligned-stack", [skew](State& s) { s.wr<4>(4, stack + skew); });
    if (!index) {
        variant("no-watched-fixture", [](State& s) { s.wr<4>(16, 0); });
        for (auto acknowledged : {0u, 1u}) variant("acknowledgement", [acknowledged](State& s) {
            s.memory.write<1>(display, 1); s.memory.write<1>(display + 0x4a, 1); s.memory.write<1>(display + 0x28, acknowledged);
        });
        variant("waiting-background", [](State& s) { s.memory.write<1>(display + 0x4b, 1); });
        for (auto sound : {0u, 1u}) variant("sound-ready", [](State& s) { s.memory.write<1>(display, 8); },
            [sound](Dependencies& d) { d.sound = sound; });
        for (auto speed : {0u, 20u}) variant("unsigned-clock-threshold", [speed](State& s) {
            s.memory.write<1>(commentary + 0x4070, speed); s.memory.write<4>(display + 0x50, 500); s.memory.write<4>(display + 0x4c, 600);
        });
        variant("pending-commentary", [](State& s) { s.memory.write<1>(display, 8); },
            [](Dependencies& d) { d.queue = control + 0x800; });
        variant("post-advance-commentary", [](State& s) { s.memory.write<1>(commentary + 5, 1); },
            [](Dependencies& d) { d.queue = control + 0x800; });
        for (auto status : {1u, 2u, 4u, 8u, 0xffu}) variant("core-result", [](State&) {},
            [status](Dependencies& d) { d.core_returns = {status}; });
        for (auto bit : {1u, 0x80u}) variant("live-incident-flags", [bit](State& s) {
            s.memory.write<1>(match + 0xe03d, 1); s.memory.write<1>(match + 0xe073, 1); s.memory.write<1>(match + 0x7928, bit);
        }, [bit](Dependencies& d) { d.before = [bit](State& s, std::uint32_t target) {
            if (target == engine::entries[2]) s.memory.write<1>(display + 0x55, bit);
        }; });
        for (auto minutes : {0u, 11u, 12u, 32767u, 0x8000u, 0xffffu}) variant("signed-time-display", [minutes](State& s) {
            s.memory.write<2>(match + 0x7948, minutes); s.memory.write<2>(result + 0x4e, 0xffff); s.memory.write<2>(result + 0x52, 10);
        });
        for (auto rank : {0u, 1u, 3u}) variant("priority-replacement", [rank](State& s) {
            s.memory.write<4>(manager + 0x38, background); s.memory.write<4>(background + 0x60, result + 0x1000);
            s.memory.write<2>(result + 0x1000 + 0x50, rank); s.memory.write<2>(result + 0x50, 2);
            s.memory.write<1>(match + 0x7928, 1);
        });
    } else {
        for (auto flags : {0u, 0x40000u, 0x80000u, 0xc0000u, 0x100000u})
            for (auto tick : {0u, 1u, 2u, 5u, 99u, 100u, 0x7fffffffu, 0xffffffffu})
                variant("hardware-gates-and-counter", [flags, tick](State& s) {
                    s.memory.write<4>(control + 0x704, flags); s.memory.write<1>(relocated_toc - 0x391a, 1); s.memory.write<4>(relocated_toc - 0x3918, tick);
                });
        variant("negative-fixture-index", [](State& s) { s.memory.write<4>(manager + 0x30, UINT32_MAX); });
        variant("null-fixture", [](State& s) { s.memory.write<4>(background + 0x58, 0); });
        for (auto status : {2u, 8u}) variant("normal-completion", [](State&) {},
            [status](Dependencies& d) { d.core_returns = {status}; });
        for (auto status : {0u, 2u, 8u}) variant("forced-advancement", [status](State& s) {
            s.wr<4>(16, 1); s.memory.write<1>(background + 0x68, status);
        }, [](Dependencies& d) { d.core_returns = {1, 2}; });
        variant("narrow-forced-byte", [](State& s) { s.wr<4>(16, 0x100); s.memory.write<4>(control + 0x704, 0xc0000); });
        variant("allocation-failure-report", [](State&) {}, [](Dependencies& d) { d.core_returns = {2}; d.allocation_failure = true; });
    }
    auto counted = clone(*seed); Dependencies count(true); count.reference(*counted);
    for (std::uint64_t budget = 0; budget < counted->executed; ++budget) {
        auto s = clone(*seed); s->budget = budget; compare(index, *s, "finite-budget", {}, false, false);
    }
    const auto accessed = compare(index, *seed, "normal-accesses");
    std::set<std::uint32_t> bytes;
    for (const auto& access : accessed) for (std::uint64_t n = 0; n < access.size; ++n) bytes.insert(access.address + std::uint32_t(n));
    auto small = minimal(*seed, accessed); compare(index, *small, "minimal-normal");
    for (auto hole : bytes) { auto s = minimal(*seed, accessed, hole); compare(index, *s, "distinct-accessed-byte-hole", {}, false, false); }
    if (index) {
        auto s = clone(*seed); s->wr<4>(16, 1);
        compare(index, *s, "independent-forced-finalization", [](Dependencies& d) { d.core_returns = {1, 2}; }, false, true,
            [](const State& state, const Dependencies& d) {
                require(d.core_calls == 2 && state.r<4>(12) == 1 && state.memory.read<4>(manager + 0x28) == 1 &&
                    state.memory.read<1>(background + 0x68) == 2 && !state.memory.read<4>(background + 0x58) &&
                    state.memory.read<4>(background + 0x60) == fixture + 0x41000,
                    "Forced advancement completes two ticks, publishes a result and detaches exactly one fixture");
            });
    }
    std::cout << hex_address(engine::entries[index]) << " comparisons=" << comparisons[index] << " faults=" << faults[index] << " negatives=" << negatives[index] << '\n';
}
}
int main() {
    try { parent_child_negative(); matrix(0); matrix(1); std::cout << "Caller comparisons=" << comparisons[0] + comparisons[1] << " budget_stops=" << budgets << '\n'; }
    catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
