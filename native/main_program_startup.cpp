#include "main_program_detail.hpp"
#include <array>
#include <iterator>

namespace cm::aot::main_program {
using namespace detail;
namespace {

// The main loop, from setjmp's return (0x1076f374): a fatal exit returns
// here too, with the code it was given.
void main_loop(Cpu& c, const Frame& frame) {
    Globals g{c};
    Toolbox os{c, frame};
    Program program{c};
    c.set(31, c.r(3));  // the result run() returns
    if (const std::uint32_t text = g.setup_text(); nonzero_logical(c, text)) {
        // Documents dropped on the application before it started: run their
        // setup lines as a script, then stop.
        c.set(3, text);
        c.link(0x1076f38c);
        run_script(c, text);
        os.DisposePtr(g.setup_text(), 0x1076f394);
        c.set(3, 0);
        g.setup_text() = 0;
        g.running() = 0;
    }
    while (nonzero(c, g.running())) {
        g.running() = program.step(0x1076f3b0);
        c.link(0x1076f3b8);
        pump_events(c);
    }
}
}

void main(Cpu& c) {
    Frame frame(c, 0x1076f2b0, 0x40, {});
    c.link(0x1076f2c0);
    run(c);
    c.set(3, 0);
}

std::uint32_t run(Cpu& c) {
    Frame frame(c, 0x1076f2e0, 0x70, {.first_gpr = 29});
    Globals g{c};
    Toolbox os{c, frame};
    Program program{c};
    const auto pool = c.toc(slot::pool);
    // The original keeps the message buffer, the pool and its result in
    // r29-r31 through the whole run.
    c.set({{31, 0}, {30, pool}, {29, c.toc(slot::message)}});
    c.link(0x1076f308);
    if (!nonzero(c, std::int8_t(result(c, process_information(c))))) return 0;
    c.link(0x1076f314);
    install_handlers(c);
    c.link(0x1076f318);
    initialize(c);
    c.link(0x1076f31c);
    set_up_menus(c);
    c.link(0x1076f320);
    find_folders(c);
    program.load_settings(c.toc(slot::settings), 0x1076f328);
    // The game starts once the open-application (or open-documents) event came.
    while (!nonzero(c, g.opened())) {
        c.link(0x1076f330);
        poll_high_level_events(c);
    }
    if (nonzero(c, program.start_game(g.command_line(), 0x1076f344))) {
        if (const std::uint32_t line = g.command_line(); nonzero_logical(c, line)) {
            os.DisposePtr(line, 0x1076f35c);
            g.command_line() = 0;
        }
        program.flush_input(0x1076f36c);
        program.setjmp(c.toc(slot::jump_buffer), 0x1076f374);
        // fatal_exit()'s longjmp lands at 0x1076f374 in this frame; the
        // transfer runtime would resume the generated body there, so the
        // C++ takes it instead and goes on as the original does.
        for (;;) {
            try {
                main_loop(c, frame);
                break;
            } catch (const GuestContextTransfer& transfer) {
                if (transfer.state != &c.s || c.s.pc != 0x1076f374 || c.sp() != frame.base()) throw;
            }
        }
    } else {
        guest::report_assertion(c, {0x1076f3dc, pool + text::file_name, pool + text::assert_format,
                                    pool + text::error_title, frame.at(0x40), frame.at(0x38), 0x1b0});
    }
    c.link(0x1076f420);
    shut_down(c);
    return c.r(31);
}

std::uint32_t process_information(Cpu& c) {
    Frame frame(c, 0x1076f440, 0x50, {.first_gpr = 31});
    Toolbox os{c, frame};
    // ProcessInfoRec: processInfoLength, and the application's FSSpec at +0x38.
    const auto info = c.toc(slot::process);
    c.set(31, info);  // kept in r31 across the calls
    c.put32(info, 0x3c);
    c.put32(info + 0x38, c.toc(slot::application));
    c.set(4, 0x3c);  // the original builds the length in r4
    const auto process = frame.at(0x3c);
    if (nonzero(c, os.GetCurrentProcess(process, 0x1076f46c))) return 0;
    if (nonzero(c, os.GetProcessInformation(process, info, 0x1076f48c))) return 0;
    const auto response = frame.at(0x38);
    const bool speech = !nonzero(c, os.Gestalt(four_char('t', 't', 's', 'c'), response, 0x1076f4b0)) && (c.u32(response) & 1);
    Globals{c}.speech_available() = speech;
    return 1;
}

void install_handlers(Cpu& c) {
    Frame frame(c, 0x1076f4f0, 0x40, {.first_gpr = 31});
    Toolbox os{c, frame};
    os.InitCursor(0x1076f504);
    struct Handler {
        std::int32_t routine;
        std::uint32_t id, upp_back, install_back;
    };
    constexpr Handler handlers[]{
        {slot::open_application_handler, four_char('o', 'a', 'p', 'p'), 0x1076f510, 0x1076f534},
        {slot::open_documents_handler, four_char('o', 'd', 'o', 'c'), 0x1076f540, 0x1076f564},
        {slot::print_documents_handler, four_char('p', 'd', 'o', 'c'), 0x1076f570, 0x1076f594},
        {slot::quit_handler, four_char('q', 'u', 'i', 't'), 0x1076f5a0, 0x1076f5c4}};
    for (const auto& handler : handlers) {
        const auto upp = os.NewAEEventHandlerUPP(c.toc(handler.routine), handler.upp_back);
        os.AEInstallEventHandler(four_char('a', 'e', 'v', 't'), handler.id, upp, 0, false, handler.install_back);
    }
    for (std::uint32_t n = 0; n < 10;) {  // the count in r31
        c.set(31, n);
        os.MoreMasters(0x1076f5d4);
        c.set(31, ++n);
        compare(c, std::int32_t(n), 10);
    }
}

void initialize(Cpu& c) {
    Frame frame(c, 0x1076f600, 0x3d0, {.first_gpr = 29});
    Toolbox os{c, frame};
    Program program{c};
    Globals g{c};
    Shared shared{c};
    const FSSpec application{c, c.toc(slot::application)};
    c.set({{30, application.address}, {29, 0}});  // kept across the calls (r29 the volume index)
    g.running() = 1;
    g.opened() = 0;
    g.sleep_ticks() = 0;
    shared.foreground() = 1;
    g.mouse_region() = os.NewRgn(0x1076f640);
    shared.right_button_modifiers() = modifier::command | 0x200 | modifier::option;  // shift too
    shared.speech_wanted() = program.option_key_down(0x1076f658);  // speech is on when option is held at launch
    g.speech_channel() = 0;
    g.setup_text() = 0;
    // The application's version ('vers' 1): its short version string names
    // the settings.
    c.set(6, shared.speech_wanted().address());  // left in r6 from the store above
    program.read_version(frame.at(0x1b4), 1, true, 0x1076f67c);
    program.copy_to_c(c.toc(slot::settings_name), frame.at(0x1bd), 0x1076f688);
    const auto line = os.NewPtr(0x400, 0x1076f690);
    const bool allocated = nonzero_logical(c, line);
    g.command_line() = line;
    if (allocated) c.put8(line, 0);
    // The name of the volume the application is on: an indexed PBHGetVInfo
    // (ioVolIndex +0x1c, ioNamePtr +0x12, ioVRefNum +0x16) until it matches.
    const auto block = frame.at(0x38), name = frame.at(0xb4);
    program.memset(block, 0, 0x7a, 0x1076f6b8);
    c.set(31, name);
    for (std::int16_t index = 0;; ++index) {
        c.put16(block + 0x1c, std::uint32_t(index));
        c.put32(block + 0x12, name);
        c.set(29, std::uint32_t(index + 1));
        if (nonzero(c, os.PBHGetVInfoSync(block, 0x1076f6d4))) break;
        if (compare(c, c.i16(block + 0x16), application.volume()) == 0) {
            program.copy_to_c(c.toc(slot::volume_name), name, 0x1076f6fc);
            break;
        }
    }
    shared.start_volume() = application.volume().get();
}

void shut_down(Cpu& c) {
    Frame frame(c, 0x1076f730, 0x40, {});
    Toolbox os{c, frame};
    Program program{c};
    Globals g{c};
    const auto started = Shared{c}.game_started();
    c.set(3, started.address());  // left in r3 either way
    if (nonzero(c, started)) program.end_game(started.address(), 0x1076f750);
    c.link(0x1076f754);
    stop_speech(c);
    program.save_settings(c.toc(slot::settings), 0x1076f75c);
    if (const std::uint32_t region = g.mouse_region(); nonzero_logical(c, region)) os.DisposeRgn(region, 0x1076f76c);
}

void stop_speech(Cpu& c) {
    Frame frame(c, 0x1076f780, 0x40, {});
    release_speech(c, frame, 0x1076f79c, 0x1076f7b0);
}

void fatal_exit(Cpu& c, std::uint32_t code) {
    Frame frame(c, 0x1076f7d0, 0x40, {.first_gpr = 31});
    Toolbox os{c, frame};
    Program program{c};
    Globals g{c};
    const auto started = Shared{c}.game_started();
    c.set({{4, started.address()}, {31, code}, {3, std::uint8_t(started)}});
    if (nonzero(c, started)) {
        // Back to run(), which shuts down; longjmp does not return.
        g.running() = 0;
        program.longjmp(c.toc(slot::jump_buffer), code, 0x1076f808);
        return;
    }
    // The original tests the clear flag once more before ending the game.
    if (nonzero(c, started)) program.end_game(c.r(3), 0x1076f818);
    release_speech(c, frame, 0x1076f828, 0x1076f83c);
    program.save_settings(c.toc(slot::settings), 0x1076f850);
    if (const std::uint32_t region = g.mouse_region(); nonzero_logical(c, region)) os.DisposeRgn(region, 0x1076f860);
    program.exit(code, 0x1076f86c);  // does not return
}

void nothing(Cpu&) {}

void set_up_menus(Cpu& c) {
    Frame frame(c, 0x1076f890, 0x40, {});
    Toolbox os{c, frame};
    if (const auto bar = os.GetNewMBar(1, 0x1076f8a4); nonzero_logical(c, bar)) {
        os.SetMenuBar(bar, 0x1076f8b4);
        c.link(0x1076f8bc);
        if (nonzero(c, std::int8_t(result(c, aqua_menus(c))))) {
            // Under Aqua the application menu has Quit: remove the File
            // menu's Quit and the separator above it.
            const Field<std::uint32_t> menu{c, frame.at(0x3c)};
            const Field<std::uint16_t> item{c, frame.at(0x38)};
            if (!nonzero(c, os.GetIndMenuItemWithCommandID(0, four_char('q', 'u', 'i', 't'), 2, menu.address(), item.address(),
                                                          0x1076f8e0))) {
                os.DeleteMenuItem(menu, std::int16_t(item), 0x1076f8fc);
                os.DeleteMenuItem(menu, std::int16_t(item - 1), 0x1076f914);
            }
        }
        if (const auto apple = os.GetMenuHandle(1, 0x1076f920); nonzero_logical(c, apple))
            os.AppendResMenu(apple, four_char('D', 'R', 'V', 'R'), 0x1076f938);
    }
    c.link(0x1076f940);
    remember_menu_texts(c);
    os.InvalMenuBar(0x1076f944);
}

void remember_menu_texts(Cpu& c) {
    Frame frame(c, 0x1076f960, 0x260, {.first_gpr = 25});
    Toolbox os{c, frame};
    Program program{c};
    const auto texts = c.toc(slot::menu_texts), pascal = frame.at(0x138), copy = frame.at(0x38);
    // The original keeps the count in r25, the menu in r26, its items in
    // r27, the item in r28, the menu id in r29, the array in r30 and the
    // offset in r31; the two copies it calls save them.
    std::uint32_t kept = 0, offset = 0;
    c.set({{30, texts}, {25, kept}, {31, offset}});
    for (std::int16_t id = 2;; ++id) {
        c.set(29, extended(id));
        const auto menu = os.GetMenuHandle(id, 0x1076f9f4);
        c.set(26, menu);
        if (!nonzero(c, std::int32_t(menu))) return;
        const auto items = os.CountMenuItems(menu, 0x1076f98c);
        c.set(27, extended(items));
        for (std::int16_t item = 1; c.set(28, extended(item)), compare(c, item, items) <= 0; ++item) {
            os.GetMenuItemText(menu, item, pascal, 0x1076f9b0);
            program.copy_to_c(copy, pascal, 0x1076f9c0);
            const auto kept_text = program.duplicate(copy, 0x1076f9c8);
            c.set(25, ++kept);
            c.put32(texts + offset, kept_text);
            const bool full = compare(c, std::int32_t(kept), 50) == 0;
            c.set(31, offset += 4);
            if (full) return;
        }
    }
}

void localize_menus(Cpu& c) {
    Frame frame(c, 0x1076fa20, 0x260, {.first_gpr = 23});
    Toolbox os{c, frame};
    Program program{c};
    const auto texts = c.toc(slot::menu_texts), localized = frame.at(0x38), pascal = frame.at(0x138);
    // The next remembered text, by offset; it advances only past items that
    // have one. An item without one stops the walk: the later items all read
    // that same empty entry (a quirk of the original). The original keeps the
    // offset in r27 and r30 (and per menu in r28, r31), the menu in r23, its
    // items in r24, the item in r25, the menu id in r26 and the array in r29.
    std::uint32_t offset = 0;
    c.set({{30, 0}, {29, texts}, {27, 0}});
    for (std::int16_t id = 2;; ++id) {
        c.set(26, extended(id));
        const auto menu = os.GetMenuHandle(id, 0x1076fac8);
        c.set(23, menu);
        if (!nonzero(c, std::int32_t(menu))) return;
        const auto items = os.CountMenuItems(menu, 0x1076fa4c);
        c.set({{28, offset}, {24, extended(items)}, {31, texts + offset}, {25, 1}});
        for (std::int16_t item = 1; compare(c, item, items) <= 0; c.set(25, extended(++item))) {
            const auto text = c.u32(texts + offset);
            if (!nonzero_logical(c, text)) continue;
            offset += 4;
            c.set({{27, offset}, {28, c.r(28) + 4}, {31, c.r(31) + 4}, {30, offset}});
            program.localize(localized, text, 0x1076fa90);
            program.copy_to_pascal(pascal, localized, 0x1076fa9c);
            os.SetMenuItemText(menu, item, pascal, 0x1076faac);
        }
    }
}

void find_folders(Cpu& c) {
    Frame frame(c, 0x1076faf0, 0x400, {.first_gpr = 20});
    Toolbox os{c, frame};
    Program program{c};
    Shared shared{c};
    const FSSpec application{c, c.toc(slot::application)}, spec{c, frame.at(0x88)};
    const auto pool = c.toc(slot::pool), name = frame.at(0x2d0);
    // The original keeps the pool in r20, the data folder's volume and
    // directory id fields in r29 and r30 and the application's FSSpec in r31;
    // the routines it calls save them.
    c.set({{29, shared.data_volume().address()}, {30, shared.data_directory().address()}, {31, application.address},
           {20, pool}});

    // The preferences file in the system's Preferences folder, created when missing.
    const Field<std::int16_t> volume{c, frame.at(0x3a)};
    const Field<std::int32_t> directory{c, frame.at(0x3c)};
    if (!nonzero(c, os.FindFolder(-0x8000, four_char('p', 'r', 'e', 'f'), true, volume.address(), directory.address(), 0x1076fb2c))) {  // kOnSystemDisk, created
        os.GetIndString(name, 1000, 2, 0x1076fb48);
        auto error = os.FSMakeFSSpec(volume, directory, name, spec, 0x1076fb60);
        if (compare(c, error, -43) == 0) {  // fnfErr
            const auto creator = c.u32(c.toc(slot::process) + 0x14);  // processSignature
            os.FSpCreateResFile(spec, creator, four_char('P', 'R', 'E', 'F'), -1, 0x1076fb8c);  // smSystemScript
            error = os.FSMakeFSSpec(volume, directory, name, spec, 0x1076fba4);
        }
        if (!nonzero(c, error)) os.FSpOpenResFile(spec, 0, 0x1076fbbc);  // fsCurPerm
    }

    // The game's data folder: next to the application (an alias of it will
    // do), else ":<name 3>:<folder>:" from the root, else created; failing
    // all, the application's own folder.
    os.GetIndString(name, 1000, 4, 0x1076fbd0);
    c.set(28, application.address + 2);
    auto error = os.FSMakeFSSpec(application.volume(), application.directory(), name, spec, 0x1076fbec);
    if (compare(c, error, -43) == 0) {
        const auto path = frame.at(0x1d0);
        os.GetIndString(path, 1000, 3, 0x1076fc0c);
        for (const auto& [part, back] : std::array<std::pair<std::uint32_t, std::uint32_t>, 3>{
                 {{pool + text::colon, 0x1076fc1c}, {name, 0x1076fc28}, {pool + text::colon, 0x1076fc34}}}) {
            program.append_pascal(path, part, back);
        }
        const FSSpec root{c, frame.at(0x40)};
        error = os.FSMakeFSSpec(0, 0, path, root, 0x1076fc48);
        if (!nonzero(c, error)) {
            // The original copies the FSSpec through r20-r27, r12-r4 and r0.
            constexpr unsigned through[]{20, 21, 22, 23, 24, 25, 26, 27, 12, 11, 10, 9, 8, 7, 6, 5, 4};
            c.set(20, extended(root.volume()));
            for (unsigned n = 1; n < std::size(through); ++n) c.set(through[n], c.u32(root.address + 4 * n - 2));
            spec.volume() = root.volume().get();
            for (std::uint32_t at = 2; at < FSSpec::size; at += 4) c.put32(spec.address + at, c.u32(root.address + at));
        } else error = os.FSpDirCreate(spec, -1, shared.data_directory().address(), 0x1076fcf8);
    }
    if (!nonzero(c, error)) {
        error = os.ResolveAliasFile(spec, true, frame.at(0x39), frame.at(0x38), 0x1076fd18);
        if (!nonzero(c, error)) {
            error = program.directory_id(spec, shared.data_directory().address(), 0x1076fd30);
            shared.data_volume() = spec.volume().get();
        }
    }
    if (nonzero(c, error)) {
        shared.data_directory() = application.directory().get();
        shared.data_volume() = application.volume().get();
    }

    // The application's Finder comment holds the game's start-up arguments.
    const auto comment = frame.at(0xd0);
    if (!nonzero(c, program.finder_comment(application, comment, 0x1076fd5c)))
        program.copy_to_c(Globals{c}.command_line(), comment, 0x1076fd70);
}

void install_window_handlers(Cpu& c) {
    Frame frame(c, 0x1076fd90, 0x60, {.first_gpr = 29});
    Toolbox os{c, frame};
    // The event types on the stack: {wind 2} at sp+0x38, {mous 10} {wind 0x55} at sp+0x40.
    const auto bounds = c.toc(slot::bounds_type), clicks = c.toc(slot::click_types);
    for (std::uint32_t at = 0; at < 8; at += 4) c.put32(frame.at(0x38 + std::int32_t(at)), c.u32(bounds + at));
    for (std::uint32_t at = 0; at < 16; at += 4) c.put32(frame.at(0x40 + std::int32_t(at)), c.u32(clicks + at));
    const auto handler = c.toc(slot::window_handler);
    const auto window = Shared{c}.game_window();
    // The original keeps the handler, the window's address and the UPP in r29-r31.
    c.set({{29, handler}, {30, window.address()}});
    const auto install = [&](std::uint32_t count, std::int32_t types, std::uint32_t upp_back, std::uint32_t target_back,
                             std::uint32_t install_back) {
        const auto upp = os.NewEventHandlerUPP(handler, upp_back);
        c.set(31, upp);
        const auto target = os.GetWindowEventTarget(window, target_back);
        os.InstallEventHandler(target, upp, count, frame.at(types), 0, 0, install_back);
    };
    install(1, 0x38, 0x1076fdf0, 0x1076fe00, 0x1076fe1c);  // window bounds
    c.link(0x1076fe24);
    if (nonzero(c, std::int8_t(result(c, aqua_menus(c))))) install(2, 0x40, 0x1076fe34, 0x1076fe44, 0x1076fe60);  // wheel, clicks
}

std::uint32_t aqua_menus(Cpu& c) {
    Frame frame(c, 0x10770ee0, 0x40, {});
    Toolbox os{c, frame};
    Globals g{c};
    if (!nonzero(c, g.menu_checked())) {
        g.menu_features() = -1;
        g.menu_checked() = 1;
    }
    if (compare(c, g.menu_features(), -1) == 0 &&
        nonzero(c, os.Gestalt(four_char('m', 'e', 'n', 'u'), g.menu_features().address(), 0x10770f24)))
        g.menu_features() = 0;
    return std::uint32_t(std::int32_t(std::int8_t(g.menu_features() & 2)));
}
}
