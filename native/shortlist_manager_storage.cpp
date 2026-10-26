#include "shortlist_manager_detail.hpp"

// The shortlist manager's construction (fresh, or from the save archive) and
// "shortlist manager" in the save archive: load_storage reads every save
// version, save_storage writes the current one.
namespace cm::aot::shortlist_manager {
using namespace detail;
using namespace storage;
namespace {
namespace routine {
constexpr std::uint32_t fatal_exit = 0x1076f7d0, abs = 0x10775550;
constexpr std::uint32_t read = 0x10747a80;   // (buffer, size, count, archive): items read
constexpr std::uint32_t write = 0x10747cc0;  // (buffer, size, count, archive): items written
constexpr std::uint32_t block_move = 0x1077fc28;  // Toolbox glue: BlockMoveData(from, to, size)
constexpr std::uint32_t construct_array = 0x10773320;  // (array, constructor TV, 0, size, count)
constexpr std::uint32_t make_date = 0x107156b0;     // (date, day, year)
constexpr std::uint32_t default_date = 0x10715880;  // (date): 40, 1972, 1
}
// The save versions the reader tells apart (absolute values, signed compares).
constexpr std::int32_t version_without_legacy = 0x3e016;  // from here on no legacy array; through it 25 records a club
constexpr std::int32_t version_complete = 0x3e017;        // from here on sixteen counts and lists, no reset after
constexpr std::uint32_t club_size = 0x248;
constexpr std::uint32_t legacy_record_size = 0x18;

// A record's defaults: kind 11, zero values, no links, rank -1, flags bits
// 0x78 set and 0x80, 0x04 clear (the other bits and the padding as they
// were). The date is the caller's.
void default_record(const Row& record) {
    record.kind() = Row::player;
    record.positions() = 0;
    for (const std::uint32_t at : {4u, 8u, 0xcu}) record.word(at) = 0;
    for (const std::uint32_t at : {0x10u, 0x14u, 0x18u}) record.word(at) = -1;
    record.flags() = std::uint8_t((record.flags() | 0x78) & ~0x84);
    record.rank() = -1;
}

// A record from a legacy save's 25 (the same layout; kind, padding and the
// flags' other bits stay). Quirk: the value at +2 comes from the legacy
// record's halfword at +0 (its kind byte and the byte after it), not +2
// (0x10358984 lha r5,0(r4); sthx r5 at +2).
void copy_legacy_record(const Row& record, const Row& legacy) {
    record.positions() = Field<std::int16_t>{legacy.c, legacy.address}.get();
    for (const std::uint32_t at : {4u, 8u, 0xcu}) record.word(at) = legacy.word(at).get();
    for (const std::uint32_t at : {0x10u, 0x14u}) record.word(at) = legacy.word(at).get();
    record.flags() = std::uint8_t((record.flags() & ~0x78) | (legacy.flags() & 0x78));
    record.word(0x18) = legacy.word(0x18).get();
    record.flags() = std::uint8_t((record.flags() & ~0x80) | (legacy.flags() & 0x80));
    record.rank() = legacy.rank().get();
    record.flags() = std::uint8_t((record.flags() & ~0x04) | (legacy.flags() & 0x04));
    record.date().copy_from(legacy.date());
}

// An assertion report whose success flag is cleared through r29 (r28 in
// construct_storage), as every site here does.
void failed(Cpu& c, const Frame& frame, std::uint32_t pool, std::uint32_t at, std::int32_t name, std::int32_t extension,
            std::uint32_t line) {
    report(c, pool, at, frame.at(name), frame.at(extension), line, false);
}
// ... then fatal_exit(-1), whose call follows the report's at +0x44.
void fatal(Cpu& c, const Frame& frame, std::uint32_t pool, std::uint32_t at, std::int32_t name, std::int32_t extension,
           std::uint32_t line) {
    failed(c, frame, pool, at, name, extension, line);
    Program{c}.call(routine::fatal_exit, at + 0x44, {std::uint32_t(-1)});
}
}

std::uint32_t construct_storage(Cpu& c, std::uint32_t address, std::uint32_t archive) {
    Frame frame(c, 0x10356410, 0xd0, {.first_gpr = 17});
    Program program{c};
    const Manager manager{c, address};
    const auto club_count = c.toc(slot::club_count), clubs = c.toc(slot::clubs), pool = c.toc(slot::pool);
    keep(c, {{31, club_count}, {21, address}, {27, clubs}, {17, archive}, {28, c.toc(slot::success)},
             {29, c.toc(slot::message)}, {30, pool}});
    manager.clubs() = program.malloc(std::uint32_t(c.i32(club_count)) * ClubEntry::size, 0x10356448);
    if (!nonzero_logical(c, manager.clubs())) fatal(c, frame, pool, 0x1035646c, 0x70, 0x48, 0x51);
    // Both banks null. Quirk: each unrolled loop still compares its counter
    // (0) with 16 once.
    for (std::uint32_t bank = 0; bank < Manager::banks; ++bank) {
        compare(c, 0, 16);
        for (std::uint32_t category = 0; category < Manager::categories; ++category) manager.list(bank, category) = 0;
    }
    if (nonzero_logical(c, archive)) {
        enter(c, 0x10356564, {address, archive});
        load_storage(c, address, archive);
        return result(c, address);
    }

    // Fresh: every club's forty records at their defaults, dated 1 1997.
    Kept<23> club{c, 0};
    Kept<25> entry_offset{c, 0};
    Kept<24> club_offset{c, 0};
    for (; compare(c, club, c.i32(club_count)) < 0;
         entry_offset = entry_offset + ClubEntry::size, club_offset = club_offset + club_size, club = club + 1) {
        const auto entry = [&] { return ClubEntry{c, manager.clubs() + entry_offset}; };
        entry().rows() = program.malloc(Row::size * Row::per_club, 0x10356584);
        if (!nonzero_logical(c, entry().rows())) fatal(c, frame, pool, 0x103565b0, 0x50, 0x40, 0x6c);
        Kept<26> record_offset{c, 0};
        entry().status() = 0;
        Kept<22> index{c, 0};
        entry().reputation() = c.i16(c.u32(clubs) + club_offset + 0x1ec);
        entry().changes() = 0;
        do {
            const auto records = entry().rows().get();
            const Row record{c, records + record_offset};
            default_record(record);
            keep(c, {{17, records}, {18, records}, {19, records}, {20, records}});
            c.set({{6, record.flags()}, {7, 0}, {8, records}, {9, records}, {10, records}});
            const Date made{c, frame.at(0x38)};
            program.call(routine::make_date, 0x10356718, {made.address, 1, 0x7cd});
            index = index + 1;
            const auto reloaded = entry().rows().get();
            const Date date{c, reloaded + record_offset + 0x1c};
            c.set({{3, reloaded}, {4, date.address}});
            record_offset = record_offset + Row::size;
            date.copy_from(made);
        } while (compare(c, index, Row::per_club) < 0);
    }
    manager.capacity_by_staff() = 0;
    manager.capacity_by_club() = 0;
    manager.count_by_staff() = 0;
    manager.count_by_club() = 0;
    manager.links_by_staff() = 0;
    manager.links_by_club() = 0;
    enter(c, 0x10356790, {address, 1});
    reset_storage(c, address);
    return result(c, address);
}

std::uint32_t initialize_record(Cpu& c, std::uint32_t record) {
    Frame frame(c, 0x10358af0, 0x40, {.first_gpr = 31});
    const Kept<31> kept{c, record};
    Program{c}.call(routine::default_date, 0x10358b0c, {record + 0x1c});
    return result(c, record);
}

// Saves before version_complete lack the sixteenth category's count and
// lists: the count reads as 0, and in place of each bank's sixteenth list
// the first word of bank 0's is set to -1 (quirk: bank 0's both times), and
// the lists are rebuilt (reset_storage) at the end. Saves before
// version_without_legacy carry an array of 24-byte records read and
// dropped; saves through it 25 records a club (the other fifteen at their
// defaults). A failed read reports and returns, keeping what was read.
void load_storage(Cpu& c, std::uint32_t address, std::uint32_t archive) {
    Frame frame(c, 0x10358020, 0x710, {.first_gpr = 14});
    Program program{c};
    const Manager manager{c, address};
    const auto version = c.toc(slot::save_version), pool = c.toc(slot::pool), club_count = c.toc(slot::club_count);
    const auto clubs = c.toc(slot::clubs), record_constructor = c.toc(slot::construct_10358af0_tv);
    keep(c, {{19, archive}, {18, address}, {25, record_constructor}, {26, version}, {27, clubs},
             {29, c.toc(slot::success)}, {30, c.toc(slot::message)}, {31, pool}, {28, club_count}});
    const auto read = [&](std::uint32_t buffer, std::uint32_t size, std::uint32_t count, std::uint32_t back) {
        return std::int32_t(program.call(routine::read, back, {buffer, size, count, archive}));
    };
    const auto one = [&](std::uint32_t buffer, std::uint32_t back) { return compare(c, read(buffer, 4, 1, back), 1) == 0; };
    // |version| (abs() of the save's), with `lis r4,4` after it at some sites.
    const auto saved_version = [&](std::uint32_t back, bool marks_r4) {
        const auto value = std::int32_t(program.call(routine::abs, back, {c.u32(version)}));
        if (marks_r4) c.set(4, 0x40000);
        return value;
    };
    const auto fail = [&](std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line) {
        failed(c, frame, pool, at, name, extension, line);
    };

    if (!one(manager.capacity_by_staff().address(), 0x10358068)) return fail(0x10358084, 0x254, 0xac, 0x2be);
    manager.count_by_staff() = manager.capacity_by_staff().get();
    manager.count_by_club() = manager.capacity_by_staff().get();
    manager.capacity_by_club() = manager.capacity_by_staff().get();
    manager.links_by_staff() = program.malloc(std::uint32_t(manager.capacity_by_staff()) << 3, 0x103580e8);
    manager.links_by_club() = program.malloc(std::uint32_t(manager.capacity_by_club()) << 3, 0x103580f8);
    if ((!nonzero_logical(c, result(c, manager.links_by_staff())) || !nonzero_logical(c, manager.links_by_club())) &&
        nonzero(c, manager.capacity_by_staff()))
        return fail(0x10358134, 0x234, 0xa4, 0x2ce);
    if (const auto done = read(manager.links_by_staff(), 8, std::uint32_t(manager.count_by_staff()), 0x10358184);
        compare(c, manager.count_by_staff(), done) != 0)
        return fail(0x103581a4, 0x214, 0x9c, 0x2d5);
    program.toolbox(routine::block_move, 0x103581f4,
                    {manager.links_by_staff(), manager.links_by_club(), std::uint32_t(manager.count_by_staff()) << 3});
    program.qsort(manager.links_by_club(), std::uint32_t(manager.count_by_club()), 8,
                  c.toc(slot::compare_103663b0_tv), 0x1035820c);

    if (compare(c, saved_version(0x10358214, true), version_without_legacy) < 0) {
        const Field<std::int32_t> legacy_count{c, frame.at(0x38)};
        if (!one(legacy_count.address(), 0x10358238)) return fail(0x10358254, 0x1f4, 0x94, 0x2e7);
        const Kept<14> legacy{c, program.malloc(std::uint32_t(legacy_count.get()) * legacy_record_size, 0x103582a0)};
        if (!nonzero(c, std::int32_t(std::uint32_t(legacy))) && nonzero(c, legacy_count))
            return fail(0x103582c8, 0x1d4, 0x8c, 0x2f0);
        if (const auto done = read(legacy, legacy_record_size, std::uint32_t(legacy_count.get()), 0x1035831c);
            compare(c, legacy_count, done) != 0)
            return fail(0x1035833c, 0x1b4, 0x84, 0x2f8);
        program.free(legacy, 0x10358384);
    }

    // The category counts.
    {
        Kept<16> offset{c, 0};
        c.set(3, 0x40000);
        const Kept<14> zero{c, 0};
        Kept<17> category{c, 0};
        const Kept<15> limit{c, std::uint32_t(version_without_legacy)};
        do {
            if (compare(c, saved_version(0x103583a0, false), std::int32_t(std::uint32_t(limit))) > 0 ||
                compare(c, category, 15) < 0) {
                if (!one(address + 0x98 + offset, 0x103583c8)) return fail(0x103583e4, 0x194, 0x7c, 0x307);
            } else {
                manager.count(category) = std::int32_t(std::uint32_t(zero));
            }
            category = category + 1;
            offset = offset + 4;
        } while (compare(c, category, 16) < 0);
    }
    if (!one(manager.capacity().address(), 0x10358454)) return fail(0x10358470, 0x174, 0x74, 0x314);

    // Both banks' lists at that capacity, then their entries.
    {
        c.set(3, 0x40000);
        Kept<23> bank{c, 0};
        const Kept<14> limit{c, std::uint32_t(version_complete)};
        Kept<21> bank_offset{c, 0};
        do {
            Kept<22> category{c, 0};
            Kept<20> category_offset{c, 0};
            do {
                const auto list = program.malloc(std::uint32_t(manager.capacity()) * CategoryEntry::size, 0x103584d8);
                const Kept<15> slot_offset{c, bank_offset + category_offset + 0x18};
                const Field<std::uint32_t> slot{c, address + slot_offset};
                slot = list;
                if (!nonzero_logical(c, slot)) fatal(c, frame, pool, 0x10358504, 0x154, 0x6c, 0x322);
                if (compare(c, saved_version(0x1035854c, false), std::int32_t(std::uint32_t(limit))) < 0 &&
                    compare(c, category, 15) == 0) {
                    const auto bank_0_last = manager.list(0, 15).get();
                    c.set(3, bank_0_last);
                    c.put32(bank_0_last, std::uint32_t(-1));
                } else {
                    const Kept<16> count_offset{c, category_offset + 0x98};
                    const Field<std::int32_t> count{c, address + count_offset};
                    if (const auto done = read(slot, CategoryEntry::size, std::uint32_t(count.get()), 0x10358588);
                        compare(c, count, done) != 0)
                        return fail(0x103585a8, 0x134, 0x64, 0x330);
                }
                category = category + 1;
                category_offset = category_offset + 4;
            } while (compare(c, category, 16) < 0);
            bank = bank + 1;
            bank_offset = bank_offset + 0x40;
        } while (compare(c, bank, 2) < 0);
    }

    // The club table, then each present club's records.
    if (const auto done = read(manager.clubs(), ClubEntry::size, c.u32(club_count), 0x10358620);
        compare(c, c.i32(club_count), done) != 0)
        return fail(0x10358640, 0x114, 0x5c, 0x33a);
    Kept<20> club{c, 0};
    Kept<23> entry_offset{c, 0};
    Kept<22> club_offset{c, 0};
    for (; compare(c, club, c.i32(club_count)) < 0;
         entry_offset = entry_offset + ClubEntry::size, club_offset = club_offset + club_size, club = club + 1) {
        const ClubEntry entry{c, manager.clubs() + entry_offset};
        c.set(5, entry.address);
        // The saved pointer only tells whether the club had records.
        if (!nonzero_logical(c, entry.rows())) continue;
        // The club's score is raised to the club's current one.
        const auto score = c.i16(c.u32(clubs) + club_offset + 0x1ec);
        if (compare(c, entry.reputation(), score) < 0) entry.reputation() = score;
        const auto records = program.malloc(Row::size * Row::per_club, 0x103586c4);
        ClubEntry{c, manager.clubs() + entry_offset}.rows() = records;
        if (!nonzero_logical(c, ClubEntry{c, manager.clubs() + entry_offset}.rows()))
            return fail(0x103586f4, 0xf4, 0x54, 0x34d);
        if (compare(c, saved_version(0x10358738, true), version_without_legacy) > 0) {
            const auto stored = ClubEntry{c, manager.clubs() + entry_offset}.rows().get();
            if (compare(c, read(stored, Row::size, Row::per_club, 0x10358764),
                        Row::per_club) != 0)
                return fail(0x10358780, 0xd4, 0x4c, 0x357);
            continue;
        }
        // A legacy club: 25 records into a stack array, then forty.
        const auto legacy = frame.at(0x274);
        program.call(routine::construct_array, 0x103587d8,
                     {legacy, record_constructor, 0, Row::size, Row::legacy_per_club});
        if (compare(c, read(legacy, Row::size, Row::legacy_per_club, 0x103587ec),
                    Row::legacy_per_club) != 0)
            return fail(0x10358808, 0xb4, 0x44, 0x363);
        Kept<21> index{c, 0};
        Kept<24> record_offset{c, 0};
        do {
            const auto stored = ClubEntry{c, manager.clubs() + entry_offset}.rows().get();
            const Row record{c, stored + record_offset};
            record.kind() = Row::player;
            if (compare(c, index, Row::legacy_per_club) < 0) {
                copy_legacy_record(record, Row{c, legacy + record_offset});
            } else {
                default_record(record);
                keep(c, {{14, record_offset + 4}, {15, record_offset + 8}, {16, record_offset + 0xc},
                         {17, record_offset + 0x10}});
                c.set({{6, record.flags()}, {7, stored}, {8, 0}, {9, record_offset + 0x24}, {10, stored}});
                const Date made{c, frame.at(0x3c)};
                program.call(routine::make_date, 0x10358948, {made.address, 1, 0x7cd});
                const auto reloaded = ClubEntry{c, manager.clubs() + entry_offset}.rows().get();
                const Date date{c, reloaded + record_offset + 0x1c};
                c.set({{3, reloaded}, {4, date.address}});
                date.copy_from(made);
            }
            index = index + 1;
            record_offset = record_offset + Row::size;
        } while (compare(c, index, Row::per_club) < 0);
    }
    if (compare(c, saved_version(0x10358ac0, true), version_complete) < 0) {
        enter(c, 0x10358adc, {address, 0});
        reset_storage(c, address);
    }
}

// The links in their saved order, the sixteen counts, the capacity, both
// banks' lists and the club table, then the records of every club that has
// them. A failed write reports and returns.
void save_storage(Cpu& c, std::uint32_t address, std::uint32_t archive) {
    Frame frame(c, 0x10358b30, 0x180, {.first_gpr = 21});
    Program program{c};
    const Manager manager{c, address};
    const auto pool = c.toc(slot::pool), club_count = c.toc(slot::club_count);
    keep(c, {{28, archive}, {27, address}, {29, c.toc(slot::success)}, {30, c.toc(slot::message)}, {31, pool},
             {26, club_count}});
    const auto write = [&](std::uint32_t buffer, std::uint32_t size, std::uint32_t count, std::uint32_t back) {
        return std::int32_t(program.call(routine::write, back, {buffer, size, count, archive}));
    };
    const auto one = [&](std::uint32_t buffer, std::uint32_t back) { return compare(c, write(buffer, 4, 1, back), 1) == 0; };
    const auto fail = [&](std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line) {
        failed(c, frame, pool, at, name, extension, line);
    };

    if (!one(manager.count_by_staff().address(), 0x10358b6c)) return fail(0x10358b88, 0x130, 0x68, 0x3a1);
    if (const auto done = write(manager.links_by_staff(), 8, std::uint32_t(manager.count_by_staff()), 0x10358bdc);
        compare(c, manager.count_by_staff(), done) != 0)
        return fail(0x10358bfc, 0x110, 0x60, 0x3a9);
    {
        Kept<22> category{c, 0};
        Kept<25> offset{c, 0};
        do {
            if (!one(address + 0x98 + offset, 0x10358c60)) return fail(0x10358c7c, 0xf0, 0x58, 0x3b2);
            category = category + 1;
            offset = offset + 4;
        } while (compare(c, category, 16) < 0);
    }
    if (!one(manager.capacity().address(), 0x10358ce0)) return fail(0x10358cfc, 0xd0, 0x50, 0x3ba);
    {
        Kept<22> bank{c, 0};
        Kept<24> bank_offset{c, 0};
        do {
            Kept<21> category{c, 0};
            Kept<25> category_offset{c, 0};
            do {
                const Kept<23> count_offset{c, category_offset + 0x98};
                const Field<std::int32_t> count{c, address + count_offset};
                const auto list = c.u32(address + bank_offset + category_offset + 0x18);
                if (const auto done = write(list, CategoryEntry::size, std::uint32_t(count.get()), 0x10358d6c);
                    compare(c, count, done) != 0)
                    return fail(0x10358d8c, 0xb0, 0x48, 0x3c6);
                category = category + 1;
                category_offset = category_offset + 4;
            } while (compare(c, category, 16) < 0);
            bank = bank + 1;
            bank_offset = bank_offset + 0x40;
        } while (compare(c, bank, 2) < 0);
    }
    if (const auto done = write(manager.clubs(), ClubEntry::size, c.u32(club_count), 0x10358e00);
        compare(c, c.i32(club_count), done) != 0)
        return fail(0x10358e20, 0x90, 0x40, 0x3d0);
    Kept<23> club{c, 0};
    Kept<24> entry_offset{c, 0};
    for (; compare(c, club, c.i32(club_count)) < 0; entry_offset = entry_offset + ClubEntry::size, club = club + 1) {
        const auto records = ClubEntry{c, manager.clubs() + entry_offset}.rows().get();
        if (!nonzero_logical(c, result(c, records))) continue;
        if (compare(c, write(records, Row::size, Row::per_club, 0x10358e90),
                    Row::per_club) != 0)
            return fail(0x10358eac, 0x70, 0x38, 0x3de);
    }
}
}
