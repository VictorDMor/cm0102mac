// 0x10672800: the start of the club story evaluation (the guards, the rival's pull and the
// story's base factor) and the dispatch by kind.
#include "media_eval_c_detail.hpp"

namespace cm::aot::media {
using namespace guest;
using namespace detail;

namespace eval_c {
Press::Press(Cpu& cpu, std::uint32_t reporter_, std::uint32_t club_, std::uint32_t kind_, std::uint32_t side_, std::uint32_t other_)
    : c(cpu), frame(cpu, evaluate_entry, 0xf0, {.first_gpr = 17, .gpr_offset = -0x5c, .first_fpr = 29}), fpu(cpu), rng(Stream::world(cpu)), program{cpu},
      pool(cpu.toc(slot::floats)), singles(cpu.toc(slot::doubles)), ratings(cpu.toc(-0x79f0)), reporter(reporter_), club(club_), kind(kind_),
      side(side_), other(other_) {
    // r30 and f31 are the caller's until the preamble sets them; r17 is a scratch the stories reuse.
    keep(c, {{17, c.toc(-0x7b5c)}, {18, 0}, {19, ratings}, {20, rng.start}, {21, rng.end}, {22, rng.seed}, {23, singles}, {24, other},
             {25, reporter}, {26, side}, {27, kind}, {28, rng.cursor}, {29, pool}, {31, club}});
    c.set_f(30, dbl(0x40));
    c.set_f(29, dbl(0x38));
}

bool Press::kind_is(std::initializer_list<std::int32_t> kinds) {
    for (const auto wanted : kinds)
        if (compare(c, half_word(kind), wanted) == 0) return true;
    return false;
}

// The guards: a club with a manager (who has +0x1c) and standing records (0x100c7d00 asks whether the
// club has its own; otherwise the club's group, from the table in r17, must have a record and a second one).
bool Press::admit() {
    compare(c, std::int32_t(club));  // or. r31,r4,r4
    if (club == 0) return false;
    const auto manager = records::Club{c, club}.manager().get();
    c.set(3, manager);
    if (!nonzero_logical(c, manager)) return false;
    if (!nonzero_logical(c, records::Person{c, manager}.non_player())) return false;
    const auto group = [&] { return records::Club{c, club}.division().get(); };
    const auto entry = [&] { return c.u32(c.u32(c.r(17)) + (c.u32(group()) << 2)); };
    if (!nonzero(c, byte_word(program.call(standing_of, 0x10672880, {club})))) {
        if (!nonzero_logical(c, group())) return false;
        leave(c, {{3, group()}, {4, c.u32(c.r(17))}});
        if (!nonzero_logical(c, entry())) return false;
    }
    if (!nonzero(c, byte_word(program.call(standing_of, 0x106728bc, {club})))) {
        leave(c, {{3, entry()}, {4, c.u32(c.r(17))}});
        c.set(18, c.u32(c.r(3) + 8));
        if (!nonzero_logical(c, c.r(18))) return false;
    }
    return true;
}

// The rival's pull on f30, the story's base factor in f31 (from 0x10457d00 when the club has a
// second record, else the pool's single) and r30, the factor's share of the pool's strength.
void Press::prepare() {
    const auto rival = c.u32(club + 0x80);
    if (nonzero_logical(c, rival)) {
        c.put32(frame.at(0x88), 0x43300000);  // stored before the load, as the original does
        const std::int32_t level = records::Person{c, rival}.ambition().get();
        leave(c, {{3, std::uint32_t(level)}});
        const double top = fpu.fsub(stacked_integer(c, frame.at(0x88), signed_word(level - 0xa)), bias());
        c.set_f(1, top);
        c.set_f(30, fpu.fadd(dbl(0x38), fpu.fdiv(top, dbl(0x48))));
    }
    if (nonzero_logical(c, c.r(18))) {
        c.set({{3, c.r(18)}, {4, club}, {5, 3}});
        c.call(base_factor, 0x1067294c);
    } else {
        c.set_f(1, single_at(8));
    }
    c.set_f(31, c.f(1));
    c.put64(frame.at(0x80), fpu.fctiwz(0, fpu.fmuls(single_at(0x54), factor())));
    c.set(30, c.u32(frame.at(0x84)));
}

void Press::run() {
    if (!admit()) return;
    prepare();
    if (kind_is({0x3c, 0x3d})) story_3c();
    else if (kind_is({0x63, 0x5b, 0x61})) story_family(families::kind_63);
    else if (kind_is({0x62, 0x5d})) story_family(families::kind_62);
    else if (kind_is({0x5f})) story_family(families::kind_5f);
    else if (kind_is({0x5c})) story_family(families::kind_5c);
    else if (kind_is({0x3f})) story_penalty(families::penalty_3f);
    else if (kind_is({0x60})) story_penalty(families::penalty_60);
    else if (kind_is({0x43})) story_singles();
    else if (kind_is({0x40, 0x41})) story_penalty(families::penalty_40);
    else if (kind_is({0x42})) story_42();
    else if (kind_is({0x52}) && nonzero_logical(c, other)) story_52();
    else if (kind_is({0x44})) story_scored(0x10677b4c, 0x10677ba4, 0x10677bc8, 0x10677c1c);
    else if (kind_is({0x5a})) story_scored(0x10677c68, 0x10677cc0, 0x10677ce4, 0x10677d38);
}

// The third column moves by pool(0x40) times the strength, within 1..10000 (the rows are 0x4c bytes).
void Press::move_third_column() {
    const auto scaled = fpu.fmul(dbl(0x40), strength_at(0x38));
    c.put64(frame.at(0x40), fpu.fctiwz(0, scaled));
    move_club_rating(c, club, 0xa, std::int32_t(c.u32(frame.at(0x44))));
}

void Press::apply_pulls(std::span<const Pull> pulls) {
    for (const auto& pull : pulls) {
        if (flag(pull.bits)) {
            scale_by(pull.pool);
            break;
        }
    }
}
}

void evaluate_club_story(Cpu& c, std::uint32_t reporter, std::uint32_t club, std::uint32_t kind, std::uint32_t side, std::uint32_t other) {
    eval_c::Press(c, reporter, club, kind, side, other).run();
}
}
