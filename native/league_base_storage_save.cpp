// LEAGUE.CPP, block `storage`: the league's save writer (vt +0x24, 0x104a8530). It writes "%s_%d.tmp" in
// CM3_TEMP with about 62 fwrites. Club pointers inside the standings, the opponent blocks, the history rows and the
// stage rows are turned into the clubs' first words (their indices) in place, written, and turned back; the
// original does this in registers the allocator-free fwrite calls and the replay's imports see, so the
// non-volatile r25-r31 and the volatile r7-r9 the fwrite mask reads are set as the original holds them at each call
// (`nv`, and the explicit c.set before the calls at 0x104a8b28, 0x104a8cf4, 0x104a8ec8 and 0x104a8f20).
// The 12 bytes MAIN.CPP 0x107795d8 reads from this frame (stack contract, "same") are the three locals at sp+0x38,
// sp+0x3c and sp+0x40.
#include "league_base_storage_detail.hpp"
#include <array>

namespace cm::aot::league_base {
using namespace storage;

namespace {
using u32 = std::uint32_t;
using s32 = std::int32_t;
u32 half(Cpu& c, u32 address) { return u32(s32(c.i16(address))); }
bool below(u32 a, u32 b) { return s32(a) < s32(b); }
// The record a club's pointer becomes: its first word, or -1 for none (and back: base + index * 0x248, 0 for -1).
u32 index_of(Cpu& c, u32 club) { return club != 0 ? c.u32(club) : 0xffffffffu; }
u32 club_at(u32 clubs, u32 index) { return index != 0xffffffffu ? clubs + index * club_size : 0; }
}

void save(Cpu& c, u32 league_object, u32 temporary) {
    const Frame frame(c, 0x104a8530, 0xc0, {.first_gpr = 25});
    const u32 pool = c.toc(toc::pool), clubs_cell = c.toc(toc::clubs), success = c.toc(toc::success), message = c.toc(toc::message);
    // r25..r31 as the original holds them (r27 is the file once it is open; r29, r30, r31 and r25 are reused as counters).
    std::array<u32, 7> nv{temporary, league_object, c.r(27), clubs_cell, success, message, pool};
    const auto sync = [&] { for (unsigned i = 0; i < nv.size(); ++i) c.set(25 + i, nv[i]); };
    const auto call = [&](u32 target, u32 back, std::initializer_list<u32> arguments) {
        sync();
        return invoke(c, target, back, arguments);
    };
    sync();
    const u32 stats = c.u32(league_object + field::stats);
    c.put32(frame.at(0x3c), 0);
    c.put32(frame.at(0x38), 0);
    c.put32(frame.at(0x40), stats != 0 ? 1 : 0);  // (-x | x) >> 31
    const auto name = [&](u32 back, u32 value) {
        const u32 record = c.u32(league_object + 4);
        call(external::format, back, {league_object + field::name, pool + text::temporary_name, record + 0x1a, value});
    };
    {
        const u32 record = c.u32(league_object + 4);
        name(0x104a8590, c.u32(record));
    }
    if (std::int8_t(temporary) == 0) {
        if (c.i8(league_object + field::active) == 0) return;
        name(0x104a85bc, half(c, league_object + field::year));
    }
    // A '/' in the name would be a folder: it becomes '_' (the loop has no bound but the terminator).
    for (u32 n = 0; c.i8(league_object + field::name + n) != 0; ++n)
        if (c.i8(league_object + field::name + n) == '/') c.put8(league_object + field::name + n, 0x5f);
    const u32 path = call(external::data_path, 0x104a85f8, {pool + text::temporary_folder, league_object + field::name});
    const u32 file = call(external::open_file, 0x104a8600, {path, pool + text::write_mode});
    nv[2] = file;
    c.set(27, file);
    if (file == 0) {
        sync();
        report(c, frame, {0x104a861c, 0x74, 0x4c, 0x1391});
        c.put8(success, 0);
        return;
    }
    const auto put = [&](u32 back, u32 address, u32 size, u32 count) {
        sync();
        c.set({{3, address}, {4, size}, {5, count}, {6, file}});
        c.call(external::write_file, back);
    };
    const auto clubs = [&] { return half(c, league_object + field::clubs); };
    const auto rows = [&] { return c.u32(league_object + league::standings); };
    put(0x104a8670, league_object + field::name, 0x50, 1);
    put(0x104a8684, frame.at(0x40), 4, 1);
    if (c.u32(frame.at(0x40)) != 0) call(external::write_stats, 0x104a869c, {c.u32(league_object + field::stats), file});
    sync();
    virtual_call(c, write_extra_slot, 0x104a86b0, {league_object, file});
    struct Field { u32 back, offset, size; };
    for (const Field& f : {Field{0x104a86c8, 0xe2, 2}, {0x104a86dc, 0xe4, 2}, {0x104a86f0, 0xe1, 1}, {0x104a8704, 0x28, 4},
                           {0x104a8718, 0xe0, 1}, {0x104a872c, 0xd4, 8}, {0x104a8740, 0xa4, 2}, {0x104a8754, 0x4c, 1},
                           {0x104a8768, 0x4b, 1}, {0x104a877c, 0x48, 1}, {0x104a8790, 0x49, 1}, {0x104a87a4, 0x24, 4},
                           {0x104a87b8, 0x46, 1}, {0x104a87cc, 0x47, 1}, {0x104a87e0, 0xb6, 2}, {0x104a87f4, 0xb8, 2},
                           {0x104a8808, 0x45, 1}, {0x104a881c, 0xb4, 1}, {0x104a8830, 0x3c, 2}, {0x104a8844, 0x3e, 2},
                           {0x104a8858, 0x40, 2}, {0x104a886c, 0x43, 1}, {0x104a8880, 0x42, 1}, {0x104a8894, 0x3a, 2}})
        put(f.back, league_object + f.offset, f.size, 1);
    // The competition record: its first word (the id). Without one the assertion, and the word of a constant.
    if (c.u32(league_object + 4) != 0) {
        put(0x104a88b0, c.u32(league_object + 4), 4, 1);
    } else {
        sync();
        report(c, frame, {0x104a88c8, 0x54, 0x44, 0x13c8});
        c.put8(success, 0);
        put(0x104a8918, c.toc(toc::constant_4720), 4, 1);
    }
    for (const Field& f : {Field{0x104a892c, 0xa6, 2}, {0x104a8940, 0xc4, 1}, {0x104a8954, 0xc5, 1}, {0x104a8968, 0xc6, 1},
                           {0x104a897c, 0xc7, 4}, {0x104a8990, 0xc0, 4}, {0x104a89a4, 0xdc, 2}, {0x104a89b8, 0xde, 2},
                           {0x104a89cc, 0xa8, 2}, {0x104a89e0, 0x1c, 4}, {0x104a89f4, 0x20, 4}, {0x104a8a08, 0xe6, 1}})
        put(f.back, league_object + f.offset, f.size, 1);
    put(0x104a8a1c, c.u32(league_object + field::schedule), 0x4c, half(c, league_object + field::schedule_count));
    put(0x104a8a30, league_object + field::word_18, 4, 1);
    put(0x104a8a44, league_object + field::row_count_34, 2, 1);
    if (c.u32(league_object + field::word_18) != 0)
        put(0x104a8a60, c.u32(league_object + field::word_18), 0x18, half(c, league_object + field::row_count_34));
    if (below(0, clubs())) {
        // The standings' clubs, and each row's opponent block's, become their first words (-1 for none).
        bool opponents_seen = false, r7_set = false;
        u32 r7 = 0;
        for (u32 i = 0; below(i, clubs()); ++i) {
            const u32 offset = i * row::size;
            c.put32(rows() + offset, index_of(c, c.u32(rows() + offset)));
            if (c.u32(rows() + row::opponents) != 0) {
                c.put32(frame.at(0x3c), 1);
                opponents_seen = true;
                for (u32 j = 0; below(j, clubs() - 1); ++j) {
                    const u32 at = c.u32(rows() + row::opponents + offset) + j * opponent_size;
                    r7 = c.u32(at);
                    r7_set = true;
                    c.put32(at, index_of(c, r7));
                }
            }
        }
        if (r7_set) c.set(7, r7);
        c.set(8, clubs());
        if (opponents_seen) c.set(9, clubs() - 1);
        put(0x104a8b28, rows(), row::size, clubs());
        put(0x104a8b3c, frame.at(0x3c), 4, 1);
        if (c.u32(frame.at(0x3c)) != 0) {
            u32 i = 0;
            nv[4] = 0;
            nv[5] = 0;
            for (; below(i, clubs()); ++i) {
                nv[4] = i * row::size;
                nv[5] = i;
                put(0x104a8b70, c.u32(rows() + nv[4] + row::opponents), opponent_size, clubs() - 1);
            }
            nv[4] = i * row::size;
            nv[5] = i;
        }
        if (c.u32(rows() + row::history) != 0) {
            c.put32(frame.at(0x38), 1);
            put(0x104a8bb0, frame.at(0x38), 4, 1);
            // The history size is still -1 on a league nobody placed a club in: settled here, as place_club does.
            if (c.i16(league_object + field::history_bytes) == -1 && below(0, clubs()))
                c.put16(league_object + field::history_bytes, (clubs() - 1) * half(c, league_object + field::rounds));
            u32 i = 0;
            nv[4] = 0;
            nv[5] = 0;
            for (; below(i, clubs()); ++i) {
                nv[4] = i * row::size;
                nv[5] = i;
                put(0x104a8c00, c.u32(rows() + nv[4] + row::history), half(c, league_object + field::history_bytes), 1);
            }
            nv[4] = i * row::size;
            nv[5] = i;
        } else {
            put(0x104a8c2c, frame.at(0x38), 4, 1);
        }
        // Back to pointers.
        bool inner_seen = false;
        u32 i = 0;
        for (; below(i, clubs()); ++i) {
            const u32 offset = i * row::size;
            c.put32(rows() + offset, club_at(c.u32(clubs_cell), c.u32(rows() + offset)));
            if (c.u32(rows() + row::opponents) != 0) {
                inner_seen = true;
                for (u32 j = 0; below(j, clubs() - 1); ++j) {
                    const u32 at = c.u32(rows() + row::opponents + offset) + j * opponent_size;
                    c.put32(at, club_at(c.u32(clubs_cell), c.u32(at)));
                }
            }
        }
        c.set({{7, i * row::size}, {8, i}});
        if (inner_seen) c.set(9, clubs() - 1);
    }
    put(0x104a8cf4, league_object + field::word_2c, 4, 1);
    put(0x104a8d08, league_object + field::last_stage, 4, 1);
    put(0x104a8d1c, league_object + field::owner, 1, 1);
    // The stages (-1 is the league itself, not written): the kind, and by kind their own records.
    const auto stage_at = [&](u32 k) { return c.u32(c.u32(league_object + field::stages) + k * 4); };
    u32 k = 0;
    nv[4] = 0;
    nv[6] = 0;
    for (; s32(k) <= c.i32(league_object + field::last_stage); ++k) {
        nv[4] = k;
        nv[6] = k * 4;
        put(0x104a8d44, stage_at(k) + stage::kind, 1, 1);
        const u32 stage = stage_at(k);
        const auto kind = c.i8(stage + stage::kind);
        if (kind == 1) {
            put(0x104a8d70, stage + stage::clubs, 2, 1);
            put(0x104a8d8c, stage_at(k) + stage::rounds, 2, 1);
            u32 m = 0, offset = 0;
            for (;; ++m, offset += stage::round_size) {
                nv[0] = m;
                nv[5] = offset;
                if (!below(m, half(c, stage_at(k) + stage::rounds))) break;
                put(0x104a8db4, c.u32(stage_at(k) + stage::round_table) + offset + stage::round_half, 2, 1);
            }
            call(external::write_league_stage, 0x104a8ddc, {stage_at(k), file});
        } else if (kind == 2) {
            put(0x104a8dfc, stage + stage::clubs, 2, 1);
            put(0x104a8e18, stage_at(k) + stage::schedule_count, 2, 1);
            call(external::write_table_stage, 0x104a8e28, {stage_at(k), file});
        }
    }
    nv[4] = k;
    nv[6] = k * 4;
    put(0x104a8e50, league_object + field::saved_38, 2, 1);
    put(0x104a8e64, league_object + field::used_36, 2, 1);
    // The 8-byte rows of +0x14: their club pointers become indices for the write and back.
    u32 i = 0;
    for (; below(i, half(c, league_object + field::used_36)); ++i) {
        const u32 at = c.u32(league_object + field::round_rows) + i * 8;
        c.put32(at, index_of(c, c.u32(at)));
    }
    c.set(7, i);
    if (c.u32(league_object + field::round_rows) != 0)
        put(0x104a8ec8, c.u32(league_object + field::round_rows), 8, half(c, league_object + field::saved_38));
    i = 0;
    for (; below(i, half(c, league_object + field::used_36)); ++i) {
        const u32 at = c.u32(league_object + field::round_rows) + i * 8;
        c.put32(at, club_at(c.u32(clubs_cell), c.u32(at)));
    }
    c.set(7, i);
    call(external::close_file, 0x104a8f20, {file});
}
}
