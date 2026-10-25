#include "player_stats_detail.hpp"
#include "player_stats_files.hpp"

// The player-stats object's destruction and "player stats.dat" (the file
// layout is in player_stats_detail.hpp, stats_file).
namespace cm::aot::player_stats {
using namespace detail;
namespace {
// An error site of load_stats/save_stats: the assertion report whose first
// call is at `at` (stack buffers `name` and `extension`, source line `line`),
// then fatal_exit(-1) when `fatal`, then the entry's close.
struct Site {
    std::uint32_t at;
    std::int32_t name, extension;
    std::uint32_t line;
    bool fatal = true;
};

// "player stats.dat" in the save archive; the original keeps the open entry
// at sp+0x38 and passes that slot's address to the close.
class Entry {
public:
    Entry(Cpu& cpu, const Frame& frame, std::uint32_t pool, bool writing)
        : c(cpu), frame(frame), pool(pool), writing(writing) {}
    std::uint32_t handle() const { return c.u32(frame.at(handle_slot)); }
    // Opens the entry: 0 when that fails.
    std::uint32_t open(std::uint32_t archive, std::uint32_t back) {
        const auto entry = Program{c}.open_entry(back, {archive, pool + text::stats_file, writing ? 1u : 0u,
                                                        stats_file::archive_type, stats_file::archive_flags});
        c.put32(frame.at(handle_slot), entry);
        return entry;
    }
    // One item of `size` bytes at `buffer`, the result tested with `cmpwi r3,1`.
    bool one(std::uint32_t buffer, std::uint32_t size, std::uint32_t back) {
        return compare(c, std::int32_t(transfer(buffer, size, 1, back)), 1) == 0;
    }
    // `count` items, the count read again after the call (`lwz r0,count; cmpw r0,r3`).
    bool all(std::uint32_t buffer, std::uint32_t size, const Field<std::int32_t>& count, std::uint32_t back) {
        const auto done = transfer(buffer, size, std::uint32_t(count.get()), back);
        return compare(c, count, std::int32_t(done)) == 0;
    }
    void close(std::uint32_t back) { Program{c}.close_entry(back, {frame.at(handle_slot)}); }
    // The failure tail: report, fatal exit, close the entry; the routine returns 0.
    std::uint32_t fail(const Site& site) {
        reported(site);
        close(site.at + (site.fatal ? 0x4c : 0x44));
        return result(c, 0);
    }
    // The report (the success flag cleared through r29) and the fatal exit.
    void reported(const Site& site) {
        report(c, pool, site.at, frame.at(site.name), frame.at(site.extension), site.line, false);
        if (site.fatal) Program{c}.fatal_exit(std::uint32_t(-1), site.at + 0x44);
    }

private:
    std::uint32_t transfer(std::uint32_t buffer, std::uint32_t size, std::uint32_t count, std::uint32_t back) {
        Program program{c};
        return writing ? program.write_entry(back, {buffer, size, count, handle()})
                       : program.read_entry(back, {buffer, size, count, handle()});
    }
    static constexpr std::int32_t handle_slot = 0x38;
    Cpu& c;
    const Frame& frame;
    std::uint32_t pool;
    bool writing;
};

// Whether the loaded save is older than the whole season records: abs() of
// its version against full_records_version (`lis r4,4; subi r0,r4,0x1feb;
// cmpw r3,r0`, which leaves 0x40000 in r4).
bool older_save(Cpu& c, std::uint32_t version, std::uint32_t back) {
    const auto value = Program{c}.call(routine::abs, back, {c.u32(version)});
    c.set(4, 0x40000);
    return compare(c, std::int32_t(value), stats_file::full_records_version) < 0;
}
}

std::uint32_t detail::new_history_file(Cpu& c, unsigned reg, std::uint32_t pool, std::uint32_t mode, std::uint32_t at) {
    Program program{c};
    c.set(reg, program.operator_new(history_file::size, at + 4));
    if (nonzero(c, std::int32_t(c.r(reg)))) {
        const auto path = program.folder_path(pool + text::temp_folder, pool + text::history_temp, at + 0x18);
        program.call(history_file::open, at + 0x28, {c.r(reg), path, mode});
    }
    return c.r(reg);
}

namespace {
// An older save's history archive kept 0x20-byte rows (the season record's
// first 0x10 bytes); each player's list is rewritten as CareerRows into a new
// file, which then replaces the old one.
void convert_history(Cpu& c, const Frame& frame, const Stats& stats, std::uint32_t pool) {
    Program program{c};
    const auto rows = frame.at(0xd4);  // a list on the stack
    program.call(list::create, 0x10307668, {rows});
    program.call(history_file::destroy, 0x10307674, {stats.archive(), 1});
    stats.archive() = new_history_file(c, 25, pool, history_file::old_mode, 0x10307678);
    const Kept<29> converted{c, program.operator_new(history_file::size, 0x103076ac)};
    if (nonzero(c, std::int32_t(std::uint32_t(converted))))
        program.call(history_file::open, 0x103076c0, {converted, pool + text::new_history_temp, history_file::mode});

    Kept<25> index{c, 0};
    Kept<26> offset{c, 0};
    for (; compare(c, index, stats.player_count()) < 0; index = index + 1, offset = offset + PlayerEntry::size) {
        const PlayerEntry entry{c, stats.players() + offset};
        const Kept<27> old_rows{c, program.call(history_file::read, 0x103076e4,
                                                {stats.archive(), std::uint32_t(entry.history_offset().get()),
                                                 std::uint32_t(entry.history_size().get())})};
        if (!nonzero(c, std::int32_t(std::uint32_t(old_rows)))) {
            c.set({{4, stats.players()}, {5, std::uint32_t(-1)}});
            entry.history_offset() = -1;
            entry.history_size() = -1;
            c.set(3, stats.players());
            continue;
        }
        c.put32(old_rows + 0xc, c.u32(old_rows + 4));  // the list's cursor to its first item
        const CareerRow row{c, frame.at(0x344)};
        for (;;) {
            const Kept<30> old{c, program.call(list::next, 0x10307774, {old_rows})};
            if (!nonzero(c, std::int32_t(std::uint32_t(old)))) break;
            // The old row: year, club, the season record's first 0x10 bytes,
            // other_club and kind, at CareerRow's offsets less 0x10 from
            // other_club on. Bytes 2-3 and 0x2d-0x2f stay as they are.
            row.year() = c.u16(old);
            row.club() = c.i32(old + 4);
            program.call(routine::memcpy, 0x10307718, {row.record().address, old + 8, stats_file::old_record_size});
            row.other_club() = c.i32(old + 0x18);
            const auto kind = c.u8(old + 0x1c);
            row.kind() = kind;
            for (std::uint32_t at = 0x10; at < 0x1c; at += 2) c.put16(row.record().address + at, 0);
            row.record().figure() = 0;
            c.set({{0, 0}, {6, kind}, {7, kind}});
            program.call(list::append, 0x1030776c, {rows, row.address, CareerRow::size});
        }
        entry.history_offset() = -1;
        entry.history_size() = -1;
        program.call(history_file::write, 0x103077b4,
                     {converted, entry.history_offset().address(), entry.history_size().address(), rows});
        program.call(list::clear, 0x103077b8, {rows});
        program.call(list::destroy, 0x103077c8, {old_rows, 1});
    }
    program.call(history_file::destroy, 0x10307808, {stats.archive(), 1});
    program.call(history_file::destroy, 0x10307814, {converted, 1});
    // The new file replaces the old one.
    program.call(routine::remove, 0x10307824,
                 {program.folder_path(pool + text::temp_folder, pool + text::history_temp, 0x10307820)});
    const auto path = program.folder_path(pool + text::temp_folder, pool + text::history_temp, 0x10307830);
    program.call(routine::copy_file, 0x1030783c, {pool + text::new_history_temp, path});
    program.call(routine::remove, 0x10307844, {pool + text::new_history_temp});
    stats.archive() = new_history_file(c, 25, pool, history_file::mode, 0x10307848);
    program.call(list::destroy, 0x10307880, {rows, std::uint32_t(-1)});
}
}

std::uint32_t destroy_stats(Cpu& c, std::uint32_t stats_address, std::uint32_t flags) {
    Frame frame(c, 0x103023e0, 0x40, {.first_gpr = 30});
    Program program{c};
    const Kept<31> kept_flags{c, flags};
    const Kept<30> object{c, stats_address};
    if (!nonzero(c, std::int32_t(stats_address))) return result(c, stats_address);
    const Stats stats{c, stats_address};
    // `lwz r3,field; cmplwi r3,0`: r3 keeps the last pointer loaded.
    const auto held = [&](std::uint32_t pointer) { return nonzero_logical(c, result(c, pointer)); };
    if (held(stats.archive())) {
        program.call(history_file::destroy, 0x10302414, {stats.archive(), 1});
        stats.archive() = 0;
    }
    if (held(stats.seasons())) {
        program.free(stats.seasons(), 0x1030242c);
        stats.seasons() = 0;
        stats.season_capacity() = 0;
        stats.season_count() = 0;
    }
    if (held(stats.players())) {
        program.free(stats.players(), 0x1030244c);
        stats.players() = 0;
        stats.player_count() = 0;
    }
    // The relation count (+0x44) is left as it is.
    if (held(stats.relations())) {
        program.free(stats.relations(), 0x10302468);
        stats.relations() = 0;
        stats.relation_capacity() = 0;
    }
    if (compare(c, std::int16_t(flags)) > 0) program.call(routine::operator_delete, 0x10302484, {stats_address});
    return result(c, stats_address);
}

std::uint32_t load_stats(Cpu& c, std::uint32_t stats_address, std::uint32_t archive) {
    Frame frame(c, 0x10306c00, 0x3a0, {.first_gpr = 25});
    Program program{c};
    compare_logical(c, archive, 0);
    const Kept<27> version{c, c.toc(slot::save_version)};
    keep(c, {{29, c.toc(slot::success)}, {30, c.toc(slot::message)}});
    const Kept<28> object{c, stats_address};
    const Kept<31> pool{c, c.toc(slot::pool)};
    const Stats stats{c, stats_address};
    Entry entry(c, frame, pool, false);
    if (!archive) {
        report(c, pool, 0x10306c40, frame.at(0x324), frame.at(0xcc), 0xd1a, false);
        return result(c, 0);
    }
    if (!nonzero_logical(c, entry.open(archive, 0x10306c9c))) {
        report(c, pool, 0x10306cbc, frame.at(0x304), frame.at(0xc4), 0xd22, false);
        return result(c, 0);
    }
    if (!entry.one(stats.address + 8, 4, 0x10306d14)) return entry.fail({0x10306d30, 0x2e4, 0xbc, 0xd29});
    if (!entry.one(stats.address + 0xc, 4, 0x10306d98)) return entry.fail({0x10306db4, 0x2c4, 0xb4, 0xd31});
    stats.seasons() = program.malloc(std::uint32_t(stats.season_capacity().get()) << 5, 0x10306e14);
    if (!nonzero_logical(c, stats.seasons())) return entry.fail({0x10306e38, 0x2a4, 0xac, 0xd3a});
    if (older_save(c, version, 0x10306e94)) {
        // Older saves stored each season record's first 0x10 bytes.
        Kept<25> index{c, 0};
        Kept<26> offset{c, 0};
        for (; compare(c, index, stats.season_count()) < 0; index = index + 1, offset = offset + SeasonRecord::size) {
            if (!entry.one(stats.seasons() + offset, stats_file::old_record_size, 0x10306ecc))
                return entry.fail({0x10306eec, 0x284, 0xa4, 0xd46});
            const SeasonRecord record{c, stats.seasons() + offset};
            for (std::uint32_t at = 0x10; at < 0x1c; at += 2) c.put16(record.address + at, 0);
            record.figure() = 0;
            c.set({{7, stats.seasons()}, {8, stats.seasons()}, {9, 0}});
        }
    } else if (!entry.all(stats.seasons(), SeasonRecord::size, stats.season_count(), 0x10306fc0)) {
        return entry.fail({0x10306fe0, 0x264, 0x9c, 0xd58});
    }
    if (!entry.one(stats.address + 0x34, 4, 0x10307048)) return entry.fail({0x10307064, 0x244, 0x94, 0xd61});
    stats.players() = program.malloc(std::uint32_t(stats.player_count().get()) * PlayerEntry::size, 0x103070c4);
    if (!nonzero_logical(c, stats.players())) return entry.fail({0x103070e8, 0x224, 0x8c, 0xd6a});
    if (!entry.all(stats.players(), PlayerEntry::size, stats.player_count(), 0x1030714c))
        return entry.fail({0x1030716c, 0x204, 0x84, 0xd72});
    if (!entry.one(stats.rating_sums(false), 8, 0x103071d4)) return entry.fail({0x103071f0, 0x1e4, 0x7c, 0xd7a});
    if (!entry.one(stats.rating_sums(false) + 8, 8, 0x10307258)) return entry.fail({0x10307274, 0x1c4, 0x74, 0xd82});
    if (!entry.one(stats.rating_sums(true), 8, 0x103072dc)) return entry.fail({0x103072f8, 0x1a4, 0x6c, 0xd8a});
    if (!entry.one(stats.rating_sums(true) + 8, 8, 0x10307360)) return entry.fail({0x1030737c, 0x184, 0x64, 0xd92});
    if (!entry.one(stats.free_seasons().address(), 4, 0x103073e4)) return entry.fail({0x10307400, 0x164, 0x5c, 0xd9a});
    if (!entry.one(stats.relation_capacity().address(), 4, 0x10307468))
        return entry.fail({0x10307484, 0x144, 0x54, 0xda2, false});
    if (!entry.one(stats.relation_count().address(), 4, 0x103074e4))
        return entry.fail({0x10307500, 0x124, 0x4c, 0xda8, false});
    stats.relations() = program.malloc(std::uint32_t(stats.relation_capacity().get()) << 3, 0x10307558);
    // The original tests the capacity, not the allocation, and reads on
    // after the fatal exit (which does not return in play).
    if (compare(c, stats.relation_capacity()) == 0) entry.reported({0x1030757c, 0x104, 0x44, 0xdaf});
    if (!entry.all(stats.relations(), Relation::size, stats.relation_capacity(), 0x103075d4))
        return entry.fail({0x103075f4, 0xe4, 0x3c, 0xdb3, false});
    entry.close(0x10307648);
    if (older_save(c, version, 0x10307650)) convert_history(c, frame, stats, pool);
    return result(c, 1);
}

std::uint32_t save_stats(Cpu& c, std::uint32_t stats_address, std::uint32_t archive) {
    Frame frame(c, 0x103078a0, 0x2b0, {.first_gpr = 28});
    compare_logical(c, archive, 0);
    const Kept<31> pool{c, c.toc(slot::pool)};
    keep(c, {{30, c.toc(slot::message)}, {29, c.toc(slot::success)}});
    const Kept<28> object{c, stats_address};
    const Stats stats{c, stats_address};
    Entry entry(c, frame, pool, true);
    if (!archive) {
        report(c, pool, 0x103078e8, frame.at(0x274), frame.at(0xac), 0xe09, false);
        return result(c, 0);
    }
    if (!nonzero_logical(c, entry.open(archive, 0x10307944))) {
        entry.reported({0x10307964, 0x254, 0xa4, 0xe11});
        return result(c, 0);
    }
    if (!entry.one(stats.address + 8, 4, 0x103079c4)) return entry.fail({0x103079e0, 0x234, 0x9c, 0xe18});
    if (!entry.one(stats.address + 0xc, 4, 0x10307a48)) return entry.fail({0x10307a64, 0x214, 0x94, 0xe20});
    if (!entry.all(stats.seasons(), SeasonRecord::size, stats.season_count(), 0x10307acc))
        return entry.fail({0x10307aec, 0x1f4, 0x8c, 0xe28});
    if (!entry.one(stats.address + 0x34, 4, 0x10307b54)) return entry.fail({0x10307b70, 0x1d4, 0x84, 0xe30});
    if (!entry.all(stats.players(), PlayerEntry::size, stats.player_count(), 0x10307bd8))
        return entry.fail({0x10307bf8, 0x1b4, 0x7c, 0xe38});
    if (!entry.one(stats.rating_sums(false), 8, 0x10307c60)) return entry.fail({0x10307c7c, 0x194, 0x74, 0xe40});
    if (!entry.one(stats.rating_sums(false) + 8, 8, 0x10307ce4)) return entry.fail({0x10307d00, 0x174, 0x6c, 0xe48});
    if (!entry.one(stats.rating_sums(true), 8, 0x10307d68)) return entry.fail({0x10307d84, 0x154, 0x64, 0xe50});
    if (!entry.one(stats.rating_sums(true) + 8, 8, 0x10307dec)) return entry.fail({0x10307e08, 0x134, 0x5c, 0xe58});
    if (!entry.one(stats.free_seasons().address(), 4, 0x10307e70)) return entry.fail({0x10307e8c, 0x114, 0x54, 0xe60});
    if (!entry.one(stats.relation_capacity().address(), 4, 0x10307ef4))
        return entry.fail({0x10307f10, 0xf4, 0x4c, 0xe68, false});
    if (!entry.one(stats.relation_count().address(), 4, 0x10307f70))
        return entry.fail({0x10307f8c, 0xd4, 0x44, 0xe6e, false});
    if (!entry.all(stats.relations(), Relation::size, stats.relation_capacity(), 0x10307fec))
        return entry.fail({0x1030800c, 0xb4, 0x3c, 0xe74, false});
    entry.close(0x10308060);
    return result(c, 1);
}
}
