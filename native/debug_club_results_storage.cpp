#include "debug_match_results.hpp"
#include "aot_runtime.hpp"
#include <fstream>
#include <iomanip>
#include <set>
#include <unistd.h>

namespace cm::aot {
namespace {
std::filesystem::path companion(const std::filesystem::path& save) {
    return std::filesystem::path(save.string() + ".forced-results");
}
std::string club_key(const State& s, std::uint32_t club) {
    // Full club name in the loaded database (not a pointer or UI label).
    std::string name;
    for (unsigned i = 0; i < 51; ++i) {
        const auto value = char(s.memory.read<1>(club + 0x1ee + i));
        if (!value) break;
        name += value;
    }
    return name;
}
}

void save_debug_club_results(const State& s, const std::filesystem::path& save) {
    const auto target = companion(save);
    if (match_result_debugging(s).clubs.empty() && !std::filesystem::exists(target)) return;
    auto temporary = target;
    temporary += ".tmp-" + std::to_string(getpid());
    try {
        std::ofstream output(temporary, std::ios::binary | std::ios::trunc);
        output.exceptions(std::ios::badbit | std::ios::failbit);
        const auto& rules = match_result_debugging(s).clubs;
        output << "CM_FORCED_CLUB_RESULTS 1\n" << rules.size() << '\n';
        for (const auto& rule : rules) {
            if (!valid_debug_result_club(s, rule.club)) throw std::runtime_error("Invalid forced-result club");
            output << s.memory.read<4>(rule.club) << ' ' << unsigned(rule.result) << ' '
                   << std::quoted(club_key(s, rule.club)) << '\n';
        }
        output.close();
        std::filesystem::rename(temporary, target);
    } catch (...) {
        std::error_code ignored;
        std::filesystem::remove(temporary, ignored);
        throw;
    }
}

void load_debug_club_results(State& s, const std::filesystem::path& save) {
    clear_match_result_debugging(s);
    const auto target = companion(save);
    if (!std::filesystem::exists(target)) return;
    if (std::filesystem::file_size(target) > 16 * 1024 * 1024)
        throw std::runtime_error("Oversized forced-result companion file");
    std::ifstream input(target, std::ios::binary);
    std::string magic;
    unsigned version = 0, count = 0;
    const auto toc = std::uint32_t(s.r<4>(8));
    const auto base = s.memory.read<4>(s.memory.read<4>(toc - 0x7be8));
    const auto clubs = s.memory.read<4>(s.memory.read<4>(toc - 0x7b50));
    if (!(input >> magic >> version >> count) || magic != "CM_FORCED_CLUB_RESULTS" || version != 1 || count > clubs)
        throw std::runtime_error("Invalid forced-result companion header");
    std::vector<ClubResultRule> rules;
    std::set<unsigned> seen;
    for (unsigned i = 0; i < count; ++i) {
        unsigned id = 0, result = 0;
        std::string name;
        if (!(input >> id >> result >> std::quoted(name)) || id >= clubs || result > unsigned(ForcedClubResult::loss) ||
            !seen.insert(id).second)
            throw std::runtime_error("Invalid forced-result companion entry");
        const auto club = std::uint32_t(base + id * 0x248);
        if (!valid_debug_result_club(s, club) || club_key(s, club) != name)
            throw std::runtime_error("Forced-result club does not match this career");
        rules.push_back({club, ForcedClubResult(result)});
    }
    input >> std::ws;
    if (!input.eof()) throw std::runtime_error("Trailing forced-result companion data");
    if (!rules.empty()) match_result_debugging(s).clubs = std::move(rules);
}
}
