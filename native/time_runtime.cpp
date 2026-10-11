#include "time_runtime.hpp"
#include "mac_abi.hpp"

namespace cm::aot::time_runtime {
namespace {
constexpr std::uint32_t seconds_per_day = 86400, seconds_per_hour = 3600;
std::uint32_t high_product(std::uint32_t a, std::uint32_t b) {
    return std::uint32_t((std::uint64_t(a) * b) >> 32);
}
struct TimeAbi : MacAbi {
    std::uint64_t escapes = 0;
    Execution* execution;
    const std::uint32_t entry, caller;
    TimeAbi(State& state, unsigned index, Execution* policy)
        : MacAbi(state, escapes, entries[index]), execution(policy), entry(entries[index]), caller(std::uint32_t(state.r<4>(4128))) {}
    bool phase(std::uint32_t pc, unsigned maximum) {
        if (!execution) return true;
        if (s.executed > s.budget || maximum > s.budget - s.executed) {
            if (!execution->resume_original) throw std::runtime_error("Missing time-runtime budget continuation");
            s.pc = pc; ++execution->budget_resumes; execution->resume_original(s, caller, entry); return false;
        }
        s.executed += maximum; return true;
    }
    bool equal() const { return s.r<1>(2304) & 2; }
    bool less() const { return s.r<1>(2304) & 8; }
    void call(std::uint32_t entry, std::uint32_t link) {
        s.wr<4>(2440, r(2)); s.wr<4>(4128, link); s.pc = entry;
        if (execution && execution->run_to) execution->run_to(s, link);
        else run_until(s, link);
    }
    void words(unsigned first, std::uint32_t pc, bool restoring) {
        s.wr<4>(2432, r(1) - (32 - first) * 4);
        for (unsigned n = first; n < 32; ++n) {
            const auto address = std::uint32_t(s.r<4>(2432));
            if (restoring) load(n, address, pc); else store(address, r(n), pc);
            s.wr<4>(2432, std::uint32_t(s.r<4>(2432)) + 4);
        }
    }
    void signed_half(unsigned dest, std::uint32_t address, std::uint32_t pc) {
        s.pc = pc; put(dest, std::uint32_t(std::int32_t(std::bit_cast<std::int16_t>(std::uint16_t(s.memory.read<2>(address))))));
    }
    bool finish_pair(std::uint32_t pc) {
        if (!phase(pc, 6)) return false;
        pop(64, pc); restore_link(); restore(31, pc + 12); restore(30, pc + 16); finish(); return true;
    }
};
bool split_day_and_weekday(TimeAbi& a) {
    // The Mac reciprocal products leave observable scratch values. The
    // unsigned day count uses the 1900 epoch modulo word32.
    if (!a.phase(0x1077f784, 21)) return false;
    a.put(3, 0xc22e0000); a.put(0, 0xc22e4507); a.put(0, high_product(a.r(0), a.r(5)));
    a.put(30, a.r(0) >> 16); a.put(3, 0x24920000); a.put(4, a.r(30) + 1);
    a.put(0, 0x24924925); a.put(3, high_product(a.r(0), a.r(4)));
    a.put(0, a.r(4) - a.r(3)); a.put(0, a.r(0) >> 1); a.put(0, a.r(0) + a.r(3));
    a.put(3, 0x10000); a.put(3, seconds_per_day); a.put(0, a.r(0) >> 2);
    a.put(3, a.r(30) * a.r(3)); a.put(0, a.r(0) * 7); a.put(0, a.r(4) - a.r(0));
    a.store(a.r(28) + 24, a.r(0), 0x1077f7c8);
    a.put(29, a.r(5) - a.r(3)); a.put(27, 0);

    return true;
}
bool find_year(TimeAbi& a) {
    // Leap tests receive years since 1900, including the original
    // century residue == 100 condition. Each call retains its own ABI.
    for (;;) {
        if (!a.phase(0x1077f7d8, 2)) return false;
        a.put(3, a.r(27)); a.call(0x1077f6e0, 0x1077f7e0);
        if (!a.phase(0x1077f7e0, 6)) return false;
        a.put(0, 0u - a.r(3)); a.put(0, a.r(0) | a.r(3)); a.put(3, a.r(0) >> 31);
        a.put(0, a.r(3) + 365); a.compare(a.r(30), a.r(0));
        if (a.less()) break;
        if (!a.phase(0x1077f7f8, 3)) return false;
        a.put(30, a.r(30) - a.r(0)); a.put(27, a.r(27) + 1);
    }
    if (!a.phase(0x1077f804, 5)) return false;
    a.store(a.r(28) + 20, a.r(27), 0x1077f804); a.put(3, a.r(27)); a.put(27, 0);
    a.store(a.r(28) + 28, a.r(30), 0x1077f810); a.call(0x1077f6e0, 0x1077f818);

    return true;
}
bool find_month(TimeAbi& a) {
    if (!a.phase(0x1077f818, 4)) return false;
    a.put(0, a.r(3) * 26); a.put(4, a.r(31) + a.r(0)); a.put(3, 0);
    // Both cumulative-month rows are live, signed big-endian halfwords.
    // A malformed row retains original traversal/fault/budget behavior.
    for (;;) {
        if (!a.phase(0x1077f828, 4)) return false;
        a.put(0, a.r(3) + 2); a.signed_half(0, a.r(4) + a.r(0), 0x1077f82c);
        a.compare(a.r(30), a.r(0));
        if (a.less()) {
            if (!a.phase(0x1077f838, 4)) return false;
            a.put(0, a.r(27) << 1); a.signed_half(0, a.r(4) + a.r(0), 0x1077f83c);
            a.put(30, a.r(30) - a.r(0)); break;
        }
        if (!a.phase(0x1077f848, 3)) return false;
        a.put(3, a.r(3) + 2); a.put(27, a.r(27) + 1);
    }

    return true;
}
bool publish_clock_fields(TimeAbi& a) {
    auto& s = a.s;
    if (!a.phase(0x1077f854, 20)) return false;
    a.put(3, seconds_per_hour); a.store(a.r(28) + 16, a.r(27), 0x1077f858);
    s.pc = 0x1077f85c; a.put(0, std::uint32_t(divide_unsigned(a.r(29), a.r(3)))); a.put(0, a.r(0) * a.r(3));
    a.put(3, 0x91a30000); a.put(3, 0x91a2b3c5); a.put(4, high_product(a.r(3), a.r(29)));
    a.put(3, 0x88890000); a.put(5, a.r(30) + 1); a.put(29, a.r(29) - a.r(0));
    a.store(a.r(28) + 12, a.r(5), 0x1077f87c); a.put(0, 0x88888889);
    a.put(0, high_product(a.r(0), a.r(29))); a.put(4, a.r(4) >> 11); a.put(3, a.r(0) >> 5);
    a.store(a.r(28) + 8, a.r(4), 0x1077f890); a.put(0, a.r(3) * 60);
    a.store(a.r(28) + 4, a.r(3), 0x1077f898); a.put(0, a.r(29) - a.r(0));
    a.store(a.r(28), a.r(0), 0x1077f8a0);
    return true;
}
}
void zero_status(State& s, Execution* execution) {
    TimeAbi a(s, 0, execution); if (!a.phase(entries[0], 2)) return;
    a.put(3, 0); a.finish();
}
void leap_year_offset(State& s, Execution* execution) {
    TimeAbi a(s, 1, execution);
    if (!a.phase(entries[1], 9)) return;
    a.save(31, 0x1077f6e0); a.capture_link(); a.put(31, 0); a.save(30, 0x1077f6ec);
    a.put(30, a.r(3)); a.put(4, 4); a.save_link(0x1077f6f8); a.frame(64, 0x1077f6fc);
    a.call(0x10775560, 0x1077f704);
    if (!a.phase(0x1077f704, 2)) return;
    a.compare(std::bit_cast<std::int32_t>(a.r(3)));
    if (a.equal()) {
        if (!a.phase(0x1077f70c, 3)) return;
        a.put(3, a.r(30)); a.put(4, 100); a.call(0x10775560, 0x1077f718);
        if (!a.phase(0x1077f718, 2)) return;
        a.compare(std::bit_cast<std::int32_t>(a.r(3)));
        if (!a.equal()) {
            if (!a.phase(0x1077f734, 1)) return;
            a.put(31, 1);
        } else {
            if (!a.phase(0x1077f720, 3)) return;
            a.put(3, a.r(30)); a.put(4, 400); a.call(0x10775560, 0x1077f72c);
            if (!a.phase(0x1077f72c, 2)) return;
            a.compare(std::bit_cast<std::int32_t>(a.r(3)), 100);
            if (a.equal()) { if (!a.phase(0x1077f734, 1)) return; a.put(31, 1); }
        }
    }
    if (!a.phase(0x1077f738, 1)) return;
    a.put(3, a.r(31)); a.finish_pair(0x1077f73c);
}
void calendar_fields(State& s, Execution* execution) {
    TimeAbi a(s, 2, execution);
    if (!a.phase(entries[2], 9)) return;
    a.words(27, 0x1077f760, false); a.capture_link(); a.put(5, a.r(3) - 0x7c560000);
    a.load(31, a.r(2) - 0x5a28, 0x1077f76c); a.put(28, a.r(4)); a.record(a.r(28));
    a.put(5, a.r(5) + 0x7e80); a.save_link(0x1077f778); a.frame(80, 0x1077f77c);
    if (!a.equal()) {
        if (!split_day_and_weekday(a) || !find_year(a) || !find_month(a) || !publish_clock_fields(a)) return;
    }
    if (!a.phase(0x1077f8a4, 5)) return;
    a.pop(80, 0x1077f8a4); a.words(27, 0x1077f8ac, true); a.restore_link(); a.finish();
}
void initialize_fields(State& s, Execution* execution) {
    TimeAbi a(s, 3, execution); if (!a.phase(entries[3], 13)) return;
    a.put(5, 0); a.store(a.r(3), a.r(5), 0x1077f8c4); a.put(4, 1); a.put(0, UINT32_MAX);
    constexpr std::array<unsigned, 7> offsets{4, 8, 12, 16, 20, 24, 28};
    for (auto offset : offsets) a.store(a.r(3) + offset, a.r(offset == 12 || offset == 24 ? 4 : 5), 0x1077f8cc + offset);
    a.store(a.r(3) + 32, a.r(0), 0x1077f8ec); a.finish();
}
void time_value(State& s, Execution* execution) {
    TimeAbi a(s, 4, execution); if (!a.phase(entries[4], 6)) return;
    a.save(31, 0x1077f900); a.capture_link(); a.put(31, a.r(3)); a.save_link(0x1077f90c); a.frame(64, 0x1077f910);
    a.call(0x1077f9a0, 0x1077f918);
    if (!a.phase(0x1077f918, 2)) return;
    a.compare(a.r(31));
    if (!a.equal()) { if (!a.phase(0x1077f920, 1)) return; a.store(a.r(31), a.r(3), 0x1077f920); }
    if (!a.phase(0x1077f924, 5)) return;
    a.pop(64, 0x1077f924); a.restore(31, 0x1077f92c); a.restore_link(); a.finish();
}
void local_fields(State& s, Execution* execution) {
    TimeAbi a(s, 5, execution); if (!a.phase(entries[5], 7)) return;
    a.save(31, 0x1077f940); a.capture_link(); a.compare(a.r(3));
    a.load(31, a.r(2) - 0x3b20, 0x1077f94c); a.save_link(0x1077f950); a.frame(64, 0x1077f954);
    if (a.equal()) {
        if (!a.phase(0x1077f95c, 2)) return;
        a.put(3, a.r(31)); a.call(0x1077f8c0, 0x1077f964);
        if (!a.phase(0x1077f964, 1)) return;
    } else {
        if (!a.phase(0x1077f968, 3)) return;
        a.load(3, a.r(3), 0x1077f968); a.put(4, a.r(31)); a.call(0x1077f760, 0x1077f974);
    }
    if (!a.phase(0x1077f974, 1)) return;
    a.call(0x1077f9e0, 0x1077f978);
    if (!a.phase(0x1077f978, 7)) return;
    a.store(a.r(31) + 32, a.r(3), 0x1077f978); a.put(3, a.r(31)); a.pop(64, 0x1077f980);
    a.restore_link(); a.restore(31, 0x1077f98c); a.finish();
}
void mac_clock_value(State& s, Execution* execution) {
    TimeAbi a(s, 6, execution); if (!a.phase(entries[6], 5)) return;
    a.capture_link(); a.save_link(0x1077f9a4); a.frame(64, 0x1077f9a8); a.put(3, a.r(1) + 56);
    a.call(0x10780900, 0x1077f9b4);
    if (!a.phase(0x1077f9b4, 8)) return;
    a.load(2, a.r(1) + 20, 0x1077f9b4); a.load(3, a.r(1) + 56, 0x1077f9b8);
    a.put(3, a.r(3) - 0x7c260000); a.put(3, a.r(3) + 0x4f80);
    a.pop(64, 0x1077f9c4); a.restore_link(); a.finish();
}
void daylight_status(State& s, Execution* execution) {
    TimeAbi a(s, 7, execution); if (!a.phase(entries[7], 5)) return;
    a.capture_link(); a.save_link(0x1077f9e4); a.frame(80, 0x1077f9e8); a.put(3, a.r(1) + 56);
    a.call(0x10781068, 0x1077f9f4);
    if (!a.phase(0x1077f9f4, 4)) return;
    a.load(2, a.r(1) + 20, 0x1077f9f4); a.load(0, a.r(1) + 56, 0x1077f9f8);
    a.compare(std::bit_cast<std::int32_t>(a.r(0)));
    bool location = !a.equal();
    if (!location) {
        if (!a.phase(0x1077fa04, 3)) return;
        a.load(0, a.r(1) + 60, 0x1077fa04); a.compare(std::bit_cast<std::int32_t>(a.r(0))); location = !a.equal();
    }
    if (!location) {
        if (!a.phase(0x1077fa10, 3)) return;
        a.load(0, a.r(1) + 64, 0x1077fa10); a.compare(std::bit_cast<std::int32_t>(a.r(0))); location = !a.equal();
    }
    if (!location) {
        if (!a.phase(0x1077fa1c, 2)) return;
        a.put(3, UINT32_MAX);
    } else {
        if (!a.phase(0x1077fa24, 5)) return;
        a.load<1>(0, a.r(1) + 64, 0x1077fa24);
        a.put(3, std::uint32_t(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(a.r(0))))));
        a.put(0, 0u - a.r(3)); a.put(0, a.r(0) | a.r(3)); a.put(3, a.r(0) >> 31);
    }
    if (!a.phase(0x1077fa38, 4)) return;
    a.pop(80, 0x1077fa38); a.restore_link(); a.finish();
}
void invoke(State& s, unsigned index, Execution* execution) {
    switch (index) {
    case 0: zero_status(s, execution); break;
    case 1: leap_year_offset(s, execution); break;
    case 2: calendar_fields(s, execution); break;
    case 3: initialize_fields(s, execution); break;
    case 4: time_value(s, execution); break;
    case 5: local_fields(s, execution); break;
    case 6: mac_clock_value(s, execution); break;
    case 7: daylight_status(s, execution); break;
    default: throw std::runtime_error("Unknown time-runtime entry");
    }
}
}
