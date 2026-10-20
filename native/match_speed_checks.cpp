#include "match_speed.hpp"
#include "match_presentation.hpp"
#include "semantic_io_checks.hpp"

using namespace cm::semantic_io_checks;
namespace {
void instruction(State& s, std::uint32_t pc) {
    s.pc = pc;
    s.budget = s.executed + 1;
    try { require(dispatch_page(s), "Missing original speed instruction"); }
    catch (const std::runtime_error& e) {
        require(std::string_view(e.what()) == "Native instruction budget exhausted", e.what());
    }
}
}
int main() {
    try {
        auto state = std::make_unique<State>();
        auto& s = *state;
        s.memory.map(fixture, 0x10000);
        require(!native_match_speed_enabled(), "Headless reference unexpectedly enables presentation changes");
        require(match_simulation_speed(0) == 0, "Original zero speed changed outside the native UI");
        initialize_native_match_speed();
        require(native_match_speed() == 0 && match_turbo_enabled(), "Native default is not Turbo");
        for (auto choice : {0u, 20u, 55u, 90u, 125u, 150u}) {
            select_native_match_speed(choice);
            // Exercise the generated getter, rather than just the preference.
            s.memory.write<2>(fixture, 125);
            s.wr<4>(12, fixture + 0x659a);
            instruction(s, 0x10147cf8);
            require(s.r<4>(12) == choice && s.memory.read<2>(fixture) == 125,
                    "Speed getter did not select the native preference");
            s.wr<4>(12, fixture);
            s.wr<4>(20, 125);
            instruction(s, 0x101fefc8);
            require(s.memory.read<1>(fixture + 0x4070) == choice && s.r<4>(20) == 125,
                    "New commentary object did not inherit the speed");
            s.wr<4>(92, fixture);
            instruction(s, 0x10203574);
            require(s.r<4>(0) == (choice == 0 ? 20 : choice), "Original cadence normalization failed");
            s.wr<4>(92, fixture);
            instruction(s, 0x102036ac);
            require(s.r<4>(20) == (choice == 0 ? 20 : choice), "Commentary cadence normalization failed");
            require(commentary_presentation_duration(s, fixture, 300) == (choice == 0 ? 20 : 300),
                    "Turbo changed an original speed's presentation delay");
            // Successful original setter is the only preference update edge.
            const auto next = choice == 150 ? 0u : 150u;
            s.wr<4>(12, fixture + 0x659a);
            s.wr<4>(16, next);
            instruction(s, 0x10147e50);
            require(s.memory.read<2>(fixture) == next && native_match_speed() == next,
                    "Original choice did not update the native speed");
        }
        select_native_match_speed(55);
        select_native_match_speed(65535);
        require(native_match_speed() == 55, "Invalid speed replaced the preference");
        s.wr<4>(12, 0x8000659a);
        s.wr<4>(16, 20);
        s.pc = 0x10147e50;
        s.budget = s.executed + 2;
        bool fault = false;
        try { dispatch_page(s); } catch (const std::runtime_error&) { fault = true; }
        require(fault && native_match_speed() == 55, "Faulting original setter persisted a choice");
        std::cout << "PASS: six native choices, Turbo default, original generated getter/setter, "
                     "constructor, cadence, presentation isolation and fault-safe preference\n";
    } catch (const std::exception& e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
