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
enum class ScreenTemplate { legacy, startup, league_selection, load_browser, tactics, club_tables, competition, competition_history, match_report, daily_fixtures, nations_and_clubs, player_details, search_tables, search_filters, transfer_history, transfer_proposal, contract_negotiation, loan_terms, negotiation_confirmation, news_inbox, training, training_coaches, training_schedule, transfer_status, previous_offers, message_screen, board_confidence, manager_statistics, world_rankings, uefa_coefficients, hall_of_fame, manager_status, job_information, holiday, bid_rejection, friendly_countries, friendly_dates, credits, web_sites, text_entry, disciplinary, role_choice, head_to_head, paged_table, tabbed_page, centered_dialog, squad_numbers };
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
    // Message screens (title, one message at 110,140-780,545, a row of buttons that is a grid of one to
    // six columns or one standalone control): an offer for one of the manager's players from the news, a
    // completed transfer, a transfer-list confirmation, an offer's restriction and feedback messages, and a
    // staff release. They get a dialog layout of their own.
    case 0x10421fb0: case 0x10423270: case 0x1042a810: case 0x10420b50: case 0x10421070:
    case 0x1009e8f0: return ScreenTemplate::message_screen;
    // The manager's confirmations (Yes/No under one question): apply for a job, rescind the contract, retire.
    // They are message screens too, with their panel at 110,145-780,535.
    case 0x101c8730: case 0x101c8b60: case 0x101c98c0: return ScreenTemplate::message_screen;
    // FILE_SCREENS' generic message box (render 0x100f08c0: every opener of 0x100f05a0 and 0x100f0720, such as Exit the game,
    // Restart the game, a full disk or a failed save) and the confirmation before a match (render 0x1029ad50, opener 0x1029ab60,
    // "Proceed to the match ?"). Both draw the wrapped message in a one-column grid at 110,145-780,535 and a No/Yes table or
    // a standalone Ok at 100,555-790,590, which is the message screen's shape.
    case 0x100f08c0: case 0x1029ad50: return ScreenTemplate::message_screen;
    // The transfer dialogs of TRANSFER_SCREENS: remove the exchange player from a proposal, add a player to the
    // shortlist, set a player as the first or second to compare, withdraw a bid, release a player, end a loan
    // early (each a question with a No/Yes footer) and the trial period selection (one row of controls and a
    // Back/Offer footer). All have the message shape: one panel at 110,140-780,545 and a row of buttons.
    case 0x10422b50: case 0x10424f80: case 0x104249e0: case 0x104265c0: case 0x1042ae90: case 0x1042f7c0:
    case 0x1042b580: return ScreenTemplate::message_screen;
    // More confirmations with the message shape: have a player scouted (Yes/No), the "choose two players" notice
    // (one OK button) and entering the English place in the Intertoto Cup (a No/Yes footer at the manager's
    // confirmation rectangle, 110,145-780,535).
    case 0x103ac810: case 0x103acc50: case 0x10075410: return ScreenTemplate::message_screen;
    // Two more questions opened from the news: accepting a job offer from a club (No/Yes) and a player's
    // transfer request (a footer of three buttons, Back first). Both are panels at 110,145-780,535.
    case 0x101c94e0: case 0x10093bd0: return ScreenTemplate::message_screen;
    // The "invalid squad" notices of the national teams (opened from the news): a panel of wrapped lines at
    // 110,145-780,535 and one Ok button at 110,555-780,590.
    case 0x102bd090: case 0x102bd920: return ScreenTemplate::message_screen;
    // The national team squad screen (a player added to, or removed from, the international squad of the manager's
    // nation; the person's action menu opens it): a confirmation (No/Yes) or the "only N players may be selected" notice (Ok),
    // both in the manager's 110,145-780,535 panel.
    case 0x102bc400: return ScreenTemplate::message_screen;
    // The invitation to a friendly tournament (opened from the news): a question and three buttons, Back first.
    case 0x10049550: return ScreenTemplate::message_screen;
    // The game's "disk full" and "not enough memory" notices (two lines and one Ok), the question to take control of
    // (or reassume control of) another club, entering the FA Cup in a later round, a player's appeal against his
    // punishment and the ultimatum to the board (opened from the news or a menu, each a question and a No/Yes footer).
    case 0x10340db0: case 0x10341120: case 0x1033bdc0: case 0x1033c970: case 0x104c65a0: case 0x106fcbb0:
    case 0x10702ba0: return ScreenTemplate::message_screen;
    // The reasons for rejecting an offer (the offer decision's Reject, the proposal screen's rejection): choices
    // in a two-column panel and a footer of two or three buttons.
    case 0x1041f8f0: return ScreenTemplate::bid_rejection;
    // Player transfer status (asking price, squad status and listing choices) and the previous offers.
    case 0x10428ab0: return ScreenTemplate::transfer_status;
    case 0x10422600: return ScreenTemplate::previous_offers;
    // The Manager menu's Board Confidence and Manager Statistics, and the Competitions menu's FIFA World
    // Rankings: Back/Next screens with one content panel (a message or a ranked table) and a pager.
    case 0x101c8fb0: return ScreenTemplate::board_confidence;
    case 0x101c4460: return ScreenTemplate::manager_statistics;
    case 0x100745f0: return ScreenTemplate::world_rankings;
    // The Competitions menu's UEFA Coefficients and the Game menu's Hall of Fame: one table and the Back/Next footer.
    case 0x10074d80: return ScreenTemplate::uefa_coefficients;
    case 0x1033fcf0: return ScreenTemplate::hall_of_fame;
    // Manager Status (dispatcher action 0x3fc): a three-column table of the game's managers.
    case 0x10338b40: return ScreenTemplate::manager_status;
    // The Manager menu's Job Information: the jobs on offer in a nation, or a message when there are none.
    case 0x101c74e0: return ScreenTemplate::job_information;
    // The Manager menu's holiday dialog: its settings and a Yes/No footer.
    case 0x101c9eb0: return ScreenTemplate::holiday;
    // Arranging a friendly tour: the country table and the date table.
    case 0x10045930: return ScreenTemplate::friendly_countries;
    case 0x10043b00: return ScreenTemplate::friendly_dates;
    // The Game menu's Credits and Web Sites.
    case 0x105e9ad0: return ScreenTemplate::credits;
    case 0x1062c380: return ScreenTemplate::web_sites;
    // Text entry dialogs: the player's nickname and Send Message.
    case 0x103ac1c0: case 0x102d0500: return ScreenTemplate::text_entry;
    // Disciplinary action on a player: two overlapping grids (the action row and the reasons) and a Cancel/Confirm footer.
    // The press question opened from the news has the same two grids with a Cancel/Ok footer.
    case 0x1010ff90: case 0x1067bea0: return ScreenTemplate::disciplinary;
    // The Manager menu's meetings with the board (demands) and with the league (issues to discuss) have the same two grids: an intro
    // line and the options, and a Cancel/Confirm footer.
    case 0x105d6ac0: case 0x106ee9a0: return ScreenTemplate::disciplinary;
    // Granting a player a special holiday or sending him to a spiritual counsellor (a confirmation opened from the player's action menu).
    case 0x10702580: return ScreenTemplate::message_screen;
    // Choosing a player's role in the club (his squad status), reached from the contract negotiation and the news.
    case 0x100952b0: return ScreenTemplate::role_choice;
    // The club screen's "Last meetings" with another club: a summary table, the meetings and the Back/Next footer.
    case 0x10041550: return ScreenTemplate::head_to_head;
    // The awards of a competition (the list of an award over the seasons and the nominees of an award) and the friendly tour's
    // invited clubs: one table under the subtitle and the Back/Next footer.
    case 0x10000180: case 0x100026d0: case 0x10048450: return ScreenTemplate::paged_table;
    // Pages with a tab bar: the transfer rules article of the news, the Historial, the game database and a club's Historial.
    // The tournament setup of the friendly tour: a fixed dialog in the middle of the window.
    case 0x10046de0: return ScreenTemplate::centered_dialog;
    // Squad Numbers and the European Squad (one builder, opened from the news and before a match): a palette of the numbers 1-50 and a list of the squad.
    case 0x1004a4a0: return ScreenTemplate::squad_numbers;
    case 0x106ff680: case 0x10145290: case 0x10106840: case 0x1003b7f0: return ScreenTemplate::tabbed_page;
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
