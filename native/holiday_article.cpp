#include "holiday_article.hpp"
#include "gui_guest_scope.hpp"
#include "holiday_digest.hpp"
#include "news_screens_detail.hpp"
#include <algorithm>
#include <limits>
#include <string>
#include <string_view>
#include <vector>

namespace cm::aot::holiday_article {
namespace {
using namespace news_screens::detail;
constexpr std::uint32_t back = 0xfffffff0;
// Type 5000 is the original direct message. Its private flag is a full word but the
// game reads its low byte. Normal senders write only 0 or 1. Its NUL-terminated text
// starts at +0x14; a game that lacks our report reader can still display the title.
constexpr std::uint32_t manager_message = 5000, holiday_private = 0x484f4c01;
constexpr unsigned history_report = 33, history_page = 34, history_count = 35;
constexpr int previous = 30511, next = 30512;

void write_text(Cpu& c, std::uint32_t destination, std::string_view text) {
    c.s.memory.copy_in(destination, {text.begin(), text.end()});
    c.put8(destination + static_cast<unsigned>(text.size()), 0);
}

std::string_view headline(Cpu& c) {
    return c.u8(c.r(2) - 0x35ac) == 9 ? "Resumo das f\351rias" : "Holiday summary";
}

std::vector<std::string> lines_for(Cpu& c, std::uint32_t id) {
    auto lines = holiday_digest::article_lines(c.s, id);
    if (lines.empty()) lines.emplace_back(c.u8(c.r(2) - 0x35ac) == 9
        ? "Detalhes indispon\355veis para estas f\351rias." : "Details are unavailable for this holiday.");
    return lines;
}

// The standard rich-text object owns a guest allocation at +0. Populate it directly
// after clear_text so inbox filtering can search an arbitrarily long saved report.
bool assign_cleared(Cpu& c, std::uint32_t output, const std::string& text) {
    if (text.size() >= std::numeric_limits<std::uint32_t>::max()) return false;
    const auto block = bl(c, news_screens::detail::allocate, back, {static_cast<std::uint32_t>(text.size()) + 1});
    if (!block) return false;
    write_text(c, block, text);
    c.put32(output, block);
    return true;
}

// draw_text checks its 50-line/500-byte scratch buffers at 0x1074bdd4. Measure each
// physical row with the same game font as the news screen, preserving every byte.
// Each row then passes to a GUI control whose inline caption holds 256 bytes
// (+0x78..+0x177). Leave room for the space draw_text appends at a newline and NUL.
std::vector<std::string> pages(const std::vector<std::string>& lines, const auto& fits) {
    constexpr unsigned rows_per_page = 48, bytes_per_row = 254;
    std::vector<std::string> result;
    unsigned rows = 0;
    for (std::string_view line : lines) {
        do {
            if (rows % rows_per_page == 0) result.emplace_back();
            else result.back().push_back('\n');
            std::size_t length = std::min<std::size_t>(line.size(), bytes_per_row);
            if (length && !fits(line.substr(0, length))) {
                std::size_t first = 1, last = length;
                while (first < last) {
                    const auto middle = (first + last + 1) / 2;
                    if (fits(line.substr(0, middle))) first = middle;
                    else last = middle - 1;
                }
                length = first;
            }
            if (length < line.size()) {
                const auto space = line.substr(0, length).find_last_of(' ');
                if (space != std::string_view::npos) length = space + 1;
            }
            result.back().append(line.substr(0, length));
            ++rows;
            line.remove_prefix(length);
        } while (!line.empty());
    }
    return result;
}

void pager_box(Cpu& c, std::uint32_t screens, unsigned left, unsigned right, unsigned top,
               std::string_view caption, int action) {
    const auto label = c.sp() + 0x80;
    write_text(c, label, caption);
    c.put32(c.sp() + 0x38, 1);
    c.put32(c.sp() + 0x3c, halfword(c, slot::ink));
    c.put32(c.sp() + 0x40, label);
    c.put32(c.sp() + 0x44, action);
    c.put32(c.sp() + 0x48, 0);
    bl(c, action ? add_labelled_box : add_box, back, {screens, left, top, right, top + 20,
       0x30, halfword(c, slot::button_font), action ? 0xcu : 0x2cu});
}
}

std::uint32_t report_id(guest::Cpu& c, std::uint32_t item) {
    return item && c.u32(item) == manager_message && c.u32(item + 0xc) == holiday_private
        ? c.u32(item + 0xcc) : 0;
}

bool publish(guest::Cpu& c, std::uint32_t manager, std::uint32_t id) {
    if (!manager || !id) return false;
    const auto queue = c.u32(c.toc(slot::news));
    if (!queue) return false;
    GuiGuestScope preserve(c.s);
    c.set(1, c.sp() - 0x200);
    const auto item = c.sp() + 0x70;
    bl(c, construct_item, back, {item, manager_message, 0});
    c.put32(item + 8, c.u32(manager));
    c.put32(item + 0xc, holiday_private);
    write_text(c, item + 0x14, headline(c));
    c.put32(item + 0xcc, id);
    return bl(c, publish_to_manager, back, {queue, item, manager}) != 0;
}

bool render(guest::Cpu& c, std::uint32_t item, std::uint32_t output, bool detail) {
    const auto id = report_id(c, item);
    if (!id) return false;
    if (!detail) {
        bl(c, set_text, back, {output, item + 0x14});
    } else {
        std::string body;
        const auto lines = lines_for(c, id);
        for (std::size_t i = 0; i < lines.size(); ++i) {
            if (i) body.push_back('\n');
            body += lines[i];
        }
        if (!assign_cleared(c, output, body)) bl(c, set_text, back, {output, item + 0x14});
    }
    return true;
}

bool prepare_page(guest::Cpu& c, std::uint32_t item, std::uint32_t output, std::uint32_t screens,
                  unsigned toolbar_y, unsigned text_width) {
    const auto id = report_id(c, item);
    if (!id) return false;
    GuiGuestScope preserve(c.s);
    c.set(1, c.sp() - 0x300);
    const History history{c, screens};
    const auto content = pages(lines_for(c, id), [&](std::string_view part) {
        const auto scratch = c.sp() + 0x80;
        write_text(c, scratch, part);
        return bl(c, 0x1076d1e0, back, {3, scratch}) <= text_width;
    });
    const auto count = static_cast<unsigned>(content.size());
    const auto page = history.value(history_report, back) == id
        ? std::min(history.value(history_page, back), count - 1) : 0;
    history.set(history_report, id, back);
    history.set(history_page, page, back);
    history.set(history_count, count, back);
    bl(c, clear_text, back, {output});
    if (!assign_cleared(c, output, content[page])) bl(c, set_text, back, {output, item + 0x14});
    if (count > 1) {
        pager_box(c, screens, 110, 135, toolbar_y, "<", page ? previous : 0);
        pager_box(c, screens, 136, 230, toolbar_y, std::to_string(page + 1) + " / " + std::to_string(count), 0);
        pager_box(c, screens, 231, 256, toolbar_y, ">", page + 1 < count ? next : 0);
    }
    return true;
}

bool change_page(guest::Cpu& c, std::uint32_t item, int action, std::uint32_t screens) {
    if (action != previous && action != next) return false;
    const auto id = report_id(c, item);
    if (!id) return false;
    GuiGuestScope preserve(c.s);
    c.set(1, c.sp() - 0x100);
    const History history{c, screens};
    if (history.value(history_report, back) != id) return false;
    const auto page = history.value(history_page, back);
    const auto count = history.value(history_count, back);
    if (action == previous && page > 0) history.set(history_page, page - 1, back);
    if (action == next && page < count && count - page > 1) history.set(history_page, page + 1, back);
    return true;
}
}
