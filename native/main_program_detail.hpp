#pragma once
// What main_program_*.cpp share: views of the Toolbox records and of the
// game's objects MAIN.CPP reads, MAIN.CPP's globals, and the Toolbox calls.
//
// The routines compute with C++ values and leave what the game can observe as
// the original does (guest_abi.hpp): memory, the stack image, the registers
// its callers and callees can read (r1, r2, r13-r31, the results, r4-r10 and
// the condition register). They also keep, in r13-r31, the values the
// original keeps there across its calls: each callee's prologue saves them
// into the stack image. r0, r11, r12, CTR and XER never reach memory or a
// decision (every callee writes r0 before reading it, the glues use r11, r12
// and CTR as scratch, and the carry is set before each instruction reading
// it), so they are not reproduced.
#include "main_program.hpp"

namespace cm::aot::main_program::detail {
using guest::Cpu;
using guest::Frame;

// A value in guest memory (big-endian), read and written as T.
template <class T> class Field {
public:
    Field(Cpu& c, std::uint32_t address) : c_(&c), address_(address) {}
    T get() const { return *this; }
    operator T() const {
        if constexpr (sizeof(T) == 1) return T(c_->u8(address_));
        else if constexpr (sizeof(T) == 2) return T(c_->u16(address_));
        else return T(c_->u32(address_));
    }
    // Writes the value (a view writes through even when const, like a reference).
    const Field& operator=(T value) const {
        if constexpr (sizeof(T) == 1) c_->put8(address_, std::uint32_t(value));
        else if constexpr (sizeof(T) == 2) c_->put16(address_, std::uint32_t(value));
        else c_->put32(address_, std::uint32_t(value));
        return *this;
    }
    // A view is not reassigned: writing a value takes a T.
    Field& operator=(const Field&) = delete;
    Field(const Field&) = default;
    std::uint32_t address() const { return address_; }

private:
    Cpu* c_;
    std::uint32_t address_;
};

// Toolbox records.
struct EventRecord {
    Cpu& c;
    std::uint32_t address;
    Field<std::uint16_t> what() const { return {c, address}; }
    Field<std::uint32_t> message() const { return {c, address + 2}; }
    Field<std::uint32_t> when() const { return {c, address + 6}; }
    Field<std::uint32_t> where() const { return {c, address + 10}; }  // Point: v << 16 | h
    Field<std::uint16_t> modifiers() const { return {c, address + 14}; }
};
enum class EventKind : std::uint16_t {
    null = 0, mouse_down = 1, mouse_up = 2, key_down = 3, key_up = 4, auto_key = 5, update = 6, disk = 7,
    activate = 8, os = 15, high_level = 23
};
namespace modifier {
constexpr std::uint16_t command = 0x100, option = 0x800;
}
namespace part {  // FindWindow
constexpr std::int16_t menu_bar = 1, system_window = 2, content = 3, drag = 4, go_away = 6;
}
struct FSSpec {  // vRefNum, parID, name (a Pascal string)
    Cpu& c;
    std::uint32_t address;
    Field<std::int16_t> volume() const { return {c, address}; }
    Field<std::int32_t> directory() const { return {c, address + 2}; }
    std::uint32_t name() const { return address + 6; }
    Field<std::uint8_t> name_length() const { return {c, address + 6}; }
    static constexpr std::uint32_t size = 70;
};
constexpr std::uint32_t four_char(char a, char b, char c, char d) {
    return std::uint32_t(std::uint8_t(a)) << 24 | std::uint32_t(std::uint8_t(b)) << 16 |
           std::uint32_t(std::uint8_t(c)) << 8 | std::uint8_t(d);
}

// MAIN.CPP's own globals (fixed offsets from the TOC).
struct Globals {
    Cpu& c;
    template <class T> Field<T> at(std::int32_t offset) const { return {c, c.r(2) + std::uint32_t(offset)}; }
    Field<std::int8_t> menu_checked() const { return at<std::int8_t>(global::menu_checked); }
    Field<std::int32_t> menu_features() const { return at<std::int32_t>(global::menu_features); }
    Field<std::uint32_t> setup_text() const { return at<std::uint32_t>(global::setup_text); }
    Field<std::uint32_t> command_line() const { return at<std::uint32_t>(global::command_line); }
    Field<std::uint32_t> spoken_text() const { return at<std::uint32_t>(global::spoken_text); }
    Field<std::uint32_t> speech_channel() const { return at<std::uint32_t>(global::speech_channel); }
    Field<std::int8_t> speech_available() const { return at<std::int8_t>(global::speech_available); }
    Field<std::uint32_t> mouse_region() const { return at<std::uint32_t>(global::mouse_region); }
    Field<std::uint32_t> sleep_ticks() const { return at<std::uint32_t>(global::sleep_ticks); }
    Field<std::int8_t> opened() const { return at<std::int8_t>(global::opened); }
    Field<std::int8_t> running() const { return at<std::int8_t>(global::running); }
};

// The objects of other modules MAIN.CPP reaches through TOC slots.
namespace slot {
constexpr std::int32_t pool = -0x3bac;             // MAIN.CPP's strings ("MAIN.CPP" first)
constexpr std::int32_t jump_buffer = -0x3ba8;      // run()'s setjmp buffer
constexpr std::int32_t menu_texts = -0x3bb0;       // char*[50]: remembered menu item texts
constexpr std::int32_t click_types = -0x3bb8;      // EventTypeSpec[2]: {mous 10}, {wind 0x55}
constexpr std::int32_t bounds_type = -0x3bb4;      // EventTypeSpec: {wind 2}
constexpr std::int32_t screen_rect = -0x3bc0;      // Rect {0, 0, 600, 800}
constexpr std::int32_t quick_start_callback = -0x3bc8;  // TVector of write_quick_start
constexpr std::int32_t setup_callback = -0x3bcc;   // TVector of add_setup_line
constexpr std::int32_t message = -0x7ba8;          // the shared message buffer
constexpr std::int32_t game = -0x7bb0;             // the game object
constexpr std::int32_t settings = -0x7408;         // the settings object
constexpr std::int32_t process = -0x5bbc;          // ProcessInfoRec
constexpr std::int32_t application = -0x5bac;      // FSSpec of the application
constexpr std::int32_t settings_name = -0x5b20;    // char[]
constexpr std::int32_t volume_name = -0x5bb8;      // char[]: the application volume's name
constexpr std::int32_t open_application_handler = -0x5ac0, open_documents_handler = -0x5ac4,
                       print_documents_handler = -0x5ac8, quit_handler = -0x5acc;  // TVectors
constexpr std::int32_t window_handler = -0x5ad4;   // TVector of window_event
// Values (Shared below).
constexpr std::int32_t left_held = -0x7bac, game_started = -0x7418, data_directory = -0x5bb4,
                       data_volume = -0x5bb0, start_volume = -0x5ba8, speech_wanted = -0x5b98,
                       foreground = -0x5b14, right_button_modifiers = -0x5ad0, game_window = -0x5af8,
                       other_window = -0x5aec, button_state = -0x5ae0, pending_scroll = -0x5ad8,
                       control_down = -0x5adc, left_down = -0x744c, right_down = -0x5c00,
                       right_held = -0x5c0c, idle_wait = -0x5c20, timer = -0x5c18, mouse_x = -0x5c10,
                       mouse_y = -0x5c14, key_code = -0x5c1c, key_character = -0x74b0, quick_start = -0x62c8;
}
struct Shared {
    Cpu& c;
    template <class T> Field<T> at(std::int32_t slot) const { return {c, c.toc(slot)}; }
    // int8: the left button is held (the game's asserts clear it too).
    Field<std::int8_t> left_held() const { return at<std::int8_t>(slot::left_held); }
    Field<std::int8_t> game_started() const { return at<std::int8_t>(slot::game_started); }
    Field<std::int32_t> data_directory() const { return at<std::int32_t>(slot::data_directory); }
    Field<std::int16_t> data_volume() const { return at<std::int16_t>(slot::data_volume); }
    Field<std::int16_t> start_volume() const { return at<std::int16_t>(slot::start_volume); }
    Field<std::int8_t> speech_wanted() const { return at<std::int8_t>(slot::speech_wanted); }
    Field<std::int8_t> foreground() const { return at<std::int8_t>(slot::foreground); }
    // Modifier keys that make a click the right button.
    Field<std::uint32_t> right_button_modifiers() const { return at<std::uint32_t>(slot::right_button_modifiers); }
    Field<std::uint32_t> game_window() const { return at<std::uint32_t>(slot::game_window); }
    Field<std::uint32_t> other_window() const { return at<std::uint32_t>(slot::other_window); }
    Field<std::uint16_t> button_state() const { return at<std::uint16_t>(slot::button_state); }  // 1 left, 2 right
    Field<std::int32_t> pending_scroll() const { return at<std::int32_t>(slot::pending_scroll); }
    Field<std::int8_t> control_down() const { return at<std::int8_t>(slot::control_down); }
    Field<std::int8_t> left_down() const { return at<std::int8_t>(slot::left_down); }
    Field<std::int8_t> right_down() const { return at<std::int8_t>(slot::right_down); }
    Field<std::int8_t> right_held() const { return at<std::int8_t>(slot::right_held); }  // the right button is held
    Field<std::int32_t> idle_wait() const { return at<std::int32_t>(slot::idle_wait); }  // turns before reading events
    Field<std::uint32_t> timer() const { return at<std::uint32_t>(slot::timer); }        // the game's tick count
    Field<std::int32_t> mouse_x() const { return at<std::int32_t>(slot::mouse_x); }
    Field<std::int32_t> mouse_y() const { return at<std::int32_t>(slot::mouse_y); }
    Field<std::uint32_t> key_code() const { return at<std::uint32_t>(slot::key_code); }
    Field<std::int8_t> key_character() const { return at<std::int8_t>(slot::key_character); }
    Field<std::int8_t> quick_start() const { return at<std::int8_t>(slot::quick_start); }
    // The game object's screen request (halfword at +0x25819e).
    Field<std::int16_t> screen_request() const { return {c, c.toc(slot::game) + 0x260000 - 0x7e62}; }
};
enum ScreenRequest : std::int16_t { request_save = 0x3fe, request_quit = 0x402, request_about = 0x42b };
constexpr std::int32_t event_not_handled = -0x2692;   // eventNotHandledErr
constexpr std::int32_t ae_not_handled = -0x6ac;       // errAEEventNotHandled
// MAIN.CPP's strings (offsets into its pool).
namespace text {
constexpr std::uint32_t file_name = 0, assert_format = 9, error_title = 0x16, colon = 0x1c,
                        quit_requested = 0x1e, quit = 0x30, save_requested = 0x35, save = 0x47, key_format = 0x67,
                        cfg = 0x83, quick_start_file = 0x87, write_mode = 0x97, rgn = 0x99, sav = 0x9d,
                        load_option = 0xa1, quote = 0xaa, auto_setup = 0xf7;
}

// The original's compares into cr0 (cmpwi/cmpw, cmplwi/cmplw, and the record
// forms, which compare their result with 0). A callee that saves the
// condition register stores it whole, so each compare is made here too.
inline std::int32_t compare(Cpu& c, std::int32_t a, std::int32_t b = 0) {
    c.compare(0, a, b);
    return a < b ? -1 : a > b ? 1 : 0;
}
inline std::int32_t compare_logical(Cpu& c, std::uint32_t a, std::uint32_t b = 0) {
    c.compare_logical(0, a, b);
    return a < b ? -1 : a > b ? 1 : 0;
}
// A 16-bit value as the original holds it in a register (sign-extended).
inline std::uint32_t extended(std::int16_t value) { return std::uint32_t(std::int32_t(value)); }
// A value a C++ routine returns, left in r3 as the original's callee leaves it.
inline std::uint32_t result(Cpu& c, std::uint32_t value) {
    c.set(3, value);
    return value;
}
// A result tested for zero as the original does (cmpwi/cmplwi, or a record form).
inline bool nonzero(Cpu& c, std::int32_t value) { return compare(c, value) != 0; }
inline bool nonzero_logical(Cpu& c, std::uint32_t value) { return compare_logical(c, value) != 0; }

// The Toolbox, through each call's glue (which saves r2 at sp+0x14; the TOC
// comes back from there). Each call sets the argument registers the
// original's call site sets; `back` is the address after its `bl`.
class Toolbox {
public:
    Toolbox(Cpu& c, const Frame& frame) : c(c), frame(frame) {}
    std::uint32_t call(std::uint32_t glue, std::uint32_t back, std::initializer_list<std::uint32_t> arguments = {}) {
        unsigned n = 3;
        for (const auto value : arguments) c.set(n++, value);
        c.call(glue, back);
        c.set(2, c.u32(frame.at(0x14)));
        return c.r(3);
    }
    // Processes and memory.
    std::int16_t GetCurrentProcess(std::uint32_t psn, std::uint32_t back) { return std::int16_t(call(0x1077fb08, back, {psn})); }
    std::int16_t GetProcessInformation(std::uint32_t psn, std::uint32_t info, std::uint32_t back) {
        return std::int16_t(call(0x1077fb20, back, {psn, info}));
    }
    std::int16_t Gestalt(std::uint32_t selector, std::uint32_t response, std::uint32_t back) {
        return std::int16_t(call(0x1077fad8, back, {selector, response}));
    }
    std::uint32_t NewPtr(std::uint32_t size, std::uint32_t back) { return call(0x1077fc70, back, {size}); }
    void DisposePtr(std::uint32_t pointer, std::uint32_t back) { call(0x1077fc58, back, {pointer}); }
    void DisposeHandle(std::uint32_t handle, std::uint32_t back) { call(0x107800c0, back, {handle}); }
    void BlockMoveData(std::uint32_t from, std::uint32_t to, std::uint32_t size, std::uint32_t back) {
        call(0x1077fc28, back, {from, to, size});
    }
    void BlockMove(std::uint32_t from, std::uint32_t to, std::uint32_t size, std::uint32_t back) {
        call(0x107800f0, back, {from, to, size});
    }
    void MoreMasters(std::uint32_t back) { call(0x10780108, back); }
    bool EqualString(std::uint32_t a, std::uint32_t b, bool case_sensitive, bool diacritics_sensitive, std::uint32_t back) {
        return call(0x1077fc40, back, {a, b, case_sensitive, diacritics_sensitive}) & 0xff;
    }
    // Apple events.
    std::uint32_t NewAEEventHandlerUPP(std::uint32_t handler, std::uint32_t back) { return call(0x1077fb50, back, {handler}); }
    void AEInstallEventHandler(std::uint32_t event_class, std::uint32_t id, std::uint32_t upp, std::uint32_t refcon,
                               bool system, std::uint32_t back) {
        call(0x1077fb68, back, {event_class, id, upp, refcon, system});
    }
    void AEProcessAppleEvent(std::uint32_t event, std::uint32_t back) { call(0x10780528, back, {event}); }
    std::int16_t AEGetParamDesc(std::uint32_t event, std::uint32_t key, std::uint32_t type, std::uint32_t result,
                                std::uint32_t back) {
        return std::int16_t(call(0x1077fb80, back, {event, key, type, result}));
    }
    // Events and windows.
    void InitCursor(std::uint32_t back) { call(0x1077fb38, back); }
    bool WaitNextEvent(std::uint16_t mask, EventRecord event, std::uint32_t sleep, std::uint32_t region, std::uint32_t back) {
        return call(0x10780510, back, {mask, event.address, sleep, region}) & 0xff;
    }
    void GetPort(std::uint32_t port, std::uint32_t back) { call(0x107803f0, back, {port}); }
    std::uint32_t GetWindowPort(std::uint32_t window, std::uint32_t back) { return call(0x107805a0, back, {window}); }
    void SetPort(std::uint32_t port, std::uint32_t back) { call(0x107804f8, back, {port}); }
    void GetMouse(std::uint32_t point, std::uint32_t back) { call(0x10780618, back, {point}); }
    std::uint32_t PinRect(std::uint32_t rect, std::uint32_t point, std::uint32_t back) { return call(0x10780630, back, {rect, point}); }
    std::int16_t FindWindow(std::uint32_t point, std::uint32_t window, std::uint32_t back) {
        return std::int16_t(call(0x10780720, back, {point, window}));
    }
    void SelectWindow(std::uint32_t window, std::uint32_t back) { call(0x10780738, back, {window}); }
    std::uint32_t GetGrayRgn(std::uint32_t back) { return call(0x10780750, back); }
    void GetRegionBounds(std::uint32_t region, std::uint32_t rect, std::uint32_t back) { call(0x10780768, back, {region, rect}); }
    void DragWindow(std::uint32_t window, std::uint32_t point, std::uint32_t bounds, std::uint32_t back) {
        call(0x10780780, back, {window, point, bounds});
    }
    bool TrackGoAway(std::uint32_t window, std::uint32_t point, std::uint32_t back) {
        return call(0x10780798, back, {window, point}) & 0xff;
    }
    bool Button(std::uint32_t back) { return call(0x10780828, back) & 0xff; }
    std::uint32_t NewRgn(std::uint32_t back) { return call(0x10780120, back); }
    void DisposeRgn(std::uint32_t region, std::uint32_t back) { call(0x107812d8, back, {region}); }
    std::uint32_t NewEventHandlerUPP(std::uint32_t handler, std::uint32_t back) { return call(0x10780cc0, back, {handler}); }
    std::uint32_t GetWindowEventTarget(std::uint32_t window, std::uint32_t back) { return call(0x10780cd8, back, {window}); }
    void InstallEventHandler(std::uint32_t target, std::uint32_t upp, std::uint32_t count, std::uint32_t types,
                             std::uint32_t data, std::uint32_t handler_out, std::uint32_t back) {
        call(0x10780cf0, back, {target, upp, count, types, data, handler_out});
    }
    std::uint32_t GetEventClass(std::uint32_t event, std::uint32_t back) { return call(0x10780eb8, back, {event}); }
    std::uint32_t GetEventKind(std::uint32_t event, std::uint32_t back) { return call(0x10780ed0, back, {event}); }
    void GetEventParameter(std::uint32_t event, std::uint32_t name, std::uint32_t type, std::uint32_t type_out,
                           std::uint32_t size, std::uint32_t size_out, std::uint32_t data, std::uint32_t back) {
        call(0x10780ee8, back, {event, name, type, type_out, size, size_out, data});
    }
    // Menus.
    std::uint32_t GetNewMBar(std::uint32_t id, std::uint32_t back) { return call(0x107801b0, back, {id}); }
    void SetMenuBar(std::uint32_t bar, std::uint32_t back) { call(0x107801c8, back, {bar}); }
    void InvalMenuBar(std::uint32_t back) { call(0x10780270, back); }
    std::uint32_t GetMenuHandle(std::int16_t id, std::uint32_t back) { return call(0x10780210, back, {extended(id)}); }
    std::uint32_t GetMenu(std::int16_t id, std::uint32_t back) { return call(0x107807b0, back, {extended(id)}); }
    std::int16_t CountMenuItems(std::uint32_t menu, std::uint32_t back) { return std::int16_t(call(0x10780240, back, {menu})); }
    void GetMenuItemText(std::uint32_t menu, std::int16_t item, std::uint32_t text, std::uint32_t back) {
        call(0x10780258, back, {menu, extended(item), text});
    }
    void SetMenuItemText(std::uint32_t menu, std::int16_t item, std::uint32_t text, std::uint32_t back) {
        call(0x107807e0, back, {menu, extended(item), text});
    }
    void EnableMenuItem(std::uint32_t menu, std::uint16_t item, std::uint32_t back) { call(0x107807f8, back, {menu, item}); }
    void DisableMenuItem(std::uint32_t menu, std::uint16_t item, std::uint32_t back) { call(0x107807c8, back, {menu, item}); }
    void DeleteMenuItem(std::uint32_t menu, std::int16_t item, std::uint32_t back) {
        call(0x107801f8, back, {menu, extended(item)});
    }
    void AppendResMenu(std::uint32_t menu, std::uint32_t type, std::uint32_t back) { call(0x10780228, back, {menu, type}); }
    std::int32_t GetIndMenuItemWithCommandID(std::uint32_t menu, std::uint32_t command, std::uint32_t index,
                                             std::uint32_t menu_out, std::uint32_t item_out, std::uint32_t back) {
        return std::int32_t(call(0x107801e0, back, {menu, command, index, menu_out, item_out}));
    }
    std::uint32_t MenuSelect(std::uint32_t point, std::uint32_t back) { return call(0x10780810, back, {point}); }
    std::uint32_t MenuKey(std::uint8_t character, std::uint32_t back) { return call(0x10780660, back, {character}); }
    void HiliteMenu(std::int16_t id, std::uint32_t back) { call(0x10780708, back, {extended(id)}); }
    // Files and folders.
    std::int16_t FindFolder(std::int16_t volume, std::uint32_t type, bool create, std::uint32_t found_volume,
                            std::uint32_t found_directory, std::uint32_t back) {
        return std::int16_t(call(0x10780288, back, {extended(volume), type, create, found_volume, found_directory}));
    }
    void GetIndString(std::uint32_t text, std::int16_t id, std::int16_t index, std::uint32_t back) {
        call(0x107802a0, back, {text, extended(id), extended(index)});
    }
    std::int16_t FSMakeFSSpec(std::int16_t volume, std::int32_t directory, std::uint32_t name, FSSpec spec,
                              std::uint32_t back) {
        return std::int16_t(call(0x1077fee0, back, {extended(volume), std::uint32_t(directory), name, spec.address}));
    }
    void FSpCreateResFile(FSSpec spec, std::uint32_t creator, std::uint32_t type, std::int16_t script, std::uint32_t back) {
        call(0x107802b8, back, {spec.address, creator, type, extended(script)});
    }
    void FSpOpenResFile(FSSpec spec, std::uint32_t permission, std::uint32_t back) { call(0x107802d0, back, {spec.address, permission}); }
    std::int16_t FSpDirCreate(FSSpec spec, std::int16_t script, std::uint32_t directory_out, std::uint32_t back) {
        return std::int16_t(call(0x107802e8, back, {spec.address, extended(script), directory_out}));
    }
    std::int16_t ResolveAliasFile(FSSpec spec, bool resolve_chains, std::uint32_t is_folder, std::uint32_t was_aliased,
                                  std::uint32_t back) {
        return std::int16_t(call(0x1077ffa0, back, {spec.address, resolve_chains, is_folder, was_aliased}));
    }
    std::int16_t PBHGetVInfoSync(std::uint32_t block, std::uint32_t back) { return std::int16_t(call(0x10780000, back, {block})); }
    // Speech and dialogs.
    std::int16_t NewSpeechChannel(std::uint32_t voice, std::uint32_t channel_out, std::uint32_t back) {
        return std::int16_t(call(0x10780ff0, back, {voice, channel_out}));
    }
    std::int16_t SpeechBusy(std::uint32_t back) { return std::int16_t(call(0x10781008, back)); }
    void SpeakText(std::uint32_t channel, std::uint32_t text, std::uint32_t length, std::uint32_t back) {
        call(0x10781020, back, {channel, text, length});
    }
    void DisposeSpeechChannel(std::uint32_t channel, std::uint32_t back) { call(0x10780648, back, {channel}); }
    std::uint32_t GetNewDialog(std::int16_t id, std::uint32_t storage, std::uint32_t behind, std::uint32_t back) {
        return call(0x10780678, back, {extended(id), storage, behind});
    }
    std::uint32_t GetDialogWindow(std::uint32_t dialog, std::uint32_t back) { return call(0x10780690, back, {dialog}); }
    void ShowWindow(std::uint32_t window, std::uint32_t back) { call(0x107806a8, back, {window}); }
    void ModalDialog(std::uint32_t filter, std::uint32_t item_out, std::uint32_t back) { call(0x107806c0, back, {filter, item_out}); }
    void DisposeDialog(std::uint32_t dialog, std::uint32_t back) { call(0x107806d8, back, {dialog}); }

private:
    Cpu& c;
    const Frame& frame;
};

// Stops the speech channel and frees the text it was given (stop_speech();
// fatal_exit() and speech_key() repeat it in line). `dispose_back` and
// `free_back` are the return addresses of its two calls.
inline void release_speech(Cpu& c, const Frame& frame, std::uint32_t dispose_back, std::uint32_t free_back) {
    Globals g{c};
    Toolbox os{c, frame};
    c.set(3, g.speech_channel());
    if (!nonzero_logical(c, c.r(3))) return;
    os.DisposeSpeechChannel(c.r(3), dispose_back);
    c.set(3, g.spoken_text());
    if (nonzero_logical(c, c.r(3))) c.call(0x10775170, free_back);  // free
    g.speech_channel() = 0;
    g.spoken_text() = 0;
}
}
