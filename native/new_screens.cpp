#include "new_screens.hpp"
#include "debug_localization.hpp"
#include "gui_guest_scope.hpp"
#include "match_debugging.hpp"
#include <array>
#include <optional>

namespace cm::aot {
namespace {
std::optional<bool> pending;
bool changed = false;
std::uint32_t selected_control(const State& s) {
    const auto root = s.memory.read<4>(s.r<4>(8) - 0x7bb0);
    const auto page = std::uint32_t(s.memory.read<4>(root + 0x24 + std::int16_t(s.memory.read<2>(root + 0x2c58)) * 0x2c0));
    const auto index = std::int16_t(s.r<4>(12));
    const auto count = s.memory.read<2>(page + 0x129da6);
    return index >= 0 && count <= 1200 && unsigned(index) < count ? page + 0xba864 + unsigned(index) * 0x17c : 0;
}
}
std::string_view new_screens_label(unsigned language) {
    // The original font renderer consumes single-byte game text, not UTF-8.
    static constexpr std::array<std::string_view, 11> labels{
        "New Screens", "New Screens", "Nouveaux \351crans", "Neue Ansichten",
        "Nuove schermate", "Nuevas pantallas", "Nye sk\346rmbilleder", "Nieuwe schermen",
        "Nye skjermbilder", "Novos \311cr\343s", "Nya sk\344rmbilder"};
    return labels[language < labels.size() ? language : 0];
}
void begin_new_screens_options() {
    pending = new_screens_enabled();
    changed = false;
}
void append_new_screens_option(State& s) {
    if (!new_screens_available()) return;
    if (!pending) begin_new_screens_options();
    guest::Cpu c(s);
    const auto grid = c.r(13), row = c.r(17) + 14 + unsigned(native_debugging_available());
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
        c.put32(c.sp() + 0x40, new_screens_action); c.put32(c.sp() + 0x44, value);
        c.set({{3, root}, {4, grid}, {5, column}, {6, row}, {7, flags}, {8, color}, {9, font}, {10, style}});
        c.call(clickable ? 0x107444e0 : 0x107445a0, return_sentinel);
    };
    label(" " + std::string(new_screens_label(c.u8(c.r(2) - 0x35ac))));
    cell(0, 1, 0, 1, 3, title, false, 0);
    for (unsigned value = 0; value < 2; ++value) {
        const bool active = *pending == bool(value);
        label(debug_text(s, value ? DebugText::on : DebugText::off));
        cell(value + 1, active ? 0x221 : 1, active ? selected : 0, 12, 2,
             active ? selected : normal, !active, value);
    }
}
unsigned new_screens_option_event(const State& s) {
    if (!new_screens_available()) return 0;
    const auto control = selected_control(s);
    auto action = control ? std::int16_t(s.memory.read<4>(control + 8)) : std::int16_t(0);
    if (!action) action = std::int16_t(s.memory.read<2>(s.memory.read<4>(s.r<4>(8) - 0x7bb0) + 0x25819e));
    return std::uint32_t(std::int32_t(action));
}
bool dispatch_new_screens_option(State& s, unsigned action) {
    if (!new_screens_available()) return false;
    if (action == 39) { pending = default_new_screens; changed = true; }
    if (action != new_screens_action) return false;
    const auto control = selected_control(s);
    if (!control || !(s.memory.read<4>(control + 12) & 2)) return false;
    pending = s.memory.read<4>(control + 0x48) != 0; changed = true;
    s.wr<4>(12, std::uint32_t(-11)); s.pc = s.r<4>(4128);
    return true;
}
void finish_new_screens_options(unsigned action, std::int32_t result) {
    if (!new_screens_available() || result != -9 || (action != 40 && action != 41)) return;
    if (action == 41 && pending && changed) select_new_screens(*pending);
    pending.reset(); changed = false;
}
}
