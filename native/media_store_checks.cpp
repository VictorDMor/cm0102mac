// Cases for the press store: its file routines over a scripted data file (a scratch handle
// of the game's own file layer, so the real read and write run), and its rows, events and
// log records over the real clubs and over rows laid out in the scratch area.
#include "media_checks.hpp"
#include <limits>

using namespace cm::aot::media_checks;

namespace {
using Writes = std::vector<Write>;
constexpr std::uint32_t entry_construct = 0x10646d90, entry_destroy = 0x10646e00, entry_load = 0x10646e90,
    entry_save = 0x10647420, entry_add_row = 0x1069e190, entry_mark_row = 0x1069e3d0, entry_prune = 0x1069e480,
    entry_has_row = 0x1069e700, entry_roll_club = 0x1069e990, entry_roll_record = 0x1069eb10, entry_log_2e = 0x106a6e60,
    entry_add_event = 0x10698bb0, entry_remove_events = 0x10698e80, entry_log_1790 = 0x10688e80,
    entry_log_178e = 0x106a15f0, entry_log_bcb = 0x1069d1a0;
// The file layer, answered where a case needs it to fail or to hand out the scratch handle. The allocator
// is native in the checker's originals, so it cannot be answered: failures come from sizes no arena has.
constexpr std::uint32_t open_entry = 0x107475b0, write_entry = 0x10747cc0, close_entry = 0x10747900;
constexpr std::uint32_t any = 0xffffffff;

// Scratch layout.
constexpr std::uint32_t store_at = scratch_base, file_at = scratch_base + 0x1000,
    events_at = scratch_base + 0xa4000, history_at = scratch_base + 0xa8000, key_at = scratch_base + 0xac000,
    record_at = scratch_base + 0xad000;

void put32(Writes& out, std::uint32_t at, std::uint32_t value) { out.push_back({4, at, value}); }
void put16(Writes& out, std::uint32_t at, std::uint32_t value) { out.push_back({2, at, value}); }
void put8(Writes& out, std::uint32_t at, std::uint32_t value) { out.push_back({1, at, value}); }
Writes join(std::initializer_list<Writes> parts) {
    Writes all;
    for (const auto& part : parts) all.insert(all.end(), part.begin(), part.end());
    return all;
}
Case make(std::uint32_t entry, std::vector<std::pair<unsigned, std::uint32_t>> registers, Writes memory, std::string label,
          std::vector<std::pair<std::uint32_t, std::uint32_t>> answers = {}) {
    return {.entry = entry, .registers = std::move(registers), .memory = std::move(memory), .f1 = {}, .label = std::move(label),
            .answers = std::move(answers)};
}

// The press object's six words.
Writes press(std::uint32_t at, std::uint32_t history, std::uint32_t history_capacity, std::uint32_t history_count,
             std::uint32_t events, std::uint32_t event_capacity, std::uint32_t event_count) {
    Writes out;
    put32(out, at, history); put32(out, at + 4, history_capacity); put32(out, at + 8, history_count);
    put32(out, at + 0xc, events); put32(out, at + 0x10, event_capacity); put32(out, at + 0x14, event_count);
    return out;
}

// A file in the game's own layer, as a scratch handle whose reads decode a run-length stream
// held in memory: bytes up to 0x80 are literal, a byte above 0x80 is a run (count - 0x80) of
// the next byte. `size` is how many bytes the layer lets be read; less than the stream makes
// reads come up short.
Writes read_file(const std::vector<std::uint8_t>& bytes, std::uint32_t size) {
    Writes out;
    std::vector<std::uint8_t> stream;
    for (const auto byte : bytes) {
        if (byte > 0x80) { stream.push_back(0x81); }
        stream.push_back(byte);
    }
    put8(out, file_at + 4, 0); put32(out, file_at + 8, 0); put32(out, file_at + 0xc, 0);
    put8(out, file_at + 0x1c, 4); put32(out, file_at + 0x10, 0); put32(out, file_at + 0x14, 0);
    put32(out, file_at + 0x28, size);
    put32(out, file_at + 0xa21e4, 0); put32(out, file_at + 0xa21ec, std::uint32_t(stream.size()));
    for (std::size_t n = 0; n < stream.size(); ++n) put8(out, file_at + 0x40764 + std::uint32_t(n), stream[n]);
    return out;
}
// A file open for writing in the same layer (the encoder buffers in memory).
Writes write_file(bool writable = true) {
    Writes out;
    put8(out, file_at + 4, writable ? 1 : 0); put32(out, file_at + 8, 0); put32(out, file_at + 0xc, 0);
    put8(out, file_at + 0x1c, 4); put32(out, file_at + 0x10, 0); put32(out, file_at + 0x14, 0);
    put32(out, file_at + 0x28, 0); put32(out, file_at + 0xa21e8, 0);
    return out;
}
void be32(std::vector<std::uint8_t>& out, std::uint32_t value) {
    for (int shift = 24; shift >= 0; shift -= 8) out.push_back(std::uint8_t(value >> shift));
}
// media.dat as the game writes it: the events (28 bytes each), then the history rows (24 bytes each).
std::vector<std::uint8_t> data_file(unsigned events, unsigned rows) {
    std::vector<std::uint8_t> out;
    be32(out, events);
    for (unsigned n = 0; n < events * 0x1c; ++n) out.push_back(std::uint8_t((n * 7 + n / 28 * 13 + 5) % 0x7f));
    be32(out, rows);
    for (unsigned n = 0; n < rows * 0x18; ++n) {
        std::uint8_t byte = std::uint8_t((n * 11 + n / 24 * 5 + 3) % 0x7f);
        // Old rows keep their grade as a signed byte at +0x11, and their first halfword may be negative.
        if (n % 24 == 0x11 || n % 24 == 0) byte = std::uint8_t(0x80 + (n / 24) * 17 % 0x7f);
        out.push_back(byte);
    }
    return out;
}
// Rows in the scratch area for a save: bytes of the row come from their position.
Writes fill(std::uint32_t at, unsigned bytes, unsigned seed) {
    Writes out;
    for (unsigned n = 0; n < bytes; ++n) put8(out, at + n, (n * 5 + seed) & 0xff);
    return out;
}

struct RowSpec { std::int16_t day; std::uint16_t year; std::uint8_t hour; std::uint32_t person, club; std::uint8_t mark; std::int16_t grade; std::uint32_t story; };
Writes history_rows(std::uint32_t at, const std::vector<RowSpec>& rows) {
    Writes out;
    for (std::size_t n = 0; n < rows.size(); ++n) {
        const auto row = at + std::uint32_t(n) * 0x18;
        const auto& r = rows[n];
        put16(out, row, std::uint16_t(r.day)); put16(out, row + 2, r.year); put8(out, row + 4, r.hour);
        put32(out, row + 8, r.person); put32(out, row + 0xc, r.club); put8(out, row + 0x10, r.mark);
        put16(out, row + 0x12, std::uint16_t(r.grade)); put32(out, row + 0x14, r.story);
    }
    return out;
}
// A date as the game keeps it: day, year, hour and a second word.
Writes date_at(std::uint32_t at, std::int16_t day, std::uint16_t year, std::uint8_t hour, std::uint32_t second = 0) {
    Writes out;
    put16(out, at, std::uint16_t(day)); put16(out, at + 2, year); put32(out, at + 4, (std::uint32_t(hour) << 24) | (second & 0xffffff));
    return out;
}
}

MEDIA_CASES("store") {
    const auto version = scenario.toc(-0x7b74);
    const std::uint32_t path = 0x2222;
    const std::vector<std::pair<std::uint32_t, std::uint32_t>> file_answers{{open_entry, file_at}, {close_entry, 0}};
    const auto with_version = [&](Writes memory, std::uint32_t value) { put32(memory, version, value); return memory; };
    const std::uint32_t old_version = 0xfffc1fca, new_version = 0xfffc1fba;

    // ---- load: every read, allocation and version
    for (const auto [events, rows, label] : {std::tuple<unsigned, unsigned, const char*>{3, 4, "3 events 4 rows"}, {0, 0, "empty"}, {1, 0, "1 event no rows"}, {0, 2, "no events 2 rows"}}) {
        const auto data = data_file(events, rows);
        const auto size = std::uint32_t(data.size());
        for (const auto old : {false, true})
            cases.push_back(make(entry_load, {{3, store_at}, {4, path}},
                with_version(join({press(store_at, 0x11111111, 0x22222222, 0x33333333, 0x44444444, 0x55555555, 0x66666666), read_file(data, size)}), old ? old_version : new_version),
                std::string(label) + (old ? " old format" : " new format"), file_answers));
    }
    {
        const auto data = data_file(3, 4);
        const std::uint32_t events_end = 4 + 3 * 0x1c, rows_start = events_end + 4, total = std::uint32_t(data.size());
        for (const auto old : {false, true}) {
            const auto v = old ? old_version : new_version;
            const std::string format = old ? " old" : " new";
            const auto short_read = [&](std::uint32_t size, const char* what) {
                cases.push_back(make(entry_load, {{3, store_at}, {4, path}},
                    with_version(join({press(store_at, 0, 0, 0, 0, 0, 0), read_file(data, size)}), v), what + format, file_answers));
            };
            short_read(0, "no data");
            short_read(2, "count cut");
            short_read(4 + 10, "events cut");
            short_read(events_end, "second count missing");
            short_read(events_end + 2, "second count cut");
            short_read(rows_start + 20, "rows cut");
            short_read(total - 1, "last row cut");
        }
        cases.push_back(make(entry_load, {{3, store_at}, {4, 0}}, join({press(store_at, 1, 2, 3, 4, 5, 6)}), "no path"));
        cases.push_back(make(entry_load, {{3, store_at}, {4, path}}, join({press(store_at, 1, 2, 3, 4, 5, 6)}), "no file",
                             {{open_entry, 0}, {close_entry, 0}}));
        // Allocation failures: events first (count > 0), then rows (events 0, rows > 0), old format staging.
        cases.push_back(make(entry_load, {{3, store_at}, {4, path}}, join({press(store_at, 1, 2, 3, 4, 5, 6), read_file(data, total)}),
                             "events allocation", {{open_entry, file_at}, {close_entry, 0}}));
        const auto only_rows = data_file(0, 2);
        for (const auto old : {false, true})
            cases.push_back(make(entry_load, {{3, store_at}, {4, path}},
                with_version(join({press(store_at, 1, 2, 3, 4, 5, 6), read_file(only_rows, std::uint32_t(only_rows.size()))}), old ? old_version : new_version),
                old ? "rows allocation old" : "rows allocation new", {{open_entry, file_at}, {close_entry, 0}}));
    }
    // Allocation failures: a count whose table cannot be had.
    {
        std::vector<std::uint8_t> huge_events, huge_rows;
        be32(huge_events, 0x08000000);
        be32(huge_rows, 0);
        be32(huge_rows, 0x09000000);
        for (const auto old : {false, true}) {
            const auto v = old ? old_version : new_version;
            cases.push_back(make(entry_load, {{3, store_at}, {4, path}}, with_version(join({press(store_at, 1, 2, 3, 4, 5, 6), read_file(huge_events, 4)}), v),
                                 old ? "events allocation old" : "events allocation new", file_answers));
            cases.push_back(make(entry_load, {{3, store_at}, {4, path}}, with_version(join({press(store_at, 1, 2, 3, 4, 5, 6), read_file(huge_rows, 8)}), v),
                                 old ? "rows allocation old" : "rows allocation new", file_answers));
        }
    }
    // Tables of 4M rows (96 MB) fit once in the arena, but not twice (old format stages the rows first).
    for (const std::uint32_t rows : {0x00400000u}) {
        std::vector<std::uint8_t> sized;
        be32(sized, 0);
        be32(sized, rows);
        cases.push_back(make(entry_load, {{3, store_at}, {4, path}}, with_version(join({press(store_at, 1, 2, 3, 4, 5, 6), read_file(sized, 8)}), old_version),
                             "staging allocation " + std::to_string(rows), file_answers));
    }
    // ---- construct: with and without a path
    cases.push_back(make(entry_construct, {{3, store_at}, {4, 0}, {5, 0x5151}, {6, 0x6161}}, press(store_at, 1, 2, 3, 4, 5, 6), "no path"));
    {
        const auto data = data_file(2, 3);
        cases.push_back(make(entry_construct, {{3, store_at}, {4, path}, {5, 0x5151}}, with_version(join({press(store_at, 1, 2, 3, 4, 5, 6), read_file(data, std::uint32_t(data.size()))}), new_version),
                             "with path", file_answers));
        const auto cut = data_file(2, 3);
        cases.push_back(make(entry_construct, {{3, store_at}, {4, path}}, join({press(store_at, 1, 2, 3, 4, 5, 6), read_file(cut, 3)}),
                             "with path, read cut", file_answers));
    }
    // ---- destroy (the real object and real heap blocks: the allocator is native and cannot be answered)
    {
        const auto real = scenario.global(-0x7b94);
        const auto events = scenario.r32(real + 0xc);
        const auto history_block = scenario.r32(scenario.global(-0x7bf0) + 8);
        for (const std::uint32_t flags : {0u, 1u, 0x10001u, 0x8000u, 0x7fffu, 0xffffu, 0x80000001u}) {
            cases.push_back(make(entry_destroy, {{3, real}, {4, flags}}, press(real, history_block, 4, 3, events, 12, 12), "both tables"));
        }
        cases.push_back(make(entry_destroy, {{3, real}, {4, 1}}, press(real, 0, 0, 0, events, 12, 12), "events only"));
        cases.push_back(make(entry_destroy, {{3, real}, {4, 1}}, press(real, history_block, 4, 3, 0, 0, 0), "history only"));
        cases.push_back(make(entry_destroy, {{3, real}, {4, 1}}, press(real, 0, 0, 0, 0, 0, 0), "empty"));
        cases.push_back(make(entry_destroy, {{3, 0}, {4, 1}}, {}, "no object"));
    }
    // ---- save
    {
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> closing{{open_entry, file_at}, {close_entry, 0}};
        const auto stored = [&](unsigned events, unsigned rows) {
            return join({press(store_at, history_at, rows, rows, events_at, events, events), fill(events_at, events * 0x1c, 3), fill(history_at, rows * 0x18, 9)});
        };
        for (const auto [events, rows] : {std::pair<unsigned, unsigned>{3, 4}, {0, 0}, {1, 0}, {0, 2}, {20, 30}})
            cases.push_back(make(entry_save, {{3, store_at}, {4, path}}, join({stored(events, rows), write_file()}),
                                 "save " + std::to_string(events) + "/" + std::to_string(rows), closing));
        cases.push_back(make(entry_save, {{3, store_at}, {4, 0}}, stored(3, 4), "no path"));
        cases.push_back(make(entry_save, {{3, store_at}, {4, path}}, stored(3, 4), "no file", {{open_entry, 0}, {close_entry, 0}}));
        cases.push_back(make(entry_save, {{3, store_at}, {4, path}}, join({stored(3, 4), write_file(false)}), "first write refused", closing));
        // A write layer that reports one item written: the later writes come up short when a count is 2.
        const std::vector<std::pair<std::uint32_t, std::uint32_t>> one{{open_entry, file_at}, {close_entry, 0}, {write_entry, 1}};
        cases.push_back(make(entry_save, {{3, store_at}, {4, path}}, join({stored(2, 1), write_file()}), "events short", one));
        cases.push_back(make(entry_save, {{3, store_at}, {4, path}}, join({stored(1, 2), write_file()}), "rows short", one));
        cases.push_back(make(entry_save, {{3, store_at}, {4, path}}, join({stored(1, 1), write_file()}), "all single", one));
    }

    // ---- history rows
    const auto real = scenario.global(-0x7b94);
    const auto club1 = scenario.club(1), club8 = scenario.club(8), club18 = scenario.club(18);
    const auto player1 = scenario.r32(club1 + 0x98), player2 = scenario.r32(club1 + 0x9c);
    const auto events_block = scenario.r32(real + 0xc);
    {
        for (const auto [story, grade, mark] : {std::tuple<std::uint32_t, std::uint32_t, std::uint32_t>{0xbd0, 5, 1}, {0xbdf, 0xff, 0}, {0xc00, 0x80, 0x1ff}, {0xbd0, 0x7f, 0x80}})
            cases.push_back(make(entry_add_row, {{3, real}, {4, player1}, {5, club1}, {6, story}, {7, grade}, {8, mark}}, {}, "add to empty store"));
        cases.push_back(make(entry_add_row, {{3, real}, {4, 0}, {5, 0}, {6, 0xbd1}, {7, 3}, {8, 1}}, {}, "add without person or club"));
        for (const std::uint32_t count : {0u, 5u, 11u})
            cases.push_back(make(entry_add_row, {{3, store_at}, {4, player2}, {5, club8}, {6, 0xbd2}, {7, 0xfe}, {8, 7}},
                                 press(store_at, events_block, 12, count, 0, 0, 0), "add with room " + std::to_string(count)));
        cases.push_back(make(entry_add_row, {{3, store_at}, {4, player2}, {5, club8}, {6, 0xbd3}, {7, 2}, {8, 9}},
                             press(store_at, events_block, 12, 12, 0, 0, 0), "add grows the table"));
        cases.push_back(make(entry_add_row, {{3, store_at}, {4, player1}, {5, club1}, {6, 0xbd3}, {7, 2}, {8, 9}},
                             press(store_at, events_block, 0x07000000, 0x07000000, 0, 0, 0), "add cannot grow the table"));
    }
    {
        // Rows sorted by person, club, story and grade (signed), the dates apart.
        const std::vector<RowSpec> rows{
            {10, 2001, 3, any, 3, 0, 1, 0xbd0}, {20, 2000, 4, 5, any, 0, 2, 0xbd0}, {30, 2001, 5, 5, 3, 0, -1, 0xbd0},
            {40, 2001, 6, 5, 3, 0, 2, 0xbd0}, {50, 1999, 7, 5, 3, 0, 2, 0xbdf}, {60, 2001, 8, 7, 3, 0, 4, 0xbdf}, {70, 2001, 9, 9, 4, 0, 4, 0xbd5}};
        const auto person_at = key_at, club_at = key_at + 8, date_ptr = key_at + 0x20;
        const auto base = join({press(store_at, history_at, 8, std::uint32_t(rows.size()), 0, 0, 0), history_rows(history_at, rows)});
        const auto with = [&](std::uint32_t person, std::uint32_t club) { return join({base, std::vector<Write>{{4, person_at, person}, {4, club_at, club}}}); };
        for (const auto [person, club, story, grade, label] : {std::tuple<std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t, const char*>{5, 3, 0xbd0, 2, "found"}, {5, 3, 0xbd0, 0xff, "found negative grade"},
                 {5, 3, 0xbd0, 3, "wrong grade"}, {5, 4, 0xbd0, 2, "wrong club"}, {6, 3, 0xbd0, 2, "wrong person"}, {5, 3, 0xbd9, 2, "wrong story"}, {9, 4, 0xbd5, 4, "last row"}}) {
            cases.push_back(make(entry_mark_row, {{3, store_at}, {4, person_at}, {5, club_at}, {6, story}, {7, grade}, {8, 0x5a}}, with(person, club), std::string("mark ") + label));
            cases.push_back(make(entry_has_row, {{3, store_at}, {4, person_at}, {5, club_at}, {6, story}, {7, grade}, {8, 0}}, with(person, club), std::string("has ") + label));
        }
        cases.push_back(make(entry_mark_row, {{3, store_at}, {4, 0}, {5, club_at}, {6, 0xbd0}, {7, 1}, {8, 0x33}}, with(0, 3), "mark without person"));
        cases.push_back(make(entry_mark_row, {{3, store_at}, {4, person_at}, {5, 0}, {6, 0xbd0}, {7, 2}, {8, 0x33}}, with(5, 0), "mark without club"));
        cases.push_back(make(entry_mark_row, {{3, store_at}, {4, person_at}, {5, club_at}, {6, 0xbd0}, {7, 2}, {8, 0x33}}, press(store_at, 0, 0, 0, 0, 0, 0), "mark in an empty store"));
        cases.push_back(make(entry_has_row, {{3, store_at}, {4, 0}, {5, club_at}, {6, 0xbd0}, {7, 1}, {8, 0}}, with(0, 3), "has without person"));
        // The scan for the story without expiry: each test failing in turn.
        for (const auto [club, grade, label] : {std::tuple<std::uint32_t, std::uint32_t, const char*>{3, 4, "scan found"}, {3, 2, "scan found first"}, {4, 4, "scan wrong club"}, {3, 9, "scan wrong grade"}, {3, 0x10004, "scan grade truncated"}})
            cases.push_back(make(entry_has_row, {{3, store_at}, {4, person_at}, {5, club_at}, {6, 0xbdf}, {7, grade}, {8, 0}}, with(5, club), label));
        cases.push_back(make(entry_has_row, {{3, store_at}, {4, person_at}, {5, club_at}, {6, 0xbdf}, {7, 4}, {8, 0}}, join({press(store_at, history_at, 0, 0, 0, 0, 0), std::vector<Write>{{4, club_at, 3}}}), "scan of nothing"));
        // The search with a date: the found row (person 5, club 3, story 0xbd0, grade 2) is dated day 40, year 2001.
        const std::vector<std::tuple<std::int16_t, std::uint16_t, const char*>> dates{{41, 2001, "later day"}, {39, 2001, "earlier day"}, {40, 2001, "same day"}, {1, 2002, "later year"},
                                                                                          {365, 2000, "earlier year"}, {-5, 2001, "negative day"}, {-50, 2001, "day below negative"}};
        for (const auto& [day, year, label] : dates)
            cases.push_back(make(entry_has_row, {{3, store_at}, {4, person_at}, {5, club_at}, {6, 0xbd0}, {7, 2}, {8, date_ptr}},
                                 join({with(5, 3), date_at(date_ptr, day, year, 3, 0x123456)}), std::string("has dated ") + label));
        cases.push_back(make(entry_has_row, {{3, store_at}, {4, person_at}, {5, club_at}, {6, 0xbd0}, {7, 9}, {8, date_ptr}}, join({with(5, 3), date_at(date_ptr, 1, 2001, 3)}), "has dated, no such row"));
    }

    // Pruning: rows of both kinds around today (day 354 of 2001); which draw each needs comes from the stream.
    {
        const auto seed_var = scenario.toc(-0x7b90), cursor_var = scenario.toc(-0x7b84), end_var = scenario.toc(-0x7b88);
        const auto end_value = scenario.r32(end_var), start_value = scenario.toc(-0x7b8c);
        (void)start_value;
        const std::vector<RowSpec> rows{
            {354, 2001, 1, 1, 1, 0, 0, 0xbd0}, {350, 2001, 1, 2, 1, 0, 0, 0xbdf}, {300, 2001, 1, 3, 1, 0, 0, 0xbd0}, {200, 2001, 1, 4, 1, 0, 0, 0xbdf},
            {100, 2001, 1, 5, 1, 0, 0, 0xbd0}, {10, 2001, 1, 6, 1, 0, 0, 0xbdf}, {300, 2000, 1, 7, 1, 0, 0, 0xbd0}, {1, 1999, 1, 8, 1, 0, 0, 0xbdf},
            {360, 2001, 1, 9, 1, 0, 0, 0xbd0}, {5, 2002, 1, 10, 1, 0, 0, 0xbdf}, {-5, 2001, 1, 11, 1, 0, 0, 0xbdf}, {-200, 1990, 1, 12, 1, 0, 0, 0xbd0}};
        const auto stored = [&](std::size_t count) {
            return join({press(store_at, history_at, 16, std::uint32_t(count), 0, 0, 0), history_rows(history_at, std::vector<RowSpec>(rows.begin(), rows.begin() + std::ptrdiff_t(count)))});
        };
        for (const std::size_t count : {1u, 2u, 3u, 4u, 6u, 8u, 12u})
            cases.push_back(make(entry_prune, {{3, store_at}}, stored(count), "prune " + std::to_string(count)));
        for (const std::uint32_t seed : {0u, 7u, 39u, 41u, 0xffffu, 0x80000000u, 0xffffff00u})
            cases.push_back(make(entry_prune, {{3, store_at}}, join({stored(12), std::vector<Write>{{4, seed_var, seed}}}), "prune with seed " + std::to_string(seed)));
        cases.push_back(make(entry_prune, {{3, store_at}}, join({stored(12), std::vector<Write>{{4, cursor_var, end_value}}}), "prune with the stream at its end"));
        cases.push_back(make(entry_prune, {{3, store_at}}, join({stored(12), std::vector<Write>{{4, cursor_var, end_value - 2}}}), "prune with the stream one before its end"));
        cases.push_back(make(entry_prune, {{3, store_at}}, press(store_at, 0, 0, 0, 0, 0, 0), "prune without history"));
        cases.push_back(make(entry_prune, {{3, store_at}}, press(store_at, history_at, 4, 0, 0, 0, 0), "prune with no rows"));
        cases.push_back(make(entry_prune, {{3, real}}, {}, "prune the real store"));
        // Everything aged out: no row is left to sort.
        cases.push_back(make(entry_prune, {{3, store_at}}, join({press(store_at, history_at, 16, 2, 0, 0, 0), history_rows(history_at, {{-200, 1990, 1, 1, 1, 0, 0, 0xbd0}, {-300, 1980, 1, 2, 1, 0, 0, 0xbd0}})}), "prune everything"));
    }

    // The draws: each outcome of the roll of 100 and of the roll of 4, a sum below zero, and a refill.
    {
        const auto seed_var = scenario.toc(-0x7b90), cursor_var = scenario.toc(-0x7b84), end_var = scenario.toc(-0x7b88);
        const auto cursor = scenario.r32(cursor_var), end_value = scenario.r32(end_var);
        const auto next = cursor + 2;
        const auto word = scenario.r32(next & ~3u);
        const std::uint32_t sample = (next & 2) ? (word & 0xffff) : (word >> 16);
        const auto seed_for = [&](std::uint32_t target, std::uint32_t modulus) { return (target + modulus * 4 - sample % modulus) % modulus; };
        const auto stream = [&](std::uint32_t seed) { return std::vector<Write>{{4, seed_var, seed}}; };
        const auto fame = [&](std::uint32_t club, std::uint32_t value) { return std::vector<Write>{{2, club + 0x1ec, value}}; };
        const auto attributes = record_at + 0x40;
        const auto record_with_fame = [&](std::uint32_t value) {
            return std::vector<Write>{{4, record_at + 0x14, attributes}, {2, attributes + 0xa, value}};
        };
        std::vector<std::pair<std::string, std::vector<Write>>> draws;
        for (const std::uint32_t target : {0u, 3u, 7u, 8u, 10u, 14u, 15u, 20u, 39u, 40u, 50u, 59u, 60u, 80u, 99u})
            draws.push_back({"hundred " + std::to_string(target), stream(seed_for(target, 100))});
        for (const std::uint32_t target : {0u, 1u, 2u, 3u}) draws.push_back({"four " + std::to_string(target), stream(seed_for(target, 4))});
        for (const std::uint32_t seed : {0x80000000u, 0x80000001u, 0x80000002u, 0x80000003u, 0xfffffff0u, 0xffffff9du})
            draws.push_back({"negative " + std::to_string(seed), stream(seed)});
        draws.push_back({"refill", {{4, cursor_var, end_value}}});
        draws.push_back({"one before the end", {{4, cursor_var, end_value - 2}}});
        for (const auto& [label, draw] : draws) {
            cases.push_back(make(entry_roll_club, {{3, club1}}, join({draw, fame(club1, 7654)}), "club known " + label));
            cases.push_back(make(entry_roll_club, {{3, club1}}, join({draw, fame(club1, 0x186b)}), "club just known " + label));
            cases.push_back(make(entry_roll_club, {{3, club8}}, join({draw, fame(club8, 0x186a)}), "club just unknown " + label));
            cases.push_back(make(entry_roll_club, {{3, club18}}, join({draw, fame(club18, 0x8000)}), "club negative fame " + label));
            cases.push_back(make(entry_roll_record, {{3, record_at}}, join({draw, record_with_fame(7000)}), "record known " + label));
            cases.push_back(make(entry_roll_record, {{3, record_at}}, join({draw, record_with_fame(0x186a)}), "record just unknown " + label));
        }
        cases.push_back(make(entry_roll_record, {{3, player1}}, {}, "record of a player"));
        cases.push_back(make(entry_roll_club, {{3, club18}}, {}, "club as it is"));
    }


    // ---- event rows (the real store holds 12, sorted by person and club, all dated today)
    {
        const auto person_id = key_at + 0x40, club_id = key_at + 0x48;
        const auto ids = [&](std::uint32_t person, std::uint32_t club) { return std::vector<Write>{{4, person_id, person}, {4, club_id, club}}; };
        const auto room = [&](std::uint32_t capacity) { return std::vector<Write>{{4, real + 0x10, capacity}}; };
        const auto add = [&](std::uint32_t person, std::uint32_t club, std::uint32_t amount, double weight, std::vector<Write> memory, std::string label,
                             std::uint32_t store = 0) {
            auto item = make(entry_add_event, {{3, store ? store : real}, {4, person}, {5, club}, {6, amount}}, std::move(memory), std::move(label));
            item.f1 = weight;
            cases.push_back(std::move(item));
        };
        add(person_id, club_id, 100, 1.0, ids(0x11f9, 0x3ca), "event already there");
        add(person_id, club_id, 100, 1.0, join({ids(0x11f9, 0x3cb)}), "event for another club (the table grows)");
        add(person_id, club_id, 100, 1.0, join({ids(0x11f9, 0x3cb), room(20)}), "event for another club (room left)");
        add(person_id, club_id, 100, 1.0, join({ids(0x1, 0x2), room(20)}), "event before the first");
        add(person_id, club_id, 100, 1.0, join({ids(0x20000, 0x2), room(20)}), "event after the last");
        add(person_id, 0, 100, 1.0, join({ids(0x11f9, 0), std::vector<Write>{{4, events_block + 0x1c + 4, any}}}), "event found with no club");
        add(person_id, club_id, 100, 1.0, join({ids(0x13476, 0x171a)}), "event found, same person twice");
        for (const std::uint32_t amount : {0u, 100u, 49'999'999u, 50'000'000u, 50'000'001u, 0x7fffffffu, 0xfffffffbu, 0x80000000u})
            add(person_id, club_id, amount, 2.5, join({ids(0x5000, 0x3), room(20)}), "event amount " + std::to_string(amount));
        for (const double weight : {0.0, 0.5, 1.0, 1e9, -3.7, 3e9, -3e9, 1e-300, 1e300, std::numeric_limits<double>::infinity(), std::numeric_limits<double>::quiet_NaN(), 2147483647.5, -0.0})
            add(person_id, club_id, 77, weight, join({ids(0x5000, 0x3), room(20)}), "event weight " + std::to_string(weight));
        add(person_id, club_id, 5, 1.0, ids(0x5000, 0x3), "event in an empty store", store_at);
        cases.back().memory = join({press(store_at, 0, 0, 0, 0, 0, 0), ids(0x5000, 0x3)});
        // Removing a person's events.
        for (const auto [id, label] : {std::pair<std::uint32_t, const char*>{0x13476, "two events"}, {0x11f9, "one event"}, {0x7de, "first"}, {0x13ada, "last"}, {0x5, "none"}})
            cases.push_back(make(entry_remove_events, {{3, real}, {4, person_id}}, ids(id, 0), std::string("remove ") + label));
        Writes same;
        for (unsigned n = 0; n < 12; ++n) put32(same, events_block + n * 0x1c, 0x42);
        cases.push_back(make(entry_remove_events, {{3, real}, {4, person_id}}, join({same, ids(0x42, 0)}), "remove all"));
        cases.push_back(make(entry_remove_events, {{3, store_at}, {4, person_id}}, join({press(store_at, 0, 0, 0, 0, 0, 0), ids(0x42, 0)}), "remove from an empty store"));
        cases.push_back(make(entry_remove_events, {{3, store_at}, {4, person_id}}, join({press(store_at, 0, 0, 0, events_block, 12, 1), ids(0x7de, 0)}), "remove the only row"));
    }

    // ---- log records, over the real clubs and people
    {
        const auto manager1 = scenario.r32(club1 + 0x90), manager8 = scenario.r32(club8 + 0x90);
        const auto seed_var = scenario.toc(-0x7b90), cursor_var = scenario.toc(-0x7b84), end_var = scenario.toc(-0x7b88);
        const auto end_value = scenario.r32(end_var);
        const auto next = scenario.r32(cursor_var) + 2;
        const auto word = scenario.r32(next & ~3u);
        const std::uint32_t sample = (next & 2) ? (word & 0xffff) : (word >> 16);
        const auto odd = std::vector<Write>{{4, seed_var, (sample & 1) ? 0u : 1u}}, even = std::vector<Write>{{4, seed_var, (sample & 1) ? 1u : 0u}};
        const std::vector<std::pair<std::string, std::vector<Write>>> parities{{"odd", odd}, {"even", even}, {"negative odd", {{4, seed_var, 0x80000000u | ((sample & 1) ? 0u : 1u)}}},
            {"negative even", {{4, seed_var, 0x80000000u | ((sample & 1) ? 1u : 0u)}}}, {"refill", {{4, cursor_var, end_value}}}};
        const auto field = [&](std::uint32_t record, std::uint32_t offset, std::uint32_t value) { return std::vector<Write>{{4, record + offset, value}}; };
        const auto half = [&](std::uint32_t record, std::uint32_t offset, std::uint32_t value) { return std::vector<Write>{{2, record + offset, value}}; };

        // Kind 0x1790.
        for (const auto& [label, draw] : parities)
            for (const std::uint32_t reputation : {9000u, 8999u, 9500u})
                cases.push_back(make(entry_log_1790, {{3, 0}, {4, club1}}, join({draw, half(club1, 0x1ec, reputation)}), "log 1790 " + label + " reputation " + std::to_string(reputation)));
        cases.push_back(make(entry_log_1790, {{3, 0}, {4, club1}}, half(club1, 0x1ec, 0x8000), "log 1790 negative reputation"));
        const auto division = scenario.r32(club1 + 0x30);
        for (const std::uint32_t level : {12u, 13u, 0x8000u})
            cases.push_back(make(entry_log_1790, {{3, 0}, {4, club1}}, join({half(club1, 0x1ec, 9500), half(division, 0x18, level)}), "log 1790 division level " + std::to_string(level)));
        cases.push_back(make(entry_log_1790, {{3, 0}, {4, club1}}, join({half(club1, 0x1ec, 9500), field(club1, 0x30, 0)}), "log 1790 no division"));
        cases.push_back(make(entry_log_1790, {{3, 0}, {4, 0}}, {}, "log 1790 no club"));
        cases.push_back(make(entry_log_1790, {{3, 0}, {4, club1}}, field(club1, 0x2c, 0), "log 1790 no +0x2c"));
        cases.push_back(make(entry_log_1790, {{3, 0}, {4, club1}}, field(club1, 0x3c, 0), "log 1790 no +0x3c"));
        cases.push_back(make(entry_log_1790, {{3, 0}, {4, club1}}, field(scenario.r32(club1 + 0x3c), 0x10, 0), "log 1790 +0x3c empty"));
        cases.push_back(make(entry_log_1790, {{3, 0}, {4, club8}}, {}, "log 1790 other club"));
        cases.push_back(make(entry_log_1790, {{3, 0}, {4, club18}}, {}, "log 1790 small club"));

        // Kind 0x178e.
        for (const auto [reason, stage, amount, status] : {std::tuple<std::uint32_t, std::uint32_t, std::uint32_t, std::uint32_t>{1, 2, 3, 4}, {0xff, 0x80, 0x8000, 0x7f}, {0x1ff, 0x17f, 0x1ffff, 0xfffffff0}})
            for (const std::uint32_t reputation : {0x128eu, 0x128fu, 7654u, 0x8000u})
                cases.push_back(make(entry_log_178e, {{3, 0}, {4, club1}, {5, manager8}, {6, reason}, {7, stage}, {8, amount}, {9, status}}, half(club1, 0x1ec, reputation),
                                     "log 178e values " + std::to_string(reason) + " reputation " + std::to_string(reputation)));
        cases.push_back(make(entry_log_178e, {{3, 0}, {4, club18}, {5, player1}, {6, 3}, {7, 4}, {8, 5}, {9, 6}}, {}, "log 178e small club"));
        cases.push_back(make(entry_log_178e, {{3, 0}, {4, 0}, {5, manager8}, {6, 3}, {7, 4}, {8, 5}, {9, 6}}, {}, "log 178e no club"));
        cases.push_back(make(entry_log_178e, {{3, 0}, {4, club1}, {5, manager8}, {6, 3}, {7, 4}, {8, 5}, {9, 6}}, field(club1, 0x2c, 0), "log 178e no +0x2c"));
        cases.push_back(make(entry_log_178e, {{3, 0}, {4, club1}, {5, manager8}, {6, 3}, {7, 4}, {8, 5}, {9, 6}}, field(club1, 0x90, 0), "log 178e no manager"));
        cases.push_back(make(entry_log_178e, {{3, 0}, {4, club1}, {5, 0}, {6, 3}, {7, 4}, {8, 5}, {9, 6}}, {}, "log 178e no other"));

        // Kind 0xbcb: the player, the club the match was against and the match.
        const auto match = record_at;
        const auto about = [&](std::uint32_t first, std::uint32_t second, std::uint32_t home, std::uint32_t away) {
            return std::vector<Write>{{1, match + 0x49, first}, {1, match + 0x4a, second}, {4, match + 0x1c, home}, {4, match + 0x20, away}};
        };
        const auto attributes = scenario.r32(player1 + 0x14);
        for (const auto [first, second, label] : {std::tuple<std::uint32_t, std::uint32_t, const char*>{0xff, 0, "no result"}, {3, 1, "home win"}, {1, 3, "away win"}, {2, 2, "draw"},
                                                    {0x7f, 0x80, "byte limits"}, {0x80, 0x7f, "byte limits reversed"}, {0, 0xff, "second unset"}}) {
            for (const auto [home, away, side] : {std::tuple<std::uint32_t, std::uint32_t, const char*>{club8, club1, "home"}, {club1, club8, "away"}, {club1, club1, "neither"}})
                for (const auto& [parity, draw] : std::vector<std::pair<std::string, std::vector<Write>>>{parities.begin(), parities.begin() + 2})
                    cases.push_back(make(entry_log_bcb, {{3, 0}, {4, player1}, {5, club8}, {6, match}}, join({about(first, second, home, away), draw}),
                                         std::string("log bcb ") + label + " " + side + " " + parity));
        }
        cases.push_back(make(entry_log_bcb, {{3, 0}, {4, player1}, {5, club8}, {6, match}}, join({about(3, 1, club8, club1), parities[4].second}), "log bcb refill"));
        for (const std::uint32_t fame : {0xdabu, 0xdacu, 0xdadu, 0x8000u})
            cases.push_back(make(entry_log_bcb, {{3, 0}, {4, player1}, {5, club8}, {6, match}}, join({about(3, 1, club8, club1), half(attributes, 0xa, fame)}), "log bcb fame " + std::to_string(fame)));
        cases.push_back(make(entry_log_bcb, {{3, 0}, {4, player1}, {5, club8}, {6, match}}, join({about(3, 1, club8, club1), {{4, scenario.toc(-0x7b40), 0}}}), "log bcb without the list"));
        // The list the player's entries come from, answered with a count (the entries read are whatever the frame held).
        for (const std::uint32_t count : {1u, 0x80u, 0xffu})
            cases.push_back(make(entry_log_bcb, {{3, 0}, {4, player1}, {5, club8}, {6, match}}, about(3, 1, club8, club1), "log bcb listed " + std::to_string(count), {{0x103fc710, count}}));
        cases.push_back(make(entry_log_bcb, {{3, 0}, {4, 0}, {5, club8}, {6, match}}, about(3, 1, club8, club1), "log bcb no player"));
        cases.push_back(make(entry_log_bcb, {{3, 0}, {4, player1}, {5, club8}, {6, match}}, join({about(3, 1, club8, club1), field(player1, 0x14, 0)}), "log bcb no attributes"));
        cases.push_back(make(entry_log_bcb, {{3, 0}, {4, player1}, {5, 0}, {6, match}}, about(3, 1, club8, club1), "log bcb no club"));

        // Kind 0x2e.
        for (const std::uint32_t flag : {0u, 1u, 0xffu, 0x80u, 0x17fu})
            for (const auto& [label, draw] : parities)
                cases.push_back(make(entry_log_2e, {{3, 0}, {4, club1}, {5, club8}, {6, flag}}, draw, "log 2e flag " + std::to_string(flag) + " " + label));
        cases.push_back(make(entry_log_2e, {{3, 0}, {4, club1}, {5, division}, {6, 3}}, {}, "log 2e own division"));
        cases.push_back(make(entry_log_2e, {{3, 0}, {4, club1}, {5, club1 + 0x30}, {6, 3}}, {}, "log 2e other is the +0x30 slot"));
        for (const std::uint32_t club : {club18, scenario.club(19), scenario.club(20), scenario.club(2), scenario.club(0)})
            cases.push_back(make(entry_log_2e, {{3, 0}, {4, club}, {5, club8}, {6, 3}}, {}, "log 2e club " + std::to_string((club - scenario.club(0)) / 0x248)));
        for (const auto [first, second] : {std::pair<std::uint32_t, std::uint32_t>{0x016207d1, 0}, {0x00010001, 0x01000000}, {0x7fff07d1, 0x7f000000}, {0x01620000, 0x01000000}, {0, 0}})
            cases.push_back(make(entry_log_2e, {{3, 0}, {4, club1}, {5, club8}, {6, 3}}, join({field(manager1, 0x4c, first), field(manager1, 0x50, second)}), "log 2e dates " + std::to_string(first) + " " + std::to_string(second)));
        cases.push_back(make(entry_log_2e, {{3, 0}, {4, club1}, {5, club8}, {6, 3}}, {{4, scenario.toc(-0x79f0), 0}}, "log 2e without the table"));
        // 0x100c7d00 compares the club's id with a limit kept in two words; below the limit the record is not made.
        for (const auto [limit, count] : {std::pair<std::uint32_t, std::uint32_t>{0, 0}, {5, 2}, {0x7fffffff, 0}})
            cases.push_back(make(entry_log_2e, {{3, 0}, {4, club1}, {5, club8}, {6, 3}}, {{4, scenario.global(-0x7b50) ? scenario.toc(-0x7b50) : 0, limit}, {4, scenario.toc(-0x7b04), count}},
                                 "log 2e limit " + std::to_string(limit) + " " + std::to_string(count)));
        cases.push_back(make(entry_log_2e, {{3, 0}, {4, club1}, {5, club8}, {6, 3}}, field(club1, 0x2c, 0), "log 2e no +0x2c"));
        cases.push_back(make(entry_log_2e, {{3, 0}, {4, 0}, {5, club8}, {6, 3}}, {}, "log 2e no club"));
        cases.push_back(make(entry_log_2e, {{3, 0}, {4, club1}, {5, club8}, {6, 3}}, field(club1, 0x90, 0), "log 2e no manager"));
        cases.push_back(make(entry_log_2e, {{3, 0}, {4, club1}, {5, 0}, {6, 3}}, {}, "log 2e no other"));
    }
}
