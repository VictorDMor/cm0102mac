#pragma once
#include "record_group_adapter.hpp"
#include "aggregate_object_fixtures.hpp"

namespace cm::record_group_checks {
using namespace cm::semantic_io_checks;
inline std::uint32_t prepare(State& s, unsigned index, unsigned kind = 0) {
    record_group_diagnostics(s).original = true;
    const auto owner = cm::utility_object_checks::allocate(s, aggregate_object::aggregate_extent + 64);
    s.memory.copy_in(owner, std::vector<std::uint8_t>(aggregate_object::aggregate_extent + 64, 0));
    aggregate_object_diagnostics(s).original = true;
    cm::utility_object_checks::call(s, 0x1073c1b0, owner);
    reset_aggregate_object_diagnostics(s);
    s.memory.write<4>(s.r<4>(8) - 0x7bb0, owner);
    s.memory.copy_in(fixture + 0x40, {'n','e','w',0});
    // Three real CFM descriptors call an original two-instruction constant
    // return body (0x10002390), never a host callback or a replaced record body.
    for (auto offset : {0x200u, 0x220u, 0x240u}) {
        s.memory.write<4>(fixture + offset, 0x10002390); s.memory.write<4>(fixture + offset + 4, s.r<4>(8));
    }
    const auto row = owner + record_group::row_stride;
    for (unsigned n = 0; n < 2; ++n) {
        const auto node = cm::utility_object_checks::allocate(s, 0x300);
        s.memory.copy_in(node, std::vector<std::uint8_t>(0x300, 0));
        s.memory.copy_in(node + 0x1fc, {'l','a','b','e','l',0});
        cm::utility_object_checks::call(s, 0x107735f0, 4);
        const auto field = std::uint32_t(s.r<4>(12)); s.memory.copy_in(field, {'o','l','d',0});
        s.memory.write<4>(node + 0x24, field); s.memory.write<1>(node + 0x28, 1);
        if (kind) s.memory.write<4>(node + 4, fixture + 0x240);
        const auto record = row + n * record_group::record_stride;
        s.memory.write<2>(record, 1); s.memory.write<4>(record + 4, n ? 22 : 11);
        s.memory.write<4>(record + 8, node); s.memory.write<4>(record + 16, node);
        s.memory.write<2>(record + 22, 1); s.memory.write<4>(record + 36, owner + 0x12d6b0);
    }
    s.memory.write<2>(owner + 0x2c3a, (index == 10 && kind) ? 1 : 2);
    // The constructor's default first row has no current node. Bulk field
    // updates require active rows to contain a node; leave this row inactive.
    if (index == 4) s.memory.write<2>(owner + 0x2c38, 0);
    s.memory.write<2>(owner + 0x2c58, 1); s.memory.write<1>(owner + 0x2c5e, 1);
    if (kind) { s.memory.write<4>(owner + 0x2581b0, fixture + 0x200); s.memory.write<4>(owner + 0x2581b4, fixture + 0x220); }
    // A closed guest stream exercises the actual write method's -1-handle
    // return path. It opens no file and cannot write a user save.
    const auto stream = cm::utility_object_checks::allocate(s, 0xc39b0);
    s.memory.copy_in(stream, std::vector<std::uint8_t>(0xc39b0, 0)); s.memory.write<2>(stream + 24, 0xffff);
    s.memory.write<4>(owner + 0x2c64, stream);
    s.wr<4>(12, owner); s.wr<4>(16, index == 8 ? 11 : 2);
    s.wr<4>(20, index == 8 ? 22 : fixture + 0x40); s.wr<4>(24, kind); s.wr<4>(28, 0);
    if (index == 1) s.wr<4>(16, kind ? 0 : fixture + 0x40);
    if (index == 4) { s.wr<4>(16, kind ? 11 : 0); s.wr<4>(20, 2); s.wr<4>(24, fixture + 0x40); s.wr<4>(28, kind); }
    if (index == 6 || index == 7) s.wr<4>(12, 1);
    if (index == 9) { s.wr<4>(16, kind ? fixture + 0x200 : 0); s.wr<4>(20, kind ? fixture + 0x220 : 0); s.wr<4>(24, 0); }
    if (index == 10) { s.wr<4>(16, 11); s.wr<4>(20, 1); }
    s.wr<4>(4128, return_sentinel); s.pc = record_group::entries[index];
    reset_record_group_diagnostics(s); return owner;
}
}
