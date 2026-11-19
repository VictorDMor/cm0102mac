// CUP.CPP 0x10484420 (vt +0x24 of the cup): the cup's save writer. It writes "<competition name>_<id>.tmp" in
// CM3_TEMP (a `temporary` byte of zero names the file by the year and writes nothing for an inactive cup) with 41
// fwrites. The club pointers of the entrant rows and of the 8-byte rows at +0x14 become the clubs' first words
// (their indices, -1: none) in place, are written, and turned back to pointers; the original does this with
// r25-r31 as loop counters, so the non-volatile registers the replay's imports see, and the volatile r7 the
// stage loop leaves for the fwrite mask, are set as the original holds them at each call (`nv`, `sync`). The
// original's frame is 0x1a0 bytes: the "has statistics" flag at sp+0x38 (the one local it stores), the assertions'
// buffers above it.
#include "cup_base_storage_detail.hpp"

namespace cm::aot::cup_base {
using namespace storage;

namespace {
using u32 = std::uint32_t;
using s32 = std::int32_t;
bool below(u32 a, u32 b) { return s32(a) < s32(b); }
inline constexpr u32 none = 0xffffffffu, club_row_size = 0x248, extra_row = 0x18;
// The assertion sites: the address of the report's first call and the frame offsets of its buffers.
inline constexpr CheckSite no_rounds{0x104844d8, 0x94, 0x4c, 0x1470}, no_open{0x10484558, 0x74, 0x44, 0x147a},
    no_record{0x104847e4, 0x54, 0x3c, 0x14b5};

class Writer {
public:
    Writer(Cpu& cpu, const Frame& frame, u32 cup, u32 temporary)
        : c(cpu), frame(frame), cup(cup), clubs_cell(c.toc(toc::clubs)), pool(c.toc(toc::pool)),
          success(c.toc(toc::success)), message(c.toc(toc::message)), nv{temporary, cup, c.r(27), clubs_cell, success, message, pool} {}
    void run();

private:
    // r25..r31 as the original holds them (r27 the file once open; r29, r30, r31 and r25 are reused as counters).
    void sync() { for (unsigned i = 0; i < nv.size(); ++i) c.set(25 + i, nv[i]); }
    u32 call(u32 routine, u32 back, std::initializer_list<u32> arguments) {
        sync();
        return invoke(c, routine, back, arguments);
    }
    // `bl fwrite` returning to `back`: (address, size, count, file).
    void put(u32 back, u32 address, u32 size, u32 count) {
        sync();
        c.set({{3, address}, {4, size}, {5, count}, {6, file}});
        c.call(routine::write_file, back);
    }
    void name(u32 back, u32 value) {
        const u32 record = c.u32(cup + competition::record);
        call(routine::format, back, {cup + 0x4d, pool + text::temporary_name, record + 0x1a, value});
    }
    void fail(const CheckSite& site) {
        sync();
        report(c, frame, site);
        c.put8(success, 0);
    }
    u32 at(u32 offset) const { return cup + offset; }
    s32 half(u32 offset) const { return c.i16(cup + offset); }
    // The club a pointer names becomes its first word (-1 for none) and back: base + index * 0x248 (0 for -1).
    u32 index_of(u32 club) const { return club != 0 ? c.u32(club) : none; }
    u32 club_at(u32 index) const { return index != none ? c.u32(clubs_cell) + index * club_row_size : 0; }
    u32 stage(u32 k) const { return c.u32(c.u32(at(competition::stages)) + k * 4); }

    bool open();
    void header();
    void tables();
    void stages();
    void round_rows();

    Cpu& c;
    const Frame& frame;
    u32 cup, clubs_cell, pool, success, message, file = 0;
    std::array<u32, 7> nv;
};

// The name (twice when the year names the file), the checks that the cup is built, the file.
bool Writer::open() {
    const u32 stats = c.u32(at(competition::stats));
    c.put32(frame.at(0x38), stats != 0 ? 1 : 0);  // (-x | x) >> 31
    sync();
    name(0x10484474, c.u32(c.u32(at(competition::record))));
    if (sext8(nv[0]) == 0) {
        if (c.i8(at(competition::byte_45)) == 0) return false;
        name(0x104844a0, sext16(c.u16(at(competition::year))));
    }
    if (c.u32(at(cup::rounds)) == 0 || c.u32(at(cup::entrants)) == 0 || c.u32(at(cup::round_matches)) == 0) {
        fail(no_rounds);
        return false;
    }
    const u32 path = call(routine::file_path, 0x10484524, {pool + text::temporary_folder, at(0x4d)});
    file = call(routine::open_file, 0x1048452c, {path, pool + text::write_mode});
    nv[2] = file;
    c.set(27, file);
    if (file == 0) {
        call(routine::format, 0x10484544, {frame.at(0xb4), pool + text::save_open, at(0x4d)});
        fail(no_open);
        return false;
    }
    return true;
}

// The name, the statistics, the derived cup's records (virtual +0x2c) and the fixed fields.
void Writer::header() {
    put(0x104845ac, at(0x4d), 0x50, 1);
    put(0x104845c0, frame.at(0x38), 4, 1);
    if (c.u32(frame.at(0x38)) != 0) call(routine::stats_write, 0x104845d8, {c.u32(at(competition::stats)), file});
    sync();
    virtual_call(c, write_records_slot, 0x104845ec, {cup, file});
    for (const Field& f : fields) put(f.save_back, at(f.offset), f.size, 1);
    put(0x10484780, at(competition::block_18), 4, 1);
    put(0x10484794, at(competition::half_34), 2, 1);
    if (c.u32(at(competition::block_18)) != 0)
        put(0x104847b0, c.u32(at(competition::block_18)), extra_row, sext16(c.u16(at(competition::half_34))));
    // The competition record: its first word (the id). Without one the assertion, and the word -1 of the pool.
    if (c.u32(at(competition::record)) != 0) {
        put(0x104847cc, c.u32(at(competition::record)), 4, 1);
    } else {
        fail(no_record);
        put(0x10484834, c.toc(toc::minus_one), 4, 1);
    }
}

// The rounds, the entrants (their clubs as indices for the write) and each round's match rows.
void Writer::tables() {
    put(0x10484848, c.u32(at(cup::rounds)), cup::round_row, sext16(c.u16(at(competition::rounds))));
    // The entrants' clubs become indices (r29 counts the rows).
    nv[4] = 0;
    for (; below(nv[4], sext16(c.u16(at(competition::club_count)))); ++nv[4]) {
        const u32 row = c.u32(at(cup::entrants)) + nv[4] * cup::entrant_row;
        c.put32(row, index_of(c.u32(row)));
    }
    put(0x104848a0, c.u32(at(cup::entrants)), cup::entrant_row, sext16(c.u16(at(competition::club_count))));
    // Back to pointers.
    nv[4] = 0;
    for (; below(nv[4], sext16(c.u16(at(competition::club_count)))); ++nv[4]) {
        const u32 row = c.u32(at(cup::entrants)) + nv[4] * cup::entrant_row;
        c.put32(row, club_at(c.u32(row)));
    }
    // The match rows: r29 the round, r30 its byte offset in the rounds, r31 the offset in the row-block array.
    nv[4] = nv[5] = nv[6] = 0;
    for (; below(nv[4], sext16(c.u16(at(competition::rounds)))); ++nv[4], nv[5] += cup::round_row, nv[6] += 4)
        put(0x10484920, c.u32(c.u32(at(cup::round_matches)) + nv[6]),
            sext16(c.u16(c.u32(at(cup::rounds)) + nv[5] + Round::ties)) * cup::match_row, 1);
    put(0x1048494c, at(competition::word_2c), 4, 1);
    put(0x10484960, at(competition::last_stage), 4, 1);
    put(0x10484974, at(competition::owner), 1, 1);
}

// The stages (-1 is the cup itself, not written): the kind byte, and by kind their own records.
void Writer::stages() {
    nv[4] = nv[5] = 0;
    for (; s32(nv[4]) <= c.i32(at(competition::last_stage)); ++nv[4], nv[5] += 4) {
        const u32 k = nv[4];
        put(0x1048499c, stage(k) + competition::kind, 1, 1);
        const u32 object = stage(k);
        const auto kind = sext8(c.u8(object + competition::kind));
        if (kind == 1) {
            put(0x104849c8, object + competition::club_count, 2, 1);
            put(0x104849e4, stage(k) + competition::rounds, 2, 1);
            nv[0] = nv[6] = 0;
            for (; below(nv[0], sext16(c.u16(stage(k) + competition::rounds))); ++nv[0], nv[6] += cup::round_row) {
                c.set(7, stage(k));
                put(0x10484a14, c.u32(stage(k) + cup::rounds) + nv[6] + Round::ties, 2, 1);
            }
            call(routine::list_write, 0x10484a38, {stage(k), file});
        } else if (kind == 2) {
            put(0x10484a58, object + competition::club_count, 2, 1);
            put(0x10484a74, stage(k) + 0xa6, 2, 1);
            call(routine::group_write, 0x10484a84, {stage(k), file});
        }
    }
}

// The 8-byte rows of +0x14: their club pointers become indices for the write and back.
void Writer::round_rows() {
    put(0x10484aac, at(competition::half_38), 2, 1);
    put(0x10484ac0, at(competition::half_36), 2, 1);
    // r3 the byte offset, r4 -1 (then 0 for the way back), r5 the row's word (the club table on the way back) and r6
    // the block: the loops leave them for the close, and for the rows' write when there is no block.
    c.set({{3, 0}, {4, none}});
    nv[4] = 0;
    for (; below(nv[4], sext16(c.u16(at(competition::half_36)))); ++nv[4]) {
        c.set(6, c.u32(at(competition::block_14)));
        c.set(5, c.u32(c.r(6) + c.r(3)));
        c.put32(c.r(6) + c.r(3), c.r(5) != 0 ? c.u32(c.r(5)) : none);
        c.set(3, c.r(3) + 8);
    }
    c.set(3, c.u32(at(competition::block_14)));
    if (c.r(3) != 0) put(0x10484b20, c.r(3), 8, sext16(c.u16(at(competition::half_38))));
    c.set({{3, 0}, {4, 0}});
    nv[4] = 0;
    for (; below(nv[4], sext16(c.u16(at(competition::half_36)))); ++nv[4]) {
        c.set(6, c.u32(at(competition::block_14)));
        const u32 word = c.u32(c.r(6) + c.r(3));
        if (word == none) {
            c.put32(c.r(6) + c.r(3), c.r(4));
        } else {
            c.set(5, c.u32(clubs_cell));
            c.put32(c.r(6) + c.r(3), c.r(5) + word * club_row_size);
        }
        c.set(3, c.r(3) + 8);
    }
    call(routine::close_file, 0x10484b78, {file});
}

void Writer::run() {
    if (!open()) return;
    header();
    tables();
    stages();
    round_rows();
}
}

void save_to_disk(Cpu& c, std::uint32_t cup, std::uint32_t temporary) {
    const Frame frame(c, 0x10484420, 0x1a0, {.first_gpr = 25});
    Writer(c, frame, cup, temporary).run();
}
}
