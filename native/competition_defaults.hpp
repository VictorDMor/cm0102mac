#pragma once
// The values each competition has before any override: what the competition editor lists and compares against.
// They are set by code (each competition's configure), so there is no file to read them from; the game records
// them as it reaches the apply points (competition_overrides.hpp `at_setup`), once per competition and edition,
// into `<directory>/<edition>.toml` in the full-dump format of competition_values.hpp. A competition appears there
// after the first career whose world builds it; the file only grows.
#include "competition_overrides.hpp"
#include <array>
#include <filesystem>
#include <optional>
#include <set>
#include <tuple>
#include <vector>

namespace cm::aot {
class CompetitionDefaults {
public:
    explicit CompetitionDefaults(std::filesystem::path directory) : directory_(std::move(directory)) {}
    static std::filesystem::path file(const std::filesystem::path& directory, CompetitionEdition);
    // Reads the object (a league or cup about to be set up, still holding what its configure wrote) and appends
    // it unless the edition's file already has its key. Only reads guest memory; failures are logged once.
    void record(const State&, std::uint32_t object, const competition_overrides::Target&, CompetitionEdition);

private:
    using Key = std::tuple<competition_values::Kind, std::int32_t, std::int32_t>;
    std::set<Key>* seen(CompetitionEdition);
    std::filesystem::path directory_;
    std::array<std::optional<std::set<Key>>, competition_edition_count> seen_;
    bool failed_ = false;
};
// The recorded entries of a file, the first of each key; empty when it does not exist. A record that does not
// parse (a torn append, or one made before the registry gained a parameter) is skipped and counted: the game
// records that competition again.
std::vector<competition_values::Entry> read_competition_defaults(const std::filesystem::path& file, unsigned* skipped = nullptr);
}
