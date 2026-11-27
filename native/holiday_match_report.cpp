#include "holiday_match_report.hpp"
#include "holiday_digest.hpp"
#include "holiday_return.hpp"
#include "manager_manager_update_detail.hpp"
#include <array>
#include <format>

namespace cm::aot::holiday_match_report {
namespace {
using guest::Cpu;
using namespace manager_manager::update_detail;

std::string text(const Cpu& c, std::uint32_t address, unsigned capacity) {
    std::string name;
    if (address) for (unsigned i = 0; i < capacity; ++i) {
        const char ch = char(c.u8(address + i));
        if (!ch) break;
        name.push_back(ch);
    }
    return name;
}

std::string club_name(const Cpu& c, std::uint32_t club) {
    auto name = text(c, club + 0x221, 26);
    return name.empty() ? text(c, club + 0x1ee, 51) : name;
}

std::string player_name(const Cpu& c, std::uint32_t id) {
    const auto people = c.u32(c.toc(-0x7be0));
    if (!people || id >= c.u32(c.toc(-0x7be4))) return {};
    const auto staff = people + id * person::size;
    auto common = text(c, c.u32(staff + 0x30), 104);
    if (!common.empty()) return common;
    auto first = text(c, c.u32(staff + 0x28), 104);
    auto last = text(c, c.u32(staff + 0x2c), 104);
    return first.empty() ? last : last.empty() ? first : first + " " + last;
}

std::string scorers(const Cpu& c, std::uint32_t report, unsigned side, int goals, bool pt) {
    // 0x1029f920 reduces the commentary to 50 sorted incidents. It stores the actor's
    // side even for own goals. Kinds 4..8 (misses, cards, shoot-out kicks) do not count.
    std::string list;
    unsigned named = 0;
    for (unsigned i = 0; report && i < 50 && int(named) < goals; ++i) {
        const auto incident = report + 0x55 + 4 * i;
        const unsigned actor_side = c.u8(incident), slot = c.u8(incident + 1), kind = c.u8(incident + 2);
        if (actor_side == 255) break;
        if (actor_side > 1 || slot >= match::slot_count || kind < 1 || kind > 3) continue;
        const unsigned credited = kind == 2 ? 1 - actor_side : actor_side;
        if (credited != side) continue;
        const auto lineup = report + match::person + actor_side * match::side_size + slot * match::slot_size;
        const auto name = player_name(c, c.u32(lineup));
        if (name.empty()) continue;
        list += list.empty() ? " - " : ", ";
        list += std::format("{} {}'", name, c.u8(incident + 3));
        if (kind == 2) list += pt ? " (contra)" : " (OG)";
        if (kind == 3) list += pt ? " (p\352n.)" : " (pen.)";
        ++named;
    }
    if (int(named) < goals)
        list += pt ? " - autoria incompleta no relato" : " - scorer details incomplete";
    return list;
}
}

void filed(Cpu& c, std::uint32_t at, std::uint32_t report) {
    if (!at || !c.s.platform) return;
    const std::array clubs{c.u32(at + fixture::home), c.u32(at + fixture::away)};
    if (!clubs[0] || !clubs[1]) return;
    // Each fixture side has its own first-team flag. The club ID is also used
    // by its reserves, including in friendlies against a first team.
    const std::array tracked{
        c.u8(at + 0x40) != 0 && holiday_digest::tracking(c.s, clubs[0]),
        c.u8(at + 0x41) != 0 && holiday_digest::tracking(c.s, clubs[1])};
    if (!tracked[0] && !tracked[1]) return;

    // Match engine 0x101dd4d0 files extra-time goals at +0x47 and shoot-out kicks
    // at +0x49. +0x4b is the aggregate across legs and does not belong to this match.
    const bool extra = c.i8(at + 0x47) >= 0;
    const auto score = at + (extra ? 0x47u : 0x43u);
    const std::array goals{int(c.i8(score)), int(c.i8(score + 1))};
    if (goals[0] < 0 || goals[1] < 0) return;
    const bool pt = c.u8(c.r(2) - 0x35ac) == 9;
    const HolidayDate date{c.i16(at + 0x2a), c.u16(at + 0x28)};
    const unsigned month = holiday_return::month_index(date);
    const int day = date.day - holiday_return::first_of_month(month, date.year).day + 1;
    const auto competition = c.u32(at + fixture::competition);
    auto reason = competition ? text(c, competition + 0x1a, 51) : std::string{};
    if (competition && reason.empty()) reason = text(c, competition + 0x4d, 26);
    if (reason.empty()) reason = pt ? "Amistoso" : "Friendly";
    auto line = std::format("{:02}/{:02}/{} - {}: {} {}:{} {}", day, month + 1, date.year, reason,
                            club_name(c, clubs[0]), goals[0], goals[1], club_name(c, clubs[1]));
    if (extra) line += pt ? " (prorr.)" : " (a.e.t.)";
    if (c.i8(at + 0x49) >= 0 && c.i8(at + 0x4a) >= 0)
        line += std::format(" ({} {}:{})", pt ? "p\352n." : "pens.", c.u8(at + 0x49), c.u8(at + 0x4a));
    for (unsigned side = 0; side < clubs.size(); ++side)
        if (tracked[side]) holiday_digest::record_event(c.s, clubs[side], "results", line + scorers(c, report, side, goals[side], pt));
}
}
