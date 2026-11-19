// CUP.CPP 0x104831a0 (vt +0x20): the cup's save reader. It builds "<competition name>_<id>.tmp" (or the year for a
// `backup` of zero) in the cup's own name buffer (+0x4d), opens it in CM3_TEMP, and reads the record field by field.
// Every checked read that comes up short, and every checked allocation that fails, reports one of 26 sites (source
// lines 0x12c0..0x142a), clears the success flag and returns 0; 20 of them first call `exit`, which the game never
// returns from (a case run stops there on both sides). Three of those sites (the three `new[]` whose result is
// stored before it is tested: 0x137e, 0x1398, 0x13c8) go on after `exit` in the original's code, and so do these.
// A failure never closes the file. The club words the file holds (-1: none) become pointers into the club table,
// the competition index a pointer into the competition records. The original's frame is 0x4a0 bytes: the locals
// the file layer fills sit at sp+0x38..0x48, the assertions' two buffers above them (sp+0x4c..0x43c).
// The reader holds r19-r23 as loop counters and the last allocations; the callees can observe them, a replay compares
// r13-r31 at every call that reaches an import, so `Held` sets them as the original has them at each call.
#include "cup_base_storage_detail.hpp"
#include <initializer_list>

namespace cm::aot::cup_base {
namespace {
using namespace storage;
using u32 = std::uint32_t;
using s32 = std::int32_t;

// An assertion site: the address of the report's first call (0x10739890), the frame offsets of its two buffers, the
// source line, the pool text of a "Memory Allocation Error" site (0: the plain assertion) and whether `exit` follows.
struct Site { u32 at; s32 name, extension; u32 line, text; bool exits; };
// In the order they occur in the routine. A plain assertion is 0x24 bytes of calls long, an allocation one 0x28.
inline constexpr std::array<Site, 26> sites{{
    {0x10483298, 0x43c, 0x114, 0x12c0, 0, false},              // 0 the name
    {0x104833e8, 0x41c, 0x10c, 0x12e4, 0, false},              // 1 byte +0x49
    {0x10483470, 0x3fc, 0x104, 0x1307, 0, true},               // 2 byte +0x46
    {0x104834ec, 0x3dc, 0x0fc, 0x130d, 0, true},               // 3 byte +0x47
    {0x10483568, 0x3bc, 0x0f4, 0x1314, 0, false},              // 4 byte +0x45
    {0x104835dc, 0x39c, 0x0ec, 0x1336, 0, true},               // 5 byte +0xb0
    {0x10483658, 0x37c, 0x0e4, 0x133c, 0, true},               // 6 halfword +0x3c
    {0x104836d4, 0x35c, 0x0dc, 0x1342, 0, true},               // 7 halfword +0x3e
    {0x10483750, 0x33c, 0x0d4, 0x1348, 0, true},               // 8 halfword +0x40
    {0x104837cc, 0x31c, 0x0cc, 0x134e, 0, true},               // 9 byte +0xa8
    {0x10483848, 0x2fc, 0x0c4, 0x1354, 0, true},               // 10 byte +0xa9 (fails only on 0)
    {0x104838c4, 0x2dc, 0x0bc, 0x135a, 0, true},               // 11 byte +0x42
    {0x10483940, 0x2bc, 0x0b4, 0x1361, 0, true},               // 12 halfword +0x3a
    {0x104839bc, 0x29c, 0x0ac, 0x1368, 0, true},               // 13 word +0x1c
    {0x10483a38, 0x27c, 0x0a4, 0x136f, 0, true},               // 14 word +0x20
    {0x10483af0, 0x25c, 0x09c, 0x137e, text::load_memory, true},     // 15 the 0x18-byte rows' block
    {0x10483b98, 0x23c, 0x094, 0x1388, text::load_id, true},         // 16 the competition index
    {0x10483c60, 0x21c, 0x08c, 0x1398, text::load_schedule, true},   // 17 the rounds' block
    {0x10483cf8, 0x1fc, 0x084, 0x139f, 0, true},               // 18 the rounds' read
    {0x10483d74, 0x1dc, 0x07c, 0x13a7, 0, true},               // 19 the entrants' block
    {0x10483df0, 0x1bc, 0x074, 0x13af, 0, true},               // 20 the entrants' read
    {0x10483ee8, 0x19c, 0x06c, 0x13c8, 0, true},               // 21 one round's matches' block
    {0x10483f94, 0x17c, 0x064, 0x13d2, 0, true},               // 22 one round's matches' read
    {0x10484078, 0x15c, 0x05c, 0x13e2, text::load_next_stages, true},  // 23 the stages' array
    {0x10484170, 0x13c, 0x054, 0x13fe, text::load_schedule_2, true},   // 24 a list stage's halfwords
    {0x10484330, 0x11c, 0x04c, 0x142a, text::load_teams, true}}};      // 25 the 8-byte rows' block
namespace site {
inline constexpr unsigned name = 0, extra_rows = 15, record_index = 16, rounds_alloc = 17,
    rounds_read = 18, entrants_alloc = 19, entrants_read = 20, matches_alloc = 21, matches_read = 22, stages_alloc = 23,
    list_alloc = 24, rows_alloc = 25;
// The site of each of `fields` (the index of the field), or none: of the first six only +0x49 (the fifth) is checked.
inline constexpr int field_site(unsigned i) { return i == 4 ? 1 : i >= 6 ? int(i) - 4 : -1; }
inline constexpr unsigned zero_fails = 10;  // byte +0xa9: the original fails on a count of 0 only
}
// The frame's locals the file layer writes into (sp+offset).
namespace local {
inline constexpr s32 stage_kind = 0x38, group_second = 0x3a, group_first = 0x3c, list_second = 0x3e, list_first = 0x40,
    stats_flag = 0x44, record_index = 0x48;
}
inline constexpr u32 club_row_size = 0x248, record_size = 0x6c, none = 0xffffffffu;
// The 0x18-byte rows, the 0x78-byte rounds, the 8-byte entrants and the 0x36-byte match rows.
inline constexpr u32 extra_row = 0x18;

// r19-r23 as the original holds them: loop counters, byte offsets and the last allocation.
struct Held { u32 r19, r20, r21, r22, r23; };

class Reader {
public:
    Reader(Cpu& cpu, const Frame& frame, u32 cup, u32 backup)
        : c(cpu), frame(frame), cup(cup), backup(backup), held{c.r(19), c.r(20), c.r(21), c.r(22), c.r(23)},
          clubs_cell(c.toc(toc::clubs)), pool(c.toc(toc::pool)), success(c.toc(toc::success)),
          message(c.toc(toc::message)), title(c.toc(dialog_title_slot)), text_buffer(c.toc(dialog_text_slot)) {}
    u32 run();

private:
    void publish() { c.set({{19, held.r19}, {20, held.r20}, {21, held.r21}, {22, held.r22}, {23, held.r23}}); }
    // `bl routine` returning to `back`, r3.. the arguments, r19-r23 in place.
    u32 call(u32 routine, u32 back, std::initializer_list<u32> arguments) {
        publish();
        return invoke(c, routine, back, arguments);
    }
    u32 read(u32 to, u32 size, u32 count, u32 back) { return call(routine::read_file, back, {to, size, count, file}); }
    u32 allocate(u32 bytes, u32 back) { return call(routine::new_block, back, {bytes}); }
    // The report of a failed check (the plain assertion, or the "Memory Allocation Error" dialog of the sites that
    // have a text), then `exit` for the sites that call it. The caller returns 0 or goes on, as the original.
    void fail(unsigned n);
    u32 club_pointer(u32 index) const { return c.u32(clubs_cell) + index * club_row_size; }
    u32 at(u32 offset) const { return cup + offset; }
    s32 half(u32 offset) const { return c.i16(cup + offset); }

    void rows_to_pointers(u32 block_field, u32 count_field);
    bool open();
    bool header();
    bool entrants();
    bool matches();
    bool stages();
    bool round_rows();

    Cpu& c;
    const Frame& frame;
    u32 cup, backup;
    Held held;
    u32 clubs_cell, pool, success, message, title, text_buffer, file = 0;
};

void Reader::fail(unsigned n) {
    const Site& s = sites[n];
    publish();
    if (s.text == 0) {
        report(c, frame, {s.at, s.name, s.extension, s.line});
    } else {
        // The memory dialog: the assertion's two buffers and the version, the message with the pool text, then the
        // localized title and text (two words of the TOC) shown by the error dialog.
        c.set({{3, pool}, {4, 0}, {5, 0}, {6, frame.at(s.name)}, {7, frame.at(s.extension)}});
        c.call(routine::split_path, s.at + 4);
        c.call(routine::version_text, s.at + 8);
        c.set({{5, c.r(3)}, {3, message}, {4, pool + text::report_format_text}, {6, frame.at(s.name)},
               {7, frame.at(s.extension)}, {8, s.line}, {9, pool + s.text}});
        c.call(routine::format, s.at + 0x28);
        invoke(c, routine::localized, s.at + 0x34, {title, pool + text::memory_title});
        c.set({{3, text_buffer}, {5, message}, {4, pool + text::memory_message}});
        c.call(routine::localized, s.at + 0x44);
        invoke(c, routine::error_dialog, s.at + 0x54, {title, text_buffer, 0});
    }
    c.put8(success, 0);
    if (s.exits) invoke(c, callee::exit, s.at + (s.text ? 0x64 : 0x44), {none});
}

// "<competition name>_<id>.tmp" (a nonzero `backup` byte) or "<competition name>_<year>.tmp", '/' replaced by '_',
// in CM3_TEMP, opened "rb". The name goes into the cup's own 0x50-byte buffer at +0x4d, which the first read then
// overwrites.
bool Reader::open() {
    const auto name = at(0x4d);
    const auto record = c.u32(cup + competition::record);
    c.set({{24, clubs_cell}, {25, title}, {27, cup}, {28, text_buffer}, {29, success}, {30, message}, {31, pool}});
    publish();
    if (sext8(backup) != 0)
        invoke(c, routine::format, 0x104831ec, {name, pool + text::temporary_name, record + 0x1a, c.u32(record)});
    else
        invoke(c, routine::format, 0x10483208, {name, pool + text::temporary_name, record + 0x1a, sext16(c.u16(at(competition::year)))});
    held.r23 = 0;
    for (; c.u8(name + held.r23) != 0; ++held.r23)
        if (c.u8(name + held.r23) == '/') c.put8(name + held.r23, '_');
    const auto path = call(routine::file_path, 0x10483250, {pool + text::temporary_folder, name});
    file = call(routine::open_file, 0x10483258, {path, pool + text::read_mode});
    c.set(26, file);
    return file != 0;
}

// The name, the statistics object (the old one is freed first), the derived cup's own records (virtual +0x28), the
// fixed fields, the 0x18-byte rows and the competition index.
bool Reader::header() {
    if (read(at(0x4d), 0x50, 1, 0x1048327c) != 1) return fail(site::name), false;
    read(frame.at(local::stats_flag), 4, 1, 0x104832f0);
    const auto old_stats = c.u32(at(competition::stats));
    if (c.u32(frame.at(local::stats_flag)) == 0) {
        if (old_stats != 0) call(routine::stats_destroy, 0x10483350, {old_stats, 1});
    } else {
        if (old_stats != 0) call(routine::stats_destroy, 0x10483310, {old_stats, 1});
        const auto stats = call(routine::new_object, 0x10483318, {0x634});
        held.r19 = stats;
        if (stats != 0) call(routine::stats_construct, 0x10483328, {stats, cup});
        c.put32(at(competition::stats), stats);
        call(routine::stats_read, 0x10483338, {c.u32(at(competition::stats)), file});
    }
    publish();
    virtual_call(c, read_records_slot, 0x10483364, {cup, file});
    for (unsigned i = 0; i < fields.size(); ++i) {
        const auto done = read(at(fields[i].offset), fields[i].size, 1, fields[i].load_back);
        const auto n = site::field_site(i);
        if (n >= 0 && (unsigned(n) == site::zero_fails ? done == 0 : done != 1)) return fail(unsigned(n)), false;
    }
    read(at(competition::block_18), 4, 1, 0x10483a98);   // the 0x18-byte rows' pointer word
    read(at(competition::half_34), 2, 1, 0x10483aac);
    if (c.u32(at(competition::block_18)) != 0 && half(competition::half_34) != 0) {
        c.put32(at(competition::block_18), allocate(sext16(c.u16(at(competition::half_34))) * extra_row, 0x10483acc));
        if (c.u32(at(competition::block_18)) == 0) fail(site::extra_rows);
        read(c.u32(at(competition::block_18)), extra_row, sext16(c.u16(at(competition::half_34))), 0x10483b68);
    }
    if (read(frame.at(local::record_index), 4, 1, 0x10483b7c) != 1) return fail(site::record_index), false;
    const auto index = c.u32(frame.at(local::record_index));
    c.put32(at(competition::record), index == none ? 0 : c.u32(c.toc(toc::records)) + index * record_size);
    return true;
}

// The rounds (0x78 bytes each), then the entrant rows (8 bytes: a club first, a table index in the file).
bool Reader::entrants() {
    c.put32(at(cup::rounds), allocate(sext16(c.u16(at(competition::rounds))) * cup::round_row, 0x10483c3c));
    if (c.u32(at(cup::rounds)) == 0) fail(site::rounds_alloc);
    if (read(c.u32(at(cup::rounds)), cup::round_row, sext16(c.u16(at(competition::rounds))), 0x10483cd8) !=
        sext16(c.u16(at(competition::rounds))))
        return fail(site::rounds_read), false;
    c.put32(at(cup::entrants), allocate(sext16(c.u16(at(competition::club_count))) << 3, 0x10483d50));
    if (c.u32(at(cup::entrants)) == 0) return fail(site::entrants_alloc), false;
    if (read(c.u32(at(cup::entrants)), cup::entrant_row, sext16(c.u16(at(competition::club_count))), 0x10483dd0) !=
        sext16(c.u16(at(competition::club_count))))
        return fail(site::entrants_read), false;
    rows_to_pointers(cup::entrants, competition::club_count);
    return true;
}

// The club words of 8-byte rows (the block at `block_field`, `count_field` halfwords of them) become pointers (-1:
// none, as 0). The original runs this loop with r3 the byte offset, r4 zero, r5 the club table (once a row has a
// club) and r6 the block, and the last of them reach the caller: the call after the last loop sees them.
void Reader::rows_to_pointers(u32 block_field, u32 count_field) {
    c.set({{3, 0}, {4, 0}});
    held.r23 = 0;
    for (; std::int32_t(held.r23) < half(count_field); ++held.r23) {
        const auto block = c.u32(at(block_field));
        c.set(6, block);
        const auto row = block + 8 * held.r23;
        const auto club = c.u32(row);
        if (club == none) {
            c.put32(row, c.r(4));
        } else {
            c.set(5, c.u32(clubs_cell));
            c.put32(row, c.r(5) + club * club_row_size);
        }
        c.set(3, 8 * (held.r23 + 1));
    }
}

// One block of match rows per round (0x36 bytes per match, the round's count at +0x1c), then each read whole.
bool Reader::matches() {
    c.put32(at(cup::round_matches), allocate(sext16(c.u16(at(competition::rounds))) << 2, 0x10483e94));
    held.r23 = held.r21 = held.r22 = 0;
    for (; std::int32_t(held.r23) < half(competition::rounds); ++held.r23, held.r21 += 4, held.r22 += cup::round_row) {
        const auto block = allocate(sext16(c.u16(c.u32(at(cup::rounds)) + held.r22 + Round::ties)) * cup::match_row, 0x10483ebc);
        c.put32(c.u32(at(cup::round_matches)) + held.r21, block);
        if (c.u32(c.u32(at(cup::round_matches)) + held.r21) == 0) fail(site::matches_alloc);
    }
    held.r23 = held.r21 = held.r22 = 0;
    for (; std::int32_t(held.r23) < half(competition::rounds); ++held.r23, held.r21 += cup::round_row, held.r22 += 4) {
        const auto bytes = sext16(c.u16(c.u32(at(cup::rounds)) + held.r21 + Round::ties)) * cup::match_row;
        if (read(c.u32(c.u32(at(cup::round_matches)) + held.r22), bytes, 1, 0x10483f78) != 1)
            return fail(site::matches_read), false;
    }
    return true;
}

// Stage objects: slot 0..last_stage of the array at +0xc, each introduced by a kind byte (1: a list of halfwords
// read into a new 0xb4-byte stage; 2: two halfwords and a 0xec-byte stage; anything else: nothing, the slot is left).
bool Reader::stages() {
    read(at(competition::word_2c), 4, 1, 0x1048400c);
    read(at(competition::last_stage), 4, 1, 0x10484020);
    read(at(competition::owner), 1, 1, 0x10484034);
    if (c.u32(at(competition::stages)) == 0 && c.i32(at(competition::word_2c)) > 0) {
        c.put32(at(competition::stages), allocate(c.u32(at(competition::word_2c)) << 2, 0x10484054));
        if (c.u32(at(competition::stages)) == 0) return fail(site::stages_alloc), false;
    }
    if (c.u32(at(competition::stages)) == 0) return true;
    const auto array = [&] { return c.u32(at(competition::stages)); };
    held.r23 = held.r22 = 0;
    for (; std::int32_t(held.r23) <= c.i32(at(competition::last_stage)); ++held.r23, held.r22 += 4) {
        read(frame.at(local::stage_kind), 1, 1, 0x10484110);
        const auto kind = sext8(c.u8(frame.at(local::stage_kind)));
        if (kind == 1) {
            read(frame.at(local::list_first), 2, 1, 0x10484134);
            read(frame.at(local::list_second), 2, 1, 0x10484148);
            const auto values = allocate(sext16(c.u16(frame.at(local::list_second))) << 1, 0x10484154);
            held.r20 = values;
            if (values == 0) return fail(site::list_alloc), false;
            held.r19 = held.r21 = 0;
            for (; std::int32_t(held.r19) < c.i16(frame.at(local::list_second)); ++held.r19, held.r21 += 2)
                read(values + held.r21, 2, 1, 0x104841fc);
            const auto list = call(routine::new_object, 0x10484218, {0xb4});
            held.r19 = list;
            if (list != 0)
                call(routine::list_construct, 0x10484230, {list, sext16(c.u16(frame.at(local::list_first))),
                                                           sext16(c.u16(frame.at(local::list_second))), values});
            c.put32(array() + held.r22, list);
            call(routine::delete_block, 0x10484240, {values, array()});
            call(routine::list_read, 0x10484250, {c.u32(array() + held.r22), file});
        } else if (kind == 2) {
            read(frame.at(local::group_first), 2, 1, 0x10484270);
            read(frame.at(local::group_second), 2, 1, 0x10484284);
            const auto group = call(routine::new_object, 0x1048428c, {0xec});
            held.r19 = group;
            if (group != 0)
                call(routine::group_construct, 0x104842a0, {group, sext16(c.u16(frame.at(local::group_first))),
                                                            sext16(c.u16(frame.at(local::group_second)))});
            c.put32(array() + held.r22, group);
            call(routine::group_read, 0x104842b8, {c.u32(array() + held.r22), file});
        }
    }
    return true;
}

// The 8-byte rows at +0x14 (a club first, a table index in the file): read whole, then each club word turned into a
// pointer. Nothing is freed: the old block of the row array is left to the owner.
bool Reader::round_rows() {
    read(at(competition::half_38), 2, 1, 0x104842e0);
    read(at(competition::half_36), 2, 1, 0x104842f4);
    if (half(competition::half_36) == 0) return true;
    c.put32(at(competition::block_14), allocate(sext16(c.u16(at(competition::half_38))) << 3, 0x1048430c));
    if (c.u32(at(competition::block_14)) == 0) return fail(site::rows_alloc), false;
    read(c.u32(at(competition::block_14)), 8, sext16(c.u16(at(competition::half_38))), 0x104843ac);
    rows_to_pointers(competition::block_14, competition::half_36);
    return true;
}

u32 Reader::run() {
    if (!open()) return 0;
    if (!header() || !entrants() || !matches() || !stages() || !round_rows()) return 0;
    call(routine::close_file, 0x10484400, {file});
    return 1;
}
}

std::uint32_t load_from_disk(Cpu& c, std::uint32_t cup, std::uint32_t backup) {
    const Frame frame(c, 0x104831a0, 0x4a0, {.first_gpr = 19});
    return Reader(c, frame, cup, backup).run();
}
}
