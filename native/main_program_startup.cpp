#include "main_program_detail.hpp"
#include <iterator>

namespace cm::aot::main_program {
using namespace detail;
namespace {
// The main loop, from setjmp's return (0x1076f374): a fatal exit returns
// here too, with the code it was given.
void main_loop(Cpu& c, const Frame& frame) {
    c.set(31, c.r(3));
    const auto text = word(c, global::setup_text);
    c.set(0, text);
    if (compare_logical(c, text) != 0) {
        // Documents dropped on the application before it started: run their
        // setup lines as a script, then stop.
        c.set(3, text);
        c.link(0x1076f38c);
        run_script(c, text);
        c.set(3, word(c, global::setup_text));
        toolbox(c, frame, 0x1077fc58, 0x1076f394);  // DisposePtr
        c.set({{3, 0}, {0, 0}});
        c.put32(at(c, global::setup_text), 0);
        c.put8(at(c, global::running), 0);
    }
    while (test8(c, c.u8(at(c, global::running)))) {
        c.call(0x10118ec0, 0x1076f3b0);  // the game's step: false when it ends
        c.put8(at(c, global::running), c.r(3));
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
    const auto pool = c.toc(slot::pool);
    c.set({{31, 0}, {30, pool}, {29, c.toc(slot::message)}});
    c.link(0x1076f308);
    c.set(3, process_information(c));
    if (!test8(c, c.r(3))) return c.r(31);
    c.link(0x1076f314);
    install_handlers(c);
    c.link(0x1076f318);
    initialize(c);
    c.link(0x1076f31c);
    set_up_menus(c);
    c.link(0x1076f320);
    find_folders(c);
    c.set(3, c.toc(slot::settings));
    c.call(0x1074c9a0, 0x1076f328);  // loads the settings
    // Wait for the open-application (or open-documents) event.
    while (!test8(c, c.u8(at(c, global::opened)))) {
        c.link(0x1076f330);
        poll_high_level_events(c);
    }
    c.set(3, word(c, global::command_line));
    c.call(0x10118ef0, 0x1076f344);  // starts the game with the arguments
    if (test8(c, c.r(3))) {
        c.set(3, word(c, global::command_line));
        if (compare_logical(c, c.r(3)) != 0) {
            toolbox(c, frame, 0x1077fc58, 0x1076f35c);  // DisposePtr
            c.set(0, 0);
            c.put32(at(c, global::command_line), 0);
        }
        c.call(0x10747080, 0x1076f36c);
        c.set(3, c.toc(slot::jump_buffer));
        c.call(0x10773eb0, 0x1076f374);  // setjmp
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
        guest::report_assertion(c, {0x1076f3dc, pool, pool + 9, pool + 0x16, frame.at(0x40), frame.at(0x38), 0x1b0});
        c.set(3, c.toc(slot::left_held));
    }
    c.link(0x1076f420);
    shut_down(c);
    return c.r(31);
}

std::uint32_t process_information(Cpu& c) {
    Frame frame(c, 0x1076f440, 0x50, {.first_gpr = 31});
    const auto info = c.toc(slot::process);
    c.set({{31, info}, {4, 0x3c}, {0, c.toc(slot::application)}});
    c.put32(info, 0x3c);             // processInfoLength
    c.put32(info + 0x38, c.r(0));    // processAppSpec
    c.set(3, frame.at(0x3c));
    toolbox(c, frame, 0x1077fb08, 0x1076f46c);  // GetCurrentProcess
    if (test16(c, c.r(3))) return 0;
    c.set({{4, info}, {3, frame.at(0x3c)}});
    toolbox(c, frame, 0x1077fb20, 0x1076f48c);  // GetProcessInformation
    if (test16(c, c.r(3))) return 0;
    c.set({{3, 0x74747363}, {4, frame.at(0x38)}});
    toolbox(c, frame, 0x1077fad8, 0x1076f4b0);  // Gestalt('ttsc')
    c.set(0, test16(c, c.r(3)) ? 0 : std::uint32_t(std::int8_t(c.u32(frame.at(0x38)) & 1)));
    c.put8(at(c, global::speech_available), c.r(0));
    return 1;
}

void install_handlers(Cpu& c) {
    Frame frame(c, 0x1076f4f0, 0x40, {.first_gpr = 31});
    toolbox(c, frame, 0x1077fb38, 0x1076f504);  // InitCursor
    struct Handler {
        std::int32_t vector;
        std::uint32_t id, upp_back, install_back;
    };
    constexpr Handler handlers[]{{slot::open_application_handler, 0x6f617070, 0x1076f510, 0x1076f534},
                                 {slot::open_documents_handler, 0x6f646f63, 0x1076f540, 0x1076f564},
                                 {slot::print_documents_handler, 0x70646f63, 0x1076f570, 0x1076f594},
                                 {slot::quit_handler, 0x71756974, 0x1076f5a0, 0x1076f5c4}};
    for (const auto& handler : handlers) {
        c.set(3, c.toc(handler.vector));
        toolbox(c, frame, 0x1077fb50, handler.upp_back);  // NewAEEventHandlerUPP
        c.set({{4, handler.id}, {5, c.r(3)}, {6, 0}, {3, 0x61657674}, {7, 0}});
        toolbox(c, frame, 0x1077fb68, handler.install_back);  // AEInstallEventHandler('aevt', id)
    }
    for (std::int32_t n = 0; n < 10;) {
        c.set(31, std::uint32_t(n));
        toolbox(c, frame, 0x10780108, 0x1076f5d4);  // MoreMasters
        c.set(31, std::uint32_t(++n));
        compare(c, n, 10);
    }
}

void initialize(Cpu& c) {
    Frame frame(c, 0x1076f600, 0x3d0, {.first_gpr = 29});
    const auto foreground = c.toc(slot::foreground), application = c.toc(slot::application);
    c.set({{3, foreground}, {4, 1}, {30, application}, {29, 0}, {0, 0}});
    c.put8(at(c, global::running), 1);
    c.put8(at(c, global::opened), 0);
    c.put32(at(c, global::sleep_ticks), 0);
    c.put8(foreground, 1);
    toolbox(c, frame, 0x10780120, 0x1076f640);  // NewRgn
    c.set({{4, c.toc(slot::right_button_modifiers)}, {0, 0xb00}});
    c.put32(at(c, global::mouse_region), c.r(3));
    c.put32(c.r(4), 0xb00);  // control and option make a right click
    c.call(0x1074dbf0, 0x1076f658);  // the speech preference
    c.set({{6, c.toc(slot::speech_wanted)}, {0, 0}});
    c.put32(at(c, global::speech_channel), 0);
    c.set({{4, 1}, {5, 1}});
    c.put8(c.r(6), c.r(3));
    c.set(3, frame.at(0x1b4));
    c.put32(at(c, global::setup_text), 0);
    c.call(0x1074d770, 0x1076f67c);
    c.set({{3, c.toc(slot::settings_name)}, {4, frame.at(0x1bd)}});
    c.call(0x1074c610, 0x1076f688);
    c.set(3, 0x400);
    toolbox(c, frame, 0x1077fc70, 0x1076f690);  // NewPtr
    const auto line = c.r(3);
    const bool none = compare_logical(c, line) == 0;
    c.put32(at(c, global::command_line), line);
    if (!none) {
        c.set(0, 0);
        c.put8(line, 0);
    }
    c.set({{3, frame.at(0x38)}, {4, 0}, {5, 0x7a}});
    c.call(0x10778300, 0x1076f6b8);  // memset: the volume parameter block
    // The name of the volume the application is on (indexed PBHGetVInfo).
    c.set(31, frame.at(0xb4));
    for (;;) {
        c.put16(frame.at(0x54), c.r(29));  // ioVolIndex
        c.set({{3, frame.at(0x38)}, {29, c.r(29) + 1}});
        c.put32(frame.at(0x4a), c.r(31));  // ioNamePtr
        toolbox(c, frame, 0x10780000, 0x1076f6d4);  // PBHGetVInfoSync
        if (test16(c, c.r(3))) break;
        const auto volume = c.i16(frame.at(0x4e)), wanted = c.i16(application);
        c.set({{3, std::uint32_t(volume)}, {0, std::uint32_t(wanted)}});
        if (compare(c, volume, wanted) == 0) {
            c.set({{3, c.toc(slot::volume_name)}, {4, frame.at(0xb4)}});
            c.call(0x1074c610, 0x1076f6fc);
            break;
        }
    }
    c.set({{0, std::uint32_t(c.i16(application))}, {3, c.toc(slot::start_volume)}});
    c.put16(c.r(3), c.r(0));
}

void shut_down(Cpu& c) {
    Frame frame(c, 0x1076f730, 0x40, {});
    c.set(3, c.toc(slot::game_started));
    c.set(0, c.u8(c.r(3)));
    if (test8(c, c.r(0))) c.call(0x10118fe0, 0x1076f750);  // ends the game
    c.link(0x1076f754);
    stop_speech(c);
    c.set(3, c.toc(slot::settings));
    c.call(0x1074ca50, 0x1076f75c);
    c.set(3, word(c, global::mouse_region));
    if (compare_logical(c, c.r(3)) != 0) toolbox(c, frame, 0x107812d8, 0x1076f76c);  // DisposeRgn
}

void stop_speech(Cpu& c) {
    Frame frame(c, 0x1076f780, 0x40, {});
    release_speech(c, frame, 0x1076f79c, 0x1076f7b0);
}

void fatal_exit(Cpu& c, std::uint32_t code) {
    Frame frame(c, 0x1076f7d0, 0x40, {.first_gpr = 31});
    const auto started = c.toc(slot::game_started);
    c.set({{4, started}, {31, code}, {3, c.u8(started)}});
    if (test8(c, c.r(3))) {
        // Back to run(), which shuts down; longjmp does not return.
        c.set({{0, 0}, {3, c.toc(slot::jump_buffer)}});
        c.put8(at(c, global::running), 0);
        c.set(4, c.r(31));
        c.call(0x10773f30, 0x1076f808);
        return;
    }
    // The original tests the cleared flag once more before ending the game.
    if (test8(c, c.r(3))) c.call(0x10118fe0, 0x1076f818);
    release_speech(c, frame, 0x1076f828, 0x1076f83c);
    c.set(3, c.toc(slot::settings));
    c.call(0x1074ca50, 0x1076f850);
    c.set(3, word(c, global::mouse_region));
    if (compare_logical(c, c.r(3)) != 0) toolbox(c, frame, 0x107812d8, 0x1076f860);  // DisposeRgn
    c.set(3, c.r(31));
    c.call(0x10773ff0, 0x1076f86c);  // exit: does not return
}

void nothing(Cpu&) {}

void set_up_menus(Cpu& c) {
    Frame frame(c, 0x1076f890, 0x40, {});
    c.set(3, 1);
    toolbox(c, frame, 0x107801b0, 0x1076f8a4);  // GetNewMBar
    if (compare_logical(c, c.r(3)) != 0) {
        toolbox(c, frame, 0x107801c8, 0x1076f8b4);  // SetMenuBar
        c.link(0x1076f8bc);
        c.set(3, aqua_menus(c));
        if (test8(c, c.r(3))) {
            // Under Aqua the application menu has Quit: remove the File
            // menu's Quit and the separator above it.
            c.set({{6, frame.at(0x3c)}, {4, 0x71756974}, {7, frame.at(0x38)}, {3, 0}, {5, 2}});
            toolbox(c, frame, 0x107801e0, 0x1076f8e0);  // GetIndMenuItemWithCommandID('quit')
            if (compare(c, std::int32_t(c.r(3))) == 0) {
                c.set(0, c.u16(frame.at(0x38)));
                c.set({{3, c.u32(frame.at(0x3c))}, {4, std::uint32_t(std::int16_t(c.r(0)))}});
                toolbox(c, frame, 0x107801f8, 0x1076f8fc);  // DeleteMenuItem
                c.set(4, c.u16(frame.at(0x38)));
                c.set({{3, c.u32(frame.at(0x3c))}, {0, c.r(4) - 1}});
                c.set(4, std::uint32_t(std::int16_t(c.r(0))));
                toolbox(c, frame, 0x107801f8, 0x1076f914);
            }
        }
        c.set(3, 1);
        toolbox(c, frame, 0x10780210, 0x1076f920);  // GetMenuHandle
        if (compare_logical(c, c.r(3)) != 0) {
            c.set(4, 0x44525652);
            toolbox(c, frame, 0x10780228, 0x1076f938);  // AppendResMenu('DRVR')
        }
    }
    c.link(0x1076f940);
    remember_menu_texts(c);
    toolbox(c, frame, 0x10780270, 0x1076f944);  // InvalMenuBar
}

void remember_menu_texts(Cpu& c) {
    Frame frame(c, 0x1076f960, 0x260, {.first_gpr = 25});
    // r25 texts kept, r26 menu, r27 its items, r28 item, r29 menu id, r30 the
    // array, r31 its byte offset.
    c.set({{30, c.toc(slot::menu_texts)}, {25, 0}, {31, 0}, {29, 2}});
    for (;; c.set(29, c.r(29) + 1)) {
        c.set(3, std::uint32_t(std::int16_t(c.r(29))));
        toolbox(c, frame, 0x10780210, 0x1076f9f4);  // GetMenuHandle
        c.set(26, c.r(3));
        if (compare(c, std::int32_t(c.r(3))) == 0) return;
        toolbox(c, frame, 0x10780240, 0x1076f98c);  // CountMenuItems
        c.set({{27, std::uint32_t(std::int16_t(c.r(3)))}, {28, 1}});
        for (; compare(c, std::int32_t(c.r(28)), std::int32_t(c.r(27))) <= 0; c.set(28, c.r(28) + 1)) {
            c.set({{3, c.r(26)}, {4, std::uint32_t(std::int16_t(c.r(28)))}, {5, frame.at(0x138)}});
            toolbox(c, frame, 0x10780258, 0x1076f9b0);  // GetMenuItemText
            c.set({{3, frame.at(0x38)}, {4, frame.at(0x138)}});
            c.call(0x1074c610, 0x1076f9c0);  // to a C string
            c.set(3, frame.at(0x38));
            c.call(0x107395f0, 0x1076f9c8);  // a copy
            c.set(25, c.r(25) + 1);
            c.put32(c.r(30) + c.r(31), c.r(3));
            const bool full = compare(c, std::int32_t(c.r(25)), 50) == 0;
            c.set(31, c.r(31) + 4);
            if (full) return;
        }
    }
}

void localize_menus(Cpu& c) {
    Frame frame(c, 0x1076fa20, 0x260, {.first_gpr = 23});
    // r23 menu, r24 its items, r25 item, r26 menu id, r29 the array; r27, r28,
    // r30 and r31 advance by one entry per item that has a text. An item
    // without one stops the walk through the array: the next items all read
    // that same empty entry (a quirk of the original).
    c.set({{30, 0}, {29, c.toc(slot::menu_texts)}, {27, 0}, {26, 2}});
    for (;; c.set(26, c.r(26) + 1)) {
        c.set(3, std::uint32_t(std::int16_t(c.r(26))));
        toolbox(c, frame, 0x10780210, 0x1076fac8);  // GetMenuHandle
        c.set(23, c.r(3));
        if (compare(c, std::int32_t(c.r(3))) == 0) return;
        toolbox(c, frame, 0x10780240, 0x1076fa4c);  // CountMenuItems
        c.set({{28, c.r(27)}, {24, std::uint32_t(std::int16_t(c.r(3)))}, {31, c.r(29) + c.r(27)}, {25, 1}});
        for (; compare(c, std::int32_t(c.r(25)), std::int32_t(c.r(24))) <= 0; c.set(25, c.r(25) + 1)) {
            c.set(0, c.u32(c.r(31)));
            if (compare_logical(c, c.r(0)) == 0) continue;
            c.set({{4, c.u32(c.r(29) + c.r(30))}, {3, frame.at(0x38)}, {27, c.r(27) + 4}, {28, c.r(28) + 4},
                   {31, c.r(31) + 4}, {30, c.r(30) + 4}});
            c.call(0x10727890, 0x1076fa90);  // the text in the game's language
            c.set({{3, frame.at(0x138)}, {4, frame.at(0x38)}});
            c.call(0x1074c5d0, 0x1076fa9c);  // to a Pascal string
            c.set({{3, c.r(23)}, {4, std::uint32_t(std::int16_t(c.r(25)))}, {5, frame.at(0x138)}});
            toolbox(c, frame, 0x107807e0, 0x1076faac);  // SetMenuItemText
        }
    }
}

void find_folders(Cpu& c) {
    Frame frame(c, 0x1076faf0, 0x400, {.first_gpr = 20});
    const auto application = c.toc(slot::application), pool = c.toc(slot::pool);
    // r28 the application's directory id, r29 and r30 the data folder's
    // volume and directory id, r31 the application's FSSpec.
    c.set({{29, c.toc(slot::data_volume)}, {4, 0x70726566}, {30, c.toc(slot::data_directory)}, {31, application},
           {3, std::uint32_t(-0x8000)}, {20, pool}, {5, 1}, {6, frame.at(0x3a)}, {7, frame.at(0x3c)}});
    toolbox(c, frame, 0x10780288, 0x1076fb2c);  // FindFolder(kOnSystemDisk, 'pref', create)
    if (!test16(c, c.r(3))) {
        c.set({{3, frame.at(0x2d0)}, {4, 0x3e8}, {5, 2}});
        toolbox(c, frame, 0x107802a0, 0x1076fb48);  // GetIndString: the preferences file's name
        const auto make_spec = [&](std::uint32_t back) {
            c.set({{3, std::uint32_t(c.i16(frame.at(0x3a)))}, {5, frame.at(0x2d0)}, {4, c.u32(frame.at(0x3c))},
                   {6, frame.at(0x88)}});
            toolbox(c, frame, 0x1077fee0, back);  // FSMakeFSSpec
        };
        make_spec(0x1076fb60);
        c.set(0, std::uint32_t(std::int16_t(c.r(3))));
        if (compare(c, std::int16_t(c.r(3)), -0x2b) == 0) {  // fnfErr: create it
            const auto process = c.toc(slot::process);
            c.set({{5, 0x50524546}, {3, frame.at(0x88)}, {6, std::uint32_t(-1)}, {4, c.u32(process + 0x14)}});
            toolbox(c, frame, 0x107802b8, 0x1076fb8c);  // FSpCreateResFile(creator, 'PREF')
            make_spec(0x1076fba4);
        }
        if (!test16(c, c.r(3))) {
            c.set({{3, frame.at(0x88)}, {4, 0}});
            toolbox(c, frame, 0x107802d0, 0x1076fbbc);  // FSpOpenResFile
        }
    }
    // The data folder next to the application, or an alias of it.
    c.set({{3, frame.at(0x2d0)}, {4, 0x3e8}, {5, 4}});
    toolbox(c, frame, 0x107802a0, 0x1076fbd0);  // GetIndString: the folder's name
    c.set({{28, application + 2}, {3, std::uint32_t(c.i16(application))}, {4, c.u32(application + 2)},
           {5, frame.at(0x2d0)}, {6, frame.at(0x88)}});
    toolbox(c, frame, 0x1077fee0, 0x1076fbec);  // FSMakeFSSpec
    c.set(0, std::uint32_t(std::int16_t(c.r(3))));
    if (compare(c, std::int16_t(c.r(3)), -0x2b) == 0) {
        // Not there: ":<name 3>:<folder>:" from the root, or create the folder.
        c.set({{3, frame.at(0x1d0)}, {4, 0x3e8}, {5, 3}});
        toolbox(c, frame, 0x107802a0, 0x1076fc0c);  // GetIndString
        c.set({{3, frame.at(0x1d0)}, {4, pool + 0x1c}});
        c.call(0x1074c690, 0x1076fc1c);  // append ":"
        c.set({{3, frame.at(0x1d0)}, {4, frame.at(0x2d0)}});
        c.call(0x1074c690, 0x1076fc28);
        c.set({{3, frame.at(0x1d0)}, {4, pool + 0x1c}});
        c.call(0x1074c690, 0x1076fc34);
        c.set({{5, frame.at(0x1d0)}, {6, frame.at(0x40)}, {3, 0}, {4, 0}});
        toolbox(c, frame, 0x1077fee0, 0x1076fc48);  // FSMakeFSSpec
        if (!test16(c, c.r(3))) {
            // The FSSpec at sp+0x40 to sp+0x88, through the registers.
            constexpr unsigned through[]{21, 22, 23, 24, 25, 26, 27, 12, 11, 10, 9, 8, 7, 6, 5, 4, 0};
            c.set(20, std::uint32_t(c.i16(frame.at(0x40))));
            for (unsigned n = 0; n < std::size(through); ++n) c.set(through[n], c.u32(frame.at(0x42 + 4 * std::int32_t(n))));
            c.put16(frame.at(0x88), c.r(20));
            for (unsigned n = 0; n < std::size(through); ++n) c.put32(frame.at(0x8a + 4 * std::int32_t(n)), c.r(through[n]));
        } else {
            c.set({{5, c.r(30)}, {3, frame.at(0x88)}, {4, std::uint32_t(-1)}});
            toolbox(c, frame, 0x107802e8, 0x1076fcf8);  // FSpDirCreate
        }
    }
    if (!test16(c, c.r(3))) {
        c.set({{3, frame.at(0x88)}, {5, frame.at(0x39)}, {6, frame.at(0x38)}, {4, 1}});
        toolbox(c, frame, 0x1077ffa0, 0x1076fd18);  // ResolveAliasFile
        if (!test16(c, c.r(3))) {
            c.set({{4, c.r(30)}, {3, frame.at(0x88)}});
            c.call(0x1074d4c0, 0x1076fd30);  // the folder's directory id
            c.set(0, std::uint32_t(c.i16(frame.at(0x88))));
            c.put16(c.r(29), c.r(0));
        }
    }
    if (test16(c, c.r(3))) {
        // Otherwise the application's own folder.
        c.set({{3, c.u32(c.r(28))}, {0, std::uint32_t(c.i16(c.r(31)))}});
        c.put32(c.r(30), c.r(3));
        c.put16(c.r(29), c.r(0));
    }
    // The application's Finder comment holds the game's arguments.
    c.set({{3, c.r(31)}, {4, frame.at(0xd0)}});
    c.call(0x1076f080, 0x1076fd5c);
    if (!test16(c, c.r(3))) {
        c.set({{3, word(c, global::command_line)}, {4, frame.at(0xd0)}});
        c.call(0x1074c610, 0x1076fd70);
    }
}

void install_window_handlers(Cpu& c) {
    Frame frame(c, 0x1076fd90, 0x60, {.first_gpr = 29});
    const auto clicks = c.toc(slot::click_specs), bounds = c.toc(slot::bounds_spec);
    // r29 the handler's TVector, r30 the game window, r31 its UPP.
    c.set({{29, c.toc(slot::window_handler)}, {30, c.toc(slot::game_window)}});
    c.set({{6, c.u32(clicks)}, {8, c.u32(bounds)}, {7, c.u32(bounds + 4)}, {5, c.u32(clicks + 4)},
           {4, c.u32(clicks + 8)}, {0, c.u32(clicks + 12)}, {3, c.r(29)}});
    // The event types on the stack: {wind 2} at sp+0x38, {mous 10} {wind 0x55} at sp+0x40.
    c.put32(frame.at(0x38), c.r(8));
    c.put32(frame.at(0x3c), c.r(7));
    c.put32(frame.at(0x40), c.r(6));
    c.put32(frame.at(0x44), c.r(5));
    c.put32(frame.at(0x48), c.r(4));
    c.put32(frame.at(0x4c), c.r(0));
    const auto install = [&](std::uint32_t upp_back, std::uint32_t target_back, std::uint32_t install_back,
                             std::uint32_t count, std::int32_t types) {
        c.set(3, c.r(29));
        toolbox(c, frame, 0x10780cc0, upp_back);  // NewEventHandlerUPP
        c.set({{31, c.r(3)}, {3, c.u32(c.r(30))}});
        toolbox(c, frame, 0x10780cd8, target_back);  // GetWindowEventTarget
        c.set({{4, c.r(31)}, {6, frame.at(types)}, {5, count}, {7, 0}, {8, 0}});
        toolbox(c, frame, 0x10780cf0, install_back);  // InstallEventHandler
    };
    install(0x1076fdf0, 0x1076fe00, 0x1076fe1c, 1, 0x38);
    c.link(0x1076fe24);
    c.set(3, aqua_menus(c));
    if (test8(c, c.r(3))) install(0x1076fe34, 0x1076fe44, 0x1076fe60, 2, 0x40);
}

std::uint32_t aqua_menus(Cpu& c) {
    Frame frame(c, 0x10770ee0, 0x40, {});
    c.set(0, c.u8(at(c, global::menu_checked)));
    if (!test8(c, c.r(0))) {
        c.set({{3, std::uint32_t(-1)}, {0, 1}});
        c.put32(at(c, global::menu_features), std::uint32_t(-1));
        c.put8(at(c, global::menu_checked), 1);
    }
    c.set(0, word(c, global::menu_features));
    if (compare(c, std::int32_t(c.r(0)), -1) == 0) {
        c.set({{3, 0x6d656e75}, {4, at(c, global::menu_features)}});
        toolbox(c, frame, 0x1077fad8, 0x10770f24);  // Gestalt('menu')
        if (test16(c, c.r(3))) {
            c.set(0, 0);
            c.put32(at(c, global::menu_features), 0);
        }
    }
    c.set(0, word(c, global::menu_features) & 2);
    return std::uint32_t(std::int32_t(std::int8_t(c.r(0))));
}
}
