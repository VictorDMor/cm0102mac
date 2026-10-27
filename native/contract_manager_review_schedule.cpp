#include "contract_manager_random.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
void copy_review_date(Cpu& c, std::uint32_t destination, std::uint32_t source) {
    const auto year = c.u16(source + 2); c.set(0, year); c.put16(destination + 2, year);
    const auto day = c.i16(source); c.set(0, extended(day)); c.put16(destination, extended(day));
    const auto leap = c.u8(source + 4); c.set(0, leap); c.put8(destination + 4, leap);
}
bool review_before(Cpu& c, std::uint32_t first, std::uint32_t second) {
    const auto first_year = c.u16(first + 2), second_year = c.u16(second + 2);
    c.set(3, first_year); c.set(0, second_year);
    const auto years = compare_logical(c, first_year, second_year);
    bool before = years < 0;
    if (!years) {
        const auto first_day = c.i16(first), second_day = c.i16(second);
        c.set(0, extended(first_day)); c.set(4, extended(second_day));
        const auto bits = extended(first_day) ^ extended(second_day);
        c.set(3, std::uint32_t(std::int32_t(bits) >> 1));
        c.s.wr<1>(guest::reg::xer_ca, std::int32_t(bits) < 0 && (bits & 1));
        before = first_day < second_day;
    }
    c.set(0, before); nonzero(c, before); return before;
}
}

void schedule_pending_review(Cpu& c, std::uint32_t manager_address, std::uint32_t person_id, std::uint32_t origin_value) {
    Frame frame(c, 0x10080120, 0x110, {.first_gpr = 18});
    Kept<20> origin(c, origin_value); Kept<18> object(c, manager_address);
    Kept<30> today(c, c.toc(-0x7b80)); Kept<19> person(c, person_id);
    Kept<24> phase(c, c.toc(-0x79ec)); Kept<22> found(c, 0xffffffff);
    Kept<25> transfers(c, c.toc(-0x7b40)); Kept<26> salt(c, c.toc(-0x7b90));
    Kept<21> row(c, 0); Kept<27> table(c, c.toc(-0x7b8c));
    Kept<28> random_end(c, c.toc(-0x7b88)); Kept<29> random_cursor(c, c.toc(-0x7b84));
    Kept<31> staff(c, c.toc(-0x7be0)); Kept<23> index(c, c.u32(object + 0x18));
    const ScheduledRecords rows{c, object}; Program program(c);
    const RandomTable random{c, salt, table, random_end, random_cursor};
    program.call(0x10717890, 0x10080178, {frame.at(0xb4), today, 14});
    auto count = rows.count().get(); c.set(5, std::uint32_t(count));
    const auto limit_day = c.i16(frame.at(0xb4));
    const auto limit_year = c.u16(frame.at(0xb6));
    c.set(4, extended(limit_day)); c.set(3, limit_year);
    const auto initial_order = compare(c, std::int32_t(std::uint32_t(index)), count);
    const auto limit_leap = c.u8(frame.at(0xb8)); c.set(0, limit_leap);
    c.put16(frame.at(0xc4), extended(limit_day)); c.put16(frame.at(0xc6), limit_year); c.put8(frame.at(0xc8), limit_leap);
    if (initial_order < 0) {
        const auto data = rows.data().get(), offset = std::uint32_t(index) * 16;
        c.set(4, data); c.set(3, offset); c.set(0, std::uint32_t(count) - std::uint32_t(index));
        const auto date_word = c.u32(frame.at(0xc4)), flags = c.u32(frame.at(0xc8));
        c.set(5, date_word); row = data + offset; c.set(3, flags);
        auto remaining = std::uint32_t(count) - std::uint32_t(index); c.s.wr<4>(guest::reg::ctr, remaining);
        c.put32(frame.at(0x68), flags);
        for (;;) {
            c.put32(frame.at(0x64), date_word);
            // This loop loads the record's year before the saved cutoff's.
            const auto row_year = c.u16(row + 6); c.set(0, row_year);
            if (review_before(c, frame.at(0x64), row + 4)) break;
            const auto source = c.u8(row + 13); c.set(0, source);
            if (compare(c, source, 3) == 0) {
                const auto id = c.i32(row); c.set(0, std::uint32_t(id));
                if (compare(c, id, std::int32_t(std::uint32_t(person))) == 0) {
                    c.set(0, 0xffffffff); c.put32(row, 0xffffffff); break;
                }
            }
            index = std::uint32_t(index) + 1; row = std::uint32_t(row) + 16;
            c.s.wr<4>(guest::reg::ctr, --remaining); if (!remaining) break;
        }
    }
    const auto added = c.u32(object + 0x1c); count = rows.count().get();
    const auto first_new = std::int32_t(std::uint32_t(count) - added);
    c.set(0, std::uint32_t(first_new)); c.set(5, std::uint32_t(count));
    if (compare(c, std::int32_t(std::uint32_t(index)), first_new) < 0) index = std::uint32_t(first_new);
    if (compare(c, std::int32_t(std::uint32_t(index)), count) < 0) {
        const auto data = rows.data().get(), offset = std::uint32_t(index) * 16;
        c.set(4, data); c.set(3, offset); c.set(0, std::uint32_t(count) - std::uint32_t(index)); row = data + offset;
        auto remaining = std::uint32_t(count) - std::uint32_t(index); c.s.wr<4>(guest::reg::ctr, remaining);
        for (;;) {
            const auto source = c.u8(row + 13); c.set(0, source);
            if (compare(c, source, 3) == 0) {
                const auto id = c.i32(row); c.set(0, std::uint32_t(id));
                if (compare(c, id, std::int32_t(std::uint32_t(person))) == 0) { found = index; break; }
            }
            index = std::uint32_t(index) + 1; row = std::uint32_t(row) + 16;
            c.s.wr<4>(guest::reg::ctr, --remaining); if (!remaining) break;
        }
    }
    if (compare(c, std::int32_t(std::uint32_t(found)), -1) == 0) {
        random.advance(0x100802d8);
        const auto position = random.position(); c.set(4, position); c.set(3, 0x66660000);
        const auto seed = c.u32(salt); c.set(5, seed);
        const auto draw = c.u16(position) + seed; c.set(6, draw);
        const auto high = multiply_high(0x66666667, std::int32_t(draw)), floor = high >> 1;
        const auto quotient = std::uint32_t(floor) + (std::uint32_t(floor) >> 31);
        const auto remainder = draw - quotient * 5;
        c.set(0, remainder + 1); c.s.wr<1>(guest::reg::xer_ca, high < 0 && (std::uint32_t(high) & 1));
        program.call(0x10717890, 0x10080324, {frame.at(0xac), today, extended(std::int16_t(remainder + 1))});
        const auto day = c.i16(frame.at(0xac)); c.set(6, extended(day)); c.set(0, std::uint32_t(person) * Staff::size);
        const auto year = c.u16(frame.at(0xae)); c.set(9, year);
        const auto leap = c.u8(frame.at(0xb0)); c.set(8, leap);
        const auto base = c.u32(staff); c.set(4, base);
        c.put16(frame.at(0xbc), extended(day)); c.put16(frame.at(0xbe), year); c.put8(frame.at(0xc0), leap);
        program.call(0x100808d0, 0x1008035c, {object, base + std::uint32_t(person) * Staff::size, frame.at(0xbc), 2, 3});
        const auto appended = rows.count().get();
        const auto data = rows.data().get();
        c.set(3, std::uint32_t(appended)); c.set(4, data);
        c.set(0, (std::uint32_t(appended) - 1) * 16); row = data + (std::uint32_t(appended) - 1) * 16;
    }
    if (compare(c, std::int32_t(std::uint32_t(origin)), -1) != 0) {
        staff = program.call(0x103eb440, 0x10080384, {c.u32(transfers), origin});
        program.call(0x103f3bf0, 0x1008039c, {c.u32(transfers), c.u32(staff), frame.at(0x39), frame.at(0x38)});
        const auto days = c.u8(frame.at(0x39)), slots = c.u8(frame.at(0x38));
        c.set(0, days * 3); c.set(6, slots); c.set(0, slots + days * 3); person = std::uint32_t(std::int32_t(std::int8_t(slots + days * 3)));
        const auto elapsed = program.call(0x10718130, 0x100803c0, {row + 4, c.u32(today), c.u32(today + 4)});
        const auto current_phase = c.u8(phase), row_phase = c.u8(row + 12);
        auto day_difference = std::int32_t(std::int8_t(elapsed));
        auto phase_difference = std::int32_t(std::int8_t(std::uint32_t(row_phase) - current_phase));
        c.set(4, current_phase); c.set(3, std::uint32_t(day_difference)); c.set(0, std::uint32_t(row_phase) - current_phase);
        c.set(5, std::uint32_t(phase_difference));
        if (compare(c, phase_difference) < 0) { phase_difference += 3; --day_difference; c.set(5, std::uint32_t(phase_difference)); c.set(3, std::uint32_t(day_difference)); }
        const auto available = std::int8_t(std::uint32_t(day_difference) * 3 + std::uint32_t(phase_difference));
        c.set(0, std::uint32_t(day_difference) * 3 + std::uint32_t(phase_difference)); c.set(5, std::uint32_t(std::int32_t(available)));
        const auto requested = std::int8_t(std::uint32_t(person));
        const auto selected = compare(c, available, requested) < 0 ? std::int32_t(available) : std::int32_t(requested);
        c.set(0, 1); const auto bounded = compare(c, selected, 1) < 0 ? 1 : selected;
        c.set(0, std::uint32_t(bounded)); c.set(5, 0x55550000); transfers = std::uint32_t(bounded);
        const auto high = multiply_high(0x55555556, bounded);
        const auto quotient = std::uint32_t(high) + (std::uint32_t(high) >> 31);
        c.set(0, quotient);
        program.call(0x10717890, 0x10080448, {frame.at(0xa4), today, extended(std::int16_t(quotient))});
        const auto year = c.u16(frame.at(0xa6)); c.set(4, year); c.set(3, 0x55550000); c.set(0, 0x55555556); c.put16(row + 6, year);
        const auto actual_high = multiply_high(0x55555556, std::int32_t(std::uint32_t(transfers))); c.set(3, std::uint32_t(actual_high));
        const auto day = c.i16(frame.at(0xa4)); c.set(0, extended(day)); c.put16(row + 4, extended(day));
        const auto leap = c.u8(frame.at(0xa8)); c.set(4, leap); c.put8(row + 8, leap);
        const auto actual_quotient = std::uint32_t(actual_high) + (std::uint32_t(actual_high) >> 31);
        const auto current = c.u8(phase); c.set(3, current);
        const auto new_phase = current + std::uint32_t(transfers) - actual_quotient * 3; c.set(0, new_phase); c.put8(row + 12, new_phase);
        const auto stored_phase = c.u8(row + 12); c.set(3, stored_phase); c.set(0, std::uint32_t(std::int32_t(std::int8_t(stored_phase))));
        if (compare(c, std::int8_t(stored_phase), 2) > 0) {
            c.set(0, std::uint32_t(stored_phase) - 3); c.put8(row + 12, std::uint32_t(stored_phase) - 3);
            program.call(0x10717890, 0x100804b0, {frame.at(0x9c), row + 4, 1}); copy_review_date(c, row + 4, frame.at(0x9c));
        }
        const auto transfer = c.u32(staff); c.set(3, transfer);
        const auto urgent = c.i8(transfer + 0x37); c.set(0, std::uint32_t(std::int32_t(urgent)));
        if (nonzero(c, urgent)) {
            const auto days_away = program.call(0x10718130, 0x100804e8, {row + 4, c.u32(today), c.u32(today + 4)});
            if (compare(c, std::int32_t(days_away), 4) < 0) {
                random.advance(0x10080510);
                const auto draw = random.value(); c.set(0, draw);
                const auto negative = draw >> 31, remainder = (draw & 1) ? 1u - negative * 2 : 0u;
                c.set(0, remainder + 4);
                program.call(0x10717890, 0x1008054c, {frame.at(0x94), today, extended(std::int16_t(remainder + 4))});
                copy_review_date(c, row + 4, frame.at(0x94));
            }
        }
    } else {
        const auto date = c.u32(today), flags = c.u32(today + 4);
        c.set(5, date); c.set(4, 0); c.set(3, flags); c.put32(frame.at(0x5c), date);
        const auto row_year = c.u16(row + 6); c.set(6, row_year);
        const auto current_year = c.u16(frame.at(0x5e)); c.set(0, current_year);
        c.put32(frame.at(0x3c), date); const bool same_year = compare_logical(c, current_year, row_year) == 0;
        c.put32(frame.at(0x40), flags); c.put32(frame.at(0x60), flags);
        bool same = false;
        if (same_year) {
            const auto current_day = c.i16(frame.at(0x5c)), row_day = c.i16(row + 4);
            c.set(3, extended(current_day)); c.set(0, extended(row_day)); same = compare(c, current_day, row_day) == 0;
            if (same) c.set(4, 1);
        }
        bool refresh = nonzero(c, same);
        if (!refresh) {
            const auto saved_date = c.u32(frame.at(0x3c)), saved_flags = c.u32(frame.at(0x40));
            c.set(3, saved_date); c.set(0, saved_flags); c.put32(frame.at(0x54), saved_date);
            const auto year = c.u16(frame.at(0x56)); c.set(3, year); c.put32(frame.at(0x58), saved_flags);
            const auto years = compare_logical(c, year, row_year); refresh = years > 0;
            if (!years) {
                const auto current_day = c.i16(frame.at(0x54)), row_day = c.i16(row + 4);
                c.set(4, extended(current_day)); c.set(0, extended(row_day));
                const auto bits = extended(current_day) ^ extended(row_day); c.set(3, std::uint32_t(std::int32_t(bits) >> 1));
                c.s.wr<1>(guest::reg::xer_ca, std::int32_t(bits) < 0 && (bits & 1)); refresh = current_day > row_day;
            }
            c.set(0, refresh); nonzero(c, refresh);
        }
        if (refresh) {
            copy_review_date(c, row + 4, today);
            const auto row_phase = c.u8(row + 12), current = c.u8(phase);
            c.set(4, row_phase); c.set(0, current); c.set(3, std::uint32_t(std::int32_t(std::int8_t(row_phase))));
            if (compare(c, std::int8_t(row_phase), current) <= 0) {
                c.set(0, std::uint32_t(row_phase) + 1); c.put8(row + 12, std::uint32_t(row_phase) + 1);
                const auto incremented = c.u8(row + 12); c.set(3, incremented); c.set(0, std::uint32_t(std::int32_t(std::int8_t(incremented))));
                if (compare(c, std::int8_t(incremented), 2) > 0) {
                    c.set(0, std::uint32_t(incremented) - 3); c.put8(row + 12, std::uint32_t(incremented) - 3);
                    program.call(0x10717890, 0x10080664, {frame.at(0x8c), row + 4, 1}); copy_review_date(c, row + 4, frame.at(0x8c));
                }
            }
        }
        person = std::uint32_t(person) * Staff::size;
        const auto base = c.u32(staff); c.set(0, base);
        const auto contract = program.call(0x10082f20, 0x10080690, {object, base + std::uint32_t(person)});
        if (nonzero_logical(c, contract)) {
            program.call(0x10717890, 0x100806a8, {frame.at(0x84), today, 180});
            const auto limit = c.u32(frame.at(0x84)), limit_flags = c.u32(frame.at(0x88));
            c.set(4, limit); c.set(5, limit_flags); const auto current_base = c.u32(staff); c.set(0, current_base);
            c.put32(frame.at(0x4c), limit); c.put32(frame.at(0x50), limit_flags);
            const auto repeated = program.call(0x10082f20, 0x100806c8, {object, current_base + std::uint32_t(person)});
            const auto limit_year = c.u16(frame.at(0x4e)), expiry_year = c.u16(repeated + 0x32);
            c.set(4, limit_year); c.set(0, expiry_year); const auto years = compare_logical(c, limit_year, expiry_year);
            bool expires = years > 0;
            if (!years) {
                const auto limit_day = c.i16(frame.at(0x4c)), expiry_day = c.i16(repeated + 0x30);
                c.set(4, extended(limit_day)); c.set(0, extended(expiry_day));
                const auto bits = extended(limit_day) ^ extended(expiry_day); c.set(3, std::uint32_t(std::int32_t(bits) >> 1));
                c.s.wr<1>(guest::reg::xer_ca, std::int32_t(bits) < 0 && (bits & 1)); expires = limit_day > expiry_day;
            }
            c.set(0, expires);
            if (nonzero(c, expires)) {
                random.advance(0x10080734);
                const auto draw = random.value(); c.set(5, draw);
                const auto remainder = std::uint32_t(std::int32_t(draw) % 4); c.set(0, remainder + 4);
                program.call(0x10717890, 0x10080774, {frame.at(0x7c), today, extended(std::int16_t(remainder + 4))});
                copy_review_date(c, row + 4, frame.at(0x7c));
            }
        }
    }
    const auto today_word = c.u32(today), today_flags = c.u32(today + 4);
    c.set(4, today_word); c.set(0, today_flags); c.put32(frame.at(0x44), today_word); c.put32(frame.at(0x48), today_flags);
    program.call(0x10717b20, 0x100807ac, {frame.at(0x74), row + 4, 14});
    if (review_before(c, frame.at(0x44), frame.at(0x74))) {
        program.call(0x10717890, 0x10080808, {frame.at(0x6c), today, 14}); copy_review_date(c, row + 4, frame.at(0x6c));
    }
    const auto added_count = c.u32(object + 0x1c); c.set(3, added_count); c.set(0, added_count + 1); c.put32(object + 0x1c, added_count + 1);
}
}
