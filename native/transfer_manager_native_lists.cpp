#include "transfer_manager_native_lists.hpp"
#include "transfer_manager_native_lists_detail.hpp"

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;

void first_accepted_day(Cpu& c, std::uint32_t manager, std::uint32_t record, std::uint32_t out) {
    constexpr std::uint32_t rule_rows = 0x9e0, rule_count = 27, rule_key = 0xc, record_key = 0x2c;
    constexpr std::uint32_t accepts_day = 0x105c4d10, add_days = 0x10717890, today = 0x10715880;
    Frame frame(c, 0x103f9200, 0xa0, {.first_gpr = 26});
    const auto start_cell = c.toc(-0x7b80), error_text = c.toc(-0x7ba8), texts = c.toc(-0x4b9c);
    const auto error_flag = c.toc(-0x7bac);
    keep(c, 26, manager);
    keep(c, 27, start_cell);
    keep(c, 28, out);
    keep(c, 29, record);
    keep(c, 30, error_text);
    keep(c, 31, texts);
    constexpr std::uint32_t current = 0x60, limit_scratch = 0x48, next_scratch = 0x40, limit_copy = 0x58;
    invoke(c, today, 0x103f9230, {frame.at(current)});
    invoke(c, add_days, 0x103f9240, {frame.at(limit_scratch), start_cell, 0x16d});
    // The day a year on is the limit; its copy (five bytes at +0x58) is read back as words.
    lists::write_date(c, frame.at(limit_copy), lists::read_date(c, frame.at(limit_scratch)));
    // The row of the rules whose key is the record's: 27 rows, none when no key matches.
    std::uint32_t row = 0;
    for (std::int8_t i = 0; i < std::int8_t(rule_count); ++i) {
        const auto candidate = c.u32(manager + rule_rows + std::uint32_t(i) * 4);
        if (c.u32(candidate + rule_key) == c.u32(record + record_key)) { row = candidate; break; }
    }
    keep(c, 29, row);
    lists::write_date(c, frame.at(current), lists::read_date(c, start_cell));
    // The limit's first word; the word at +0x5c is stale beyond its first byte and is copied to +0x3c.
    const auto limit_word = c.u32(frame.at(limit_copy));
    c.put32(frame.at(0x3c), c.u32(frame.at(0x5c)));
    keep(c, 27, limit_word);
    c.put32(frame.at(0x38), limit_word);
    const lists::Date limit{std::int16_t(limit_word >> 16), std::uint16_t(limit_word), 0};
    for (;;) {
        const auto now = lists::read_date(c, frame.at(current));
        if (!lists::date_after(limit, now)) break;
        if (row == 0 || std::int8_t(invoke(c, accepts_day, 0x103f92d8, {row, frame.at(current), 0})) != 0) {
            lists::write_date(c, out, now);
            return;
        }
        invoke(c, add_days, 0x103f930c, {frame.at(next_scratch), frame.at(current), 1});
        lists::write_date(c, frame.at(current), lists::read_date(c, frame.at(next_scratch)));
    }
    // No day is accepted within the year: the original reports it and clears the flag.
    invoke(c, 0x10739890, 0x103f938c, {texts + 0x37, 0, 0, frame.at(0x68), frame.at(0x50)});
    const auto line = invoke(c, 0x10768810, 0x103f9390);
    invoke(c, 0x1077b720, 0x103f93ac, {error_text, texts + 0x6ff, line, frame.at(0x68), frame.at(0x50), 0x32c5});
    invoke(c, 0x10769b60, 0x103f93bc, {texts + 0x70c, error_text, 0});
    c.put8(error_flag, 0);
}
}
