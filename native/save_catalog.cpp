#include "save_catalog.hpp"
#include <algorithm>
#include <cctype>
#include <fstream>
#include <map>
#include <mutex>
#include <system_error>
#include <sys/stat.h>
#include <ctime>

namespace cm::aot::save_catalog {
namespace {
namespace fs = std::filesystem;
// The container: a word, the entry count, then 1000 directory slots of 264 bytes
// (payload offset, decoded size, a 256-byte name), then the payloads.
constexpr std::size_t table_start = 8, slot_size = 264, slot_count = 1000, payload_start = table_start + slot_count * slot_size;
constexpr std::int32_t first_readable = 0x3e023, first_with_save_time = 0x3e027;
constexpr std::size_t old_summary = 0xf70, summary_size = 0xf74;
constexpr char entry_name[] = "general.dat";

std::uint32_t word(const std::string& data, std::size_t at) {
    return at + 4 <= data.size() ? (std::uint32_t(std::uint8_t(data[at])) << 24 | std::uint32_t(std::uint8_t(data[at + 1])) << 16 |
                                    std::uint32_t(std::uint8_t(data[at + 2])) << 8 | std::uint8_t(data[at + 3])) : 0;
}
std::string text_at(const std::string& data, std::size_t at, std::size_t size) {
    if (at >= data.size()) return {};
    const auto end = std::min(data.size(), at + size);
    const auto stop = std::find(data.begin() + std::ptrdiff_t(at), data.begin() + std::ptrdiff_t(end), '\0');
    return {data.begin() + std::ptrdiff_t(at), stop};
}
}

std::string decode_prefix(const std::string& payload, std::size_t limit) {
    std::string out;
    out.reserve(limit);
    for (std::size_t i = 0; i < payload.size() && out.size() < limit;) {
        const auto value = std::uint8_t(payload[i++]);
        if (value <= 0x80) out.push_back(char(value));
        else {
            if (i == payload.size()) break;
            out.append(std::min<std::size_t>(value - 0x80, limit - out.size()), payload[i++]);
        }
    }
    return out;
}

std::optional<Summary> read_summary(const fs::path& path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return std::nullopt;
    std::string head(payload_start, '\0');
    file.read(head.data(), std::streamsize(head.size()));
    if (std::size_t(file.gcount()) < payload_start || (std::uint8_t(head[0]) != 4 && std::uint8_t(head[0]) != 3)) return std::nullopt;
    const auto count = word(head, 4);
    if (count < 1 || count > slot_count) return std::nullopt;
    std::error_code error;
    const auto total = std::uint64_t(fs::file_size(path, error));
    if (error) return std::nullopt;
    for (std::size_t i = 0; i < count; ++i) {
        const auto slot = table_start + i * slot_size;
        if (text_at(head, slot + 8, slot_size - 8) != entry_name) continue;
        const std::uint64_t begin = word(head, slot);
        const std::uint64_t end = i + 1 < count ? word(head, slot + slot_size) : total;
        if (begin < payload_start || end < begin || end > total) return std::nullopt;
        // Every input byte makes at least one output byte, and the longest read is 4 + 0xf74 bytes.
        std::string payload(std::size_t(std::min<std::uint64_t>(end - begin, 2 * (4 + summary_size))), '\0');
        file.clear();
        file.seekg(std::streamoff(begin));
        file.read(payload.data(), std::streamsize(payload.size()));
        payload.resize(std::size_t(file.gcount()));
        // Layout 4 stores its entries run-length coded; layout 3 (older saves) stores them as they are.
        const auto data = head[0] == 3 ? payload.substr(0, 4 + summary_size) : decode_prefix(payload, 4 + summary_size);
        if (data.size() < 4 + old_summary) return std::nullopt;
        Summary s;
        const auto raw = std::int32_t(word(data, 0));
        s.version = raw < 0 ? -raw : raw;  // the original takes the absolute value
        if (s.version < first_readable) return std::nullopt;
        const bool with_time = s.version >= first_with_save_time;
        if (with_time && data.size() < 4 + summary_size) return std::nullopt;
        const std::size_t at = 4;
        s.foreground = text_at(data, at, 0x3e8);
        s.background = text_at(data, at + 0x3e8, 0x3e8);
        s.game_version = text_at(data, at + 0x7d0, 0x14);
        for (std::size_t m = 0; m < 16; ++m) {
            const auto row = at + 0x7e4 + m * 0x78;
            if (!data[row]) continue;
            s.managers.push_back({text_at(data, row, 0x3c), text_at(data, row + 0x3c, 0x3c)});
        }
        s.day = std::uint16_t(std::uint8_t(data[at + 0xf64]) << 8 | std::uint8_t(data[at + 0xf65]));
        s.year = std::uint16_t(std::uint8_t(data[at + 0xf66]) << 8 | std::uint8_t(data[at + 0xf67]));
        s.leap = std::uint8_t(data[at + 0xf68]);
        s.played = word(data, at + 0xf6c);
        if (with_time) {
            s.has_saved_time = true;
            s.saved = word(data, at + 0xf70);
        }
        return s;
    }
    return std::nullopt;
}

std::int64_t modification_time(const fs::path& path) {
    struct stat info{};
    if (::stat(path.c_str(), &info) != 0) return 0;
    // The game's clock_time counts local wall-clock seconds, and its local_time shows them as they are.
    struct tm local{};
    const time_t when = info.st_mtime;
    return localtime_r(&when, &local) ? std::int64_t(when) + local.tm_gmtoff : std::int64_t(when);
}

std::optional<Summary> cached_summary(const fs::path& path) {
    struct Key {
        std::uintmax_t size;
        fs::file_time_type time;
    };
    static std::mutex lock;
    static std::map<fs::path, std::pair<Key, std::optional<Summary>>> memory;
    std::error_code error;
    const auto size = fs::file_size(path, error);
    if (error) return std::nullopt;
    const auto time = fs::last_write_time(path, error);
    if (error) return std::nullopt;
    {
        std::lock_guard guard(lock);
        const auto found = memory.find(path);
        if (found != memory.end() && found->second.first.size == size && found->second.first.time == time) return found->second.second;
    }
    auto summary = read_summary(path);
    std::lock_guard guard(lock);
    if (memory.size() > 2000) memory.clear();
    memory[path] = {{size, time}, summary};
    return summary;
}

void sort_recent_first(std::vector<Entry>& entries) {
    const auto lower = [](const std::string& text) {
        std::string result(text);
        for (auto& ch : result) ch = char(std::tolower(static_cast<unsigned char>(ch)));
        return result;
    };
    std::stable_sort(entries.begin(), entries.end(), [&](const Entry& a, const Entry& b) {
        if (a.saved_at() != b.saved_at()) return a.saved_at() > b.saved_at();
        return lower(a.name) < lower(b.name);
    });
}
}
