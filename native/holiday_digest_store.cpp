#include "holiday_digest_store.hpp"
#include <array>
#include <unistd.h>
#include <fstream>
#include <stdexcept>
#include <utility>

namespace cm::aot::holiday_digest {
namespace {
constexpr std::string_view header = "CM-HOLIDAY-1\n";
// Bounds the optional member independently of the original archive and rejects
// malicious counts before they can request large allocations.
constexpr std::uint32_t max_bytes = 64 * 1024 * 1024;

class Reader {
    std::ifstream file_;
    std::uintmax_t left_;

public:
    explicit Reader(const std::filesystem::path& file)
        : file_(file, std::ios::binary), left_(std::filesystem::file_size(file)) {
        if (!file_ || left_ > max_bytes) throw std::runtime_error("Unreadable or oversized holiday news member");
    }
    void bytes(char* dst, std::size_t n) {
        if (n > left_ || !file_.read(dst, static_cast<std::streamsize>(n)))
            throw std::runtime_error("Truncated holiday news member");
        left_ -= n;
    }
    std::uint32_t number() {
        std::array<unsigned char, 4> b{};
        bytes(reinterpret_cast<char*>(b.data()), b.size());
        return (std::uint32_t(b[0]) << 24) | (std::uint32_t(b[1]) << 16) | (std::uint32_t(b[2]) << 8) | b[3];
    }
    std::string text() {
        const auto size = number();
        if (size > left_) throw std::runtime_error("Truncated holiday news text");
        std::string s(size, '\0');
        bytes(s.data(), size);
        return s;
    }
    std::vector<std::string> lines() {
        const auto n = number();
        if (n > left_ / 4 || n > 1000000) throw std::runtime_error("Invalid holiday news line count");
        std::vector<std::string> result;
        for (std::uint32_t i = 0; i < n; ++i) result.push_back(text());
        return result;
    }
    bool finished() const { return left_ == 0; }
};

void put_number(std::ostream& out, std::uint32_t n) {
    const std::array<char, 4> bytes{char(n >> 24), char(n >> 16), char(n >> 8), char(n)};
    out.write(bytes.data(), bytes.size());
}
void put_text(std::ostream& out, const std::string& s) {
    if (s.size() > max_bytes) throw std::runtime_error("Oversized holiday news text");
    put_number(out, static_cast<std::uint32_t>(s.size()));
    out.write(s.data(), static_cast<std::streamsize>(s.size()));
}
void put_lines(std::ostream& out, const std::vector<std::string>& lines) {
    if (lines.size() > 1000000) throw std::runtime_error("Too many holiday news lines");
    put_number(out, static_cast<std::uint32_t>(lines.size()));
    for (const auto& line : lines) put_text(out, line);
}
}

void Store::load(const std::filesystem::path& file) {
    Store loaded;
    if (std::filesystem::exists(file)) {
        Reader r(file);
        std::array<char, header.size()> magic{};
        r.bytes(magic.data(), magic.size());
        if (std::string_view(magic.data(), magic.size()) != header) throw std::runtime_error("Unknown holiday news format");
        loaded.next_id = r.number();
        if (!loaded.next_id) throw std::runtime_error("Invalid holiday news sequence");
        const auto count = r.number();
        for (std::uint32_t i = 0; i < count; ++i) {
            Report entry;
            entry.id = r.number();
            entry.manager = r.number();
            entry.club = r.number();
            const auto active = r.number();
            if (!entry.id || active > 1 || entry.id >= loaded.next_id || loaded.reports.contains(entry.id))
                throw std::runtime_error("Invalid holiday news record");
            entry.active = active != 0;
            if (entry.active) loaded.active_ids.push_back(entry.id);
            entry.starting_date = r.text();
            entry.starting_position = r.text();
            entry.results = r.lines();
            entry.incoming = r.lines();
            entry.outgoing = r.lines();
            entry.lines = r.lines();
            loaded.reports.emplace(entry.id, std::move(entry));
        }
        if (!r.finished()) throw std::runtime_error("Trailing holiday news records");
    }
    *this = std::move(loaded);
}

void Store::save(const std::filesystem::path& file) const {
    if (reports.empty() && next_id == 1) {
        std::filesystem::remove(file);
        return;
    }
    std::filesystem::create_directories(file.parent_path());
    // The original enumerates *.tmp after this hook. An incomplete staging
    // file must never match the glob or replace the last complete member.
    auto directory = (file.parent_path() / ".holiday-news-XXXXXX").string();
    if (!mkdtemp(directory.data())) throw std::runtime_error("Cannot stage holiday news member");
    const auto staging = std::filesystem::path(directory) / "member";
    std::error_code ignored;
    try {
        std::ofstream out;
        out.exceptions(std::ios::failbit | std::ios::badbit);
        out.open(staging, std::ios::binary | std::ios::trunc);
        out.write(header.data(), header.size());
        if (!next_id || reports.size() >= next_id) throw std::runtime_error("Invalid holiday news sequence");
        put_number(out, next_id);
        put_number(out, static_cast<std::uint32_t>(reports.size()));
        for (const auto& [id, entry] : reports) {
            if (!id || entry.id != id || id >= next_id) throw std::runtime_error("Invalid holiday news sequence");
            put_number(out, id);
            put_number(out, entry.manager);
            put_number(out, entry.club);
            put_number(out, entry.active);
            put_text(out, entry.starting_date);
            put_text(out, entry.starting_position);
            put_lines(out, entry.results);
            put_lines(out, entry.incoming);
            put_lines(out, entry.outgoing);
            put_lines(out, entry.lines);
            if (out.tellp() > max_bytes) throw std::runtime_error("Oversized holiday news member");
        }
        out.close();
        std::filesystem::rename(staging, file);
    } catch (...) {
        std::filesystem::remove(staging, ignored);
        std::filesystem::remove(directory, ignored);
        throw;
    }
    std::filesystem::remove(directory, ignored);
}
}
