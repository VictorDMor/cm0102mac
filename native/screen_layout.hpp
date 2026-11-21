#pragma once
#include "presentation.hpp"
#include <algorithm>
#include <cstdint>

namespace cm::aot {
// Logical coordinates shared by drawing, controls and their input bounds.
// Styles, text and actions remain owned by the original screen builders.
struct ScreenRect {
    int left, top, right, bottom;
    constexpr int width() const { return right - left; }
    constexpr int height() const { return bottom - top; }
    bool operator==(const ScreenRect&) const = default;
};
enum class LayoutAxis { fixed, stretch, trailing, centered };
enum class ScreenTemplate { legacy, startup, league_selection, load_browser, tactics, club_tables, competition, competition_history, match_report, daily_fixtures, nations_and_clubs, player_details, search_tables, search_filters, transfer_history, transfer_proposal, contract_negotiation, loan_terms, negotiation_confirmation, news_inbox, training, training_coaches, training_schedule };
constexpr ScreenTemplate screen_template(std::uint32_t builder) {
    switch (builder) {
    case 0x10334550: return ScreenTemplate::startup;
    case 0x10335500: return ScreenTemplate::league_selection;
    case 0x100ee220: return ScreenTemplate::load_browser;
    case 0x103b3c50: case 0x103baec0: return ScreenTemplate::tactics;
    case 0x1001e2b0: return ScreenTemplate::club_tables;
    case 0x1006ff50: return ScreenTemplate::competition_history;
    case 0x10065340: return ScreenTemplate::competition;
    case 0x1028c430: case 0x1028f280: return ScreenTemplate::match_report;
    case 0x1028ae80: return ScreenTemplate::daily_fixtures;
    case 0x103dc920: return ScreenTemplate::training_schedule;
    case 0x103ddd10: return ScreenTemplate::training_coaches;
    case 0x103d6db0: return ScreenTemplate::training;
    case 0x10103850: return ScreenTemplate::nations_and_clubs;
    case 0x10389ce0: return ScreenTemplate::player_details;
    case 0x1031af20: return ScreenTemplate::search_tables;
    case 0x10329fd0: return ScreenTemplate::search_filters;
    case 0x10426aa0: return ScreenTemplate::transfer_history;
    case 0x10417b60: return ScreenTemplate::transfer_proposal;
    case 0x10096c60: return ScreenTemplate::contract_negotiation;
    case 0x1042ca40: return ScreenTemplate::loan_terms;
    case 0x1041eb10: case 0x1009dc00: case 0x1042efa0: return ScreenTemplate::negotiation_confirmation;
    // Message screens of the same shape (title, one wrapped message at 110,140-780,545, a footer of
    // buttons): an offer for one of the manager's players from the news, a completed transfer, a
    // transfer-list confirmation, an offer's restriction and feedback messages, and a staff release.
    case 0x10421fb0: case 0x10423270: case 0x1042a810: case 0x10420b50: case 0x10421070:
    case 0x1009e8f0: return ScreenTemplate::negotiation_confirmation;
    case 0x102cea30: return ScreenTemplate::news_inbox;
    default: return ScreenTemplate::legacy;
    }
}

struct ScreenLayout {
    GuiCanvas canvas;

    // Adapt a declared legacy rectangle. This is opt-in per component: the
    // framework never guesses a widget's purpose from its coordinates.
    constexpr ScreenRect anchor(ScreenRect original, LayoutAxis horizontal, LayoutAxis vertical) const {
        const auto adapt = [](int& first, int& last, int extra, LayoutAxis axis) {
            switch (axis) {
            case LayoutAxis::fixed: break;
            case LayoutAxis::stretch: last += extra; break;
            case LayoutAxis::trailing: first += extra; last += extra; break;
            case LayoutAxis::centered: first += extra / 2; last += extra / 2; break;
            }
        };
        adapt(original.left, original.right, canvas.width - 800, horizontal);
        adapt(original.top, original.bottom, canvas.height - 600, vertical);
        return original;
    }
    constexpr ScreenRect title() const { return anchor({100, 10, 790, 70}, LayoutAxis::stretch, LayoutAxis::fixed); }
    constexpr ScreenRect subtitle() const { return anchor({100, 80, 790, 125}, LayoutAxis::stretch, LayoutAxis::fixed); }
    constexpr ScreenRect content(int top = 145) const {
        return anchor({110, top, 780, 545}, LayoutAxis::stretch, LayoutAxis::stretch);
    }
    constexpr ScreenRect footer() const {
        return anchor({100, 555, 790, 590}, LayoutAxis::stretch, LayoutAxis::trailing);
    }
    ScreenRect buttons(int width = 300) const {
        if (canvas.original()) return footer();
        const auto bottom = footer();
        return {content().right - width, bottom.top, content().right, bottom.bottom};
    }
    // A proportional panel docked to the content's right edge. Extra width
    // remains with the leading list rather than enlarging every control.
    ScreenRect proportional_right(int top, int aspect_width, int aspect_height,
                                  int minimum_width, int minimum_leading, int percent, int gap = 10) const {
        const auto area = content(top);
        const auto width = std::min({area.width() - gap - minimum_leading,
                                    area.height() * aspect_width / aspect_height,
                                    std::max(minimum_width, area.width() * percent / 100)});
        return {area.right - width, area.top, area.right, area.top + width * aspect_height / aspect_width};
    }
};
// Details pages keep their original labels, values and number of rows. Share
// extra height between stacked panels by row count, so attributes and summary
// tables gain the same spacing while fixed-height metadata retains its size.
struct DetailsLayout {
    ScreenLayout screen;
    constexpr ScreenRect rows(ScreenRect original, int first, int count, int total) const {
        const int extra = screen.canvas.height - 600;
        auto result = screen.anchor(original, LayoutAxis::stretch, LayoutAxis::fixed);
        result.top += extra * first / total;
        result.bottom += extra * (first + count) / total;
        return result;
    }
    // Rich text owns separate rectangles for its label and each hyperlink.
    // Move them together without regenerating text or changing its wrapping.
    static constexpr ScreenRect centered_text(ScreenRect text, ScreenRect before, ScreenRect after) {
        const int shift = (after.left + after.right - before.left - before.right) / 2;
        text.left += shift; text.right += shift;
        return text;
    }
};
struct State;
// Apply a legacy-table recipe once, after its builder emitted all controls.
// Native layout consumers (tactics) do not need this second pass.
void layout_built_screen(State&, std::uint32_t display, std::uint32_t builder);
}
