#pragma once
#include "record_history_adapter.hpp"
#include "record_group_fixtures.hpp"

namespace cm::record_history_checks {
using namespace cm::semantic_io_checks;
inline std::uint32_t prepare(State& s, unsigned index, unsigned kind = 0, unsigned scenario = 0) {
    record_history_diagnostics(s).original = true;
    const auto owner = cm::record_group_checks::prepare(s, 4, kind);
    const auto row = owner + record_history::row_stride;
    for (unsigned n = 0; n < 2; ++n) {
        const auto cached = row + n * record_history::record_stride;
        s.memory.write<4>(cached + 12, s.memory.read<4>(cached + 16));
    }
    const auto first = std::uint32_t(s.memory.read<4>(row + 16));
    if (scenario >= 1 && scenario <= 3) {
        const auto second = cm::utility_object_checks::allocate(s, 0x2fc);
        s.memory.copy_in(second, std::vector<std::uint8_t>(0x2fc, 0));
        if (scenario == 1) for (unsigned field : {0u, 59u}) {
            cm::utility_object_checks::call(s, 0x107735f0, 4);
            const auto text = std::uint32_t(s.r<4>(12));
            require(text != 0, "Actual owned history-field allocation"); s.memory.copy_in(text, {'o','l','d',0});
            s.memory.write<4>(second + 20 + field * 8, text); s.memory.write<1>(second + 24 + field * 8, 0x80);
        }
        s.memory.write<4>(first + 0x1f8, second); s.memory.write<4>(second + 0x1f4, first);
        s.memory.write<4>(row + 12, second); s.memory.write<2>(row + 22, 2);
        if (scenario >= 2) {
            s.memory.write<4>(first, 9); s.memory.write<1>(first + 16, 1);
            s.memory.write<4>(second, scenario == 2 ? 9 : 10); s.memory.write<1>(second + 16, 0x80);
        }
    }
    if (scenario == 4 || scenario == 5) {
        // The original bound compares the live history count with 100 before
        // appending. Use a bounded, well-formed 101-node chain at that boundary.
        auto last = first;
        for (unsigned n = 1; n < 101; ++n) {
            const auto node = cm::utility_object_checks::allocate(s, 0x2fc);
            s.memory.copy_in(node, std::vector<std::uint8_t>(0x2fc, 0));
            s.memory.write<4>(node, n + 20); s.memory.write<4>(node + 0x1f4, last);
            s.memory.write<4>(last + 0x1f8, node); last = node;
        }
        s.memory.write<4>(row + 12, last); s.memory.write<4>(row + 16, last); s.memory.write<2>(row + 22, 101);
        s.memory.write<1>(first + 16, scenario == 4 ? 0x80 : 0);
    }
    if (scenario == 6) {
        for (unsigned field : {4u, 8u, 12u, 16u}) s.memory.write<4>(row + field, 0);
        s.memory.write<2>(row + 22, 0); s.memory.write<4>(owner + 0x2c00, 9);
        s.memory.write<4>(owner + 0x2c04, fixture + 0x40); s.memory.write<1>(owner + 0x2c08, 0x80);
        s.memory.write<4>(owner + 0x2c0c, fixture + 0x240); s.memory.write<4>(owner + 0x2c10, fixture + 0x60);
        s.memory.write<4>(fixture + 0x300, fixture + 0x40); s.memory.write<4>(fixture + 0x304, fixture + 0x60);
        s.memory.write<4>(owner + 0x2c14, fixture + 0x300); s.memory.write<2>(owner + 0x2c18, 2);
    }
    s.wr<4>(12, owner);
    if (index == 0) {
        s.wr<4>(16, kind ? 22 : 0); s.wr<4>(20, 9); s.wr<4>(24, fixture + 0x40);
        s.wr<4>(28, kind ? 0x80 : 0); s.wr<4>(32, kind ? fixture + 0x240 : 0); s.wr<4>(36, fixture + 0x60);
    } else {
        s.wr<4>(16, 9); s.wr<4>(20, fixture + 0x40); s.wr<4>(24, kind || scenario == 2 || scenario == 3 ? 0x80 : 0);
        s.wr<4>(28, kind ? fixture + 0x240 : 0); s.wr<4>(32, fixture + 0x60);
    }
    s.wr<4>(4128, return_sentinel); s.pc = record_history::entries[index];
    reset_record_history_diagnostics(s); return owner;
}
}
