#include "tactics_setup.hpp"
#include <array>

namespace cm::aot::tactics::setup {
using guest::Cpu;
using guest::Frame;
namespace {
// TOC slots of the globals these routines read.
constexpr std::int32_t pool_slot = -0x4cf0;          // TACTICS.CPP's string pool
constexpr std::int32_t message_slot = -0x7ba8, success_slot = -0x7bac;
constexpr std::int32_t formation_destructor_slot = -0x6418, formation_constructor_slot = -0x6414;
constexpr std::int32_t club_count_slot = -0x7b50;    // -> int: number of clubs
constexpr std::int32_t staff_count_slot = -0x7be4;   // -> int: number of staff
constexpr std::int32_t save_slot = -0x7b74;          // -> the saved game being loaded
constexpr std::int32_t volume_slots[2] = {-0x4ce8, -0x4cec};  // the CD's name, in two halves
// Strings in the pool.
constexpr std::uint32_t insert_cd_title = 0x0, insert_cd_text = 0xa, folder = 0x37, entry_name = 0x371;

// Routines that still run generated code.
constexpr std::uint32_t new_block = 0x107735f0, construct_array = 0x10773150, allocate = 0x10775130,
                        release = 0x10775170, clear = 0x10778300, copy_string = 0x1077dbf0,
                        folder_path = 0x10746280, make_file_spec = 0x1077fee0, pascal_string = 0x10727890,
                        alert = 0x10769b60, open_entry = 0x107475b0, read_entry = 0x10747a80,
                        write_entry = 0x10747cc0, close_entry = 0x10747900, save_version = 0x10775550,
                        set_instruction = 0x10113e30, set_team_option = 0x10113b60;

// A signed compare into cr0 (cmpw, cmpwi, a record form against 0): less,
// greater, equal and XER's summary overflow.
void compare(Cpu& c, std::int32_t a, std::int32_t b) {
    c.set_cr(0, std::uint8_t((a < b) << 3 | (a > b) << 2 | (a == b) << 1 | (c.s.r<1>(guest::reg::xer_so) & 1)));
}

void fail(Cpu& c, const Frame& frame, std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line) {
    const auto pool = c.toc(pool_slot);
    guest::report_assertion(c, {at, pool + 0x4e, pool + 0x5a, pool + 0x67, frame.at(name), frame.at(extension), line});
}

// Asks for the CD until its volume exists: the name is kept as two Pascal
// strings whose characters interleave. The merge runs until the first half
// ends in a zero byte, which lies past the 17 bytes copied to the stack: the
// original reads its stack there.
void wait_for_cd(Cpu& c, const Frame& frame) {
    const auto first = c.toc(volume_slots[0]), second = c.toc(volume_slots[1]), pool = c.toc(pool_slot);
    const auto a = frame.at(0x7f0), b = frame.at(0x1c0), volume = frame.at(0x84c);
    for (;;) {
        c.set({{19, c.u32(first)}, {22, 0}});
        for (std::uint32_t i = 0; i < 0x11; ++i) c.put8(a + i, c.u8(first + i));
        for (std::uint32_t i = 0; i < 0x10; ++i) c.put8(b + i, c.u8(second + i));
        for (std::uint32_t from_a = a + 1, from_b = b + 1, to = volume + 1; c.i8(from_a); to += 2) {
            const auto x = c.u8(from_a++), y = c.u8(from_b++);
            c.put8(to, x);
            c.put8(to + 1, y);
        }
        c.put8(volume, c.u8(a) + c.u8(b));
        c.set({{3, 0}, {4, 0}, {5, volume}, {6, frame.at(0x804)}});
        c.call(make_file_spec, 0x103ad05c);  // FSMakeFSSpec through the glue, which saves r2 at sp+0x14
        c.set(2, c.u32(frame.at(0x14)));
        if (!std::int16_t(c.r(3))) {
            c.set(22, 1);
            return;
        }
        c.set({{3, frame.at(0xa14)}, {4, pool + insert_cd_title}});
        c.call(pascal_string, 0x103ad080);
        c.set({{3, frame.at(0x94c)}, {4, pool + insert_cd_text}});
        c.call(pascal_string, 0x103ad08c);
        c.set({{3, frame.at(0xa14)}, {4, frame.at(0x94c)}, {5, 1}});
        c.call(alert, 0x103ad09c);
    }
}

// new[] of `count` formation records (0x107735f0, then the array
// constructor 0x10773150 with the record's constructor and destructor); the
// original keeps a flag (allocated) and the array in non-volatile registers.
// `back` follows the allocation's call.
std::uint32_t new_formations(Cpu& c, std::uint32_t count, unsigned flag, unsigned array, std::uint32_t back) {
    c.set(flag, 0);
    c.set(3, count * formation_size + 0x10);
    c.call(new_block, back);
    c.set(array, c.r(3));
    if (!c.r(3)) return 0;
    c.set(flag, 1);
    c.set({{4, c.toc(formation_constructor_slot)}, {5, c.toc(formation_destructor_slot)}, {6, formation_size},
           {7, count}});
    c.call(construct_array, back + 0x20);
    c.set(array, c.r(3));
    return c.r(3);
}

// A cleared team record's empty squad, captains, set-piece takers and line-up
// (and pick, when `pick`).
void reset_team(Cpu& c, std::uint32_t team, bool pick) {
    for (std::uint32_t slot = 0; slot < std::uint32_t(squad_size); ++slot) c.put32(team + field::squad + slot * 4, 0xffffffffu);
    if (pick) c.put32(team + field::pick, 0xffffffffu);
    for (std::uint32_t which = 0; which < 2; ++which) c.put32(team + field::captains + which * 4, 0xffffffffu);
    for (std::uint32_t taker = 0; taker < std::uint32_t(set_pieces) * 2; ++taker)
        c.put32(team + field::takers + taker * 4, 0xffffffffu);
    for (std::uint32_t place = 0; place < 11; ++place) c.put8(team + field::lineup + place, 0xff);
}

// An old 0x8c-byte team record (no captains; takers at 0x58, line-up at
// 0x80) in the current layout.
void convert_team(Cpu& c, std::uint32_t old, std::uint32_t team) {
    c.put16(team + field::team_formation, c.u16(old));
    for (std::uint32_t slot = 0; slot < std::uint32_t(squad_size); ++slot)
        c.put32(team + field::squad + slot * 4, c.u32(old + field::squad + slot * 4));
    c.put32(team + field::pick, c.u32(old + field::pick));
    for (std::uint32_t which = 0; which < 2; ++which) c.put32(team + field::captains + which * 4, 0xffffffffu);
    for (std::uint32_t taker = 0; taker < std::uint32_t(set_pieces) * 2; ++taker)
        c.put32(team + field::takers + taker * 4, c.u32(old + 0x58 + taker * 4));
    for (std::uint32_t place = 0; place < 11; ++place) c.put8(team + field::lineup + place, c.u8(old + 0x80 + place));
}

// The preset files, loaded into the extra formation records, and the
// default ones, loaded into the first records and given their titles: pool
// offsets of the file name (and title), and the line of the assert that
// fires when the array is full.
struct Preset {
    std::uint16_t file, title, line;
};
constexpr std::array<Preset, preset_count> presets{{
    {0x43, 0, 0x71},  {0x6d, 0, 0x72},  {0x78, 0, 0x73},  {0x83, 0, 0x74},  {0x8e, 0, 0x75},  {0x99, 0, 0x76},
    {0xa4, 0, 0x77},  {0xaf, 0, 0x78},  {0xba, 0, 0x79},  {0xc5, 0, 0x7a},  {0xd0, 0, 0x7b},  {0xdb, 0, 0x7c},
    {0xe6, 0, 0x7d},  {0xf1, 0, 0x7e},  {0x100, 0, 0x7f}, {0x10d, 0, 0x80}, {0x123, 0, 0x81}, {0x12f, 0, 0x82},
    {0x13a, 0, 0x83}, {0x146, 0, 0x84}, {0x153, 0, 0x85}, {0x15c, 0, 0x87}, {0x16c, 0, 0x88}, {0x146, 0, 0x89},
    {0x17c, 0, 0x8a}, {0x190, 0, 0x8b}, {0x1a2, 0, 0x8c}, {0x1b4, 0, 0x8d},
}};
constexpr std::array<Preset, default_count> defaults{{
    {0x1c3, 0x1d3, 0x9d}, {0x1d9, 0x1f3, 0x9e}, {0x100, 0x203, 0x9f}, {0x213, 0x22b, 0xa0}, {0x15c, 0x239, 0xa1},
    {0x23f, 0x24f, 0xa2}, {0x255, 0x26f, 0xa3}, {0x27f, 0x299, 0xa4}, {0x2a9, 0x2bb, 0xa5}, {0x2c5, 0x2d5, 0xa6},
    {0x2db, 0x2eb, 0xa7}, {0x16c, 0x2f1, 0xa8}, {0x2f7, 0x307, 0xa9}, {0x30d, 0x327, 0xaa}, {0x337, 0x351, 0xab},
    {0x17c, 0x361, 0xac}, {0x153, 0x369, 0xad},
}};
// Where the original handles one file of a list: its calls making the path,
// loading and copying the title (the `bl`s), the assertion's first call,
// and the assertion's stack buffers (one pair per file, then one for the
// final check).
struct List {
    std::uint32_t array, count;  // CTACTICS fields
    std::uint32_t first_block, block_size;
    std::uint32_t first_path, first_load, first_title, first_assertion;  // the first file's code is shorter
    std::uint32_t path, load, title, assertion;                          // offsets in later blocks
    std::int32_t name, extension;
    std::uint32_t final_assertion, final_line;
    bool keep_offset;  // the original keeps the record's offset in r19 (not for the first file)
};
constexpr List preset_list{field::extra_formations, field::extra_count, 0x103ad1f4, 0x94,
                           0x103ad17c, 0x103ad190, 0, 0x103ad1b8, 0x14, 0x30, 0, 0x58,
                           0x7d0, 0x1b8, 0x103ae1b0, 0x92, false};
constexpr List default_list{field::formations, field::formation_count, 0x103ae2cc, 0xa4,
                            0x103ae248, 0x103ae25c, 0x103ae270, 0x103ae290, 0x14, 0x30, 0x48, 0x68,
                            0x430, 0xd0, 0x103aed2c, 0xb4, true};

// Loads each file of a list into the next record of its array (a file that
// fails to load leaves its record to the next one) and asserts when the
// array is full before the list ends or not full after it. Counts in r30.
template <std::size_t size>
void load_list(Cpu& c, const Frame& frame, std::uint32_t tactics, const List& list,
               const std::array<Preset, size>& files) {
    const auto pool = c.toc(pool_slot);
    std::int32_t loaded = 0;
    c.set(30, 0);
    for (std::uint32_t k = 0; k < size; ++k) {
        const auto block = list.first_block + (k - 1) * list.block_size;
        const auto path_call = k ? block + list.path : list.first_path;
        const auto load_call = k ? block + list.load : list.first_load;
        const auto title_call = k ? block + list.title : list.first_title;
        const auto name = list.name - std::int32_t(k) * 0x20, extension = list.extension - std::int32_t(k) * 8;
        if (loaded >= c.i32(tactics + list.count)) {
            fail(c, frame, k ? block + list.assertion : list.first_assertion, name, extension, files[k].line);
            continue;
        }
        c.set({{3, pool + folder}, {4, pool + files[k].file}});
        c.call(folder_path, path_call + 4);
        const auto path = c.r(3);
        const auto offset = std::uint32_t(loaded) * formation_size;
        if (list.keep_offset && k) c.set(19, offset);
        const auto record = c.u32(tactics + list.array) + offset;
        c.set({{3, record}, {4, path}, {5, 0}, {6, 1}});
        c.link(load_call + 4);
        c.set(3, formation::load_from_disk(c, record, path, 0, 1));
        if (!std::int8_t(c.r(3))) continue;
        if (files[k].title) {
            c.set({{3, c.u32(tactics + list.array) + offset}, {4, pool + files[k].title}});
            c.call(copy_string, title_call + 4);
        }
        c.set(30, std::uint32_t(++loaded));
    }
    if (loaded < c.i32(tactics + list.count))
        fail(c, frame, list.final_assertion, list.name - std::int32_t(size) * 0x20,
             list.extension - std::int32_t(size) * 8, list.final_line);
}

// A step of reading or writing the saved game's entry that failed: the
// assertion (its first call, stack buffers, line), then the entry's close.
struct Abort {
    std::uint32_t at;
    std::int32_t name, extension;
    std::uint32_t line, back;
};
class Entry {
public:
    Entry(Cpu& cpu, const Frame& frame, std::uint32_t slot) : c(cpu), frame_(frame), slot_(slot) {}
    // The entry routine's count of items transferred.
    std::uint32_t transfer(std::uint32_t routine, std::uint32_t buffer, std::uint32_t size, std::uint32_t count,
                           std::uint32_t back) const {
        c.set({{3, buffer}, {4, size}, {5, count}, {6, c.u32(slot_)}});
        c.call(routine, back);
        return c.r(3);
    }
    // `a` and `b` are what the original compared last (cr0 stays so).
    std::uint32_t abort(const Abort& site, std::uint32_t a, std::uint32_t b) const {
        compare(c, std::int32_t(a), std::int32_t(b));
        fail(c, frame_, site.at, site.name, site.extension, site.line);
        close(site.back);
        return 0;
    }
    void close(std::uint32_t back) const {
        c.set(3, slot_);
        c.call(close_entry, back);
    }

private:
    Cpu& c;
    const Frame& frame_;
    std::uint32_t slot_;
};

// One array of formation records in the saved game: its count, then the
// records. The original's registers and code addresses for each array.
struct FormationArray {
    std::uint32_t array, count;
    std::uint32_t count_read;
    Abort no_count;
    unsigned count_register, flag_register, array_register;
    std::uint32_t allocated;
    Abort no_array;
    std::uint32_t version;
    // Old saved games: one 0x1888-byte record at a time.
    unsigned index_register, offset_register, limit_register, player_register, word_register, pair_register;
    std::uint32_t record_read;
    Abort no_record;
    std::uint32_t first_instruction, second_instruction, old_option;
    // Current saved games: the whole array.
    std::uint32_t array_read;
    Abort short_array;
    std::uint32_t option;
};
constexpr FormationArray extra_formations{
    field::extra_formations, field::extra_count, 0x103b1e20, {0x103b1e3c, 0x324, 0xcc, 0x7d7, 0x103b1e80},
    17, 19, 18, 0x103b1e9c, {0x103b1ee4, 0x304, 0xc4, 0x7e0, 0x103b1f28}, 0x103b1f38,
    19, 20, 17, 18, 22, 21, 0x103b1f74, {0x103b1f90, 0x2e4, 0xbc, 0x7ed, 0x103b1fd4}, 0x103b2028, 0x103b2040, 0x103b2078,
    0x103b20a4, {0x103b20c4, 0x2c4, 0xb4, 0x803, 0x103b2108}, 0x103b2144};
constexpr FormationArray first_formations{
    field::formations, field::formation_count, 0x103b216c, {0x103b2188, 0x2a4, 0xac, 0x815, 0x103b21cc},
    17, 18, 25, 0x103b21e8, {0x103b2230, 0x284, 0xa4, 0x81e, 0x103b2274}, 0x103b2284,
    23, 22, 18, 24, 20, 21, 0x103b22c0, {0x103b22dc, 0x264, 0x9c, 0x82b, 0x103b2320}, 0x103b2374, 0x103b238c, 0x103b23c4,
    0x103b23f0, {0x103b2410, 0x244, 0x94, 0x841, 0x103b2454}, 0x103b2494};

// Team instructions below 0x8000 come from before option 0x20000 existed.
constexpr std::int32_t option_limit = 0x8000;
constexpr std::uint32_t old_team_option = 0x20000;

bool old_save(Cpu& c, std::uint32_t back) {
    c.set(3, c.u32(c.toc(save_slot)));
    c.call(save_version, back);
    return std::int32_t(c.r(3)) < old_save_version;
}

bool read_formations(Cpu& c, const Entry& entry, std::uint32_t tactics, const FormationArray& a) {
    const auto counted = entry.transfer(read_entry, tactics + a.count, 4, 1, a.count_read);
    if (counted != 1) return entry.abort(a.no_count, counted, 1);
    const auto count = c.u32(tactics + a.count);
    c.set(a.count_register, count);
    c.put32(tactics + a.array, new_formations(c, count, a.flag_register, a.array_register, a.allocated));
    if (!c.u32(tactics + a.array)) return entry.abort(a.no_array, 0, 0);
    const auto record = [&](std::uint32_t offset) { return c.u32(tactics + a.array) + offset; };
    if (old_save(c, a.version)) {
        // Records without the 64-bit player instructions: widen the 32-bit
        // ones (at 0x58c) and set two instruction fields of each player.
        c.set({{a.index_register, 0}, {a.limit_register, std::uint32_t(option_limit)}, {a.offset_register, 0}});
        std::int32_t index = 0;
        for (; index < c.i32(tactics + a.count); ++index) {
            const auto offset = std::uint32_t(index) * formation_size;
            c.set({{a.index_register, std::uint32_t(index)}, {a.offset_register, offset}});
            const auto items = entry.transfer(read_entry, record(offset), old_formation_size, 1, a.record_read);
            if (items != 1) return entry.abort(a.no_record, items, 1);
            for (std::uint32_t player = 0; player < std::uint32_t(formation::players); ++player) {
                c.set({{a.word_register, player * 4}, {a.player_register, player}, {a.pair_register, player * 8}});
                const auto instructions = record(offset) + formation::field::instructions + player * 8;
                c.put32(instructions + 4, c.u32(record(offset) + 0x58c + player * 4));
                c.put32(instructions, 0);
                c.set({{3, record(offset)}, {4, player}, {5, 8}, {6, 0}});
                c.call(set_instruction, a.first_instruction);
                c.set({{3, record(offset)}, {4, player}, {5, 2}, {6, 0}});
                c.call(set_instruction, a.second_instruction);
            }
            c.set({{a.word_register, 44}, {a.player_register, 11}, {a.pair_register, 88}});
            if (c.i32(record(offset) + formation::disk::team_instructions) < option_limit) {
                c.set({{3, record(offset)}, {4, old_team_option}});
                c.call(set_team_option, a.old_option);
            }
        }
        c.set({{a.index_register, std::uint32_t(index)}, {a.offset_register, std::uint32_t(index) * formation_size}});
        return true;
    }
    const auto read = entry.transfer(read_entry, c.u32(tactics + a.array), formation_size, c.u32(tactics + a.count), a.array_read);
    if (c.u32(tactics + a.count) != read) return entry.abort(a.short_array, c.u32(tactics + a.count), read);
    c.set({{20, 0}, {17, 0}, {18, old_team_option}, {19, std::uint32_t(option_limit)}});
    std::int32_t index = 0;
    for (; index < c.i32(tactics + a.count); ++index) {
        const auto offset = std::uint32_t(index) * formation_size;
        c.set({{20, std::uint32_t(index)}, {17, offset}});
        if (c.i32(record(offset) + formation::disk::team_instructions) < option_limit) {
            c.set({{3, record(offset)}, {4, old_team_option}});
            c.call(set_team_option, a.option);
        }
    }
    c.set({{20, std::uint32_t(index)}, {17, std::uint32_t(index) * formation_size}});
    return true;
}
}

std::uint32_t construct(Cpu& c, std::uint32_t tactics, std::uint32_t archive) {
    Frame frame(c, 0x103acf80, 0xb20, {.first_gpr = 19});
    const auto pool = c.toc(pool_slot);
    c.set({{21, archive}, {23, c.toc(volume_slots[1])}, {24, c.toc(formation_constructor_slot)},
           {25, c.toc(formation_destructor_slot)}, {27, c.toc(success_slot)}, {28, c.toc(message_slot)}, {29, pool},
           {30, c.toc(volume_slots[0])}, {31, tactics}});
    wait_for_cd(c, frame);
    for (std::uint32_t human = 0; human < std::uint32_t(human_count); ++human)
        c.put8(tactics + field::backups + human * backup_size + field::backup_active, 0);
    c.put8(tactics + field::backup_count, 0);
    if (archive) {
        c.set({{3, tactics}, {4, archive}});
        c.link(0x103ad110);
        c.set(3, load(c, tactics, archive));
        compare(c, std::int8_t(c.r(3)), 0);
        if (std::int8_t(c.r(3))) return tactics;
    }

    c.put32(tactics + field::extra_count, preset_count);
    const auto extra = c.u32(tactics + field::extra_count);
    c.set(19, extra);
    c.put32(tactics + field::extra_formations, new_formations(c, extra, 21, 20, 0x103ad13c));
    load_list(c, frame, tactics, preset_list, presets);
    c.put32(tactics + field::formation_count, default_count);
    const auto first = c.u32(tactics + field::formation_count);
    c.set(19, first);
    c.put32(tactics + field::formations, new_formations(c, first, 20, 26, 0x103ae208));
    load_list(c, frame, tactics, default_list, defaults);

    // A team record for every club, then 16 for the human managers' clubs
    // and 16 for their national teams.
    c.put32(tactics + field::team_count, c.u32(c.toc(club_count_slot)) + 0x20);
    c.set(3, c.u32(tactics + field::team_count) * team_size);
    c.call(allocate, 0x103aed84);
    c.put32(tactics + field::teams, c.r(3));
    if (!c.u32(tactics + field::teams)) {
        compare(c, 0, 0);
        fail(c, frame, 0x103aeda8, 0x1f0, 0x40, 0xc3);
        return tactics;
    }
    std::int32_t index = 0;
    c.set({{20, 0}, {19, 0}});
    for (; index < c.i32(tactics + field::team_count); ++index) {
        const auto offset = std::uint32_t(index) * team_size, team = c.u32(tactics + field::teams) + offset;
        c.set({{20, std::uint32_t(index)}, {19, offset}, {21, team}});
        c.set({{3, team}, {4, 0}, {5, team_size}});
        c.call(clear, 0x103aee10);
        reset_team(c, team, true);
    }
    c.set({{20, std::uint32_t(index)}, {19, std::uint32_t(index) * team_size}});

    // Results for every formation record, each knowing its index.
    c.put32(tactics + result_count, c.u32(tactics + field::formation_count) + c.u32(tactics + field::extra_count));
    c.set(3, c.u32(tactics + result_count) * results_size);
    c.call(new_block, 0x103aef34);
    c.put32(tactics + field::results, c.r(3));
    if (!c.u32(tactics + field::results)) {
        compare(c, 0, 0);
        fail(c, frame, 0x103aef58, 0x1d0, 0x38, 0xe2);
        return tactics;
    }
    index = 0;
    c.set({{20, 0}, {19, 0}});
    for (; index < c.i32(tactics + result_count); ++index) {
        const auto offset = std::uint32_t(index) * results_size, results = c.u32(tactics + field::results) + offset;
        c.set({{20, std::uint32_t(index)}, {19, offset}, {21, results}});
        c.set({{3, results}, {4, 0}, {5, results_size}});
        c.call(clear, 0x103aefc0);
        c.put16(results, std::uint32_t(index));
    }
    c.set({{20, std::uint32_t(index)}, {19, std::uint32_t(index) * results_size}});
    // Shape codes (formation::update_shape) of the standard formations; the
    // first stays as it is.
    constexpr std::array<std::uint32_t, 9> standard_shapes{0x280e8a89, 0x280e8b81, 0x281b06c1, 0x381506c1, 0x2a2a06c1,
                                                           0x281b2381, 0x288a26c1, 0x101f06c1, 0x381b0381};
    for (std::uint32_t i = 0; i < standard_shapes.size(); ++i) c.put32(tactics + shapes + 4 + i * 4, standard_shapes[i]);
    // The registers the original builds them in, and its last loop compare.
    c.set({{4, standard_shapes[7]}, {5, standard_shapes[6]}, {6, standard_shapes[5]}, {7, standard_shapes[4]},
           {8, 0x281b0000}});
    compare(c, index, c.i32(tactics + result_count));
    return tactics;
}

std::uint32_t load(Cpu& c, std::uint32_t tactics, std::uint32_t archive) {
    Frame frame(c, 0x103b1d00, 0x3d0, {.first_gpr = 17});
    const auto pool = c.toc(pool_slot);
    c.set({{23, c.toc(formation_destructor_slot)}, {24, c.toc(formation_constructor_slot)}, {26, c.toc(save_slot)},
           {27, c.toc(success_slot)}, {28, c.toc(message_slot)}, {29, pool}, {30, tactics},
           {31, c.toc(staff_count_slot)}});
    if (!archive) {
        compare(c, 0, 0);
        fail(c, frame, 0x103b1d4c, 0x364, 0xdc, 0x7c8);
        return 0;
    }
    const auto slot = frame.at(0x38);
    c.set({{3, archive}, {4, pool + entry_name}, {5, 0}, {6, 0x16}, {7, 4}});
    c.call(open_entry, 0x103b1da8);
    c.put32(slot, c.r(3));
    if (!c.r(3)) {
        compare(c, 0, 0);
        fail(c, frame, 0x103b1dc8, 0x344, 0xd4, 0x7d0);
        return 0;
    }
    const Entry entry(c, frame, slot);
    const auto read = [&](std::uint32_t buffer, std::uint32_t size, std::uint32_t count, std::uint32_t back) {
        return entry.transfer(read_entry, buffer, size, count, back);
    };
    if (!read_formations(c, entry, tactics, extra_formations)) return 0;
    if (!read_formations(c, entry, tactics, first_formations)) return 0;

    // Team records: the clubs' and the human managers' first 16.
    const auto count = [&] { return c.u32(tactics + field::team_count); };
    const auto teams = [&] { return c.u32(tactics + field::teams); };
    if (const auto items = read(tactics + field::team_count, 4, 1, 0x103b24bc); items != 1)
        return entry.abort({0x103b24d8, 0x224, 0x8c, 0x853, 0x103b251c}, items, 1);
    if (old_save(c, 0x103b252c)) {
        c.set(3, (count() + 0x10) * old_team_size);
        c.call(allocate, 0x103b254c);
        const auto old = c.r(3);
        c.set(18, old);
        if (!old) return entry.abort({0x103b2568, 0x204, 0x84, 0x85f, 0x103b25ac}, 0, 0);
        if (const auto items = read(old, old_team_size, count(), 0x103b25c4); count() != items)
            return entry.abort({0x103b25e4, 0x1e4, 0x7c, 0x867, 0x103b2628}, count(), items);
        c.set(3, (count() + 0x10) * team_size);
        c.call(allocate, 0x103b263c);
        c.put32(tactics + field::teams, c.r(3));
        if (!teams()) return entry.abort({0x103b2660, 0x1c4, 0x74, 0x870, 0x103b26a4}, 0, 0);
        for (std::int32_t index = 0; index < std::int32_t(count()); ++index) {
            const auto from = old + std::uint32_t(index) * old_team_size;
            convert_team(c, from, teams() + std::uint32_t(index) * team_size);
            // The original copies the line-up through r17 and r19.
            c.set({{17, 7}, {19, c.u8(from + 0x87)}});
        }
        c.set(3, old);
        c.call(release, 0x103b28d8);
    } else {
        c.set(3, (count() + 0x10) * team_size);
        c.call(allocate, 0x103b28ec);
        c.put32(tactics + field::teams, c.r(3));
        if (!teams()) return entry.abort({0x103b2910, 0x1a4, 0x6c, 0x88c, 0x103b2954}, 0, 0);
        if (const auto items = read(teams(), team_size, count(), 0x103b296c); count() != items)
            return entry.abort({0x103b298c, 0x184, 0x64, 0x894, 0x103b29d0}, count(), items);
    }

    // Formation results and the standard shapes.
    if (const auto items = read(tactics + result_count, 4, 1, 0x103b29ec); items != 1)
        return entry.abort({0x103b2a08, 0x164, 0x5c, 0x89d, 0x103b2a4c}, items, 1);
    c.set(3, c.u32(tactics + result_count) * results_size);
    c.call(new_block, 0x103b2a60);
    c.put32(tactics + field::results, c.r(3));
    if (!c.u32(tactics + field::results)) return entry.abort({0x103b2a84, 0x144, 0x54, 0x8a6, 0x103b2ac8}, 0, 0);
    const auto results = read(c.u32(tactics + field::results), results_size, c.u32(tactics + result_count), 0x103b2ae0);
    if (c.u32(tactics + result_count) != results) return entry.abort({0x103b2b00, 0x124, 0x4c, 0x8ae, 0x103b2b44}, c.u32(tactics + result_count), results);
    if (const auto items = read(tactics + shapes, 4, 10, 0x103b2b60); items != 10)
        return entry.abort({0x103b2b7c, 0x104, 0x44, 0x8b6, 0x103b2bc0}, items, 10);

    // The other 16 human managers' team records follow, when the saved game
    // has them. Without them the count grows by 16 and they start empty
    // (pick 0). An old saved game's records are read but never used.
    const auto add_empty_teams = [&](unsigned index_register, unsigned offset_register, unsigned record_register,
                                     std::uint32_t back, auto&& before) {
        c.put32(tactics + field::team_count, count() + 0x10);
        std::uint32_t index = count() - 0x10;
        for (; std::int32_t(index) < std::int32_t(count()); ++index) {
            const auto offset = index * team_size, team = teams() + offset;
            c.set({{index_register, index}, {offset_register, offset}});
            before(index);
            c.set(record_register, team);
            c.set({{3, team}, {4, 0}, {5, team_size}});
            c.call(clear, back);
            reset_team(c, team, false);
        }
        c.set({{index_register, index}, {offset_register, index * team_size}});
    };
    if (old_save(c, 0x103b2bd0)) {
        c.set(3, (count() + 0x10) * old_team_size);
        c.call(allocate, 0x103b2bf0);
        const auto old = c.r(3);
        c.set(19, old);
        if (!old) return entry.abort({0x103b2c0c, 0xe4, 0x3c, 0x8c2, 0x103b2c50}, 0, 0);
        if (read(old + count() * old_team_size, old_team_size, 0x10, 0x103b2c74) != 0x10) {
            add_empty_teams(22, 21, 18, 0x103b2eb0, [&](std::uint32_t index) {
                // Converted first, then cleared: only the clearing remains.
                c.set(20, index * old_team_size);
                convert_team(c, old + index * old_team_size, teams() + index * team_size);
            });
            c.set(20, (count()) * old_team_size);
        }
        c.set(3, old);
        c.call(release, 0x103b2fdc);
    } else if (read(teams() + count() * team_size, team_size, 0x10, 0x103b3000) != 0x10) {
        add_empty_teams(21, 20, 22, 0x103b303c, [](std::uint32_t) {});
    }

    // Staff ids past the staff table are dropped.
    const auto staff_count = c.toc(staff_count_slot);
    const auto check = [&](std::uint32_t address) {
        if (c.i32(address) >= c.i32(staff_count)) c.put32(address, 0xffffffffu);
    };
    std::int32_t index = 0;
    for (; index < std::int32_t(count()); ++index) {
        const auto team = teams() + std::uint32_t(index) * team_size;
        for (std::uint32_t slot = 0; slot < std::uint32_t(squad_size); ++slot) check(team + field::squad + slot * 4);
        for (std::uint32_t which = 0; which < 2; ++which) check(team + field::captains + which * 4);
        const auto last = team + field::takers + (std::uint32_t(set_pieces) * 2 - 1) * 4;
        const auto last_id = c.u32(last);
        for (std::uint32_t taker = 0; taker < std::uint32_t(set_pieces) * 2; ++taker) check(team + field::takers + taker * 4);
        // The registers the original's loops leave for the entry's close.
        c.set({{3, team}, {4, 0xffffffffu}, {5, last_id}, {6, 8}, {7, 0x28}, {9, 0x84}, {11, 5}, {12, 2}});
    }
    c.set({{0, count()}, {8, std::uint32_t(index) * team_size}, {10, std::uint32_t(index)}});
    compare(c, index, std::int32_t(count()));
    entry.close(0x103b326c);
    return 1;
}

std::uint32_t save(Cpu& c, std::uint32_t tactics, std::uint32_t archive) {
    Frame frame(c, 0x103b3290, 0x210, {.first_gpr = 28});
    const auto pool = c.toc(pool_slot);
    c.set({{28, tactics}, {29, c.toc(success_slot)}, {30, c.toc(message_slot)}, {31, pool}});
    if (!archive) {
        compare(c, 0, 0);
        fail(c, frame, 0x103b32d8, 0x1d4, 0x8c, 0x96c);
        return 0;
    }
    const auto slot = frame.at(0x38);
    c.set({{3, archive}, {4, pool + entry_name}, {5, 1}, {6, 0x16}, {7, 4}});
    c.call(open_entry, 0x103b3334);
    c.put32(slot, c.r(3));
    if (!c.r(3)) {
        compare(c, 0, 0);
        fail(c, frame, 0x103b3354, 0x1b4, 0x84, 0x974);
        return 0;
    }
    const Entry entry(c, frame, slot);
    // Each array's count, then its records; the code for each pair of writes
    // is 0xfc bytes long.
    constexpr std::array<std::pair<std::uint32_t, std::uint32_t>, 4> arrays{{
        {field::extra_formations, field::extra_count}, {field::formations, field::formation_count},
        {field::teams, field::team_count}, {field::results, result_count}}};
    constexpr std::array<std::uint32_t, 4> sizes{formation_size, formation_size, team_size, results_size};
    for (std::uint32_t i = 0; i < arrays.size(); ++i) {
        const auto [array, count] = arrays[i];
        const auto code = 0x103b33a8 + i * 0xfc;  // the count's write
        const auto name = 0x194 - std::int32_t(i) * 0x40, extension = 0x7c - std::int32_t(i) * 0x10;
        const auto line = 0x97b + i * 0x10;
        if (const auto items = entry.transfer(write_entry, tactics + count, 4, 1, code + 4); items != 1)
            return entry.abort({code + 0x20, name, extension, line, code + 0x64}, items, 1);
        const auto written = entry.transfer(write_entry, c.u32(tactics + array), sizes[i], c.u32(tactics + count), code + 0x80);
        if (c.u32(tactics + count) != written)
            return entry.abort({code + 0xa0, name - 0x20, extension - 8, line + 8, code + 0xe4}, c.u32(tactics + count), written);
    }
    const auto items = entry.transfer(write_entry, tactics + shapes, 4, 10, 0x103b379c);
    if (items != 10) return entry.abort({0x103b37b8, 0x94, 0x3c, 0x9bb, 0x103b37fc}, items, 10);
    compare(c, std::int32_t(items), 10);
    entry.close(0x103b380c);
    return 1;
}
}
