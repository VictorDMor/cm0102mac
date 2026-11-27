#pragma once
#include <cstdint>
#include <filesystem>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace cm::aot::holiday_digest {

struct Report {
    std::uint32_t id = 0, manager = 0, club = 0;
    bool active = false;
    std::string starting_date, starting_position;
    std::vector<std::string> results, incoming, outgoing, lines;
    friend bool operator==(const Report&, const Report&) = default;
};

// One companion member per career regardless of the number of news items or pages.
// Only stable record IDs and owned display text survive a reload.
class Store {
public:
    static constexpr std::string_view member_name = "holiday_news.tmp";
    std::map<std::uint32_t, Report> reports;
    std::vector<std::uint32_t> active_ids; // derived from the active flag on load
    std::uint32_t next_id = 1;
    void load(const std::filesystem::path&);
    void save(const std::filesystem::path&) const;
};
}
