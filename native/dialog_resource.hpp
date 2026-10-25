#pragma once
// DLOG and DITL resources (the Rez layouts in the macOS SDK's Dialogs.r), for
// the host's Dialog Manager.
#include "regions.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace cm::aot {
struct DialogItem {
    enum Kind : std::uint8_t { user = 0, button = 4, check_box = 5, radio_button = 6, control = 7, static_text = 8,
                               edit_text = 16, icon = 32, picture = 64 };
    std::uint8_t type = 0;  // the kind, plus 128 when the item is disabled
    LegacyRect bounds{};    // top, left, bottom, right, in the dialog
    std::string text;       // MacRoman; empty for user, icon and picture items
    Kind kind() const { return Kind(type & 0x7f); }
    bool enabled() const { return (type & 0x80) == 0; }
};
struct DialogDefinition {
    LegacyRect bounds{};  // global
    std::string title;
    std::int16_t items = 0;  // the DITL's id
};
DialogDefinition parse_dialog(std::span<const std::uint8_t> bytes);
std::vector<DialogItem> parse_dialog_items(std::span<const std::uint8_t> bytes);
}
