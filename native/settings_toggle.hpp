#pragma once
#include <cstdint>
#include <optional>
#include <string_view>

namespace cm::aot {
struct State;
// Host options appended to the original Game Settings grid as On/Off rows.
// A choice stays pending until the screen's OK; Cancel and Escape drop it.
struct SettingsToggle {
    unsigned action;  // the controls' action number, outside the original's range
    std::optional<bool> pending{};
    bool changed = false;

    void begin(bool current) { pending = current; changed = false; }
    // Appends the row inside the screen builder (its registers are live).
    void append(State&, unsigned row, std::string_view label) const;
    // Inside the action dispatcher: takes a click on this row (the caller
    // returns at once) or the original Defaults action (the caller goes on).
    bool dispatch(State&, unsigned event, bool default_value);
    // After the dispatcher: the value to commit when the screen closed with
    // OK and the choice changed. Ends the pending choice when the screen closed.
    std::optional<bool> finish(unsigned event, std::int32_t result);
};
// Game Settings rows the host adds after the original fourteen.
unsigned host_settings_rows();
// The action of the control the dispatcher was entered for.
unsigned settings_event(const State&);
}
