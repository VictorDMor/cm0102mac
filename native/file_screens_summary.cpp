#include "file_screens_detail.hpp"
#include "guest_fpu.hpp"

namespace cm::aot::file_screens {
using namespace detail;
namespace {
// The summary as read from a save, in the screen's frame.
constexpr std::uint32_t summary_at = 0x110;
constexpr std::int32_t first_summary_with_save_time = 0x3e027;
constexpr std::int32_t seconds_per_day = 86400, seconds_per_hour = 3600, seconds_per_minute = 60;

// A summary that can't be read: the assertion text reaches the load error dialog.
struct LoadError {
    std::uint32_t at, name, extension, line, reason;
};
void report_load_error(Cpu& c, const Frame& f, const LoadError& e, std::uint32_t pool, std::uint32_t message,
                       std::uint32_t caption, std::uint32_t body) {
    Program p{c};
    p.call(split_path, e.at + 4, {pool, 0, 0, f.at(e.name), f.at(e.extension)});
    const auto version = p.call(version_text, e.at + 8);
    p.call(format_text, e.at + 0x28, {message, pool + text::load_assertion, version, f.at(e.name), f.at(e.extension), e.line, pool + e.reason});
    p.call(localized_text, e.at + 0x34, {caption, pool + text::load_error_title});
    p.call(localized_text, e.at + 0x44, {body, pool + text::load_error, message});
    p.call(error_dialog, e.at + 0x54, {caption, body, 0});
}
}

void open_save_info(Cpu& c, std::uint32_t path) {
    Frame f(c, 0x100ef7d0, 0x40, {.first_gpr = 30});
    const auto screens = c.toc(slot::screens);
    keep(c, {{31, screens}, {30, path}});
    Program p{c};
    if (compare(c, std::int8_t(p.call(append_history, 0x100ef808, {screens, c.toc(slot::info_render), c.toc(slot::info_action), 1, 0, 0}))) != 0)
        p.store(0x100ef824, screens, 0, path, 1);
}

void render_save_info(Cpu& c) {
    Frame f(c, 0x100ef840, 0x10d0, {.first_gpr = 21});
    const auto screens = c.toc(slot::screens), label = c.toc(slot::text), pool = c.toc(slot::pool),
               message = c.toc(slot::message), body = c.toc(slot::secondary_text), caption = c.toc(slot::title_text),
               marked = c.toc(slot::marked_colour), plain = c.toc(slot::plain_colour), layout = c.toc(slot::list_layout);
    keep(c, {{24, marked}, {25, plain}, {26, label}, {27, screens}, {29, pool}});
    Kept<21> width_or_row(c, layout);
    Kept<22> body_or_grid(c, body);
    Kept<23> caption_or_days(c, caption);
    Kept<28> message_or_time(c, message);
    Program p{c};
    const auto guard = c.r(2) - 0x39b6;
    if (compare(c, c.i8(guard)) == 0) {
        const auto colour = p.call(make_colour, 0x100ef894, {0, 0xc0, 0, 0});
        c.put16(c.r(2) - 0x39b4, colour);
        c.put8(guard, 1);
    }
    const auto columns = f.at(0x48), writer = f.at(0x88), entry = f.at(0x50), summary = f.at(summary_at);
    const SaveSummary saved_game{c, summary};
    c.put16(columns, c.u16(c.r(2) - 0x39bc));
    Kept<31> date(c, saved_game.date());
    p.call(make_date, 0x100ef8b4, {date});
    p.call(init_text, 0x100ef8c0, {writer, 0});
    Kept<30> path_or_weekday(c, p.value(0x100ef8cc, screens, 0));
    const auto opened = p.call(open_entry, 0x100ef8e4, {path_or_weekday, pool + text::general_data, 0, 0x15, 4});
    const bool found = compare_logical(c, opened) != 0;
    c.put32(entry, opened);
    if (!found) {
        report_load_error(c, f, {0x100ef904, 0xf0, 0x80, 0x413, text::no_general_data}, pool, message, caption, body);
        p.call(destroy_text, 0x100ef964, {writer, none});
        return;
    }
    const auto failed_read = [&](const LoadError& e) {
        report_load_error(c, f, e, pool, message, caption, body);
        p.call(close_entry, e.at + 0x5c, {entry});
        p.call(destroy_text, e.at + 0x68, {writer, none});
    };
    if (compare(c, std::int32_t(p.call(read_entry, 0x100ef97c, {f.at(0x4c), 4, 1, opened})), 1) != 0)
        return failed_read({0x100ef998, 0xd0, 0x78, 0x41a, text::no_version});
    const auto version = std::int32_t(p.call(absolute, 0x100efa0c, {c.u32(f.at(0x4c))}));
    if (compare(c, version, first_summary_with_save_time) < 0) {
        if (compare(c, std::int32_t(p.call(read_entry, 0x100efa30, {summary, SaveSummary::old_size, 1, c.u32(entry)})), 1) != 0)
            return failed_read({0x100efa4c, 0xb0, 0x70, 0x425, text::no_summary});
        saved_game.saved() = 0;
    } else if (compare(c, std::int32_t(p.call(read_entry, 0x100efad8, {summary, SaveSummary::size, 1, c.u32(entry)})), 1) != 0) {
        return failed_read({0x100efaf4, 0x90, 0x68, 0x432, text::no_summary});
    }
    p.call(close_entry, 0x100efb68, {entry});
    p.call(begin_plain_screen, 0x100efb70, {0});
    body_or_grid = screens + 0x2c58;
    caption_or_days = screens + 0x24;
    p.stack(f.base(), {0, 2, 0, none});
    p.call(add_table, 0x100efbc0, {window(c, screens), 0, 0, 0x31f, 0x257, 1, 0, 1});
    const auto background = p.call(make_colour, 0x100efbd4, {0, 0x38, 0, 0});
    p.stack(f.base(), {0, 0x30, background, none});
    p.call(add_table, 0x100efc1c, {window(c, screens), 0x78, 0x64, 0x301, 0x1f3, 1, 0, 1});
    p.call(localized_text, 0x100efc28, {label, pool + text::info_title});
    p.stack(f.base(), {4, c.u16(plain), label});
    p.call(add_box, 0x100efc64, {screens, 0x82, 0x6e, 0x2f2, 0x9b, 0x30, c.u16(c.toc(slot::heading_colour)), 0xc});
    p.stack(f.base(), {0, 1, 0, none});
    const auto buttons = p.call(add_table, 0x100efcac, {window(c, screens), 0x82, 0x1cb, 0x2f2, 0x1e9, 1, 0, 1});
    message_or_time = buttons;
    p.call(localized_text, 0x100efcbc, {label, pool + text::ok});
    p.stack(f.base(), {c.u16(plain), label, 1, 0});
    p.call(add_button, 0x100efd00, {screens, buttons, 0, 0, 0x30, c.u16(c.toc(slot::folder_colour)), 0xc, 3});
    c.set_f(1, double(c.f32(layout)));
    p.stack(f.base(), {1, buttons});
    p.call(add_frame, 0x100efd34, {0x8c, 0xaf, 0x2e8, 0x1b7, f.at(0x64), f.at(0x60), f.at(0x5c), f.at(0x58)});
    p.stack(f.base(), {0, 1, 0, none});
    const auto grid = p.call(add_table, 0x100efd7c, {window(c, screens), 0x91, 0xb9, 0x2e3, 0x1b2, 2, columns, 0xc});
    body_or_grid = grid;

    // The first column's share of the 594 wide list, as the original computes it.
    const std::uint32_t first = c.u8(columns), share = first + c.u8(columns + 1);
    const auto integers = c.toc(slot::integer_bases);
    c.put32(f.at(0x1094), 0x43300000);
    c.put32(f.at(0x1098), share ^ 0x80000000);
    c.put32(f.at(0x108c), 0x43300000);
    c.put32(f.at(0x1090), first);
    Fpu fpu(c);
    const auto total = fpu.fsubs(c.f64(f.at(0x1094)), c.f64(integers));
    const auto first_share = fpu.fsubs(c.f64(f.at(0x108c)), c.f64(integers + 8));
    const auto width_of_list = double(c.f32(layout + 0xc)), left = double(c.f32(layout + 8)), margin = double(c.f32(layout + 4));
    const auto column = fpu.fdivs(width_of_list, fpu.fdivs(total, first_share));
    const auto position = fpu.fadds(margin, fpu.fadds(left, column));
    c.put64(f.at(0x1084), fpu.fctiwz(0, position));
    c.set_f(1, c.f64(integers + 8));
    c.set_f(2, total);
    c.set_f(3, width_of_list);
    c.set_f(4, left);
    c.set_f(5, margin);
    width_or_row = c.u32(f.at(0x1088));
    const auto wrap_width = c.u32(f.at(0x1088));
    const auto row_number = f.at(0x54);
    c.put32(row_number, 0);
    c.set(5, 0x43300000);
    c.set(6, integers);
    c.set(7, first);
    p.call(localized_text, 0x100efdfc, {label, pool + text::file_name});

    const auto cell = [&](std::uint32_t back, std::uint32_t column_index, std::uint32_t colour, std::uint32_t content) {
        p.stack(f.base(), {c.u16(colour), content});
        p.call(add_cell, back, {screens, grid, column_index, c.u32(row_number), 1, 0, 1, 2});
    };
    // The next row's heading: the original reads the row into r5 as it advances.
    const auto heading = [&](std::uint32_t back, std::uint32_t caption_text) {
        const auto row = c.u32(row_number);
        c.set(5, row);
        c.put32(row_number, row + 1);
        p.call(localized_text, back, {label, pool + caption_text});
    };
    const auto next_row = [&]() {
        const auto row = c.u32(row_number);
        c.set(3, row);
        c.put32(row_number, row + 1);
    };
    cell(0x100efe2c, 0, plain, label);
    cell(0x100efe5c, 1, marked, path_or_weekday);
    heading(0x100efe74, text::last_saved);
    cell(0x100efea4, 0, plain, label);
    const auto time = p.call(local_time, 0x100efeac, {saved_game.saved().address()});
    message_or_time = time;
    auto weekday = std::int8_t(c.u32(time + 0x18) - 1);
    if (compare(c, weekday) < 0) weekday = std::int8_t(std::int32_t(weekday) + 7);
    path_or_weekday = std::uint32_t(std::int32_t(weekday));
    const auto month = p.call(month_name, 0x100efed4, {std::uint32_t(std::int32_t(std::int8_t(c.u32(time + 0x10))))});
    caption_or_days = month;
    const auto day = p.call(day_name, 0x100efee0, {std::uint32_t(std::int32_t(weekday))});
    p.stack(f.base(), {month, c.u32(time + 0x14) + 1900});
    p.call(localized_text, 0x100eff14, {label, pool + text::saved_at, 100, 100, c.u32(time + 8), c.u32(time + 4), day, c.u32(time + 0xc)});
    cell(0x100eff44, 1, marked, label);
    heading(0x100eff5c, text::version);
    cell(0x100eff8c, 0, plain, label);
    cell(0x100effc0, 1, marked, saved_game.version());
    heading(0x100effd8, text::date);
    cell(0x100f0008, 0, plain, label);
    p.call(date_text, 0x100f001c, {date, label, 0x7d0, 0xff});
    cell(0x100f004c, 1, marked, label);
    heading(0x100f0064, text::game_time);

    // The original keeps each remainder in the summary's own field; it ends as
    // the seconds, and r31 as the seconds past the hour.
    const auto total_seconds = std::int32_t(saved_game.played().get());
    const auto days = total_seconds / seconds_per_day, past_day = total_seconds % seconds_per_day;
    const auto hours = past_day / seconds_per_hour, past_hour = past_day % seconds_per_hour;
    const auto minutes = past_hour / seconds_per_minute, seconds = past_hour % seconds_per_minute;
    saved_game.played() = std::uint32_t(seconds);
    caption_or_days = std::uint32_t(days);
    message_or_time = std::uint32_t(hours);
    path_or_weekday = std::uint32_t(minutes);
    date = std::uint32_t(past_hour);
    cell(0x100f013c, 0, plain, label);
    p.call(localized_text, 0x100f0158, {label, pool + text::time_played, std::uint32_t(days), std::uint32_t(hours),
                                        std::uint32_t(minutes), std::uint32_t(seconds)});
    cell(0x100f0188, 1, marked, label);
    next_row();
    c.call(current_language, 0x100f0198);
    if (compare(c, std::int8_t(c.r(3))) == 0 && compare(c, days) > 0) {
        p.call(format_text, 0x100f01b4, {label, pool + text::addiction});
        cell(0x100f01e4, 0, plain, label);
        struct Rating { std::int32_t over_days; std::uint32_t text, back; };
        constexpr Rating ratings[] = {{10, text::addiction_over_10_days, 0x100f01f8}, {8, text::addiction_over_8_days, 0x100f0210},
            {7, text::addiction_over_7_days, 0x100f0228}, {6, text::addiction_over_6_days, 0x100f0240},
            {5, text::addiction_over_5_days, 0x100f0258}, {4, text::addiction_over_4_days, 0x100f0270},
            {3, text::addiction_over_3_days, 0x100f0288}, {2, text::addiction_over_2_days, 0x100f02a0},
            {1, text::addiction_over_1_day, 0x100f02b8}};
        bool rated = false;
        for (const auto& rating : ratings) {
            if (compare(c, days, rating.over_days) > 0) {
                p.call(format_text, rating.back, {label, pool + rating.text});
                rated = true;
                break;
            }
        }
        if (!rated) {
            if (compare(c, hours, 12) > 0) p.call(format_text, 0x100f02d0, {label, pool + text::addiction_over_12_hours});
            else p.call(format_text, 0x100f02e0, {label, pool + text::addiction_mild});
        }
        cell(0x100f0310, 1, marked, label);
        next_row();
    }
    p.call(localized_text, 0x100f0328, {label, pool + text::foreground_leagues});
    cell(0x100f0358, 0, plain, label);
    const auto wrapped = [&](std::uint32_t after_set, std::uint32_t after_draw, std::uint32_t content) {
        p.call(set_text, after_set, {writer, content});
        p.stack(f.base(), {0});
        p.call(draw_text, after_draw, {writer, grid, wrap_width, 0x2e3, row_number, 2, c.u16(marked), 0});
    };
    wrapped(0x100f0364, 0x100f0390, saved_game.foreground());
    const auto background_leagues = saved_game.background();
    caption_or_days = background_leagues;
    const auto after_foreground = c.u32(row_number) + 1;
    c.set(3, after_foreground);
    const bool has_background = compare(c, c.i8(background_leagues)) != 0;
    c.put32(row_number, after_foreground);
    if (has_background) {
        p.call(localized_text, 0x100f03b8, {label, pool + text::background_leagues});
        cell(0x100f03e8, 0, plain, label);
        wrapped(0x100f03f4, 0x100f0420, background_leagues);
        next_row();
    }
    p.call(localized_text, 0x100f0438, {label, pool + text::managers});
    cell(0x100f0468, 0, plain, label);
    caption_or_days = 0;
    width_or_row = 0;
    while (compare(c, std::int8_t(caption_or_days), std::int32_t(SaveSummary::manager_count)) < 0) {
        const auto manager = saved_game.manager(width_or_row);
        c.set(6, summary + std::uint32_t(width_or_row));
        if (compare(c, c.i8(manager.name())) != 0) {
            if (compare(c, c.i8(manager.club())) != 0)
                p.call(localized_text, 0x100f04ac, {label, pool + text::manager_with_club, manager.name(), manager.club()});
            else
                p.call(string_copy, 0x100f04bc, {label, manager.name()});
            cell(0x100f04ec, 1, marked, label);
            next_row();
        }
        width_or_row = width_or_row + SaveSummary::manager_size;
        caption_or_days = caption_or_days + 1;
    }
    p.call(destroy_text, 0x100f0518, {writer, none});
}

void save_info_action(Cpu& c, std::uint32_t control) {
    const auto screens = c.toc(slot::screens);
    c.set(4, screens);
    const auto action = std::int32_t(control_action(c, screens, control));
    c.set(3, compare(c, action, 1) == 0 ? std::uint32_t(closed) : 0);
}

void fill_save_summary(Cpu& c, std::uint32_t summary) {
    Frame f(c, 0x100f0d70, 0xd0, {.first_gpr = 21});
    const auto leagues = c.toc(slot::leagues), last_time = c.toc(slot::last_time), total_time = c.toc(slot::total_time),
               date = c.toc(slot::game_date), person_count = c.toc(slot::person_count), people = c.toc(slot::people),
               pool = c.toc(slot::pool);
    keep(c, {{23, summary}, {26, last_time}, {27, total_time}, {28, date}, {29, person_count}, {30, people}, {31, pool}});
    const SaveSummary saved_game{c, summary};
    Kept<21> list_or_row(c, leagues);
    Kept<22> league_or_person(c, leagues);
    Kept<24> first_or_index(c, 1);
    Kept<25> count_or_offset(c, 0);
    c.put8(saved_game.foreground(), 0);
    c.put8(saved_game.background(), 0);
    const auto line = f.at(0x3c);
    Program p{c};
    // One line of names: the first bare, the rest after a separator.
    const auto add_name = [&](const League& league, std::uint32_t first_back, std::uint32_t later_back, unsigned first_register,
                              std::uint32_t append_back, std::uint32_t names) {
        if (compare(c, std::int8_t(c.r(first_register))) != 0) {
            p.call(format_text, first_back, {line, league.name()});
            c.set(first_register, 0);
        } else {
            p.call(localized_text, later_back, {line, pool + text::league_separator, league.name()});
        }
        p.call(string_append, append_back, {names, line});
    };
    while (compare(c, std::int8_t(count_or_offset), std::int32_t(league_count)) < 0) {
        const League league{c, c.u32(league_or_person)};
        c.set(5, league.address);
        if (compare_logical(c, league.address) != 0 && compare(c, std::int32_t(league.flags() & League::in_foreground)) != 0)
            add_name(league, 0x100f0dec, 0x100f0e04, 24, 0x100f0e10, saved_game.foreground());
        league_or_person = league_or_person + league_record_size;
        count_or_offset = count_or_offset + 1;
    }
    first_or_index = list_or_row;
    list_or_row = 0;
    league_or_person = 1;
    while (compare(c, std::int8_t(list_or_row), std::int32_t(league_count)) < 0) {
        const League league{c, c.u32(first_or_index)};
        c.set(5, league.address);
        if (compare_logical(c, league.address) != 0 && compare(c, std::int32_t(league.flags() & League::in_background)) != 0)
            add_name(league, 0x100f0e60, 0x100f0e78, 22, 0x100f0e84, saved_game.background());
        first_or_index = first_or_index + league_record_size;
        list_or_row = list_or_row + 1;
    }
    if (compare(c, std::int8_t(league_or_person)) != 0) c.put8(saved_game.background(), 0);
    first_or_index = 0;
    count_or_offset = 0;
    while (compare(c, std::int8_t(first_or_index), std::int32_t(SaveSummary::manager_count)) < 0) {
        // People are listed from the end backwards.
        const auto from_end = c.u32(person_count) - std::uint32_t(std::int32_t(std::int8_t(first_or_index)));
        const Person person{c, c.u32(people) + (from_end - 1) * person_record_size};
        league_or_person = person.address;
        c.set(3, from_end);
        c.set(4, c.u32(people));
        const auto row = saved_game.manager(count_or_offset);
        if (compare(c, std::int32_t(person.role()), manager_role) != 0) {
            c.set(3, 0);
            c.put8(row.name(), 0);
        } else {
            list_or_row = summary + std::uint32_t(count_or_offset);
            p.call(person_name, 0x100f0ef4, {person.address, row.name(), 0xc8, 1});
            const auto club = person.club();
            c.set(5, club);
            if (compare_logical(c, club) != 0) {
                const auto other = person.other_club();
                c.set(6, other);
                if (compare_logical(c, other) != 0) {
                    p.call(localized_text, 0x100f0f20, {row.club(), pool + text::two_clubs, club + club_name, other + club_name});
                } else {
                    compare_logical(c, club);
                    p.call(string_copy, 0x100f0f38, {row.club(), club + club_name});
                }
            } else {
                const auto other = person.other_club();
                c.set(4, other);
                if (compare_logical(c, other) != 0) p.call(string_copy, 0x100f0f54, {row.club(), other + club_name});
                else c.put8(row.club(), 0);
            }
        }
        count_or_offset = count_or_offset + SaveSummary::manager_size;
        first_or_index = first_or_index + 1;
    }
    c.put16(saved_game.date() + 2, c.u16(date + 2));
    c.put16(saved_game.date(), c.u16(date));
    c.put8(saved_game.date() + 4, c.u8(date + 4));
    c.call(version_text, 0x100f0fa0);
    p.call(string_copy, 0x100f0fac, {saved_game.version(), c.r(3)});
    const auto now = p.call(clock_time, 0x100f0fb4, {f.at(0x38)});
    const auto previous = c.u32(last_time);
    const auto played = c.u32(total_time) + now - previous;
    c.put32(f.at(0x38), now);
    c.set(4, previous);
    c.put32(total_time, played);
    saved_game.played() = played;
    c.put32(last_time, c.u32(f.at(0x38)));
    saved_game.saved() = p.call(clock_time, 0x100f0fe0, {0});
}
}
