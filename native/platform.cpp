#include "platform.hpp"
#include "aot_runtime.hpp"
#include "text_bridge.hpp"
#include "process_bridge.hpp"
#include "ui_bridge.hpp"
#include "native_drawing.hpp"
#include "direct_commands.hpp"
#include "quick_save.hpp"
#include <functional>
#include "quick_save_transaction.hpp"
#include "quick_save_runtime.hpp"
#include "guest_context_transfer.hpp"
#include "match_debugging.hpp"
#include "debug_attribute_persistence.hpp"
#include "resource_bridge.hpp"
#include "filesystem_bridge.hpp"
#include "directory_catalog.hpp"
#include "printing_bridge.hpp"
#include "callbacks.hpp"
#include "apple_events.hpp"
#include "speech_bridge.hpp"
#include "data_forks.hpp"
#include "fork_io_bridge.hpp"
#include "unicode_bridge.hpp"
#include "file_reference_bridge.hpp"
#include "hfs_names.hpp"
#include "sound_bridge.hpp"
#include "performance_trace.hpp"
#include "automation.hpp"
#include "season_reference.hpp"
#include "guest_profiler.hpp"
#include <fstream>
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
// EqualString: two Pascal strings of the same length whose MacRoman
// characters match one by one, ignoring case and diacritics unless asked.
bool equal_mac_strings(State& s, std::uint32_t first, std::uint32_t second, bool case_sensitive, bool diacritic_sensitive) {
    const auto length = s.memory.read<1>(first);
    if (length != s.memory.read<1>(second)) return false;
    const auto options = (case_sensitive ? 0 : kCFCompareCaseInsensitive) | (diacritic_sensitive ? 0 : kCFCompareDiacriticInsensitive);
    for (unsigned n = 1; n <= length; ++n) {
        const UInt8 a = s.memory.read<1>(first + n), b = s.memory.read<1>(second + n);
        if (a == b) continue;
        if (!options) return false;
        auto x = CFStringCreateWithBytes(nullptr, &a, 1, kCFStringEncodingMacRoman, false);
        auto y = CFStringCreateWithBytes(nullptr, &b, 1, kCFStringEncodingMacRoman, false);
        const bool same = x && y && CFStringCompare(x, y, options) == kCFCompareEqualTo;
        if (x) CFRelease(x);
        if (y) CFRelease(y);
        if (!same) return false;
    }
    return true;
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
namespace {
// r2 inside an import is that import's. The rewritten event loop calls imports
// directly, so the caller's linkage area does not hold the game's value either.
constexpr std::uint32_t game_toc = 0x108bba90;
}
struct NativePlatform::Impl {
    // At the last screen boundary, the original's save conditions held. They
    // can only be read there: event waits leave the current row at 16.
    bool quick_save_possible = false;
    NativeDrawingObserver drawing;
    std::optional<fs::path> debug_attribute_load_target, debug_attribute_save_target, debug_attribute_save_override;
    bool debug_attribute_loading = false, debug_attribute_saving = false;
    bool debug_attribute_rename_needed = false, debug_attribute_rename_ok = false;
    QuickSaveDestination quick_save_destination;
    bool quick_save_queued = false, quick_save_rename_ok = false, quick_save_tracking_write = false, quick_save_active = false;
    bool quick_save_needs_rename = false, quick_save_loading = false, quick_save_quiet = false;
    std::optional<fs::path> quick_save_last_target, quick_save_load_target, quick_save_write_target;
    std::unique_ptr<QuickSaveTransaction> quick_save_transaction;
    std::unique_ptr<QuickSaveTransaction> quick_save_previous_transaction;
    std::uint32_t quick_save_path_address = 0;
    bool quick_save_swept = false;
    // What the writer's successful path overwrites, in case promotion then fails.
    std::optional<std::array<std::uint8_t, 7>> quick_save_saved_markers;
    enum class QuickSaveOutcome { none, complete, failed, uncertain } quick_save_outcome = QuickSaveOutcome::none;
    std::function<void(const std::string&)> quick_save_alert_observer;
    void quick_save_alert(const std::string& message) {
        try {
            if (quick_save_alert_observer) quick_save_alert_observer(message);
            else if (automation) std::fprintf(stderr, "Quick save alert: %s\n", message.c_str());
            else if (ui) ui->show_alert("Quick save failed", message);
        } catch (const std::exception& error) { std::fprintf(stderr, "Quick save alert: %s\n", error.what()); }
    }
    explicit Impl(AudioMode mode, EventMode events) : sound(mode), event_mode(events) {}
    std::optional<fs::path> debug_attribute_target(const State& s, std::uint32_t address) const {
        if (!address) return std::nullopt;
        try {
            std::string guest;
            for (unsigned i = 0; i < 256; ++i) {
                const auto byte = s.memory.read<1>(address + i);
                if (!byte) {
                    if (guest.empty()) return std::nullopt;
                    const auto name = convert(guest, kCFStringEncodingMacRoman, kCFStringEncodingUTF8);
                    const auto logical = !name.empty() && name.front() == '/' ? allowed(fs::path(name)) : path(0, 0, name);
                    return writable_path(logical);
                }
                guest += char(byte);
            }
        } catch (const FileError&) {} catch (const std::exception&) {}
        return std::nullopt;
    }
    NativeSound sound;
    EventMode event_mode;
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
    NativeSpeech speech;
    std::deque<UIEvent> pending_events;
    std::unique_ptr<Automation> automation = Automation::from_environment();
    // A request that only observes (snapshot, memory, profile, capture),
    // answered at any event boundary.
    void observe(State& s, const AutomationRequest& request) {
        using Kind = AutomationRequest::Kind;
        std::string result;
        try {
            if (request.kind == Kind::snapshot) {
                for (const auto& path : user_interface().save_window_images(automation->output("snapshots", request.name)))
                    result += (result.empty() ? "" : " ") + path.string();
            } else if (request.kind == Kind::profile) {
                if (request.name.empty()) { guest_profiler::reset(1000000); guest_profiler::start(s); result = "sampling"; }
                else if (!guest_profiler::running()) throw std::runtime_error("not sampling");
                else {
                    guest_profiler::stop();
                    const auto path = automation->output("profiles", request.name + ".samples");
                    std::filesystem::create_directories(path.parent_path());
                    result = std::to_string(guest_profiler::write(path)) + " samples in " + path.string();
                    guest_profiler::release();
                }
            } else if (request.kind == Kind::capture) {
                // Recorded from the end of this event call; replay with cm-world-replay.
                result = request.name.empty() ? season_session_request(std::nullopt)
                    : season_session_request(std::filesystem::absolute(automation->output("captures", request.name)));
            } else {
                const auto bytes = s.memory.copy_out(request.address, request.size);
                const auto path = automation->output("memory", request.name + ".bin");
                std::filesystem::create_directories(path.parent_path());
                std::ofstream(path, std::ios::binary).write(reinterpret_cast<const char*>(bytes.data()), std::streamsize(bytes.size()));
                result = path.string();
            }
            automation->finish(request, true, result);
        } catch (const std::exception& failure) { automation->finish(request, false, failure.what()); }
    }
    // The game window (the one whose content is the 800 x 600 screen) and its
    // content bounds; 0 when there is none.
    std::pair<std::uint32_t, LegacyRect> game_screen() {
        std::pair<std::uint32_t, LegacyRect> found{0, {}};
        for (const auto window : windows) {
            const auto bounds = user_interface().window_bounds(window, true);
            if (bounds[2] - bounds[0] == 600 && bounds[3] - bounds[1] == 800) found = {window, bounds};
        }
        return found;
    }
    // A document an open- or print-documents event may name: a file under
    // the game's or the writable folders.
    bool reachable_document(const fs::path& file) const {
        std::error_code error;
        try {
            return fs::is_regular_file(allowed(file), error);
        } catch (const FileError&) { return false; }
    }
    std::uint32_t front_dialog = 0;  // the modal dialog ModalDialog runs
    std::uint32_t new_dialog(State& s, std::int16_t id) {
        const auto dialog = resource_manager().get(0x444c4f47, id, false);  // DLOG
        if (!dialog.loaded) return 0;
        const auto definition = parse_dialog(dialog.bytes);
        const auto items = resource_manager().get(0x4449544c, definition.items, false);  // DITL
        if (!items.loaded) throw std::runtime_error("Dialog references a missing DITL resource");
        // The DialogRef: guest storage standing for the dialog record.
        const auto token = allocate(s, 0xaa, true);
        if (!token) return 0;
        user_interface().create_dialog(token, definition, parse_dialog_items(items.bytes));
        front_dialog = token;
        return token;
    }
    // ModalDialog: waits for a click on an enabled item of the front dialog,
    // or an automation `dialog N` request, which the channel offers while the
    // dialog waits.
    std::int16_t modal_dialog(State& s) {
        if (!front_dialog) throw std::runtime_error("ModalDialog without a dialog");
        CommandContext waiting;
        waiting.ready = true;
        waiting.identity = 0xd1a1000000000000ull | front_dialog;
        bool automation_save = false;
        for (;;) {
            // Automation is offered at most every 50 ms; without it the wait is longer.
            if (const auto item = user_interface().dialog_click(front_dialog, automation ? 0.05 : 0.5)) return item;
            // The writer cannot run here: event waits leave the current row at
            // 16, and it picks its view by that row. Preferences has no way out
            // but closing, and the next screen boundary saves.
            if (s.r<4>(4128) == 0x10770e94 && (std::exchange(automation_save, false) || user_interface().take_quick_save_request())) {
                if (s.platform && s.platform->quick_save_request(s)) command_status = "Quick save queued until this dialog closes";
                else {
                    command_status = "Quick save unavailable";
                    user_interface().beep();
                }
            }
            if (!automation) continue;
            automation->enter(s.executed);
            automation->boundary(s, waiting, "Native dialog: awaiting interaction");
            auto request = automation->take(waiting);
            automation->leave(s.executed);
            if (!request) continue;
            using Kind = AutomationRequest::Kind;
            if (!request->input()) { observe(s, *request); continue; }
            if (request->kind == Kind::quit) {
                // As AppKit's would, it waits until the dialog closes.
                pending_events.push_back({UIEvent::Kind::quit});
                automation->finish(*request, true, "quit queued");
                continue;
            }
            // As the key equivalent reaches this modal in play.
            if (request->kind == Kind::key && request->keys.size() == 1 &&
                (request->keys[0].first & 0xff) == 's' && (request->keys[0].second & 0x100)) {
                automation_save = true;
                automation->finish(*request, true, "quick save requested");
                continue;
            }
            const bool click = request->kind == Kind::dialog;
            if (click && user_interface().dialog_item_enabled(front_dialog, request->control)) {
                automation->finish(*request, true, "item " + std::to_string(request->control));
                return request->control;
            }
            automation->finish(*request, false, click ? "the item is not enabled" : "a dialog is open");
        }
    }
    std::map<std::array<std::uint8_t, 16>, UIEvent> delivered_events;
    bool application_opened = false; // an 'oapp' or 'odoc' event was delivered
    std::uint32_t last_event_tick = 0;
    std::optional<std::uint32_t> menu_selection;
    // An automation drag, timed like a person's from when the game takes the
    // mouse-down: the button is held at the press for 400 ms (list drags
    // start only after a hold), the pointer GetMouse and Button report moves
    // to the target over 400 ms and holds there for 200 ms, then the button
    // is released and a mouse-up is queued (global coordinates). The request
    // finishes when the game takes the mouse-up, or 2 s after the release if
    // it never does (FlushEvents).
    struct SyntheticDrag {
        std::int16_t from_x, from_y, to_x, to_y;
        std::optional<AutomationRequest> request;
        std::optional<std::chrono::steady_clock::time_point> start;  // when the game took the mouse-down
        bool released = false;
        double elapsed() const {
            return start ? std::chrono::duration<double>(std::chrono::steady_clock::now() - *start).count() : 0.0;
        }
        NativePointerState state() const {
            const auto f = std::clamp((elapsed() - 0.4) / 0.4, 0.0, 1.0);
            const auto x = std::int16_t(from_x + std::lround((to_x - from_x) * f));
            const auto y = std::int16_t(from_y + std::lround((to_y - from_y) * f));
            return {x, y, std::uint16_t(released ? 0x81 : 0x01), !released};
        }
    };
    std::optional<SyntheticDrag> drag;
    NativePointerState pointer_now() { advance_drag(); return drag ? drag->state() : user_interface().pointer_state(); }
    void end_drag(bool delivered) {
        if (drag && drag->request && automation)
            automation->finish(*drag->request, delivered, delivered ? "dragged" : "the game never took the mouse-up");
        drag.reset();
    }
    // At the end of the drag the button goes up and a mouse-up is queued.
    void advance_drag() {
        if (!drag || !drag->start) return;
        if (drag->released) { if (drag->elapsed() > 3.0) end_drag(false); return; }
        if (drag->elapsed() < 1.0) return;
        drag->released = true;
        UIEvent up{UIEvent::Kind::mouse_up};
        up.x = drag->to_x; up.y = drag->to_y; up.modifiers = 0x81;
        pending_events.push_back(std::move(up));
    }
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
    // Entries a MacRoman File Manager call can name within the 63 bytes
    // write_spec accepts. The game stops a directory scan at its first error,
    // so one Finder-renamed file would otherwise hide every later save.
    std::vector<fs::path> mac_entries(const fs::path& input) const {
        std::vector<fs::path> result;
        for (const auto& entry : listing(input)) {
            const auto name = entry.path.filename().string();
            bool fits = name.size() <= 63 && std::ranges::all_of(name, [](unsigned char c) { return c < 0x80; });
            if (!fits) {
                try { fits = convert(hfs_name(name), kCFStringEncodingUTF8, kCFStringEncodingMacRoman).size() <= 63; }
                catch (const FileError&) {}
            }
            if (fits) result.push_back(entry.path);
        }
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
        const auto fail = [&](std::int16_t error) { data_forks.note_write_audit_failure(target); return error; };
        if (!fs::exists(fs::symlink_status(target))) return fail(file_not_found);
        if (volume_root(target) || target == overlay() ||
            logical_path(target) == logical_path(directory(current_volume, current_directory))) return fail(-47);
        // Original media is immutable. Removing a private replacement must not
        // reveal the original again: that case needs persistent tombstones.
        const auto logical = logical_path(target);
        if (!under(fs::canonical(target.parent_path()), writable) ||
            (under(logical, game) && fs::exists(fs::symlink_status(logical)))) {
            data_forks.note_write_audit_failure(target);
            throw std::runtime_error("Deleting an original media entry requires private tombstone support: " + target.string());
        }
        data_forks.make_durable(target.parent_path());
        const auto status = native_delete_object(target);
        if (status) data_forks.note_write_audit_failure(target);
        if (!status) {
            file_ids.erase(logical);
            std::erase_if(directories, [&](const auto& entry) { return logical_path(entry.second) == logical; });
        }
        return status;
    }

    // `name` is a host name; callers convert Mac names with host_name.
    std::int16_t rename_object(const fs::path& target, const std::string& name, std::uint32_t encoding = UINT32_MAX) {
        const auto fail = [&](std::int16_t error) { data_forks.note_write_audit_failure(target); return error; };
        if (name.empty() || name == "." || name == ".." || name.find('/') != std::string::npos ||
            name.find('\0') != std::string::npos) return fail(bad_name);
        if (!fs::exists(fs::symlink_status(target))) return fail(file_not_found);
        const auto logical = logical_path(target);
        // Renaming original entries requires a tombstone so the old media
        // name cannot reappear. Keep this unsupported case mutation-free.
        if (volume_root(target) || target == overlay() ||
            !under(fs::canonical(target.parent_path()), writable) ||
            (under(logical, game) && fs::exists(fs::symlink_status(logical)))) {
            data_forks.note_write_audit_failure(target);
            throw std::runtime_error("Renaming an original media entry requires private tombstone support: " + target.string());
        }
        const auto destination = visible_child(target.parent_path(), name);
        if (fs::exists(fs::symlink_status(destination)) && !fs::equivalent(target, destination)) return fail(-48);
        data_forks.make_durable(target.parent_path());
        const auto error = native_rename_object(target, name, encoding);
        if (error) return fail(error);
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
    // The context hashes the screen's whole control table, and a working game
    // polls thousands of times a second. Reuse it for 50 ms while the same
    // screen stays active and no input has reached the game since; delivering
    // a command still checks it against a fresh one.
    std::optional<std::pair<CommandScreen, CommandContext>> command_cache;
    std::chrono::steady_clock::time_point command_cache_at{};
    // The original main loop calls WaitNextEvent with no sleep about 10,000
    // times a second, even on an idle screen: one core at 100%. Once an
    // interactive screen has done little work between polls for 250 ms, a
    // zero-sleep poll waits up to 8 ms for input instead (input returns at
    // once). Polls while the game works (not interactive, or heavy work
    // between polls) are never delayed, so processing speed is unaffected.
    std::uint64_t polled_at = 0;
    std::chrono::steady_clock::time_point quiet_since{};
    std::string command_status = "Ready";
    bool next_event(State& s, std::uint16_t mask, std::uint32_t address, std::uint32_t sleep, std::uint32_t mouse_region) {
        // An isolated engine has no original game window or input source.
        // Original progress polling still receives a valid null EventRecord,
        // with the live system tick, without entering AppKit's event loop.
        if (event_mode == EventMode::isolated) {
            s.memory.clear(address, 16);
            s.memory.write<4>(address + 6, native_tick_count());
            return false;
        }
        // A native command is an edge, not a held action. At the next original
        // event boundary its handler has consumed the input (or entered a
        // nested dialog). Withdraw only the mailbox value owned by this event.
        if (delivered_command_action) {
            const auto [root, action] = *delivered_command_action;
            if (s.memory.read<2>(root + 0x25819e) == action) s.memory.write<2>(root + 0x25819e, 0);
            delivered_command_action.reset();
        }
        if (const auto screen = original_command_screen(s, mask); !screen) commands = {};
        else if (command_cache && command_cache->first == *screen &&
                 std::chrono::steady_clock::now() - command_cache_at < std::chrono::milliseconds(50)) commands = command_cache->second;
        else {
            commands = original_command_context(s, *screen);
            command_cache.emplace(*screen, commands);
            command_cache_at = std::chrono::steady_clock::now();
        }
        // The writer pumps native events while serializing. Do not dispatch
        // another host command against its transient progress screen.
        if (s.platform && s.platform->quick_save_busy()) {
            commands = {};
            command_cache.reset();
        }
        struct AutomationWait {
            Automation* automation; const State& s; std::uint64_t& polled_at;
            AutomationWait(Automation* a, const State& state, std::uint64_t& polled) : automation(a), s(state), polled_at(polled) {
                if (automation) automation->enter(s.executed);
            }
            ~AutomationWait() { if (automation) automation->leave(s.executed); polled_at = s.executed; }
        } automation_wait(automation.get(), s, polled_at);
        const auto poll_start = std::chrono::steady_clock::now();
        if (!commands.ready || s.executed - polled_at > 200000) quiet_since = poll_start;
        const bool quiet = commands.ready && poll_start - quiet_since > std::chrono::milliseconds(250);
        if (commands.ready) {
            if (commands.handler == 0x100ee9d0) command_status = "Original file dialog: awaiting interaction";
            else if (commands.handler == 0x1033f840) command_status = "Original options: awaiting interaction";
            else if (commands.handler == 0x100f0ba0) command_status = "Original confirmation: awaiting interaction";
            else if (command_status.starts_with("Dispatched") || command_status.starts_with("Original"))
                command_status = "Original game returned to an interactive screen";

        }
        user_interface().update_commands(commands, commands.ready ? command_status : "Unavailable while processing",
                                         s.platform && s.platform->quick_save_available());
        if (automation) automation->boundary(s, commands, commands.ready ? command_status : "Unavailable while processing");
        auto matches = [&](const UIEvent& event) {
            // The writer may pump events while the staged output is open.
            // Let its dialogs receive ordinary input, but do not start a new
            // career or quit until the current save has finished.
            if (s.platform && s.platform->quick_save_busy() &&
                (event.kind == UIEvent::Kind::quit || event.kind == UIEvent::Kind::open_files ||
                 event.kind == UIEvent::Kind::print_files || event.kind == UIEvent::Kind::launch)) return false;
            return (legacy_event_mask(event) & mask) != 0;
        };
        const auto deadline = poll_start + std::chrono::duration<double>(sleep ? sleep / 60.0 : quiet ? 0.008 : 0.0);
        auto pointer = pointer_now();
        auto mouse_outside = [&] {
            if (!mouse_region || !(mask & 0x8000)) return false;
            const auto& region = handles.at(mouse_region);
            return !region_contains(s.memory.copy_out(region.payload, region.size), pointer.x, pointer.y);
        };
        const auto request_quick_save = [&] {
            const bool accepted = s.platform && s.platform->quick_save_request(s);
            if (!accepted) user_interface().beep();
            return accepted;
        };
        const auto queued_save_status = [&]() -> const char* {
            try {
                if (s.memory.read<1>(game_toc - 0x356f))
                    return "Quick save queued until a safe screen";
            } catch (const std::runtime_error&) {}
            return "Quick save queued";
        };
        auto found = std::find_if(pending_events.begin(), pending_events.end(), matches);
        while (found == pending_events.end()) {
            sound.poll(s);
            auto timeout = std::max(0.0, std::chrono::duration<double>(deadline - std::chrono::steady_clock::now()).count());
            // The original passes a region it never moves, so a pointer outside
            // it reports movement on every poll; on a quiet screen those reports
            // wait for the poll's few milliseconds like any other.
            if (mouse_region) timeout = mouse_outside() && !quiet ? 0 : std::min(timeout, 1.0 / 60);
            // Native audio completion must reach the original game while its
            // event loop is sleeping, without calling game code on audio threads.
            if (sound.has_pending_commands()) timeout = std::min(timeout, 1.0 / 60);
            if (automation && commands.ready) timeout = std::min(timeout, 0.02);
            UIEvent event{UIEvent::Kind::update};
            // The original busy loops call WaitNextEvent with no sleep, thousands
            // of times a second while they work: throttle those AppKit pumps.
            auto available = user_interface().poll(event, timeout, sleep == 0);
            std::optional<AutomationRequest> request;
            if (!available && automation) {
                request = automation->take(commands);
                // Idleness and refused requests change within one event wait.
                if (!request && (commands.ready || automation->dirty())) automation->boundary(s, commands, command_status);
            }
            if (request) {
                using Kind = AutomationRequest::Kind;
                if (request->input()) commands = original_command_context(s, mask);
                if (request->kind == Kind::command) {
                    event = UIEvent{UIEvent::Kind::command};
                    event.message = std::uint32_t(request->command);
                    event.command_context = commands.identity;
                    available = true;
                } else if (request->kind == Kind::control) {
                    const bool delivered = deliver_original_control(s, commands, request->control, request->popup);
                    automation->finish(*request, delivered, delivered ? "" : request->popup ? "not an item of a closed pop-up" : "control unavailable");
                    if (delivered) {
                        command_status = "Dispatched to the original game";
                        user_interface().update_commands({}, command_status);
                        automation->boundary(s, {}, command_status);
                        s.memory.clear(address, 16);
                        return false;
                    }
                    automation->boundary(s, commands, command_status);
                    continue;
                } else if (request->kind == Kind::drag) {
                    const auto from = original_control_rect(s, commands, request->control);
                    const auto to = original_control_rect(s, commands, request->target);
                    const auto [game_window, content] = game_screen();
                    if (!from || !to || !game_window || drag) {
                        automation->finish(*request, false, drag ? "a drag is in progress" : "control or window unavailable");
                        automation->boundary(s, commands, command_status);
                        continue;
                    }
                    const auto centre = [&](const std::array<std::int32_t, 4>& r, int axis) {
                        return std::int16_t(content[axis == 0 ? 1 : 0] + (r[axis] + r[axis + 2]) / 2);
                    };
                    drag = SyntheticDrag{centre(*from, 0), centre(*from, 1), centre(*to, 0), centre(*to, 1), *request, std::nullopt, false};
                    // As a left click in the game window arrives: its Carbon
                    // handlers first, then the event queue.
                    UIEvent down{UIEvent::Kind::mouse_down, game_window};
                    down.x = drag->from_x; down.y = drag->from_y; down.modifiers = 0x01; down.button = 1;
                    if (!window_event(s, down)) pending_events.push_back(std::move(down));
                } else if (request->kind == Kind::dialog) {
                    automation->finish(*request, false, "no dialog is open");
                    automation->boundary(s, commands, command_status);
                } else if (request->kind == Kind::activate || request->kind == Kind::print) {
                    // As AppKit queues them: windowDidBecomeKey's activate
                    // event for the game window, or the delegate's print-files.
                    UIEvent queued{request->kind == Kind::activate ? UIEvent::Kind::activate : UIEvent::Kind::print_files};
                    std::string refused;
                    if (request->kind == Kind::activate) {
                        queued.window = game_screen().first;
                        queued.modifiers = 1;  // activeFlag
                        if (!queued.window) refused = "no game window";
                    } else {
                        queued.files.push_back(fs::path(request->name).lexically_normal());
                        if (!reachable_document(queued.files.front())) refused = "not a file in the game's folders";
                    }
                    if (refused.empty()) pending_events.push_back(std::move(queued));
                    automation->finish(*request, refused.empty(), refused.empty() ? "queued" : refused);
                    automation->boundary(s, commands, command_status);
                } else if (request->kind == Kind::menu || request->kind == Kind::quit) {
                    // As AppKit queues them: a menu choice runs the original's
                    // menu-bar path (FindWindow, menu preparation, MenuSelect).
                    UIEvent queued{request->kind == Kind::menu ? UIEvent::Kind::menu : UIEvent::Kind::quit};
                    queued.message = request->menu;
                    pending_events.push_back(std::move(queued));
                    automation->finish(*request, true, request->kind == Kind::menu ? "menu choice queued" : "quit queued");
                    automation->boundary(s, commands, command_status);
                } else if (request->kind == Kind::wheel) {
                    // Through the game window's Carbon handler, as a scroll wheel.
                    UIEvent wheel{UIEvent::Kind::wheel};
                    wheel.window = game_screen().first;
                    wheel.wheel_delta = request->wheel;
                    // The original installs its wheel handler only with the Aqua
                    // menu layout; it notes the delta and returns eventNotHandledErr,
                    // so whether a handler exists decides delivery.
                    const bool delivered = wheel.window && std::any_of(window_handlers.begin(), window_handlers.end(),
                        [&](const auto& entry) {
                            const auto& handler = entry.second;
                            return handler.target == wheel.window &&
                                   std::find(handler.types.begin(), handler.types.end(),
                                             std::pair<std::uint32_t, std::uint32_t>{0x6d6f7573, 10}) != handler.types.end();
                        });
                    if (delivered) window_event(s, wheel);
                    automation->finish(*request, delivered, delivered ? "wheel delivered" : "the game has no wheel handler");
                    automation->boundary(s, commands, command_status);
                } else if (request->kind == Kind::key || request->kind == Kind::text) {
                    // Classic key events, as AppKit would queue them for the
                    // active game window: button up (0x80) and active (1).
                    for (const auto& [message, modifiers] : request->keys)
                        for (const auto kind : {UIEvent::Kind::key_down, UIEvent::Kind::key_up}) {
                            UIEvent key{kind};
                            key.message = message;
                            key.modifiers = std::uint16_t(modifiers | 0x81);
                            pending_events.push_back(std::move(key));
                        }
                    automation->finish(*request, true, std::to_string(request->keys.size()) + " keys queued");
                    automation->boundary(s, commands, command_status);
                } else {
                    observe(s, *request);
                    automation->boundary(s, commands, command_status);
                    continue;
                }
            }
            const auto report = [&](bool accepted, std::string_view result) {
                if (!request) return;
                automation->finish(*request, accepted, result);
                automation->boundary(s, accepted ? CommandContext{} : commands, command_status);
            };
            if (available && event.kind == UIEvent::Kind::quick_save) {
                const bool accepted = request_quick_save();
                command_status = accepted ? queued_save_status() : "Quick save unavailable";
                user_interface().update_commands(commands, command_status, false);
                s.memory.clear(address, 16);
                return false;
            }
            if (available && (event.kind == UIEvent::Kind::command || event.kind == UIEvent::Kind::menu)) {
                if (!commands.ready || (event.kind == UIEvent::Kind::command && event.command_context != commands.identity)) {
                    command_status = "Refused: the game context changed";
                    user_interface().update_commands({}, command_status);
                    report(false, command_status);
                    continue;
                }
                if (event.kind == UIEvent::Kind::command) {
                    if (event.message >= direct_command_count) { report(false, "unknown command"); continue; }
                    const auto command = DirectCommand(event.message);
                    std::fprintf(stderr, "Native command: %s action=%d control=%d context=%016llx\n",
                        std::string(direct_command_names[std::size_t(command)]).c_str(), commands.actions[std::size_t(command)],
                        commands.controls[std::size_t(command)], static_cast<unsigned long long>(commands.identity));
                    if (command == DirectCommand::save_game && commands.actions[std::size_t(command)]) {
                        event.kind = UIEvent::Kind::menu;
                        event.message = 0x00020001;
                        command_status = "Save dispatched to original File menu";
                        report(true, command_status);
                        // Disable duplicates, retaining the original File item's
                        // own preparation/validation until MenuSelect returns.
                    } else {
                        const bool delivered = deliver_original_command(s, commands, command);
                        command_status = delivered ? "Dispatched to the original game" : "Refused: action unavailable";
                        user_interface().update_commands({}, command_status);
                        report(delivered, command_status);
                        s.memory.clear(address, 16);
                        return false;
                    }
                }
            }
            if (available) {
                const auto handled = window_event(s, event);
                if (!handled && event.kind != UIEvent::Kind::wheel) pending_events.push_back(std::move(event));
            }
            pointer = pointer_now();
            found = std::find_if(pending_events.begin(), pending_events.end(), matches);
            if (!available && ((mouse_outside() && !quiet) || std::chrono::steady_clock::now() >= deadline)) break;
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
        // Automation supplies the original key events rather than AppKit menu
        // equivalents. Give the same gesture the same one-shot host behavior.
        if (found->kind == UIEvent::Kind::key_down && (found->modifiers & 0x0100) &&
            (found->message & 0xff) == 's') {
            const auto message = found->message;
            pending_events.erase(found);
            const auto released = std::find_if(pending_events.begin(), pending_events.end(), [&](const auto& event) {
                return event.kind == UIEvent::Kind::key_up && event.message == message;
            });
            if (released != pending_events.end()) pending_events.erase(released);
            const bool accepted = request_quick_save();
            command_status = accepted ? queued_save_status() : "Quick save unavailable";
            command_cache.reset();
            return false;
        }
        if (deliver_match_shortcut(s, commands, *found)) {
            const auto action = std::uint16_t(s.memory.read<2>(commands.root + 0x25819e));
            if (action) delivered_command_action = {commands.root, action};
            pending_events.erase(found);
            command_cache.reset();
            return false;
        }
        if (found->kind == UIEvent::Kind::open_files || found->kind == UIEvent::Kind::print_files) {
            // Only documents the game may reach become an open-documents
            // event; with none left, a launch still opens the application.
            std::erase_if(found->files, [&](const fs::path& file) {
                if (reachable_document(file)) return false;
                std::fprintf(stderr, "Document not opened (not a file in the game's folders): %s\n", file.c_str());
                return true;
            });
            if (found->files.empty()) {
                if (application_opened || found->kind == UIEvent::Kind::print_files) { pending_events.erase(found); return false; }
                found->kind = UIEvent::Kind::launch;
            }
        }
        if (found->kind == UIEvent::Kind::menu) {
            // AppKit has already selected by identity. Let the original event
            // loop run FindWindow -> menu preparation -> MenuSelect -> handler.
            // The reserved Point is a bridge token, never a physical click.
            menu_selection = found->message;
            s.memory.write<2>(address, 1);
            s.memory.write<4>(address + 10, 0x80008000);
        } else if (found->kind == UIEvent::Kind::launch || found->kind == UIEvent::Kind::quit ||
                   found->kind == UIEvent::Kind::open_files || found->kind == UIEvent::Kind::print_files) {
            if (found->kind == UIEvent::Kind::quit && commands.root)
                delivered_command_action = {commands.root, 1026};
            if (found->kind == UIEvent::Kind::launch || found->kind == UIEvent::Kind::open_files) application_opened = true;
            const auto id = found->kind == UIEvent::Kind::launch ? 0x6f617070u   // 'oapp'
                : found->kind == UIEvent::Kind::open_files ? 0x6f646f63u          // 'odoc'
                : found->kind == UIEvent::Kind::print_files ? 0x70646f63u         // 'pdoc'
                : 0x71756974u;                                                   // 'quit'
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
            if (what == 1 && drag && !drag->start) drag->start = std::chrono::steady_clock::now();
            if (what == 2 && drag && drag->released) end_drag(true);
        }
        pending_events.erase(found);
        command_cache.reset(); // input may change the screen's controls
        return true;
    }
    std::uint32_t process_event(State& s, std::uint32_t address) {
        const auto found = delivered_events.find(event_record(s, address));
        if (found == delivered_events.end()) return std::uint32_t(-1708);
        const auto id = std::uint32_t(s.memory.read<4>(address + 10));
        const auto handler = apple_handlers.find({0x61657674, id});
        if (handler == apple_handlers.end()) { delivered_events.erase(found); return std::uint32_t(-1708); }
        NativeAEDescriptor descriptor;
        std::int32_t error = 0;
        if (found->second.kind == UIEvent::Kind::open_files || found->second.kind == UIEvent::Kind::print_files) {
            // Each document as the FSSpec the guest's file calls resolve.
            std::vector<std::vector<std::uint8_t>> specs;
            const auto spec = allocate(s, 70, true);
            if (!spec) return std::uint32_t(-108);
            // A name the original cannot hold (over 63 MacRoman bytes) is
            // left out; the handler still runs, so the game still starts.
            for (const auto& file : found->second.files) {
                try {
                    write_spec(s, spec, file);
                    specs.push_back(s.memory.copy_out(spec, 70));
                } catch (const FileError&) {
                    std::fprintf(stderr, "Document not opened (its name has no FSSpec): %s\n", file.c_str());
                }
            }
            deallocate(s, spec);
            error = apple_events.documents_event(id, specs, descriptor);
        } else {
            error = apple_events.application_event(id, descriptor);
        }
        delivered_events.erase(found);  // delivered once, whatever happens next
        if (error) return std::uint32_t(error);
        const auto arguments = allocate(s, 16, true);
        if (!arguments) { apple_events.dispose(descriptor); return std::uint32_t(-108); }
        s.memory.write<4>(arguments, descriptor.type);
        s.memory.write<4>(arguments + 4, descriptor.token);
        s.memory.write<4>(arguments + 8, 0x6e756c6c); // A null reply descriptor.
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
        std::error_code gone;  // A directory removed since it got its ID matches nothing.
        for (const auto& [id, existing] : directories)
            if (logical_path(existing) == logical_path(path) || fs::equivalent(existing, path, gone)) return id;
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
        if (name.find('\0') != std::string::npos) throw FileError{bad_name};
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
            const auto component = host_name(name.substr(start, end - start));
            if (component.empty()) base = base.parent_path();
            else base = visible_child(base, component);
            base = allowed(base);
            start = end + 1;
        }
        return allowed(base);
    }
    std::optional<fs::path> quick_save_target(const std::optional<std::string>& guest) const {
        if (!guest) return std::nullopt;
        try {
            const auto name = convert(*guest, kCFStringEncodingMacRoman, kCFStringEncodingUTF8);
            const auto logical = !name.empty() && name.front() == '/' ? allowed(fs::path(name)) : path(0, 0, name);
            return writable_path(logical);
        } catch (const FileError&) { return std::nullopt; }
        catch (const std::exception&) { return std::nullopt; }
    }
    std::optional<std::string> quick_save_guest_staging_path(const fs::path& staged) const {
        if (!under(staged, writable)) return std::nullopt;
        try {
            std::string result = "CM User Data";
            for (const auto& component : staged.lexically_relative(writable)) {
                const auto part = component.string();
                if (part.empty() || part == "." || part == "..") return std::nullopt;
                result += ':';
                result += convert(hfs_name(part), kCFStringEncodingUTF8, kCFStringEncodingMacRoman);
            }
            // The original autosave dispatcher and writer use 256-byte path buffers.
            if (result.size() >= 256) return std::nullopt;
            return result;
        } catch (const FileError&) { return std::nullopt; }
        catch (const std::exception&) { return std::nullopt; }
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
        const auto name = root ? std::string{} : convert(hfs_name(target.filename().string()), kCFStringEncodingUTF8, kCFStringEncodingMacRoman);
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
            const auto children = mac_entries(directory(ref, parent_id));
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
            s.memory.write<2>(pb + 52, std::min<std::size_t>(mac_entries(target).size(), UINT16_MAX));
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
        if (name && index) write_mac_string(s, name, root ? volume(target) == -1 ? "CM 01-02" : "CM User Data" : hfs_name(target.filename().string()));
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
NativePlatform::NativePlatform(fs::path game, fs::path writable, AudioMode audio)
    : NativePlatform(std::move(game), std::move(writable), audio, EventMode::interactive) {}
NativePlatform::NativePlatform(fs::path game, fs::path writable, AudioMode audio, EventMode events)
    : impl_(std::make_unique<Impl>(audio, events)) {
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

void NativePlatform::debug_attribute_load_begin(State& s) {
    if (impl_->event_mode != EventMode::interactive) return;
    // Tables may be replaced in place while the loader runs.
    s.memory.clear_debug_player_attributes();
    impl_->debug_attribute_loading = true;
    impl_->debug_attribute_load_target = impl_->debug_attribute_target(s, std::uint32_t(s.r<4>(12)));
}
void NativePlatform::debug_attribute_load_end(State& s) {
    if (impl_->event_mode != EventMode::interactive) return;
    if (std::int8_t(s.r<4>(12)) != 0 && impl_->debug_attribute_load_target) {
        try { debug_attribute_persistence::load(s, impl_->writable / "DebugAttributeLocks", *impl_->debug_attribute_load_target); }
        catch (const std::exception& error) {
            s.memory.clear_debug_player_attributes();
            impl_->command_status = "Could not restore player attribute locks";
            std::fprintf(stderr, "Attribute locks: %s\n", error.what());
        }
    }
    impl_->debug_attribute_load_target.reset();
    impl_->debug_attribute_loading = false;
}
bool NativePlatform::debug_attribute_save_may_enter(const State& s) const {
    return impl_->event_mode != EventMode::interactive || !impl_->debug_attribute_saving ||
        (!s.memory.debug_attribute_metadata_required() && !native_debugging_enabled());
}
bool NativePlatform::debug_attribute_save_begin(State& s) {
    if (impl_->event_mode != EventMode::interactive) return true;
    if (!debug_attribute_save_may_enter(s)) return false;
    impl_->debug_attribute_saving = s.r<4>(16) == 0x16;
    impl_->debug_attribute_rename_needed = false;
    impl_->debug_attribute_rename_ok = false;
    if (impl_->debug_attribute_saving)
        impl_->debug_attribute_save_target = impl_->debug_attribute_save_override ? impl_->debug_attribute_save_override :
            impl_->debug_attribute_target(s, std::uint32_t(s.r<4>(12)));
    return true;
}
void NativePlatform::debug_attribute_save_mode(State& s) {
    if (impl_->debug_attribute_saving) impl_->debug_attribute_rename_needed = (s.r<1>(2304) & 2) == 0;
}
void NativePlatform::debug_attribute_save_renamed(State& s) {
    if (impl_->debug_attribute_saving) impl_->debug_attribute_rename_ok = !impl_->debug_attribute_rename_needed || s.r<4>(12) == 0;
}
void NativePlatform::debug_attribute_save_end(State& s) {
    if (impl_->event_mode != EventMode::interactive || !impl_->debug_attribute_saving) return;
    if (std::int8_t(s.r<4>(12)) != 0 && impl_->debug_attribute_rename_ok && s.memory.debug_attribute_metadata_required()) {
        try {
            if (!impl_->debug_attribute_save_target) throw std::runtime_error("Saved career path is unavailable");
            debug_attribute_persistence::save(s, impl_->writable / "DebugAttributeLocks", *impl_->debug_attribute_save_target);
        }
        catch (const std::exception& error) {
            impl_->command_status = "Saved career, but could not save player attribute locks";
            std::fprintf(stderr, "Attribute locks: %s\n", error.what());
        }
    }
    impl_->debug_attribute_save_target.reset();
    impl_->debug_attribute_save_override.reset();
    impl_->debug_attribute_saving = false;
}
void NativePlatform::debug_attribute_save_abort() noexcept {
    impl_->debug_attribute_save_target.reset();
    impl_->debug_attribute_save_override.reset();
    impl_->debug_attribute_saving = false;
    impl_->debug_attribute_rename_needed = false;
    impl_->debug_attribute_rename_ok = false;
}
bool NativePlatform::debug_attribute_change_available() const {
    return !impl_->debug_attribute_saving && !impl_->debug_attribute_loading;
}

namespace {
std::optional<std::string> quick_save_guest_path(const State& s, std::uint32_t address) {
    if (!address) return std::nullopt;
    try {
        std::string path;
        // The original autosave path occupies 256 bytes in the dispatcher frame.
        for (unsigned i = 0; i < 256; ++i) {
            const auto byte = s.memory.copy_out(address + i, 1)[0];
            if (!byte) return path.empty() ? std::nullopt : std::optional<std::string>(std::move(path));
            path.push_back(char(byte));
        }
    } catch (const std::runtime_error&) {}
    return std::nullopt;
}
enum class HumanMatchState { none, playing, finishing };
HumanMatchState human_match_state(const State& s) {
    try {
        // The original keeps sixteen human-manager records across match views,
        // including their nested tactics and confirmation screens. A match the
        // human only watches has no team link at +0xf0, but its engine is just
        // as live: nothing shows the writer can serialize a world around one.
        auto result = HumanMatchState::none;
        for (unsigned i = 0; i < 16; ++i) {
            const auto record = 0x109806f0u + i * 0xfcu;
            const auto bytes = s.memory.copy_out(record + 0xec, 4);
            const auto fixture = std::uint32_t(read_be<4>(bytes.data()));
            if (!fixture) continue;
            const auto engine = s.memory.copy_out(fixture + 0x58, 4);
            const auto phase = s.memory.copy_out(fixture + 0x68, 1)[0];
            if (read_be<4>(engine.data())) {
                if (phase != 2 && phase != 8) return HumanMatchState::playing;
                result = HumanMatchState::finishing;
            }
        }
        return result;
    } catch (const std::runtime_error&) { return HumanMatchState::playing; }
}
// The conditions under which the original's save dispatcher (0x102a54b0) gets
// as far as saving, for the routes that do not pass through it. Its screen-count
// test at 0x10744020 is left out on purpose: saving from a sub-screen is what
// Command-S is for, and the dispatcher hook at 0x102a55b4 skips it as well.
bool original_save_possible(const State& s) {
    try {
        const auto toc = std::uint32_t(s.r<4>(8));
        if (toc < 0x8000) return false;
        const auto u32 = [&](std::uint32_t address) { return std::uint32_t(s.memory.read<4>(address)); };
        const auto root = u32(toc - 0x7bb0);
        if (!root) return false;
        const auto view = root + std::uint32_t(std::int32_t(std::int16_t(s.memory.read<2>(root + 0x2c58))) * 0x2c0);
        if (s.memory.read<2>(view)) return false;  // 0x102a5590: a remote screen
        const auto game = u32(view + 4);
        if (!game || s.memory.read<1>(game + 0x6b) == 5) return false;  // 0x103557e0
        // 0x102aa3d0: the record of the manager this view belongs to.
        const auto index = std::int32_t(u32(game) - u32(u32(toc - 0x7be4))) + 0x10;
        if (index < 0 || index >= 16) return false;
        if (s.memory.read<1>(u32(toc - 0x5158) + std::uint32_t(index) * 0xfc) != 2) return false;  // 0x102a556c
        // The live match view; a watched match need not be in any record.
        const auto descriptor = u32(root + 0x10);
        const auto vector = descriptor ? u32(descriptor + 8) : 0;
        return !vector || u32(vector) != 0x1028dc80;
    } catch (const std::runtime_error&) { return false; }
}
// What passes by itself, so a request can wait for it.
bool original_save_settled(const State& s) {
    try {
        const auto toc = std::uint32_t(s.r<4>(8));
        // An alert from 0x10769b60 can be up in the middle of any mutation.
        if (toc < 0x8000 || s.memory.read<1>(toc - 0x356f)) return false;
        return !s.memory.read<1>(std::uint32_t(s.memory.read<4>(toc - 0x7bb0)) + 0x2c5f);  // 0x102a5560
    } catch (const std::runtime_error&) { return false; }
}
std::optional<std::array<std::uint8_t, 7>> quick_save_markers(const State& s) {
    try {
        const auto toc = std::uint32_t(s.r<4>(8));
        if (toc < 0x8000) return std::nullopt;
        std::array<std::uint8_t, 7> markers{};
        markers[0] = std::uint8_t(s.memory.read<1>(std::uint32_t(s.memory.read<4>(toc - 0x63c0))));
        markers[1] = std::uint8_t(s.memory.read<1>(std::uint32_t(s.memory.read<4>(toc - 0x6434))));
        const auto date = s.memory.copy_out(std::uint32_t(s.memory.read<4>(toc - 0x63c8)), 5);
        std::copy(date.begin(), date.end(), markers.begin() + 2);
        return markers;
    } catch (const std::runtime_error&) { return std::nullopt; }
}
void restore_quick_save_markers(State& s, const std::array<std::uint8_t, 7>& markers) {
    const auto toc = std::uint32_t(s.r<4>(8));
    s.memory.write<1>(std::uint32_t(s.memory.read<4>(toc - 0x63c0)), markers[0]);
    s.memory.write<1>(std::uint32_t(s.memory.read<4>(toc - 0x6434)), markers[1]);
    s.memory.copy_in(std::uint32_t(s.memory.read<4>(toc - 0x63c8)),
                     std::vector<std::uint8_t>(markers.begin() + 2, markers.end()));
}
}
void NativePlatform::set_quick_save_alert_observer(std::function<void(const std::string&)> observer) {
    impl_->quick_save_alert_observer = std::move(observer);
}
void NativePlatform::quick_save_load_begin(State& s) {
    if (impl_->event_mode == EventMode::interactive) {
        // The first load of a session: not the constructor, which check tools
        // and replays also run on folders a game may be using.
        if (!impl_->quick_save_swept && !impl_->quick_save_active) {
            impl_->quick_save_swept = true;
            remove_stale_quick_save_staging(impl_->writable);
        }
        // A request from the previous career must not overwrite that career
        // after the original loader starts replacing the in-memory world.
        if (impl_->quick_save_queued) impl_->command_status = "Quick save canceled: loading another career";
        impl_->quick_save_queued = false;
        impl_->quick_save_loading = true;
        auto path = quick_save_guest_path(s, std::uint32_t(s.r<4>(12)));
        impl_->quick_save_load_target = impl_->quick_save_target(path);
        impl_->quick_save_destination.begin_load(std::move(path));
    }
}
void NativePlatform::quick_save_load_end(State& s) {
    if (impl_->event_mode != EventMode::interactive) return;
    const bool success = std::int8_t(s.r<4>(12)) != 0;
    impl_->quick_save_destination.finish_load(success);
    if (success) impl_->quick_save_last_target = std::move(impl_->quick_save_load_target);
    else {
        if (impl_->quick_save_last_target) impl_->command_status = "Quick save off until the next save or load";
        impl_->quick_save_last_target.reset();
    }
    impl_->quick_save_load_target.reset();
    impl_->quick_save_loading = false;
    if (!success || !impl_->quick_save_destination.last() || !impl_->quick_save_last_target) impl_->quick_save_queued = false;
}
bool NativePlatform::quick_save_write_begin(State& s) {
    if (impl_->event_mode != EventMode::interactive) return true;
    // The original writer pumps events. A nested writer must not replace the
    // active transaction's tracking or write into the destination mid-save.
    if (impl_->quick_save_active && impl_->quick_save_tracking_write) return false;
    impl_->debug_attribute_save_override = impl_->quick_save_active ? impl_->quick_save_last_target : std::nullopt;
    if (impl_->quick_save_active) impl_->quick_save_saved_markers = quick_save_markers(s);
    impl_->quick_save_tracking_write = s.r<4>(16) == 0x16;
    impl_->quick_save_rename_ok = false;
    impl_->quick_save_needs_rename = false;
    if (impl_->quick_save_tracking_write && !impl_->quick_save_active) {
        auto path = quick_save_guest_path(s, std::uint32_t(s.r<4>(12)));
        impl_->quick_save_write_target = impl_->quick_save_target(path);
        impl_->quick_save_destination.begin_save(std::move(path));
    }
    return true;
}
void NativePlatform::quick_save_write_mode(State& s) {
    if (impl_->event_mode == EventMode::interactive && impl_->quick_save_tracking_write)
        impl_->quick_save_needs_rename = (s.r<1>(2304) & 2) == 0;
}
void NativePlatform::quick_save_write_renamed(State& s) {
    if (impl_->event_mode == EventMode::interactive && impl_->quick_save_tracking_write)
        impl_->quick_save_rename_ok = !impl_->quick_save_needs_rename || s.r<4>(12) == 0;
}
void NativePlatform::quick_save_write_end(State& s) {
    if (impl_->event_mode != EventMode::interactive || !impl_->quick_save_tracking_write) return;
    const bool success = std::int8_t(s.r<4>(12)) != 0 && impl_->quick_save_rename_ok;
    if (impl_->quick_save_active) {
        bool promoted = false;
        bool has_previous_save = false;
        bool promotion_started = false;
        bool exchanged = false;  // The destination may already hold the new file.
        bool unconfirmed = false;
        try {
            // The dispatcher also uses this buffer in its failure dialog.
            // Restore the user's path before returning from the staged writer.
            const auto& destination = impl_->quick_save_destination.last();
            if (!destination || !impl_->quick_save_path_address)
                throw std::runtime_error("quick-save destination unavailable at completion");
            std::vector<std::uint8_t> path(destination->begin(), destination->end());
            path.push_back(0);
            s.memory.copy_in(impl_->quick_save_path_address, path);
            if (impl_->quick_save_transaction)
                impl_->data_forks.make_durable(impl_->quick_save_transaction->staging_directory());
            const bool io_ok = impl_->data_forks.finish_write_audit();
            if (success && io_ok && impl_->quick_save_transaction) {
                promotion_started = true;
                const auto result = impl_->quick_save_transaction->promote();
                promoted = result.state == QuickSavePromotionState::promoted;
                has_previous_save = !result.previous_save.empty();
                unconfirmed = result.state == QuickSavePromotionState::promoted_unconfirmed;
                exchanged = unconfirmed;
                if (!promoted) {
                    std::fprintf(stderr, "Quick save promotion: %s: %s; retained: %s\n",
                                 result.error.operation.c_str(), result.error.code.message().c_str(),
                                 result.error.retained_directory.c_str());
                } else if (!result.previous_save.empty()) {
                    std::fprintf(stderr, "Quick save retained previous file: %s\n", result.previous_save.c_str());
                }
            }
        } catch (const std::exception& error) {
            // An exception during promotion can follow the atomic exchange.
            // Preserve the staging directory and do not report success.
            std::fprintf(stderr, "Quick save completion error: %s\n", error.what());
            exchanged = promotion_started;
            impl_->data_forks.abort_write_audit();
        }
        using Outcome = Impl::QuickSaveOutcome;
        impl_->quick_save_outcome = promoted ? Outcome::complete : exchanged ? Outcome::uncertain : Outcome::failed;
        impl_->command_status = promoted ? "Quick save complete" : exchanged ?
            "Quick save uncertain; previous save retained" : "Quick save failed; previous save unchanged";
        if (!promoted) {
            // The writer's successful path has already told the game it is
            // saved, even after a failed rename. It is not, or not surely: keep
            // the save prompt and the autosave clock as they were.
            if (impl_->quick_save_saved_markers)
                restore_quick_save_markers(s, *impl_->quick_save_saved_markers);
            // The exchange happened, so the new career is probably in place,
            // and its attribute locks must go with it.
            if (unconfirmed) { s.wr<4>(12, 1); debug_attribute_save_end(s); }
            s.wr<4>(12, 0);
            if (exchanged && impl_->quick_save_transaction) {
                // Both versions must survive later cleanups.
                const auto kept = impl_->quick_save_transaction->keep_for_recovery();
                if (!kept) std::fprintf(stderr, "Quick save recovery mark: %s: %s\n",
                                        kept.error().operation.c_str(), kept.error().code.message().c_str());
            } else if (!promotion_started && impl_->quick_save_transaction) {
                // Unlike a failed promotion, which leaves a complete new save
                // there, this output is of no use to anyone.
                const auto discarded = impl_->quick_save_transaction->discard_failed_staging();
                if (!discarded) std::fprintf(stderr, "Quick save staging retained: %s: %s\n",
                                             discarded.error().operation.c_str(), discarded.error().code.message().c_str());
            }
        }
        impl_->quick_save_saved_markers.reset();
        if (promoted && impl_->quick_save_transaction) {
            if (has_previous_save) {
                // Keep one recovery copy until the next quick-save request.
                impl_->quick_save_previous_transaction = std::move(impl_->quick_save_transaction);
            } else {
                const auto removed = impl_->quick_save_transaction->remove_empty_staging_directory();
                if (!removed) std::fprintf(stderr, "Quick save staging retained: %s: %s\n",
                                           removed.error().operation.c_str(), removed.error().code.message().c_str());
            }
        }
        impl_->quick_save_transaction.reset();
        impl_->quick_save_path_address = 0;
        impl_->quick_save_quiet = false;
        impl_->quick_save_active = false;
    } else {
        impl_->quick_save_destination.finish_save(success);
        if (success) impl_->quick_save_last_target = std::move(impl_->quick_save_write_target);
        impl_->quick_save_write_target.reset();
    }
    impl_->quick_save_tracking_write = false;
    if (!impl_->quick_save_destination.last() || !impl_->quick_save_last_target) impl_->quick_save_queued = false;
}
bool NativePlatform::quick_save_busy() const { return impl_->quick_save_active; }
bool NativePlatform::quick_save_quiet_progress() const { return impl_->quick_save_active && impl_->quick_save_quiet; }
bool NativePlatform::quick_save_available() const {
    return impl_->event_mode == EventMode::interactive && impl_->quick_save_destination.last().has_value() &&
        impl_->quick_save_last_target.has_value() && !impl_->quick_save_queued && !impl_->quick_save_active;
}
void NativePlatform::quick_save_note_boundary(const State& s) { impl_->quick_save_possible = original_save_possible(s); }
bool NativePlatform::quick_save_request(State& s) {
    if (!quick_save_available() || human_match_state(s) == HumanMatchState::playing ||
        !impl_->quick_save_possible) return false;
    impl_->quick_save_queued = true;
    return true;
}
bool NativePlatform::quick_save_pending(State& s) {
    if (!impl_->quick_save_queued || impl_->quick_save_loading) return false;
    const auto match = human_match_state(s);
    if (match == HumanMatchState::playing) {
        impl_->quick_save_queued = false;
        impl_->command_status = "Quick save canceled: match started";
    }
    return impl_->quick_save_queued && match == HumanMatchState::none && !impl_->quick_save_tracking_write &&
        impl_->quick_save_possible && original_save_settled(s);
}
bool NativePlatform::quick_save_prepare_path(State& s) {
    return quick_save_prepare_path_at(s, std::uint32_t(s.r<4>(4)) + 0x133c);
}
namespace {
constexpr const char* quick_save_interrupted_message =
    "The save was interrupted. Your previous save was not changed. Save again before quitting.";
}
void NativePlatform::quick_save_abandon() noexcept {
    if (!impl_->quick_save_active) return;
    if (impl_->debug_attribute_save_override) debug_attribute_save_abort();
    impl_->data_forks.abort_write_audit();
    try {
        if (impl_->quick_save_transaction) (void)impl_->quick_save_transaction->discard_failed_staging();
    } catch (...) {}
    impl_->quick_save_transaction.reset();
    impl_->quick_save_saved_markers.reset();
    impl_->quick_save_outcome = Impl::QuickSaveOutcome::failed;
    impl_->quick_save_path_address = 0;
    impl_->quick_save_quiet = false;
    impl_->quick_save_active = false;
    impl_->quick_save_tracking_write = false;
    impl_->command_cache.reset();
    try { impl_->command_status = "Quick save interrupted; previous save unchanged"; } catch (...) {}
}
bool NativePlatform::quick_save_call_dispatcher_writer(State& s) {
    // A nested dispatcher must take the ordinary call path, where the writer
    // entry guard refuses it without abandoning the outer transaction.
    if (!impl_->quick_save_active || impl_->quick_save_tracking_write) return false;
    constexpr std::uint32_t continuation = 0x102a5694;
    // Reproduce the dispatcher's original bl and give this invocation its
    // own host boundary, so an escaping guest transfer can release the audit.
    s.wr<4>(2440, s.r<4>(8));
    s.wr<4>(4128, continuation);
    s.pc = 0x1034ba30;
    try { run_quick_save_until(s, continuation); }
    catch (...) {
        quick_save_abandon();
        impl_->quick_save_alert(quick_save_interrupted_message);
        throw;
    }
    // A transfer may reach the continuation without the writer's epilogue, so
    // the writer never reported anything: answer as a failed save, so that the
    // dispatcher's own failure handling tells the player about the real path.
    if (impl_->quick_save_active) {
        try {
            const auto& destination = impl_->quick_save_destination.last();
            if (destination && impl_->quick_save_path_address) {
                std::vector<std::uint8_t> path(destination->begin(), destination->end());
                path.push_back(0);
                s.memory.copy_in(impl_->quick_save_path_address, path);
            }
        } catch (const std::runtime_error&) {}
        quick_save_abandon();
        s.wr<4>(12, 0);
    }
    return true;
}
bool NativePlatform::quick_save_prepare_path_at(State& s, std::uint32_t path_address, bool quiet_progress) {
    if (!quick_save_pending(s)) return true;
    const auto fail = [&](const char* reason) {
        std::fprintf(stderr, "Quick save preparation failed: %s\n", reason);
        impl_->command_status = "Quick save failed before writing";
        impl_->quick_save_queued = false;
        impl_->quick_save_alert("The save could not start, so nothing was written. Your previous save was not changed. "
                                "Save again before quitting.");
        return false;
    };
    if (!impl_->quick_save_last_target) return fail("no resolved destination");
    if (impl_->quick_save_previous_transaction) {
        std::error_code error;
        const bool same_target = fs::equivalent(*impl_->quick_save_last_target,
            impl_->quick_save_previous_transaction->destination_path(), error);
        if (!error && same_target) {
            const auto discarded = impl_->quick_save_previous_transaction->discard_previous_save();
            if (discarded) {
                const auto removed = impl_->quick_save_previous_transaction->remove_empty_staging_directory();
                if (!removed) std::fprintf(stderr, "Quick save staging retained: %s: %s\n",
                                           removed.error().operation.c_str(), removed.error().code.message().c_str());
            } else {
                std::fprintf(stderr, "Quick save previous file retained: %s: %s\n",
                             discarded.error().operation.c_str(), discarded.error().code.message().c_str());
            }
        }
        impl_->quick_save_previous_transaction.reset();
    }
    remove_stale_quick_save_staging(impl_->writable);
    remove_stale_quick_save_staging(impl_->quick_save_last_target->parent_path());
    auto transaction = QuickSaveTransaction::create(*impl_->quick_save_last_target, impl_->writable);
    if (!transaction) {
        std::fprintf(stderr, "Quick save staging: %s: %s\n", transaction.error().operation.c_str(),
                     transaction.error().code.message().c_str());
        return fail("cannot create private staging");
    }
    const auto path = impl_->quick_save_guest_staging_path((*transaction)->staged_path());
    if (!path) {
        (void)(*transaction)->discard_failed_staging();
        return fail("staging path cannot be represented by the original game");
    }
    std::vector<std::uint8_t> bytes(path->begin(), path->end());
    bytes.push_back(0);
    if (!impl_->data_forks.begin_write_audit((*transaction)->staging_directory())) {
        (void)(*transaction)->discard_failed_staging();
        return fail("cannot start staged-write audit");
    }
    try { s.memory.copy_in(path_address, bytes); }
    catch (...) { impl_->data_forks.abort_write_audit(); throw; }
    impl_->quick_save_transaction = std::move(*transaction);
    impl_->quick_save_path_address = path_address;
    impl_->quick_save_quiet = quiet_progress;
    impl_->quick_save_queued = false;
    impl_->quick_save_active = true;
    impl_->quick_save_outcome = Impl::QuickSaveOutcome::none;
    return true;
}
void NativePlatform::quick_save_at_screen_boundary(State& s) {
    quick_save_note_boundary(s);
    if (!quick_save_pending(s)) return;
    struct Saved {
        decltype(State::registers) registers;
        decltype(State::unique) unique;
        std::uint32_t pc;
    };
    auto saved = std::make_unique<Saved>(Saved{s.registers, s.unique, s.pc});
    const auto previous_stack = std::uint32_t(s.r<4>(4));
    if (previous_stack < 0x150) throw std::runtime_error("Invalid quick-save callback stack");
    const auto stack = (previous_stack - 0x140) & ~15u;
    const auto scratch = s.memory.copy_out(stack, previous_stack - stack);
    s.memory.clear(stack, previous_stack - stack);
    s.memory.write<4>(stack, previous_stack);
    const auto buffer = stack + 0x40;
    try {
        if (!quick_save_prepare_path_at(s, buffer, true)) {
            s.memory.copy_in(stack, scratch);
            return;
        }
    } catch (...) { s.memory.copy_in(stack, scratch); throw; }
    s.wr<4>(4, stack);
    s.wr<4>(12, buffer);
    s.wr<4>(16, 0x16);
    s.wr<4>(4128, return_sentinel);
    s.pc = 0x1034ba30;
    auto* const caller_dispatch = s.dispatch_scope;
    try { run_quick_save_until(s, return_sentinel); }
    catch (...) {
        quick_save_abandon();
        impl_->quick_save_alert(quick_save_interrupted_message);
        throw;
    }
    // A context transfer can reach the continuation without the writer's
    // epilogue. Preserve that guest continuation rather than restoring the
    // abandoned screen callback's saved registers.
    if (impl_->quick_save_active) {
        quick_save_abandon();
        impl_->quick_save_alert(quick_save_interrupted_message);
        if (caller_dispatch) throw GuestContextTransfer{&s, caller_dispatch};
        throw std::runtime_error("Quick-save writer abandoned without a dispatch owner");
    }
    // No dispatcher will look at the result on this route, so do its part:
    // clear the writer's disk flags (0x102a56c4, 0x102a56f0) and tell the player.
    using Outcome = Impl::QuickSaveOutcome;
    if (impl_->quick_save_outcome != Outcome::complete) {
        bool disk_full = false;
        try {
            const auto toc = std::uint32_t(s.r<4>(8));
            for (const auto slot : {0x63ccu, 0x63d0u}) {
                const auto flag = std::uint32_t(s.memory.read<4>(toc - slot));
                if (s.memory.read<1>(flag)) disk_full = true;
                s.memory.write<1>(flag, 0);
            }
        } catch (const std::runtime_error&) {}
        impl_->quick_save_alert(
            impl_->quick_save_outcome == Outcome::uncertain ?
                "The save may have replaced your career file, but this could not be confirmed. "
                "The previous version was kept. Save again before quitting." :
            disk_full ? "There is not enough disk space. Your previous save was not changed. "
                        "Free some space and save again before quitting." :
                "The save did not complete. Your previous save was not changed. Save again before quitting.");
    }
    s.memory.copy_in(stack, scratch);
    s.registers = saved->registers;
    s.unique = saved->unique;
    s.pc = saved->pc;
}
std::vector<fs::path> NativePlatform::save_window_images(const fs::path& directory) const {
    return impl_->ui ? impl_->ui->save_window_images(directory) : std::vector<fs::path>{};
}
bool NativePlatform::dispatch(State& s, std::string_view name) {
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
    const auto failed_mutation = [&] {
        if (name == "CarbonLib::FSpCreate" || name == "CarbonLib::PBHCreateSync" ||
            name == "CarbonLib::FSCreateFileUnicode" || name == "CarbonLib::FSDeleteObject" ||
            name == "CarbonLib::FSRenameUnicode" || name == "CarbonLib::PBHDeleteSync" ||
            name == "CarbonLib::PBHRenameSync" ||
            (name == "CarbonLib::FSOpenFork" && std::int8_t(arg(3)) != 1) ||
            (name == "CarbonLib::FSpOpenDF" && std::int8_t(arg(1)) != 1)) return true;
        if (name != "CarbonLib::PBHOpenDFSync") return false;
        try { return std::int8_t(s.memory.read<1>(arg(0) + 27)) != 1; }
        catch (const std::runtime_error&) { return true; }
    };
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
        } else if (name == "CarbonLib::Gestalt" && arg(0) == 0x74747363) {
            // gestaltSpeechMgrPresent (bit 0) only when CM_AOT_SPEECH asks for speech.
            s.memory.write<4>(arg(1), impl_->speech.available() ? 1 : 0);
        } else if (name == "CarbonLib::NewSpeechChannel") {
            if (arg(0)) throw std::runtime_error("A speech voice chosen by the original is unsupported");
            std::uint32_t token = 0;
            status = impl_->speech.new_channel(token);
            s.memory.write<4>(arg(1), status ? 0 : token);
        } else if (name == "CarbonLib::SpeakText") {
            const auto bytes = s.memory.copy_out(arg(1), arg(2));
            status = impl_->speech.speak(arg(0), {bytes.begin(), bytes.end()});
        } else if (name == "CarbonLib::SpeechBusy") {
            return_value = std::uint32_t(std::int32_t(impl_->speech.busy()));
        } else if (name == "CarbonLib::DisposeSpeechChannel") {
            status = impl_->speech.dispose(arg(0));
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
        } else if (name == "CarbonLib::GetNewDialog") {
            if (arg(1)) throw std::runtime_error("Dialog storage supplied by the original is unsupported");
            return_value = impl_->new_dialog(s, std::int16_t(arg(0)));
        } else if (name == "CarbonLib::GetDialogWindow") {
            return_value = arg(0);  // the dialog is its own window
        } else if (name == "CarbonLib::ModalDialog") {
            if (arg(0)) throw std::runtime_error("ModalDialog filter procedures are unsupported");
            s.memory.write<2>(arg(1), std::uint16_t(impl_->modal_dialog(s)));
        } else if (name == "CarbonLib::DisposeDialog") {
            if (!impl_->user_interface().is_dialog(arg(0))) throw std::runtime_error("DisposeDialog of something not a dialog");
            impl_->user_interface().dispose_dialog(arg(0));
            if (impl_->front_dialog == arg(0)) impl_->front_dialog = 0;
            deallocate(s, arg(0));
        } else if (name == "CarbonLib::ShowWindow" && impl_->user_interface().is_dialog(arg(0))) {
            impl_->user_interface().show_dialog(arg(0));
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
        } else if (name == "CarbonLib::PMSessionDefaultPrintSettings") {
            return_value = std::uint32_t(impl_->printing.default_print_settings(arg(0), arg(1)));
        } else if (name == "CarbonLib::PMSessionPrintDialog") {
            bool accepted = false;
            return_value = std::uint32_t(impl_->printing.print_dialog(arg(0), arg(1), arg(2), accepted));
            if (!*return_value) s.memory.write<1>(arg(3), accepted);
        } else if (name == "CarbonLib::PMSessionPageSetupDialog") {
            bool accepted = false;
            return_value = std::uint32_t(impl_->printing.page_setup_dialog(arg(0), arg(1), accepted));
            if (!*return_value) s.memory.write<1>(arg(2), accepted);
        } else if (name == "CarbonLib::PMSessionBeginDocument") {
            return_value = std::uint32_t(impl_->printing.begin_document(arg(0), arg(1), arg(2)));
        } else if (name == "CarbonLib::PMSessionEndDocument") {
            return_value = std::uint32_t(impl_->printing.end_document(arg(0)));
        } else if (name == "CarbonLib::PMSessionBeginPage") {
            return_value = std::uint32_t(impl_->printing.begin_page(arg(0), arg(1)));
        } else if (name == "CarbonLib::PMSessionEndPage") {
            return_value = std::uint32_t(impl_->printing.end_page(arg(0)));
        } else if (name == "CarbonLib::FMGetFontFamilyFromName") {
            return_value = std::uint32_t(std::int32_t(impl_->printing.font_family(
                s.memory.copy_out(arg(0) + 1, s.memory.read<1>(arg(0))))));
        } else if (name == "CarbonLib::TextFont") {
            impl_->printing.set_font(std::int16_t(arg(0)));
            return_value = arg(0);
        } else if (name == "CarbonLib::SetFractEnable") {
            // Printed text is laid out with CoreText's fractional widths.
            return_value = arg(0);
        } else if (name == "CarbonLib::TextSize") {
            impl_->printing.set_font_size(std::int16_t(arg(0)));
            return_value = arg(0);
        } else if (name == "CarbonLib::MoveTo") {
            impl_->printing.move_to(std::int16_t(arg(0)), std::int16_t(arg(1)));
            return_value = arg(0);
        } else if (name == "CarbonLib::TextWidth") {
            const auto count = std::max<std::int32_t>(std::int16_t(arg(2)), 0);
            return_value = std::uint32_t(std::int32_t(impl_->printing.text_width(
                s.memory.copy_out(arg(0) + std::int16_t(arg(1)), count))));
        } else if (name == "CarbonLib::GetFontInfo") {
            const auto info = impl_->printing.font_info();
            s.memory.write<2>(arg(0), std::uint16_t(info.ascent));
            s.memory.write<2>(arg(0) + 2, std::uint16_t(info.descent));
            s.memory.write<2>(arg(0) + 4, std::uint16_t(info.width_max));
            s.memory.write<2>(arg(0) + 6, std::uint16_t(info.leading));
            return_value = arg(0);
        } else if (name == "CarbonLib::DrawText") {
            const auto count = std::max<std::int32_t>(std::int16_t(arg(2)), 0);
            impl_->printing.draw_text(s.memory.copy_out(arg(0) + std::int16_t(arg(1)), count));
            return_value = arg(0);
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
            const auto point = impl_->pointer_now();
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
            return_value = impl_->pointer_now().down;
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
        } else if (name == "CarbonLib::WaitNextEvent" ||
                   (name == "CarbonLib::GetNextEvent" && impl_->event_mode == EventMode::isolated)) {
            return_value = impl_->next_event(s, arg(0), arg(1), name == "CarbonLib::WaitNextEvent" ? arg(2) : 0,
                name == "CarbonLib::WaitNextEvent" ? arg(3) : 0);
        } else if (name == "CarbonLib::AEProcessAppleEvent") {
            return_value = impl_->process_event(s, arg(0));
        } else if (name == "CarbonLib::AEGetParamDesc" || name == "CarbonLib::AECountItems" ||
                   name == "CarbonLib::AEGetNthDesc" || name == "CarbonLib::AECoerceDesc" ||
                   name == "CarbonLib::AEGetDescData") {
            // Guest descriptors are {type, token} pairs naming native ones.
            const auto descriptor = [&](std::uint32_t address) {
                return NativeAEDescriptor{std::uint32_t(s.memory.read<4>(address)), std::uint32_t(s.memory.read<4>(address + 4))};
            };
            const auto put = [&](std::uint32_t address, NativeAEDescriptor result) {
                s.memory.write<4>(address, result.token ? result.type : 0x6e756c6c);
                s.memory.write<4>(address + 4, result.token);
            };
            NativeAEDescriptor result;
            std::int32_t error = 0;
            if (name == "CarbonLib::AEGetParamDesc") {
                error = impl_->apple_events.parameter(descriptor(arg(0)), arg(1), arg(2), result);
                put(arg(3), result);
            } else if (name == "CarbonLib::AECountItems") {
                std::int32_t items = 0;
                error = impl_->apple_events.count(descriptor(arg(0)), items);
                s.memory.write<4>(arg(1), std::uint32_t(items));
            } else if (name == "CarbonLib::AEGetNthDesc") {
                std::uint32_t keyword = 0;
                error = impl_->apple_events.nth(descriptor(arg(0)), std::int32_t(arg(1)), arg(2), keyword, result);
                if (arg(3)) s.memory.write<4>(arg(3), keyword);
                put(arg(4), result);
            } else if (name == "CarbonLib::AECoerceDesc") {
                error = impl_->apple_events.coerce(descriptor(arg(0)), arg(1), result);
                put(arg(2), result);
            } else {
                std::vector<std::uint8_t> bytes;
                error = impl_->apple_events.data(descriptor(arg(0)), arg(2), bytes);
                if (!error && !bytes.empty()) s.memory.copy_in(arg(1), bytes);
            }
            return_value = std::uint32_t(error);
        } else if (name == "CarbonLib::EqualString") {
            return_value = equal_mac_strings(s, arg(0), arg(1), arg(2) & 0xff, arg(3) & 0xff) ? 1u : 0u;
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
            try {
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
                if (status) impl_->data_forks.note_write_audit_directory_failure(parent);
            } catch (...) { impl_->data_forks.note_write_audit_directory_failure(parent); throw; }
        } else if (name == "CarbonLib::FSOpenFork") {
            auto target = impl_->from_reference(s, arg(0));
            auto permission = std::int8_t(arg(3));
            if (!permission) permission = under(target, impl_->writable) ? 3 : 1;
            try {
                if (permission != 1) target = impl_->writable_file(target);
                std::int16_t ref = 0;
                status = impl_->data_forks.open(target, impl_->unicode_units(s, arg(2), arg(1)), permission, ref);
                impl_->performance.mark("file-open", s, std::uint16_t(ref));
                if (!status) s.memory.write<2>(arg(4), std::uint16_t(ref));
            } catch (...) {
                if (permission != 1) impl_->data_forks.note_write_audit_failure(target);
                throw;
            }
        } else if (name == "CarbonLib::FSDeleteObject") {
            status = impl_->delete_object(impl_->from_reference(s, arg(0)));
        } else if (name == "CarbonLib::FSRenameUnicode") {
            const auto target = impl_->from_reference(s, arg(0));
            try {
                std::string leaf;
                status = native_unicode_name(impl_->unicode_units(s, arg(2), arg(1)), leaf);
                if (!status) status = impl_->rename_object(target, leaf, arg(3));
                if (!status && arg(4)) impl_->write_reference(s, arg(4), impl_->reference_for(target.parent_path() / leaf));
                if (status) impl_->data_forks.note_write_audit_failure(target);
            } catch (...) { impl_->data_forks.note_write_audit_failure(target); throw; }
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
            try {
                if (fs::exists(target)) throw FileError{-48};
                const auto output = impl_->writable_path(target);
                fs::create_directories(output.parent_path());
                status = impl_->data_forks.create(output, pb ? 0 : arg(1), pb ? 0 : arg(2));
            } catch (...) { impl_->data_forks.note_write_audit_failure(target); throw; }
        } else if (name == "CarbonLib::PBHRenameSync") {
            const auto pb = arg(0);
            const auto replacement = std::uint32_t(s.memory.read<4>(pb + 28));
            if (!replacement || s.memory.read<1>(replacement) > 63) throw FileError{bad_name};
            const auto target = impl_->path(std::int16_t(s.memory.read<2>(pb + 22)),
                s.memory.read<4>(pb + 48), read_mac_string(s, s.memory.read<4>(pb + 18)));
            try {
                const auto leaf = read_mac_string(s, replacement);
                status = leaf.find(':') != std::string::npos ? bad_name : impl_->rename_object(target, host_name(leaf));
                if (status) impl_->data_forks.note_write_audit_failure(target);
                if (std::getenv("CM_AOT_TRACE_FILES"))
                    std::fprintf(stderr, "Native file rename: %s -> %s status=%d\n", target.c_str(), leaf.c_str(), status);
            } catch (...) { impl_->data_forks.note_write_audit_failure(target); throw; }
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
            try {
                if (permission != 1) target = impl_->writable_file(target);
                std::int16_t ref = 0;
                status = impl_->data_forks.open(target, permission, ref);
                impl_->performance.mark("file-open", s, std::uint16_t(ref));
                if (std::getenv("CM_AOT_TRACE_FILES"))
                    std::fprintf(stderr, "Native file open: %s permission=%d status=%d\n", target.c_str(), permission, status);
                if (!status) s.memory.write<2>(parameter_block ? arg(0) + 24 : arg(2), std::uint16_t(ref));
            } catch (...) {
                if (permission != 1) impl_->data_forks.note_write_audit_failure(target);
                throw;
            }
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
    } catch (const FileError& error) {
        status = error.code;
        if (failed_mutation()) impl_->data_forks.note_unattributed_write_audit_failure();
    } catch (const fs::filesystem_error&) {
        status = parameter_error;
        if (failed_mutation()) impl_->data_forks.note_unattributed_write_audit_failure();
    }
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
