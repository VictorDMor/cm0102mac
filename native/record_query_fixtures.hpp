#pragma once
#include "record_query_adapter.hpp"
#include "record_group_fixtures.hpp"

namespace cm::record_query_checks {
using namespace cm::semantic_io_checks;
inline std::uint32_t prepare(State& s, unsigned index, unsigned kind = 0) {
    record_query_diagnostics(s).original = true;
    const auto owner = cm::record_group_checks::prepare(s, 0);
    const auto row = owner + record_query::row_stride;
    const auto node = std::uint32_t(s.memory.read<4>(row + 16));
    const auto second = std::uint32_t(s.memory.read<4>(row + record_query::record_stride + 16));
    s.memory.write<4>(node + 0x1f8, kind ? second : 0);
    s.memory.write<4>(node + 0x1f4, kind ? second : 0);
    s.memory.write<4>(node + 12, kind ? fixture + 0x40 : 0);
    s.memory.write<1>(node + 16, kind ? 0x80 : 0);
    for (unsigned n = 0; n < 2; ++n) {
        s.memory.write<1>(row + n * record_query::record_stride + 25, 0);
        s.memory.write<1>(row + n * record_query::record_stride + 26, kind && n ? 0x80 : 0);
    }
    s.wr<4>(12, owner); s.wr<4>(16, index == 0 ? 0x87654321 : kind);
    s.wr<4>(20, 0x12345678); s.wr<4>(24, 0x180); s.wr<4>(28, fixture + 0x40);
    s.wr<4>(32, UINT32_MAX); s.wr<4>(36, 7); s.wr<4>(40, 0x18001);
    s.wr<4>(4128, return_sentinel); s.pc = record_query::entries[index];
    reset_record_query_diagnostics(s); return owner;
}
}
