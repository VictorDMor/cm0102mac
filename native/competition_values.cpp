#include "competition_values.hpp"
#include "aot_runtime.hpp"
#include "comp_detail.hpp"
#include "cup_base_detail.hpp"
#include "league_base_detail.hpp"
#include <algorithm>
#include <charconv>
#include <cstdio>
#include <set>
#include <tuple>

namespace cm::aot::competition_values {

namespace {
constexpr std::int32_t kMax = INT32_MAX;
constexpr std::int32_t tie_break_codes[] = {0, 1, 2, 3, 4, 5, 6};  // 0 ends the list; fields.md
using E = Effect;
using S = Scope;
constexpr Parameter registry[] = {
    // League (LEAGUE.CPP object, kind byte 2).
    {"league.promotion.automatic", S::league, 0xc0, 1, true, false, 0, 127, {}, "Automatic promotion places", E::at_once},
    {"league.promotion.playoff", S::league, 0xc1, 1, false, false, 0, 255, {}, "Promotion playoff clubs", E::at_once},
    {"league.relegation.places", S::league, 0xc3, 1, true, false, 0, 127, {}, "Relegation places", E::at_once},
    {"league.points.win", S::league, 0xc4, 1, true, false, 0, 127, {}, "Points for a win", E::results_after_edit},
    {"league.points.draw", S::league, 0xc5, 1, true, false, 0, 127, {}, "Points for a draw", E::results_after_edit},
    {"league.tiebreak.1", S::league, 0xc7, 1, true, false, 0, 6, tie_break_codes, "Tie-break criterion 1", E::at_once},
    {"league.tiebreak.2", S::league, 0xc8, 1, true, false, 0, 6, tie_break_codes, "Tie-break criterion 2", E::at_once},
    {"league.tiebreak.3", S::league, 0xc9, 1, true, false, 0, 6, tie_break_codes, "Tie-break criterion 3", E::at_once},
    {"league.tiebreak.4", S::league, 0xca, 1, true, false, 0, 6, tie_break_codes, "Tie-break criterion 4", E::at_once},
    {"league.bench", S::league, 0x46, 1, false, false, 0, 9, {}, "Named reserves per match", E::fixtures_after_edit},
    {"league.substitutions", S::league, 0x47, 1, false, false, 0, 9, {}, "Substitutions allowed", E::fixtures_after_edit},
    {"league.prize.pool", S::league, 0x28, 4, true, false, 0, kMax, {}, "Prize pool", E::season_end},
    {"league.prize.sharing", S::league, 0xe1, 1, false, false, 0, 3, {}, "How the prize pool is shared", E::season_end},
    {"league.ruling_body", S::league, 0x4a, 1, true, false, -1, 41, {}, "Ruling body (discipline rules)", E::at_once},
    {"league.structure.clubs", S::league, 0x3e, 2, true, true, 0, 32767, {}, "Number of clubs", E::at_once},
    {"league.structure.meetings", S::league, 0x3c, 2, true, true, 0, 32767, {}, "Meetings per pair of clubs", E::at_once},
    {"league.structure.rounds", S::league, 0xa6, 2, true, true, 0, 32767, {}, "Schedule rounds", E::at_once},
    // Cup (CUP.CPP object, kind byte 1).
    {"cup.bench", S::cup, 0x46, 1, false, false, 0, 9, {}, "Named reserves per match", E::fixtures_after_edit},
    {"cup.substitutions", S::cup, 0x47, 1, false, false, 0, 9, {}, "Substitutions allowed", E::fixtures_after_edit},
    {"cup.prize.pool", S::cup, 0x28, 4, true, false, 0, kMax, {}, "Prize pool", E::season_end},
    {"cup.ruling_body", S::cup, 0x4a, 1, true, false, -1, 41, {}, "Ruling body (discipline rules)", E::at_once},
    {"cup.structure.rounds", S::cup, 0x3c, 2, true, true, 0, 32767, {}, "Number of rounds", E::at_once},
    // Cup round row (0x78 bytes at cup +0xa0).
    {"cup.round.legs", S::cup_round, 0x23, 1, true, false, 1, 2, {}, "Legs per tie", E::rounds_not_drawn},
    {"cup.round.replay_ties", S::cup_round, 0x22, 1, true, false, 0, 127, {}, "Ties needed in a single-leg round (replays)", E::rounds_not_drawn},
    {"cup.round.prize.match", S::cup_round, 0x6c, 4, true, false, 0, kMax, {}, "Prize per club per match played", E::rounds_not_drawn},
    {"cup.round.prize.winner", S::cup_round, 0x70, 4, true, false, -1, kMax, {}, "Winner's prize (-1: share of the pool)", E::rounds_not_drawn},
    {"cup.round.prize.loser", S::cup_round, 0x74, 4, true, false, -1, kMax, {}, "Loser's prize (-1: a quarter)", E::rounds_not_drawn},
    {"cup.round.ties", S::cup_round, 0x1c, 2, true, true, 0, 32767, {}, "Ties in the round", E::at_once},
    {"cup.round.label", S::cup_round, 0x08, 2, true, true, -32768, 32767, {}, "Round label", E::at_once},
};
}  // namespace

std::span<const Parameter> parameters() { return registry; }
const Parameter* find_parameter(std::string_view id) {
    for (const auto& p : registry) if (p.id == id) return &p;
    return nullptr;
}
std::string toml_key(const Parameter& p) {
    std::string_view id = p.id;
    for (const std::string_view prefix : {"league.structure.", "cup.structure.", "league.", "cup.round.", "cup."})
        if (id.starts_with(prefix)) { id.remove_prefix(prefix.size()); break; }
    return std::string(id);
}
std::string_view effect_name(Effect e) {
    switch (e) {
    case Effect::at_once: return "at once";
    case Effect::results_after_edit: return "results after the edit";
    case Effect::fixtures_after_edit: return "fixtures made after the edit";
    case Effect::season_end: return "the season's end";
    case Effect::rounds_not_drawn: return "rounds not yet drawn";
    }
    return "";
}
std::string range_problem(const Parameter& p, std::int32_t value) {
    if (value < p.min || value > p.max) return std::string(p.id) + " = " + std::to_string(value) + " outside " + std::to_string(p.min) + ".." + std::to_string(p.max);
    if (!p.allowed.empty() && std::find(p.allowed.begin(), p.allowed.end(), value) == p.allowed.end())
        return std::string(p.id) + " = " + std::to_string(value) + " not an allowed value";
    return {};
}
namespace {
bool in_top_table(const Parameter& p) { return p.scope != Scope::cup_round && !p.read_only; }
bool in_structure(const Parameter& p) { return p.scope != Scope::cup_round && p.read_only; }
bool for_kind(const Parameter& p, Kind kind) { return p.scope == (kind == Kind::league ? Scope::league : Scope::cup); }

// Where a key may appear: a competition's own table, its `.structure` table (read-only values), or a round table
// (every round parameter, read-only ones included). Built once from the registry.
enum class Place { top, structure, round };
const Parameter* lookup(Kind kind, Place place, std::string_view key) {
    using Table = std::map<std::string, const Parameter*, std::less<>>;
    static const auto tables = [] {
        std::array<Table, 5> t;  // league top, league structure, cup top, cup structure, rounds
        for (const auto& p : registry) {
            const auto index = p.scope == Scope::cup_round ? 4 : (p.scope == Scope::league ? 0 : 2) + (p.read_only ? 1 : 0);
            t[std::size_t(index)].emplace(toml_key(p), &p);
        }
        return t;
    }();
    const auto index = place == Place::round ? 4 : (kind == Kind::league ? 0 : 2) + (place == Place::structure ? 1 : 0);
    const auto& table = tables[std::size_t(index)];
    const auto it = table.find(key);
    return it == table.end() ? nullptr : it->second;
}
}  // namespace
namespace {
constexpr std::uint32_t toc_base = 0x108bba90;
constexpr std::uint32_t long_name_size = 0x33, short_name_size = 0x1a;

class Guest {
public:
    explicit Guest(const State& s) : s_(s) {}
    bool readable(std::uint32_t at, std::uint32_t size = 1) const {
        if (at == 0 || size == 0) return false;
        for (const auto& [low, length] : s_.memory.mapped_ranges())
            if (at >= low && std::uint64_t(at) + size <= std::uint64_t(low) + length) return true;
        return false;
    }
    std::int32_t u8(std::uint32_t at) const { return std::int32_t(s_.memory.read<1>(at)); }
    std::int32_t s8(std::uint32_t at) const { return std::int8_t(u8(at)); }
    std::int32_t s16(std::uint32_t at) const { return std::int16_t(s_.memory.read<2>(at)); }
    std::uint32_t u32(std::uint32_t at) const { return std::uint32_t(s_.memory.read<4>(at)); }
    std::int32_t s32(std::uint32_t at) const { return std::int32_t(u32(at)); }
    std::int32_t global(std::int32_t slot) const { return s32(u32(toc_base + std::uint32_t(slot))); }
    std::string text(std::uint32_t at, std::uint32_t size) const {
        std::string out;
        for (std::uint32_t i = 0; i < size; ++i) {
            const auto c = std::uint8_t(u8(at + i));
            if (c == 0) break;
            out.push_back(char(c));
        }
        return out;
    }

private:
    const State& s_;
};

namespace C = comp::competition;

bool object_kind(const Guest& g, std::uint32_t object, Kind& kind) {
    if (!g.readable(object, 0xec)) return false;
    const auto v = g.u32(object + C::vtable);
    if (v < 0x108b0000 || v >= 0x10a00000) return false;
    switch (g.u8(object + C::kind)) {
    case league_base::league::kind: kind = Kind::league; return true;
    case cup_base::cup::kind: kind = Kind::cup; return true;
    default: return false;
    }
}

std::int32_t read_value(const Guest& g, std::uint32_t at, const Parameter& p) {
    switch (p.width) {
    case 1: return p.is_signed ? g.s8(at) : g.u8(at);
    case 2: return p.is_signed ? g.s16(at) : std::int32_t(std::uint16_t(g.s16(at)));
    default: return g.s32(at);
    }
}

Entry read_entry(const Guest& g, std::uint32_t object, Kind kind) {
    Entry e;
    e.kind = kind;
    const auto record = g.u32(object + C::record);
    if (g.readable(record, comp::record::size)) {
        e.identity.id = g.s32(record + comp::record::id);
        // record +4 and +8 point at the continent and nation records (inferred from the stored bytes: the nation's
        // number at +0x68 and three-letter code at +0xa4, the continent's code at +0xc); 0 or unreadable: none.
        const auto continent = g.u32(record + comp::record::continent), nation = g.u32(record + comp::record::nation);
        if (g.readable(continent, 0x10)) e.identity.continent = g.text(continent + 0xc, 4);
        if (g.readable(nation, 0xa8)) {
            e.identity.nation_number = g.s32(nation + 0x68);
            e.identity.nation = g.text(nation + 0xa4, 4);
        }
        e.identity.name = g.text(record + comp::record::name, long_name_size);
        e.identity.short_name = g.text(record + comp::record::short_name, short_name_size);
    }
    for (const auto& p : parameters())
        if (for_kind(p, kind)) e.values[std::string(p.id)] = read_value(g, object + p.offset, p);
    if (kind == Kind::cup) {
        const auto count = g.s16(object + C::rounds);
        const auto table = g.u32(object + cup_base::cup::rounds);
        for (std::int32_t i = 0; i < count && i < 64 && g.readable(table + std::uint32_t(i) * 0x78, 0x78); ++i) {
            Values row;
            for (const auto& p : parameters())
                if (p.scope == Scope::cup_round) row[std::string(p.id)] = read_value(g, table + std::uint32_t(i) * cup_base::cup::round_row + p.offset, p);
            e.rounds.push_back(std::move(row));
        }
    }
    return e;
}
}  // namespace

Entry read_competition(const State& state, std::uint32_t object, Kind kind) { return read_entry(Guest(state), object, kind); }

std::vector<Entry> read_competitions(const State& state, unsigned* skipped) {
    const Guest g(state);
    std::vector<Entry> out;
    std::set<std::uint32_t> seen;
    unsigned other = 0;
    const auto table = std::uint32_t(g.global(comp::toc::competitions));
    const auto count = g.global(comp::toc::competition_count);
    for (std::int32_t id = 0; g.readable(table, 4) && id < count && id < 4096; ++id) {
        const auto object = g.u32(table + std::uint32_t(id) * 4);
        if (!object) continue;
        const auto last = g.s32(object + C::last_stage);
        const auto stages = g.u32(object + C::stages);
        for (std::int32_t n = -1; n <= last && n < 64; ++n) {
            const auto at = n < 0 ? object : (g.readable(stages, 4u * (n + 1)) ? g.u32(stages + std::uint32_t(n) * 4) : 0);
            Kind kind;
            if (!at) continue;
            if (!seen.insert(at).second) continue;
            if (!object_kind(g, at, kind)) { ++other; continue; }
            auto e = read_entry(g, at, kind);
            if (n >= 0) {
                e.identity.stage = n;
                e.identity.parent = g.readable(object + C::record, 4) ? g.s32(g.u32(object + C::record)) : id;
                e.identity.id = e.identity.parent;
            }
            out.push_back(std::move(e));
        }
    }
    std::stable_sort(out.begin(), out.end(), [](const Entry& a, const Entry& b) {
        return std::tie(a.identity.id, a.identity.stage) < std::tie(b.identity.id, b.identity.stage);
    });
    if (skipped) *skipped = other;
    return out;
}

std::string key(const Identity& identity) {
    auto text = std::to_string(identity.id);
    if (identity.stage >= 0) text += "-s" + std::to_string(identity.stage);
    return text;
}

namespace {
void quote(std::string& out, const std::string& value) {
    out += '"';
    for (const unsigned char c : value) {
        if (c == '"') out += "\\\"";
        else if (c == '\\') out += "\\\\";
        else if (c < 0x20 || c >= 0x7f) {
            char buffer[8];
            std::snprintf(buffer, sizeof buffer, "\\u%04X", c);
            out += buffer;
        } else out += char(c);
    }
    out += '"';
}
void put(std::string& out, const char* name, std::int32_t value) { out += name; out += " = "; out += std::to_string(value); out += '\n'; }
void put(std::string& out, const char* name, const std::string& value) { out += name; out += " = "; quote(out, value); out += '\n'; }
}  // namespace

std::string write(const std::vector<Entry>& entries) {
    std::string out = "# Competition values: the plain values of each competition record (competition_values.hpp).\n";
    std::vector<const Entry*> sorted;
    for (const auto& e : entries) sorted.push_back(&e);
    std::stable_sort(sorted.begin(), sorted.end(), [](const Entry* a, const Entry* b) {
        return std::tie(a->identity.id, a->identity.stage, a->kind) < std::tie(b->identity.id, b->identity.stage, b->kind);
    });
    const auto value = [](const Values& values, const Parameter& p) {
        const auto it = values.find(p.id);
        return it == values.end() ? 0 : it->second;
    };
    for (const auto* entry : sorted) {
        const auto& e = *entry;
        const auto table = std::string(e.kind == Kind::league ? "league." : "cup.") + key(e.identity);
        out += "\n[" + table + "]\n";
        put(out, "id", e.identity.id);
        put(out, "stage", e.identity.stage);
        put(out, "parent", e.identity.parent);
        put(out, "name", e.identity.name);
        put(out, "short_name", e.identity.short_name);
        put(out, "continent", e.identity.continent);
        put(out, "nation", e.identity.nation);
        put(out, "nation_number", e.identity.nation_number);
        for (const auto& p : parameters())
            if (for_kind(p, e.kind) && in_top_table(p)) put(out, toml_key(p).c_str(), value(e.values, p));
        bool header = false;
        for (const auto& p : parameters()) {
            if (!for_kind(p, e.kind) || !in_structure(p)) continue;
            if (!header) { out += "\n[" + table + ".structure]\n"; header = true; }
            put(out, toml_key(p).c_str(), value(e.values, p));
        }
        for (const auto& row : e.rounds) {
            out += "\n[[" + table + ".round]]\n";
            for (const auto& p : parameters())
                if (p.scope == Scope::cup_round) put(out, toml_key(p).c_str(), value(row, p));
        }
    }
    return out;
}

namespace {
struct Parser {
    std::vector<Entry> entries;
    std::vector<unsigned> entry_lines;                // the header line of each entry, for completeness errors
    std::vector<std::vector<unsigned>> round_lines;
    Entry* entry = nullptr;
    enum class Section { none, top, structure, round } section = Section::none;
    Values* round = nullptr;
    bool structure_seen = false;
    unsigned line = 0;
    std::set<std::string> seen_keys, seen_tables;
    std::string error;

    bool fail(std::string message) { error = std::move(message); return false; }

    static bool number(std::string_view text, std::int32_t& value) {
        std::int64_t wide = 0;
        const auto end = text.data() + text.size();
        const auto [p, ec] = std::from_chars(text.data(), end, wide);
        if (text.empty() || ec != std::errc() || p != end || wide < INT32_MIN || wide > INT32_MAX) return false;
        value = std::int32_t(wide);
        return true;
    }
    static bool unquote(std::string_view text, std::string& out) {
        if (text.size() < 2 || text.front() != '"' || text.back() != '"') return false;
        out.clear();
        for (std::size_t i = 1; i + 1 < text.size(); ++i) {
            const char c = text[i];
            if (c == '"') return false;
            if (c != '\\') { out.push_back(c); continue; }
            if (++i + 1 >= text.size()) return false;
            const char e = text[i];
            if (e == '"' || e == '\\') out.push_back(e);
            else if (e == 'u') {
                if (i + 5 > text.size() - 1) return false;
                unsigned code = 0;
                const auto [p, ec] = std::from_chars(text.data() + i + 1, text.data() + i + 5, code, 16);
                if (ec != std::errc() || p != text.data() + i + 5 || code > 0xff) return false;
                out.push_back(char(code));
                i += 4;
            } else return false;
        }
        return true;
    }
    static bool key_id(std::string_view text, std::int32_t& id, std::int32_t& stage) {
        const auto dash = text.find("-s");
        stage = -1;
        if (dash == std::string_view::npos) return number(text, id) && id >= 0;
        return number(text.substr(0, dash), id) && id >= 0 && number(text.substr(dash + 2), stage) && stage >= 0;
    }

    bool header(std::string_view text, bool array) {
        const auto first = text.find('.');
        if (first == std::string_view::npos) return fail("a table needs a kind and a key");
        const auto kind_text = text.substr(0, first);
        auto rest = text.substr(first + 1);
        std::string_view tail;
        if (const auto dot = rest.find('.'); dot != std::string_view::npos) { tail = rest.substr(dot + 1); rest = rest.substr(0, dot); }
        Kind kind;
        if (kind_text == "league") kind = Kind::league;
        else if (kind_text == "cup") kind = Kind::cup;
        else return fail("unknown table kind '" + std::string(kind_text) + "'");
        std::int32_t id, stage;
        if (!key_id(rest, id, stage)) return fail("bad competition key '" + std::string(rest) + "'");
        if (tail.empty()) {
            if (array) return fail("a competition is a plain table");
            if (!seen_tables.insert(std::string(text)).second) return fail("duplicate table [" + std::string(text) + "]");
            entries.emplace_back();
            entry_lines.push_back(line);
            round_lines.emplace_back();
            entry = &entries.back();
            entry->kind = kind;
            entry->identity.id = id;
            entry->identity.stage = stage;
            section = Section::top;
            seen_keys.clear();
            round = nullptr;
            structure_seen = false;
            return true;
        }
        if (!entry || entry->kind != kind || entry->identity.id != id || entry->identity.stage != stage)
            return fail("[" + std::string(text) + "] does not follow its competition table");
        if (tail == "structure" && !array) {
            if (structure_seen) return fail("duplicate table [" + std::string(text) + "]");
            structure_seen = true;
            section = Section::structure;
            seen_keys.clear();
            return true;
        }
        if (tail == "round" && kind == Kind::cup && array) {
            entry->rounds.emplace_back();
            round_lines.back().push_back(line);
            round = &entry->rounds.back();
            section = Section::round;
            seen_keys.clear();
            return true;
        }
        return fail("unknown table [" + std::string(text) + "]");
    }

    bool assign(std::string_view name, std::string_view value) {
        if (section == Section::none) return fail("a key outside any table");
        if (!seen_keys.insert(std::string(name)).second) return fail("duplicate key '" + std::string(name) + "'");
        std::int32_t n = 0;
        std::string s;
        const auto integer = [&](std::int32_t& target) {
            if (!number(value, n)) return fail("'" + std::string(name) + "' needs an integer");
            target = n;
            return true;
        };
        auto& id = entry->identity;
        if (section == Section::top) {
            struct Int { std::string_view name; std::int32_t* target; };
            const Int ints[] = {{"id", &id.id}, {"stage", &id.stage}, {"parent", &id.parent}, {"nation_number", &id.nation_number}};
            for (const auto& i : ints) if (name == i.name) return integer(*i.target);
            const struct { std::string_view name; std::string* target; } strings[] = {
                {"name", &id.name}, {"short_name", &id.short_name}, {"continent", &id.continent}, {"nation", &id.nation}};
            for (const auto& t : strings)
                if (name == t.name) {
                    if (!unquote(value, s)) return fail("'" + std::string(name) + "' needs a quoted string");
                    *t.target = s;
                    return true;
                }
        }
        const auto place = section == Section::round ? Place::round : section == Section::top ? Place::top : Place::structure;
        if (const auto* p = lookup(entry->kind, place, name))
            return integer((section == Section::round ? *round : entry->values)[std::string(p->id)]);
        return fail("unknown key '" + std::string(name) + "'");
    }

    // Every registered parameter of every table must be given.
    bool complete() {
        for (std::size_t i = 0; i < entries.size(); ++i) {
            const auto& e = entries[i];
            for (const auto& p : parameters())
                if (for_kind(p, e.kind) && !e.values.contains(std::string(p.id))) {
                    line = entry_lines[i];
                    return fail("missing key '" + toml_key(p) + "' (" + std::string(p.id) + ")");
                }
            for (std::size_t r = 0; r < e.rounds.size(); ++r)
                for (const auto& p : parameters())
                    if (p.scope == Scope::cup_round && !e.rounds[r].contains(std::string(p.id))) {
                        line = round_lines[i][r];
                        return fail("missing key '" + toml_key(p) + "' (" + std::string(p.id) + ")");
                    }
        }
        return true;
    }
};

std::string_view trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) s.remove_suffix(1);
    return s;
}
}  // namespace

std::expected<std::vector<Entry>, ParseError> parse(std::string_view text) {
    Parser p;
    while (!text.empty()) {
        ++p.line;
        const auto newline = text.find('\n');
        auto line = trim(text.substr(0, newline));
        text = newline == std::string_view::npos ? std::string_view() : text.substr(newline + 1);
        if (line.empty() || line.front() == '#') continue;
        bool ok;
        if (line.starts_with("[[")) {
            if (!line.ends_with("]]")) return std::unexpected(ParseError{p.line, "unterminated table header"});
            ok = p.header(line.substr(2, line.size() - 4), true);
        } else if (line.front() == '[') {
            if (line.back() != ']') return std::unexpected(ParseError{p.line, "unterminated table header"});
            ok = p.header(line.substr(1, line.size() - 2), false);
        } else {
            const auto eq = line.find('=');
            if (eq == std::string_view::npos) return std::unexpected(ParseError{p.line, "expected 'key = value'"});
            ok = p.assign(trim(line.substr(0, eq)), trim(line.substr(eq + 1)));
        }
        if (!ok) return std::unexpected(ParseError{p.line, p.error});
    }
    if (!p.complete()) return std::unexpected(ParseError{p.line, p.error});
    return std::move(p.entries);
}

namespace {
void quote_utf8(std::string& out, std::string_view value);
std::string table_name(Kind kind, std::int32_t id, std::int32_t stage, const std::optional<std::int32_t>& from) {
    std::string name = kind == Kind::league ? "league." : "cup.";
    name += key(Identity{.id = id, .stage = stage});
    if (from) name += ".from." + std::to_string(*from);
    return name;
}
}  // namespace

std::string write_differences(const Differences& d) {
    std::string out = "# Custom competition database: only the values that differ from its base (competition_values.hpp).\n";
    put(out, "format", d.format);
    for (const auto& [key, value] : {std::pair{"base", &d.base}, std::pair{"name", &d.name}}) {
        out += key;
        out += " = ";
        quote_utf8(out, *value);
        out += '\n';
    }
    // One table per competition and season, in key order; a table's own values in registry order, then its rounds.
    using Group = std::tuple<std::int32_t, std::int32_t, int, std::int32_t>;  // id, stage, kind, season (none first)
    const auto group_of = [](const Difference& v) {
        return Group{v.id, v.stage, int(v.kind), v.from_season ? *v.from_season : INT32_MIN};
    };
    const auto rank = [](const Parameter* p) { return p ? p - parameters().data() : -1; };
    std::vector<const Difference*> sorted;
    for (const auto& v : d.values) if (v.parameter) sorted.push_back(&v);
    std::stable_sort(sorted.begin(), sorted.end(), [&](const Difference* a, const Difference* b) {
        return std::tuple(group_of(*a), a->round, rank(a->parameter)) < std::tuple(group_of(*b), b->round, rank(b->parameter));
    });
    for (std::size_t i = 0; i < sorted.size();) {
        const auto& first = *sorted[i];
        const auto group = group_of(first);
        const auto name = table_name(first.kind, first.id, first.stage, first.from_season);
        out += "\n[" + name + "]\n";
        std::int32_t written_rounds = 0;
        for (; i < sorted.size() && group_of(*sorted[i]) == group; ++i) {
            const auto& v = *sorted[i];
            if (v.round >= 0) {
                // The rounds before this one, empty, keep the positions; then this round's header once.
                for (; written_rounds <= v.round; ++written_rounds)
                    out += "\n# round " + std::to_string(written_rounds) + "\n[[" + name + ".round]]\n";
            }
            put(out, toml_key(*v.parameter).c_str(), v.value);
        }
    }
    return out;
}

namespace {
// TOML basic strings as UTF-8 (the differences file is the one people and their tools edit): the escapes \b \t \n
// \f \r \" \\ \uXXXX \UXXXXXXXX, raw UTF-8 otherwise; control characters other than tab must be escaped.
void append_utf8(std::string& out, std::uint32_t code) {
    if (code < 0x80) out.push_back(char(code));
    else if (code < 0x800) { out.push_back(char(0xc0 | code >> 6)); out.push_back(char(0x80 | (code & 0x3f))); }
    else if (code < 0x10000) {
        out.push_back(char(0xe0 | code >> 12)); out.push_back(char(0x80 | (code >> 6 & 0x3f))); out.push_back(char(0x80 | (code & 0x3f)));
    } else {
        out.push_back(char(0xf0 | code >> 18)); out.push_back(char(0x80 | (code >> 12 & 0x3f)));
        out.push_back(char(0x80 | (code >> 6 & 0x3f))); out.push_back(char(0x80 | (code & 0x3f)));
    }
}
// The length of the well-formed UTF-8 sequence at `text[i]` (a lead byte >= 0x80), or 0.
std::size_t utf8_sequence(std::string_view text, std::size_t i) {
    const auto lead = std::uint8_t(text[i]);
    const std::size_t length = lead >= 0xc2 && lead <= 0xdf ? 2 : lead >= 0xe0 && lead <= 0xef ? 3 : lead >= 0xf0 && lead <= 0xf4 ? 4 : 0;
    if (!length || i + length > text.size()) return 0;
    std::uint32_t code = lead & (0x7f >> length);
    for (std::size_t k = 1; k < length; ++k) {
        const auto b = std::uint8_t(text[i + k]);
        if ((b & 0xc0) != 0x80) return 0;
        code = code << 6 | (b & 0x3f);
    }
    const std::uint32_t minimum[] = {0, 0, 0x80, 0x800, 0x10000};
    if (code < minimum[length] || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff)) return 0;
    return length;
}
bool unquote_utf8(std::string_view text, std::string& out) {
    if (text.size() < 2 || text.front() != '"' || text.back() != '"') return false;
    out.clear();
    const auto body = text.substr(1, text.size() - 2);
    for (std::size_t i = 0; i < body.size();) {
        const auto c = std::uint8_t(body[i]);
        if (c == '"') return false;
        if ((c < 0x20 && c != '\t') || c == 0x7f) return false;
        if (c >= 0x80) {
            const auto length = utf8_sequence(body, i);
            if (!length) return false;
            out.append(body.substr(i, length));
            i += length;
            continue;
        }
        if (c != '\\') { out.push_back(char(c)); ++i; continue; }
        if (++i >= body.size()) return false;
        const char e = body[i++];
        switch (e) {
        case 'b': out.push_back('\b'); break;
        case 't': out.push_back('\t'); break;
        case 'n': out.push_back('\n'); break;
        case 'f': out.push_back('\f'); break;
        case 'r': out.push_back('\r'); break;
        case '"': out.push_back('"'); break;
        case '\\': out.push_back('\\'); break;
        case 'u': case 'U': {
            const std::size_t digits = e == 'u' ? 4 : 8;
            if (i + digits > body.size()) return false;
            std::uint32_t code = 0;
            const auto [p, ec] = std::from_chars(body.data() + i, body.data() + i + digits, code, 16);
            if (ec != std::errc() || p != body.data() + i + digits || code > 0x10ffff || (code >= 0xd800 && code <= 0xdfff)) return false;
            append_utf8(out, code);
            i += digits;
            break;
        }
        default: return false;
        }
    }
    return true;
}
// Writes a UTF-8 string as a TOML basic string with only the escapes TOML requires.
void quote_utf8(std::string& out, std::string_view value) {
    out += '"';
    for (const unsigned char c : value) {
        switch (c) {
        case '"': out += "\\\""; break;
        case '\\': out += "\\\\"; break;
        case '\b': out += "\\b"; break;
        case '\t': out += "\\t"; break;
        case '\n': out += "\\n"; break;
        case '\f': out += "\\f"; break;
        case '\r': out += "\\r"; break;
        default:
            if (c < 0x20 || c == 0x7f) {
                char buffer[8];
                std::snprintf(buffer, sizeof buffer, "\\u%04X", c);
                out += buffer;
            } else out += char(c);
        }
    }
    out += '"';
}

// The differences mode: every key optional, header keys first, ranges enforced, read-only values refused.
struct DifferencesParser {
    Differences result;
    unsigned line = 0;
    std::string error;
    bool in_table = false;
    Kind kind = Kind::league;
    std::int32_t id = 0, stage = -1, round = -1;
    std::optional<std::int32_t> from;
    std::set<std::string> seen_keys, seen_tables, header_keys;
    std::map<std::string, std::int32_t> round_counts;   // per table name, the round tables given so far

    bool fail(std::string message) { error = std::move(message); return false; }

    bool header(std::string_view text, bool array) {
        std::vector<std::string_view> parts;
        for (std::string_view rest = text;;) {
            const auto dot = rest.find('.');
            parts.push_back(rest.substr(0, dot));
            if (dot == std::string_view::npos) break;
            rest.remove_prefix(dot + 1);
        }
        if (parts.size() < 2) return fail("a table needs a kind and a key");
        if (parts[0] == "league") kind = Kind::league;
        else if (parts[0] == "cup") kind = Kind::cup;
        else return fail("unknown table kind '" + std::string(parts[0]) + "'");
        if (!Parser::key_id(parts[1], id, stage)) return fail("bad competition key '" + std::string(parts[1]) + "'");
        if (kind == Kind::cup && stage >= 0)
            return fail("[" + std::string(text) + "]: a cup's stages are not editable in this version, only the cup itself");
        std::size_t next = 2;
        from.reset();
        if (parts.size() > next && parts[next] == "from") {
            std::int32_t season = 0;
            if (parts.size() < next + 2 || !Parser::number(parts[next + 1], season) || season < 0)
                return fail("[" + std::string(text) + "]: 'from' needs a season (the year it starts in)");
            from = season;
            next += 2;
        }
        const auto name = table_name(kind, id, stage, from);
        seen_keys.clear();
        in_table = true;
        round = -1;
        if (parts.size() == next) {
            if (array) return fail("a competition is a plain table");
            if (!seen_tables.insert(name).second) return fail("duplicate table [" + std::string(text) + "]");
            return true;
        }
        if (parts.size() == next + 1 && parts[next] == "round" && array) {
            if (kind != Kind::cup) return fail("only a cup has round tables");
            round = round_counts[name]++;
            if (round >= 64) return fail("more than 64 round tables");
            return true;
        }
        if (parts.size() == next + 1 && parts[next] == "structure")
            return fail("[" + std::string(text) + "]: the structure is read-only and cannot be changed");
        return fail("unknown table [" + std::string(text) + "]");
    }

    bool assign(std::string_view name, std::string_view value) {
        std::int32_t n = 0;
        std::string s;
        if (!in_table) {
            if (!header_keys.insert(std::string(name)).second) return fail("duplicate key '" + std::string(name) + "'");
            if (name == "format") {
                if (!Parser::number(value, n)) return fail("'format' needs an integer");
                if (n != differences_format) return fail("unknown format " + std::to_string(n) + " (this build reads " + std::to_string(differences_format) + ")");
                result.format = n;
                return true;
            }
            if (name == "base" || name == "name") {
                if (!unquote_utf8(value, s)) return fail("'" + std::string(name) + "' needs a TOML basic string (UTF-8)");
                if (name == "base") { result.base = s; result.base_line = line; }
                else result.name = s;
                return true;
            }
            return fail("unknown key '" + std::string(name) + "' before the first table");
        }
        if (!seen_keys.insert(std::string(name)).second) return fail("duplicate key '" + std::string(name) + "'");
        if (round < 0) {
            if (name == "id" || name == "stage") {
                if (!Parser::number(value, n)) return fail("'" + std::string(name) + "' needs an integer");
                if (n != (name == "id" ? id : stage)) return fail("'" + std::string(name) + "' disagrees with the table's key");
                return true;
            }
            if (name == "parent" || name == "nation_number") {
                if (!Parser::number(value, n)) return fail("'" + std::string(name) + "' needs an integer");
                return true;
            }
            if (name == "name" || name == "short_name" || name == "continent" || name == "nation") {
                if (!unquote_utf8(value, s)) return fail("'" + std::string(name) + "' needs a TOML basic string (UTF-8)");
                return true;
            }
        }
        const auto* p = round >= 0 ? lookup(kind, Place::round, name) : lookup(kind, Place::top, name);
        if (!p && round < 0) p = lookup(kind, Place::structure, name);
        if (!p) return fail("unknown key '" + std::string(name) + "'");
        if (p->read_only) return fail("'" + std::string(name) + "' (" + std::string(p->id) + ") is read-only");
        if (!Parser::number(value, n)) return fail("'" + std::string(name) + "' needs an integer");
        if (const auto problem = range_problem(*p, n); !problem.empty()) return fail(problem);
        result.values.push_back({.kind = kind, .id = id, .stage = stage, .from_season = from, .round = round,
                                 .parameter = p, .value = n, .line = line});
        return true;
    }
};

// A '#' outside a quoted string starts a comment.
std::string_view without_comment(std::string_view line) {
    bool quoted = false;
    for (std::size_t i = 0; i < line.size(); ++i) {
        if (quoted && line[i] == '\\') { ++i; continue; }
        if (line[i] == '"') quoted = !quoted;
        else if (line[i] == '#' && !quoted) return line.substr(0, i);
    }
    return line;
}
}  // namespace

std::expected<Differences, ParseError> parse_differences(std::string_view text) {
    DifferencesParser p;
    if (text.starts_with("\xEF\xBB\xBF")) text.remove_prefix(3);
    while (!text.empty()) {
        ++p.line;
        const auto newline = text.find('\n');
        auto line = trim(without_comment(text.substr(0, newline)));
        text = newline == std::string_view::npos ? std::string_view() : text.substr(newline + 1);
        if (line.empty()) continue;
        bool ok;
        if (line.starts_with("[[")) {
            if (!line.ends_with("]]")) return std::unexpected(ParseError{p.line, "unterminated table header"});
            ok = p.header(trim(line.substr(2, line.size() - 4)), true);
        } else if (line.front() == '[') {
            if (line.back() != ']') return std::unexpected(ParseError{p.line, "unterminated table header"});
            ok = p.header(trim(line.substr(1, line.size() - 2)), false);
        } else {
            const auto eq = line.find('=');
            if (eq == std::string_view::npos) return std::unexpected(ParseError{p.line, "expected 'key = value'"});
            ok = p.assign(trim(line.substr(0, eq)), trim(line.substr(eq + 1)));
        }
        if (!ok) return std::unexpected(ParseError{p.line, p.error});
    }
    for (const auto* required : {"format", "base", "name"})
        if (!p.header_keys.contains(required)) return std::unexpected(ParseError{1, std::string("missing header key '") + required + "'"});
    return std::move(p.result);
}
}
