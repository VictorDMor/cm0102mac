#include "aot_runtime.hpp"
#include "floating_point.hpp"
#include <iostream>

using namespace cm::aot;
namespace {
constexpr unsigned destination = 16416, source = 16480;
constexpr std::uint32_t instruction = 0x1022cd24;
void require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}
void execute(State& s, const std::string& expected = "Native instruction budget exhausted") {
    s.pc = instruction;
    s.executed = 0;
    s.budget = 1;
    bool stopped = false;
    try { run(s); }
    catch (const std::runtime_error& error) {
        require(error.what() == expected, std::string("Unexpected conversion stop: ") + error.what());
        stopped = true;
    }
    require(stopped, "Missing expected instruction boundary");
    require(s.pc == instruction + (expected == "Native instruction budget exhausted" ? 4 : 0),
            "Conversion stopped at the wrong original instruction");
}
struct Case {
    const char* name;
    std::uint64_t input;
    std::uint32_t output;
    bool invalid = false, inexact = false, signaling = false;
};
}
int main() {
    try {
        // Expected low words and exception flags follow the PowerPC manual,
        // Appendix D.4.2; these exercise the actual instruction that stopped
        // league initialization, not just the new C++ helper in isolation.
        const Case cases[] = {
            {"positive zero", 0, 0},
            {"negative zero", UINT64_C(0x8000000000000000), 0},
            {"positive subnormal", 1, 0, false, true},
            {"negative subnormal", UINT64_C(0x8000000000000001), 0, false, true},
            {"positive fraction", float_bits<8>(0.9), 0, false, true},
            {"negative fraction", float_bits<8>(-0.9), 0, false, true},
            {"positive truncation", float_bits<8>(1.9), 1, false, true},
            {"negative truncation", float_bits<8>(-1.9), 0xffffffffu, false, true},
            {"positive integer", float_bits<8>(123456789.0), 123456789},
            {"negative integer", float_bits<8>(-1.0), 0xffffffffu},
            {"maximum integer", float_bits<8>(2147483647.0), 0x7fffffffu},
            {"minimum integer", float_bits<8>(-2147483648.0), 0x80000000u},
            {"upper boundary truncation", float_bits<8>(2147483647.75), 0x7fffffffu, false, true},
            {"lower boundary truncation", float_bits<8>(-2147483648.75), 0x80000000u, false, true},
            {"positive overflow", float_bits<8>(2147483648.0), 0x7fffffffu, true},
            {"negative overflow", float_bits<8>(-2147483649.0), 0x80000000u, true},
            {"positive huge", float_bits<8>(1e300), 0x7fffffffu, true},
            {"negative huge", float_bits<8>(-1e300), 0x80000000u, true},
            {"positive infinity", UINT64_C(0x7ff0000000000000), 0x7fffffffu, true},
            {"negative infinity", UINT64_C(0xfff0000000000000), 0x80000000u, true},
            {"positive quiet NaN", UINT64_C(0x7ff8000000001234), 0x80000000u, true},
            {"negative quiet NaN", UINT64_C(0xfff8000000001234), 0x80000000u, true},
            {"positive signaling NaN", UINT64_C(0x7ff0000000001234), 0x80000000u, true, false, true},
            {"negative signaling NaN", UINT64_C(0xfff0000000001234), 0x80000000u, true, false, true},
        };
        unsigned checked = 0;
        auto state = std::make_unique<State>();
        auto& s = *state;
        for (const auto& item : cases) for (unsigned mode = 0; mode < 4; ++mode) {
            s.registers.fill(0);
            s.wr<8>(source, item.input);
            s.wr<8>(destination, UINT64_C(0x1234567887654321));
            s.wr<1>(1293, 1);
            s.wr<1>(1294, 1);
            s.wr<1>(1310, mode >> 1);
            s.wr<1>(1311, mode & 1);
            s.wr<4>(0, 0xaabbccddu);
            const auto before = s.registers;
            execute(s);
            const std::string label = std::string(item.name) + " RN=" + std::to_string(mode);
            require(std::uint32_t(s.r<8>(destination)) == item.output, label + ": integer result");
            require(s.r<1>(1303) == item.invalid && s.r<1>(1282) == item.invalid,
                    label + ": invalid conversion flags");
            require(s.r<1>(1287) == item.signaling, label + ": signaling NaN flag");
            require(s.r<1>(1286) == item.inexact && s.r<1>(1294) == item.inexact,
                    label + ": inexact flags");
            require(s.r<1>(1280) == (item.invalid || item.inexact) && !s.r<1>(1281),
                    label + ": exception summary");
            require(!s.r<1>(1293), label + ": round direction");
            for (unsigned i = 0; i < before.size(); ++i) {
                if ((i >= destination && i < destination + 8) || i == 1280 || i == 1281 ||
                    i == 1282 || i == 1286 || i == 1287 || i == 1293 || i == 1294 || i == 1303) continue;
                require(s.registers[i] == before[i], label + ": changed unrelated register " + std::to_string(i));
            }
            ++checked;
        }
        s.registers.fill(0);
        s.wr<8>(source, float_bits<8>(1.5));
        execute(s);
        s.wr<1>(1280, 0);
        execute(s);
        require(!s.r<1>(1280) && s.r<1>(1286), "Repeated inexact must not newly set FX");
        s.wr<8>(source, float_bits<8>(1.0));
        execute(s);
        require(!s.r<1>(1294) && s.r<1>(1286), "Exact result must preserve sticky XX and clear FI");
        s.wr<8>(source, UINT64_C(0x7ff8000000001234));
        execute(s);
        require(s.r<1>(1280) && s.r<1>(1286) && !s.r<1>(1287), "New invalid after inexact");
        s.wr<1>(1280, 0);
        execute(s);
        require(!s.r<1>(1280), "Repeated invalid must not newly set FX");
        s.wr<8>(source, UINT64_C(0x7ff0000000001234));
        execute(s);
        require(s.r<1>(1280) && s.r<1>(1287), "New signaling NaN must set FX");
        s.wr<1>(1304, 1);
        s.wr<8>(destination, UINT64_C(0xabcdef1234567890));
        execute(s, "Original enabled floating invalid-conversion exception requires a handler");
        require(s.r<8>(destination) == UINT64_C(0xabcdef1234567890) && s.r<1>(1281) &&
                !s.r<1>(1293) && !s.r<1>(1294), "Enabled invalid must preserve destination");
        s.registers.fill(0);
        s.wr<1>(1308, 1);
        s.wr<8>(source, float_bits<8>(-1.5));
        execute(s, "Original enabled floating inexact exception requires a handler");
        require(std::uint32_t(s.r<8>(destination)) == 0xffffffffu && s.r<1>(1281) && s.r<1>(1286),
                "Enabled inexact must deliver rounded result before unsupported handler");
        s.registers.fill(0);
        s.wr<8>(source, float_bits<8>(-5.5));
        original_fctiwz(s, source, source);
        require(std::uint32_t(s.r<8>(source)) == 0xfffffffbu && s.r<1>(1294), "Aliased source/destination");
        std::cout << checked << " original fctiwz cases and sticky/enabled/alias checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
