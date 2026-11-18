#include "new_screens.hpp"
#include "aot_runtime.hpp"
#include "match_debugging.hpp"
#include "settings_toggle.hpp"
#include <array>

namespace cm::aot {
namespace {
SettingsToggle toggle{new_screens_action};
}
std::string_view new_screens_label(unsigned language) {
    // The original font renderer consumes single-byte game text, not UTF-8.
    static constexpr std::array<std::string_view, 11> labels{
        "New Screens", "New Screens", "Nouveaux \351crans", "Neue Ansichten",
        "Nuove schermate", "Nuevas pantallas", "Nye sk\346rmbilleder", "Nieuwe schermen",
        "Nye skjermbilder", "Novos \311cr\343s", "Nya sk\344rmbilder"};
    return labels[language < labels.size() ? language : 0];
}
void begin_new_screens_options() { toggle.begin(new_screens_enabled()); }
void append_new_screens_option(State& s) {
    if (!new_screens_available()) return;
    if (!toggle.pending) begin_new_screens_options();
    toggle.append(s, 14 + unsigned(native_debugging_available()),
                  new_screens_label(s.memory.read<1>(s.r<4>(8) - 0x35ac)));
}
bool dispatch_new_screens_option(State& s, unsigned action) {
    return new_screens_available() && toggle.dispatch(s, action, default_new_screens);
}
void finish_new_screens_options(unsigned action, std::int32_t result) {
    if (!new_screens_available()) return;
    if (const auto commit = toggle.finish(action, result)) select_new_screens(*commit);
}
}
