#include "database_seasons.hpp"
#include "guest_abi.hpp"
#include "platform.hpp"
#include "season_competition_rules.hpp"
#include <algorithm>
#include <array>
#include <string_view>

namespace cm::aot {
namespace {
using guest::Cpu;
bool legacy(const State& s) {
    return competition_edition(s) == CompetitionEdition::cm0001_389;
}
bool unavailable_row(const State& s, unsigned row) {
    const auto rows = competition_rules(s).unavailable_countries;
    return std::ranges::find(rows, row) != rows.end();
}
bool equals(Cpu& c, unsigned at, std::string_view value) {
    if (!at) return false;
    for (unsigned i = 0; i <= value.size(); ++i)
        if (c.u8(at + i) != (i == value.size() ? 0 : std::uint8_t(value[i]))) return false;
    return true;
}
struct IndexRequirement { unsigned caller; int slot; };
// Only the original validation call sites for audited unavailable-country
// dependencies and guarded German reserve names. Unknown failures retain the
// original diagnostic. Positive identity comes from the embedded names' SHA256.
constexpr std::array optional_indexes{
    // Italian
    IndexRequirement{0x10152f90, -0x6774},
    // Finnish
    IndexRequirement{0x10153e30, -0x69a0},
    IndexRequirement{0x10153e48, -0x69a4},
    IndexRequirement{0x10153e60, -0x69a8},
    IndexRequirement{0x10153e78, -0x69ac},
    // Turkish
    IndexRequirement{0x101542b0, -0x6a74},
    IndexRequirement{0x101542c8, -0x6a78},
    IndexRequirement{0x101542e0, -0x6a7c},
    IndexRequirement{0x101542f8, -0x6a80},
    IndexRequirement{0x10154310, -0x6a84},
    IndexRequirement{0x10154328, -0x6a88},
    IndexRequirement{0x10154340, -0x6a8c},
    IndexRequirement{0x10154358, -0x6a90},
    IndexRequirement{0x10154388, -0x6a9c},
    IndexRequirement{0x101543a0, -0x6aa0},
    IndexRequirement{0x101543b8, -0x6aa4},
    IndexRequirement{0x101543d0, -0x6aa8},
    IndexRequirement{0x101543e8, -0x6aac},
    // Brazilian
    IndexRequirement{0x101547d8, -0x6b04},
    IndexRequirement{0x10154808, -0x6b0c},
    IndexRequirement{0x10154820, -0x6b10},
    IndexRequirement{0x10154850, -0x6b18},
    IndexRequirement{0x10154880, -0x6b20},
    IndexRequirement{0x10154898, -0x6b24},
    IndexRequirement{0x101548b0, -0x6b28},
    IndexRequirement{0x101548c8, -0x6b2c},
    IndexRequirement{0x101548e0, -0x6b30},
    IndexRequirement{0x101548f8, -0x6b34},
    IndexRequirement{0x10154928, -0x6b3c},
    IndexRequirement{0x10154940, -0x6b40},
    IndexRequirement{0x10154958, -0x6b44},
    IndexRequirement{0x10154988, -0x6b4c},
    IndexRequirement{0x101549a0, -0x6b50},
    IndexRequirement{0x101549b8, -0x6b54},
    IndexRequirement{0x101549d0, -0x6b58},
    IndexRequirement{0x101549e8, -0x6b5c},
    IndexRequirement{0x10154a00, -0x6b60},
    IndexRequirement{0x10154a18, -0x6b64},
    IndexRequirement{0x10154a30, -0x6b68},
    IndexRequirement{0x10154a48, -0x6b6c},
    IndexRequirement{0x10154a60, -0x6b70},
    IndexRequirement{0x10154a78, -0x6b74},
    IndexRequirement{0x10154a90, -0x6b78},
    // Irish
    IndexRequirement{0x10154b08, -0x6b8c},
    IndexRequirement{0x10154b20, -0x6b90},
    // Italian
    IndexRequirement{0x101551f8, -0x6cb0},
    IndexRequirement{0x10155228, -0x6cb8},
    IndexRequirement{0x10155240, -0x6cbc},
    IndexRequirement{0x10155258, -0x6cc0},
    IndexRequirement{0x10155270, -0x6cc4},
    // Turkish
    IndexRequirement{0x101567b8, -0x7050},
    IndexRequirement{0x101567d0, -0x7054},
    // Brazilian
    IndexRequirement{0x10156d70, -0x7108},
    IndexRequirement{0x10156d88, -0x710c},
    IndexRequirement{0x10156da0, -0x7110},
    IndexRequirement{0x10156db8, -0x7114},
    IndexRequirement{0x10156dd0, -0x7118},
    IndexRequirement{0x10156de8, -0x711c},
    IndexRequirement{0x10156e00, -0x7120},
    IndexRequirement{0x10156e18, -0x7124},
    IndexRequirement{0x10156e30, -0x7128},
    IndexRequirement{0x10156e48, -0x712c},
    IndexRequirement{0x10156e60, -0x7130},
    IndexRequirement{0x10156e78, -0x7134},
    IndexRequirement{0x10156e90, -0x7138},
    IndexRequirement{0x10156ea8, -0x713c},
    IndexRequirement{0x10156ec0, -0x7140},
    IndexRequirement{0x10156ed8, -0x7144},
    IndexRequirement{0x10156ef0, -0x7148},
    IndexRequirement{0x10156f08, -0x714c},
    IndexRequirement{0x10156f20, -0x7150},
    IndexRequirement{0x10156f98, -0x7164},
    IndexRequirement{0x10156fb0, -0x7168},
    IndexRequirement{0x10156fc8, -0x716c},
    IndexRequirement{0x10156fe0, -0x7170},
    IndexRequirement{0x10156ff8, -0x7174},
    IndexRequirement{0x10157010, -0x7178},
    IndexRequirement{0x10157028, -0x717c},
    IndexRequirement{0x10157040, -0x7180},
    IndexRequirement{0x10157058, -0x7184},
    IndexRequirement{0x10157070, -0x7188},
    IndexRequirement{0x10157088, -0x718c},
    IndexRequirement{0x101570a0, -0x7190},
    IndexRequirement{0x10157178, -0x71b4},
    IndexRequirement{0x10157190, -0x71b8},
    IndexRequirement{0x101571a8, -0x71bc},
    IndexRequirement{0x101571c0, -0x71c0},
    IndexRequirement{0x101571d8, -0x71c4},
    IndexRequirement{0x101571f0, -0x71c8},
    IndexRequirement{0x10157208, -0x71cc},
    IndexRequirement{0x10157220, -0x71d0},
    IndexRequirement{0x10157238, -0x71d4},
    IndexRequirement{0x10157250, -0x71d8},
    IndexRequirement{0x10157268, -0x71dc},
    IndexRequirement{0x10157280, -0x71e0},
    IndexRequirement{0x10157298, -0x71e4},
    IndexRequirement{0x101572b0, -0x71e8},
    IndexRequirement{0x101572c8, -0x71ec},
    IndexRequirement{0x101572e0, -0x71f0},
    // Argentina
    IndexRequirement{0x10157bb0, -0x72dc},
    // German Reserve
    IndexRequirement{0x10158fa8, -0x795c},
    IndexRequirement{0x10158fc0, -0x7820},
    IndexRequirement{0x10158fd8, -0x7960},
    IndexRequirement{0x10159008, -0x7964},
    IndexRequirement{0x10159038, -0x7968},
};
}
void database_season_index_begin(State& s) {
    if (s.platform) s.platform->inspect_database_season_names();
}
void database_season_competition_index_name(State& s) {
    if (!legacy(s)) return;
    Cpu c(s);
    if (!equals(c, c.r(3), "Brazilian National Championship")) return;
    const auto ordinal = c.r(4), competitions = c.u32(c.toc(-0x7bbc));
    if (!competitions || ordinal >= c.u32(c.toc(-0x7b64))) return;
    const auto nation = c.u32(competitions + ordinal * 0x6c + 8);
    const auto brazil = c.i32(c.toc(-0x7b68));
    const auto nations = c.u32(c.toc(-0x7b98));
    if (brazil < 0 || unsigned(brazil) >= c.u32(c.toc(-0x7b04)) || !nations ||
        nation != nations + unsigned(brazil) * 0x128) return;
    // Same top flight under its historical name. Continental qualification
    // needs this semantic slot even when the Brazilian country is inactive.
    c.put32(c.toc(-0x6b88), ordinal);
}
bool database_season_optional_index(State& s) {
    if (!legacy(s)) return false;
    Cpu c(s);
    const auto found = std::ranges::find(optional_indexes, c.lr(), &IndexRequirement::caller);
    return found != optional_indexes.end() && c.i32(c.toc(found->slot)) == -1;
}
CompetitionEdition competition_edition(const State& s) {
    if (!s.platform) return CompetitionEdition::cm0102;
    switch (s.platform->database_seasons().release()) {
    case DatabaseRelease::cm0001_389: return CompetitionEdition::cm0001_389;
    case DatabaseRelease::si_3968: return CompetitionEdition::si_3968;
    case DatabaseRelease::other: break;
    }
    return CompetitionEdition::cm0102;
}
void database_season_limit_leagues(State& s) {
    if (competition_rules(s).unavailable_countries.empty()) return;
    Cpu c(s);
    const auto descriptors = c.toc(-0x7b44), selected = c.toc(-0x64c4);
    for (auto index : competition_rules(s).unavailable_countries) {
        const auto row = descriptors + index * 0x4c, nation = c.u32(row);
        if (!nation) continue;
        c.put8(nation + 0x120, 0);
        c.put32(row + 8, 0); // Bulk selection skips unavailable constructors.
        if (c.u32(selected) == nation) c.put32(selected, 0);
    }
    unsigned total = 0, full = 0, first = 0;
    for (unsigned i = 0; i < 34; ++i) {
        const auto nation = c.u32(descriptors + i * 0x4c);
        if (!nation) continue;
        const auto flags = c.u8(nation + 0x120);
        total += flags != 0; full += (flags & 2) != 0;
        if (!first && (flags & 2)) first = nation;
    }
    // The language's original default may be unavailable (e.g. Italian).
    // Choose an existing full-detail country, otherwise England, without RNG.
    if (!c.u32(selected)) {
        if (!first) {
            first = c.u32(descriptors + 14 * 0x4c);
            if (first) {
                total += c.u8(first + 0x120) == 0;
                full += (c.u8(first + 0x120) & 2) == 0;
                c.put8(first + 0x120, 2);
            }
        }
        c.put32(selected, first);
    }
    c.put32(c.toc(-0x62b8), total); c.put32(c.toc(-0x62b4), full);
}
void database_season_filter_league_row(State& s) {
    if (competition_rules(s).unavailable_countries.empty()) return;
    Cpu c(s);
    // Only the builder's local pointer changes. A null descriptor nation means
    // a global competition to the world constructor and must never be written.
    if (unavailable_row(s, c.u8(c.r(15)))) c.set(17, 0);
}
}
