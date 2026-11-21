#pragma once
#include "screen_layout.hpp"
#include <span>
#include <vector>

namespace cm::aot {
// A renderer-independent form shell. All dimensions are logical pixels; the
// original controls still own typography, colors, text, actions and input.
struct FormMetrics {
    int maximum_width = 1320;
    int maximum_height = 600;
    int title_height = 45;
    int toolbar_height = 20;
    int toolbar_leading_width = 250;
    int toolbar_trailing_width = 125;
    int footer_height = 30;
    int footer_width = 654;
};
struct FormLayout {
    ScreenRect panel, title, toolbar_leading, toolbar_trailing, body, fields, footer;
};
FormLayout form_layout(ScreenRect available, FormMetrics);
// A track has a minimum size, optional growth and a finite size limit. The
// same primitive lays out stacked sections and label/value/button columns.
struct FormTrack { int minimum; float grow = 0; int maximum = 0; };
std::vector<ScreenRect> form_tracks(ScreenRect, std::span<const FormTrack>, bool horizontal);
// Stacked list and reading area, with a compact toolbar and story heading.
// Extra height favors reading; the list stops growing at a finite height.
struct ReadingMetrics {
    int minimum_list_height=90;
    int maximum_list_height=360;
    int minimum_body_height=215;
    int toolbar_height=20;
    int headline_height=35;
    int gap=5;
};
struct ReadingLayout { ScreenRect list, toolbar, headline, body; };
ReadingLayout reading_layout(ScreenRect available, ReadingMetrics = {});
struct State;
void layout_startup(State&, std::uint32_t display, GuiCanvas);
void layout_league_selection(State&, std::uint32_t display, GuiCanvas, bool selector);
void layout_load_browser(State&, std::uint32_t display, GuiCanvas);
void layout_search_filters(State&, std::uint32_t display, GuiCanvas);
void layout_transfer_proposal(State&, std::uint32_t display, GuiCanvas);
void layout_contract_negotiation(State&, std::uint32_t display, GuiCanvas);
void layout_negotiation_dialog(State&, std::uint32_t display, GuiCanvas, bool loan_terms);
void layout_news_inbox(State&, std::uint32_t display, GuiCanvas);
void layout_next_match(State&, std::uint32_t display, GuiCanvas);
void layout_competition_stage(State&, std::uint32_t display, GuiCanvas);
void layout_competition_list(State&, std::uint32_t display, GuiCanvas, unsigned section, bool historical);
ScreenRect live_match_update_rect(State&, std::uint32_t display, int control, ScreenRect);
void layout_match_report(State&, std::uint32_t display, GuiCanvas);
void layout_daily_fixtures(State&, std::uint32_t display, GuiCanvas);
void layout_staff_profile(State&, std::uint32_t display, GuiCanvas);
void layout_training_schedule(State&, std::uint32_t display, GuiCanvas);
void layout_training_coaches(State&, std::uint32_t display, GuiCanvas);
void layout_training(State&, std::uint32_t display, GuiCanvas);
void layout_nations_and_clubs(State&, std::uint32_t display, GuiCanvas);
void layout_club_information(State&, std::uint32_t display, GuiCanvas);
}
