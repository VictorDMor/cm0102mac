#pragma once
#include "guest_abi.hpp"

namespace cm::aot::file_screens {
using guest::Cpu;
// The file browser (load, save and delete dialogs).
struct FileDialog {
    std::uint32_t heading, prompt, mode, folder, extension, preset, filter, text_colour;
};
void open_file_dialog(Cpu&, const FileDialog&);
void render_file_dialog(Cpu&);
void file_dialog_action(Cpu&, std::uint32_t control);
// The saved game's information screen and the summary saved with each game.
void open_save_info(Cpu&, std::uint32_t path);
void render_save_info(Cpu&);
void save_info_action(Cpu&, std::uint32_t control);
void fill_save_summary(Cpu&, std::uint32_t summary);
// The message box, with its text as a string or as a text object.
void open_message(Cpu&, std::uint32_t heading, std::uint32_t title, std::uint32_t body, std::uint32_t confirm,
                  std::uint32_t text_colour, std::uint32_t back_colour, std::uint32_t parent_value);
void open_long_message(Cpu&, std::uint32_t heading, std::uint32_t title, std::uint32_t body, std::uint32_t confirm,
                       std::uint32_t text_colour, std::uint32_t back_colour, std::uint32_t parent_value);
void render_message(Cpu&);
void message_action(Cpu&, std::uint32_t control);
}
