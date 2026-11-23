// FONT.CPP, block `record` (0x1074f390, 0x1074f410, 0x10750200, 0x1074f470; 350 instructions): the life of a font record (0x24 bytes,
// font_record in font_cpp_detail.hpp). The constructor and destructor are reached through the descriptors of the array of eight records
// the start-up builds; the loader fills a record from a font file or resource (found by block fontfiles) and the release frees what the
// record holds.
//
// Registers and stack: as in block fontfiles (the original's frames, the registers set where it sets them and the non-volatile registers
// as it holds them at each call). The engine's routines are called typed with a link register at the original's return address and
// announced to the checker's call trace; the game's own run-time and the Toolbox glue through the dispatcher. No routine here leaves
// stack bytes for a reader (the four leavers of the module are in other blocks); the engine's delete routines are called at the original's
// stack pointer and with r28-r31 as the original holds them, because the stack leaver they reach saves those.
#include "font_cpp_files_detail.hpp"
#include "utils.hpp"

namespace cm::aot::font_cpp {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::extended;

std::int32_t byte_of(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }

// `bl 0x107755c0` of an assertion site: the failed expression, the unit's file name and the line.
void assertion(Cpu& c, std::uint32_t pool, std::uint32_t text, std::uint32_t line, std::uint32_t back) {
    c.set({{3, pool + text}, {4, pool + pool_file::name}, {5, line}});
    c.call(lib::assertion, back);
}
}

// 0x1074f390: the constructor of an element of the array: the font list is built (0x10750a00), a record allocated into `slot` and zeroed.
std::uint32_t new_font_record(Cpu& c, std::uint32_t slot) {
    Frame frame(c, 0x1074f390, 0x40, {.first_gpr = 30});
    const auto pool = c.toc(toc::pool);
    c.set({{31, pool}, {30, slot}});
    c.call(own::load_list, 0x1074f3b0);
    c.set(3, font_data::record_size);
    c.call(lib::new_object, 0x1074f3b8);
    c.put32(slot, c.r(3));
    const auto record = c.u32(slot);
    compare_logical(c, record, 0);
    if (record == 0) assertion(c, pool, pool_file::assertion_data, assertion_line::record_allocate, 0x1074f3d8);
    c.set({{3, c.u32(slot)}, {4, 0}, {5, font_data::record_size}});
    c.call(lib::fill_memory, 0x1074f3e8);
    c.set(3, slot);
    return slot;
}

// 0x1074f410: the destructor: when `slot` is not null the record is released (0x10750200) and freed, and with a positive `flag` the slot too.
std::uint32_t delete_font_record(Cpu& c, std::uint32_t slot, std::uint32_t flag) {
    Frame frame(c, 0x1074f410, 0x40, {.first_gpr = 30});
    c.set({{31, flag}, {30, slot}});
    compare(c, std::int32_t(slot), 0);
    if (slot != 0) {
        c.call(own::release_font, 0x1074f434);
        c.set(3, c.u32(slot));
        c.call(lib::delete_object, 0x1074f43c);
        compare(c, std::int32_t(std::int16_t(flag)), 0);
        if (std::int16_t(flag) > 0) {
            c.set(3, slot);
            c.call(lib::delete_object, 0x1074f44c);
        }
    }
    c.set(3, slot);
    return slot;
}

// 0x10750200: frees everything the record `*slot` holds (each cached glyph record and its bitmap, the cache array, the scaler, the sfnt,
// the stream, the memory object and the file copy) and zeroes it. The record is not freed.
std::uint32_t release_font(Cpu& c, std::uint32_t slot) {
    Frame frame(c, 0x10750200, 0x50, {.first_gpr = 28});
    const auto pool = c.toc(toc::pool), error = frame.at(0x38);
    c.set(4, pool);
    const auto record = c.u32(slot);
    c.set(30, record);
    compare_logical(c, record, 0);
    if (record == 0) assertion(c, pool, pool_file::assertion_data, assertion_line::release_record, 0x1075023c);
    c.set({{29, 0}, {31, 0}});
    for (;;) {
        const auto count = c.i32(record + font_record::cache_count);
        const auto index = std::int32_t(std::int16_t(c.r(29)));
        c.set(3, extended(std::int16_t(index)));
        compare(c, index, count);
        if (index >= count) break;
        c.set(3, c.u32(record + font_record::cache));
        const auto glyph = c.u32(c.r(3) + c.r(31));
        c.set(28, glyph);
        const auto bitmap = c.u32(glyph + glyph_cache::bitmap);
        c.set(3, bitmap);
        compare_logical(c, bitmap, 0);
        if (bitmap != 0) c.call(lib::release, 0x10750264);
        c.set(3, glyph);
        c.call(lib::delete_object, 0x1075026c);
        c.set({{31, c.r(31) + 4}, {29, c.r(29) + 1}});
    }
    const auto cache = c.u32(record + font_record::cache);
    c.set(3, cache);
    compare_logical(c, cache, 0);
    if (cache != 0) c.call(lib::delete_array, 0x10750294);
    const auto scaler = c.u32(record + font_record::scaler);
    c.set(3, scaler);
    compare_logical(c, scaler, 0);
    if (scaler != 0) {
        c.set(4, error);
        c.call(engine::delete_scaler, 0x107502a8);
    }
    const auto sfnt = c.u32(record + font_record::sfnt);
    c.set(3, sfnt);
    compare_logical(c, sfnt, 0);
    if (sfnt != 0) {
        c.set(4, error);
        c.call(engine::delete_sfnt, 0x107502bc);
    }
    const auto stream = c.u32(record + font_record::stream);
    c.set(3, stream);
    compare_logical(c, stream, 0);
    if (stream != 0) {
        c.set(4, error);
        c.call(engine::delete_stream, 0x107502d0);
    }
    const auto memory = c.u32(record + font_record::sfnt_memory);
    c.set(3, memory);
    compare_logical(c, memory, 0);
    if (memory != 0) {
        c.call(engine::delete_memory, 0x107502e0);
    }
    const auto copy = c.u32(record + font_data::copy);
    c.set(3, copy);
    compare_logical(c, copy, 0);
    if (copy != 0) c.call(lib::release, 0x107502f0);
    c.set({{3, record}, {4, 0}, {5, font_data::record_size}});
    c.call(lib::fill_memory, 0x10750300);
    return c.r(3);
}

// 0x1074f470: loads a font into the record `*slot` (released first). `kind` says which: 0 and 4+ the small font, 1 medium, 2 large (the
// selected font's file of the FONTS folder, else small.t2k / medium.t2k / large.t2k of the data folder), 3 symbol.ttf of the data folder.
// The file's bytes are copied into a block (+4), a memory object (+8) and an input stream over the copy (+0xc) made, and +0 set.
// 1 when loaded. The selection's row gives the file name and the font number of the file.
std::uint32_t load_font(Cpu& c, std::uint32_t slot, std::uint32_t kind) {
    Frame frame(c, 0x1074f470, 0x160, {.first_gpr = 26});
    const auto table_variable = c.toc(toc::list_table), selection_variable = c.toc(toc::selection), pool = c.toc(toc::pool);
    const auto error = frame.at(0x38), file = frame.at(0x3c), handle = frame.at(0x40), path = frame.at(0x44);
    c.set({{27, table_variable}, {26, slot}, {28, selection_variable}, {29, kind}, {31, pool}});
    const auto record = c.u32(slot);
    c.set(30, record);
    compare_logical(c, record, 0);
    if (record == 0) assertion(c, pool, pool_file::assertion_data, assertion_line::loader_record, 0x1074f4b0);
    c.set(3, slot);
    c.call(own::release_font, 0x1074f4b8);

    // The font's name (r6) and its number in the file (r27): the selection's row, or the data folder's own file for a built-in selection.
    const auto kind8 = kind & 0xff;
    std::uint32_t name;
    const auto from_selection = [&](std::uint32_t built_in) {
        const auto selected = c.i32(selection_variable);
        compare(c, selected, 0);
        if (selected < 0) {
            c.set({{6, pool + built_in}, {27, 0}});
        } else {
            const auto offset = std::uint32_t(selected) * font_row::size;
            const auto table = c.u32(table_variable);
            c.set({{4, table}, {3, table + offset}});
            c.set({{6, c.u32(table + offset)}, {27, c.u16(table + offset + 8)}});
        }
    };
    compare(c, std::int32_t(kind8), 2);
    if (kind8 == 2) {
        from_selection(pool_file::large_font);
    } else if (kind8 > 2) {
        compare(c, std::int32_t(kind8), 4);
        if (kind8 >= 4) {
            from_selection(pool_file::small_font);
        } else {
            c.set({{6, pool + pool_file::symbol_font}, {27, 0}});
        }
    } else {
        compare(c, std::int32_t(kind8), 0);
        if (kind8 == 1) from_selection(pool_file::medium_font);
        else from_selection(pool_file::small_font);
    }
    name = c.r(6);
    const auto selected = c.i32(selection_variable);
    compare(c, selected, 0);
    if (selected >= 0) compare_logical(c, kind8, 3);
    if (selected >= 0 && kind8 != 3) {
        c.set({{5, c.toc(toc::folder_path)}, {3, path}, {4, 0}, {7, 0}});
        c.call(lib::make_path, 0x1074f5a4);
    } else {
        c.set({{4, name}, {3, pool + pool_file::data_variable}});
        direct_call(c, data_path, 0x1074f5b4);
        const auto data = utils::data_path(c, c.r(3), c.r(4));
        c.set({{3, path}, {4, data}, {5, 0x100}});
        c.call(copy_limited, 0x1074f5c8);
        c.put8(frame.at(0x143), 0);
    }
    c.set({{5, c.r(27)}, {3, path}, {4, file}});
    c.link(0x1074f5e0);
    const auto found = find_font_data(c, path, file, c.r(5));
    compare(c, byte_of(found), 0);
    if (byte_of(found) == 0) {
        c.set(3, 0);
        return 0;
    }
    // The size of the data: the Handle's, or the FILE's length.
    const auto close_data = [&](std::uint32_t handle_back, std::uint32_t file_back) {
        const auto open_handle = c.u32(handle);
        c.set(3, open_handle);
        compare_logical(c, open_handle, 0);
        if (open_handle != 0) {
            c.call(glue::dispose_handle, handle_back); reload_toc(c);
            c.put32(handle, 0);
        }
        const auto open_file = c.u32(file);
        c.set(3, open_file);
        compare_logical(c, open_file, 0);
        if (open_file != 0) {
            c.call(lib::fclose, file_back);
            c.put32(file, 0);
        }
    };
    const auto data_handle = c.u32(handle);
    c.set(3, data_handle);
    compare_logical(c, data_handle, 0);
    if (data_handle != 0) {
        c.call(glue::handle_size, 0x1074f600); reload_toc(c);
        c.set(27, c.r(3));
    } else {
        const auto open_file = c.u32(file);
        c.set({{3, open_file}, {4, 0}, {5, 2}});
        c.call(lib::fseek, 0x1074f61c);
        compare(c, std::int32_t(c.r(3)), 0);
        if (c.r(3) != 0) {
            c.set(3, 0);
        } else {
            c.set(3, c.u32(file));
            c.call(lib::ftell, 0x1074f634);
        }
        c.set(27, c.r(3));
    }
    const auto size = c.r(27);
    compare_logical(c, size, 0);
    if (size == 0) {
        close_data(0x1074f650, 0x1074f66c);
        c.set(3, 0);
        return 0;
    }
    c.set(3, size);
    c.call(lib::allocate, 0x1074f684);
    c.put32(record + font_data::copy, c.r(3));
    compare_logical(c, c.u32(record + font_data::copy), 0);
    if (c.u32(record + font_data::copy) == 0) assertion(c, pool, pool_file::data_buffer, assertion_line::loader_buffer, 0x1074f6a4);
    const auto buffer = c.u32(record + font_data::copy);
    const auto source_handle = c.u32(handle);
    c.set({{3, source_handle}, {28, buffer}});
    compare_logical(c, source_handle, 0);
    bool copied;
    if (source_handle != 0) {
        c.call(glue::handle_size, 0x1074f6b8); reload_toc(c);
        compare_logical(c, size, c.r(3));
        if (size > c.r(3)) {
            c.set(28, 0);
            copied = false;
        } else {
            c.set({{3, c.u32(handle)}, {4, c.r(28)}, {5, size}});
            c.set(3, c.u32(c.r(3)));
            c.call(lib::block_move, 0x1074f6e0); reload_toc(c);
            c.set(28, 1);
            copied = true;
        }
    } else {
        const auto open_file = c.u32(file);
        c.set(3, open_file);
        compare_logical(c, open_file, 0);
        if (open_file == 0) {
            c.set(28, 0);
            copied = false;
        } else {
            c.set({{4, 0}, {5, 0}});
            c.call(lib::fseek, 0x1074f70c);
            compare(c, std::int32_t(c.r(3)), 0);
            if (c.r(3) != 0) {
                c.set(28, 0);
                copied = false;
            } else {
                c.set({{6, c.u32(file)}, {3, c.r(28)}, {4, size}, {5, 1}});
                c.call(lib::fread, 0x1074f730);
                copied = c.r(3) == 1;
                c.set(28, copied ? 1 : 0);
            }
        }
    }
    close_data(0x1074f750, 0x1074f76c);
    compare(c, byte_of(c.r(28)), 0);
    if (!copied) {
        c.set(3, 0);
        return 0;
    }
    c.set(3, error);
    c.call(engine::new_memory, 0x1074f78c);
    c.put32(record + font_record::sfnt_memory, c.r(3));
    const auto first_error = c.u32(error);
    compare(c, std::int32_t(first_error), 0);
    if (first_error != 0) assertion(c, pool, pool_file::no_error, assertion_line::loader_memory, 0x1074f7ac);
    const auto memory = c.u32(record + font_record::sfnt_memory), data = c.u32(record + font_data::copy);
    c.set({{3, memory}, {5, size}, {4, data}, {6, error}});
    c.call(engine::new_stream, 0x1074f7c0);
    c.put32(record + font_record::stream, c.r(3));
    const auto second_error = c.u32(error);
    compare(c, std::int32_t(second_error), 0);
    if (second_error != 0) assertion(c, pool, pool_file::no_error, assertion_line::loader_stream, 0x1074f7e0);
    c.set(3, 1);
    c.put8(record + font_record::loaded, 1);
    return 1;
}
}
