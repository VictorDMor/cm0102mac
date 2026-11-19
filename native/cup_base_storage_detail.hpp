#pragma once
// CUP.CPP block `storage`: what the save reader (cup_base_storage_load.cpp) and writer
// (cup_base_storage_save.cpp) share: the order of the fixed fields of the record and the two routines' assertion
// sites. Field names are cup_base_detail.hpp's and comp::competition's; "(inferred)" marks a meaning no reader
// confirmed.
#include "cup_base_detail.hpp"
#include "cup_base_storage.hpp"
#include <array>

namespace cm::aot::cup_base::storage {
namespace routine {
inline constexpr std::uint32_t
    format = 0x1077b720, file_path = 0x10746280, open_file = 0x10739de0, read_file = 0x10775fe0,
    write_file = 0x10776360, close_file = 0x107766f0, new_object = 0x10773520, new_block = 0x10775130,
    delete_block = 0x10775170, localized = 0x10727890, error_dialog = 0x10769b60, version_text = 0x10768810,
    split_path = 0x10739890, stats_destroy = 0x104536c0, stats_construct = 0x104533a0, stats_read = 0x1045f6a0,
    stats_write = 0x10460230, list_construct = 0x104bc750, list_read = 0x104bcb20, list_write = 0x104bd550,
    group_construct = 0x1056e120, group_read = 0x1056e430, group_write = 0x1056f6f0;
}
// Slots of the cup's vtable the reader and the writer call (the derived cup's records).
inline constexpr std::uint32_t read_records_slot = 0x28, write_records_slot = 0x2c;
// TOC cells only the reader loads: two words the localized "Memory Allocation Error" dialog takes as its buffers
// (the `r25` and `r28` of 0x104831a0).
inline constexpr std::int32_t dialog_title_slot = -0x79dc, dialog_text_slot = -0x79e0;
// Pool texts of this block: text:: in cup_base_detail.hpp (temporary_name, temporary_folder, read_mode, write_mode,
// save_open and the load_* messages).

// The fixed fields between the derived records and the stage tables, in the file's order (the reader and the
// writer agree). `back` is the return address of the reader's fread / the writer's fwrite of the field.
struct Field { std::uint32_t offset, size, load_back, save_back; };
inline constexpr std::array<Field, 19> fields{{
    {competition::word_28, 4, 0x1048337c, 0x10484604}, {competition::byte_4c, 1, 0x10483390, 0x10484618},
    {competition::byte_4b, 1, 0x104833a4, 0x1048462c}, {competition::byte_48, 1, 0x104833b8, 0x10484640},
    {competition::byte_49, 1, 0x104833cc, 0x10484654}, {competition::word_24, 4, 0x10483440, 0x10484668},
    {competition::bench, 1, 0x10483454, 0x1048467c}, {competition::substitutes, 1, 0x104834d0, 0x10484690},
    {competition::byte_45, 1, 0x1048354c, 0x104846a4}, {cup::byte_b0, 1, 0x104835c0, 0x104846b8},
    {competition::rounds, 2, 0x1048363c, 0x104846cc}, {competition::club_count, 2, 0x104836b8, 0x104846e0},
    {competition::year, 2, 0x10483734, 0x104846f4}, {cup::current_round, 1, 0x104837b0, 0x10484708},
    {cup::byte_a9, 1, 0x1048382c, 0x1048471c}, {competition::type, 1, 0x104838a8, 0x10484730},
    {competition::code, 2, 0x10483924, 0x10484744}, {competition::parent, 4, 0x104839a0, 0x10484758},
    {competition::next, 4, 0x10483a1c, 0x1048476c}}};
}
