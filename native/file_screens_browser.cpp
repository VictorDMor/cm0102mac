#include "file_screens_detail.hpp"

namespace cm::aot::file_screens {
using namespace detail;
namespace {
// Values of the file dialog's screen history.
enum Value : std::uint32_t {
    v_heading = 0, v_prompt = 1, v_mode = 2, v_folder = 3, v_extension = 4, v_filter = 5, v_folder_copy = 6, v_name = 7,
    v_path = 8, v_question = 9, v_text_colour = 10, v_back_colour = 11, v_parent = 12, v_folders_only = 13, v_row = 14, v_list = 15,
    v_unused = 16, v_plain_cancel = 17, v_info_button = 18,
};
enum Mode : std::uint32_t { load_mode = 0, save_mode = 1, delete_mode = 2 };
enum Question : std::int32_t { create_folder_question = 1, delete_question = 2, replace_question = 3 };
enum Action : std::int32_t { check_folder_action = 1, pick_action = 2, cancel_action = 3, ok_action = 4, info_action = 5 };
constexpr std::int32_t stay = -4, redraw = -11;

// The name with the expected extension, unless it already has it: the
// compare's zero and a missing extension both leave the path without one.
void build_path(Cpu& c, std::uint32_t back, std::uint32_t output, std::uint32_t folder, std::uint32_t file, std::uint32_t extension,
                std::uint32_t scratch, unsigned flag_register) {
    Program p{c};
    p.call(split_path, back, {file, 0, 0, 0, scratch});
    const bool has_extension = compare_logical(c, extension) != 0;
    c.set(flag_register, 0);
    if (has_extension && compare(c, std::int32_t(p.call(compare_nocase, back + 0x18, {scratch, extension}))) != 0)
        c.set(flag_register, 1);
    const bool add = compare(c, std::int32_t(c.r(flag_register))) != 0;
    p.call(make_path, back + 0x4c, {output, 0, folder, file, add ? extension : 0});
}
// The buffers the dialog allocated go, and the screen below gets its answer.
void finish(Cpu& c, std::uint32_t back, std::uint32_t screens, std::uint32_t answer_address, std::uint32_t answer,
            std::uint32_t folder_copy, std::uint32_t name) {
    Program p{c};
    p.call(release, back, {folder_copy});
    p.call(release, back + 8, {name});
    const auto parent = p.value(back + 0x14, screens, v_parent);
    c.set(4, screens + screens_high);
    c.put32(screens + screens_high - returned_value, parent);
    c.set(3, std::uint32_t(closed));
    c.put32(answer_address, answer);
}
}

void open_file_dialog(Cpu& c, const FileDialog& d) {
    Frame f(c, 0x100edf20, 0xb0, {.first_gpr = 15});
    const auto caller = f.caller();
    const auto screens = c.toc(slot::screens), pool = c.toc(slot::pool);
    const auto back_colour = extended(c.i16(caller + 0x3a));
    const auto parent = c.u32(caller + 0x3c);
    const auto folders_only = c.u8(caller + 0x43), unused = c.u8(caller + 0x47), plain_cancel = c.u8(caller + 0x4b),
               info_button = c.u8(caller + 0x4f);
    keep(c, {{26, screens}, {27, d.heading}, {28, d.prompt}, {29, d.mode}, {30, d.folder}, {31, d.extension}, {18, d.filter},
             {19, d.text_colour}, {20, back_colour}, {21, parent}, {22, folders_only}, {23, unused}, {24, plain_cancel}, {25, info_button}});
    Kept<15> folder_copy(c, c.toc(slot::message));
    Kept<16> name(c, pool);
    Kept<17> path(c, d.preset);
    if (compare(c, std::int32_t(d.heading)) == 0 || compare_logical(c, d.prompt) == 0 || compare_logical(c, d.folder) == 0) {
        report_assertion(c, {0x100edf9c, pool, pool + text::assertion_format, pool + text::assertion_title, f.at(0x40), f.at(0x38), 0x77});
        c.set(3, c.toc(slot::success));
        return;
    }
    Program p{c};
    if (compare(c, std::int8_t(p.call(append_history, 0x100edffc, {screens, c.toc(slot::dialog_render), c.toc(slot::dialog_action), 1, 0, 0}))) == 0)
        return;
    folder_copy = p.call(allocate, 0x100ee00c, {0x100});
    p.call(string_copy_limited, 0x100ee01c, {folder_copy, d.folder, 0x100});
    c.put8(folder_copy + 0xff, 0);
    name = p.call(allocate, 0x100ee02c, {0x1f});
    if (compare_logical(c, d.mode & 0xff, save_mode) == 0 && compare_logical(c, d.preset) != 0) {
        p.call(string_copy_limited, 0x100ee050, {name, d.preset, 0x1f});
        c.put8(name + 0x1e, 0);
    } else {
        c.put8(name, 0);
    }
    path = p.call(allocate, 0x100ee06c, {0x100});
    c.put8(path, 0);
    p.store(0x100ee08c, screens, v_heading, d.heading, 1);
    p.store(0x100ee0a0, screens, v_prompt, d.prompt, 1);
    p.store(0x100ee0b4, screens, v_mode, d.mode & 0xff);
    p.store(0x100ee0c8, screens, v_folder, d.folder);
    p.store(0x100ee0e8, screens, v_extension, d.extension, d.extension ? 1 : 0);
    p.store(0x100ee0fc, screens, v_filter, d.filter);
    p.store(0x100ee110, screens, v_folder_copy, folder_copy);
    p.store(0x100ee124, screens, v_name, name);
    p.store(0x100ee138, screens, v_path, path);
    p.store(0x100ee14c, screens, v_question, 0);
    p.store(0x100ee160, screens, v_text_colour, extended(std::int16_t(d.text_colour)));
    p.store(0x100ee174, screens, v_back_colour, back_colour);
    p.store(0x100ee188, screens, v_parent, parent);
    p.store(0x100ee19c, screens, v_folders_only, std::uint32_t(std::int32_t(std::int8_t(folders_only))));
    p.store(0x100ee1b0, screens, v_row, 0);
    p.store(0x100ee1c4, screens, v_list, 0);
    p.store(0x100ee1d8, screens, v_unused, std::uint32_t(std::int32_t(std::int8_t(unused))));
    p.store(0x100ee1ec, screens, v_plain_cancel, std::uint32_t(std::int32_t(std::int8_t(plain_cancel))));
    p.store(0x100ee200, screens, v_info_button, std::uint32_t(std::int32_t(std::int8_t(info_button))));
}

void render_file_dialog(Cpu& c) {
    Frame f(c, 0x100ee220, 0x4d0, {.first_gpr = 14});
    const auto screens = c.toc(slot::screens), label = c.toc(slot::text), pool = c.toc(slot::pool);
    const auto marked = c.toc(slot::marked_colour), folder_colour = c.toc(slot::folder_colour), row_colour = c.toc(slot::row_colour),
               plain = c.toc(slot::plain_colour);
    keep(c, {{29, screens}, {19, marked}, {26, folder_colour}, {20, row_colour}, {27, plain}, {28, label}, {30, pool}});
    const auto wide_columns = f.at(0x6c), two_columns = f.at(0x68), three_columns = f.at(0x70);
    c.set(6, c.u16(c.r(2) - 0x39c4));
    c.set(5, c.u16(c.r(2) - 0x39c0));
    c.put16(wide_columns, c.u16(c.r(2) - 0x39c8));
    c.put16(two_columns, c.r(6));
    c.put16(three_columns, c.r(5));
    c.put8(three_columns + 2, c.u8(c.r(2) - 0x39be));
    Program p{c};
    Kept<24> heading_or_windows(c, p.value(0x100ee278, screens, v_heading));
    Kept<25> prompt_or_current(c, p.value(0x100ee28c, screens, v_prompt));
    Kept<23> folder(c, p.value(0x100ee2a0, screens, v_folder));
    Kept<17> extension(c, p.value(0x100ee2b4, screens, v_extension));
    Kept<22> filter(c, p.value(0x100ee2c8, screens, v_filter));
    Kept<18> name_or_list(c, p.value(0x100ee2dc, screens, v_name));
    Kept<16> text_colour_or_flags(c, p.value(0x100ee2ec, screens, v_text_colour) & 0xffff);
    Kept<15> back_colour_or_rows(c, p.value(0x100ee2fc, screens, v_back_colour) & 0xffff);
    Kept<21> folders_only(c, std::uint32_t(std::int32_t(std::int8_t(p.value(0x100ee30c, screens, v_folders_only)))));
    p.value(0x100ee31c, screens, v_unused);
    Kept<31> plain_cancel(c, std::uint32_t(std::int32_t(std::int8_t(p.value(0x100ee328, screens, v_plain_cancel)))));
    Kept<14> info_button_or_grid(c, std::uint32_t(std::int32_t(std::int8_t(p.value(0x100ee338, screens, v_info_button)))));
    c.call(begin_screen, 0x100ee340);
    p.call(set_screen_colours, 0x100ee354, {screens, back_colour_or_rows, text_colour_or_flags, heading_or_windows});
    p.call(set_panel, 0x100ee360, {4, 0});
    p.call(set_screen_title, 0x100ee36c, {screens, prompt_or_current});
    prompt_or_current = screens + 0x2c58;
    heading_or_windows = screens + 0x24;
    p.stack(f.base(), {0, 2, 0, none});
    back_colour_or_rows = p.call(add_table, 0x100ee3bc, {window(c, screens), 0xbe, 0xc3, 0x2bc, 0xe7, 2, wide_columns, 1});
    p.call(localized_text, 0x100ee3cc, {label, pool + text::file_label});
    p.stack(f.base(), {c.u16(plain), label});
    p.call(add_cell, 0x100ee3fc, {screens, back_colour_or_rows, 0, 0, 1, 0, 0xc, 3});
    p.stack(f.base(), {1, 0, 0, 1, 3, c.u16(c.toc(slot::list_colour)), name_or_list, 0x1e, 0, 0, back_colour_or_rows});
    p.call(add_table_field, 0x100ee46c, {window(c, screens), 0x18, 0, 0, 0, 0, 1, 0});
    p.stack(f.base(), {0, 1, 0, none});
    const auto list = p.call(add_table, 0x100ee4b4, {window(c, screens), 0xbe, 0xea, 0x2bc, 0x1fe, 1, 0, 6});
    const bool only_folders = compare(c, std::int8_t(folders_only)) != 0;
    name_or_list = list;
    const auto search = f.at(0x74), found = f.at(0x174);
    p.call(make_path, only_folders ? 0x100ee4d8 : 0x100ee508, {search, 0, folder, pool + text::any_file, extension});
    p.call(find_first, only_folders ? 0x100ee4e8 : 0x100ee518, {search, found, 0});
    auto finished = std::int8_t(c.r(3));
    back_colour_or_rows = 0;
    while (compare(c, finished) == 0) {
        if (compare(c, std::int8_t(folders_only)) == 0) {
            p.call(make_path, 0x100ee544, {search, 0, folder, found, 0});
            if (compare_logical(c, filter) != 0) {
                c.set(12, filter);
                p.call(pointer_glue, 0x100ee558, {search});
                c.set(2, c.u32(c.sp() + 0x14));
            } else {
                c.set(3, 1);
            }
            const auto flags = extended(std::int16_t(c.r(3)));
            text_colour_or_flags = flags;
            if (compare(c, std::int32_t(flags & 1)) != 0) {
                p.call(split_path, 0x100ee588, {found, 0, 0, search, 0});
                auto colour = plain;
                auto back = 0x100ee66cu;
                if (compare(c, std::int32_t(flags & 4)) != 0) {
                    colour = folder_colour;
                    back = 0x100ee5d4;
                } else if (compare(c, std::int32_t(flags & 8)) != 0) {
                    colour = marked;
                    back = 0x100ee624;
                }
                p.stack(f.base(), {c.u16(colour), search, 2, 0});
                p.call(add_button, back, {screens, list, 0, back_colour_or_rows, 0x22, c.u16(row_colour), 0xc, 4});
                back_colour_or_rows = back_colour_or_rows + 1;
            }
        }
        finished = std::int8_t(p.call(find_next, 0x100ee678, {found}));
    }
    p.call(close_find, 0x100ee68c, {found});
    text_colour_or_flags = 0;
    while (compare(c, std::int32_t(std::uint32_t(back_colour_or_rows)), 6) < 0) {
        p.stack(f.base(), {c.u16(plain), text_colour_or_flags});
        p.call(add_cell, 0x100ee6c4, {screens, list, 0, back_colour_or_rows, 0x22, c.u16(row_colour), 0xc, 4});
        back_colour_or_rows = back_colour_or_rows + 1;
    }
    const auto row = p.value(0x100ee6dc, screens, v_row);
    const auto index = std::uint32_t(c.i16(screens + 0x2c58)) * 0x2c0;
    const auto controls = c.u32(screens + 0x24 + index);
    c.set(4, index);
    c.set(6, controls);
    const auto table = std::int32_t(std::int16_t(list));
    if (compare(c, table, -1) != 0) {
        c.set(5, std::uint32_t(table) * 0xbf0);
        c.set(4, row);
        p.call(select_row, 0x100ee708, {controls + c.r(5) + 4});
    }
    p.store(0x100ee71c, screens, v_list, std::uint32_t(table));
    const auto cancel_button = [&](std::uint32_t after_add, std::uint32_t after_cell, std::uint32_t grid) {
        p.call(localized_text, after_add, {label, pool + text::cancel});
        if (compare(c, std::int8_t(plain_cancel)) == 0) {
            p.stack(f.base(), {c.u16(plain), label, 3, 0});
            p.call(add_button, after_cell - 0x34, {screens, grid, 0, 0, 0x30, c.u16(folder_colour), 0xc, 3});
        } else {
            p.stack(f.base(), {c.u16(plain), label});
            p.call(add_cell, after_cell, {screens, grid, 0, 0, 0x30, c.u16(folder_colour), 0x2c, 3});
        }
    };
    const auto button = [&](std::uint32_t after_text, std::uint32_t after_add, std::uint32_t grid, std::uint32_t caption,
                            std::uint32_t column, std::uint32_t code) {
        p.call(localized_text, after_text, {label, pool + caption});
        p.stack(f.base(), {c.u16(plain), label, code, 0});
        p.call(add_button, after_add, {screens, grid, column, 0, 0x30, c.u16(folder_colour), 0xc, 3});
    };
    p.stack(f.base(), {0, 1, 0, none});
    if (compare(c, std::int8_t(info_button_or_grid)) != 0) {
        const auto grid = p.call(add_table, 0x100ee76c, {window(c, screens), 0x64, 0x22b, 0x316, 0x24e, 3, three_columns, 1});
        info_button_or_grid = grid;
        cancel_button(0x100ee77c, 0x100ee7f8, grid);
        button(0x100ee804, 0x100ee844, grid, text::info, 1, 5);
        button(0x100ee850, 0x100ee890, grid, text::ok, 2, 4);
    } else {
        const auto grid = p.call(add_table, 0x100ee8dc, {window(c, screens), 0x64, 0x22b, 0x316, 0x24e, 2, two_columns, 1});
        info_button_or_grid = grid;
        cancel_button(0x100ee8ec, 0x100ee968, grid);
        button(0x100ee974, 0x100ee9b4, grid, text::ok, 1, 4);
    }
}

void file_dialog_action(Cpu& c, std::uint32_t control) {
    Frame f(c, 0x100ee9d0, 0x4b0, {.first_gpr = 18});
    const auto screens = c.toc(slot::screens), key = c.toc(slot::key), label = c.toc(slot::text), pool = c.toc(slot::pool);
    keep(c, {{26, screens}, {25, label}, {27, pool}});
    Kept<28> pressed_key(c, key);
    Kept<29> pressed(c, control);
    Kept<30> folder_or_back_colour(c, c.r(30));
    Kept<31> copy_or_text_colour(c, c.r(31));
    Kept<18> held(c, c.r(18));
    Kept<19> path_or_heading(c, c.r(19));
    Kept<20> heading_or_extension(c, c.r(20));
    Kept<21> name_or_copy(c, c.r(21));
    Kept<22> text_colour_or_path(c, c.r(22));
    Kept<23> back_colour_or_folder(c, c.r(23));
    Kept<24> ok_heading(c, c.r(24));
    Program p{c};
    const auto answer_address = screens + screens_high - answer_field;
    const auto answer_box = [&](std::uint32_t after_title, std::uint32_t buffer, std::uint32_t caption, std::uint32_t heading,
                                std::uint32_t confirm, std::uint32_t text_colour, std::uint32_t back_colour) {
        // The text was just formatted; the title follows it.
        held = c.r(3);
        const auto title = p.call(localized_text, after_title, {buffer, pool + caption});
        enter(c, after_title + 0x20, {heading, title, held, confirm, extended(std::int16_t(text_colour)), extended(std::int16_t(back_colour)), 0});
        open_message(c, heading, title, held, confirm, extended(std::int16_t(text_colour)), extended(std::int16_t(back_colour)), 0);
    };
    if (compare(c, c.i8(screens + 0x2c5f)) != 0) {
        heading_or_extension = p.value(0x100eea0c, screens, v_heading);
        folder_or_back_colour = p.value(0x100eea20, screens, v_folder);
        copy_or_text_colour = p.value(0x100eea34, screens, v_folder_copy);
        name_or_copy = p.value(0x100eea48, screens, v_name);
        path_or_heading = p.value(0x100eea5c, screens, v_path);
        const auto question = p.value(0x100eea6c, screens, v_question);
        held = question;
        text_colour_or_path = p.value(0x100eea7c, screens, v_text_colour) & 0xffff;
        back_colour_or_folder = p.value(0x100eea8c, screens, v_back_colour) & 0xffff;
        if (compare(c, std::int32_t(question), create_folder_question) == 0) {
            p.store(0x100eeaac, screens, v_question, 0);
            c.set(4, answer_address);
            const auto answer = c.i32(answer_address);
            if (compare(c, answer, std::int32_t(yes_answer)) == 0) {
                c.put32(answer_address, 0);
                if (compare(c, std::int32_t(p.call(create_directory, 0x100eead0, {folder_or_back_colour}))) != 0) {
                    p.call(localized_text, 0x100eeae8, {label, pool + text::cannot_create, folder_or_back_colour});
                    answer_box(0x100eeaf8, f.at(0x2a8), text::cannot_create_title, heading_or_extension, 0, text_colour_or_path, back_colour_or_folder);
                    p.call(string_copy_limited, 0x100eeb28, {folder_or_back_colour, copy_or_text_colour, 0x100});
                    c.put8(folder_or_back_colour + 0xff, 0);
                    result(c, std::uint32_t(stay));
                    return;
                }
            } else if (compare(c, answer, std::int32_t(no_answer)) == 0) {
                c.put32(answer_address, 0);
                p.call(string_copy_limited, 0x100eeb58, {folder_or_back_colour, copy_or_text_colour, 0x100});
                c.put8(folder_or_back_colour + 0xff, 0);
            }
        } else if (compare(c, std::int32_t(question), delete_question) == 0 || compare(c, std::int32_t(question), replace_question) == 0) {
            const bool deleting = question == std::uint32_t(delete_question);
            p.store(deleting ? 0x100eeb80 : 0x100eebfc, screens, v_question, 0);
            held = answer_address;
            const auto answer = c.i32(answer_address);
            if (compare(c, answer, std::int32_t(yes_answer)) == 0) {
                c.put32(answer_address, 0);
                finish(c, deleting ? 0x100eeba4 : 0x100eec20, screens, answer_address, path_or_heading, copy_or_text_colour, name_or_copy);
                return;
            }
            if (compare(c, answer, std::int32_t(no_answer)) == 0) c.put32(answer_address, 0);
        } else {
            c.set(3, screens + screens_high);
            c.put32(answer_address, 0);
        }
    } else if (compare(c, std::int16_t(control), -1) != 0) {
        const auto table = extended(std::int16_t(p.value(0x100eec84, screens, v_list)));
        const auto row = p.call(row_for_item, 0x100eec90, {screens, table});
        p.store(0x100eeca4, screens, v_row, row);
    }
    folder_or_back_colour = screens + 0x2c58;
    copy_or_text_colour = screens + 0x24;
    std::int32_t action = control_action(c, screens, control);
    if (action == 0) {
        const std::int32_t typed = c.i8(key);
        if (is_no_key(typed)) {
            c.put8(key, 0);
            action = cancel_action;
        } else if (is_yes_key(typed)) {
            c.put8(key, 0);
            action = ok_action;
        }
    }
    if (compare(c, action, check_folder_action) == 0) {
        const auto heading = p.value(0x100eed88, screens, v_heading);
        path_or_heading = heading;
        const auto mode = p.value(0x100eed9c, screens, v_mode) & 0xff;
        held = mode;
        const auto folder = p.value(0x100eedac, screens, v_folder);
        heading_or_extension = folder;
        const auto copy = p.value(0x100eedc0, screens, v_folder_copy);
        name_or_copy = copy;
        const auto text_colour = p.value(0x100eedd4, screens, v_text_colour) & 0xffff;
        text_colour_or_path = text_colour;
        const auto back_colour = p.value(0x100eede4, screens, v_back_colour) & 0xffff;
        back_colour_or_folder = back_colour;
        if (compare_logical(c, p.call(string_length, 0x100eedf0, {folder})) != 0
            && compare(c, std::int8_t(p.call(usable_path, 0x100eee00, {folder}))) == 0) {
            if (compare_logical(c, mode, save_mode) == 0) {
                p.store(0x100eee24, screens, v_question, create_folder_question);
                p.call(localized_text, 0x100eee34, {label, pool + text::create_folder, folder});
                answer_box(0x100eee44, f.at(0x1e0), text::no_folder_title, heading, 1, text_colour, back_colour);
                result(c, std::uint32_t(stay));
                return;
            }
            p.call(localized_text, 0x100eee7c, {label, pool + text::no_folder, folder});
            answer_box(0x100eee8c, f.at(0x1e0), text::no_folder_title, heading, 0, text_colour, back_colour);
            p.call(string_copy_limited, 0x100eeebc, {folder, copy, 0x100});
            c.put8(folder + 0xff, 0);
            result(c, std::uint32_t(stay));
            return;
        }
        p.call(string_copy_limited, 0x100eeedc, {copy, folder, 0x100});
        c.put8(copy + 0xff, 0);
        result(c, std::uint32_t(redraw));
        return;
    }
    if (compare(c, action, pick_action) == 0) {
        const auto name = p.value(0x100eef00, screens, v_name);
        held = name;
        const auto id = std::int16_t(control);
        compare(c, id, -1);
        const auto chosen = id != -1 ? window(c, screens) + 0xc0000 + std::uint32_t(std::int32_t(id)) * 0x17c - 0x5724 : 0u;
        p.call(string_copy_limited, 0x100eef40, {name, chosen, 0x1f});
        c.put8(name + 0x1e, 0);
        result(c, std::uint32_t(redraw));
        return;
    }
    if (compare(c, action, cancel_action) == 0) {
        const auto copy = p.value(0x100eef64, screens, v_folder_copy);
        held = copy;
        const auto name = p.value(0x100eef78, screens, v_name);
        path_or_heading = name;
        const auto path = p.value(0x100eef8c, screens, v_path);
        held = path;
        p.call(release, 0x100eef9c, {copy});
        p.call(release, 0x100eefa4, {name});
        p.call(release, 0x100eefac, {path});
        const auto parent = p.value(0x100eefb8, screens, v_parent);
        c.set(4, screens + screens_high);
        c.put32(screens + screens_high - returned_value, parent);
        c.put32(answer_address, 0);
        result(c, std::uint32_t(closed));
        return;
    }
    if (compare(c, action, info_action) == 0) {
        const auto heading = p.value(0x100eefe4, screens, v_heading);
        pressed_key = heading;
        p.value(0x100eeff8, screens, v_mode);
        const auto folder = p.value(0x100ef004, screens, v_folder);
        path_or_heading = folder;
        const auto extension = p.value(0x100ef018, screens, v_extension);
        heading_or_extension = extension;
        p.value(0x100ef02c, screens, v_folder_copy);
        const auto name = p.value(0x100ef038, screens, v_name);
        name_or_copy = name;
        const auto path = p.value(0x100ef04c, screens, v_path);
        text_colour_or_path = path;
        const auto text_colour = p.value(0x100ef060, screens, v_text_colour) & 0xffff;
        pressed = text_colour;
        const auto back_colour = p.value(0x100ef070, screens, v_back_colour) & 0xffff;
        folder_or_back_colour = back_colour;
        p.value(0x100ef080, screens, v_folders_only);
        if (compare_logical(c, p.call(string_length, 0x100ef088, {name})) == 0) {
            p.call(localized_text, 0x100ef09c, {label, pool + text::blank_file});
            answer_box(0x100ef0ac, f.at(0x118), text::blank_file_title, heading, 0, text_colour, back_colour);
            result(c, std::uint32_t(stay));
            return;
        }
        build_path(c, 0x100ef0ec, path, folder, name, extension, f.at(0x48), 18);
        if (compare(c, std::int8_t(p.call(file_exists_at, 0x100ef140, {path}))) == 0) {
            p.call(split_path, 0x100ef160, {path, 0, 0, f.at(0x370), 0});
            p.call(localized_text, 0x100ef170, {label, pool + text::no_file, f.at(0x370)});
            answer_box(0x100ef180, f.at(0x118), text::no_file_title, heading, 0, text_colour, back_colour);
            result(c, std::uint32_t(stay));
            return;
        }
        const auto entry = p.call(open_entry, 0x100ef1c0, {path, pool + text::general_data, 0, 0x15, 4});
        const bool opened = compare_logical(c, entry) != 0;
        c.put32(f.at(0x3c), entry);
        if (opened) {
            if (compare(c, std::int32_t(p.call(read_entry, 0x100ef1e0, {f.at(0x38), 4, 1, entry})), 1) == 0) {
                p.call(close_entry, 0x100ef1f0, {f.at(0x3c)});
                // Saves from before 0x3e023 can't be shown; the entry then closes twice.
                const auto version = std::int32_t(p.call(absolute, 0x100ef1f8, {c.u32(f.at(0x38))}));
                c.set(4, 0x40000);
                if (compare(c, version, 0x3e023) >= 0) {
                    enter(c, 0x100ef210, {path});
                    open_save_info(c, path);
                    result(c, std::uint32_t(stay));
                    return;
                }
            }
            p.call(close_entry, 0x100ef220, {f.at(0x3c)});
        }
        p.call(localized_text, 0x100ef22c, {label, pool + text::no_info});
        answer_box(0x100ef23c, f.at(0x118), text::no_info_title, heading, 0, text_colour, back_colour);
        result(c, std::uint32_t(stay));
        return;
    }
    if (compare(c, action, ok_action) == 0) {
        const auto heading = p.value(0x100ef278, screens, v_heading);
        ok_heading = heading;
        const auto mode = p.value(0x100ef28c, screens, v_mode) & 0xff;
        pressed_key = mode;
        const auto folder = p.value(0x100ef29c, screens, v_folder);
        back_colour_or_folder = folder;
        const auto extension = p.value(0x100ef2b0, screens, v_extension);
        text_colour_or_path = extension;
        const auto copy = p.value(0x100ef2c4, screens, v_folder_copy);
        name_or_copy = copy;
        const auto name = p.value(0x100ef2d8, screens, v_name);
        heading_or_extension = name;
        const auto path = p.value(0x100ef2ec, screens, v_path);
        path_or_heading = path;
        const auto text_colour = p.value(0x100ef300, screens, v_text_colour) & 0xffff;
        copy_or_text_colour = text_colour;
        const auto back_colour = p.value(0x100ef310, screens, v_back_colour) & 0xffff;
        folder_or_back_colour = back_colour;
        const auto folders_only = std::int8_t(p.value(0x100ef320, screens, v_folders_only));
        held = std::uint32_t(std::int32_t(folders_only));
        if (compare_logical(c, p.call(string_length, 0x100ef32c, {name})) == 0) {
            if (compare(c, folders_only) != 0) {
                p.call(localized_text, 0x100ef348, {label, pool + text::blank_folder});
                answer_box(0x100ef358, f.at(0x50), text::blank_folder_title, heading, 0, text_colour, back_colour);
            } else {
                p.call(localized_text, 0x100ef38c, {label, pool + text::blank_file});
                answer_box(0x100ef39c, f.at(0x50), text::blank_file_title, heading, 0, text_colour, back_colour);
            }
            result(c, std::uint32_t(stay));
            return;
        }
        const auto report_missing = [&](std::uint32_t after_split) {
            p.call(split_path, after_split, {path, 0, 0, f.at(0x370), 0});
            p.call(localized_text, after_split + 0x10, {label, pool + text::no_file, f.at(0x370)});
            answer_box(after_split + 0x20, f.at(0x50), text::no_file_title, heading, 0, text_colour, back_colour);
            result(c, std::uint32_t(stay));
        };
        const auto ask = [&](std::uint32_t after_store, std::uint32_t question, std::uint32_t wording, std::uint32_t caption) {
            p.store(after_store, screens, v_question, question);
            p.call(split_path, after_store + 0x18, {path, 0, 0, f.at(0x370), 0});
            p.call(localized_text, after_store + 0x28, {label, pool + wording, f.at(0x370)});
            answer_box(after_store + 0x38, f.at(0x50), caption, heading, 1, text_colour, back_colour);
            result(c, std::uint32_t(stay));
        };
        if (compare_logical(c, mode, load_mode) == 0) {
            build_path(c, 0x100ef3e4, path, folder, name, extension, f.at(0x40), 28);
            if (compare(c, folders_only) == 0 && compare(c, std::int8_t(p.call(file_exists_at, 0x100ef440, {path}))) == 0)
                return report_missing(0x100ef460);
            finish(c, 0x100ef4b0, screens, answer_address, path, copy, name);
            return;
        }
        if (compare_logical(c, mode, save_mode) == 0) {
            build_path(c, 0x100ef4f8, path, folder, name, extension, f.at(0x40), 28);
            if (compare(c, folders_only) == 0 && compare(c, std::int8_t(p.call(file_exists_at, 0x100ef554, {path}))) != 0)
                return ask(0x100ef570, replace_question, text::file_exists, text::file_exists_title);
            finish(c, 0x100ef5d8, screens, answer_address, path, copy, name);
            return;
        }
        if (compare_logical(c, mode, delete_mode) == 0) {
            build_path(c, 0x100ef620, path, folder, name, extension, f.at(0x40), 28);
            if (compare(c, folders_only) == 0) {
                if (compare(c, std::int8_t(p.call(file_exists_at, 0x100ef67c, {path}))) == 0) return report_missing(0x100ef69c);
                return ask(0x100ef6f8, delete_question, text::confirm_delete, text::confirm_title);
            }
            finish(c, 0x100ef760, screens, answer_address, path, copy, name);
            return;
        }
    }
    if (compare(c, std::int32_t(p.call(default_action, 0x100ef790, {pressed}))) != 0) return;
    p.call(common_action, 0x100ef7a4, {pressed});
}
}
