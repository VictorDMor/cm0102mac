#include "exception_search_fixtures.hpp"
#include <set>

using namespace cm::semantic_io_checks;
using namespace cm::exception_checks;
namespace cm::aot { bool exception_capture_reference(State&); bool exception_scalar_reference(State&); }
namespace {
enum class Mode { vector, scalar, select_scalar, select_vector };
unsigned comparisons = 0, faults = 0, negatives = 0;
std::uint32_t entry(Mode mode) {
    return mode == Mode::vector ? 0x10772b10 : mode == Mode::scalar ? 0x10772ca0 : 0x10772ad0;
}
bool selected(Mode mode) { return mode == Mode::select_scalar || mode == Mode::select_vector; }
bool vectors(Mode mode) { return mode == Mode::vector || mode == Mode::select_vector; }
Operation operation(Mode mode) {
    return mode == Mode::vector ? exception_runtime::throw_vector :
        mode == Mode::scalar ? exception_runtime::throw_scalar : exception_runtime::throw_dispatch;
}
void original(State& s, std::uint32_t stop, bool prefix) {
    while (s.pc != stop) if (!(prefix ? cm::aot::exception_scalar_reference(s) : cm::aot::exception_capture_reference(s)))
        run_until(s, std::uint32_t(s.r<4>(4128)));
}
Effects compare(std::unique_ptr<State> a, Operation semantic, std::uint32_t stop,
        const std::string& label, bool prefix = false, bool fault = false, bool negative = false,
        const Expect& expect = {}, const Mutation& mutation = {}) {
    auto b = clone(*a); Effects ea, eb; ea.state = a.get(); eb.state = b.get(); ea.on_read = eb.on_read = mutation;
    a->memory.date_observer = &ea; b->memory.date_observer = &eb; std::string ax, bx;
    try { original(*a, stop, prefix); } catch (const std::runtime_error& e) { ax = e.what(); }
    try { semantic(*b); } catch (const std::runtime_error& e) { bx = e.what(); }
    a->memory.date_observer = b->memory.date_observer = nullptr;
    require(ax == bx, label + " original=" + ax + " semantic=" + bx);
    require(fault == !ax.empty(), label + " expected outcome=" + ax);
    if (fault) require(ax.starts_with("Unmapped data access at "), label + " mapping fault");
    Image image(*a); image.compare(*b, label); require(ea.bytes == eb.bytes && ea.callbacks == eb.callbacks, label + " ordered access/CFM prefix");
    if (!fault && expect) expect(*b, eb);
    if (negative) {
        b->wr<4>(120, b->r<4>(120) ^ 1); bool rejected = false;
        try { image.compare(*b, label + " corrupted capture result"); } catch (const std::runtime_error&) { rejected = true; }
        require(rejected, label + " own negative accepted"); ++negatives;
    }
    ++comparisons; faults += fault; eb.state = nullptr; eb.on_read = {}; return eb;
}
std::unique_ptr<State> capture_initial(const char* program, Mode mode, unsigned misalignment = 0, bool tail = false) {
    auto f = setup(program, false, vectors(mode)); auto s = std::move(f.state);
    constexpr auto code = 0x10772000u, caller_function = code + 0x100;
    s->memory.write<4>(node + 8, code); s->memory.write<4>(node + 12, code + 0x1000);
    if (selected(mode)) {
        s->memory.write<4>(node + 4, ranges + 24); s->memory.write<4>(ranges + 12, 0xad0);
        s->memory.write<4>(ranges + 16, 0x34); s->memory.write<4>(ranges + 20, 0x400);
    }
    const auto incoming = stack + misalignment;
    s->memory.write<4>(incoming, stack + 128); s->wr<4>(4, incoming);
    s->wr<4>(12, actual); s->wr<4>(16, object); s->wr<4>(20, callback);
    s->wr<4>(4128, caller_function + 0x20); s->pc = entry(mode);
    s->wr<4>(5120, 0x80000100); s->memory.write<1>(toc - 0x352a, vectors(mode) ? 7 : 0);
    if (tail) {
        const auto destination = vectors(mode) ? 0x10772c84u : 0x10772d64u;
        s->memory.write<2>(action + 6, destination - caller_function);
        s->wr<4>(120, incoming - (selected(mode) ? 64 : 0));
    }
    return s;
}
void scalar_prefix_checks(const char* program) {
    for (unsigned offset : {0u, 3u, 15u}) {
        auto s = capture_initial(program, Mode::scalar, offset); const auto before = s->registers;
        const auto incoming = stack + offset, captured = (incoming & ~15u) - 0x3c0;
        compare(std::move(s), exception_runtime::capture_scalar_throw, 0x10772810, "Scalar capture prefix alignment=" + std::to_string(offset),
            true, false, offset == 0, [=](const State& state, const Effects&) {
                require(state.pc == 0x10772810 && state.r<4>(12) == captured && state.r<4>(4128) == 0x10772d64,
                    "Scalar prefix enters search with original context/link");
                require(state.memory.read<4>(captured + 0x39c) == incoming && state.memory.read<4>(captured + 0x3a8) == 0x10772120,
                    "Scalar prefix captures caller frame/link");
                for (auto [field, value] : {std::pair{0x3acu, actual}, {0x3b0u, object}, {0x3b4u, callback}})
                    require(state.memory.read<4>(captured + field) == value, "Scalar prefix captures three throw arguments");
                for (unsigned n = 14; n < 32; ++n)
                    require(state.memory.read<8>(captured + 0x218 + n * 8) == read_be<8>(before.data() + 0x4000 + n * 16),
                        "Scalar prefix captures floating suffix");
                require(std::equal(before.begin() + 0x4200, before.end(), state.registers.begin() + 0x4200),
                    "Scalar prefix leaves vector/VSCR registers intact");
                require(state.memory.read<4>(captured + 0x210) == 0xa7a7a7a7, "Scalar prefix does not publish VRSAVE");
            });
    }
    auto initial = capture_initial(program, Mode::scalar); auto successful = clone(*initial);
    const auto effects = compare(std::move(successful), exception_runtime::capture_scalar_throw, 0x10772810, "Scalar prefix fault inventory", true);
    std::set<std::uint32_t> omitted;
    for (const auto& access : effects.log) if (access.size) {
        omitted.insert(access.address); omitted.insert(access.address + std::uint32_t(access.size) - 1);
    }
    for (auto address : omitted)
        compare(without_byte(*initial, address), exception_runtime::capture_scalar_throw, 0x10772810,
            "Scalar prefix missing byte " + hex_address(address), true, true);
}
void complete_checks(const char* program) {
    for (auto mode : {Mode::vector, Mode::scalar, Mode::select_scalar, Mode::select_vector}) {
        for (bool tail : {false, true}) {
            auto s = capture_initial(program, mode, 0, tail); const auto stop = tail ? 0x10772120u : 0x10772300u;
            compare(std::move(s), operation(mode), stop, "Full capture mode=" + std::to_string(unsigned(mode)) + " tail=" + std::to_string(tail),
                false, false, !tail, [=](const State& state, const Effects&) {
                    require(state.pc == stop && state.r<4>(4128) == stop, "Complete capture destination/link");
                    require(state.r<4>(4) == stack - (!tail && selected(mode) ? 64 : 0), "Complete capture restored/returned stack");
                    require(state.memory.read<4>(stack + 0x40) == object && state.memory.read<4>(stack + 0x44) == actual &&
                        state.memory.read<4>(stack + 0x48) == callback, "Complete capture installs caller catch fields");
                });
        }
    }
    auto live = capture_initial(program, Mode::select_scalar);
    compare(std::move(live), exception_runtime::throw_dispatch, 0x10772300, "Live selector flag changes before loaded value is consumed",
        false, false, false, [](const State& state, const Effects& effects) {
            require(effects.mutated && state.pc == 0x10772300, "Live capability flag mutation reaches handler");
            const auto captured = stack - 64 - 0x3c0;
            require(state.memory.read<4>(captured + 0x210) == 0x80000100, "Live nonzero flag selects actual vector capture");
        }, [](State& state, Effects& effects, std::uint32_t address, unsigned width) {
            if (!effects.mutated && address == toc - 0x352a && width == 1 && state.pc == 0x10772adc) {
                effects.mutated = true; state.memory.write<1>(address, 7);
            }
        });
    for (auto mode : {Mode::vector, Mode::scalar, Mode::select_scalar, Mode::select_vector}) {
        auto initial = capture_initial(program, mode, 0, true);
        const auto incoming = stack - (selected(mode) ? 64 : 0);
        for (auto address : {stack + 0x43, stack + 0x4f, incoming - 1, incoming - 5})
            compare(without_byte(*initial, address), operation(mode), 0x10772120,
                "Full capture fault mode=" + std::to_string(unsigned(mode)) + " byte=" + hex_address(address), false, true);
        auto tail = capture_initial(program, mode, 0, true); tail->wr<4>(120, restored_stack);
        tail->memory.write<4>(restored_stack + 8, selected(mode) ? (vectors(mode) ? 0x10772aec : 0x10772af4) : 0x10772120);
        for (auto address : {restored_stack + 11, restored_stack - 1, restored_stack - 5})
            compare(without_byte(*tail, address), operation(mode), 0x10772120,
                "Capture returning-epilogue fault mode=" + std::to_string(unsigned(mode)) + " byte=" + hex_address(address), false, true);
    }
}
void search_child_negative(const char* program) {
    auto a = capture_initial(program, Mode::vector, 0, true), b = clone(*a);
    std::unique_ptr<Image> child; Effects ea, eb; ea.state = a.get(); eb.state = b.get();
    ea.on_read = [&](State& state, Effects&, std::uint32_t, unsigned) {
        if (state.pc == 0x10772c84) child = std::make_unique<Image>(state);
    };
    a->memory.date_observer = &ea; original(*a, 0x10772120, false); a->memory.date_observer = nullptr;
    require(bool(child), "Original search child reaches vector capture epilogue");
    bool reached = false;
    eb.on_read = [&](State& state, Effects&, std::uint32_t, unsigned) {
        if (state.pc == 0x10772c84) {
            reached = true; state.wr<4>(52, state.r<4>(52) ^ 1);
            child->compare(state, "Corrupted search child before capture epilogue");
        }
    };
    b->memory.date_observer = &eb; bool rejected = false;
    try { exception_runtime::throw_vector(*b); }
    catch (const std::runtime_error& e) { rejected = std::string_view(e.what()).starts_with("Corrupted search child before capture epilogue"); }
    b->memory.date_observer = nullptr;
    require(reached && rejected && b->pc == 0x10772c84 && b->r<4>(4) == stack,
        "Search child corruption is rejected before its capture parent consumes the return word");
    ++negatives;
}
}
int main(int argc, char** argv) {
    try {
        require(argc == 2, "Usage: cm-exception-capture-checks <program>");
        scalar_prefix_checks(argv[1]); complete_checks(argv[1]); search_child_negative(argv[1]);
        require(negatives == 6, "Scalar prefix, complete capture roots and search child need corruption negatives");
        std::cout << comparisons << " original/C++ capture comparisons (" << faults << " mapping faults), " << negatives
                  << " corruption negatives; scalar/vector selection and returning tails passed\n"; return 0;
    } catch (const std::exception& e) { std::cerr << e.what() << '\n'; return 1; }
}
