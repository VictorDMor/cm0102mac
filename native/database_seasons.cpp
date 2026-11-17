#include "database_seasons.hpp"
#include "guest_abi.hpp"
#include "platform.hpp"
#include <algorithm>
#include <array>
#include <cstdio>
#include <fstream>
#include <iomanip>
#include <set>
#include <CommonCrypto/CommonDigest.h>

namespace cm::aot {
namespace {
bool token(std::string_view value) {
    return !value.empty() && value.size() <= 48 && std::ranges::all_of(value, [](unsigned char ch) {
        return (ch >= 'a' && ch <= 'z') || (ch >= '0' && ch <= '9') || ch == '-';
    });
}
std::string text(const State& s, std::uint32_t at) {
    std::string result;
    for (unsigned i = 0; at && i < 256; ++i) {
        const char ch = s.memory.read<1>(at + i);
        if (!ch) return result;
        result += ch;
    }
    throw std::runtime_error("Unterminated database path");
}
}
DatabaseSeasons::DatabaseSeasons(std::filesystem::path runtime) : root_(runtime / "Files/Databases") {
    // Seasons are optional: a damaged catalog must leave the original
    // 2001/2002 game (and every harness on this runtime) able to start.
    try {
        std::ifstream input(root_ / "catalog.txt");
        if (!input) return;
        std::string magic;
        unsigned version;
        if (!(input >> magic >> version) || magic != "CM_DATABASE_SEASONS" || version != 1)
            throw std::runtime_error("Invalid database season catalog");
        std::set<std::string> directories, labels;
        DatabaseSeason pack;
        while (input >> std::ws && input.peek() != EOF) {
            if (!(input >> pack.year >> std::quoted(pack.label) >> std::quoted(pack.directory)) ||
                (pack.year < 2000 || pack.year > 2002) || !token(pack.directory) ||
                pack.label != std::to_string(pack.year) + "/" + std::to_string(pack.year + 1) ||
                !directories.insert(pack.directory).second || !labels.insert(pack.label).second || packs_.size() >= 32)
                throw std::runtime_error("Invalid database season entry");
            packs_.push_back(pack);
        }
        const auto original = std::ranges::find(packs_, 2001u, &DatabaseSeason::year);
        if (original == packs_.end())
            throw std::runtime_error("Missing default 2001 database");
        default_ = std::size_t(original - packs_.begin());
        selected_ = default_;
    } catch (const std::exception& e) {
        packs_.clear();
        selected_ = default_ = 0;
        std::fprintf(stderr, "Database seasons disabled: %s\n", e.what());
    }
}
void DatabaseSeasons::begin(bool tables_present) {
    if (!enabled()) return;
    selected_ = default_;
    // A cold opener performs the original load exactly once. A warm opener
    // can reuse the same source; a different/saved source must be reloaded only
    // after the monitor has destroyed its controls.
    pending_ = tables_present && loaded_ != default_ ? std::optional(default_) : std::nullopt;
    active_ = true;
    error.clear();
}
void DatabaseSeasons::end() {
    active_ = false;
    pending_.reset();
    previous_control = next_control = -1;
}
void DatabaseSeasons::inspect_database_names(const std::filesystem::path& path) {
    legacy_2000_ = false;
    std::error_code error;
    if (std::filesystem::file_size(path, error) != 1130151 || error) return;
    std::vector<unsigned char> bytes(1130151);
    std::ifstream input(path, std::ios::binary);
    if (!input.read(reinterpret_cast<char*>(bytes.data()), bytes.size())) return;
    std::array<unsigned char, CC_SHA256_DIGEST_LENGTH> digest;
    CC_SHA256(bytes.data(), CC_LONG(bytes.size()), digest.data());
    constexpr char hex[] = "0123456789abcdef";
    std::string actual;
    for (auto byte : digest) { actual += hex[byte >> 4]; actual += hex[byte & 15]; }
    // Embedded name data survives pruning/regeneration and is restored by the
    // loader before INDEX. Never infer the profile from year or staff counts.
    legacy_2000_ = actual == "b2b61aed6afaf90b2632b5f85ded0566884810d02e59d2aec1526ab4689a68a2";
}
bool DatabaseSeasons::request(int direction) {
    if (!enabled() || !active_ || pending_ || (direction != -1 && direction != 1)) return false;
    const auto next = std::ptrdiff_t(selected_) + direction;
    if (next < 0 || std::size_t(next) >= packs_.size()) return false;
    pending_ = std::size_t(next);
    return true;
}
void DatabaseSeasons::activate(std::size_t index) {
    if (index >= packs_.size()) throw std::runtime_error("Unknown database season");
    selected_ = index;
    active_ = true;
}
void DatabaseSeasons::preflight(std::size_t index) const {
    const auto directory = root_ / packs_.at(index).directory;
    if (std::filesystem::is_symlink(directory) || std::filesystem::is_symlink(directory / "Data"))
        throw std::runtime_error("Database pack directory is a symbolic link");
    std::ifstream input(directory / "files.txt");
    std::string magic, name;
    unsigned version;
    if (!(input >> magic >> version) || magic != "CM_DATABASE_FILES" || version != 1)
        throw std::runtime_error("Missing database inventory");
    std::uintmax_t size;
    unsigned count = 0;
    bool found_index = false, found_english = false;
    while (input >> std::ws && input.peek() != EOF) {
        if (!(input >> std::quoted(name) >> size) || name.empty() || name.size() > 63 ||
            name.find_first_of("/:\\") != std::string::npos || name == "." || name == ".." || ++count > 128)
            throw std::runtime_error("Invalid database inventory entry");
        const auto path = directory / "Data" / name;
        if (std::filesystem::is_symlink(path) || !std::filesystem::is_regular_file(path) ||
            std::filesystem::file_size(path) != size)
            throw std::runtime_error("Missing or incomplete database file: " + name);
        found_index |= name == "index.dat" || name == "index.dam";
        found_english |= name == "eng.lng";
    }
    if (!found_index || !found_english) throw std::runtime_error("Incomplete database inventory");
}
std::optional<std::string> DatabaseSeasons::data_path(std::string_view variable, std::string_view name) const {
    if (!active_ || !enabled() || (variable != "SI_DATA" && variable != "CM3_DATA")) return {};
    // UI resources, fonts, tactics, weather and engine scripts remain the Mac
    // release's. No default database fallback is permitted for tables/languages.
    if (!name.empty() && name != "player_setup.cfg" && name != "*" && name != "*.*" &&
        !name.ends_with(".dat") && !name.ends_with(".dam") && !name.ends_with(".lng")) return {};
    if (name.find_first_of("/:\\") != std::string_view::npos) return {};
    return "CM 01-02:Databases:" + packs_[selected_].directory + ":Data:" + std::string(name);
}
void database_season_data_path(State& s, std::uint32_t variable, std::uint32_t name, std::uint32_t output) {
    if (!s.platform) return;
    auto& seasons = s.platform->database_seasons();
    const auto folder = text(s, variable), file = text(s, name);
    // DATABASE copies/extracts both plain and packed names into CM3_TEMP before
    // INDEX. Preserve the resolved guest directory, including its legitimate
    // empty/current-directory fallback, and inspect it only after extraction.
    if (folder == "CM3_TEMP" && file.empty()) seasons.note_names_directory(text(s, output));
    const auto replacement = seasons.data_path(folder, file);
    if (!replacement) return;
    if (replacement->size() >= 256) throw std::runtime_error("Database path exceeds guest buffer");
    for (std::size_t i = 0; i <= replacement->size(); ++i)
        s.memory.write<1>(output + i, i == replacement->size() ? 0 : std::uint8_t((*replacement)[i]));
}
void database_season_nation_index_name(State& s) {
    // INDEX's symbolic TAIWAN slot predates SI 3.9.68's naming. Give only its
    // name mapper the existing synonym; preserve the record and all display
    // names. Saved databases use the same mapper without an active season pack.
    guest::Cpu c(s);
    constexpr std::string_view name = "Chinese Taipei";
    for (std::size_t i = 0; i <= name.size(); ++i)
        if (c.u8(c.r(3) + i) != (i == name.size() ? 0 : std::uint8_t(name[i]))) return;
    c.set(3, c.toc(-0x547c) + 0x8320); // Original INDEX literal: Taiwan.
}
}
