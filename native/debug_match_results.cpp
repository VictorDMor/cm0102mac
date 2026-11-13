#include "debug_match_results.hpp"
#include "aot_runtime.hpp"
#include "match_debugging.hpp"
#include "match_eng_fields.hpp"
#include <algorithm>

namespace cm::aot {
namespace {
MatchResultPlan identity(const State& s, std::uint32_t fixture, ForcedMatchResult result) {
    const auto word = [&](unsigned offset) { return std::uint32_t(s.memory.read<4>(fixture + offset)); };
    return {word(0x1c), word(0x20), word(0x28), word(0), word(0x30), result};
}
bool matches(const MatchResultPlan& a, const MatchResultPlan& b) {
    return a.home == b.home && a.away == b.away && a.date == b.date &&
        a.competition == b.competition && a.round == b.round;
}
bool human_fixture(const State& s, std::uint32_t fixture) {
    const auto wanted = identity(s, fixture, ForcedMatchResult::normal);
    // Human-manager records include spectators without a managed team. The
    // record points at a queue entry: original core pointer, then copied core.
    for (unsigned i = 0; i < 16; ++i) {
        const auto queued = std::uint32_t(s.memory.read<4>(0x109806f0 + i * 0xfc + 0xec));
        if (!queued) continue;
        if (s.memory.read<4>(queued) == fixture || matches(wanted, identity(s, queued + 4, ForcedMatchResult::normal)))
            return true;
    }
    return false;
}
}

bool valid_debug_result_club(const State& s, std::uint32_t club) {
    if (!club) return false;
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto base = s.memory.read<4>(s.memory.read<4>(toc - 0x7be8));
    const auto count = s.memory.read<4>(s.memory.read<4>(toc - 0x7b50));
    return base && club >= base && (club - base) % 0x248 == 0 &&
        (club - base) / 0x248 < count && s.memory.read<4>(club) == (club - base) / 0x248;
}

bool add_debug_result_club(State& s, std::uint32_t club) {
    if (!native_debugging_available() || !valid_debug_result_club(s, club)) return false;
    auto& rules = match_result_debugging(s).clubs;
    if (std::ranges::find(rules, club, &ClubResultRule::club) == rules.end())
        rules.push_back({club, ForcedClubResult::win});
    return true;
}

bool set_debug_club_result(State& s, std::uint32_t club, ForcedClubResult result) {
    if (!native_debugging_available() ||
        (result != ForcedClubResult::win && result != ForcedClubResult::draw && result != ForcedClubResult::loss)) return false;
    auto& rules = match_result_debugging(s).clubs;
    const auto found = std::ranges::find(rules, club, &ClubResultRule::club);
    if (found == rules.end()) return false;
    found->result = result;
    return true;
}

bool remove_debug_result_club(State& s, std::uint32_t club) {
    if (!native_debugging_available()) return false;
    return std::erase_if(match_result_debugging(s).clubs, [&](const auto& rule) { return rule.club == club; }) != 0;
}

ForcedMatchResult effective_debug_match_result(const State& s, std::uint32_t fixture) {
    if (!fixture) return ForcedMatchResult::normal;
    if (native_debugging_feature(DebugFeature::match_result)) {
        const auto manual = debug_match_result(s, fixture);
        if (manual != ForcedMatchResult::normal) return manual;
    }
    if (!native_debugging_available()) return ForcedMatchResult::normal;
    const auto& rules = match_result_debugging(s).clubs;
    if (rules.empty()) return ForcedMatchResult::normal;
    const auto home = s.memory.read<4>(fixture + 0x1c), away = s.memory.read<4>(fixture + 0x20);
    const auto h = std::ranges::find(rules, home, &ClubResultRule::club);
    const auto a = std::ranges::find(rules, away, &ClubResultRule::club);
    if ((h == rules.end()) == (a == rules.end())) return ForcedMatchResult::normal;
    const bool home_rule = h != rules.end();
    const auto result = (home_rule ? h : a)->result;
    if (result == ForcedClubResult::draw) return ForcedMatchResult::draw;
    return home_rule == (result == ForcedClubResult::win) ? ForcedMatchResult::home_win : ForcedMatchResult::away_win;
}

bool debug_match_result_eligible(const State& s, std::uint32_t fixture) {
    if (!fixture || std::int8_t(s.memory.read<1>(fixture + 0x43)) != -1) return false;
    const auto plan = identity(s, fixture, ForcedMatchResult::normal);
    return plan.home && plan.away && plan.home != plan.away && !human_fixture(s, fixture);
}

ForcedMatchResult debug_match_result(const State& s, std::uint32_t fixture) {
    const auto& plans = match_result_debugging(s).fixtures;
    if (!fixture || plans.empty()) return ForcedMatchResult::normal;
    const auto wanted = identity(s, fixture, ForcedMatchResult::normal);
    const auto found = std::ranges::find_if(plans, [&](const auto& plan) { return matches(plan, wanted); });
    return found == plans.end() ? ForcedMatchResult::normal : found->result;
}

bool set_debug_match_result(State& s, std::uint32_t fixture, ForcedMatchResult result) {
    if (!native_debugging_feature(DebugFeature::match_result) || !debug_match_result_eligible(s, fixture)) return false;
    if (result != ForcedMatchResult::normal && result != ForcedMatchResult::home_win &&
        result != ForcedMatchResult::draw && result != ForcedMatchResult::away_win) return false;
    const auto selected = identity(s, fixture, result);
    auto& plans = match_result_debugging(s).fixtures;
    std::erase_if(plans, [&](const auto& plan) { return matches(plan, selected); });
    if (result != ForcedMatchResult::normal) plans.push_back(selected);
    return true;
}

std::optional<std::array<unsigned, 2>> debug_match_score(const State& s, std::uint32_t match) {
    const auto& choices = match_result_debugging(s);
    if (choices.fixtures.empty() && choices.clubs.empty()) return {};
    // Scores are scheduled by the quick simulation. Background preparation
    // also routes selected, unobserved detailed matches through that engine.
    if (!s.memory.read<1>(match)) return {};
    const auto fixture = std::uint32_t(s.memory.read<4>(match + 0x3118));
    if (!debug_match_result_eligible(s, fixture)) return {};
    const auto result = effective_debug_match_result(s, fixture);
    if (result == ForcedMatchResult::normal) return {};
    const auto plan = identity(s, fixture, result);
    // Stable variation across fixtures, without consuming the game's RNG just
    // to configure this optional feature. Football events still use its RNG.
    const auto home_id = std::uint32_t(s.memory.read<4>(plan.home));
    const auto away_id = std::uint32_t(s.memory.read<4>(plan.away));
    auto variation = (home_id * 2654435761u) ^ (away_id * 2246822519u) ^ plan.date ^ plan.round;
    // Fold both halves of the packed date before indexing the score table.
    variation ^= variation >> 16;
    variation *= 0x7feb352du;
    variation ^= variation >> 15;
    if (result == ForcedMatchResult::draw) {
        constexpr unsigned goals[]{0, 1, 1, 1, 2};
        const auto value = goals[variation % std::size(goals)];
        return std::array{value, value};
    }
    // Prefer narrow wins while retaining occasional ordinary larger scores.
    constexpr std::array<unsigned, 2> scores[]{
        {1, 0}, {1, 0}, {1, 0}, {1, 0}, {2, 0}, {2, 0}, {2, 0}, {2, 1},
        {2, 1}, {2, 1}, {2, 1}, {3, 0}, {3, 1}, {3, 2}, {4, 0}, {4, 1}};
    auto score = scores[variation % std::size(scores)];
    if (result == ForcedMatchResult::away_win) std::swap(score[0], score[1]);
    return score;
}

void prepare_debug_match_result(State& s, std::uint32_t match) {
    const auto& choices = match_result_debugging(static_cast<const State&>(s));
    if (choices.fixtures.empty() && choices.clubs.empty()) return;
    // Both background scheduler branches keep their queue entry in r29. The
    // immediate background path keeps it in r18 and bypasses that scheduler.
    // The watched caller and standalone debugger are deliberately excluded.
    unsigned queue_register;
    switch (std::uint32_t(s.r<4>(4128))) {
        case 0x101cc284: queue_register = 18; break;
        case 0x101cd6fc:
        case 0x101cd748: queue_register = 29; break;
        default: return;
    }
    const auto queue_entry = std::uint32_t(s.r<4>(queue_register * 4));
    if (!queue_entry || s.memory.read<4>(queue_entry + 0x58) != match) return;
    if (!match || s.memory.read<1>(match)) return;
    // Detail level is not attendance: the original can simulate background
    // games in detail for a league, nation or reputation setting. Switch only
    // at their first background advance, before any football has been played
    // (tested first: every later advance of every such match stops here).
    if (s.memory.read<2>(match + match_eng::clock_at) || s.memory.read<2>(match + 0x794a)) return;
    const auto fixture = std::uint32_t(s.memory.read<4>(match + match_eng::fixture_at));
    if (!debug_match_result_eligible(s, fixture) ||
        effective_debug_match_result(s, fixture) == ForcedMatchResult::normal) return;
    const auto display = std::uint32_t(s.memory.read<4>(queue_entry + 0x5c));
    if (display && s.memory.read<1>(display + 0x4b)) return;
    s.memory.write<1>(match, 1);
}

void finish_debug_match_result(State& s, std::uint32_t fixture) {
    if (!fixture || match_result_debugging(static_cast<const State&>(s)).fixtures.empty()) return;
    const auto finished = identity(s, fixture, ForcedMatchResult::normal);
    std::erase_if(match_result_debugging(s).fixtures, [&](const auto& plan) { return matches(plan, finished); });
}

}
