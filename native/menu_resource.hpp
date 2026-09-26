#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace cm::aot {
struct MenuItem {
    std::string text; // Original MacRoman bytes.
    std::uint8_t icon = 0, key = 0, mark = 0, style = 0;
    bool enabled = true;
};
struct MenuDefinition {
    std::int16_t id;
    std::string title;
    bool enabled;
    std::vector<MenuItem> items;
};
MenuDefinition parse_menu(std::span<const std::uint8_t> bytes);
std::vector<std::int16_t> parse_menu_bar(std::span<const std::uint8_t> bytes);
}
