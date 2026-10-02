#pragma once
#include "aot_runtime.hpp"

namespace cm::aot {
// Consume the next original Mac UI control-cursor entry. This replaces only
// the audited 0x1071a828..0x1071a860 block. It does not select football actors
// or advance the match simulation; the record layout belongs to the UI.
template <class Counter> void next_control_cursor_entry(State& s, Counter& counter) {
    counter.step(0x1071a828);
    const auto record_index = std::uint32_t(signed_value<2>(
        s.memory.read<2>(std::uint32_t(s.r<4>(112) + s.r<4>(76)))));
    s.wr<4>(0, record_index);

    counter.step(0x1071a82c);
    s.wr<4>(12, record_index * UINT64_C(3056));
    counter.step(0x1071a830);
    s.wr<4>(16, s.r<4>(12) + 4);
    counter.step(0x1071a834);
    s.wr<4>(16, s.r<4>(112) + s.r<4>(16));
    const auto record = std::uint32_t(s.r<4>(16));

    counter.step(0x1071a838);
    const auto consumed = std::uint32_t(signed_value<2>(
        s.memory.read<2>(std::uint32_t(record + 2936))));
    s.wr<4>(12, consumed);
    counter.step(0x1071a83c);
    const auto available = std::uint32_t(signed_value<2>(
        s.memory.read<2>(std::uint32_t(record + 2932))));
    s.wr<4>(0, available);

    counter.step(0x1071a840);
    const auto left = signed_value<4>(consumed);
    const auto right = signed_value<4>(available);
    const auto flags = std::uint8_t((left < right ? 8 : left > right ? 4 : 2) |
                                    (s.r<1>(1024) & 1));
    s.wr<1>(2304, flags);
    counter.step(0x1071a844);
    if (left >= right) {
        counter.step(0x1071a860);
        s.wr<4>(92, UINT32_MAX);
        return;
    }

    counter.step(0x1071a848);
    s.wr<4>(0, consumed + 1);
    counter.step(0x1071a84c);
    s.wr<4>(12, std::uint32_t(consumed << 1) & ~1u);
    counter.step(0x1071a850);
    s.memory.write<2>(std::uint32_t(record + 2936), s.r<2>(2));
    counter.step(0x1071a854);
    s.wr<4>(0, s.r<4>(12) + 530);
    counter.step(0x1071a858);
    s.wr<4>(92, std::uint64_t(signed_value<2>(
        s.memory.read<2>(std::uint32_t(record + s.r<4>(0))))));
    counter.step(0x1071a85c);
}
}
