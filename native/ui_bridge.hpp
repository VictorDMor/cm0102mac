#pragma once
#include <cstdint>
#include <array>
#include <filesystem>
#include <memory>
#include <vector>
#include <optional>
#include "menu_resource.hpp"
#include "dialog_resource.hpp"
#include "regions.hpp"
#include "cursor_resource.hpp"
#include "presentation.hpp"
#include "direct_commands.hpp"

namespace cm::aot {
struct NativeDrawing;
struct UIEvent {
    enum class Kind { launch, quit, open_files, mouse_down, mouse_up, key_down, update, menu, key_up, auto_key, wheel, activate, suspend_resume, mouse_moved, command };
    Kind kind;
    std::uint32_t window = 0, message = 0;
    std::int16_t x = 0, y = 0;
    std::uint16_t modifiers = 0;
    std::vector<std::filesystem::path> files{};
    std::uint32_t when = 0;
    std::uint16_t button = 0;
    std::int32_t wheel_delta = 0;
    std::uint64_t command_context = 0;
};
struct NativeEventParameter {
    std::int32_t error = 0;
    std::uint32_t type = UINT32_MAX, size = UINT32_MAX;
    std::vector<std::uint8_t> bytes;
};
std::uint16_t legacy_event_mask(const UIEvent& event);
struct NativeDisplay {
    std::uint32_t id;
    std::array<std::int16_t, 4> bounds; // Top, left, bottom, right in logical screen coordinates.
    bool main;
    bool active;
};
struct NativePointerState {
    std::int16_t x, y;
    std::uint16_t modifiers;
    bool down;
};
struct NativeWindowHit { std::int16_t part; std::uint32_t window; };
class NativeUI {
public:
    // Empty paths keep component checks entirely in memory.
    explicit NativeUI(std::filesystem::path presentation_preference = {});
    ~NativeUI();
    void initialize_cursor();
    void create_cursor(std::uint32_t token, const ColorCursor& cursor);
    void set_cursor(std::uint32_t token);
    void dispose_cursor(std::uint32_t token);
    std::array<std::uint8_t, 16> key_map() const;
    NativePointerState pointer_state() const;
    // `throttled`: the caller polls with no wait in a busy loop; pump AppKit
    // at most every 2 ms (queued input is still returned at once).
    bool poll(UIEvent& event, double timeout_seconds, bool throttled = false);
    bool take_window_paint(UIEvent& event);
    void flush_events(std::uint16_t which, std::uint16_t stop);
    void begin_event(std::uint32_t token, std::uint32_t event_class, std::uint32_t kind, const UIEvent& event);
    void end_event(std::uint32_t token);
    NativeEventParameter event_parameter(std::uint32_t token, std::uint32_t name,
                                        std::uint32_t type, std::uint32_t size, bool data);
    void create_menu(std::uint32_t token, const MenuDefinition& menu);
    void set_menu_bar(const std::vector<std::uint32_t>& menus);
    std::uint32_t menu_key(std::uint8_t key) const;
    bool menu_selection_enabled(std::uint32_t selection) const;
    void set_menu_item_enabled(std::uint32_t menu, std::uint16_t item, bool enabled);
    std::optional<std::pair<std::uint32_t, std::uint16_t>> find_menu_command(
        std::uint32_t menu, std::uint32_t command, std::uint32_t index) const;
    void delete_menu_item(std::uint32_t menu, std::uint16_t item);
    void append_menu_item(std::uint32_t menu, const MenuItem& item);
    std::uint16_t count_menu_items(std::uint32_t menu) const;
    std::string menu_item_text(std::uint32_t menu, std::uint16_t item) const;
    void set_menu_item_text(std::uint32_t menu, std::uint16_t item, const std::string& text);
    void redraw_menu_bar();
    void update_commands(const CommandContext& context, std::string_view status);
    bool set_interface_scale(InterfaceScale scale);
    InterfaceScale interface_scale() const;
    double presentation_scale() const;
    std::vector<NativeDisplay> displays() const;
    std::vector<LegacyRect> desktop_rectangles() const;
    // A modal dialog (Dialog Manager): its items as AppKit controls. As in
    // the Dialog Manager, a click reports the item and changes no control's
    // value (the application does that).
    void create_dialog(std::uint32_t token, const DialogDefinition& dialog, const std::vector<DialogItem>& items);
    bool is_dialog(std::uint32_t token) const;
    void show_dialog(std::uint32_t token);
    // Waits up to `timeout_seconds` for a click on an enabled item: its number, or 0.
    std::int16_t dialog_click(std::uint32_t token, double timeout_seconds);
    bool dialog_item_enabled(std::uint32_t token, std::int16_t item) const;
    void dispose_dialog(std::uint32_t token);
    void create_window(std::uint32_t token, std::uint32_t kind, std::uint32_t attributes, const LegacyRect& bounds);
    void dispose_window(std::uint32_t token);
    void show_window(std::uint32_t token, bool activate);
    LegacyRect window_bounds(std::uint32_t token, bool content) const;
    std::int32_t window_number(std::uint32_t token) const;
    NativeWindowHit find_window(std::int16_t x, std::int16_t y) const;
    void set_window_title(std::uint32_t token, const std::string& title);
    void present_window(std::uint32_t token, std::uint32_t width, std::uint32_t height,
                        std::uint32_t row_bytes, const std::vector<std::uint8_t>& argb,
                        std::shared_ptr<const NativeDrawing> native = {});
    std::vector<std::filesystem::path> save_window_images(const std::filesystem::path& directory) const;
private:
    struct Impl;
    std::unique_ptr<Impl> impl_;
};
}
