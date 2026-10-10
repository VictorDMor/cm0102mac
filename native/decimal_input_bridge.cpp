#include "decimal_input_bridge.hpp"
#include "aot_runtime.hpp"
#include <CoreServices/CoreServices.h>
#include <algorithm>
#include <cfenv>
#include <cstring>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

namespace cm::aot {
namespace {
class FloatingEnvironment {
    std::fenv_t saved_{};
public:
    FloatingEnvironment() {
        if (std::feholdexcept(&saved_)) throw std::runtime_error("Cannot save native floating environment");
    }
    ~FloatingEnvironment() { std::fesetenv(&saved_); }
};
}
void original_dec2num(State& s) {
    static_assert(sizeof(decimal) == 42 && offsetof(decimal, exp) == 2 &&
                  offsetof(decimal, sig) == 4 && SIGDIGLEN == 36);
    const auto bytes = s.memory.copy_out(std::uint32_t(s.r<4>(12)), sizeof(decimal));
    decimal input;
    std::memcpy(&input, bytes.data(), sizeof(input));
    input.exp = std::int16_t(read_be<2>(bytes.data() + 2));
    if (input.sig.length > SIGDIGLEN) throw std::runtime_error("Undefined dec2num decimal digit count");
    if (s.r<1>(1309)) throw std::runtime_error("dec2num requires IEEE arithmetic");
    double value;
    {
        // PowerPC Numerics, Table E-9, specifies no exceptions, including
        // inexact/overflow/underflow. ARM64 may set them. Discard native flags
        // and restore the caller's environment; leave the entire guest FPSCR.
        FloatingEnvironment environment;
        constexpr int modes[]{FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD};
        const auto mode = (s.r<1>(1310) << 1) | s.r<1>(1311);
        if (mode > 3 || std::fesetround(modes[mode]))
            throw std::runtime_error("Cannot set original decimal rounding mode");
        value = ::dec2num(&input);
    }
    s.wr<8>(16400, float_bits<8>(value));
}
void original_nan(State& s) {
    // MathLib's code is the eighth through fifteenth most significant bits
    // of the fraction (double bits 44..37), not the host nan() low payload.
    // Read the complete NUL-terminated guest tag before interpreting it.
    std::vector<std::uint8_t> tag;
    auto address = std::uint32_t(s.r<4>(12));
    for (;;) {
        const auto c = std::uint8_t(s.memory.read<1>(address++));
        if (!c) break;
        tag.push_back(c);
    }
    unsigned cursor = 0;
    while (cursor < tag.size() && (tag[cursor] == ' ' || (tag[cursor] >= 9 && tag[cursor] <= 13))) ++cursor;
    bool negative = false;
    if (cursor < tag.size() && (tag[cursor] == '+' || tag[cursor] == '-')) negative = tag[cursor++] == '-';
    unsigned code = 0;
    while (cursor < tag.size() && tag[cursor] >= '0' && tag[cursor] <= '9') {
        code = std::min(255u, code * 10 + tag[cursor++] - '0');
    }
    if (negative) code = 0;
    s.wr<8>(16400, UINT64_C(0x7ff8000000000000) | (std::uint64_t(code) << 37));
}
}
