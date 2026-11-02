// 0x106a52d0: how a standing record (the person's mood at his club, +0x4c) and the person himself react to
// an event of kind 1, 2 or 3 about a club: draws against his traits, the flag changes of the standing, and
// the item 0xbe8 posted for the press.
#include "media_pick_c.hpp"
#include "media_pick_c_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;
using namespace detail::pick_c;

void react_to_event(Cpu& c, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t kind) {
    Frame frame(c, 0x106a52d0, 0x80, {.first_gpr = 20});
    const auto rng = Stream::world(c);
    // r20: self, then draw results; r23: draw ranges and results; r24: the press slot, then draw results and ranges;
    // r21: the club; r22: the kind; r30: the person; r31: his standing at the club.
    Kept<20> r20(c, self);
    Kept<23> r23(c, 0);
    Kept<24> r24(c, c.toc(slots::press));
    Kept<31> standing(c, c.r(31));
    keep(c, {{21, club}, {22, kind}, {25, rng.start}, {26, rng.end}, {27, rng.cursor}, {28, c.toc(slot::floats)}, {29, rng.seed}, {30, person}});
    const Program program{c};
    const Person subject{c, person};
    if (compare(c, std::int32_t(person)) == 0) return;
    if (compare_logical(c, subject.club()) == 0) return;
    if (compare_logical(c, subject.player()) == 0) return;
    if (compare_logical(c, club) == 0) return;
    if (compare_logical(c, Club{c, club}.manager()) == 0) return;
    standing = program.call(callee::standing_of, 0x106a5354, {c.u32(c.toc(slots::standings)), person, subject.club()});
    if (compare(c, std::int32_t(standing)) == 0) return;
    // The original compares the contract's rating and two traits here and nothing reads the result; the loads can fault.
    c.set(3, subject.player());
    compare(c, std::int32_t(c.i16(c.r(3) + 0xa)), 0x1c52);
    const std::uint32_t mood = standing + 0x4c;

    if (compare(c, byte_word(kind), 2) == 0) {
        // Kind 2: a person with a spare point of one trait (+0x66) may lose it, and his mood drops by 25.
        if (compare(c, subject.professionalism().get(), 1) > 0) {
            const auto draw = draw_byte<23>(c, rng, raw_byte(subject.professionalism()), 0x106a5408, 24);
            if (compare(c, draw) != 0) {
                const auto old = raw_byte(subject.professionalism());
                c.set(3, old);
                subject.professionalism() = std::int8_t(old - 1);
                shift_score(c, mood, -0x19);
                return;
            }
        }
        if (compare(c, subject.professionalism().get(), 0x14) >= 0) return;
        const auto old = raw_byte(subject.professionalism());
        c.set(3, old);
        subject.professionalism() = std::int8_t(old + 1);
        c.set(3, subject.player());
        if (compare(c, byte_word(c.u8(c.r(3) + 0x44)), 0x14) < 0) {
            const auto contract = subject.player();
            c.set(3, contract);
            const auto draw = draw_byte<23>(c, rng, c.u8(contract + 0x44), 0x106a5610, 20);
            if (compare(c, draw) == 0) {
                c.set(4, contract);
                const auto trait = c.u8(contract + 0x44);
                c.set(3, trait);
                c.put8(contract + 0x44, trait + 1);
            }
        }
        shift_score(c, mood, -5);
    } else if (compare(c, byte_word(kind), 1) == 0) {
        if (compare(c, byte_word(program.call(callee::person_flag, 0x106a57cc, {person, 0}))) != 0) {
            shift_score(c, mood, -0x19);
            return;
        }
        if (compare(c, byte_word(c.u8(standing + 0x45)), 1) > 0) {
            // Looks for an earlier item 0xbe8 about this club in the press's history. Quirk: the original's loop has
            // no increment, so only the first row is looked at; a row that is not it (or no rows) leaves -1,
            // and only a first row that is it with a zero person ends the branch.
            const auto history = c.u32(c.r(20));
            const auto count = c.i32(c.r(20) + 8);
            std::int32_t seen = -1;
            c.set(5, 0);
            if (compare(c, 0, count) < 0) {
                c.set(3, history);
                if (compare(c, c.i32(history + 0x14), 0xbe8) == 0) {
                    const auto club_id = c.u32(club);
                    c.set(4, club_id);
                    if (compare(c, std::int32_t(club_id), std::int32_t(c.i16(history + 0x12))) == 0) seen = c.i32(history + 8);
                }
            }
            if (compare(c, seen) != 0) {
                program.call(callee::modify, 0x106a58c4, {standing, 0x8000000, 0});
                enter(c, 0x106a58d4, {c.u32(c.r(24)), person, 2});
                post_item_be8(c, c.r(3), person, 2);
                shift_score(c, mood, -0xf);
                return;
            }
        }
        if (compare(c, subject.loyalty().get(), 5) > 0) {
            const auto draw = draw_byte<23>(c, rng, raw_byte(subject.loyalty()), 0x106a5968, 20);
            if (compare(c, draw) != 0) {
                enter(c, 0x106a5abc, {c.u32(c.r(24)), person, 1});
                post_item_be8(c, c.r(3), person, 1);
                const auto flags = program.call(callee::flags_of, 0x106a5ac4, {standing});
                if (compare(c, std::int32_t(flags & 0x200)) != 0) program.call(callee::clear_flag, 0x106a5ad8, {standing, 0x200});
            }
        }
        shift_score(c, mood, 0xf);
    } else if (compare(c, byte_word(kind), 3) == 0) {
        const auto first = draw_byte<24>(c, rng, raw_byte(subject.professionalism()), 0x106a5b68, 23);
        const auto second = draw_byte<24>(c, rng, raw_byte(subject.trait_69()), 0x106a5cd8, 20);
        if (compare(c, first, second) < 0) program.call(callee::set_flag, 0x106a5e28, {standing, 0x80});
        shift_score(c, mood, -5);
    }
}

// 0x106a2e10: the person's side of an event of kind 1, 2 or 3 (r7): his contract traits (+0x1c, +0x25) and personal
// traits move, the club's manager (and the manager of a second club, r6) takes notice, and a story (0xbda) may
// follow. `score` weighs how much the person matters (his rating and two traits).
void settle_event(Cpu& c, std::uint32_t self, std::uint32_t person, std::uint32_t club, std::uint32_t other, std::uint32_t kind) {
    Frame frame(c, 0x106a2e10, 0x80, {.first_gpr = 20});
    const auto rng = Stream::world(c);
    // r20: the kind, then draw results; r21: self, then draw results; r22: his standing, then draw results; r23: the
    // second club, then draw ranges; r25: the score; r24: the club; r30: the person.
    Kept<20> r20(c, kind);
    Kept<21> r21(c, self);
    Kept<22> standing(c, c.r(22));
    Kept<23> r23(c, other);
    Kept<25> score(c, 0);
    keep(c, {{24, club}, {26, rng.end}, {27, rng.start}, {28, c.toc(slot::floats)}, {29, rng.seed}, {30, person}, {31, rng.cursor}});
    const Program program{c};
    const Person subject{c, person};
    if (compare(c, std::int32_t(person)) == 0) return;
    if (compare_logical(c, subject.club()) == 0) return;
    if (compare_logical(c, subject.player()) == 0) return;
    if (compare_logical(c, club) == 0) return;
    if (compare_logical(c, Club{c, club}.manager()) == 0) return;
    standing = program.call(callee::standing_of, 0x106a2e94, {c.u32(c.toc(slots::standings)), person, subject.club()});
    if (compare(c, std::int32_t(standing)) == 0) return;
    c.set(3, subject.player());
    const auto add = [&](std::uint32_t amount) { score = std::uint32_t(score) + amount; };
    if (compare(c, std::int32_t(subject.player_record().reputation()), 0x1c52) > 0) {
        // A strong player scores for a rare or low trait (+0x69, +0x67).
        if (compare(c, std::int32_t(raw_byte(subject.trait_69())), 1) == 0) add(3);
        else if (compare(c, subject.trait_69().get(), 5) < 0) add(1);
        if (compare(c, std::int32_t(raw_byte(subject.sportsmanship())), 1) == 0) add(5);
        else if (compare(c, subject.sportsmanship().get(), 5) < 0) add(3);
        else if (compare(c, subject.sportsmanship().get(), 10) < 0) add(1);
    }
    const auto contract = [&] { return subject.player(); };
    const auto manager = [&](std::uint32_t of) { return Club{c, of}.manager().get(); };
    const auto signed_score = [&] { return byte_word(score); };

    if (compare(c, byte_word(kind), 2) == 0) {
        c.set(3, contract());
        if (compare(c, byte_word(c.u8(c.r(3) + 0x1c)), 0xa) > 0) {
            c.set(3, contract());
            if (compare(c, draw_byte<23>(c, rng, c.u8(contract() + 0x1c), 0x106a2f6c, 21)) == 0) step_contract_byte(c, person, 0x1c, ~0u);
        }
        c.set(3, contract());
        if (compare(c, byte_word(c.u8(c.r(3) + 0x25)), 1) > 0) {
            if (compare(c, draw_byte<23>(c, rng, raw_byte(subject.sportsmanship()), 0x106a3108, 21)) != 0) step_contract_byte(c, person, 0x25, ~0u);
        }
        c.set(3, extended(std::int16_t(subject.loyalty().get())));
        const auto range = subject.loyalty().get() + 5;
        r23 = std::uint32_t(range);
        const auto draw = roll(c, rng, 0x106a3294, 21, range);
        rng.advance(0x106a33f0);
        const auto sixth = reduced_by(rng.sample(), divisors::by_6);
        leave(c, {{3, std::uint32_t(sixth.rest)}, {4, sixth.shifted}, {5, sixth.sample}});
        if (compare(c, draw, signed_score() + sixth.rest) >= 0) return;
        if (compare(c, byte_word(program.call(0x100d9520, 0x106a3440, {person, manager(club)}))) != 0)
            program.call(0x100d9e10, 0x106a3454, {person, manager(club)});
        else
            program.call(0x100d9b80, 0x106a3468, {person, manager(club), 1});
        program.call(callee::set_flag, 0x106a3474, {standing, 0x1000});
    } else if (compare(c, byte_word(kind), 1) == 0) {
        if (compare_logical(c, other) != 0 && compare_logical(c, manager(other)) != 0)
            program.call(0x100d9b80, 0x106a34a8, {manager(other), manager(club), 1});
        c.set(3, contract());
        if (compare(c, byte_word(c.u8(c.r(3) + 0x1c)), 0x14) < 0) {
            c.set(3, contract());
            if (compare(c, draw_byte<20>(c, rng, c.u8(contract() + 0x1c), 0x106a34f4, 22)) == 0) step_contract_byte(c, person, 0x1c, 1);
        }
        c.set(3, contract());
        if (compare(c, byte_word(c.u8(c.r(3) + 0x25)), 0x14) < 0) {
            if (compare(c, draw_byte<22>(c, rng, raw_byte(subject.sportsmanship()), 0x106a3690, 20)) == 0) step_contract_byte(c, person, 0x25, 1);
        }
        if (compare(c, subject.trait_68().get(), 1) > 0) {
            if (compare(c, draw_byte<22>(c, rng, raw_byte(subject.sportsmanship()), 0x106a3828, 20)) == 0) {
                const auto old = raw_byte(subject.trait_68());
                c.set(3, old);
                subject.trait_68() = std::int8_t(old - 1);
            }
        }
        const auto draw = draw_byte<22>(c, rng, raw_byte(subject.loyalty()), 0x106a39ac, 20);
        rng.advance(0x106a3b08);
        const auto tenth = reduced_by(rng.sample(), divisors::by_10);
        leave(c, {{3, tenth.sign}, {4, tenth.sample}});
        if (compare(c, draw, tenth.rest) > 0) {
            if (compare(c, byte_word(program.call(0x100d9580, 0x106a3b54, {person, manager(club)}))) != 0)
                program.call(0x100d9e90, 0x106a3b68, {person, manager(club)});
            else
                program.call(0x100d9960, 0x106a3b7c, {person, manager(club), 1});
            enter(c, 0x106a3b94, {self, person, club, 0x82, 0});
            post_item_bda(c, self, person, club, 0x82, 0);
        }
        if (compare_logical(c, other) == 0) return;
        if (compare_logical(c, manager(other)) == 0) return;
        const auto second = draw_byte<20>(c, rng, raw_byte(subject.sportsmanship()), 0x106a3bdc, 21);
        c.set(3, std::uint32_t(signed_score()));
        if (compare(c, second, signed_score() + 5) >= 0) return;
        program.call(0x100d9b80, 0x106a3d38, {person, manager(other), 1});
    } else if (compare(c, byte_word(kind), 3) == 0) {
        c.set(3, contract());
        if (compare(c, byte_word(c.u8(c.r(3) + 0x1c)), 0x14) < 0) {
            c.set(3, contract());
            if (compare(c, draw_byte<21>(c, rng, c.u8(contract() + 0x1c), 0x106a3d94, 20)) == 0) step_contract_byte(c, person, 0x1c, 1);
        }
        c.set(3, contract());
        if (compare(c, byte_word(c.u8(c.r(3) + 0x25)), 0x14) >= 0) return;
        if (compare(c, draw_byte<20>(c, rng, raw_byte(subject.sportsmanship()), 0x106a3f30, 21)) == 0) step_contract_byte(c, person, 0x25, 1);
    }
}
}
