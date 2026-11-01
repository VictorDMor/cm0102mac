// The history rows: adding one, marking one, dropping the old ones, and the
// lookup. Rows are kept sorted (by the game's comparator at -0x5df8) after each
// change; a row's date is five bytes (day, year, hour).
#include "media_store.hpp"
#include "media_store_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::store;
namespace {
constexpr std::uint32_t story_without_expiry = 0xbdf;

// The key's id word for a person or club record: its first word, or -1 for none.
std::uint32_t id_of(Cpu& c, std::uint32_t record) { return compare_logical(c, record) == 0 ? no_id : c.u32(record); }

// Is `date` (day at +0, year at +2, in the frame) later than the row's? The years are compared
// first (unsigned), the days only when the years agree (signed); r4 is left holding the last
// value it compared.
bool later_than(Cpu& c, std::uint32_t date, std::uint32_t row) {
    const auto year = c.u16(date + 2);
    c.set(4, year);
    bool later;
    if (compare_logical(c, year, c.u16(row + 2)) > 0) later = true;
    else {
        c.set(4, c.u16(date + 2));
        if (compare_logical(c, c.u16(date + 2), c.u16(row + 2)) < 0) later = false;
        else {
            const auto day = extended(c.i16(date)), other = extended(c.i16(row));
            c.set(4, day);
            // The original's sign-bit idiom: day > other.
            const std::uint32_t x = day ^ other, y = std::uint32_t(std::int32_t(x) >> 1) - (x & day);
            later = (y >> 31) != 0;
        }
    }
    compare(c, later ? 1 : 0);
    return later;
}
}

void add_history_row(Cpu& c, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t story,
                     std::uint32_t grade, std::uint32_t mark) {
    Frame frame(c, 0x1069e190, 0xb0, {.first_gpr = 22});
    keep(c, {{22, self}, {23, person}, {24, club}, {25, story}, {26, grade}, {27, mark}, {28, c.toc(slots::today)},
             {29, c.toc(slot::flag)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    const Press press{c, self};
    Program program{c};
    if (compare_logical(c, press.history()) == 0) {
        press.history() = program.call(library::allocate, 0x1069e1dc, {store_size});
        if (compare_logical(c, press.history()) == 0) out_of_memory(c, frame, 0x1069e200, 0x68, 0x40, 0x6dae, 0x1069e244);
        press.history_capacity() = grow_step;
    }
    if (compare(c, press.history_capacity(), press.history_count()) == 0) {
        press.history() = program.call(library::reallocate, 0x1069e270,
                                       {press.history(), (press.history_capacity() + grow_step) * history_size});
        if (compare_logical(c, press.history()) == 0) out_of_memory(c, frame, 0x1069e294, 0x48, 0x38, 0x6db9, 0x1069e2d8);
        press.history_capacity() = press.history_capacity() + grow_step;
    }
    const auto row = press.history_row(std::uint32_t(press.history_count()));
    const auto today = c.toc(slots::today);
    row.person() = id_of(c, person);
    row.club() = id_of(c, club);
    row.date().assign(today);
    row.story() = story;
    row.grade() = std::int16_t(std::int8_t(grade));
    row.mark() = std::uint8_t(mark);
    press.history_count() = press.history_count() + 1;
    program.call(library::sort, 0x1069e3bc,
                 {press.history(), std::uint32_t(press.history_count()), history_size, c.toc(slots::history_order), press.history()});
}

// Sets the mark of the row for this person, club, story and grade, if the store has one.
std::uint32_t mark_history_row(Cpu& c, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t story,
                               std::uint32_t grade, std::uint32_t mark) {
    Frame frame(c, 0x1069e3d0, 0x70, {.first_gpr = 26});
    keep(c, {{26, self}, {27, person}, {28, club}, {29, story}, {30, grade}, {31, mark}});
    const Press press{c, self};
    Program program{c};
    const auto key = frame.at(0x38);
    program.call(library::today, 0x1069e400, {key});
    c.put32(key + 8, id_of(c, person));
    c.put32(key + 0xc, id_of(c, club));
    c.put16(key + 0x12, std::uint32_t(std::int32_t(std::int8_t(grade))));
    c.put32(key + 0x14, story);
    c.put8(key + 0x10, 0);
    const auto found = program.call(library::search, 0x1069e45c,
                                    {key, press.history(), std::uint32_t(press.history_count()), history_size, c.toc(slots::history_order)});
    if (compare_logical(c, found) != 0) c.put8(found + 0x10, mark);
    return c.r(3);
}

// Drops the rows that have aged out (a limit of 40-79 days, drawn from the shared stream, for
// the stories of the one kind, 100 days for the others), then re-sorts.
void prune_history(Cpu& c, std::uint32_t self) {
    Frame frame(c, 0x1069e480, 0x80, {.first_gpr = 25});
    const auto stream = Stream::world(c);
    keep(c, {{25, self}, {27, c.toc(slots::today)}, {28, stream.seed}, {29, stream.start}, {30, stream.end}, {31, stream.cursor}});
    const Press press{c, self};
    Program program{c};
    if (compare_logical(c, press.history()) == 0) return;
    Kept<26> index(c, 0);
    for (; compare(c, std::int32_t(index), press.history_count()) < 0; index = std::uint32_t(index) + 1) {
        const auto row = press.history_row(index);
        std::uint32_t cutoff, copy, back;
        if (compare(c, std::int32_t(row.story().get()), std::int32_t(story_without_expiry)) == 0) {
            stream.advance(0x1069e4f4);
            const auto drawn = std::uint32_t(stream.sample());
            // The sample plus the seed (unmasked, so it can be negative) gives 40 + drawn % 40 days.
            program.call(0x10717b20, 0x1069e540, {frame.at(0x50), c.toc(slots::today), std::uint32_t(std::int32_t(drawn) % 40 + 40), drawn});
            cutoff = frame.at(0x50); copy = frame.at(0x40); back = 0x1069e5dc;
        } else {
            program.call(0x10717b20, 0x1069e604, {frame.at(0x48), c.toc(slots::today), 100});
            cutoff = frame.at(0x48); copy = frame.at(0x38); back = 0x1069e6a0;
        }
        c.put32(copy, c.u32(cutoff));
        c.put32(copy + 4, c.u32(cutoff + 4));
        if (later_than(c, copy, row.at)) {
            // The last row takes this one's place; the same index is looked at again.
            const auto last = press.history_row(std::uint32_t(press.history_count()) - 1).at;
            program.call(library::move, back, {row.at, last, history_size, index * history_size, press.history()});
            c.set(3, std::uint32_t(press.history_count()));
            index = std::uint32_t(index) - 1;
            press.history_count() = press.history_count() - 1;
        }
    }
    if (compare(c, press.history_count()) != 0)
        program.call(library::sort, 0x1069e6e0,
                     {press.history(), std::uint32_t(press.history_count()), history_size, c.toc(slots::history_order)});
}

// Whether the store has a row for these. A story without expiry is found by a scan;
// the others by search, and a row older than `date` (when given) does not count.
std::uint32_t has_history_row(Cpu& c, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t story,
                              std::uint32_t grade, std::uint32_t date) {
    Frame frame(c, 0x1069e700, 0x70, {.first_gpr = 26});
    keep(c, {{28, self}, {26, person}, {29, club}, {30, story}, {31, grade}, {27, date}});
    const Press press{c, self};
    Program program{c};
    const auto key = frame.at(0x40);
    program.call(library::today, 0x1069e730, {key});
    if (compare(c, std::int32_t(story), std::int32_t(story_without_expiry)) == 0) {
        std::int32_t index = 0;
        c.set(5, 0);
        for (; compare(c, index, press.history_count()) < 0; ++index) {
            c.set(5, std::uint32_t(index));
            const auto row = press.history_row(std::uint32_t(index));
            if (compare(c, std::int32_t(story), std::int32_t(row.story().get())) != 0) continue;
            c.set(4, c.u32(club));
            if (compare(c, c.i32(club), std::int32_t(row.club().get())) != 0) continue;
            c.set(4, extended(std::int16_t(grade)));
            if (compare(c, std::int32_t(std::int16_t(grade)), row.grade().get()) != 0) continue;
            return result(c, 1);
        }
        c.set(5, std::uint32_t(index));
        return result(c, 0);
    }
    c.put32(key + 8, id_of(c, person));
    c.put32(key + 0xc, c.u32(club));
    c.put16(key + 0x12, grade);
    c.put32(key + 0x14, story);
    const auto found = program.call(library::search, 0x1069e7f0,
                                    {key, press.history(), std::uint32_t(press.history_count()), history_size, c.toc(slots::history_order)});
    if (compare_logical(c, found) == 0) return result(c, 0);
    if (compare_logical(c, date) == 0) return result(c, 1);
    const auto copy = frame.at(0x38);
    c.put32(copy, c.u32(date));
    c.set(5, c.u32(date + 4));
    c.put32(copy + 4, c.r(5));
    return result(c, later_than(c, copy, found) ? 0 : 1);
}
}
