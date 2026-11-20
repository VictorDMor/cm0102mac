#pragma once
// The competition editor inside the game's own interface. The start screen ("Game Setup", builder 0x10334550)
// gains one button; while the editor is open the same screen is rebuilt with the editor's controls in place of
// the nine choices, made by the game's own control routines (as the season selector of
// database_season_screen.cpp), so fonts, colours, picking and both canvases are the game's.
//
// It edits a custom database (competition_overrides.hpp) against an edition's complete values through
// CompetitionEditorDocument. Every change is saved at once to the database's file under "Custom Databases";
// the first change to the original rules starts a new database. The values come from "Competition Bases"
// (installed with the app: every competition of an edition) and from what a New Game recorded
// (competition_defaults.hpp), the installed file first.
#include <cstdint>

namespace cm::aot {
struct State;
// True when the editor took the event: the monitor then rebuilds the screen.
bool competition_editor_event(State&, std::uint32_t handler, std::int16_t control);
// After the original builder and the layouts ran.
void competition_editor_after_build(State&, std::uint32_t builder);
// After the screen's background was painted and before its controls: while the editor is open it darkens the
// picture under its panel, so the values read on any background. `table` is the screen being drawn: only the
// editor's own (a dialog over it keeps its picture). Only frame buffer pixels change.
void competition_editor_backdrop(State&, std::uint32_t table);
}
