#include "aot_runtime.hpp"
#include "regions.hpp"
#include <iostream>
#include <memory>
#include <numbers>

using namespace cm::aot;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
void call(State& s, const char* name, std::uint32_t a, std::uint32_t b = 0) {
    s.pc = 0x1234;
    s.wr<4>(12, a);
    s.wr<4>(16, b);
    s.wr<4>(4128, return_sentinel);
    require(bridge_import(s, name), "Expected implemented native bridge");
    require(s.pc == return_sentinel, "Bridge did not return to original caller");
}
}
int main() {
    try {
        const std::vector<std::vector<LegacyRect>> regions = {
            {}, {{0, 0, 0, 3}}, {{-2, -4, 5, 7}}, {{0, 0, 2, 3}, {0, 3, 2, 6}},
            {{0, 0, 4, 4}, {1, 1, 3, 3}}, {{-3, -5, 2, -1}, {0, 2, 6, 7}},
            {{-4, -1, 6, 2}, {0, -5, 2, 7}}, {{0, 0, 3, 3}, {2, 2, 5, 6}}};
        for (const auto& rectangles : regions) {
            const auto encoded = region_union(rectangles);
            for (std::int16_t y = -7; y < 10; ++y) for (std::int16_t x = -7; x < 10; ++x) {
                bool expected = false;
                for (const auto& r : rectangles) expected |= y >= r[0] && y < r[2] && x >= r[1] && x < r[3];
                require(region_contains(encoded, x, y) == expected,
                        "Original scanline region lost an overlap, gap, or half-open boundary");
            }
        }
        require(region_union(regions[3]).size() == 10,
                "Touching rectangles were not reduced to the compact rectangular region format");
        auto s = std::make_unique<State>();
        s->wr<8>(16384, float_bits<8>(7.25)); s->wr<8>(16416, float_bits<8>(-3.5));
        for (const auto& [angle, sine, cosine] : std::array<std::array<double, 3>, 4>{{
            {0, 0, 1}, {std::numbers::pi / 2, 1, 0}, {-std::numbers::pi / 2, -1, 0}, {std::numbers::pi, 0, -1}}}) {
            s->wr<8>(16400, float_bits<8>(angle));
            call(*s, "CarbonLib::sin", 0x12345678);
            require(std::abs(float_value<8>(s->r<8>(16400)) - sine) < 1e-14 && s->r<4>(12) == 0x12345678,
                    "Native sine lost the original f1 double ABI");
            s->wr<8>(16400, float_bits<8>(angle));
            call(*s, "CarbonLib::cos", 0x12345678);
            require(std::abs(float_value<8>(s->r<8>(16400)) - cosine) < 1e-14,
                    "Native cosine lost the original f1 double ABI");
        }
        require(float_value<8>(s->r<8>(16384)) == 7.25 && float_value<8>(s->r<8>(16416)) == -3.5,
                "Native math bridge overwrote neighboring floating registers");
        auto math = [&](const char* name, double a, double b = 0) {
            s->wr<8>(16400, float_bits<8>(a)); s->wr<8>(16416, float_bits<8>(b));
            call(*s, name, 0x12345678);
            require(s->r<4>(12) == 0x12345678 && float_value<8>(s->r<8>(16416)) == b, "Native math clobbered integer or f2 state");
            return float_value<8>(s->r<8>(16400));
        };
        require(std::abs(math("CarbonLib::atan", 1) - std::numbers::pi / 4) < 1e-14 &&
                std::abs(math("CarbonLib::atan2", 1, -1) - 3 * std::numbers::pi / 4) < 1e-14 &&
                std::abs(math("CarbonLib::atan2", -1, 1) + std::numbers::pi / 4) < 1e-14,
                "Native arctangent argument order or quadrant");
        require(math("CarbonLib::sqrt", 9) == 3 && std::isnan(math("CarbonLib::sqrt", -1)) &&
                math("CarbonLib::pow", 2, 3) == 8 && math("CarbonLib::pow", 4, 0.5) == 2,
                "Native square root/power result or double argument order");
        require(std::signbit(math("CarbonLib::sin", -0.0)) &&
                math("CarbonLib::atan2", -0.0, -1) == -std::numbers::pi,
                "Native math lost signed-zero semantics");
        s->memory.map(0x70000000, 32);
        call(*s, "CarbonLib::Gestalt", 0x70706366, 0x70000000);
        require(s->r<4>(12) == 0 && s->memory.read<4>(0x70000000) == 0,
                "Scalar feature selection failed");
        s->memory.write<4>(0x70000000, 0x12345678);
        call(*s, "CarbonLib::Gestalt", 0x7174696d, 0x70000000);
        require(s->r<4>(12) == std::uint32_t(-5551) && s->memory.read<4>(0x70000000) == 0x12345678,
                "Unavailable legacy QuickTime was advertised or overwrote the version output");
        s->wr<4>(12, 0x12345678);
        require(!bridge_import(*s, "CarbonLib::Gestalt"), "Unknown selector was faked");
        require(!bridge_import(*s, "CarbonLib::Unknown"), "Unknown import was faked");
        s->memory.copy_in(0x70000000, {0, 1, 2, 3, 4, 5, 6, 7});
        s->wr<4>(12, 0x70000000);
        s->wr<4>(16, 0x70000002);
        s->wr<4>(20, 6);
        require(bridge_import(*s, "CarbonLib::BlockMoveData"), "BlockMoveData bridge missing");
        require(s->memory.copy_out(0x70000000, 8) == std::vector<std::uint8_t>({0, 1, 0, 1, 2, 3, 4, 5}),
                "Overlapping BlockMoveData changed its source prematurely");
        call(*s, "CarbonLib::NewPtrClear", 31);
        const auto a = std::uint32_t(s->r<4>(12));
        require(a != 0 && a % 16 == 0, "Allocation address/alignment");
        require(s->memory.copy_out(a, 31) == std::vector<std::uint8_t>(31), "NewPtrClear payload");
        s->memory.write<4>(a, 0x12345678);
        require(s->memory.copy_out(a, 4) == std::vector<std::uint8_t>({0x12, 0x34, 0x56, 0x78}),
                "Original big-endian pointer memory was not preserved");
        call(*s, "CarbonLib::NewPtr", 48);
        const auto b = std::uint32_t(s->r<4>(12));
        s->memory.write<4>(b, 0xabcdef01);
        call(*s, "CarbonLib::DisposePtr", a);
        call(*s, "CarbonLib::NewPtrClear", 31);
        require(s->r<4>(12) == a, "Reusable released allocation");
        require(s->memory.read<4>(a) == 0 && s->memory.read<4>(b) == 0xabcdef01,
                "Clear affected a neighboring live allocation");
        call(*s, "CarbonLib::DisposePtr", a);
        call(*s, "CarbonLib::DisposePtr", b);
        call(*s, "CarbonLib::NewPtr", 80);
        require(s->r<4>(12) == a, "Adjacent released blocks were not reusable");
        call(*s, "CarbonLib::NewPtr", 0xffffffff);
        require(s->r<4>(12) == 0 && s->memory_error == -108, "Allocation failure ABI");
        bool rejected = false;
        try { (void)s->memory.read<4>(0x7000001f); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Out-of-range data access was accepted");
        rejected = false;
        try { (void)s->memory.read<4>(UINT64_C(0x100000000)); }
        catch (const std::runtime_error&) { rejected = true; }
        require(rejected, "Address truncation was accepted");
        std::cout << "Native memory and feature bridge checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
