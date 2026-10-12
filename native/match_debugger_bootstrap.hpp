#pragma once

#include "aot_runtime.hpp"
#include "match_debugger_database.hpp"
#include <filesystem>
#include <memory>
#include <optional>
#include <vector>

namespace cm::aot::match_debugger {

struct RosterMember {
    std::uint32_t staff_id = 0, staff = 0, player = 0;
    bool operator==(const RosterMember&) const = default;
};

// A fresh original database world, before any career or match is created.
// Remaining original dependencies execute through the production dispatcher.
// The private directory must be new: existing configuration and career files
// cannot silently become inputs to a debugger experiment.
class DatabaseWorld {
public:
    static DatabaseWorld load(const std::filesystem::path& original_program,
        const std::filesystem::path& original_game,
        const std::filesystem::path& new_private_directory,
        std::uint64_t instruction_limit = 600000000,
        std::optional<std::uint32_t> random_seed = std::nullopt);

    const DatabaseCatalog& catalog() const { return catalog_; }
    const State& state() const { return *state_; }
    State& state() { return *state_; }
    std::uint32_t club_pointer(std::uint32_t id) const;
    std::vector<RosterMember> roster(std::uint32_t club_id) const;
    std::uint32_t staff_count() const { return staff_count_; }
    std::unique_ptr<State> release_state() &&;

private:
    DatabaseCatalog catalog_;
    std::unique_ptr<State> state_;
    std::uint32_t clubs_ = 0, staff_ = 0, staff_count_ = 0;
    std::uint32_t players_ = 0, player_count_ = 0;
};

}
