#pragma once
#include "tree_mutation_adapter.hpp"
#include "utility_object_fixtures.hpp"

namespace cm::tree_mutation_checks {
using namespace cm::semantic_io_checks;
inline constexpr std::uint32_t compare_vector = fixture + 0x100, delete_vector = fixture + 0x108;
inline std::uint32_t prepare(State& s, unsigned index, unsigned kind = 0) {
    tree_mutation_diagnostics(s).original = true; utility_object_diagnostics(s).original = true;
    s.memory.write<4>(compare_vector, 0x1077dc90); s.memory.write<4>(compare_vector + 4, s.r<4>(8));
    s.memory.write<4>(delete_vector, 0x107735a0); s.memory.write<4>(delete_vector + 4, s.r<4>(8));
    const auto owner = cm::utility_object_checks::allocate(s, 16);
    cm::utility_object_checks::call(s, 0x1074dec0, owner, compare_vector, delete_vector);
    // Build identical prerequisites with the actual original insertion, scalar
    // allocator and CFM byte-string comparator. Ascending keys deliberately
    // create a chain deeper than the cleanup's four expanded levels.
    if (kind != 3) for (unsigned i = 0; i < 7; ++i) {
        const auto text = fixture + 0x400 + i * 16;
        s.memory.write<1>(text, 'M' + i); s.memory.write<1>(text + 1, 0);
        s.wr<4>(12, owner); s.wr<4>(16, owner); s.wr<4>(20, text); s.wr<4>(24, 0x100 + i);
        s.wr<4>(4128, return_sentinel); s.pc = 0x1074e030; run_until(s, return_sentinel);
    }
    const auto item = fixture + 0x600;
    s.memory.write<1>(item, kind == 1 ? 'M' : kind == 2 ? 'Z' : 'A'); s.memory.write<1>(item + 1, 0);
    s.wr<4>(12, owner); s.wr<4>(16, index == 2 ? s.memory.read<4>(owner) : owner);
    s.wr<4>(20, index == 2 ? delete_vector : item); s.wr<4>(24, 1);
    s.memory.write<4>(s.r<4>(4) + 8, return_sentinel); s.wr<4>(4128, return_sentinel); s.pc = tree_mutation::entries[index];
    reset_tree_mutation_diagnostics(s); reset_utility_object_diagnostics(s); return owner;
}
}
