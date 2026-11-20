#pragma once
// What the competition editor edits, without any window: the recorded defaults of one edition
// (competition_defaults.hpp) and a custom database being made from them (competition_overrides.hpp). The window
// (competition_editor.mm) only shows this and calls it.
#include "competition_overrides.hpp"
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace cm::aot {
// The database chosen for new careers: the path the selection file holds on its first line, if any.
std::optional<std::filesystem::path> selected_custom_database(const std::filesystem::path& selection_file);

class CompetitionEditorDocument {
public:
    using Entry = competition_values::Entry;
    using Parameter = competition_values::Parameter;
    CompetitionEditorDocument(CompetitionEdition base, std::vector<Entry> defaults);

    CompetitionEdition base() const { return base_; }
    std::string name;
    // Sorted by nation, kind (leagues first) and id. Cup stages are left out: they are not editable.
    const std::vector<Entry>& competitions() const { return competitions_; }
    // The parameters the window shows for a competition: `rounds` false gives the object's (editable first, then
    // the read-only structure), true a cup round's.
    static std::vector<const Parameter*> parameters(competition_values::Kind, bool rounds);

    // `round` is -1 for the object's own parameters.
    std::int32_t original(std::size_t competition, const Parameter&, std::int32_t round) const;
    // The value the database gives from the career's start: the edit, else the original.
    std::int32_t value(std::size_t competition, const Parameter&, std::int32_t round) const;
    bool edited(std::size_t competition, const Parameter&, std::int32_t round) const;
    // Sets it; the original value removes the edit. "" or why it was refused (nothing changes then).
    std::string set(std::size_t competition, const Parameter&, std::int32_t round, std::int32_t value);
    void revert(std::size_t competition);
    // Whether the database changes this competition at all, and how many of its entries start in a later season
    // (made by hand or by the in-career screen; the window keeps them and does not edit them).
    bool modified(std::size_t competition) const;
    std::size_t scheduled(std::size_t competition) const;
    // What the user should know about the competition's edits; they are saved and applied all the same.
    std::vector<std::string> warnings(std::size_t competition) const;
    void rename(std::string to) { if (to != name) { name = std::move(to); dirty_ = true; } }
    bool dirty() const { return dirty_; }
    void saved() { dirty_ = false; }

    // Replaces the edits with a database's. Refused ("made from ...") for another base edition. Entries for
    // competitions that are not in the defaults are kept and saved back.
    std::string load(const competition_overrides::CustomDatabase&);
    competition_overrides::CustomDatabase database() const;

private:
    competition_overrides::Target target(std::size_t competition) const;
    const competition_overrides::Override* find(std::size_t competition, const Parameter&, std::int32_t round) const;
    CompetitionEdition base_;
    std::vector<Entry> competitions_;
    std::vector<competition_overrides::Override> entries_;
    bool dirty_ = false;
};
}
