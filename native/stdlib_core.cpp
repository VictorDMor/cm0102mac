#include "stdlib_core.hpp"
#include "mac_abi.hpp"

namespace cm::aot::stdlib_core {
namespace {
struct SearchAbi : MacAbi {
    std::uint64_t escapes = 0;
    SearchExecution* execution;
    const std::uint32_t caller;
    SearchAbi(State& state, SearchExecution* policy)
        : MacAbi(state, escapes, 0x10775600), execution(policy), caller(std::uint32_t(state.r<4>(4128))) {}
    bool checkpoint(std::uint32_t pc, std::uint64_t maximum) {
        if (!execution) return true;
        if (s.executed <= s.budget && maximum <= s.budget - s.executed) return true;
        if (!execution->resume_original) throw std::runtime_error("Missing binary-search budget continuation");
        s.pc = pc; ++execution->budget_resumes; execution->resume_original(s, caller); return false;
    }
    void charge(std::uint64_t count) { if (execution) s.executed += count; }
    void words(unsigned first, std::uint32_t pc, bool restoring) {
        s.wr<4>(2432, r(1) - (32 - first) * 4);
        for (unsigned n = first; n < 32; ++n) {
            const auto address = std::uint32_t(s.r<4>(2432));
            if (restoring) load(n, address, pc); else store(address, r(n), pc);
            s.wr<4>(2432, std::uint32_t(s.r<4>(2432)) + 4);
        }
    }
    void finish_frame() {
        if (!checkpoint(0x107756dc, 5)) return;
        charge(5); load(0, r(1) + 104, 0x107756dc); put(1, r(1) + 96);
        words(24, 0x107756e4, true); restore_link(); finish();
    }
};
}
void absolute_word(State& state) {
    const auto value = std::uint32_t(state.r<4>(12));
    const auto sign = std::bit_cast<std::int32_t>(value) < 0 ? UINT32_MAX : 0u;
    state.wr<1>(1027, sign != 0 && (value & 0x7fffffff) != 0);
    state.wr<4>(16, sign); state.wr<4>(0, sign ^ std::uint32_t(state.r<4>(12)));
    state.wr<4>(12, std::uint32_t(state.r<4>(0)) - std::uint32_t(state.r<4>(16)));
    state.pc = std::uint32_t(state.r<4>(4128));
}
void absolute_long_word(State& state) { absolute_word(state); }
void floor_remainder(State& state, bool account) {
    std::uint64_t escapes = 0; MacAbi a(state, escapes, 0x10775560);
    const bool numerator_negative = std::bit_cast<std::int32_t>(a.r(3)) < 0;
    const bool denominator_negative = std::bit_cast<std::int32_t>(a.r(4)) < 0;
    a.compare(std::bit_cast<std::int32_t>(a.r(3))); a.put(6, 1); a.put(7, 1);
    if (std::bit_cast<std::int32_t>(a.r(3)) < 0) { a.put(3, 0u - a.r(3)); a.put(6, UINT32_MAX); }
    a.compare(std::bit_cast<std::int32_t>(a.r(4)));
    if (std::bit_cast<std::int32_t>(a.r(4)) < 0) { a.put(4, 0u - a.r(4)); a.put(7, UINT32_MAX); }
    if (account) { state.executed += 6 + 2 * unsigned(numerator_negative) + 2 * unsigned(denominator_negative); state.step(0x10775588); }
    else state.pc = 0x10775588;
    a.put(0, std::uint32_t(divide_signed<4>(a.r(3), a.r(4))));
    a.put(8, a.r(6) * a.r(7)); a.put(0, a.r(8) * a.r(0)); a.put(0, a.r(0) * a.r(4));
    a.put(5, a.r(7) * a.r(0)); a.put(0, a.r(3) * a.r(6));
    a.put(3, a.r(0) - a.r(5)); a.record(a.r(3));
    if (account) state.executed += 7;
    if (!(state.r<1>(2304) & 2)) {
        if (account) state.executed += 2;
        a.compare(std::bit_cast<std::int32_t>(a.r(8)));
        if (std::bit_cast<std::int32_t>(a.r(8)) < 0) {
            if (account) state.executed += 3;
            a.put(0, a.r(4) * a.r(7)); a.put(3, a.r(3) + a.r(0));
        }
    }
    a.finish();
}
void binary_search(State& state, SearchExecution* execution) {
    SearchAbi a(state, execution);
    if (!a.checkpoint(0x10775600, 10)) return;
    a.charge(10); a.words(24, 0x10775600, false); a.capture_link();
    a.put(24, a.r(3)); a.record(a.r(24)); a.put(25, a.r(4)); a.put(29, a.r(5));
    a.put(26, a.r(6)); a.put(27, a.r(7)); a.save_link(0x1077561c); a.frame(96, 0x10775620);
    bool valid = !(state.r<1>(2304) & 2);
    if (valid) {
        constexpr unsigned arguments[]{25, 29, 26, 27};
        for (unsigned n = 0; n < 4; ++n) {
            if (!a.checkpoint(0x10775628 + n * 8, 2)) return;
            a.charge(2); a.compare(a.r(arguments[n]));
            if (state.r<1>(2304) & 2) { valid = false; break; }
        }
    }
    if (!valid) {
        if (!a.checkpoint(0x10775648, 2)) return;
        a.charge(2); a.put(3, 0); a.finish_frame(); return;
    }
    // Probe element zero before constructing the [1,count-1] interval. Its
    // live arguments and comparator clobbers are part of the Mac contract.
    if (!a.checkpoint(0x10775650, 3)) return;
    a.charge(3); a.put(28, a.r(25)); a.put(12, a.r(27)); a.call(0x10781320, 0x1077565c);
    if (!a.checkpoint(0x1077565c, 7)) return;
    a.charge(3); a.load(2, a.r(1) + 20, 0x1077565c); a.compare(std::bit_cast<std::int32_t>(a.r(3)));
    if (state.r<1>(2304) & 2) {
        a.charge(2); a.put(3, a.r(28)); a.finish_frame(); return;
    }
    a.charge(1); // The signed branch after a nonzero first comparison.
    if (state.r<1>(2304) & 8) {
        a.charge(2); a.put(3, 0); a.finish_frame(); return;
    }
    a.charge(3); a.put(29, a.r(29) - 1); a.put(30, 1);
    for (;;) {
        if (!a.checkpoint(0x107756d0, 2)) return;
        a.charge(2); a.compare(a.r(30), a.r(29));
        if (!(state.r<1>(2304) & 10)) {
            if (!a.checkpoint(0x107756d8, 1)) return;
            a.charge(1); a.put(3, 0); break;
        }
        if (!a.checkpoint(0x10775688, 8)) return;
        a.charge(8); a.put(0, a.r(30) + a.r(29)); a.put(28, a.r(0) >> 1);
        a.put(0, a.r(26) * a.r(28)); a.put(31, a.r(25) + a.r(0));
        a.put(3, a.r(24)); a.put(4, a.r(31)); a.put(12, a.r(27)); a.call(0x10781320, 0x107756a8);
        if (!a.checkpoint(0x107756a8, 6)) return;
        a.charge(3); a.load(2, a.r(1) + 20, 0x107756a8); a.compare(std::bit_cast<std::int32_t>(a.r(3)));
        if (state.r<1>(2304) & 2) { a.charge(2); a.put(3, a.r(31)); break; }
        a.charge(1);
        if (state.r<1>(2304) & 8) { a.charge(2); a.put(29, a.r(28) - 1); }
        else { a.charge(1); a.put(30, a.r(28) + 1); }
    }
    a.finish_frame();
}
}
