#pragma once
// Pieces the three DB_FILES.CPP sources share: the module's string pool and
// the runtime's stream routines, whose stack frames hold the caller's registers.
#include "db_files.hpp"
#include "guest_values.hpp"

namespace cm::aot::db_files {
inline constexpr std::uint32_t stream_read = 0x10775fe0, stream_write = 0x10776360, stream_close = 0x107766f0,
    packed_read = 0x10747a80, packed_write = 0x10747cc0, read_end_marker = 0x107464d0,
    write_end_marker = 0x107465a0, open_stream = 0x10739de0, merge_path = 0x10739b40,
    split_path = 0x10739890, data_folder_path = 0x10746280, replace_extension = 0x10746470,
    allocate = 0x10775130, release = 0x10775170, copy_string = 0x1077dbf0, copy_string_limited = 0x1077dc10,
    append_string = 0x1077dc60, block_move = 0x1077fc28, exit_program = 0x1076f7d0;
// Every table file ends in this word when it holds Mac records.
inline constexpr std::uint32_t mac_end_marker = 0xf1fffff1;
inline constexpr std::int32_t pool_slot = -0x42e8;
inline constexpr std::uint32_t pool_rb = 0, pool_wb = 3, pool_space = 0x15, pool_dam = 0x17, pool_data_folder = 0x1c,
    pool_source_file = 0x25, pool_format = 0x32, pool_error = 0x3f;

inline guest::Assertion assertion(Cpu& c, std::uint32_t at, std::uint32_t name, std::uint32_t extension,
                                  std::uint32_t line) {
    const auto pool = c.toc(pool_slot);
    return {at, pool + pool_source_file, pool + pool_format, pool + pool_error, name, extension, line};
}
}
