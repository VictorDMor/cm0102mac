#include "exception_runtime_context.hpp"
#include "code_fragment_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <set>

using namespace cm::semantic_io_checks;
namespace cm::aot { bool exception_context_reference(State&); }
namespace {
constexpr std::uint32_t context = fixture + 0x2000, descriptor = fixture + 0x4000,
    action_state = fixture + 0x5000, action = fixture + 0x6000, table = fixture + 0x7000,
    callback = fixture + 0x8000, frame = fixture + 0x9000, object = fixture + 0xa000;
constexpr std::array<std::uint32_t, 28> targets{0x10772404, 0x10771bc4, 0x10771bd8, 0x10771c30,
    0x10771cf8, 0x10771d90, 0x10771e40, 0x10771ee8, 0x10771f90, 0x107720a8, 0x107721a8,
    0x10772238, 0x10772338, 0x1077234c, 0x10772404, 0x107723e4, 0x10772404, 0x10771c04,
    0x10771c94, 0x10771d44, 0x10771de8, 0x10771e94, 0x10771f3c, 0x1077201c, 0x10772128,
    0x107721f0, 0x107722b8, 0x10772398};
unsigned comparisons = 0, faults = 0, negatives = 0, callback_witnesses = 0;
using Mutation = decltype(Effects::on_read);
using Expect = std::function<void(const State&, const Effects&)>;
struct Fixture {
    std::unique_ptr<State> state;
    std::uint32_t stop;
    std::vector<std::uint32_t> arguments;
    std::optional<std::uint32_t> argument4;
    bool deferred = false;
};
std::unique_ptr<State> setup(const char* program, unsigned kind, std::uint32_t live_toc) {
    auto s = std::make_unique<State>(); load_program(*s, program);
    s->memory.map(fixture, 0x10000); s->memory.clear(fixture, 0x10000);
    s->memory.copy_in(stack - 8192, std::vector<std::uint8_t>(8256, 0xa7));
    s->registers.fill(0x5b); s->unique.fill(0x6d);
    for (unsigned n = 0; n < 32; ++n) s->wr<4>(n * 4, 0xcafe0000 + n);
    s->wr<4>(4, stack); s->wr<4>(8, live_toc); s->wr<4>(4128, return_sentinel);
    s->wr<4>(4132, 0x31415926); s->wr<1>(2304, kind % 2 ? 0xf9 : 0xa6);
    s->wr<1>(1024, kind % 4); s->wr<1>(1027, (kind / 2) % 2); s->budget = 10000;
    s->memory.write<4>(live_toc - 0x3b8c, table);
    s->memory.write<4>(live_toc - 0x3520, callback);
    for (unsigned i = 0; i < targets.size(); ++i) s->memory.write<4>(table + i * 4, targets[i]);
    s->memory.write<4>(callback, 0x10773700); s->memory.write<4>(callback + 4, fixture + 0xf100);
    s->memory.write<4>(context + 0x3a0, frame); s->memory.write<4>(context + 0x3b0, object);
    s->memory.write<4>(context + 0x3b4, 0xaabbccdd); s->memory.write<4>(action_state, descriptor);
    s->memory.write<4>(action_state + 8, action); s->memory.write<1>(action, kind);
    s->wr<4>(12, context); s->wr<4>(16, action_state); s->pc = 0x10771b30; return s;
}
Fixture make_fixture(const char* program, unsigned kind, unsigned flags = 0, std::uint32_t count = 3,
        unsigned condition = 1, unsigned active = 0, bool negative_offset = false,
        std::uint32_t live_toc = toc) {
    Fixture f{setup(program, kind, live_toc), 0, {}, {}, false}; auto& s = *f.state;
    const bool wide = kind >= 17;
    const unsigned family = kind == 27 ? 13 : wide ? kind - 15 : kind;
    const auto base_offset = negative_offset ? 0xfffffff0u : 0x40u;
    const auto pointer_offset = flags & 0x80 ? 8u : 0x20u;
    const auto condition_offset = flags & 0x80 ? 7u : 0x10u;
    const auto second_pointer_offset = flags & 0x40 ? 8u : 0x20u;
    const auto offset = [&](unsigned field, std::uint32_t value) {
        if (wide) s.memory.write<4>(action + field, value); else s.memory.write<2>(action + field, value);
    };
    const auto vector = [&](unsigned field) { s.memory.write<4>(action + field, callback); };
    s.memory.write<1>(action + 1, flags);
    s.memory.write<4>(frame + 0x20, object); s.memory.write<4>(context + 0x318 + 8 * 4, object);
    s.memory.write<4>(context + 0x318 + 7 * 4, 0x11bb0000 | condition);
    if (family == 8) s.memory.write<2>(frame + 0x10, condition);
    else s.memory.write<1>(frame + 0x10, condition);
    unsigned size = 0;
    switch (family) {
        case 1:
            offset(2, 0x88); f.stop = descriptor + 0x88; break;
        case 2:
            offset(2, base_offset); vector(wide ? 6 : 4); size = wide ? 10 : 8;
            f.arguments = {frame + base_offset}; f.argument4 = 0xffffffff; break;
        case 3:
            offset(2, condition_offset); offset(wide ? 6 : 4, base_offset); vector(wide ? 10 : 6); size = wide ? 14 : 10;
            if (std::uint8_t(condition)) f.arguments = {frame + base_offset}; f.argument4 = 0xffffffff; break;
        case 4:
            offset(2, pointer_offset); vector(wide ? 6 : 4); size = wide ? 10 : 8;
            f.arguments = {object}; f.argument4 = 0xffffffff; break;
        case 5:
            offset(2, base_offset);
            if (wide) { s.memory.write<4>(action + 6, count); s.memory.write<4>(action + 10, 8); }
            else { s.memory.write<2>(action + 4, count); s.memory.write<2>(action + 6, 8); }
            vector(wide ? 14 : 8); size = wide ? 18 : 12;
            for (std::uint32_t n = count; std::bit_cast<std::int32_t>(n) > 0; --n) f.arguments.push_back(frame + base_offset + (n - 1) * 8);
            f.argument4 = 0xffffffff; break;
        case 6: case 7:
            offset(2, pointer_offset); s.memory.write<4>(action + (wide ? 6 : 4), 4); vector(wide ? 10 : 8);
            size = wide ? 14 : 12; f.arguments = {object + 4}; f.argument4 = family == 6 ? 0 : 0xffffffff; break;
        case 8:
            offset(2, condition_offset); offset(wide ? 6 : 4, second_pointer_offset);
            s.memory.write<4>(action + (wide ? 10 : 6), 4); vector(wide ? 14 : 10); size = wide ? 18 : 14;
            if (std::uint16_t(condition)) f.arguments = {object + 4}; f.argument4 = 0; break;
        case 9:
            offset(2, pointer_offset); s.memory.write<4>(action + (wide ? 6 : 4), 4);
            s.memory.write<4>(action + (wide ? 10 : 8), count); s.memory.write<4>(action + (wide ? 14 : 12), 8);
            vector(wide ? 18 : 16); size = wide ? 22 : 20;
            for (std::uint32_t n = count; std::bit_cast<std::int32_t>(n) > 0; --n) f.arguments.push_back(object + 4 + (n - 1) * 8);
            f.argument4 = 0xffffffff; break;
        case 10:
            offset(2, pointer_offset); vector(wide ? 6 : 4); size = wide ? 10 : 8;
            f.arguments = {object}; f.argument4 = action_state; break;
        case 11:
            offset(2, condition_offset); offset(wide ? 6 : 4, second_pointer_offset); vector(wide ? 10 : 6); size = wide ? 14 : 10;
            if (std::uint8_t(condition)) f.arguments = {object}; f.argument4 = flags & 0x80 ? 7u * 4 : frame; break;
        case 12: size = 10; break;
        case 13:
            offset(2, base_offset); s.memory.write<4>(frame + base_offset, active == 1 ? object : object + 32);
            s.memory.write<4>(frame + base_offset + 8, active == 2 ? 0 : callback); size = wide ? 6 : 4;
            if (active == 0) f.arguments = {object + 32}; f.argument4 = 0xffffffff; f.deferred = active == 1; break;
        case 15: offset(2, count); size = 12 + count * 4; break;
        default: break;
    }
    if (!f.stop) f.stop = action + (size ? size : 0x80);
    s.memory.write<1>(f.stop, 12); s.wr<4>(20, f.stop); return f;
}
std::unique_ptr<State> without_byte(const State& initial, std::uint32_t omitted) {
    auto s = std::make_unique<State>(); s->registers = initial.registers; s->unique = initial.unique; s->pc = initial.pc;
    s->budget = initial.budget; s->heap_end = initial.heap_end; s->memory_error = initial.memory_error;
    s->allocated = initial.allocated; s->available = initial.available;
    const auto copy = [&](std::uint32_t address, std::uint64_t size) {
        if (!size) return;
        s->memory.map(address, size); s->memory.copy_in(address, initial.memory.copy_out(address, size));
    };
    for (const auto& [address, size] : initial.memory.mapped_ranges()) {
        if (omitted >= address && std::uint64_t(omitted) < std::uint64_t(address) + size) {
            copy(address, omitted - address); const auto remaining = std::uint64_t(address) + size - omitted - 1;
            if (remaining) copy(omitted + 1, remaining);
        } else copy(address, size);
    }
    return s;
}
void reference_until(State& s) {
    while (s.pc != return_sentinel) {
        if (!cm::aot::exception_context_reference(s)) run_until(s, std::uint32_t(s.r<4>(4128)));
    }
}
Effects check(std::unique_ptr<State> original, const std::string& label, const Expect& expected = {},
        const Mutation& mutation = {}, bool fault = false, bool negative = false) {
    auto semantic = clone(*original); Effects a, b; a.state = original.get(); b.state = semantic.get();
    a.on_read = b.on_read = mutation; original->memory.date_observer = &a; semantic->memory.date_observer = &b;
    std::string x, y;
    try { reference_until(*original); } catch (const std::runtime_error& e) { x = e.what(); }
    try { exception_runtime::cleanup_actions(*semantic); } catch (const std::runtime_error& e) { y = e.what(); }
    original->memory.date_observer = semantic->memory.date_observer = nullptr;
    require(x == y, label + " outcome original=" + x + " semantic=" + y);
    require(fault == !x.empty(), label + " fault expectation " + x);
    if (fault) require(x.starts_with("Unmapped data access at "), label + " mapping fault");
    Image image(*original); image.compare(*semantic, label);
    require(a.bytes == b.bytes && a.callbacks == b.callbacks, label + " ordered effects/callback ABI");
    if (expected) expected(*semantic, b);
    if (negative) {
        semantic->wr<4>(0, semantic->r<4>(0) ^ 1); bool rejected = false;
        try { image.compare(*semantic, label + " deliberately corrupted cleanup return"); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " negative accepted"); ++negatives;
    }
    ++comparisons; faults += fault; callback_witnesses += b.callbacks.size(); b.state = nullptr; b.on_read = {}; return b;
}
Expect expectation(const Fixture& f) {
    const auto stop = f.stop; const auto args = f.arguments;
    const auto argument4 = f.argument4; const bool deferred = f.deferred;
    return [=](const State& s, const Effects& effects) {
        require(s.memory.read<4>(action_state + 8) == stop && s.pc == return_sentinel && s.r<4>(4) == stack, "Independent selected cleanup boundary and ABI return");
        require(effects.callbacks.size() == args.size(), "Independent destructor callback count");
        for (unsigned n = 0; n < args.size(); ++n) {
            require(effects.callbacks[n][0] == args[n] && effects.callbacks[n][3] == callback, "Independent callback target/object order");
            if (argument4) require(effects.callbacks[n][1] == *argument4, "Independent destructor callback flag/scratch");
        }
        require(s.memory.read<4>(context + 0x3b4) == (deferred ? callback : 0xaabbccddu), "Active exception callback deferral");
    };
}
void normal_checks(const char* program) {
    for (unsigned kind = 1; kind < targets.size(); ++kind) if (kind != 14 && kind != 16) {
        const auto family = kind == 27 ? 13 : kind >= 17 ? kind - 15 : kind;
        for (unsigned flags : {0u, 0x80u, 0x40u, 0xc0u}) {
            if ((family == 1 || family == 2 || family == 5 || family == 12 || family == 13 || family == 15) && flags) continue;
            auto f = make_fixture(program, kind, flags); const auto expected = expectation(f);
            check(std::move(f.state), "Cleanup kind=" + std::to_string(kind) + " flags=" + std::to_string(flags), expected, {}, false, kind == 27 && !flags);
        }
        if (family == 3 || family == 8 || family == 11) for (unsigned value : {0u, 0x80u, 0x8000u, 0xffu}) {
            for (unsigned flags : {0u, 0xc0u}) {
                auto f = make_fixture(program, kind, flags, 3, value); const auto expected = expectation(f);
                check(std::move(f.state), "Conditional cleanup kind=" + std::to_string(kind) + " value=" + std::to_string(value), expected);
            }
        }
        if (family == 5 || family == 9) for (auto count : {0u, 1u, 0xffffffffu}) {
            if (family == 5 && kind < 17 && count == 0xffffffff) continue;
            auto f = make_fixture(program, kind, 0, count); const auto expected = expectation(f);
            check(std::move(f.state), "Reverse cleanup kind=" + std::to_string(kind) + " count=" + std::to_string(count), expected);
        }
        if (family == 2 || family == 13) {
            auto f = make_fixture(program, kind, 0, 3, 1, 0, true); const auto expected = expectation(f);
            check(std::move(f.state), "Negative cleanup offset kind=" + std::to_string(kind), expected);
        }
        if (family == 13) for (unsigned active : {1u, 2u}) {
            auto f = make_fixture(program, kind, 0, 3, 1, active); const auto expected = expectation(f);
            check(std::move(f.state), "Active exception object kind=" + std::to_string(kind) + " state=" + std::to_string(active), expected);
        }
    }
}
void mutable_checks(const char* program) {
    auto live = make_fixture(program, 17, 0, 3, 1, 0, false, fixture + 0xf000);
    live.state->memory.write<1>(action, 2); live.state->memory.write<4>(table + 8, 0x10771c04);
    const auto expected = expectation(live);
    check(std::move(live.state), "Relocated TOC and live cleanup destination", expected);
    for (unsigned kind : {2u, 5u, 9u, 17u, 20u, 24u}) {
        auto f = make_fixture(program, kind); const auto span = f.stop - action, stop = action + 0x100 + span;
        f.state->memory.write<1>(stop, 12); f.state->wr<4>(20, stop); f.stop = stop;
        const auto expected_live = expectation(f);
        check(std::move(f.state), "Callback changes live action pointer kind=" + std::to_string(kind), expected_live,
            [](State& s, Effects& e, std::uint32_t, unsigned) {
                if (s.pc == 0x10781320 && !e.mutated) { e.mutated = true; s.memory.write<4>(action_state + 8, action + 0x100); }
            });
    }
    for (unsigned kind : {0u, 14u, 16u, 63u}) {
        auto f = make_fixture(program, kind); const auto stop = f.stop;
        check(std::move(f.state), "Returning termination cleanup kind=" + std::to_string(kind),
            [=](const State& s, const Effects& e) {
                require(s.memory.read<4>(action_state + 8) == stop && e.callbacks.size() == 1, "Returning termination callback resumes cleanup at its live boundary");
            }, [=](State& s, Effects& e, std::uint32_t, unsigned) {
                if (s.pc == 0x10781320 && !e.mutated) { e.mutated = true; s.memory.write<4>(action_state + 8, stop); }
            });
    }
    for (unsigned kind : {12u, 15u}) {
        auto f = make_fixture(program, kind); f.state->wr<4>(20, action); f.stop = action;
        const auto stop_expected = expectation(f);
        check(std::move(f.state), "Selected cleanup boundary kind=" + std::to_string(kind), stop_expected);
    }
    for (unsigned kind : {5u, 9u, 20u, 24u}) {
        auto f = make_fixture(program, kind); const auto args = f.arguments; const auto stop = f.stop;
        constexpr auto alternate = callback + 0x20;
        const auto field = kind == 5 ? 8u : kind == 9 ? 16u : kind == 20 ? 14u : 18u;
        f.state->memory.write<4>(alternate, 0x10773700); f.state->memory.write<4>(alternate + 4, fixture + 0xf120);
        check(std::move(f.state), "Array reloads live callback kind=" + std::to_string(kind),
            [=](const State& s, const Effects& e) {
                require(s.memory.read<4>(action_state + 8) == stop && e.callbacks.size() == args.size(), "Live callback array boundary/count");
                for (unsigned n = 0; n < args.size(); ++n)
                    require(e.callbacks[n][0] == args[n] && e.callbacks[n][3] == (n ? alternate : callback), "Reloaded per-element callback and reverse object order");
            }, [=](State& s, Effects& e, std::uint32_t, unsigned) {
                if (s.pc == 0x10781320 && !e.mutated) { e.mutated = true; s.memory.write<4>(action + field, alternate); }
            });
    }
}
Fixture frame_fixture(const char* program, bool indirect_frame, bool initially_empty) {
    auto f = make_fixture(program, 2);
    auto& s = *f.state;
    constexpr auto anchor = frame + 0x200, caller = frame + 0x300, alternate = frame + 0x700,
        node = fixture + 0x1000, ranges = fixture + 0x1800, metadata = descriptor + 0x100, base = 0x10000000u;
    s.memory.copy_in(caller - 256, std::vector<std::uint8_t>(272, 0x39));
    s.memory.write<4>(anchor, caller); s.memory.write<4>(caller - 40, 0xabc00030);
    s.memory.write<4>(caller - 36, alternate); s.memory.write<4>(caller + 4, 0x13579bdf);
    s.memory.write<4>(caller + 8, base + 0x130); s.memory.write<4>(context + 0x39c, anchor);
    s.memory.write<4>(context + 0x394, 0xdecafbad);
    s.memory.write<2>(descriptor, (2u << 11) | (3u << 6) | 4 | 32); s.memory.write<1>(descriptor + 2, (2u << 3) | 4);
    s.memory.write<4>(context + 0x3b4, 0xaabbccdd);
    if (initially_empty) { s.memory.write<4>(action_state + 8, 0); f.arguments.clear(); }
    else s.memory.write<1>(action, 0x82);
    s.memory.write<1>(toc - 0x3529, 1); s.memory.write<1>(toc - 0x352a, 0);
    s.memory.write<4>(toc - 0x3524, node); s.memory.write<4>(toc - 0x3528, 0);
    for (auto [offset, value] : {std::pair{0u, ranges}, {4u, ranges}, {8u, base},
            {12u, base + 0x1000}, {16u, descriptor}, {24u, toc}}) s.memory.write<4>(node + offset, value);
    s.memory.write<4>(ranges, 0x100); s.memory.write<4>(ranges + 4, 0x40); s.memory.write<4>(ranges + 8, 0x100);
    s.memory.write<1>(metadata + 1, indirect_frame ? 16 : 0);
    s.memory.write<2>(metadata + 2, 0x10); s.memory.write<2>(metadata + 4, 0x30);
    s.memory.write<2>(metadata + 6, 0x80); s.memory.write<1>(metadata + 0x80, 12);
    f.stop = metadata + 0x80; s.wr<4>(20, f.stop); return f;
}
struct ChildNegative final : CodeFragmentBoundaryObserver {
    Effects* effects = nullptr;
    std::unique_ptr<Image> boundary;
    std::vector<std::uint8_t> prefix;
    bool rejected = false;
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& s, std::uint32_t entry, bool original) override {
        if (entry != 0x107713e0) return;
        if (original) { boundary = std::make_unique<Image>(s); prefix = effects->bytes; return; }
        require(bool(boundary), "Missing cleanup metadata child boundary");
        boundary->compare(s, "Cleanup metadata child before deliberate corruption");
        require(prefix == effects->bytes, "Cleanup metadata child ordered prefix");
        s.wr<4>(12, s.r<4>(12) ^ 1);
        try { boundary->compare(s, "Corrupted cleanup metadata child return"); }
        catch (const std::runtime_error&) { rejected = true; throw std::runtime_error("Rejected cleanup child before parent resumed"); }
        throw std::runtime_error("Corrupted cleanup child accepted");
    }
};
void child_negative(const State& initial) {
    auto original = clone(initial); auto semantic = clone(initial); original->code_fragment_original = true;
    ChildNegative observer; Effects a, b; a.state = original.get(); b.state = semantic.get();
    observer.effects = &a; original->memory.date_observer = &a; original->code_fragment_observer = &observer;
    reference_until(*original); original->memory.date_observer = nullptr; original->code_fragment_observer = nullptr;
    require(bool(observer.boundary), "Original cleanup metadata child witness");
    observer.effects = &b; semantic->memory.date_observer = &b; semantic->code_fragment_observer = &observer;
    std::string outcome;
    try { exception_runtime::cleanup_actions(*semantic); } catch (const std::runtime_error& e) { outcome = e.what(); }
    semantic->memory.date_observer = nullptr; semantic->code_fragment_observer = nullptr;
    require(observer.rejected && outcome == "Rejected cleanup child before parent resumed" && b.bytes == observer.prefix,
        "Cleanup child negative must reject before the parent publishes an effect: " + outcome); ++negatives;
}
void frame_checks(const char* program) {
    constexpr auto caller = frame + 0x300, alternate = frame + 0x700;
    for (bool indirect : {false, true}) for (bool empty : {false, true}) {
        auto f = frame_fixture(program, indirect, empty); const auto boundary_expected = expectation(f);
        check(std::move(f.state), "Cleanup caller frame indirect=" + std::to_string(indirect) + " initially empty=" + std::to_string(empty),
            [=](const State& s, const Effects& effects) {
                boundary_expected(s, effects);
                require(s.memory.read<4>(context + 0x39c) == caller && s.memory.read<4>(context + 0x394) == alternate,
                    "Cleanup composed C++ register recovery updates live caller and r31");
                require(s.memory.read<4>(context + 0x3a0) == (indirect ? alternate : caller) && s.memory.read<4>(context + 0x398) == 0x13579bdf,
                    "Cleanup selects updated frame and retains recovered CR");
            });
    }
    auto f = frame_fixture(program, true, false);
    for (auto address : {descriptor + 1, descriptor + 2, frame + 0x203, caller - 1, caller - 37,
            caller - 0x81, caller + 7, caller + 11, fixture + 0x1003, descriptor + 0x101})
        check(without_byte(*f.state, address), "Cleanup caller recovery missing byte=" + hex_address(address), {}, {}, true);
    child_negative(*f.state);
}
void fault_checks(const char* program) {
    for (unsigned kind : {2u, 3u, 8u, 9u, 11u, 13u, 17u, 18u, 23u, 24u, 26u, 27u}) {
        auto f = make_fixture(program, kind, 0xc0);
        const auto effects = check(clone(*f.state), "Access witness for cleanup kind=" + std::to_string(kind), expectation(f));
        std::set<std::uint32_t> addresses;
        for (auto access : effects.log) if ((access.kind == 2 || access.kind == 4) && access.size)
            addresses.insert(access.address + std::uint32_t(access.size) - 1);
        for (auto address : addresses) check(without_byte(*f.state, address),
            "Cleanup kind=" + std::to_string(kind) + " missing byte=" + hex_address(address), {}, {}, true);
    }
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-exception-cleanup-checks <program>");
        normal_checks(argv[1]); mutable_checks(argv[1]); frame_checks(argv[1]); fault_checks(argv[1]);
        std::cout << comparisons << " C++ cleanup comparisons (" << faults << " mapping faults), " << negatives
                  << " own/child corruption negatives, " << callback_witnesses << " callback witnesses passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
