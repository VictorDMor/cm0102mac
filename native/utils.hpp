#pragma once
#include "guest_abi.hpp"

namespace cm::aot::utils {
using guest::Cpu;
// Text.
void swap_memory(Cpu&, std::uint32_t first, std::uint32_t second, std::uint32_t size);
void trim(Cpu&, std::uint32_t text);
std::int32_t compare_text(Cpu&, std::uint32_t first, std::uint32_t second, std::uint32_t exact_case, std::uint32_t limit);
std::uint32_t to_upper(Cpu&, std::uint32_t character);
std::uint32_t to_lower(Cpu&, std::uint32_t character);
std::uint32_t ordinal_text(Cpu&, std::uint32_t number, std::uint32_t gender);
std::uint32_t number_text(Cpu&, std::uint32_t number, std::uint32_t text, std::uint32_t capacity, std::uint32_t lowercase);
std::uint32_t break_line(Cpu&, std::uint32_t text, std::uint32_t before_middle, std::uint32_t after_middle);
void short_name(Cpu&, std::uint32_t name, std::uint32_t output);
// Files and directories.
std::uint32_t environment_directory(Cpu&, std::uint32_t variable);
std::uint32_t data_path(Cpu&, std::uint32_t variable, std::uint32_t file);
std::uint32_t folder_path(Cpu&, std::uint32_t folder, std::uint32_t path);
std::uint32_t usable_path(Cpu&, std::uint32_t path);
void path_with_extension(Cpu&, std::uint32_t source, std::uint32_t path, std::uint32_t extension);
std::uint32_t read_file_marker(Cpu&, std::uint32_t stream);
std::uint32_t write_file_marker(Cpu&, std::uint32_t stream, std::uint32_t marker);
std::uint32_t read_line(Cpu&, std::uint32_t stream, std::uint32_t line);
std::uint32_t copy_file(Cpu&, std::uint32_t source, std::uint32_t destination);
void delete_temporary_files(Cpu&, std::uint32_t pattern);
// System.
std::uint32_t milliseconds(Cpu&);
void flush_input(Cpu&);
void open_url(Cpu&, std::uint32_t url);
void initialize_random(Cpu&, std::uint32_t seed);
}
