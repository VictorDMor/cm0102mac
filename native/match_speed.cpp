#include "match_speed.hpp"
#include "aot_runtime.hpp"

namespace cm::aot {
void insert_turbo_match_speed(State& s) {
    if (!native_match_speed_enabled()) return;
    // The original first choice is ready to be constructed. Insert Turbo with
    // the same parent and layout, then restore the first choice's arguments.
    const auto registers = s.registers;
    const auto temporaries = s.unique;
    const auto stack = std::uint32_t(s.r<4>(4));
    const auto label = std::uint32_t(s.r<4>(112));
    // Keep the localized choices' exact text inset in the shared label buffer.
    std::vector<std::uint8_t> turbo_label;
    while (turbo_label.size() < 32 && s.memory.read<1>(label + turbo_label.size()) == ' ')
        turbo_label.push_back(' ');
    turbo_label.insert(turbo_label.end(), {'T', 'u', 'r', 'b', 'o', 0});
    const auto saved_label = s.memory.copy_out(label, turbo_label.size());
    const auto arguments = s.memory.copy_out(stack + 0x38, 0x2c);
    const auto speed = native_match_speed();
    s.memory.copy_in(label, turbo_label);
    s.memory.write<4>(stack + 0x5c, turbo_match_speed);
    s.wr<4>(16, speed == turbo_match_speed ? 0x2002 : 2);
    s.wr<4>(4128, return_sentinel);
    s.pc = 0x10719740;
    run_until(s, return_sentinel);
    s.memory.copy_in(label, saved_label);
    s.memory.copy_in(stack + 0x38, arguments);
    s.registers = registers;
    s.unique = temporaries;
}

bool finish_turbo_match_speed_menu(State& s) {
    if (!native_match_speed_enabled()) return false;
    const auto manager = signed_value<2>(s.memory.read<2>(s.r<4>(80)));
    const auto root = s.memory.read<4>(std::uint32_t(s.r<4>(76) + manager * 0x2c0));
    const auto speed = native_match_speed();
    // Controls are consecutive: Turbo immediately precedes the first original
    // choice. The menu expects the predecessor of the selected control.
    const auto previous = speed == 0 ? s.r<4>(52) : speed == 20 ? s.r<4>(68) - 1 : speed == 55 ? s.r<4>(68) :
        speed == 90 ? s.r<4>(56) : speed == 125 ? s.r<4>(84) : s.r<4>(88);
    s.memory.write<2>(root + 0x12a902, previous);
    s.memory.write<1>(root + 0x12a901, 0x20);
    s.pc = 0x1028d660;
    return true;
}
}
