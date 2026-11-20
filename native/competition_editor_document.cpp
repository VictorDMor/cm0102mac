#include "competition_editor_document.hpp"
#include "career_rules.hpp"
#include <algorithm>
#include <array>
#include <fstream>
#include <tuple>

namespace cm::aot {
namespace ov = competition_overrides;
using competition_values::Kind;
using competition_values::Scope;

std::optional<std::filesystem::path> selected_custom_database(const std::filesystem::path& selection_file) {
    std::ifstream input(selection_file, std::ios::binary);
    std::string line;
    if (!input || !std::getline(input, line) || line.empty()) return std::nullopt;
    return std::filesystem::path(line);
}

CompetitionEditorDocument::CompetitionEditorDocument(CompetitionEdition base, std::vector<Entry> defaults) : base_(base) {
    for (auto& e : defaults)
        if (!(e.kind == Kind::cup && e.identity.stage >= 0)) competitions_.push_back(std::move(e));
    std::stable_sort(competitions_.begin(), competitions_.end(), [](const Entry& a, const Entry& b) {
        return std::tie(a.identity.nation, a.kind, a.identity.id, a.identity.stage) <
               std::tie(b.identity.nation, b.kind, b.identity.id, b.identity.stage);
    });
}

std::vector<const competition_values::Parameter*> CompetitionEditorDocument::parameters(Kind kind, bool rounds) {
    std::vector<const Parameter*> out;
    const auto scope = rounds ? Scope::cup_round : kind == Kind::league ? Scope::league : Scope::cup;
    for (const bool read_only : {false, true})
        for (const auto& p : competition_values::parameters())
            if (p.scope == scope && p.read_only == read_only) out.push_back(&p);
    return out;
}

ov::Target CompetitionEditorDocument::target(std::size_t competition) const {
    const auto& e = competitions_.at(competition);
    return {.kind = e.kind, .id = e.identity.id, .stage = e.identity.stage};
}

const ov::Override* CompetitionEditorDocument::find(std::size_t competition, const Parameter& p, std::int32_t round) const {
    const auto t = target(competition);
    for (const auto& o : entries_)
        if (!o.from_season && o.target == t && o.parameter == &p && o.round == round) return &o;
    return nullptr;
}

std::int32_t CompetitionEditorDocument::original(std::size_t competition, const Parameter& p, std::int32_t round) const {
    const auto& e = competitions_.at(competition);
    const auto* values = round < 0 ? &e.values : std::size_t(round) < e.rounds.size() ? &e.rounds[std::size_t(round)] : nullptr;
    if (!values) return 0;
    const auto it = values->find(p.id);
    return it == values->end() ? 0 : it->second;
}

std::int32_t CompetitionEditorDocument::value(std::size_t competition, const Parameter& p, std::int32_t round) const {
    const auto* o = find(competition, p, round);
    return o ? o->value : original(competition, p, round);
}

bool CompetitionEditorDocument::edited(std::size_t competition, const Parameter& p, std::int32_t round) const {
    return find(competition, p, round) != nullptr;
}

CompetitionEditorDocument::Refusal CompetitionEditorDocument::set(std::size_t competition, const Parameter& p, std::int32_t round, std::int32_t value) {
    using Why = Refusal::Kind;
    const auto t = target(competition);
    const auto same = [&](const ov::Override& o) { return !o.from_season && o.target == t && o.parameter == &p && o.round == round; };
    if (value == original(competition, p, round)) {
        dirty_ |= std::erase_if(entries_, same) != 0;
        return {};
    }
    const ov::Override entry{.target = t, .parameter = &p, .round = round, .value = value};
    if (!competition_values::range_problem(p, value).empty()) return {Why::range, p.min, p.max, {}};
    if (p.id.starts_with("league.tiebreak.") && value == 4) return {Why::head_to_head, 0, 0, {}};
    if (auto why = ov::problem(entry); !why.empty()) return {Why::other, 0, 0, std::move(why)};
    if (round >= 0 && std::size_t(round) >= competitions_.at(competition).rounds.size()) return {Why::no_round, 0, 0, {}};
    if (t.kind == Kind::league) {
        // The game does not write places beyond the club count (competition_overrides.hpp), so neither is saved.
        static const std::array<std::string_view, 3> places{"league.promotion.automatic", "league.promotion.playoff", "league.relegation.places"};
        if (std::ranges::find(places, p.id) != places.end()) {
            const auto clubs = original(competition, *competition_values::find_parameter("league.structure.clubs"), -1);
            std::int32_t sum = 0;
            for (const auto id : places) sum += id == p.id ? value : this->value(competition, *competition_values::find_parameter(id), -1);
            if (value > clubs || sum > clubs) return {Why::places, sum, clubs, {}};
        }
    }
    if (const auto at = std::ranges::find_if(entries_, same); at != entries_.end()) at->value = value;
    else entries_.push_back(entry);
    dirty_ = true;
    return {};
}

void CompetitionEditorDocument::revert(std::size_t competition) {
    const auto t = target(competition);
    dirty_ |= std::erase_if(entries_, [&](const ov::Override& o) { return o.target == t && !o.from_season; }) != 0;
}

bool CompetitionEditorDocument::modified(std::size_t competition) const {
    const auto t = target(competition);
    return std::ranges::any_of(entries_, [&](const ov::Override& o) { return o.target == t; });
}

std::size_t CompetitionEditorDocument::scheduled(std::size_t competition) const {
    const auto t = target(competition);
    return std::size_t(std::ranges::count_if(entries_, [&](const ov::Override& o) { return o.target == t && o.from_season; }));
}

std::vector<CompetitionEditorDocument::Warning> CompetitionEditorDocument::warnings(std::size_t competition) const {
    std::vector<Warning> out;
    const auto t = target(competition);
    const auto touches = [&](std::string_view prefix) {
        return std::ranges::any_of(entries_, [&](const ov::Override& o) { return o.target == t && o.parameter->id.starts_with(prefix); });
    };
    if (touches("league.promotion.") || touches("league.relegation."))
        out.push_back({Warning::Kind::places_unchecked});
    if (touches("league.bench") || touches("cup.bench") || touches("league.substitutions") || touches("cup.substitutions")) {
        const auto kind = t.kind == Kind::league ? "league." : "cup.";
        const auto bench = value(competition, *competition_values::find_parameter(std::string(kind) + "bench"), -1);
        const auto subs = value(competition, *competition_values::find_parameter(std::string(kind) + "substitutions"), -1);
        if (subs > bench) out.push_back({Warning::Kind::more_substitutions_than_bench});
    }
    if (const auto n = scheduled(competition)) out.push_back({Warning::Kind::scheduled, n});
    return out;
}

std::string CompetitionEditorDocument::load(const ov::CustomDatabase& database) {
    if (database.base != base_)
        return "made from " + std::string(edition_name(database.base)) + ", not " + std::string(edition_name(base_));
    entries_ = database.overrides.entries();
    name = database.name;
    dirty_ = false;
    return {};
}

ov::CustomDatabase CompetitionEditorDocument::database() const {
    ov::CustomDatabase out{.base = base_, .name = name};
    // Every entry passed problem() when it was set or read; a repeat cannot arise (set replaces).
    if (auto set = ov::OverrideSet::from(entries_, ov::OverrideSet::Repeats::keep_last)) out.overrides = std::move(*set);
    return out;
}
}
