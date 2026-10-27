#include "contract_manager_detail.hpp"

namespace cm::aot::contract_manager {
using namespace detail;

void compact_schedule(Cpu& c, std::uint32_t manager_address) {
    const ScheduledRecords rows{c, manager_address};
    const auto count = rows.count().get(); c.set(0, std::uint32_t(count));
    if (compare(c, count) <= 0) return;
    auto read_index = rows.cursor().get(); c.set(4, std::uint32_t(read_index));
    if (compare(c, read_index, count) >= 0) return;
    const auto data = rows.data().get(); c.set(5, data); c.set(0, std::uint32_t(read_index) * 16);
    auto destination = data, source = data + std::uint32_t(read_index) * 16;
    c.set(7, destination); c.set(6, source);
    std::int32_t last_count;
    for (;;) {
        last_count = rows.count().get(); c.set(5, std::uint32_t(last_count));
        if (compare(c, read_index, last_count) >= 0) break;
        const auto person = c.u32(source); c.set(0, person);
        read_index = std::int32_t(std::uint32_t(read_index) + 1); c.set(4, std::uint32_t(read_index));
        c.put32(destination, person);
        const auto year = c.u16(source + 6); c.set(0, year); c.put16(destination + 6, year);
        const auto day = c.i16(source + 4); c.set(0, extended(day)); c.put16(destination + 4, extended(day));
        for (const auto offset : {8u, 12u, 13u}) {
            const auto value = c.u8(source + offset); c.set(0, value); c.put8(destination + offset, value);
        }
        source += 16; destination += 16; c.set(6, source); c.set(7, destination);
    }
    const auto cursor = rows.cursor().get(); c.set(4, std::uint32_t(cursor)); c.set(0, 0);
    const auto remaining = std::uint32_t(last_count) - std::uint32_t(cursor);
    c.set(4, remaining); rows.count() = std::int32_t(remaining); rows.cursor() = 0;
}

void add_scheduled(Cpu& c, std::uint32_t manager_address, std::uint32_t person_address,
                    std::uint32_t date_address, std::uint32_t kind_value, std::uint32_t origin_value) {
    Frame frame(c, 0x100808d0, 0xa0, {.first_gpr = 22});
    Kept<24> object(c, manager_address); Kept<25> person(c, person_address);
    Kept<30> message(c, c.toc(-0x79e0)); Kept<26> date(c, date_address);
    Kept<31> title(c, c.toc(-0x79dc)); Kept<27> kind(c, kind_value);
    Kept<22> error_buffer(c, c.toc(-0x7ba8)); Kept<28> origin(c, origin_value);
    Kept<23> pool(c, c.toc(slot::pool)); Kept<29> notice_scale(c, 100);
    const ScheduledRecords rows{c, object}; Program program(c);
    const auto capacity = rows.capacity().get(), count = rows.count().get();
    c.set(3, std::uint32_t(capacity)); c.set(4, std::uint32_t(count));
    const auto margin = std::int32_t(std::uint32_t(capacity) - 4); c.set(0, std::uint32_t(margin));
    if (compare(c, count, margin) > 0) {
        const auto enlarged = std::uint32_t(capacity) + 100; c.set(0, enlarged); rows.capacity() = std::int32_t(enlarged);
        const auto old = rows.data().get(); c.set(3, old);
        if (nonzero_logical(c, old)) {
            const auto current_capacity = std::uint32_t(rows.capacity().get()); c.set(0, current_capacity);
            rows.data() = program.call(0x107751b0, 0x1008093c, {old, current_capacity * 16});
        } else {
            const auto current_capacity = std::uint32_t(rows.capacity().get()); c.set(0, current_capacity);
            rows.data() = program.call(0x10775130, 0x10080950, {current_capacity * 16});
        }
        const auto allocated = rows.data().get(); c.set(0, allocated);
        if (!nonzero_logical(c, allocated)) {
            program.call(0x10739890, 0x10080978, {pool + 0x37, 0, 0, frame.at(0x50), frame.at(0x48)});
            const auto version = program.call(0x10768810, 0x1008097c);
            program.call(0x1077b720, 0x1008099c,
                         {error_buffer, pool + 0x4c, version, frame.at(0x50), frame.at(0x48), 0x1029, pool + 0x3fe});
            program.call(0x10727890, 0x100809a8, {title, pool + 0xb8});
            program.call(0x10727890, 0x100809b8, {message, pool + 0xd0, error_buffer});
            program.call(0x10769b60, 0x100809c8, {title, message, 0}); return;
        }
    }

    // Incrementing a row leaves its address in r3. Warning rows only set
    // person/date/origin: their kind byte and padding deliberately stay.
    auto finish_row = [&](std::uint32_t marker) {
        const auto count = std::uint32_t(rows.count().get()), data = rows.data().get();
        c.set(4, count); c.set(5, data); c.set(3, count + 1); c.set(0, count * 16);
        rows.count() = std::int32_t(count + 1);
        const auto row = data + count * 16; c.set(3, row); c.put8(row + 13, marker);
    };
    auto copy_date_to_row = [&](Date source, bool warning) {
        const auto count = std::uint32_t(rows.count().get()), data = rows.data().get();
        if (warning) c.set(6, 2);
        c.set(0, count); c.set(4, data); c.set(3, count * 16);
        const auto year = source.year().get(); c.set(0, year);
        const Date destination{c, data + count * 16 + 4}; c.set(3, destination.address); destination.year() = year;
        const auto day = source.day().get(); c.set(0, extended(day)); destination.day() = day;
        const auto leap = source.leap().get(); c.set(0, leap); destination.leap() = leap;
    };
    const auto source_kind = std::int8_t(std::uint32_t(origin)); c.set(0, std::uint32_t(std::int32_t(source_kind)));
    if (compare(c, source_kind, 1) == 0) {
        const auto attributes = c.u32(person + 0x14); c.set(0, attributes);
        if (nonzero_logical(c, attributes)) {
            const auto contract = program.call(0x10082f20, 0x100809f0, {object, person});
            const auto value = c.u8(contract + 0x4c); c.set(3, value); c.set(0, std::uint32_t(value) + 100);
            notice_scale = std::uint8_t(unsigned(value) + 100);
        }
        const auto scale = std::uint8_t(std::uint32_t(notice_scale)); c.set(0, scale);
        if (compare_logical(c, scale, 80) > 0) {
            auto warning = [&](unsigned numerator, unsigned local, std::uint32_t back, bool early) {
                const auto count = std::uint32_t(rows.count().get());
                const auto id = c.u32(person), data = rows.data().get();
                if (early) c.set(7, id); else c.set(6, id);
                c.put32(data + count * 16, id);
                const auto days = (std::uint32_t(scale) * numerator) / 200;
                c.set(0, days); c.s.wr<1>(guest::reg::xer_ca, 0);
                if (early) c.set(6, data);
                program.call(0x10717b20, back, {frame.at(local), date, days});
                copy_date_to_row({c, frame.at(local)}, true); finish_row(2);
            };
            if (compare_logical(c, scale, 150) > 0) warning(90, 0x40, 0x10080a50, true);
            warning(50, 0x38, 0x10080ae0, false);
        }
    }
    const auto row = rows.append_address(), id = c.u32(person);
    c.set(4, id); c.set(3, rows.data().get()); c.set(0, std::uint32_t(rows.count().get()) * 16);
    c.put32(row, id);
    const auto kind_row = rows.append_address(); c.set(3, kind_row); c.put8(kind_row + 12, kind);
    copy_date_to_row({c, date}, false); finish_row(origin);
}

void consume_scheduled(Cpu& c, std::uint32_t manager_address, std::uint32_t date_address, std::uint32_t kind_value) {
    Frame frame(c, 0x10080bb0, 0x60, {.first_gpr = 28});
    Kept<31> staff_holder(c, c.toc(-0x7be0)); Kept<30> kind(c, kind_value);
    Kept<29> cutoff(c, date_address); Kept<28> object(c, manager_address);
    const ScheduledRecords rows{c, object}; Program program(c);
    auto index = rows.cursor().get(), count = rows.count().get();
    c.set(3, std::uint32_t(index)); c.set(0, std::uint32_t(count));
    if (compare(c, index, count) == 0) return;
    for (;;) {
        index = rows.cursor().get(); count = rows.count().get();
        c.set(3, std::uint32_t(index)); c.set(0, std::uint32_t(count));
        if (compare(c, index, count) >= 0) return;
        const auto first_word = c.u32(cutoff); c.set(4, first_word);
        const auto offset = std::uint32_t(index) * 16; c.set(0, offset);
        const auto data = rows.data().get(); c.set(3, data);
        c.put32(frame.at(0x48), first_word);
        const auto row = data + offset; c.set(3, row);
        const auto second_word = c.u32(cutoff + 4); c.set(0, second_word);
        const auto cutoff_year = c.u16(frame.at(0x4a)), row_year = c.u16(row + 6);
        c.set(5, cutoff_year); c.set(6, row_year); c.put32(frame.at(0x38), first_word);
        const auto by_year = compare_logical(c, cutoff_year, row_year);
        c.put32(frame.at(0x3c), second_word); c.put32(frame.at(0x4c), second_word);
        bool before = by_year < 0;
        if (!by_year) {
            const auto cutoff_day = c.i16(frame.at(0x48)), row_day = c.i16(row + 4);
            c.set(0, extended(cutoff_day)); c.set(5, extended(row_day));
            const auto bits = extended(row_day) ^ extended(cutoff_day);
            c.set(4, std::uint32_t(std::int32_t(bits) >> 1));
            c.s.wr<1>(guest::reg::xer_ca, std::int32_t(bits) < 0 && (bits & 1));
            before = cutoff_day < row_day;
        }
        c.set(0, before);
        if (nonzero(c, before)) return;
        const auto saved_date = c.u32(frame.at(0x38)); c.set(0, saved_date); c.set(5, 0);
        const auto saved_flags = c.u32(frame.at(0x3c)); c.set(4, saved_flags);
        c.put32(frame.at(0x40), saved_date);
        const auto repeated_year = c.u16(frame.at(0x42)); c.set(0, repeated_year);
        c.put32(frame.at(0x44), saved_flags);
        bool same = compare_logical(c, repeated_year, row_year) == 0;
        if (same) {
            const auto repeated_day = c.i16(frame.at(0x40)), row_day = c.i16(row + 4);
            c.set(4, extended(repeated_day)); c.set(0, extended(row_day));
            same = compare(c, repeated_day, row_day) == 0;
        }
        c.set(5, same);
        if (nonzero(c, same)) {
            const auto row_kind = c.i8(row + 12), cutoff_kind = std::int8_t(std::uint32_t(kind));
            c.set(4, std::uint32_t(std::int32_t(row_kind))); c.set(0, std::uint32_t(std::int32_t(cutoff_kind)));
            if (compare(c, row_kind, cutoff_kind) > 0) return;
        }
        const auto person_id = c.i32(row); c.set(4, std::uint32_t(person_id));
        if (compare(c, person_id, -1) != 0) {
            const auto staff = c.u32(staff_holder); c.set(5, staff); c.set(0, 0xffffffff);
            const auto person_offset = std::uint32_t(person_id) * Staff::size;
            c.set(4, person_offset); c.put32(row, 0xffffffff);
            const auto origin = c.u8(row + 13); c.set(0, origin);
            const auto person = staff + person_offset; c.set(4, person);
            if (compare(c, origin, 3) == 0) program.call(0x10081bd0, 0x10080cec, {object, person});
        }
        const auto current = std::uint32_t(rows.cursor().get()); c.set(3, current); c.set(0, current + 1);
        rows.cursor() = std::int32_t(current + 1);
    }
}

std::uint32_t minimum_contract_quote(Cpu& c, std::uint32_t person_address, std::uint32_t club_address) {
    Frame frame(c, 0x10081b10, 0xb0, {.first_gpr = 29});
    Kept<30> club(c, club_address); Kept<29> person(c, person_address); Program program(c);
    program.call(0x103721f0, 0x10081b38, {frame.at(0x38)});
    Kept<31> minimum(c, 10'000'000);
    const auto first = program.call(0x10374f50, 0x10081b64,
                                   {frame.at(0x38), person, club, 0xffffffff, 1, 11, 0, 0});
    c.set(4, 0x990000); c.set(0, 10'000'000);
    if (compare(c, std::int32_t(first), 10'000'000) < 0) minimum = first;
    const auto second = program.call(0x10374f50, 0x10081b9c,
                                    {frame.at(0x38), person, club, 0xffffffff, 2, 11, 0, 0});
    if (compare(c, std::int32_t(second), std::int32_t(std::uint32_t(minimum))) < 0) minimum = second;
    return result(c, minimum);
}
}
