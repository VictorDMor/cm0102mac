#include "career_rules_option.hpp"
#include "aot_runtime.hpp"
#include "career_rules.hpp"
#include "match_debugging.hpp"
#include "new_screens.hpp"
#include "platform.hpp"
#include "settings_toggle.hpp"
#include <array>

namespace cm::aot {
namespace {
SettingsToggle toggle{realistic_rules_action};
}
bool realistic_rules_available(const State& s) {
    return s.platform && s.platform->career_present() && s.platform->career_rules().editable();
}
std::string_view realistic_rules_label(unsigned language) {
    // Single-byte game text, in the order of the game's language list.
    static constexpr std::array<std::string_view, 11> labels{
        "Realistic Rules", "Realistic Rules", "R\350gles r\351alistes", "Realistische Regeln",
        "Regole realistiche", "Reglas realistas", "Realistiske regler", "Realistische regels",
        "Realistiske regler", "Regras realistas", "Realistiska regler"};
    return labels[language < labels.size() ? language : 0];
}
bool realistic_rules_shown() { return toggle.pending.has_value(); }
void begin_realistic_rules_option(const State& s) {
    if (realistic_rules_available(s)) toggle.begin(s.platform->career_rules().historical_rules());
    else toggle.pending.reset();
}
void prepare_realistic_rules_option(const State& s) {
    if (!realistic_rules_available(s)) toggle.pending.reset();
    else if (!toggle.pending) begin_realistic_rules_option(s);
}
void append_realistic_rules_option(State& s) {
    if (!realistic_rules_shown()) return;
    toggle.append(s, 14 + unsigned(native_debugging_available()) + unsigned(new_screens_available()),
                  realistic_rules_label(s.memory.read<1>(s.r<4>(8) - 0x35ac)));
}
bool dispatch_realistic_rules_option(State& s, unsigned action) {
    // The original Defaults action leaves a career's rules alone: they are
    // the career's, not a preference of this installation.
    return action != 39 && realistic_rules_shown() && toggle.dispatch(s, action, false);
}
void finish_realistic_rules_option(State& s, unsigned action, std::int32_t result) {
    const auto commit = toggle.finish(action, result);
    if (commit && realistic_rules_available(s)) s.platform->edit_career_historical_rules(*commit);
}
}
