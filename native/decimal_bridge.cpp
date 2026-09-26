#include "decimal_bridge.hpp"
#include "aot_runtime.hpp"
#include <CoreServices/CoreServices.h>
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
void original_num2dec(State& s) {
    // Apple's fp.h defines the same 42-byte decimal record on PPC and ARM64.
    // Only its signed 16-bit exponent changes byte order. Preserve unused
    // bytes, including the exponent for zero, infinity and NaN.
    static_assert(sizeof(decimal) == 42 && offsetof(decimal, exp) == 2 &&
                  offsetof(decimal, sig) == 4 && sizeof(decform) == 4 && SIGDIGLEN == 36);
    const auto form_address = std::uint32_t(s.r<4>(12));
    // CFM's double argument occupies r4/r5 slots but its value is in f1;
    // the output pointer is r6. Verified at original call site 0x1077a06c.
    const auto output_address = std::uint32_t(s.r<4>(24));
    const auto form_bytes = s.memory.copy_out(form_address, 4);
    const decform form{char(form_bytes[0]), char(form_bytes[1]),
                      std::int16_t(read_be<2>(form_bytes.data() + 2))};
    if ((form.style != FLOATDECIMAL && form.style != FIXEDDECIMAL) || form.digits > SIGDIGLEN ||
        (form.style == FLOATDECIMAL && form.digits < 1))
        throw std::runtime_error("Unsupported num2dec format");
    if (s.r<1>(1309)) throw std::runtime_error("num2dec requires IEEE arithmetic");
    auto bytes = s.memory.copy_out(output_address, sizeof(decimal));
    decimal output;
    std::memcpy(&output, bytes.data(), sizeof(output));
    output.exp = std::int16_t(read_be<2>(bytes.data() + 2));
    const auto input = s.r<8>(16400);
    {
        // Inside Macintosh: PowerPC Numerics, Table E-9, specifies no numeric
        // exceptions for num2dec. ARM64's implementation can set FE_INEXACT;
        // mask and discard those host side effects, preserving all original
        // FPSCR flags, enables and the caller's native floating environment.
        // Reference: https://www.vintageapple.org/inside_r/pdf/PPC_Numerics_1994.pdf
        FloatingEnvironment environment;
        constexpr int modes[]{FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD};
        const auto mode = (s.r<1>(1310) << 1) | s.r<1>(1311);
        if (mode > 3 || std::fesetround(modes[mode]))
            throw std::runtime_error("Cannot set original decimal rounding mode");
        ::num2dec(&form, float_value<8>(input), &output);
    }
    if (output.sig.length > SIGDIGLEN) throw std::runtime_error("Invalid native decimal digit count");
    std::memcpy(bytes.data(), &output, sizeof(output));
    write_be<2>(bytes.data() + 2, std::uint16_t(output.exp));
    s.memory.copy_in(output_address, bytes);
}
}
