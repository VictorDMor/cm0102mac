#include "holiday_digest.hpp"
#include "holiday_digest_store.hpp"
#include "holiday_article.hpp"
#include "holiday_status.hpp"
#include "gui_guest_scope.hpp"
#include "holiday_return.hpp"
#include "news_detail.hpp"
#include "platform.hpp"
#include <algorithm>
#include <format>
#include <set>

namespace cm::aot::holiday_digest {
namespace {
constexpr std::uint32_t no_club = 0xffffffff;

std::string date_text(State& s) {
    const auto date = holiday_return::today(s);
    const auto month = holiday_return::month_index(date);
    const auto day = date.day - holiday_return::first_of_month(month, date.year).day + 1;
    return std::format("{:02}/{:02}/{}", day, month + 1, date.year);
}

// The game's own circular queues retain the last 100 items per human. A published digest can
// go once every recipient's queue has evicted it; active holidays always keep their events.
void prune(guest::Cpu& c, Store& store) {
    const auto address = c.u32(c.toc(-0x7bf0));
    if (!address) return;
    const news::detail::Queue queue{c, address};
    std::set<std::uint32_t> retained;
    for (int i = 0; i < queue.item_count().get(); ++i) {
        const auto item = queue.item(i);
        if (!c.u32(item + 0xdc)) continue;
        if (const auto id = holiday_article::report_id(c, item)) retained.insert(id);
    }
    std::erase_if(store.reports, [&](const auto& entry) {
        return !entry.second.active && !retained.contains(entry.first);
    });
}

void section(std::vector<std::string>& lines, std::string heading, const std::vector<std::string>& entries,
             const char* empty) {
    lines.emplace_back();
    lines.push_back(std::move(heading));
    if (entries.empty()) lines.emplace_back(empty);
    else for (const auto& entry : entries) lines.push_back("- " + entry);
}
}

bool tracking(const State& s, std::uint32_t club) {
    if (!club || !s.platform) return false;
    const auto& store = s.platform->holiday_digests();
    if (store.active_ids.empty()) return false;
    const auto id = s.memory.read<4>(club);
    return std::ranges::any_of(store.active_ids, [&](auto report_id) {
        return store.reports.at(report_id).club == id;
    });
}

void record_event(State& s, std::uint32_t club, std::string_view key, std::string line) {
    if (!club || !s.platform) return;
    const auto id = s.memory.read<4>(club);
    auto& store = s.platform->holiday_digests();
    for (const auto report_id : store.active_ids) {
        auto& report = store.reports.at(report_id);
        if (report.club != id) continue;
        if (key == "results") report.results.push_back(line);
        else if (key == "incoming") report.incoming.push_back(line);
        else if (key == "outgoing") report.outgoing.push_back(line);
    }
}

void transition(guest::Cpu& c, std::uint32_t manager, bool away) {
    if (!c.s.platform || c.s.platform->career_loading()) return;
    GuiGuestScope preserve(c.s);
    c.set(1, c.sp() - 0x400);
    auto& store = c.s.platform->holiday_digests();
    prune(c, store);
    const auto manager_id = c.u32(manager);
    auto active = std::ranges::find_if(store.reports, [&](const auto& entry) {
        return entry.second.active && entry.second.manager == manager_id;
    });
    const auto club = c.u32(manager + 0x24) ? c.u32(manager + 0x24) : c.u32(manager + 0x20);
    const bool pt = c.u8(c.r(2) - 0x35ac) == 9;
    if (away || active == store.reports.end()) {
        if (active != store.reports.end()) return;
        // Also give a manager already on holiday in an older save a report on return.
        while (store.next_id == 0 || store.reports.contains(store.next_id)) ++store.next_id;
        const auto id = store.next_id++;
        auto& report = store.reports[id];
        report.id = id;
        report.manager = manager_id;
        report.club = club ? c.u32(club) : no_club;
        report.active = true;
        store.active_ids.push_back(id);
        if (away) {
            report.starting_date = date_text(c.s);
            if (club) report.starting_position = holiday_status::league_position(c, club);
            return;
        }
        active = store.reports.find(id);
    }

    auto& report = active->second;
    auto& lines = report.lines;
    const auto date = date_text(c.s);
    lines.push_back(report.starting_date.empty() ? date : report.starting_date + " - " + date);
    const char* empty = pt ? "Nenhum registro." : "None recorded.";
    if (report.starting_date.empty())
        lines.emplace_back(pt ? "Registro de eventos indispon\355vel para estas f\351rias." :
                                "Event history is unavailable for this holiday.");
    section(lines, pt ? "Resultados" : "Results", report.results, empty);
    section(lines, pt ? "Entradas" : "Arrivals", report.incoming, empty);
    section(lines, pt ? "Sa\355das" : "Departures", report.outgoing, empty);
    if (club) {
        const auto position = holiday_status::league_position(c, club);
        std::vector<std::string> positions;
        if (!report.starting_position.empty())
            positions.push_back((pt ? "Antes: " : "Before: ") + report.starting_position);
        if (!position.empty()) positions.push_back((pt ? "Agora: " : "Now: ") + position);
        section(lines, pt ? "Classifica\347\343o" : "League position", positions, empty);
        section(lines, pt ? "Desfalques no retorno" : "Unavailable on return", holiday_status::absences(c, club), empty);
    }
    section(lines, pt ? "Decis\365es pendentes" : "Pending decisions", holiday_status::pending(c, manager), empty);
    report.active = false;
    std::erase(store.active_ids, report.id);
    report.results.clear();
    report.incoming.clear();
    report.outgoing.clear();
    holiday_article::publish(c, manager, report.id);
}

std::vector<std::string> article_lines(const State& s, std::uint32_t id) {
    if (s.platform) {
        const auto& reports = s.platform->holiday_digests().reports;
        if (const auto it = reports.find(id); it != reports.end()) return it->second.lines;
    }
    return {};
}
}
