#include "player_stats_detail.hpp"
#include <optional>

namespace cm::aot::player_stats {
using namespace detail;
namespace {
// The key bsearch looks relations up by (a Relation on the stack at `key`).
std::uint32_t find_relation(Cpu& c, const Stats& stats, std::uint32_t key, std::uint32_t person, std::uint32_t group,
                            std::uint32_t back) {
    c.put32(key, std::uint32_t(Person{c, person}.id().get()));
    c.put8(key + 4, group);
    return Program{c}.bsearch(key, stats.relations(), std::uint32_t(stats.relation_count().get()), Relation::size,
                              c.toc(slot::compare_relations_tv), back);
}

// The four regional nations' extra groups: a person of one of them belongs
// to these groups too, `score` for the first three nations, `strong` for the
// fourth (none when the nation or the group does not match).
std::optional<std::uint32_t> regional(Cpu& c, std::int32_t nation, std::int32_t wanted, std::uint32_t score,
                                      std::uint32_t strong) {
    const auto either = [&](std::int32_t a, std::int32_t b) {
        return compare(c, wanted, a) == 0 || compare(c, wanted, b) == 0;
    };
    if (compare(c, nation, c.i32(c.toc(slot::regional_nation_1))) == 0)
        return either(0x16, 0x20) ? std::optional(score) : std::nullopt;
    if (compare(c, nation, c.i32(c.toc(slot::regional_nation_2))) == 0)
        return either(0x1e, 0x20) ? std::optional(score) : std::nullopt;
    if (compare(c, nation, c.i32(c.toc(slot::regional_nation_3))) == 0)
        return either(0x16, 0x1e) ? std::optional(score) : std::nullopt;
    if (compare(c, nation, c.i32(c.toc(slot::regional_nation_4))) == 0 && compare(c, wanted, 7) == 0)
        return strong;
    return std::nullopt;
}

// The original's branch-free `a == b ? value : 0`: ~((b - a) | (a - b)) >> 31.
std::uint32_t when_equal(std::int32_t a, std::int32_t b, std::uint32_t value) {
    const auto differ = (std::uint32_t(b) - std::uint32_t(a)) | (std::uint32_t(a) - std::uint32_t(b));
    return std::uint32_t(std::int32_t(~differ) >> 31) & value;
}
}

std::uint32_t affinity(Cpu& c, std::uint32_t stats_address, std::uint32_t person, std::uint32_t club,
                       std::uint32_t group, std::uint32_t ask_contract) {
    Frame frame(c, 0x103092f0, 0x50, {.first_gpr = 28});
    keep(c, {{31, group}, {30, person}, {29, stats_address}, {28, club}});
    const Person who{c, person};
    const std::int32_t wanted = std::int8_t(group);
    if (nonzero(c, std::int32_t(person))) {
        // Persons past the players: only their nation's group counts.
        const std::int32_t id = who.id();
        leave(c, {{4, c.toc(slot::person_count)}, {5, std::uint32_t(id)}});
        if (compare(c, id, c.i32(c.toc(slot::person_count))) >= 0) {
            const std::int32_t own = Nation{c, who.nation()}.group();
            leave(c, {{4, std::uint32_t(own - wanted)}, {5, std::uint32_t(own)}});
            return when_equal(own, wanted, 10);
        }
    }
    // With a contract at a club of the person's own club's nation, the
    // contract's affinity (the high nibble of its byte +0x60).
    std::uint32_t contract = 0;
    if (nonzero(c, std::int8_t(ask_contract)))
        contract = Program{c}.call(0x10082f90, 0x10309380, {c.u32(c.toc(slot::contracts)), person, club});
    if (nonzero_logical(c, contract)) {
        const std::uint32_t own_club = who.club();
        c.set(5, own_club);
        if (nonzero_logical(c, own_club)) {
            const std::uint32_t nation = Club{c, club}.nation();
            c.set(4, nation);
            if (compare_logical(c, nation, Club{c, own_club}.nation()) == 0) return (c.u8(contract + 0x60) >> 4) & 0xf;
        }
    }
    // A club of the person's nation's (or second nation's) group.
    if (nonzero_logical(c, club)) {
        const std::uint32_t club_nation = Club{c, club}.nation();
        c.set(4, club_nation);
        if (nonzero_logical(c, club_nation)) {
            const std::int8_t club_group = Nation{c, club_nation}.group();
            c.set(4, std::uint8_t(club_group));
            if (compare(c, Nation{c, who.nation()}.group(), club_group) == 0) return 10;
            const std::uint32_t second = who.second_nation();
            c.set(4, second);
            if (nonzero_logical(c, second) && compare(c, Nation{c, second}.group(), club_group) == 0) return 10;
        }
    }
    // The person's nation (or second nation) in the wanted group.
    const std::uint32_t nation = who.nation();
    c.set(4, nation);
    if (compare(c, Nation{c, nation}.group(), wanted) == 0) return 10;
    const std::uint32_t second = who.second_nation();
    c.set(4, second);
    if (nonzero_logical(c, second) && compare(c, Nation{c, second}.group(), wanted) == 0) return 10;
    // Otherwise the relation kept for the person and group, if any.
    const Stats stats{c, stats_address};
    const std::uint32_t relations = stats.relations();
    c.set(4, relations);
    if (!nonzero_logical(c, relations)) return 0;
    const std::uint32_t found = find_relation(c, stats, frame.at(0x38), person, group, 0x10309478);
    if (!nonzero_logical(c, found)) return 0;
    return std::uint32_t(std::int32_t(Relation{c, found}.strength()));
}

std::uint32_t group_score(Cpu& c, std::uint32_t person, std::uint32_t group) {
    const Person who{c, person};
    const std::uint32_t nation = who.nation();
    leave(c, {{9, nation}, {8, c.toc(slot::regional_nation_4)}, {7, c.toc(slot::regional_nation_3)},
              {6, c.toc(slot::regional_nation_2)}});
    if (!nonzero_logical(c, nation)) return 0;
    const std::int32_t wanted = std::int8_t(group);
    const std::int32_t id = Nation{c, nation}.id();
    c.set(10, std::uint32_t(id));
    if (const auto score = regional(c, id, wanted, 500, 1000)) return *score;
    if (compare(c, Nation{c, nation}.first_group(), wanted) == 0) return 500;
    if (compare(c, Nation{c, nation}.second_group(), wanted) == 0) return 300;
    const std::uint32_t second = who.second_nation();
    c.set(4, second);
    if (!nonzero_logical(c, second)) return 0;
    const std::int32_t second_id = Nation{c, second}.id();
    c.set(9, std::uint32_t(second_id));
    if (const auto score = regional(c, second_id, wanted, 300, 500)) return *score;
    c.set(5, std::uint32_t(wanted));
    if (compare(c, Nation{c, second}.first_group(), wanted) == 0) return 300;
    const std::int32_t last = Nation{c, second}.second_group();
    c.set(4, std::uint32_t(last - wanted));
    return when_equal(last, wanted, 100);
}

void add_relation(Cpu& c, std::uint32_t stats_address, std::uint32_t person, std::uint32_t group, std::uint32_t strength,
                  std::uint32_t sort) {
    Frame frame(c, 0x10309680, 0xb0, {.first_gpr = 24});
    keep(c, {{31, c.toc(slot::success)}, {26, stats_address}, {24, c.toc(slot::message)}, {27, person},
             {25, c.toc(slot::pool)}, {28, group}, {30, sort}});
    Kept<29> kept_strength{c, strength};
    Program program{c};
    const Stats stats{c, stats_address};
    const std::uint32_t pool = c.toc(slot::pool);
    // Out of memory: the assertion report, then the program ends.
    const auto fail = [&](std::uint32_t at, std::int32_t name, std::int32_t extension, std::uint32_t line,
                          std::uint32_t back) {
        report(c, pool, at, frame.at(name), frame.at(extension), line, false);
        program.fatal_exit(std::uint32_t(-1), back);
    };
    if (!nonzero_logical(c, stats.relations())) {
        stats.relations() = program.malloc(100 * Relation::size, 0x103096c4);
        if (!nonzero_logical(c, stats.relations())) fail(0x103096e8, 0x68, 0x40, 0x11fd, 0x1030972c);
        stats.relation_capacity() = 100;
    }
    const std::int32_t capacity = stats.relation_capacity();
    if (compare(c, capacity, stats.relation_count()) == 0) {
        stats.relations() = program.realloc(stats.relations(), std::uint32_t(capacity + 100) * Relation::size, 0x10309754);
        if (!nonzero_logical(c, stats.relations())) fail(0x10309778, 0x48, 0x38, 0x1208, 0x103097bc);
        stats.relation_capacity() = stats.relation_capacity() + 100;
    }
    const std::int32_t count = stats.relation_count();
    const std::int32_t id = Person{c, person}.id();
    const Relation entry = stats.relation(count);
    entry.person() = id;
    entry.target() = std::int8_t(group);
    if (compare(c, std::int8_t(strength), 10) > 0) kept_strength = 10;
    entry.strength() = std::int8_t(std::uint32_t(kept_strength));
    stats.relation_count() = count + 1;
    leave(c, {{3, std::uint32_t(count)}, {4, stats.relations()}, {5, std::uint32_t(id)}});
    if (nonzero(c, std::int8_t(sort)))
        program.qsort(stats.relations(), std::uint32_t(count + 1), Relation::size, c.toc(slot::compare_relations_tv),
                      0x1030983c);
}

void remove_relation(Cpu& c, std::uint32_t stats_address, std::uint32_t person, std::uint32_t group) {
    Frame frame(c, 0x10309850, 0x50, {.first_gpr = 31});
    keep(c, {{31, stats_address}});
    const Stats stats{c, stats_address};
    const std::uint32_t relations = stats.relations();
    leave(c, {{3, relations}});
    if (!nonzero_logical(c, relations)) return;
    const std::uint32_t found = find_relation(c, stats, frame.at(0x38), person, group, 0x10309894);
    if (!nonzero_logical(c, found)) return;
    // Close the gap: the entries after it move down one; the last becomes
    // empty (person -1, target 0; its strength byte is left).
    const std::uint32_t offset = found - stats.relations();
    std::int32_t index = std::int32_t(offset >> 3);
    std::int32_t count;
    while (compare(c, index, (count = stats.relation_count()) - 1) < 0) {
        const Relation to = stats.relation(index), from = stats.relation(index + 1);
        to.person() = from.person().get();
        to.target() = from.target().get();
        to.strength() = from.strength().get();
        ++index;
    }
    const Relation last = stats.relation(count - 1);
    last.person() = -1;
    last.target() = 0;
    stats.relation_count() = count - 1;
    leave(c, {{3, std::uint32_t(count)}, {4, 0}, {5, std::uint32_t(-1)}, {6, std::uint32_t(index)}});
}

void add_to_relation(Cpu& c, std::uint32_t stats_address, std::uint32_t person, std::uint32_t group,
                     std::uint32_t amount, std::uint32_t sort) {
    Frame frame(c, 0x10309940, 0x60, {.first_gpr = 27});
    keep(c, {{27, stats_address}, {28, person}, {29, group}, {30, amount}, {31, sort}});
    const Stats stats{c, stats_address};
    const std::uint32_t relations = stats.relations();
    c.set(8, relations);
    if (nonzero_logical(c, relations)) {
        const std::uint32_t found = find_relation(c, stats, frame.at(0x38), person, group, 0x1030999c);
        if (nonzero_logical(c, found)) {
            // At most 10.
            const Relation relation{c, found};
            relation.strength() = std::int8_t(std::uint32_t(relation.strength().get()) + amount);
            if (compare(c, relation.strength(), 10) > 0) relation.strength() = 10;
            return;
        }
        enter(c, 0x103099e4, {stats_address, person, group, amount, sort});
    } else {
        enter(c, 0x10309974, {stats_address, person, group, amount, sort});
    }
    add_relation(c, stats_address, person, group, amount, sort);
}

void merge_history_rows(Cpu& c) {
    // Frameless: stmw r24,-0x20(r1) on entry, lmw on return. The C++ leaves
    // r24-r31 as they were, but the saved words are stack-image bytes.
    for (unsigned n = 24; n < 32; ++n) c.put32(c.sp() - 0x20 + (n - 24) * 4, c.r(n));
    const std::uint32_t count_slot = c.toc(slot::history_count), rows_slot = c.toc(slot::history_rows);
    leave(c, {{3, rows_slot}, {4, count_slot}});
    // Rows sorted by player and year: a player's two clubs of one season are
    // put so that the club he also plays for the next season comes second.
    std::uint32_t offset = HistoryRow::size;
    for (std::int32_t index = 1;; ++index, offset += HistoryRow::size) {
        const std::int32_t count = c.i32(count_slot);
        c.set(5, std::uint32_t(count));
        if (compare(c, index, count - 2) >= 0) break;
        const std::uint32_t rows = c.u32(rows_slot);
        const HistoryRow previous{c, rows + offset - HistoryRow::size}, row{c, rows + offset},
            next{c, rows + offset + HistoryRow::size};
        const std::uint32_t player = previous.player();
        if (player != row.player()) continue;
        const std::int16_t year = previous.year();
        c.set(9, extended(year));
        if (year == next.year() || year != row.year()) continue;
        const std::uint32_t club = previous.club();
        c.set(10, club);
        if (club == row.club() || club != next.club() || player != next.player()) continue;
        const std::uint32_t first = previous.first();
        const std::uint8_t loan = previous.loan(), appearances = previous.appearances(), goals = previous.goals();
        previous.first() = row.first().get();
        previous.player() = row.player().get();
        previous.year() = row.year().get();
        previous.club() = row.club().get();
        previous.loan() = row.loan().get();
        previous.appearances() = row.appearances().get();
        previous.goals() = row.goals().get();
        row.first() = first;
        row.player() = player;
        row.year() = year;
        row.club() = club;
        row.loan() = std::int8_t(loan);
        row.appearances() = appearances;
        row.goals() = goals;
        leave(c, {{6, appearances}, {7, goals}, {9, rows}});
    }
    c.set(8, offset);
}

std::uint32_t compare_relations(Cpu& c, std::uint32_t a, std::uint32_t b) {
    if (!nonzero_logical(c, a)) return std::uint32_t(-1);
    if (!nonzero_logical(c, b)) return 1;
    const Relation left{c, a}, right{c, b};
    const std::int32_t person = left.person();
    c.set(5, std::uint32_t(person));
    if (compare(c, person, right.person()) != 0) return std::uint32_t(person) - std::uint32_t(right.person().get());
    // -1 matches any target.
    const std::int32_t target = left.target();
    if (compare(c, target, -1) == 0) return 0;
    const std::int32_t other = right.target();
    if (compare(c, other, -1) == 0) return 0;
    return std::uint32_t(target) - std::uint32_t(other);
}

std::uint32_t compare_history_by_club(Cpu& c, std::uint32_t a, std::uint32_t b) {
    if (!nonzero_logical(c, a)) return 1;
    if (!nonzero_logical(c, b)) return std::uint32_t(-1);
    const HistoryRow left{c, a}, right{c, b};
    const std::uint32_t club = left.club();
    c.set(5, club);
    if (const auto order = compare_logical(c, club, right.club())) return std::uint32_t(order);
    const std::uint32_t player = left.player(), other = right.player();
    c.set(5, player);
    if (compare_logical(c, player, other) > 0) return 1;
    // The original's branch-free sign of (player - other) | (other - player):
    // -1 when they differ, 0 when equal (player <= other here).
    return std::uint32_t(std::int32_t((player - other) | (other - player)) >> 31);
}

std::uint32_t compare_history_by_player(Cpu& c, std::uint32_t a, std::uint32_t b) {
    if (!nonzero_logical(c, a)) return 1;
    if (!nonzero_logical(c, b)) return std::uint32_t(-1);
    const HistoryRow left{c, a}, right{c, b};
    const std::uint32_t player = left.player();
    c.set(5, player);
    if (const auto order = compare_logical(c, player, right.player())) return std::uint32_t(order);
    const std::int32_t year = left.year();
    c.set(5, std::uint32_t(year));
    if (const auto order = compare(c, year, right.year())) return std::uint32_t(order);
    const std::uint32_t first = right.first();
    c.set(4, first);
    return left.first() - first;
}
}
