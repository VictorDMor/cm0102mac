#pragma once
#include <cstdint>
#include <filesystem>
#include <span>
#include <string>
#include <vector>

namespace cm::aot::match_debugger {
enum class ClubKind { club, national_team };
struct Club {
    std::uint32_t id = 0;
    ClubKind kind = ClubKind::club;
    std::string name, short_name;
};

// Read-only original database input for club selection and the match factory.
// Career files and captured guest worlds are not catalog inputs. Raw Mac records
// retain their original byte order; guest pointer resolution belongs to bootstrap.
class DatabaseCatalog {
public:
    static DatabaseCatalog load(const std::filesystem::path& original_game);
    static DatabaseCatalog parse(std::span<const std::uint8_t> index,
        std::span<const std::uint8_t> club_records,
        std::span<const std::uint8_t> national_records,
        std::span<const std::uint8_t> aligned_english_names);

    const std::vector<Club>& clubs() const { return clubs_; }
    const Club& club(std::uint32_t id) const;
    std::span<const std::uint8_t> original_record(std::uint32_t id) const;

private:
    std::vector<Club> clubs_;
    std::vector<std::uint8_t> records_;
};
}
