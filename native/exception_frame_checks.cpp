#include "exception_runtime_context.hpp"
#include "code_fragment_adapter.hpp"
#include "semantic_io_checks.hpp"
#include <functional>

using namespace cm::semantic_io_checks;
namespace cm::aot { bool exception_context_reference(State&); }
namespace {
constexpr std::uint32_t context = fixture + 0x2000, descriptor = fixture + 0x4000,
    action_state = fixture + 0x5000, action = fixture + 0x6000, table = fixture + 0x7000,
    current_frame = fixture + 0x9000, caller = fixture + 0xb000;
unsigned comparisons = 0, faults = 0, negatives = 0;
using Operation = void(*)(State&);
using Mutation = decltype(Effects::on_read);
using Expect = std::function<void(const State&, const Effects&)>;
std::unique_ptr<State> setup(const char* program) {
    auto s = std::make_unique<State>(); load_program(*s, program);
    s->memory.map(fixture, 0x10000); s->memory.clear(fixture, 0x10000);
    s->memory.copy_in(stack - 8192, std::vector<std::uint8_t>(8256, 0xa7));
    s->registers.fill(0x5b); s->unique.fill(0x6d);
    for (unsigned n = 0; n < 32; ++n) s->wr<4>(n * 4, 0xcafe0000 + n);
    s->wr<4>(4, stack); s->wr<4>(8, toc); s->wr<4>(4128, return_sentinel);
    s->wr<4>(4132, 0x31415926); s->budget = 10000; return s;
}
std::unique_ptr<State> without_byte(const State& initial, std::uint32_t omitted) {
    auto s = std::make_unique<State>();
    s->registers = initial.registers; s->unique = initial.unique; s->pc = initial.pc;
    s->budget = initial.budget; s->heap_end = initial.heap_end; s->memory_error = initial.memory_error;
    s->allocated = initial.allocated; s->available = initial.available;
    const auto copy = [&](std::uint32_t address, std::uint64_t size) {
        if (!size) return;
        s->memory.map(address, size); s->memory.copy_in(address, initial.memory.copy_out(address, size));
    };
    for (const auto& [address, size] : initial.memory.mapped_ranges()) {
        if (omitted >= address && std::uint64_t(omitted) < std::uint64_t(address) + size) {
            copy(address, omitted - address);
            const auto remaining = std::uint64_t(address) + size - omitted - 1;
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
struct ChildNegative final : CodeFragmentBoundaryObserver {
    std::uint32_t target;
    Effects* effects = nullptr;
    std::unique_ptr<Image> boundary;
    std::vector<std::uint8_t> prefix;
    bool rejected = false;
    explicit ChildNegative(std::uint32_t entry) : target(entry) {}
    void enter(const State&, std::uint32_t, bool) override {}
    void leave(State& state, std::uint32_t entry, bool original) override {
        if (entry != target) return;
        if (original) {
            if (!boundary) { boundary = std::make_unique<Image>(state); prefix = effects->bytes; }
            return;
        }
        require(bool(boundary), "Missing original child boundary");
        boundary->compare(state, "Child boundary before deliberate corruption");
        require(prefix == effects->bytes, "Child ordered prefix before deliberate corruption");
        state.wr<4>(12, state.r<4>(12) ^ 1);
        try { boundary->compare(state, "Deliberately corrupted child return"); }
        catch (const std::runtime_error&) {
            rejected = true;
            throw std::runtime_error("Rejected corrupted exception-frame child before parent resumed");
        }
        throw std::runtime_error("Corrupted exception-frame child was accepted");
    }
};
void child_negative(const State& initial, std::uint32_t target) {
    auto original = clone(initial); auto semantic = clone(initial); original->code_fragment_original = true;
    ChildNegative observer(target); Effects a, b; a.state = original.get(); b.state = semantic.get();
    original->memory.date_observer = &a; original->code_fragment_observer = &observer; observer.effects = &a;
    reference_until(*original);
    original->memory.date_observer = nullptr; original->code_fragment_observer = nullptr;
    require(bool(observer.boundary), "Original child negative witness");
    observer.effects = &b; semantic->memory.date_observer = &b; semantic->code_fragment_observer = &observer;
    std::string outcome;
    try { exception_runtime::advance_action(*semantic); } catch (const std::runtime_error& e) { outcome = e.what(); }
    semantic->memory.date_observer = nullptr; semantic->code_fragment_observer = nullptr;
    require(observer.rejected && outcome == "Rejected corrupted exception-frame child before parent resumed",
        "Child negative was not rejected before parent resumed: " + outcome);
    require(b.bytes == observer.prefix, "No parent effect after corrupted child"); ++negatives;
}
void check(std::unique_ptr<State> original, Operation operation, const std::string& label,
        const Expect& expected = {}, const Mutation& mutation = {}, bool fault = false, bool negative = false) {
    auto semantic = clone(*original); Effects a, b; a.state = original.get(); b.state = semantic.get();
    a.on_read = b.on_read = mutation;
    original->memory.date_observer = &a; semantic->memory.date_observer = &b;
    std::string x, y;
    try { reference_until(*original); } catch (const std::runtime_error& e) { x = e.what(); }
    try { operation(*semantic); } catch (const std::runtime_error& e) { y = e.what(); }
    original->memory.date_observer = semantic->memory.date_observer = nullptr;
    require(x == y, label + " outcome original=" + x + " semantic=" + y);
    require(fault == !x.empty(), label + " fault expectation " + x);
    if (fault) require(x.starts_with("Unmapped data access at "), label + " mapping fault");
    Image reference(*original); reference.compare(*semantic, label);
    require(a.bytes == b.bytes, label + " ordered access prefix");
    require(a.callbacks == b.callbacks, label + " callback ABI/history");
    if (expected) expected(*semantic, b);
    if (negative) {
        semantic->wr<4>(12, semantic->r<4>(12) ^ 1);
        bool rejected = false;
        try { reference.compare(*semantic, label + " deliberately corrupted return"); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " negative was accepted"); ++negatives;
    }
    ++comparisons; faults += fault;
}
struct SavedSuffixes { unsigned floating, general, vector; bool vectors, padding, condition; };
std::unique_ptr<State> saved_fixture(const char* program, SavedSuffixes n,
        std::uint32_t destination = context, std::uint32_t source = caller) {
    auto s = setup(program);
    std::vector<std::uint8_t> bytes(0x810);
    for (unsigned i = 0; i < bytes.size(); ++i) bytes[i] = std::uint8_t(i * 37 + 19);
    s->memory.copy_in(source - 0x800, bytes);
    s->memory.write<4>(source + 4, 0x9abcdef1); s->memory.write<4>(source + 8, 0x4567abcd);
    s->memory.write<4>(current_frame, source); s->memory.write<4>(action_state, descriptor);
    s->memory.write<2>(descriptor, (n.general << 11) | (n.floating << 6) | (n.vectors ? 4 : 0) | (n.condition ? 32 : 0));
    s->memory.write<1>(descriptor + 2, (n.vector << 3) | (n.padding ? 4 : 0));
    s->memory.write<4>(destination + 0x398, 0xdeadbeef); s->memory.write<4>(destination + 0x39c, current_frame);
    s->wr<4>(12, destination); s->wr<4>(16, action_state); s->pc = 0x107717f0; return s;
}
Expect saved_expectation(const State& before, SavedSuffixes n, std::uint32_t destination, std::uint32_t source) {
    const auto floating_source = source - n.floating * 8;
    const auto general_source = floating_source - (n.floating & 1 ? 8 : 0) - n.general * 4;
    const auto vector_source = (general_source - n.vector * 16 - (n.padding ? 64 : 0)) & ~std::uint32_t(15);
    const auto fp = before.memory.copy_out(floating_source, n.floating * 8);
    const auto gp = before.memory.copy_out(general_source, n.general * 4);
    const auto vp = n.vectors ? before.memory.copy_out(vector_source, n.vector * 16) : std::vector<std::uint8_t>{};
    const auto untouched_vectors = before.memory.copy_out(destination, (n.vectors ? 32 - n.vector : 32) * 16);
    return [=](const State& s, const Effects& effects) {
        require(s.memory.copy_out(destination + 0x218 + (32 - n.floating) * 8, fp.size()) == fp, "Independent floating suffix");
        require(s.memory.copy_out(destination + 0x318 + (32 - n.general) * 4, gp.size()) == gp, "Independent general suffix");
        if (n.vectors) require(s.memory.copy_out(destination + (32 - n.vector) * 16, vp.size()) == vp, "Independent vector suffix");
        require(s.memory.copy_out(destination, untouched_vectors.size()) == untouched_vectors, "Unsaved vector prefix preserved");
        require(s.memory.read<4>(destination + 0x398) == (n.condition ? 0x9abcdef1u : 0xdeadbeefu), "Conditional saved CR");
        require(s.memory.read<4>(destination + 0x39c) == source && s.r<4>(12) == 0x4567abcd, "Live caller and returned LR");
        for (auto access : effects.log) if (access.address >= destination && access.address < destination + 0x200)
            require(access.size == 4, "Saved vector copy retains ordered four-byte operations");
    };
}
void saved_checks(const char* program) {
    for (unsigned phase = 0; phase < 3; ++phase) for (unsigned n = 0; n < 32; ++n) {
        SavedSuffixes counts{phase == 0 ? n : 9, phase == 1 ? n : 10, phase == 2 ? n : 5,
            phase != 0 || n % 3 != 0, n % 2 != 0, n % 2 == 0};
        auto s = saved_fixture(program, counts);
        s->wr<1>(2304, n % 2 ? 0xf9 : 0xa6); s->wr<1>(1024, n % 4); s->wr<1>(1027, (n / 2) % 2);
        const auto expected = saved_expectation(*s, counts, context, caller);
        check(std::move(s), exception_runtime::copy_saved_registers, "Saved suffix phase=" + std::to_string(phase) + " count=" + std::to_string(n), expected,
            {}, false, phase == 0 && n == 31);
    }
    SavedSuffixes full{31, 31, 31, true, true, true};
    auto s = saved_fixture(program, full, context + 3, caller + 5);
    auto expected = saved_expectation(*s, full, context + 3, caller + 5);
    check(std::move(s), exception_runtime::copy_saved_registers, "Unaligned frame and context", expected);
    auto alias = saved_fixture(program, full, caller - 0x300);
    check(std::move(alias), exception_runtime::copy_saved_registers, "Saved source/destination overlap");
    auto live = saved_fixture(program, full);
    check(std::move(live), exception_runtime::copy_saved_registers, "Live descriptor between scalar and vector phases", {},
        [](State& state, Effects& effects, std::uint32_t address, unsigned) {
            if (state.pc == 0x107718c0 && address == action_state && !effects.mutated) {
                effects.mutated = true;
                state.memory.write<2>(descriptor, (5u << 11) | 4 | 32);
                state.memory.write<1>(descriptor + 2, 3u << 3);
            }
        });
    SavedSuffixes mixed{9, 10, 5, true, true, true};
    auto mapped = saved_fixture(program, mixed);
    const auto fs = caller - mixed.floating * 8, gs = fs - 8 - mixed.general * 4;
    const auto vs = (gs - mixed.vector * 16 - 64) & ~std::uint32_t(15);
    for (auto omitted : {action_state + 3, context + 0x39f, descriptor + 1, current_frame + 3,
            fs + 63, context + 0x317, gs + 31, context + 0x393, descriptor + 2,
            vs + 3, vs + 7, vs + 15, context + (32 - mixed.vector) * 16 + 7,
            caller + 7, context + 0x39b, caller + 11})
        check(without_byte(*mapped, omitted), exception_runtime::copy_saved_registers,
            "Saved frame missing byte " + hex_address(omitted), {}, {}, true);
}
constexpr std::array<std::uint32_t, 16> targets{0x107717a4, 0x107717a4, 0x10771704, 0x10771710,
    0x1077171c, 0x10771728, 0x10771734, 0x10771734, 0x10771744, 0x10771750,
    0x1077175c, 0x10771768, 0x10771774, 0x10771780, 0x107717a4, 0x1077178c};
std::unique_ptr<State> action_fixture(const char* program, unsigned kind, unsigned words = 2,
        std::uint32_t live_toc = toc, std::uint32_t live_action = action) {
    auto s = setup(program);
    s->wr<1>(2304, kind % 2 ? 0xf9 : 0xa6); s->wr<1>(1024, kind % 4); s->wr<1>(1027, (kind / 2) % 2);
    if (live_action >= 0xffff0000) { s->memory.map(0xffff0000, 0x10000); s->memory.map(0, 32); }
    s->memory.write<4>(live_toc - 0x3b88, table);
    for (unsigned i = 0; i < targets.size(); ++i) s->memory.write<4>(table + i * 4, targets[i]);
    s->memory.write<4>(action_state, descriptor); s->memory.write<4>(action_state + 8, live_action);
    s->memory.write<1>(live_action, kind); s->memory.write<2>(live_action + 2, words);
    s->wr<4>(8, live_toc); s->wr<4>(12, action_state); s->pc = 0x10771640; return s;
}
void action_checks(const char* program) {
    constexpr std::array<unsigned, 16> sizes{0, 0, 8, 10, 8, 12, 12, 12, 14, 20, 8, 10, 10, 4, 0, 20};
    for (unsigned kind = 2; kind < 16; ++kind) if (kind != 14) {
        for (bool redirect : {false, true}) {
            auto s = action_fixture(program, kind); const auto next = action + sizes[kind];
            s->memory.write<1>(next, redirect ? 1 : 0x95); s->memory.write<2>(next + 2, 0x88);
            s->memory.write<1>(descriptor + 0x88, 0x96);
            const auto result = redirect ? descriptor + 0x88 : next;
            check(std::move(s), exception_runtime::advance_action, "Action kind=" + std::to_string(kind) + (redirect ? " relative jump" : " sequential"),
                [=](const State& state, const Effects&) {
                    require(state.memory.read<4>(action_state + 8) == result && state.r<4>(12) == (redirect ? 22u : 21u), "Independent action size and kind");
                }, {}, false, kind == 15 && redirect);
        }
    }
    auto relocated = action_fixture(program, 2, 0, fixture + 0x9000);
    relocated->memory.write<4>(table + 8, 0x10771780); relocated->memory.write<1>(action + 4, 0x97);
    check(std::move(relocated), exception_runtime::advance_action, "Relocated TOC and mutable case destination",
        [](const State& s, const Effects&) { require(s.memory.read<4>(action_state + 8) == action + 4 && s.r<4>(12) == 23, "Live table redirects kind 2 to four-byte case"); });
    auto wrapping = action_fixture(program, 15, 1, toc, 0xfffffff0);
    wrapping->memory.write<1>(0, 0x98);
    check(std::move(wrapping), exception_runtime::advance_action, "Action advance wraps word32",
        [](const State& s, const Effects&) { require(s.memory.read<4>(action_state + 8) == 0 && s.r<4>(12) == 24, "Wrapped next-action pointer"); });
    for (unsigned words : {0u, 1u, 7u, 65535u}) {
        auto s = action_fixture(program, 15, words); const auto next = action + words * 4 + 12;
        if (next >= fixture + 0x10000) s->memory.map(next, 16);
        s->memory.write<1>(next, 0x9b);
        check(std::move(s), exception_runtime::advance_action, "Variable action count=" + std::to_string(words),
            [=](const State& state, const Effects&) {
                require(state.memory.read<4>(action_state + 8) == next && state.r<4>(12) == 27, "Live unsigned halfword action count");
            });
    }
    for (unsigned kind : {5u, 6u, 7u}) {
        auto s = action_fixture(program, kind); s->memory.write<1>(action + 12, 0x99); s->memory.write<1>(action + 0x10c, 0x9a);
        check(std::move(s), exception_runtime::advance_action, "Live action reread kind=" + std::to_string(kind),
            [=](const State& state, const Effects& effects) {
                require(effects.mutated && state.r<4>(12) == (kind == 5 ? 25u : 26u), "Case-specific action reread");
            }, [](State& state, Effects& effects, std::uint32_t address, unsigned) {
                if (state.pc == 0x10771664 && address == action && !effects.mutated) {
                    effects.mutated = true; state.memory.write<4>(action_state + 8, action + 0x100);
                }
            });
    }
    constexpr auto termination_callback = fixture + 0x8000;
    for (unsigned kind : {0u, 1u, 14u, 16u, 127u}) {
        auto s = action_fixture(program, kind);
        s->memory.write<4>(toc - 0x3520, termination_callback);
        s->memory.write<4>(termination_callback, 0x10773700); s->memory.write<4>(termination_callback + 4, fixture + 0x9100);
        s->memory.write<2>(action + 2, 0x88); s->memory.write<1>(descriptor + 0x88, 0x96);
        check(std::move(s), exception_runtime::advance_action, "Returning termination callback kind=" + std::to_string(kind),
            [=](const State& state, const Effects& effects) {
                require(effects.callbacks.size() == 1 && effects.callbacks[0][3] == termination_callback,
                    "Termination callback CFM descriptor witness");
                require(state.r<4>(12) == (kind == 1 ? 22u : kind) && state.r<4>(8) == toc,
                    "Returning termination callback retains caller TOC and resumes action interpretation");
            });
    }
    auto callback_fault = action_fixture(program, 14);
    callback_fault->memory.write<4>(toc - 0x3520, termination_callback);
    callback_fault->memory.write<4>(termination_callback, 0x10773700); callback_fault->memory.write<4>(termination_callback + 4, toc);
    for (auto omitted : {toc - 0x351d, termination_callback + 3, termination_callback + 7})
        check(without_byte(*callback_fault, omitted), exception_runtime::advance_action,
            "Termination callback missing byte " + hex_address(omitted), {}, {}, true);
    auto no_metadata = action_fixture(program, 0);
    no_metadata->memory.write<4>(action_state + 8, 0); no_metadata->memory.write<4>(action_state + 0x18, current_frame);
    no_metadata->memory.write<4>(current_frame, caller); no_metadata->memory.write<4>(caller + 8, 0x10000130);
    no_metadata->memory.write<1>(toc - 0x3529, 1); no_metadata->memory.write<1>(toc - 0x352a, 0);
    no_metadata->memory.write<4>(toc - 0x3524, 0); no_metadata->memory.write<4>(toc - 0x3528, 0);
    no_metadata->memory.write<4>(toc - 0x3520, termination_callback);
    no_metadata->memory.write<4>(termination_callback, 0x10773700); no_metadata->memory.write<4>(termination_callback + 4, toc);
    check(std::move(no_metadata), exception_runtime::advance_action, "Missing metadata after returning termination callback", {}, {}, true);
    auto mapped = action_fixture(program, 15); mapped->memory.write<1>(action + 20, 1);
    mapped->memory.write<2>(action + 22, 0x88); mapped->memory.write<1>(descriptor + 0x88, 0x96);
    for (auto omitted : {stack - 1, stack - 5, stack + 11, stack - 64, action_state + 11,
            action, toc - 0x3b85, table + 63, action + 3, action + 20, action + 23, descriptor + 0x88})
        check(without_byte(*mapped, omitted), exception_runtime::advance_action,
            "Action missing byte " + hex_address(omitted), {}, {}, true);
}
void frame_traversal_checks(const char* program) {
    constexpr auto node = fixture + 0x1000, ranges = fixture + 0x1800, base = 0x10000000u,
        first_descriptor = descriptor + 0x200, second_descriptor = descriptor + 0x300, saved = fixture + 0xa500;
    for (bool initial_action : {false, true}) for (bool save_register : {false, true})
        for (bool indirect_frame : {false, true}) for (bool two_frames : {false, true}) {
            auto s = setup(program); s->pc = 0x10771640; s->wr<4>(12, action_state);
            s->memory.write<4>(action_state, descriptor); s->memory.write<4>(action_state + 8, initial_action ? action : 0);
            s->memory.write<1>(action, 0x80); s->memory.write<4>(action_state + 0x18, current_frame);
            s->memory.write<4>(action_state + 0x20, saved); s->memory.write<4>(current_frame, caller);
            s->memory.write<2>(descriptor, (save_register ? 1u << 11 : 0) | (3u << 6));
            s->memory.write<4>(caller - 36, saved); s->memory.write<4>(caller + 8, base + 0x130);
            s->memory.write<4>(caller, caller + 0x1000); s->memory.write<4>(caller + 0x1008, base + 0x230);
            s->memory.write<1>(toc - 0x3529, 1); s->memory.write<1>(toc - 0x352a, 0);
            s->memory.write<4>(toc - 0x3524, node); s->memory.write<4>(toc - 0x3528, 0);
            for (auto [offset, value] : {std::pair{0u, ranges}, {4u, ranges + 12}, {8u, base},
                    {12u, base + 0x1000}, {16u, descriptor}, {24u, toc}}) s->memory.write<4>(node + offset, value);
            for (unsigned n = 0; n < 2; ++n) {
                s->memory.write<4>(ranges + n * 12, 0x100 + n * 0x100);
                s->memory.write<4>(ranges + n * 12 + 4, 0x40); s->memory.write<4>(ranges + n * 12 + 8, 0x200 + n * 0x100);
                const auto d = descriptor + 0x200 + n * 0x100;
                s->memory.write<1>(d + 1, indirect_frame ? 16 : 0);
                s->memory.write<2>(d + 2, two_frames && n == 0 ? 0 : 0x10);
                s->memory.write<2>(d + 4, 0x30); s->memory.write<2>(d + 6, 0x80); s->memory.write<1>(d + 0x80, 0x93);
            }
            const auto resolved = two_frames ? second_descriptor : first_descriptor;
            const auto frame = caller + (two_frames ? 0x1000 : 0);
            if (initial_action && save_register && indirect_frame && two_frames) {
                for (auto omitted : {descriptor, current_frame + 3, caller - 33, caller + 11,
                        node + 3, ranges + 3, first_descriptor + 3, action_state + 0x23,
                        caller + 0x100b, second_descriptor + 0x80})
                    check(without_byte(*s, omitted), exception_runtime::advance_action,
                        "Frame traversal missing byte " + hex_address(omitted), {}, {}, true);
                child_negative(*s, 0x107715f0); child_negative(*s, 0x107713e0);
            }
            check(std::move(s), exception_runtime::advance_action, "Caller traversal " + std::to_string(comparisons),
                [=](const State& state, const Effects&) {
                    require(state.memory.read<4>(action_state) == resolved && state.memory.read<4>(action_state + 8) == resolved + 0x80,
                        "Resolved live frame descriptor and action");
                    require(state.memory.read<4>(action_state + 0x18) == frame && state.memory.read<4>(action_state + 0x1c) == (indirect_frame ? saved : frame),
                        "Selected frame and saved-register slot");
                    require(state.r<4>(12) == 19, "Caller action masked kind");
                });
        }
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-exception-frame-checks <program>");
        saved_checks(argv[1]); action_checks(argv[1]); frame_traversal_checks(argv[1]);
        std::cout << comparisons << " C++ exception-frame comparisons (" << faults << " mapping faults), "
                  << negatives << " own/child corruption negatives passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
