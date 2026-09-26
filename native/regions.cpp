#include "regions.hpp"
#include <algorithm>
#include <iterator>
#include <set>
#include <stdexcept>
namespace cm::aot {
namespace {
void word(std::vector<std::uint8_t>& bytes, std::int16_t value) {
    bytes.push_back(std::uint16_t(value) >> 8);
    bytes.push_back(std::uint16_t(value) & 255);
}
std::int16_t word(std::span<const std::uint8_t> bytes, std::size_t at) {
    if (at + 2 > bytes.size()) throw std::runtime_error("Truncated original region");
    return std::int16_t(std::uint16_t(bytes[at]) << 8 | bytes[at + 1]);
}
}
std::vector<std::uint8_t> region_union(std::span<const LegacyRect> input) {
    std::vector<LegacyRect> rectangles;
    std::set<std::int16_t> rows;
    LegacyRect bounds{};
    for (const auto& r : input) {
        if (r[0] >= r[2] || r[1] >= r[3]) continue;
        if (std::find(r.begin(), r.end(), INT16_MAX) != r.end())
            throw std::runtime_error("Region coordinates collide with the QuickDraw scanline sentinel");
        if (rectangles.empty()) bounds = r;
        else { bounds[0] = std::min(bounds[0], r[0]); bounds[1] = std::min(bounds[1], r[1]);
               bounds[2] = std::max(bounds[2], r[2]); bounds[3] = std::max(bounds[3], r[3]); }
        rectangles.push_back(r);
        rows.insert(r[0]); rows.insert(r[2]);
    }
    std::vector<std::uint8_t> bytes;
    word(bytes, 10);
    for (auto v : bounds) word(bytes, v);
    if (rectangles.empty()) return bytes;
    std::vector<std::int16_t> previous;
    bool rectangular = true;
    for (auto y : rows) {
        std::vector<std::pair<std::int16_t, std::int16_t>> intervals;
        for (const auto& r : rectangles) if (y >= r[0] && y < r[2]) intervals.emplace_back(r[1], r[3]);
        std::sort(intervals.begin(), intervals.end());
        std::vector<std::int16_t> current;
        for (auto [left, right] : intervals) {
            if (!current.empty() && left <= current.back()) current.back() = std::max(right, current.back());
            else { current.push_back(left); current.push_back(right); }
        }
        if (y < bounds[2] && current != std::vector<std::int16_t>{bounds[1], bounds[3]}) rectangular = false;
        std::vector<std::int16_t> changes;
        std::set_symmetric_difference(previous.begin(), previous.end(), current.begin(), current.end(),
                                      std::back_inserter(changes));
        if (!changes.empty()) {
            word(bytes, y);
            for (auto x : changes) word(bytes, x);
            word(bytes, INT16_MAX);
        }
        previous = std::move(current);
    }
    if (rectangular) bytes.resize(10);
    else word(bytes, INT16_MAX);
    if (bytes.size() > 32766) throw std::runtime_error("Desktop region exceeds original QuickDraw size limit");
    bytes[0] = bytes.size() >> 8; bytes[1] = bytes.size() & 255;
    return bytes;
}
bool region_contains(std::span<const std::uint8_t> bytes, std::int16_t x, std::int16_t y) {
    if (bytes.size() < 10 || std::uint16_t(word(bytes, 0)) != bytes.size())
        throw std::runtime_error("Invalid original region size");
    if (y < word(bytes, 2) || x < word(bytes, 4) || y >= word(bytes, 6) || x >= word(bytes, 8)) return false;
    if (bytes.size() == 10) return true;
    std::set<std::int16_t> transitions;
    for (std::size_t at = 10; ; ) {
        const auto row = word(bytes, at); at += 2;
        if (row == INT16_MAX || row > y) break;
        for (;;) {
            const auto column = word(bytes, at); at += 2;
            if (column == INT16_MAX) break;
            if (!transitions.erase(column)) transitions.insert(column);
        }
    }
    return std::distance(transitions.begin(), transitions.upper_bound(x)) % 2 != 0;
}
}
