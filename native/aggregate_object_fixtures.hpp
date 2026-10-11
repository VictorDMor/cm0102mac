#pragma once
#include "aggregate_object_adapter.hpp"
#include "utility_object_fixtures.hpp"

namespace cm::aggregate_object_checks {
using namespace cm::semantic_io_checks;
inline std::uint32_t prepare(State& s, unsigned index, unsigned kind = 0) {
    aggregate_object_diagnostics(s).original = true;
    const auto size = (index < 2 ? aggregate_object::table_extent : aggregate_object::aggregate_extent) + 16;
    const auto owner = cm::utility_object_checks::allocate(s, size);
    // The production globals begin in zero-initialized storage. Explicitly
    // reproduce that prestate before calling the actual original constructors.
    s.memory.copy_in(owner, std::vector<std::uint8_t>(size, 0));
    if (index == 1) cm::utility_object_checks::call(s, 0x10718b40, owner, kind);
    if (index == 1 && kind) {
        // The original root owns a global-allocator pointer array, whose
        // descriptor is retained. Use the actual allocator family on both sides.
        cm::utility_object_checks::call(s, 0x10775130, 8);
        const auto pointers = std::uint32_t(s.r<4>(12));
        for (unsigned n = 0; n < 2; ++n) {
            cm::utility_object_checks::call(s, 0x10775130, 16);
            s.memory.write<4>(pointers + n * 4, s.r<4>(12));
        }
        const auto descriptor = cm::utility_object_checks::allocate(s, 8);
        s.memory.write<4>(descriptor, pointers); s.memory.write<4>(descriptor + 4, 2);
        s.memory.write<4>(owner + 0x12a92c, descriptor);
        s.memory.write<2>(owner + 0x129da4, 1); s.memory.write<2>(owner + 0x129da6, 1);
    }
    if (index >= 3) cm::utility_object_checks::call(s, 0x1073c1b0, owner);
    if (index == 4 && kind) for (unsigned row = 0; row < 16; ++row) s.memory.write<2>(owner + 0x2c38 + row * 2, 0xffff);
    if (index == 5 && kind) {
        // An attached, empty zero-initialized guest stream exercises the actual
        // generated flush/close/deleting path without opening a user file.
        const auto stream = cm::utility_object_checks::allocate(s, 0xc3800);
        s.memory.copy_in(stream, std::vector<std::uint8_t>(0xc3800, 0));
        s.memory.write<4>(owner + 0x2c64, stream);
    }
    s.wr<4>(12, owner); s.wr<4>(16, (index == 1 || index == 3) ? (kind ? 1 : UINT32_MAX) : kind);
    s.wr<4>(4128, return_sentinel); s.pc = aggregate_object::entries[index];
    reset_aggregate_object_diagnostics(s); return owner;
}
}
