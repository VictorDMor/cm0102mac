#include "national_callup.hpp"
#include "aot_runtime.hpp"
#include "calendar.hpp"
#include <algorithm>
#include <optional>
#include <vector>

namespace cm::aot {
namespace {
std::uint32_t word(const State& s, std::uint32_t at) { return std::uint32_t(s.memory.read<4>(at)); }
std::uint32_t global(const State& s, unsigned offset) { return word(s, word(s, std::uint32_t(s.r<4>(8)) - offset)); }
struct NationalSquad { std::uint32_t nation; unsigned kind; };
std::optional<NationalSquad> national_squad(const State& s, std::uint32_t club) {
    if (!club) return {};
    // DATABASE 100c76c0 / 100c7860: the last two nation-count blocks
    // of the club table are senior and under-21 national teams, respectively.
    const auto clubs = global(s, 0x7be8), count = global(s, 0x7b50), nations = global(s, 0x7b04);
    if (!clubs || !nations || nations > count / 2 || club < clubs || (club - clubs) % 0x248) return {};
    const auto id = (club - clubs) / 0x248;
    if (id < count - 2 * nations || id >= count || word(s, club) != id) return {};
    const auto nation = word(s, club + 0x2c);
    const unsigned kind = id >= count - nations ? 2 : 0;
    if (!nation || word(s, nation + 0x68) != id - (count - (kind == 2 ? nations : 2 * nations))) return {};
    return NationalSquad{nation, kind};
}

// A toolbar action is inserted between guest operations. Its private call frame
// and register restoration follow the existing Lineup actions' convention.
class Calls {
    State& s;
    decltype(State::registers) registers;
    decltype(State::unique) unique;
    std::uint32_t pc;
public:
    explicit Calls(State& state) : s(state), registers(s.registers), unique(s.unique), pc(s.pc) {
        const auto sp = std::uint32_t(s.r<4>(4));
        if (sp < 0x800) throw std::runtime_error("Invalid call-up stack");
        (void)s.memory.copy_out(sp - 0x400, 0x400);
        s.wr<4>(4, sp - 0x400);
    }
    ~Calls() { s.registers = registers; s.unique = unique; s.pc = pc; }
    std::uint32_t operator()(std::uint32_t entry, std::initializer_list<std::uint32_t> args) {
        unsigned reg = 3;
        for (auto value : args) s.wr<4>(reg++ * 4, value);
        s.wr<4>(4128, return_sentinel); s.pc = entry;
        run_until(s, return_sentinel);
        return std::uint32_t(s.r<4>(12));
    }
};
struct Candidate { std::uint32_t staff, id; int ability; };
unsigned squad_kind(const State& s, NationalSquad team, std::uint32_t staff) {
    return team.kind == 2 ? 2u : (s.memory.read<1>(staff + 0x6a) & 4) ? 0u : 1u;
}

bool fit_for_call_up(const State& s, std::uint32_t medical, std::uint32_t id) {
    const auto index = std::int32_t(word(s, word(s, medical) + id * 0x24 + 0x10));
    // The daily updater clears a stale index before the original status tail.
    if (index < 0 || index > std::int32_t(word(s, medical + 8))) return true;
    const auto injury = word(s, medical + 4) + std::uint32_t(index) * 0x38;
    // Read the status tail of 10187680 without its 10187a40 daily-update
    // side effect: choosing a squad must not advance injuries or draw RNG.
    const auto date = [&](std::uint32_t at) {
        return calendar::Date{std::int16_t(s.memory.read<2>(at)), std::uint16_t(s.memory.read<2>(at + 2))};
    };
    auto end = date(word(s, std::uint32_t(s.r<4>(8)) - 0x7b80)), start = date(injury + 0x28);
    // 10717dd0 returns a positive interval in either order. Borrow the end
    // year's length, then apply the status operation's 365*years + days.
    // Compute locally so even the original date service's scratch is untouched.
    if (calendar::is_before(end, start)) std::swap(end, start);
    auto days = int(end.day) - int(start.day);
    auto years = int(end.year) - int(start.year);
    if (days < 0) { days += calendar::year_length(end.year); --years; }
    const auto elapsed = int(std::uint16_t(years)) * 365 + std::int16_t(days);
    return elapsed >= std::int16_t(s.memory.read<2>(injury + 0x30)) &&
           elapsed >= std::int16_t(s.memory.read<2>(injury + 0x32));
}
}

bool is_national_squad(const State& s, std::uint32_t club) { return national_squad(s, club).has_value(); }

bool call_up_national_squad(State& s, std::uint32_t club, unsigned side) {
    auto team = national_squad(s, club);
    if (!team || side > 1) return false;
    if (team->kind == 0) team->kind = side;
    Calls call(s);
    const auto people = global(s, 0x7be0), count = global(s, 0x7be4);
    const auto histories = global(s, 0x7b9c), medical = global(s, 0x7ba0);
    const auto national_teams = global(s, 0x7a64);
    if (!people || !histories || !medical) return false;
    const auto special_nation = global(s, 0x79fc);
    std::vector<Candidate> candidates;
    for (std::uint32_t id = 0; id < count; ++id) {
        const auto staff = people + id * 0x70, player = word(s, staff + 0x14);
        if (!player) continue;
        const auto primary = word(s, staff + 0xc), secondary = word(s, staff + 0x10);
        if (!primary) continue;
        if (primary != team->nation && secondary != team->nation) continue;
        if (!call(0x100c7a20, {std::uint32_t(s.memory.read<1>(staff + 0x62))})) continue;
        const auto previous = word(s, staff + 0x20);
        if (previous && (word(s, previous + 0x2c) != team->nation || s.memory.read<1>(staff + 0x60) != 11)) continue;
        if (previous && !national_squad(s, previous)) continue;
        // NATIONAL_TEAMS 102aee30: caps and the international commitment bit
        // prevent changing country through a second nationality.
        if (primary != team->nation && (s.memory.read<1>(staff + 0x5e) ||
            (s.memory.read<1>(histories + id * 0x64 + 0xd) & 0x40))) continue;
        if (word(s, team->nation + 0x68) == special_nation && secondary &&
            word(s, secondary + 0x68) == special_nation) continue;
        if (team->kind == 2) {
            // Same age gate as the original manual eligibility check (102aeea4).
            if (std::int8_t(s.memory.read<2>(staff + 0x5c)) > 21) continue;
        }
        // Fitness/retirement plus the original manual refusal check (102af1d4):
        // 102b8e60 checks the senior team of his primary nationality.
        // Free agents remain eligible, as in the manual call-up flow.
        if (!fit_for_call_up(s, medical, id) || call(0x102de3c0, {0, staff}) ||
            call(0x102b8e60, {0, staff})) continue;
        if (previous && (previous != club || squad_kind(s, *national_squad(s, previous), staff) != team->kind)) {
            const auto old = *national_squad(s, previous);
            // Moving between squads also needs the manual release permission
            // for the previous squad (102bc640..102bc664). Do not pull players
            // out of a sibling team's locked tournament list.
            const auto kind = squad_kind(s, old, staff);
            if (!national_teams || !call(0x102af090, {national_teams, old.nation, kind, staff, 0, 0, 0})) continue;
        }
        candidates.push_back({staff, id, std::int16_t(s.memory.read<2>(player + 4))});
    }
    std::ranges::sort(candidates, [](const auto& a, const auto& b) {
        return a.ability != b.ability ? a.ability > b.ability : a.id < b.id;
    });
    if (candidates.size() > 23) candidates.resize(23);
    const auto selected = [&](std::uint32_t staff) {
        return std::ranges::any_of(candidates, [=](const auto& p) { return p.staff == staff; });
    };
    std::vector<std::uint32_t> retained;
    for (unsigned slot = 0; slot < 50; ++slot) {
        const auto staff = word(s, club + 0x98 + slot * 4);
        if (!staff) continue;
        // Senior and reserve national sides share one 50-slot club roster.
        // Leave the other side's players alone unless selected for promotion.
        if (squad_kind(s, *team, staff) != team->kind) continue;
        if (selected(staff)) retained.push_back(staff);
        // The original release clears both tactical squads, captaincies and
        // set pieces, the roster slot, national employer, job and call-up bits.
        else call(0x102b8d60, {0, team->nation, team->kind, staff});
    }
    for (const auto& candidate : candidates) {
        if (std::ranges::find(retained, candidate.staff) != retained.end()) continue;
        if (const auto previous = word(s, candidate.staff + 0x20)) {
            const auto old = national_squad(s, previous);
            if (!old) throw std::runtime_error("Invalid previous national squad");
            call(0x102b8d60, {0, old->nation, squad_kind(s, *old, candidate.staff), candidate.staff});
        }
        // Original manual call-up (102bcdfc), including date, employer and flags.
        call(0x102b8c50, {0, team->nation, team->kind, candidate.staff});
    }
    return true;
}
}
