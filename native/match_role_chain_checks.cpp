#include "match_eng_adapter.hpp"
#include "match_pl_adapter.hpp"
#include "native_drawing.hpp"
#include "routine_check.hpp"
#include "semantic_io_checks.hpp"

namespace {
using namespace cm::semantic_io_checks;
constexpr std::uint32_t player = fixture + 0x100, match = fixture + 0x200,
                        role_table = fixture + 0x8000, pixels = fixture + 0x8100;
constexpr std::array<std::uint32_t, 3> entries{0x10289560, 0x102895a0, 0x102895e0};

std::unique_ptr<State> seed(unsigned index, int position, int side) {
    auto s = std::make_unique<State>();
    s->budget = UINT64_MAX;
    s->registers.fill(0xa5);
    s->memory.map(fixture, 0x8200);
    s->memory.map(stack - 0x90, 0xa0);
    s->pc = entries[index];
    s->wr<4>(4, stack);
    s->wr<4>(8, toc);
    s->wr<4>(12, player);
    s->wr<4>(4128, return_sentinel);
    s->memory.write<4>(player + 0x1a4, match);
    s->memory.write<1>(player + 0x19, index == 1 ? 0xff : position);
    s->memory.write<1>(player + 0x1a, index == 1 ? position : 0xff);
    s->memory.write<1>(player + 0x27, side);
    const auto slot = match + (index == 2 ? 0x793c : 0x7934) + std::uint32_t(side) * 4;
    s->memory.write<4>(slot, role_table);
    for (unsigned n = 0; n <= 10; ++n) s->memory.write<2>(role_table + n * 2, 0x8000 + n * 5);
    return s;
}

void run_role(State& s, unsigned index) {
    require(try_modern_match_pl(s, entries[index]), "missing production role route");
}

void exact_return(const State& seed_state, unsigned index, bool drawing, bool corrupt = false) {
    auto original = clone(seed_state);
    auto native = clone(seed_state);
    NativeDrawingObserver observer;
    if (drawing) observer.register_surface(*native, pixels, 1, 1, 2, 16);
    match_family_run_original(*original, entries[index]);
    run_role(*native, index);
    if (corrupt) native->wr<4>(12, native->r<4>(12) ^ 1);
    bool mismatch = false;
    try {
        Image(*original).compare(*native, "role entry " + hex_address(entries[index]));
    } catch (const std::runtime_error&) {
        if (!corrupt) throw;
        mismatch = true;
    }
    require(mismatch == corrupt, "corrupted role result was accepted");
}

// Move an input into a saved word. The child must see the frame's live bytes.
void frame_alias() {
    auto s = seed(0, 3, 0);
    s->memory.write<4>(match + 0x7934, stack - 0x48 - 6);
    s->wr<4>(30 * 4, 0xbeef1234);
    exact_return(*s, 0, false);
}

// The diagnostic changes the position after its second read. An optimized
// single read would miss both the read callback and the new table index.
void observed_position() {
    auto s = seed(0, 3, 0);
    Effects effects;
    effects.state = s.get();
    unsigned positions = 0;
    effects.on_read = [&](State& state, Effects&, std::uint32_t address, unsigned) {
        if (address == player + 0x19 && ++positions == 2)
            state.memory.write<1>(player + 0x19, 7);
    };
    s->memory.date_observer = &effects;
    run_role(*s, 0);
    require(positions == 3 && s->r<4>(12) == 0xffff8023, "role read observer did not control lookup");
}

void probed_child() {
    auto s = seed(0, 3, 0);
    dispatch_probe = +[](State& state) {
        if (state.pc != 0x101e3d50) return false;
        state.wr<4>(12, 0x2345);
        state.pc = std::uint32_t(state.r<4>(4128));
        return true;
    };
    run_role(*s, 0);
    dispatch_probe = nullptr;
    require(s->r<4>(12) == 0x2345, "role lookup bypassed the dispatcher probe");
}

void partial_frame() {
    auto seed_state = seed(0, 3, 0);
    auto make_sparse = [&] {
        auto s = std::make_unique<State>();
        s->budget = seed_state->budget;
        s->registers = seed_state->registers;
        s->pc = seed_state->pc;
        s->memory.map(fixture, 0x8200);
        s->memory.copy_in(fixture, seed_state->memory.copy_out(fixture, 0x8200));
        s->memory.map(stack - 0x90, 0x4c);
        s->memory.map(stack - 0x40, 0x50);
        return s;
    };
    auto actual = make_sparse();
    auto expected = make_sparse();
    auto fault = [](State& s) {
        try { run_role(s, 0); } catch (const std::runtime_error& e) { return std::string(e.what()); }
        throw std::runtime_error("fragmented frame failed to fault");
    };
    std::vector<std::pair<std::uint32_t, unsigned>> reads;
    expected->memory.read_trace = &reads;
    require(fault(*actual) == fault(*expected), "fragmented frame fault changed");
    // Both legacy routes reach the partial child spill before its stack update.
    require(actual->r<4>(4) == stack - 0x40 && actual->memory.read<4>(stack - 0x48) == 0xa5a5a5a5,
            "fragmented frame lost its visible partial writes");
    Image(*expected).compare(*actual, "fragmented frame");
}
}

int main() try {
    unsigned cases = 0;
    for (unsigned index = 0; index < entries.size(); ++index) {
        for (int side : {-1, 0, 1, 2}) {
            for (int position : {-128, -1, 0, 3, 10, 11, 127}) {
                auto s = seed(index, position, side);
                exact_return(*s, index, false);
                exact_return(*s, index, true);
                cases += 2;
            }
        }
        auto s = seed(index, 3, 0);
        s->memory.write<4>(player + 0x1a4, 0);
        exact_return(*s, index, false);
        ++cases;
    }
    frame_alias();
    observed_position();
    probed_child();
    partial_frame();
    exact_return(*seed(0, 3, 0), 0, false, true);
    std::cout << cases << " generated role comparisons, alias, observer, probe, partial fault and negative control\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
