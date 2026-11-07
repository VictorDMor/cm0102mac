// DATABASE.CPP's load block, part 4: the pass that reads every table from the data files (database_load.hpp).
//
// Each kind has one handler: when the kind's table exists the file named by the entry is opened (packed open or
// plain stdio), positioned at the entry's offset, handed to the table's reader (a routine of the file modules, with
// the file, the packed flag, the table and the entry), and closed. A reader's failure closes the file and ends the
// load; so does a failed open, except for kinds 17 and 18, whose tables are optional (the table is released and
// its count cleared). Kind 16 reads a second time, for the placeholder rows, when the header's first word is
// positive; kind 6 reads either the extended records or the people. Every body is the original's: two variants
// (packed or plain) at addresses in a fixed layout from the handler's address.
#include "database_load.hpp"
#include "database_load_detail.hpp"

namespace cm::aot::database {
using guest::compare;
using guest::compare_logical;
using namespace load;

namespace {
// Offsets of the calls from the handler's address: the handlers that load the table's cell through the TOC
// ("standard") are four bytes longer before the open and eight before the reader than those that test a register.
constexpr Layout standard{0x2c, 0x3c, 0x8c, 0xa0, 0xb8, 0xdc, 0xe8, 0x104, 0x110};
constexpr Layout direct{0x28, 0x38, 0x88, 0x9c, 0xb0, 0xd4, 0xe0, 0xfc, 0x108};
// Kinds 17 and 18 (optional tables) have a longer body: the open's failure and the table's release come first.
constexpr Layout optional_17{0x2c, 0x3c, 0xe8, 0xfc, 0x114, 0x138, 0x144, 0x160, 0x16c};
constexpr Layout optional_18{0x2c, 0x3c, 0xd0, 0xe4, 0xfc, 0x120, 0x12c, 0x148, 0x154};

struct Handler {
    std::uint8_t kind;
    std::uint32_t base;
    std::int32_t cell;
    std::uint32_t reader, slot;
    const Layout* layout;
};
// The handlers by kind; kinds 6, 16, 17 and 18 are written out.
constexpr std::array<Handler, 18> handlers{{
    {0, 0x100a96c4, cells::clubs_table, 0x105d8f60, 0x118, &standard},
    {1, 0x100a97dc, cells::second_table, 0x105d8f60, 0x10c, &standard},
    {2, 0x100aa054, cells::rows_3c_table, 0x105dfa80, 0xb4, &standard},
    {3, 0x100a9034, cells::rows_c8_table, 0x105d7250, 0x160, &standard},
    {4, 0x100aa16c, cells::rows_128_table, 0x105d7750, 0xa8, &standard},
    {5, 0x100aa284, cells::rows_50_table, 0x105d8b40, 0x9c, &standard},
    {7, 0x100a914c, cells::rows_2c_table, 0x105df630, 0x154, &standard},
    {9, 0x100aa6cc, cells::rows_44_table, 0x105db230, 0x6c, &direct},
    {10, 0x100aa7dc, cells::rows_48_table, 0x105dc1d0, 0x60, &standard},
    {11, 0x100a98f4, cells::rows_68_table, 0x105dede0, 0x100, &standard},
    {12, 0x100a9a0c, cells::placeholder_table, 0x105de150, 0xf4, &standard},
    {13, 0x100a9264, cells::subs_a_table, 0x105dfe60, 0x148, &standard},
    {14, 0x100a937c, cells::subs_b_table, 0x105dfe60, 0x13c, &standard},
    {15, 0x100a9494, cells::subs_c_table, 0x105dfe60, 0x130, &standard},
    {19, 0x100a9e24, cells::rows_1c_table, 0x105dea20, 0xcc, &standard},
    {20, 0x100a9f3c, cells::rows_1c_b_table, 0x105dea20, 0xc0, &standard},
    {21, 0x100a95ac, cells::rows_38_table, 0x105d85d0, 0x124, &standard},
    {22, 0x100aa5bc, cells::rows_34_table, 0x105dd980, 0x78, &direct},
}};
}

// Opens the entry's file, seeks to its offset and runs the reader on `p.value` (the table); a failed read closes the
// file in the slot below p.slot. A failed open is the caller's.
Loader::Outcome Loader::open_and_read(const Pass& p) {
    c.set(0, p.value);
    if (compare_logical(c, p.value) == 0) return Outcome::skipped;
    const auto& l = *p.layout;
    const bool pk = packed();
    c.set({{3, c.r(15)}, {4, c.r(14)}});
    if (pk) c.set({{5, 0}, {6, 0x15}, {7, 4}});
    call(p.base + (pk ? l.open_packed : l.open_plain), pk ? callee::packed_open : callee::plain_open);
    c.set(19, c.r(3));
    compare(c, std::int32_t(c.r(3)));
    if (c.r(19) == 0) {
        compare_logical(c, c.r(19));
        return Outcome::open_failed;
    }
    const bool seeking_packed = packed();
    c.set({{4, c.u32(c.r(14) + entry::offset)}, {3, c.r(19)}, {5, 0}});
    call(p.base + (seeking_packed ? l.seek_packed : l.seek_plain), seeking_packed ? callee::seek_packed : callee::seek_plain);
    c.set({{5, p.value}, {3, c.r(19)}, {4, c.r(16)}, {6, c.r(14)}});
    call(p.base + l.reader, p.reader);
    const auto answer = ext(c.r(3));
    c.set(0, answer);
    if (compare(c, std::int32_t(answer)) != 0) return Outcome::read_ok;
    compare_logical(c, c.r(19));
    close_file(p.slot - 4, {p.base + l.fail_packed, p.base + l.fail_plain});
    return Outcome::read_failed;
}

// The success close of a handler.
void Loader::close_read(const Pass& p) {
    close_file(p.slot - 8, {p.base + p.layout->ok_packed, p.base + p.layout->ok_plain});
}

// Kinds that end the load when their table cannot be opened or read.
bool Loader::load_required(const Pass& p) {
    switch (open_and_read(p)) {
    case Outcome::skipped: return true;
    case Outcome::read_ok: close_read(p); return true;
    default: return false;
    }
}

// Kinds 17 and 18: with the file missing, the table is released and its count cleared and the load goes on.
bool Loader::load_optional(const Pass& p, std::int32_t table_slot, std::int32_t count_slot, std::uint32_t free_bl,
                           bool clears_when_null) {
    const auto outcome = open_and_read(p);
    if (outcome == Outcome::skipped) return true;
    if (outcome == Outcome::read_failed) return false;
    if (outcome == Outcome::read_ok) {
        close_read(p);
        return true;
    }
    // The open failed: the original closes the (null) handle, then drops the table.
    close_file(p.slot, {p.base + 0x58, p.base + 0x64});
    const auto table_word = c.toc(table_slot);
    const auto table = c.u32(table_word);
    c.set(0, table);
    if (compare_logical(c, table) == 0) {
        // Kind 17 reaches its clearing from the null test too; kind 18 goes on to the next entry.
        if (clears_when_null) {
            c.set({{3, c.toc(table_slot)}, {4, 0}, {0, 0}});
            c.put32(c.toc(table_slot), 0);
            c.set(3, c.toc(count_slot));
            c.put32(c.r(3), 0);
        } else {
            c.set(3, c.toc(table_slot));  // the null test's load
        }
        return true;
    }
    const auto flag = ext(c.u8(c.r(2) + std::uint32_t(toc::single_block_flag)));
    c.set(0, flag);
    const bool flagged = compare(c, std::int32_t(flag)) != 0;
    c.set(3, c.u32(c.toc(table_slot)));
    call(free_bl + (flagged ? 0 : 0x10), callee::release);
    c.set({{3, c.toc(table_slot)}, {4, 0}, {0, 0}});
    c.put32(c.toc(table_slot), 0);
    c.set(3, c.toc(count_slot));
    c.put32(c.r(3), 0);
    if (clears_when_null) {
        // Kind 17 clears twice (the second is the shared tail).
        c.set({{3, c.toc(table_slot)}, {4, 0}, {0, 0}});
        c.put32(c.r(3), 0);
        c.set(3, c.toc(count_slot));
        c.put32(c.r(3), 0);
    }
    return true;
}

// Kind 6: the extended records (their word at TOC -0x39e0) when there are some, else the people.
bool Loader::load_kind_6() {
    const Pass detail{0x100aa39c, c.u32(c.r(2) + std::uint32_t(toc::detail_table)), 0x105dac80, 0x90, &direct};
    c.set(0, detail.value);
    if (compare_logical(c, detail.value) != 0) return load_required(detail);
    const Pass people{0x100aa4ac, c.u32(c.toc(cells::people_table)), 0x105da6e0, 0x84, &direct};
    return load_required(people);
}

// Kind 16: the table, then (with a positive header word) the placeholder rows' second read through a name copied
// from the entry.
bool Loader::load_kind_16() {
    const Pass p{0x100a9b24, c.u32(c.toc(cells::rows_6c_table)), 0x105de150, 0xe8, &standard};
    const auto outcome = open_and_read(p);
    if (outcome == Outcome::skipped) return true;
    if (outcome != Outcome::read_ok) return false;
    const auto header = c.toc(toc::header_a);
    c.set(0, header);
    if (compare(c, std::int32_t(header)) > 0) {
        c.put32(frame.at(0x698), c.toc(toc::header_a));
        c.put32(frame.at(0x6a0), c.u32(c.r(14) + entry::flag));
        c.set({{4, c.r(14)}, {3, frame.at(0x660)}});
        call(0x100a9c3c, callee::copy_text);
        const auto placeholders = c.u32(c.r(22));
        const auto where = c.u32(c.toc(cells::placeholder_table)) + (placeholders - 0x7f) * 0x6c;
        c.set({{0, (placeholders - 0x7f) * 0x6c}, {4, c.r(16)}, {5, where}, {3, c.r(19)}, {6, frame.at(0x660)}});
        call(0x100a9c68, 0x105de150);
        const auto answer = ext(c.r(3));
        c.set(0, answer);
        if (compare(c, std::int32_t(answer)) == 0) {
            compare_logical(c, c.r(19));
            close_file(0xe0, {0x100a9c8c, 0x100a9c98});
            return false;
        }
    }
    close_file(0xdc, {0x100a9cb4, 0x100a9cc0});
    return true;
}

bool Loader::load_tables() {
    c.set(13, 0);
    while (compare(c, std::int32_t(c.r(13)), entry::count_of_entries) < 0) {
        const auto index = c.r(13);
        const auto at = entry_at(index);
        c.set(14, at);
        progress(0x100a9010);
        const auto kind = c.u32(at + entry::kind);
        if (each_kind_jump(toc::load_jumps, kind) && kind != 8) {
            bool ok = true;
            if (kind == 6) ok = load_kind_6();
            else if (kind == 16) ok = load_kind_16();
            else if (kind == 17) {
                const Pass p{0x100aa8f4, c.u32(c.toc(cells::rows_14_table)), 0x105ddd90, 0x54, &optional_17};
                ok = load_optional(p, cells::rows_14_table, cells::rows_14_count, 0x100aa980, true);
            } else if (kind == 18) {
                const Pass p{0x100a9cc8, c.u32(c.toc(cells::rows_3c_b_table)), 0x105df210, 0xd8, &optional_18};
                ok = load_optional(p, cells::rows_3c_b_table, cells::rows_3c_b_count, 0x100a9d54, false);
            } else {
                for (const auto& h : handlers) {
                    if (h.kind != kind) continue;
                    ok = load_required({h.base, c.u32(c.toc(h.cell)), h.reader, h.slot, h.layout});
                    break;
                }
            }
            if (!ok) return false;
        }
        c.set(13, index + 1);
    }
    return true;
}
}
