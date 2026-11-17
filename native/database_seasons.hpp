#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <utility>
#include <vector>

namespace cm::aot {
struct State;
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
    void forget_loaded() { loaded_.reset(); legacy_2000_ = false; names_directory_.reset(); }
    void inspect_database_names(const std::filesystem::path& file);
    void note_names_directory(std::string directory) { names_directory_ = std::move(directory); }
    const std::optional<std::string>& names_directory() const { return names_directory_; }
    bool legacy_2000() const { return legacy_2000_; }
    void complete() { pending_.reset(); }
    bool active() const { return active_; }
    void preflight(std::size_t index) const;
    std::optional<std::string> data_path(std::string_view variable, std::string_view name) const;
    std::string error;
    int previous_control = -1, next_control = -1;
private:
    std::filesystem::path root_;
    std::vector<DatabaseSeason> packs_;
    std::size_t selected_ = 0, default_ = 0;
    std::optional<std::size_t> pending_;
    std::optional<std::size_t> loaded_;
    bool active_ = false;
    bool legacy_2000_ = false;
    std::optional<std::string> names_directory_;
};
// Added controls use original creation/picking/drawing; only these two actions
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
