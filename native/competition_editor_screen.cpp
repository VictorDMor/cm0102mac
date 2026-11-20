#include "competition_editor_screen.hpp"
#include "adaptive_gui.hpp"
#include "competition_defaults.hpp"
#include "competition_editor_document.hpp"
#include "competition_editor_text.hpp"
#include "guest_screen.hpp"
#include "gui_guest_scope.hpp"
#include "platform.hpp"
#include <algorithm>
#include <fstream>
#include <map>
#include <optional>
#include <set>

namespace cm::aot {
namespace {
namespace fs = std::filesystem;
using competition_values::Entry;
using competition_values::Kind;
using competition_values::Parameter;
using T = EditorText;
constexpr unsigned startup_builder = 0x10334550, startup_handler = 0x10334940;
constexpr unsigned game_toc = 0x108bba90;

struct Action {
    enum class Kind { open, back, edition, file, new_file, use, nation, list_page, pick, round, page, step, revert } kind;
    std::int32_t a = 0;
    const Parameter* parameter = nullptr;
};
struct Database {
    fs::path file;
    std::string name;
};
// One editor per process: the start screen exists once, and nothing here belongs to a career.
struct Editor {
    bool open = false;
    std::vector<CompetitionEdition> editions;
    std::size_t edition = 0;
    std::optional<CompetitionEditorDocument> document;
    std::vector<Database> files;           // this edition's custom databases
    std::optional<std::size_t> file;       // none: the original rules, nothing written yet
    std::vector<std::string> nations;      // the filter's choices after "all": nation codes, "" international
    std::size_t nation = 0;                // 0: all
    std::size_t list_page = 0, page = 0;
    std::optional<std::size_t> selected;   // index in the document's competitions
    std::int32_t round = 0;
    std::string message;                   // UTF-8
    std::map<std::int16_t, Action> actions;
    std::uint32_t display = 0;             // the start screen's table, while the editor's controls are on it
};
Editor editor;

struct Paths {
    fs::path bases, recorded, databases, selection;
};
Paths paths(const State& s) {
    const auto recorded = s.platform->competition_defaults_directory();
    const auto writable = recorded.parent_path();
    return {writable.parent_path() / "Competition Bases", recorded, writable / "Custom Databases",
            writable / "Custom Databases" / "selected.txt"};
}
unsigned language(const State& s) { return std::uint32_t(s.memory.read<1>(game_toc - 0x35ac)); }

// The game's records and controls hold Windows-1252 text.
std::string game_text(std::string_view utf8) {
    std::string out;
    for (std::size_t i = 0; i < utf8.size(); ++i) {
        const auto b = std::uint8_t(utf8[i]);
        if (b < 0x80) { out += char(b); continue; }
        unsigned code = '?';
        std::size_t extra = 0;
        if ((b & 0xe0) == 0xc0) { code = b & 0x1f; extra = 1; }
        else if ((b & 0xf0) == 0xe0) { code = b & 0x0f; extra = 2; }
        else if ((b & 0xf8) == 0xf0) { code = b & 0x07; extra = 3; }
        for (std::size_t k = 0; k < extra && i + 1 < utf8.size(); ++k) code = code << 6 | (std::uint8_t(utf8[++i]) & 0x3f);
        out += code == 0x2026 ? std::string("...") : code == 0x2013 || code == 0x2014 ? std::string("-")
             : code < 0x100 && code >= 0xa0 ? std::string(1, char(code)) : std::string("?");
    }
    return out;
}

std::vector<Entry> values_of(const Paths& p, CompetitionEdition edition) {
    std::vector<Entry> out;
    std::set<std::string> seen;
    for (const auto& directory : {p.bases, p.recorded})
        for (auto& e : read_competition_defaults(CompetitionDefaults::file(directory, edition)))
            if (seen.insert(competition_values::key(e.identity)).second) out.push_back(std::move(e));
    return out;
}

std::string read_file(const fs::path& path) {
    std::ifstream input(path, std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
}
bool write_file(const fs::path& path, const std::string& text) {
    std::error_code error;
    fs::create_directories(path.parent_path(), error);
    auto temporary = path;
    temporary += ".writing";
    {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        if (!output.write(text.data(), std::streamsize(text.size())).flush()) {
            output.close();
            fs::remove(temporary, error);
            return false;
        }
    }
    fs::rename(temporary, path, error);
    if (error) { fs::remove(temporary, error); return false; }
    return true;
}

void filter_reset() {
    editor.list_page = editor.page = 0;
    editor.selected.reset();
    editor.round = 0;
}
// A competition and its stages (the same id), the competition itself first.
std::vector<std::size_t> group_of(std::int32_t id) {
    std::vector<std::size_t> out;
    if (!editor.document) return out;
    const auto& all = editor.document->competitions();
    for (std::size_t i = 0; i < all.size(); ++i)
        if (all[i].identity.id == id) out.push_back(i);
    std::stable_sort(out.begin(), out.end(), [&](std::size_t a, std::size_t b) { return all[a].identity.stage < all[b].identity.stage; });
    return out;
}
// The list's rows: one per competition, its stages behind it.
std::vector<std::size_t> shown() {
    std::vector<std::size_t> out;
    if (!editor.document) return out;
    const auto& all = editor.document->competitions();
    std::set<std::int32_t> listed;
    for (std::size_t i = 0; i < all.size(); ++i) {
        if (editor.nation && all[i].identity.nation != editor.nations[editor.nation - 1]) continue;
        if (listed.insert(all[i].identity.id).second) out.push_back(group_of(all[i].identity.id).front());
    }
    return out;
}

// Loads the edition's values and lists its databases; `keep` is the database file to stay on, if it is there.
void load_edition(const State& s, const std::optional<fs::path>& keep) {
    const auto p = paths(s);
    const auto edition = editor.editions.at(editor.edition);
    editor.document.emplace(edition, values_of(p, edition));
    editor.files.clear();
    editor.file.reset();
    std::error_code error;
    std::vector<fs::path> found;
    for (fs::directory_iterator it(p.databases, error), end; !error && it != end; it.increment(error))
        if (it->path().extension() == ".toml") found.push_back(it->path());
    std::sort(found.begin(), found.end());
    for (const auto& path : found) {
        const auto database = competition_overrides::read_custom_database(read_file(path));
        if (!database || database->base != edition) continue;
        editor.files.push_back({path, database->name.empty() ? path.stem().string() : database->name});
    }
    editor.nations.clear();
    std::set<std::string> codes;
    for (const auto& e : editor.document->competitions()) codes.insert(e.identity.nation);
    for (const auto& code : codes) editor.nations.push_back(code);
    if (editor.nation > editor.nations.size()) editor.nation = 0;
    if (keep)
        for (std::size_t i = 0; i < editor.files.size(); ++i)
            if (editor.files[i].file == *keep) editor.file = i;
}
// The document's edits become the current database's (none: the original rules).
void load_file(const State& s) {
    const auto edition = editor.editions.at(editor.edition);
    const auto selected = editor.selected;
    editor.document.emplace(edition, values_of(paths(s), edition));
    editor.selected = selected;
    if (!editor.file) return;
    const auto& entry = editor.files[*editor.file];
    const auto database = competition_overrides::read_custom_database(read_file(entry.file));
    if (!database) { editor.message = std::string(editor_text(language(s), T::cannot_read)); editor.file.reset(); return; }
    if (const auto why = editor.document->load(*database); !why.empty()) { editor.message = why; editor.file.reset(); return; }
    editor.document->name = entry.name;
}
void open_editor(const State& s) {
    const auto p = paths(s);
    editor = {};
    editor.open = true;
    for (std::size_t e = 0; e < competition_edition_count; ++e)
        if (!values_of(p, CompetitionEdition(e)).empty()) editor.editions.push_back(CompetitionEdition(e));
    if (editor.editions.empty()) return;
    // Start on the database new careers use, else on the game's own season.
    const auto chosen = selected_custom_database(p.selection);
    std::optional<CompetitionEdition> start;
    if (chosen)
        if (const auto database = competition_overrides::read_custom_database(read_file(*chosen))) start = database->base;
    const auto wanted = start.value_or(CompetitionEdition::cm0102);
    for (std::size_t i = 0; i < editor.editions.size(); ++i)
        if (editor.editions[i] == wanted) editor.edition = i;
    load_edition(s, chosen);
    load_file(s);
}

// Tells the game which database new careers start with, and keeps the choice. A file the game refuses leaves none
// chosen (select_custom_database), and so does the selection file.
void choose(State& s, std::optional<fs::path> file) {
    const auto p = paths(s);
    std::error_code error;
    if (const auto why = s.platform->select_custom_database(file); !why.empty()) {
        editor.message = why;
        file.reset();
    }
    if (!file) { fs::remove(p.selection, error); return; }
    fs::create_directories(p.selection.parent_path(), error);
    std::ofstream output(p.selection, std::ios::binary | std::ios::trunc);
    if (!(output << file->string() << '\n').flush()) editor.message = std::string(editor_text(language(s), T::choice_not_saved));
}
bool save(State& s) {
    if (!editor.document || !editor.file) return false;
    const auto& entry = editor.files[*editor.file];
    editor.document->name = entry.name;
    if (!write_file(entry.file, competition_overrides::write_custom_database(editor.document->database()))) {
        editor.message = std::string(editor_text(language(s), T::not_saved));
        return false;
    }
    editor.document->saved();
    // The game holds what it read when the file was chosen: hand the chosen file over again.
    if (const auto chosen = selected_custom_database(paths(s).selection); chosen && *chosen == entry.file) choose(s, entry.file);
    return true;
}
// A new, empty database of the current edition, written at once so that it is listed.
bool new_file(State& s) {
    const auto p = paths(s);
    const auto base = std::string(editor_text(language(s), T::default_name));
    for (unsigned n = 1; n < 1000; ++n) {
        const auto file = p.databases / ("custom-" + std::to_string(n) + ".toml");
        std::error_code error;
        if (fs::exists(file, error)) continue;
        editor.files.push_back({file, base + " " + std::to_string(n)});
        editor.file = editor.files.size() - 1;
        if (save(s)) return true;
        editor.files.pop_back();
        editor.file.reset();
        return false;
    }
    return false;
}

std::string refusal_text(unsigned lang, const CompetitionEditorDocument::Refusal& refusal) {
    using R = CompetitionEditorDocument::Refusal::Kind;
    const auto a = std::to_string(refusal.a), b = std::to_string(refusal.b);
    switch (refusal.kind) {
    case R::none: return {};
    case R::range: return filled(editor_text(lang, T::need_range), {a, b});
    case R::places: return filled(editor_text(lang, T::places_exceed), {a, b});
    case R::head_to_head: return std::string(editor_text(lang, T::head_to_head_refused));
    case R::no_round: return std::string(editor_text(lang, T::no_such_round));
    case R::other: return refusal.detail;
    }
    return {};
}
bool is_tie_break(const Parameter& p) { return p.id.starts_with("league.tiebreak."); }
std::string value_text(unsigned lang, const Parameter& p, std::int32_t value) {
    if (is_tie_break(p) && value >= 0 && value <= 6) {
        constexpr T names[]{T::tie_break_end, T::tie_break_goal_difference, T::tie_break_goals_scored, T::tie_break_wins,
                            T::tie_break_head_to_head, T::tie_break_away_goals, T::tie_break_keep_order};
        return std::string(editor_text(lang, names[value]));
    }
    return std::to_string(value);
}
// The steps the two pairs of buttons make: one for small ranges, money in thousands.
std::pair<std::int32_t, std::int32_t> steps(const Parameter& p) {
    const auto span = std::int64_t(p.max) - p.min;
    if (span > 100000) return {10000, 100000};
    if (span > 300) return {1, 10};
    return {1, 0};
}
std::int32_t stepped(const Parameter& p, std::int32_t value, std::int32_t delta) {
    if (!p.allowed.empty()) {
        auto at = std::find(p.allowed.begin(), p.allowed.end(), value);
        if (at == p.allowed.end()) return p.allowed.front();
        // Head-to-head (4) is refused by the document until stage 2: the steps pass over it.
        const auto refused = [&](std::int32_t v) { return is_tie_break(p) && v == 4; };
        auto index = std::int64_t(at - p.allowed.begin());
        do index += delta > 0 ? 1 : -1;
        while (index >= 0 && index < std::int64_t(p.allowed.size()) && refused(p.allowed[std::size_t(index)]));
        return index < 0 || index >= std::int64_t(p.allowed.size()) ? value : p.allowed[std::size_t(index)];
    }
    // -1 is a code ("share of the pool"): it steps to and from zero.
    if (value < 0) return delta > 0 ? 0 : value;
    if (value == 0 && delta < 0 && p.min < 0) return p.min;
    return std::int32_t(std::clamp<std::int64_t>(std::int64_t(value) + delta, std::max<std::int32_t>(p.min, 0), p.max));
}

void act(State& s, const Action& action) {
    using K = Action::Kind;
    const auto lang = language(s);
    editor.message.clear();
    switch (action.kind) {
    case K::open: open_editor(s); return;
    case K::back: editor = {}; return;
    case K::edition:
        editor.edition = std::size_t(action.a);
        editor.nation = 0;
        filter_reset();
        load_edition(s, std::nullopt);
        load_file(s);
        return;
    case K::file:
        editor.file = action.a < 0 ? std::nullopt : std::optional<std::size_t>(std::size_t(action.a));
        load_file(s);
        return;
    case K::new_file:
        // Empty, not a copy of the database on screen.
        editor.file.reset();
        load_file(s);
        if (new_file(s)) load_file(s);
        return;
    case K::use: {
        const auto chosen = selected_custom_database(paths(s).selection);
        if (!editor.file) choose(s, std::nullopt);
        else if (chosen && *chosen == editor.files[*editor.file].file) choose(s, std::nullopt);
        else choose(s, editor.files[*editor.file].file);
        return;
    }
    case K::nation: editor.nation = std::size_t(action.a); filter_reset(); return;
    case K::list_page: editor.list_page = std::size_t(action.a); return;
    case K::pick: editor.selected = std::size_t(action.a); editor.round = 0; editor.page = 0; return;
    case K::round: editor.round = action.a; return;
    case K::page: editor.page = std::size_t(action.a); return;
    case K::step: {
        if (!editor.document || !editor.selected || !action.parameter) return;
        const auto& p = *action.parameter;
        const auto round = p.scope == competition_values::Scope::cup_round ? editor.round : -1;
        const auto next = stepped(p, editor.document->value(*editor.selected, p, round), action.a);
        if (const auto refusal = editor.document->set(*editor.selected, p, round, next); !refusal.empty()) {
            editor.message = refusal_text(lang, refusal);
            return;
        }
        // The first change to the original rules starts a database of its own (new_file saves it).
        if (editor.file) save(s);
        else if (!new_file(s)) load_file(s);
        return;
    }
    case K::revert:
        if (!editor.document || !editor.selected || !editor.file) return;
        editor.document->revert(*editor.selected);
        save(s);
        return;
    }
}

// The screen's controls, made by the game's own routines on the caller's scratch stack.
struct Builder {
    State& s;
    guest::Cpu c;
    std::uint32_t manager, display;
    std::uint32_t ink, background, highlight;
    unsigned lang;

    explicit Builder(State& state) : s(state), c(state), manager(c.toc(-0x7bb0)), display(c.u32(manager + 0x24)),
        ink(c.u16(c.toc(-0x7bc8))), background(c.u16(c.toc(-0x7bc4))), highlight(c.u16(c.toc(-0x7bd0))), lang(language(state)) {}

    std::int16_t make(ScreenRect box, const std::string& text, bool button, bool enabled, std::uint32_t colour) {
        const auto at = c.sp() + 0x100;
        const auto bytes = text.substr(0, 200);
        for (std::size_t i = 0; i <= bytes.size(); ++i) c.put8(at + std::uint32_t(i), i == bytes.size() ? 0 : std::uint8_t(bytes[i]));
        c.put32(c.sp() + 0x38, 1);
        c.put32(c.sp() + 0x3c, colour);
        c.put32(c.sp() + 0x40, at);
        c.put32(c.sp() + 0x44, button ? 0x7100 : 0);
        c.put32(c.sp() + 0x48, 0);
        unsigned reg = 3;
        for (const auto value : {manager, unsigned(box.left), unsigned(box.top), unsigned(box.right), unsigned(box.bottom),
                                 button ? (enabled ? 0x30u : 0x830u) : 1u, button ? background : 0u, 12u})
            c.set(reg++, value);
        c.call(button ? 0x10744430 : 0x10744390, 0xfffffff0);
        return std::int16_t(c.r(3));
    }
    void label(ScreenRect box, const std::string& text, bool marked = false) { make(box, text, false, true, marked ? highlight : ink); }
    void button(ScreenRect box, const std::string& text, std::optional<Action> action, bool marked = false) {
        const auto index = make(box, text, true, action.has_value(), marked ? highlight : ink);
        if (action && index >= 0) editor.actions[index] = *action;
    }
    std::string tr(T text) const { return game_text(editor_text(lang, text)); }
    std::string tr(T text, std::initializer_list<std::string> values) const { return game_text(filled(editor_text(lang, text), values)); }
    // "<" value ">" over a list of choices: returns the rectangle's right edge.
    int chooser(int left, int top, int width, const std::string& text, std::optional<Action> previous, std::optional<Action> next, bool marked = false) {
        // An end of the list has no arrow: a disabled button reads as a selected one.
        if (previous) button({left, top, left + 22, top + 20}, "<", previous);
        label({left + 24, top, left + 24 + width, top + 20}, text, marked);
        if (next) button({left + 26 + width, top, left + 48 + width, top + 20}, ">", next);
        return left + 48 + width;
    }
};

std::string edition_text(const Builder& b, CompetitionEdition edition) {
    const auto year = edition_season(edition);
    const auto season = std::to_string(year) + "/" + (year % 100 + 1 < 10 ? "0" : "") + std::to_string(year % 100 + 1);
    return b.tr(T::edition_database, {season});
}
std::string competition_text(const Builder& b, const Entry& e) {
    auto text = e.identity.name;
    if (e.identity.stage >= 0) text += " (" + b.tr(T::stage, {std::to_string(e.identity.stage + 1)}) + ")";
    return text;
}

void hide_start_choices(Builder& b) {
    const GuestScreen screen{b.c, b.display};
    const unsigned controls = std::min<unsigned>(1200, b.c.u16(b.display + 0x129da6));
    std::set<int> areas;
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = screen.control(i);
        const auto action = b.c.i16(control + 0xa);
        if (action < 1 || action > 9 || b.c.i16(control + 0x72) < 0) continue;
        areas.insert(b.c.i16(control + 0x72));
        // Neither picked nor found by the Game menu's commands.
        b.c.put32(control + 0xc, b.c.u32(control + 0xc) & ~2u);
        // The cell's own position (the adaptive layout's override), out of the canvas.
        b.c.put32(control + 0x28, std::uint32_t(-4000)); b.c.put32(control + 0x2c, std::uint32_t(-3900));
        b.c.put32(control + 0x30, std::uint32_t(-4000)); b.c.put32(control + 0x34, std::uint32_t(-3900));
    }
    for (const int area : areas) {
        screen.write(screen.grid(unsigned(area)), {-4000, -4000, -3400, -3600});
        screen.measure(screen.grid(unsigned(area)));
        screen.publish(screen.grid(unsigned(area)));
    }
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = screen.control(i);
        const auto action = b.c.i16(control + 0xa);
        if (action >= 1 && action <= 9 && b.c.i16(control + 0x72) >= 0) screen.write(control + 0x10, {-4000, -4000, -3900, -3900});
    }
}
void retitle(Builder& b, const std::string& text) {
    const GuestScreen screen{b.c, b.display};
    const ScreenLayout shell{gui_canvas(b.s)};
    const unsigned controls = std::min<unsigned>(1200, b.c.u16(b.display + 0x129da6));
    for (unsigned i = 0; i < controls; ++i) {
        const auto control = screen.control(i);
        const auto box = screen.rect(control + 0x10);
        if (b.c.i16(control + 0x72) >= 0 || (box != shell.subtitle() && box != ScreenRect{100, 80, 790, 125})) continue;
        for (std::size_t k = 0; k <= text.size() && k < 0xff; ++k) b.c.put8(control + 0x78 + std::uint32_t(k), k == text.size() ? 0 : std::uint8_t(text[k]));
        b.c.put8(control + 0x178, 1);
    }
}

// The editor's panel: the content area, as build_editor lays it out.
ScreenRect panel_of(const State& s) {
    auto area = ScreenLayout{gui_canvas(s)}.content();
    const int width = std::min(1180, area.width());
    area.left += (area.width() - width) / 2;
    area.right = area.left + width;
    return area;
}

void build_editor(Builder& b) {
    using K = Action::Kind;
    hide_start_choices(b);
    retitle(b, b.tr(T::window_title));
    const auto area = panel_of(b.s);
    const int width = area.width();
    const int bottom = area.bottom;
    b.button({area.right - 150, bottom - 22, area.right, bottom}, b.tr(T::back), Action{K::back});
    if (!editor.document) {
        b.label({area.left, area.top + 40, area.right, area.top + 60}, b.tr(T::no_bases));
        return;
    }
    const auto& document = *editor.document;
    const auto p = paths(b.s);

    // The base season, the database and whether new careers use it.
    int x = area.left;
    const auto e = editor.edition;
    x = b.chooser(x, area.top, 170, edition_text(b, editor.editions[e]),
                  e > 0 ? std::optional(Action{K::edition, std::int32_t(e) - 1}) : std::nullopt,
                  e + 1 < editor.editions.size() ? std::optional(Action{K::edition, std::int32_t(e) + 1}) : std::nullopt) + 12;
    // -1 (the original rules), then the databases in order.
    const auto at = editor.file ? std::int32_t(*editor.file) : -1;
    const auto name = editor.file ? game_text(editor.files[*editor.file].name) : b.tr(T::original_rules);
    const int name_width = std::clamp(area.right - x - 48 - 12 - 70 - 12 - 190 - 46, 110, 240);
    x = b.chooser(x, area.top, name_width, name, at >= 0 ? std::optional(Action{K::file, at - 1}) : std::nullopt,
                  at + 1 < std::int32_t(editor.files.size()) ? std::optional(Action{K::file, at + 1}) : std::nullopt,
                  editor.file.has_value()) + 6;
    b.button({x, area.top, x + 64, area.top + 20}, b.tr(T::new_database), Action{K::new_file});
    x += 64 + 12;
    const auto chosen = selected_custom_database(p.selection);
    const bool used = editor.file ? chosen && *chosen == editor.files[*editor.file].file : !chosen;
    const int use_right = area.right;
    b.label({x, area.top, use_right - 46, area.top + 20}, b.tr(T::use_for_new));
    b.button({use_right - 44, area.top, use_right, area.top + 20}, b.tr(used ? T::yes : T::no), Action{K::use}, used);

    // Left: the nation filter and the competitions, in pages.
    const int left_width = std::clamp(width * 36 / 100, 215, 420);
    const int list_right = area.left + left_width;
    const int top = area.top + 28;
    const auto nation_name = [&](std::size_t index) {
        if (!index) return b.tr(T::all_nations);
        const auto& code = editor.nations[index - 1];
        return code.empty() ? b.tr(T::international) : code;
    };
    b.chooser(area.left, top, left_width - 48, nation_name(editor.nation),
              editor.nation > 0 ? std::optional(Action{K::nation, std::int32_t(editor.nation) - 1}) : std::nullopt,
              editor.nation < editor.nations.size() ? std::optional(Action{K::nation, std::int32_t(editor.nation) + 1}) : std::nullopt);
    const auto list = shown();
    const int row = 19, list_top = top + 26, list_bottom = bottom - 26;
    const std::size_t per_page = std::size_t(std::max(1, (list_bottom - list_top) / row));
    const std::size_t pages = std::max<std::size_t>(1, (list.size() + per_page - 1) / per_page);
    editor.list_page = std::min(editor.list_page, pages - 1);
    for (std::size_t i = editor.list_page * per_page, line = 0; i < list.size() && line < per_page; ++i, ++line) {
        const auto& entry = document.competitions()[list[i]];
        const int y = list_top + int(line) * row;
        const auto group = group_of(entry.identity.id);
        const bool current = editor.selected && std::ranges::find(group, *editor.selected) != group.end();
        const bool modified = std::ranges::any_of(group, [&](std::size_t g) { return document.modified(g); });
        auto text = entry.identity.name;
        if (modified) text = "* " + text;
        b.button({area.left, y, list_right, y + row - 1}, text, current ? std::nullopt : std::optional(Action{K::pick, std::int32_t(list[i])}),
                 modified);
    }
    b.chooser(area.left, bottom - 22, left_width - 48, std::to_string(editor.list_page + 1) + " / " + std::to_string(pages),
              editor.list_page > 0 ? std::optional(Action{K::list_page, std::int32_t(editor.list_page) - 1}) : std::nullopt,
              editor.list_page + 1 < pages ? std::optional(Action{K::list_page, std::int32_t(editor.list_page) + 1}) : std::nullopt);

    // Right: the selected competition's values.
    const int right = list_right + 16;
    if (!editor.selected) {
        b.label({right, top, area.right, top + 20}, editor.message.empty() ? b.tr(T::choose) : game_text(editor.message));
        return;
    }
    const auto selected = *editor.selected;
    const auto& entry = document.competitions()[selected];
    const auto group = group_of(entry.identity.id);
    if (group.size() > 1) {
        const auto at = std::size_t(std::ranges::find(group, selected) - group.begin());
        b.chooser(right, top, std::min(420, area.right - right - 48), competition_text(b, entry),
                  at > 0 ? std::optional(Action{K::pick, std::int32_t(group[at - 1])}) : std::nullopt,
                  at + 1 < group.size() ? std::optional(Action{K::pick, std::int32_t(group[at + 1])}) : std::nullopt, true);
    } else b.label({right, top, area.right, top + 20}, competition_text(b, entry), true);
    struct Line {
        const Parameter* parameter;
        std::int32_t round;
    };
    std::vector<Line> lines;
    for (const auto* parameter : CompetitionEditorDocument::parameters(entry.kind, false)) lines.push_back({parameter, -1});
    const auto rounds = std::int32_t(entry.rounds.size());
    int y = top + 26;
    if (entry.kind == Kind::cup && rounds > 0) {
        editor.round = std::clamp(editor.round, 0, rounds - 1);
        for (const auto* parameter : CompetitionEditorDocument::parameters(entry.kind, true)) lines.push_back({parameter, editor.round});
    }
    const int value_bottom = bottom - 72;
    const bool round_row = entry.kind == Kind::cup && rounds > 0;
    const std::size_t per = std::size_t(std::max(1, (value_bottom - y - (round_row ? 22 : 0)) / 21));
    const std::size_t value_pages = std::max<std::size_t>(1, (lines.size() + per - 1) / per);
    editor.page = std::min(editor.page, value_pages - 1);
    const int buttons = 4 * 30;
    const int value_width = std::clamp((area.right - right) * 26 / 100, 80, 170);
    const int value_left = area.right - buttons - value_width - 4;
    bool round_header = false;
    for (std::size_t i = editor.page * per, n = 0; i < lines.size() && n < per; ++i, ++n) {
        const auto& line = lines[i];
        const auto& parameter = *line.parameter;
        if (line.round >= 0 && !round_header) {
            round_header = true;
            b.chooser(right, y, 200, b.tr(T::round_of, {std::to_string(editor.round + 1), std::to_string(rounds)}),
                      editor.round > 0 ? std::optional(Action{K::round, editor.round - 1}) : std::nullopt,
                      editor.round + 1 < rounds ? std::optional(Action{K::round, editor.round + 1}) : std::nullopt);
            y += 22;
        }
        const auto value = document.value(selected, parameter, line.round);
        const bool edited = document.edited(selected, parameter, line.round);
        b.label({right, y, value_left - 4, y + 20}, game_text(editor_label(b.lang, parameter)), edited);
        auto text = value_text(b.lang, parameter, value);
        if (edited) text += " (" + value_text(b.lang, parameter, document.original(selected, parameter, line.round)) + ")";
        b.label({value_left, y, value_left + value_width, y + 20}, game_text(text), edited);
        if (!parameter.read_only) {
            const auto [small, large] = steps(parameter);
            const auto step = [&](std::int32_t delta) -> std::optional<Action> {
                if (stepped(parameter, value, delta) == value) return std::nullopt;
                return Action{K::step, delta, &parameter};
            };
            const int bx = area.right - buttons;
            if (large && step(-large)) b.button({bx, y, bx + 28, y + 20}, "--", step(-large));
            if (step(-small)) b.button({bx + 30, y, bx + 58, y + 20}, "-", step(-small));
            if (step(small)) b.button({bx + 60, y, bx + 88, y + 20}, "+", step(small));
            if (large && step(large)) b.button({bx + 90, y, bx + 118, y + 20}, "++", step(large));
        }
        y += 21;
    }
    if (value_pages > 1)
        b.chooser(right, bottom - 22, 70, std::to_string(editor.page + 1) + " / " + std::to_string(value_pages),
                  editor.page > 0 ? std::optional(Action{K::page, std::int32_t(editor.page) - 1}) : std::nullopt,
                  editor.page + 1 < value_pages ? std::optional(Action{K::page, std::int32_t(editor.page) + 1}) : std::nullopt);
    // Revert takes back the edits from the career's start; entries that start in a later season stay.
    bool revertable = false;
    for (const auto* parameter : CompetitionEditorDocument::parameters(entry.kind, false)) revertable |= document.edited(selected, *parameter, -1);
    for (std::int32_t r = 0; r < rounds; ++r)
        for (const auto* parameter : CompetitionEditorDocument::parameters(entry.kind, true)) revertable |= document.edited(selected, *parameter, r);
    if (revertable && editor.file)
        b.button({area.right - 158 - (width > 800 ? 200 : 150), bottom - 22, area.right - 158, bottom}, b.tr(T::revert), Action{K::revert});

    // What the user should know: a refusal, else the warnings of this competition's edits, one per line.
    std::vector<std::string> notes;
    if (!editor.message.empty()) notes.push_back(editor.message);
    else
        for (const auto& warning : document.warnings(selected)) {
            using W = CompetitionEditorDocument::Warning::Kind;
            switch (warning.kind) {
            case W::places_unchecked: notes.emplace_back(editor_text(b.lang, T::warn_places)); break;
            case W::more_substitutions_than_bench: notes.emplace_back(editor_text(b.lang, T::warn_substitutions)); break;
            case W::scheduled: notes.push_back(filled(editor_text(b.lang, T::warn_scheduled), {std::to_string(warning.n)})); break;
            }
        }
    for (std::size_t i = 0; i < notes.size() && i < 2; ++i)
        b.label({right, bottom - 68 + int(i) * 21, area.right, bottom - 48 + int(i) * 21}, game_text(notes[i]), true);
}

void build_start_button(Builder& b) {
    const auto canvas = gui_canvas(b.s);
    const ScreenLayout shell{canvas};
    // Classic: the choices' empty sixth row. A larger canvas: the footer's left end.
    const auto content = shell.content();
    const int width = 300;
    const ScreenRect box = canvas.original() ? ScreenRect{445 - width / 2, 492, 445 + width / 2, 522}
        : ScreenRect{content.left + (content.width() - std::min(1120, content.width())) / 2, canvas.height - 45,
                     content.left + (content.width() - std::min(1120, content.width())) / 2 + width, canvas.height - 10};
    b.button(box, b.tr(T::window_title), Action{Action::Kind::open});
}

// As the season selector: only the local start screen, never a network session or a running career.
bool local_setup(guest::Cpu& c) {
    const auto manager = c.toc(-0x7bb0);
    return manager && !c.u16(manager + 0x2c58) && !c.u32(manager + 0x2c64) && !c.u32(manager + 4) && !c.u8(c.toc(-0x79e4));
}
}

bool competition_editor_event(State& s, std::uint32_t handler, std::int16_t control) {
    if (!s.platform || handler != startup_handler) return false;
    guest::Cpu c(s);
    if (!local_setup(c)) return false;
    const auto found = editor.actions.find(control);
    if (found == editor.actions.end()) return false;
    const auto action = found->second;
    act(s, action);
    return true;
}

void competition_editor_backdrop(State& s, std::uint32_t table) {
    if (!editor.open || table != editor.display) return;
    const auto canvas = gui_canvas(s);
    const auto destination = std::uint32_t(s.memory.read<4>(game_toc - 0x3564));
    const auto stride = std::uint32_t(s.memory.read<2>(game_toc - 0x3566));
    if (!destination || stride < std::uint32_t(canvas.width) * 2) return;
    auto box = panel_of(s);
    box = {std::max(0, box.left - 8), std::max(0, box.top - 8), std::min(canvas.width, box.right + 8), std::min(canvas.height, box.bottom + 8)};
    if (box.width() <= 0 || box.height() <= 0) return;
    // 15-bit pixels, big-endian: a quarter of each channel.
    for (int y = box.top; y < box.bottom; ++y) {
        const auto at = destination + std::uint32_t(y) * stride + std::uint32_t(box.left) * 2;
        auto row = s.memory.copy_out(at, std::uint64_t(box.width()) * 2);
        for (std::size_t i = 0; i + 1 < row.size(); i += 2) {
            const unsigned pixel = (unsigned(row[i]) << 8 | row[i + 1]) >> 2 & 0x1ce7;
            row[i] = std::uint8_t(pixel >> 8); row[i + 1] = std::uint8_t(pixel);
        }
        s.memory.copy_in(at, row);
    }
}

void competition_editor_after_build(State& s, std::uint32_t builder) {
    editor.actions.clear();
    editor.display = 0;
    if (builder != startup_builder) { editor.open = false; return; }
    if (!s.platform) return;
    {
        guest::Cpu c(s);
        if (!local_setup(c)) { editor.open = false; return; }
    }
    GuiGuestScope preserve(s);
    Builder b(s);
    b.c.set(1, b.c.sp() - 0x400);
    if (editor.open) { editor.display = b.display; build_editor(b); }
    else build_start_button(b);
}
}
