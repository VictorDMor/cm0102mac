#include "holiday_status.hpp"
#include "calendar.hpp"
#include "discipline_bans_detail.hpp"
#include "injury_unit_detail.hpp"
#include "league_base_table_detail.hpp"
#include "news_detail.hpp"
#include <algorithm>
#include <array>
#include <memory>
#include <set>

namespace cm::aot::holiday_status {
namespace {
using guest::Cpu;
using Address = std::uint32_t;

std::string text(Cpu& c, Address at, unsigned limit) {
    std::string result;
    if (at) for (unsigned i = 0; i < limit; ++i) {
        const auto ch = c.u8(at + i);
        if (!ch) break;
        result += char(ch);
    }
    return result;
}

std::string name(Cpu& c, Address staff) {
    const auto common = text(c, c.u32(staff + 0x30), 104);
    if (!common.empty()) return common;
    const auto first = text(c, c.u32(staff + 0x28), 104);
    const auto last = text(c, c.u32(staff + 0x2c), 104);
    if (first.empty()) return last;
    return last.empty() ? first : first + " " + last;
}

calendar::Date date(Cpu& c, Address at) { return {c.i16(at), c.u16(at + 2)}; }

int days_injured(Cpu& c, Address medical, Address id, calendar::Date today) {
    using namespace injury_unit;
    if (!medical || !c.u32(medical + manager::status_rows) || !c.u32(medical + manager::injuries)) return 0;
    const auto row = c.u32(medical + manager::status_rows) + id * status::size;
    const auto index = c.i32(row + status::injury);
    if (index < 0 || index >= c.i32(medical + manager::injury_count)) return 0;
    const auto record = c.u32(medical + manager::injuries) + unsigned(index) * injury::size;
    // Types 77..80 represent medical treatment, including surgery. They can
    // still make the player unfit. Type 81 is a player on holiday.
    if (c.u8(record) >= 81) return 0;
    const auto elapsed = calendar::difference(today, date(c, record + injury::started));
    if (elapsed < 0) return 0;
    return std::max(0, int(c.i16(record + injury::days_unfit)) - elapsed);
}

bool suspended(Cpu& c, Address controller, Address id, calendar::Date today) {
    using namespace discipline;
    if (!controller || !c.u32(controller) || !c.u32(controller + manager::people) ||
        !c.u32(controller + manager_arrays::bans)) return false;
    const auto row = row_address(c, controller, id);
    // The fourth category ends at +0x20. The following words belong to history.
    for (unsigned category = 0; category < 4; ++category) {
        const auto slot = row + category * row::slot_size;
        const auto body = c.i8(slot);
        if (body < 0 || body >= manager::body_count) continue;
        const auto rules = ruling_body(c, controller, body);
        if (!rules || c.u8(rules + 4) != category || c.i8(rules + 5) != body) continue;
        const auto index = c.i32(slot + row::slot_value);
        const auto entries = c.u32(rules + 0xc);
        if (!entries || index < 0 || index >= c.i32(rules + 0x14)) continue;
        const auto entry = entries + unsigned(index) * 0x10;
        if (c.u32(entry) != id) continue;
        const auto ban_index = c.i16(entry + body_entry_index);
        if (ban_index < 0 || ban_index >= c.i16(controller + manager_arrays::ban_count)) continue;
        const auto record = ban_record(c, controller, ban_index);
        const auto elapsed = calendar::difference(today, date(c, record + ban::date_high));
        if (elapsed < 0) continue;
        const int matches = c.i8(record + ban::matches);
        if (matches == -1 ? elapsed < c.i16(record + ban::days) : matches > c.i8(record + ban::served))
            return true;
    }
    return false;
}

bool portuguese(Cpu& c) { return c.u8(c.r(2) - 0x35ac) == 9; }

// Cases that can return 2 in the game's news::action_label. An informational
// item cannot require a response even when flagged as important or unread.
constexpr std::array decision_types{6, 9, 22, 24, 25, 30, 34, 40, 3000, 3007, 3009, 3024, 3033, 3037, 3039,
                                    3046, 3048, 4000, 4002, 4004, 4005, 4011, 4012, 4020, 4021, 4028, 4032, 6004};

// The regular news label expires offers and advances contract list cursors.
// Evaluate it and render the headline in disposable guest memory so they can
// use their normal allocators, scratch buffers and any presentation RNG.
std::unique_ptr<State> inbox_view(const State& source) {
    auto view = std::make_unique<State>();
    view->registers = source.registers;
    view->allocated = source.allocated;
    view->available = source.available;
    view->heap_end = source.heap_end;
    view->memory_error = source.memory_error;
    view->platform = source.platform;
    view->budget = UINT64_MAX;
    for (const auto& [start, size] : source.memory.mapped_ranges()) {
        view->memory.map(start, size);
        constexpr std::uint64_t chunk = 1u << 20;
        for (std::uint64_t offset = 0; offset < size; offset += chunk) {
            const auto at = Address(start + offset);
            view->memory.copy_in(at, source.memory.copy_out(at, std::min(chunk, size - offset)));
        }
    }
    return view;
}
}

std::string league_position(Cpu& c, Address club) {
    if (!club) return {};
    namespace league = league_base::league;
    namespace row = league_base::table::row;
    namespace competition = comp::competition;
    const auto definition = c.u32(club + 0x30);
    const auto objects = c.u32(c.toc(comp::toc::competitions));
    if (!definition || !objects) return {};
    const auto id = c.u32(definition);
    if (id >= c.u32(c.toc(comp::toc::competition_count))) return {};
    const auto root = c.u32(objects + id * 4);
    if (!root || c.u32(root + competition::record) != definition) return {};
    const auto stages = c.u32(root + competition::stages);
    const int last = c.i32(root + competition::last_stage);
    // Original stage numbers are signed bytes, with -1 denoting the root.
    if (last > INT8_MAX || last < -1 || (last >= 0 && !stages)) return {};
    for (int stage = last; stage >= -1; --stage) {
        const auto object = stage < 0 ? root : c.u32(stages + unsigned(stage) * 4);
        if (!object || c.u8(object + competition::kind) != league::kind) continue;
        const auto rows = c.u32(object + league::standings);
        const int count = c.i16(object + competition::club_count);
        if (!rows) continue;
        for (int i = 0; i < count; ++i) {
            const auto at = rows + unsigned(i) * league::standing_row;
            if (c.u32(at + row::club) != club || !c.u8(at + row::totals + row::played)) continue;
            const int rank = c.i8(at + row::position);
            if (rank < 0 || rank >= count) break;
            auto league_name = text(c, definition + comp::record::name, 51);
            if (league_name.empty()) league_name = text(c, definition + comp::record::short_name, 26);
            const auto place = std::to_string(rank + 1);
            return league_name + (portuguese(c) ? " - " + place + "\272" : " - #" + place) +
                   " (" + std::to_string(c.i16(at + row::totals + row::points)) + " pts)";
        }
    }
    return {};
}

std::vector<std::string> absences(Cpu& c, Address club) {
    std::vector<std::string> lines;
    if (!club) return lines;
    const auto today = date(c, c.toc(-0x7b80));
    const auto medical = c.u32(c.toc(-0x7ba0)), discipline = c.u32(c.toc(-0x7aa4));
    const auto count = c.u32(c.toc(injury_unit::toc::people_count));
    const bool pt = portuguese(c);
    std::set<Address> visited;
    for (unsigned i = 0; i < 50; ++i) {
        const auto staff = c.u32(club + 0x98 + 4 * i);
        if (!staff || !c.u32(staff + 0x14) || !visited.insert(staff).second) continue;
        const auto id = c.u32(staff);
        if (id >= count) continue;
        const int days = days_injured(c, medical, id, today);
        const bool banned = suspended(c, discipline, id, today);
        if (!days && !banned) continue;
        auto line = name(c, staff);
        if (line.empty()) continue;
        line += " - ";
        if (days) {
            line += pt ? "lesionado (" : "injured (";
            line += std::to_string(days);
            line += pt ? (days == 1 ? " dia)" : " dias)") : (days == 1 ? " day)" : " days)");
        }
        if (banned) {
            if (days) line += ", ";
            line += pt ? "suspenso" : "suspended";
        }
        lines.push_back(std::move(line));
    }
    return lines;
}

std::vector<std::string> pending(Cpu& live, Address manager) {
    using namespace news::detail;
    std::vector<std::string> lines;
    const auto address = live.u32(live.toc(-0x7bf0));
    if (!address || !manager) return lines;
    Queue queue{live, address};
    const auto index = manager_index(live, manager);
    if (queue.manager_count().get() <= 0 || index >= unsigned(queue.manager_count().get()) ||
        !queue.managers().get() || !queue.items().get()) return lines;
    Inbox inbox{live, queue.managers().get() + index * Inbox::size};
    const int first = inbox.first().get(), next = inbox.next().get();
    if (first < 0 || first > queue.item_count().get() - int(Inbox::capacity) ||
        next < first || next - first >= int(Inbox::capacity)) return lines;
    std::vector<Address> candidates;
    for (unsigned i = 0; i < Inbox::capacity; ++i) {
        const auto row = (unsigned(next - first) + Inbox::capacity - i - 1) % Inbox::capacity;
        const auto item = queue.item(unsigned(first) + row);
        if (!live.u32(item + 0xdc)) break;
        if (std::ranges::binary_search(decision_types, live.i32(item))) candidates.push_back(item);
    }
    if (candidates.empty()) return lines;
    const auto view = inbox_view(live.s);
    Cpu c(*view);
    c.set(1, c.sp() - 0x1000);
    const auto reply = c.sp() + 0x100, callback = c.sp() + 0x80, rich = c.sp() + 0x90;
    Program game{c};
    game.call(0x1074b6e0, return_sentinel, {rich, 0});
    for (const auto item : candidates) {
        if (game.call(0x102c6b10, return_sentinel, {item, manager, reply, 2000, callback}) != 2) continue;
        game.call(0x102be040, return_sentinel, {item, manager, rich, 0, 1});
        auto headline = text(c, c.u32(rich), 2000);
        if (headline.empty()) headline = text(c, reply, 2000);
        if (!headline.empty()) lines.push_back(std::move(headline));
    }
    game.call(0x1074b7c0, return_sentinel, {rich, 0xffffffffu});
    return lines;
}
}
