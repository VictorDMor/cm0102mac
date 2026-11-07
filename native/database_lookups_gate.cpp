// DATABASE.CPP's lookups block, part 3: the person/club gate over nation sets, the
// contract manager and the date, its classes and the value picked by class
// (database_lookups.hpp).
#include "database_lookups.hpp"
#include "database_lookups_detail.hpp"
#include "database.hpp"

namespace cm::aot::database {
namespace {
using namespace lookups;
using guest::compare;
using guest::compare_logical;
using guest::result;

// srawi by 1 sets XER[CA] when a negative value shifts out a 1 bit.
std::uint32_t shift_right_1(Cpu& c, std::uint32_t value) {
    c.s.wr<1>(guest::reg::xer_ca, std::uint8_t((value >> 31) & value & 1));
    return std::uint32_t(std::int32_t(value) >> 1);
}
// One test of the nation chains: the pointer of the TOC slot into `slot_register`,
// the record's nation word into `value_register`, the slot's word into r0, then
// cmpw into cr0. (The original may have loaded a record pointer into the value
// register first; that value is overwritten at once.)
bool nation_matches(Cpu& c, unsigned slot_register, unsigned value_register, std::uint32_t record,
                    std::int32_t slot_offset) {
    const auto at = slot(c, slot_register, slot_offset);
    const auto nation = load32(c, value_register, record + club_record::nation);
    const auto word = load32(c, 0, at);
    return compare(c, std::int32_t(nation), std::int32_t(word)) == 0;
}
// The same where the slot pointer is already in a register (cab10 loads seven at
// its start).
// `word_first` is the order of the two loads (they differ between the chains).
bool nation_matches_at(Cpu& c, unsigned value_register, std::uint32_t record, std::uint32_t word_address,
                       bool word_first) {
    std::uint32_t word = 0;
    if (word_first) word = load32(c, 0, word_address);
    const auto nation = load32(c, value_register, record + club_record::nation);
    if (!word_first) word = load32(c, 0, word_address);
    return compare(c, std::int32_t(nation), std::int32_t(word)) == 0;
}
}

// 0x100ca6a0 (a leaf). True when
//  - the person's first club (+0xc) has status 2 or the nation at TOC -0x74f4, or the
//    second club (+0x10) has status 2;
//  - the club given has the nation at -0x7af4 and the first club's nation record
//    (+0x74) is the one of nation -0x7a70 in the 0xc8-byte rows (-0x7520);
//  - the club's nation is -0x7a14;
//  - the club's nation is -0x750c and the first club's is -0x75c4, -0x7504 or -0x75c8.
bool person_exempt_from_club_rule(Cpu& c, std::uint32_t person, std::uint32_t club) {
    const auto yes = [&] { return result(c, 1) != 0; };
    const auto no = [&] { return result(c, 0) != 0; };
    const auto first_club = [&](unsigned r) { return load32(c, r, person + holder::first_club); };
    const auto status = load8(c, 0, first_club(5) + club_record::status);
    if (compare(c, std::int32_t(status), 2) == 0) return yes();
    first_club(6);
    if (nation_matches(c, 5, 6, c.r(6), -0x74f4)) return yes();
    const auto second_club = load32(c, 0, person + holder::second_club);
    if (compare_logical(c, second_club) != 0) {
        const auto second_status = load8(c, 0, load32(c, 5, person + holder::second_club) + club_record::status);
        if (compare(c, std::int32_t(second_status), 2) == 0) return yes();
    }
    if (compare_logical(c, club) != 0 && nation_matches(c, 5, 6, club, -0x7af4)) {
        const auto id_at = slot(c, 5, -0x7a70);
        const auto rows_at = slot(c, 6, -0x7520);
        const auto id = load32(c, 0, id_at);
        const auto first = first_club(7);
        c.set(0, id * 0xc8);
        const auto rows = load32(c, 5, rows_at);
        const auto record = load32(c, 6, first + club_record::nation_record);
        c.set(0, rows + c.r(0));
        if (compare_logical(c, record, c.r(0)) == 0) return yes();
    }
    if (compare_logical(c, club) != 0 && nation_matches(c, 5, 6, club, -0x7a14)) return yes();
    if (compare_logical(c, club) == 0) return no();
    if (!nation_matches(c, 5, 4, club, -0x750c)) return no();
    first_club(5);
    if (nation_matches(c, 4, 5, c.r(5), -0x75c4)) return yes();
    first_club(5);
    if (nation_matches(c, 4, 5, c.r(5), -0x7504)) return yes();
    first_club(4);
    return nation_matches(c, 3, 4, c.r(4), -0x75c8) ? yes() : no();
}

// 0x100cab10: a frame of 0x50 saving r31 (the person). Seven TOC pointers go to r3
// (-0x7a10), r6 (-0x7a1c), r7 (-0x7574), r8 (-0x7578), r9 (-0x757c), r10 (-0x7650)
// and r11 (-0x7654) and the chains below compare nation words with them. The end
// calls 0x100ca6a0, the contract manager's primary contract of the person and the
// date. False when the club is one of the person's clubs or a nation chain excludes
// the pair, or when the primary contract's date field has not been reached.
bool person_club_gate(Cpu& c, std::uint32_t person_address, std::uint32_t club, std::uint32_t flag) {
    constexpr std::uint32_t entry = 0x100cab10;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x50, {.first_gpr = 31});
    guest::Kept<31> person(c, person_address);
    compare_logical(c, club);
    const auto slot_11 = slot(c, 11, -0x7654), slot_10 = slot(c, 10, -0x7650), slot_9 = slot(c, 9, -0x757c),
               slot_8 = slot(c, 8, -0x7578), slot_7 = slot(c, 7, -0x7574), slot_6 = slot(c, 6, -0x7a1c),
               slot_3 = slot(c, 3, -0x7a10);
    const auto yes = [&] { return result(c, 1) != 0; };
    const auto no = [&] { return result(c, 0) != 0; };
    if (club == 0) return yes();
    const auto first_club = load32(c, 0, person + holder::first_club);
    if (compare_logical(c, first_club, club) == 0) return no();
    const auto second_club = load32(c, 0, person + holder::second_club);
    if (compare_logical(c, second_club, club) == 0) return no();
    c.set(0, sext8(flag));
    if (compare(c, std::int32_t(sext8(flag))) == 0) return yes();
    const auto club_nation_in = [&](std::initializer_list<std::uint32_t> words) {
        for (const auto word : words)
            if (nation_matches_at(c, 5, club, word, false)) return true;
        return false;
    };
    if (club_nation_in({slot_3, slot_6, slot_9, slot_8, slot_7})) {
        const std::initializer_list<std::uint32_t> words{slot_3, slot_6, slot_9, slot_7, slot_8};
        // The first club's nation, reloaded into r5 each time.
        for (const auto word : words) {
            load32(c, 5, person + holder::first_club);
            if (nation_matches_at(c, 5, c.r(5), word, true)) return no();
        }
        const auto second = load32(c, 0, person + holder::second_club);
        if (compare_logical(c, second) != 0) {
            // The first link keeps the slot pointer in r3 until the nation overwrites it.
            bool first_link = true;
            for (const auto word : words) {
                const auto record = load32(c, first_link ? 5 : 3, person + holder::second_club);
                const auto listed = load32(c, 0, word);
                const auto nation = load32(c, 3, record + club_record::nation);
                first_link = false;
                if (compare(c, std::int32_t(nation), std::int32_t(listed)) == 0) return no();
            }
        }
    }
    if (compare_logical(c, load32(c, 0, person + holder::first_club)) != 0) {
        // The club's nation is read before the word, the first club's after it.
        const auto club_equals = [&](std::uint32_t word_address) {
            const auto nation = load32(c, 3, club + club_record::nation);
            const auto word = load32(c, 0, word_address);
            return compare(c, std::int32_t(nation), std::int32_t(word)) == 0;
        };
        const auto first_club_equals = [&](std::uint32_t word_address) {
            const auto record = load32(c, 3, person + holder::first_club);
            const auto word = load32(c, 0, word_address);
            const auto nation = load32(c, 3, record + club_record::nation);
            return compare(c, std::int32_t(nation), std::int32_t(word)) == 0;
        };
        if (club_equals(slot_10) || club_equals(slot_11)) {
            if (first_club_equals(slot_10)) return no();
            if (first_club_equals(slot_11)) return no();
        }
    }
    {
        const auto at = slot(c, 3, -0x7a14);
        const auto nation = load32(c, 5, club + club_record::nation);
        const auto word = load32(c, 0, at);
        if (compare(c, std::int32_t(nation), std::int32_t(word)) == 0) {
            const auto first_club_again = load32(c, 0, person + holder::first_club);
            if (compare_logical(c, first_club_again) != 0) {
                load32(c, 5, person + holder::first_club);
                if (nation_matches(c, 3, 5, c.r(5), -0x7658)) return no();
            }
        }
    }
    {
        const auto at = slot(c, 3, -0x79fc);
        const auto nation = load32(c, 4, club + club_record::nation);
        const auto word = load32(c, 0, at);
        if (compare(c, std::int32_t(nation), std::int32_t(word)) != 0) return yes();
    }
    guest::enter(c, entry + 0x260, {person, 0});
    const auto exempt = person_exempt_from_club_rule(c, person, 0);
    c.set(0, sext8(exempt));
    if (compare(c, std::int32_t(sext8(exempt))) != 0) return yes();
    c.set(3, c.toc(manager_slot));
    c.set(4, person);
    c.set(3, c.u32(c.r(3)));
    c.call_in_place(0x10082f20, entry + 0x278);
    person = c.r(3);  // or. r31,r3,r3
    if (compare(c, std::int32_t(c.r(3))) == 0) return yes();
    c.set({{3, frame.at(0x40)}, {4, 1}, {5, 0}, {6, 0x7ce}, {7, 0xffffffff}});
    c.call_in_place(0x107156f0, entry + 0x298);
    const auto today_high = load32(c, 5, c.r(3));
    const auto today_low = load32(c, 4, c.r(3) + 4);
    c.put32(frame.at(0x38), today_high);
    const auto contract_year = load16(c, 0, person + 0x2a);
    const auto today_year = load16(c, 3, frame.at(0x3a));
    c.put32(frame.at(0x3c), today_low);
    std::uint32_t after;
    if (compare_logical(c, today_year, contract_year) > 0) {
        after = 1;
        c.set(0, 1);
    } else {
        load16(c, 3, frame.at(0x3a));
        load16(c, 0, person + 0x2a);
        if (compare_logical(c, today_year, contract_year) < 0) {
            after = 0;
            c.set(0, 0);
        } else {
            const auto today_signed = load16s(c, 4, frame.at(0x38));
            const auto contract_signed = load16s(c, 0, person + 0x28);
            const auto x = std::uint32_t(today_signed) ^ std::uint32_t(contract_signed);
            c.set(0, x);
            c.set(3, shift_right_1(c, x));
            c.set(0, x & std::uint32_t(today_signed));
            c.set(0, c.r(3) - c.r(0));
            c.set(0, (c.r(0) >> 31) & 1);
            c.set(0, sext8(c.r(0)));
            after = c.r(0);
        }
    }
    c.set(0, sext8(after));
    return compare(c, std::int32_t(sext8(after))) == 0 ? yes() : no();
}

// 0x100ce0a0: a frame of 0x40 saving r30 (the person) and r31 (the other). Classes:
// 4 without an other; 0 when both have the same +0x24 record or their +0x24 records
// share the +0x2c record; 2 when the first clubs (+0xc) are the same or the gate
// (0x100cab10, flag 1) refuses the other for the person's first club; 3 when it
// refuses the other for the person's +0x24 record's +0x2c record, or when the other's
// first word is below the club count - 16, the person has a player and the other a
// +0x24 record whose rating plus the player's +8 value reaches 0x36b0; else 4.
std::uint32_t person_club_class(Cpu& c, std::uint32_t person_address, std::uint32_t other_address) {
    constexpr std::uint32_t entry = 0x100ce0a0;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x40, {.first_gpr = 30});
    guest::Kept<31> other(c, other_address);
    guest::Kept<30> person(c, person_address);
    const auto done = [&](std::uint32_t value) { return result(c, value); };
    if (compare(c, std::int32_t(other)) == 0) return done(4);  // or. r31,r4,r4 (r0 keeps LR)
    const auto person_record = load32(c, 0, person + holder::current_club);
    if (compare_logical(c, person_record) != 0) {
        const auto first = load32(c, 3, person + holder::current_club);
        const auto second = load32(c, 0, other + holder::current_club);
        if (compare_logical(c, first, second) == 0) return done(0);
    }
    const auto person_current = load32(c, 0, person + holder::current_club);
    if (compare_logical(c, person_current) != 0) {
        const auto other_current = load32(c, 0, other + holder::current_club);
        if (compare_logical(c, other_current) != 0) {
            const auto first = load32(c, 4, person + holder::current_club);
            const auto second = load32(c, 3, other + holder::current_club);
            const auto linked_first = load32(c, 4, first + holder::linked);
            const auto linked_second = load32(c, 0, second + holder::linked);
            if (compare_logical(c, linked_first, linked_second) == 0) return done(0);
        }
    }
    const auto person_club = load32(c, 3, person + holder::first_club);
    const auto other_club = load32(c, 0, other + holder::first_club);
    if (compare_logical(c, person_club, other_club) == 0) return done(2);
    {
        const auto club = load32(c, 4, person + holder::first_club);
        guest::enter(c, entry + 0xac, {other, club, 1});
        const auto gate = person_club_gate(c, other, club, 1);
        c.set(0, sext8(gate));
        if (compare(c, std::int32_t(sext8(gate))) == 0) return done(2);
    }
    const auto current = load32(c, 0, person + holder::current_club);
    if (compare_logical(c, current) != 0) {
        const auto club = load32(c, 4, person + holder::current_club);
        const auto linked = load32(c, 4, club + holder::linked);
        guest::enter(c, entry + 0xdc, {other, linked, 1});
        const auto gate = person_club_gate(c, other, linked, 1);
        c.set(0, sext8(gate));
        if (compare(c, std::int32_t(sext8(gate))) == 0) return done(3);
    }
    const auto count_at = slot(c, 3, link_rows::count_slot);
    const auto index = load32(c, 4, other + club_record::id);
    const auto count = load32(c, 3, count_at);
    c.set(0, count - 0x10);
    if (compare(c, std::int32_t(index), std::int32_t(count - 0x10)) < 0) {
        const auto player = load32(c, 0, person + database::person::player);
        if (compare_logical(c, player) != 0) {
            const auto other_current_again = load32(c, 0, other + holder::current_club);
            if (compare_logical(c, other_current_again) != 0) {
                const auto club = load32(c, 4, other + holder::current_club);
                const auto record = load32(c, 3, person + database::person::player);
                const auto rating = load16s(c, 4, club + club_record::rating);
                const auto value = load16s(c, 0, record + 8);
                const auto total = std::uint32_t(rating) + std::uint32_t(value);
                c.set(0, total);
                if (compare(c, std::int32_t(total), 0x36b0) >= 0) return done(3);
            }
        }
    }
    return done(4);
}

// 0x100cdef0: a frame of 0x40 saving r31 (the person). With a flag (a signed byte,
// tested record-form) the 16-bit values come from the record at +0x1c (+0x2c, +0x28,
// +0x2a), without it from the player record at +0x14 (+0xc, +0x8, +0xa): by class
// 4, 2 and 0 the value itself, by class 3 the average of the first two (halves
// rounded to zero before the sum), else of the last two.
std::int32_t person_club_class_value(Cpu& c, std::uint32_t person_address, std::uint32_t other, std::uint32_t flag) {
    constexpr std::uint32_t entry = 0x100cdef0;
    c.set(0, c.lr());
    guest::Frame frame(c, entry, 0x40, {.first_gpr = 31});
    guest::Kept<31> person(c, person_address);
    c.set(0, sext8(flag));
    const bool with_flag = compare(c, std::int32_t(sext8(flag))) != 0;
    const auto record_offset = with_flag ? holder::score : database::person::player;
    const std::uint32_t at_4 = with_flag ? 0x2c : 0xc, at_2 = with_flag ? 0x28 : 0x8, at_0 = with_flag ? 0x2a : 0xa;
    guest::enter(c, entry + (with_flag ? 0x24 : 0xe4), {person});
    const auto cls = person_club_class(c, person, other);
    const auto average = [&](std::uint32_t first_offset, std::uint32_t second_offset) {
        const auto first_record = load32(c, 4, person + record_offset);
        const auto second_record = load32(c, 3, person + record_offset);
        const auto first = load16s(c, 4, first_record + first_offset);
        const auto second = load16s(c, 3, second_record + second_offset);
        c.set(0, std::uint32_t(first) >> 31);
        c.set(4, c.r(0) + std::uint32_t(first));
        c.set(0, std::uint32_t(second) >> 31);
        c.set(4, shift_right_1(c, c.r(4)));
        c.set(0, c.r(0) + std::uint32_t(second));
        c.set(0, shift_right_1(c, c.r(0)));
        c.set(0, c.r(4) + c.r(0));
        return std::int32_t(result(c, sext16(c.r(0))));
    };
    const auto single = [&](std::uint32_t offset) {
        const auto record = load32(c, 3, person + record_offset);
        return std::int32_t(load16s(c, 3, record + offset));
    };
    c.set(0, sext8(cls));
    if (compare(c, std::int32_t(sext8(cls)), 4) == 0) return single(at_4);
    c.set(0, sext8(cls));
    if (compare(c, std::int32_t(sext8(cls)), 2) == 0) return single(at_2);
    c.set(0, sext8(cls));
    if (compare(c, std::int32_t(sext8(cls))) == 0) return single(at_0);
    c.set(0, sext8(cls));
    if (compare(c, std::int32_t(sext8(cls)), 3) == 0) return average(at_4, at_2);
    return average(at_0, at_2);
}
}
