#include "shortlist_manager_squads_detail.hpp"

// 0x1035a9f0: the review of every candidate left after the picks
// (0x1035c2d4-0x1035ca60). A candidate at another club (with a second
// contract) is let go; one of the club is rated (r22, from his age and the
// club's standard; r27, a worth from his attributes and the club's
// reputation) and, by how he compares, offered out, marked for a primary
// contract or given a squad status.
namespace cm::aot::shortlist_manager::squads {
namespace {
void release(Assessment& a, std::uint32_t person, std::uint32_t back) {
    a.program.call(call::release_contract, back, {a.contracts(), person, a.club()});
}
// Released unless the club has its own staff (staff_flag) and the player's
// +0xc (inferred: his current ability) is 5000 or more.
void release_unless_kept(Assessment& a, std::uint32_t person, std::uint32_t back) {
    Cpu& c = a.c;
    if (nonzero(c, a.local8(local::staff_flag)) && compare(c, c.i16(c.u32(person + 0x14) + 0xc), 5000) >= 0) return;
    release(a, person, back);
}
void set_status(Cpu& c, std::uint32_t contract, std::uint32_t status) {
    c.put8(contract + 0x60, (c.u8(contract + 0x60) & 0xf0) | status);
}
// Unhappy (happiness -100 or -50) and the contract told (0x1007d800).
void upset(Assessment& a, std::uint32_t person, std::uint32_t contract, std::int8_t happiness, std::uint32_t back) {
    Cpu& c = a.c;
    c.put8(contract + 0x44, std::uint8_t(happiness));
    c.put8(contract + 0x45, 0);
    c.put8(contract + 0x46, 0);
    a.program.call(call::update_contract, back, {a.contracts(), person, a.club(), contract});
}
std::int32_t mood(Assessment& a, std::uint32_t back) {
    return std::int8_t(a.program.call(call::club_mood, back, {a.club(), 0, 0}));
}

// A first-team player rated below his worth (score < r22): his contract's
// terms decide whether he is offered out (0x103e6290) or kept on and marked.
void below_worth(Assessment& a, std::uint32_t person, std::uint32_t contract) {
    Cpu& c = a.c;
    const auto club = a.club();
    const std::int32_t reputation = a.reputation();
    const auto today = c.r(16);
    const auto offers = [&] { return c.u32(c.toc(toc::offers)); };
    bool keep_on = false;  // 0x1035c674
    if (nonzero_logical(c, contract) && std::int8_t(c.u8(contract + 0x38)) >> 2 == 2) {
        // A contract of mode 2 (inferred: a loan) near its end, or one
        // that started within 450 days.
        const std::int32_t since_birth = std::int32_t(a.program.call(call::days_between, 0x1035c500,
                                                                     {today, c.u32(person + 0x4c), c.u32(person + 0x50)}));
        const std::int32_t limit = std::int32_t(c.i8(person + 0x67)) + c.i8(person + 0x66) + 0x1c2;
        if (compare(c, since_birth, limit) <= 0) keep_on = true;
        else if (compare(c, std::int32_t(a.program.call(call::days_between, 0x1035c530, {contract + 0x30, c.u32(today), c.u32(today + 4)})),
                         0x1c2) >= 0)
            keep_on = true;
    }
    if (!keep_on) {
        if (compare(c, Person{c, person}.value(), std::int32_t(std::uint32_t(reputation) * 50)) >= 0) {
            keep_on = true;
        } else {
            // Days left on the contract (to its end date, +0x30) times its
            // +0xc (inferred: wage), a thirtieth: what it still costs.
            const std::int32_t days_left = std::int32_t(std::uint32_t(c.i16(contract + 0x30)) +
                                                        (std::uint32_t(c.u16(contract + 0x32)) - c.u16(today + 2)) * 0x16d -
                                                        std::uint32_t(std::int32_t(c.i16(today))));
            const std::int32_t cost = std::int32_t(std::uint32_t(c.i32(contract + 0xc)) * std::uint32_t(days_left)) / 30;
            leave(c, {{6, std::uint32_t(c.i32(contract + 0xc))}, {7, extended(c.i16(contract + 0x30))}});
            if (compare(c, Person{c, person}.value(), cost) >= 0) keep_on = true;
        }
    }
    if (!keep_on) {
        // Offered out, unless anything stands in the way.
        if (a.human(1, 0x1035c5b0)) return;
        if (nonzero(c, std::int8_t(c.r(21)))) return;
        if (compare(c, std::int32_t(a.program.call(call::collect_offer_indices, 0x1035c5d8, {offers(), person, 0, 0})), 0) != 0) return;
        if (compare(c, std::int32_t(a.program.call(call::collect_club_offer_indices, 0x1035c5f4, {offers(), person, 0})), 0) != 0) return;
        if (nonzero(c, std::int8_t(a.program.call(call::contains_staff_entry, 0x1035c60c, {offers(), person})))) return;
        if (nonzero_logical(c, contract) && compare(c, mood(a, 0x1035c62c), reputation / 500 + 25) <= 0) return;
        a.program.call(call::append_staff_entry, 0x1035c670, {offers(), person, 0, 0xffffffff});
        return;
    }
    if (compare(c, mood(a, 0x1035c684), reputation / 500 + 20) <= 0) return;
    if (nonzero(c, std::int8_t(c.r(21)))) upset(a, person, contract, -100, 0x1035c6e0);
    if (a.human(1, 0x1035c6f8)) return;
    const std::int32_t score = half_of(a.program.call(call::player_score, 0x1035c718, {a.club_manager(), person, club, 2, c.r(27)}));
    if (compare(c, score, std::int32_t(c.r(28))) >= 0) return;
    const auto player = c.u32(person + 0x14);
    const std::int32_t young = c.i8(player + 0xf) / 5 + 21;
    if (compare(c, std::int32_t(c.r(23)), young) <= 0 && compare(c, c.i16(player + 0xa), reputation / 2) <= 0) return;
    const std::uint32_t not_refreshing = std::int8_t(c.r(21)) == 0;
    a.program.call(call::mark_primary_contract, 0x1035c78c, {a.manager, person, not_refreshing});
}

// A player rated at or above his worth; with option_7e70 nothing happens.
void at_worth(Assessment& a, std::uint32_t person, std::uint32_t contract) {
    Cpu& c = a.c;
    if (nonzero(c, a.local8(local::option))) return;
    if (nonzero(c, std::int8_t(c.r(21)))) upset(a, person, contract, -50, 0x1035c7f8);
    if (!nonzero(c, a.local8(local::staff_flag)) || compare(c, c.i16(c.u32(person + 0x14) + 0xc), 5000) < 0)
        release(a, person, 0x1035c824);
    const std::int32_t age = std::int32_t(c.r(23));
    if (nonzero(c, a.local8(local::staff_flag)) || a.human(0, 0x1035c848)) {
        // Young players worth a second look: status 5 (hot prospect) when
        // the score at three quarters of their worth beats r22 by 20, else 6.
        const bool top = c.u8(c.u32(person + 0x14) + 0xf) == 0x14;
        if (compare(c, age, top ? 27 : 23) >= 0) return;
        const std::uint32_t back = top ? 0x1035c8b0 : 0x1035c93c;
        const double worth = a.fpu.fmul(a.constant(0xa0), a.integer(0x99c, half_of(c.r(27))));
        c.set_f(1, a.constant(8));  // the original loads the bias into f1 (lfd f1)
        const std::int32_t scaled = a.truncated(0x9a4, worth);
        c.set(7, std::uint32_t(scaled));
        const std::int32_t score = half_of(a.program.call(call::player_score, back, {a.club_manager(), person, a.club(), 1, std::uint32_t(scaled)}));
        set_status(c, contract, compare(c, score, half_of(c.r(22)) + 20) > 0 ? 5 : 6);
        return;
    }
    // A computer club's unhappy young first-teamer: transfer-listed
    // (inferred: +0x5c bit 1) and, under 24, status 6.
    const std::int32_t happiness = c.i8(contract + 0x44);
    if (compare(c, happiness, 0) >= 0) return;
    if (c.u8(person + 0x62) != 0xb) return;
    const bool top = c.u8(c.u32(person + 0x14) + 0xf) == 0x14;
    if (compare(c, age, top ? 19 : 17) <= 0) return;
    if (compare(c, happiness, std::int32_t(std::uint32_t(age) * 50) / 3 - (top ? 0x1b1 : 0x190)) > 0) return;
    c.put8(contract + 0x5c, c.u8(contract + 0x5c) | 2);
    if (compare(c, age, 24) >= 0) return;
    set_status(c, contract, 6);
}

void review(Assessment& a, std::uint32_t person) {
    Cpu& c = a.c;
    const auto club = a.club();
    if (nonzero_logical(c, a.program.call(call::second_contract, 0x1035c2fc, {a.contracts(), person}))) {
        if (compare_logical(c, Person{c, person}.club(), club) == 0) return;
        release_unless_kept(a, person, 0x1035c33c);
        return;
    }
    if (compare_logical(c, Person{c, person}.club(), club) != 0) return;
    const Kept<18> contract{c, a.program.call(call::club_contract, 0x1035c35c, {a.contracts(), person, club})};
    if (!nonzero_logical(c, contract)) return;
    keep(c, {{23, std::uint32_t(std::int32_t(Person{c, person}.age()))}});
    if (nonzero(c, std::int8_t(a.program.call(call::contains_staff_entry, 0x1035c37c, {c.u32(c.toc(toc::offers)), person})))) {
        if (nonzero(c, std::int8_t(c.r(21)))) c.put8(contract + 0x44, std::uint8_t(-100));
        return;
    }
    // r22: (standard * (0.13 * age - 1.98)) / 50, in single precision as
    // the original; r27: 12.5 * (+4 + +6) + 0.25 * (+0xa + reputation) of the
    // player's attributes (inferred: his worth); r28: standard / 68 - 35.
    auto& fpu = a.fpu;
    const std::int32_t standard = a.local32(local::standard);
    const auto player = c.u32(person + 0x14);
    const double age = a.integer(0x9b4, std::int32_t(c.r(23)));
    const double slope = fpu.multiply_add(0x1035c3d4, a.constant(0xa8), age, a.constant(0xb0));
    const double scaled = fpu.fsubs(stacked_integer(c, a.at(0x9ac), signed_word(standard)), a.constant(8));
    c.put32(a.at(0x9a4), 0x43300000);
    c.put32(a.at(0x9a8), signed_word(a.reputation()));
    const double product = fpu.fmuls(scaled, fpu.frsp(slope));
    const double quotient = fpu.fdiv(product, a.constant(0xb8));
    keep(c, {{28, std::uint32_t(std::int32_t(std::int16_t(standard / 68 - 35)))}});
    c.put64(a.at(0x9bc), fpu.fctiwz(1, quotient));
    c.put32(a.at(0x9d4), 0x43300000);
    c.put32(a.at(0x9d8), signed_word(c.i16(player + 0xa)));
    keep(c, {{22, c.u32(a.at(0x9c0))}});
    const double quarter = a.constant(0x18), eighth = a.constant(0xc0);
    double worth = fpu.fmul(quarter, fpu.fsub(c.f64(a.at(0x9a4)), a.constant(8)));
    const double a10 = a.integer(0x9d4, c.i16(player + 0xa));
    c.put32(a.at(0x9cc), 0x43300000);
    c.put32(a.at(0x9d0), signed_word(c.i16(player + 6)));
    c.put32(a.at(0x9c4), 0x43300000);
    c.put32(a.at(0x9c8), signed_word(c.i16(player + 4)));
    worth = fpu.multiply_add(0x1035c478, quarter, a10, worth);
    const double a6 = fpu.fsub(c.f64(a.at(0x9cc)), a.constant(8));
    worth = fpu.multiply_add(0x1035c48c, eighth, a6, worth);
    const double a4 = fpu.fsub(c.f64(a.at(0x9c4)), a.constant(8));
    c.set_f(1, a4);  // the original computes it into f1
    worth = fpu.multiply_add(0x1035c494, eighth, a4, worth);
    keep(c, {{27, std::uint32_t(a.truncated(0x99c, worth))}});
    // The block's integer temporaries left in r7-r9.
    const std::int32_t sixty_eighths = multiply_high(0x78787879, standard) >> 5;
    leave(c, {{7, signed_word(c.i16(player + 4))}, {8, std::uint32_t(sixty_eighths)}, {9, std::uint32_t(sixty_eighths) >> 31}});

    if (c.u8(person + 0x62) == 0xb) {
        const std::int32_t score = half_of(a.program.call(call::player_score, 0x1035c4c0, {a.club_manager(), person, club, 1, c.r(27)}));
        if (compare(c, score, half_of(c.r(22))) < 0) {
            below_worth(a, person, contract);
            release_unless_kept(a, person, 0x1035c7b8);
            return;
        }
    }
    at_worth(a, person, contract);
}
}

void review_candidates(Assessment& a) {
    Cpu& c = a.c;
    Kept<14> cursor{c, a.at(local::candidates)};
    Kept<15> index{c, 0};
    for (; std::int8_t(std::uint32_t(index)) < squad_size; cursor = cursor + 4, index = index + 1) {
        const Kept<17> person{c, c.u32(cursor)};
        if (!nonzero_logical(c, person)) continue;
        review(a, person);
    }
}
}
