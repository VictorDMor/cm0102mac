#include "match_speed.hpp"
#include "aot_runtime.hpp"

namespace cm::aot {
bool append_turbo_match_speed(State& s) {
    if (!native_match_speed_enabled()) return false;
    // The original builder has emitted the five localized choices. Append a
    // sixth through that same control constructor and select its predecessor
    // in the original circular menu. No host widget or synthetic click is used.
    const auto registers = s.registers;
    const auto temporaries = s.unique;
    const auto stack = std::uint32_t(s.r<4>(4));
    const auto label = std::uint32_t(s.r<4>(112));
    const auto saved_label = s.memory.copy_out(label, 6);
    const auto arguments = s.memory.copy_out(stack + 0x38, 0x2c);
    const auto manager = signed_value<2>(s.memory.read<2>(s.r<4>(80)));
    const auto root = s.memory.read<4>(std::uint32_t(s.r<4>(76) + manager * 0x2c0));
    const auto speed = native_match_speed();
    const auto previous = speed == 0 ? s.r<4>(52) : speed == 55 ? s.r<4>(68) :
        speed == 90 ? s.r<4>(56) : speed == 125 ? s.r<4>(84) : s.r<4>(88);
    s.memory.copy_in(label, {'T', 'u', 'r', 'b', 'o', 0});
    s.memory.write<4>(stack + 0x5c, turbo_match_speed);
    s.wr<4>(12, root);
    s.wr<4>(16, speed == turbo_match_speed ? 0x2002 : 2);
    for (unsigned reg = 5; reg <= 10; ++reg) s.wr<4>(reg * 4, 0);
    s.wr<4>(4128, return_sentinel);
    s.pc = 0x10719740;
    run_until(s, return_sentinel);
    const auto turbo_control = s.r<4>(12);
    s.memory.copy_in(label, saved_label);
    s.memory.copy_in(stack + 0x38, arguments);
    s.registers = registers;
    s.unique = temporaries;
    s.memory.write<2>(root + 0x12a902, speed == 20 ? turbo_control : previous);
    s.memory.write<1>(root + 0x12a901, 0x20);
    s.pc = 0x1028d660;
    return true;
}
}
