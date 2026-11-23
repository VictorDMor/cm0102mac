#pragma once
// FONT.CPP, blocks `fontfiles` and `record`: the routines they call outside the module's own pages and the pool strings they pass.
// Everything here is reached by `c.call` (dispatched) unless a block says otherwise; addresses are those of the original's `bl`s.
#include "font_cpp_detail.hpp"

namespace cm::aot::font_cpp {
// The Toolbox glue (the resource manager, the file system and the memory manager): answered per call by the checker's cases, since a
// snapshot has no host behind them.
namespace glue {
inline constexpr std::uint32_t current_res_file = 0x10780378,  // CurResFile() -> refnum
    open_res_file = 0x107802d0,                                // FSpOpenResFile(spec, permission) -> refnum, -1 on failure
    create_res_file = 0x107802b8,                              // FSpCreateResFile(spec, creator, type, script)
    use_res_file = 0x107803c0,                                 // UseResFile(refnum)
    close_res_file = 0x107803d8,                               // CloseResFile(refnum)
    make_fsspec = 0x1077fee0,                                  // FSMakeFSSpec(volume, dir, name, spec) -> error
    resolve_alias = 0x1077ffa0,                                // ResolveAliasFile(spec, resolve chain, &is folder, &was aliased)
    count_resources = 0x10780f48,                              // Count1Resources(type)
    indexed_resource = 0x10780f60,                             // Get1IndResource(type, index from 1) -> handle
    detach_resource = 0x10780f78,                              // DetachResource(handle)
    handle_size = 0x107800a8,                                  // GetHandleSize(handle)
    dispose_handle = 0x107800c0;                               // DisposeHandle(handle)
inline constexpr std::uint32_t sfnt_type = 0x73666e74;         // 'sfnt'
inline constexpr std::uint32_t name_table_tag = 0x6e616d65;    // 'name'
}
// `lwz r2,0x14(r1)`: after a call into the glue the original reloads the TOC pointer the glue saved in the frame's linkage area.
inline void reload_toc(Cpu& c) { c.set(2, c.u32(c.sp() + 0x14)); }
// The C library and the game's own helpers the two blocks call.
namespace lib {
inline constexpr std::uint32_t new_object = 0x10773520, delete_object = 0x107735a0, allocate = 0x107735f0, release = 0x10773610,
                               delete_array = 0x10775170, fill_memory = 0x10778300, move_bytes = 0x10739ef0, block_move = 0x1077fc28,
                               fopen = 0x107768f0, fclose = 0x107766f0, fread = 0x10775fe0, fseek = 0x10777f10, ftell = 0x10777cc0,
                               split_path = 0x10739890, same_extension = 0x107393f0, make_path = 0x10739b40, find_first = 0x10739690,
                               find_next = 0x107397d0, find_close = 0x107156a0, copy_name = 0x107395f0, pascal_name = 0x1074c5d0,
                               assertion = 0x107755c0;
}
// The font engine's entries the record block calls (all through the dispatcher: see font_cpp_record.cpp).
namespace engine {
inline constexpr std::uint32_t delete_scaler = 0x10760f50, delete_sfnt = 0x10767190, delete_stream = 0x107632a0, delete_memory = 0x10767720,
                               new_memory = 0x10767460, new_stream = 0x107631c0;
}
// Pool strings (offsets from the pool the TOC slot -0x3e0c holds).
namespace pool_file {
inline constexpr std::uint32_t ttf = 0, t2k = 5, read_mode = 0xa, name = 0x12, symbol_font = 0x1b, large_font = 0x26, medium_font = 0x30,
                               small_font = 0x3b, data_variable = 0x45, data_buffer = 0x4d, no_error = 0x5a, any_name = 0xf6;
inline constexpr std::uint32_t assertion_data = 0xd;  // "data": the assertion's text at the allocation and release sites
}
// The module's own entries reached through the dispatcher (cross-page calls): the resource file helpers, the font data finder, the
// release, and the list builder.
namespace own {
inline constexpr std::uint32_t open_resource_file = 0x1074d380, close_resource_file = 0x1074d450, find_font_data = 0x1074f170,
                               release_font = 0x10750200, load_list = 0x10750a00;
}
// The font record's field the loader fills besides the ones font_cpp_detail.hpp names: the copy of the font file's bytes (+4, allocated).
namespace font_data {
inline constexpr std::uint32_t copy = 4, record_size = 0x24;
}
// The source lines of the assertions of the two blocks.
namespace assertion_line {
inline constexpr std::uint32_t record_allocate = 0x1e2, release_record = 0x3cd, loader_record = 0x20b, loader_buffer = 0x25a,
                               loader_memory = 0x268, loader_stream = 0x26e;
}
}
