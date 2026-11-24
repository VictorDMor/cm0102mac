#include "parallel_matches.hpp"
#include "settings_toggle.hpp"
#include "debug_localization.hpp"
#include "gui_guest_scope.hpp"
#include "match_debugging.hpp"
#include "new_screens.hpp"
#include <string>
#include <vector>

namespace cm::aot {
namespace {
std::uint32_t selected_control(const State& s) {
    const auto root = s.memory.read<4>(s.r<4>(8) - 0x7bb0);
    const auto page = std::uint32_t(s.memory.read<4>(root + 0x24 + std::int16_t(s.memory.read<2>(root + 0x2c58)) * 0x2c0));
    const auto index = std::int16_t(s.r<4>(12));
    const auto count = s.memory.read<2>(page + 0x129da6);
    return index >= 0 && count <= 1200 && unsigned(index) < count ? page + 0xba864 + unsigned(index) * 0x17c : 0;
}
}
unsigned host_settings_rows() {
    return unsigned(native_debugging_available()) + unsigned(new_screens_available()) + unsigned(parallel_matches_available());
}
unsigned settings_event(const State& s) {
    // Without a host row the original dispatcher reads the control itself.
    if (!new_screens_available() && !parallel_matches_available()) return 0;
    const auto control = selected_control(s);
    auto action = control ? std::int16_t(s.memory.read<4>(control + 8)) : std::int16_t(0);
    if (!action) action = std::int16_t(s.memory.read<2>(s.memory.read<4>(s.r<4>(8) - 0x7bb0) + 0x25819e));
    return std::uint32_t(std::int32_t(action));
}
void SettingsToggle::append(State& s, unsigned row_offset, std::string_view name) const {
    guest::Cpu c(s);
    const auto grid = c.r(13), row = c.r(17) + row_offset;
    const auto root = c.r(29), normal = unsigned(c.u16(c.r(2) - 0x3890));
    const auto selected = unsigned(c.u16(c.r(27))), title = unsigned(c.u16(c.toc(-0x7b10)));
    GuiGuestScope preserve(s);
    c.set(1, c.sp() - 0x400);
    const auto text = c.sp() + 0x100;
    const auto label = [&](std::string_view value) {
        std::vector<std::uint8_t> bytes(value.begin(), value.end()); bytes.push_back(0);
        s.memory.copy_in(text, bytes);
    };
    const auto cell = [&](unsigned column, unsigned flags, unsigned color, unsigned font, unsigned style,
                          unsigned ink, bool clickable, unsigned value) {
        c.put32(c.sp() + 0x38, ink); c.put32(c.sp() + 0x3c, text);
        c.put32(c.sp() + 0x40, action); c.put32(c.sp() + 0x44, value);
        c.set({{3, root}, {4, grid}, {5, column}, {6, row}, {7, flags}, {8, color}, {9, font}, {10, style}});
        c.call(clickable ? 0x107444e0 : 0x107445a0, return_sentinel);
    };
    label(" " + std::string(name));
    cell(0, 1, 0, 1, 3, title, false, 0);
    for (unsigned value = 0; value < 2; ++value) {
        const bool active = pending.value_or(false) == bool(value);
        label(debug_text(s, value ? DebugText::on : DebugText::off));
        cell(value + 1, active ? 0x221 : 1, active ? selected : 0, 12, 2,
             active ? selected : normal, !active, value);
    }
}
bool SettingsToggle::dispatch(State& s, unsigned event, bool default_value) {
    if (event == 39) { pending = default_value; changed = true; }
    if (event != action) return false;
    const auto control = selected_control(s);
    if (!control || !(s.memory.read<4>(control + 12) & 2)) return false;
    pending = s.memory.read<4>(control + 0x48) != 0; changed = true;
    s.wr<4>(12, std::uint32_t(-11)); s.pc = s.r<4>(4128);
    return true;
}
std::optional<bool> SettingsToggle::finish(unsigned event, std::int32_t result) {
    if (result != -9 || (event != 40 && event != 41)) return {};
    const auto commit = event == 41 && changed ? pending : std::nullopt;
    pending.reset(); changed = false;
    return commit;
}
}
