// 0x106620b0: the press's reaction to a story about a player and his club: the prologue, the club's third
// rating column, the assistant's pull, the player's contract and the story's verdict, then the club's 50
// persons one by one (the stages are in the other files).
#include "media_eval_b_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;

namespace eval_b {
Reaction::Reaction(Cpu& cpu, std::uint32_t reporter_, std::uint32_t club_, std::uint32_t kind_, std::uint32_t flags_,
                   double factor_, std::uint32_t side_, std::uint32_t amount_, std::uint32_t player_)
    : c(cpu), frame(cpu, entry, 0xf0, {.first_gpr = 13, .first_fpr = 28}), fpu(cpu), rng(Stream::world(cpu)),
      program{cpu}, pool(cpu.toc(slot::floats)), singles(cpu.toc(slot::doubles)), reporter(cpu, reporter_),
      club(cpu, club_), kind(cpu, kind_), flags(cpu, flags_), player(cpu, player_), rival_kind(cpu, 0),
      person(cpu, side_), asked_for_neither(cpu, 0), contract(cpu), person_contract(cpu), drawn(cpu), range(cpu),
      factor(factor_), amount(amount_) {
    compare(c, std::int32_t(club));  // or. r24,r4,r4
    // r14 the tally table, r17 the pool of singles, r18 the pool of doubles, r19-r22 the random stream's seed, start,
    // end and cursor.
    keep(c, {{14, c.toc(records::tables::tallies)}, {17, singles}, {18, pool}, {19, rng.seed}, {20, rng.start},
             {21, rng.end}, {22, rng.cursor}});
    c.set_f(28, factor);
    c.set_f(31, dbl(0x40));
    c.put8(frame.at(frame_flag), 1);
}

void Reaction::run() {
    if (nonzero_logical(c, club)) move_third_column(amount);
    assistant_pull();
    if (compare(c, half_word(kind), 0x13) == 0) rival_kind = kind;
    if (nonzero_logical(c, player)) {
        if (!find_contract()) return;
        weigh_story();
    }
    every_person();
}

// The club's third column (+0xa of its 0x4c-byte rating row) moves by `delta`, kept within 1..10000.
void Reaction::move_third_column(std::uint32_t delta) {
    const auto table = c.u32(c.toc(records::tables::club_ratings));
    c.set(3, table);
    c.set(4, table);
    const auto column = [&] { return club_record().rating_row().third_column(); };
    column() = std::int16_t(extended(column().get()) + delta);
    std::uint32_t value;
    if (compare(c, column().get(), 1) < 0) value = 1;
    else if (compare(c, column().get(), 0x2710) > 0) value = 0x2710;
    else value = extended(column().get());
    c.set(4, value);
    c.set(3, club_record().rating_row().at);
    column() = std::int16_t(value);
}

// The assistant's pull on f31 (as the other evaluations compute it from the club's +0x80 person's ambition).
void Reaction::assistant_pull() {
    const auto assistant = club_record().assistant().get();
    if (!nonzero_logical(c, assistant)) return;
    c.put32(frame.at(0x68), 0x43300000);  // stored before the load, as the original does
    const auto level = who(assistant).ambition().get();
    c.set(3, std::uint32_t(level));
    const double top = fpu.fsub(stacked_integer(c, frame.at(0x68), signed_word(level - 0xa)), bias());
    c.set_f(1, top);
    c.set_f(31, fpu.fadd(dbl(0x38), fpu.fdiv(top, dbl(0x48))));
}

// r30: the player's contract with the club. Without one the routine ends.
bool Reaction::find_contract() {
    contract = program.call(contract_of, 0x10662220, {c.u32(c.toc(records::tables::standings)), player, club});
    return nonzero(c, std::int32_t(std::uint32_t(contract)));
}

// f29 is the player's lean (his professionalism and the contract's morale against the singles, less/plus a single
// when the club's manager is liked/disliked by him), f30 the story's value; then whether the reporter lists the
// player as a target, and the staged rating changes by flag.
void Reaction::weigh_story() {
    enter(c, 0x1066224c, {reporter, player, club, contract, kind, person});
    story_value(c, factor, player, club, contract, kind, person);
    c.set_f(30, c.f(1));
    const auto me = who(player);
    const auto morale = player_contract().morale().get();
    const auto professionalism_term = signed_word(me.professionalism().get() - 0xa);
    const auto sportsmanship_term = signed_word(me.sportsmanship().get() - 0xa);
    c.put32(frame.at(0x5c), signed_word(morale));
    c.put32(frame.at(0x58), 0x43300000);
    c.put32(frame.at(0x64), professionalism_term);
    c.put32(frame.at(0x60), 0x43300000);
    c.put32(frame.at(0x68), 0x43300000);  // (the pair at 0x68 is stored but never read)
    c.put32(frame.at(0x6c), sportsmanship_term);
    const double morale_term = fpu.fsubs(c.f64(frame.at(0x58)), bias());
    c.set_f(1, morale_term);
    const double morale_share = fpu.fdivs(morale_term, single(0x48));
    const double professionalism_share = fpu.fdivs(fpu.fsubs(c.f64(frame.at(0x60)), bias()), single(0x44));
    c.set_f(29, fpu.fadds(professionalism_share, morale_share));
    // The scratch the original has in r5-r8 at the first query: the stacked pair's high word, the two trait words, the
    // contract byte.
    leave(c, {{5, 0x43300000}, {6, professionalism_term}, {7, sportsmanship_term}, {8, std::uint8_t(morale)}});
    const auto manager = [&] { return club_record().manager().get(); };
    if (asks(relations::disliked_by, 0x106622d8, manager(), player)) c.set_f(29, fpu.fsubs(lean(), single(0x3c)));
    else if (asks(relations::liked_by, 0x106622f8, manager(), player)) c.set_f(29, fpu.fadds(lean(), single(0x3c)));
    if (listed_as_target()) {
        const auto temperament = me.temperament().get();
        c.set_f(29, fpu.fsubs(lean(), single(0x3c)));
        const auto roll = draw(temperament, 0x106623a8);
        if (compare(c, roll, beaten_by(divisors::by_6, 0x10662504)) < 0) c.put8(frame.at(frame_flag), 0);
    }
    rate_by_flags();
}

// Whether the reporter's list (rows of 0x18 bytes, its count at +8) has the player's id at +8 of a row whose
// byte +0x10 is 2.
bool Reaction::listed_as_target() {
    c.set(3, 0);
    std::uint32_t index = 0;
    bool found = false;
    while (compare(c, std::int32_t(index), c.i32(reporter + 8)) < 0) {
        c.set(3, index);
        const auto row = c.u32(reporter) + index * 0x18;
        c.set(4, row);
        c.set(5, c.u32(player));
        if (compare(c, c.i32(player), c.i32(row + 8)) == 0 && compare(c, c.u8(row + 0x10), 2) == 0) {
            found = true;
            break;
        }
        ++index;
        c.set(3, index);
    }
    c.set(0, found ? 1 : 0);
    nonzero(c, byte_word(found ? 1 : 0));
    return found;
}

// The amount the story moves a club rating by (a single times the verdict less the lean) and the changes
// it asks for through the two club-rating routines, which kind of flag decides.
void Reaction::rate_by_flags() {
    if (has(0x400)) {
        const bool upward = has(0x100);
        const double scaled = fpu.fmuls(single(0x4c), fpu.fsubs(verdict(), lean()));
        c.put64(frame.at(0x58), fpu.fctiwz(0, scaled));
        const auto gain = c.u32(frame.at(0x5c));
        drawn = gain;  // the gain stays in r15
        const auto assistant = [&] { return club_record().assistant().get(); };
        // The second rating moves by the pool's double times the assistant's pull, plus/minus the figure: `site` is the
        // fused instruction. The figure goes through the stack's bias trick.
        const auto fused = [&](std::uint32_t site, std::uint32_t pool_offset, std::uint32_t value) {
            const double stacked = stacked_integer(c, frame.at(0x58), signed_word(std::int32_t(value)));
            const double figure = fpu.fsub(stacked, bias());
            c.put64(frame.at(0x60), fpu.fctiwz(0, fpu.multiply_add(site, dbl(pool_offset), c.f(31), figure)));
            return c.u32(frame.at(0x64));
        };
        const auto signed_gain = extended(std::int16_t(gain));
        const auto favoured = [&](std::uint32_t back) { return asks(relations::favoured_by_club, back, player, club); };
        const auto disfavoured = [&](std::uint32_t back) {
            return asks(relations::disfavoured_by_club, back, player, club);
        };
        const auto assistant_likes = [&](std::uint32_t back) {
            return asks(relations::liked_by, back, player, assistant());
        };
        const auto assistant_dislikes = [&](std::uint32_t back) {
            return asks(relations::disliked_by, back, player, assistant());
        };
        if (upward) {
            if (favoured(0x1066257c)) change_a(extended(std::int16_t(gain + 0x3e8)), 0x106625a4);
            else if (disfavoured(0x106625b4)) change_a(extended(std::int16_t(gain - 0x3e8)), 0x106625dc);
            else change_a(signed_gain, 0x106625fc);
            if (assistant_likes(0x10662608)) change_b(fused(0x1066264c, 0x50, signed_gain), 0x10662660);
            else if (assistant_dislikes(0x10662670)) change_b(fused(0x106626b4, 0x50, signed_gain), 0x106626c8);
            else change_b(signed_gain, 0x106626ec);
        } else if (has(0x80)) {
            if (favoured(0x10662704)) change_a(extended(std::int16_t(-0x3e8 - gain)), 0x1066272c);
            else if (disfavoured(0x1066273c)) change_a(extended(std::int16_t(0x3e8 - gain)), 0x10662764);
            else change_a(extended(std::int16_t(0u - gain)), 0x10662788);
            if (assistant_likes(0x10662794)) change_b(fused(0x106627d8, 0x58, signed_gain), 0x106627ec);
            else if (assistant_dislikes(0x106627fc)) change_b(fused(0x10662840, 0x50, signed_gain), 0x10662854);
            else change_b(extended(std::int16_t(0u - gain)), 0x1066287c);
        } else {
            // |gain / 2|: the halving rounds toward zero, the game's abs follows.
            const auto half_magnitude = [&](std::uint32_t back) {
                const auto halved = (std::int32_t(signed_gain) + std::int32_t(signed_gain >> 31)) >> 1;
                return std::uint32_t(program.call(absolute_value, back, {std::uint32_t(halved)}));
            };
            if (favoured(0x1066288c)) change_a(extended(std::int16_t(-0xc8 - half_magnitude(0x106628a8))), 0x106628cc);
            else if (disfavoured(0x106628dc))
                change_a(extended(std::int16_t(0xc8 - half_magnitude(0x106628f8))), 0x1066291c);
            else change_a(extended(std::int16_t(0u - half_magnitude(0x10662934))), 0x10662958);
            if (assistant_likes(0x10662964)) change_b(fused(0x106629b8, 0x60, half_magnitude(0x10662980)), 0x106629cc);
            else if (assistant_dislikes(0x106629dc))
                change_b(fused(0x10662a30, 0x68, half_magnitude(0x106629f8)), 0x10662a44);
            else change_b(extended(std::int16_t(0u - half_magnitude(0x10662a5c))), 0x10662a84);
        }
    }
    if (!has(0x80) && !has(0x100)) {
        asked_for_neither = 1;
        const double threshold = fpu.fadd(dbl(0x38), lean());
        fpu.fcmp(0, verdict(), threshold);
        if (threshold < verdict()) flags = flags | 0x80;
    }
}

// The club's 50 slots, one person at a time.
void Reaction::every_person() {
    c.put32(frame.at(0x74), 0);
    while (compare(c, std::int32_t(c.u32(frame.at(0x74))), records::Club::squad_slots) < 0) {
        one_person();
        const auto next = c.u32(frame.at(0x74)) + 1;
        c.set(3, next);
        c.put32(frame.at(0x74), next);
    }
}

// One of the club's persons: a person with a profile, known to the world, who has a contract with the club;
// then the stages by flag.
void Reaction::one_person() {
    if (!gate()) return;
    stages();
    if (!nonzero_logical(c, player)) return;
    if (has(0x80)) {
        if (compare_logical(c, person, player) == 0) self_80();
        else other_80();
    } else if (has(0x100)) {
        if (compare_logical(c, person, player) == 0) self_100();
        else other_100();
    }
}

bool Reaction::gate() {
    const auto slot_index = c.u32(frame.at(0x74));
    c.set(3, club + slot_index * 4);
    person = club_record().squad(slot_index);
    if (!nonzero_logical(c, person)) return false;
    if (!nonzero_logical(c, who(person).player())) return false;
    nonzero_logical(c, person);
    const auto count = c.u32(c.toc(records::tables::person_count));
    c.set(3, c.toc(records::tables::person_count));
    c.set(4, who(person).id());
    if (compare(c, c.i32(person), std::int32_t(count)) >= 0) return false;
    person_contract = program.call(contract_of, 0x10662b18, {c.u32(c.toc(records::tables::standings)), person, club});
    if (!nonzero(c, std::int32_t(std::uint32_t(person_contract)))) return false;
    const auto happiness = person_contract_record().happiness().get();
    range = std::uint8_t(happiness);
    c.set(3, std::uint32_t(happiness));
    const auto n = happiness + 0x96;
    const auto roll = draw_narrow(c, rng, n, 0x10662b5c, 15);
    // This site's schedule: r4 the range, r5 the seed, r6 the table entry (r3 the sample).
    if (n != 0) leave(c, {{4, std::uint32_t(n)}, {5, c.u32(rng.seed)}, {6, c.u16(c.u32(rng.cursor))}});
    return compare(c, roll, beaten_by(divisors::by_100, 0x10662cec)) >= 0;
}
}
}

namespace cm::aot::media {
void react_to_story(guest::Cpu& c, std::uint32_t reporter, std::uint32_t club, std::uint32_t kind, std::uint32_t flags,
                    double factor, std::uint32_t side, std::uint32_t amount, std::uint32_t player) {
    eval_b::Reaction(c, reporter, club, kind, flags, factor, side, amount, player).run();
}
}
