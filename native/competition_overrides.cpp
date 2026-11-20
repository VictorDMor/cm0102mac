#include "competition_overrides.hpp"
#include "aot_runtime.hpp"
#include "career_rules.hpp"
#include "comp_detail.hpp"
#include "cup_base_detail.hpp"
#include "league_base_detail.hpp"
#include "platform.hpp"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdio>
#include <stdexcept>
#include <tuple>

namespace cm::aot::competition_overrides {
namespace {
namespace C = comp::competition;
constexpr std::uint32_t toc_base = 0x108bba90;
constexpr std::int32_t max_rounds = 64;  // competition_values reads at most this many round rows

const Parameter& parameter(std::string_view id) {
    const auto* p = competition_values::find_parameter(id);
    if (!p) throw std::logic_error("competition_overrides: registry has no " + std::string(id));
    return *p;
}
const Parameter& league_ruling_body() { static const auto& p = parameter("league.ruling_body"); return p; }
const Parameter& cup_ruling_body() { static const auto& p = parameter("cup.ruling_body"); return p; }
const Parameter& league_bench() { static const auto& p = parameter("league.bench"); return p; }
const Parameter& cup_prize_pool() { static const auto& p = parameter("cup.prize.pool"); return p; }
bool is_ruling_body(const Parameter& p) { return &p == &league_ruling_body() || &p == &cup_ruling_body(); }

std::ptrdiff_t rank(const Parameter* p) { return p ? p - competition_values::parameters().data() : -1; }
// The order of entries, and the slot (all but the season) supersession works within.
auto slot(const Override& o) { return std::tuple(o.target, rank(o.parameter), o.round); }
auto order(const Override& o) { return std::tuple(slot(o), o.from_season ? *o.from_season : INT32_MIN); }

competition_values::Scope scope_of(Kind kind) {
    return kind == Kind::league ? competition_values::Scope::league : competition_values::Scope::cup;
}

void write_value(State& s, std::uint32_t at, const Parameter& p, std::int32_t value) {
    switch (p.width) {
    case 1: s.memory.write<1>(at, std::uint8_t(value)); break;
    case 2: s.memory.write<2>(at, std::uint16_t(value)); break;
    default: s.memory.write<4>(at, std::uint32_t(value)); break;
    }
}
std::int32_t season_of(const State& s, std::uint32_t object) { return std::int16_t(s.memory.read<2>(object + C::year)); }

// Every value of `set` in force for `target` at `season`, in entry order: (parameter, round, value).
template <class Visit> void each_in_force(const OverrideSet& set, const Target& target, std::int32_t season, Visit&& visit) {
    const auto& all = set.entries();
    auto it = std::lower_bound(all.begin(), all.end(), target, [](const Override& o, const Target& t) { return o.target < t; });
    while (it != all.end() && it->target == target) {
        const auto here = slot(*it);
        std::optional<std::int32_t> value;
        for (; it != all.end() && slot(*it) == here; ++it)
            if (!it->from_season || *it->from_season <= season) value = it->value;
        if (value) visit(*std::prev(it)->parameter, std::get<2>(here), *value);
    }
}

std::int32_t read_value(const State& s, std::uint32_t at, const Parameter& p) {
    switch (p.width) {
    case 1: { const auto b = std::uint8_t(s.memory.read<1>(at)); return p.is_signed ? std::int32_t(std::int8_t(b)) : std::int32_t(b); }
    case 2: { const auto h = std::uint16_t(s.memory.read<2>(at)); return p.is_signed ? std::int32_t(std::int16_t(h)) : std::int32_t(h); }
    default: return std::int32_t(std::uint32_t(s.memory.read<4>(at)));
    }
}

struct InForce {
    const Parameter* parameter;
    std::int32_t round, value;
};

// The places at the ends of a league table. With the record's own values for those without an entry, no one may
// exceed the club count (+0x3e, which collect_clubs set before set_up) and neither may their sum; otherwise the
// league's entries for them are not written.
void check_places(const State& s, std::uint32_t league, const Target& target, std::vector<InForce>& values) {
    static const std::array<const Parameter*, 3> places{&parameter("league.promotion.automatic"),
                                                        &parameter("league.promotion.playoff"),
                                                        &parameter("league.relegation.places")};
    std::array<std::int32_t, 3> count{};
    bool overridden = false;
    for (std::size_t i = 0; i < places.size(); ++i) {
        count[i] = read_value(s, league + places[i]->offset, *places[i]);
        for (const auto& v : values)
            if (v.parameter == places[i]) { count[i] = v.value; overridden = true; }
    }
    if (!overridden) return;
    const auto clubs = std::int32_t(std::int16_t(s.memory.read<2>(league + C::club_count)));
    const auto sum = count[0] + count[1] + count[2];
    if (sum <= clubs && std::ranges::all_of(count, [&](std::int32_t n) { return n <= clubs; })) return;
    std::fprintf(stderr, "Competition overrides for league %s: promotion and relegation places not applied: automatic %d, "
                 "playoff %d, relegation %d exceed its %d clubs\n", key(target).c_str(), count[0], count[1], count[2], clubs);
    std::erase_if(values, [](const InForce& v) { return std::ranges::find(places, v.parameter) != places.end(); });
}

// Writes what `set` holds for `object` at its season into the object and its round rows. The ruling body is
// never written (governing_body reads it from the entries).
void write_in_force(State& s, const OverrideSet& set, std::uint32_t object, const Target& target) {
    std::vector<InForce> values;
    each_in_force(set, target, season_of(s, object), [&](const Parameter& p, std::int32_t round, std::int32_t value) {
        if (!is_ruling_body(p)) values.push_back({&p, round, value});
    });
    if (target.kind == Kind::league) check_places(s, object, target, values);
    for (const auto& [p, round, value] : values) {
        if (round < 0) {
            write_value(s, object + p->offset, *p, value);
            continue;
        }
        const auto rounds = std::int32_t(std::int16_t(s.memory.read<2>(object + C::rounds)));
        const auto table = std::uint32_t(s.memory.read<4>(object + cup_base::cup::rounds));
        if (round >= rounds || table == 0) continue;  // no such round row: ignored
        write_value(s, table + std::uint32_t(round) * cup_base::cup::round_row + p->offset, *p, value);
    }
}

// A CUP_STAGE object: its constructor stores the class's vtable (TOC -0x613c) before it calls the tree directly
// (0x104bc6cc, the only direct call of 0x10479100), and its +0x44 is the constructor's competition-type argument,
// not a stage index.
constexpr std::int32_t cup_stage_vtable_slot = -0x613c;
bool is_cup_stage(const State& s, std::uint32_t object) {
    return std::uint32_t(s.memory.read<4>(object)) == std::uint32_t(s.memory.read<4>(toc_base + std::uint32_t(cup_stage_vtable_slot)));
}
}  // namespace

std::string key(const Target& t) { return competition_values::key(competition_values::Identity{.id = t.id, .stage = t.stage}); }

std::optional<Target> parse_target(Kind kind, std::string_view text) {
    const auto number = [](std::string_view digits, std::int32_t& out) {
        const auto [end, error] = std::from_chars(digits.data(), digits.data() + digits.size(), out);
        return !digits.empty() && error == std::errc{} && end == digits.data() + digits.size() && out >= 0;
    };
    Target t{.kind = kind};
    const auto dash = text.find("-s");
    if (dash == std::string_view::npos) return number(text, t.id) ? std::optional(t) : std::nullopt;
    if (!number(text.substr(0, dash), t.id) || !number(text.substr(dash + 2), t.stage)) return std::nullopt;
    return t;
}

std::string describe(const Override& o) {
    std::string text = o.target.kind == Kind::league ? "league " : "cup ";
    text += key(o.target) + " ";
    text += o.parameter ? std::string(o.parameter->id) : std::string("<no parameter>");
    if (o.round >= 0) text += " round " + std::to_string(o.round);
    text += " = " + std::to_string(o.value);
    if (o.from_season) text += " from " + std::to_string(*o.from_season);
    return text;
}

std::string problem(const Override& o) {
    if (!o.parameter) return "unknown parameter";
    const auto& p = *o.parameter;
    const auto id = std::string(p.id);
    if (o.target.id < 0 || o.target.stage < -1) return "bad competition key";
    if (p.read_only) return id + " is read-only";
    const bool round_parameter = p.scope == competition_values::Scope::cup_round;
    if (round_parameter ? o.target.kind != Kind::cup : p.scope != scope_of(o.target.kind))
        return id + " is not a value of a " + (o.target.kind == Kind::league ? "league" : "cup");
    if (round_parameter && (o.round < 0 || o.round >= max_rounds)) return id + " needs a round index 0.." + std::to_string(max_rounds - 1);
    if (!round_parameter && o.round != -1) return id + " is not a round value";
    if (o.target.kind == Kind::cup && o.target.stage >= 0) return "a cup's stages are not editable in this version, only the cup itself";
    if (o.from_season && (*o.from_season < 1 || *o.from_season > 32767)) return "first season outside 1..32767";
    if (const auto range = competition_values::range_problem(p, o.value); !range.empty()) return range;
    if (id.starts_with("league.tiebreak.") && o.value == 4)
        return "tie-break 4 (head-to-head) needs blocks the club placement allocates before the override applies";
    if (is_ruling_body(p) && o.target.stage >= 0) return "a stage's ruling body is never read: discipline asks the competition";
    return {};
}

std::string OverrideSet::add(const Override& o) {
    if (auto why = problem(o); !why.empty()) return why;
    const auto at = std::lower_bound(entries_.begin(), entries_.end(), o,
                                     [](const Override& a, const Override& b) { return order(a) < order(b); });
    if (at != entries_.end() && order(*at) == order(o)) at->value = o.value;
    else entries_.insert(at, o);
    return {};
}

std::expected<OverrideSet, OverrideSet::BuildError> OverrideSet::from(std::vector<Override> entries, Repeats repeats) {
    for (std::size_t i = 0; i < entries.size(); ++i)
        if (auto why = problem(entries[i]); !why.empty()) return std::unexpected(BuildError{i, std::move(why)});
    std::vector<std::size_t> index(entries.size());
    for (std::size_t i = 0; i < index.size(); ++i) index[i] = i;
    std::stable_sort(index.begin(), index.end(), [&](std::size_t a, std::size_t b) { return order(entries[a]) < order(entries[b]); });
    OverrideSet set;
    set.entries_.reserve(entries.size());
    for (std::size_t k = 0; k < index.size(); ++k) {
        const auto& entry = entries[index[k]];
        if (!set.entries_.empty() && order(set.entries_.back()) == order(entry)) {
            if (repeats == Repeats::refuse) return std::unexpected(BuildError{index[k], "repeats an earlier entry: " + describe(entry)});
            set.entries_.back() = entry;  // the stable sort keeps the given order: the later one wins
            continue;
        }
        set.entries_.push_back(entry);
    }
    return set;
}

std::optional<std::int32_t> OverrideSet::value(const Target& target, const Parameter& p, std::int32_t round,
                                               std::int32_t season) const {
    const auto wanted = std::tuple(target, rank(&p), round);
    auto it = std::lower_bound(entries_.begin(), entries_.end(), wanted,
                               [](const Override& o, const auto& w) { return slot(o) < w; });
    std::optional<std::int32_t> result;
    for (; it != entries_.end() && slot(*it) == wanted; ++it)
        if (!it->from_season || *it->from_season <= season) result = it->value;
    return result;
}

std::expected<CustomDatabase, ParseError> read_custom_database(std::string_view text) {
    auto parsed = competition_values::parse_differences(text);
    if (!parsed) return std::unexpected(parsed.error());
    CustomDatabase database;
    database.name = parsed->name;
    bool known = false;
    for (std::size_t e = 0; e < competition_edition_count; ++e)
        if (edition_name(CompetitionEdition(e)) == parsed->base) { database.base = CompetitionEdition(e); known = true; }
    if (!known) return std::unexpected(ParseError{parsed->base_line, "unknown base edition '" + parsed->base + "'"});
    std::vector<Override> entries;
    entries.reserve(parsed->values.size());
    for (const auto& d : parsed->values)
        entries.push_back({.target = {.kind = d.kind, .id = d.id, .stage = d.stage}, .parameter = d.parameter,
                           .round = d.round, .value = d.value, .from_season = d.from_season});
    auto set = OverrideSet::from(entries, OverrideSet::Repeats::refuse);
    if (!set) return std::unexpected(ParseError{parsed->values[set.error().index].line, describe(entries[set.error().index]) + ": " + set.error().message});
    database.overrides = std::move(*set);
    return database;
}

std::string write_custom_database(const CustomDatabase& database) {
    competition_values::Differences d{.base = std::string(edition_name(database.base)), .name = database.name};
    for (const auto& o : database.overrides.entries())
        d.values.push_back({.kind = o.target.kind, .id = o.target.id, .stage = o.target.stage, .from_season = o.from_season,
                            .round = o.round, .parameter = o.parameter, .value = o.value});
    return competition_values::write_differences(d);
}

const OverrideSet* active(const State& s) {
    if (!s.platform) return nullptr;
    const auto& set = s.platform->career_rules().competition_overrides();
    return set.empty() ? nullptr : &set;
}

std::optional<Target> target_of(const State& s, std::uint32_t object) {
    Target t;
    switch (s.memory.read<1>(object + C::kind)) {
    case league_base::league::kind: t.kind = Kind::league; break;
    case cup_base::cup::kind:
        if (is_cup_stage(s, object)) return std::nullopt;
        t.kind = Kind::cup;
        break;
    default: return std::nullopt;
    }
    const auto record = std::uint32_t(s.memory.read<4>(object + C::record));
    if (record == 0) return std::nullopt;
    t.id = std::int32_t(std::uint32_t(s.memory.read<4>(record + comp::record::id)));
    t.stage = std::int8_t(s.memory.read<1>(object + C::owner));
    if (t.id < 0 || t.stage < -1) return std::nullopt;
    return t;
}

void at_setup(State& s, std::uint32_t object, Kind kind) {
    const auto* set = active(s);
    if (!set) return;
    const auto target = target_of(s, object);
    if (!target || target->kind != kind) return;
    write_in_force(s, *set, object, *target);
}

std::int8_t ruling_body(const State& s, std::uint32_t competition, std::int8_t stored) {
    const auto* set = active(s);
    if (!set) return stored;
    const auto target = target_of(s, competition);
    if (!target) return stored;
    const auto& p = target->kind == Kind::league ? league_ruling_body() : cup_ruling_body();
    const auto value = set->value(*target, p, -1, season_of(s, competition));
    return value ? std::int8_t(*value) : stored;
}

bool bench_overridden(const State& s, std::int32_t competition_id, std::int32_t season) {
    const auto* set = active(s);
    return set && set->value(Target{.kind = Kind::league, .id = competition_id}, league_bench(), -1, season).has_value();
}

ApplyReport apply_now(State& s, const Target& target) {
    ApplyReport report;
    const Memory::ExactBounds exact(s.memory);
    std::uint32_t object = 0;
    try {
        const auto cell = [&](std::int32_t slot) { return std::uint32_t(s.memory.read<4>(std::uint32_t(s.memory.read<4>(toc_base + std::uint32_t(slot))))); };
        const auto table = cell(comp::toc::competitions);
        const auto count = std::int32_t(cell(comp::toc::competition_count));
        if (table == 0 || target.id >= count) return report;
        object = std::uint32_t(s.memory.read<4>(table + std::uint32_t(target.id) * 4));
        if (object != 0 && target.stage >= 0) {
            const auto last = std::int32_t(std::uint32_t(s.memory.read<4>(object + C::last_stage)));
            const auto stages = std::uint32_t(s.memory.read<4>(object + C::stages));
            object = target.stage <= last && stages ? std::uint32_t(s.memory.read<4>(stages + std::uint32_t(target.stage) * 4)) : 0;
        }
        if (object == 0) return report;
        const auto found = target_of(s, object);
        if (!found || *found != target) return report;
    } catch (const std::runtime_error&) { return report; }
    report.found = true;
    const auto* set = active(s);
    if (!set) return report;
    const auto season = season_of(s, object);
    const auto deferred = [&](const Parameter& p, std::int32_t round) {
        return round >= 0 || &p == &cup_prize_pool();
    };
    std::vector<InForce> values;
    each_in_force(*set, target, season, [&](const Parameter& p, std::int32_t round, std::int32_t value) {
        values.push_back({&p, round, value});
    });
    // The same limit as at set_up: places beyond the club count are not written (and not reported).
    if (target.kind == Kind::league) check_places(s, object, target, values);
    for (const auto& [p, round, value] : values) {
        auto& list = is_ruling_body(*p) ? report.live : deferred(*p, round) ? report.deferred : report.written;
        if (std::ranges::find(list, p) == list.end()) list.push_back(p);
        if (&list == &report.written) write_value(s, object + p->offset, *p, value);
    }
    return report;
}
}
