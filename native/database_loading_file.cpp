// DATABASE.CPP's loading block, part "file": 0x100b6050, the language file loader (database_loading.hpp).
//
// One frame of 0x600 bytes saving r13-r31. Held across the calls: r17 the language, r18 the file, r19 the mode
// (1 or 0 for English with or without a second argument, 2 for every other language, which loads English first),
// r20-r31 the TOC words of the tables (the club table r31 with its count r30, the person table r22, ...), r13 and
// r14 (r15 in one loop) the block or the record index of the loop in progress. The original is the file's header
// check followed by eight sections, one per array; each reads the whole array into one allocation and merges
// it, or, when the allocation does not answer, merges it record by record through a buffer in the frame (and
// seeks where it must). Line numbers in the comments are the original's instruction numbers from 0x100b6050
// (line n is at 0x100b6050 + 4 * (n - 1)): every call's return address derives from its line.
#include "database_loading.hpp"
#include "database_loading_detail.hpp"
#include "guest_values.hpp"
#include <array>
#include <bit>

namespace cm::aot::database {
namespace {
using guest::compare;
using guest::compare_logical;
using guest::extended;
namespace callee = loading::callee;
namespace toc = loading::toc;
namespace layout = loading::object;

constexpr std::uint32_t entry = 0x100b6050;
// The return address of the call on instruction `line`.
constexpr std::uint32_t after(unsigned line) { return entry + 4 * line; }

// Frame offsets: the file's header (eight words), the path and the status text, and one record buffer per array.
namespace local {
constexpr std::uint32_t header = 0xb0, path = 0x474, status = 0x3ac;
}

std::uint32_t ext(std::uint32_t byte) { return extended(std::int16_t(std::int8_t(byte))); }

class Loader {
public:
    Loader(Cpu& cpu, guest::Frame& f) : c(cpu), frame(f) {}
    void run(std::uint32_t language, std::uint32_t second);

private:
    // One array's file section: where its header count and records are, and how many bytes a record has.
    struct Section {
        std::uint32_t count_offset, record_size, buffer;
        unsigned alloc_line, read_line, free_line, stream_read_line;
        std::uint32_t stream_bound_offset;  // the header word that bounds the record-by-record loop
    };

    std::uint32_t header_word(std::uint32_t offset) { return c.u32(frame.at(local::header + offset)); }
    bool present(std::uint32_t at) { return ext(c.u8(at)) != 0; }  // lbz; extsb.; beq
    std::uint32_t pool() { return c.r(29); }

    void select_file(std::uint32_t language);
    bool open_and_check();
    void swap_header();
    bool header_matches();
    void assertion(unsigned first_call, std::uint32_t name, std::uint32_t extension, std::uint32_t line);
    void progress(unsigned line, std::uint32_t high, std::uint32_t low);
    void copy(std::uint32_t destination, std::uint32_t source, std::uint32_t length, unsigned line) {
        c.set({{3, destination}, {4, source}, {5, length}});
        c.call(callee::copy_limited, after(line));
    }
    std::uint32_t clamp(std::uint32_t at) {  // a byte kept only when positive as a signed byte; returns it
        const auto value = ext(c.u8(at));
        const auto kept = std::int32_t(value) > 0 ? value : 0;
        c.put8(at, kept);
        return kept;
    }
    std::uint32_t clamped(std::uint32_t at) {
        const auto value = ext(c.u8(at));
        return std::int32_t(value) > 0 ? value : 0;
    }
    void clubs();
    void club_row(std::uint32_t club, std::uint32_t source, std::uint32_t index, const std::array<unsigned, 6>& lines);
    void clubs_bulk(bool mapped);
    void clubs_stream(bool mapped);
    void person_row(std::uint32_t row, std::uint32_t source, const std::array<unsigned, 4>& lines);
    void section_2();
    void section_3();
    void section_4();
    void section_5();
    void section_6();
    void section_7();
    void section_8();
    void finish();

    // The generic bulk/stream driver of sections 2 to 8: `dest(index)` is the destination row of record `index`,
    // `merge(row, source, bulk)` merges one record.
    template <class Dest, class MergeRow>
    void table_section(const Section& s, Dest&& dest, MergeRow&& merge);

    Cpu& c;
    guest::Frame& frame;
};

void Loader::assertion(unsigned first_call, std::uint32_t name, std::uint32_t extension, std::uint32_t line) {
    guest::report_assertion(c, {entry + 4 * (first_call - 1), pool(), pool() + 0xd, pool() + 0x1a, frame.at(name),
                                frame.at(extension), line});
    c.set({{3, c.toc(toc::success_flag)}, {0, 0}});
}

// The status call between sections: r3 the status word, r4 the text, r5 a step number (the high number when the
// mode is 2 or more), r6 0x10 or 8 by the mode, r7 -1.
void Loader::progress(unsigned line, std::uint32_t high, std::uint32_t low) {
    const auto mode = ext(c.r(19));
    c.set(0, mode);
    const auto flag = compare(c, std::int32_t(mode));
    c.set(6, flag != 0 ? 0x10 : 8);
    c.set(0, mode);
    c.set({{3, c.toc(-0x7bb0)}, {4, frame.at(local::status)}});
    c.set(5, high);
    if (compare(c, std::int32_t(mode), 2) < 0) c.set(5, low);
    c.set(7, 0xffffffff);
    c.call(callee::show_progress, after(line));
}

// Lines 36-184: the file's name for the language and, for every language but English, English loaded first.
void Loader::select_file(std::uint32_t language) {
    constexpr std::array<std::uint32_t, 11> names{0x846, 0x846, 0x836, 0x83e, 0x84e, 0x866, 0x876, 0x82e, 0x85e, 0x856, 0x86e};
    // Block start line per language; 0 and 1 are English's own block, 154 the default.
    constexpr std::array<unsigned, 11> blocks{69, 69, 47, 58, 88, 121, 143, 36, 110, 99, 132};
    const auto number = ext(language);
    c.set(0, number);
    unsigned start = 154;
    if (compare_logical(c, number, 10) <= 0) {
        const auto table = c.toc(-0x5630);
        c.set(0, number << 2);
        c.set(3, c.u32(table + (number << 2)));
        c.s.wr<4>(guest::reg::ctr, c.r(3));  // mtspr CTR,r3; bctr
        start = blocks[number];
    }
    if (start == 154) {
        assertion(159, 0x90, 0x48, 0x1df2);
        c.set({{0, 0}, {3, 0}, {4, 1}});
        guest::enter(c, after(177), {0, 1});
        load_language_file(c, 0, 1);
        c.set({{3, pool() + loading::pool::folder}, {4, pool() + 0x836}});
        c.call(callee::data_folder_path, after(180));
        c.set({{4, c.r(3)}, {3, frame.at(local::path)}});
        c.call(callee::copy_text, after(183));
        c.set(19, 2);
        return;
    }
    if (start == 69) {  // English
        c.set({{3, pool() + loading::pool::folder}, {4, pool() + names[number]}});
        c.call(callee::data_folder_path, after(71));
        c.set({{4, c.r(3)}, {3, frame.at(local::path)}});
        c.call(callee::copy_text, after(74));
        const auto object_slot = c.r(2) + std::uint32_t(toc::language_object);
        const auto object = c.u32(object_slot);
        c.set(0, object);
        const auto second = ext(c.r(13));
        c.set(4, second);
        c.set(3, 0 - second);
        compare_logical(c, object);
        c.set(0, c.r(3) | c.r(4));
        c.set(19, c.r(0) >> 31);
        if (object != 0) {
            c.set({{3, c.u32(object_slot)}, {4, 1}});
            guest::enter(c, after(84), {c.r(3), 1});
            release_language_object(c, c.r(3), 1);
            c.set(0, 0);
            c.put32(object_slot, 0);
        }
        return;
    }
    // The other languages: English first (language 0, second argument 1), then their own name.
    guest::enter(c, after(start + 2), {0, 1});
    load_language_file(c, 0, 1);
    c.set({{3, pool() + loading::pool::folder}, {4, pool() + names[number]}});
    c.call(callee::data_folder_path, after(start + 5));
    c.set({{4, c.r(3)}, {3, frame.at(local::path)}});
    c.call(callee::copy_text, after(start + 8));
    c.set(19, 2);
}

// Lines 244-343: the header in the other byte order. The compiler read each word several times and spilled
// the copies; the stack words and the registers it leaves (r0, r3-r16) are what the checker compares.
void Loader::swap_header() {
    std::array<std::uint32_t, 8> raw{};
    for (std::uint32_t i = 0; i < 8; ++i) raw[i] = header_word(4 * i);
    const auto swapped = [&](unsigned i) { return std::byteswap(raw[i]); };
    // Spill slots (offsets from the new stack pointer) and the raw word each holds.
    struct Spill {
        std::uint32_t offset;
        unsigned word;
    };
    constexpr std::array<Spill, 14> spills{{{0x574, 1}, {0x578, 2}, {0x57c, 3}, {0x580, 4}, {0x584, 5}, {0x588, 5}, {0x58c, 6},
                                            {0x590, 6}, {0x594, 7}, {0x598, 7}, {0x59c, 7}, {0x5a0, 0}, {0x5a4, 1}, {0x5a8, 5}}};
    for (const auto& [offset, word] : spills) c.put32(frame.at(offset), raw[word]);
    for (std::uint32_t i = 0; i < 8; ++i) c.put32(frame.at(local::header + 4 * i), swapped(i));
    c.set({{0, swapped(7)}, {3, swapped(6)}, {4, swapped(5)}, {5, swapped(4)}, {6, raw[7]}, {7, swapped(3)}, {8, raw[3]},
           {9, swapped(2)}, {10, raw[4]}, {11, swapped(1)}, {12, swapped(0)}, {13, raw[1]}, {14, raw[6]}, {15, raw[0]},
           {16, raw[0]}});
}

// Lines 344-374: the header's counts against the tables' counts.
bool Loader::header_matches() {
    // A check through a TOC slot: r3 the word's address, r4 the header word, r0 the expected count.
    const auto against = [&](std::int32_t slot, std::uint32_t offset) {
        const auto expected_at = c.toc(slot);
        c.set({{3, expected_at}, {4, header_word(offset)}});
        c.set(0, c.u32(expected_at));
        c.compare(0, std::int32_t(c.r(4)), std::int32_t(c.r(0)));
        return c.r(4) == c.r(0);
    };
    if (!against(-0x7524, 0x4)) return false;
    {  // The second check compares through r27 and leaves r4 as the first check set it: r3 the header word.
        c.set({{3, header_word(0xc)}, {0, c.u32(c.r(27))}});
        c.compare(0, std::int32_t(c.r(3)), std::int32_t(c.r(0)));
        if (c.r(3) != c.r(0)) return false;
    }
    if (!against(-0x7558, 0x10)) return false;
    c.set(4, c.u32(c.r(20)));
    c.set({{3, header_word(0x14)}, {0, header_word(0x8)}});
    c.set(4, c.r(4) - 0x7f);
    c.set(0, c.r(3) + c.r(0));
    c.compare(0, std::int32_t(c.r(4)), std::int32_t(c.r(0)));
    if (c.r(4) != c.r(0)) return false;
    return against(-0x7568, 0x18) && against(-0x755c, 0x1c);
}

// Lines 200-398: open the file, read the 32-byte header, swap it when the signature differs, check it.
bool Loader::open_and_check() {
    constexpr std::uint32_t signature = 0xf1fffff1;
    c.set({{3, frame.at(local::path)}, {4, pool() + loading::pool::mode_read}});
    c.call(callee::open_stream, after(202));
    const auto file = c.r(3);
    c.set(18, file);
    if (compare(c, std::int32_t(file)) == 0) {
        assertion(210, 0x70, 0x40, 0x1e03);
        return false;
    }
    c.set(3, file);
    c.call(callee::read_file_marker, after(228));
    const auto marker = c.r(3);
    const auto different = ((signature - marker) | (marker - signature)) >> 31;
    c.set(13, different);  // extsb r13,r0
    c.set({{0, different}, {3, frame.at(local::header)}, {4, 0x20}, {5, 1}, {6, file}});
    c.call(callee::read_block, after(241));
    c.set(0, ext(c.r(13)));
    if (compare(c, std::int32_t(c.r(0))) != 0) swap_header();
    if (!header_matches()) {
        c.set(3, c.r(18));
        c.call(callee::close_stream, after(376));
        assertion(382, 0x50, 0x38, 0x1e23);
        return false;
    }
    return true;
}

// The block of one array, record by record where the allocation does not answer. `dest(index, bulk)` gives the
// destination row for record `index` (also setting the registers the original keeps it in), `merge` merges one
// record. The bulk loop's index lives in r14 and the stream loop's in r13. Section 2's stream loop is bounded
// by the first array's count where its bulk loop uses its own (a quirk of the original, kept).
template <class Dest, class MergeRow>
void Loader::table_section(const Section& s, Dest&& dest, MergeRow&& merge) {
    const auto count = header_word(s.count_offset);
    c.set({{0, count}, {3, count * s.record_size}});
    c.call(callee::allocate, after(s.alloc_line));
    const auto block = c.r(3);
    c.set(13, block);
    if (compare(c, std::int32_t(block)) != 0) {
        c.set({{5, header_word(s.count_offset)}, {3, block}, {6, c.r(18)}, {4, s.record_size}});
        c.call(callee::read_block, after(s.read_line));
        c.set(14, 0);
        for (;;) {
            const auto bound = header_word(s.count_offset);
            c.set(0, bound);
            if (compare(c, std::int32_t(c.r(14)), std::int32_t(bound)) >= 0) break;
            merge(dest(c.r(14), true), block + c.r(14) * s.record_size, true);
            c.set(14, c.r(14) + 1);
        }
        c.set(3, c.r(13));
        c.call(callee::release, after(s.free_line));
        return;
    }
    c.set(13, 0);
    for (;;) {
        const auto bound = header_word(s.stream_bound_offset);
        c.set(0, bound);
        if (compare(c, std::int32_t(c.r(13)), std::int32_t(bound)) >= 0) break;
        c.set({{6, c.r(18)}, {3, frame.at(s.buffer)}, {4, s.record_size}, {5, 1}});
        c.call(callee::read_block, after(s.stream_read_line));
        merge(dest(c.r(13), false), frame.at(s.buffer), false);
        c.set(13, c.r(13) + 1);
    }
}

// Lines 437-653 and their three variants: one club row from one name record. `lines` are the calls' lines in
// order: the name comparison, the length test, the copy into the second name, the copy into the first, the
// copy from the record's second name and the copy of the first name into the second.
void Loader::club_row(std::uint32_t club, std::uint32_t source, std::uint32_t index, const std::array<unsigned, 6>& lines) {
    constexpr std::uint32_t first_name = 0x1ee, second_name = 0x221, first_flag = 0x242, second_flag = 0x23b, cleared = 0x23a;
    if (!present(source + 0x34) && present(source)) {
        c.set({{3, club + first_name}, {4, club + second_name}});
        c.call(callee::compare_text, after(lines[0]));
        if (compare(c, std::int32_t(c.r(3))) == 0) {
            c.set(3, source);
            c.call(callee::text_length, after(lines[1]));
            if (compare_logical(c, c.r(3), 0x1a) < 0) copy(club + second_name, source, 0x1a, lines[2]);
        }
    }
    if (present(source)) copy(club + first_name, source, 0x33, lines[3]);
    c.put8(club + first_flag, c.u8(source + 0x33));
    if (present(source + 0x34)) {
        copy(club + second_name, source + 0x34, 0x1a, lines[4]);
        c.put8(club + second_flag, c.u8(source + 0x4e));
    } else {
        const auto limit = c.u32(c.r(30)) - (c.u32(c.r(27)) << 1);  // the club count less twice a word
        if (std::int32_t(index) < std::int32_t(limit)) {
            if (c.i8(club + second_name) == c.i8(club + first_name) && c.i8(source + 0x4e) == -1)
                c.put8(club + second_flag, c.u8(club + first_flag));
            else
                c.put8(club + second_flag, c.u8(source + 0x4e));
        } else {
            copy(club + second_name, club + first_name, 0x1a, lines[5]);
            c.put8(club + cleared, 0);
            c.put8(club + second_flag, c.u8(club + first_flag));
        }
    }
    if (ext(c.r(17)) == 5) {  // language 5 keeps -1 as a sentinel
        if (c.i8(club + first_flag) == -1) c.put8(club + first_flag, 1);
        if (c.i8(club + second_flag) == -1) c.put8(club + second_flag, c.u8(club + first_flag));
    } else {
        clamp(club + first_flag);
        clamp(club + second_flag);
    }
}

constexpr std::array<unsigned, 6> club_lines_bulk_mapped{437, 445, 458, 476, 506, 533}, club_lines_stream_mapped{689, 693, 702, 712, 727, 749},
    club_lines_bulk_direct{899, 904, 914, 926, 947, 971}, club_lines_stream_direct{1113, 1117, 1126, 1136, 1151, 1173};

void Loader::clubs_bulk(bool mapped) {
    constexpr std::uint32_t row_size = 0x248, record_size = layout::clubs_4f.record_size;
    const unsigned alloc = mapped ? 404 : 872, read = mapped ? 411 : 879, release = mapped ? 658 : 1090;
    const auto count = header_word(layout::clubs_4f.count);
    c.set({{0, count}, {3, count * record_size}});
    c.call(callee::allocate, after(alloc));
    const auto block = c.r(3);
    c.set(13, block);
    if (compare(c, std::int32_t(block)) == 0) return clubs_stream(mapped);
    c.set({{5, header_word(layout::clubs_4f.count)}, {3, block}, {6, c.r(18)}, {4, record_size}});
    c.call(callee::read_block, after(read));
    c.set(14, 0);
    for (;;) {
        // The mapped loop runs over the club count, the direct one over the header's count.
        const auto bound = mapped ? c.u32(c.r(30)) : header_word(layout::clubs_4f.count);
        c.set(0, bound);
        if (std::int32_t(c.r(14)) >= std::int32_t(bound)) break;
        const auto index = c.r(14);
        const auto record = mapped ? c.u32(c.u32(c.r(26)) + 4 * index) : index;
        club_row(c.u32(c.r(31)) + index * row_size, block + record * record_size, index,
                 mapped ? club_lines_bulk_mapped : club_lines_bulk_direct);
        c.set(14, index + 1);
    }
    c.set(3, c.r(13));
    c.call(callee::release, after(release));
}

void Loader::clubs_stream(bool mapped) {
    constexpr std::uint32_t row_size = 0x248, record_size = layout::clubs_4f.record_size, buffer = 0x2a0;
    c.set(13, 0);
    for (;;) {
        const auto bound = header_word(layout::clubs_4f.count);
        c.set(0, bound);
        if (std::int32_t(c.r(13)) >= std::int32_t(bound)) break;
        const auto index = c.r(13);
        if (mapped) {
            const auto record = c.u32(c.u32(c.r(26)) + 4 * index);
            c.set({{3, c.r(18)}, {5, 0}, {4, record * record_size + 0x20}});
            c.call(callee::seek_stream, after(669));
        }
        c.set({{6, c.r(18)}, {3, frame.at(buffer)}, {4, record_size}, {5, 1}});
        c.call(callee::read_block, after(mapped ? 674 : 1098));
        club_row(c.u32(c.r(31)) + index * row_size, frame.at(buffer), index,
                 mapped ? club_lines_stream_mapped : club_lines_stream_direct);
        c.set(13, index + 1);
    }
    if (mapped) {  // after the last record the file is positioned past the array
        c.set({{3, c.r(18)}, {5, 0}, {4, header_word(layout::clubs_4f.count) * record_size + 0x20}});
        c.call(callee::seek_stream, after(868));
    }
}

void Loader::clubs() {
    const auto map_word = c.u32(c.r(26));
    c.set(0, map_word);
    clubs_bulk(compare_logical(c, map_word) != 0);
}

// Sections 3 and 8: a person-table row (0x6c bytes) from a 0x53-byte record.
void Loader::person_row(std::uint32_t row, std::uint32_t source, const std::array<unsigned, 4>& lines) {
    if (present(source)) copy(row + 0x1a, source, 0x33, lines[0]);
    c.put8(row + 0x6a, c.u8(source + 0x33));
    if (present(source + 0x34)) {
        copy(row + 0x4d, source + 0x34, 0x1a, lines[1]);
        c.put8(row + 0x67, c.u8(source + 0x4e));
    } else {
        copy(row + 0x4d, row + 0x1a, 0x1a, lines[2]);
        c.put8(row + 0x66, 0);
        c.put8(row + 0x67, c.u8(row + 0x6a));
    }
    if (present(source + 0x4f)) copy(row + 0x14, source + 0x4f, 4, lines[3]);
    clamp(row + 0x6a);
    c.set(4, clamp(row + 0x67));  // the register that holds the value stays: the last record's r4 reaches the close
}

void Loader::section_2() {  // 0xba-byte records into the 0xc8-byte rows of the table at TOC -0x7520
    constexpr std::array<unsigned, 5> bulk{1323, 1344, 1358, 1372, 1393}, stream{1441, 1456, 1466, 1476, 1491};
    table_section({layout::rows_ba_to_c8.count, layout::rows_ba_to_c8.record_size, 0x2f0, 1302, 1309, 1423, 1431,
                   layout::clubs_4f.count /* the quirk: the first array's count */},
                  [&](std::uint32_t index, bool) { return c.u32(c.r(25)) + index * 0xc8; },
                  [&](std::uint32_t row, std::uint32_t source, bool is_bulk) {
                      const auto& lines = is_bulk ? bulk : stream;
                      if (present(source)) copy(row + 0x10, source, 0x1a, lines[0]);
                      c.put8(row + 0xc3, c.u8(source + 0x1a));
                      if (present(source + 0x1f)) copy(row + 0x2a, source + 0x1f, 0x1a, lines[1]);
                      if (present(source + 0x1b)) copy(row + 0xc, source + 0x1b, 4, lines[2]);
                      if (present(source + 0x39)) copy(row + 0x44, source + 0x39, 0x65, lines[3]);
                      c.put8(row + 0xc4, c.u8(source + 0x9e));
                      if (present(source + 0x9f)) copy(row + 0xa9, source + 0x9f, 0x1a, lines[4]);
                      c.put8(row + 0xc5, c.u8(source + 0xb9));
                      clamp(row + 0xc3);
                  });
}

void Loader::section_3() {  // 0x53-byte records appended to the person table at TOC -0x7bbc
    table_section({layout::people_53_appended.count, layout::people_53_appended.record_size, 0x24c, 1533, 1540, 1665, 1673,
                   layout::people_53_appended.count},
                  [&](std::uint32_t index, bool is_bulk) {
                      // The row index: the person count less 0x7f and the section's count, plus the record's.
                      const auto row_index = c.u32(c.r(20)) - 0x7f + index - header_word(0x8);
                      c.set(is_bulk ? 15 : 14, row_index);
                      return c.u32(c.r(22)) + row_index * 0x6c;
                  },
                  [&](std::uint32_t row, std::uint32_t source, bool is_bulk) {
                      person_row(row, source, is_bulk ? std::array<unsigned, 4>{1559, 1580, 1598, 1625}
                                                      : std::array<unsigned, 4>{1688, 1703, 1719, 1742});
                  });
}

void Loader::section_4() {  // 0x6d-byte records into the 0x128-byte rows of the table at TOC -0x7b98
    constexpr std::array<unsigned, 5> bulk{1817, 1838, 1856, 1883, 1897}, stream{1955, 1970, 1986, 2009, 2019};
    table_section({layout::rows_6d_to_128.count, layout::rows_6d_to_128.record_size, 0x1dc, 1796, 1803, 1937, 1945,
                   layout::rows_6d_to_128.count},
                  [&](std::uint32_t index, bool) { return c.u32(c.r(28)) + index * 0x128; },
                  [&](std::uint32_t row, std::uint32_t source, bool is_bulk) {
                      const auto& lines = is_bulk ? bulk : stream;
                      if (present(source)) copy(row + 0xae, source, 0x33, lines[0]);
                      c.put8(row + 0x115, c.u8(source + 0x33));
                      if (present(source + 0x34)) {
                          copy(row + 0xe1, source + 0x34, 0x1a, lines[1]);
                          c.put8(row + 0x116, c.u8(source + 0x4e));
                      } else {
                          copy(row + 0xe1, row + 0xae, 0x1a, lines[2]);
                          c.put8(row + 0xfa, 0);
                          c.put8(row + 0x116, c.u8(row + 0x115));
                      }
                      if (present(source + 0x4f)) copy(row + 0xa4, source + 0x4f, 4, lines[3]);
                      if (present(source + 0x53)) copy(row + 0xfb, source + 0x53, 0x1a, lines[4]);
                      clamp(row + 0x115);
                      clamp(row + 0x116);
                  });
}

void Loader::section_5() {  // 0x34-byte records into the 0x50-byte rows of the table at TOC -0x7548
    table_section({layout::rows_34_to_50.count, layout::rows_34_to_50.record_size, 0xd0, 2073, 2080, 2115, 2123,
                   layout::rows_34_to_50.count},
                  [&](std::uint32_t index, bool) { return c.u32(c.r(21)) + index * 0x50; },
                  [&](std::uint32_t row, std::uint32_t source, bool is_bulk) {
                      if (present(source)) copy(row + 0x18, source, 0x33, is_bulk ? 2094 : 2133);
                      c.put8(row + 0x4b, clamped(source + 0x33));
                  });
}

void Loader::section_6() {  // 0x34-byte records into the 0x38-byte rows of the table at TOC -0x7538
    table_section({layout::rows_34_to_38.count, layout::rows_34_to_38.record_size, 0x104, 2164, 2171, 2206, 2214,
                   layout::rows_34_to_38.count},
                  [&](std::uint32_t index, bool) { return c.u32(c.r(24)) + index * 0x38; },
                  [&](std::uint32_t row, std::uint32_t source, bool is_bulk) {
                      if (present(source)) copy(row + 0x1c, source, 0x1a, is_bulk ? 2185 : 2224);
                      c.put8(row + 0x36, clamped(source + 0x33));
                  });
}

void Loader::section_7() {  // 0x4f-byte records into the 0x68-byte rows of the table at TOC -0x753c
    constexpr std::array<unsigned, 3> bulk{2276, 2297, 2315}, stream{2386, 2401, 2417};
    table_section({layout::rows_4f_to_68.count, layout::rows_4f_to_68.record_size, 0x138, 2255, 2262, 2368, 2376,
                   layout::rows_4f_to_68.count},
                  [&](std::uint32_t index, bool) { return c.u32(c.r(23)) + index * 0x68; },
                  [&](std::uint32_t row, std::uint32_t source, bool is_bulk) {
                      const auto& lines = is_bulk ? bulk : stream;
                      if (present(source)) copy(row + 0x30, source, 0x33, lines[0]);
                      c.put8(row + 0x64, c.u8(source + 0x33));
                      if (present(source + 0x34)) {
                          copy(row + 0x16, source + 0x34, 0x1a, lines[1]);
                          c.put8(row + 0x63, c.u8(source + 0x4e));
                      } else {
                          copy(row + 0x16, row + 0x30, 0x1a, lines[2]);
                          c.put8(row + 0x2f, 0);
                          c.put8(row + 0x63, c.u8(row + 0x64));
                      }
                      clamp(row + 0x64);
                      clamp(row + 0x63);
                  });
}

void Loader::section_8() {  // 0x53-byte records merged into the person rows from the first
    table_section({layout::people_53.count, layout::people_53.record_size, 0x188, 2484, 2491, 2611, 2619,
                   layout::people_53.count},
                  [&](std::uint32_t index, bool) { return c.u32(c.r(22)) + index * 0x6c; },
                  [&](std::uint32_t row, std::uint32_t source, bool is_bulk) {
                      person_row(row, source, is_bulk ? std::array<unsigned, 4>{2505, 2526, 2544, 2571}
                                                      : std::array<unsigned, 4>{2629, 2644, 2660, 2683});
                  });
}

// Lines 2722-2755: close the file, the last status, the language byte and the language service object.
void Loader::finish() {
    c.set(3, c.r(18));
    c.call(callee::close_stream, after(2723));
    progress(2736, 0x10, 8);
    const auto service_slot = c.toc(-0x7570);
    c.set(3, service_slot);
    const auto service = c.u32(service_slot);
    c.set(0, service);
    const auto language_byte = c.toc(toc::language_byte);
    c.set(3, language_byte);
    compare_logical(c, service);
    c.put8(language_byte, c.r(17));
    if (service == 0) return;
    c.set({{3, service_slot}, {4, 1}});
    c.set(3, c.u32(c.r(3)));
    c.call(callee::service_release, after(2746));
    c.set(3, 0xc);
    c.call(callee::operator_new, after(2748));
    c.set(13, c.r(3));
    if (compare(c, std::int32_t(c.r(13))) != 0) {
        c.set({{3, c.r(13)}, {4, ext(c.r(17))}});
        c.call(callee::service_construct, after(2753));
    }
    c.set(3, c.toc(-0x7570));
    c.put32(c.r(3), c.r(13));
}

void Loader::run(std::uint32_t language, std::uint32_t second) {
    c.set({{28, c.toc(-0x7b98)}, {17, language}, {20, c.toc(-0x7b64)}, {13, second}, {21, c.toc(-0x7548)},
           {22, c.toc(-0x7bbc)}, {23, c.toc(-0x753c)}, {24, c.toc(-0x7538)}, {25, c.toc(-0x7520)}, {26, c.toc(-0x74f8)},
           {27, c.toc(-0x7b04)}, {29, c.toc(toc::string_pool)}, {30, c.toc(-0x7b50)}, {31, c.toc(-0x7be8)}});
    const auto clubs_word = c.u32(c.r(28));
    c.set(0, clubs_word);
    if (compare_logical(c, clubs_word) == 0) return;  // no club table loaded
    const auto current = ext(c.u8(c.toc(toc::language_byte)));
    const auto wanted = ext(language);
    c.set({{3, current}, {0, wanted}});
    if (compare(c, std::int32_t(current), std::int32_t(wanted)) == 0) return;  // already the current language
    select_file(language);
    c.set({{3, frame.at(local::status)}, {4, pool() + 0x87e}});
    c.call(callee::copy_path, after(187));
    const auto mode = ext(c.r(19));
    c.set(0, mode);
    if (compare(c, std::int32_t(mode), 2) < 0) {  // the first status call, only for the modes below 2
        c.set(0, mode);
        const bool zero = compare(c, std::int32_t(mode)) == 0;
        c.set({{3, c.toc(-0x7bb0)}, {4, frame.at(local::status)}, {5, 0}, {6, zero ? 8 : 0x10}, {7, 0xffffffff}});
        c.call(callee::show_progress, after(199));
    }
    if (!open_and_check()) return;
    clubs();
    progress(1299, 9, 1);
    section_2();
    progress(1530, 0xa, 2);
    section_3();
    progress(1793, 0xb, 3);
    section_4();
    progress(2070, 0xc, 4);
    section_5();
    progress(2161, 0xd, 5);
    section_6();
    progress(2252, 0xe, 6);
    section_7();
    progress(2481, 0xf, 7);
    section_8();
    finish();
}
}

void load_language_file(Cpu& c, std::int32_t language, std::int32_t second) {
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x600, {.first_gpr = 13});
    Loader(c, frame).run(std::uint32_t(language), std::uint32_t(second));
}
}
