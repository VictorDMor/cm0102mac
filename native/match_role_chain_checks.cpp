#include "match_eng_adapter.hpp"
#include "match_eng_space.hpp"
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

unsigned drawing_spatial_callers() {
    constexpr auto constants = fixture + 0x8800, distances = fixture + 0x8200;
    unsigned cases = 0, faults = 0;
    for (const auto entry : {0x101e4130u, 0x101e32e0u}) for (unsigned side = 0; side < 2; ++side) {
        for (unsigned scenario = 0; scenario < 7; ++scenario) {
            auto initial = seed(0, 3, 0);
            initial->memory.map(stack - 0x200, 0x170);
            initial->memory.map(stack + 0x10, 0x50);
            initial->memory.map(toc - 0x8000, 0x8000);
            initial->memory.map(fixture + 0x8200, 0x1000);
            guest::Cpu c(*initial);
            c.put32(toc - 0x52c4, constants);
            c.put32(toc - 0x52c8, constants);
            c.put32(toc - 0x6424, distances);
            c.put_f64(constants + 0x50, 0);
            c.put_f32(constants + 0xc8, 1000);
            for (unsigned y = 0; y < 9; ++y) for (unsigned x = 0; x < 9; ++x)
                c.put_f32(distances + y * 36 + x * 4, float(x + y));
            for (unsigned team = 0; team < 2; ++team) {
                c.put32(match + 0x7934 + team * 4, role_table);
                for (unsigned n = 0; n < 20; ++n) {
                    const auto actor = match + match_eng::players_at + team * match_eng::side_size + n * match_eng::player_size;
                    c.put8(actor + 0x19, n < 11 ? n : 0xff);
                    c.put8(actor + 0x27, team);
                    c.put8(actor + 0x2c, 0);
                    c.put8(actor + 0x105, n % 12);
                    c.put8(actor + 0x106, (n + team) % 9);
                    c.put32(actor + 0x1a4, match);
                }
            }
            for (unsigned n = 0; n < 11; ++n) c.put16(role_table + n * 2, n == 0 ? 1 : 0);
            const auto actor = match + match_eng::players_at + side * match_eng::side_size + 2 * match_eng::player_size;
            if (scenario == 1) c.put8(actor + 0x19, 11);
            if (scenario == 2) c.put32(actor + 0x1a4, 0);
            if (scenario == 3) c.put32(match + 0x7934 + side * 4, stack - 0xa8);
            if (scenario == 4) c.put32(match + 0x7934 + side * 4, 0xffe00000);
            if (scenario == 5) c.put32(toc - 0x6424, 0xffe00000);
            c.set({{3, match}, {4, side}, {5, 3}, {6, 4}, {7, 0}});
            c.set_lr(return_sentinel);
            initial->pc = entry;
            auto expected = clone(*initial), actual = clone(*initial);
            NativeDrawingObserver old_drawing, new_drawing;
            const auto surface = scenario == 6 ? stack - 0xa0 : pixels;
            old_drawing.register_surface(*expected, surface, 1, 1, 2, 16);
            new_drawing.register_surface(*actual, surface, 1, 1, 2, 16);
            // The diagnostic forces the accepted dispatch route. The other
            // state uses the normal drawing-aware direct route when eligible.
            Effects observe;
            observe.state = expected.get();
            expected->memory.date_observer = &observe;
            const auto run = [entry, side](State& s) {
                guest::Cpu c(s);
                try {
                    if (entry == 0x101e4130) match_eng::update_line_limits(c, match);
                    else match_eng::nearest_player_to_cell(c, match, side, 3, 4, 0);
                    c.finish();
                    return std::string{};
                } catch (const std::runtime_error& error) { return std::string(error.what()); }
            };
            const auto old_stop = run(*expected), new_stop = run(*actual);
            require(old_stop == new_stop, "drawing spatial caller fault changed");
            const auto expected_fault = scenario == 4 || (scenario == 5 && entry == 0x101e32e0);
            require(old_stop.empty() != expected_fault, "unexpected drawing spatial caller outcome");
            expected->memory.date_observer = nullptr;
            Image(*expected).compare(*actual, "drawing spatial caller " + std::to_string(cases++), !old_stop.empty());
            faults += !old_stop.empty();
            if (!old_stop.empty()) std::cout << hex_address(entry) << " side=" << side << " scenario=" << scenario
                                             << " partial=" << old_stop << '\n';
        }
    }
    require(faults == 6, "drawing spatial caller fault scenarios " + std::to_string(faults) + "/6");
    return cases;
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
    const auto drawing_cases = drawing_spatial_callers();
    std::cout << cases << " generated role comparisons, alias, observer, probe, partial fault and negative control; "
              << drawing_cases << " drawing spatial caller cases (six partial faults)\n";
    return 0;
} catch (const std::exception& e) {
    std::cerr << e.what() << '\n';
    return 1;
}
