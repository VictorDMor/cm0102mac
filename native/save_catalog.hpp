#pragma once
// A host-side, read-only view of a saved career's summary (the block the
// original's Info screen reads from the save's general.dat entry), and the
// order the saved-game browser lists saves in. Nothing here touches guest
// memory, the guest stack or the saves: a file is opened for reading, only
// its entry directory and the first bytes of one entry are decoded, and the
// result is cached by path, size and modification time.
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace cm::aot::save_catalog {
// Strings stay in the bytes the game wrote (MacRoman); the guest shows them as they are.
struct Manager {
    std::string name, club;
};
struct Summary {
    std::int32_t version = 0;       // the save's version word, as the original reads it (absolute value)
    bool has_saved_time = false;    // saves from before 0x3e027 carry no save time or play time
    std::uint32_t saved = 0;        // the guest's clock_time at the save (local wall-clock seconds since 1970)
    std::uint32_t played = 0;       // seconds of play
    std::uint16_t day = 0, year = 0;  // the game date record: day of the year, year
    std::uint8_t leap = 0;
    std::string game_version, foreground, background;
    std::vector<Manager> managers;
};
// The summary, or nothing when the file is not a save this reader knows
// (unreadable, another container layout, no general.dat, or a version older
// than the original's own Info screen shows).
std::optional<Summary> read_summary(const std::filesystem::path&);
// The file's modification time in the game's clock: local wall-clock seconds, like the save time inside a save; 0 when it cannot be read.
std::int64_t modification_time(const std::filesystem::path&);
// The same, remembered while the file's size and modification time are unchanged.
std::optional<Summary> cached_summary(const std::filesystem::path&);

// One save as the browser lists it.
struct Entry {
    std::string name;                // the name the dialog shows and loads (guest bytes, no extension)
    std::filesystem::path path;      // empty when the host file could not be found
    std::int64_t modified = 0;       // the file's modification time, seconds since 1970; 0 when unknown
    std::optional<Summary> summary;
    bool flagged = false;            // the dialog's filter marked it (the classic list draws such a name in the marked colour)
    // When the save was last written: the save time inside the file, else the file's time.
    std::int64_t saved_at() const { return summary && summary->has_saved_time ? std::int64_t(summary->saved) : modified; }
};
// Most recently saved first; equal times keep the names in case-insensitive order.
// A pure reordering: every entry keeps its own name and path.
void sort_recent_first(std::vector<Entry>&);
// Decodes the container's first `limit` bytes of one entry (the original's run-length
// coding: a byte up to 0x80 stands for itself; a higher byte repeats the next one
// that many minus 0x80 times). Exposed for the checks.
std::string decode_prefix(const std::string& payload, std::size_t limit);
}
