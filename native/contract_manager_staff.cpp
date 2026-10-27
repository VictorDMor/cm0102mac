#include "contract_manager_random.hpp"

namespace cm::aot::contract_manager {
using namespace detail;
namespace {
void copy_date(Cpu& c, Date to, Date from) {
    const auto year = from.year().get(); c.set(0, year); to.year() = year;
    const auto day = from.day().get(); c.set(0, extended(day)); to.day() = day;
    const auto leap = from.leap().get(); c.set(0, leap); to.leap() = leap;
}

bool default_expiry(Cpu& c, Program& program, const Frame& frame, Date expiry,
                     unsigned made, unsigned copied, std::uint32_t back) {
    const auto source = program.call(0x107156b0, back, {frame.at(made), 31, 1900});
    const auto first = c.u32(source); c.set(0, first); c.set(4, 0);
    const auto second = c.u32(source + 4); c.set(5, second);
    c.put32(frame.at(copied), first);
    const auto year = expiry.year().get(); c.set(0, year);
    const auto default_year = c.u16(frame.at(copied + 2)); c.set(3, default_year);
    c.put32(frame.at(copied + 4), second);
    bool same = compare_logical(c, default_year, year) == 0;
    if (same) {
        const auto default_day = c.i16(frame.at(copied)), day = expiry.day().get();
        c.set(3, extended(default_day)); c.set(0, extended(day));
        same = compare(c, default_day, day) == 0;
    }
    c.set(4, same);
    return nonzero(c, same);
}

// Whether the minimum date (eight weeks after the game start) is later
// than the staff member's expiry. The copied words are original locals.
bool before_minimum(Cpu& c, Program& program, const Frame& frame, Date expiry, std::uint32_t start) {
    program.call(0x10717a10, 0x100786c0, {frame.at(0x60), start, 8});
    const auto first = c.u32(frame.at(0x60)), second = c.u32(frame.at(0x64));
    c.set(3, first); c.set(0, second); c.put32(frame.at(0x40), first);
    const auto year = expiry.year().get(), minimum_year = c.u16(frame.at(0x42));
    c.set(3, year); c.set(4, minimum_year); c.put32(frame.at(0x44), second);
    const auto order = compare_logical(c, minimum_year, year);
    bool later = order > 0;
    if (!order) {
        const auto minimum_day = c.i16(frame.at(0x40)), day = expiry.day().get();
        c.set(4, extended(minimum_day)); c.set(0, extended(day));
        const auto bits = extended(minimum_day) ^ extended(day);
        c.set(3, std::uint32_t(std::int32_t(bits) >> 1));
        c.s.wr<1>(guest::reg::xer_ca, std::int32_t(bits) < 0 && (bits & 1));
        later = minimum_day > day;
    }
    c.set(0, later);
    return nonzero(c, later);
}
}

void initialize_staff_contracts(Cpu& c, std::uint32_t manager_address) {
    Frame frame(c, 0x10078400, 0x150, {.first_gpr = 17});
    Kept<23> start(c, c.toc(-0x7a9c));
    Kept<31> object(c, manager_address);
    Kept<24> salt(c, c.toc(-0x7b90));
    Kept<25> table(c, c.toc(-0x7b8c));
    Kept<26> end(c, c.toc(-0x7b88));
    Kept<27> cursor(c, c.toc(-0x7b84));
    Kept<28> indices(c, c.toc(slot::contract_index));
    const auto message = c.toc(-0x79e0), title = c.toc(-0x79dc), error_buffer = c.toc(-0x7ba8);
    keep(c, {{20, message}, {19, title}, {18, error_buffer}});
    Kept<29> person_count(c, c.toc(slot::person_count));
    Kept<30> today(c, c.toc(-0x7b80));
    const auto pool = c.toc(slot::pool); keep(c, {{17, pool}});
    Program program(c);
    const Manager manager{c, object};
    const RandomTable random{c, salt, table, end, cursor};
    program.call(0x103721f0, 0x1007844c, {frame.at(0xa8)});
    const auto people = c.i32(person_count); c.set(0, std::uint32_t(people));
    manager.capacity() = people;
    const auto capacity = manager.capacity().get(); c.set(0, std::uint32_t(capacity));
    manager.records() = program.call(0x10775130, 0x10078460, {std::uint32_t(capacity) * Contract::size});
    const auto records = manager.records().get(); c.set(0, records);
    if (!nonzero_logical(c, records)) {
        program.call(0x10739890, 0x10078488, {pool + 0x37, 0, 0, frame.at(0x88), frame.at(0x80)});
        const auto version = program.call(0x10768810, 0x1007848c);
        program.call(0x1077b720, 0x100784ac,
                     {error_buffer, pool + 0x4c, version, frame.at(0x88), frame.at(0x80), 0xd4, pool + 0x5e});
        program.call(0x10727890, 0x100784b8, {title, pool + 0xb8});
        program.call(0x10727890, 0x100784c8, {message, pool + 0xd0, error_buffer});
        program.call(0x10769b60, 0x100784d8, {title, message, 0});
        return;
    }
    Kept<20> row_offset(c, 0);
    const auto staff_holder = c.toc(-0x7be0); c.set(3, staff_holder);
    manager.count() = 0;
    Kept<19> person_index(c, 0);
    Kept<18> person(c, c.u32(staff_holder));
    for (;;) {
        const auto count = c.i32(person_count); c.set(0, std::uint32_t(count));
        if (compare(c, std::int32_t(std::uint32_t(person_index)), count) >= 0) break;
        const Staff staff{c, person};
        const auto index_base = c.u32(indices), flags_at = index_base + std::uint32_t(row_offset) + 12;
        c.set(7, index_base); c.set(5, std::uint32_t(row_offset) + 12); c.set(4, 0);
        const auto flags = c.u8(flags_at) & 0xfeu; c.set(0, flags); c.put8(flags_at, flags);
        const auto club = staff.club().get();
        staff.value() = program.call(0x10084be0, 0x10078528, {object, person, club, 1, 0xffffffff, 0});
        Kept<17> primary_club(c, staff.club().get());
        if (nonzero_logical(c, primary_club)) {
            if (default_expiry(c, program, frame, staff.expiry(), 0x78, 0x48, 0x10078548)) {
                Kept<22> current_year(c, Date{c, today}.year().get());
                random.advance(0x100785ac);
                const auto day_cursor = random.position(), current_salt = c.u32(salt);
                const auto entry = c.u16(day_cursor);
                const auto draw = std::uint32_t(entry) + current_salt;
                const auto high = multiply_high(std::int32_t(0x92492493), std::int32_t(draw));
                const auto floor = std::int32_t(std::uint32_t(high) + draw) >> 4;
                const auto sign = std::uint32_t(floor) >> 31;
                const auto quotient = std::uint32_t(floor) + sign;
                const auto next = day_cursor + 2, last = c.u32(end);
                c.set({{7, day_cursor}, {5, draw}, {6, entry}, {3, next}, {0, last}});
                c.put32(cursor, next);
                const bool wraps = compare_logical(c, next, last) > 0;
                c.set(4, std::uint32_t(high)); c.set(3, sign); c.set(0, quotient * 28);
                Kept<21> day_index(c, draw - quotient * 28);
                c.s.wr<1>(guest::reg::xer_ca, std::int32_t(std::uint32_t(high) + draw) < 0 &&
                         ((std::uint32_t(high) + draw) & 15));
                if (wraps) {
                    c.put32(cursor, table);
                    const auto entropy = program.call(0x1077b770, 0x10078604);
                    c.set(0, entropy & 0xffff); c.put32(salt, entropy & 0xffff);
                }
                const auto year_salt = c.u32(salt);
                const auto year_entry = c.u16(random.position());
                const auto year_draw = std::uint32_t(year_entry) + year_salt;
                const auto year_high = multiply_high(0x2aaaaaab, std::int32_t(year_draw));
                const auto year_quotient = std::uint32_t(year_high) + (std::uint32_t(year_high) >> 31);
                const auto year_index = year_draw - year_quotient * 6;
                const auto calendar_owner = std::uint32_t(primary_club);
                Kept<17> refinement_years(c, year_index);
                c.set(0, year_quotient * 6); c.set(5, std::uint32_t(year_high)); c.set(6, year_draw);
                const auto month = program.call(0x10089e40, 0x10078644, {object, calendar_owner});
                const auto year = std::uint32_t(current_year) + std::uint32_t(refinement_years) + 1;
                const auto day = std::int8_t(std::uint32_t(day_index) + 1);
                c.set(0, year);
                program.call(0x107156f0, 0x10078668,
                             {frame.at(0x70), std::uint32_t(std::int32_t(day)), month, year & 0xffff, 0xffffffff});
                copy_date(c, staff.expiry(), {c, frame.at(0x70)});
            }
            while (before_minimum(c, program, frame, staff.expiry(), start)) {
                program.call(0x10717a10, 0x10078698, {frame.at(0x68), staff.expiry().address, 52});
                copy_date(c, staff.expiry(), {c, frame.at(0x68)});
            }
        }
        const auto secondary = staff.secondary_club().get(); c.set(0, secondary);
        if (nonzero_logical(c, secondary) &&
            default_expiry(c, program, frame, staff.secondary_expiry(), 0x58, 0x38, 0x10078738)) {
            Kept<21> current_year(c, Date{c, today}.year().get());
            random.advance(0x1007879c);
            const auto year_cursor = random.position(), year_salt = c.u32(salt);
            const auto year_entry = c.u16(year_cursor);
            const auto draw = std::uint32_t(year_entry) + year_salt;
            const auto high = multiply_high(0x66666667, std::int32_t(draw));
            const auto floor = high >> 1;
            const auto sign = std::uint32_t(floor) >> 31;
            const auto delta = draw - (std::uint32_t(floor) + sign) * 5;
            const auto year = std::uint32_t(current_year) + delta + 1;
            c.set(8, draw); c.set(0, year);
            c.s.wr<1>(guest::reg::xer_ca, high < 0 && (std::uint32_t(high) & 1));
            program.call(0x107156f0, 0x100787f4, {frame.at(0x50), 10, 5, year & 0xffff, 0xffffffff});
            copy_date(c, staff.secondary_expiry(), {c, frame.at(0x50)});
        }
        const auto primary = staff.club().get(); c.set(0, primary);
        if (nonzero_logical(c, primary)) {
            const auto kind = c.u8(person + 0x62);
            const auto eligible = std::int8_t(program.call(0x1007e7b0, 0x10078824, {object, kind}));
            c.set(0, std::uint32_t(std::int32_t(eligible)));
            if (nonzero(c, eligible)) {
                program.call(0x10078e70, 0x1007883c, {object, person, staff.value().get()});
                const auto attributes = staff.attributes().get(); c.set(3, attributes);
                if (nonzero_logical(c, attributes)) {
                    const auto rating = c.i8(attributes + 14); c.set(0, std::uint32_t(std::int32_t(rating)));
                    std::int32_t clamped = 1;
                    if (compare(c, rating, 1) >= 0) {
                        const auto order = compare(c, rating, 50); c.set(3, 50);
                        clamped = order > 0 ? 50 : rating;
                    }
                    c.set(3, std::uint32_t(clamped)); c.set(0, 0u - std::uint32_t(clamped));
                    const Kept<17> adjustment(c, std::uint32_t(std::int32_t(std::int8_t(-clamped))));
                    const auto contract = program.call(0x10082f20, 0x10078884, {object, person});
                    c.put8(contract + 0x40, adjustment);
                }
            }
        } else {
            const auto secondary_club = staff.secondary_club().get(); c.set(0, secondary_club);
            if (nonzero_logical(c, secondary_club)) {
                const auto kind = c.u8(person + 0x60);
                const auto eligible = std::int8_t(program.call(0x1007e7f0, 0x100788a4, {object, kind}));
                c.set(0, std::uint32_t(std::int32_t(eligible)));
                if (nonzero(c, eligible))
                    program.call(0x10078e70, 0x100788bc, {object, person, staff.value().get()});
            }
        }
        row_offset = std::uint32_t(row_offset) + Contract::size;
        person_index = std::uint32_t(person_index) + 1;
        person = std::uint32_t(person) + Staff::size;
    }
}
}
