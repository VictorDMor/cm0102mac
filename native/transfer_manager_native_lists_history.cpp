// 0x103f3670: the dated entries between two days, from the manager's table and the yearly files.
#include "transfer_manager_native_lists.hpp"
#include "transfer_manager_native_lists_detail.hpp"
#include <stdexcept>

namespace cm::aot::transfer_manager_native {
using guest::Cpu;
using guest::Frame;
namespace {
constexpr std::uint32_t make_date = 0x107156f0, day_of_entry = 0x10718360, grow_list = 0x107751b0, new_list = 0x10775130,
                        read_records = 0x10775fe0, close_file = 0x107766f0, open_file = 0x10739de0;
constexpr std::uint32_t table = 0x994, table_count = 0x99c;

// Where the original calls from when the list is full: the two growth calls and the failure report differ
// between the table loop and the file loop in their return addresses, frame buffers and source line.
struct GrowSite {
    std::uint32_t realloc_back, alloc_back, locate_back, line_back, line_buffer_a, line_buffer_b, line, report_back,
        report_text_back, report_end_back, report_flush_back;
};
constexpr GrowSite table_site{0x103f38a0, 0x103f38b4, 0x103f38dc, 0x103f38e0, 0x88, 0x60, 0x291e,
                              0x103f3900, 0x103f390c, 0x103f391c, 0x103f392c};
constexpr GrowSite file_site{0x103f3aa4, 0x103f3ab8, 0x103f3ae0, 0x103f3ae4, 0x68, 0x58, 0x295e,
                             0x103f3b04, 0x103f3b10, 0x103f3b20, 0x103f3b30};

struct Loader {
    Cpu& c;
    Frame& frame;
    std::uint32_t list_cell, capacity_cell, count = 0;
    std::uint32_t error_text, texts, report_a, report_b;

    // Room for one more entry: 100 more are asked of the allocator when the list is full; a failed allocation is
    // reported and the copy goes on into whatever the list pointer then holds.
    void make_room(const GrowSite& site) {
        const auto capacity = c.u32(capacity_cell);
        if (count != capacity) return;
        c.put32(capacity_cell, capacity + 100);
        const auto bytes = c.u32(capacity_cell) * entry_size_;
        if (c.u32(list_cell) != 0) c.put32(list_cell, invoke(c, grow_list, site.realloc_back, {c.u32(list_cell), bytes}));
        else c.put32(list_cell, invoke(c, new_list, site.alloc_back, {bytes}));
        if (c.u32(list_cell) != 0) return;
        invoke(c, 0x10739890, site.locate_back, {texts + 0x37, 0, 0, frame.at(std::int32_t(site.line_buffer_a)), frame.at(std::int32_t(site.line_buffer_b))});
        const auto line = invoke(c, 0x10768810, site.line_back);
        invoke(c, 0x1077b720, site.report_back,
               {error_text, texts + 0x4c, line, frame.at(std::int32_t(site.line_buffer_a)), frame.at(std::int32_t(site.line_buffer_b)),
                site.line, texts + 0x1b9b});
        invoke(c, 0x10727890, site.report_text_back, {report_b, texts + 0xa6});
        invoke(c, 0x10727890, site.report_end_back, {report_a, texts + 0xbe, error_text});
        invoke(c, 0x10769b60, site.report_flush_back, {report_b, report_a, 0});
    }
    void append(const GrowSite& site, std::uint32_t from) {
        make_room(site);
        const auto to = c.u32(list_cell) + count * entry_size_;
        ++count;
        keep(c, 22, count);
        lists::copy_entry(c, to, from);
    }
    static constexpr std::uint32_t entry_size_ = lists::entry_size;
};
}

void list_dated_entries(Cpu& c, std::uint32_t manager, std::uint32_t from_day, std::uint32_t from_year, std::uint32_t to_day,
                        std::uint32_t to_year, std::uint32_t list_cell, std::uint32_t capacity_cell) {
    Frame frame(c, 0x103f3670, 0x120, {.first_gpr = 13});
    constexpr std::int32_t record_buffer = 0xa8, record_date = 0xc8;
    const auto report_a = c.toc(-0x79e0), report_b = c.toc(-0x79dc), error_text = c.toc(-0x7ba8), texts = c.toc(-0x4b9c);
    const auto record_date_at = frame.at(record_date);
    keep(c, 25, report_a);
    keep(c, 15, manager);
    keep(c, 26, report_b);
    keep(c, 16, from_day);
    keep(c, 27, error_text);
    keep(c, 14, from_year);
    keep(c, 28, texts);
    keep(c, 17, to_day);
    keep(c, 13, to_year);
    keep(c, 18, list_cell);
    keep(c, 19, capacity_cell);
    keep(c, 22, 0);
    keep(c, 31, 0);
    keep(c, 20, 0);
    keep(c, 23, record_date_at);
    Loader load{c, frame, list_cell, capacity_cell, 0, error_text, texts, report_a, report_b};
    invoke(c, 0x10715880, 0x103f36c4, {record_date_at});
    if (c.u32(list_cell) == 0) c.put32(capacity_cell, 0);
    const auto first_day = std::int8_t(from_day), last_day = std::int8_t(to_day);
    const auto start_year = std::uint16_t(from_year), end_year = std::uint16_t(to_year);
    if (c.u32(list_cell) != 0 && std::int32_t(c.u32(capacity_cell)) > 0) {
        // A list that already covers the days (its first entry before the start, its last after the end) stays as it is.
        const auto start_words = invoke(c, make_date, 0x103f3708, {frame.at(0x50), 1, from_day, from_year, 0xffffffff});
        const lists::Date start{std::int16_t(c.u32(start_words) >> 16), std::uint16_t(c.u32(start_words)), 0};
        if (lists::date_after(start, lists::entry_date_of(c, c.u32(list_cell)))) {
            const auto end_words = invoke(c, make_date, 0x103f3780, {frame.at(0x48), 0x1c, to_day, to_year, 0xffffffff});
            const lists::Date end{std::int16_t(c.u32(end_words) >> 16), std::uint16_t(c.u32(end_words)), 0};
            const auto last = c.u32(list_cell) + (c.u32(capacity_cell) - 1) * lists::entry_size;
            if (lists::date_after(lists::entry_date_of(c, last), end)) return;
        }
    }
    // The manager's table of entries, in date order: those from the start day up to the end day.
    keep(c, 29, end_year);
    keep(c, 30, start_year);
    keep(c, 21, 0);
    keep(c, 24, 0);
    std::int32_t index = 0;
    for (std::uint32_t offset = 0; index < c.i32(manager + table_count);
         offset += lists::entry_size, ++index, keep(c, 21, std::uint32_t(index)), keep(c, 24, offset)) {
        auto entry = c.u32(manager + table) + offset;
        const auto day_of = [&](std::uint32_t back) { return std::int8_t(invoke(c, day_of_entry, back, {entry + lists::entry_date})); };
        if (c.u16(entry + 0x22) > end_year) break;
        if (c.u16(entry + 0x22) == end_year && day_of(0x103f3830) > last_day) break;
        entry = c.u32(manager + table) + offset;
        const auto year = c.u16(entry + 0x22);
        if (year < start_year) continue;
        if (year == start_year && day_of(0x103f3864) < first_day) continue;
        load.append(table_site, entry);
        keep(c, 31, load.count * lists::entry_size);
    }
    // The yearly files from the start year to the end year.
    std::uint32_t handle = 0, year = from_year;
    keep(c, 29, year);
    keep(c, 30, end_year);
    while (std::uint16_t(year) <= end_year) {
        if (handle != 0) throw std::logic_error("0x103f39c4: the file handle is 0 at the top of every year");
        invoke(c, 0x1077b720, 0x103f39dc, {c.toc(-0x7bb8), texts + 0x1838, year & 0xffff});
        const auto path = invoke(c, 0x10746280, 0x103f39e8, {texts + 0x1849, c.toc(-0x7bb8)});
        handle = invoke(c, open_file, 0x103f39f0, {path, texts + 0x1bdc});
        keep(c, 20, handle);
        if (handle != 0) {
            keep(c, 15, load.count * lists::entry_size);
            keep(c, 24, end_year);
            keep(c, 21, start_year);
            while (invoke(c, read_records, 0x103f3bac, {frame.at(record_buffer), lists::entry_size, 1, handle}) == 1) {
                const auto date = lists::read_date(c, record_date_at);
                const auto day_of = [&](std::uint32_t back) { return std::int8_t(invoke(c, day_of_entry, back, {record_date_at})); };
                if (date.year > end_year || (date.year == end_year && day_of(0x103f3a28) > last_day)) {
                    invoke(c, close_file, 0x103f3a40, {handle});
                    handle = 0;
                    keep(c, 20, 0);
                    break;
                }
                if (date.year < start_year) continue;
                if (date.year == start_year && day_of(0x103f3a68) < first_day) continue;
                load.append(file_site, frame.at(record_buffer));
                keep(c, 15, load.count * lists::entry_size);
            }
            if (handle != 0) {
                invoke(c, close_file, 0x103f3bc4, {handle});
                handle = 0;
                keep(c, 20, 0);
            }
        }
        ++year;
        keep(c, 29, year);
    }
    c.put32(capacity_cell, load.count);
}
}
