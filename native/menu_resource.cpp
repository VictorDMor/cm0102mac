#include "menu_resource.hpp"
#include <stdexcept>

namespace cm::aot {
namespace {
struct Reader {
    std::span<const std::uint8_t> bytes;
    std::size_t position = 0;
    std::uint8_t byte() {
        if (position >= bytes.size()) throw std::runtime_error("Truncated menu resource");
        return bytes[position++];
    }
    std::uint16_t word() { const auto high = byte(); return (high << 8) | byte(); }
    std::uint32_t longword() { const auto high = word(); return (std::uint32_t(high) << 16) | word(); }
    std::string string() {
        const auto size = byte();
        std::string result;
        for (unsigned i = 0; i < size; ++i) result += char(byte());
        return result;
    }
};
}
MenuDefinition parse_menu(std::span<const std::uint8_t> bytes) {
    // The Rez MENU layout in the macOS SDK's Menus.r, in original byte order.
    Reader reader{bytes};
    MenuDefinition menu{};
    menu.id = std::int16_t(reader.word());
    reader.word(); reader.word();
    if (reader.word() != 0) throw std::runtime_error("Custom menu drawing procedure is unsupported");
    reader.word();
    const auto flags = reader.longword();
    menu.enabled = (flags & 1) != 0;
    menu.title = reader.string();
    for (unsigned index = 1;; ++index) {
        auto title = reader.string();
        if (title.empty()) break;
        MenuItem item;
        item.text = std::move(title);
        item.icon = reader.byte(); item.key = reader.byte();
        item.mark = reader.byte(); item.style = reader.byte();
        item.enabled = index > 31 || ((flags >> index) & 1);
        menu.items.push_back(std::move(item));
    }
    if (reader.position != bytes.size()) throw std::runtime_error("Unexpected trailing MENU data");
    return menu;
}
std::vector<std::int16_t> parse_menu_bar(std::span<const std::uint8_t> bytes) {
    Reader reader{bytes};
    const auto count = reader.word();
    std::vector<std::int16_t> result;
    for (unsigned i = 0; i < count; ++i) result.push_back(std::int16_t(reader.word()));
    if (reader.position != bytes.size()) throw std::runtime_error("Unexpected trailing MBAR data");
    return result;
}
}
