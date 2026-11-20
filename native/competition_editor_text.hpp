#pragma once
// The competition editor's texts in the game's languages. The editor follows the language chosen in the game (the
// live language byte, TOC -0x35ac; the IDs of debug_localization.hpp: 0 and 1 English, 2 French, 3 German,
// 4 Italian, 5 Spanish, 6 Danish, 7 Dutch, 8 Norwegian, 9 Portuguese, 10 Swedish); an unknown ID reads English.
// Texts are UTF-8; the screen converts them to the game's Windows-1252. `{0}`, `{1}` are filled by `filled`.
// Some rows served the first editor, a macOS window, and wait for text entry and file dialogs in the game's
// interface (menu_item, open, save_as, search, save_changes and its buttons, saved_as).
#include "competition_values.hpp"
#include <initializer_list>
#include <string>
#include <string_view>

namespace cm::aot {
enum class EditorText {
    menu_item, window_title, database, name_placeholder, new_database, open, save, save_as, use_for_new, search,
    edition_database, effect_results, effect_fixtures, effect_season_end,
    tie_break_end, tie_break_goal_difference, tie_break_goals_scored, tie_break_wins, tie_break_head_to_head,
    tie_break_away_goals, tie_break_keep_order,
    original, rounds, round_of, revert, choose, empty_state, stage, cup_tag,
    changed_values, not_saved_yet, uses_original, uses_this, uses_other,
    cannot_read, not_custom_database, line, no_base_title, no_base_text, not_opened, not_saved, choice_not_saved,
    save_changes, cancel, dont_save, default_name,
    need_number, need_range, places_exceed, head_to_head_refused, no_such_round,
    warn_places, warn_substitutions, warn_scheduled,
    back, all_nations, international, yes, no, original_rules, no_bases, saved_as,
    count
};
inline constexpr unsigned editor_language_count = 11;
std::string_view editor_text(unsigned language, EditorText);
// A registry parameter's label ("Points for a win"); the registry's English label for one without a translation.
std::string editor_label(unsigned language, const competition_values::Parameter&);
// `text` with {0}, {1}... replaced.
std::string filled(std::string_view text, std::initializer_list<std::string> values);
}
