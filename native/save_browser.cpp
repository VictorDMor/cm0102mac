#include "save_browser.hpp"
#include "adaptive_gui.hpp"
#include "aot_runtime.hpp"
#include "file_screens_detail.hpp"
#include "new_screens.hpp"
#include "platform.hpp"
#include "save_catalog.hpp"
#include <array>
#include <cctype>
#include <chrono>
#include <string>
#include <vector>

namespace cm::aot::file_screens {
using namespace detail;
namespace {
namespace fs = std::filesystem;
constexpr std::uint32_t classic_builder = 0x100ee220;
constexpr std::uint32_t mode_value = 2;  // the dialog history's mode slot (file_screens_browser.cpp, Value::v_mode)
constexpr std::uint32_t small_font = 1;
constexpr std::uint32_t short_date = 0x107168a0;
// Control record: the value a button was added with.
constexpr std::uint32_t control_base = 0xba864, control_size = 0x17c, control_value = 0x48;

// The names of the saves the rows stand for, rebuilt with every build of the screen. Host
// state of the open dialog: never saved, never read by the game. The pick handler only
// trusts it through a control of this browser's own action code.
std::vector<std::string> row_names;

// The primary button's caption, for the game's languages (the byte at TOC-0x35ac; Latin-1).
struct Captions { const char *load, *save, *erase; };
constexpr std::array<Captions, 11> captions{{
    {"Load", "Save", "Delete"}, {"Load", "Save", "Delete"}, {"Charger", "Sauvegarder", "Supprimer"},
    {"Laden", "Speichern", "L\366schen"}, {"Carica", "Salva", "Elimina"}, {"Cargar", "Guardar", "Eliminar"},
    {"Hent", "Gem", "Slet"}, {"Laden", "Opslaan", "Verwijderen"}, {"Last inn", "Lagre", "Slett"},
    {"Carregar", "Gravar", "Apagar"}, {"Ladda", "Spara", "Radera"}}};

std::uint32_t call(Cpu& c, std::uint32_t target, std::initializer_list<std::uint32_t> arguments = {}) {
    unsigned reg = 3;
    for (const auto value : arguments) c.set(reg++, value);
    c.call(target, return_sentinel);
    return c.r(3);
}
void stack(Cpu& c, const Frame& f, std::initializer_list<std::uint32_t> values) {
    auto at = f.base() + 0x38;
    for (const auto value : values) { c.put32(at, value); at += 4; }
}
std::string guest_string(Cpu& c, std::uint32_t at, std::size_t limit = 300) {
    std::string text;
    while (text.size() < limit) {
        const auto ch = c.u8(at + std::uint32_t(text.size()));
        if (!ch) break;
        text.push_back(char(ch));
    }
    return text;
}
void put_string(Cpu& c, std::uint32_t at, const std::string& text, std::size_t limit) {
    const auto size = std::min(text.size(), limit - 1);
    for (std::size_t i = 0; i < size; ++i) c.put8(at + std::uint32_t(i), std::uint8_t(text[i]));
    c.put8(at + std::uint32_t(size), 0);
}
// Cuts text to what a column of `pixels` can show at about `per_char` pixels a glyph.
std::string fit(const std::string& text, int pixels, int per_char) {
    const auto chars = std::size_t(std::max(4, pixels / per_char));
    return text.size() <= chars ? text : text.substr(0, chars - 3) + "...";
}
bool same_name(const std::string& a, const std::string& b) {
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin(), [](char x, char y) {
        return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
    });
}

struct Builder {
    Cpu& c;
    const Frame& f;
    SavedGameBrowserInput in;
    std::uint32_t plain, marked, folder_colour, row_colour, list_colour;
    std::uint32_t window;
    std::uint32_t scratch;   // text the cells copy, in the frame's free area

    // sp+0x38 on: the arguments past the eighth, as the original's builders leave them.
    void args(std::initializer_list<std::uint32_t> values) const { stack(c, f, values); }
    std::uint32_t table(ScreenRect r, unsigned columns, std::uint32_t widths, unsigned rows, std::initializer_list<std::uint32_t> style) const {
        args(style);
        return call(c, add_table, {window, std::uint32_t(r.left), std::uint32_t(r.top), std::uint32_t(r.right), std::uint32_t(r.bottom),
                                   columns, widths, rows});
    }
    // Proportions of a table's columns, bytes in the frame.
    std::uint32_t widths(std::initializer_list<unsigned> shares) const {
        auto at = f.at(0x68);
        for (const auto share : shares) c.put8(at++, share);
        return f.at(0x68);
    }
    void text(const std::string& value, std::size_t limit = 200) const { put_string(c, scratch, value, limit); }
    void cell(std::uint32_t grid, unsigned column, unsigned row, const std::string& value, std::uint32_t ink,
              std::uint32_t background, unsigned flags, unsigned font, unsigned style) const {
        text(value);
        args({c.u16(ink), scratch});
        call(c, add_cell, {in.screens, grid, column, row, flags, background ? c.u16(background) : 0u, font, style});
    }
    void button(std::uint32_t grid, unsigned column, unsigned row, const std::string& value, std::uint32_t ink,
                std::uint32_t background, unsigned flags, unsigned font, unsigned style, std::uint32_t action, std::uint32_t item) const {
        text(value);
        args({c.u16(ink), scratch, action, item});
        call(c, add_button, {in.screens, grid, column, row, flags, c.u16(background), font, style});
    }
    // The game's own wording, filled from `pool + id` and its arguments.
    std::string localized(std::uint32_t id, std::initializer_list<std::uint32_t> values = {}) const {
        unsigned reg = 5;
        c.set(3, in.label);
        c.set(4, in.pool + id);
        for (const auto value : values) c.set(reg++, value);
        c.call(localized_text, return_sentinel);
        return guest_string(c, in.label);
    }
};

struct Found {
    std::string name, path;
    bool marked;
};
// The classic builder's own scan of the folder: the same calls in the same order, collecting
// what it would have turned into rows.
std::vector<Found> scan(Cpu& c, const Frame& f, const SavedGameBrowserInput& in) {
    std::vector<Found> result;
    const auto search = f.at(0x74), found = f.at(0x174);
    call(c, make_path, {search, 0, in.folder, in.pool + text::any_file, in.extension});
    call(c, find_first, {search, found, 0});
    auto finished = std::int8_t(c.r(3));
    while (finished == 0) {
        call(c, make_path, {search, 0, in.folder, found, 0});
        const auto path = guest_string(c, search);
        std::uint32_t flags = 1;
        if (in.filter) {
            c.set(12, in.filter);
            call(c, pointer_glue, {search});
            c.set(2, c.u32(c.sp() + 0x14));
            flags = extended(std::int16_t(c.r(3)));
        }
        if (flags & 1) {
            call(c, split_path, {found, 0, 0, search, 0});
            result.push_back({guest_string(c, search), path, (flags & 4) == 0 && (flags & 8) != 0});
        }
        finished = std::int8_t(call(c, find_next, {found}));
    }
    call(c, close_find, {found});
    return result;
}

// A guest time: the game's own local_time of `seconds`, as the fields the Info screen reads.
struct Moment { std::uint32_t minute, hour, mday, month, year, weekday; };
Moment moment(const Builder& b, std::uint32_t seconds) {
    const auto at = b.f.at(0x80);
    b.c.put32(at, seconds);
    const auto time = call(b.c, local_time, {at});
    auto weekday = std::int8_t(b.c.u32(time + 0x18) - 1);
    if (weekday < 0) weekday = std::int8_t(weekday + 7);
    return {b.c.u32(time + 4), b.c.u32(time + 8), b.c.u32(time + 0xc), b.c.u32(time + 0x10), b.c.u32(time + 0x14) + 1900,
            std::uint32_t(weekday)};
}
std::string month_abbreviation(const Builder& b, std::uint32_t month) {
    return guest_string(b.c, call(b.c, month_name, {month}), 3);
}
std::string two(std::uint32_t value) { return (value < 10 ? "0" : "") + std::to_string(value); }
std::string saved_short(const Builder& b, std::int64_t seconds) {
    if (seconds <= 0) return {};
    const auto m = moment(b, std::uint32_t(seconds));
    return std::to_string(m.mday) + " " + month_abbreviation(b, m.month) + " " + std::to_string(m.year) + "  " + two(m.hour) + ":" + two(m.minute);
}
std::string saved_long(const Builder& b, std::int64_t seconds) {
    const auto m = moment(b, std::uint32_t(seconds));
    const auto month = call(b.c, month_name, {m.month});
    const auto day = call(b.c, day_name, {m.weekday});
    b.args({month, m.year});
    return b.localized(text::saved_at, {100, 100, m.hour, m.minute, day, m.mday});
}
// The game date record (day of the year, year, leap flag) in the game's short form.
std::string game_date_short(const Builder& b, const save_catalog::Summary& s) {
    const auto record = b.f.at(0x90), out = b.f.at(0xa0);
    b.c.put16(record, s.day);
    b.c.put16(record + 2, s.year);
    b.c.put8(record + 4, s.leap);
    b.c.put8(out, 0);
    call(b.c, short_date, {record, out, 100, 255});
    return guest_string(b.c, out, 100);
}
std::string game_date_long(const Builder& b, const save_catalog::Summary& s) {
    // As the Info screen: make_date constructs the record, then the saved fields go in.
    const auto record = b.f.at(0x90);
    call(b.c, make_date, {record});
    b.c.put16(record, s.day);
    b.c.put16(record + 2, s.year);
    b.c.put8(record + 4, s.leap);
    call(b.c, date_text, {record, b.in.label, 0x7d0, 0xff});
    return guest_string(b.c, b.in.label);
}
// "Name (Club)" in the game's own wording; the strings go through guest memory.
std::string manager_text(const Builder& b, const save_catalog::Manager& m) {
    const auto name = b.f.at(0xb0), club = b.f.at(0x100);
    put_string(b.c, name, m.name, 0x50);
    put_string(b.c, club, m.club, 0x50);
    return m.club.empty() ? m.name : b.localized(text::manager_with_club, {name, club});
}
std::string played_text(const Builder& b, std::uint32_t seconds) {
    const auto days = seconds / 86400, hours = seconds % 86400 / 3600, minutes = seconds % 3600 / 60;
    return b.localized(text::time_played, {days, hours, minutes, seconds % 60});
}
}

bool saved_game_browser(Cpu& c) {
    return c.s.platform && new_screens_enabled() && adaptive_screen_available(c.s, classic_builder);
}

void build_saved_game_browser(Cpu& c, const Frame& f, const SavedGameBrowserInput& in) {
    const auto mode = call(c, history_value, {in.screens, mode_value}) & 0xff;
    const bool save_mode = mode == 1;
    Builder b{c, f, in, c.toc(slot::plain_colour), c.toc(slot::marked_colour), c.toc(slot::folder_colour), c.toc(slot::row_colour),
              c.toc(slot::list_colour), window(c, in.screens), f.at(0x174)};
    // `plain` and the other colour slots are the addresses of 16-bit colours, as in the classic builder.
    const int line_height = c.i32(c.toc(-0x740c)) != -1 ? 21 : c.i32(c.toc(-0x3bdc) + 3 * 0x1404);
    auto g = saved_game_geometry(gui_canvas(c.s), std::max(line_height, 1), save_mode);

    // The saves, most recent first. The scan is the classic builder's; the order is the only change.
    std::vector<save_catalog::Entry> entries;
    for (auto& item : scan(c, f, in)) {
        save_catalog::Entry entry;
        entry.name = std::move(item.name);
        entry.flagged = item.marked;
        if (const auto path = c.s.platform->guest_file_path(item.path)) {
            entry.path = *path;
            entry.modified = save_catalog::modification_time(*path);
            entry.summary = save_catalog::cached_summary(*path);
        }
        entries.push_back(std::move(entry));
    }
    save_catalog::sort_recent_first(entries);
    row_names.clear();
    for (const auto& entry : entries) row_names.push_back(entry.name);

    // The selection is the dialog's name buffer: the name a click copied there, or one typed in Save.
    // Load starts on the newest save, so one click on the primary button loads it.
    std::string current = guest_string(c, in.name_buffer, 0x1f);
    if (current.empty() && !save_mode && mode == 0 && !entries.empty()) {
        current = entries.front().name;
        put_string(c, in.name_buffer, current, 0x1f);
    }
    int selected = -1;
    for (std::size_t i = 0; i < entries.size(); ++i)
        if (!current.empty() && same_name(entries[i].name, current)) { selected = int(i); break; }

    // The list is as tall as its saves (at least four), up to what the canvas allows.
    {
        const int wanted = std::max(4, int(entries.size())) * g.lines_per_save;
        if (wanted < g.visible_rows) {
            g.visible_rows = wanted;
            g.list.bottom = g.list.top + wanted * g.row_height + 4;
        }
    }

    // The Save name field.
    if (save_mode) {
        const auto field = b.table(g.field, 2, b.widths({22, 78}), 1, {0, 2, 0, none});
        call(c, localized_text, {in.label, in.pool + text::file_label});
        stack(c, f, {c.u16(b.plain), in.label});
        call(c, add_cell, {in.screens, field, 0, 0, 1, 0, 0xc, 3});
        stack(c, f, {1, 0, 0, 1, 3, c.u16(b.list_colour), in.name_buffer, 0x1e, 0, 0, field});
        call(c, add_table_field, {b.window, 0x18, 0, 0, 0, 0, 1, 0});
    }

    // The list: one table row per save on wide canvases (name, club, game date, last saved), two on compact ones.
    const bool two_lines = g.lines_per_save == 2;
    const auto list = two_lines ? b.table(g.list, 2, b.widths({62, 38}), unsigned(g.visible_rows), {0, 1, 0, none})
                                : b.table(g.list, 4, b.widths({30, 26, 19, 25}), unsigned(g.visible_rows), {0, 1, 0, none});
    const int width = g.list.width();
    for (std::size_t i = 0; i < entries.size(); ++i) {
        const auto& entry = entries[i];
        const bool on = int(i) == selected;
        const auto background = on ? b.folder_colour : b.row_colour;
        const unsigned flags = on ? 0x30 : 0x22;
        const auto first_ink = entries[i].flagged ? b.marked : b.plain;
        const unsigned row = unsigned(i) * unsigned(g.lines_per_save);
        const std::string club = entry.summary && !entry.summary->managers.empty()
            ? entry.summary->managers.front().club + (entry.summary->managers.size() > 1 ? "  +" + std::to_string(entry.summary->managers.size() - 1) : "")
            : std::string();
        const std::string date = entry.summary ? game_date_short(b, *entry.summary) : std::string();
        const std::string saved = saved_short(b, entry.saved_at());
        const auto item = std::uint32_t(i);
        if (!two_lines) {
            b.button(list, 0, row, fit(entry.name, width * 30 / 100, 7), first_ink, background, flags, small_font, 2, saved_game_pick_action, item);
            b.button(list, 1, row, fit(club, width * 26 / 100, 7), b.marked, background, flags, small_font, 2, saved_game_pick_action, item);
            b.button(list, 2, row, fit(date, width * 19 / 100, 7), b.marked, background, flags, small_font, 2, saved_game_pick_action, item);
            b.button(list, 3, row, fit(saved, width * 25 / 100, 7), b.marked, background, flags, small_font, 2, saved_game_pick_action, item);
        } else {
            b.button(list, 0, row, fit(entry.name, width * 62 / 100, 7), first_ink, background, flags, small_font, 2, saved_game_pick_action, item);
            b.button(list, 1, row, fit(saved, width * 38 / 100, 7), b.marked, background, flags, small_font, 2, saved_game_pick_action, item);
            b.button(list, 0, row + 1, fit(club, width * 62 / 100, 7), b.marked, background, flags, small_font, 2, saved_game_pick_action, item);
            b.button(list, 1, row + 1, fit(date, width * 38 / 100, 7), b.marked, background, flags, small_font, 2, saved_game_pick_action, item);
        }
    }
    // The panel: the selected save's summary, as the Info screen lists it.
    const auto background = call(c, make_colour, {0, 0x38, 0, 0});
    b.table(g.panel, 1, 0, 1, {0, 0x30, background, none});
    if (selected >= 0) {
        const auto& entry = entries[std::size_t(selected)];
        const auto* summary = entry.summary ? &*entry.summary : nullptr;
        struct Row { std::string label, value; };
        std::vector<Row> rows;
        const auto label = [&](std::uint32_t id) { call(c, localized_text, {in.label, in.pool + id}); return guest_string(c, in.label); };
        rows.push_back({label(text::file_name), entry.name});
        const auto when = entry.saved_at();
        if (when > 0) rows.push_back({label(text::last_saved), saved_long(b, when)});
        if (summary) {
            if (!g.compact) rows.push_back({label(text::version), summary->game_version});
            rows.push_back({label(text::date), game_date_long(b, *summary)});
            rows.push_back({label(text::game_time), played_text(b, summary->played)});
            if (!summary->foreground.empty()) rows.push_back({label(text::foreground_leagues), summary->foreground});
            if (!summary->background.empty()) rows.push_back({label(text::background_leagues), summary->background});
            bool first = true;
            for (const auto& manager : summary->managers) {
                rows.push_back({first ? label(text::managers) : std::string(), manager_text(b, manager)});
                first = false;
            }
        }
        if (int(rows.size()) > g.detail_rows) rows.resize(std::size_t(g.detail_rows));
        const ScreenRect inner{g.panel.left + 8, g.panel.top + 8, g.panel.right - 8, g.panel.top + 8 + int(rows.size()) * g.detail_row_height};
        const auto grid = b.table(inner, 2, b.widths({34, 66}), unsigned(rows.size()), {0, 1, 0, none});
        const auto font = small_font;
        const int per_char = 7;
        for (std::size_t i = 0; i < rows.size(); ++i) {
            b.cell(grid, 0, unsigned(i), rows[i].label, b.plain, 0, 1, font, 2);
            b.cell(grid, 1, unsigned(i), fit(rows[i].value, inner.width() * 66 / 100, per_char), b.marked, 0, 1, font, 2);
        }
    }

    // The list's scroll and the table the click handler asks about, as the classic builder leaves them.
    const auto row = call(c, history_value, {in.screens, 14});
    const auto index = std::uint32_t(c.i16(in.screens + 0x2c58)) * 0x2c0;
    const auto controls = c.u32(in.screens + 0x24 + index);
    const auto table_id = std::int32_t(std::int16_t(list));
    if (table_id != -1) call(c, select_row, {controls + std::uint32_t(table_id) * 0xbf0 + 4, row});
    call(c, set_history_value, {in.screens, 15, std::uint32_t(table_id), 0});

    // Cancel apart, and the primary button at the panel's foot (the footer in compact mode).
    const auto word = [&](const char* Captions::* member) { 
        const auto language = c.u8(c.r(2) - 0x35ac);
        return std::string(captions[language < captions.size() ? language : 0].*member);
    };
    const auto cancel_grid = b.table(g.cancel, 1, 0, 1, {0, 1, 0, none});
    call(c, localized_text, {in.label, in.pool + text::cancel});
    stack(c, f, {c.u16(b.plain), in.label, 3, 0});
    call(c, add_button, {in.screens, cancel_grid, 0, 0, 0x30, c.u16(b.folder_colour), 0xc, 3});
    const auto primary_grid = b.table(g.primary, 1, 0, 1, {0, 1, 0, none});
    b.button(primary_grid, 0, 0, word(mode == 1 ? &Captions::save : mode == 2 ? &Captions::erase : &Captions::load), b.plain,
             b.folder_colour, 0x30, 0xc, 3, 4, 0);
}

std::uint32_t saved_game_pick_name(Cpu& c, std::uint32_t control, std::uint32_t scratch) {
    const auto screens = c.toc(slot::screens);
    const auto id = std::int16_t(control);
    if (id < 0) return 0;
    const auto address = window(c, screens) + control_base + std::uint32_t(id) * control_size;
    const auto item = c.u32(address + control_value);
    if (item >= row_names.size()) return 0;
    put_string(c, scratch, row_names[item], 0x1f);
    return scratch;
}
}
