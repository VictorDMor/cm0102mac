#pragma once
#include "utility_object_adapter.hpp"
#include "engine_initialization_fixtures.hpp"

namespace cm::utility_object_checks {
using namespace cm::semantic_io_checks;
inline constexpr std::uint32_t destructor_vector = fixture + 0x100;
inline void call(State& state, std::uint32_t entry, std::uint32_t first,
                 std::uint32_t second = 0, std::uint32_t third = 0) {
    state.wr<4>(12, first); state.wr<4>(16, second); state.wr<4>(20, third);
    state.wr<4>(4128, return_sentinel); state.pc = entry;
    run_until(state, return_sentinel);
    require(state.pc == return_sentinel, "Original fixture prerequisite returned");
}
inline std::uint32_t allocate(State& state, unsigned size) {
    call(state, 0x10773520, size);
    const auto pointer = std::uint32_t(state.r<4>(12));
    require(pointer != 0, "Actual guest object allocation");
    state.memory.copy_in(pointer, std::vector<std::uint8_t>(size, 0xa7));
    return pointer;
}
inline std::uint32_t node(State& state) {
    const auto owner = allocate(state, 16);
    call(state, 0x10738b20, owner, fixture + 0x400, 7);
    return owner;
}
inline void prepare(State& state, unsigned index) {
    // Set up both sides using actual original object constructors and actual
    // allocator helpers. A candidate never supplies its own fixture oracle.
    utility_object_diagnostics(state).original = true;
    state.memory.write<4>(destructor_vector, 0x107735a0);
    state.memory.write<4>(destructor_vector + 4, state.r<4>(8));
    const auto owner = allocate(state, 16);
    if (index == 1) call(state, 0x10738b20, owner, fixture + 0x400, 7);
    if (index == 3 || index == 4) {
        call(state, 0x10738c00, owner);
        const auto first = node(state), second = node(state);
        state.memory.write<4>(first + 8, second);
        state.memory.write<2>(owner, 2);
        state.memory.write<4>(owner + 4, first);
        state.memory.write<4>(owner + 8, second);
    }
    if (index == 6) call(state, 0x1073ae10, owner, 97);
    if (index >= 8) {
        call(state, 0x1074dec0, owner, destructor_vector, destructor_vector);
        const auto root = allocate(state, 16), left = allocate(state, 16), right = allocate(state, 16);
        for (auto pointer : {root, left, right}) {
            state.memory.write<4>(pointer + 8, 0); state.memory.write<4>(pointer + 12, 0);
        }
        state.memory.write<4>(root + 8, left); state.memory.write<4>(root + 12, right);
        state.memory.write<4>(owner, root); state.memory.write<4>(owner + 12, 3);
    }
    state.wr<4>(12, owner); state.wr<4>(16, 1); state.wr<4>(20, 7);
    if (index == 0) state.wr<4>(16, fixture + 0x400);
    if (index == 5) state.wr<4>(16, 97);
    if (index == 7) { state.wr<4>(16, destructor_vector); state.wr<4>(20, destructor_vector); }
    if (index == 10) { state.wr<4>(16, destructor_vector); state.wr<4>(20, 1); }
    state.wr<4>(4128, return_sentinel); state.pc = cm::aot::utility_object::entries[index];
    reset_utility_object_diagnostics(state);
}
}
