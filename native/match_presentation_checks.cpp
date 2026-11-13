#include "match_presentation.hpp"
#include <iostream>
#include <memory>
#include <string_view>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000;
constexpr std::uint32_t commentary = fixture + 0x100;
constexpr std::uint32_t enabled_channels = fixture + 0x6000;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void advance(State& s) {
    s.budget = s.executed + 1;
    try { require(dispatch_page(s), "Original commentary page is missing"); }
    catch (const std::runtime_error& error) {
        if (std::string_view(error.what()) != "Native instruction budget exhausted") throw;
    }
}

void check_channel(bool turbo, unsigned speed, std::uint32_t duration,
                   std::uint32_t start, std::uint32_t now, int sound_handle,
                   bool sound_busy, unsigned channel, bool enabled) {
    auto state = std::make_unique<State>();
    auto& s = *state;
    s.memory.map(fixture, 0x10000);
    s.memory.write<4>(commentary + 0x406c, duration);
    s.memory.write<1>(commentary + 0x4070, speed);
    s.memory.write<4>(commentary + 0x402c + channel * 4, start);
    s.memory.write<1>(commentary + 0x10 + channel, sound_handle);
    s.memory.write<1>(enabled_channels + channel, enabled);
    // Unrelated engine flags and simulation clock are canaries in mapped memory.
    s.memory.write<4>(fixture + 0x8000, 0xa5c0ffee);
    s.memory.write<4>(fixture + 0x8004, 0x12345678);
    auto expected_memory = s.memory.copy_out(fixture, 0x10000);
    if (enabled && sound_handle >= 0 && !sound_busy)
        expected_memory[commentary - fixture + 0x10 + channel] = 0xff;
    s.wr<4>(104, commentary);       // Original r26: commentary object.
    s.wr<4>(108, enabled_channels); // Original r27: active channels.
    s.wr<4>(112, channel);
    s.wr<4>(116, channel * 4);
    s.wr<4>(120, fixture + 0x9000); // Original r30: sound owner.
    s.wr<4>(124, now);              // Original r31: milliseconds from native clock.
    s.pc = 0x10203724;
    set_match_turbo_enabled(turbo);
    unsigned sound_queries = 0, sound_releases = 0;
    for (unsigned steps = 0; steps < 100 &&
         s.pc != 0x102037cc && s.pc != 0x102037e4; ++steps) {
        if (s.pc == 0x10742b00 || s.pc == 0x10742b80) {
            require(s.r<4>(12) == fixture + 0x9000 &&
                    s.r<4>(16) == unsigned(sound_handle),
                    "Original sound arguments changed");
            if (s.pc == 0x10742b00) {
                ++sound_queries;
                s.wr<4>(12, sound_busy);
            } else {
                ++sound_releases;
            }
            s.pc = std::uint32_t(s.r<4>(4128));
        } else {
            advance(s);
        }
    }
    const auto turbo_speed = native_match_speed_enabled() ? 0u : 20u;
    const auto effective_duration = turbo && speed == turbo_speed && duration > 20 ? 20 : duration;
    const auto deadline = std::uint32_t(start + effective_duration);
    const bool elapsed = now > deadline || now < start;
    const bool ready = !enabled || (sound_handle != -2 &&
                       (sound_handle == -1 || !sound_busy) && elapsed);
    require(s.pc == (ready ? 0x102037ccu : 0x102037e4u),
            "Original commentary timer/sentinel continuation changed");
    require(sound_queries == unsigned(enabled && sound_handle >= 0) &&
            sound_releases == unsigned(enabled && sound_handle >= 0 && !sound_busy),
            "Original sound completion/cleanup path changed");
    require(s.r<4>(124) == now, "Turbo changed the original clock register");
    require(s.memory.copy_out(fixture, 0x10000) == expected_memory,
            "Turbo changed stored times, flags, or unrelated guest state");
}

void check_load_copy(bool turbo, unsigned speed, std::uint32_t duration) {
    auto state = std::make_unique<State>();
    auto& s = *state;
    // P-code unique-space temporaries are compiler-internal, not architectural
    // outputs. Poison their incoming bytes to expose accidental dependencies;
    // instruction-local lowering need not materialize their outgoing values.
    s.unique.fill(0xa5);
    s.memory.map(fixture, 0x10000);
    s.memory.write<4>(commentary + 0x406c, duration);
    s.memory.write<1>(commentary + 0x4070, speed);
    s.wr<4>(104, commentary);
    s.pc = 0x10203738;
    set_match_turbo_enabled(turbo);
    auto registers = s.registers;
    const auto memory = s.memory.copy_out(fixture, 0x10000);
    const auto turbo_speed = native_match_speed_enabled() ? 0u : 20u;
    const auto expected = turbo && speed == turbo_speed && duration > 20 ? 20 : duration;
    s.wr<4>(0, expected);
    const auto expected_registers = s.registers;
    s.registers = registers;
    advance(s);
    require(s.pc == 0x1020373c && s.registers == expected_registers &&
            s.memory.copy_out(fixture, 0x10000) == memory,
            "Presentation lowering changed more than the loaded duration copy");
}

// The end of the commentary routine 0x102021e0 reads its event's line table
// twice without a check: the line's sound flag (0x10203580, speeds above 100)
// and its pace (the tail from 0x10203684, which stores the line's start time
// and its presentation delay: speed times pace over 100). `row` is the
// event's table row, kept at sp+0x206c, and `line_table` what that row holds
// at +8. An empty event (code 0) has row -8000 and, in the reported runs, a
// null there; the original read both values through page zero.
enum class Lines { read, none, fault };
constexpr std::uint32_t line_stack = fixture + 0x9000, line_toc = fixture + 0xe000;
constexpr std::uint32_t line_rows = fixture + 0xa000, line_event = fixture + 0xd000;
constexpr std::uint32_t line_records = fixture + 0xd100;

std::unique_ptr<State> line_state(int row, std::uint32_t line_table, int line, unsigned speed) {
    auto state = std::make_unique<State>();
    auto& s = *state;
    s.unique.fill(0xa5);
    s.memory.map(fixture, 0x10000);
    s.memory.write<4>(line_toc - 0x5298, line_rows);
    s.memory.write<2>(line_stack + 0x206c, std::uint16_t(row));
    s.memory.write<4>(std::uint32_t(line_rows + row * 0x14 + 8), line_table);
    s.memory.write<2>(line_event + 2, std::uint16_t(line));
    s.memory.write<1>(commentary + 0x4070, speed);
    s.wr<4>(4, line_stack); s.wr<4>(8, line_toc);
    s.wr<4>(92, commentary); s.wr<4>(96, line_event);
    return state;
}

bool run_lines(State& s, std::uint32_t from, std::uint32_t to) {
    s.pc = from;
    try {
        for (int steps = 0; steps < 32 && s.pc != to; ++steps) advance(s);
    } catch (const std::runtime_error&) { return false; }
    return s.pc == to;
}

void check_pace(int row, std::uint32_t line_table, int line, unsigned speed, std::int16_t pace, Lines expected) {
    constexpr std::uint32_t channel = 3, started = 0x00c0ffee, returned = 0x0badc0dc;
    auto state = line_state(row, line_table, line, speed);
    auto& s = *state;
    s.memory.write<2>(line_records + line * 0x10c + 2, std::uint16_t(pace));
    s.memory.write<4>(commentary + 0x406c, 0xdeadbeef);
    s.wr<4>(12, started); s.wr<4>(52, channel);
    // The tail runs on into the routine's return, which is where it is met.
    s.memory.write<4>(line_stack + 0x20d8, returned);
    auto memory = s.memory.copy_out(fixture, 0x10000);
    const auto put = [&](std::uint32_t address, std::uint32_t value) {
        for (int byte = 0; byte < 4; ++byte) memory[address - fixture + byte] = std::uint8_t(value >> (24 - 8 * byte));
    };
    put(commentary + 0x402c + channel * 4, started);
    if (expected != Lines::fault) {
        const auto cadence = std::int32_t(match_simulation_speed(speed));
        put(commentary + 0x406c, std::uint32_t(std::int32_t(std::int16_t(
            expected == Lines::none ? 0 : cadence * pace / 100))));
    }
    const auto finished = run_lines(s, 0x10203684, returned);
    require(finished == (expected != Lines::fault), expected == Lines::fault
            ? "A line table outside mapped memory no longer faults in the commentary tail"
            : "The commentary tail faulted on its line table");
    require(finished || s.pc == 0x102036bc, "The commentary tail stopped at the wrong instruction");
    require(s.memory.copy_out(fixture, 0x10000) == memory,
            "The commentary tail stored a wrong start time or presentation delay");
}

// With a sound flag of zero the sound path joins the tail at 0x10203678, and
// that is where an event without lines must arrive too, having stored nothing.
void check_sound(int row, std::uint32_t line_table, int line, Lines expected) {
    auto state = line_state(row, line_table, line, 125);
    auto& s = *state;
    const auto memory = s.memory.copy_out(fixture, 0x10000);
    const auto finished = run_lines(s, 0x10203580, 0x10203678);
    require(finished == (expected != Lines::fault), expected == Lines::fault
            ? "A line table outside mapped memory no longer faults in the commentary sound path"
            : "The commentary sound path faulted on its line table");
    require(s.memory.copy_out(fixture, 0x10000) == memory, "The commentary sound path stored something");
}

unsigned check_lines() {
    unsigned cases = 0;
    for (int line : {0, 11}) {
        // In the table: a line table is read, a null one has no lines, and any
        // other bad one still faults. Outside it nothing is read at all.
        check_sound(3, line_records, line, Lines::read);
        check_sound(3, 0, line, Lines::none);
        check_sound(3, 0x10, line, Lines::fault);
        for (int row : {-8, 0x2a5}) for (std::uint32_t table : {0u, 0x10u, line_records}) {
            check_sound(row, table, line, Lines::none);
            ++cases;
        }
        cases += 3;
        for (unsigned speed : {0u, 20u, 55u, 150u, 255u}) {
            for (std::int16_t pace : {std::int16_t(0), std::int16_t(1), std::int16_t(300),
                                      std::int16_t(32767), std::int16_t(-300)}) {
                check_pace(3, line_records, line, speed, pace, Lines::read);
                check_pace(3, 0, line, speed, pace, Lines::none);
                check_pace(3, 0x10, line, speed, pace, Lines::fault);
                for (int row : {-8, 0x2a5}) for (std::uint32_t table : {0u, 0x10u, line_records}) {
                    check_pace(row, table, line, speed, pace, Lines::none);
                    ++cases;
                }
                cases += 3;
            }
        }
    }
    return cases;
}
}

int main() {
    try {
        unsigned cases = 0;
        for (bool native : {false, true}) {
            if (native) initialize_native_match_speed();
            cases += check_lines();
            for (bool turbo : {false, true}) {
                for (unsigned speed = 0; speed < 256; ++speed) {
                    for (auto duration : {0u, 19u, 20u, 21u, 100u, UINT32_MAX}) {
                        check_load_copy(turbo, speed, duration);
                        ++cases;
                    }
                }
                for (unsigned speed : {0u, 5u, 10u, 15u, 19u, 20u, 21u, 55u, 90u, 125u, 150u, 255u}) {
                    for (auto now : {999u, 1000u, 1020u, 1021u, 1100u, 1101u}) {
                        for (int handle : {-2, -1, 3}) {
                            for (bool busy : {false, true}) {
                                for (unsigned channel : {0u, 15u}) {
                                    check_channel(turbo, speed, 100, 1000, now,
                                                  handle, busy, channel, true);
                                    ++cases;
                                }
                            }
                        }
                    }
                }
                for (auto duration : {0u, 1u, 19u, 20u, 21u, 100u, UINT32_MAX}) {
                    for (auto now : {0u, 10u, UINT32_MAX - 10, UINT32_MAX}) {
                        check_channel(turbo, native ? 0 : 20, duration, UINT32_MAX - 10, now,
                                      -1, false, 0, true);
                        ++cases;
                    }
                }
                check_channel(turbo, native ? 0 : 20, 100, 1000, 1000, -2, true, 15, false);
                ++cases;
            }
        }
        set_match_turbo_enabled(false);
        std::cout << cases << " original commentary duration, sound, sentinel, and state checks passed\n";
        return 0;
    } catch (const std::exception& error) {
        set_match_turbo_enabled(false);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
