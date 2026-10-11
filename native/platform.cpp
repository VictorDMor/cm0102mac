#include "platform.hpp"
#include "aot_runtime.hpp"
#include "text_bridge.hpp"
#include "process_bridge.hpp"
#include "ui_bridge.hpp"
#include "native_drawing.hpp"
#include "direct_commands.hpp"
#include "resource_bridge.hpp"
#include "filesystem_bridge.hpp"
#include "directory_catalog.hpp"
#include "printing_bridge.hpp"
#include "callbacks.hpp"
#include "apple_events.hpp"
#include "data_forks.hpp"
#include "fork_io_bridge.hpp"
#include "unicode_bridge.hpp"
#include "file_reference_bridge.hpp"
#include "sound_bridge.hpp"
#include "performance_trace.hpp"
#include <bit>
#include <deque>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <CoreFoundation/CoreFoundation.h>
#include <map>
#include <set>
#include <optional>
#include <algorithm>
#include <type_traits>
#include <sys/stat.h>
#include <sys/xattr.h>
#include <copyfile.h>
#include <cerrno>

namespace cm::aot {
namespace fs = std::filesystem;
namespace {
constexpr std::int16_t no_err = 0, file_not_found = -43, directory_not_found = -120;
constexpr std::int16_t bad_name = -37, volume_not_found = -35, parameter_error = -50;
struct FileError { std::int16_t code; };

std::string convert(const std::string& input, CFStringEncoding from, CFStringEncoding to) {
    CFStringRef value = CFStringCreateWithBytes(nullptr, reinterpret_cast<const UInt8*>(input.data()),
                                               input.size(), from, false);
    if (!value) throw FileError{bad_name};
    const auto length = CFStringGetLength(value);
    const auto maximum = CFStringGetMaximumSizeForEncoding(length, to);
    std::string result(maximum, '\0');
    CFIndex written = 0;
    const auto converted = CFStringGetBytes(value, CFRangeMake(0, length), to, 0, false,
                          reinterpret_cast<UInt8*>(result.data()), maximum, &written);
    CFRelease(value);
    if (converted != length) throw FileError{bad_name};
    result.resize(written);
    return result;
}
// Native File Manager creation decomposes accented filenames; original
// MacRoman paths are composed. Compare canonical equivalents without erasing
// accents (kCFCompareDiacriticInsensitive would change filename identity).
constexpr auto filename_comparison = kCFCompareCaseInsensitive | kCFCompareNonliteral;
bool same_name(const std::string& a, const std::string& b) {
    auto x = CFStringCreateWithCString(nullptr, a.c_str(), kCFStringEncodingUTF8);
    auto y = CFStringCreateWithCString(nullptr, b.c_str(), kCFStringEncodingUTF8);
    const bool same = x && y && CFStringCompare(x, y, filename_comparison) == kCFCompareEqualTo;
    if (x) CFRelease(x);
    if (y) CFRelease(y);
    return same;
}
std::string read_mac_string(State& s, std::uint32_t address) {
    if (!address) return {};
    const auto bytes = s.memory.copy_out(address + 1, s.memory.read<1>(address));
    return convert({bytes.begin(), bytes.end()}, kCFStringEncodingMacRoman, kCFStringEncodingUTF8);
}
bool under(const fs::path& path, const fs::path& root) {
    auto a = path.begin(), b = root.begin();
    for (; b != root.end(); ++a, ++b) if (a == path.end() || *a != *b) return false;
    return true;
}
void write_mac_string(State& s, std::uint32_t address, const std::string& value) {
    const auto bytes = convert(value, kCFStringEncodingUTF8, kCFStringEncodingMacRoman);
    if (bytes.size() > 255) throw FileError{bad_name};
    s.memory.write<1>(address, bytes.size());
    s.memory.copy_in(address + 1, {bytes.begin(), bytes.end()});
}
std::array<std::uint8_t, 32> finder_info(const fs::path& path) {
    std::array<std::uint8_t, 32> data{};
    // A file without Finder metadata has an all-zero FinderInfo record.
    const auto size = getxattr(path.c_str(), "com.apple.FinderInfo", data.data(), data.size(), 0, 0);
    if (size < 0) data.fill(0);
    else if (size != 32) throw FileError{parameter_error};
    return data;
}
std::uint32_t mac_date(std::int64_t unix_seconds) {
    return std::uint32_t(std::clamp<std::int64_t>(unix_seconds + 2082844800LL, 0, UINT32_MAX));
}
}
struct NativePlatform::Impl {
    NativeDrawingObserver drawing;
    explicit Impl(AudioMode mode) : sound(mode) {}
    NativeSound sound;
    PerformanceTrace performance;
    struct Handle { std::uint32_t payload, size; bool locked = false; };
    std::map<std::uint32_t, Handle> handles;
    // Owned PixMap, pixels, and palette handles for each GetCCursor result.
    std::map<std::uint32_t, std::array<std::uint32_t, 3>> color_cursors;
    std::vector<std::uint32_t> master_slots;
    std::unique_ptr<NativeUI> ui;
    std::unique_ptr<NativeResources> resources;
    NativePrinting printing;
    NativeDataForks data_forks;
    NativeUnicode unicode;
    NativeAppleEvents apple_events;
    std::deque<UIEvent> pending_events;
    std::map<std::array<std::uint8_t, 16>, UIEvent> delivered_events;
    std::uint32_t last_event_tick = 0;
    std::optional<std::uint32_t> menu_selection;
    // No drawing port is selected before the first window/offscreen port exists.
    std::uint32_t current_port = 0;
    std::uint32_t desktop_region = 0;
    std::uint32_t next_window = 0x52000000, next_window_handler = 0x53000000;
    std::set<std::uint32_t> windows;
    struct WindowHandler {
        std::uint32_t target, routine, reference;
        std::vector<std::pair<std::uint32_t, std::uint32_t>> types;
    };
    std::map<std::uint32_t, WindowHandler> window_handlers;
    struct Port {
        std::uint32_t pixels, pixel_map, color_table, row_bytes, window = 0;
        std::uint16_t depth;
        LegacyRect bounds;
        bool locked = false;
        std::uint32_t foreground = 0;
    };
    std::uint32_t next_port = 0x55000000;
    std::map<std::uint32_t, Port> ports;
    std::map<std::uint32_t, std::uint32_t> window_ports;
    std::uint32_t painting_window = 0;
    struct CarbonEvent { std::uint32_t event_class, kind, window; };
    std::uint32_t next_carbon_event = 0x54000000;
    std::map<std::uint32_t, CarbonEvent> carbon_events;
    std::uint32_t next_string = 0x56000000;
    std::map<std::uint32_t, CFStringRef> strings;
    ~Impl() { for (const auto& [token, string] : strings) { (void)token; CFRelease(string); } }
    struct GraphicsDevice { std::uint32_t handle, pixel_map, color_table; };
    std::map<std::uint32_t, GraphicsDevice> graphics_devices;
    std::map<std::uint64_t, std::uint32_t> resource_handles;
    std::map<std::uint32_t, std::vector<std::uint32_t>> menu_bars;
    std::map<std::int16_t, std::uint32_t> menu_ids;
    struct EventHandler { std::uint32_t routine, reference; };
    std::map<std::pair<std::uint32_t, std::uint32_t>, EventHandler> apple_handlers;
    fs::path game, writable;
    std::map<std::uint32_t, fs::path> directories;
    std::map<fs::path, std::uint32_t> file_ids;
    std::uint32_t next_directory = 16, current_directory = 2;
    std::int16_t current_volume = -1;

    fs::path overlay() const { return writable / "Files"; }
    fs::path logical_path(const fs::path& input) const {
        if (input == overlay()) return game;
        return under(input, overlay()) ? (game / input.lexically_relative(overlay())).lexically_normal() : input;
    }
    bool volume_root(const fs::path& input) const { return logical_path(input) == game || input == writable; }
    mutable NativeDirectoryCatalog catalog;
    std::array<fs::path, 3> catalog_paths(const fs::path& input) const {
        const auto logical = logical_path(input);
        if (under(logical, game)) return {logical, overlay() / logical.lexically_relative(game), {}};
        return {input, {}, input == writable ? overlay() : fs::path{}};
    }
    const std::vector<NativeDirectoryCatalog::Entry>& listing(const fs::path& input) const {
        const auto paths = catalog_paths(input);
        return catalog.list(paths[0], paths[1], paths[2]);
    }
    std::vector<fs::path> entries(const fs::path& input) const {
        std::vector<fs::path> result;
        for (const auto& entry : listing(input)) result.push_back(entry.path);
        return result;
    }
    fs::path visible_child(const fs::path& parent, const std::string& name) const {
        const auto paths = catalog_paths(parent);
        const auto found = catalog.child(paths[0], paths[1], paths[2], name);
        return found.empty() ? parent / name : found;
    }
    fs::path writable_path(const fs::path& input) const {
        const auto logical = logical_path(input);
        return allowed(under(logical, game) ? overlay() / logical.lexically_relative(game) : input);
    }
    fs::path writable_file(const fs::path& input) const {
        if (!fs::is_regular_file(input)) throw FileError{file_not_found};
        const auto output = writable_path(input);
        if (output == input) return output;
        fs::create_directories(output.parent_path());
        // Preserve data, resource fork, and Finder metadata in private backing.
        // The native write handle never points at the original media file.
        if (!fs::exists(output) && copyfile(input.c_str(), output.c_str(), nullptr, COPYFILE_ALL | COPYFILE_EXCL))
            throw FileError{errno == EACCES ? std::int16_t(-54) : parameter_error};
        fs::permissions(output, fs::perms::owner_read | fs::perms::owner_write, fs::perm_options::add);
        return output;
    }
    std::int16_t delete_object(const fs::path& target) {
        if (!fs::exists(fs::symlink_status(target))) return file_not_found;
        if (volume_root(target) || target == overlay() ||
            logical_path(target) == logical_path(directory(current_volume, current_directory))) return -47;
        // Original media is immutable. Removing a private replacement must not
        // reveal the original again: that case needs persistent tombstones.
        const auto logical = logical_path(target);
        if (!under(fs::canonical(target.parent_path()), writable) ||
            (under(logical, game) && fs::exists(fs::symlink_status(logical))))
            throw std::runtime_error("Deleting an original media entry requires private tombstone support: " + target.string());
        const auto status = native_delete_object(target);
        if (!status) {
            file_ids.erase(logical);
            std::erase_if(directories, [&](const auto& entry) { return logical_path(entry.second) == logical; });
        }
        return status;
    }

    std::int16_t rename_object(const fs::path& target, const std::string& name, std::uint32_t encoding = UINT32_MAX) {
        if (name.empty() || name == "." || name == ".." || name.find_first_of(":/") != std::string::npos ||
            name.find('\0') != std::string::npos) return bad_name;
        if (!fs::exists(fs::symlink_status(target))) return file_not_found;
        const auto logical = logical_path(target);
        // Renaming original entries requires a tombstone so the old media
        // name cannot reappear. Keep this unsupported case mutation-free.
        if (volume_root(target) || target == overlay() ||
            !under(fs::canonical(target.parent_path()), writable) ||
            (under(logical, game) && fs::exists(fs::symlink_status(logical))))
            throw std::runtime_error("Renaming an original media entry requires private tombstone support: " + target.string());
        const auto destination = visible_child(target.parent_path(), name);
        if (fs::exists(fs::symlink_status(destination)) && !fs::equivalent(target, destination)) return -48;
        const auto error = native_rename_object(target, name, encoding);
        if (error) return error;
        const auto renamed = fs::canonical(target.parent_path() / name);
        const auto new_logical = logical_path(renamed);
        // PBHRename preserves file and directory IDs, including descendants of
        // a renamed directory and the current-directory binding.
        std::map<fs::path, std::uint32_t> renamed_ids;
        for (const auto& [path, id] : file_ids)
            renamed_ids.emplace(path == logical ? new_logical :
                under(path, logical) ? new_logical / path.lexically_relative(logical) : path, id);
        file_ids.swap(renamed_ids);
        for (auto& [id, path] : directories) {
            (void)id;
            if (path == target) path = renamed;
            else if (under(path, target)) path = renamed / path.lexically_relative(target);
        }
        return 0;
    }

    NativeUI& user_interface() {
        if (!ui) ui = std::make_unique<NativeUI>(writable.parent_path() / "Presentation.plist");
        return *ui;
    }
    std::uint32_t new_port(State& s, std::uint16_t depth, const LegacyRect& bounds) {
        if (depth != 16 && depth != 32) throw std::runtime_error("Unsupported original GWorld pixel depth");
        const auto width = std::int32_t(bounds[3]) - bounds[1], height = std::int32_t(bounds[2]) - bounds[0];
        if (width <= 0 || height <= 0) throw FileError{parameter_error};
        const auto row_bytes = (std::uint32_t(width) * (depth / 8) + 3u) & ~3u;
        if (row_bytes > 0x3ffe || std::uint64_t(row_bytes) * height > 0x10000000) throw FileError{-108};
        const auto pixels = allocate(s, row_bytes * height, true);
        if (!pixels) throw FileError{-108};
        const auto pixel_map = new_handle(s, 50), table = new_handle(s, 16);
        if (!pixel_map || !table) {
            if (pixel_map) dispose_handle(s, pixel_map);
            if (table) dispose_handle(s, table);
            deallocate(s, pixels);
            throw FileError{-108};
        }
        const auto map = handles.at(pixel_map).payload, colors = handles.at(table).payload;
        s.memory.clear(map, 50); s.memory.clear(colors, 16);
        s.memory.write<4>(map, pixels);
        s.memory.write<2>(map + 4, row_bytes | 0x8000);
        for (unsigned i = 0; i < 4; ++i) s.memory.write<2>(map + 6 + i * 2, std::uint16_t(bounds[i]));
        s.memory.write<4>(map + 22, 72u << 16); s.memory.write<4>(map + 26, 72u << 16);
        s.memory.write<2>(map + 30, 16); // RGBDirect
        s.memory.write<2>(map + 32, depth);
        s.memory.write<2>(map + 34, 3); s.memory.write<2>(map + 36, depth == 16 ? 5 : 8);
        s.memory.write<4>(map + 42, table);
        s.memory.write<4>(colors, depth); // Stable direct-color compatibility table.
        const auto token = next_port++;
        ports.emplace(token, Port{pixels, pixel_map, table, row_bytes, 0, depth, bounds});
        drawing.register_surface(s, pixels, width, height, row_bytes, depth);
        return token;
    }
    Port& port_for_pixels(std::uint32_t pixel_map) {
        const auto found = std::find_if(ports.begin(), ports.end(), [pixel_map](const auto& item) {
            return item.second.pixel_map == pixel_map;
        });
        if (found == ports.end()) throw std::runtime_error("Unknown native offscreen PixMap handle");
        return found->second;
    }
    Port& port_for_bitmap(std::uint32_t address) {
        const auto found = std::find_if(ports.begin(), ports.end(), [&](const auto& item) {
            return handles.at(item.second.pixel_map).payload == address;
        });
        if (found == ports.end()) throw std::runtime_error("CopyBits received an unknown bitmap record");
        return found->second;
    }
    std::uint32_t window_port(State& s, std::uint32_t window) {
        if (auto found = window_ports.find(window); found != window_ports.end()) return found->second;
        const auto global = user_interface().window_bounds(window, true);
        const LegacyRect bounds{0, 0, std::int16_t(global[2] - global[0]), std::int16_t(global[3] - global[1])};
        // A spanning desktop window can be wider than a 32-bit QuickDraw row
        // can encode. RGB555 keeps its original PixMap row valid; Quartz still
        // receives a full 32-bit presentation image.
        const auto token = new_port(s, bounds[3] <= 4095 ? 32 : 16, bounds);
        ports.at(token).window = window;
        window_ports.emplace(window, token);
        return token;
    }
    void present(State& s, const Port& port) {
        if (!port.window) return;
        const auto width = std::int32_t(port.bounds[3]) - port.bounds[1];
        const auto height = std::int32_t(port.bounds[2]) - port.bounds[0];
        const auto original = s.memory.copy_out(port.pixels, port.row_bytes * height);
        const auto native = drawing.snapshot(s, port.pixels);
        if (port.depth == 32) user_interface().present_window(port.window, width, height, port.row_bytes, original, native);
        else {
            std::vector<std::uint8_t> argb(std::size_t(width) * height * 4);
            for (int y = 0; y < height; ++y) for (int x = 0; x < width; ++x) {
                const auto pixel = read_be<2>(original.data() + y * port.row_bytes + x * 2);
                const auto at = (std::size_t(y) * width + x) * 4;
                for (unsigned i = 0; i < 3; ++i) {
                    const auto channel = (pixel >> (10 - i * 5)) & 31;
                    argb[at + 1 + i] = (channel << 3) | (channel >> 2);
                }
            }
            user_interface().present_window(port.window, width, height, width * 4, argb, native);
        }
        performance.mark("frame", s, port.window);
    }
    bool window_event(State& s, const UIEvent& event) {
        if (!windows.contains(event.window)) return false;
        bool handled = false;
        const auto previous_port = current_port, previous_window = painting_window;
        for (auto it = window_handlers.rbegin(); it != window_handlers.rend(); ++it) {
            const auto& [token, handler] = *it;
            if (handler.target != event.window) continue;
            std::uint32_t kind = 0, event_class = 0;
            for (const auto& type : handler.types) {
                if ((event.kind == UIEvent::Kind::update && type.first == 0x77696e64 && (type.second == 1013 || type.second == 2)) ||
                    (event.kind == UIEvent::Kind::mouse_down && type == std::pair<std::uint32_t, std::uint32_t>{0x77696e64, 85}) ||
                    (event.kind == UIEvent::Kind::wheel && type == std::pair<std::uint32_t, std::uint32_t>{0x6d6f7573, 10})) {
                    event_class = type.first; kind = type.second; break;
                }
            }
            if (!kind) continue;
            try { if (event.kind == UIEvent::Kind::update) current_port = window_port(s, event.window); }
            catch (const FileError& error) {
                throw std::runtime_error("Cannot allocate the native paint surface: " + std::to_string(error.code));
            }
            painting_window = kind == 1013 ? event.window : 0;
            const auto event_ref = next_carbon_event++;
            user_interface().begin_event(event_ref, event_class, kind, event);
            carbon_events.emplace(event_ref, CarbonEvent{event_class, kind, event.window});
            const std::array<std::uint32_t, 3> args{token, event_ref, handler.reference};
            try { handled = call_original(s, handler.routine, args) == 0; }
            catch (...) {
                user_interface().end_event(event_ref); carbon_events.erase(event_ref);
                current_port = previous_port; painting_window = previous_window; throw;
            }
            user_interface().end_event(event_ref);
            carbon_events.erase(event_ref);
            current_port = previous_port; painting_window = previous_window;
            if (handled) break;
        }
        return handled;
    }
    void paint_events(State& s) {
        UIEvent event{UIEvent::Kind::update};
        while (user_interface().take_window_paint(event)) {
            if (!window_event(s, event)) pending_events.push_back(std::move(event));
        }
    }
    std::uint32_t main_device(State& s) {
        const auto displays = user_interface().displays();
        std::uint32_t main = 0, previous = 0;
        for (const auto& display : displays) {
            auto found = graphics_devices.find(display.id);
            if (found == graphics_devices.end()) {
                const auto device = new_handle(s, 62), pixels = new_handle(s, 50), table = new_handle(s, 16);
                if (!device || !pixels || !table) throw std::runtime_error("Native graphics device metadata allocation failed");
                s.memory.clear(handles.at(table).payload, 16);
                s.memory.write<4>(handles.at(table).payload, 24); // Direct-color compatibility table seed.
                found = graphics_devices.emplace(display.id, GraphicsDevice{device, pixels, table}).first;
            }
            const auto& device = found->second;
            const auto record = handles.at(device.handle).payload;
            const auto pixels = handles.at(device.pixel_map).payload;
            const auto width = std::int32_t(display.bounds[3]) - display.bounds[1];
            if (width <= 0 || width * 4 > 0x3ffe) throw std::runtime_error("Native screen row exceeds original PixMap limits");
            s.memory.clear(record, 62);
            s.memory.clear(pixels, 50);
            s.memory.write<2>(record + 4, 2); // directType
            s.memory.write<2>(record + 20, 0x2001 | (display.main ? 0x0800 : 0) | (display.active ? 0x8000 : 0));
            s.memory.write<4>(record + 22, device.pixel_map);
            for (unsigned i = 0; i < 4; ++i) {
                s.memory.write<2>(record + 34 + i * 2, std::uint16_t(display.bounds[i]));
                s.memory.write<2>(pixels + 6 + i * 2, std::uint16_t(display.bounds[i]));
            }
            // Quartz presentation uses 32-bit RGB storage. The system display
            // has no directly mapped legacy framebuffer; window/offscreen ports
            // own their pixels separately, so this metadata's baseAddr is null.
            s.memory.write<2>(pixels + 4, 0x8000 | (width * 4));
            s.memory.write<4>(pixels + 22, 72u << 16);
            s.memory.write<4>(pixels + 26, 72u << 16);
            s.memory.write<2>(pixels + 30, 16); // RGBDirect
            s.memory.write<2>(pixels + 32, 32);
            s.memory.write<2>(pixels + 34, 3);
            s.memory.write<2>(pixels + 36, 8);
            s.memory.write<4>(pixels + 42, device.color_table);
            if (previous) s.memory.write<4>(handles.at(previous).payload + 30, device.handle);
            previous = device.handle;
            if (display.main) main = device.handle;
        }
        if (!main) throw std::runtime_error("Native main display is absent from the connected display list");
        return main;
    }
    std::array<std::uint8_t, 16> event_record(State& s, std::uint32_t address) const {
        const auto bytes = s.memory.copy_out(address, 16);
        std::array<std::uint8_t, 16> record;
        std::copy(bytes.begin(), bytes.end(), record.begin());
        return record;
    }
    std::int16_t read_data(State& s, std::int16_t ref, std::uint16_t mode, std::int32_t offset,
                         std::uint32_t buffer, std::int32_t requested, std::uint32_t& actual) {
        return read_guest_fork(s, data_forks, ref, mode, offset, buffer, requested, actual);
    }
    std::int16_t write_data(State& s, std::int16_t ref, std::uint16_t mode, std::int32_t offset,
                           std::uint32_t buffer, std::int32_t requested, std::uint32_t& actual) {
        return write_guest_fork(s, data_forks, ref, mode, offset, buffer, requested, actual);
    }
    std::optional<std::pair<std::uint32_t, std::uint16_t>> delivered_command_action;
    CommandContext commands;
    std::string command_status = "Ready";
    bool next_event(State& s, std::uint16_t mask, std::uint32_t address, std::uint32_t sleep, std::uint32_t mouse_region) {
        // A native command is an edge, not a held action. At the next original
        // event boundary its handler has consumed the input (or entered a
        // nested dialog). Withdraw only the mailbox value owned by this event.
        if (delivered_command_action) {
            const auto [root, action] = *delivered_command_action;
            if (s.memory.read<2>(root + 0x25819e) == action) s.memory.write<2>(root + 0x25819e, 0);
            delivered_command_action.reset();
        }
        commands = original_command_context(s, mask);
        if (commands.ready) {
            if (commands.handler == 0x100ee9d0) command_status = "Original file dialog: awaiting interaction";
            else if (commands.handler == 0x1033f840) command_status = "Original options: awaiting interaction";
            else if (commands.handler == 0x100f0ba0) command_status = "Original confirmation: awaiting interaction";
            else if (command_status.starts_with("Dispatched") || command_status.starts_with("Original"))
                command_status = "Original game returned to an interactive screen";

        }
        user_interface().update_commands(commands, commands.ready ? command_status : "Unavailable while processing");
        auto matches = [&](const UIEvent& event) { return (legacy_event_mask(event) & mask) != 0; };
        const auto deadline = std::chrono::steady_clock::now() + std::chrono::duration<double>(sleep / 60.0);
        auto pointer = user_interface().pointer_state();
        auto mouse_outside = [&] {
            if (!mouse_region || !(mask & 0x8000)) return false;
            const auto& region = handles.at(mouse_region);
            return !region_contains(s.memory.copy_out(region.payload, region.size), pointer.x, pointer.y);
        };
        auto found = std::find_if(pending_events.begin(), pending_events.end(), matches);
        while (found == pending_events.end()) {
            sound.poll(s);
            auto timeout = std::max(0.0, std::chrono::duration<double>(deadline - std::chrono::steady_clock::now()).count());
            if (mouse_region) timeout = mouse_outside() ? 0 : std::min(timeout, 1.0 / 60);
            // Native audio completion must reach the original game while its
            // event loop is sleeping, without calling game code on audio threads.
            if (sound.has_pending_commands()) timeout = std::min(timeout, 1.0 / 60);
            UIEvent event{UIEvent::Kind::update};
            const auto available = user_interface().poll(event, timeout);
            if (available && (event.kind == UIEvent::Kind::command || event.kind == UIEvent::Kind::menu)) {
                if (!commands.ready || (event.kind == UIEvent::Kind::command && event.command_context != commands.identity)) {
                    command_status = "Refused: the game context changed";
                    user_interface().update_commands({}, command_status);
                    continue;
                }
                if (event.kind == UIEvent::Kind::command) {
                    if (event.message >= direct_command_count) continue;
                    const auto command = DirectCommand(event.message);
                    std::fprintf(stderr, "Native command: %s action=%d control=%d context=%016llx\n",
                        std::string(direct_command_names[std::size_t(command)]).c_str(), commands.actions[std::size_t(command)],
                        commands.controls[std::size_t(command)], static_cast<unsigned long long>(commands.identity));
                    if (command == DirectCommand::save_game && commands.actions[std::size_t(command)]) {
                        event.kind = UIEvent::Kind::menu;
                        event.message = 0x00020001;
                        command_status = "Save dispatched to original File menu";
                        // Disable duplicates, retaining the original File item's
                        // own preparation/validation until MenuSelect returns.
                    } else {
                        const bool delivered = deliver_original_command(s, commands, command);
                        command_status = delivered ? "Dispatched to the original game" : "Refused: action unavailable";
                        user_interface().update_commands({}, command_status);
                        s.memory.clear(address, 16);
                        return false;
                    }
                }
            }
            if (available) {
                const auto handled = window_event(s, event);
                if (!handled && event.kind != UIEvent::Kind::wheel) pending_events.push_back(std::move(event));
            }
            pointer = user_interface().pointer_state();
            found = std::find_if(pending_events.begin(), pending_events.end(), matches);
            if (!available && (mouse_outside() || std::chrono::steady_clock::now() >= deadline)) break;
        }
        const auto tick = native_tick_count();
        s.memory.clear(address, 16);
        s.memory.write<4>(address + 6, tick);
        s.memory.write<2>(address + 10, std::uint16_t(pointer.y));
        s.memory.write<2>(address + 12, std::uint16_t(pointer.x));
        s.memory.write<2>(address + 14, pointer.modifiers);
        if (found == pending_events.end()) {
            if (!mouse_outside()) return false;
            s.memory.write<2>(address, 15); // osEvt / mouseMovedMessage
            s.memory.write<4>(address + 2, 0xfa000000);
            return true;
        }
        if (found->kind == UIEvent::Kind::open_files)
            throw std::runtime_error("Open-document events require file descriptor marshalling");
        if (found->kind == UIEvent::Kind::menu) {
            // AppKit has already selected by identity. Let the original event
            // loop run FindWindow -> menu preparation -> MenuSelect -> handler.
            // The reserved Point is a bridge token, never a physical click.
            menu_selection = found->message;
            s.memory.write<2>(address, 1);
            s.memory.write<4>(address + 10, 0x80008000);
        } else if (found->kind == UIEvent::Kind::launch || found->kind == UIEvent::Kind::quit) {
            if (found->kind == UIEvent::Kind::quit && commands.root)
                delivered_command_action = {commands.root, 1026};
            const auto id = found->kind == UIEvent::Kind::launch ? 0x6f617070u : 0x71756974u;
            s.memory.write<2>(address, 23);
            s.memory.write<4>(address + 2, 0x61657674);
            last_event_tick = std::max(tick, last_event_tick + 1);
            s.memory.write<4>(address + 6, last_event_tick);
            s.memory.write<4>(address + 10, id);
            delivered_events.emplace(event_record(s, address), std::move(*found));
        } else {
            std::uint16_t what = 0;
            auto message = found->message;
            switch (found->kind) {
                case UIEvent::Kind::mouse_down: what = 1; break;
                case UIEvent::Kind::mouse_up: what = 2; break;
                case UIEvent::Kind::key_down: what = 3; break;
                case UIEvent::Kind::key_up: what = 4; break;
                case UIEvent::Kind::auto_key: what = 5; break;
                case UIEvent::Kind::update: what = 6; message = found->window; break;
                case UIEvent::Kind::activate: what = 8; message = found->window; break;
                case UIEvent::Kind::suspend_resume: case UIEvent::Kind::mouse_moved: what = 15; break;
                default: throw std::runtime_error("Unsupported original event record type");
            }
            s.memory.write<2>(address, what);
            s.memory.write<4>(address + 2, message);
            s.memory.write<4>(address + 6, found->when ? found->when : tick);
            s.memory.write<2>(address + 10, std::uint16_t(found->y));
            s.memory.write<2>(address + 12, std::uint16_t(found->x));
            s.memory.write<2>(address + 14, found->modifiers);
            if (std::getenv("CM_AOT_TRACE_INPUT") && (what == 1 || what == 2))
                std::fprintf(stderr, "Original mouse event: what=%u window=%08x global=%d,%d live=%d,%d button=%u modifiers=%04x\n",
                    what, found->window, found->x, found->y, pointer.x, pointer.y, found->button, found->modifiers);
            if (what >= 1 && what <= 5) performance.mark("input", s, what);
        }
        pending_events.erase(found);
        return true;
    }
    std::uint32_t process_event(State& s, std::uint32_t address) {
        const auto found = delivered_events.find(event_record(s, address));
        if (found == delivered_events.end()) return std::uint32_t(-1708);
        const auto id = std::uint32_t(s.memory.read<4>(address + 10));
        const auto handler = apple_handlers.find({0x61657674, id});
        if (handler == apple_handlers.end()) { delivered_events.erase(found); return std::uint32_t(-1708); }
        NativeAEDescriptor descriptor;
        const auto error = apple_events.application_event(id, descriptor);
        if (error) return std::uint32_t(error);
        const auto arguments = allocate(s, 16, true);
        if (!arguments) { apple_events.dispose(descriptor); return std::uint32_t(-108); }
        s.memory.write<4>(arguments, descriptor.type);
        s.memory.write<4>(arguments + 4, descriptor.token);
        s.memory.write<4>(arguments + 8, 0x6e756c6c); // A null reply descriptor.
        delivered_events.erase(found);
        try {
            const std::array<std::uint32_t, 3> callback_arguments{arguments, arguments + 8, handler->second.reference};
            const auto result = call_original(s, handler->second.routine, callback_arguments);
            apple_events.dispose(descriptor);
            deallocate(s, arguments);
            return result;
        } catch (...) {
            apple_events.dispose(descriptor);
            deallocate(s, arguments);
            throw;
        }
    }
    std::uint32_t new_menu_bar(State& s, std::int16_t id) {
        const auto resource = resource_manager().get(0x4d424152, id, false);
        if (!resource.loaded) return 0;
        const auto ids = parse_menu_bar(resource.bytes);
        std::vector<std::uint32_t> tokens;
        for (auto menu_id : ids) {
            const auto contents = resource_manager().get(0x4d454e55, menu_id, false);
            if (!contents.loaded) throw std::runtime_error("Menu bar references a missing MENU resource");
            const auto definition = parse_menu(contents.bytes);
            const auto token = new_handle(s, contents.bytes.size());
            if (!token) throw std::runtime_error("Native menu allocation failed");
            s.memory.copy_in(handles.at(token).payload, contents.bytes);
            user_interface().create_menu(token, definition);
            menu_ids[definition.id] = token;
            tokens.push_back(token);
        }
        const auto bar = new_handle(s, 2 + tokens.size() * 4);
        if (!bar) throw std::runtime_error("Native menu bar allocation failed");
        const auto payload = handles.at(bar).payload;
        s.memory.write<2>(payload, tokens.size());
        for (std::size_t i = 0; i < tokens.size(); ++i) s.memory.write<4>(payload + 2 + i * 4, tokens[i]);
        menu_bars[bar] = std::move(tokens);
        return bar;
    }

    NativeResources& resource_manager() {
        if (!resources) resources = std::make_unique<NativeResources>(game / "CM 01-02");
        return *resources;
    }

    std::uint32_t new_handle(State& s, std::uint32_t size) {
        const auto payload = allocate(s, size, false);
        if (!payload) return 0;
        std::uint32_t master;
        if (master_slots.empty()) master = allocate(s, 4, false);
        else { master = master_slots.back(); master_slots.pop_back(); }
        if (!master) {
            deallocate(s, payload);
            s.memory_error = -108;
            return 0;
        }
        s.memory.write<4>(master, payload);
        handles.emplace(master, Handle{payload, size});
        return master;
    }
    void dispose_handle(State& s, std::uint32_t master) {
        if (!master) { s.memory_error = 0; return; }
        auto found = handles.find(master);
        if (found == handles.end()) throw std::runtime_error("Invalid or double DisposeHandle");
        deallocate(s, found->second.payload);
        s.memory.write<4>(master, 0);
        master_slots.push_back(master);
        handles.erase(found);
    }
    std::uint32_t resource_handle(State& s, const NativeResource& resource) {
        if (!resource.ticket) return 0;
        const auto found = resource_handles.find(resource.ticket);
        if (found == resource_handles.end()) {
            const auto master = new_handle(s, resource.bytes.size());
            if (master) {
                resource_handles.emplace(resource.ticket, master);
                if (resource.loaded && !resource.bytes.empty())
                    s.memory.copy_in(handles.at(master).payload, resource.bytes);
                if (!resource.loaded) s.memory.write<4>(master, 0);
            }
            return master;
        }
        const auto master = found->second;
        auto& handle = handles.at(master);
        if (!s.memory.read<4>(master) && resource.loaded) {
            const auto payload = allocate(s, resource.bytes.size(), false);
            if (!payload) throw std::runtime_error("Could not load original resource data");
            deallocate(s, handle.payload);
            handle.payload = payload;
            handle.size = resource.bytes.size();
            s.memory.write<4>(master, payload);
            if (!resource.bytes.empty()) s.memory.copy_in(payload, resource.bytes);
        }
        return master;
    }

    fs::path allowed(const fs::path& input) const {
        std::error_code error;
        const auto result = fs::weakly_canonical(input, error);
        if (error || (!under(result, game) && !under(result, writable))) throw FileError{directory_not_found};
        return input.lexically_normal();
    }
    std::uint32_t directory_id(const fs::path& input) {
        const auto permitted = allowed(input);
        if (!fs::is_directory(permitted)) throw FileError{directory_not_found};
        const auto path = fs::canonical(permitted);
        for (const auto& [id, existing] : directories)
            if (logical_path(existing) == logical_path(path) || fs::equivalent(existing, path)) return id;
        const auto id = next_directory++;
        directories.emplace(id, path);
        return id;
    }
    std::int16_t volume(const fs::path& path) const { return under(logical_path(path), game) ? -1 : -2; }
    fs::path directory(std::int16_t ref, std::uint32_t id) const {
        if (ref && ref != -1 && ref != -2) throw FileError{volume_not_found};
        if (ref == -2 && id == 2) id = 3;
        if (!id) id = !ref || ref == current_volume ? current_directory : ref == -1 ? 2 : 3;
        auto found = directories.find(id);
        if (found == directories.end()) throw FileError{directory_not_found};
        if (ref && ref != volume(found->second)) throw FileError{directory_not_found};
        return found->second;
    }
    fs::path path(std::int16_t ref, std::uint32_t id, const std::string& name) const {
        auto base = directory(ref, id);
        if (name.empty()) return base;
        if (name.find('/') != std::string::npos || name.find('\0') != std::string::npos)
            throw FileError{bad_name};
        std::size_t start = 0;
        if (name.front() == ':') start = 1;
        else if (auto colon = name.find(':'); colon != std::string::npos) {
            const auto vol = name.substr(0, colon);
            if (same_name(vol, "CM 01-02")) base = game;
            else if (same_name(vol, "CM User Data")) base = writable;
            else throw FileError{volume_not_found};
            start = colon + 1;
        }
        while (start < name.size()) {
            auto end = name.find(':', start);
            if (end == std::string::npos) end = name.size();
            const auto component = name.substr(start, end - start);
            if (component.empty()) base = base.parent_path();
            else base = visible_child(base, component);
            base = allowed(base);
            start = end + 1;
        }
        return allowed(base);
    }
    fs::path from_spec(State& s, std::uint32_t spec) const {
        return path(std::int16_t(s.memory.read<2>(spec)), s.memory.read<4>(spec + 2), read_mac_string(s, spec + 6));
    }
    NativeFileReference reference_for(const fs::path& path) const {
        NativeFileReference result{};
        const auto error = native_file_reference(allowed(path), result);
        if (error) throw FileError{error};
        return result;
    }
    fs::path from_reference(State& s, std::uint32_t address) const {
        NativeFileReference ref{};
        const auto bytes = s.memory.copy_out(address, ref.size());
        std::copy(bytes.begin(), bytes.end(), ref.begin());
        fs::path path;
        const auto error = native_reference_path(ref, path);
        if (error) throw FileError{error};
        path = allowed(path);
        const auto logical = logical_path(path);
        // An identity obtained before copy-on-write continues to use the same
        // visible namespace. The original media object remains immutable.
        if (under(logical, game) && logical != game)
            path = allowed(visible_child(logical.parent_path(), logical.filename().string()));
        return path;
    }
    void write_reference(State& s, std::uint32_t address, const NativeFileReference& ref) const {
        s.memory.copy_in(address, {ref.begin(), ref.end()});
    }
    std::vector<std::uint16_t> unicode_units(State& s, std::uint32_t address, std::uint32_t length) const {
        if (length > 255) throw FileError{bad_name};
        std::vector<std::uint16_t> result(length);
        for (unsigned n = 0; n < length; ++n) result[n] = s.memory.read<2>(std::uint64_t(address) + n * 2);
        return result;
    }
    std::uint32_t node_id(const fs::path& target) {
        if (fs::is_directory(target)) return directory_id(target);
        const auto [entry, inserted] = file_ids.try_emplace(logical_path(fs::canonical(target)), next_directory);
        if (inserted) ++next_directory;
        return entry->second;
    }
    void write_spec(State& s, std::uint32_t address, const fs::path& target) {
        const bool root = volume_root(target);
        const auto name = root ? std::string{} : convert(target.filename().string(), kCFStringEncodingUTF8, kCFStringEncodingMacRoman);
        if (name.size() > 63) throw FileError{bad_name};
        const auto parent = directory_id(root ? target : target.parent_path());
        s.memory.clear(address, 70);
        s.memory.write<2>(address, std::uint16_t(volume(target)));
        s.memory.write<4>(address + 2, parent);
        s.memory.write<1>(address + 6, name.size());
        s.memory.copy_in(address + 7, {name.begin(), name.end()});
    }
    void catalog_info(State& s, std::uint32_t pb) {
        // Original 32-bit, two-byte-aligned CInfoPBRec offsets from Files.h.
        // Do not overlay the modern SDK's 64-bit pointer fields on game memory.
        const auto name = std::uint32_t(s.memory.read<4>(pb + 18));
        const auto ref = std::int16_t(s.memory.read<2>(pb + 22));
        const auto index = std::int16_t(s.memory.read<2>(pb + 28));
        const auto parent_id = std::uint32_t(s.memory.read<4>(pb + 48));
        fs::path target;
        if (index < 0) target = directory(ref, parent_id);
        else if (!index) target = path(ref, parent_id, read_mac_string(s, name));
        else {
            const auto children = entries(directory(ref, parent_id));
            if (std::size_t(index) > children.size()) throw FileError{file_not_found};
            target = children[index - 1];
        }
        target = allowed(target);
        struct stat info{};
        if (stat(target.c_str(), &info)) throw FileError{file_not_found};
        const bool is_directory = S_ISDIR(info.st_mode);
        s.memory.write<1>(pb + 30, is_directory ? 0x10 : 0);
        s.memory.write<1>(pb + 31, 0);
        const auto finder = finder_info(target);
        s.memory.copy_in(pb + 32, {finder.begin(), finder.begin() + 16});
        s.memory.copy_in(pb + 84, {finder.begin() + 16, finder.end()});
        if (is_directory) {
            s.memory.write<4>(pb + 48, directory_id(target));
            s.memory.write<2>(pb + 52, std::min<std::size_t>(entries(target).size(), UINT16_MAX));
        } else {
            const auto [entry, inserted] = file_ids.try_emplace(logical_path(fs::canonical(target)), next_directory);
            if (inserted) ++next_directory;
            s.memory.write<4>(pb + 48, entry->second);
            s.memory.write<2>(pb + 52, 0);
            s.memory.write<4>(pb + 54, std::uint32_t(info.st_size));
            s.memory.write<4>(pb + 58, std::uint32_t(info.st_blocks * 512));
            s.memory.write<2>(pb + 62, 0);
            struct stat fork{};
            const auto resource = target / "..namedfork/rsrc";
            const auto resource_size = stat(resource.c_str(), &fork) == 0 ? fork.st_size : 0;
            s.memory.write<4>(pb + 64, std::uint32_t(resource_size));
            s.memory.write<4>(pb + 68, std::uint32_t(resource_size));
        }
        s.memory.write<4>(pb + 72, mac_date(info.st_birthtimespec.tv_sec));
        s.memory.write<4>(pb + 76, mac_date(info.st_mtimespec.tv_sec));
        s.memory.write<4>(pb + 80, 0); // No catalog backup date exists for these native files.
        const bool root = volume_root(target);
        s.memory.write<4>(pb + 100, root ? 1 : directory_id(target.parent_path()));
        if (name && index) write_mac_string(s, name, root ? volume(target) == -1 ? "CM 01-02" : "CM User Data" : target.filename().string());
    }
    void set_catalog_info(State& s, std::uint32_t pb) {
        auto target = path(std::int16_t(s.memory.read<2>(pb + 22)), s.memory.read<4>(pb + 48),
            read_mac_string(s, s.memory.read<4>(pb + 18)));
        if (!fs::exists(target)) throw FileError{file_not_found};
        if (fs::is_directory(target)) {
            const auto backing = writable_path(target);
            if (backing != target && !fs::exists(backing)) {
                fs::create_directories(backing);
                if (copyfile(target.c_str(), backing.c_str(), nullptr, COPYFILE_METADATA)) throw FileError{parameter_error};
            }
            target = backing;
        } else target = writable_file(target);
        GuestFileCatalog info{};
        write_be<2>(info.data(), s.memory.read<1>(pb + 30) & 0x11);
        const auto finder = s.memory.copy_out(pb + 32, 16), extended = s.memory.copy_out(pb + 84, 16);
        std::copy(finder.begin(), finder.end(), info.begin() + 72);
        std::copy(extended.begin(), extended.end(), info.begin() + 88);
        for (const auto& [source, destination] : {std::pair{72u, 16u}, {76u, 24u}, {80u, 48u}})
            write_be<4>(info.data() + destination + 2, s.memory.read<4>(pb + source));
        const auto error = native_set_file_catalog(reference_for(target), 0x2 | 0x20 | 0x40 | 0x200 | 0x800 | 0x1000, info);
        if (error) throw FileError{error};
    }
    void volume_info(State& s, std::uint32_t pb) {
        const auto name = std::uint32_t(s.memory.read<4>(pb + 18));
        const auto index = std::int16_t(s.memory.read<2>(pb + 28));
        auto ref = std::int16_t(s.memory.read<2>(pb + 22));
        if (std::getenv("CM_AOT_TRACE_IMPORTS") || std::getenv("CM_AOT_TRACE_FILES")) {
            std::fprintf(stderr, "Volume query index=%d ref=%d", index, ref);
            if (index < 0 && name) {
                try { std::fprintf(stderr, " name=%s", read_mac_string(s, name).c_str()); }
                catch (...) { std::fprintf(stderr, " name=<unreadable>"); }
            }
            std::fprintf(stderr, "\n");
        }
        if (index > 0) {
            if (index > 2) throw FileError{volume_not_found};
            ref = -index;
        } else if (index < 0) {
            const auto input = read_mac_string(s, name);
            const auto colon = input.find(':');
            if (colon != std::string::npos && colon != 0) {
                const auto requested = input.substr(0, colon);
                if (same_name(requested, "CM 01-02")) ref = -1;
                else if (same_name(requested, "CM User Data")) ref = -2;
                else throw FileError{volume_not_found};
            }
            // A null name, leaf name, or colon-prefixed relative path uses the
            // supplied volume reference (zero means the default volume).
        }
        if (!ref) ref = current_volume;
        const auto root = directory(ref, ref == -2 ? 3 : 2);
        // Writes to either logical volume are backed by the private directory.
        const auto space = fs::space(writable);
        struct stat info{};
        if (stat(root.c_str(), &info)) throw FileError{volume_not_found};
        const auto geometry = legacy_volume_geometry(space);
        std::uint32_t files = 0, folders = 0;
        std::vector<fs::path> pending{root};
        while (!pending.empty()) {
            auto parent = std::move(pending.back());
            pending.pop_back();
            for (const auto& entry : listing(parent)) {
                // A symlink target can change without changing its parent.
                // Resolve its type afresh and retain the original no-recursion rule.
                const bool link = entry.type == fs::file_type::symlink;
                const auto type = link ? fs::status(entry.path).type() : entry.type;
                if (type == fs::file_type::directory) {
                    ++folders;
                    if (!link) pending.push_back(entry.path);
                } else if (type == fs::file_type::regular) ++files;
            }
        }
        s.memory.clear(pb + 30, 92);
        s.memory.write<2>(pb + 22, std::uint16_t(ref));
        s.memory.write<4>(pb + 30, mac_date(info.st_birthtimespec.tv_sec));
        s.memory.write<4>(pb + 34, mac_date(info.st_mtimespec.tv_sec));
        s.memory.write<2>(pb + 40, std::min(files, std::uint32_t(UINT16_MAX)));
        s.memory.write<2>(pb + 46, geometry.blocks);
        s.memory.write<4>(pb + 48, geometry.block_size);
        s.memory.write<4>(pb + 52, geometry.block_size);
        s.memory.write<4>(pb + 58, next_directory);
        s.memory.write<2>(pb + 62, geometry.free_blocks);
        s.memory.write<2>(pb + 64, 0x482b); // HFS-compatible catalog view.
        s.memory.write<4>(pb + 82, files);
        s.memory.write<4>(pb + 86, folders);
        if (name) write_mac_string(s, name, ref == -1 ? "CM 01-02" : "CM User Data");
    }
};
NativePlatform::NativePlatform(fs::path game, fs::path writable, AudioMode audio) : impl_(std::make_unique<Impl>(audio)) {
    if (!fs::is_directory(game)) throw std::runtime_error("Game directory does not exist");
    impl_->game = fs::canonical(game);
    impl_->writable = fs::weakly_canonical(fs::absolute(writable));
    if (under(impl_->writable, impl_->game) || under(impl_->game, impl_->writable))
        throw std::runtime_error("Game data and writable data must use separate directories");
    fs::create_directories(impl_->writable);
    impl_->directories.emplace(2, impl_->game);
    impl_->directories.emplace(3, impl_->writable);
}
NativePlatform::~NativePlatform() = default;
const fs::path& NativePlatform::game_directory() const { return impl_->game; }
const fs::path& NativePlatform::writable_directory() const { return impl_->writable; }
std::vector<fs::path> NativePlatform::save_window_images(const fs::path& directory) const {
    return impl_->ui ? impl_->ui->save_window_images(directory) : std::vector<fs::path>{};
}
bool NativePlatform::dispatch(State& s, const std::string& name) {
    if (const auto result = impl_->unicode.dispatch(s, name)) {
        s.wr<4>(12, std::uint32_t(*result));
        s.pc = std::uint32_t(s.r<4>(4128));
        return true;
    }
    if (impl_->sound.dispatch(s, name)) return true;
    if (const auto result = dispatch_fork_io(s, impl_->data_forks, name)) {
        complete_fork_io(s, *result);
        if (name == "CarbonLib::FSCloseFork")
            impl_->performance.mark("file-close", s, std::uint16_t(s.r<4>(12)));
        // Fork async APIs are void; Classic async APIs return queue acceptance.
        // Their completed operation result is published separately in ioResult.
        switch (result->return_kind) {
        case ForkIoResult::Return::result: s.wr<4>(12, std::uint32_t(std::int32_t(result->error))); break;
        case ForkIoResult::Return::queued: s.wr<4>(12, 0); break;
        case ForkIoResult::Return::void_result: break;
        }
        s.pc = std::uint32_t(s.r<4>(4128));
        return true;
    }
    // Deliver original audio callbacks at cooperative timing/event boundaries,
    // not inside arbitrary synchronous OS services. Match cleanup quiets a
    // channel, frees its PCM/header, closes the file, then disposes the channel.
    // Re-entering its loop callback from FSClose dereferences those freed data.
    // The native renderer keeps running; only main-thread callback delivery is
    // deferred. Sound Manager flush/quiet/disposal queue semantics stay intact.
    if (name == "CarbonLib::TickCount" || name == "CarbonLib::WaitNextEvent")
        impl_->sound.poll(s);
    auto arg = [&](unsigned i) { return std::uint32_t(s.r<4>((i + 3) * 4)); };
    std::int16_t status = no_err;
    std::optional<std::uint32_t> return_value;
    try {
        if (name == "Apple;Carbon;Networking::InitOpenTransportInContext") {
            // No Open Transport backend is available in this port. Report its
            // documented unsupported-operation error, without creating a client
            // context. The original menu handles initialization failure and
            // returns to local play; claiming success would enter unported I/O.
            if (arg(1)) s.memory.write<4>(arg(1), 0);
            return_value = std::uint32_t(-3167); // kOTNotSupportedErr, MacErrors.h.
        } else if (name == "CarbonLib::Gestalt" && arg(0) == 0x6d656e75) {
            // Native menu bar, Aqua application-menu layout, duplicate command lookup.
            impl_->user_interface();
            s.memory.write<4>(arg(1), 7);
        } else if (name == "CarbonLib::Gestalt" && arg(0) == 0x66732020) {
            // FSSpec, HFS+ volumes, and the accepted FSRef/Unicode/fork family.
            // The original stream controllers now choose their 64-bit path.
            s.memory.write<4>(arg(1), 0x1302);
        } else if (name == "CarbonLib::Gestalt" && (arg(0) == 0x70636c6b || arg(0) == 0x72616d20)) {
            const auto hardware = native_hardware_value(arg(0));
            status = hardware.error;
            if (!status) s.memory.write<4>(arg(1), hardware.value);
        } else if (name == "CarbonLib::GetCursor") {
            const auto resource = impl_->resource_manager().get(0x43555253, std::int16_t(arg(0)), false);
            if (resource.loaded) (void)parse_monochrome_cursor(resource.bytes);
            // GetCursor returns a Resource Manager handle. The original quit
            // routine releases it with ReleaseResource, unlike GetCCursor.
            return_value = impl_->resource_handle(s, resource);
        } else if (name == "CarbonLib::SetCursor") {
            impl_->user_interface().create_cursor(arg(0), parse_monochrome_cursor(s.memory.copy_out(arg(0), 68)));
            impl_->user_interface().set_cursor(arg(0));
            return_value = arg(0);
        } else if (name == "CarbonLib::GetCCursor") {
            const auto resource = impl_->resource_manager().get(0x63727372, std::int16_t(arg(0)), false);
            return_value = 0;
            if (resource.loaded) {
                const auto cursor = parse_color_cursor(resource.bytes);
                const auto record = impl_->new_handle(s, 96), map = impl_->new_handle(s, 50);
                const auto pixels = impl_->new_handle(s, cursor.pixel_bytes), table = impl_->new_handle(s, cursor.palette_bytes);
                if (!record || !map || !pixels || !table) throw std::runtime_error("Native cursor allocation failed");
                const auto data = impl_->handles.at(record).payload, pixel_map = impl_->handles.at(map).payload;
                s.memory.copy_in(data, {resource.bytes.begin(), resource.bytes.begin() + 96});
                s.memory.write<4>(data + 2, map); s.memory.write<4>(data + 6, pixels);
                s.memory.clear(data + 10, 10); s.memory.write<4>(data + 88, 0);
                s.memory.copy_in(pixel_map, {resource.bytes.begin() + cursor.map_offset, resource.bytes.begin() + cursor.map_offset + 50});
                s.memory.write<4>(pixel_map, impl_->handles.at(pixels).payload); s.memory.write<4>(pixel_map + 42, table);
                s.memory.copy_in(impl_->handles.at(pixels).payload,
                    {resource.bytes.begin() + cursor.pixels_offset, resource.bytes.begin() + cursor.pixels_offset + cursor.pixel_bytes});
                s.memory.copy_in(impl_->handles.at(table).payload,
                    {resource.bytes.begin() + cursor.palette_offset, resource.bytes.end()});
                impl_->user_interface().create_cursor(record, cursor);
                impl_->color_cursors.emplace(record, std::array{map, pixels, table});
                return_value = record;
            }
        } else if (name == "CarbonLib::DisposeCCursor") {
            const auto found = impl_->color_cursors.find(arg(0));
            if (found == impl_->color_cursors.end()) throw std::runtime_error("Invalid or double DisposeCCursor");
            impl_->user_interface().dispose_cursor(arg(0));
            for (const auto handle : found->second) impl_->dispose_handle(s, handle);
            impl_->dispose_handle(s, arg(0));
            impl_->color_cursors.erase(found);
            return_value = arg(0);
        } else if (name == "CarbonLib::SetCCursor") {
            impl_->user_interface().set_cursor(arg(0));
            return_value = arg(0);
        } else if (name == "CarbonLib::NewHandle") {
            return_value = impl_->new_handle(s, arg(0));
        } else if (name == "CarbonLib::DisposeHandle") {
            impl_->dispose_handle(s, arg(0));
            return_value = arg(0);
        } else if (name == "CarbonLib::PtrToHand") {
            const auto handle = impl_->new_handle(s, arg(2));
            s.memory.write<4>(arg(1), handle);
            status = s.memory_error;
            if (handle && arg(2))
                s.memory.copy_in(impl_->handles.at(handle).payload, s.memory.copy_out(arg(0), arg(2)));
        } else if (name == "CarbonLib::GetHandleSize" || name == "CarbonLib::HLock") {
            auto found = impl_->handles.find(arg(0));
            if (found == impl_->handles.end()) {
                s.memory_error = -109;
                return_value = 0;
            } else {
                s.memory_error = 0;
                if (name == "CarbonLib::HLock") found->second.locked = true;
                return_value = name == "CarbonLib::HLock" ? arg(0) : found->second.size;
            }
        } else if (name == "CarbonLib::Munger") {
            auto found = impl_->handles.find(arg(0));
            if (found == impl_->handles.end()) throw std::runtime_error("Munger received an unknown handle");
            const auto offset = std::int32_t(arg(1));
            const auto first_length = std::int32_t(arg(3)), second_length = std::int32_t(arg(5));
            if (offset < 0 || first_length < 0 || second_length < 0)
                throw std::runtime_error("Negative Munger parameters require verification");
            std::optional<std::vector<std::uint8_t>> first, second;
            if (arg(2)) first = first_length ? s.memory.copy_out(arg(2), first_length) : std::vector<std::uint8_t>{};
            if (arg(4)) second = second_length ? s.memory.copy_out(arg(4), second_length) : std::vector<std::uint8_t>{};
            auto& handle = found->second;
            const auto changed = native_munger(s.memory.copy_out(handle.payload, handle.size), offset,
                                               first, first_length, second, second_length);
            s.memory_error = changed.error;
            return_value = std::uint32_t(changed.offset);
            if (!changed.error) {
                const auto capacity = (std::max(s.allocated.at(handle.payload), 1u) + 15u) & ~15u;
                if (changed.bytes.size() > capacity && handle.locked) {
                    s.memory_error = -117; // memLockedErr: this provider cannot relocate a locked block.
                    return_value = UINT32_MAX;
                } else if (changed.bytes.size() > capacity) {
                    const auto payload = allocate(s, changed.bytes.size(), false);
                    if (!payload) return_value = UINT32_MAX;
                    else {
                        s.memory.copy_in(payload, changed.bytes);
                        deallocate(s, handle.payload);
                        handle.payload = payload;
                        handle.size = changed.bytes.size();
                        s.memory.write<4>(arg(0), payload);
                    }
                } else {
                    if (!changed.bytes.empty()) s.memory.copy_in(handle.payload, changed.bytes);
                    handle.size = changed.bytes.size();
                }
            }
        } else if (name == "CarbonLib::GetScriptManagerVariable" && arg(0) == 40) {
            return_value = std::uint32_t(native_region_code());
        } else if (name == "CarbonLib::UppercaseText") {
            const auto length = std::int16_t(arg(1));
            if (length < 0) throw std::runtime_error("Negative original text length");
            auto bytes = s.memory.copy_out(arg(0), length);
            native_uppercase(bytes, std::int16_t(arg(2)));
            if (!bytes.empty()) s.memory.copy_in(arg(0), bytes);
            return_value = arg(0);
        } else if (name == "CarbonLib::GetCurrentProcess") {
            const auto process = current_native_process();
            status = process.error;
            if (!status) {
                s.memory.write<4>(arg(0), process.high);
                s.memory.write<4>(arg(0) + 4, process.low);
            }
        } else if (name == "CarbonLib::GetProcessInformation") {
            const auto process = native_process_information(s.memory.read<4>(arg(0)), s.memory.read<4>(arg(0) + 4));
            const auto record = arg(1);
            if (s.memory.read<4>(record) != 60) throw FileError{parameter_error};
            status = process.error;
            if (!status) {
                const auto name_ptr = std::uint32_t(s.memory.read<4>(record + 4));
                const auto spec_ptr = std::uint32_t(s.memory.read<4>(record + 56));
                s.memory.write<4>(record + 8, process.high);
                s.memory.write<4>(record + 12, process.low);
                s.memory.write<4>(record + 16, process.type);
                s.memory.write<4>(record + 20, process.signature);
                s.memory.write<4>(record + 24, process.mode);
                // macOS does not expose a Classic process zone or its free size.
                s.memory.write<4>(record + 28, 0);
                s.memory.write<4>(record + 32, 0);
                s.memory.write<4>(record + 36, 0);
                s.memory.write<4>(record + 40, process.launcher_high);
                s.memory.write<4>(record + 44, process.launcher_low);
                s.memory.write<4>(record + 48, process.launch_date);
                s.memory.write<4>(record + 52, process.active_time);
                // Original code locates its resources/data through the module's
                // FSSpec. Its native executable and its data module are separate.
                if (name_ptr) write_mac_string(s, name_ptr, "CM 01-02");
                if (spec_ptr) impl_->write_spec(s, spec_ptr, impl_->game / "CM 01-02");
            }
        } else if (name == "CarbonLib::CFStringGetSystemEncoding") {
            return_value = CFStringGetSystemEncoding();
        } else if (name == "CarbonLib::CFStringCreateWithCStringNoCopy") {
            if (arg(0) || arg(3)) throw std::runtime_error("Custom original CFString allocators require verification");
            std::vector<std::uint8_t> bytes;
            for (std::uint32_t i = 0; ; ++i) {
                if (i == 65536) throw std::runtime_error("Original CFString exceeds supported length");
                const auto byte = std::uint8_t(s.memory.read<1>(std::uint64_t(arg(1)) + i));
                if (!byte) break;
                bytes.push_back(byte);
            }
            // Copy the original bytes into a native CF object. Original data
            // addresses must never become native allocator-owned pointers.
            const auto string = CFStringCreateWithBytes(nullptr, bytes.data(), bytes.size(), arg(2), false);
            return_value = 0;
            if (string) {
                const auto token = impl_->next_string++;
                impl_->strings.emplace(token, string);
                return_value = token;
            }
        } else if (name == "CarbonLib::CFRelease") {
            CFRelease(impl_->strings.at(arg(0)));
            impl_->strings.erase(arg(0));
            return_value = arg(0);
        } else if (name == "CarbonLib::SetWindowTitleWithCFString") {
            const auto string = impl_->strings.at(arg(1));
            std::vector<char> bytes(CFStringGetMaximumSizeForEncoding(CFStringGetLength(string), kCFStringEncodingUTF8) + 1);
            if (!CFStringGetCString(string, bytes.data(), bytes.size(), kCFStringEncodingUTF8))
                throw std::runtime_error("Original window title cannot be represented in UTF-8");
            impl_->user_interface().set_window_title(arg(0), bytes.data());
        } else if (name == "CarbonLib::GetDateTime") {
            s.memory.write<4>(arg(0), native_local_date_time());
            return_value = arg(0);
        } else if (name == "CarbonLib::TickCount") {
            return_value = native_tick_count();
        } else if (name == "CarbonLib::Microseconds") {
            // UnsignedWide is {hi, lo} on the original PowerPC ABI, but the
            // native little-endian struct reverses those fields.
            s.memory.write<8>(arg(0), native_microseconds());
            return_value = arg(0);
        } else if (name == "CarbonLib::ReadLocation") {
            const auto location = native_machine_location();
            s.memory.write<4>(arg(0), location.latitude);
            s.memory.write<4>(arg(0) + 4, location.longitude);
            s.memory.write<4>(arg(0) + 8, location.gmt_and_daylight);
            return_value = arg(0);
        } else if (name == "CarbonLib::FlushEvents") {
            bool stopped = false;
            for (auto it = impl_->pending_events.begin(); it != impl_->pending_events.end();) {
                const auto mask = legacy_event_mask(*it);
                if (mask & arg(1)) { stopped = true; break; }
                if (mask & arg(0)) it = impl_->pending_events.erase(it);
                else ++it;
            }
            if (!stopped) impl_->user_interface().flush_events(arg(0), arg(1));
            return_value = arg(0);
        } else if (name == "CarbonLib::GetEventClass" || name == "CarbonLib::GetEventKind") {
            const auto& event = impl_->carbon_events.at(arg(0));
            return_value = name == "CarbonLib::GetEventClass" ? event.event_class : event.kind;
        } else if (name == "CarbonLib::GetEventParameter") {
            const auto result = impl_->user_interface().event_parameter(arg(0), arg(1), arg(2), arg(4), arg(6) != 0);
            if (arg(3) && result.type != UINT32_MAX) s.memory.write<4>(arg(3), result.type);
            if (arg(5) && result.size != UINT32_MAX) s.memory.write<4>(arg(5), result.size);
            if (arg(6) && !result.bytes.empty()) s.memory.copy_in(arg(6), result.bytes);
            return_value = std::uint32_t(result.error);
        } else if (name == "CarbonLib::CopyBits") {
            const auto& source = impl_->port_for_bitmap(arg(0));
            const auto& destination = impl_->port_for_bitmap(arg(1));
            if (arg(4) != 0) throw std::runtime_error("CopyBits transfer mode requires a verified raster operation");
            LegacyRect from, to;
            for (unsigned i = 0; i < 4; ++i) {
                from[i] = std::int16_t(s.memory.read<2>(arg(2) + i * 2));
                to[i] = std::int16_t(s.memory.read<2>(arg(3) + i * 2));
            }
            const auto width = std::int32_t(from[3]) - from[1], height = std::int32_t(from[2]) - from[0];
            if (width != std::int32_t(to[3]) - to[1] || height != std::int32_t(to[2]) - to[0])
                throw std::runtime_error("Scaled CopyBits requires verified sampling semantics");
            const auto source_bytes = s.memory.copy_out(source.pixels, source.row_bytes * (source.bounds[2] - source.bounds[0]));
            std::vector<std::uint8_t> mask;
            if (arg(5)) {
                const auto& region = impl_->handles.at(arg(5));
                mask = s.memory.copy_out(region.payload, region.size);
            }
            impl_->drawing.copy_surface(s, source.pixels, destination.pixels,
                {from[1] - source.bounds[1], from[0] - source.bounds[0], width, height},
                to[1] - destination.bounds[1], to[0] - destination.bounds[0], mask, destination.bounds[1], destination.bounds[0]);
            impl_->drawing.copying = true;
            for (std::int32_t y = 0; y < height; ++y) for (std::int32_t x = 0; x < width; ++x) {
                const auto sx = from[1] + x, sy = from[0] + y, dx = to[1] + x, dy = to[0] + y;
                if (sx < source.bounds[1] || sy < source.bounds[0] || sx >= source.bounds[3] || sy >= source.bounds[2] ||
                    dx < destination.bounds[1] || dy < destination.bounds[0] || dx >= destination.bounds[3] || dy >= destination.bounds[2]) continue;
                if (!mask.empty() && !region_contains(mask, dx, dy)) continue;
                const auto at = (sy - source.bounds[0]) * source.row_bytes + (sx - source.bounds[1]) * (source.depth / 8);
                std::uint32_t rgb;
                if (source.depth == 16) {
                    const auto value = read_be<2>(source_bytes.data() + at);
                    rgb = 0;
                    for (unsigned i = 0; i < 3; ++i) {
                        const auto channel = (value >> (10 - i * 5)) & 31;
                        rgb = (rgb << 8) | (channel << 3) | (channel >> 2);
                    }
                } else rgb = read_be<4>(source_bytes.data() + at) & 0xffffff;
                const auto output = destination.pixels + (dy - destination.bounds[0]) * destination.row_bytes +
                                    (dx - destination.bounds[1]) * (destination.depth / 8);
                if (destination.depth == 16)
                    s.memory.write<2>(output, ((rgb >> 9) & 0x7c00) | ((rgb >> 6) & 0x3e0) | ((rgb >> 3) & 31));
                else s.memory.write<4>(output, rgb);
            }
            impl_->drawing.copying = false;
            impl_->present(s, destination);
            return_value = arg(0);
        } else if (name == "CarbonLib::ForeColor") {
            auto& port = impl_->ports.at(impl_->current_port);
            if (arg(0) == 33) port.foreground = 0;
            else if (arg(0) == 30) port.foreground = 0xffffff;
            else throw std::runtime_error("Original foreground color requires a verified RGB mapping");
            return_value = arg(0);
        } else if (name == "CarbonLib::PaintRect") {
            auto& port = impl_->ports.at(impl_->current_port);
            std::array<std::int32_t, 4> rect;
            for (unsigned i = 0; i < 4; ++i) rect[i] = std::int16_t(s.memory.read<2>(arg(0) + i * 2));
            if (impl_->painting_window) {
                const auto global = impl_->user_interface().window_bounds(impl_->painting_window, true);
                for (unsigned i = 0; i < 4; ++i) rect[i] -= global[i % 2];
            }
            rect[0] = std::max<std::int32_t>(rect[0], port.bounds[0]);
            rect[1] = std::max<std::int32_t>(rect[1], port.bounds[1]);
            rect[2] = std::min<std::int32_t>(rect[2], port.bounds[2]);
            rect[3] = std::min<std::int32_t>(rect[3], port.bounds[3]);
            const auto rgb = port.foreground;
            const auto rgb555 = ((rgb >> 9) & 0x7c00) | ((rgb >> 6) & 0x3e0) | ((rgb >> 3) & 31);
            for (auto y = rect[0]; y < rect[2]; ++y) for (auto x = rect[1]; x < rect[3]; ++x) {
                const auto address = port.pixels + (y - port.bounds[0]) * port.row_bytes + (x - port.bounds[1]) * (port.depth / 8);
                if (port.depth == 16) s.memory.write<2>(address, rgb555);
                else s.memory.write<4>(address, rgb);
            }
            impl_->present(s, port);
            return_value = arg(0);
        } else if (name == "CarbonLib::GetWindowPort") {
            return_value = impl_->window_port(s, arg(0));
        } else if (name == "CarbonLib::QDFlushPortBuffer") {
            impl_->present(s, impl_->ports.at(arg(0)));
            return_value = arg(0);
        } else if (name == "CarbonLib::NewGWorld") {
            s.memory.write<4>(arg(0), 0);
            if (arg(3) || arg(4) || arg(5))
                throw std::runtime_error("Custom GWorld color tables, devices, or flags require verification");
            LegacyRect bounds;
            for (unsigned i = 0; i < 4; ++i) bounds[i] = std::int16_t(s.memory.read<2>(arg(2) + i * 2));
            s.memory.write<4>(arg(0), impl_->new_port(s, std::uint16_t(arg(1)), bounds));
        } else if (name == "CarbonLib::GetPortPixMap") {
            return_value = impl_->ports.at(arg(0)).pixel_map;
        } else if (name == "CarbonLib::LockPixels") {
            impl_->port_for_pixels(arg(0)).locked = true;
            return_value = 1;
        } else if (name == "CarbonLib::GetPixBaseAddr") {
            return_value = impl_->port_for_pixels(arg(0)).pixels;
        } else if (name == "CarbonLib::GetPortBitMapForCopyBits") {
            return_value = impl_->handles.at(impl_->ports.at(arg(0)).pixel_map).payload;
        } else if (name == "CarbonLib::DisposeGWorld") {
            const auto port = impl_->ports.at(arg(0));
            if (port.window) throw std::runtime_error("A native window port is not a disposable GWorld");
            impl_->drawing.remove_surface(port.pixels);
            impl_->dispose_handle(s, port.pixel_map); impl_->dispose_handle(s, port.color_table);
            deallocate(s, port.pixels);
            impl_->ports.erase(arg(0));
            if (impl_->current_port == arg(0)) impl_->current_port = 0;
            return_value = arg(0);
        } else if (name == "CarbonLib::CreateNewWindow") {
            LegacyRect bounds;
            for (unsigned i = 0; i < 4; ++i) bounds[i] = std::int16_t(s.memory.read<2>(arg(2) + i * 2));
            const auto token = impl_->next_window++;
            impl_->user_interface().create_window(token, arg(0), arg(1), bounds);
            impl_->windows.insert(token);
            s.memory.write<4>(arg(3), token);
        } else if (name == "CarbonLib::GetWindowEventTarget") {
            if (!impl_->windows.contains(arg(0))) throw std::runtime_error("Unknown native window event target");
            return_value = arg(0);
        } else if (name == "CarbonLib::NewEventHandlerUPP") {
            (void)s.memory.read<4>(arg(0));
            (void)s.memory.read<4>(arg(0) + 4);
            return_value = arg(0);
        } else if (name == "CarbonLib::InstallEventHandler") {
            if (!impl_->windows.contains(arg(0))) throw std::runtime_error("Unknown native event target");
            if (arg(2) > 256) throw std::runtime_error("Original window event type count exceeds supported bounds");
            Impl::WindowHandler handler{arg(0), arg(1), arg(4), {}};
            for (std::uint32_t i = 0; i < arg(2); ++i) {
                const auto event_class = std::uint32_t(s.memory.read<4>(arg(3) + i * 8));
                const auto kind = std::uint32_t(s.memory.read<4>(arg(3) + i * 8 + 4));
                if (!(event_class == 0x77696e64 && (kind == 1013 || kind == 2 || kind == 85)) &&
                    !(event_class == 0x6d6f7573 && kind == 10))
                    throw std::runtime_error("Original event handler requires an additional native event mapping");
                handler.types.emplace_back(event_class, kind);
            }
            const auto token = impl_->next_window_handler++;
            impl_->window_handlers.emplace(token, std::move(handler));
            if (arg(5)) s.memory.write<4>(arg(5), token);
        } else if (name == "CarbonLib::ShowWindow" || name == "CarbonLib::BringToFront" || name == "CarbonLib::SelectWindow") {
            impl_->user_interface().show_window(arg(0), name == "CarbonLib::SelectWindow");
            impl_->paint_events(s);
            return_value = arg(0);
        } else if (name == "CarbonLib::GetWindowBounds") {
            if (arg(1) != 32 && arg(1) != 33) throw std::runtime_error("Unsupported original window region selector");
            const auto bounds = impl_->user_interface().window_bounds(arg(0), arg(1) == 33);
            for (unsigned i = 0; i < 4; ++i) s.memory.write<2>(arg(2) + i * 2, std::uint16_t(bounds[i]));
        } else if (name == "CarbonLib::DisposeWindow") {
            if (auto found = impl_->window_ports.find(arg(0)); found != impl_->window_ports.end()) {
                const auto port = impl_->ports.at(found->second);
                impl_->drawing.remove_surface(port.pixels);
                impl_->dispose_handle(s, port.pixel_map); impl_->dispose_handle(s, port.color_table);
                deallocate(s, port.pixels);
                impl_->ports.erase(found->second);
                if (impl_->current_port == found->second) impl_->current_port = 0;
                impl_->window_ports.erase(found);
            }
            impl_->user_interface().dispose_window(arg(0));
            impl_->windows.erase(arg(0));
            std::erase_if(impl_->window_handlers, [&](const auto& handler) { return handler.second.target == arg(0); });
            return_value = arg(0);
        } else if (name == "CarbonLib::GetMainDevice") {
            return_value = impl_->main_device(s);
        } else if (name == "CarbonLib::GetPort") {
            s.memory.write<4>(arg(0), impl_->current_port);
            return_value = arg(0);
        } else if (name == "CarbonLib::SetPort") {
            if (arg(0) && !impl_->ports.contains(arg(0))) throw std::runtime_error("Unknown native drawing port");
            impl_->current_port = arg(0);
            return_value = arg(0);
        } else if (name == "CarbonLib::PMCreateSession" || name == "CarbonLib::PMCreatePageFormat" || name == "CarbonLib::PMCreatePrintSettings") {
            std::uint32_t token = 0;
            const auto kind = name == "CarbonLib::PMCreateSession" ? NativePrinting::Kind::session :
                name == "CarbonLib::PMCreatePageFormat" ? NativePrinting::Kind::page_format : NativePrinting::Kind::settings;
            return_value = std::uint32_t(impl_->printing.create(kind, token));
            s.memory.write<4>(arg(0), token);
        } else if (name == "CarbonLib::PMSessionDefaultPageFormat") {
            return_value = std::uint32_t(impl_->printing.default_page_format(arg(0), arg(1)));
        } else if (name == "CarbonLib::PMSessionValidatePageFormat") {
            bool changed = false;
            return_value = std::uint32_t(impl_->printing.validate_page_format(arg(0), arg(1), changed));
            if (!*return_value && arg(2)) s.memory.write<1>(arg(2), changed);
        } else if (name == "CarbonLib::PMGetAdjustedPageRect") {
            std::array<double, 4> rect;
            return_value = std::uint32_t(impl_->printing.page_rect(arg(0), rect));
            if (!*return_value)
                for (unsigned i = 0; i < 4; ++i) s.memory.write<8>(arg(1) + i * 8, std::bit_cast<std::uint64_t>(rect[i]));
        } else if (name == "CarbonLib::PMRelease") {
            return_value = std::uint32_t(impl_->printing.release(arg(0)));
        } else if (name == "CarbonLib::PMFlattenPageFormat") {
            std::vector<std::uint8_t> bytes;
            return_value = std::uint32_t(impl_->printing.flatten(arg(0), bytes));
            if (!*return_value) {
                const auto handle = impl_->new_handle(s, bytes.size());
                if (handle) s.memory.copy_in(impl_->handles.at(handle).payload, bytes);
                else return_value = std::uint32_t(-108);
                s.memory.write<4>(arg(1), handle);
            }
        } else if (name == "CarbonLib::PMUnflattenPageFormat") {
            const auto& handle = impl_->handles.at(arg(0));
            std::uint32_t token = 0;
            return_value = std::uint32_t(impl_->printing.unflatten(s.memory.copy_out(handle.payload, handle.size), token));
            s.memory.write<4>(arg(1), token);
        } else if (name == "CarbonLib::GetNewMBar") {
            return_value = impl_->new_menu_bar(s, std::int16_t(arg(0)));
        } else if (name == "CarbonLib::SetMenuBar") {
            impl_->user_interface().set_menu_bar(impl_->menu_bars.at(arg(0)));
            return_value = arg(0);
        } else if (name == "CarbonLib::GetIndMenuItemWithCommandID") {
            const auto item = impl_->user_interface().find_menu_command(arg(0), arg(1), arg(2));
            if (!item) status = -5622;
            else {
                if (arg(3)) s.memory.write<4>(arg(3), item->first);
                if (arg(4)) s.memory.write<2>(arg(4), item->second);
            }
        } else if (name == "CarbonLib::MenuKey") {
            return_value = impl_->user_interface().menu_key(std::uint8_t(arg(0)));
        } else if (name == "CarbonLib::MenuSelect") {
            const auto selected = impl_->menu_selection.value_or(0);
            impl_->menu_selection.reset();
            return_value = impl_->user_interface().menu_selection_enabled(selected) ? selected : 0;
            if (*return_value == 0x00020001 && impl_->commands.ready) {
                clear_previous_command_key(s);
                impl_->delivered_command_action = {impl_->commands.root, 1022};
            }
        } else if (name == "CarbonLib::EnableMenuItem" || name == "CarbonLib::DisableMenuItem") {
            impl_->user_interface().set_menu_item_enabled(arg(0), arg(1), name == "CarbonLib::EnableMenuItem");
            return_value = arg(0);
        } else if (name == "CarbonLib::HiliteMenu") {
            impl_->user_interface().redraw_menu_bar();
            return_value = arg(0);
        } else if (name == "CarbonLib::GetMenuHandle" || name == "CarbonLib::GetMenu") {
            const auto found = impl_->menu_ids.find(std::int16_t(arg(0)));
            return_value = found == impl_->menu_ids.end() ? 0 : found->second;
        } else if (name == "CarbonLib::DeleteMenuItem") {
            impl_->user_interface().delete_menu_item(arg(0), arg(1));
            return_value = arg(0);
        } else if (name == "CarbonLib::AppendResMenu") {
            for (const auto& title : impl_->resource_manager().names(arg(1))) {
                MenuItem item;
                item.text = title;
                impl_->user_interface().append_menu_item(arg(0), item);
            }
            return_value = arg(0);
        } else if (name == "CarbonLib::CountMenuItems") {
            return_value = impl_->user_interface().count_menu_items(arg(0));
        } else if (name == "CarbonLib::GetMenuItemText") {
            const auto title = impl_->user_interface().menu_item_text(arg(0), arg(1));
            if (title.size() > 255) throw std::runtime_error("Native menu title is too long");
            s.memory.write<1>(arg(2), title.size());
            s.memory.copy_in(arg(2) + 1, {title.begin(), title.end()});
            return_value = arg(0);
        } else if (name == "CarbonLib::SetMenuItemText") {
            const auto bytes = s.memory.copy_out(arg(2) + 1, s.memory.read<1>(arg(2)));
            impl_->user_interface().set_menu_item_text(arg(0), arg(1), {bytes.begin(), bytes.end()});
            return_value = arg(0);
        } else if (name == "CarbonLib::InvalMenuBar" || name == "CarbonLib::DrawMenuBar") {
            impl_->user_interface().redraw_menu_bar();
            return_value = arg(0);
        } else if (name == "CarbonLib::InitCursor") {
            impl_->user_interface().initialize_cursor();
            return_value = arg(0);
        } else if (name == "CarbonLib::GetMouse" || name == "CarbonLib::GetGlobalMouse") {
            const auto point = impl_->user_interface().pointer_state();
            std::int32_t x = point.x, y = point.y;
            if (name == "CarbonLib::GetMouse") {
                const auto& port = impl_->ports.at(impl_->current_port);
                if (!port.window) throw std::runtime_error("GetMouse on an offscreen port needs a verified global origin");
                const auto bounds = impl_->user_interface().window_bounds(port.window, true);
                x += std::int32_t(port.bounds[1]) - bounds[1];
                y += std::int32_t(port.bounds[0]) - bounds[0];
            }
            s.memory.write<2>(arg(0), std::uint16_t(y));
            s.memory.write<2>(arg(0) + 2, std::uint16_t(x));
            return_value = arg(0);
        } else if (name == "CarbonLib::Button") {
            return_value = impl_->user_interface().pointer_state().down;
        } else if (name == "CarbonLib::FindWindow") {
            const auto hit = impl_->menu_selection && arg(0) == 0x80008000
                ? NativeWindowHit{1, 0}
                : impl_->user_interface().find_window(std::int16_t(arg(0)), std::int16_t(arg(0) >> 16));
            s.memory.write<4>(arg(1), hit.window);
            return_value = std::uint32_t(std::int32_t(hit.part));
        } else if (name == "CarbonLib::GetKeys") {
            const auto keys = impl_->user_interface().key_map();
            s.memory.copy_in(arg(0), {keys.begin(), keys.end()});
            return_value = arg(0);
        } else if (name == "CarbonLib::WaitNextEvent") {
            return_value = impl_->next_event(s, arg(0), arg(1), arg(2), arg(3));
        } else if (name == "CarbonLib::AEProcessAppleEvent") {
            return_value = impl_->process_event(s, arg(0));
        } else if (name == "CarbonLib::AEDisposeDesc") {
            const NativeAEDescriptor descriptor{std::uint32_t(s.memory.read<4>(arg(0))), std::uint32_t(s.memory.read<4>(arg(0) + 4))};
            return_value = std::uint32_t(impl_->apple_events.dispose(descriptor));
            if (!*return_value) { s.memory.write<4>(arg(0), 0x6e756c6c); s.memory.write<4>(arg(0) + 4, 0); }
        } else if (name == "CarbonLib::NewAEEventHandlerUPP") {
            // A UPP is an opaque token in the original ABI. The callback keeps
            // its original transition vector; native event delivery resolves it.
            (void)s.memory.read<4>(arg(0));
            (void)s.memory.read<4>(arg(0) + 4);
            return_value = arg(0);
        } else if (name == "CarbonLib::AEInstallEventHandler") {
            if (arg(4)) throw std::runtime_error("System-wide AppleEvent handlers are unsupported");
            impl_->apple_handlers[{arg(0), arg(1)}] = {arg(2), arg(3)};
        } else if (name == "CarbonLib::MoreMasters") {
            // Reserve real master-pointer slots for later relocatable handles.
            for (unsigned i = 0; i < 64; ++i) {
                const auto slot = allocate(s, 4, true);
                if (!slot) break;
                impl_->master_slots.push_back(slot);
            }
            return_value = arg(0);
        } else if (name == "CarbonLib::GetGrayRgn") {
            const auto bytes = region_union(impl_->user_interface().desktop_rectangles());
            if (!impl_->desktop_region) impl_->desktop_region = impl_->new_handle(s, bytes.size());
            auto& handle = impl_->handles.at(impl_->desktop_region);
            if (handle.size != bytes.size()) {
                const auto payload = allocate(s, bytes.size(), false);
                if (!payload) throw std::runtime_error("Native desktop region allocation failed");
                deallocate(s, handle.payload);
                handle.payload = payload; handle.size = bytes.size();
                s.memory.write<4>(impl_->desktop_region, payload);
            }
            s.memory.copy_in(handle.payload, bytes);
            return_value = impl_->desktop_region;
        } else if (name == "CarbonLib::GetRegionBounds") {
            const auto& handle = impl_->handles.at(arg(0));
            if (handle.size < 10) throw std::runtime_error("Invalid original region handle");
            s.memory.copy_in(arg(1), s.memory.copy_out(handle.payload + 2, 8));
            return_value = arg(1);
        } else if (name == "CarbonLib::SetRect") {
            const std::array<unsigned, 4> fields{2, 1, 4, 3};
            for (unsigned i = 0; i < 4; ++i) s.memory.write<2>(arg(0) + i * 2, std::uint16_t(arg(fields[i])));
            return_value = arg(0);
        } else if (name == "CarbonLib::PinRect") {
            const auto x = std::int16_t(arg(1)), y = std::int16_t(arg(1) >> 16);
            const auto top = std::int16_t(s.memory.read<2>(arg(0))), left = std::int16_t(s.memory.read<2>(arg(0) + 2));
            const auto bottom = std::int16_t(s.memory.read<2>(arg(0) + 4)), right = std::int16_t(s.memory.read<2>(arg(0) + 6));
            if (right < left || bottom < top) throw std::runtime_error("PinRect received an inverted original rectangle");
            // Preserve Inside Macintosh's documented edge quirk: equality at
            // right/bottom is unchanged; only points beyond an edge lose one.
            const auto pinned_x = x < left ? left : x > right ? right - 1 : x;
            const auto pinned_y = y < top ? top : y > bottom ? bottom - 1 : y;
            return_value = (std::uint32_t(std::uint16_t(pinned_y)) << 16) | std::uint16_t(pinned_x);
        } else if (name == "CarbonLib::PtInRect") {
            const auto x = std::int16_t(arg(0)), y = std::int16_t(arg(0) >> 16);
            return_value = y >= std::int16_t(s.memory.read<2>(arg(1))) &&
                           x >= std::int16_t(s.memory.read<2>(arg(1) + 2)) &&
                           y < std::int16_t(s.memory.read<2>(arg(1) + 4)) &&
                           x < std::int16_t(s.memory.read<2>(arg(1) + 6));
        } else if (name == "CarbonLib::InsetRect") {
            for (unsigned i = 0; i < 4; ++i) {
                const auto delta = std::int16_t(arg(i % 2 == 0 ? 2 : 1));
                const auto old = std::int16_t(s.memory.read<2>(arg(0) + i * 2));
                s.memory.write<2>(arg(0) + i * 2, std::uint16_t(old + (i < 2 ? delta : -delta)));
            }
            return_value = arg(0);
        } else if (name == "CarbonLib::PtInRgn") {
            const auto& handle = impl_->handles.at(arg(1));
            return_value = region_contains(s.memory.copy_out(handle.payload, handle.size),
                                           std::int16_t(arg(0)), std::int16_t(arg(0) >> 16));
        } else if (name == "CarbonLib::NewRgn") {
            const auto handle = impl_->new_handle(s, 10);
            if (handle) {
                const auto data = impl_->handles.at(handle).payload;
                s.memory.clear(data, 10);
                s.memory.write<2>(data, 10); // Empty rectangular QuickDraw region.
            }
            return_value = handle;
        } else if (name == "CarbonLib::DisposeRgn") {
            impl_->dispose_handle(s, arg(0));
            return_value = arg(0);
        } else if (name == "CarbonLib::GetResource" || name == "CarbonLib::Get1Resource" || name == "CarbonLib::Get1IndResource") {
            const auto resource = impl_->resource_manager().get(arg(0), std::int16_t(arg(1)),
                name != "CarbonLib::GetResource", name == "CarbonLib::Get1IndResource");
            return_value = impl_->resource_handle(s, resource);
        } else if (name == "CarbonLib::AddResource") {
            const auto& handle = impl_->handles.at(arg(0));
            const auto name_size = arg(3) ? s.memory.read<1>(arg(3)) : 0;
            const auto resource_name = name_size ? s.memory.copy_out(arg(3) + 1, name_size) : std::vector<std::uint8_t>{};
            const auto ticket = impl_->resource_manager().add(arg(1), std::int16_t(arg(2)), resource_name,
                s.memory.copy_out(handle.payload, handle.size));
            if (ticket) impl_->resource_handles[ticket] = arg(0);
            return_value = arg(0);
        } else if (name == "CarbonLib::WriteResource" || name == "CarbonLib::RemoveResource") {
            const auto found = std::find_if(impl_->resource_handles.begin(), impl_->resource_handles.end(),
                [&](const auto& item) { return item.second == arg(0); });
            if (found == impl_->resource_handles.end()) throw std::runtime_error("Unknown original resource handle");
            if (name == "CarbonLib::WriteResource") {
                const auto& handle = impl_->handles.at(arg(0));
                impl_->resource_manager().write(found->first, s.memory.copy_out(handle.payload, handle.size));
            } else {
                impl_->resource_manager().remove(found->first);
                if (!impl_->resource_manager().error()) impl_->resource_handles.erase(found);
            }
            return_value = arg(0);
        } else if (name == "CarbonLib::ReleaseResource" || name == "CarbonLib::DetachResource") {
            auto found = std::find_if(impl_->resource_handles.begin(), impl_->resource_handles.end(),
                [&](const auto& item) { return item.second == arg(0); });
            if (found == impl_->resource_handles.end()) throw std::runtime_error("Unknown original resource handle");
            impl_->resource_manager().release(found->first, name == "CarbonLib::DetachResource");
            if (!impl_->resource_manager().error()) {
                if (name == "CarbonLib::ReleaseResource") impl_->dispose_handle(s, found->second);
                impl_->resource_handles.erase(found);
            }
            return_value = arg(0);
        } else if (name == "CarbonLib::GetIndString") {
            s.memory.copy_in(arg(0), impl_->resource_manager().string(std::int16_t(arg(1)), std::int16_t(arg(2))));
            return_value = arg(0);
        } else if (name == "CarbonLib::CurResFile") {
            return_value = std::uint32_t(std::int32_t(impl_->resource_manager().current()));
        } else if (name == "CarbonLib::UseResFile") {
            impl_->resource_manager().use(std::int16_t(arg(0)));
            return_value = arg(0);
        } else if (name == "CarbonLib::SetResLoad") {
            impl_->resource_manager().set_load(arg(0) != 0);
            return_value = arg(0);
        } else if (name == "CarbonLib::Count1Resources") {
            return_value = std::uint32_t(std::int32_t(impl_->resource_manager().count(arg(0))));
        } else if (name == "CarbonLib::FindFolder") {
            const auto ref = std::int16_t(arg(0));
            if (ref != std::int16_t(0x8000) && ref != 0 && ref != -1 && ref != -2)
                throw FileError{volume_not_found};
            std::string folder;
            if (arg(1) == 0x70726566) folder = "Preferences";
            else if (arg(1) == 0x74656d70) folder = "Temporary Items";
            else throw std::runtime_error("Unsupported original FindFolder selector: " + std::to_string(arg(1)));
            const auto target = impl_->allowed(impl_->writable / folder);
            if (arg(2)) fs::create_directories(target);
            if (!fs::is_directory(target)) throw FileError{directory_not_found};
            s.memory.write<2>(arg(3), std::uint16_t(-2));
            s.memory.write<4>(arg(4), impl_->directory_id(target));
        } else if (name == "CarbonLib::FSpDirCreate") {
            const auto target = impl_->from_spec(s, arg(0));
            if (fs::exists(target)) throw FileError{-48};
            const auto backing = impl_->writable_path(target);
            fs::create_directories(backing.parent_path());
            if (!fs::create_directory(backing)) throw FileError{-48};
            s.memory.write<4>(arg(2), impl_->directory_id(backing));
        } else if (name == "CarbonLib::FSpCreateResFile") {
            const auto target = impl_->from_spec(s, arg(0));
            if (!under(target, impl_->writable)) throw std::runtime_error("Resource creation requires the writable data directory");
            impl_->resource_manager().create(target, arg(1), arg(2));
            return_value = arg(0);
        } else if (name == "CarbonLib::FSpOpenResFile") {
            const auto target = impl_->from_spec(s, arg(0));
            auto permission = std::int8_t(arg(1));
            if (!under(target, impl_->writable)) {
                if (permission != 0 && permission != 1)
                    throw std::runtime_error("Writable resource access requires the writable data directory");
                // fsCurPerm uses the permissions exposed by this provider.
                // Original media files are read-only; private files retain the
                // native writable default used by the preferences resource file.
                permission = 1;
            }
            return_value = std::uint32_t(std::int32_t(impl_->resource_manager().open(target, permission)));
        } else if (name == "CarbonLib::CloseResFile") {
            for (auto ticket : impl_->resource_manager().close(std::int16_t(arg(0)))) {
                const auto found = impl_->resource_handles.find(ticket);
                if (found != impl_->resource_handles.end()) {
                    impl_->dispose_handle(s, found->second);
                    impl_->resource_handles.erase(found);
                }
            }
            return_value = arg(0);
        } else if (name == "CarbonLib::ResError") {
            return_value = std::uint32_t(std::int32_t(impl_->resource_manager().error()));
        } else if (name == "CarbonLib::FSpMakeFSRef") {
            impl_->write_reference(s, arg(1), impl_->reference_for(impl_->from_spec(s, arg(0))));
        } else if (name == "CarbonLib::FSMakeFSRefUnicode") {
            const auto parent = impl_->from_reference(s, arg(0));
            if (!fs::is_directory(parent)) throw FileError{directory_not_found};
            std::string leaf;
            status = native_unicode_name(impl_->unicode_units(s, arg(2), arg(1)), leaf);
            if (!status) impl_->write_reference(s, arg(4), impl_->reference_for(impl_->allowed(impl_->visible_child(parent, leaf))));
        } else if (name == "CarbonLib::FSGetCatalogInfo") {
            if (arg(1) && !arg(2)) throw FileError{parameter_error};
            const auto target = impl_->from_reference(s, arg(0));
            NativeFileCatalog info;
            status = native_file_catalog(impl_->reference_for(target), arg(1), info);
            if (!status) {
                write_be<2>(info.bytes.data() + 2, std::uint16_t(impl_->volume(target)));
                if (arg(1) & 0x8) write_be<4>(info.bytes.data() + 4, impl_->volume_root(target) ? 1 : impl_->directory_id(target.parent_path()));
                if (arg(1) & 0x10) write_be<4>(info.bytes.data() + 8, impl_->node_id(target));
                if (fs::is_directory(target) && (arg(1) & 0x2000))
                    write_be<4>(info.bytes.data() + 136, impl_->entries(target).size());
                if (arg(2)) write_guest_file_catalog(s, arg(2), arg(1), info.bytes);
                if (arg(3)) {
                    if (impl_->volume_root(target)) info.name = impl_->volume(target) == -1 ? u"CM 01-02" : u"CM User Data";
                    s.memory.write<2>(arg(3), info.name.size());
                    for (unsigned n = 0; n < info.name.size(); ++n) s.memory.write<2>(std::uint64_t(arg(3)) + 2 + n * 2, info.name[n]);
                }
                if (arg(4)) impl_->write_spec(s, arg(4), target);
                if (arg(5)) impl_->write_reference(s, arg(5), impl_->reference_for(impl_->volume_root(target) ? target : target.parent_path()));
            }
        } else if (name == "CarbonLib::FSCompareFSRefs") {
            status = native_compare_references(impl_->reference_for(impl_->from_reference(s, arg(0))),
                impl_->reference_for(impl_->from_reference(s, arg(1))));
        } else if (name == "CarbonLib::FSCreateFileUnicode") {
            const auto parent = impl_->from_reference(s, arg(0));
            if (!fs::is_directory(parent)) throw FileError{directory_not_found};
            if (arg(3) && !arg(4)) throw FileError{parameter_error};
            if (arg(3) & 0x400000) throw FileError{parameter_error};
            const auto characters = impl_->unicode_units(s, arg(2), arg(1));
            std::string leaf;
            status = native_unicode_name(characters, leaf);
            if (!status) {
                if (fs::exists(fs::symlink_status(impl_->visible_child(parent, leaf)))) throw FileError{-48};
                const auto input_catalog = read_guest_file_catalog(s, arg(4), arg(3));
                const auto backing = impl_->writable_path(parent);
                fs::create_directories(backing);
                NativeFileReference created{};
                status = native_create_unicode_file(impl_->reference_for(backing), characters, arg(3),
                    input_catalog, created);
                if (!status) {
                    fs::path target;
                    status = native_reference_path(created, target);
                    if (!status) {
                        if (arg(5)) impl_->write_reference(s, arg(5), created);
                        if (arg(6)) impl_->write_spec(s, arg(6), target);
                    }
                }
            }
        } else if (name == "CarbonLib::FSOpenFork") {
            auto target = impl_->from_reference(s, arg(0));
            auto permission = std::int8_t(arg(3));
            if (!permission) permission = under(target, impl_->writable) ? 3 : 1;
            if (permission != 1) target = impl_->writable_file(target);
            std::int16_t ref = 0;
            status = impl_->data_forks.open(target, impl_->unicode_units(s, arg(2), arg(1)), permission, ref);
            impl_->performance.mark("file-open", s, std::uint16_t(ref));
            if (!status) s.memory.write<2>(arg(4), std::uint16_t(ref));
        } else if (name == "CarbonLib::FSDeleteObject") {
            status = impl_->delete_object(impl_->from_reference(s, arg(0)));
        } else if (name == "CarbonLib::FSRenameUnicode") {
            const auto target = impl_->from_reference(s, arg(0));
            std::string leaf;
            status = native_unicode_name(impl_->unicode_units(s, arg(2), arg(1)), leaf);
            if (!status) status = impl_->rename_object(target, leaf, arg(3));
            if (!status && arg(4)) impl_->write_reference(s, arg(4), impl_->reference_for(target.parent_path() / leaf));
        } else if (name == "CarbonLib::FSMakeFSSpec") {
            const auto target = impl_->path(std::int16_t(arg(0)), arg(1), read_mac_string(s, arg(2)));
            impl_->write_spec(s, arg(3), target);
            if (!fs::exists(target)) status = file_not_found;
        } else if (name == "CarbonLib::ResolveAliasFile") {
            const auto input = impl_->from_spec(s, arg(0));
            if (!fs::exists(input)) throw FileError{file_not_found};
            const auto target = impl_->allowed(fs::canonical(input));
            impl_->write_spec(s, arg(0), target);
            s.memory.write<1>(arg(2), fs::is_directory(target));
            s.memory.write<1>(arg(3), fs::is_symlink(input));
        } else if (name == "CarbonLib::FSpCreate" || name == "CarbonLib::PBHCreateSync") {
            const bool pb = name == "CarbonLib::PBHCreateSync";
            const auto target = pb ? impl_->path(std::int16_t(s.memory.read<2>(arg(0) + 22)),
                s.memory.read<4>(arg(0) + 48), read_mac_string(s, s.memory.read<4>(arg(0) + 18))) : impl_->from_spec(s, arg(0));
            if (fs::exists(target)) throw FileError{-48};
            const auto output = impl_->writable_path(target);
            fs::create_directories(output.parent_path());
            status = impl_->data_forks.create(output, pb ? 0 : arg(1), pb ? 0 : arg(2));
        } else if (name == "CarbonLib::PBHRenameSync") {
            const auto pb = arg(0);
            const auto replacement = std::uint32_t(s.memory.read<4>(pb + 28));
            if (!replacement || s.memory.read<1>(replacement) > 63) throw FileError{bad_name};
            const auto target = impl_->path(std::int16_t(s.memory.read<2>(pb + 22)),
                s.memory.read<4>(pb + 48), read_mac_string(s, s.memory.read<4>(pb + 18)));
            const auto leaf = read_mac_string(s, replacement);
            status = impl_->rename_object(target, leaf);
            if (std::getenv("CM_AOT_TRACE_FILES"))
                std::fprintf(stderr, "Native file rename: %s -> %s status=%d\n", target.c_str(), leaf.c_str(), status);
        } else if (name == "CarbonLib::PBHDeleteSync") {
            const auto target = impl_->path(std::int16_t(s.memory.read<2>(arg(0) + 22)),
                s.memory.read<4>(arg(0) + 48), read_mac_string(s, s.memory.read<4>(arg(0) + 18)));
            status = impl_->delete_object(target);
            if (std::getenv("CM_AOT_TRACE_FILES"))
                std::fprintf(stderr, "Native file delete: %s status=%d\n", target.c_str(), status);
        } else if (name == "CarbonLib::PBHOpenDFSync" || name == "CarbonLib::FSpOpenDF") {
            const bool parameter_block = name == "CarbonLib::PBHOpenDFSync";
            auto permission = std::int8_t(parameter_block ? s.memory.read<1>(arg(0) + 27) : arg(1));
            if (permission < 0 || permission > 3) throw FileError{parameter_error};
            auto target = parameter_block ? impl_->path(std::int16_t(s.memory.read<2>(arg(0) + 22)),
                s.memory.read<4>(arg(0) + 48), read_mac_string(s, s.memory.read<4>(arg(0) + 18))) : impl_->from_spec(s, arg(0));
            if (!permission) permission = under(target, impl_->writable) ? 3 : 1;
            if (permission != 1) target = impl_->writable_file(target);
            std::int16_t ref = 0;
            status = impl_->data_forks.open(target, permission, ref);
            impl_->performance.mark("file-open", s, std::uint16_t(ref));
            if (std::getenv("CM_AOT_TRACE_FILES"))
                std::fprintf(stderr, "Native file open: %s permission=%d status=%d\n", target.c_str(), permission, status);
            if (!status) s.memory.write<2>(parameter_block ? arg(0) + 24 : arg(2), std::uint16_t(ref));
        } else if (name == "CarbonLib::PBWriteSync" || name == "CarbonLib::FSWrite") {
            const bool pb = name == "CarbonLib::PBWriteSync";
            const auto ref = std::int16_t(pb ? s.memory.read<2>(arg(0) + 24) : arg(0));
            const auto mode = std::uint16_t(pb ? s.memory.read<2>(arg(0) + 44) : 0);
            const auto offset = std::int32_t(pb ? s.memory.read<4>(arg(0) + 46) : 0);
            const auto requested = std::int32_t(s.memory.read<4>(pb ? arg(0) + 36 : arg(1)));
            const auto buffer = std::uint32_t(pb ? s.memory.read<4>(arg(0) + 32) : arg(2));
            std::uint32_t actual = 0;
            status = impl_->write_data(s, ref, mode, offset, buffer, requested, actual);
            s.memory.write<4>(pb ? arg(0) + 40 : arg(1), actual);
            if (pb) {
                std::int64_t position;
                if (!impl_->data_forks.position(ref, position)) s.memory.write<4>(arg(0) + 46, std::uint32_t(position));
            }
        } else if (name == "CarbonLib::PBSetEOFSync" || name == "CarbonLib::SetEOF") {
            const bool pb = name == "CarbonLib::PBSetEOFSync";
            const auto ref = std::int16_t(pb ? s.memory.read<2>(arg(0) + 24) : arg(0));
            status = impl_->data_forks.resize(ref, std::int32_t(pb ? s.memory.read<4>(arg(0) + 28) : arg(1)));
        } else if (name == "CarbonLib::PBReadSync" || name == "CarbonLib::FSRead") {
            const bool pb = name == "CarbonLib::PBReadSync";
            const auto ref = std::int16_t(pb ? s.memory.read<2>(arg(0) + 24) : arg(0));
            const auto mode = std::uint16_t(pb ? s.memory.read<2>(arg(0) + 44) : 0);
            const auto offset = std::int32_t(pb ? s.memory.read<4>(arg(0) + 46) : 0);
            const auto requested = std::int32_t(s.memory.read<4>(pb ? arg(0) + 36 : arg(1)));
            const auto buffer = std::uint32_t(pb ? s.memory.read<4>(arg(0) + 32) : arg(2));
            std::uint32_t actual = 0;
            status = impl_->read_data(s, ref, mode, offset, buffer, requested, actual);
            s.memory.write<4>(pb ? arg(0) + 40 : arg(1), actual);
            if (pb) {
                std::int64_t position;
                if (!impl_->data_forks.position(ref, position)) s.memory.write<4>(arg(0) + 46, std::uint32_t(position));
            }
        } else if (name == "CarbonLib::PBGetEOFSync" || name == "CarbonLib::GetEOF") {
            const bool pb = name == "CarbonLib::PBGetEOFSync";
            const auto ref = std::int16_t(pb ? s.memory.read<2>(arg(0) + 24) : arg(0));
            std::int64_t size = 0;
            status = impl_->data_forks.size(ref, size);
            if (!status) {
                if (size > INT32_MAX) throw std::runtime_error("Large file requires the 64-bit fork API bridge");
                s.memory.write<4>(pb ? arg(0) + 28 : arg(1), std::uint32_t(size));
            }
        } else if (name == "CarbonLib::PBSetFPosSync" || name == "CarbonLib::SetFPos") {
            const bool pb = name == "CarbonLib::PBSetFPosSync";
            const auto ref = std::int16_t(pb ? s.memory.read<2>(arg(0) + 24) : arg(0));
            const auto mode = std::uint16_t(pb ? s.memory.read<2>(arg(0) + 44) : arg(1));
            const auto offset = std::int32_t(pb ? s.memory.read<4>(arg(0) + 46) : arg(2));
            status = impl_->data_forks.seek(ref, mode, offset);
            if (pb) {
                std::int64_t position;
                const auto position_error = impl_->data_forks.position(ref, position);
                if (!position_error) s.memory.write<4>(arg(0) + 46, std::uint32_t(position));
                else if (!status) status = position_error;
            }
        } else if (name == "CarbonLib::PBCloseSync" || name == "CarbonLib::FSClose") {
            const auto ref = std::int16_t(name == "CarbonLib::PBCloseSync" ? s.memory.read<2>(arg(0) + 24) : arg(0));
            status = impl_->data_forks.close(ref);
            impl_->performance.mark("file-close", s, std::uint16_t(ref));
        } else if (name == "CarbonLib::PBGetCatInfoSync") {
            impl_->catalog_info(s, arg(0));
        } else if (name == "CarbonLib::PBSetCatInfoSync") {
            impl_->set_catalog_info(s, arg(0));
        } else if (name == "CarbonLib::PBHGetVInfoSync") {
            impl_->volume_info(s, arg(0));
        } else if (name == "CarbonLib::PBHGetVolParmsSync") {
            const auto pb = arg(0);
            const auto ref = std::int16_t(s.memory.read<2>(pb + 22));
            const auto parameters = native_volume_parameters(impl_->directory(ref, 0));
            status = parameters.error;
            s.memory.write<4>(pb + 40, 0);
            if (!status) {
                const auto buffer = std::uint32_t(s.memory.read<4>(pb + 32));
                const auto requested = std::int32_t(s.memory.read<4>(pb + 36));
                if (requested < 0) throw FileError{parameter_error};
                std::vector<std::uint8_t> bytes(20, 0);
                auto put = [&](unsigned offset, unsigned size, std::uint32_t value) {
                    for (unsigned i = 0; i < size; ++i) bytes[offset + i] = value >> (8 * (size - i - 1));
                };
                put(0, 2, parameters.version); put(2, 4, parameters.attributes);
                put(10, 4, parameters.server_address); put(14, 4, parameters.speed);
                put(18, 2, std::uint16_t(parameters.privilege_model));
                bytes.resize(std::min<std::size_t>(bytes.size(), requested));
                if (!bytes.empty()) s.memory.copy_in(buffer, bytes);
                s.memory.write<4>(pb + 40, bytes.size());
            }
        } else if (name == "CarbonLib::FSpGetFInfo") {
            const auto target = impl_->from_spec(s, arg(0));
            if (!fs::is_regular_file(target)) throw FileError{file_not_found};
            const auto data = finder_info(target);
            s.memory.copy_in(arg(1), {data.begin(), data.begin() + 16});
        } else if (name == "CarbonLib::HGetVol") {
            if (arg(0)) write_mac_string(s, arg(0), impl_->current_volume == -1 ? "CM 01-02" : "CM User Data");
            s.memory.write<2>(arg(1), std::uint16_t(impl_->current_volume));
            s.memory.write<4>(arg(2), impl_->current_directory);
        } else {
            return false;
        }
    } catch (const FileError& error) { status = error.code; }
    catch (const fs::filesystem_error&) { status = parameter_error; }
    if (status && std::getenv("CM_AOT_TRACE_FILES") && name == "CarbonLib::PBGetCatInfoSync") {
        try {
            std::fprintf(stderr, "Native catalog failure: name=%s ref=%d dir=%u index=%d status=%d caller=%08x\n",
                read_mac_string(s, s.memory.read<4>(arg(0) + 18)).c_str(),
                std::int16_t(s.memory.read<2>(arg(0) + 22)), unsigned(s.memory.read<4>(arg(0) + 48)),
                std::int16_t(s.memory.read<2>(arg(0) + 28)), status, unsigned(s.r<4>(4128)));
        } catch (...) { std::fprintf(stderr, "Native catalog failure: unreadable parameters\n"); }
    }
    if (name == "CarbonLib::PBGetCatInfoSync" || name == "CarbonLib::PBSetCatInfoSync" || name == "CarbonLib::PBHGetVInfoSync" || name == "CarbonLib::PBHGetVolParmsSync" ||
        name == "CarbonLib::PBHOpenDFSync" || name == "CarbonLib::PBReadSync" || name == "CarbonLib::PBGetEOFSync" ||
        name == "CarbonLib::PBHCreateSync" || name == "CarbonLib::PBHDeleteSync" || name == "CarbonLib::PBHRenameSync" || name == "CarbonLib::PBWriteSync" || name == "CarbonLib::PBSetEOFSync" ||
        name == "CarbonLib::PBSetFPosSync" || name == "CarbonLib::PBCloseSync")
        s.memory.write<2>(arg(0) + 16, std::uint16_t(status));
    s.wr<4>(12, return_value.value_or(std::uint32_t(std::int32_t(status))));
    s.pc = std::uint32_t(s.r<4>(4128));
    return true;
}
std::vector<float> NativePlatform::render_audio_offline(std::uint32_t frames) { return impl_->sound.render_offline(frames); }
void NativePlatform::observe_drawing(State& s, std::uint32_t pc) { impl_->drawing.boundary(s, pc); }
}
