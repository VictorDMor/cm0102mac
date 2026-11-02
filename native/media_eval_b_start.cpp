// 0x106620b0: the press's reaction to a story about a player and his club: the prologue, the club's third
// rating column, the rival's pull, the player's contract and the story's verdict, then the club's 50
// persons one by one (the stages are in the other files).
#include "media_eval_b_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;

namespace eval_b {
Reaction::Reaction(Cpu& cpu, std::uint32_t reporter_, std::uint32_t club_, std::uint32_t kind_, std::uint32_t flags_, double factor_,
                   std::uint32_t side_, std::uint32_t amount_, std::uint32_t player_)
    : c(cpu), frame(cpu, entry, 0xf0, {.first_gpr = 13, .first_fpr = 28}), fpu(cpu), rng(Stream::world(cpu)), program{cpu},
      pool(cpu.toc(slot::floats)), singles(cpu.toc(slot::doubles)), tallies(cpu.toc(-0x7b9c)), reporter(reporter_), club(club_), kind(kind_),
      flags(flags_), player(player_), rival_kind(0), person(side_), standing(0), person_standing(0), factor(factor_), amount(amount_) {
    compare(c, std::int32_t(club));  // or. r24,r4,r4
    keep(c, {{14, tallies}, {17, singles}, {18, pool}, {19, rng.seed}, {20, rng.start}, {21, rng.end}, {22, rng.cursor}, {23, reporter},
             {24, club}, {25, kind}, {26, flags}, {27, player}, {28, 0}, {29, person}, {13, 0}});
    c.set_f(28, factor);
    c.set_f(31, dbl(0x40));
    c.put8(frame.at(0x70), 1);
}

void Reaction::run() {
    if (nonzero_logical(c, club)) move_third_column(amount);
    rival_pull();
    if (compare(c, half_word(kind), 0x13) == 0) {
        rival_kind = kind;
        c.set(28, kind);
    }
    if (nonzero_logical(c, player)) {
        if (!find_standing()) return;
        weigh_story();
    }
    c.put32(frame.at(0x74), 0);
    while (compare(c, std::int32_t(c.u32(frame.at(0x74))), 0x32) < 0) {
        one_person();
        const auto next = c.u32(frame.at(0x74)) + 1;
        c.set(3, next);
        c.put32(frame.at(0x74), next);
    }
}

// The club's third column (+0xa of its 0x4c-byte rating row) moves by `amount`, kept within 1..10000.
void Reaction::move_third_column(std::uint32_t delta) {
    const auto table = c.u32(c.toc(-0x79f0));
    c.set(3, table);
    c.set(4, table);
    const auto row = [&] { return c.u32(table) + c.u32(club) * 0x4c; };
    c.put16(row() + 0xa, extended(c.i16(row() + 0xa)) + delta);
    std::uint32_t value;
    if (compare(c, c.i16(row() + 0xa), 1) < 0) value = 1;
    else if (compare(c, c.i16(row() + 0xa), 0x2710) > 0) value = 0x2710;
    else value = extended(c.i16(row() + 0xa));
    c.set(4, value);
    c.set(3, row());
    c.put16(row() + 0xa, value);
}

// The rival's pull on f31 (as the other evaluations compute it from the club's +0x80 person's ambition).
void Reaction::rival_pull() {
    const auto rival = c.u32(club + 0x80);
    if (!nonzero_logical(c, rival)) return;
    c.put32(frame.at(0x68), 0x43300000);  // stored before the load, as the original does
    const auto level = byte_word(c.u8(rival + 0x63));
    c.set(3, std::uint32_t(level));
    const double top = fpu.fsub(stacked_integer(c, frame.at(0x68), signed_word(level - 0xa)), bias());
    c.set_f(1, top);
    c.set_f(31, fpu.fadd(dbl(0x38), fpu.fdiv(top, dbl(0x48))));
}

// r30: the player's contract with the club. Without one the routine ends.
bool Reaction::find_standing() {
    standing = program.call(contract_of, 0x10662220, {c.u32(c.toc(-0x7b7c)), player, club});
    c.set(30, standing);
    return nonzero(c, std::int32_t(standing));
}

// f29 is the player's lean (the contract's loyalty-like byte and his two traits against the singles, less/plus a
// single when the club's manager is for/against him), f30 the story's value; then whether the reporter lists
// the player as a target, and the staged rating changes by flag.
void Reaction::weigh_story() {
    enter(c, 0x1066224c, {reporter, player, club, standing, kind, person});
    story_value(c, factor, player, club, standing, kind, person);
    c.set_f(30, c.f(1));
    c.put32(frame.at(0x5c), signed_word(byte_word(c.u8(standing + 0x4c))));
    c.put32(frame.at(0x58), 0x43300000);
    c.put32(frame.at(0x64), signed_word(byte_word(c.u8(player + 0x66)) - 0xa));
    c.put32(frame.at(0x60), 0x43300000);
    c.put32(frame.at(0x68), 0x43300000);  // (the pair at 0x68 is stored but never read)
    c.put32(frame.at(0x6c), signed_word(byte_word(c.u8(player + 0x67)) - 0xa));
    const double numerator = fpu.fsubs(c.f64(frame.at(0x58)), bias());
    c.set_f(1, numerator);
    const double second = fpu.fdivs(numerator, single(0x48));
    const double first = fpu.fdivs(fpu.fsubs(c.f64(frame.at(0x60)), bias()), single(0x44));
    c.set_f(29, fpu.fadds(first, second));
    // The scratch the original has in r5-r8 at the first query: the stacked pair's high word, the two trait words, the contract byte.
    c.set(5, 0x43300000);
    c.set(6, signed_word(byte_word(c.u8(player + 0x66)) - 0xa));
    c.set(7, signed_word(byte_word(c.u8(player + 0x67)) - 0xa));
    c.set(8, c.u8(standing + 0x4c));
    const auto manager = [&] { return c.u32(club + 0x90); };
    if (asks(query_rival_bad, 0x106622d8, manager(), player)) c.set_f(29, fpu.fsubs(lean(), single(0x3c)));
    else if (asks(query_rival_good, 0x106622f8, manager(), player)) c.set_f(29, fpu.fadds(lean(), single(0x3c)));
    if (listed_as_target()) {
        c.set(16, c.u8(player + 0x69));
        c.set_f(29, fpu.fsubs(lean(), single(0x3c)));
        const auto n = byte_word(c.r(16));
        c.set(16, std::uint32_t(n));
        const auto drawn = draw_narrow(c, rng, n, 0x106623a8, 15);
        if (compare(c, drawn, beaten_by(divisors::by_6, 0x10662504)) < 0) c.put8(frame.at(0x70), 0);
    }
    rate_by_flags();
}

// Whether the reporter's list (rows of 0x18 bytes, its count at +8) has the player's id at +8 of a row whose
// byte +0x10 is 2.
bool Reaction::listed_as_target() {
    c.set(3, 0);
    std::uint32_t index = 0, row = c.r(4);
    bool found = false;
    c.set(5, c.r(5));
    while (compare(c, std::int32_t(index), c.i32(reporter + 8)) < 0) {
        c.set(3, index);
        row = c.u32(reporter) + index * 0x18;
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
    if (nonzero(c, std::int32_t(flags & 0x400))) {
        const bool upward = nonzero(c, std::int32_t(flags & 0x100));
        const double scaled = fpu.fmuls(single(0x4c), fpu.fsubs(verdict(), lean()));
        c.put64(frame.at(0x58), fpu.fctiwz(0, scaled));
        const auto gain = c.u32(frame.at(0x5c));
        c.set(15, gain);
        const auto rival = [&] { return c.u32(club + 0x80); };
        // The second rating moves by the pool's double times the rival pull, plus/minus the gain: `site` is the fused instruction.
        const auto fused = [&](std::uint32_t site, std::uint32_t pool_offset, std::uint32_t value) {
            const double figure = fpu.fsub(stacked_integer(c, frame.at(0x58), signed_word(std::int32_t(value))), bias());
            c.put64(frame.at(0x60), fpu.fctiwz(0, fpu.multiply_add(site, dbl(pool_offset), c.f(31), figure)));
            return c.u32(frame.at(0x64));
        };
        if (upward) {
            if (asks(query_best, 0x1066257c, player, club)) change_a(extended(std::int16_t(gain + 0x3e8)), 0x106625a4);
            else if (asks(query_worst, 0x106625b4, player, club)) change_a(extended(std::int16_t(gain - 0x3e8)), 0x106625dc);
            else change_a(extended(std::int16_t(gain)), 0x106625fc);
            if (asks(query_rival_good, 0x10662608, player, rival())) change_b(fused(0x1066264c, 0x50, extended(std::int16_t(gain))), 0x10662660);
            else if (asks(query_rival_bad, 0x10662670, player, rival())) change_b(fused(0x106626b4, 0x50, extended(std::int16_t(gain))), 0x106626c8);
            else change_b(extended(std::int16_t(gain)), 0x106626ec);
        } else if (nonzero(c, std::int32_t(flags & 0x80))) {
            if (asks(query_best, 0x10662704, player, club)) change_a(extended(std::int16_t(-0x3e8 - gain)), 0x1066272c);
            else if (asks(query_worst, 0x1066273c, player, club)) change_a(extended(std::int16_t(0x3e8 - gain)), 0x10662764);
            else change_a(extended(std::int16_t(0u - gain)), 0x10662788);
            if (asks(query_rival_good, 0x10662794, player, rival())) change_b(fused(0x106627d8, 0x58, extended(std::int16_t(gain))), 0x106627ec);
            else if (asks(query_rival_bad, 0x106627fc, player, rival())) change_b(fused(0x10662840, 0x50, extended(std::int16_t(gain))), 0x10662854);
            else change_b(extended(std::int16_t(0u - gain)), 0x1066287c);
        } else {
            const auto half = [&](std::uint32_t back) {
                const auto value = std::int32_t(extended(std::int16_t(gain)));
                return std::int32_t(program.call(round_half, back, {std::uint32_t((value + std::int32_t(std::uint32_t(value) >> 31)) >> 1)}));
            };
            const auto fused_half = [&](std::uint32_t back, std::uint32_t site, std::uint32_t pool_offset) {
                const auto rounded = half(back);
                c.put32(frame.at(0x5c), signed_word(rounded));
                c.put32(frame.at(0x58), 0x43300000);
                const double figure = fpu.fsub(c.f64(frame.at(0x58)), bias());
                c.put64(frame.at(0x60), fpu.fctiwz(0, fpu.multiply_add(site, dbl(pool_offset), c.f(31), figure)));
                return c.u32(frame.at(0x64));
            };
            if (asks(query_best, 0x1066288c, player, club)) change_a(extended(std::int16_t(-0xc8 - std::uint32_t(half(0x106628a8)))), 0x106628cc);
            else if (asks(query_worst, 0x106628dc, player, club)) change_a(extended(std::int16_t(0xc8 - std::uint32_t(half(0x106628f8)))), 0x1066291c);
            else change_a(extended(std::int16_t(0u - std::uint32_t(half(0x10662934)))), 0x10662958);
            if (asks(query_rival_good, 0x10662964, player, rival())) change_b(fused_half(0x10662980, 0x106629b8, 0x60), 0x106629cc);
            else if (asks(query_rival_bad, 0x106629dc, player, rival())) change_b(fused_half(0x106629f8, 0x10662a30, 0x68), 0x10662a44);
            else change_b(extended(std::int16_t(0u - std::uint32_t(half(0x10662a5c)))), 0x10662a84);
        }
    }
    if (!nonzero(c, std::int32_t(flags & 0x80)) && !nonzero(c, std::int32_t(flags & 0x100))) {
        c.set(13, 1);
        const double threshold = fpu.fadd(dbl(0x38), lean());
        fpu.fcmp(0, verdict(), threshold);
        if (threshold < verdict()) {
            flags |= 0x80;
            c.set(26, flags);
        }
    }
}

// One of the club's persons: a person with a profile, known to the world, who has a contract with the club;
// then the stages by flag.
void Reaction::one_person() {
    if (!gate()) return;
    stages();
    if (!nonzero_logical(c, player)) return;
    if (nonzero(c, std::int32_t(flags & 0x80))) {
        if (compare_logical(c, person, player) == 0) self_80();
        else other_80();
    } else if (nonzero(c, std::int32_t(flags & 0x100))) {
        if (compare_logical(c, person, player) == 0) self_100();
        else other_100();
    }
}

bool Reaction::gate() {
    const auto index = c.u32(frame.at(0x74));
    c.set(3, club + index * 4);
    person = c.u32(club + index * 4 + 0x98);
    c.set(29, person);
    if (!nonzero_logical(c, person)) return false;
    if (!nonzero_logical(c, who(person).player())) return false;
    nonzero_logical(c, person);
    const auto count = c.u32(c.toc(-0x7be4));
    c.set(3, c.toc(-0x7be4));
    c.set(4, c.u32(person));
    if (compare(c, c.i32(person), std::int32_t(count)) >= 0) return false;
    person_standing = program.call(contract_of, 0x10662b18, {c.u32(c.toc(-0x7b7c)), person, club});
    c.set(31, person_standing);
    if (!nonzero(c, std::int32_t(person_standing))) return false;
    const auto byte = c.u8(person_standing + 0x44);
    c.set(16, byte);
    c.set(3, std::uint32_t(byte_word(byte)));
    const auto n = byte_word(byte) + 0x96;
    const auto drawn = draw_narrow(c, rng, n, 0x10662b5c, 15);
    // This site's schedule: r4 the range, r5 the seed, r6 the table entry (r3 the sample).
    if (n != 0) leave(c, {{4, std::uint32_t(n)}, {5, c.u32(rng.seed)}, {6, c.u16(c.u32(rng.cursor))}});
    return compare(c, drawn, beaten_by(divisors::by_100, 0x10662cec)) >= 0;
}
}
}

namespace cm::aot::media {
void react_to_story(guest::Cpu& c, std::uint32_t reporter, std::uint32_t club, std::uint32_t kind, std::uint32_t flags, double factor,
                    std::uint32_t side, std::uint32_t amount, std::uint32_t player) {
    eval_b::Reaction(c, reporter, club, kind, flags, factor, side, amount, player).run();
}
}
