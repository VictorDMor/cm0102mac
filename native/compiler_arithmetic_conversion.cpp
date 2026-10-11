#include "compiler_arithmetic.hpp"
#include "floating_point.hpp"
#include "mac_abi.hpp"
#include <bit>

namespace cm::aot::compiler_arithmetic {
namespace {
struct ConversionAbi : MacAbi {
    std::uint64_t escapes = 0;
    explicit ConversionAbi(State& state) : MacAbi(state, escapes) {}
    std::uint64_t floating(unsigned n) const { return s.r<8>(0x4000 + n * 16); }
    void floating(unsigned n, std::uint64_t bits) { s.wr<8>(0x4000 + n * 16, bits); }
    void floating_load(unsigned n, std::uint32_t address, std::uint32_t pc) {
        s.pc = pc; floating(n, s.memory.read<8>(address));
    }
    void floating_store(unsigned n, std::uint32_t address, std::uint32_t pc) {
        s.pc = pc; s.memory.write<8>(address, floating(n));
    }
    void negate_pair() {
        const auto low = r(4), upper = r(3); put(4, 0 - low);
        const auto sum = std::uint64_t(~upper) + unsigned(low == 0);
        s.wr<1>(1027, sum >> 32); put(3, std::uint32_t(sum));
    }
    void add_with_carry(unsigned destination, std::uint32_t value) {
        const auto sum = std::uint64_t(r(destination)) + value;
        s.wr<1>(1027, sum >> 32); put(destination, std::uint32_t(sum));
    }
    std::uint32_t left(std::uint32_t value, std::uint32_t count) const {
        count &= 63; return count < 32 ? value << count : 0;
    }
    std::uint32_t right(std::uint32_t value, std::uint32_t count) const {
        count &= 63; return count < 32 ? value >> count : 0;
    }
    void floating_compare(unsigned field, unsigned left_register, unsigned right_register) {
        const auto x = float_value<8>(floating(left_register)), y = float_value<8>(floating(right_register));
        const auto less = x < y, greater = y < x, equal = x == y;
        s.wr<1>(1296, less); s.wr<1>(1297, greater); s.wr<1>(1298, equal);
        s.wr<1>(2304 + field, (less ? 8 : 0) | (greater ? 4 : 0) | (equal ? 2 : 0) | (std::isnan(x) || std::isnan(y)));
    }
    // Preserve the accepted export's bytewise sticky flags and enabled-summary
    // XOR. This is not a host rounding-mode or floating-exception policy.
    void floating_result_status(std::uint64_t result, bool operand_nan, bool single) {
        const auto value = float_value<8>(result);
        s.wr<1>(1296, value < 0); s.wr<1>(1297, 0 < value); s.wr<1>(1298, value == 0);
        s.wr<1>(1299, std::isnan(value));
        if (single) s.wr<1>(1286, s.r<1>(1286) | s.r<1>(1294));
        s.wr<1>(1287, s.r<1>(1287) | operand_nan);
        std::uint8_t invalid = 0;
        for (auto offset : {1287u,1288u,1289u,1290u,1291u,1292u,1301u,1302u,1303u}) invalid |= s.r<1>(offset);
        s.wr<1>(1282, invalid);
        s.wr<1>(1280, s.r<1>(1280) | s.r<1>(1283) | s.r<1>(1284) | s.r<1>(1285) | s.r<1>(1286));
        std::uint8_t enabled = 0;
        for (auto [flag, mask] : {std::pair{1282u,1304u}, {1283u,1305u}, {1284u,1306u}, {1285u,1307u}, {1286u,1308u}})
            enabled ^= s.r<1>(flag) & s.r<1>(mask);
        s.wr<1>(1281, enabled);
    }
    void normalize_signed_pair() {
        put(5, r(3) & 0x80000000); record(r(5)); if (r(5)) negate_pair();
        put(7, r(3) | r(4)); record(r(7)); put(6, 0);
        if (!r(7)) return;
        const auto high = r(3), low = r(4);
        put(7, unsigned(std::countl_zero(high))); put(8, unsigned(std::countl_zero(low)));
        put(9, high ? 0 : r(8)); put(7, r(7) + r(9));
        put(8, 32 - r(7)); s.wr<1>(1027, r(7) <= 32);
        put(9, r(7) - 32); s.wr<1>(1027, r(7) >= 32);
        put(3, left(high, r(7))); put(10, right(low, r(8))); put(3, r(3) | r(10));
        put(10, left(low, r(9))); put(3, r(3) | r(10)); put(4, left(low, r(7)));
        put(6, 0 - r(7)); put(7, r(4) & 0x7ff); compare(r(7), 0x400); put(6, r(6) + 0x43e);
        bool round = r(7) > 0x400;
        if (r(7) == 0x400) { put(7, r(4) & 0x800); record(r(7)); round = r(7) != 0; }
        if (round) {
            add_with_carry(4, 0x800); add_with_carry(3, std::uint32_t(s.r<1>(1027)));
            add_with_carry(6, std::uint32_t(s.r<1>(1027)));
        }
        put(4, std::rotl(r(4), 21));
        put(4, (r(4) & ~0xffe00000u) | (std::rotl(r(3), 21) & 0xffe00000u));
        put(3, std::rotl(r(3), 21) & 0x000fffff); put(6, std::rotl(r(6), 20) & 0xfff00000);
        put(3, r(3) | r(6)); put(3, r(3) | r(5));
    }
};
void signed_pair_to_floating(State& state, bool single) {
    ConversionAbi a(state); a.normalize_signed_pair();
    const auto tail = single ? 0x10773dc8u : 0x10773d18u;
    a.store(a.r(1) + 24, a.r(3), tail); a.store(a.r(1) + 28, a.r(4), tail + 4);
    a.floating_load(1, a.r(1) + 24, tail + 8);
    if (single) {
        const auto input = a.floating(1);
        const auto result = float_bits<8>(float_value<4>(float_bits<4>(float_value<8>(input))));
        a.floating(1, result); a.floating_result_status(result, std::isnan(float_value<8>(input)), true);
    }
    a.finish();
}
}
void signed_pair_to_double(State& state) { signed_pair_to_floating(state, false); }
void signed_pair_to_single(State& state) { signed_pair_to_floating(state, true); }
void double_to_unsigned_word(State& state) {
    ConversionAbi a(state);
    a.load(4, a.r(2) - 0x3b7c, 0x10773780); a.put(3, 0);
    a.floating_load(0, a.r(4), 0x10773788); a.floating_load(3, a.r(4) + 8, 0x1077378c);
    a.floating_load(4, a.r(4) + 16, 0x10773790);
    a.floating_compare(0, 1, 0); a.floating_compare(6, 1, 3);
    if (state.r<1>(2304) & 8) { a.finish(); return; }
    a.put(3, UINT32_MAX); if (!(state.r<1>(2310) & 8)) { a.finish(); return; }
    a.floating_compare(7, 1, 4); a.floating(2, a.floating(1));
    if (!(state.r<1>(2311) & 8)) {
        const auto x = a.floating(1), y = a.floating(4);
        const auto result = float_bits<8>(float_value<8>(x) - float_value<8>(y));
        a.floating(2, result); a.floating_result_status(result, std::isnan(float_value<8>(x)) || std::isnan(float_value<8>(y)), false);
    }
    state.pc = 0x107737b8; original_fctiwz(state, 0x4020, 0x4020);
    a.floating_store(2, a.r(1) - 8, 0x107737bc); a.load(3, a.r(1) - 4, 0x107737c0);
    if (!(state.r<1>(2311) & 8)) a.put(3, a.r(3) + 0x80000000u);
    a.finish();
}
void double_to_signed_pair(State& state) {
    ConversionAbi a(state);
    a.floating_store(1, a.r(1) + 24, 0x10773de0);
    a.load(3, a.r(1) + 24, 0x10773de4); a.load(4, a.r(1) + 28, 0x10773de8);
    a.put(5, std::rotl(a.r(3), 12) & 0x7ff); a.compare(a.r(5), 0x3ff);
    if (a.r(5) < 0x3ff) { a.put(3, 0); a.put(4, 0); a.finish(); return; }
    a.put(6, a.r(3)); a.put(3, (a.r(3) & 0xfffff) | 0x100000);
    a.put(5, a.r(5) - 0x433); a.compare(std::bit_cast<std::int32_t>(a.r(5)));
    const auto high = a.r(3), low = a.r(4);
    if (std::bit_cast<std::int32_t>(a.r(5)) < 0) {
        a.put(5, 0 - a.r(5)); a.put(8, 32 - a.r(5)); state.wr<1>(1027, a.r(5) <= 32);
        a.put(9, a.r(5) - 32); state.wr<1>(1027, a.r(5) >= 32);
        a.put(4, a.right(low, a.r(5))); a.put(10, a.left(high, a.r(8))); a.put(4, a.r(4) | a.r(10));
        a.put(10, a.right(high, a.r(9))); a.put(4, a.r(4) | a.r(10)); a.put(3, a.right(high, a.r(5)));
    } else {
        a.compare(std::bit_cast<std::int32_t>(a.r(5)), 10);
        if (a.r(5) > 10) {
            a.put(6, a.r(6) & 0x80000000); a.record(a.r(6));
            a.put(3, a.r(6) ? 0x80000000 : 0x7fffffff); a.put(4, a.r(6) ? 0 : UINT32_MAX); a.finish(); return;
        }
        a.put(8, 32 - a.r(5)); state.wr<1>(1027, a.r(5) <= 32);
        a.put(9, a.r(5) - 32); state.wr<1>(1027, a.r(5) >= 32);
        a.put(3, a.left(high, a.r(5))); a.put(10, a.right(low, a.r(8))); a.put(3, a.r(3) | a.r(10));
        a.put(10, a.left(low, a.r(9))); a.put(3, a.r(3) | a.r(10)); a.put(4, a.left(low, a.r(5)));
    }
    a.put(6, a.r(6) & 0x80000000); a.record(a.r(6)); if (a.r(6)) a.negate_pair(); a.finish();
}
}
