#include "resource_fork.hpp"
#include "menu_resource.hpp"
#include "ui_bridge.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <stdexcept>

namespace {
void require(bool condition, const char* message) { if (!condition) throw std::runtime_error(message); }
}
int main(int argc, char** argv) {
    if (argc != 2) { std::cerr << "Usage: cm-menu-checks <original-application>\n"; return 2; }
    try {
        std::ifstream file(std::filesystem::path(argv[1]) / "..namedfork/rsrc", std::ios::binary);
        if (!file) throw std::runtime_error("Cannot open original resource fork");
        const std::vector<std::uint8_t> bytes((std::istreambuf_iterator<char>(file)), {});
        cm::ResourceFork resources(bytes);
        cm::aot::NativeUI ui;
        const auto* color_cursor = resources.find(0x63727372, 129);
        require(color_cursor != nullptr, "Original color cursor is missing");
        const auto cursor = cm::aot::parse_color_cursor(color_cursor->data);
        require(cursor.x == 8 && cursor.y == 8 && cursor.rgba.size() == 1024,
                "Original color cursor hotspot or pixel dimensions");
        unsigned opaque = 0, transparent = 0;
        for (unsigned i = 3; i < cursor.rgba.size(); i += 4) {
            opaque += cursor.rgba[i] == 255; transparent += cursor.rgba[i] == 0;
        }
        require(opaque && transparent && opaque + transparent == 256, "Original cursor alpha mask was lost");
        for (std::size_t size = 0; size < color_cursor->data.size(); ++size) {
            bool rejected = false;
            try { (void)cm::aot::parse_color_cursor(std::span(color_cursor->data).first(size)); }
            catch (const std::runtime_error&) { rejected = true; }
            require(rejected, "Truncated color cursor was accepted");
        }
        ui.create_cursor(900, cursor);
        for (std::int16_t id : {130, 131, 132}) {
            const auto* resource = resources.find(0x43555253, id);
            require(resource != nullptr, "Original monochrome cursor is missing");
            const auto mono = cm::aot::parse_monochrome_cursor(resource->data);
            for (unsigned i = 0; i < 256; ++i) {
                require(mono.rgba[i * 4] == mono.rgba[i * 4 + 1] && mono.rgba[i * 4] == mono.rgba[i * 4 + 2],
                        "Monochrome cursor acquired a color tint");
                const bool visible = resource->data[32 + i / 8] & (0x80 >> (i % 8));
                require(mono.rgba[i * 4 + 3] == (visible ? 255 : 0), "Monochrome cursor alpha differs from its original mask");
            }
            ui.create_cursor(id, mono);
        }
        unsigned menus = 0, items = 0;
        for (const auto& resource : resources.resources()) {
            if (resource.type != 0x4d454e55) continue;
            const auto menu = cm::aot::parse_menu(resource.data);
            require(menu.id == resource.id, "Original MENU ID mismatch");
            ui.create_menu(menu.id, menu);
            for (std::size_t i = 0; i < menu.items.size(); ++i) {
                require(ui.menu_item_text(menu.id, i + 1) == menu.items[i].text,
                        "AppKit menu item differs from original resource");
                ++items;
            }
            for (std::size_t size = 0; size < resource.data.size(); ++size) {
                bool rejected = false;
                try { (void)cm::aot::parse_menu(std::span(resource.data).first(size)); }
                catch (const std::runtime_error&) { rejected = true; }
                require(rejected, "Truncated MENU resource was accepted");
            }
            ++menus;
        }
        const auto* bar = resources.find(0x4d424152, 1);
        require(bar != nullptr, "Original MBAR is missing");
        const auto ids = cm::aot::parse_menu_bar(bar->data);
        require(ids == std::vector<std::int16_t>({1, 2, 3}), "Original startup menu order");
        ui.set_menu_bar({ids.begin(), ids.end()});
        const auto duplicate = ui.find_menu_command(0, 0x71756974, 2);
        require(duplicate && duplicate->first == 2 && duplicate->second == 5,
                "Original duplicate Quit lookup did not find File/Quit");
        ui.delete_menu_item(duplicate->first, duplicate->second);
        ui.delete_menu_item(duplicate->first, duplicate->second - 1);
        require(ui.count_menu_items(2) == 3 && ui.menu_item_text(2, 3) == std::string("Page Setup") + char(0xc9),
                "Original startup menu deletions changed unrelated items");
        require(!ui.find_menu_command(0, 0x71756974, 2), "Deleted command remains discoverable");
        const auto translated = std::string("Mise en page") + char(0xc9);
        ui.set_menu_item_text(2, 3, translated);
        require(ui.menu_item_text(2, 3) == translated && ui.count_menu_items(2) == 3,
                "Native localized menu title update changed encoding or item count");
        ui.set_menu_item_enabled(2, 3, false);
        require(!ui.menu_selection_enabled(0x00020003), "Disabled original menu selection accepted");
        ui.set_menu_item_enabled(2, 3, true);
        require(ui.menu_selection_enabled(0x00020003) && !ui.menu_selection_enabled(0x00020002) &&
            !ui.menu_selection_enabled(0x00020005), "MenuSelect must reject separator/deleted item identities");
        ui.set_menu_item_enabled(2, 0, false);
        require(!ui.menu_selection_enabled(0x00020003), "Disabled whole menu accepted a selection");
        ui.set_menu_item_enabled(2, 0, true);
        ui.set_menu_bar({ids.begin(), ids.end()});
        require(ui.menu_selection_enabled(0x00020003) && ui.menu_item_text(2, 3) == translated,
            "Repeated SetMenuBar changed original selection identity or title");
        ui.redraw_menu_bar();
        std::cout << menus << " original menus and " << items << " item texts verified in native AppKit menus\n";
        return 0;
    } catch (const std::exception& error) { std::cerr << error.what() << '\n'; return 1; }
}
