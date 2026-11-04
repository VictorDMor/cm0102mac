#include "transfer_manager_history.hpp"
#include "guest_values.hpp"

namespace cm::aot::transfer_manager {
namespace {
using namespace guest;
constexpr std::uint32_t history_rows = 0x994, history_capacity = 0x998, history_count = 0x99c, row_size = 0x28;
constexpr auto pool_slot = -0x4b9c, title_slot = -0x79dc, body_slot = -0x79e0, message_slot = -0x7ba8;
constexpr std::uint32_t format_string = 0x1077b720, close_archive = 0x107766f0;

std::uint32_t call(Cpu& c, std::uint32_t entry, std::uint32_t back,
                   std::initializer_list<std::uint32_t> args = {}) {
    unsigned r = 3;
    for (auto arg : args) c.set(r++, arg);
    c.call(entry, back);
    return c.r(3);
}

struct DiagnosticBuffers {
    std::uint32_t pool, message, title, body, name, extension;
};

std::uint32_t split_source(Cpu& c, const DiagnosticBuffers& buffers, std::uint32_t after_split) {
    call(c, 0x10739890, after_split, {buffers.pool + 0x37, 0, 0, buffers.name, buffers.extension});
    return call(c, 0x10768810, after_split + 4);
}

void report_history_allocation(Cpu& c, const DiagnosticBuffers& buffers, std::uint32_t after_split,
                               std::uint32_t line, std::uint32_t label) {
    const auto version = split_source(c, buffers, after_split);
    const auto pool = buffers.pool, message = buffers.message;
    call(c, format_string, after_split + 0x24,
         {message, pool + 0x4c, version, buffers.name, buffers.extension, line, pool + label});
    call(c, 0x10727890, after_split + 0x30, {buffers.title, pool + 0xa6});
    call(c, 0x10727890, after_split + 0x40, {buffers.body, pool + 0xbe, message});
    call(c, 0x10769b60, after_split + 0x50, {buffers.title, buffers.body, 0});
}

void report_archive_io(Cpu& c, const DiagnosticBuffers& buffers, std::uint32_t after_split,
                       std::uint32_t line, std::uint32_t success) {
    const auto version = split_source(c, buffers, after_split);
    call(c, format_string, after_split + 0x20,
         {buffers.message, buffers.pool + 0x6ff, version, buffers.name, buffers.extension, line});
    call(c, 0x10769b60, after_split + 0x30, {buffers.pool + 0x70c, buffers.message, 0});
    c.set(0, 0);
    c.put8(success, 0);
}

bool older_than_cutoff(Cpu& c, std::uint32_t record, std::uint32_t year, std::uint32_t date) {
    const auto cutoff_year = c.u16(date + 2);
    const auto order = compare_logical(c, cutoff_year, year);
    bool older = order > 0;
    if (order == 0) {
        const auto cutoff_day = c.i16(date), transfer_day = c.i16(record + 0x20);
        const auto difference = std::int32_t(cutoff_day ^ transfer_day);
        const auto half = difference >> 1;
        const auto bits = std::uint32_t(difference);
        c.s.wr<1>(reg::xer_ca, (bits >> 31) & bits);
        c.set({{3, std::uint32_t(half)}, {4, std::uint32_t(cutoff_day)}});
        older = half - (difference & cutoff_day) < 0;
    }
    c.set(0, older);
    return nonzero(c, older);
}
}

void append_past_transfer(Cpu& c, std::uint32_t manager, std::uint32_t transfer) {
    Frame frame(c, 0x103f2d50, 0x80, {.first_gpr = 26});
    const Kept<26> transfers(c, manager);
    const Kept<27> body(c, c.toc(body_slot));
    const Kept<28> title(c, c.toc(title_slot));
    const Kept<29> message(c, c.toc(message_slot));
    const Kept<30> pool(c, c.toc(pool_slot));
    const Kept<31> record(c, transfer);
    const auto count = c.u32(transfers + history_count), capacity = c.u32(transfers + history_capacity);
    c.set({{0, count}, {3, capacity}});
    if (compare(c, capacity, count) == 0) {
        c.put32(transfers + history_capacity, capacity + 100);
        const auto old = c.u32(transfers + history_rows);
        c.set(3, old);
        const bool resize = nonzero_logical(c, old);
        const auto grown = c.u32(transfers + history_capacity);
        c.set(0, grown);
        const auto size = grown * row_size;
        const auto array = resize ? call(c, 0x107751b0, 0x103f2dbc, {old, size})
                                  : call(c, 0x10775130, 0x103f2da8, {size});
        c.put32(transfers + history_rows, array);
        c.set(0, c.u32(transfers + history_rows));
        if (!nonzero_logical(c, c.r(0))) {
            report_history_allocation(c, {pool, message, title, body, frame.at(0x40), frame.at(0x38)},
                                      0x103f2de4, 0x2793, 0x1b1b);
            return;
        }
    }
    const auto length = c.u32(transfers + history_count), array = c.u32(transfers + history_rows);
    const auto out = array + length * row_size;
    c.set({{3, out}, {4, array}});
    c.put32(transfers + history_count, length + 1);
    for (unsigned offset = 0; offset < 0x1c; offset += 4)
        c.put32(out + offset, c.u32(record + offset));
    c.put8(out + 0x1c, c.u8(record + 0x1c));
    c.put16(out + 0x22, c.u16(record + 0x22));
    c.put16(out + 0x20, c.u16(record + 0x20));
    c.put8(out + 0x24, c.u8(record + 0x24));
}

void archive_past_transfers(Cpu& c, std::uint32_t manager) {
    Frame frame(c, 0x103e92b0, 0x110, {.first_gpr = 17});
    c.set(0, c.lr());
    const Kept<23> filename(c, c.toc(-0x7bb8));
    const Kept<24> success(c, c.toc(-0x7bac));
    const Kept<25> body(c, c.toc(body_slot));
    const Kept<26> title(c, c.toc(title_slot));
    const Kept<27> message(c, c.toc(message_slot));
    const Kept<28> pool(c, c.toc(pool_slot));
    const Kept<31> transfers(c, manager);
    Kept<17> archive(c, 0);
    Kept<18> archive_year(c, 0);
    Kept<20> moved(c, 0);
    const auto cutoff = frame.at(0x40), date_copy = frame.at(0x60), date = frame.at(0x38);
    call(c, 0x10717cb0, 0x103e92f8, {cutoff, c.toc(-0x7b80), 52});
    const auto cutoff_day = c.i16(cutoff);
    const auto cutoff_year = c.u16(cutoff + 2);
    const auto cutoff_leap = c.u8(cutoff + 4);
    c.set({{0, cutoff_leap}, {3, cutoff_year}, {4, std::uint32_t(cutoff_day)}});
    c.put16(date_copy, std::uint32_t(cutoff_day));
    Kept<19> index(c, 0);
    Kept<21> record(c, c.u32(transfers + history_rows));
    c.put16(date_copy + 2, cutoff_year);
    c.put8(date_copy + 4, cutoff_leap);
    const Kept<29> cutoff_word(c, c.u32(date_copy));
    const Kept<30> cutoff_tail(c, c.u32(date_copy + 4));
    Kept<22> year(c, c.r(22));
    for (;; index = index + 1, record = record + row_size) {
        const auto count = c.u32(transfers + history_count);
        c.set(0, count);
        if (compare(c, index, count) >= 0) break;
        c.put32(date, cutoff_word);
        year = c.u16(record + 0x22);
        c.put32(date + 4, cutoff_tail);
        if (!older_than_cutoff(c, record, year, date)) break;

        const auto current_year = std::uint16_t(std::uint32_t(archive_year));
        c.set(0, current_year);
        if (compare_logical(c, current_year, year) != 0) {
            const bool open = nonzero_logical(c, archive);
            archive_year = year;
            if (open) call(c, close_archive, 0x103e9398, {archive});
            call(c, format_string, 0x103e93a8, {filename, pool + 0x1838, year});
            const auto path = call(c, 0x10746280, 0x103e93b4, {pool + 0x1849, filename});
            archive = call(c, 0x10739de0, 0x103e93bc, {path, pool + 0x1852});
            if (!nonzero(c, archive)) {
                report_archive_io(c, {pool, message, 0, 0, frame.at(0xa8), frame.at(0x58)},
                                  0x103e93dc, 0x1331, success);
                continue;
            }
        }
        if (nonzero_logical(c, archive) &&
            compare_logical(c, call(c, 0x10776360, 0x103e9434, {record, row_size, 1, archive}), 1) != 0) {
            report_archive_io(c, {pool, message, 0, 0, frame.at(0x88), frame.at(0x50)},
                              0x103e9454, 0x133b, success);
        } else {
            moved = moved + 1;
        }
    }
    if (nonzero_logical(c, archive)) call(c, close_archive, 0x103e94b8, {archive});
    if (!nonzero(c, moved)) return;
    const auto count = c.u32(transfers + history_count);
    c.set(0, count);
    archive_year = count - moved + 100;
    archive = call(c, 0x10775130, 0x103e94d4, {archive_year * row_size});
    if (!nonzero(c, archive)) {
        report_history_allocation(c, {pool, message, title, body, frame.at(0x68), frame.at(0x48)},
                                  0x103e94f4, 0x135c, 0x1855);
        return;
    }
    const auto old = c.u32(transfers + history_rows);
    const auto remaining = c.u32(transfers + history_count) - moved;
    c.set({{0, remaining}, {6, old}});
    call(c, 0x1077fc28, 0x103e9568, {old + moved * row_size, archive, remaining * row_size});
    c.set(2, c.u32(frame.at(0x14)));
    call(c, 0x10775170, 0x103e9574, {c.u32(transfers + history_rows)});
    c.put32(transfers + history_rows, archive);
    c.put32(transfers + history_count, c.u32(transfers + history_count) - moved);
    c.put32(transfers + history_capacity, archive_year);
}
}
