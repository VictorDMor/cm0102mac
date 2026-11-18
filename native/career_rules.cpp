#include "career_rules.hpp"
#include <algorithm>
#include <charconv>
#include <ranges>

namespace cm::aot {
namespace {
constexpr std::string_view header = "CM-NATIVE-RULES 1";
constexpr std::array<std::string_view, competition_edition_count> edition_names{"cm0001_389", "cm0102", "si_3968"};

// The rules a career may override, each with the values the engine paths
// behind it are known to handle. Anything else is kept in the member and ignored.
struct Field {
    std::string_view name;
    bool (*set)(CompetitionRules&, long);
};
struct Override {
    const Field* field;
    long value;
};
constexpr bool one_or_three(long value) { return value == 1 || value == 3; }
constexpr std::array fields{
    // Places each way between the Third Division and the Conference; the two must agree.
    Field{"england.promotion.conference_promoted", [](CompetitionRules& rules, long value) {
        if (!one_or_three(value)) return false;
        rules.england.promotion.conference_promoted = unsigned(value);
        return true;
    }},
    Field{"england.promotion.third_relegated", [](CompetitionRules& rules, long value) {
        if (!one_or_three(value)) return false;
        rules.england.promotion.third_relegated = unsigned(value);
        return true;
    }},
    // Seven named reserves, or zero for the engine's own five.
    Field{"england.premier_named_reserves", [](CompetitionRules& rules, long value) {
        if (value != 0 && value != 7) return false;
        rules.england.premier_named_reserves = unsigned(value);
        return true;
    }},
    Field{"brazil.serie_b_single_table", [](CompetitionRules& rules, long value) {
        if (value != 0 && value != 1) return false;
        rules.brazil.serie_b_single_table = value != 0;
        return true;
    }},
};

std::optional<long> number(std::string_view text) {
    long value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size()) return std::nullopt;
    return value;
}
// "word rest": the rest after one space, when the line starts with the word.
std::optional<std::string_view> after(std::string_view line, std::string_view word) {
    if (!line.starts_with(word) || line.size() <= word.size() || line[word.size()] != ' ') return std::nullopt;
    return line.substr(word.size() + 1);
}
std::vector<std::string_view> lines(std::string_view text) {
    std::vector<std::string_view> result;
    while (!text.empty()) {
        const auto end = text.find('\n');
        auto line = text.substr(0, end);
        text.remove_prefix(end == std::string_view::npos ? text.size() : end + 1);
        while (!line.empty() && (line.back() == '\r' || line.back() == ' ' || line.back() == '\t')) line.remove_suffix(1);
        if (!line.empty()) result.push_back(line);
    }
    return result;
}
}

std::string_view edition_name(CompetitionEdition edition) { return edition_names[std::size_t(edition)]; }

std::vector<std::string> CareerRules::read(std::string_view text) {
    reset();
    const auto keep_unread = [&](std::string why) {
        reset();
        opaque_ = std::string(text);
        return std::vector{std::move(why)};
    };
    const auto all = lines(text);
    if (all.empty()) return {};
    if (all.front() != header) return keep_unread("unknown format");
    std::optional<CompetitionEdition> edition;
    std::optional<long> revision;
    std::vector<std::string> ignored;
    std::vector<Override> overrides;
    for (auto line : all | std::views::drop(1)) {
        if (const auto name = after(line, "edition")) {
            const auto known = std::ranges::find(edition_names, *name);
            if (known == edition_names.end() || edition) return keep_unread("unknown or repeated edition");
            edition = CompetitionEdition(known - edition_names.begin());
        } else if (const auto value = after(line, "revision")) {
            if (revision || !(revision = number(*value))) return keep_unread("unreadable revision");
        } else if (const auto state = after(line, "historical_rules"); state && (*state == "on" || *state == "off")) {
            historical_rules_ = *state == "on";
        } else {
            statements_.emplace_back(line);
            bool applied = false;
            if (const auto assignment = after(line, "set")) {
                const auto space = assignment->find(' ');
                const auto field = std::ranges::find(fields, assignment->substr(0, space), &Field::name);
                const auto value = space == std::string_view::npos ? std::nullopt : number(assignment->substr(space + 1));
                CompetitionRules probe;
                if (field != fields.end() && value && field->set(probe, *value)) {
                    std::erase_if(overrides, [&](const Override& other) { return other.field == &*field; });
                    overrides.push_back({&*field, *value});
                    applied = true;
                }
            }
            if (!applied) ignored.emplace_back(line);
        }
    }
    if (!edition || !revision) return keep_unread("no edition or revision");
    if (*revision != long(rules_revision(*edition))) return keep_unread("rules revision " + std::to_string(*revision) + " is not this build's");
    edition_ = edition;
    const auto resolve = [&] {
        auto rules = edition_rules(*edition);
        for (const auto& item : overrides) item.field->set(rules, item.value);
        return rules;
    };
    // The exchange between the Third Division and the Conference moves the
    // same number of clubs each way; a member that says otherwise changes neither.
    if (const auto& promotion = resolve().england.promotion; promotion.conference_promoted != promotion.third_relegated) {
        std::erase_if(overrides, [](const Override& item) { return item.field->name.starts_with("england.promotion."); });
        ignored.emplace_back("england.promotion: conference_promoted and third_relegated differ");
    }
    if (!overrides.empty()) customized_ = original_presets(resolve());
    return ignored;
}

std::string CareerRules::text(CompetitionEdition current) const {
    if (opaque_) return *opaque_;
    const auto edition = edition_.value_or(current);
    std::string result{header};
    result += "\nedition ";
    result += edition_name(edition);
    result += "\nrevision " + std::to_string(rules_revision(edition));
    result += historical_rules_ ? "\nhistorical_rules on\n" : "\nhistorical_rules off\n";
    for (const auto& statement : statements_) result += statement + "\n";
    return result;
}
}
