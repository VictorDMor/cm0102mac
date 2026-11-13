#include "aot_runtime.hpp"
#include "staff_screens_fixups.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

// The two player-profile walks of STAFF_SCREENS.CPP at the two instructions
// that load an offer from its slot (+0) and the offer's club (+8), neither of
// which they test. An existing slot or offer is read as it is; a null one (an
// offer ended since the screen built its list) continues with the next index
// and changes nothing; any other one outside mapped memory still faults.
using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000, offer = fixture + 0x100;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

// One instruction; true when it faulted on memory.
bool instruction(State& s, std::uint32_t pc) {
    s.pc = pc;
    s.budget = s.executed + 1;
    bool present = false;
    try { present = dispatch_page(s); }
    catch (const std::runtime_error& error) {
        const std::string_view what = error.what();
        if (what == "Native instruction budget exhausted") return false;
        require(what.starts_with("Unmapped data access"), error.what());
        return true;
    }
    require(present, "The profile page is missing");
    return false;
}

enum class Outcome { read, skipped, fault };

void check(std::uint32_t site, int source, int target, std::uint32_t field, std::uint32_t skip,
           std::uint32_t value, std::uint32_t club, Outcome outcome) {
    auto state = std::make_unique<State>();
    auto& s = *state;
    s.unique.fill(0xa5);
    s.memory.map(fixture, 0x1000);
    for (int index = 0; index < 32; ++index) s.wr<4>(index * 4, 0x51000000 + index);
    s.wr<4>(source * 4, value);
    s.memory.write<4>(offer + field, club);
    const auto memory = s.memory.copy_out(fixture, 0x1000);
    const auto registers = s.registers;
    s.wr<4>(target * 4, club);
    const auto expected = s.registers;
    s.registers = registers;
    require(instruction(s, site) == (outcome == Outcome::fault), outcome == Outcome::fault
            ? "An offer outside mapped memory no longer faults"
            : "The club read faulted");
    require(s.memory.copy_out(fixture, 0x1000) == memory, "The club read stored something");
    switch (outcome) {
    case Outcome::read:
        require(s.pc == site + 4 && s.registers == expected, "An existing offer's club was not read as it is");
        break;
    case Outcome::skipped:
        require(s.pc == skip && s.registers == registers, "An ended offer was not skipped untouched");
        break;
    case Outcome::fault:
        require(s.pc == site && s.registers == registers, "The faulting read changed state");
        break;
    }
}
}

int main() {
    try {
        unsigned cases = 0;
        struct Site { std::uint32_t pc; int source, target; std::uint32_t field, skip; };
        for (const auto& [pc, source, target, field, skip] : {Site{0x1038aaec, 3, 4, 0, 0x1038ab04},
                                                              Site{0x1038aaf4, 4, 4, 8, 0x1038ab04},
                                                              Site{0x1039a4b4, 19, 5, 0, 0x1039a980},
                                                              Site{0x1039a4bc, 5, 0, 8, 0x1039a980}}) {
            for (std::uint32_t club : {0u, 1u, 0x7fffffffu, 0xffffffffu}) {
                check(pc, source, target, field, skip, offer, club, Outcome::read);
                ++cases;
            }
            check(pc, source, target, field, skip, 0, 0x55, Outcome::skipped);
            check(pc, source, target, field, skip, 0x10, 0x55, Outcome::fault);
            check(pc, source, target, field, skip, 0x20000000, 0x55, Outcome::fault);
            cases += 3;
        }
        std::cout << cases << " profile offer checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
