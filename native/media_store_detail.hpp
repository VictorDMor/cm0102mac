#pragma once
// The press's store: the object behind media.dat. It holds two sorted tables,
// the history rows (24 bytes: who was written about, when, and why) and the
// event rows (28 bytes), each with a capacity and a count. The rest of the
// module reaches both through these views.
#include "media_detail.hpp"

namespace cm::aot::media::detail::store {
namespace library {
constexpr std::uint32_t allocate = 0x10775130, release = 0x10775170, reallocate = 0x107751b0,
    sort = 0x10768300, search = 0x10775600, move = 0x10739ef0, exit_game = 0x1076f7d0,
    open_file = 0x107475b0, read_file = 0x10747a80, write_file = 0x10747cc0, close_file = 0x10747900,
    today = 0x10715880, save_version = 0x10775550;
}
namespace slots {
constexpr int log_queue = -0x7bf0, history_order = -0x5df8, event_order = -0x5df4, today = -0x7b80, version = -0x7b74;
}
constexpr std::uint32_t history_size = 0x18, event_size = 0x1c, store_size = 0x1e0;
// A table grows by this many rows; -1 is both "no id" in a row's key and the exit status of the out-of-memory path.
constexpr std::uint32_t grow_step = 0x14, no_id = 0xffffffff;

// Exits the game on an allocation failure, as the original does after reporting it.
inline void out_of_memory(Cpu& c, const Frame& frame, std::uint32_t site, unsigned name, unsigned extension, unsigned line,
                          std::uint32_t back) {
    assertion(c, frame, site, name, extension, line);
    Program{c}.call(library::exit_game, back, {no_id});
}

// A game date: day of the year (signed), year, hour.
struct GameDate {
    Cpu& c;
    std::uint32_t at;
    Field<std::int16_t> day() const { return {c, at}; }
    Field<std::uint16_t> year() const { return {c, at + 2}; }
    Field<std::uint8_t> hour() const { return {c, at + 4}; }
    // Stamps the date of the record at `today` (the game date): year, day, hour.
    void assign(std::uint32_t today) const {
        year() = c.u16(today + 2);
        day() = c.i16(today);
        hour() = c.u8(today + 4);
    }
};

// One history row. The date is five bytes: day of the year (signed), year, hour.
struct HistoryRow {
    Cpu& c;
    std::uint32_t at;
    Field<std::int16_t> day() const { return {c, at}; }
    Field<std::uint16_t> year() const { return {c, at + 2}; }
    Field<std::uint8_t> hour() const { return {c, at + 4}; }
    GameDate date() const { return {c, at}; }
    Field<std::uint32_t> person() const { return {c, at + 8}; }
    Field<std::uint32_t> club() const { return {c, at + 0xc}; }
    Field<std::uint8_t> mark() const { return {c, at + 0x10}; }
    Field<std::int16_t> grade() const { return {c, at + 0x12}; }
    Field<std::uint32_t> story() const { return {c, at + 0x14}; }
};
// One event row.
struct EventRow {
    Cpu& c;
    std::uint32_t at;
    Field<std::uint32_t> person() const { return {c, at}; }
    Field<std::uint32_t> club() const { return {c, at + 4}; }
    GameDate date() const { return {c, at + 8}; }
    Field<std::uint8_t> mark() const { return {c, at + 0x10}; }
    // The amount the event is for (capped at 50 million when added); the event pass reads this word as `story`.
    Field<std::int32_t> amount() const { return {c, at + 0x14}; }
    Field<std::uint32_t> story() const { return {c, at + 0x14}; }
    Field<std::uint32_t> weight() const { return {c, at + 0x18}; }
};
struct Press {
    Cpu& c;
    std::uint32_t at;
    Field<std::uint32_t> history() const { return {c, at}; }
    Field<std::uint32_t> history_capacity() const { return {c, at + 4}; }
    Field<std::int32_t> history_count() const { return {c, at + 8}; }
    Field<std::uint32_t> events() const { return {c, at + 0xc}; }
    Field<std::uint32_t> event_capacity() const { return {c, at + 0x10}; }
    Field<std::int32_t> event_count() const { return {c, at + 0x14}; }
    HistoryRow history_row(std::uint32_t index) const { return {c, history().get() + index * history_size}; }
    EventRow event_row(std::uint32_t index) const { return {c, events().get() + index * event_size}; }
};
}
