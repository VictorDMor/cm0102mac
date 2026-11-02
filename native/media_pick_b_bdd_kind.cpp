// 0x1067e210, first half: the guards and the choice of the story kind and roll. The kind is a byte (stories
// 0x29-0x32); every path that picks one ends with a roll drawn from the shared stream.
#include "media_pick_b_bdd_detail.hpp"

namespace cm::aot::media {
using namespace detail;
using namespace detail::pick_b;

Bdd::Bdd(Cpu& cpu, std::uint32_t query_, std::uint32_t person_, std::uint32_t club_, std::uint32_t kind_, std::uint32_t extra_)
    : c(cpu), program{cpu}, frame(cpu, 0x1067e210, 0x240, {.first_gpr = 14}), fpu(cpu), pool(cpu), rng(Stream::world(cpu)),
      query(query_), person(person_), club(club_), extra(extra_),
      r14(cpu, cpu.toc(-0x7570)), r15(cpu, cpu.toc(-0x7b9c)), r20(cpu, cpu.toc(-0x7b80)), standing(cpu, cpu.r(23)),
      kind(cpu, kind_), roll(cpu, cpu.r(27)), item{cpu, frame.at(0xf4)}, numbered{item, {cpu, frame.at(0x38)}},
      subject{cpu, person_}, team{cpu, club_} {
    keep(c, {{16, c.toc(-0x7a64)}, {17, c.toc(-0x7b40)}, {18, c.toc(-0x7bf0)}, {19, query}, {21, c.toc(slot::floats)},
             {22, extra}, {24, rng.end}, {26, rng.start}, {28, club}, {29, rng.seed}, {30, person}, {31, rng.cursor}});
    compare(c, std::int32_t(person));  // or. r30,r4,r4
}

// True when the story may be considered: a player with a contract worth at least 3500, an owner, a club with a
// manager and an owner, and (unless the club's first check passes) a standing record.
bool Bdd::admits() {
    if (compare(c, std::int32_t(person)) == 0) return false;
    if (!nonzero_logical(c, player_record())) return false;
    c.set(3, player_record());
    if (compare(c, subject.player_record().reputation(), 0xdac) < 0) return false;
    if (!nonzero_logical(c, subject.nation())) return false;
    if (!nonzero_logical(c, club)) return false;
    if (!nonzero_logical(c, team.manager())) return false;
    if (!nonzero_logical(c, team.nation())) return false;
    const auto special = program.call(0x100c7d00, 0x1067e2b4, {club});
    if (compare(c, byte_word(special)) == 0) {
        if (!nonzero_logical(c, team.division())) return false;
        leave(c, {{3, c.toc(-0x7a0c)}, {4, c.u32(team.division())}});
        if (compare(c, c.i32(team.division()), c.i32(c.toc(-0x7a0c))) == 0) return false;
        standing = program.call(0x10082f90, 0x1067e2f8, {slot_value(-0x7b7c), person, club});
        if (!nonzero(c, std::int32_t(standing))) return false;
        compare(c, std::int32_t(standing_record().flags() & 0x2008));  // andi.: its result is not used (quirk)
    }
    return true;
}

// a > scale * b, both 16-bit ratings through the int-to-double stack temporaries (the pool's scale at
// `scale_offset`); leaves f1 = a and cr0 as fcmpo does.
bool Bdd::scaled_exceeds(std::uint32_t a_at, std::int32_t a, std::uint32_t b_at, std::int32_t b, std::uint32_t scale_offset) {
    const auto floats = c.toc(slot::floats);
    const double left = stacked_integer(c, b_at, signed_word(b));
    const double right = stacked_integer(c, a_at, signed_word(a));
    const double scaled = fpu.fmul(c.f64(floats + scale_offset), fpu.fsub(left, pool.bias));
    const double figure = fpu.fsub(right, pool.bias);
    c.set_f(1, figure);
    fpu.fcmp(0, figure, scaled);
    return figure > scaled;
}

// Players under 22 with byte 0x5e clear: story 0x2e when he is doing better than the club's size suggests (or is
// well regarded), otherwise 0x29 or 0x2a. False for everyone else; the later stories decide.
bool Bdd::dislikes() {
    if (compare(c, age(), 0x16) >= 0) return false;
    if (compare_logical(c, subject.ability()) != 0) return false;
    const auto club_rating = std::int32_t(team.reputation().get()), rating = std::int32_t(subject.player_record().reputation().get());
    if (scaled_exceeds(frame.at(0x1e8), rating, frame.at(0x1e0), club_rating, 0xc8)) {
        const auto grade = std::int32_t(subject.ambition().get());
        kind = std::uint32_t(grade);
        const auto first = draw_below(c, frame, fpu, rng, pool, {0x1067e67c, 0x1067e754, 0x1e0, 0x1e8, 0x1d8, 20}, grade);
        const auto bound = sample(0x1067e7d8) % 20;
        compare(c, first, bound);
        if (first > bound) {
            const auto busy = program.call(0x103eaea0, 0x1067e82c, {slot_value(-0x7b40), person, 0, 0});
            if (compare(c, std::int32_t(busy)) == 0) {
                pick_roll(0x2e, 0x1067e858, 6);
                return true;
            }
        }
    }
    if (compare(c, std::int32_t(standing_record().flags() & 1)) != 0) {
        const auto busy = program.call(0x103eaea0, 0x1067e8b4, {slot_value(-0x7b40), person, 0, 0});
        if (compare(c, std::int32_t(busy)) == 0) {
            const auto grade = std::int32_t(subject.ambition().get());
            const auto half = (grade + std::int32_t(std::uint32_t(grade) >> 31)) >> 1;  // srawi
            r20 = std::uint32_t(half);
            const auto draw = draw_below(c, frame, fpu, rng, pool, {0x1067e8fc, 0x1067e9d4, 0x1d8, 0x1e0, 0x1e8, 23}, half);
            // The high nibble of the player's byte in the table at -0x7b9c (indexed by his id * 100 + 12).
            const auto nibble = std::int32_t((c.u8(c.u32(r15) + subject.id() * 0x64 + 0xc) >> 4) & 0xf);
            if (compare(c, nibble, draw) >= 0) {
                pick_roll(0x2e, 0x1067ea7c, 6);
                return true;
            }
        }
    }
    const auto league = [&](std::uint32_t back) { return byte_word(program.call(0x102af680, back, {c.u32(c.r(16)), person})); };
    const bool eligible = compare(c, league(0x1067eac4), 1) == 0 || compare(c, league(0x1067eadc), 2) == 0;
    if (eligible && compare(c, subject.player_record().home_reputation(), 0x109a) > 0 &&
        compare(c, subject.player_record().home_reputation(), std::int32_t(subject.nation_record().rating().get()) * 0x1e) > 0) {
        pick_roll(0x29, 0x1067eb38, 4);
        return true;
    }
    pick_roll(0x2a, 0x1067eb90, 4);
    return true;
}

// Story 0x2f for an established player far from home; false otherwise.
bool Bdd::settled_rules() {
    if (compare(c, age(), 0x1e) > 0 && compare(c, byte_word(c.u8(player_record() + 0x2f)), 0xf) > 0) {
        const auto distance = program.call(0x10718130, 0x1067ebf8, {c.r(20), subject.joined_first(), subject.joined_second()});
        if (compare(c, std::int32_t(distance), 0x190) > 0 && compare(c, subject.player_record().reputation(), 0x1c52) > 0 &&
            compare(c, subject.sportsmanship().get(), 0xf) > 0) {
            if (compare(c, sample(0x1067ec40) % 3) == 0) {
                kind = 0x2f;
                return true;
            }
        }
    }
    return false;
}

// Stories 0x2c, 0x2d, 0x30, 0x31 by the contract's ratings against the owner's and the club's, else 0x2b.
bool Bdd::ambitions() {
    const auto owner = std::int32_t(subject.nation_record().rating().get());
    const auto club_rating = std::int32_t(team.reputation().get());
    if (scaled_exceeds(frame.at(0x1d8), std::int32_t(subject.player_record().home_reputation().get()), frame.at(0x1e0), owner, 0x100) &&
        scaled_exceeds(frame.at(0x1d8), std::int32_t(subject.player_record().world_reputation().get()), frame.at(0x1e0), owner, 0x108) &&
        compare(c, age(), 0x1c) < 0 &&
        scaled_exceeds(frame.at(0x1d8), club_rating, frame.at(0x1e0), owner, 0x98) &&
        compare_logical(c, subject.ability()) == 0) {
        const auto draw = sample(0x1067edc0) % 4;
        if (compare(c, draw) != 0 && compare_logical(c, subject.national_team()) == 0) {
            pick_roll(0x2c, 0x1067ee20, 3);
            return true;
        }
    }
    if (compare(c, age(), 0x18) < 0 && compare(c, subject.player_record().reputation(), 0x1770) < 0) {
        pick_roll(0x2d, 0x1067eea0, 3);
        return true;
    }
    const auto limit = 0x1b - sample(0x1067eefc) % 4;
    if (compare(c, age(), limit) < 0 && compare(c, subject.player_record().reputation(), 0x1f40) > 0 &&
        compare(c, subject.player_record().world_reputation(), 0x1b58) < 0) {
        const auto wage = sample(0x1067ef7c) % 20 + 0x9b;
        if (compare(c, subject.player_record().ability(), wage) > 0) {
            const auto bar = sample(0x1067efe8) % 5 + 10;
            if (compare(c, std::int32_t(subject.ability().get()), bar) < 0) {
                const auto fee = sample(0x1067f050) % 2000 + 0x1964;
                if (compare(c, std::int32_t(team.reputation().get()), fee) > 0) {
                    pick_roll(0x30, 0x1067f0bc, 3);
                    return true;
                }
            }
        }
    }
    if (compare(c, age(), 0x21) < 0 && compare(c, subject.player_record().reputation(), 0x2328) > 0 &&
        compare(c, subject.player_record().world_reputation(), 0x2134) > 0) {
        const auto wage = sample(0x1067f148) % 20 + 0xaf;
        if (compare(c, subject.player_record().ability(), wage) > 0) {
            const auto fee = sample(0x1067f1b4) % 2000 + 0x1d4c;
            if (compare(c, std::int32_t(team.reputation().get()), fee) > 0) {
                pick_roll(0x31, 0x1067f220, 3);
                return true;
            }
        }
    }
    pick_roll(0x2b, 0x1067f280, 8);
    return true;
}

// Chooses the story kind and its roll (kinds 0x33 and 0x34 come with the call and only draw the roll).
void Bdd::choose_kind() {
    const auto k = byte_word(kind);
    if (compare(c, k, 0x33) == 0 || compare(c, byte_word(kind), 0x34) == 0) {
        roll = std::uint32_t(sample(0x1067e414) % 4);
        return;
    }
    if (compare(c, sample(0x1067e468) % 3) != 0) {
        c.put32(frame.at(0x48), c.u32(r20));
        c.put32(frame.at(0x4c), c.u32(r20 + 4));
        const auto x = sample(0x1067e4d0);
        const auto when = std::uint32_t(x % 150 + 0x1e);
        program.call(0x10717890, 0x1067e51c, {frame.at(0x50), person + 0x4c, extended(std::int16_t(when)), std::uint32_t(x)});
        // today (S+0x48) before the date the call computed (S+0x50): by year, then by day.
        const auto year = c.u16(frame.at(0x4a)), later = c.u16(frame.at(0x52));
        bool before;
        if (compare_logical(c, year, later) < 0) before = true;
        else if (compare_logical(c, year, c.u16(frame.at(0x52))) > 0) before = false;
        else before = c.i16(frame.at(0x48)) < c.i16(frame.at(0x50));
        if (compare(c, before ? 1 : 0) != 0) {
            pick_roll(0x32, 0x1067e59c, 3);
            return;
        }
    }
    if (dislikes() || settled_rules()) return;
    ambitions();
}
}
