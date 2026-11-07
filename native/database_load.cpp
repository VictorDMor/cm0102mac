// DATABASE.CPP's load block, part 1: 0x100a55a0, the database loader (database_load.hpp). This file holds the
// entry, the index file's opening and its header, and the order of the passes; the other parts are
// database_load_counts.cpp (sizing and allocating), database_load_defaults.cpp (default records),
// database_load_tables.cpp (the readers) and database_load_finish.cpp (the closing re-read and the fix-up).
//
// Addresses in `call(bl, target)` are those of the original's `bl`: the return address is the next instruction.
// The original has two bodies for most steps, one for the packed reads and one for plain stdio (selected by
// `extsb. r0,r16`, which also leaves cr0), so every site has a pair of addresses (Pair).
#include "database_load.hpp"
#include "database_load_detail.hpp"
#include "database_loading.hpp"
#include "database_tables.hpp"
#include "database_resolve.hpp"
#include <bit>

namespace cm::aot::database {
using guest::compare;
using guest::compare_logical;
using namespace load;

bool Loader::packed() {
    const auto mode = ext(c.r(16));
    c.set(0, mode);
    return compare(c, std::int32_t(mode)) != 0;
}

// The status line: r3 the status word, r4 the text, r5 the step, r6 0x2e, r7 -1, r8 the next step (stored back
// unless this is the last call).
void Loader::progress(std::uint32_t bl, bool advance) {
    const auto step = guest::extended(std::int16_t(c.u16(frame.at(local::step))));
    c.set({{4, frame.at(local::progress_text)}, {5, step}, {6, 0x2e}, {3, c.toc(toc::status)}, {7, 0xffffffff}});
    if (advance) {
        c.set(8, step + 1);
        c.put16(frame.at(local::step), step + 1);
    }
    call(bl, callee::show_progress);
}

std::uint32_t Loader::read(const Pair& r, Pair site, std::uint32_t destination, std::uint32_t size, std::uint32_t count) {
    const bool plain_ = !packed();
    c.set({{3, destination}, {4, size}, {5, count}, {6, c.r(19)}});
    call(plain_ ? site.plain : site.packed, plain_ ? r.plain : r.packed);
    return c.r(3);
}

// A failed read or check closes the file: the packed close takes the address of the stack slot the handle is
// stored in, the plain one the handle.
void Loader::close_file(std::uint32_t slot, Pair site) {
    if (packed()) {
        c.put32(frame.at(slot), c.r(19));
        c.set(3, frame.at(slot));
        call(site.packed, callee::close_packed);
    } else {
        c.set(3, c.r(19));
        call(site.plain, callee::close_plain);
    }
}

// One four-byte word of the header (`cmplwi r3,1; beq`).
bool Loader::read_word(std::uint32_t at, Pair site, Pair close, std::uint32_t slot, const Pair& r) {
    const auto got = read(r, site, at, 4, 1);
    if (compare_logical(c, got, 1) == 0) return true;
    close_file(slot, close);
    return false;
}

void Loader::swap_word(std::uint32_t at) { c.put32(at, std::byteswap(c.u32(at))); }

// Lines 0x100a5690-0x100a5844: the file's location (found through the packed open or by walking the folder),
// the language file's presence, and the open of the index file under its two names.
bool Loader::open_files() {
    if (packed()) {
        c.set({{3, c.r(15)}, {4, frame.at(local::path)}, {5, pool() + pool::path}});
        call(0x100a56a4, callee::packed_open_path);
        if (compare(c, ext(c.r(3))) == 0) {
            c.set({{3, c.u32(c.r(19))}, {4, 0}});
            call(0x100a56b8, callee::reset_language);
        }
    } else {
        // Walk the folder until the finder answers a found name.
        c.set({{5, c.r(15)}, {3, frame.at(local::finder)}, {6, pool() + pool::path}, {4, 0}, {7, 0}});
        call(0x100a56d4, callee::find_first);
        c.set({{3, frame.at(local::finder)}, {4, frame.at(local::found_name)}, {5, 0}});
        call(0x100a56e4, callee::find_next);
        auto found = ext(c.r(3));
        c.set(0, found);
        while (compare(c, std::int32_t(found)) == 0) {
            c.set({{5, c.r(15)}, {3, frame.at(local::finder)}, {6, frame.at(local::found_name)}, {4, 0}, {7, 0}});
            call(0x100a5704, callee::find_first);
            c.set({{3, pool() + pool::folder}, {4, frame.at(local::found_name)}});
            call(0x100a5710, callee::data_folder_path);
            c.set({{4, c.r(3)}, {3, frame.at(local::second_path)}});
            call(0x100a571c, callee::copy_text);
            c.set({{3, frame.at(local::finder)}, {4, frame.at(local::second_path)}});
            call(0x100a5728, callee::find_name);
            c.set(3, frame.at(local::found_name));
            call(0x100a5730, callee::find_step);
            found = ext(c.r(3));
            c.set(0, found);
        }
        c.set(3, frame.at(local::found_name));
        call(0x100a5744, callee::find_close);
    }
    // The language file must exist; with none the language resets.
    const auto language = c.u8(c.u32(c.r(19)) + 8);
    c.set(3, language);
    guest::enter(c, 0x100a5754, {language});
    language_file_exists(c, std::int32_t(language));
    if (compare(c, ext(c.r(3))) == 0) {
        c.set({{3, c.u32(c.r(19))}, {4, 0}});
        call(0x100a5764, callee::reset_language);
    }
    c.set({{3, frame.at(local::alternate_name)}, {4, pool() + pool::alternate}});
    call(0x100a5770, callee::copy_text);
    c.set({{3, pool() + pool::alternate}, {4, frame.at(local::alternate_name)}, {5, pool() + pool::alternate_suffix}});
    call(0x100a5780, callee::packed_name);
    // The first name opens; failing, the second name does (and must).
    const auto open = [&](std::uint32_t name, Pair site) {
        const bool pk = packed();
        c.set({{3, c.r(15)}, {4, name}});
        if (pk) c.set({{5, 0}, {6, 0x15}, {7, 4}});
        call(pk ? site.packed : site.plain, pk ? callee::packed_open : callee::plain_open);
        c.set(19, c.r(3));
        compare(c, std::int32_t(c.r(3)));
    };
    open(frame.at(local::alternate_name), {0x100a57a0, 0x100a57b0});
    if (c.r(19) != 0) {
        c.put8(frame.at(local::swapped), 0);
        return true;
    }
    open(pool() + pool::alternate, {0x100a57e4, 0x100a57f4});
    if (c.r(19) == 0) return false;
    // Plain files carry a marker in their first word: a different one means the other byte order.
    std::uint32_t flag = 0;
    if (!packed()) {
        c.set(3, c.r(19));
        call(0x100a581c, callee::read_marker);
        flag = c.r(3) != 0xf1fffff1;
    }
    c.put8(frame.at(local::swapped), flag);
    return true;
}

// 0x100a5844-0x100a5d5c: the two header words and the 22 index entries, byte-swapped when the file is.
bool Loader::read_index() {
    const auto swapped = [&] {
        const auto flag = ext(c.u8(frame.at(local::swapped)));
        c.set(0, flag);
        return compare(c, std::int32_t(flag)) != 0;
    };
    const auto word_a = c.r(2) + std::uint32_t(toc::header_a), word_b = c.r(2) + std::uint32_t(toc::header_b);
    if (!read_word(word_a, {0x100a585c, 0x100a5874}, {0x100a5890, 0x100a589c}, 0x188)) return false;
    if (swapped()) swap_word(word_a);
    if (!read_word(word_b, {0x100a58f0, 0x100a5908}, {0x100a5924, 0x100a5930}, 0x184)) return false;
    if (swapped()) swap_word(word_b);
    const auto entries = c.r(21);
    if (!swapped()) {
        // One read for all the entries.
        const auto got = read(reader, {0x100a5990, 0x100a59a8}, entries, entry::size, entry::count_of_entries);
        if (compare_logical(c, got, entry::count_of_entries) != 0) {
            close_file(0x180, {0x100a59c4, 0x100a59d0});
            return false;
        }
    } else {
        // Entry by entry (the four words swap after each entry's reads).
        struct Part {
            std::uint32_t offset, size, count;
            Pair site, close;
            std::uint32_t slot;
        };
        constexpr std::array<Part, 5> parts{{{0, 1, entry::name_length, {0x100a5a00, 0x100a5a1c}, {0x100a5a38, 0x100a5a44}, 0x17c},
                                             {entry::kind, 4, 1, {0x100a5a70, 0x100a5a90}, {0x100a5aac, 0x100a5ab8}, 0x178},
                                             {entry::count, 4, 1, {0x100a5ae4, 0x100a5b04}, {0x100a5b20, 0x100a5b2c}, 0x174},
                                             {entry::offset, 4, 1, {0x100a5b58, 0x100a5b78}, {0x100a5b94, 0x100a5ba0}, 0x170},
                                             {entry::flag, 4, 1, {0x100a5bcc, 0x100a5bec}, {0x100a5c08, 0x100a5c14}, 0x16c}}};
        for (std::uint32_t i = 0; i < entry::count_of_entries; ++i) {
            c.set(20, i);
            const auto at = entries + i * entry::size;
            for (const auto& part : parts) {
                const auto got = read(reader, part.site, at + part.offset, part.size, part.count);
                if (compare_logical(c, got, 1) < 0) {
                    close_file(part.slot, part.close);
                    return false;
                }
            }
            for (const auto offset : {entry::kind, entry::count, entry::offset, entry::flag}) swap_word(at + offset);
        }
        c.set(20, entry::count_of_entries);
    }
    return true;
}

// 0x100a5d5c-0x100a5f34: the optional language block (a flag byte; with it a 0x1cc-byte block, a count and that
// many words of a map table, and the language byte) and the close of the file.
bool Loader::read_language_block() {
    read(reader, {0x100a5d74, 0x100a5d8c}, frame.at(local::extended), 1, 1);
    const auto flag = ext(c.u8(frame.at(local::extended)));
    c.set(0, flag);
    if (compare(c, std::int32_t(flag)) != 0) {
        read(reader, {0x100a5db4, 0x100a5dcc}, frame.at(local::language_block), 0x1cc, 1);
        read(reader, {0x100a5de8, 0x100a5e00}, frame.at(local::map_count), 4, 1);
        const auto words = c.u32(frame.at(local::map_count));
        c.set(0, words);
        if (compare(c, std::int32_t(words), -1) != 0) {
            c.set(3, words << 2);
            call(0x100a5e18, callee::allocate);
            c.put32(c.toc(toc::map_table), c.r(3));
            const auto block = c.u32(c.toc(toc::map_table));
            c.set(0, block);
            if (compare_logical(c, block) == 0) {
                guest::report_assertion(c, {0x100a5e48, pool(), pool() + 0xd, pool() + 0x1a, frame.at(0x60c),
                                            frame.at(0x2c4), 0x908});
                c.set({{0, 0}, {3, 0xffffffff}});
                call(0x100a5e88, callee::abort_run);
            }
            read(reader, {0x100a5eac, 0x100a5ecc}, c.u32(c.toc(toc::map_table)), words << 2, 1);
        }
        read(reader, {0x100a5ee8, 0x100a5f00}, c.toc(toc::language_byte), 1, 1);
    } else {
        c.put8(c.toc(toc::language_byte), 0);
    }
    close_file(0x164, {0x100a5f24, 0x100a5f30});
    return true;
}

void Loader::run(std::uint32_t name, std::uint32_t single_block, std::uint32_t mode) {
    // The prologue: the TOC words the loop keeps in r13-r31 and the frame's zeroed locals.
    c.set({{25, c.toc(toc::pool)}, {15, name}, {21, c.toc(toc::entries)}, {16, mode}, {19, c.toc(toc::language_cell)},
           {22, c.toc(cells::placeholder_count)}, {18, 0x10}, {14, c.toc(cells::subs_c_count)},
           {17, 0}, {13, c.toc(cells::subs_b_count)}, {23, c.toc(toc::success_flag)}, {24, c.toc(toc::message)},
           {26, c.toc(cells::people_table)}, {27, c.toc(cells::rows_34_count)}, {28, c.toc(cells::rows_44_count)},
           {29, c.toc(cells::people_count)}, {30, c.toc(cells::rows_34_table)}, {31, c.toc(cells::rows_44_table)}});
    c.put8(frame.at(local::extended), 0);
    c.put16(frame.at(local::step), 0);
    for (const auto offset : {local::clubs, local::people, local::rows_48, local::rows_44, local::rows_14,
                              local::rows_1c, local::rows_3c_b})
        c.put32(frame.at(offset), 0);
    c.put8(frame.at(local::single_block), single_block);
    c.set({{0, 0}, {3, pool() + pool::folder}, {4, pool() + pool::folder_end}});
    call(0x100a564c, callee::data_folder_path);
    c.set({{4, c.r(3)}, {3, frame.at(local::path)}});
    call(0x100a5658, callee::copy_text);
    guest::enter(c, 0x100a5660);
    release_all_tables(c);
    c.set({{3, frame.at(local::progress_text)}, {4, pool() + pool::text}});
    call(0x100a5668, callee::copy_path);
    progress(0x100a568c);

    const auto fail = [&] { c.set(3, 0); };
    if (!open_files() || !read_index()) return fail();
    read_language_block();
    c.set(18, c.r(18) - c.toc(toc::header_b));
    progress(0x100a5f60);

    const auto flag = ext(c.u8(frame.at(local::single_block)));
    c.set(0, flag);
    const bool carved = compare(c, std::int32_t(flag)) != 0;
    c.put8(c.r(2) + std::uint32_t(toc::single_block_flag), c.u8(frame.at(local::single_block)));
    count_kinds(carved);
    if (carved) {
        sizing_totals();
        carve();
        placeholder_clubs();
        if (c.u8(frame.at(local::extended)) == 0) default_rows_biased();
        default_rows();
    }
    if (!load_tables()) return fail();
    if (ext(c.u8(frame.at(local::swapped))) != 0 && !reread_index()) return fail();
    sizes_and_indexes();
    fix_people();
    call(0x100ab5b0, callee::finish);
    c.set(3, 1);
}

bool load_database(Cpu& c, std::uint32_t name, std::uint32_t single_block, std::uint32_t packed) {
    constexpr std::uint32_t entry = 0x100a55a0;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x12f0, {.first_gpr = 13});
    Loader loader(c, frame);
    loader.run(name, single_block, packed);
    return c.r(3) != 0;
}
}
