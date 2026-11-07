#pragma once
// DATABASE.CPP's load block: TOC words, callees, per-kind tables and the loader (database_load.hpp).
//
// The original (0x100a55a0, one frame of 0x12f0 bytes saving r13-r31) reads the index file, whose 22 entries of 0x44
// bytes each name a table by a kind (0-22; 8 is unused) with its record count and file offset, then makes up to
// three passes over the entries, each a jump table on the kind: the first sizes or allocates the tables, the
// last reads them through one reader per kind. Held across the calls: r15 the file name, r16 the packed flag,
// r17 the single-block size, r18 a record bias (0x10 less the second header word), r19 the file, r20 the entry
// index of the sizing/default loops, r21 the index entries, r13 and r14 the entry index and address of the last
// pass, r22-r31 the TOC words of the count and table cells the kinds use. Names are inferred from the uses.
#include "database_load.hpp"
#include "guest_values.hpp"
#include <array>
#include <cstdint>

namespace cm::aot::database::load {
namespace callee {
constexpr std::uint32_t data_folder_path = 0x10746280, copy_text = 0x1077dbf0, copy_path = 0x10727890,
                        show_progress = 0x10743120, packed_open_path = 0x10748460, reset_language = 0x1013fb60,
                        find_first = 0x10739b40, find_next = 0x10739690, find_step = 0x107397d0,
                        find_name = 0x107470b0, find_close = 0x107156a0, packed_name = 0x10746470,
                        packed_open = 0x107475b0, plain_open = 0x105d71b0, read_marker = 0x107464d0,
                        read_packed = 0x10747a80, read_plain = 0x10775fe0, close_packed = 0x10747900,
                        close_plain = 0x107766f0, allocate = 0x10775130, release = 0x10775170,
                        format_text = 0x1077b720, abort_run = 0x1076f7d0, seek_packed = 0x10747fe0,
                        seek_plain = 0x10777f10, reopen = 0x105d7200, reread_packed = 0x10747cc0,
                        reread_plain = 0x10776360, date_default = 0x10715880, date_make = 0x107156f0,
                        flush = 0x10776690, finish = 0x1014d560, random = 0x1077b770;
}
// TOC slots (offsets from r2) the routine reads; each holds a word, or the address of one.
namespace toc {
constexpr std::int32_t entries = -0x756c, pool = -0x5618, success_flag = -0x7bac, message = -0x7ba8,
                       status = -0x7bb0, language_cell = -0x7ac8, language_byte = -0x7ab0, map_table = -0x74f8,
                       header_a = -0x39d4, header_b = -0x39d8, single_block_flag = -0x39d0, detail_count = -0x39dc,
                       detail_table = -0x39e0, text_buffer = -0x7bb8, sample_cursor = -0x7b84,
                       sample_end = -0x7b88, sample_start = -0x7b8c, sample_salt = -0x7b90, constants = -0x5614,
                       sizing_jumps = -0x5620, count_jumps = -0x561c, load_jumps = -0x5624,
                       person_jumps = -0x5628, person_jumps_second = -0x562c;
}
// The count and table cells of the 22 kinds (TOC slot of the cell's address; r13-r31 hold some of them).
namespace cells {
constexpr std::int32_t clubs_count = -0x7b50, clubs_table = -0x7be8, second_count = -0x7500, second_table = -0x74fc,
                       rows_3c_count = -0x7564, rows_3c_table = -0x7544, rows_c8_count = -0x7524,
                       rows_c8_table = -0x7520, rows_128_count = -0x7b04, rows_128_table = -0x7b98,
                       rows_50_count = -0x7568, rows_50_table = -0x7548, people_count = -0x7be4,
                       people_table = -0x7be0, rows_2c_count = -0x7528, rows_2c_table = -0x7ab4,
                       rows_44_count = -0x79f4, rows_44_table = -0x79f8, rows_48_count = -0x74d4,
                       rows_48_table = -0x7b38, rows_68_count = -0x755c, rows_68_table = -0x753c,
                       placeholder_count = -0x7b64, placeholder_table = -0x7bbc, subs_a_count = -0x754c,
                       subs_a_table = -0x752c, subs_b_count = -0x7550, subs_b_table = -0x7530,
                       subs_c_count = -0x7554, subs_c_table = -0x7534, rows_6c_count = -0x7560,
                       rows_6c_table = -0x7540, rows_14_count = -0x74c8, rows_14_table = -0x74c4,
                       rows_3c_b_count = -0x74d0, rows_3c_b_table = -0x74cc, rows_1c_count = -0x7a38,
                       rows_1c_table = -0x7a34, rows_1c_b_count = -0x751c, rows_1c_b_table = -0x7518,
                       rows_38_count = -0x7558, rows_38_table = -0x7538, rows_34_count = -0x74c0,
                       rows_34_table = -0x74bc;
}
// The index file's entry (0x44 bytes): a 0x33-byte name and four words.
namespace entry {
constexpr std::uint32_t size = 0x44, name_length = 0x33, kind = 0x34, count = 0x38, offset = 0x3c, flag = 0x40;
constexpr std::uint32_t count_of_entries = 0x16, highest_kind = 0x16;
}
// The frame's locals (offsets from the new r1).
namespace local {
constexpr std::uint32_t extended = 0x38, progress_text = 0x62c, found_name = 0xf2c, finder = 0x96c, path = 0xa6c,
                        second_path = 0x86c, alternate_name = 0x76c, language_block = 0xd60, map_count = 0x168,
                        step = 0x1270, swapped = 0x1272, single_block = 0x130f, rows_14 = 0x125c,
                        rows_3c_b = 0x1254, rows_1c = 0x1258, rows_44 = 0x1260, rows_48 = 0x1264,
                        people = 0x1268, clubs = 0x126c, name_buffer = 0x6a4, long_name_buffer = 0xb6c;
}
namespace pool {
constexpr std::uint32_t folder = 0xc7, folder_end = 0xd0, empty = 0xd0, path = 0xc1, text = 0xd1, alternate = 0xe2,
                        alternate_suffix = 0xec, flush_text = 0xf1;
}

// A pair of addresses for the original's two bodies (packed and plain): the `bl` of a call site, or the two routines a read calls.
struct Pair {
    std::uint32_t packed, plain;
};
// The two targets of a read (packed routine, plain routine): same shape as a call site's pair.
constexpr Pair reader{callee::read_packed, callee::read_plain};
constexpr Pair rereader{callee::reread_packed, callee::reread_plain};

// An assertion site of the allocation checks: the first call's address, the two stack buffers and the line.
struct Assert {
    std::uint32_t at, name, extension, line;
};

// Offsets of a table handler's calls from its address (the packed and the plain body of each).
struct Layout {
    std::uint32_t open_packed, open_plain, seek_packed, seek_plain, reader, fail_packed, fail_plain, ok_packed, ok_plain;
};
// One table pass: the handler's address, the table's pointer (its test and the reader's third argument), the
// reader, the first of the three stack slots the closes use, and the handler's layout.
struct Pass {
    std::uint32_t base, value, reader, slot;
    const Layout* layout;
};

class Loader {
public:
    Loader(guest::Cpu& cpu, guest::Frame& f) : c(cpu), frame(f) {}
    void run(std::uint32_t name, std::uint32_t single_block, std::uint32_t packed);

private:
    enum class Outcome { skipped, open_failed, read_failed, read_ok };

    std::uint32_t pool() const { return c.r(25); }
    std::uint32_t ext(std::uint32_t byte) const { return guest::extended(std::int16_t(std::int8_t(byte))); }
    std::uint32_t entry_at(std::uint32_t i) const { return c.r(21) + i * entry::size; }
    void call(std::uint32_t bl, std::uint32_t target) { c.call(target, bl + 4); }

    // database_load.cpp: the index file.
    bool packed();
    void progress(std::uint32_t bl, bool advance = true);
    std::uint32_t read(const Pair& r, Pair site, std::uint32_t destination, std::uint32_t size, std::uint32_t count);
    void close_file(std::uint32_t slot, Pair site);
    bool read_word(std::uint32_t at, Pair site, Pair close, std::uint32_t slot, const Pair& r = reader);
    void swap_word(std::uint32_t at);
    bool open_files();
    bool read_index();
    bool read_language_block();
    // database_load_counts.cpp: the sizing or allocating pass.
    bool each_kind_jump(std::int32_t table_slot, std::uint32_t kind);
    void count_kinds(bool sizing);
    void allocate_kind(std::uint32_t kind, std::uint32_t entry_at);
    void size_kind(std::uint32_t kind, std::uint32_t entry_at);
    void grow(std::int32_t cell_slot, std::uint32_t count, bool add);
    void table_for(std::int32_t table_slot, std::uint32_t bl, std::uint32_t bytes, const Assert& a);
    void assertion(const Assert& a);
    // database_load_defaults.cpp: the single block and the default records.
    void sizing_totals();
    void carve();
    void placeholder_clubs();
    void default_rows_biased();
    void default_rows();
    void date_default(std::uint32_t bl, std::uint32_t local, std::uint32_t destination);
    // database_load_tables.cpp: the readers.
    bool load_tables();
    Outcome open_and_read(const Pass& p);
    void close_read(const Pass& p);
    bool load_required(const Pass& p);
    bool load_optional(const Pass& p, std::int32_t table_slot, std::int32_t count_slot, std::uint32_t free_bl,
                       bool clears_when_null);
    bool load_kind_6();
    bool load_kind_16();
    // database_load_finish.cpp: the closing re-read and the fix-up.
    bool reread_index();
    void sizes_and_indexes();
    std::uint32_t pool_sum();
    void draw_club_record(std::uint32_t person);
    void person_name_at(std::uint32_t person, std::uint32_t buffer, std::uint32_t limit, std::uint32_t bl);
    void report_line(std::uint32_t bl, std::uint32_t format, std::uint32_t buffer);
    void check_role(std::uint32_t person, std::uint32_t offset, bool second);
    void fix_person(std::uint32_t person);
    void fix_people();

    guest::Cpu& c;
    guest::Frame& frame;
};
}
