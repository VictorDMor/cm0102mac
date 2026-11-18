// COMP.CPP's calendar: a competition's fixture dates (0x10450ee0), their sort order (0x10451470, the comparator
// the sort uses) and the competition's first date (0x10453190).
//
// The virtual +0x44 of a competition returns an array of "fixture rounds" and, through three out bytes in the
// caller's frame, how to read it: a count (halfword, frame +0x3c in 0x10450ee0), a halfword (frame +0x3a, the
// second out value: a competition number, inferred) and a flag byte (frame +0x38). Flag 0: rows of 0x78 bytes
// (a round of a league: day +4, year offset +6, a kind halfword +0xe whose bit 0x100 says that the eight
// 8-byte slots from +0x2c hold extra dates, a kind byte +0x23, a day count +0x24 ...). Flag nonzero: rows of
// 0x4c bytes (a knockout's rounds: day +0, year offset +2, eight slots from +0xc). Their meaning is inferred.
#include "comp_detail.hpp"
#include "date.hpp"
#include <array>

namespace cm::aot::comp {
namespace {

constexpr std::uint32_t date_new_size = 8, fixture_row = 0x14;
// The competition's virtual that returns the fixture rounds (vtable +0x44, base 0x1044fff0 answers 0).
constexpr std::uint32_t rounds_slot = 0x44;

// The registers r14-r31 as the original holds them at each of its calls: the allocator and the date
// routines can reach imports, where a replay compares the non-volatile registers. `v[n]` is the original's
// rN; it is written where the original writes it, so a register a path has not reached yet (and the stale
// value of an earlier round) is what the original has too.
struct Held {
    Cpu& c;
    std::array<std::uint32_t, 32> v{};
    explicit Held(Cpu& cpu) : c(cpu) { for (unsigned n = 14; n < 32; ++n) v[n] = c.r(n); }
    void publish() const { for (unsigned n = 14; n < 32; ++n) c.set(n, v[n]); }
};

// The builder of 0x10450ee0: the rows array and its count (the original's r31 and r19), and the walk over a
// stage's rounds.
struct Dates {
    Cpu& c;
    const Frame& frame;
    Held& h;
    std::uint32_t competition;
    std::uint32_t& stage = h.v[18];   // the stage, -1 the competition itself
    std::uint32_t& count = h.v[19];   // rows written
    std::uint32_t& rows = h.v[31];    // the new[] of 1000 rows

    Dates(Cpu& cpu, const Frame& f, Held& held, std::uint32_t object) : c(cpu), frame(f), h(held), competition(object) {}

    std::int16_t out_count() const { return c.i16(frame.at(0x3c)); }
    std::int16_t out_number() const { return c.i16(frame.at(0x3a)); }
    std::int16_t year() const { return c.i16(competition + competition::year); }
    // A row: the date's two halfwords, the stage, the round, the number, `extra` (+0x12) and the record's id.
    void put_row(std::uint32_t row, std::uint32_t first_half, std::uint32_t second_half, std::uint32_t round, std::uint32_t extra) {
        c.put16(row + 0xc, first_half);
        c.put16(row + 0xe, second_half);
        c.put32(row + 0, stage);
        c.put32(row + 4, round);
        c.put16(row + 0x10, std::uint16_t(out_number()));
        c.put16(row + 0x12, extra);
        c.put32(row + 8, c.u32(c.u32(competition + competition::record)));
    }
    // Rows of a league's rounds (flag 0): 0x78 bytes each.
    void league_rounds(std::uint32_t fixtures) {
        auto& date = h.v[24];
        auto& row = h.v[20];
        auto& slot_offset = h.v[15];
        auto& slot = h.v[23];
        auto& slot_row = h.v[21];
        auto& tail = h.v[16];
        auto& round = h.v[22];
        auto& offset = h.v[17];
        round = 0;
        offset = 0;
        for (; std::int32_t(round) < out_count(); ++round, offset += 0x78) {
            h.publish();
            date = invoke(c, callee::allocate_object, 0x1045116c, {date_new_size});
            const auto at = fixtures + offset;
            if (date) {
                c.link(0x10451194);
                date::from_day_and_year(c, date, sext16(c.u16(at + 4)), std::uint16_t(std::uint32_t(year()) + sext8(c.u8(at + 6))));
            }
            row = fixtures + offset;
            if (c.i16(row + 0xe) & 0x100) {
                tail = count * fixture_row;
                slot = 0;
                slot_offset = 0;
                do {
                    slot_row = row + slot_offset;
                    const auto kind = std::int8_t(c.u8(slot_row + 0x2e));
                    if (kind == -1) break;
                    if (kind == 3) {
                        h.publish();
                        c.link(0x104511e4);
                        date::nearest_weekday(c, date, sext8(c.u8(slot_row + 0x2c)), frame.at(0x70));
                        const auto word = c.u32(date);
                        const bool same = std::uint16_t(word) == c.u16(frame.at(0x72)) && std::int16_t(word >> 16) == c.i16(frame.at(0x70));
                        if (!same || std::int8_t(c.u8(slot_row + 0x2d)) != std::int8_t(c.u8(row + 7))) {
                            put_row(rows + tail, c.u16(frame.at(0x70)), c.u16(frame.at(0x72)), round, std::uint16_t(c.i16(row + 8)));
                            tail += fixture_row;
                            ++count;
                        }
                    }
                    ++slot;
                    slot_offset += 8;
                } while (std::int32_t(slot) < 8);
            }
            if (c.u8(row + 0x23) == 2) {
                // A two-leg round (inferred): its date, then a second date as many days on as the signed byte at row+0x24
                // says; the two rows carry +3 and +4 as their kinds.
                put_row(rows + count * fixture_row, c.u16(date), c.u16(date + 2), round, std::uint16_t(c.i16(row + 8)) + 3u);
                h.publish();
                c.link(0x104512ec);
                date::add_days(c, frame.at(0x50), date, sext8(c.u8(row + 0x24)));
                // The original copies the result to frame 0x68..0x6c and reads it straight back: only the read values matter.
                put_row(rows + (count + 1) * fixture_row, c.u16(frame.at(0x50)), c.u16(frame.at(0x52)), round,
                        std::uint16_t(c.i16(row + 8)) + 4u);
                count += 2;
            } else {
                put_row(rows + count * fixture_row, c.u16(date), c.u16(date + 2), round, std::uint16_t(c.i16(row + 8)));
                ++count;
            }
            h.publish();
            invoke(c, callee::free_object, 0x10451398, {date});
        }
    }

    // Rows of a knockout's rounds (flag nonzero): 0x4c bytes each.
    void knockout_rounds(std::uint32_t fixtures) {
        auto& date = h.v[15];
        auto& row = h.v[20];
        auto& slot = h.v[16];
        auto& slot_offset = h.v[24];
        auto& slot_row = h.v[21];
        auto& tail = h.v[23];
        auto& offset = h.v[22];
        auto& round = h.v[17];
        auto& next = h.v[27];
        round = 0;
        offset = 0;
        next = count * fixture_row;
        for (; std::int32_t(round) < out_count(); ++round, offset += 0x4c) {
            h.publish();
            date = invoke(c, callee::allocate_object, 0x10450fe4, {date_new_size});
            const auto at = fixtures + offset;
            if (date) {
                c.link(0x10451008);
                date::from_day_and_year(c, date, sext16(c.u16(at)), std::uint16_t(std::uint32_t(year()) + c.u16(at + 2)));
            }
            tail = count * fixture_row;
            row = fixtures + offset;
            slot = 0;
            slot_offset = 0;
            do {
                slot_row = row + slot_offset;
                const auto kind = std::int8_t(c.u8(slot_row + 0xe));
                if (kind == -1) break;
                if (kind == 3) {
                    h.publish();
                    c.link(0x1045104c);
                    date::nearest_weekday(c, date, sext8(c.u8(slot_row + 0xc)), frame.at(0x70));
                    const auto word = c.u32(date);
                    const bool same = std::uint16_t(word) == c.u16(frame.at(0x72)) && std::int16_t(word >> 16) == c.i16(frame.at(0x70));
                    if (!same || std::int8_t(c.u8(slot_row + 0xd)) != std::int8_t(c.u8(row + 4))) {
                        put_row(rows + tail, c.u16(frame.at(0x70)), c.u16(frame.at(0x72)), round, 0);
                        tail += fixture_row;
                        ++count;
                        next += fixture_row;
                    }
                }
                ++slot;
                slot_offset += 8;
            } while (std::int32_t(slot) < 8);
            put_row(rows + next, c.u16(date), c.u16(date + 2), round, 0);
            ++count;
            next += fixture_row;
            h.publish();
            invoke(c, callee::free_object, 0x10451140, {date});
        }
    }
};

}

std::uint32_t fixture_dates(Cpu& c, std::uint32_t competition, std::uint32_t count_out) {
    const Frame frame(c, 0x10450ee0, 0x100, {.first_gpr = 14});
    Held h(c);
    auto& success = h.v[14];
    auto& message = h.v[25];
    auto& pool = h.v[26];
    success = c.toc(toc::success);
    message = c.toc(toc::message);
    pool = c.toc(toc::pool);
    h.v[28] = competition;
    h.v[29] = count_out;
    Dates dates(c, frame, h, competition);
    h.publish();
    c.link(0x10450f0c);
    date::set_default(c, frame.at(0x70));
    h.publish();
    c.link(0x10450f14);
    date::set_default(c, frame.at(0x68));
    dates.count = 0;
    c.put32(count_out, 1000);
    h.publish();
    dates.rows = invoke(c, callee::allocate_block, 0x10450f2c, {c.u32(count_out) * fixture_row});
    if (!dates.rows) {
        h.publish();
        report(c, frame, {0x10450f48, 0x98, 0x60, 0x3dc});
        return 0;
    }
    for (dates.stage = 0xffffffffu; std::int32_t(dates.stage) < c.i32(competition + competition::word_2c); ++dates.stage) {
        h.publish();
        const auto fixtures = virtual_call(c, rounds_slot, 0x10450fb4,
                                           {competition, sext8(dates.stage), frame.at(0x3c), frame.at(0x3a), frame.at(0x38)});
        h.v[30] = fixtures;
        if (!fixtures) continue;
        if (c.i8(frame.at(0x38)) == 0) dates.league_rounds(fixtures);
        else dates.knockout_rounds(fixtures);
        h.publish();
        invoke(c, callee::free_block, 0x104513b4, {fixtures});
    }
    if (dates.count == 0) return 0;  // the 1000-row block stays allocated, as the original leaves it
    c.put32(count_out, dates.count);
    h.publish();
    const auto resized = invoke(c, callee::resize_block, 0x104513e8, {dates.rows, c.u32(count_out) * fixture_row});
    h.v[15] = resized;
    if (!resized) {
        h.publish();
        report(c, frame, {0x10451404, 0x78, 0x58, 0x495});
        return 0;
    }
    h.publish();
    invoke(c, callee::sort, 0x10451458, {resized, c.u32(count_out), fixture_row, c.toc(toc::stop_sort)});
    return resized;
}

std::int32_t compare_fixture_dates(Cpu& c, std::uint32_t left, std::uint32_t right) {
    const Frame frame(c, 0x10451470, 0x50, {.first_gpr = 29});
    if (c.i16(left + 0xe) > c.i16(right + 0xe)) return 1;
    if (c.i16(left + 0xe) < c.i16(right + 0xe)) return -1;
    if (c.i16(left + 0xc) > c.i16(right + 0xc)) return 1;
    if (c.i16(left + 0xc) < c.i16(right + 0xc)) return -1;
    // The competitions' reputations, the larger first (the difference of two sign-extended halfwords).
    const auto records = c.u32(c.toc(toc::records));
    const auto reputation = [&](std::uint32_t row) {
        return std::int32_t(c.i16(records + record::reputation + c.u32(row + 8) * record::size));
    };
    if (const auto difference = reputation(right) - reputation(left)) return difference;
    // The texts, into the shared buffer (r31) and 1000 bytes on (r30); r29 the right row.
    const auto first = c.toc(toc::text), second = first + 1000;
    c.set({{29, right}, {30, second}, {31, first}});
    c.set({{3, 0}, {4, sext16(c.u16(left + 0x10))}, {5, sext16(c.u16(left + 0x12))}, {6, 1}, {7, first}, {8, 1000}});
    c.call(0x104769d0, 0x1045152c);
    c.set({{3, 0}, {4, sext16(c.u16(right + 0x10))}, {5, sext16(c.u16(right + 0x12))}, {6, 1}, {7, second}, {8, 1000}});
    c.call(0x104769d0, 0x10451548);
    invoke(c, callee::strcmp, 0x10451554, {first, second});
    return std::int32_t(c.r(3));
}

void first_date(Cpu& c, std::uint32_t out, std::uint32_t competition) {
    const Frame frame(c, 0x10453190, 0x70, {.first_gpr = 29});
    c.set({{29, competition}, {31, out}});
    c.link(0x104531b8);
    date::set_default(c, frame.at(0x50));
    const auto fixtures = virtual_call(c, rounds_slot, 0x104531d8, {competition, 0xffffffffu, frame.at(0x3a), frame.at(0x3c), frame.at(0x38)});
    c.set(30, fixtures);
    const auto year = c.i16(competition + competition::year);
    if (fixtures) {
        // The date lands in the frame (0x50) first; the original's copies go through its other date locals.
        if (c.i8(frame.at(0x38)) != 0) {
            c.link(0x10453228);
            date::from_day_and_year(c, frame.at(0x48), sext16(c.u16(fixtures)), std::uint16_t(std::uint32_t(year) + c.u16(fixtures + 2)));
            c.put16(frame.at(0x52), c.u16(frame.at(0x4a)));
            c.put16(frame.at(0x50), c.u16(frame.at(0x48)));
            c.put8(frame.at(0x54), c.u8(frame.at(0x4c)));
        } else {
            c.link(0x10453264);
            date::from_day_and_year(c, frame.at(0x40), sext16(c.u16(fixtures + 4)), std::uint16_t(std::uint32_t(year) + sext8(c.u8(fixtures + 6))));
            c.put16(frame.at(0x52), c.u16(frame.at(0x42)));
            c.put16(frame.at(0x50), c.u16(frame.at(0x40)));
            c.put8(frame.at(0x54), c.u8(frame.at(0x44)));
        }
        // delete[] at the original stack pointer with r29 the competition, r30 the fixtures, r31 the output:
        // the BlockMoveData bridge reads 417 bytes of what it leaves (the stack audit).
        invoke(c, callee::free_block, 0x10453284, {fixtures});
    }
    c.put16(out, c.u16(frame.at(0x50)));
    c.put16(out + 2, c.u16(frame.at(0x52)));
    c.put8(out + 4, c.u8(frame.at(0x54)));
}

}
