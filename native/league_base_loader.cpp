// LEAGUE.CPP 0x104a6850: the league's save reader. It builds the file name from the competition's name, opens it in
// CM3_TEMP, and reads the league record field by field; every checked read that comes up short, and every allocation
// that fails, reports one of 51 assertion sites (source lines 0x1106..0x1344), clears the success flag and returns 0.
// A failure never closes the file. The club indices the file holds (-1: none) become pointers into the club table, the
// competition index a pointer into the competition records. The original's frame is 0x880 bytes: the locals the file
// layer fills sit at sp+0x38..0x50, the assertions' two buffers above them (sp+0x1ec..0x82c).
#include "english_competition_rules.hpp"
#include "league_base_detail.hpp"
#include "league_base_loader.hpp"
#include <array>
#include <initializer_list>

namespace cm::aot::league_base {
namespace {
namespace routine {
inline constexpr std::uint32_t
    file_path = 0x10746280, open_file = 0x10739de0, read_file = 0x10775fe0, close_file = 0x107766f0,
    new_object = 0x10773520, new_block = 0x10775130, delete_block = 0x10775170, format = 0x1077b720,
    new_stats = 0x104533a0, read_stats = 0x1045f6a0,    // COMP_STATS.CPP: construct, then read from the file
    new_list = 0x104bc750, read_list = 0x104bcb20,      // the 0xb4-byte stage of kind 1 (CUP_STAGE.CPP)
    new_group = 0x1056e120, read_group = 0x1056e430;    // the 0xec-byte stage of kind 2
}
// Fields of the league object this routine reads, by what the writer 0x104a8530 and the readers show (inferred).
namespace field {
inline constexpr std::uint32_t
    stats = 0x08,            // COMP_STATS' object (0x634 bytes), when the file has one
    stages = 0x0c,           // word array: one stage object per entry 0..last_stage
    round_array = 0x14,      // 8-byte rows whose first word is a club
    extra = 0x18, extra_count = 0x34,      // 0x18-byte rows
    stage_count = 0x2c, last_stage = 0x30, owner = 0x44,
    round_count = 0x36, round_capacity = 0x38,
    rounds = 0x3c, clubs = 0x3e, year = 0x40,
    table_size = 0xa4, schedule_count = 0xa6, standings = 0xb0, schedule = 0xbc;
}
struct Field { std::uint32_t offset, size; };
// The reads up to the competition index, in the file's order (sites 1..24; 25 is the index, read into the frame).
inline constexpr std::array<Field, 24> header_fields{{
    {0xe2, 2}, {0xe4, 2}, {0xe1, 1}, {0x28, 4}, {0xe0, 1}, {0xd4, 8}, {0xa4, 2}, {0x4c, 1}, {0x4b, 1}, {0x48, 1},
    {0x49, 1}, {0x24, 4}, {0x46, 1}, {0x47, 1}, {0xb6, 2}, {0xb8, 2}, {0x45, 1}, {0xb4, 1}, {0x3c, 2}, {0x3e, 2},
    {0x40, 2}, {0x43, 1}, {0x42, 1}, {0x3a, 2}}};
// The reads after it (sites 26..37).
inline constexpr std::array<Field, 12> body_fields{{
    {0xa6, 2}, {0xc4, 1}, {0xc5, 1}, {0xc6, 1}, {0xc7, 4}, {0xc0, 4}, {0xdc, 2}, {0xde, 2}, {0xa8, 2}, {0x1c, 4},
    {0x20, 4}, {0xe6, 1}}};
// The assertion sites: the address of each report's first call and its source line. The report's two buffers sit at
// sp+0x82c-0x20n (the file name part) and sp+0x1e4-8n (the extension part).
struct Site { std::uint32_t at, line; };
inline constexpr std::array<Site, 51> sites{{
    {0x104a6938, 0x1106}, {0x104a6a18, 0x111d}, {0x104a6a8c, 0x1122}, {0x104a6b00, 0x1127}, {0x104a6b74, 0x112c},
    {0x104a6be8, 0x1131}, {0x104a6c5c, 0x1136}, {0x104a6cd0, 0x113c}, {0x104a6d44, 0x1142}, {0x104a6db8, 0x1148},
    {0x104a6e2c, 0x114e}, {0x104a6ea0, 0x1155}, {0x104a6f14, 0x1177}, {0x104a6f88, 0x117d}, {0x104a6ffc, 0x1183},
    {0x104a7070, 0x1189}, {0x104a70e4, 0x118f}, {0x104a7158, 0x1197}, {0x104a71cc, 0x11ba}, {0x104a7240, 0x11c0},
    {0x104a72b4, 0x11c6}, {0x104a7328, 0x11cc}, {0x104a739c, 0x11d3}, {0x104a7410, 0x11da}, {0x104a7484, 0x11e1},
    {0x104a74f8, 0x11e8}, {0x104a7598, 0x11f7}, {0x104a760c, 0x11fe}, {0x104a7680, 0x1205}, {0x104a76f4, 0x120c},
    {0x104a7768, 0x1213}, {0x104a77dc, 0x121a}, {0x104a7850, 0x1221}, {0x104a78c4, 0x1228}, {0x104a7938, 0x122f},
    {0x104a79ac, 0x1236}, {0x104a7a20, 0x123d}, {0x104a7a94, 0x1244}, {0x104a7b08, 0x124b}, {0x104a7b7c, 0x1251},
    {0x104a7c58, 0x1269}, {0x104a7ccc, 0x1270}, {0x104a7d40, 0x1278}, {0x104a7ddc, 0x1286}, {0x104a7e58, 0x128f},
    {0x104a7ef0, 0x129b}, {0x104a7fb0, 0x12b0}, {0x104a8020, 0x12b9}, {0x104a81f0, 0x12f5}, {0x104a82c0, 0x1311},
    {0x104a8470, 0x1344}}};
namespace site {
inline constexpr unsigned name = 0, header_first = 1, record_index = 25, body_first = 26, schedule_alloc = 38,
    schedule_read = 39, standings_alloc = 40, standings_read = 41, club_flag = 42, club_row_alloc = 43,
    club_row_read = 44, table_flag = 45, table_alloc = 46, table_read = 47, stage_array_alloc = 48,
    values_alloc = 49, round_array_alloc = 50;
}
// The frame's locals the file layer writes into (sp+offset).
namespace local {
inline constexpr std::int32_t stage_kind = 0x38, group_second = 0x3a, group_first = 0x3c, list_second = 0x3e,
    list_first = 0x40, table_flag = 0x44, row_flag = 0x48, stats_flag = 0x4c, record_index = 0x50;
}
inline constexpr std::uint32_t read_step = 0x74;  // the checked reads repeat at this distance in the original
inline constexpr std::uint32_t club_row_size = 0x248, record_size = 0x6c, none = 0xffffffffu;

// r21-r25 as the original holds them (loop counters, the last allocation): the callees can observe them, a replay
// compares r13-r31 at every call that reaches an import.
struct Held { std::uint32_t r21, r22, r23, r24, r25; };

class Loader {
public:
    Loader(Cpu& cpu, const Frame& frame, std::uint32_t league, std::uint32_t backup)
        : c(cpu), frame(frame), league(league), backup(backup), held{c.r(21), c.r(22), c.r(23), c.r(24), c.r(25)},
          clubs_cell(c.toc(toc::clubs)), pool(c.toc(toc::pool)) {}
    std::uint32_t run();

private:
    // `bl routine` at `at` (the return address is the next instruction), r3.. the arguments, r21-r25 in place.
    std::uint32_t call(std::uint32_t routine, std::uint32_t at, std::initializer_list<std::uint32_t> arguments) {
        publish();
        return invoke(c, routine, at + 4, arguments);
    }
    void publish() { c.set({{21, held.r21}, {22, held.r22}, {23, held.r23}, {24, held.r24}, {25, held.r25}}); }
    std::uint32_t read(std::uint32_t to, std::uint32_t size, std::uint32_t count, std::uint32_t at) {
        return call(routine::read_file, at, {to, size, count, file});
    }
    std::uint32_t allocate(std::uint32_t bytes, std::uint32_t at) { return call(routine::new_block, at, {bytes}); }
    // The report of a failed check; the caller returns its 0.
    std::uint32_t fail(unsigned n) {
        publish();
        report(c, frame, {sites[n].at, 0x82c - 0x20 * std::int32_t(n), 0x1e4 - 8 * std::int32_t(n), sites[n].line});
        return 0;
    }
    // A club's row in the club table (the TOC slot holds the address of the table's cell, r26 in the original).
    std::uint32_t club_pointer(std::uint32_t index) const { return c.u32(clubs_cell) + index * club_row_size; }
    std::int32_t clubs() const { return c.i16(league + field::clubs); }
    std::uint32_t at(std::uint32_t offset) const { return league + offset; }

    bool open();
    bool header();
    bool body();
    bool standings();
    bool club_rows(std::uint32_t row_flag);
    bool table_rows(std::uint32_t table_flag);
    void club_pointers();
    bool stage_objects();
    bool round_rows();

    Cpu& c;
    const Frame& frame;
    std::uint32_t league, backup;
    Held held;
    std::uint32_t clubs_cell, pool, file = 0;
};

// "<competition name>_<id>.tmp" (a nonzero `backup`) or "<competition name>_<year>.bak", '/' replaced by '_', in
// CM3_TEMP, opened "rb". The name goes into the league's own 0x50-byte buffer at +0x4d, which the first read then
// overwrites.
bool Loader::open() {
    const auto name = at(0x4d);
    const auto record = c.u32(league + 4);
    c.set({{26, clubs_cell}, {27, league}, {29, c.toc(toc::success)}, {30, c.toc(toc::message)}, {31, pool}});
    publish();
    if (sext8(backup) != 0)
        invoke(c, routine::format, 0x104a6894, {name, pool + text::temporary_name, record + 0x1a, c.u32(record)});
    else
        invoke(c, routine::format, 0x104a68b0,
               {name, pool + text::backup_name, record + 0x1a, sext16(c.u16(league + field::year))});
    for (std::uint32_t i = 0; c.u8(name + i) != 0; ++i)
        if (c.u8(name + i) == '/') c.put8(name + i, '_');
    const auto path = invoke(c, routine::file_path, 0x104a68f0, {pool + text::temporary_folder, name});
    file = invoke(c, routine::open_file, 0x104a68f8, {path, pool + text::read_mode});
    c.set(28, file);
    return file != 0;
}

// The first record (it overwrites the name), the statistics object, the derived league's own records (virtual +0x28),
// the fixed fields, and the competition index.
bool Loader::header() {
    if (read(at(0x4d), 0x50, 1, 0x104a6918) != 1) return fail(site::name), false;
    read(frame.at(local::stats_flag), 4, 1, 0x104a698c);
    if (c.u32(frame.at(local::stats_flag)) == 0) c.put32(at(field::stats), 0);
    else {
        const auto stats = call(routine::new_object, 0x104a69a0, {0x634});
        held.r21 = stats;
        if (stats) call(routine::new_stats, 0x104a69b0, {stats, league});
        c.put32(at(field::stats), stats);
        call(routine::read_stats, 0x104a69c0, {c.u32(at(field::stats)), file});
    }
    publish();
    virtual_call(c, 0x28, 0x104a69e4, {league, file});
    for (unsigned i = 0; i < header_fields.size(); ++i)
        if (read(at(header_fields[i].offset), header_fields[i].size, 1, 0x104a69f8 + read_step * i) != 1)
            return fail(site::header_first + i), false;
    if (read(frame.at(local::record_index), 4, 1, 0x104a69f8 + read_step * 24) != 1) return fail(site::record_index), false;
    const auto index = c.u32(frame.at(local::record_index));
    c.put32(league + 4, index == none ? 0 : c.u32(c.toc(toc::records)) + index * record_size);
    return true;
}

// The fixed fields after the index, then the schedule rows (+0xbc, 0x4c bytes each) and the extra rows (+0x18).
bool Loader::body() {
    for (unsigned i = 0; i < body_fields.size(); ++i)
        if (read(at(body_fields[i].offset), body_fields[i].size, 1, 0x104a7578 + read_step * i) != 1)
            return fail(site::body_first + i), false;
    c.put32(at(field::schedule), allocate(c.i16(at(field::schedule_count)) * 0x4cu, 0x104a7ae0));
    if (c.u32(at(field::schedule)) == 0) return fail(site::schedule_alloc), false;
    const auto count = sext16(c.u16(at(field::schedule_count)));
    if (read(c.u32(at(field::schedule)), 0x4c, count, 0x104a7b58) != sext16(c.u16(at(field::schedule_count))))
        return fail(site::schedule_read), false;
    read(at(0x18), 4, 1, 0x104a7bd0);
    read(at(field::extra_count), 2, 1, 0x104a7be4);
    if (c.u32(at(field::extra)) != 0 && c.i16(at(field::extra_count)) != 0) {
        c.put32(at(field::extra), allocate(sext16(c.u16(at(field::extra_count))) * 0x18u, 0x104a7c04));
        read(c.u32(at(field::extra)), 0x18, sext16(c.u16(at(field::extra_count))), 0x104a7c1c);
    }
    return true;
}

// The standings: one 0x44-byte row per club. Their first word is a club (a table index in the file, -1: none).
// The file says whether each row has an array of the other clubs (+8, 0x10 bytes per club but one) and whether each
// has a block of table_size bytes (+0xc).
bool Loader::standings() {
    if (clubs() == 0) { c.put32(at(field::standings), 0); return true; }
    c.put32(at(field::standings), allocate(sext16(c.u16(at(field::clubs))) * 0x44u, 0x104a7c30));
    if (c.u32(at(field::standings)) == 0) return fail(site::standings_alloc), false;
    const auto count = sext16(c.u16(at(field::clubs)));
    if (read(c.u32(at(field::standings)), 0x44, count, 0x104a7ca8) != sext16(c.u16(at(field::clubs))))
        return fail(site::standings_read), false;
    if (read(frame.at(local::row_flag), 4, 1, 0x104a7d20) != 1) return fail(site::club_flag), false;
    if (!club_rows(c.u32(frame.at(local::row_flag)))) return false;
    if (read(frame.at(local::table_flag), 4, 1, 0x104a7ed0) != 1) return fail(site::table_flag), false;
    if (!table_rows(c.u32(frame.at(local::table_flag)))) return false;
    club_pointers();
    return true;
}

// Per row (r24 the row, r25 its byte offset): the array of the other clubs, or none.
bool Loader::club_rows(std::uint32_t row_flag) {
    held.r24 = held.r25 = 0;
    const auto cell = [&](std::uint32_t row) { return c.u32(at(field::standings)) + 0x44u * row + 8; };
    for (std::int32_t row = 0; row < clubs(); ++row) {
        held.r24 = std::uint32_t(row);
        held.r25 = 0x44u * held.r24;
        if (row_flag == 0) { c.put32(cell(held.r24), 0); continue; }
        c.put32(cell(held.r24), allocate((sext16(c.u16(at(field::clubs))) - 1u) << 4, 0x104a7da8));
        if (c.u32(cell(held.r24)) == 0) return fail(site::club_row_alloc), false;
        const auto expected = sext16(c.u16(at(field::clubs))) - 1u;
        const auto got = read(c.u32(cell(held.r24)), 0x10, expected, 0x104a7e30);
        if (std::int32_t(got) != std::int32_t(sext16(c.u16(at(field::clubs))) - 1u)) return fail(site::club_row_read), false;
    }
    held.r24 = std::uint32_t(clubs() > 0 ? clubs() : 0);
    held.r25 = 0x44u * held.r24;
    return true;
}

// Per row (r23 the row, r24 its byte offset): the block of table_size bytes, or none. A table_size of -1 becomes
// (clubs - 1) * rounds first when the league has clubs.
bool Loader::table_rows(std::uint32_t table_flag) {
    const auto cell = [&](std::uint32_t row) { return c.u32(at(field::standings)) + 0x44u * row + 0xc; };
    if (table_flag == 0) {
        for (std::int32_t row = 0; row < clubs(); ++row) c.put32(cell(std::uint32_t(row)), 0);
        return true;
    }
    if (c.i16(at(field::table_size)) == -1 && clubs() > 0)
        c.put16(at(field::table_size), (sext16(c.u16(at(field::clubs))) - 1u) * sext16(c.u16(at(field::rounds))));
    held.r23 = held.r24 = 0;
    for (std::int32_t row = 0; row < clubs(); ++row) {
        held.r23 = std::uint32_t(row);
        held.r24 = 0x44u * held.r23;
        c.put32(cell(held.r23), allocate(sext16(c.u16(at(field::table_size))), 0x104a7f7c));
        if (c.u32(cell(held.r23)) == 0) return fail(site::table_alloc), false;
        if (read(c.u32(cell(held.r23)), sext16(c.u16(at(field::table_size))), 1, 0x104a8000) != 1)
            return fail(site::table_read), false;
    }
    held.r23 = std::uint32_t(clubs() > 0 ? clubs() : 0);
    held.r24 = 0x44u * held.r23;
    return true;
}

// Table indices to pointers, in the standings' first words and in the other-clubs arrays (-1: none, as 0). The
// original leaves r7 = the byte offset of the next row, r8 = the rows done, r9 = the other-clubs count of the last
// array it walked, which the next read (0x104a8180) can observe.
void Loader::club_pointers() {
    std::uint32_t offset = 0, done = 0;
    c.set({{7, 0}, {8, 0}});
    for (; std::int32_t(done) < clubs(); ++done, offset += 0x44) {
        const auto row = c.u32(at(field::standings)) + offset;
        const auto club = c.u32(row);
        c.put32(row, club == none ? 0 : club_pointer(club));
        if (c.u32(c.u32(at(field::standings)) + 8) == 0) continue;
        std::uint32_t other = 0;
        for (; std::int32_t(other) < clubs() - 1; ++other) {
            const auto entry = c.u32(c.u32(at(field::standings)) + 8 + offset) + 0x10 * other;
            const auto id = c.u32(entry);
            c.put32(entry, id == none ? 0 : club_pointer(id));
        }
        c.set(9, other);
    }
    c.set({{7, offset}, {8, done}});
}

// Stage objects: slot 0..last_stage of the array at +0xc, each introduced by a kind byte (1: a list of halfwords
// read into a new 0xb4-byte stage; 2: two halfwords and a 0xec-byte stage; anything else: nothing, the slot is left).
bool Loader::stage_objects() {
    const auto array = [&] { return c.u32(at(field::stages)); };
    if (array() == 0 && c.i32(at(field::stage_count)) > 0) {
        c.put32(at(field::stages), allocate(c.u32(at(field::stage_count)) << 2, 0x104a81c8));
        if (array() == 0) return fail(site::stage_array_alloc), false;
    }
    if (array() == 0) return true;
    held.r24 = held.r25 = 0;
    for (std::int32_t stage = 0; stage <= c.i32(at(field::last_stage)); ++stage, ++held.r24, held.r25 += 4) {
        read(frame.at(local::stage_kind), 1, 1, 0x104a825c);
        const auto kind = sext8(c.u8(frame.at(local::stage_kind)));
        if (kind == 1) {
            read(frame.at(local::list_first), 2, 1, 0x104a8280);
            read(frame.at(local::list_second), 2, 1, 0x104a8294);
            const auto values = allocate(sext16(c.u16(frame.at(local::list_second))) << 1, 0x104a82a0);
            held.r22 = values;
            if (values == 0) return fail(site::values_alloc), false;
            held.r21 = held.r23 = 0;
            while (std::int32_t(held.r21) < c.i16(frame.at(local::list_second))) {
                read(values + held.r23, 2, 1, 0x104a8320);
                held.r23 += 2;
                ++held.r21;
            }
            const auto list = call(routine::new_object, 0x104a833c, {0xb4});
            held.r21 = list;
            if (list)
                call(routine::new_list, 0x104a8354, {list, sext16(c.u16(frame.at(local::list_first))),
                                                     sext16(c.u16(frame.at(local::list_second))), values});
            c.put32(array() + held.r25, list);
            call(routine::delete_block, 0x104a8364, {values});
            call(routine::read_list, 0x104a8374, {c.u32(array() + held.r25), file});
        } else if (kind == 2) {
            read(frame.at(local::group_first), 2, 1, 0x104a8394);
            read(frame.at(local::group_second), 2, 1, 0x104a83a8);
            const auto group = call(routine::new_object, 0x104a83b0, {0xec});
            held.r21 = group;
            if (group)
                call(routine::new_group, 0x104a83c4, {group, sext16(c.u16(frame.at(local::group_first))),
                                                      sext16(c.u16(frame.at(local::group_second)))});
            c.put32(array() + held.r25, group);
            call(routine::read_group, 0x104a83dc, {c.u32(array() + held.r25), file});
        }
    }
    return true;
}

// The 8-byte rows at +0x14 (a club first, a table index in the file): the previous block is freed, a new one read.
bool Loader::round_rows() {
    if (const auto old = c.u32(at(field::round_array)); old != 0) {
        call(routine::delete_block, 0x104a8400, {old});
        c.put32(at(field::round_array), 0);
    }
    read(at(field::round_capacity), 2, 1, 0x104a841c);
    read(at(field::round_count), 2, 1, 0x104a8430);
    if (c.i16(at(field::round_count)) == 0) return true;
    c.put32(at(field::round_array), allocate(sext16(c.u16(at(field::round_capacity))) << 3, 0x104a8448));
    if (c.u32(at(field::round_array)) == 0) return fail(site::round_array_alloc), false;
    read(c.u32(at(field::round_array)), 8, sext16(c.u16(at(field::round_capacity))), 0x104a84c0);
    std::uint32_t done = 0;
    c.set(7, 0);
    for (; std::int32_t(done) < c.i16(at(field::round_count)); ++done) {
        const auto row = c.u32(at(field::round_array)) + 8 * done;
        const auto club = c.u32(row);
        c.put32(row, club == none ? 0 : club_pointer(club));
    }
    c.set(7, done);
    return true;
}

std::uint32_t Loader::run() {
    if (!open()) return 0;
    if (!header() || !body() || !standings()) return 0;
    read(at(field::stage_count), 4, 1, 0x104a8180);
    read(at(field::last_stage), 4, 1, 0x104a8194);
    read(at(field::owner), 1, 1, 0x104a81a8);
    if (!stage_objects() || !round_rows()) return 0;
    call(routine::close_file, 0x104a8514, {file});
    // The English named-reserves rule (english_competition_rules.hpp): the generated body runs it at 0x104a8518, the
    // success path's `li r3,1`.
    english_rules::premier_bench(c, league);
    return 1;
}
}

std::uint32_t load_save(Cpu& c, std::uint32_t league, std::uint32_t backup) {
    const Frame frame(c, 0x104a6850, 0x880, {.first_gpr = 21});
    return Loader(c, frame, league, backup).run();
}
}
