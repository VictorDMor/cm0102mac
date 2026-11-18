#pragma once
#include <cstdint>
#include <string_view>

namespace cm::aot {
struct State;
// "Regras realistas": the career's switch for historical rule changes, as an
// On/Off row of Game Settings. Shown only while a career that this build can
// edit is in memory; the choice is saved with the career (CareerRules).
inline constexpr unsigned realistic_rules_action = 30034;
bool realistic_rules_available(const State&);
// Decided when Game Settings opens, so the grid's row count and its rows agree.
bool realistic_rules_shown();
std::string_view realistic_rules_label(unsigned language);
void begin_realistic_rules_option(const State&);
// At the start of every build of the screen, before its grid is sized: a
// rebuild keeps the pending choice, a screen reached without its opener gets one.
void prepare_realistic_rules_option(const State&);
void append_realistic_rules_option(State&);
bool dispatch_realistic_rules_option(State&, unsigned action);
void finish_realistic_rules_option(State&, unsigned action, std::int32_t result);
}
