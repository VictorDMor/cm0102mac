#include "find_screens_fixups.hpp"
#include <array>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

// The two club-list builders of FIND_SCREENS.CPP, at the two instructions that
// use their nation (r20) unchecked: the title read `lbz r5,0x115(r20)` and the
// filter `cmplw r0,r20` on a club's nation. With a nation both run as they
// are; without one the title byte is 0 and the filter keeps no club; any
// other nation outside mapped memory still faults.
using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000, nation = fixture + 0x100;

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
    require(present, "The club-list page is missing");
    return false;
}

std::unique_ptr<State> state_with(std::uint32_t r20) {
    auto state = std::make_unique<State>();
    state->unique.fill(0xa5);
    state->memory.map(fixture, 0x1000);
    for (int index = 0; index < 32; ++index) state->wr<4>(index * 4, 0x51000000 + index);
    state->wr<4>(80, r20);
    return state;
}

void check_title(std::uint32_t site, std::uint32_t r20, std::uint8_t byte, bool faults) {
    auto state = state_with(r20);
    auto& s = *state;
    s.memory.write<1>(nation + 0x115, byte);
    const auto memory = s.memory.copy_out(fixture, 0x1000);
    const auto registers = s.registers;
    s.wr<4>(20, r20 == nation ? byte : 0);
    const auto expected = s.registers;
    s.registers = registers;
    require(instruction(s, site) == faults, faults
            ? "A nation outside mapped memory no longer faults at the title read"
            : "The title read faulted");
    require(s.memory.copy_out(fixture, 0x1000) == memory, "The title read stored something");
    require(faults ? s.pc == site && s.registers == registers : s.pc == site + 4 && s.registers == expected,
            "The title read changed more than r5, or read a wrong byte");
}

// `club_nation` is r0, the nation word the builder has just loaded from a club.
void check_filter(std::uint32_t site, std::uint32_t skip, std::uint32_t r20, std::uint32_t club_nation) {
    auto state = state_with(r20);
    auto& s = *state;
    s.wr<4>(0, club_nation);
    const auto registers = s.registers;
    require(!instruction(s, site), "The filter faulted");
    if (r20 == 0) {
        require(s.pc == skip && s.registers == registers, "Without a nation the filter did not skip the club untouched");
        return;
    }
    // With a nation the comparison runs, and the branch after it decides.
    require(s.pc == site + 4, "With a nation the filter did not run its comparison");
    require(!instruction(s, site + 4), "The filter's branch faulted");
    require(s.pc == (club_nation == r20 ? site + 8 : skip), "With a nation the filter decided wrongly");
}
}

int main() {
    try {
        unsigned cases = 0;
        for (auto [title, filter, skip] : {std::array{0x10104ce4u, 0x10104dd4u, 0x10104e0cu},
                                           std::array{0x10105220u, 0x10105310u, 0x10105348u}}) {
            for (std::uint8_t byte : {std::uint8_t(0), std::uint8_t(1), std::uint8_t(0x7f), std::uint8_t(0xff)}) {
                check_title(title, nation, byte, false);
                ++cases;
            }
            check_title(title, 0, 0x55, false);
            check_title(title, 0x10, 0x55, true);
            check_title(title, 0x20000000, 0x55, true);
            check_filter(filter, skip, nation, nation);
            check_filter(filter, skip, nation, nation + 4);
            check_filter(filter, skip, nation, 0);
            // A club whose own nation is null is the row the original read through page zero.
            check_filter(filter, skip, 0, 0);
            check_filter(filter, skip, 0, nation);
            cases += 8;
        }
        std::cout << cases << " club-list nation checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
