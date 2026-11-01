#include "european_cup_detail.hpp"
#include "guest_fpu.hpp"
#include <bit>

namespace cm::aot::european_cup {
using namespace detail;
namespace {
// Group sizes of the draw tables: 8 groups of 4, with 2 halves of 4 groups.
constexpr std::int32_t group_size = 4, group_count = 8, pot_entry_size = 12;
// A draw-pot entry: club, partner (a club of the same nation), then its seed
// tier: 0 undecided, 1 or 2 the half of the draw it goes to.
constexpr std::uint32_t pot_partner = 4, pot_tier = 8;
// A second-draw entry: club, nation count, group, winner (0) or runner-up (1).
constexpr std::uint32_t second_count = 4, second_group = 5, second_place = 6;

std::uint32_t nation_of(Cpu& c, std::uint32_t club) { return Club{c, club}.nation(); }

Stream stream(Cpu& c, unsigned cursor, unsigned end, unsigned start, unsigned seed) {
    return {c, c.r(cursor), c.r(end), c.r(start), c.r(seed)};
}

// One club entering a group phase: a reputation point, the phase's money and its credit.
void enter_group_club(Cpu& c, std::uint32_t self, std::uint32_t club, std::uint32_t money, std::uint32_t reputation_variable,
                      std::uint32_t finances_variable, std::uint32_t event, std::uint32_t back_points,
                      std::uint32_t back_finance, std::uint32_t back_pay, std::uint32_t back_credit) {
    c.set({{3, c.u32(reputation_variable)}, {4, c.u32(club + 0x2c)}, {5, 1}});
    c.call(0x104b79a0, back_points);
    c.set({{3, c.u32(finances_variable)}, {4, club}});
    c.call(0x10103510, back_finance);
    c.set(4, money);
    c.call(0x100f71f0, back_pay);
    c.set({{3, self}, {4, club}, {6, money}, {5, 0}, {7, 0xffffffff}, {8, event}, {9, 0}, {10, 0xfffffffe}});
    c.call(0x10452010, back_credit);
}

// Constructs a group stage object with its fixed format.
void build_group(Cpu& c, const Frame& f, const Competition& cup, std::uint32_t stage, std::uint32_t clubs, std::uint32_t schedule,
                 std::uint32_t id, unsigned date_a, unsigned date_b, std::uint32_t back) {
    c.put32(f.at(0x38), 3); c.put32(f.at(0x3c), 1); c.put32(f.at(0x40), 8);
    c.put32(f.at(0x44), f.at(date_a)); c.put32(f.at(0x48), f.at(date_b));
    c.put32(f.at(0x4c), extended(cup.year()));
    c.put32(f.at(0x50), id);
    c.put32(f.at(0x54), extended(c.i16(f.at(0x78))));
    c.put32(f.at(0x58), 0xf); c.put32(f.at(0x5c), 2); c.put32(f.at(0x60), 0); c.put32(f.at(0x64), 8);
    c.put32(f.at(0x68), 0xffffffff); c.put32(f.at(0x6c), 0); c.put32(f.at(0x70), 2);
    c.set({{3, stage}, {4, cup.address}, {5, 4}, {6, clubs}, {7, 2}, {8, cup.definition()}, {9, schedule}, {10, extended(c.i16(f.at(0x7a)))}});
    c.call(0x1056dc60, back);
}

// Appends a losing group team to the other competition's entrants, entered at route 5.
void pass_to_uefa_cup(Cpu& c, std::uint32_t other, std::uint32_t qualifier_slot, std::uint32_t club) {
    c.put32(qualifier_slot, club);
    c.put8(qualifier_slot + 4, 0); c.put8(qualifier_slot + 5, 0);
    const auto count = std::uint32_t(std::int32_t(c.i16(other + 0x36)));
    c.put32(c.u32(other + 0x14) + (count << 3), c.u32(qualifier_slot));
    const auto list = c.u32(other + 0x14);
    c.put16(other + 0x36, count + 1);
    c.put8(list + (count << 3) + 4, 5);
}
}

void advance_stage(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x104ec210, 0x70, {.first_gpr = 30});
    keep(c, {{31, c.toc(slot::pool)}, {30, c.toc(slot::message)}});
    const Competition cup{c, address};
    const auto count = std::uint32_t(cup.stage_count().get()), last = std::uint32_t(cup.last_stage().get());
    c.set({{4, count}, {5, last}});
    if (compare(c, std::int32_t(last), std::int32_t(count - 1)) >= 0) {
        assertion(c, f, 0x104ec254, 0x40, 0x38, 0x495);
        c.set(3, c.toc(slot::flag));
        return;
    }
    cup.last_stage() = std::int32_t(last + 1);
    const auto stage = std::int32_t(cup.last_stage());
    if (compare(c, stage, 8) == 0) {
        c.set(4, 0);
        c.link(0x104ec2d8);
        second_group_draw(c, address, 0);
    } else if (stage < 8) {
        if (compare(c, stage) == 0) { c.link(0x104ec2cc); first_group_draw(c, address); }
    } else if (compare(c, stage, 12) == 0) {
        c.link(0x104ec2e0);
        knockout_draw(c, address);
    }
}

void first_group_draw(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x104ec300, 0x1c0, {.first_gpr = 13});
    keep(c, {{31, address}, {14, c.toc(slot::doubles)}, {27, c.toc(slot::rng_seed)}, {15, 0}, {28, c.toc(slot::rng_start)},
             {29, c.toc(slot::rng_end)}, {30, c.toc(slot::rng_cursor)}, {13, c.toc(slot::finances)}});
    const Competition cup{c, address};
    Program p{c};
    const auto rng = stream(c, 30, 29, 28, 27);
    const auto toc = c.r(2);
    c.set({{7, c.u16(toc - 0x3718)}, {6, c.u16(toc - 0x3716)}, {5, c.u16(toc - 0x3714)}});
    c.put16(f.at(0x80), c.r(7)); c.put16(f.at(0x82), c.r(6));
    c.put16(f.at(0x7c), c.r(5)); c.put16(f.at(0x7e), c.u16(toc - 0x3712));
    c.set(17, p.call(0x10775200, 0x104ec35c, {0x20, pot_entry_size}));
    const auto pot = c.r(17);
    if (compare(c, std::int32_t(pot)) == 0) { reloading_assertion(c, f, 0x104ec378, 0x12c, 0xa4, 0x4cc); return; }

    // The sixteen top seeds, then the qualifiers' round winners.
    const auto entrants = cup.entrants();
    for (std::uint32_t i = 0; i < 16; ++i) {
        c.put32(pot + i * pot_entry_size, c.u32(entrants + i * 8));
        c.put32(pot + i * pot_entry_size + pot_partner, 0);
    }
    Kept<15> filled(c, 16);
    keep(c, {{16, pot + 12 * pot_entry_size}});
    c.set({{3, 16}, {4, 0}, {5, 16 * pot_entry_size}, {6, pot + 15 * pot_entry_size}, {7, 0}, {8, entrants}, {9, 0x68}, {10, 0x60}});
    for (;;) {
        c.set(3, extended(cup.qualifier_count()));
        if (compare(c, std::int32_t(c.r(7)), std::int32_t(c.r(3))) >= 0) break;
        const auto q = cup.qualifiers() + c.r(4);
        c.set(6, q);
        c.set(3, c.u8(q + 5));
        if (compare(c, std::int32_t(c.r(3)), 1) == 0) {
            c.set(3, c.u32(q));
            const auto entry = pot + c.r(5);
            c.set(6, entry);
            c.set(5, c.r(5) + pot_entry_size);
            filled = filled + 1;
            c.put32(entry, c.r(3));
            c.put32(entry + pot_partner, 0);
        }
        c.set({{4, c.r(4) + 8}, {7, c.r(7) + 1}});
    }
    if (compare(c, std::int16_t(std::uint32_t(filled)), 32) < 0) {
        reloading_assertion(c, f, 0x104ec56c, 0x10c, 0x9c, 0x4ee);
        c.set(4, 0);
        cup.last_stage() = -1;
        p.call(0x10775170, 0x104ec5c4, {pot});
        c.put8(address + 0xb0, 0);
        return;
    }
    c.set({{3, pot}, {4, 0x20}, {5, pot_entry_size}, {6, c.toc(slot::compare_seeds)}});
    c.call(0x10768300, 0x104ec5e4);

    // Each club's partner: the next club of its nation without one.
    std::uint32_t scan = c.r(9), scan_offset = c.r(5);
    for (std::uint32_t i = 0; i < 32; ++i) {
        const auto entry = pot + i * pot_entry_size;
        c.set(7, entry);
        if (compare_logical(c, c.u32(entry + pot_partner)) != 0) { c.set({{8, i + 1}, {6, (i + 1) * pot_entry_size}}); compare(c, std::int32_t(i + 1), 32); continue; }
        scan = i + 1;
        scan_offset = scan * pot_entry_size;
        if (compare(c, std::int32_t(scan), 32) < 0) {
            for (; scan < 32; ++scan, scan_offset += pot_entry_size) {
                const auto other = pot + scan_offset;
                if (compare_logical(c, c.u32(other + pot_partner)) != 0) continue;
                if (compare_logical(c, nation_of(c, c.u32(entry)), nation_of(c, c.u32(other))) != 0) continue;
                c.put32(entry + pot_partner, c.u32(other));
                c.put32(other + pot_partner, c.u32(entry));
                break;
            }
        }
        c.set({{8, i + 1}, {6, (i + 1) * pot_entry_size}});
        compare(c, std::int32_t(i + 1), 32);
    }
    c.set({{5, scan_offset}, {9, scan}});

    c.set(19, p.call(0x10775200, 0x104ec684, {0x20, 4}));
    const auto table = c.r(19);
    if (compare(c, std::int32_t(table)) == 0) {
        reloading_assertion(c, f, 0x104ec6a0, 0xec, 0x94, 0x515);
        c.set({{3, 0xffffffff}, {4, c.toc(slot::flag)}});
        c.call(0x1076f7d0, 0x104ec6f0);
    }

    // Seed rows: each row deals eight of the pot to the eight groups, half to
    // each half of the draw, so partners land in different halves.
    Kept<22> row_index(c, 0);
    Kept<26> row_base(c, 0);
    Kept<15> second_half_left(c, c.r(15));
    Kept<16> first_half_left(c, c.r(16));
    Kept<18> position(c, c.r(18));
    Kept<20> roll(c, c.r(20));
    Kept<21> dealt(c, c.r(21));
    Kept<23> second_seats(c, c.r(23));
    Kept<24> all_seats(c, c.r(24));
    Kept<25> drawn(c, c.r(25));
    do {
        first_half_left = 4;
        second_half_left = 4;
        const std::uint32_t row = row_index, base = row_base;
        for (std::uint32_t j = 0; j < 8; ++j) {
            const auto tier = c.i8(pot + (j + base) * pot_entry_size + pot_tier);
            if (compare(c, tier, 2) == 0) second_half_left = second_half_left - 1;
            else if (compare(c, tier, 1) == 0) first_half_left = first_half_left - 1;
        }
        c.set({{4, 8}, {3, (7 + base) * pot_entry_size}});
        dealt = 0;
        do {
            rng.advance(0x104ec804);
            const auto draw_position = [&] {
                position = base + std::uint32_t(signed_remainder(c, rng.sample(), 3));
                drawn = pot + position * pot_entry_size;
            };
            draw_position();
            while (compare_logical(c, c.u32(drawn)) == 0) {
                rng.advance(0x104ec858);
                draw_position();
            }
            const std::uint32_t entry = drawn;
            if (compare(c, c.i8(entry + pot_tier)) == 0) {
                second_seats = signed_byte(second_half_left);
                all_seats = second_seats + signed_byte(first_half_left);
                const auto seats = std::int32_t(all_seats);
                if (compare(c, seats) == 0) roll = 0;
                else {
                    rng.advance(0x104ec8e0);
                    // Both counts are signed bytes, so the sum never leaves the
                    // small range; the original's wide-range branch can't run.
                    c.set(3, 0x10000); compare(c, seats, 0xffff);
                    c.set(3, 0xffff0000); compare(c, seats, -0xffff);
                    const auto value = rng.sample();
                    c.set(3, value);
                    compare(c, seats);
                    const auto remainder = std::uint32_t(std::int64_t(std::int32_t(value)) % seats);
                    roll = seats > 0 ? remainder : 0 - remainder;
                }
                if (compare(c, std::int32_t(roll), std::int32_t(second_seats)) < 0) {
                    second_half_left = second_half_left - 1;
                    c.put8(entry + pot_tier, 2);
                } else {
                    first_half_left = first_half_left - 1;
                    c.put8(entry + pot_tier, 1);
                }
                // The partner, when it sits in this same row, takes the other half.
                const auto partner = c.u32(entry + pot_partner);
                c.set(6, partner);
                if (compare_logical(c, partner) != 0) {
                    c.set({{4, pot}, {3, 0}, {5, 0}});
                    for (std::uint32_t i = 0; i < 32; ++i) {
                        if (compare_logical(c, c.u32(c.r(4)), partner) != 0) {
                            c.set({{5, c.r(5) + pot_entry_size}, {4, c.r(4) + pot_entry_size}, {3, c.r(3) + 1}});
                            continue;
                        }
                        const auto index = c.r(3);
                        c.set(4, std::uint32_t(over_eight(index)));
                        bool settled = false;
                        if (compare(c, std::int32_t(c.r(4)), over_eight(position)) == 0) {
                            const auto tier = c.u8(entry + pot_tier);
                            c.set(4, tier);
                            if (compare(c, std::int32_t(tier), 2) == 0 && compare(c, std::int8_t(first_half_left)) == 0) settled = true;
                            if (!settled && compare(c, std::int8_t(tier), 1) == 0 && compare(c, std::int8_t(second_half_left)) == 0) settled = true;
                            if (!settled) {
                                if (compare(c, std::int8_t(tier), 2) == 0) first_half_left = first_half_left - 1;
                                else second_half_left = second_half_left - 1;
                            }
                        }
                        if (!settled) {
                            const auto own = signed_byte(c.u8(entry + pot_tier));
                            const auto other_tier = own == 2 ? 1u : 2u;
                            c.set({{4, 2 - own}, {5, own - 2}, {3, other_tier}});
                            c.put8(pot + index * pot_entry_size + pot_tier, other_tier);
                        }
                        break;
                    }
                }
            }
            // Tier 1 sits in slots 0-3 of the row's groups, the rest in 4-7.
            const auto tier = signed_byte(c.u8(entry + pot_tier));
            const std::uint32_t first = tier == 1 ? 0 : 4;
            std::uint32_t slot = first, offset = first << 2;
            c.set({{3, first + 4}, {4, 1 - tier}});
            compare(c, std::int32_t(first), std::int32_t(first + 4));
            bool placed = false;
            for (std::uint32_t tries = 0; tries < 4 && !placed; ++tries) {
                c.set({{5, slot}, {4, offset}});
                if (compare_logical(c, c.u32(table + ((row + offset) << 2))) == 0) {
                    const auto at = row + (slot << 2);
                    c.set({{4, c.u32(entry)}, {3, at << 2}});
                    c.put32(table + (at << 2), c.r(4));
                    c.put32(entry, 0);
                    placed = true;
                } else { ++slot; offset += 4; }
            }
            if (!placed) c.set({{5, slot}, {4, offset}});
            dealt = dealt + 1;
        } while (compare(c, std::int32_t(dealt), 8) < 0);
        row_index = row + 1; row_base = base + 8;
    } while (compare(c, std::int32_t(row_index), 4) < 0);
    p.call(0x10775170, 0x104ecb98, {pot});

    // Within each half, swap a club away from a group that holds its nation.
    const auto club_at = [&](std::uint32_t index) { return c.u32(table + (index << 2)); };
    for (std::uint32_t half = 0; half < 2; ++half) {
        const auto start = half * 4;
        c.set({{5, half}, {4, start}, {8, start + 4}});
        for (std::uint32_t slot = 0; slot < group_size; ++slot) {
            c.set(6, slot);
            for (std::uint32_t group = start; group < start + 4; ++group) {
                c.set(7, group);
                const auto row = group << 2;
                c.set(3, row);
                bool shared = false;
                for (std::uint32_t k = 0; k < group_size && !shared; ++k) {
                    c.set(9, k);
                    if (compare(c, std::int32_t(slot), std::int32_t(k)) == 0) continue;
                    const auto own = nation_of(c, club_at(slot + row));
                    c.set({{10, own}, {9, club_at(row + k)}});
                    shared = compare_logical(c, own, nation_of(c, c.r(9))) == 0;
                }
                if (compare(c, std::int8_t(shared)) == 0) continue;
                for (std::uint32_t other = start; other < start + 4; ++other) {
                    c.set(9, other);
                    if (compare(c, std::int32_t(group), std::int32_t(other)) == 0) continue;
                    const auto own = nation_of(c, club_at(slot + row));
                    bool also_there = false;
                    for (std::uint32_t k = 0; k < group_size && !also_there; ++k) {
                        const auto theirs = nation_of(c, club_at((other << 2) + k));
                        c.set({{10, own}, {9, theirs}});
                        also_there = compare_logical(c, own, theirs) == 0;
                    }
                    c.set(9, std::uint32_t(also_there));
                    if (compare(c, std::int32_t(also_there)) != 0) continue;
                    const auto a = slot + row, b = slot + (other << 2);
                    c.set({{10, a << 2}, {9, b << 2}});
                    const auto kept = club_at(a);
                    c.put32(table + (a << 2), club_at(b));
                    c.put32(table + (b << 2), kept);
                    break;
                }
            }
            c.set({{3, (start + 4) << 2}, {7, start + 4}});
        }
    }
    c.set({{5, 2}, {4, 8}, {6, 4}});

    // Eight group stages. Each club gets a reputation point and the group
    // stage's money; each group gets its calendar, then the stage object.
    keep(c, {{14, 0x1b0000}});
    Kept<18> group(c, 0);
    Kept<15> first_club(c, 0);
    Kept<16> seat_offset(c, 0);
    Kept<17> stage_offset(c, 0);
    const auto reputation = c.toc(slot::reputation), finances = c.r(13);
    while (compare(c, std::int8_t(std::uint32_t(group)), group_count) < 0) {
        for (Kept<21> place(c, 0); compare(c, std::int8_t(std::uint32_t(place)), group_size) < 0; place = place + 1) {
            Kept<20> club(c, club_at(signed_byte(place) + first_club));
            enter_group_club(c, address, club, 0x1b7740, reputation, finances, 0x449, 0x104ece00, 0x104ece0c, 0x104ece14, 0x104ece38);
        }
        c.set({{3, address}, {4, group}, {5, f.at(0x7a)}, {6, f.at(0x78)}, {7, 0}});
        Kept<20> schedule(c, p.virtual_call(address, 0x44, 0x104ece68));
        Kept<21> stage(c, p.call(0x10773520, 0x104ece7c, {0xec}));
        if (compare(c, std::int32_t(std::uint32_t(stage))) != 0) {
            keep(c, {{22, 8}});
            build_group(c, f, cup, stage, table + seat_offset, schedule, group, 0x80, 0x7c, 0x104ecf0c);
        }
        c.put32(cup.stages() + stage_offset, stage);
        c.set(3, cup.stages());
        if (compare_logical(c, c.u32(c.r(3) + stage_offset)) == 0) { reloading_assertion(c, f, 0x104ecf38, 0xcc, 0x8c, 0x5c9); return; }
        p.call(0x10775170, 0x104ecf8c, {schedule});
        cup.last_stage() = std::int8_t(std::uint32_t(group));
        p.call(0x104a0e40, 0x104ecfa0, {c.u32(cup.stages() + stage_offset)});
        first_club = first_club + 4;
        seat_offset = seat_offset + 0x10;
        stage_offset = stage_offset + 4;
        group = group + 1;
    }
    p.call(0x10775170, 0x104ecfc4, {table});

    // The other cup takes the third-placed teams of the qualifying rounds.
    c.set(4, c.toc(slot::competitions));
    const auto other = c.u32(c.u32(c.r(4)) + (c.u32(c.toc(slot::uefa_cup)) << 2));
    c.set(3, other);
    if (compare_logical(c, other) == 0) { reloading_assertion(c, f, 0x104ecff8, 0xac, 0x84, 0x5d8); return; }
    const auto other_qualifiers = c.u32(other + 0xa4);
    std::uint32_t seat = (std::uint32_t(std::int32_t(c.i16(other + 0x3e))) - 0x18) << 3;
    c.set({{5, seat}, {8, 0}, {9, 0}});
    for (;;) {
        if (compare(c, std::int32_t(c.r(9)), cup.qualifier_count()) >= 0) break;
        const auto q = cup.qualifiers() + c.r(8);
        c.set(4, q);
        if (compare(c, std::int32_t(c.u8(q + 5)), 2) == 0) {
            const auto club = c.u32(q);
            const auto to = other_qualifiers + seat;
            seat += 8;
            pass_to_uefa_cup(c, other, to, club);
            c.put32(club + 0x14, c.u32(c.toc(slot::uefa_cup)));
            c.set({{5, seat}, {6, c.u32(other + 0x14)}, {7, 5}, {10, to}, {4, club}});
        }
        c.set({{8, c.r(8) + 8}, {9, c.r(9) + 1}});
    }
    c.set(3, other);
    p.virtual_call(other, 0x64, 0x104ed0f8);
}

std::uint32_t random_below(Cpu& c, std::uint32_t bound) {
    Frame f(c, 0x104ed110, 0x80, {.first_gpr = 20});
    Fpu fp(c);
    Kept<30> limit(c, bound);
    keep(c, {{25, c.toc(slot::doubles)}, {26, c.toc(slot::rng_seed)}, {27, c.toc(slot::rng_start)},
             {28, c.toc(slot::rng_end)}, {29, c.toc(slot::rng_cursor)}});
    if (compare(c, std::int32_t(bound)) == 0) return 0;
    const auto rng = stream(c, 29, 28, 27, 26);
    const auto small_sample = [&](std::uint32_t range) {
        c.set(3, c.u32(rng.cursor));
        compare(c, std::int32_t(range));
        const auto sum = rng.sample();
        c.set(3, sum);
        const auto remainder = std::int32_t(std::int64_t(std::int32_t(sum)) % std::int32_t(range));
        return std::int32_t(range) > 0 ? std::uint32_t(remainder) : 0u - std::uint32_t(remainder);
    };
    rng.advance(0x104ed164);
    c.set(3, 0x10000);
    const bool above = compare(c, std::int32_t(bound), 65535) > 0;
    if (!above) {
        c.set(3, 0xffff0000);
        if (compare(c, std::int32_t(bound), -65535) >= 0) return small_sample(bound);
    }
    const auto constants = c.r(25), cursor = c.u32(rng.cursor);
    c.put32(f.at(0x4c), bound ^ 0x80000000u); c.put32(f.at(0x48), 0x43300000);
    c.set_f(2, c.f64(constants + 8));
    c.set_f(1, fp.fsub(c.f64(f.at(0x48)), c.f(2)));
    c.set_f(1, fp.fdiv(c.f(1), c.f64(constants)));
    const auto sample = c.u16(cursor);
    const auto sum = std::uint32_t(sample) + c.u32(rng.seed);
    const auto wrapped_product = std::int32_t(bound * 10u);
    Kept<22> reduced(c, std::uint32_t(wrapped_product / 65535));
    const auto quotient = std::int32_t(sum) / 65535;
    const auto remainder = std::uint32_t(std::int32_t(sum) % 65535);
    c.put32(f.at(0x40), 0x43300000); c.put32(f.at(0x44), remainder ^ 0x80000000u);
    c.set_f(0, fp.fsub(c.f64(f.at(0x40)), c.f(2)));
    c.set_f(0, fp.fmul(c.f(1), c.f(0)));
    c.put64(f.at(0x38), fp.fctiwz(0, c.f(0)));
    Kept<31> sampled(c, c.u32(f.at(0x3c)));
    // Wrapping multiplication bounds the reduced range to [-32768,32768], so
    // the deeper inlined wide-range branches and the recursion can't run.
    const auto high = [](std::int32_t x) {
        return std::int32_t((std::int64_t(std::int32_t(0x80008001u)) * x) >> 32);
    };
    const auto sum_shift = std::int32_t(std::uint32_t(high(std::int32_t(sum))) + sum) >> 15;
    const auto reduced_shift = std::int32_t(std::uint32_t(high(wrapped_product)) + std::uint32_t(wrapped_product)) >> 15;
    leave(c, {{3, cursor}, {4, std::uint32_t(reduced_shift) >> 31}, {5, std::uint32_t(quotient) * 65535u},
              {6, std::uint32_t(sum_shift) >> 31}, {7, sum}, {8, sample}, {9, 0x43300000}});
    std::uint32_t additional = 0;
    if (compare(c, std::int32_t(std::uint32_t(reduced))) != 0) {
        rng.advance(0x104ed250);
        c.set(3, 0x10000); compare(c, std::int32_t(std::uint32_t(reduced)), 65535);
        c.set(3, 0xffff0000); compare(c, std::int32_t(std::uint32_t(reduced)), -65535);
        additional = small_sample(reduced);
    }
    c.set(24, additional);
    const auto correction = std::int32_t(additional + 5) / 10;
    sampled = std::uint32_t(sampled) + std::uint32_t(correction);
    if (compare(c, std::int32_t(std::uint32_t(sampled)), std::int32_t(bound - 1)) > 0) sampled = bound - 1;
    const auto signed_add = std::int32_t(additional + 5);
    const auto high_ten = std::int32_t((std::int64_t(0x66666667) * signed_add) >> 32) >> 2;
    c.set(4, std::uint32_t(high_ten) >> 31);
    return sampled;
}

void second_group_draw(Cpu& c, std::uint32_t address, std::uint32_t attempt) {
    Frame f(c, 0x104ed690, 0x1b0, {.first_gpr = 13});
    keep(c, {{15, address}, {13, c.toc(slot::nation_tables)}, {25, c.toc(slot::rng_seed)}, {26, c.toc(slot::rng_start)},
             {27, c.toc(slot::rng_end)}, {28, c.toc(slot::rng_cursor)}, {29, c.toc(slot::finances)}, {14, c.toc(slot::reputation)}});
    const Competition cup{c, address};
    Program p{c};
    const auto rng = stream(c, 28, 27, 26, 25);
    const auto reputation = c.r(14), finances = c.r(29);
    const auto toc = c.r(2);
    c.put8(f.at(0x1cf), attempt);
    c.set({{7, c.u16(toc - 0x3710)}, {6, c.u16(toc - 0x370e)}, {5, c.u16(toc - 0x370c)}});
    c.put16(f.at(0x84), c.r(7)); c.put16(f.at(0x86), c.r(6));
    c.put16(f.at(0x80), c.r(5)); c.put16(f.at(0x82), c.u16(toc - 0x370a));
    c.set(21, p.call(0x10775200, 0x104ed6f0, {0x10, 8}));
    const auto pool_address = c.r(21);
    if (compare(c, std::int32_t(pool_address)) == 0) { reloading_assertion(c, f, 0x104ed70c, 0x130, 0xa8, 0x61c); return; }

    // The eight winners and eight runners-up of the first group phase.
    keep(c, {{16, 0}, {19, 0}, {17, 0}, {18, 0}});
    while (compare(c, std::int8_t(c.r(19)), 8) < 0) {
        const auto group = c.r(19);
        p.call(0x1049ea00, 0x104ed77c, {c.u32(cup.stages() + c.r(16)), 0});
        const auto first_entry = pool_address + c.r(17);
        c.set({{3, c.u32(c.r(3))}, {5, first_entry}, {4, 1}});
        c.put32(first_entry, c.r(3));
        c.put8(first_entry + second_place, 0); c.put8(first_entry + second_group, group); c.put8(first_entry + second_count, 0);
        p.call(0x1049ea00, 0x104ed7a8, {c.u32(cup.stages() + c.r(16))});
        const auto second_entry = pool_address + ((c.r(18) + 1) << 3);
        const auto runner_up = c.u32(c.r(3));
        c.set({{4, runner_up}, {3, 1}, {5, second_entry}});
        keep(c, {{17, c.r(17) + 0x10}, {18, c.r(18) + 2}, {16, c.r(16) + 4}});
        c.put32(second_entry, runner_up);
        c.put8(second_entry + second_place, 1); c.put8(second_entry + second_group, group);
        keep(c, {{19, group + 1}});
        c.put8(second_entry + second_count, 0);
    }
    // Each entry counts the clubs of its nation among the sixteen, itself included.
    for (std::uint32_t i = 0; i < 16; ++i) {
        const auto entry = pool_address + i * 8;
        for (std::uint32_t j = 0; j < 16; ++j)
            if (nation_of(c, c.u32(entry)) == nation_of(c, c.u32(pool_address + j * 8)))
                c.put8(entry + second_count, c.u8(entry + second_count) + 1);
    }
    const auto last = pool_address + 15 * 8;
    c.compare_logical(0, 16, 16);
    c.set({{3, pool_address}, {4, 0x10}, {5, 8}, {6, c.toc(slot::compare_draw)}, {7, 0x80}, {8, last}, {9, 8}, {10, 0x10}});
    c.call(0x10768300, 0x104ed8c8);
    keep(c, {{17, 0}, {16, pool_address}});
    do {
        const auto tables = c.u32(c.u32(c.r(13)));
        const auto nation_table = c.u32(tables + (signed_byte(c.u8(address + 0x4a)) << 2));
        c.set({{3, nation_table}, {4, c.u32(c.r(16))}});
        c.call(0x105bd480, 0x104ed8f0);
        keep(c, {{17, c.r(17) + 1}, {16, c.r(16) + 8}});
    } while (compare(c, std::int32_t(c.r(17)), 0x10) < 0);
    c.set(20, p.call(0x10775200, 0x104ed90c, {0x10, 8}));
    const auto split = c.r(20);
    if (compare(c, std::int32_t(split)) == 0) { reloading_assertion(c, f, 0x104ed928, 0x110, 0xa0, 0x649); return; }
    c.set(19, p.call(0x10775200, 0x104ed980, {0x10, 8}));
    const auto seats = c.r(19);
    if (compare(c, std::int32_t(seats)) == 0) { reloading_assertion(c, f, 0x104ed99c, 0xf0, 0x98, 0x651); return; }

    // Split the pool in two: each entry goes with its group's other team on
    // the opposite side of the same row.
    const auto copy_entry = [&](std::uint32_t to, std::uint32_t from) {
        c.put32(to, c.u32(from));
        copy_bytes(c, to, from);
    };
    std::uint32_t low = 0, high = 0;
    c.set({{7, 0}, {3, 0}, {4, 0}, {5, 0}});
    for (std::uint32_t i = 0; i < 16; ++i) {
        const auto entry = pool_address + i * 8;
        c.set({{6, entry}, {8, c.u32(entry)}});
        if (compare_logical(c, c.r(8)) != 0) {
            if (compare(c, c.i8(entry + second_place)) == 0) {
                c.set(9, split + low);
                c.put32(c.r(9), c.r(8)); copy_bytes(c, c.r(9), entry);
            } else {
                c.set(9, split + high);
                c.put32(c.r(9) + 8, c.r(8)); copy_bytes(c, c.r(9) + 8, entry);
            }
            c.put32(entry, 0);
            std::uint32_t k = 0;
            for (;; ++k) {
                c.set(8, k);
                if (compare(c, std::int8_t(k), 16) >= 0) break;
                c.set(10, signed_byte(k));
                if (compare(c, std::int32_t(i), std::int32_t(signed_byte(k))) == 0) continue;
                c.set(9, signed_byte(c.u8(entry + second_group)));
                if (compare(c, std::int32_t(c.r(9)), c.i8(pool_address + k * 8 + second_group)) != 0) continue;
                const auto low_slot = split + low;
                const auto mate = pool_address + (signed_byte(k) << 3);
                if (compare_logical(c, c.u32(low_slot)) != 0) {
                    c.set(9, split + high);
                    copy_entry(c.r(9) + 8, mate);
                } else {
                    c.set(9, mate);
                    copy_entry(low_slot, mate);
                }
                c.put32(mate, 0);
                low += 0x10; high += 0x10;
                c.set({{4, low}, {5, high}, {6, 0}});
                break;
            }
        }
        c.set({{3, (i + 1) * 8}, {7, i + 1}, {4, low}, {5, high}});
    }

    // Deal the second group phase, four clubs per side of a group, in a
    // random group order for each club, avoiding its nation and its old group.
    Kept<13> placing(c, 0);
    Kept<16> best_group(c, c.r(16));
    Kept<17> fewest(c, c.r(17));
    Kept<18> half_end(c, c.r(18));
    Kept<22> first_index_held(c, c.r(22));
    Kept<23> shuffles(c, c.r(23));
    Kept<24> entry_held(c, c.r(24));
    Kept<30> half_start(c, c.r(30));
    c.put8(f.at(0x7c), 0); c.put8(f.at(0x7d), 1); c.put8(f.at(0x7e), 2); c.put8(f.at(0x7f), 3);
    c.set({{4, 1}, {3, 2}});
    keep(c, {{31, f.at(0x7c)}});
    const auto order = f.at(0x7c);
    c.put32(f.at(0x150), 0);
    do {
        fewest = 4;
        best_group = 0xffffffff;
        shuffles = 0;
        entry_held = split + c.u32(f.at(0x150));
        const std::uint32_t entry = entry_held;
        const std::uint32_t half = c.i8(entry + second_place) == 0 ? 0 : 2;
        half_start = half;
        half_end = half + 2;
        c.set(3, half == 0 ? 0xffffffff : 0);
        while (compare(c, std::int8_t(shuffles), 5) < 0) {
            rng.advance(0x104edbcc);
            const auto first_sum = rng.sample();
            const auto next = c.u32(rng.cursor) + 2;
            c.set(3, next);
            c.put32(rng.cursor, next);
            const auto end = c.u32(rng.end);
            c.set(5, first_sum);
            const auto sign = first_sum >> 31;
            c.set({{4, first_sum << 30}, {3, sign}});
            const auto first_index = std::uint32_t(std::int32_t(std::int8_t(std::rotl((first_sum << 30) - sign, 2) + sign)));
            first_index_held = first_index;
            if (compare_logical(c, next, end) > 0) {
                c.put32(rng.cursor, rng.start);
                c.call(refill, 0x104edc18);
                c.put32(rng.seed, c.r(3) & 0xffff);
            }
            shuffles = shuffles + 1;
            const auto second_sum = rng.sample();
            const auto second_sign = second_sum >> 31;
            const auto second_index = std::uint32_t(std::int32_t(std::int8_t(std::rotl((second_sum << 30) - second_sign, 2) + second_sign)));
            const auto a = c.u8(order + first_index), b = c.u8(order + second_index);
            c.put8(order + first_index, b);
            c.put8(order + second_index, a);
            c.set({{4, second_index}, {3, b}});
        }
        c.set(6, 0);
        while (compare(c, std::int8_t(c.r(6)), group_size) < 0) {
            const auto group = c.u8(order + signed_byte(c.r(6)));
            c.set({{5, 0}, {4, group}, {8, 0}, {7, 0}});
            const auto row = signed_byte(group) << 2;
            for (;;) {
                c.set(3, signed_byte(c.r(5)));
                if (compare(c, std::int32_t(c.r(3)), group_size) >= 0) break;
                const auto at = seats + ((signed_byte(c.r(5)) + row) << 3);
                c.set({{9, at}, {3, c.u32(at)}});
                if (compare_logical(c, c.r(3)) != 0) {
                    c.set(8, c.r(8) + 1);
                    const auto mine = signed_byte(c.u8(entry + second_group));
                    c.set({{10, mine}, {9, signed_byte(c.u8(at + second_group))}});
                    if (compare(c, std::int32_t(mine), std::int32_t(c.r(9))) == 0) { c.set(7, 1); break; }
                    const auto own = c.u32(entry);
                    c.set(9, own);
                    c.set(10, nation_of(c, c.r(3)));
                    c.set(3, nation_of(c, own));
                    if (compare_logical(c, c.r(10), c.r(3)) == 0) { c.set(7, 1); break; }
                }
                c.set(5, c.r(5) + 1);
            }
            const auto base = signed_byte(group) << 2;
            c.set(3, base);
            if (compare_logical(c, c.u32(seats + ((half_start + base) << 3))) != 0) {
                c.set(3, half_end + base);
                if (compare_logical(c, c.u32(seats + ((c.r(3) - 1) << 3))) != 0) c.set(7, 1);
            }
            if (compare(c, std::int8_t(c.r(7))) == 0) {
                c.set(3, signed_byte(c.r(8)));
                if (compare(c, std::int32_t(c.r(3)), std::int32_t(signed_byte(fewest))) < 0) {
                    fewest = c.r(8);
                    best_group = group;
                }
            }
            c.set(6, c.r(6) + 1);
        }
        const auto chosen = signed_byte(best_group);
        if (compare(c, std::int32_t(chosen), -1) != 0) {
            c.set(4, chosen << 2);
            auto target = seats + ((half_start + c.r(4)) << 3);
            c.set(3, target);
            if (compare_logical(c, c.u32(target)) != 0) {
                c.set(3, half_end + c.r(4));
                target = seats + ((c.r(3) - 1) << 3);
                c.set({{4, c.u32(entry)}, {3, target}});
            }
            copy_entry(target, entry);
            c.put32(entry, 0);
        }
        c.put32(f.at(0x150), c.u32(f.at(0x150)) + 8);
        c.set(3, c.u32(f.at(0x150)));
        placing = placing + 1;
    } while (compare(c, std::int32_t(placing), 0x10) < 0);

    // Whoever is left over means a retry (20 at most), then any free seat.
    for (std::uint32_t i = 0; i < 16; ++i) {
        const auto leftover = split + i * 8;
        c.set({{5, i * 8}, {7, leftover}, {6, c.u32(leftover)}});
        if (compare_logical(c, c.r(6)) == 0) continue;
        if (compare(c, c.i8(f.at(0x1cf)), 0x14) < 0) {
            p.call(0x10775170, 0x104ede3c, {seats});
            p.call(0x10775170, 0x104ede44, {split});
            p.call(0x10775170, 0x104ede4c, {pool_address});
            const auto next_attempt = signed_byte(c.u8(f.at(0x1cf)) + 1);
            enter(c, 0x104ede60, {address, next_attempt});
            second_group_draw(c, address, next_attempt);
            return;
        }
        c.set({{8, 0}, {4, 0}, {3, seats}});
        for (std::uint32_t free = 0; compare(c, std::int8_t(free), 0x10) < 0; ++free) {
            const auto seat = seats + free * 8;
            c.set(3, seat);
            if (compare_logical(c, c.u32(seat)) == 0) {
                c.set(3, seat);
                c.put32(seat, c.r(6)); copy_bytes(c, seat, leftover);
                break;
            }
            c.set({{4, (free + 1) * 8}, {3, seat + 8}, {8, free + 1}});
        }
    }
    c.set(5, 0x80);
    const auto clubs = p.call(0x10775130, 0x104eded8, {0x40});
    keep(c, {{22, 0}, {23, clubs}, {13, 0x120000}, {16, 0}, {17, 0}});
    for (unsigned i = 0; i < 16; ++i) c.put32(clubs + i * 4, c.u32(seats + i * 8));

    // Groups 8-11: the same as the first phase, with the second phase's money.
    const auto club_at = [&](std::uint32_t index) { return c.u32(seats + (index << 3)); };
    Kept<16> first_club(c, c.r(16));
    Kept<17> seat_offset(c, c.r(17));
    for (Kept<22> group(c, c.r(22)); compare(c, std::int8_t(std::uint32_t(group)), group_size) < 0; group = group + 1) {
        for (Kept<18> place(c, 0);;) {
            Kept<24> club(c, club_at(place + first_club));
            enter_group_club(c, address, club, 0x124f80, reputation, finances, 0x44a, 0x104edf90, 0x104edf9c, 0x104edfa4, 0x104edfc8);
            place = place + 1;
            if (compare(c, std::int32_t(std::uint32_t(place)), group_size) >= 0) break;
        }
        Kept<18> stage_index(c, signed_byte(group) + 8);
        c.set({{3, address}, {4, signed_byte(stage_index)}, {5, f.at(0x7a)}, {6, f.at(0x78)}, {7, 0}});
        Kept<24> schedule(c, p.virtual_call(address, 0x44, 0x104edffc));
        Kept<25> stage(c, p.call(0x10773520, 0x104ee010, {0xec}));
        if (compare(c, std::int32_t(std::uint32_t(stage))) != 0) {
            keep(c, {{26, extended(c.i16(f.at(0x78)))}, {27, extended(cup.year())}});
            build_group(c, f, cup, stage, clubs + seat_offset, schedule, signed_byte(stage_index), 0x84, 0x80, 0x104ee0a4);
        }
        const auto at = cup.stages() + first_club + 0x20;
        c.put32(at, stage);
        if (compare_logical(c, c.u32(at)) == 0) { reloading_assertion(c, f, 0x104ee0d4, 0xd0, 0x90, 0x715); return; }
        p.call(0x10775170, 0x104ee128, {schedule});
        cup.last_stage() = std::int32_t(std::uint32_t(stage_index));
        p.call(0x104a0e40, 0x104ee13c, {c.u32(cup.stages() + first_club + 0x20)});
        first_club = first_club + 4;
        seat_offset = seat_offset + 0x10;
    }
    p.call(0x10775170, 0x104ee15c, {pool_address});
    p.call(0x10775170, 0x104ee164, {seats});
    p.call(0x10775170, 0x104ee16c, {split});
    p.call(0x10775170, 0x104ee174, {clubs});

    // The first phase's third-placed teams drop to the other cup's entrants.
    const auto other = c.u32(c.u32(c.toc(slot::competitions)) + (c.u32(c.toc(slot::uefa_cup)) << 2));
    c.set(4, other);
    if (compare_logical(c, other) == 0) { reloading_assertion(c, f, 0x104ee1a8, 0xb0, 0x88, 0x729); return; }
    keep(c, {{16, other}});
    Kept<14> group(c, 0);
    Kept<13> stage_offset(c, 0);
    Kept<18> last_seat(c, (std::uint32_t(std::int32_t(c.i16(other + 0x3e))) - 1) << 3);
    do {
        c.set(4, 2);
        const auto stage = c.u32(cup.stages() + stage_offset);
        Kept<17> seat(c, c.u32(other + 0xa4) + last_seat);
        last_seat = last_seat - 8;
        p.call(0x1049ea00, 0x104ee228, {stage});
        const auto third = c.u32(c.r(3));
        keep(c, {{19, 0xffffffff}});
        pass_to_uefa_cup(c, other, seat, third);
        c.set({{4, 3}, {5, c.u32(other + 0x14)}, {6, 5}});
        c.put32(third + 0x14, c.u32(c.toc(slot::uefa_cup)));
        p.call(0x1049ea00, 0x104ee294, {c.u32(cup.stages() + stage_offset)});
        const auto fourth = c.u32(c.r(3));
        group = group + 1;
        stage_offset = stage_offset + 4;
        c.set(3, fourth);
        c.put32(fourth + 0x14, 0xffffffff);
    } while (compare(c, std::int32_t(std::uint32_t(group)), 8) < 0);
    c.set(3, other);
    p.virtual_call(other, 0x64, 0x104ee2bc);
}

void knockout_draw(Cpu& c, std::uint32_t address) {
    Frame f(c, 0x104ee2e0, 0xf0, {.first_gpr = 18});
    keep(c, {{19, address}, {24, c.toc(slot::nation_tables)}, {25, c.toc(slot::rng_seed)}, {26, c.toc(slot::rng_start)},
             {27, c.toc(slot::rng_end)}, {28, c.toc(slot::rng_cursor)}, {29, c.toc(slot::flag)}, {30, c.toc(slot::message)}, {31, c.toc(slot::pool)}});
    const Competition cup{c, address};
    Program p{c};
    const auto rng = stream(c, 28, 27, 26, 25);
    c.set(23, p.call(0x10775200, 0x104ee320, {8, 4}));
    const auto seats = c.r(23);
    if (compare(c, std::int32_t(seats)) == 0) { assertion(c, f, 0x104ee33c, 0x8c, 0x64, 0x762); return; }
    // Four distinct groups, in a random order: a permutation of 0..3.
    const auto draw = [&](std::uint32_t back) {
        rng.advance(back);
        return signed_byte(std::uint32_t(signed_remainder(c, rng.sample(), 2)));
    };
    Kept<20> fourth(c, draw(0x104ee39c));
    Kept<21> third(c, fourth);
    Kept<22> second(c, fourth);
    while (compare(c, std::int8_t(std::uint32_t(second)), std::int32_t(std::uint32_t(fourth))) == 0) second = draw(0x104ee3f8);
    for (;;) {
        c.set(3, signed_byte(third));
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(std::uint32_t(fourth))) != 0 &&
            compare(c, std::int32_t(c.r(3)), std::int8_t(std::uint32_t(second))) != 0) break;
        third = draw(0x104ee45c);
    }
    Kept<18> first(c, fourth);
    for (;;) {
        c.set(3, signed_byte(fourth));
        if (compare(c, std::int32_t(c.r(3)), std::int32_t(std::uint32_t(first))) != 0 &&
            compare(c, std::int32_t(c.r(3)), std::int8_t(std::uint32_t(second))) != 0 &&
            compare(c, std::int32_t(c.r(3)), std::int8_t(std::uint32_t(third))) != 0) break;
        fourth = draw(0x104ee4d0);
    }
    const auto qualifier = [&](std::uint32_t group, std::uint32_t place, std::uint32_t back) {
        const auto stage = c.u32(cup.stages() + group + 0x20);
        p.call(0x1049ea00, back, {stage, place});
        return c.u32(c.r(3));
    };
    first = first << 2;
    c.put32(seats, qualifier(first, 0, 0x104ee53c));
    second = signed_byte(second) << 2;
    c.put32(seats + 4, qualifier(second, 1, 0x104ee560));
    c.put32(seats + 8, qualifier(first, 1, 0x104ee57c));
    c.put32(seats + 0xc, qualifier(second, 0, 0x104ee598));
    first = signed_byte(third) << 2;
    c.put32(seats + 0x10, qualifier(first, 0, 0x104ee5bc));
    fourth = signed_byte(fourth) << 2;
    c.put32(seats + 0x14, qualifier(fourth, 1, 0x104ee5e0));
    c.put32(seats + 0x18, qualifier(first, 1, 0x104ee5fc));
    c.put32(seats + 0x1c, qualifier(fourth, 0, 0x104ee618));
    // The eight quarter-finalists join their nation's competition register.
    Kept<20> seat(c, seats);
    Kept<18> registered(c, 0);
    while (compare(c, std::int8_t(std::uint32_t(registered)), 8) < 0) {
        const auto tables = c.u32(c.r(24));
        c.set({{3, c.u32(c.u32(tables) + (signed_byte(c.u8(address + 0x4a)) << 2))}, {4, c.u32(seat)}});
        c.call(0x105bd480, 0x104ee64c);
        seat = seat + 4;
        registered = registered + 1;
    }
    c.set({{3, address}, {5, f.at(0x5a)}, {6, f.at(0x58)}, {4, 0xc}, {7, 0}});
    Kept<18> schedule(c, p.virtual_call(address, 0x44, 0x104ee680));
    Kept<20> stage(c, p.call(0x10773520, 0x104ee694, {0xb4}));
    if (compare(c, std::int32_t(std::uint32_t(stage))) != 0) {
        c.put32(f.at(0x38), 0xc); c.put32(f.at(0x3c), 2); c.put32(f.at(0x40), extended(c.i16(f.at(0x58))));
        c.put32(f.at(0x44), 0x14); c.put32(f.at(0x48), 1); c.put32(f.at(0x4c), 0); c.put32(f.at(0x50), 0); c.put32(f.at(0x54), 0);
        c.set({{4, address}, {5, 8}, {6, seats}, {7, extended(c.i16(f.at(0x5a)))}, {8, cup.definition()}, {9, schedule}, {10, extended(cup.year())}});
        c.call(0x104bc3c0, 0x104ee6f4);
    }
    c.set({{4, cup.stages()}, {3, seats}});
    c.put32(c.r(4) + 0x30, stage);
    c.call(0x10775170, 0x104ee704);
    p.call(0x10775170, 0x104ee70c, {schedule});
    c.set(3, cup.stages());
    if (compare_logical(c, c.u32(c.r(3) + 0x30)) == 0) { assertion(c, f, 0x104ee730, 0x6c, 0x5c, 0x78a); return; }
    // The eliminated clubs of the second phase leave the zone's list.
    keep(c, {{20, 0xffffffff}});
    Kept<21> group(c, 8);
    Kept<18> stage_offset(c, 0x20);
    while (compare(c, std::int8_t(std::uint32_t(group)), 12) < 0) {
        p.call(0x1049ea00, 0x104ee790, {c.u32(cup.stages() + stage_offset), 2});
        c.set(3, c.u32(c.r(3)));
        c.set(4, 3);
        c.put32(c.r(3) + 0x14, 0xffffffff);
        p.call(0x1049ea00, 0x104ee7a8, {c.u32(cup.stages() + stage_offset)});
        c.set(3, c.u32(c.r(3)));
        stage_offset = stage_offset + 4;
        group = group + 1;
        c.put32(c.r(3) + 0x14, 0xffffffff);
    }
    p.call(0x1047ef50, 0x104ee7d4, {c.u32(cup.stages() + 0x30), 0});
    cup.last_stage() = 12;
}
}
