#pragma once
#include "season_competition_rules.hpp"
#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace cm::aot {
// A career's own rules: the third layer, after the global set and the
// edition. They travel in the save as the member `native_rules.tmp`, which the
// original writer bundles and the original loader restores like any other
// temporary file (docs/season-competition-rules-plan.md, Container audit).
// Host-only state, owned by the game worker.
//
//   CM-NATIVE-RULES 1
//   edition cm0001_389
//   revision 1
//   historical_rules off
//   set england.premier_named_reserves 0
class CareerRules {
public:
    inline static constexpr std::string_view member_name = "native_rules.tmp";
    // The format is a few lines of text; a larger member is dropped.
    inline static constexpr std::size_t member_limit = 1024 * 1024;

    // A new career, or a load that replaces the running one.
    void reset() { *this = {}; }
    // Takes a member's text. Returns what was not applied, for the status
    // line and the log; empty when everything was. Never fails: text this
    // build cannot interpret is kept and written back unchanged. An empty
    // member is no member.
    std::vector<std::string> read(std::string_view text);
    // The member to save. `current` is the career's edition where no member pinned one.
    std::string text(CompetitionEdition current) const;

    // The edition a member pinned; without one the database names decide.
    std::optional<CompetitionEdition> edition() const { return edition_; }
    // Whether the career follows its competitions' historical timelines
    // ("Regras realistas"). Off for careers saved without the switch.
    bool historical_rules() const { return historical_rules_; }
    // The pinned edition's rules with the career's overrides, under each test
    // preset, or null when it has no override.
    const RulePresets* customized() const { return customized_ ? &*customized_ : nullptr; }

private:
    std::optional<CompetitionEdition> edition_;
    bool historical_rules_ = false;
    // Kept verbatim when the header, edition or revision is not this build's.
    std::optional<std::string> opaque_;
    // Every statement after the header ones, as read, applied or not.
    std::vector<std::string> statements_;
    std::optional<RulePresets> customized_;
};

// The revision of an edition's checked-in rules. A member with another
// revision is kept and not applied, so a build that raises this must keep the
// previous revision's rules and resolve members that name it.
constexpr unsigned rules_revision(CompetitionEdition) { return 1; }
std::string_view edition_name(CompetitionEdition);
}
