#include "aot_runtime.hpp"
#include "callbacks.hpp"
#include <chrono>
#include <iostream>
#include <string_view>

using namespace cm::aot;
using namespace std::chrono;
namespace {
constexpr std::uint32_t fixture = 0x30000000, input = fixture + 16, output = fixture + 32;
constexpr std::uint32_t forward_scratch = fixture + 48, backward_scratch = fixture + 64;
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}
std::vector<std::uint8_t> date_record(sys_days date) {
    const year_month_day ymd(date);
    const int y = int(ymd.year());
    const auto day = unsigned((date - sys_days(ymd.year()/January/1)).count());
    return {std::uint8_t(day >> 8), std::uint8_t(day), std::uint8_t(y >> 8),
            std::uint8_t(y), std::uint8_t(ymd.year().is_leap())};
}
}
int main(int argc, char** argv) {
    if (argc != 2 && !(argc == 3 && std::string_view(argv[2]) == "--modern")) { std::cerr << "Usage: cm-calendar-reference-checks <original-segments>\n"; return 2; }
    try {
        auto state = std::make_unique<State>();
        auto& s = *state;
        s.calendar_original = argc == 2;
        load_program(s, argv[1]);
        s.memory.map(fixture, 256);
        s.memory.write<4>(0x108b7ae4, forward_scratch);
        s.memory.write<4>(0x108b7adc, backward_scratch);
        for (const auto& [global, offset] : std::array<std::pair<std::uint32_t, unsigned>, 3>{
            {{0x108b3f00, 128}, {0x108b3f0c, 132}, {0x108b3f08, 136}}}) {
            s.memory.write<4>(global, fixture + offset);
            s.memory.write<4>(fixture + offset, 0x12340000 + offset);
        }
        unsigned cases = 0;
        for (const int y : {1900, 1999, 2000, 2001, 2002, 2004, 2100}) {
          for (const auto md : {January/1, February/28, March/1, December/31}) {
           for (const int offset : {-730, -366, -365, -28, -1, 0, 1, 28, 365, 366, 730}) {
            for (const bool subtract : {false, true}) {
             for (const bool in_place : {false, true}) {
                const sys_days initial_date(year(y)/md);
                const auto initial = date_record(initial_date);
                const auto expected = date_record(initial_date + days(subtract ? -offset : offset));
                s.memory.copy_in(input, initial);
                s.memory.clear(output, 5);
                s.memory.clear(forward_scratch, 5); s.memory.clear(backward_scratch, 5);
                s.memory.write<1>(0x108b84d5, 0); s.memory.write<1>(0x108b84d3, 0);
                s.memory.write<4>(fixture, subtract ? 0x10717b20 : 0x10717890);
                s.memory.write<4>(fixture + 4, s.r<4>(8));
                const auto registers = s.registers;
                const auto temporary = s.unique;
                const auto rng = s.memory.copy_out(0x108b3f00, 16);
                const auto rng_values = s.memory.copy_out(fixture + 128, 12);
                const auto destination = in_place ? input : output;
                s.budget = s.executed + 10000;
                (void)call_original(s, fixture, std::array<std::uint32_t, 3>{
                    destination, input, std::uint32_t(offset)});
                require(s.memory.copy_out(destination, 5) == expected,
                        "Original calendar date result differs at a tested edge");
                if (!in_place) require(s.memory.copy_out(input, 5) == initial,
                                      "Original calendar modified the separate input date");
                require(s.registers == registers && s.unique == temporary,
                        "Original calendar callback failed ABI restoration");
                require(s.memory.copy_out(0x108b3f00, 16) == rng &&
                        s.memory.copy_out(fixture + 128, 12) == rng_values && s.native_calls == 0,
                        "Calendar arithmetic consumed RNG or a native service");
                const auto scratch = subtract ? backward_scratch : forward_scratch;
                require(s.memory.copy_out(scratch, 5) == expected &&
                        s.memory.read<1>(subtract ? 0x108b84d3 : 0x108b84d5) == 1,
                        "Original calendar scratch or lazy initialization side effect changed");
                if (offset < 0) require(s.memory.copy_out(subtract ? forward_scratch : backward_scratch, 5) == expected,
                                        "Original signed offset did not preserve the delegated scratch result");
                ++cases;
             }
            }
           }
          }
        }
        if (!s.calendar_original) require(s.calendar_calls[0] == cases / 2 && s.calendar_calls[1] == cases / 2 &&
                                          !s.calendar_exceptional_calls, "Modern oracle did not use both semantic entries exclusively");
        std::cout << cases << (s.calendar_original ? " original" : " modern") << " calendar cases: month/year/leap/century edges, signed offsets, in-place dates, scratch initialization, ABI and no native/RNG calls\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
