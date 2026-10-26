#include "shortlist_manager_detail.hpp"
#include "shortlist_manager_policy.hpp"

namespace cm::aot::shortlist_manager {
using namespace detail;
using namespace policy;
namespace {
// TOC slots this group reads that the shared header does not name.
constexpr std::int32_t transfers = -0x7b40;  // variable holding the transfer manager object (inferred)
// Nation id variables (each TOC word is the address of an int32): the
// nations whose clubs get the larger reference amounts in
// club_category_reference (meaning inferred from use).
constexpr std::int32_t top_nation_1 = -0x7a14, top_nation_2 = -0x7a10, top_nation_3 = -0x7a18;
constexpr std::int32_t mid_nation_1 = -0x7af4, mid_nation_2 = -0x7a00;

// The player-record rating bytes (offsets into the record) a category needs
// at least 15 in: `first`, and `second` when it is not 0. The jump table
// (slot::cases_10365d10) holds one case label per category; the label read
// from it picks the pair, so the table stays the game's.
struct RatingPair {
    std::uint32_t first, second;
};
constexpr RatingPair needed_ratings(std::uint32_t label) {
    switch (label) {
    case 0x10365d74: return {0xf, 0};
    case 0x10365d8c: return {0x10, 0x19};
    case 0x10365db4: return {0x11, 0x18};
    case 0x10365ddc: return {0x11, 0x19};
    case 0x10365e04: return {0x11, 0x17};
    case 0x10365e2c: return {0x16, 0x18};
    case 0x10365e54: return {0x12, 0x19};
    case 0x10365e7c: return {0x16, 0x17};
    case 0x10365ea4: return {0x13, 0x18};
    case 0x10365ecc: return {0x13, 0x19};
    case 0x10365ef4: return {0x13, 0x17};
    case 0x10365f1c: return {0x14, 0x18};
    case 0x10365f44: return {0x14, 0x19};
    case 0x10365f6c: return {0x14, 0x17};
    case 0x10365f94: return {0x15, 0};
    default: return {0, 0};  // 0x10365fac: category 15, no ratings
    }
}
constexpr std::uint32_t category_15_label = 0x10365fac;

// The role names (text:: offsets), by the case label of the jump table
// slot::cases_1036f7e0; every label calls 0x10727890 at label + 0xc.
constexpr std::uint32_t role_text(std::uint32_t label) {
    switch (label) {
    case 0x1036f87c: return text::indispensable;
    case 0x1036f88c: return text::important;
    case 0x1036f89c: return text::rotation;
    case 0x1036f8ac: return text::backup;
    case 0x1036f8bc: return text::hot_prospect;
    case 0x1036f8cc: return text::decent_young;
    case 0x1036f8dc: return text::not_needed;
    case 0x1036f8ec: return text::on_trial;
    default: return text::uncertain;  // 0x1036f8fc
    }
}
}

std::uint32_t eligible_for_category(Cpu& c, std::uint32_t staff, std::uint32_t category) {
    Frame frame(c, 0x10365d10, 0x70, {.first_gpr = 29});
    const std::int32_t kind = std::int8_t(category);  // extsb r0,r5
    const Person person{c, staff};
    const std::uint32_t profile = person.player();
    // Without a player record only category 15 goes on (out-of-range ones
    // answer 0 here, before the assertion).
    if (!nonzero_logical(c, profile) && compare(c, kind, 15) != 0) return result(c, 0);
    if (compare_logical(c, std::uint32_t(kind), 15) > 0) {
        const auto pool = c.toc(slot::pool);
        keep(c, {{31, pool}, {30, c.toc(slot::message)}, {29, staff}});
        report(c, pool, 0x10366044, frame.at(0x40), frame.at(0x38), 0x1c2a, false);
        return result(c, 0);
    }
    const std::uint32_t label = c.u32(c.toc(slot::cases_10365d10) + std::uint32_t(kind) * 4);
    c.set(4, label);
    if (label != category_15_label) {
        const auto [first, second] = needed_ratings(label);
        const Player ratings{c, profile};
        // Quirk: the bytes are signed, so a raw 255 reads as -1 and fails.
        if (compare(c, ratings.rating(first), 15) < 0) return result(c, 0);
        if (second && compare(c, ratings.rating(second), 15) < 0) return result(c, 0);
        return result(c, 1);
    }
    // Category 15: needs no player record. A staff member the club predicate
    // (0x1014c2e0) rejects is out; otherwise one with a staff record whose
    // id is below person_count - 16 is in.
    const std::uint32_t club = person.club();
    c.set(3, club);
    if (nonzero_logical(c, club)) {
        const std::int32_t role = person.job();
        if (compare(c, role, 0xb) == 0 || compare(c, role, 0xf) == 0 || compare(c, role, 0xd) == 0) {
            keep(c, {{31, c.toc(slot::pool)}, {30, c.toc(slot::message)}, {29, staff}});
            const std::uint32_t answer = Program{c}.call(0x1014c2e0, 0x10365fec, {club, 0, 1, 0, 0});
            if (nonzero(c, std::int8_t(answer))) return result(c, 0);
        }
    }
    if (nonzero_logical(c, person.non_player())) {
        const std::uint32_t count_address = c.toc(slot::person_count);
        const std::int32_t id = person.id();
        c.set(4, std::uint32_t(id));
        const std::int32_t count = c.i32(count_address);
        // No lower bound on the id: a negative one passes.
        if (compare(c, id, std::int32_t(std::uint32_t(count) - 16)) < 0) return result(c, 1);
    }
    return result(c, 0);
}

std::uint32_t count_and_prune_club_rows(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t category) {
    Frame frame(c, 0x103660b0, 0x70, {.first_gpr = 23});
    const std::uint32_t today = c.toc(slot::today), transfer_manager = c.toc(transfers),
                        persons = c.toc(slot::persons);
    const std::uint32_t header = c.u32(manager + 0xdc) + std::uint32_t(c.i32(club)) * 12;
    std::uint32_t count = 0;
    // The registers the original keeps across the calls (its callees save
    // them into the stack image).
    const auto hold = [&](std::uint32_t index, std::uint32_t offset) {
        keep(c, {{23, club}, {24, category}, {25, index}, {26, header}, {27, count}, {28, offset}, {29, today},
                 {30, transfer_manager}, {31, persons}});
    };
    std::uint32_t offset = 0;
    for (std::uint32_t index = 0; index < 40; ++index, offset += Row::size) {
        const std::uint32_t row_address = c.u32(header) + offset;
        const Row row{c, row_address};
        c.set(4, row_address);
        if (compare(c, row.target(), -1) <= 0 || compare(c, row.approach(), -1) <= 0) continue;
        const std::int32_t wanted = std::int8_t(category);
        c.set(3, std::uint32_t(wanted));
        if (compare(c, wanted, -1) != 0 && compare(c, wanted, std::int8_t(row.kind())) != 0) continue;
        // Rows of a header of kind 2 are live only while the club's check
        // (0x1014c2e0) holds and the manager has been there over 20 days
        // (inferred from the date test below).
        if (compare(c, c.u8(header + 4), 2) != 0) { ++count; continue; }
        hold(index, offset);
        const std::uint32_t asked = Program{c}.call(0x1014c2e0, 0x10366158, {club, 1, 1, 0, 0});
        if (!nonzero(c, std::int8_t(asked))) { ++count; continue; }
        hold(index, offset);
        // today minus 20 days, into the stack at sp+0x40; its first eight
        // bytes (day, year) are copied to sp+0x38.
        Program{c}.call(0x10717b20, 0x10366170, {frame.at(0x40), today, 0x14});
        const std::uint32_t manager_person = c.u32(club + 0x90);
        c.set(3, manager_person);
        c.put32(frame.at(0x38), c.u32(frame.at(0x40)));
        const std::uint32_t year = c.u16(frame.at(0x3a));
        c.set(5, year);
        const std::uint32_t joined_year = c.u16(manager_person + 0x4e);
        c.set(4, joined_year);
        c.put32(frame.at(0x3c), c.u32(frame.at(0x44)));
        std::uint32_t after_joining;  // (today - 20 days) is after the manager's joining date
        const std::int32_t order = compare_logical(c, year, joined_year);
        if (order > 0) after_joining = 1;
        else if (order < 0) after_joining = 0;
        else {
            const std::uint32_t day = extended(c.i16(frame.at(0x38))),
                                joined_day = extended(c.i16(manager_person + 0x4c));
            c.set(4, day);
            // The original's branch-free signed `day > joined_day`.
            const std::uint32_t difference = day ^ joined_day;
            const std::uint32_t halved = std::uint32_t(std::int32_t(difference) >> 1);
            c.set(3, halved);
            after_joining = (halved - (difference & day)) >> 31;
        }
        if (!nonzero(c, std::int8_t(after_joining))) { ++count; continue; }
        const std::uint32_t staff_index = c.u32(c.u32(header) + offset + 0x10);
        const std::uint32_t person = c.u32(persons) + staff_index * Person::size;
        hold(index, offset);
        const std::uint32_t found = Program{c}.call(0x103eaea0, 0x103661f8, {c.u32(transfer_manager), person, 0, 0});
        if (compare(c, std::int32_t(found), 0) != 0) { ++count; continue; }
        // The staff member is not usable any more: the row loses both references.
        const std::uint32_t base = c.u32(header);
        c.set(4, base);
        c.set(5, std::uint32_t(-1));
        Row{c, base + offset}.target() = -1;
        const std::uint32_t reloaded = c.u32(header);
        c.set(3, reloaded);
        Row{c, reloaded + offset}.approach() = -1;
    }
    compare(c, 40, 0x28);
    return result(c, count);
}

void refresh_unassigned_rows(Cpu& c, std::uint32_t manager, std::uint32_t club, std::uint32_t header) {
    Frame frame(c, 0x10366250, 0x70, {.first_gpr = 20});
    const std::uint32_t contracts = c.toc(slot::contracts), current_contract = c.toc(-0x7b78),
                        count_address = c.toc(slot::person_count), persons = c.toc(slot::persons),
                        clubs = c.toc(slot::clubs);
    std::uint32_t flag = 0, person = 0;
    const auto hold = [&](std::uint32_t index, std::uint32_t offset) {
        keep(c, {{20, flag}, {21, manager}, {22, club}, {23, header}, {24, index}, {25, person}, {26, offset},
                 {27, contracts}, {28, current_contract}, {29, count_address}, {30, persons}, {31, clubs}});
    };
    std::uint32_t offset = 0;
    for (std::uint32_t index = 0; index < 40; ++index, offset += Row::size) {
        std::uint32_t rows = c.u32(header);
        const std::int32_t count = c.i32(count_address);
        c.set(5, rows);
        c.set(6, offset + 0x10);
        c.set(3, std::uint32_t(count));
        const std::int32_t staff_index = Row{c, rows + offset}.target();
        c.set(4, std::uint32_t(staff_index));
        // A staff index past the last person is cleared.
        if (compare(c, staff_index, count - 1) > 0) Row{c, rows + offset}.target() = -1;
        rows = c.u32(header);
        c.set(3, rows);
        const std::int32_t staff = Row{c, rows + offset}.target();
        if (compare(c, staff, -1) <= 0) continue;
        flag = 0;
        const std::uint32_t persons_base = c.u32(persons);
        c.set(3, c.u32(contracts));
        person = persons_base + std::uint32_t(staff) * Person::size;
        c.set(4, person);
        hold(index, offset);
        const std::uint32_t contract = Program{c}.call(0x10082f20, 0x103662e4, {c.u32(contracts), person});
        c.put32(current_contract, contract);
        // Only a contract whose mode is not 5 makes the row's club the
        // contract's club (a mode of 5 reads as no club).
        if (nonzero_logical(c, contract) && Contract{c, contract}.mode() != 5) flag = 1;
        std::uint32_t contract_club = 0;
        if (nonzero(c, std::int32_t(flag))) {
            const std::uint32_t current = c.u32(current_contract);
            c.set(3, current);
            const std::uint32_t clubs_base = c.u32(clubs);
            c.set(4, clubs_base);
            contract_club = clubs_base + c.u32(current + 4) * 0x248;
        }
        if (compare_logical(c, contract_club, club) == 0) continue;
        if (compare_logical(c, Person{c, person}.club(), club) == 0) continue;
        const std::uint32_t row_address = c.u32(header) + offset;
        c.set(3, row_address);
        const Row row{c, row_address};
        if (compare(c, row.approach(), -1) != 0) continue;
        const std::int32_t category = std::int8_t(row.kind());
        // The row's staff is registered for the club (0x10365590); the
        // original's `category != 11` is computed branch-free.
        hold(index, offset);
        Program{c}.call(0x10365590, 0x10366388,
                        {manager, person, club, 3, std::uint32_t(category != 11), std::uint32_t(category)});
    }
    compare(c, 40, 0x28);
}

std::uint32_t club_category_reference(Cpu& c, std::uint32_t club, std::uint32_t category) {
    const std::int32_t kind = std::int8_t(category);
    const Club view{c, club};
    const auto scaled = [&](std::int32_t factor) {
        return result(c, std::uint32_t(extended(view.reputation())) * std::uint32_t(factor));
    };
    const auto matches = [&](std::int32_t id, std::initializer_list<std::int32_t> slots) {
        for (const auto offset : slots)
            if (compare(c, id, c.i32(c.toc(offset))) == 0) return true;
        return false;
    };
    // The reference is the club's reputation times a factor: 0x226 for
    // categories 5 and (4: three nations), 0x6e for 6, 0x1db / 0x14a for 4;
    // the others have no reference (-1).
    if (compare(c, kind, 5) == 0) return scaled(0x226);
    if (kind > 5) {
        compare(c, kind, 7);
        return kind >= 7 ? result(c, std::uint32_t(-1)) : scaled(0x6e);
    }
    compare(c, kind, 4);
    if (kind < 4) {
        compare(c, kind, 1);
        return result(c, std::uint32_t(-1));
    }
    // Category 4 goes by the club's nation: 0x226 for the top three nations,
    // 0x1db for two others, 0x14a for the rest.
    const std::uint32_t nation = view.nation();
    c.set(5, nation);
    if (nonzero_logical(c, nation)) {
        const std::int32_t id = Nation{c, nation}.id();
        c.set(6, std::uint32_t(id));
        if (matches(id, {top_nation_1, top_nation_2, top_nation_3})) return scaled(0x226);
    }
    if (nonzero_logical(c, nation)) {
        const std::int32_t id = Nation{c, nation}.id();
        c.set(5, std::uint32_t(id));
        if (matches(id, {mid_nation_1, mid_nation_2})) return scaled(0x1db);
    }
    return scaled(0x14a);
}

std::uint32_t candidate_category_reference(Cpu& c, std::uint32_t unused, std::uint32_t club, std::uint32_t person,
                                           std::uint32_t category) {
    Frame frame(c, 0x1036f670, 0x50, {.first_gpr = 29});
    keep(c, {{31, category}, {30, club}, {29, unused}});
    // A person with no player record has no reference.
    if (!nonzero_logical(c, person)) return result(c, std::uint32_t(-1));
    const std::uint32_t profile = Person{c, person}.player();
    c.set(3, profile);
    if (!nonzero_logical(c, profile)) return result(c, std::uint32_t(-1));
    const std::uint32_t qualification = Program{c}.call(0x10117490, 0x1036f6b4, {profile});
    enter_direct(c, 0x1036f6cc, {unused, club, qualification, category});
    return club_category_reference(c, club, category);
}

std::uint32_t write_role_name(Cpu& c, std::uint32_t kind, std::uint32_t destination, std::uint32_t limit) {
    Frame frame(c, 0x1036f7e0, 0x70, {.first_gpr = 30});
    const auto pool = c.toc(slot::pool);
    keep(c, {{31, pool}, {30, c.toc(slot::message)}});
    // (inferred) a limit below 30 is a caller error: the assertion.
    if (compare(c, std::int32_t(limit), 0x1e) < 0) {
        report(c, pool, 0x1036f818, frame.at(0x40), frame.at(0x38), 0x2d82, false);
        return result(c, c.toc(slot::success));
    }
    const std::int32_t role = std::int8_t(kind);
    std::uint32_t label = 0x1036f8fc;  // kinds above 8 take the last case
    if (compare_logical(c, std::uint32_t(role), 8) <= 0) {
        label = c.u32(c.toc(slot::cases_1036f7e0) + std::uint32_t(role) * 4);
        c.set(3, label);
    }
    return Program{c}.call(0x10727890, label + 0xc, {destination, pool + role_text(label)});
}

std::uint32_t fits_category(Cpu& c, std::uint32_t person, std::uint32_t category) {
    Frame frame(c, 0x1036f920, 0x70, {.first_gpr = 30});
    const Person view{c, person};
    if (!nonzero_logical(c, view.player())) {
        const auto pool = c.toc(slot::pool);
        keep(c, {{31, pool}, {30, c.toc(slot::message)}});
        report(c, pool, 0x1036f95c, frame.at(0x40), frame.at(0x38), 0x2db4);
        return result(c, 0);
    }
    const std::int32_t kind = std::int8_t(category);
    if (compare(c, kind, 6) != 0 && compare(c, kind, 5) != 0) return result(c, 1);
    // Categories 5 and 6: the low byte of the halfword at +0x5c (sign-
    // extended, as extsb) must be below 24 (age, inferred).
    return result(c, compare(c, std::int8_t(view.age_halfword()), 0x18) < 0 ? 1 : 0);
}

std::uint32_t record_name_equals(Cpu& c, std::uint32_t record, std::uint32_t text) {
    Frame frame(c, 0x1036ff00, 0x240, {.first_gpr = 31});
    keep(c, {{31, text}});
    if (!nonzero_logical(c, record)) return result(c, 0);
    // The name goes into a 500-byte stack buffer, then strcmp.
    Program{c}.call(0x100caeb0, 0x1036ff34, {record, frame.at(0x38), 0x1f4, 1});
    const std::uint32_t difference = Program{c}.call(0x1077dc90, 0x1036ff40, {frame.at(0x38), text});
    return result(c, difference == 0 ? 1 : 0);
}
}
