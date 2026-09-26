#include "aot_runtime.hpp"
#include <CoreServices/CoreServices.h>
#include <algorithm>
#include <cfenv>
#include <cstring>
#include <iostream>

#pragma clang diagnostic ignored "-Wdeprecated-declarations"

using namespace cm::aot;
namespace {
constexpr std::uint32_t base = 0x30000000, form_address = base + 4, output_address = base + 20;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
unsigned bridge_checks() {
    auto owner = std::make_unique<State>();
    auto& s = *owner;
    s.memory.map(base, 128);
    constexpr int modes[]{FE_TONEAREST, FE_TOWARDZERO, FE_UPWARD, FE_DOWNWARD};
    const double values[]{0, -0.0, 1.25, -1.25, 1.75, -1.75, 9.99, 12.375, -123456.75,
        1e-30, 1e30, std::numeric_limits<double>::min(), std::numeric_limits<double>::denorm_min(),
        std::numeric_limits<double>::max(), std::numeric_limits<double>::infinity(),
        -std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN(),
        float_value<8>(UINT64_C(0xfff8000000000001)), float_value<8>(UINT64_C(0x7ff0000000000001))};
    std::fenv_t saved{};
    require(!std::fegetenv(&saved), "Cannot save test floating environment");
    unsigned count = 0;
    for (unsigned rounding = 0; rounding < 4; ++rounding) {
        for (char style : {FLOATDECIMAL, FIXEDDECIMAL}) {
            for (short precision : {short(1), short(2), short(6), short(17), short(32), short(36), short(0), short(-2)}) {
                if (style == FLOATDECIMAL && precision <= 0) continue;
                for (double value : values) {
                    s.registers.fill(0);
                    s.memory.copy_in(base, std::vector<std::uint8_t>(128, 0xa5));
                    s.memory.write<1>(form_address, style);
                    s.memory.write<2>(form_address + 2, std::uint16_t(precision));
                    s.wr<4>(12, form_address);
                    s.wr<4>(16, 0xdead0000); // Deliberately invalid r4/r5, not the output pointer.
                    s.wr<4>(20, 0xdead0004);
                    s.wr<4>(24, output_address);
                    s.wr<8>(16384, float_bits<8>(7.25));
                    s.wr<8>(16400, float_bits<8>(value));
                    s.wr<8>(16416, float_bits<8>(-3.5));
                    // The documented conversion raises no original numeric
                    // exceptions, regardless of pre-existing flags/enables.
                    for (unsigned flag = 1280; flag < 1309; ++flag)
                        s.wr<1>(flag, (count + flag) % 2);
                    s.wr<1>(1310, rounding >> 1); s.wr<1>(1311, rounding & 1);
                    s.wr<4>(4128, return_sentinel);
                    s.pc = 0x10bf7b10;
                    const auto registers = s.registers;
                    const auto before = s.memory.copy_out(base, 128);
                    decimal expected;
                    std::memset(&expected, 0xa5, sizeof(expected));
                    const decform form{style, char(0xa5), precision};
                    std::fesetround(modes[rounding]);
                    std::feclearexcept(FE_ALL_EXCEPT);
                    ::num2dec(&form, value, &expected);
                    std::fesetround(modes[(rounding + 1) % 4]);
                    std::feclearexcept(FE_ALL_EXCEPT);
                    std::feraiseexcept(FE_DIVBYZERO);
                    require(bridge_import(s, "CarbonLib::num2dec"), "num2dec bridge not dispatched");
                    require(std::fegetround() == modes[(rounding + 1) % 4] &&
                            std::fetestexcept(FE_ALL_EXCEPT) == FE_DIVBYZERO,
                            "num2dec changed its caller's native floating environment");
                    require(s.pc == return_sentinel, "num2dec return address");
                    std::vector<std::uint8_t> bytes(sizeof(expected));
                    std::memcpy(bytes.data(), &expected, bytes.size());
                    write_be<2>(bytes.data() + 2, std::uint16_t(expected.exp));
                    require(s.memory.copy_out(output_address, bytes.size()) == bytes,
                            "num2dec differs from native conversion or original big-endian record layout");
                    auto after = s.memory.copy_out(base, 128);
                    std::copy(before.begin() + 20, before.begin() + 62, after.begin() + 20);
                    require(after == before, "num2dec wrote outside its 42-byte output");
                    require(s.registers == registers, "num2dec changed original registers or floating status");
                    ++count;
                }
            }
        }
    }
    // Refuse an undefined precision before changing the destination.
    const auto before = s.memory.copy_out(output_address, 42);
    s.memory.write<2>(form_address + 2, 37);
    bool rejected = false;
    try { bridge_import(s, "CarbonLib::num2dec"); }
    catch (const std::runtime_error&) { rejected = true; }
    require(rejected && s.memory.copy_out(output_address, 42) == before, "Undefined decimal precision was silently accepted");
    std::fesetenv(&saved);
    return count;
}
unsigned original_formatting(const char* program) {
    auto owner = std::make_unique<State>();
    auto& s = *owner;
    load_program(s, program);
    s.memory.map(base, 0x10000);
    constexpr auto text = base + 0x100, format = base + 0x1000, arguments = base + 0x2000;
    struct Example { const char* format; double value; const char* expected; };
    const Example examples[]{
        {"%.2f", 7.25, "7.25"}, {"%.2f", -7.25, "-7.25"},
        {"%.0f", 1.5, "2"}, {"%.0f", 2.5, "2"}, {"%+.1f", 12.375, "+12.4"},
        {"%.2f", -0.0, "-0.00"}, {"%08.2f", 1.25, "00001.25"},
        {"%.2e", 1234.0, "1.23e+03"}, {"%.4g", 0.000012345, "1.234e-05"},
        {"%.4g", 1234567.0, "1.235e+06"}, {"%.2f", 0.0, "0.00"},
        {"%.2f", 99.999, "100.00"}, {"%.2f", std::numeric_limits<double>::infinity(), "inf"},
        {"%.2f", std::numeric_limits<double>::quiet_NaN(), "nan"},
    };
    unsigned count = 0;
    for (const auto& example : examples) {
        const auto format_string = std::string(example.format);
        s.memory.copy_in(format, {format_string.begin(), format_string.end()});
        s.memory.write<1>(format + format_string.size(), 0);
        s.memory.copy_in(text, std::vector<std::uint8_t>(128, 0xa5));
        s.memory.write<8>(arguments, float_bits<8>(example.value));
        s.wr<4>(4, 0x700fff00); s.wr<4>(8, 0x108bba90);
        s.wr<4>(12, text); s.wr<4>(16, 100); s.wr<4>(20, format); s.wr<4>(24, arguments);
        s.wr<4>(4128, return_sentinel);
        s.pc = 0x1077b680; // Original bounded varargs formatter used by the player panel.
        s.budget = s.executed + 100000;
        try { run(s); }
        catch (const std::exception& error) {
            throw std::runtime_error(std::string(example.format) + " at " + hex_address(s.pc) + ": " + error.what());
        }
        const auto bytes = s.memory.copy_out(text, 128);
        const auto end = std::find(bytes.begin(), bytes.end(), 0);
        const std::string actual(bytes.begin(), end);
        if (actual != example.expected)
            throw std::runtime_error(std::string(example.format) + ": expected " + example.expected + ", got " + actual);
        require(s.r<4>(12) == actual.size() && s.r<4>(4) == 0x700fff00 && s.r<4>(8) == 0x108bba90,
                "Original decimal formatter length, stack or TOC");
        require(std::all_of(end + 1, bytes.end(), [](auto byte) { return byte == 0xa5; }),
                "Original formatter changed output guard bytes");
        ++count;
    }
    return count;
}
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-decimal-checks <original-segments>\n"; return 2; }
    try {
        const auto conversions = bridge_checks();
        const auto formats = original_formatting(argv[1]);
        std::cout << conversions << " decimal ABI cases and " << formats << " original formatting cases passed\n";
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
