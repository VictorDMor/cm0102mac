// FONT.CPP, block `fontfiles` (0x1074d380, 0x1074d450, 0x1074f170, 0x1074f2c0, 0x10750d30, 0x10750f00; 599 instructions): where a font's bytes
// come from. A font is either an 'sfnt' resource of a file in the FONTS folder (found by the Resource Manager) or the file itself
// (a .ttf or .t2k opened with fopen); the list builder walks the folder, asks each file for its fonts and reads each one's full name.
//
// Registers: every routine leaves r3 and r4-r10 as the original does (the checker compares them in full, with no exemption): the code
// sets what the original sets before each call, in the original's order, and the registers a callee leaves stay in place. The
// non-volatile registers hold what the original holds at each call (callees save them in their frames), assigned where the original
// assigns them. Each routine builds the original's frame (size, saved registers): the callees' frames then lie at the original's depth
// and the locals are at the original's addresses; the list builder keeps its array of rows in its guest stack, because the table it
// allocates is a copy of that array and takes the bytes the rows leave unwritten (the two pad bytes after each row's index) from the
// stack as it was.
#include "font_cpp_files_detail.hpp"

namespace cm::aot::font_cpp {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::extended;

std::int32_t byte_of(std::uint32_t value) { return std::int32_t(std::int8_t(value)); }
std::int32_t half_of(std::uint32_t value) { return std::int32_t(std::int16_t(value)); }

// The four calls of one "read `length` bytes at `offset` of the font data into `destination`" of the name reader: a Handle (the
// resource, copied with BlockMoveData after a size check) or a FILE (an fseek and an fread of one item). Each of the five reads of
// 0x10750f00 is a copy of the same code with its own return addresses.
struct ReadSites {
    std::uint32_t size, move, seek, read;
    bool first;  // the first read compares the handle size with an immediate (`cmplwi r3,0xc`); the others compare the end with the size
};
constexpr ReadSites header_read{0x10750f2c, 0x10750f54, 0x10750f80, 0x10750fa4, true};
constexpr ReadSites table_read{0x10750fdc, 0x1075100c, 0x10751038, 0x1075105c, false};
constexpr ReadSites name_header_read{0x10751098, 0x107510c8, 0x107510f4, 0x10751118, false};
constexpr ReadSites record_read{0x10751164, 0x10751194, 0x107511c0, 0x107511e4, false};
constexpr ReadSites string_read{0x1075123c, 0x1075126c, 0x10751298, 0x107512bc, false};

// The font data of `font` ({+0 FILE, +4 Handle}): `length` bytes at `offset`, into `destination`. True when all arrived. Ends, as the
// original does, with the byte result tested (`extsb. r0,r0`).
bool read_part(Cpu& c, std::uint32_t font, std::uint32_t offset, std::uint32_t length, std::uint32_t destination, const ReadSites& at) {
    bool done;
    const auto handle = c.u32(font + 4);
    c.set(3, handle);
    compare_logical(c, handle, 0);
    if (handle != 0) {
        c.call(glue::handle_size, at.size); reload_toc(c);
        const auto size = c.r(3);
        if (at.first) compare_logical(c, size, length);
        else compare_logical(c, offset + length, size);
        if (at.first ? size < length : offset + length > size) {
            done = false;
        } else {
            const auto handle_again = c.u32(font + 4);
            c.set({{3, handle_again}, {4, destination}, {5, length}});
            c.set(3, c.u32(c.r(3)) + offset);
            c.call(lib::block_move, at.move); reload_toc(c);
            done = true;
        }
    } else {
        const auto file = c.u32(font);
        c.set(3, file);
        compare_logical(c, file, 0);
        if (file == 0) {
            done = false;
        } else {
            c.set({{4, offset}, {5, 0}});
            c.call(lib::fseek, at.seek);
            compare(c, std::int32_t(c.r(3)), 0);
            if (c.r(3) != 0) {
                done = false;
            } else {
                c.set({{6, c.u32(font)}, {3, destination}, {4, length}, {5, 1}});
                c.call(lib::fread, at.read);
                done = c.r(3) == 1;
            }
        }
    }
    compare(c, done ? 1 : 0);
    return done;
}
}

// 0x1074d380: an object over a resource file: the current resource file at +0, the file opened with `permission` at +2 (created first
// when `create` says so and the open failed), made current when open.
std::uint32_t open_resource_file(Cpu& c, std::uint32_t object, std::uint32_t spec, std::uint32_t permission, std::uint32_t create) {
    Frame frame(c, 0x1074d380, 0x50, {.first_gpr = 28});
    c.set({{31, create}, {30, permission}, {29, spec}, {28, object}});
    c.call(glue::current_res_file, 0x1074d3b0); reload_toc(c);
    c.put16(object, c.r(3));
    c.set({{3, spec}, {4, extended(std::int16_t(byte_of(permission)))}});
    c.call(glue::open_res_file, 0x1074d3c4); reload_toc(c);
    c.put16(object + 2, c.r(3));
    const auto refnum = half_of(c.u16(object + 2));
    compare(c, refnum, -1);
    if (refnum == -1) {
        compare(c, std::int32_t(create & 0xff), 0);
        if ((create & 0xff) != 0) {
            c.set({{4, 0x3f3f3f3f}, {3, spec}, {6, 0xffffffff}});
            c.set(5, c.r(4));
            c.call(glue::create_res_file, 0x1074d3f8); reload_toc(c);
            c.set({{3, spec}, {4, extended(std::int16_t(byte_of(permission)))}});
            c.call(glue::open_res_file, 0x1074d408); reload_toc(c);
            c.put16(object + 2, c.r(3));
        }
    }
    const auto open = half_of(c.u16(object + 2));
    c.set(3, extended(std::int16_t(open)));
    compare(c, open, -1);
    if (open != -1) {
        c.call(glue::use_res_file, 0x1074d420);
        reload_toc(c);
    }
    c.set(3, object);
    return object;
}

// 0x1074d450: closes the object's resource file (the previous one made current first) and, when `flag` is positive, frees the object.
std::uint32_t close_resource_file(Cpu& c, std::uint32_t object, std::uint32_t flag) {
    Frame frame(c, 0x1074d450, 0x40, {.first_gpr = 30});
    c.set({{31, flag}, {30, object}});
    compare(c, std::int32_t(object), 0);
    if (object != 0) {
        c.set(3, extended(c.i16(object)));
        c.call(glue::use_res_file, 0x1074d478); reload_toc(c);
        const auto refnum = c.i16(object + 2);
        c.set(3, extended(refnum));
        compare(c, refnum, -1);
        if (refnum != -1) {
            c.call(glue::close_res_file, 0x1074d48c);
            reload_toc(c);
        }
        compare(c, half_of(flag), 0);
        if (half_of(flag) > 0) {
            c.set(3, object);
            c.call(lib::delete_object, 0x1074d4a0);
        }
    }
    c.set(3, object);
    return object;
}

// 0x1074f2c0: a font file (.ttf or .t2k by its extension) opened for reading into `font`'s FILE; only index 0 exists. 1 when opened.
std::uint32_t open_font_file(Cpu& c, std::uint32_t path, std::uint32_t font, std::uint32_t index) {
    Frame frame(c, 0x1074f2c0, 0x150, {.first_gpr = 28});
    const auto pool = c.toc(toc::pool), buffer = frame.at(0x38);
    c.set({{31, pool}, {30, index}, {29, font}, {28, path}});
    c.set({{6, 0}, {5, 0}, {4, 0}, {7, buffer}});
    c.call(lib::split_path, 0x1074f300);
    c.set({{4, pool + pool_file::ttf}, {3, buffer}});
    c.call(lib::same_extension, 0x1074f30c);
    compare(c, std::int32_t(c.r(3)), 0);
    if (c.r(3) != 0) {
        c.set({{3, buffer}, {4, pool + pool_file::t2k}});
        c.call(lib::same_extension, 0x1074f320);
        compare(c, std::int32_t(c.r(3)), 0);
        if (c.r(3) != 0) {
            c.set(3, 0);
            return 0;
        }
    }
    compare_logical(c, index & 0xffff, 0);
    if ((index & 0xffff) != 0) {
        c.set(3, 0);
        return 0;
    }
    c.set({{3, path}, {4, pool + pool_file::read_mode}});
    c.call(lib::fopen, 0x1074f34c);
    c.put32(font, c.r(3));
    const auto file = c.u32(font);
    c.set(3, file != 0 ? 1 : 0);  // `neg; or; rlwinm 1,31,31; extsb`
    return c.r(3);
}

// 0x1074f170: the data of font number `index` of the file `path`: an 'sfnt' resource of it (detached, its Handle at +4 of `font`) or,
// when the file has no sfnt resources, the file itself (opened by 0x1074f2c0, FILE at +0; index 0 only). 1 when found.
std::uint32_t find_font_data(Cpu& c, std::uint32_t path, std::uint32_t font, std::uint32_t index) {
    Frame frame(c, 0x1074f170, 0x1a0, {.first_gpr = 28});
    const auto pascal = frame.at(0x88), spec = frame.at(0x40), resource_file = frame.at(0x3c);
    c.set({{31, 0}, {30, index}, {29, font}, {28, path}});
    c.put32(font + 4, 0);
    c.put32(font, 0);
    c.set({{3, pascal}, {4, path}});
    c.call(lib::pascal_name, 0x1074f1b4);
    c.set({{5, pascal}, {6, spec}, {3, 0}, {4, 0}});
    c.call(glue::make_fsspec, 0x1074f1c8); reload_toc(c);
    const auto made = half_of(c.r(3));
    compare(c, made, 0);
    if (made == 0) {
        c.set({{3, spec}, {5, frame.at(0x39)}, {6, frame.at(0x38)}, {4, 1}});
        c.call(glue::resolve_alias, 0x1074f1e8); reload_toc(c);
        c.set({{3, resource_file}, {4, spec}, {5, 0}, {6, 0}});
        c.call(own::open_resource_file, 0x1074f200);
        const auto refnum = half_of(c.u16(frame.at(0x3e)));
        compare(c, refnum, -1);
        bool found = false;
        if (refnum != -1) {
            c.set(3, glue::sfnt_type);
            c.call(glue::count_resources, 0x1074f218); reload_toc(c);
            c.set(31, c.r(3));
            c.set(4, index & 0xffff);
            compare(c, std::int32_t(c.r(4)), half_of(c.r(31)));
            if (std::int32_t(c.r(4)) <= half_of(c.r(31))) {
                c.set({{3, glue::sfnt_type}, {4, extended(std::int16_t(c.r(4) + 1))}});
                c.call(glue::indexed_resource, 0x1074f244); reload_toc(c);
                c.put32(font + 4, c.r(3));
                c.set(3, c.u32(font + 4));
                compare_logical(c, c.r(3), 0);
                if (c.r(3) != 0) {
                    c.call(glue::detach_resource, 0x1074f25c); reload_toc(c);
                    found = true;
                }
            }
        }
        c.set({{3, resource_file}, {4, 0xffffffff}});
        c.call(own::close_resource_file, found ? 0x1074f26c : 0x1074f280);
        if (found) {
            c.set(3, 1);
            return 1;
        }
    }
    compare(c, half_of(c.r(31)), 0);
    if (half_of(c.r(31)) != 0) {
        c.set(3, 0);
        return 0;
    }
    c.set({{3, path}, {4, font}, {5, index}});
    c.link(0x1074f2a0);
    return open_font_file(c, path, font, index);
}

// 0x10750f00: the full name (name id 4) of the font data `font` ({+0 FILE, +4 Handle}) read from its TrueType 'name' table into `buffer`
// (at most size - 1 bytes, terminated); 1 when a nonempty name was read. The data is read through the Handle when there is one, else
// through the FILE, in pieces: the sfnt header (12 bytes), each table directory entry (16) until 'name', the name table's header (6),
// each name record (12) until name id 4, then the string. A piece that cannot be read ends that level's search.
std::uint32_t read_font_name(Cpu& c, std::uint32_t font, std::uint32_t buffer, std::uint32_t size) {
    Frame frame(c, 0x10750f00, 0x90, {.first_gpr = 22});
    const auto header = frame.at(0x5c), entry = frame.at(0x4c), name_header = frame.at(0x38), name_record = frame.at(0x40);
    const auto tables = [&] { return std::uint32_t(c.u16(header + 4)); };
    const auto records = [&] { return std::uint32_t(c.u16(name_header + 2)); };
    c.set({{30, font}, {31, buffer}, {29, size}});
    if (!read_part(c, font, 0, 0xc, header, header_read)) {
        c.set(3, 0);
        return 0;
    }
    c.set({{28, 0}, {27, 0}});
    for (std::uint32_t table = 0, table_offset = 0;; ++table, table_offset += 0x10) {
        const auto count = tables();
        compare(c, std::int32_t(table), std::int32_t(count));
        if (std::int32_t(table) >= std::int32_t(count)) break;
        c.set(22, table_offset + 0xc);
        const bool entry_read = read_part(c, font, table_offset + 0xc, 0x10, entry, table_read);
        const auto tag = c.u32(entry);
        if (entry_read) {
            c.set(3, tag);
            compare_logical(c, tag - 0x6e610000, 0x6d65);
        }
        if (entry_read && tag == glue::name_table_tag) {
            const auto table_start = c.u32(entry + 8);
            c.set(23, table_start);
            if (read_part(c, font, table_start, 6, name_header, name_header_read)) {
                const auto start = c.u32(entry + 8);
                c.set(3, start);
                const auto strings = start + c.u16(name_header + 4);
                c.set({{24, 0}, {23, 0}, {25, strings}});
                for (std::uint32_t record = 0, record_offset = 0;; ++record, record_offset += 0xc) {
                    const auto limit = records();
                    compare(c, std::int32_t(record), std::int32_t(limit));
                    if (std::int32_t(record) >= std::int32_t(limit)) break;
                    const auto at = c.u32(entry + 8) + record_offset + 6;
                    c.set(22, at);
                    bool name_found = false;
                    if (read_part(c, font, at, 0xc, name_record, record_read)) {
                        const auto name_id = std::uint32_t(c.u16(name_record + 6));
                        compare(c, std::int32_t(name_id), 4);
                        if (name_id == 4) {
                            auto length = std::int32_t(size - 1);
                            c.set(26, std::uint32_t(length));
                            const auto stored = std::int32_t(c.u16(name_record + 8));
                            compare(c, stored, length);
                            if (stored < length) {
                                length = stored;
                                c.set(26, std::uint32_t(length));
                            }
                            compare(c, length, 0);
                            if (length > 0) {
                                const auto string_at = strings + c.u16(name_record + 0xa);
                                c.set(22, string_at);
                                if (read_part(c, font, string_at, std::uint32_t(length), buffer, string_read)) {
                                    c.put8(buffer + std::uint32_t(length), 0);
                                    const auto first = byte_of(c.u8(buffer));
                                    compare(c, first, 0);
                                    if (first != 0) {
                                        compare(c, 1, 0);
                                        name_found = true;
                                    }
                                }
                            }
                        }
                        if (!name_found) compare(c, 0, 0);
                    }
                    if (name_found) {
                        c.set(3, 1);
                        return 1;
                    }
                    c.set({{24, record + 1}, {23, record_offset + 0xc}});
                }
            }
        }
        c.set({{28, table + 1}, {27, table_offset + 0x10}});
    }
    c.set(3, 0);
    return 0;
}

// 0x10750d30: the list of fonts in the FONTS folder. Each file of the folder is asked for its fonts (0x1074f170, number 0 up and on while it
// finds one); a font's row {name pointer of the file, alias pointer (its full name, 0 when unreadable), file font number} goes into an array
// in this routine's stack, the font data it opened is released, and the count (the list's variable) grows. The walk stops at the end of the
// folder or, checked after each file, at 100 rows. The rows are then copied into a table allocated for them (the list's table variable).
// 1 when the list has rows and the table was allocated, else 0.
std::uint32_t scan_font_folder(Cpu& c) {
    Frame frame(c, 0x10750d30, 0xa20, {.first_gpr = 27});
    const auto pool = c.toc(toc::pool);
    const auto folder = c.toc(toc::folder_path), count_variable = c.toc(toc::list_count), table_variable = c.toc(toc::list_table);
    const auto pattern = frame.at(0x140), found = frame.at(0x6f4), font = frame.at(0x38), name = frame.at(0x40), rows = frame.at(0x244);
    const auto row_at = [&](std::uint32_t index) { return rows + index * font_row::size; };
    c.set({{30, folder}, {29, count_variable}, {31, table_variable}});
    c.set({{3, pattern}, {4, 0}, {5, folder}, {6, pool + pool_file::any_name}, {7, 0}});
    c.call(lib::make_path, 0x10750d68);
    c.set({{3, pattern}, {4, found}, {5, 0}});
    c.call(lib::find_first, 0x10750d78);
    auto status = byte_of(c.r(3));
    compare(c, status, 0);
    while (status == 0) {
        c.set(3, found);
        c.call(lib::copy_name, 0x10750d84);
        const auto file_name = c.r(3);
        c.set(27, file_name);
        compare(c, std::int32_t(file_name), 0);
        if (file_name != 0) {
            c.set({{5, folder}, {6, file_name}, {3, pattern}, {28, 0}, {4, 0}, {7, 0}});
            c.call(lib::make_path, 0x10750da8);
            for (std::uint32_t number = 0;; ++number) {
                c.set({{5, number}, {3, pattern}, {4, font}});
                c.call(own::find_font_data, 0x10750e68);
                compare(c, byte_of(c.r(3)), 0);
                if (byte_of(c.r(3)) == 0) break;
                const auto row = row_at(c.u16(count_variable));
                c.set({{6, row}, {3, font}, {4, name}, {5, 0xff}});
                c.put32(row, file_name);
                c.put16(row + 8, number);
                c.link(0x10750dd4);
                const auto named = read_font_name(c, font, name, 0xff);
                compare(c, byte_of(named), 0);
                const auto count_now = std::uint32_t(c.u16(count_variable));
                if (byte_of(named) != 0) {
                    c.set(3, name);
                    c.call(lib::copy_name, 0x10750de4);
                    const auto alias_row = row_at(c.u16(count_variable));
                    c.set({{4, rows}, {5, std::uint32_t(c.u16(count_variable)) * font_row::size}});
                    c.put32(alias_row + font_row::alias, c.r(3));
                } else {
                    c.set({{3, rows}, {5, 0}, {4, count_now * font_row::size}});
                    c.put32(row_at(count_now) + font_row::alias, 0);
                }
                auto handle = c.u32(font + 4);
                c.set(3, handle);
                compare_logical(c, handle, 0);
                if (handle != 0) {
                    c.call(glue::dispose_handle, 0x10750e24); reload_toc(c);
                    c.put32(font + 4, 0);
                }
                auto stream = c.u32(font);
                c.set(3, stream);
                compare_logical(c, stream, 0);
                if (stream != 0) {
                    c.call(lib::fclose, 0x10750e40);
                    c.put32(font, 0);
                }
                const auto before = std::uint32_t(c.u16(count_variable));
                c.set({{3, before}, {28, number + 1}});
                c.put16(count_variable, before + 1);
            }
        }
        const auto count = std::uint32_t(c.u16(count_variable));
        c.set(27, 0);
        compare_logical(c, count, 100);
        if (count == 100) {
            c.set(27, 1);
        } else {
            c.set(3, found);
            c.call(lib::find_next, 0x10750e88);
            compare(c, byte_of(c.r(3)), 0);
            if (byte_of(c.r(3)) != 0) c.set(27, 1);
        }
        c.set(3, extended(std::int16_t(byte_of(c.r(27)))));
        status = byte_of(c.r(3));
        compare(c, status, 0);
    }
    c.set(3, found);
    c.call(lib::find_close, 0x10750ea8);
    const auto count = std::uint32_t(c.u16(count_variable));
    compare_logical(c, count, 0);
    if (count == 0) {
        c.set(3, 0);
        return 0;
    }
    c.set(3, count * font_row::size);
    c.call(lib::allocate, 0x10750ec4);
    const auto table = c.r(3);
    compare_logical(c, table, 0);
    c.put32(table_variable, table);
    if (table == 0) {
        c.set(3, 0);
        return 0;
    }
    const auto copied = std::uint32_t(c.u16(count_variable));
    c.set({{4, rows}, {5, copied * font_row::size}});
    c.call(lib::move_bytes, 0x10750ee8);
    c.set(3, 1);
    return 1;
}
}
