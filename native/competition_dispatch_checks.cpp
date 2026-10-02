#include "aot_runtime.hpp"
#include <array>
#include <iostream>

using namespace cm::aot;
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
}
int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Usage: cm-competition-dispatch-checks <original-segments>\n";
        return 2;
    }
    try {
        auto state = std::make_unique<State>();
        auto& s = *state;
        load_program(s, argv[1]);
        const auto table = s.memory.read<4>(0x108b6688);
        constexpr std::array<std::uint32_t, 7> destinations{
            0x101b8a4c, 0x101b8908, 0x101b87c0, 0x101b8680,
            0x101b8adc, 0x101b8548, 0x101b84cc};
        unsigned cases = 0;
        for (const int outcome : {-128, -4, -3, -2, -1, 0, 1, 2, 3, 4, 127}) {
            s.pc = 0x101b84a8;
            s.wr<4>(8, 0x108bba90);
            // The dispatch sign-extends its byte result. Nonzero upper bits
            // ensure the check exercises that instruction as well as the table.
            s.wr<4>(12, 0xabcdee00 | std::uint8_t(outcome));
            const bool in_table = outcome >= -3 && outcome <= 3;
            const auto destination = in_table ? destinations[unsigned(outcome + 3)] : 0x101b8adc;
            if (in_table) require(s.memory.read<4>(table + unsigned(outcome + 3) * 4) == destination,
                                  "Original competition jump table differs from the audited layout");
            const auto steps = in_table ? 9u : 4u;
            s.budget = s.executed + steps;
            bool compiled_destination = false;
            try { run(s); }
            catch (const std::runtime_error& error) {
                compiled_destination = std::string(error.what()) == "Native instruction budget exhausted";
                if (!compiled_destination) throw;
            }
            require(compiled_destination && s.pc == destination,
                    "Original competition dispatch did not reach the compiled destination");
            require(s.native_calls == 0, "Competition dispatch unexpectedly called a native service");
            ++cases;
        }
        std::cout << cases << " original competition dispatch cases: all seven indirect destinations and signed out-of-range results compiled\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
