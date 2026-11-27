#pragma once
#include "season_competition_rules.hpp"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cm::aot {
struct State;
// Known releases of the database names a career embeds. Anything else is a
// database this port has no profile for (an edited or translated names file,
// a data update); its rules are the Mac 3.9.62 data's.
enum class DatabaseRelease { other, cm0102_3962, cm0001_389, si_3968 };
// The edition a recognised release selects; none for `other`
// (competition_edition() then uses cm0102).
std::optional<CompetitionEdition> release_edition(DatabaseRelease);
struct DatabaseSeason {
    unsigned year;
    std::string label, directory;
};
// Host-only new-game state. Guest pointers are never retained across reloads.
class DatabaseSeasons {
public:
    explicit DatabaseSeasons(std::filesystem::path runtime);
    const std::vector<DatabaseSeason>& packs() const { return packs_; }
    bool enabled() const { return packs_.size() > 1; }
    std::size_t selected() const { return selected_; }
    std::size_t default_index() const { return default_; }
    unsigned year() const { return packs_.empty() ? 2001 : packs_[selected_].year; }
    void begin(bool tables_present = false);
    void end();
    bool request(int direction);
    std::optional<std::size_t> pending() const { return pending_; }
    void activate(std::size_t index);
    std::optional<std::size_t> loaded() const { return loaded_; }
    void note_load(bool success) { if (!success) forget_loaded(); else loaded_ = active_ ? std::optional(selected_) : std::nullopt; }
    void forget_loaded() { loaded_.reset(); release_ = DatabaseRelease::other; names_directory_.reset(); }
    // A saved-career load stops pack routing while it extracts its own files.
    // Until its database load starts, the running career still owns the
    // loaded tables and their edition, and a refused load resumes routing.
    void suspend() { suspended_ |= active_; active_ = false; names_directory_.reset(); }
    void resume() { active_ |= suspended_; suspended_ = false; }
    void inspect_database_names(const std::filesystem::path& file);
    void note_names_directory(std::string directory) { names_directory_ = std::move(directory); }
    const std::optional<std::string>& names_directory() const { return names_directory_; }
    DatabaseRelease release() const { return release_; }
    bool legacy_2000() const { return release_ == DatabaseRelease::cm0001_389; }
    void complete() { pending_.reset(); }
    bool active() const { return active_; }
    void preflight(std::size_t index) const;
    std::optional<std::string> data_path(std::string_view variable, std::string_view name) const;
    std::string error;
    int previous_control = -1, next_control = -1, historical_control = -1;
    bool historical_rules = false;
private:
    std::filesystem::path root_;
    std::vector<DatabaseSeason> packs_;
    std::size_t selected_ = 0, default_ = 0;
    std::optional<std::size_t> pending_;
    std::optional<std::size_t> loaded_;
    bool active_ = false, suspended_ = false;
    DatabaseRelease release_ = DatabaseRelease::other;
    std::optional<std::string> names_directory_;
};
// Added controls use original creation/picking/drawing; only the added actions
// bypass the original handler. Calls run exclusively on the game worker.
void database_season_before_build(State&, std::uint32_t builder);
void database_season_after_build(State&, std::uint32_t builder);
bool database_season_event(State&, std::uint32_t handler, std::int16_t control);
void database_season_data_path(State&, std::uint32_t variable, std::uint32_t name, std::uint32_t output);
void database_season_nation_index_name(State&);
void database_season_competition_index_name(State&);
void database_season_index_begin(State&);
bool database_season_optional_index(State&);
void database_season_limit_leagues(State&);
void database_season_filter_league_row(State&);
}
