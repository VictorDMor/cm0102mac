#pragma once
// The competition editor's window: a separate tool inside the app, opened from the Game menu while no career is
// running. It edits a custom database (a TOML file, competition_overrides.hpp) against the recorded defaults of an
// edition (competition_defaults.hpp) through CompetitionEditorDocument, and chooses which database new careers
// start with. Main thread only; it never touches the guest.
#include <filesystem>
#include <functional>
#include <optional>

namespace cm::aot {
struct CompetitionEditorSetup {
    std::filesystem::path defaults_directory;   // one recorded-defaults file per edition
    std::filesystem::path databases_directory;  // where custom databases are saved by default
    std::filesystem::path selection_file;       // holds the path of the database new careers start with
    // The choice changed, or the chosen file was saved again: none when no database is chosen.
    std::function<void(std::optional<std::filesystem::path>)> selection_changed;
};
// Before the app quits: asks about unsaved edits. False when the user cancels.
bool competition_editor_may_quit();
// Opens the window, or brings it forward.
void show_competition_editor(const CompetitionEditorSetup&);
}
