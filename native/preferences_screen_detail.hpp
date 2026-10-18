#pragma once
#include "semantic_guest_call.hpp"
namespace cm::aot::preferences_screen::detail {
using Call=SemanticGuestCall;
void initialize_resources(Call&);
void capture_session(Call&);
void create_settings_frame(Call&);
void build_language_selector(Call&);
void build_currency_selector(Call&);
void build_font_selector(Call&);
void create_options_grid(Call&);
void build_wage_choices(Call&);
void build_screen_choices(Call&);
void build_background_change_choices(Call&);
void build_sound_choices(Call&);
void build_database_choices(Call&);
void build_match_storage_choices(Call&);
void build_autosave_choices(Call&);
void build_compression_choices(Call&);
void build_safe_save_choices(Call&);
void build_flashing_choices(Call&);
void build_background_match_choices(Call&);
void build_print_choices(Call&);
void return_to_caller(Call&);
}
