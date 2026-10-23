#include "club_control.hpp"
#include "aot_runtime.hpp"

namespace cm::aot::club_control {
namespace {
// TOC slots (offsets below r2) of the globals these routines read.
namespace toc {
constexpr std::uint32_t staff_count = 0x7be4;     // -> int: number of staff
constexpr std::uint32_t club_count = 0x7b50;      // -> int: number of clubs
constexpr std::uint32_t nation_count = 0x7b04;    // -> int: number of nations
constexpr std::uint32_t clubs = 0x7be8;           // -> club array (0x248-byte records)
constexpr std::uint32_t nations_ready = 0x7b98;   // -> word, nonzero once nations are loaded
constexpr std::uint32_t humans = 0x7a54;          // -> human manager table
}
// Clubs: +0 id, +0x2c nation, +0x90 manager. Staff: +0 id, +0x20 club job.
// Nations: +0x68 index. Human manager table: +0 settings array, +4 count;
// each manager's settings take 0xa65c bytes.
constexpr std::uint32_t human_settings = 0xa65c;
constexpr unsigned human_staff_slots = 16;

class Reader {
public:
    explicit Reader(const State& state) : s(state), r2(std::uint32_t(state.r<4>(8))) {}
    std::uint32_t word(std::uint32_t address) const { return std::uint32_t(s.memory.read<4>(address)); }
    std::int32_t integer(std::uint32_t address) const { return std::int32_t(word(address)); }
    std::uint32_t slot(std::uint32_t offset) const { return word(r2 - offset); }

    // 0x100c7d00 / 0x100ce250: the club is among the last 2 x nations clubs
    // (national teams), or among the last nations clubs.
    std::optional<bool> national_team(std::uint32_t club, unsigned per_nation) const {
        if (!club) return std::nullopt;
        const auto nations = integer(slot(toc::nation_count));
        const auto id = integer(club), count = integer(slot(toc::club_count));
        return id >= std::int32_t(std::uint32_t(count) - std::uint32_t(nations) * per_nation);
    }
    // 0x100c76c0: the team of a nation (the club list ends with them).
    std::optional<std::uint32_t> nation_team(std::uint32_t nation) const {
        if (!nation) return std::nullopt;
        const auto clubs = slot(toc::clubs);
        if (!word(clubs) || !word(slot(toc::nations_ready))) return std::nullopt;
        const auto nations = word(slot(toc::nation_count)), count = word(slot(toc::club_count));
        const auto index = word(nation + 0x68) + (count - nations * 2);
        return word(clubs) + index * 0x248;
    }
    // 0x10149170, 0x10147e70, 0x101481d0, 0x101484b0 (r5 null): one option
    // byte of a human manager (null: the current human).
    // (A null manager means the current human there; 0x1014c2e0 never passes one.)
    std::optional<std::int8_t> option(std::uint32_t table, std::uint32_t manager, std::int32_t offset,
                                      bool staffed_club_check) const {
        const auto index = std::int32_t(std::uint32_t(integer(manager)) - std::uint32_t(integer(slot(toc::staff_count)))) +
                           std::int32_t(human_staff_slots);
        if (index < 0 || index > integer(table + 4) - 1) return std::nullopt;
        if (staffed_club_check) {
            if (const auto job = word(manager + 0x20)) {
                if (integer(word(job + 0x2c) + 0x6c) < 0x190) return std::int8_t(0);
            }
        }
        const auto settings = word(table) + 0x10000 + std::uint32_t(index) * human_settings;
        return std::int8_t(s.memory.read<1>(settings + std::uint32_t(offset)));
    }

    // Human managers hold the last 16 staff ids.
    bool human(std::uint32_t manager) const {
        return !(integer(manager) < std::int32_t(std::uint32_t(integer(slot(toc::staff_count))) - human_staff_slots));
    }
    std::optional<bool> controlled(std::uint32_t club, std::uint32_t r4, std::uint32_t r5, std::uint32_t r6,
                                   std::uint32_t r7) const {
        if (!club) return std::nullopt;
        auto manager = r6 ? r6 : word(club + 0x90);
        const auto own = word(club + 0x90);
        if (own && manager == own && human(manager)) {
            const auto table = slot(toc::humans);
            bool vetoed = false;
            if (!std::int8_t(r4)) {
                for (const auto offset : {-0x659b, -0x667c}) {
                    const auto value = option(word(table), manager, offset, false);
                    if (!value) return std::nullopt;
                    if (*value) { vetoed = true; break; }
                }
            }
            if (!vetoed) {
                const auto national = national_team(club, 2);
                if (!national) return std::nullopt;
                if (std::int8_t(r7) || std::int8_t(r5)) return true;
                const auto value = *national ? option(word(table), manager, -0x667a, true)
                                             : option(word(table), manager, -0x667b, false);
                if (!value) return std::nullopt;
                if (*value) return true;
            }
        }
        const auto deferred = national_team(club, 1);
        if (!deferred) return std::nullopt;
        if (!*deferred) return false;
        const auto team = nation_team(word(club + 0x2c));
        if (!team) return std::nullopt;
        return controlled(*team, r4, 0, manager, r7);
    }

private:
    const State& s;
    std::uint32_t r2;
};
}

std::optional<bool> human_controlled(const State& s, std::uint32_t club, std::uint32_t r4, std::uint32_t r5,
                                     std::uint32_t r6, std::uint32_t r7) {
    return Reader(s).controlled(club, r4, r5, r6, r7);
}
bool plainly_uncontrolled(const State& s, std::uint32_t club) {
    const Reader reader(s);
    if (!club) return false;
    if (const auto manager = reader.word(club + 0x90); manager && reader.human(manager)) return false;
    return !*reader.national_team(club, 1);
}
std::uint32_t human_table_slot(const State& s) { return Reader(s).slot(toc::humans); }
}
