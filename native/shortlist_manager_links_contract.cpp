#include "shortlist_manager_detail.hpp"
#include "shortlist_manager_random.hpp"

// The staff-club links and the contracts: the primary-contract marks and the
// scheduling of a link transition.
namespace cm::aot::shortlist_manager {
using namespace detail;
using namespace links;
namespace {
// The bit 0 of the contract's flag byte at +0x5c (primary contract, flag one) and bit 1 (flag two).
constexpr std::uint32_t flag_one = 1, flag_two = 2;
}

void mark_primary_contract(Cpu& c, std::uint32_t recruitment, std::uint32_t staff, std::uint32_t follow_up) {
    Frame frame(c, 0x10363130, 0x60, {.first_gpr = 23});
    const auto contracts = c.toc(slot::contracts);
    keep(c, {{24, staff}, {23, recruitment}, {26, contracts}, {25, follow_up}, {27, c.toc(slot::random_salt)},
             {28, c.toc(slot::random_table)}, {29, c.toc(slot::random_end)}, {30, c.toc(slot::random_cursor)}});
    Program program(c);
    if (nonzero(c, byte_of(program.call(0x100d3040, 0x1036316c, {staff, 0})))) return;
    const auto contract = program.call(0x10082f20, 0x10363180, {c.u32(contracts), staff});
    Kept<31> held(c, contract);
    if (!nonzero(c, std::int32_t(contract))) return;
    // Flag one on, and status 7 in the low nibble of +0x60.
    c.put8(contract + 0x5c, c.u8(contract + 0x5c) | flag_one);
    c.put8(contract + 0x60, (c.u8(contract + 0x60) & 0xf0) | 7);
    c.set(6, 7);
    const auto blocked = program.call(0x103f43b0, 0x103631b8, {c.u32(c.toc(link_slot::transfer_manager)), staff, 0});
    if (nonzero(c, byte_of(blocked))) return;
    const auto club = c.u32(staff + 0x24);
    bool react = nonzero(c, byte_of(program.call(0x100da0b0, 0x103631cc, {staff, club})));
    if (!react) {
        // Two draws from the game's random table (shortlist_manager_random.hpp),
        // each modulo 100 as the compiler divides (mulhw by 0x51eb851f, shift
        // 5, plus the sign bit, left in r3). The original steps the cursor for
        // the second draw before it finishes the first. The first draw's
        // residue stays in r29; the contract's byte at +0x4c (signed, kept in
        // r31) must not exceed the difference of the draws.
        const RandomTable random{c, c.r(30), c.r(29), c.r(28), c.r(27)};
        const auto by_100 = [&](std::uint32_t value) {
            std::int32_t quotient = multiply_high(0x51eb851f, std::int32_t(value)) >> 5;
            const std::uint32_t sign = std::uint32_t(quotient) >> 31;
            quotient += std::int32_t(sign);
            c.set(3, sign);
            return value - std::uint32_t(quotient) * 100;
        };
        held = c.u8(contract + 0x4c);
        random.advance(0x103631f8);
        const std::uint32_t first_at = random.position(), first_entry = c.u16(first_at);
        const std::uint32_t first_value = first_entry + random.salt_value();
        random.advance(0x1036324c);
        const Kept<29> first(c, by_100(first_value));
        const std::uint32_t second_value = random.value();
        const std::uint32_t second = by_100(second_value);
        const std::int32_t limit = std::int8_t(std::uint32_t(held));
        leave(c, {{4, second_value}, {5, std::uint32_t(limit)}, {6, first_entry}, {7, first_at}});
        react = compare(c, limit, std::int32_t(std::uint32_t(first) - second)) > 0;
    }
    if (react) program.call(0x100d9b80, 0x103632a8, {staff, c.u32(c.u32(staff + 0x24) + 0x90), 1});
    if (nonzero(c, byte_of(follow_up))) {
        program.call(0x10363ca0, 0x103632c0, {recruitment, staff, 1});
        program.call(0x1008cd20, 0x103632d0, {c.u32(contracts), staff, c.u32(staff + 0x24)});
    }
}

std::uint32_t clear_primary_contract_flag_one(Cpu& c) {
    Frame frame(c, 0x10365c30, 0x40, {});
    const auto contract = Program(c).call(0x10082f20, 0x10365c48, {c.u32(c.toc(slot::contracts)), c.r(4)});
    if (!nonzero_logical(c, contract)) return result(c, 0);
    c.put8(contract + 0x5c, c.u8(contract + 0x5c) & ~flag_one);
    return result(c, 1);
}

std::uint32_t set_primary_contract_flag_two(Cpu& c) {
    Frame frame(c, 0x10365c80, 0x40, {});
    const auto contract = Program(c).call(0x10082f20, 0x10365c98, {c.u32(c.toc(slot::contracts)), c.r(4)});
    if (nonzero_logical(c, contract)) c.put8(contract + 0x5c, c.u8(contract + 0x5c) | flag_two);
    return result(c, contract);
}

std::uint32_t clear_primary_contract_flag_two(Cpu& c) {
    Frame frame(c, 0x10365cc0, 0x40, {});
    const auto contract = Program(c).call(0x10082f20, 0x10365cd8, {c.u32(c.toc(slot::contracts)), c.r(4)});
    if (!nonzero_logical(c, contract)) return result(c, 0);
    c.put8(contract + 0x5c, c.u8(contract + 0x5c) & ~flag_two);
    return result(c, 1);
}

std::uint32_t schedule_staff_transition(Cpu& c, std::uint32_t links, std::uint32_t staff, std::uint32_t club) {
    Frame frame(c, 0x103636b0, 0xb0, {.first_gpr = 19});
    const auto constants = c.toc(slot::constants);
    const auto constant = [&](std::uint32_t offset) { return c.f64(constants + offset); };
    const auto clubs = c.toc(slot::clubs), contracts = c.toc(slot::contracts), manager_slot = c.toc(link_slot::transfer_manager);
    const auto last_contract = c.toc(link_slot::last_contract), today = c.toc(slot::today);
    keep(c, {{20, staff}, {21, club}, {25, today}, {19, links}, {26, manager_slot}, {27, contracts}, {28, last_contract},
             {29, constants}, {30, clubs}});
    Program program(c);
    Fpu fpu{c};
    // The staff's link row at the club (ordinary kind).
    enter(c, 0x103636f8, {links, club, staff, 0, 0});
    Kept<22> row(c, find_link_row(c, links, club, staff, 0, 0));
    // The club of the staff's contract, if any.
    const auto contract_club_of = [&](std::uint32_t back) {
        const auto contract = program.call(0x10082f20, back, {c.u32(contracts), staff});
        const bool has = nonzero_logical(c, contract);
        c.put32(c.r(28), contract);
        if (!has) return std::uint32_t(0);
        c.set(3, c.u32(clubs));
        return c.u32(clubs) + c.u32(contract + 4) * 0x248;
    };
    Kept<24> contract_club(c, contract_club_of(0x10363708));
    if (!nonzero_logical(c, contract_club)) {
        if (nonzero_logical(c, row)) {
            enter(c, 0x10363754, {links, staff, club, 1, 0});
            transition_staff_links(c, links, staff, club, 1, 0);
        }
        return result(c, std::uint32_t(-1));
    }
    // The transfer manager's shortlist object judges the link at its status (the row's, bits 3-6 of +0x28);
    // too low a figure (f1 under constant +0x160) ends it.
    const std::uint32_t status = (c.u8(row + 0x28) >> 3) & 0xf;
    c.set(8, extended(std::int8_t(status)));
    program.call(0x10369080, 0x10363788, {c.u32(c.u32(manager_slot) + 0x988), staff, club, 0, 1, extended(std::int8_t(status)), 0});
    fpu.fcmp(0, c.f(1), constant(0x160));
    if ((c.cr(0) & 8) != 0) return result(c, std::uint32_t(-1));
    Kept<31> month(c, program.call(0x10718360, 0x103637a4, {today}));
    // The contract again (the call may have changed it), then the scouting dates of the transfer manager.
    const auto second_contract = program.call(0x10082f20, 0x103637b4, {c.u32(contracts), staff});
    const bool has_second = nonzero_logical(c, second_contract);
    c.put32(c.r(28), second_contract);
    const std::uint32_t second_club = has_second ? c.u32(clubs) + c.u32(second_contract + 4) * 0x248 : 0;
    if (has_second) c.set(3, c.u32(clubs));
    const auto bytes_at = frame.at(0x38);  // four values the callee writes: +0 and +1 bytes, +2 and +4 halfwords
    program.call(0x103fa8e0, 0x103637f8, {c.u32(manager_slot), club, second_club, staff, bytes_at + 1, bytes_at + 4, bytes_at, bytes_at + 2});
    Kept<30> year_at(c, today + 2);
    const std::uint32_t year = c.u16(today + 2);
    c.set(7, year);
    Kept<28> month_signed(c, extended(std::int8_t(month)));
    const std::int32_t month_value = std::int32_t(std::uint32_t(month_signed));
    c.set(3, c.u16(bytes_at + 4));
    c.set(4, extended(std::int8_t(c.u8(bytes_at + 1))));
    c.set(5, c.u16(bytes_at + 2));
    c.set(3, (c.r(3) - year) * 12);
    c.set(6, extended(std::int8_t(c.u8(bytes_at))));
    c.set(5, (c.r(5) - year) * 12);
    c.set(3, c.r(4) + c.r(3));
    c.set(3, c.r(3) - std::uint32_t(month_value));
    c.set(4, extended(std::int16_t(c.r(3))));
    c.set(3, c.r(6) + c.r(5));
    c.set(3, c.r(3) - std::uint32_t(month_value));
    // (earliest, latest) in months from now: the window the link's expiry must fall in.
    const std::int32_t earliest = half_of(c.r(4));
    row = extended(std::int16_t(c.r(3)));
    const std::int32_t latest = half_of(row);
    compare(c, earliest, 1);
    Kept<23> start(c, extended(std::int16_t(earliest < 1 ? 1 : earliest)));
    if (compare(c, half_of(start), latest) > 0) return result(c, std::uint32_t(-1));
    // Not in the October-February window for the nation of a special regional kind.
    const std::uint32_t nation_at = c.u32(club + 0x2c);
    c.set(4, nation_at);
    if (nonzero_logical(c, nation_at)) {
        c.set(3, c.toc(slot::regional_nation_2));
        c.set(4, c.u32(nation_at + 0x68));
        if (compare(c, std::int32_t(c.r(4)), std::int32_t(c.u32(c.r(3)))) == 0) {
            const std::int32_t m = byte_of(month);
            if (compare(c, m, 10) >= 0 || compare(c, m, 2) <= 0) return result(c, std::uint32_t(-1));
        }
    }
    program.call(0x10369080, 0x103638c4, {links, staff, club, 0, 0, 1, 0});
    fpu.fcmp(0, c.f(1), constant(0x2c0));
    bool use_latest = (c.cr(0) & 8) != 0;
    if (!use_latest) {
        const std::uint32_t nation = c.u32(c.u32(club + 0x2c) + 0x68);
        c.set(4, nation);
        for (const auto slot_offset : {link_slot::nation_a, link_slot::nation_b, link_slot::nation_c}) {
            c.set(3, c.toc(slot_offset));
            if (compare(c, std::int32_t(nation), std::int32_t(c.u32(c.r(3)))) == 0) {
                use_latest = true;
                break;
            }
        }
    }
    // The month the link will be settled (r19, a signed byte).
    Kept<19> target(c, extended(std::int8_t(month_signed + std::uint32_t(use_latest ? latest : std::int32_t(half_of(start))))));
    Kept<22> year_value(c, c.u16(year_at));
    // Normalise the month into 0-11, carrying into the year.
    while (compare(c, byte_of(target), 11) > 0) {
        target = target - 12;
        year_value = year_value + 1;
    }
    auto day = half_of(program.call(0x107182f0, 0x10363944, {today}));
    if (compare(c, day, 0x1c) > 0) day = 0x1c;
    else day = half_of(program.call(0x107182f0, 0x10363960, {today}));
    const auto built = frame.at(0x68);
    program.call(0x107156f0, 0x1036397c, {built, extended(std::int16_t(day)), target, year_value, std::uint32_t(-1)});
    const auto settled = program.call(0x103fa850, 0x10363998, {c.u32(manager_slot), club, staff, 0, built, 1});
    if (!nonzero(c, byte_of(settled))) return result(c, std::uint32_t(-1));
    const auto contract_again = program.call(0x10082f20, 0x103639b4, {c.u32(contracts), staff});
    if (!nonzero_logical(c, contract_again)) return result(c, std::uint32_t(-1));

    // Three date tests (each leaves its registers as the original's inline code does); any that holds ends it.
    // less-than of two dates by the year, then by the signed day, as the compiler wrote it.
    const auto day_bit = [&](std::int32_t x, std::int32_t mask_value) {
        c.set(3, std::uint32_t(x >> 1));
        return ((std::uint32_t(x >> 1) - std::uint32_t(x & mask_value)) >> 31) & 1;
    };
    c.set(4, c.u32(contract_again + 0x30));
    c.set(0, c.u32(contract_again + 0x34));
    c.put32(frame.at(0x50), c.r(4));
    c.put32(frame.at(0x54), c.u32(contract_again + 0x34));
    c.set(3, c.u16(frame.at(0x6a)));
    c.set(4, c.u16(frame.at(0x52)));
    std::uint32_t before_built;
    if (compare_logical(c, c.r(4), c.r(3)) < 0) before_built = 1;
    else if (compare_logical(c, c.r(4), c.r(3)) > 0) before_built = 0;
    else {
        const std::int32_t a = c.i16(frame.at(0x50)), b = c.i16(frame.at(0x68));
        c.set(4, std::uint32_t(b));
        before_built = day_bit(b ^ a, b);
    }
    if (nonzero(c, byte_of(before_built))) return result(c, std::uint32_t(-1));
    const auto calendar_index = byte_of(program.call(0x10191fd0, 0x10363a28, {club}));
    const std::uint32_t calendar = c.toc(slot::calendars) + std::uint32_t(calendar_index) * 0x4c;
    c.set(7, calendar);
    c.set(6, c.u32(calendar + 0x2c));
    c.put32(frame.at(0x48), c.u32(calendar + 0x2c));
    c.put32(frame.at(0x4c), c.u32(calendar + 0x30));
    program.call(0x10717cb0, 0x10363a58, {frame.at(0x60), built, 4});
    std::uint32_t calendar_after;
    c.set(3, c.u16(frame.at(0x4a)));
    c.set(0, c.u16(frame.at(0x62)));
    if (compare_logical(c, c.r(3), c.u16(frame.at(0x62))) > 0) calendar_after = 1;
    else if (compare_logical(c, c.r(3), c.u16(frame.at(0x62))) < 0) calendar_after = 0;
    else {
        const std::int32_t a = c.i16(frame.at(0x48)), b = c.i16(frame.at(0x60));
        c.set(4, std::uint32_t(a));
        calendar_after = day_bit(a ^ b, a);
    }
    if (nonzero(c, byte_of(calendar_after))) return result(c, std::uint32_t(-1));
    program.call(0x10717a10, 0x10363abc, {frame.at(0x58), today, 3});
    c.set(3, c.u32(frame.at(0x58)));
    c.put32(frame.at(0x40), c.r(3));
    c.put32(frame.at(0x44), c.u32(frame.at(0x5c)));
    c.set(3, c.u16(frame.at(0x6a)));
    c.set(4, c.u16(frame.at(0x42)));
    std::uint32_t built_early;
    if (compare_logical(c, c.r(4), c.r(3)) > 0) built_early = 1;
    else if (compare_logical(c, c.r(4), c.r(3)) < 0) built_early = 0;
    else {
        const std::int32_t a = c.i16(frame.at(0x40)), b = c.i16(frame.at(0x68));
        c.set(4, std::uint32_t(a));
        built_early = day_bit(a ^ b, a);
    }
    if (nonzero(c, byte_of(built_early))) return result(c, std::uint32_t(-1));
    // The offer's size by the difference of the two clubs' ratings.
    const std::int32_t difference = std::int32_t(Field<std::int16_t>{c, club + 0x1ec}.get()) -
                                    std::int32_t(Field<std::int16_t>{c, contract_club + 0x1ec}.get());
    std::uint32_t offer;
    if (compare(c, difference, 0x9c4) > 0) offer = 100;
    else if (compare(c, difference, -0x5dc) > 0) offer = 50;
    else offer = 0;
    return result(c, program.call(0x103e3170, 0x10363b6c, {c.u32(manager_slot), staff, club, built, offer, 1, 1}));
}
}
