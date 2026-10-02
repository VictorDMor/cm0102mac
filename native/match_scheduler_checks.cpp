#include "aot_runtime.hpp"
#include "match_turbo.hpp"
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string_view>

using namespace cm::aot;
namespace {
constexpr std::uint32_t fixture = 0x30000000;
constexpr std::uint32_t toc = fixture + 0x8000;
constexpr std::uint32_t scheduler_counter = toc - 0x3918;
constexpr std::uint32_t runtime_holder = fixture + 0x100;
constexpr std::uint32_t runtime = fixture + 0x200;
constexpr std::uint32_t skip = 0x101cd660;
constexpr std::uint32_t simulate = 0x101cd6ac;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

std::uint32_t scheduler_destination(std::uint32_t flags, unsigned tick,
                                    bool forced, bool turbo) {
    auto state = std::make_unique<State>();
    auto& s = *state;
    s.memory.map(fixture, 0x10000);
    s.memory.write<4>(runtime_holder, runtime);
    s.memory.write<4>(runtime + 4, flags);
    s.memory.write<4>(scheduler_counter, tick - 1);
    s.memory.write<4>(fixture + 0x330, UINT32_MAX);
    s.wr<4>(8, toc);             // PowerPC r2: original global table.
    s.wr<4>(16, forced ? 1 : 0); // PowerPC r4: bypass the rate gate.
    s.wr<4>(24, runtime_holder); // PowerPC r6: runtime holder.
    s.wr<4>(112, fixture + 0x300); // PowerPC r28: match record.
    s.pc = 0x101cd608;
    set_match_turbo_enabled(turbo);
    std::uint32_t destination = 0;
    for (unsigned steps = 0; steps < 80; ++steps) {
        if (s.pc == skip || s.pc == 0x101cd664 || s.pc == 0x101cd85c) {
            destination = skip;
            break;
        }
        if (s.pc >= simulate && s.pc <= 0x101cd6b8) {
            destination = simulate;
            break;
        }
        s.budget = s.executed + 1;
        try {
            require(dispatch_page(s), "Mac match scheduler page was not compiled");
        } catch (const std::runtime_error& error) {
            if (std::string_view(error.what()) != "Native instruction budget exhausted") throw;
        }
    }
    require(destination != 0,
            "Mac background-work gate missed both original destinations");
    require(s.memory.read<4>(runtime + 4) == flags,
            "Turbo modified the guest runtime flags");
    require(s.memory.read<4>(scheduler_counter) == (tick > 100 ? 0 : tick),
            "Turbo changed the original tick counter or its reset after 100");
    return destination;
}
}

int main() {
    try {
        // This is background work, not the football-event presentation clock.
        // Execution-strategy selection must preserve the original gates.
        for (bool turbo : {false, true}) {
          for (unsigned tick = 1; tick <= 101; ++tick) {
            const auto original_tick = tick > 100 ? 0 : tick;
            require(scheduler_destination(0x80000, tick, false, turbo) ==
                    (original_tick % 6 ? skip : simulate), "Original 5-of-6 gate changed");
            require(scheduler_destination(0x40000, tick, false, turbo) ==
                    (original_tick % 3 ? skip : simulate), "Original 2-of-3 gate changed");
            require(scheduler_destination(0xc0000, tick, false, turbo) ==
                    (original_tick % 6 ? skip : simulate), "Combined original gates changed");
            require(scheduler_destination(0x100000, tick, false, turbo) == simulate,
                    "Original full-rate band was changed");
            require(scheduler_destination(0x80000, tick, true, turbo) == simulate,
                    "Original forced tick was changed");
          }
        }
        set_match_turbo_enabled(false);
        std::cout << "Original Mac background-work gates and reset preserved with both execution strategies\n";
        return 0;
    } catch (const std::exception& error) {
        set_match_turbo_enabled(false);
        std::cerr << error.what() << '\n';
        return 1;
    }
}
