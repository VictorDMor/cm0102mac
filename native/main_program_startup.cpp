#include "main_program_detail.hpp"
#include <array>
#include <iterator>

namespace cm::aot::main_program {
using namespace detail;
namespace {

// The main loop, from setjmp's return (0x1076f374): a fatal exit returns
// here too, with the code it was given, which becomes run()'s result.
void main_loop(Cpu& c, const Frame& frame, Kept<31>& code) {
    Globals g{c};
    Toolbox os{c, frame};
    Program program{c};
    code = c.r(3);
    if (const std::uint32_t text = g.setup_text(); nonzero_logical(c, text)) {
        // Documents dropped on the application before it started: run their
        // setup lines as a script, then stop.
        enter(c, 0x1076f38c, {text});
        run_script(c, text);
        os.DisposePtr(g.setup_text(), 0x1076f394);
        c.set(3, 0);  // the original clears it through r3
        g.setup_text() = 0;
        g.running() = 0;
    }
    while (nonzero(c, g.running())) {
        g.running() = program.step(0x1076f3b0);
        enter(c, 0x1076f3b8);
        pump_events(c);
    }
}
}

void main(Cpu& c) {
    Frame frame(c, 0x1076f2b0, 0x40, {});
    enter(c, 0x1076f2c0);
    run(c);
    leave(c, {{3, 0}});
}

std::uint32_t run(Cpu& c) {
    Frame frame(c, 0x1076f2e0, 0x70, {.first_gpr = 29});
    Globals g{c};
    Toolbox os{c, frame};
    Program program{c};
    Kept<31> code{c, 0};  // the result
    const Kept<30> pool{c, c.toc(slot::pool)};
    const Kept<29> message{c, c.toc(slot::message)};
    enter(c, 0x1076f308);
    if (!nonzero(c, std::int8_t(result(c, process_information(c))))) return 0;
    enter(c, 0x1076f314);
    install_handlers(c);
    enter(c, 0x1076f318);
    initialize(c);
    enter(c, 0x1076f31c);
    set_up_menus(c);
    enter(c, 0x1076f320);
    find_folders(c);
    program.load_settings(c.toc(slot::settings), 0x1076f328);
    // The game starts once the open-application (or open-documents) event came.
    while (!nonzero(c, g.opened())) {
        enter(c, 0x1076f330);
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
                main_loop(c, frame, code);
                break;
            } catch (const GuestContextTransfer& transfer) {
                if (transfer.state != &c.s || c.s.pc != 0x1076f374 || c.sp() != frame.base()) throw;
            }
        }
    } else {
        guest::report_assertion(c, {0x1076f3dc, pool + text::file_name, pool + text::assert_format,
                                    pool + text::error_title, frame.at(0x40), frame.at(0x38), 0x1b0});
    }
    enter(c, 0x1076f420);
    shut_down(c);
    return code;
}

std::uint32_t process_information(Cpu& c) {
    Frame frame(c, 0x1076f440, 0x50, {.first_gpr = 31});
    Toolbox os{c, frame};
    // ProcessInfoRec: processInfoLength, and the application's FSSpec at +0x38.
    const Kept<31> info{c, c.toc(slot::process)};
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
    Kept<31> count{c, 0};
    do {
        os.MoreMasters(0x1076f5d4);
        count = count + 1;
    } while (compare(c, std::int32_t(count), 10) < 0);
}

void initialize(Cpu& c) {
    Frame frame(c, 0x1076f600, 0x3d0, {.first_gpr = 29});
    Toolbox os{c, frame};
    Program program{c};
    Globals g{c};
    Shared shared{c};
    const FSSpec application{c, c.toc(slot::application)};
    const Kept<30> kept_application{c, application.address};
    Kept<29> next_volume{c, 0};
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
    const Kept<31> kept_name{c, name};
    for (std::int16_t index = 0;; ++index) {
        c.put16(block + 0x1c, std::uint32_t(index));
        c.put32(block + 0x12, name);
        next_volume = std::uint32_t(index + 1);
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
    c.set(3, started.address());  // in r3 either way
    if (nonzero(c, started)) program.end_game(started.address(), 0x1076f750);
    enter(c, 0x1076f754);
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
    const Kept<31> kept_code{c, code};
    c.set({{4, started.address()}, {3, std::uint8_t(started)}});  // the flag's address and value
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
        enter(c, 0x1076f8bc);
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
    enter(c, 0x1076f940);
    remember_menu_texts(c);
    os.InvalMenuBar(0x1076f944);
}

void remember_menu_texts(Cpu& c) {
    Frame frame(c, 0x1076f960, 0x260, {.first_gpr = 25});
    Toolbox os{c, frame};
    Program program{c};
    const auto pascal = frame.at(0x138), copy = frame.at(0x38);
    const Kept<30> texts{c, c.toc(slot::menu_texts)};
    Kept<25> count{c, 0};
    Kept<31> offset{c, 0};
    for (std::int16_t id = 2;; ++id) {
        const Kept<29> menu_id{c, extended(id)};
        const Kept<26> menu{c, os.GetMenuHandle(id, 0x1076f9f4)};
        if (!nonzero(c, std::int32_t(menu))) return;
        const Kept<27> items{c, extended(os.CountMenuItems(menu, 0x1076f98c))};
        for (Kept<28> item{c, 1}; compare(c, std::int32_t(item), std::int32_t(items)) <= 0; item = item + 1) {
            os.GetMenuItemText(menu, std::int16_t(item), pascal, 0x1076f9b0);
            program.copy_to_c(copy, pascal, 0x1076f9c0);
            const auto text = program.duplicate(copy, 0x1076f9c8);
            count = count + 1;
            c.put32(texts + offset, text);
            const bool full = compare(c, std::int32_t(count), 50) == 0;  // at most 50
            offset = offset + 4;
            if (full) return;
        }
    }
}

void localize_menus(Cpu& c) {
    Frame frame(c, 0x1076fa20, 0x260, {.first_gpr = 23});
    Toolbox os{c, frame};
    Program program{c};
    const auto localized = frame.at(0x38), pascal = frame.at(0x138);
    // The next remembered text, by offset; it advances only past items that
    // have one. An item without one stops the walk: the later items all read
    // that same empty entry (a quirk of the original). The original keeps the
    // offset twice, and per menu the offset and the text's address again.
    Kept<30> offset_copy{c, 0};
    const Kept<29> texts{c, c.toc(slot::menu_texts)};
    Kept<27> offset{c, 0};
    for (std::int16_t id = 2;; ++id) {
        const Kept<26> menu_id{c, extended(id)};
        const Kept<23> menu{c, os.GetMenuHandle(id, 0x1076fac8)};
        if (!nonzero(c, std::int32_t(menu))) return;
        const auto items = os.CountMenuItems(menu, 0x1076fa4c);
        Kept<28> menu_offset{c, offset};
        const Kept<24> menu_items{c, extended(items)};
        Kept<31> menu_text{c, texts + offset};
        for (Kept<25> item{c, 1}; compare(c, std::int32_t(item), items) <= 0; item = item + 1) {
            const auto text = c.u32(texts + offset);
            if (!nonzero_logical(c, text)) continue;
            offset = offset + 4;
            menu_offset = menu_offset + 4;
            menu_text = menu_text + 4;
            offset_copy = offset;
            program.localize(localized, text, 0x1076fa90);
            program.copy_to_pascal(pascal, localized, 0x1076fa9c);
            os.SetMenuItemText(menu, std::int16_t(item), pascal, 0x1076faac);
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
    const Kept<29> data_volume{c, shared.data_volume().address()};
    const Kept<30> data_directory{c, shared.data_directory().address()};
    const Kept<31> kept_application{c, application.address};
    const Kept<20> kept_pool{c, pool};

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
    const Kept<28> application_directory{c, application.address + 2};
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
    const Kept<29> handler{c, c.toc(slot::window_handler)};
    const auto window = Shared{c}.game_window();
    const Kept<30> window_field{c, window.address()};
    const auto install = [&](std::uint32_t count, std::int32_t types, std::uint32_t upp_back, std::uint32_t target_back,
                             std::uint32_t install_back) {
        const Kept<31> upp{c, os.NewEventHandlerUPP(handler, upp_back)};
        const auto target = os.GetWindowEventTarget(window, target_back);
        os.InstallEventHandler(target, upp, count, frame.at(types), 0, 0, install_back);
    };
    install(1, 0x38, 0x1076fdf0, 0x1076fe00, 0x1076fe1c);  // window bounds
    enter(c, 0x1076fe24);
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
